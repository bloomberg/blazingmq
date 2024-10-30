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

// bmqimp_application.cpp                                             -*-C++-*-
#include <bmqimp_application.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqex_executionpolicy.h>
#include <bmqex_systemexecutor.h>
#include <bmqio_channelutil.h>
#include <bmqio_connectoptions.h>
#include <bmqio_status.h>
#include <bmqio_tcpendpoint.h>
#include <bmqma_countingallocatorutil.h>
#include <bmqp_crc32c.h>
#include <bmqp_protocolutil.h>
#include <bmqscm_version.h>
#include <bmqst_statvalue.h>
#include <bmqst_tableutil.h>
#include <bmqsys_threadutil.h>
#include <bmqsys_time.h>
#include <bmqt_resultcode.h>
#include <bmqt_uri.h>
#include <bmqu_blob.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

// BDE
#include <bdlb_scopeexit.h>
#include <bdlf_bind.h>
#include <bdlf_memfn.h>
#include <bdlf_placeholder.h>
#include <bdlma_localsequentialallocator.h>
#include <bdlt_currenttime.h>
#include <bsl_algorithm.h>
#include <bsl_cstdlib.h>
#include <bsl_ctime.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutexassert.h>
#include <bslmt_once.h>
#include <bslmt_threadattributes.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>
#include <bsls_platform.h>
#include <bsls_systemclocktype.h>
#include <bsls_systemtime.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace bmqimp {

namespace {

// CONSTANTS
const double             k_RECONNECT_INTERVAL_MS = 500;
const int                k_RECONNECT_COUNT = bsl::numeric_limits<int>::max();
const bsls::Types::Int64 k_CHANNEL_LOW_WATERMARK = 512 * 1024;

/// Create the StatContextConfiguration to use, from the specified
/// `options`, and using the specified `allocator` for memory allocations.
bmqst::StatContextConfiguration
statContextConfiguration(const bmqt::SessionOptions& options,
                         bslma::Allocator*           allocator)
{
    bmqst::StatContextConfiguration config("stats", allocator);
    if (options.statsDumpInterval() != bsls::TimeInterval()) {
        // Stats configuration:
        //   we snapshot every second
        //   first level keeps 30s of history
        //   second level keeps enough for the dump interval
        // Because some stats require range computation, second level actually
        // has to be of size 1 more than the dump interval
        config.defaultHistorySize(
            30,
            (options.statsDumpInterval().seconds() / 30) + 1);
    }
    else {
        config.defaultHistorySize(2);
    }

    return config;
}

/// Create the ntca::InterfaceConfig to use given the specified
/// `sessionOptions`.  Use the specified `allocator` for any memory
/// allocation.
ntca::InterfaceConfig
ntcCreateInterfaceConfig(const bmqt::SessionOptions& sessionOptions,
                         bslma::Allocator*           allocator)
{
    ntca::InterfaceConfig config(allocator);

    config.setThreadName("bmqimp");

    config.setMaxThreads(1);  // there is only one channel used on this
                              // ChannelPool, with the bmqbrkr
    config.setMaxConnections(128);
    config.setWriteQueueLowWatermark(k_CHANNEL_LOW_WATERMARK);
    config.setWriteQueueHighWatermark(sessionOptions.channelHighWatermark());

    config.setDriverMetrics(false);
    config.setDriverMetricsPerWaiter(false);
    config.setSocketMetrics(false);
    config.setSocketMetricsPerHandle(false);

    config.setAcceptGreedily(false);
    config.setSendGreedily(false);
    config.setReceiveGreedily(false);

    config.setNoDelay(true);
    config.setKeepAlive(true);
    config.setKeepHalfOpen(false);

    return config;
}

}  // close unnamed namespace

// -------------------------
// class bmqimp::Application
// -------------------------

void Application::onChannelDown(const bsl::string&   peerUri,
                                const bmqio::Status& status)
{
    // executed by the *IO* thread

    BALL_LOG_INFO << "Session with '" << peerUri << "' is now DOWN"
                  << " [status: " << status << "]";

    d_brokerSession.setChannel(0);
}

