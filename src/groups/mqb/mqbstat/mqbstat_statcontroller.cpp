// Copyright 2017-2023 Bloomberg Finance L.P.
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

// mqbstat_statcontroller.cpp                                         -*-C++-*-
#include <mqbstat_statcontroller.h>

#include <mqbscm_version.h>
// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbcmd_messages.h>
#include <mqbplug_pluginfactory.h>
#include <mqbplug_pluginmanager.h>
#include <mqbplug_plugintype.h>
#include <mqbplug_statconsumer.h>
#include <mqbscm_versiontag.h>
#include <mqbstat_brokerstats.h>
#include <mqbstat_clusterstats.h>
#include <mqbstat_domainstats.h>
#include <mqbstat_queuestats.h>

// MWC
#include <mwcio_statchannelfactory.h>
#include <mwcst_statcontext.h>
#include <mwcst_statvalue.h>
#include <mwcsys_threadutil.h>
#include <mwcsys_time.h>
#include <mwctsk_alarmlog.h>
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>
#include <mwcu_stringutil.h>

// BDE
#include <ball_context.h>
#include <ball_log.h>
#include <ball_loggermanager.h>
#include <bdlb_scopeexit.h>
#include <bdlb_string.h>
#include <bdlb_stringrefutil.h>
#include <bdlbb_blob.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlmt_eventscheduler.h>
#include <bdlt_timeunitratio.h>
#include <bsl_algorithm.h>
#include <bsl_ctime.h>
#include <bsl_iostream.h>
#include <bslma_allocator.h>
#include <bslmt_semaphore.h>
#include <bslmt_threadutil.h>
#include <bsls_performancehint.h>
#include <bsls_systemclocktype.h>
#include <bsls_timeinterval.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbstat {

namespace {

const char k_PUBLISHINTERVAL_SUFFIX[] = ".PUBLISHINTERVAL";

typedef bsl::unordered_set<mqbplug::PluginFactory*> PluginFactories;

/// Post on the optionally specified `semaphore`.
void optionalSemaphorePost(bslmt::Semaphore* semaphore)
{
    if (semaphore) {
        semaphore->post();
    }
}

}  // close unnamed namespace

// ------------------------
// class StatContextDetails
// ------------------------

StatController::StatContextDetails::StatContextDetails()
: d_statContext_sp(0)
, d_managed(false)
{
    // NOTHING
}

StatController::StatContextDetails::StatContextDetails(
    const StatContextSp& statContext,
    bool                 managed)
: d_statContext_sp(statContext)
, d_managed(managed)
{
    // NOTHING
}

StatController::StatContextDetails::StatContextDetails(
    const StatContextDetails& rhs,
    BSLS_ANNOTATION_UNUSED bslma::Allocator* allocator_p)
: d_statContext_sp(rhs.d_statContext_sp)
, d_managed(rhs.d_managed)
{
    // NOTHING
}

// --------------------
// class StatController
// --------------------

