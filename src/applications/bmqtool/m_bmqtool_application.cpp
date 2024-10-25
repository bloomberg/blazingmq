// Copyright 2014-2023 Bloomberg Finance L.P.
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// m_bmqtool_application.cpp                                          -*-C++-*-
#include <m_bmqtool_application.h>

// BMQTOOL
#include <m_bmqtool_inpututil.h>
#include <m_bmqtool_parameters.h>
#include <m_bmqtool_statutil.h>

// BMQ
#include <bmqa_message.h>
#include <bmqa_messageevent.h>
#include <bmqa_messageeventbuilder.h>
#include <bmqa_messageiterator.h>
#include <bmqa_messageproperties.h>
#include <bmqa_queueid.h>
#include <bmqa_sessionevent.h>
#include <bmqimp_event.h>
#include <bmqp_confirmeventbuilder.h>
#include <bmqt_queueflags.h>
#include <bmqt_resultcode.h>
#include <bmqt_sessioneventtype.h>

// BMQ
#include <bmqu_blob.h>
#include <bmqu_memoutstream.h>
#include <bmqu_outstreamformatsaver.h>
#include <bmqu_printutil.h>

// BMQ
#include <bmqst_statutil.h>

// BDE
#include <ball_log.h>
#include <ball_loggermanager.h>
#include <ball_loggermanagerconfiguration.h>
#include <ball_streamobserver.h>
#include <bdlbb_blobutil.h>
#include <bdlf_bind.h>
#include <bdlf_memfn.h>
#include <bdlf_placeholder.h>
#include <bdlt_timeunitratio.h>
#include <bsl_numeric.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslmt_semaphore.h>
#include <bslmt_turnstile.h>
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace m_bmqtool {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("BMQTOOL.APPLICATION");

}  // close unnamed namespace

// -----------------
// class Application
// -----------------

void Application::setUpLog()
{
    // Create the logger manager singleton
    ball::LoggerManager::initSingleton(&d_multiplexObserver,
                                       ball::LoggerManagerConfiguration(),
                                       d_allocator_p);

    ball::Severity::Level logLevel = ball::Severity::INFO;
    switch (d_parameters_p->verbosity()) {
    case ParametersVerbosity::e_SILENT: {
        logLevel = ball::Severity::OFF;
    } break;
    case ParametersVerbosity::e_TRACE: {
        logLevel = ball::Severity::TRACE;
    } break;
    case ParametersVerbosity::e_DEBUG: {
        logLevel = ball::Severity::DEBUG;
    } break;
    case ParametersVerbosity::e_INFO: {
        logLevel = ball::Severity::INFO;
    } break;
    case ParametersVerbosity::e_WARNING: {
        logLevel = ball::Severity::WARN;
    } break;
    case ParametersVerbosity::e_ERROR:
    case ParametersVerbosity::e_FATAL: {
        logLevel = ball::Severity::ERROR;
    } break;
    }

    const bsl::string logFormat =
        d_parameters_p->logFormat().empty()
            ? CommandLineParameters::DEFAULT_INITIALIZER_LOG_FORMAT
            : d_parameters_p->logFormat();

    d_consoleObserver.setSeverityThreshold(ball::Severity::TRACE);
    // We use the ballLoggerManager global threshold to determine what gets
    // logged, so the console observer should print any logs it receives.

    d_consoleObserver.setLogFormat(logFormat)
        .setCategoryColor("DMC*", "gray")
        .setCategoryColor("BMQ*", "green")
        .setCategoryColor("MQB*", "green")
        .setCategoryColor("APPLICATION", "yellow")
        .setCategoryColor("STORAGEINSPECTOR", "yellow");

    // Set default verbosity to the specified one
    ball::LoggerManager::singleton().setDefaultThresholdLevels(
        ball::Severity::OFF,   // recording level
        logLevel,              // passthrough level
        ball::Severity::OFF,   // trigger level
        ball::Severity::OFF);  // triggerAll level

    d_multiplexObserver.registerObserver(&d_consoleObserver);
}

void Application::tearDownLog()
{
    // Unregister the BALL log observer
    d_multiplexObserver.deregisterObserver(&d_consoleObserver);
    ball::LoggerManager::shutDownSingleton();
}

bsl::shared_ptr<bmqst::StatContext>
Application::createStatContext(int historySize, bslma::Allocator* allocator)
{
    // message: value is data bytes; increments is number of messages
    // event:   value is total (data + protocol) bytes; increments is number of
    //          events
    // latency: discrete value with nanoseconds of latency reported for each
    //          message (only used in consumer mode)
    bmqst::StatContextConfiguration config("bmqtool", allocator);
    config.isTable(true);
    config.value("message", historySize)
        .value("event", historySize)
        .value("latency", bmqst::StatValue::e_DISCRETE, historySize);
    return bsl::make_shared<bmqst::StatContext>(config, allocator);
}

