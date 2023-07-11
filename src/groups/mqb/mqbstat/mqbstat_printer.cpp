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

// mqbstat_printer.cpp                                                -*-C++-*-
#include <mqbstat_printer.h>

#include <mqbscm_version.h>
// MQB
#include <mqbscm_versiontag.h>
#include <mqbstat_queuestats.h>

// MWC
#include <mwcio_statchannelfactory.h>
#include <mwcma_countingallocator.h>
#include <mwcst_statvalue.h>
#include <mwcst_tableutil.h>
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>

// BDE
#include <ball_context.h>
#include <ball_log.h>
#include <ball_logfilecleanerutil.h>
#include <ball_loggermanager.h>
#include <ball_multiplexobserver.h>
#include <ball_recordattributes.h>
#include <ball_recordstringformatter.h>
#include <ball_transmission.h>
#include <bdls_processutil.h>
#include <bdlt_datetime.h>
#include <bdlt_epochutil.h>
#include <bsl_algorithm.h>
#include <bsl_ctime.h>
#include <bsl_iostream.h>
#include <bslma_allocator.h>
#include <bslmt_threadutil.h>
#include <bsls_assert.h>
#include <bsls_timeinterval.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbstat {

namespace {

const char k_LOG_CATEGORY[] = "MQBSTAT.PRINTER";

// Subcontext names
const char k_SUBCONTEXT_ALLOCATORS[] = "allocators";

}  // close unnamed namespace

// -------------
// class Printer
// -------------

void Printer::initializeTablesAndTips()
{
    const int historySize = d_config.printer().printInterval() /
                                d_config.snapshotInterval() +
                            1;

    ContextsMap::iterator it = d_contexts.find("allocators");
    if (it != d_contexts.end()) {
        Context* context = it->second.get();
        mwcma::CountingAllocator::configureStatContextTableInfoProvider(
            &context->d_table,
            &context->d_tip,
            mwcst::StatValue::SnapshotLocation(0, 0),
            mwcst::StatValue::SnapshotLocation(0, 1));
        context->d_table.records().setContext(
            context->d_statContext_p->getSubcontext(k_SUBCONTEXT_ALLOCATORS));
    }

    Context* context = d_contexts["domainQueues"].get();
    QueueStatsUtil::initializeTableAndTipDomains(&context->d_table,
                                                 &context->d_tip,
                                                 historySize,
                                                 context->d_statContext_p);

    context = d_contexts["clients"].get();
    QueueStatsUtil::initializeTableAndTipClients(&context->d_table,
                                                 &context->d_tip,
                                                 historySize,
                                                 context->d_statContext_p);

    context = d_contexts["clusterNodes"].get();
    QueueStatsUtil::initializeTableAndTipClients(&context->d_table,
                                                 &context->d_tip,
                                                 historySize,
                                                 context->d_statContext_p);

    context = d_contexts["channels"].get();
    mwcst::StatValue::SnapshotLocation start(0, 0);
    mwcst::StatValue::SnapshotLocation end(0, historySize - 1);
    mwcio::StatChannelFactoryUtil::initializeStatsTable(
        &context->d_table,
        &context->d_tip,
        context->d_statContext_p,
        start,
        end);
}

Printer::Printer(const mqbcfg::StatsConfig& config,
                 bdlmt::EventScheduler*     eventScheduler,
                 const StatContextsMap&     statContextsMap,
                 bslma::Allocator*          allocator)
: d_config(config)
, d_statsLogFile(allocator)
, d_lastStatId(0)
, d_actionCounter(0)
, d_lastAllocatorSnapshot(0)
, d_contexts(allocator)
, d_statLogCleaner(eventScheduler, allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(eventScheduler->clockType() ==
                     bsls::SystemClockType::e_MONOTONIC);

    // Insert all the required Contexts
    for (StatContextsMap::const_iterator it = statContextsMap.begin();
         it != statContextsMap.end();
         ++it) {
        bsl::shared_ptr<Context> contextSp;
        contextSp.createInplace(allocator);

        ContextsMap::iterator cit =
            d_contexts.insert(bsl::make_pair(it->first, contextSp)).first;
        cit->second->d_statContext_p = it->second;
        // Table and Tip are left default constructed, and will be set
        // during 'start'.
    }
}

int Printer::start(BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription)
{
    // Setup the print of stats if configured for it
    if (!isEnabled()) {
        return 0;  // RETURN
    }

    d_actionCounter = d_config.printer().printInterval() /
                      d_config.snapshotInterval();

    // Initialize table and tips
    initializeTablesAndTips();

    // Configure the stats dump log file
    d_statsLogFile.enableFileLogging(d_config.printer().file().c_str());
    d_statsLogFile.rotateOnSize(d_config.printer().rotateBytes() / 1024);
    d_statsLogFile.rotateOnTimeInterval(
        bdlt::DatetimeInterval(d_config.printer().rotateDays()));
    d_statsLogFile.setLogFileFunctor(ball::RecordStringFormatter("%m\n"));
    // Record's time is printed not through the record, but part of the
    // 'id banner' (see 'onSnapshot').

    // LogCleanup
    if (d_config.printer().maxAgeDays() <= 0 ||
        d_config.printer().file().empty()) {
        BALL_LOG_INFO << "StatLogCleaning is *disabled* "
                      << "[reason: either 'maxAgeDays' is set to 0 in config "
                      << "or file pattern is empty]";
        return 0;  // RETURN
    }

    bsl::string filePattern;
    ball::LogFileCleanerUtil::logPatternToFilePattern(
        &filePattern,
        d_config.printer().file());
    bsls::TimeInterval maxAge(0, 0);
    maxAge.addDays(d_config.printer().maxAgeDays());

    int rc = d_statLogCleaner.start(filePattern, maxAge);
    if (rc != 0) {
        BALL_LOG_ERROR << "#STATLOG_CLEANING "
                       << "Failed to start log cleaning of '" << filePattern
                       << "' [rc: " << rc << "]";
    }

    return 0;
}