void Application::onChannelWatermark(const bsl::string&                peerUri,
                                     bmqio::ChannelWatermarkType::Enum type)
{
    // executed by the *IO* thread

    BALL_LOG_INFO << type << " write watermark for '" << peerUri << "'.";
    d_brokerSession.handleChannelWatermark(type);
}

void Application::readCb(const bmqio::Status&                   status,
                         int*                                   numNeeded,
                         bdlbb::Blob*                           blob,
                         const bsl::shared_ptr<bmqio::Channel>& channel)
{
    // executed by the *IO* thread

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!status)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        if (status.category() != bmqio::StatusCategory::e_CANCELED) {
            BALL_LOG_ERROR << "#TCP_READ_ERROR " << channel->peerUri()
                           << ": ReadCallback error [status: " << status
                           << "]";
            // Nothing much we can do, close the channel
            channel->close();
        }

        // There is nothing to do in the event of a 'e_CANCELED' event, so
        // simply return.
        return;  // RETURN
    }

    bdlbb::Blob readBlob;

    const int rc = bmqio::ChannelUtil::handleRead(&readBlob, numNeeded, blob);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << "#TCP_READ_ERROR " << channel->peerUri()
                       << ": ReadCallback unrecoverable error "
                       << "[status: " << status << "]:\n"
                       << bmqu::BlobStartHexDumper(blob);
        // Nothing much we can do, close the channel
        channel->close();
        return;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(readBlob.length() == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Don't yet have a full blob
        return;  // RETURN
    }

    BALL_LOG_TRACE << channel->peerUri() << ": ReadCallback got a blob\n"
                   << bmqu::BlobStartHexDumper(&readBlob);

    d_brokerSession.processPacket(readBlob);
}

void Application::channelStateCallback(
    const bsl::string&                     endpoint,
    bmqio::ChannelFactoryEvent::Enum       event,
    const bmqio::Status&                   status,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    // executed by the *IO* thread

    BALL_LOG_TRACE << "Application: channelStateCallback "
                   << "[event: " << event << ", status: " << status
                   << ", channel: '" << (channel ? channel->peerUri() : "none")
                   << "']";

    switch (event) {
    case bmqio::ChannelFactoryEvent::e_CHANNEL_UP: {
        BALL_LOG_INFO << "Session with '" << channel->peerUri()
                      << "' is now UP";

        channel->onClose(
            bdlf::BindUtil::bind(&Application::onChannelDown,
                                 this,
                                 channel->peerUri(),
                                 bdlf::PlaceHolders::_1));  // status
        channel->onWatermark(
            bdlf::BindUtil::bind(&Application::onChannelWatermark,
                                 this,
                                 channel->peerUri(),
                                 bdlf::PlaceHolders::_1));  // type

        d_brokerSession.setChannel(channel);

        // Initiate read flow
        bmqio::Status st;
        channel->read(
            &st,
            bmqp::Protocol::k_PACKET_MIN_SIZE,
            bdlf::BindUtil::bind(&Application::readCb,
                                 this,
                                 bdlf::PlaceHolders::_1,  // status
                                 bdlf::PlaceHolders::_2,  // numNeeded
                                 bdlf::PlaceHolders::_3,  // blob
                                 channel));
        if (!st) {
            BALL_LOG_ERROR << "Could not read from channel:"
                           << " [peer: " << channel->peerUri()
                           << ", status: " << status << "]";
            channel->close();
            return;  // RETURN
        }

        // Cancel the timeout event (if the handle is invalid, this will just
        // do nothing)
        d_scheduler.cancelEvent(&d_startTimeoutHandle);
    } break;  // BREAK
    case bmqio::ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED: {
        BALL_LOG_DEBUG << "Failed an attempt to establish a session with '"
                       << endpoint << "' [event: " << event
                       << ", status: " << status << "]";
    } break;  // BREAK
    case bmqio::ChannelFactoryEvent::e_CONNECT_FAILED: {
        BALL_LOG_ERROR << "Could not establish session with '" << endpoint
                       << "' [event: " << event << ", status: " << status
                       << "]";
    } break;  // BREAK
    default: {
        BALL_LOG_ERROR << "Session with '" << endpoint << "' is now"
                       << " in an unknown state [event: " << event
                       << ", status: " << status << "]";
    }
    }
}