void StatController::initializeStats()
{
    // NOTE: For now, use only one level and with the snapshot interval (10s).
    //       Later, we may need to snapshot every seconds, or keep 5 minutes
    //       stats with multiple levels..
    const mqbcfg::AppConfig& brkrCfg = mqbcfg::BrokerConfig::get();
    int historySize = brkrCfg.stats().printer().printInterval() /
                          brkrCfg.stats().snapshotInterval() +
                      1;

    // ----------
    // Allocators
    if (d_allocatorsStatContext_p) {
        // Allocator stat context may be null as determined by higher level
        // components (not the case in this branch).
        d_statContextsMap.insert(bsl::make_pair(
            bsl::string("allocators"),
            StatContextDetails(StatContextSp(d_allocatorsStatContext_p,
                                             bslstl::SharedPtrNilDeleter(),
                                             d_allocator_p),
                               true)));
        d_allocatorsStatContext_p->snapshot();
        // Call snaphost to create the 'subContext', so that the
        // 'mqbstat::Printer::start' can access it.
    }

    // --------
    // Channels
    StatContextSp channels(
        mwcio::StatChannelFactoryUtil::createStatContext("channels",
                                                         historySize,
                                                         d_allocator_p),
        d_allocator_p);
    d_statContextsMap.insert(
        bsl::make_pair(bsl::string("channels"),
                       StatContextDetails(channels, false)));
    d_statContextChannelsLocal_mp = channels->addSubcontext(
        mwcst::StatContextConfiguration("local").storeExpiredSubcontextValues(
            true));
    d_statContextChannelsRemote_mp = channels->addSubcontext(
        mwcst::StatContextConfiguration("remote").storeExpiredSubcontextValues(
            true));

    // ------
    // System
    d_statContextsMap.insert(bsl::make_pair(
        bsl::string("system"),
        StatContextDetails(StatContextSp(d_systemStatMonitor_mp->statContext(),
                                         bslstl::SharedPtrNilDeleter(),
                                         d_allocator_p),
                           true)));

    // ------
    // Broker
    bslma::Allocator* brokerAllocator = d_allocators.get("BrokerStats");
    d_statContextsMap.insert(bsl::make_pair(
        bsl::string("broker"),
        StatContextDetails(
            BrokerStatsUtil::initializeStatContext(historySize,
                                                   brokerAllocator),
            false)));

    // -------
    // Domains
    bslma::Allocator* domainsAllocator = d_allocators.get("DomainsStats");
    d_statContextsMap.insert(bsl::make_pair(
        bsl::string("domains"),
        StatContextDetails(
            DomainStatsUtil::initializeStatContext(historySize,
                                                   domainsAllocator),
            false)));

    // ------------
    // DomainQueues
    bslma::Allocator* domainQueuesAllocator = d_allocators.get(
        "DomainQueuesStats");
    d_statContextsMap.insert(bsl::make_pair(
        bsl::string("domainQueues"),
        StatContextDetails(QueueStatsUtil::initializeStatContextDomains(
                               historySize,
                               domainQueuesAllocator),
                           false)));

    // -------
    // Clients
    bslma::Allocator* clientsAllocator = d_allocators.get("ClientsStats");
    d_statContextsMap.insert(bsl::make_pair(
        bsl::string("clients"),
        StatContextDetails(
            QueueStatsUtil::initializeStatContextClients(historySize,
                                                         clientsAllocator),
            false)));

    // ------------
    // ClusterNodes
    bslma::Allocator* clusterNodesAllocator = d_allocators.get(
        "ClusterNodesStats");
    d_statContextsMap.insert(bsl::make_pair(
        bsl::string("clusterNodes"),
        StatContextDetails(ClusterStatsUtil::initializeStatContextClusterNodes(
                               historySize,
                               clusterNodesAllocator),
                           false)));

    // --------
    // Clusters
    bslma::Allocator* clustersAllocator = d_allocators.get("ClustersStats");
    d_statContextsMap.insert(bsl::make_pair(
        bsl::string("clusters"),
        StatContextDetails(
            ClusterStatsUtil::initializeStatContextCluster(historySize,
                                                           clustersAllocator),
            false)));
}

void StatController::captureStats(mqbcmd::StatResult* result)
{
    // This must execute in the *SCHEDULER* thread.

    if (d_allocatorsStatContext_p) {
        // When using test allocator, we don't have a stat context
        d_allocatorsStatContext_p->snapshot();
    }

    mwcu::MemOutStream os;
    d_printer_mp->printStats(os);
    result->makeStats(os.str());
}

void StatController::captureStatsAndSemaphorePost(mqbcmd::StatResult* result,
                                                  bslmt::Semaphore* semaphore)
{
    // executed by the *SCHEDULER* thread

    captureStats(result);
    semaphore->post();
}