void Application::snapshotStats()
{
    d_statContext_sp->snapshot();

    static unsigned int count = 0;
    if (++count % k_STAT_DUMP_INTERVAL == 0 &&
        d_parameters_p->verbosity() != ParametersVerbosity::e_SILENT &&
        d_isConnected) {
        printStats(k_STAT_DUMP_INTERVAL);
    }
}

void Application::autoReadShutdown()
{
    if (!d_autoReadActivity) {
        d_shutdownSemaphore_p->post();
    }

    d_autoReadActivity = false;
}

void Application::printStatHeader() const
{
    static bool headerPrinted = false;  // To only print it once
    if (headerPrinted) {
        return;  // RETURN
    }
    headerPrinted = true;

    bool printLatency = bmqt::QueueFlagsUtil::isReader(
                            d_parameters_p->queueFlags()) &&
                        d_parameters_p->latency() != ParametersLatency::e_NONE;

    bsl::cout << "Mode     |"
              << " Msg/s         |"
              << " Delta         |"
              << " Bytes/s       ||"
              << " Evt/s         |"
              << " Delta         |"
              << " Bytes/s       ";
    if (printLatency) {
        bsl::cout << "||Latency";
    }
    bsl::cout << bsl::endl;
    bsl::cout << "---------+"
              << "---------------+"
              << "---------------+"
              << "---------------++"
              << "---------------+"
              << "---------------+"
              << "---------------";
    if (printLatency) {
        bsl::cout << "++------------------------------------------------";
    }
    bsl::cout << bsl::endl;
}

void Application::printStats(int interval) const
{
    // Header
    printStatHeader();

    // Gather metrics
    const bmqst::StatValue& msg = d_statContext_sp->value(
        bmqst::StatContext::e_DIRECT_VALUE,
        k_STAT_MSG);
    const bmqst::StatValue& evt = d_statContext_sp->value(
        bmqst::StatContext::e_DIRECT_VALUE,
        k_STAT_EVT);

    bmqst::StatValue::SnapshotLocation t0(0, 0);
    bmqst::StatValue::SnapshotLocation t1(0, interval);

    double msgBytesRate = bmqst::StatUtil::ratePerSecond(msg, t0, t1);
    double msgRate      = bmqst::StatUtil::incrementsPerSecond(msg, t0, t1);
    bsls::Types::Int64 msgDelta = bmqst::StatUtil::incrementsDifference(msg,
                                                                        t0,
                                                                        t1);
    double evtBytesRate         = bmqst::StatUtil::ratePerSecond(evt, t0, t1);
    double evtRate = bmqst::StatUtil::incrementsPerSecond(evt, t0, t1);
    bsls::Types::Int64 evtDelta = bmqst::StatUtil::incrementsDifference(evt,
                                                                        t0,
                                                                        t1);

    bmqu::MemOutStream ss;
    if (bmqt::QueueFlagsUtil::isReader(d_parameters_p->queueFlags())) {
        ss << "consumed ";
    }
    else if (bmqt::QueueFlagsUtil::isWriter(d_parameters_p->queueFlags())) {
        ss << "produced ";
    }
    else {
        BSLS_ASSERT_OPT(false);
    }

    // Msg
    {
        bmqu::OutStreamFormatSaver streamFmtSaver(ss);
        ss << "|" << bsl::setw(14)
           << bmqu::PrintUtil::prettyNumber(
                  static_cast<bsls::Types::Int64>(msgRate))
           << " |" << bsl::setw(14) << bmqu::PrintUtil::prettyNumber(msgDelta)
           << " |" << bsl::setw(14)
           << bmqu::PrintUtil::prettyBytes(
                  static_cast<bsls::Types::Int64>(msgBytesRate));
    }

    // Event
    {
        bmqu::OutStreamFormatSaver streamFmtSaver(ss);
        ss << " ||" << bsl::setw(14)
           << bmqu::PrintUtil::prettyNumber(
                  static_cast<bsls::Types::Int64>(evtRate))
           << " |" << bsl::setw(14) << bmqu::PrintUtil::prettyNumber(evtDelta)
           << " |" << bsl::setw(14)
           << bmqu::PrintUtil::prettyBytes(
                  static_cast<bsls::Types::Int64>(evtBytesRate));
    }

    // Latency
    if (bmqt::QueueFlagsUtil::isReader(d_parameters_p->queueFlags()) &&
        d_parameters_p->latency() != ParametersLatency::e_NONE) {
        const bmqst::StatValue& latency = d_statContext_sp->value(
            bmqst::StatContext::e_DIRECT_VALUE,
            k_STAT_LAT);
        bsls::Types::Int64 latencyMin = bmqst::StatUtil::rangeMin(latency,
                                                                  t0,
                                                                  t1);
        bsls::Types::Int64 latencyAvg = static_cast<bsls::Types::Int64>(
            bmqst::StatUtil::averagePerEvent(latency, t0, t1));
        bsls::Types::Int64 latencyMax = bmqst::StatUtil::rangeMax(latency,
                                                                  t0,
                                                                  t1);
        bmqu::OutStreamFormatSaver streamFmtSaver(ss);
        ss << " ||" << bsl::setw(14)
           << bmqu::PrintUtil::prettyTimeInterval(latencyMin) << " < "
           << bsl::setw(14) << bmqu::PrintUtil::prettyTimeInterval(latencyAvg)
           << " < " << bsl::setw(14)
           << bmqu::PrintUtil::prettyTimeInterval(latencyMax);
    }

    bsl::cout << ss.str() << bsl::endl;
}