bslma::ManagedPtr<bmqst::StatContext> Application::channelStatContextCreator(
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<bmqio::Channel>& channel,
    const bsl::shared_ptr<bmqio::StatChannelFactoryHandle>&       handle)
{
    // The SDK only connects
    BSLS_ASSERT_SAFE(handle->options().is<bmqio::ConnectOptions>());

    bmqst::StatContextConfiguration config(
        handle->options().the<bmqio::ConnectOptions>().endpoint());
    return d_channelsStatContext_mp->addSubcontext(config);
}

void Application::brokerSessionStopped(
    bmqimp::BrokerSession::FsmEvent::Enum event)
{
    // executed by the FSM thread

    if (event != bmqimp::BrokerSession::FsmEvent::e_START_FAILURE) {
        // This code assumes that there is no need to stop both factories upon
        // e_START_FAILURE.
        // If we wanted that, we would need another event.
        d_reconnectingChannelFactory.stop();
        d_channelFactory.stop();
    }

    d_scheduler.cancelAllEventsAndWait();

    // Print final stats
    d_nextStatDump = -1;  // To prevent snapshotStats from printing
    snapshotStats();
    printStats(true);

    // Cleanup stats to be ready for a potential restart of the session.
    d_rootStatContext.cleanup();

    BALL_LOG_INFO << "bmqimp::Application stop completed";
}

bmqt::GenericResult::Enum Application::startChannel()
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_brokerSession.state() ==
                     bmqimp::BrokerSession::State::e_STARTING);

    int rc = 0;

    // Start the channel factories.
    rc = d_channelFactory.start();
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to start channelFactory [rc: " << rc << "]";
        return bmqt::GenericResult::e_UNKNOWN;  // RETURN
    }
    bdlb::ScopeExitAny tcpScopeGuard(
        bdlf::BindUtil::bind(&bmqio::NtcChannelFactory::stop,
                             &d_channelFactory));

    rc = d_reconnectingChannelFactory.start();
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to start reconnectingChannelFactory [rc: "
                       << rc << "]";
        return bmqt::GenericResult::e_UNKNOWN;  // RETURN
    }
    bdlb::ScopeExitAny reconnectingScopeGuard(
        bdlf::BindUtil::bind(&bmqio::ReconnectingChannelFactory::stop,
                             &d_reconnectingChannelFactory));

    // Connect to the broker.
    bmqio::TCPEndpoint endpoint(d_sessionOptions.brokerUri());
    if (!endpoint) {
        BALL_LOG_ERROR << "Invalid brokerURI '" << d_sessionOptions.brokerUri()
                       << "'";
        return bmqt::GenericResult::e_INVALID_ARGUMENT;  // RETURN
    }

    bdlma::LocalSequentialAllocator<32> localAllocator(&d_allocator);
    bmqu::MemOutStream                  out(&localAllocator);
    out << endpoint.host() << ":" << endpoint.port();

    bsls::TimeInterval attemptInterval;
    attemptInterval.setTotalMilliseconds(k_RECONNECT_INTERVAL_MS);

    bmqio::Status         status;
    bmqio::ConnectOptions options;
    options.setEndpoint(out.str())
        .setNumAttempts(k_RECONNECT_COUNT)
        .setAttemptInterval(attemptInterval)
        .setAutoReconnect(true);

    d_negotiatedChannelFactory.connect(
        &status,
        &d_connectHandle_mp,
        options,
        bdlf::BindUtil::bind(&Application::channelStateCallback,
                             this,
                             options.endpoint(),
                             bdlf::PlaceHolders::_1,    // event
                             bdlf::PlaceHolders::_2,    // status
                             bdlf::PlaceHolders::_3));  // channel
    if (!status) {
        BALL_LOG_ERROR << "Failed to connect to broker at '"
                       << d_sessionOptions.brokerUri()
                       << "' [status: " << status << "]";

        d_brokerSession.stop();
        return bmqt::GenericResult::e_UNKNOWN;  // RETURN
    }

    // NOTE: 'channelStateCb' callback may be invoked as soon as 'connect' is
    // called, before it even returns here; therefore we *must* be *extremely*
    // cautious about what code gets put here after that call to 'connect': it
    // could be executed *AFTER* the '(channelUp)'.

    // Schedule a stats snapshot every seconds, if configured for
    if (d_sessionOptions.statsDumpInterval() != bsls::TimeInterval()) {
        d_nextStatDump = d_sessionOptions.statsDumpInterval().seconds();
        d_scheduler.scheduleRecurringEvent(
            &d_statSnaphotTimerHandle,
            bsls::TimeInterval(1.0),
            bdlf::MemFnUtil::memFn(&Application::snapshotStats, this));
    }

    reconnectingScopeGuard.release();
    tcpScopeGuard.release();

    return bmqt::GenericResult::e_SUCCESS;
}