void StatController::setTunable(mqbcmd::StatResult*       result,
                                const mqbcmd::SetTunable& tunable,
                                bslmt::Semaphore*         semaphore)
{
    // executed by the *SCHEDULER* thread

    // RAII to ensure we will post on the semaphore no matter how we return
    bdlb::ScopeExitAny semaphorePost(
        bdlf::BindUtil::bind(&optionalSemaphorePost, semaphore));

    const mqbcfg::StatsConfig& statsCfg = mqbcfg::BrokerConfig::get().stats();
    const int                  snapshotInterval = statsCfg.snapshotInterval();
    const int maxPublishInterval = d_statConsumerMaxPublishInterval;

    // Handle '<STATCONSUMER>.PUBLISHINTERVAL' tunable.
    size_t suffixPos = tunable.name().size() -
                       (sizeof(k_PUBLISHINTERVAL_SUFFIX) - 1);
    if (tunable.name().size() > sizeof(k_PUBLISHINTERVAL_SUFFIX) &&
        bdlb::StringRefUtil::areEqualCaseless(
            bslstl::StringRef(tunable.name().begin() + suffixPos,
                              tunable.name().end()),
            k_PUBLISHINTERVAL_SUFFIX)) {
        // Target consumer's name is whatever precedes ".PUBLISHINTERVAL".
        bslstl::StringRef consumerName(tunable.name().begin(),
                                       tunable.name().begin() + suffixPos);

        // Look up the target configuration.
        const mqbcfg::StatPluginConfig* targetConsumerCfg = 0;
        {
            bsl::vector<mqbcfg::StatPluginConfig>::const_iterator it =
                statsCfg.plugins().begin();
            for (; it != statsCfg.plugins().end(); ++it) {
                if (bdlb::StringRefUtil::areEqualCaseless(it->name(),
                                                          consumerName)) {
                    targetConsumerCfg = &(*it);
                    break;
                }
            }
        }
        if (!targetConsumerCfg) {
            mwcu::MemOutStream output;
            output << "No configuration found for StatConsumer '"
                   << consumerName << "'";
            result->makeError();
            result->error().message() = output.str();
            return;  // RETURN
        }

        // Look up the target consumer.
        mqbplug::StatConsumer* targetConsumer = 0;
        {
            bsl::vector<StatConsumerMp>::const_iterator it =
                d_statConsumers.begin();
            for (; it != d_statConsumers.end(); ++it) {
                if (bdlb::StringRefUtil::areEqualCaseless(it->get()->name(),
                                                          consumerName)) {
                    targetConsumer = it->get();
                    break;
                }
            }
        }
        if (!targetConsumer) {
            mwcu::MemOutStream output;
            output << "StatConsumer '" << consumerName << "' does not exist";
            result->makeError();
            result->error().message() = output.str();
            return;  // RETURN
        }

        if (!tunable.value().isTheIntegerValue() ||
            tunable.value().theInteger() < -1 ||
            (tunable.value().theInteger() > 0 &&
             (tunable.value().theInteger() < snapshotInterval ||
              (tunable.value().theInteger() % snapshotInterval) != 0 ||
              tunable.value().theInteger() > maxPublishInterval))) {
            mwcu::MemOutStream output;
            output << "StatConsumer PUBLISHINTERVAL tunables must be "
                      "multiples of snapshot interval ("
                   << snapshotInterval
                   << ") and cannot be greater than max publish interval ("
                   << maxPublishInterval << "); it can be specified as 0 to "
                   << "disable the publishing or as -1 to reset the publish "
                   << "interval to default value, but instead the "
                   << "following was specified: " << tunable.value();
            result->makeError();
            result->error().message() = output.str();
            return;  // RETURN
        }

        int newValue = tunable.value().theInteger();
        int oldValue = targetConsumer->publishInterval().seconds();

        if (newValue == -1) {
            newValue = targetConsumerCfg->publishInterval();
            BALL_LOG_INFO << "Reset StatConsumer publish interval to default "
                             "value: "
                          << newValue << "[consumer: '" << consumerName
                          << "']";
        }
        else if (newValue == 0) {
            BALL_LOG_INFO << "Disable StatConsumer reporting [consumer: '"
                          << consumerName << "']";
        }

        mwcu::MemOutStream tunableConfirmationName;
        {
            bsl::string consumerNameLower(consumerName, d_allocator_p);
            bdlb::String::toLower(&consumerNameLower);
            tunableConfirmationName << consumerNameLower << ".publishInterval";
        }
        mqbcmd::TunableConfirmation& tunableConfirmation =
            result->makeTunableConfirmation();
        tunableConfirmation.name() = tunableConfirmationName.str();
        tunableConfirmation.oldValue().makeTheInteger(oldValue);
        tunableConfirmation.newValue().makeTheInteger(newValue);

        targetConsumer->setPublishInterval(bsls::TimeInterval(newValue));
        return;  // RETURN
    }

    mwcu::MemOutStream output;
    output << "Unsupported tunable '" << tunable << "': Issue the "
           << "LIST_TUNABLES command for the list of supported tunables.";
    result->makeError();
    result->error().message() = output.str();
}

