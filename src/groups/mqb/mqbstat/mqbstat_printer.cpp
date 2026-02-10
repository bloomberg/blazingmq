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
#include <mqbstat_queuestats.h>

#include <bmqio_statchannelfactory.h>
#include <bmqma_countingallocator.h>
#include <bmqst_statvalue.h>
#include <bmqst_tableutil.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

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
#include <bsl_utility.h>
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
// class TablePrinter
// -------------

void TablePrinter::initializeTablesAndTips()
{
    const int historySize = d_config.printer().printInterval() /
                                d_config.snapshotInterval() +
                            1;

    ContextsMap::iterator it = d_contexts.find("allocators");
    if (it != d_contexts.end()) {
        Context* context = it->second.get();
        bmqma::CountingAllocator::configureStatContextTableInfoProvider(
            &context->d_table,
            &context->d_tip,
            bmqst::StatValue::SnapshotLocation(0, 0),
            bmqst::StatValue::SnapshotLocation(0, 1));
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
    bmqst::StatValue::SnapshotLocation start(0, 0);
    bmqst::StatValue::SnapshotLocation end(0, historySize - 1);
    bmqio::StatChannelFactoryUtil::initializeStatsTable(
        &context->d_table,
        &context->d_tip,
        context->d_statContext_p,
        start,
        end);
}

TablePrinter::TablePrinter(const mqbcfg::StatsConfig& config,
                           bdlmt::EventScheduler*     eventScheduler,
                           const StatContextsMap&     statContextsMap,
                           bslma::Allocator*          allocator)
: d_config(config)
, d_statsLogFile(allocator)
, d_lastAllocatorSnapshot(0)
, d_contexts(allocator)
, d_statLogCleaner(eventScheduler, allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(eventScheduler->clockType() ==
                     bsls::SystemClockType::e_MONOTONIC);

    // Insert all the required Contexts
    for (StatContextsMap::const_iterator it = statContextsMap.cbegin();
         it != statContextsMap.cend();
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

int TablePrinter::start(BSLA_UNUSED bsl::ostream& errorDescription)
{
    // Setup the print of stats if configured for it
    if (!isEnabled()) {
        return 0;  // RETURN
    }

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

void TablePrinter::stop()
{
    // Stop the log cleaner
    d_statLogCleaner.stop();
}

int TablePrinter::printStats(bsl::ostream&         stream,
                             int                   statsId,
                             const bdlt::Datetime& datetime)
{
    // This must execute in the 'snapshot' thread

    stream << "===== ===== ===== ===== ===== ===== ===== ===== ===== =====\n"
           << "Stats id: " << statsId << " @ " << datetime << "\n"
           << "===== ===== ===== ===== ===== ===== ===== ===== ===== =====\n";

    // DOMAINQUEUES
    stream << "\n"
           << ":::::::::: :::::::::: DOMAINQUEUES >>";
    Context* context = d_contexts["domainQueues"].get();
    context->d_table.records().update();
    bmqst::TableUtil::printTable(stream, context->d_tip);

    // CLIENTS
    stream << "\n"
           << ":::::::::: :::::::::: CLIENTS >>";
    context = d_contexts["clients"].get();
    context->d_table.records().update();
    bmqst::TableUtil::printTable(stream, context->d_tip);

    // CLUSTERS
    stream << "\n"
           << ":::::::::: :::::::::: CLUSTERS >>";
    context = d_contexts["clusterNodes"].get();
    context->d_table.records().update();
    bmqst::TableUtil::printTable(stream, context->d_tip);

    // CHANNELS
    stream << "\n"
           << ":::::::::: :::::::::: TCP CHANNELS >>";
    context = d_contexts["channels"].get();
    context->d_table.records().update();
    bmqst::TableUtil::printTable(stream, context->d_tip);

    // ALLOCATORS
    stream << "\n"
           << ":::::::::: :::::::::: ALLOCATORS >>";
    ContextsMap::iterator it = d_contexts.find("allocators");
    if (it == d_contexts.end()) {
        // When using test allocator, we don't have a stat context
        stream << " Unavailable\n";
        return 0;  // RETURN
    }
    // NOTE: By contract, snapshot needs to be invoked on the statcontexts
    //       prior to this method.
    if (d_lastAllocatorSnapshot != 0) {
        stream << " Last snapshot was "
               << bmqu::PrintUtil::prettyTimeInterval(
                      bsls::TimeUtil::getTimer() - d_lastAllocatorSnapshot)
               << " ago.";
    }
    d_lastAllocatorSnapshot = bsls::TimeUtil::getTimer();

    context = it->second.get();
    context->d_table.records().update();
    bmqst::TableUtil::printTable(stream, context->d_tip);

    return 0;
}

void TablePrinter::logStats(int lastStatId)
{
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
    attributes.setSeverity(ball::Severity::e_INFO);

    // Dump stats into bmqbrkr.stats.log
    attributes.clearMessage();
    printStats(attributes.messageStream(), lastStatId, now);

    d_statsLogFile.publish(
        record,
        ball::Context(ball::Transmission::e_MANUAL_PUBLISH, 0, 1));
}

}  // close package namespace
}  // close enterprise namespace