void Application::onStartTimeout()
{
    // executed by the *SCHEDULER* thread

    d_brokerSession.onStartTimeout();
}

void Application::snapshotStats()
{
    //         executed by the *SCHEDULER* thread
    // (and by the *MAIN* thread (in destructor))

    d_rootStatContext.snapshot();
    if (d_nextStatDump > 0 && --d_nextStatDump == 0) {
        // NOTE: This is subject to the scheduler issue where it will try to
        //       catch-up after falling behind.
        d_nextStatDump = d_sessionOptions.statsDumpInterval().seconds();
        printStats(false);
    }
}

void Application::printStats(bool isFinal)
{
    //         executed by the *SCHEDULER* thread
    // (and by the *MAIN* thread (in destructor))

    bmqu::MemOutStream os;

    os << "#### stats [delta = last "
       << d_sessionOptions.statsDumpInterval().seconds() << " seconds] ####\n";

    // Print broker session stats
    d_brokerSession.printStats(os, !isFinal);

    // Print allocators stats
    if (!isFinal) {
        d_allocatorStatContext.snapshot();
        os << ":: Allocators";
        if (d_lastAllocatorSnapshot != 0) {
            os << " [Last snapshot was "
               << bmqu::PrintUtil::prettyTimeInterval(
                      bmqsys::Time::highResolutionTimer() -
                      d_lastAllocatorSnapshot)
               << " ago.]";
        }
        d_lastAllocatorSnapshot = bmqsys::Time::highResolutionTimer();

        bmqma::CountingAllocatorUtil::printAllocations(os,
                                                       *d_allocator.context());
        os << "\n";
    }

    os << "::::: TCP Channels >>";
    if (isFinal) {
        // For the final stats, no need to print the 'delta' columns
        bmqst::Table                  table;
        bmqst::BasicTableInfoProvider tip;
        bmqio::StatChannelFactoryUtil::initializeStatsTable(
            &table,
            &tip,
            d_channelsStatContext_mp.get());
        table.records().update();
        bmqst::TableUtil::printTable(os, tip);
    }
    else {
        d_channelsTable.records().update();
        bmqst::TableUtil::printTable(os, d_channelsTip);
    }

    BALL_LOG_INFO << os.str();

    // We don't cleanup the stat context: we deleted the
    // managedPtr<StatContext> for the closed queue, so they will show up as
    // deleted, but we still want to print the stats for all queues (especially
    // during the shutdown print).
    // d_rootStatContext.cleanup();
}

bmqt::GenericResult::Enum
Application::stateCb(bmqimp::BrokerSession::State::Enum    oldState,
                     bmqimp::BrokerSession::State::Enum    newState,
                     bmqimp::BrokerSession::FsmEvent::Enum event)
{
    // executed by the FSM thread

    BALL_LOG_INFO << "State transitioning from " << oldState << " to "
                  << newState << " on " << event;

    bmqt::GenericResult::Enum res = bmqt::GenericResult::e_SUCCESS;
    if (newState == bmqimp::BrokerSession::State::e_CLOSING_SESSION) {
        if (d_connectHandle_mp) {
            // Cancel the reconnecting handle: do it here (as soon as possible
            // after user called 'stop()') instead of in 'e_STOPPED' to prevent
            // a race where the channel would be reconnected and a channel up
            // event would trigger while state is closing.
            d_connectHandle_mp->cancel();
            d_connectHandle_mp.reset();
        }
    }
    else if (newState == bmqimp::BrokerSession::State::e_STOPPED) {
        BALL_LOG_INFO << "::: STOP (FINALIZE) :::";

        if (oldState != bmqimp::BrokerSession::State::e_STOPPED) {
            // Session was once started, perform cleanup

            if (d_connectHandle_mp) {
                // In most cases, the reconnecting handle has been canceled
                // inside the 'e_CLOSING_SESSION' state transition.  However,
                // if the session is being stopped before the start succeeded
                // (when state is 'e_STARTING'), it then transitions straight
                // to 'e_STOPPED'), therefore we need to also cancel it here.
                d_connectHandle_mp->cancel();
                d_connectHandle_mp.reset();
            }

            brokerSessionStopped(event);
        }
    }
    else if (newState == bmqimp::BrokerSession::State::e_STARTING) {
        res = startChannel();
    }

    return res;
}