void Application::printFinalStats()
{
    d_statContext_sp->snapshot();

    bmqst::StatValue::SnapshotLocation loc(0, 0);

    const bmqst::StatValue& msg = d_statContext_sp->value(
        bmqst::StatContext::e_DIRECT_VALUE,
        k_STAT_MSG);
    const bmqst::StatValue& evt = d_statContext_sp->value(
        bmqst::StatContext::e_DIRECT_VALUE,
        k_STAT_EVT);

    bsls::Types::Int64 nbMsg    = bmqst::StatUtil::increments(msg, loc);
    bsls::Types::Int64 msgBytes = bmqst::StatUtil::value(msg, loc);
    bsls::Types::Int64 nbEvt    = bmqst::StatUtil::increments(evt, loc);
    bsls::Types::Int64 evtBytes = bmqst::StatUtil::value(evt, loc);

    bmqu::MemOutStream ss;
    if (bmqt::QueueFlagsUtil::isReader(d_parameters_p->queueFlags())) {
        ss << "consumed ";
    }
    else if (bmqt::QueueFlagsUtil::isWriter(d_parameters_p->queueFlags())) {
        ss << "produced ";
    }
    else {
        BSLS_ASSERT_OPT(false && "Neither writer nor reader flags are set");
    }

    ss << bmqu::PrintUtil::prettyNumber(nbMsg) << " messages ["
       << bmqu::PrintUtil::prettyBytes(msgBytes) << "] in "
       << bmqu::PrintUtil::prettyNumber(nbEvt) << " events ["
       << bmqu::PrintUtil::prettyBytes(evtBytes) << "]";

    if (msgBytes != 0) {
        double protocol = (evtBytes - msgBytes) * 100.0 / evtBytes;
        ss << " (Protocol: " << bsl::setprecision(4) << protocol << "%)";
    }

    // Latency
    if (bmqt::QueueFlagsUtil::isReader(d_parameters_p->queueFlags()) &&
        d_parameters_p->latency() != ParametersLatency::e_NONE) {
        const bmqst::StatValue& latency = d_statContext_sp->value(
            bmqst::StatContext::e_DIRECT_VALUE,
            k_STAT_LAT);

        bsls::Types::Int64 latencyMin = bmqst::StatUtil::absoluteMin(latency);
        bsls::Types::Int64 latencyMax = bmqst::StatUtil::absoluteMax(latency);
        ss << " ~ latency { "
           << bmqu::PrintUtil::prettyTimeInterval(latencyMin) << " < "
           << bmqu::PrintUtil::prettyTimeInterval(latencyMax) << " }";
    }

    bsl::cout << "\n"
              << "Final stats: " << ss.str() << bsl::endl;
}