void StatController::getTunable(mqbcmd::StatResult* result,
                                const bsl::string&  tunable,
                                bslmt::Semaphore*   semaphore)
{
    // executed by the *SCHEDULER* thread

    // RAII to ensure we will post on the semaphore no matter how we return
    bdlb::ScopeExitAny semaphorePost(
        bdlf::BindUtil::bind(&optionalSemaphorePost, semaphore));

    size_t suffixPos = tunable.size() - (sizeof(k_PUBLISHINTERVAL_SUFFIX) - 1);
    if (tunable.size() > sizeof(k_PUBLISHINTERVAL_SUFFIX) &&
        bdlb::StringRefUtil::areEqualCaseless(
            bslstl::StringRef(tunable.begin() + suffixPos, tunable.end()),
            k_PUBLISHINTERVAL_SUFFIX)) {
        // Target consumer's name is whatever precedes ".PUBLISHINTERVAL".
        bslstl::StringRef consumerName(tunable.begin(),
                                       tunable.begin() + suffixPos);

        // Look up the target consumer.
        mqbplug::StatConsumer*                      targetConsumer = 0;
        bsl::vector<StatConsumerMp>::const_iterator it =
            d_statConsumers.begin();
        for (; it != d_statConsumers.end(); ++it) {
            if (bdlb::StringRefUtil::areEqualCaseless(it->get()->name(),
                                                      consumerName)) {
                targetConsumer = it->get();
                break;
            }
        }
        if (!targetConsumer) {
            mwcu::MemOutStream output;
            output << "StatConsumer '" << consumerName << "' does not exist";
            result->makeError();
            result->error().message() = output.str();
            return;  // RETURN
        }

        mwcu::MemOutStream tunableConfirmationName;
        {
            bsl::string consumerNameLower(consumerName, d_allocator_p);
            bdlb::String::toLower(&consumerNameLower);
            tunableConfirmationName << consumerNameLower << ".publishInterval";
        }
        mqbcmd::Tunable& tunableObj = result->makeTunable();
        tunableObj.name()           = tunableConfirmationName.str();
        tunableObj.value().makeTheInteger(
            targetConsumer->publishInterval().seconds());
        return;  // RETURN
    }

    mwcu::MemOutStream output;
    output << "Unsupported tunable '" << tunable << "': Issue the "
           << "LIST_TUNABLES command for the list of supported tunables.";
    result->makeError();
    result->error().message() = output.str();
}

void StatController::listTunables(mqbcmd::StatResult* result,
                                  bslmt::Semaphore*   semaphore)
{
    // executed by the *SCHEDULER* thread

    // RAII to ensure we will post on the semaphore no matter how we return
    bdlb::ScopeExitAny semaphorePost(
        bdlf::BindUtil::bind(&optionalSemaphorePost, semaphore));

    bsls::TimeInterval publishInterval(0);
    if (d_statConsumers.size() > 0) {
        publishInterval = (*d_statConsumers.begin())->publishInterval();
    }

    mqbcmd::Tunables& tunables = result->makeTunables();

    bsl::vector<StatConsumerMp>::const_iterator it = d_statConsumers.begin();
    for (; it != d_statConsumers.end(); ++it) {
        mqbcmd::Tunable& tunable = tunables.tunables().emplace_back();

        mwcu::MemOutStream tunableConfirmationName;
        {
            bsl::string consumerNameUpper((*it)->name(), d_allocator_p);
            bdlb::String::toUpper(&consumerNameUpper);
            tunableConfirmationName << consumerNameUpper << ".PUBLISHINTERVAL";
        }
        tunable.name() = tunableConfirmationName.str();
        tunable.value().makeTheInteger((*it)->publishInterval().seconds());

        mwcu::MemOutStream description;
        description
            << "non-negative integer value of the publish interval for the '"
            << (*it)->name()
            << "' StatConsumer. Must be a multiple of "
               "snapshot interval and cannot be greater than max publish "
               "interval. It can be specified as 0 to disable the publishing "
               "or as -1 to reset the publish interval to default value.";
        tunable.description() = description.str();
    }
}