Application::Application(
    const bmqt::SessionOptions&             sessionOptions,
    const bmqp_ctrlmsg::NegotiationMessage& negotiationMessage,
    const EventQueue::EventHandlerCallback& eventHandlerCB,
    bslma::Allocator*                       allocator)
: d_allocatorStatContext(bmqst::StatContextConfiguration("Allocators",
                                                         allocator),
                         allocator)
, d_allocator("Application", &d_allocatorStatContext, allocator)
, d_allocators(&d_allocator)
, d_rootStatContext(statContextConfiguration(sessionOptions, allocator),
                    d_allocators.get("Statistics"))
, d_channelsStatContext_mp(d_rootStatContext.addSubcontext(
      bmqio::StatChannelFactoryUtil::statContextConfiguration("channels",
                                                              -1,
                                                              allocator)))
, d_sessionOptions(sessionOptions, &d_allocator)
, d_channelsTable(&d_allocator)
, d_channelsTip(&d_allocator)
, d_blobBufferFactory(sessionOptions.blobBufferSize(),
                      d_allocators.get("BlobBufferFactory"))
, d_blobSpPool(
      bmqp::BlobPoolUtil::createBlobPool(&d_blobBufferFactory,
                                         d_allocators.get("BlobSpPool")))
, d_scheduler(bsls::SystemClockType::e_MONOTONIC, &d_allocator)
, d_channelFactory(ntcCreateInterfaceConfig(sessionOptions, allocator),
                   &d_blobBufferFactory,
                   allocator)
, d_resolvingChannelFactory(
      bmqio::ResolvingChannelFactoryConfig(
          &d_channelFactory,
          bmqex::ExecutionPolicyUtil::oneWay().alwaysBlocking().useExecutor(
              bmqex::SystemExecutor())),
      allocator)
, d_reconnectingChannelFactory(
      bmqio::ReconnectingChannelFactoryConfig(&d_resolvingChannelFactory,
                                              &d_scheduler,
                                              allocator),
      allocator)
, d_statChannelFactory(
      bmqio::StatChannelFactoryConfig(
          &d_reconnectingChannelFactory,
          bdlf::BindUtil::bind(&Application::channelStatContextCreator,
                               this,
                               bdlf::PlaceHolders::_1,   // channel
                               bdlf::PlaceHolders::_2),  // handle
          allocator),
      allocator)
, d_negotiatedChannelFactory(
      NegotiatedChannelFactoryConfig(&d_statChannelFactory,
                                     negotiationMessage,
                                     sessionOptions.connectTimeout(),
                                     &d_blobBufferFactory,
                                     &d_blobSpPool,
                                     allocator),
      allocator)
, d_connectHandle_mp()
, d_brokerSession(&d_scheduler,
                  &d_blobBufferFactory,
                  &d_blobSpPool,
                  d_sessionOptions,
                  eventHandlerCB,
                  bdlf::MemFnUtil::memFn(&Application::stateCb, this),
                  d_allocators.get("Session"))