void Application::generateLatencyReport()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_latencies.empty());
    BSLS_ASSERT_SAFE(!d_parameters_p->latencyReportPath().empty());

    if (!(bmqt::QueueFlagsUtil::isReader(d_parameters_p->queueFlags()) &&
          d_parameters_p->latency() != ParametersLatency::e_NONE)) {
        // Not a consumer, or not asked to gather latency, nothing to do
        return;  // RETURN
    }

    bsl::cout << "====================\n"
              << "Latency Report (generated at: "
              << d_parameters_p->latencyReportPath() << ")\n"
              << "====================\n";

    // 1. Remove the first 30s worth of data, to avoid initial warmup to
    //    interfere and skew the results.  Since we know the frequency at which
    //    messages are stamped with a timestamp (k_LATENCY_INTERVAL_MS), we can
    //    easily do the reverse computation to compute how many metrics are in
    //    30s.  Note that this assumes that the producer and consumer are using
    //    the same build (the constant is used by the producer only, the
    //    consumer just checks each message if it has a timestamp).  Also, note
    //    that the 'k_LATENCY_INTERVAL_MS' represents the maximum sampling
    //    rate, if the publisher is publishing very little (i.e. at less
    //    frequently than one message per k_LATENCY_INTERVAL_MS, then all
    //    messages will be stamped, but also we won't be having lots of
    //    messages; therefore only delete if we have a large enough sample set.
    const unsigned int k_toRemoveCount = 1000 / k_LATENCY_INTERVAL_MS * 30;
    // Number of stamped messages in a 30s time interval

    if (d_latencies.size() >= k_toRemoveCount * 2) {
        bsl::list<bsls::Types::Int64>::iterator itEnd = d_latencies.begin();
        bsl::advance(itEnd, k_toRemoveCount);
        d_latencies.erase(d_latencies.begin(), itEnd);
    }
    else {
        bsl::cout << " **/!\\: Too few data points (" << d_latencies.size()
                  << "), the resulting statistics may not be representative."
                  << bsl::endl;
    }

    // 2. Convert the list to a sorted vector: we use a list while collecting
    //    data to avoid overhead of resizing the vector; but now convert to a
    //    sorted vector to make it easier to compute some statistic metrics.
    bsl::vector<bsls::Types::Int64> dataSet(d_latencies.begin(),
                                            d_latencies.end());
    bsl::sort(dataSet.begin(), dataSet.end());

    // 3. Compute some interesting metrics
    const bsls::Types::Int64 min = *bsl::min_element(dataSet.begin(),
                                                     dataSet.end());
    const bsls::Types::Int64 max = *bsl::max_element(dataSet.begin(),
                                                     dataSet.end());

    const bsls::Types::Int64 sum = bsl::accumulate(dataSet.begin(),
                                                   dataSet.end(),
                                                   0LL);
    const double             avg = static_cast<double>(sum) / dataSet.size();

    const bsls::Types::Int64 median = StatUtil::computePercentile(dataSet,
                                                                  50.0);

    const bsls::Types::Int64 p99 = StatUtil::computePercentile(dataSet, 99.0);
    const bsls::Types::Int64 p98 = StatUtil::computePercentile(dataSet, 98.0);
    const bsls::Types::Int64 p97 = StatUtil::computePercentile(dataSet, 97.0);
    const bsls::Types::Int64 p96 = StatUtil::computePercentile(dataSet, 96.0);
    const bsls::Types::Int64 p95 = StatUtil::computePercentile(dataSet, 95.0);

    // 4. Print summary stats to stdout
    bsl::cout
        << "  Population size.: " << dataSet.size() << "\n"
        << "  min.............: " << bmqu::PrintUtil::prettyTimeInterval(min)
        << "\n"
        << "  avg.............: " << bmqu::PrintUtil::prettyTimeInterval(avg)
        << "\n"
        << "  max.............: " << bmqu::PrintUtil::prettyTimeInterval(max)
        << "\n"
        << "  median..........: "
        << bmqu::PrintUtil::prettyTimeInterval(median) << "\n"
        << "  95Percentile....: " << bmqu::PrintUtil::prettyTimeInterval(p95)
        << "\n"
        << "  96Percentile....: " << bmqu::PrintUtil::prettyTimeInterval(p96)
        << "\n"
        << "  97Percentile....: " << bmqu::PrintUtil::prettyTimeInterval(p97)
        << "\n"
        << "  98Percentile....: " << bmqu::PrintUtil::prettyTimeInterval(p98)
        << "\n"
        << "  99Percentile....: " << bmqu::PrintUtil::prettyTimeInterval(p99)
        << "\n"
        << bsl::endl;

    // 5. Generate the JSON report
    bsl::ofstream output(d_parameters_p->latencyReportPath().c_str());
    if (!output) {
        bsl::cout << "Unable to generate latency report, failed to open '"
                  << d_parameters_p->latencyReportPath().c_str() << "'"
                  << bsl::endl;
        return;  // RETURN
    }
    output << "{\n"
           << "  \"min\": " << min << ",\n"
           << "  \"avg\": " << avg << ",\n"
           << "  \"max\": " << max << ",\n"
           << "  \"median\": " << median << ",\n"
           << "  \"99percentile\": " << p99 << ",\n"
           << "  \"98percentile\": " << p98 << ",\n"
           << "  \"97percentile\": " << p97 << ",\n"
           << "  \"96percentile\": " << p96 << ",\n"
           << "  \"95percentile\": " << p95 << ",\n"
           << "  \"dataPoints\": [";
    // Print the unsorted (i.e. in reporting order) data, so that if needed, we
    // could see the evolution over time and maybe some pattern (for example
    // initial latency being higher due to cache warmup, ...).
    int                                           idx = 0;
    bsl::list<bsls::Types::Int64>::const_iterator it  = d_latencies.begin();
    while (it != d_latencies.end()) {
        if (idx++ % 10 == 0) {
            output << "\n    ";
        }
        output << *it;
        if (++it != d_latencies.end()) {
            output << ", ";
        }
    }
    output << "\n  ]\n"
           << "}\n";

    output.close();
}

