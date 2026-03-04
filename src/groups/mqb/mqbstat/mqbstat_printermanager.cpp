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
#include <mqbstat_printermanager.h>

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
// class PrinterManager
// -------------

PrinterManager::PrinterManager(const mqbcfg::StatsConfig& brkrCfg,
                               bdlmt::EventScheduler*     eventScheduler,
                               const StatContextsMap&     statContextsMap,
                               bslma::Allocator*          allocator)
: d_config(brkrCfg)
, d_lastStatId(0)
, d_actionCounter(0)
, d_statContextsMap(statContextsMap, allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(eventScheduler->clockType() ==
                     bsls::SystemClockType::e_MONOTONIC);

    // Create text printer
    d_printer_mp.load(
        new (*allocator)
            TablePrinter(brkrCfg, eventScheduler, statContextsMap, allocator),
        allocator);

    // Create json printer
    d_jsonPrinter_mp.load(
        new (*allocator)
            JsonPrinter(brkrCfg, eventScheduler, statContextsMap, allocator),
        allocator);
}

int PrinterManager::start(bsl::ostream& errorDescription)
{
    // Setup the print of stats if configured for it
    if (!isEnabled()) {
        return 0;  // RETURN
    }

    // Start the table printer
    {
        int rc = d_printer_mp->start(errorDescription);
        if (rc != 0) {
            return rc;  // RETURN
        }
    }

    // Start the json printer
    {
        int rc = d_jsonPrinter_mp->start(errorDescription);
        if (rc != 0) {
            return rc;  // RETURN
        }
    }

    return 0;
}

void PrinterManager::stop()
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

    // Stop the log cleaners
    d_printer_mp->stop();
    d_jsonPrinter_mp->stop();
}

int PrinterManager::printTableStats(bsl::ostream&         stream,
                                    int                   statsId,
                                    const bdlt::Datetime& datetime)
{
    return d_printer_mp->printStats(stream, statsId, datetime);
}

int PrinterManager::printJsonStats(bsl::ostream&         stream,
                                   bool                  compact,
                                   int                   statsId,
                                   const bdlt::Datetime& datetime,
                                   bool                  needValidJson)
{
    return d_jsonPrinter_mp->printStats(stream,
                                        compact,
                                        statsId,
                                        datetime,
                                        needValidJson);
}

void PrinterManager::logStats()
{
    ++d_lastStatId;

    d_printer_mp->logStats(d_lastStatId);
    d_jsonPrinter_mp->logStats(d_lastStatId);
}

void PrinterManager::onSnapshot()
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