void StatController::snapshot()
{
    // executed by the *SCHEDULER* thread

    // Safeguard against too frequent invocation from the scheduler.
    const bsls::Types::Int64 now     = mwcsys::Time::highResolutionTimer();
    const bsls::Types::Int64 nsDelta = now - d_lastSnapshotTime;
    const bsls::Types::Int64 minDelta =
        mqbcfg::BrokerConfig::get().stats().snapshotInterval() *
        bdlt::TimeUnitRatio::k_NS_PER_S / 2;
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(nsDelta < minDelta)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        if (d_lastSnapshotLogLimiter.requestPermission()) {
            BALL_LOG_INFO << "snapshot invoked too frequently (delta = "
                          << mwcu::PrintUtil::prettyTimeInterval(nsDelta)
                          << "), skipping snapshot";
        }
        return;  // RETURN
    }

    d_lastSnapshotTime = now;

    // Snapshot all root stat contexts
    for (StatContextDetailsMap::iterator mit = d_statContextsMap.begin();
         mit != d_statContextsMap.end();
         ++mit) {
        if (!mit->second.d_managed) {
            BSLS_ASSERT_SAFE(mit->second.d_statContext_sp);
            mit->second.d_statContext_sp->snapshot();
        }
    }

    // The system stat monitor does some additional work that is invoked
    // through snapshot
    d_systemStatMonitor_mp->snapshot();

    // StatConsumers will report all stats
    bsl::vector<StatConsumerMp>::iterator it = d_statConsumers.begin();
    for (; it != d_statConsumers.end(); ++it) {
        (*it)->onSnapshot();
    }

    // Printer needs to be notified of every snapshot, but has an internal
    // action counter to know when it's time to print.  Allocator stat context
    // only has an history size of 2, so we need to snapshot only once, just
    // before printing.
    const bool willPrint = d_printer_mp->nextSnapshotWillPrint();
    if (d_allocatorsStatContext_p && willPrint) {
        d_allocatorsStatContext_p->snapshot();
    }

    d_printer_mp->onSnapshot();

    // Finally, perform cleanup of expired stat contexts if we have printed
    // them.
    // TBD: Currently the code relies on the fact that 'printer' is the one
    //      with the largest 'actionInterval', but normally cleanup should be
    //      done after the less frequent consumer has performed its action.
    if (!d_printer_mp->isEnabled() || willPrint) {
        // Cleanup deleted subcontexts now that we printed them, or don't care
        for (StatContextDetailsMap::iterator mit = d_statContextsMap.begin();
             mit != d_statContextsMap.end();
             ++mit) {
            if (mit->second.d_statContext_sp) {
                mit->second.d_statContext_sp->cleanup();
            }
        }
    }
}

int StatController::validateConfig(bsl::ostream& errorDescription) const
{
    const mqbcfg::AppConfig& brkrCfg = mqbcfg::BrokerConfig::get();

    if (brkrCfg.stats().snapshotInterval() <= 0) {
        // Stats are disabled, nothing to validate
        return 0;  // RETURN
    }

    if ((brkrCfg.stats().printer().printInterval() %
         brkrCfg.stats().snapshotInterval()) != 0) {
        errorDescription << "StatController: 'printInterval' ("
                         << brkrCfg.stats().printer().printInterval()
                         << ") must be a multiple of 'snapshotInterval' ("
                         << brkrCfg.stats().snapshotInterval() << ")";
        return -1;  // RETURN
    }

    bsl::vector<mqbcfg::StatPluginConfig>::const_iterator it =
        brkrCfg.stats().plugins().begin();
    for (; it != brkrCfg.stats().plugins().end(); ++it) {
        if (it->name().empty()) {
            errorDescription << "StatController: Plugin 'name' must be given "
                                "a value";
            return -1;
        }

        if ((it->publishInterval() % brkrCfg.stats().snapshotInterval()) !=
            0) {
            errorDescription << "StatController: 'publishInterval' for "
                                "consumer '"
                             << it->name() << "' (" << it->publishInterval()
                             << ") must be a multiple of 'snapshotInterval' ("
                             << brkrCfg.stats().snapshotInterval() << ")";
            return -1;  // RETURN
        }

        if (it->publishInterval() >
            brkrCfg.stats().printer().printInterval()) {
            errorDescription << "StatController: publishInterval for "
                                "consumer '"
                             << it->name() << "' (" << it->publishInterval()
                             << ") cannot be greater than 'printInterval' ("
                             << brkrCfg.stats().printer().printInterval()
                             << ")";
            return -1;  // RETURN
        }
    }
    return 0;
}