int Application::initialize()
{
    enum RC {
        e_OK                          = 0,
        e_INIT_INTERACTIVE_ERROR      = -1,
        e_INIT_STORAGE_ERROR          = -2,
        e_OPEN_QUEUE_ERROR            = -3,
        e_VALIDATE_SUBSCRIPTION_ERROR = -4,
        e_START_SESSION_ERROR         = -10
    };

    // Dump parameters used, unless in silent mode.
    if (d_parameters_p->verbosity() != ParametersVerbosity::e_SILENT) {
        d_parameters_p->dump(bsl::cout);
        bsl::cout << bsl::endl;
    }

    // First, setup the session options
    int                  rc = 0;
    bmqt::SessionOptions options;
    options.setBrokerUri(d_parameters_p->broker())
        .setNumProcessingThreads(d_parameters_p->numProcessingThreads())
        .configureEventQueue(1000, 10 * 1000);

    // Create the session
    if (d_parameters_p->noSessionEventHandler()) {
        d_session_mp.load(new (*d_allocator_p)
                              bmqa::Session(options, d_allocator_p),
                          d_allocator_p);
    }
    else {
        bslma::ManagedPtr<bmqa::SessionEventHandler> managedHandler(
            this,
            d_allocator_p,
            bslma::ManagedPtrNilDeleter<bmqa::SessionEventHandler>::deleter);
        d_session_mp.load(
            new (*d_allocator_p)
                bmqa::Session(managedHandler, options, d_allocator_p),
            d_allocator_p);
    }

    if (d_parameters_p->mode() == ParametersMode::e_CLI) {
        // Initialize the interactive mode
        rc = d_interactive.initialize(d_session_mp.ptr(), this);
        if (rc != 0) {
            BALL_LOG_ERROR << "Failed init interactive [" << rc << "]";
            return e_INIT_INTERACTIVE_ERROR;  // RETURN
        }
    }
    else if (d_parameters_p->mode() == ParametersMode::e_STORAGE) {
        rc = d_storageInspector.initialize();
        if (rc != 0) {
            BALL_LOG_ERROR << "Failed to initialize storage inspector  [" << rc
                           << "]";
            return e_INIT_STORAGE_ERROR;  // RETURN
        }
    }
    else if (d_parameters_p->mode() == ParametersMode::e_AUTO) {
        rc = d_session_mp->start();
        if (rc != 0) {
            BALL_LOG_ERROR << "Unable to start session [rc: " << rc << " - "
                           << bmqt::GenericResult::Enum(rc) << "]";
            return e_START_SESSION_ERROR + rc;  // RETURN
        }

        BALL_LOG_INFO << "Session started.";

        bmqt::QueueOptions queueOptions;
        queueOptions
            .setMaxUnconfirmedMessages(d_parameters_p->maxUnconfirmedMsgs())
            .setMaxUnconfirmedBytes(d_parameters_p->maxUnconfirmedBytes());

        if (!InputUtil::populateSubscriptions(
                &queueOptions,
                d_parameters_p->subscriptions())) {
            BALL_LOG_ERROR << "Invalid subscriptions";
            return e_VALIDATE_SUBSCRIPTION_ERROR;  // RETURN
        }

        bmqa::OpenQueueStatus result = d_session_mp->openQueueSync(
            &d_queueId,
            d_parameters_p->queueUri(),
            d_parameters_p->queueFlags(),
            queueOptions);
        if (!result) {
            BALL_LOG_ERROR << "Error while opening queue: [result: " << result
                           << "]";
            return e_OPEN_QUEUE_ERROR;  // RETURN
        }

        // Schedule a clock to collect / dump stats
        bdlmt::EventScheduler::RecurringEventHandle handle;
        d_scheduler.scheduleRecurringEvent(
            &handle,
            bsls::TimeInterval(1.0),
            bdlf::BindUtil::bind(&Application::snapshotStats, this));
    }

    return e_OK;
}