void Printer::stop()
{
    // Dump the final stats (to ensure all events have been accounted for, even
    // if they happen within less than the print interval).  Note that since
    // the scheduler has been stopped, there is no more snapshot happening,
    // therefore it is fine to print stats from this (arbitrary) thread.
    if (d_lastStatId != 0) {
        // Printing needs to access at least 'printInterval' snapshot; in case
        // the broker starts and stops before that amount of snapshot has been
        // taken, it is unsafe to print stats.  The above check ensures that at
        // least one regular stat dump already happened.  This check also takes
        // care of if the printer was disabled.
        logStats();
    }

    // Stop the log cleaner
    d_statLogCleaner.stop();
}

void Printer::printStats(bsl::ostream& stream)
{
    // This must execute in the 'snapshot' thread

    // DOMAINQUEUES
    stream << "\n"
           << ":::::::::: :::::::::: DOMAINQUEUES >>";
    Context* context = d_contexts["domainQueues"].get();
    context->d_table.records().update();
    mwcu::TableUtil::printTable(stream, context->d_tip);

    // CLIENTS
    stream << "\n"
           << ":::::::::: :::::::::: CLIENTS >>";
    context = d_contexts["clients"].get();
    context->d_table.records().update();
    mwcu::TableUtil::printTable(stream, context->d_tip);

    // CLUSTERS
    stream << "\n"
           << ":::::::::: :::::::::: CLUSTERS >>";
    context = d_contexts["clusterNodes"].get();
    context->d_table.records().update();
    mwcu::TableUtil::printTable(stream, context->d_tip);

    // CHANNELS
    stream << "\n"
           << ":::::::::: :::::::::: TCP CHANNELS >>";
    context = d_contexts["channels"].get();
    context->d_table.records().update();
    mwcu::TableUtil::printTable(stream, context->d_tip);

    // ALLOCATORS
    stream << "\n"
           << ":::::::::: :::::::::: ALLOCATORS >>";
    ContextsMap::iterator it = d_contexts.find("allocators");
    if (it == d_contexts.end()) {
        // When using test allocator, we don't have a stat context
        stream << " Unavailable\n";
        return;  // RETURN
    }
    // NOTE: By contract, snapshot needs to be invoked on the statcontexts
    //       prior to this method.
    if (d_lastAllocatorSnapshot != 0) {
        stream << " Last snapshot was "
               << mwcu::PrintUtil::prettyTimeInterval(
                      bsls::TimeUtil::getTimer() - d_lastAllocatorSnapshot)
               << " ago.";
    }
    d_lastAllocatorSnapshot = bsls::TimeUtil::getTimer();

    context = it->second.get();
    context->d_table.records().update();
    mwcu::TableUtil::printTable(stream, context->d_tip);
}

void Printer::logStats()
{
    ++d_lastStatId;

    // Put a 'reference' in the main log file. We do that first in case it
    // crashes/hangs in dump stat, this will help figuring it
    BALL_LOG_INFO << "Stats dumped [id: " << d_lastStatId << "]";

    // Dump to statslog file
    // Prepare the log record and associated attributes
    ball::Record            record;
    ball::RecordAttributes& attributes = record.fixedFields();
    bdlt::Datetime          now;
    bdlt::EpochUtil::convertFromTimeT(&now, time(0));
    attributes.setTimestamp(now);
    attributes.setProcessID(bdls::ProcessUtil::getProcessId());
    attributes.setThreadID(bslmt::ThreadUtil::selfIdAsUint64());
    attributes.setFileName(__FILE__);
    attributes.setLineNumber(__LINE__);
    attributes.setCategory(k_LOG_CATEGORY);
    attributes.setSeverity(ball::Severity::INFO);

    // Dump stats into bmqbrkr.stats.log
    attributes.clearMessage();
    bsl::ostream os(&attributes.messageStreamBuf());
    os << "===== ===== ===== ===== ===== ===== ===== ===== ===== =====\n"
       << "Stats id: " << d_lastStatId << " @ " << now << "\n"
       << "===== ===== ===== ===== ===== ===== ===== ===== ===== =====\n";

    printStats(os);

    d_statsLogFile.publish(
        record,
        ball::Context(ball::Transmission::e_MANUAL_PUBLISH, 0, 1));
}

void Printer::onSnapshot()
{
    // Check if we need to print the stats to log
    if (!isEnabled() || --d_actionCounter != 0) {
        return;  // RETURN
    }

    d_actionCounter = d_config.printer().printInterval() /
                      d_config.snapshotInterval();

    logStats();
}

}  // close package namespace
}  // close enterprise namespace