StatController::StatController(const CommandProcessorFn& commandProcessor,
                               mqbplug::PluginManager*   pluginManager,
                               bdlbb::BlobBufferFactory* bufferFactory,
                               mwcst::StatContext*       allocatorsStatContext,
                               bdlmt::EventScheduler*    eventScheduler,
                               bslma::Allocator*         allocator)
: d_allocators(allocator)
, d_scheduler_mp(0)
, d_lastSnapshotTime()
, d_allocatorsStatContext_p(allocatorsStatContext)
, d_statContextsMap(allocator)
, d_statContextChannelsLocal_mp(0)
, d_statContextChannelsRemote_mp(0)
, d_systemStatMonitor_mp(0)
, d_pluginManager_p(pluginManager)
, d_bufferFactory_p(bufferFactory)
, d_commandProcessorFn(bsl::allocator_arg, allocator, commandProcessor)
, d_printer_mp(0)
, d_statConsumers(allocator)
, d_statConsumerMaxPublishInterval(0)
, d_eventScheduler_p(eventScheduler)
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(eventScheduler->clockType() ==
                     bsls::SystemClockType::e_MONOTONIC);

    d_lastSnapshotLogLimiter.initialize(1,
                                        15 * bdlt::TimeUnitRatio::k_NS_PER_M);
    // Throttling of one maximum alarm per 15 minutes
}