void Application::onSessionEvent(const bmqa::SessionEvent& event)
{
    if (d_parameters_p->verbosity() != ParametersVerbosity::e_SILENT) {
        BALL_LOG_INFO << "==> EVENT received: " << event;
    }

    // Keep track of connected/disconnected state
    if (event.type() == bmqt::SessionEventType::e_CONNECTED ||
        event.type() == bmqt::SessionEventType::e_RECONNECTED) {
        d_isConnected = true;
    }
    else if (event.type() == bmqt::SessionEventType::e_DISCONNECTED ||
             event.type() == bmqt::SessionEventType::e_CONNECTION_LOST) {
        d_isConnected = false;
    }

    // Write to log file
    d_fileLogger.writeSessionEvent(event);
}

void Application::onMessageEvent(const bmqa::MessageEvent& event)
{
    if (d_parameters_p->verbosity() != ParametersVerbosity::e_SILENT &&
        d_parameters_p->mode() == ParametersMode::e_CLI) {
        BALL_LOG_INFO << "==> EVENT received: " << event;
    }

    BSLS_ASSERT_SAFE(d_session_mp);

    // Update stats if push ...
    if (event.type() == bmqt::MessageEventType::e_PUSH) {
        // Update event size (i.e. including protocol overhead)
        const bsl::shared_ptr<bmqimp::Event>& eventImpl =
            reinterpret_cast<const bsl::shared_ptr<bmqimp::Event>&>(event);
        d_statContext_sp->adjustValue(k_STAT_EVT,
                                      eventImpl->rawEvent().blob()->length());
    }

    if (d_parameters_p->mode() == ParametersMode::e_AUTO) {
        d_autoReadActivity = true;

        if (!d_autoReadInProgress && d_parameters_p->shutdownGrace() != 0) {
            // This is the first message in this session.  Schedule a recurring
            // event to check if a message has been received during the grace
            // period.
            d_autoReadInProgress = true;
            // Schedule a clock to check for activity
            bdlmt::EventScheduler::RecurringEventHandle handle;
            d_scheduler.scheduleRecurringEvent(
                &handle,
                bsls::TimeInterval(d_parameters_p->shutdownGrace()),
                bdlf::BindUtil::bind(&Application::autoReadShutdown, this));
        }
    }

    int msgId = 0;
    // Load a ConfirmEventBuilder from the session
    bmqa::ConfirmEventBuilder confirmBuilder;
    d_session_mp->loadConfirmEventBuilder(&confirmBuilder);

    for (bmqa::MessageIterator iter = event.messageIterator();
         iter.nextMessage();
         ++msgId) {
        const bmqa::Message& message = iter.message();
        if (event.type() == bmqt::MessageEventType::e_ACK) {
            if (d_parameters_p->dumpMsg()) {
                BALL_LOG_INFO << "ACK #" << msgId << ": " << message;
            }

            // Write to log file
            d_fileLogger.writeAckMessage(message);

            if (d_numExpectedAcks != 0 &&
                d_numExpectedAcks == ++d_numAcknowledged) {
                BALL_LOG_INFO << "All posted messages have been acknowledged";
                d_shutdownSemaphore_p->post();
            }
        }
        else {
            // Message is a push message
            bdlbb::Blob blob;
            message.getData(&blob);

            // Write to log file
            d_fileLogger.writePushMessage(message);

            if (d_parameters_p->confirmMsg()) {
                // disambiguate ConfirmEventBuilder::addMessageConfirmation
                bdlf::MemFn<bmqt::EventBuilderResult::Enum (
                    bmqa::ConfirmEventBuilder::*)(
                    const bmqa::Message& message)>
                    f(&bmqa::ConfirmEventBuilder::addMessageConfirmation);

                bmqt::EventBuilderResult::Enum rc =
                    bmqp::ProtocolUtil::buildEvent(
                        bdlf::BindUtil::bind(f, &confirmBuilder, message),
                        bdlf::BindUtil::bind(&bmqa::Session::confirmMessages,
                                             d_session_mp.get(),
                                             &confirmBuilder));

                BSLS_ASSERT_SAFE(rc == 0);
                (void)rc;  // compiler happiness

                // Write to log file
                d_fileLogger.writeConfirmMessage(message);
                // Note that we add to the fileLogger here, despite sending
                // the batched confirm event out of this loop because we
                // need to access each individual's message details.
            }

            // Update latency stats if required
            if (d_parameters_p->latency() != ParametersLatency::e_NONE) {
                bdlb::BigEndianInt64 time;

                int rc = bmqu::BlobUtil::readNBytes(
                    reinterpret_cast<char*>(&time),
                    blob,
                    bmqu::BlobPosition(0, 0),
                    sizeof(bdlb::BigEndianInt64));
                BSLS_ASSERT_SAFE(rc == 0);
                (void)rc;

                if (time != 0) {
                    bsls::Types::Int64 now = StatUtil::getNowAsNs(
                        d_parameters_p->latency());
                    bsls::Types::Int64 delta = now - time;
                    if (delta >= 0) {
                        // Apparently, for some reasons, the delta sometimes
                        // comes up negative, don't report it so that the stats
                        // remain decently representative of actual measures.
                        d_statContext_sp->reportValue(k_STAT_LAT, delta);

                        // Keep each individual latency when requested to
                        // generate a latency report.  Note that since only one
                        // message every k_LATENCY_INTERVAL_MS time interval
                        // has latency, this list will not grow out of control
                        // when run during a 'decent' amount of time.  With a
                        // default of 5ms, this implies 200 items per second,
                        // or 120,000 for 10 minutes.
                        if (!d_parameters_p->latencyReportPath().empty()) {
                            d_latencies.push_back(delta);
                        }
                    }
                }
            }

            // Update msg/event stats
            d_statContext_sp->adjustValue(k_STAT_MSG, blob.length());

            // Call 'onMessage' before logging PUSH message. 'onMessage' blocks
            // until open queue response is logged.  This is done for
            // integration tests that rely on order of those logs.

            if (d_parameters_p->mode() == ParametersMode::e_CLI &&
                !d_parameters_p->confirmMsg()) {
                // Save in case of interactive mode.
                d_interactive.onMessage(message);
            }

            // DUMP
            if (d_parameters_p->dumpMsg()) {
                BALL_LOG_INFO_BLOCK
                {
                    BALL_LOG_OUTPUT_STREAM << "PUSH #" << msgId << ": "
                                           << message;
                    if (message.hasProperties()) {
                        bmqa::MessageProperties properties;
                        message.loadProperties(&properties);
                        BALL_LOG_OUTPUT_STREAM << " with properties: "
                                               << properties;
                    }
                }
            }

            bmqa::MessageProperties in(d_allocator_p);
            BSLA_MAYBE_UNUSED int   rc = message.loadProperties(&in);
            BSLS_ASSERT_SAFE(rc == 0);
            InputUtil::verifyProperties(in,
                                        d_parameters_p->messageProperties());
        }
    }

    // Confirm messages in batches if asked for it.  Note that
    // 'bmqa::Session:confirmMessages' method will reset the builder.
    if (bmqt::MessageEventType::e_PUSH == event.type() &&
        d_parameters_p->confirmMsg()) {
        int rc = d_session_mp->confirmMessages(&confirmBuilder);
        if (rc != 0) {
            BALL_LOG_ERROR << "Failed to send " << msgId << " confirms for "
                           << event << " [rc: " << rc << "]";
            BSLS_ASSERT_SAFE(!d_isRunning);
        }
    }
}