, d_startTimeoutHandle()
, d_statSnaphotTimerHandle()
, d_nextStatDump(-1)
, d_lastAllocatorSnapshot(0)
{
    // NOTE:
    //   o The persistent session pool must live longer than the brokerSession
    //     because brokerSession enqueue blob from the IO to the eventQueue,
    //     and those blobs are using the blob buffer factory from the
    //     ChannelPool. We don't use the ChannelPool constructor that allows to
    //     inject the BlobBufferFactory because the ChannelPool is supplied to
    //     this object at constructor time, and it would then require to also
    //     inject the counting allocator catalog.
    //   o The 'createNegotiator' is passing the &d_brokerSession *before*
    //     BrokerSession is created, but this is fine, the address of the
    //     object is not being changed by constructing the object.

    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM << "Creating Application "
                               << "[bmq: " << bmqscm::Version::version()
                               << "], options:\n";
        d_sessionOptions.print(BALL_LOG_OUTPUT_STREAM, 1, 4);
    }

    BSLMT_ONCE_DO
    {
        // Make TimeUtil thread-safe by calling initialize
        bsl::srand(unsigned(bsl::time(0)));
        // For calls to 'rand()' in the reconnecting channel factory

        bmqsys::Time::initialize();
        bmqp::Crc32c::initialize();
    }

    // UriParser and ProtocolUtil initialization/shutdown are thread-safe and
    // refcounted
    bmqt::UriParser::initialize();
    bmqp::ProtocolUtil::initialize();

    // Start the EventScheduler.  We do this here in constructor and not in
    // 'start()', because the 'finalizeCb', which is used to perform the final
    // shutdown sequence will use the scheduler to execute code in a different
    // thread than the EventHandler thread (where it's being called from). So
    // we can not call 'stop' on the scheduler from the 'finalizeCb', and
    // therefore have to let it stop in the application thread, i.e., from the
    // destructor of this object.
    bslmt::ThreadAttributes attr = bmqsys::ThreadUtil::defaultAttributes();
    attr.setThreadName("bmqScheduler");
    int rc = d_scheduler.start(attr);
    if (rc != 0) {
        BSLS_ASSERT_OPT(false && "Failed to start eventScheduler");
        return;  // RETURN
    }

    // Initialize stats
    bmqst::StatValue::SnapshotLocation start;
    bmqst::StatValue::SnapshotLocation end;
    if (d_sessionOptions.statsDumpInterval() != bsls::TimeInterval()) {
        start.setLevel(1).setIndex(0);
        end.setLevel(1).setIndex(
            d_sessionOptions.statsDumpInterval().seconds() / 30);
    }
    else {
        start.setLevel(0).setIndex(0);
        end.setLevel(0).setIndex(1);
    }

    d_brokerSession.initializeStats(&d_rootStatContext, start, end);

    // Create the channels stat context table
    bmqio::StatChannelFactoryUtil::initializeStatsTable(
        &d_channelsTable,
        &d_channelsTip,
        d_channelsStatContext_mp.get(),
        start,
        end);
}

Application::~Application()
{
    BALL_LOG_INFO << "Destroying Application";

    // Calling stop() here. It is ok even if the session is already stopped
    stop();

    d_scheduler.stop();

    // ProtocolUtil::shutdown is thread-safe and ref-counted
    bmqp::ProtocolUtil::shutdown();
    bmqt::UriParser::shutdown();
}

int Application::start(const bsls::TimeInterval& timeout)
{
    BALL_LOG_INFO << "::: START (SYNC) << [state: " << d_brokerSession.state()
                  << "] :::";

    return d_brokerSession.start(timeout);
}

int Application::startAsync(const bsls::TimeInterval& timeout)
{
    BALL_LOG_INFO << "::: START (ASYNC) [state: " << d_brokerSession.state()
                  << "] :::";

    int rc = d_brokerSession.startAsync();
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to start brokerSession [rc: " << rc << "]";
        return rc;  // RETURN
    }

    // Schedule a timeout
    d_scheduler.scheduleEvent(
        &d_startTimeoutHandle,
        bmqsys::Time::nowMonotonicClock() + timeout,
        bdlf::MemFnUtil::memFn(&Application::onStartTimeout, this));

    return bmqt::GenericResult::e_SUCCESS;
}

void Application::stop()
{
    BALL_LOG_INFO << "::: STOP (SYNC) [state: " << d_brokerSession.state()
                  << "] :::";

    // Stop the brokerSession
    d_brokerSession.stop();
}

void Application::stopAsync()
{
    BALL_LOG_INFO << "::: STOP (ASYNC) [state: " << d_brokerSession.state()
                  << "] :::";

    // Stop the brokerSession
    d_brokerSession.stopAsync();
}

}  // close package namespace
}  // close enterprise namespace