int StatController::start(bsl::ostream& errorDescription)
{
    const mqbcfg::AppConfig& brkrCfg = mqbcfg::BrokerConfig::get();

    int rc = 0;

    if (validateConfig(errorDescription) != 0) {
        return -1;  // RETURN
    }

    if (brkrCfg.stats().snapshotInterval() <= 0) {
        BALL_LOG_WARN << "#STATS Statistics collection have been *disabled* "
                      << "[reason: snapshotInterval is <= 0]";
        return 0;  // RETURN
    }

    // Start the scheduler
    d_scheduler_mp.load(new (*d_allocator_p) bdlmt::TimerEventScheduler(
                            bsls::SystemClockType::e_MONOTONIC,
                            d_allocator_p),
                        d_allocator_p);
    rc = d_scheduler_mp->start();
    if (rc != 0) {
        errorDescription << "Failed to start StatController "
                         << "TimerEventScheduler component (rc: " << rc << ")";
        return -2;  // RETURN
    }

    if (mwcsys::ThreadUtil::k_SUPPORT_THREAD_NAME) {
        d_scheduler_mp->scheduleEvent(
            bsls::TimeInterval(0),  // execute as soon as possible
            bdlf::BindUtil::bind(&mwcsys::ThreadUtil::setCurrentThreadName,
                                 "bmqSchedStat"));
    }

    // Create and start the system stat monitor.  The SystemStats are used in
    // the dashboard screen, stat consumers, stats printer, ...  So we need to
    // configure the SystemStatMonitor to hold the max of those two intervals.
    int maxInterval = brkrCfg.stats().printer().printInterval();
    bsl::vector<mqbcfg::StatPluginConfig>::const_iterator consumerIt =
        brkrCfg.stats().plugins().begin();
    for (; consumerIt != brkrCfg.stats().plugins().end(); ++consumerIt) {
        maxInterval = bsl::max(maxInterval, consumerIt->publishInterval());
    }
    d_systemStatMonitor_mp.load(
        new (*d_allocator_p) mwcsys::StatMonitor(maxInterval, d_allocator_p),
        d_allocator_p);

    // Failing to start some subsystems of StatController should not result in
    // a broker shutdown, but rather be intercepted here and printed as an
    // error, therefore use a local 'errorDescription' stream and not the
    // supplied one.
    mwcu::MemOutStream errorStream(d_allocator_p);

    rc = d_systemStatMonitor_mp->start(errorStream);
    if (rc != 0) {
        MWCTSK_ALARMLOG_ALARM("#STATS")
            << "Failed to start SystemStatMonitor [rc: " << rc << ","
            << " error: '" << errorStream.str() << "']" << MWCTSK_ALARMLOG_END;
        rc = 0;
        errorStream.reset();
    }

    // We now need to initialize our stats *AFTER* the system stat monitor has
    // been created to ensure that we have a valid stat contexts in our map.
    initializeStats();

    // Build Map to be passed to all lower level components
    bsl::unordered_map<bsl::string, mwcst::StatContext*> ctxPtrMap(
        d_allocator_p);
    for (StatContextDetailsMap::iterator ctxIt = d_statContextsMap.begin();
         ctxIt != d_statContextsMap.end();
         ++ctxIt) {
        ctxPtrMap.insert(bsl::make_pair(ctxIt->first,
                                        ctxIt->second.d_statContext_sp.get()));
    }

    // Initialize StatConsumers from plugins.
    {
        PluginFactories pluginFactories(d_allocator_p);
        d_pluginManager_p->get(mqbplug::PluginType::e_STATS_CONSUMER,
                               &pluginFactories);

        PluginFactories::const_iterator factoryIt = pluginFactories.cbegin();
        for (; factoryIt != pluginFactories.cend(); ++factoryIt) {
            mqbplug::StatConsumerPluginFactory* factory =
                dynamic_cast<mqbplug::StatConsumerPluginFactory*>(*factoryIt);
            StatConsumerMp consumer = factory->create(ctxPtrMap,
                                                      d_commandProcessorFn,
                                                      d_bufferFactory_p,
                                                      d_allocator_p);

            if (int status = consumer->start(errorStream)) {
                MWCTSK_ALARMLOG_ALARM("#STATS")
                    << "Failed to start StatConsumer '" << consumer->name()
                    << "' [rc: " << status << ", error: '" << errorStream.str()
                    << "']" << MWCTSK_ALARMLOG_END;
                errorStream.reset();
                continue;  // CONTINUE
            }

            // Look up the target configuration.
            const mqbcfg::StatPluginConfig*              consumerCfg = 0;
            const bsl::vector<mqbcfg::StatPluginConfig>& consumersCfg =
                mqbcfg::BrokerConfig::get().stats().plugins();
            bsl::vector<mqbcfg::StatPluginConfig>::const_iterator cfgIt =
                consumersCfg.begin();
            for (; cfgIt != consumersCfg.end(); ++cfgIt) {
                if (bdlb::StringRefUtil::areEqualCaseless(cfgIt->name(),
                                                          consumer->name())) {
                    consumerCfg = &(*cfgIt);
                    break;
                }
            }
            if (!consumerCfg) {
                BALL_LOG_ERROR << "No configuration found for StatConsumer '"
                               << consumer->name() << "'";
                return -4;  // RETURN
            }

            // Configure the consumer.
            consumer->setPublishInterval(
                bsls::TimeInterval(consumerCfg->publishInterval()));

            // Take ownership of the 'StatController'.
            d_statConsumers.emplace_back(
                bslmf::MovableRefUtil::move(consumer));
        }
    }

    // Start the printer
    d_printer_mp.load(new (*d_allocator_p) Printer(brkrCfg.stats(),
                                                   d_eventScheduler_p,
                                                   ctxPtrMap,
                                                   d_allocator_p),
                      d_allocator_p);

    // Give the printer a map of statContext ptrs for its printing. It has been
    // done this way to break the cyclic dependency of ctxPtrMap,
    // initializeStats() and creation of d_printer.
    rc = d_printer_mp->start(errorStream);
    if (rc != 0) {
        MWCTSK_ALARMLOG_ALARM("#STATS")
            << "Failed to start Printer [rc: " << rc << ", error: '"
            << errorStream.str() << "']" << MWCTSK_ALARMLOG_END;
        rc = 0;
        errorStream.reset();
    }

    // Max value for the stat publish interval must be the minimum history size
    // of all stat contexts.
    d_statConsumerMaxPublishInterval =
        bsl::min(maxInterval, brkrCfg.stats().printer().printInterval());
    d_lastSnapshotTime = mwcsys::Time::highResolutionTimer();

    // Start the clock
    d_scheduler_mp->startClock(
        bsls::TimeInterval(brkrCfg.stats().snapshotInterval()),
        bdlf::BindUtil::bind(&StatController::snapshot, this));
    BALL_LOG_INFO << "Starting statistics  [SnapshotInterval: "
                  << brkrCfg.stats().snapshotInterval() << ", PrintInterval: "
                  << brkrCfg.stats().printer().printInterval() << "]";

    return 0;
}