void Application::producerThread()
{
    BSLS_ASSERT_SAFE(d_session_mp);

    bslmt::Turnstile turnstile(1000.0);
    if (d_parameters_p->postInterval() != 0) {
        turnstile.reset(1000.0 / d_parameters_p->postInterval());
    }

    bsl::shared_ptr<PostingContext> postingContext =
        d_poster.createPostingContext(d_session_mp.get(),
                                      d_parameters_p,
                                      d_queueId);

    while (d_isRunning && postingContext->pendingPost()) {
        if (d_isConnected) {
            postingContext->postNext();
        }
        if (d_parameters_p->postInterval() != 0) {
            turnstile.waitTurn();
        }
    }

    if (!bmqt::QueueFlagsUtil::isAck(d_parameters_p->queueFlags())) {
        d_shutdownSemaphore_p->post();
    }
}

// CLASS METHODS
int Application::syschk(const m_bmqtool::Parameters& parameters)
{
    // Setup logging
    ball::StreamObserver             observer(&bsl::cout);
    ball::LoggerManagerConfiguration configuration;
    configuration.setDefaultThresholdLevelsIfValid(ball::Severity::WARN);

    ball::LoggerManagerScopedGuard guard(&observer, configuration);

    // Initialize session options
    bmqt::SessionOptions options;
    options.setBrokerUri(parameters.broker())
        .setConnectTimeout(
            bsls::TimeInterval(3 * bdlt::TimeUnitRatio::k_SECONDS_PER_MINUTE));
    // NOTE: We use a 3 minutes timeout because sometimes the sysqc script may
    //       execute right after the broker was started, and the broker may not
    //       be able to accept/process the bmqtool connection request in due
    //       time.

    // Create the session
    bmqa::Session session(options);

    // Start the session
    const int rc = session.start();
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to start a session: [rc: " << rc << "]";
        return rc;  // RETURN
    }

    if (!parameters.queueUri().empty()) {
        bmqa::QueueId         queueId(bmqt::CorrelationId::autoValue());
        bmqa::OpenQueueStatus result = session.openQueueSync(
            &queueId,
            parameters.queueUri(),
            bmqt::QueueFlags::e_WRITE,
            bmqt::QueueOptions(),
            bsls::TimeInterval(300));

        if (!result) {
            BALL_LOG_ERROR << "Error while opening queue: [result: " << result
                           << "]";
            return result.result();  // RETURN
        }

        session.closeQueueSync(&queueId, bsls::TimeInterval(300));
    }

    // Stop the connection
    session.stop();

    return 0;
}