void StatController::stop()
{
#define STOP_OBJ(OBJ, NAME)                                                   \
    if (OBJ) {                                                                \
        OBJ->stop();                                                          \
    }

#define DESTROY_OBJ(OBJ, NAME)                                                \
    if (OBJ) {                                                                \
        OBJ.clear();                                                          \
    }

    // Stop the scheduler and cancel all clocks first to prevent any additional
    // periodic functions being invoked
    if (d_scheduler_mp) {
        d_scheduler_mp->cancelAllClocks(true);
        d_scheduler_mp->stop();
    }

    // Stop everything
    bsl::vector<StatConsumerMp>::iterator it = d_statConsumers.begin();
    for (; it != d_statConsumers.end(); ++it) {
        STOP_OBJ((*it), it->name());
    }

    STOP_OBJ(d_printer_mp, "Printer");
    STOP_OBJ(d_systemStatMonitor_mp, "SystemStatMonitor");

    // Destroy everything!!
    for (it = d_statConsumers.begin(); it != d_statConsumers.end(); ++it) {
        DESTROY_OBJ((*it), it->name());
    }
    DESTROY_OBJ(d_printer_mp, "Printer");
    DESTROY_OBJ(d_systemStatMonitor_mp, "SystemStatMonitor");
    DESTROY_OBJ(d_scheduler_mp, "Scheduler");

#undef DESTROY_OBJ
#undef STOP_OBJ
}

void StatController::loadStatContexts(StatContexts* contexts)
{
    contexts->reserve(d_statContextsMap.size());
    for (StatContextDetailsMap::const_iterator cit = d_statContextsMap.begin();
         cit != d_statContextsMap.end();
         ++cit) {
        StatContextSp statContextSp = cit->second.d_statContext_sp;
        (*contexts)[cit->first]     = statContextSp.get();
    }
}

int StatController::processCommand(mqbcmd::StatResult*        result,
                                   const mqbcmd::StatCommand& command)
{
    if (command.isShowValue()) {
        bslmt::Semaphore semaphore;
        d_scheduler_mp->scheduleEvent(
            bsls::TimeInterval(),  // as soon as possible
            bdlf::BindUtil::bind(&StatController::captureStatsAndSemaphorePost,
                                 this,
                                 result,
                                 &semaphore));
        semaphore.wait();
        return 0;  // RETURN
    }

    if (command.isSetTunableValue()) {
        bslmt::Semaphore semaphore;

        d_scheduler_mp->scheduleEvent(
            bsls::TimeInterval(),  // asap
            bdlf::BindUtil::bind(&StatController::setTunable,
                                 this,
                                 result,
                                 command.setTunable(),
                                 &semaphore));
        semaphore.wait();
        return 0;  // RETURN
    }

    if (command.isGetTunableValue()) {
        bslmt::Semaphore semaphore;

        d_scheduler_mp->scheduleEvent(
            bsls::TimeInterval(),  // asap
            bdlf::BindUtil::bind(&StatController::getTunable,
                                 this,
                                 result,
                                 command.getTunable(),
                                 &semaphore));
        semaphore.wait();
        return 0;  // RETURN
    }

    if (command.isListTunablesValue()) {
        bslmt::Semaphore semaphore;

        d_scheduler_mp->scheduleEvent(
            bsls::TimeInterval(),  // asap
            bdlf::BindUtil::bind(&StatController::listTunables,
                                 this,
                                 result,
                                 &semaphore));

        semaphore.wait();
        return 0;  // RETURN
    }

    mwcu::MemOutStream os;
    os << "Unknown command '" << command << "'";
    result->makeError();
    result->error().message() = os.str();
    return -1;
}

}  // close package namespace
}  // close enterprise namespace