// CREATORS
Application::Application(Parameters*       parameters,
                         bslmt::Semaphore* shutdownSemaphore,
                         bslma::Allocator* allocator)
: d_allocator_p(bslma::Default::allocator(allocator))
, d_parameters_p(parameters)
, d_shutdownSemaphore_p(shutdownSemaphore)
, d_runningThread(bslmt::ThreadUtil::invalidHandle())
, d_queueId(d_allocator_p)
, d_statContext_sp(createStatContext(10, d_allocator_p))
, d_isConnected(false)
, d_isRunning(false)
, d_consoleObserver(d_allocator_p)
, d_storageInspector(d_allocator_p)
, d_fileLogger(d_parameters_p->logFilePath(), d_allocator_p)
, d_poster(&d_fileLogger, d_statContext_sp.get(), d_allocator_p)
, d_interactive(parameters, &d_poster, d_allocator_p)
, d_latencies(allocator)
, d_autoReadInProgress(false)
, d_autoReadActivity(false)
, d_numExpectedAcks(0)
, d_numAcknowledged(0)
{
    // NOTHING
}

Application::~Application()
{
    // NOTHING
}

// MANIPULATORS

int Application::start()
{
    setUpLog();

    d_scheduler.start();

    if (!d_parameters_p->logFilePath().empty()) {
        bool rc = d_fileLogger.open();
        BSLS_ASSERT_SAFE(rc);
        (void)rc;  // Compiler happiness
    }

    int rc = initialize();
    if (rc != 0) {
        if (d_session_mp) {
            d_session_mp->stop();
        }
        d_session_mp.reset();
        d_scheduler.stop();
        tearDownLog();

        return rc;  // RETURN
    }

    return run();
}

int Application::run()
{
    d_isRunning = true;

    int rc = 0;

    if (d_parameters_p->mode() == ParametersMode::e_CLI) {
        // Process commands (returns when user quits)
        rc = d_interactive.mainLoop();
        d_shutdownSemaphore_p->post();
    }
    else if (d_parameters_p->mode() == ParametersMode::e_STORAGE) {
        // Storage inspector (returns when user quits)
        rc = d_storageInspector.mainLoop();
        d_shutdownSemaphore_p->post();
    }
    else {
        if (bmqt::QueueFlagsUtil::isWriter(d_parameters_p->queueFlags())) {
            d_numExpectedAcks = d_parameters_p->eventsCount() *
                                d_parameters_p->eventSize();
            d_numAcknowledged = 0;

            // Start the thread
            rc = bslmt::ThreadUtil::create(
                &d_runningThread,
                bdlf::MemFnUtil::memFn(&Application::producerThread, this));
        }
    }

    return rc;
}

void Application::stop()
{
    if (!d_isRunning) {
        return;  // RETURN
    }

    d_isRunning = false;

    d_scheduler.cancelAllEventsAndWait();
    d_scheduler.stop();

    if (d_runningThread != bslmt::ThreadUtil::invalidHandle()) {
        bslmt::ThreadUtil::join(d_runningThread);
        d_runningThread = bslmt::ThreadUtil::invalidHandle();
    }

    // Disconnect from the broker
    if (d_parameters_p->mode() == ParametersMode::e_AUTO) {
        if (d_parameters_p->shutdownGrace() != 0) {
            bslmt::ThreadUtil::sleep(
                bsls::TimeInterval(d_parameters_p->shutdownGrace()));
        }
        if (d_queueId.isValid()) {
            d_session_mp->closeQueueSync(&d_queueId);
        }
        d_session_mp->stop();
    }

    // Delete the session
    d_session_mp.reset();

    if (d_fileLogger.isOpen()) {
        d_fileLogger.close();
    }

    // Display final stats
    if (d_parameters_p->mode() != ParametersMode::e_CLI &&
        d_parameters_p->mode() != ParametersMode::e_STORAGE &&
        d_parameters_p->verbosity() != ParametersVerbosity::e_SILENT) {
        printFinalStats();
    }

    if (!d_latencies.empty()) {
        generateLatencyReport();
    }

    BALL_LOG_INFO << "Goodbye.";

    tearDownLog();
}

}  // close package namespace
}  // close enterprise namespace
