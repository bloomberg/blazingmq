// Copyright 2026 Bloomberg Finance L.P.
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

// mqbstat_printer.h                                                  -*-C++-*-
#ifndef INCLUDED_MQBSTAT_PRINTERMANAGER
#define INCLUDED_MQBSTAT_PRINTERMANAGER

//@PURPOSE: Provide a mechanism to print statistics to streams
//
//@CLASSES:
//  mqbstat::PrinterManager: bmqbrkr statistics printer manager
//
//@DESCRIPTION: 'mqbstat::PrinterManager' handles the printing of all the
// statistics.

// MQB
#include <mqbcfg_messages.h>
#include <mqbstat_jsonprinter.h>
#include <mqbstat_printer.h>

// BMQ
#include <bmqst_basictableinfoprovider.h>
#include <bmqst_statcontext.h>
#include <bmqst_table.h>
#include <bmqtsk_logcleaner.h>

// BDE
#include <ball_fileobserver2.h>
#include <ball_log.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_cpp11.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace mqbstat {

// =============
// class PrinterManager
// =============

class PrinterManager {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBSTAT.PRINTERMANAGER");

  private:
    // PRIVATE TYPES
    typedef bsl::unordered_map<bsl::string, bmqst::StatContext*>
                                            StatContextsMap;
    typedef bslma::ManagedPtr<TablePrinter> TablePrinterMp;
    typedef bslma::ManagedPtr<JsonPrinter>  JsonPrinterMp;

  private:
    // DATA

    /// Config to use.
    const mqbcfg::StatsConfig& d_config;

    /// Sequence number for stat log
    /// records, used to synchronize the
    /// stat log and the normal log.
    int d_lastStatId;

    /// Counter to know when to periodically
    /// print the stats to file.
    int d_actionCounter;

    /// Contexts map
    StatContextsMap d_statContextsMap;

    /// Console and log file stats printer as table
    TablePrinterMp d_printer_mp;

    /// Console and log file stats printer as json
    JsonPrinterMp d_jsonPrinter_mp;

  private:
    // NOT IMPLEMENTED
    PrinterManager(const PrinterManager& other) BSLS_CPP11_DELETED;
    PrinterManager& operator=(const PrinterManager& other) BSLS_CPP11_DELETED;

    /// Initialize table and tips.
    void initializeTablesAndTips();

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(PrinterManager, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `PrinterManager` object, using the specified `config`,
    /// `eventScheduler`, `statContextsMap` and the specified `allocator`
    /// for memory allocation.
    explicit PrinterManager(const mqbcfg::StatsConfig& config,
                            bdlmt::EventScheduler*     eventScheduler,
                            const StatContextsMap&     statContextsMap,
                            bslma::Allocator*          allocator);

    // MANIPULATORS

    /// Start the Printer.  Return 0 on success, or a non-zero return
    /// code on error and fill in the specified `errorDescription` stream
    /// with the description of the error.
    int start(bsl::ostream& errorDescription);

    /// Stop the printer manager.
    void stop();

    /// Print the stats to the specified `stream`.
    ///
    /// THREAD: This method is called in the `snapshot` thread.
    int printTableStats(bsl::ostream&         stream,
                        int                   statsId,
                        const bdlt::Datetime& datetime);

    /// Print the stats to the specified `stream`.
    ///
    /// THREAD: This method is called in the `snapshot` thread.
    int printJsonStats(bsl::ostream&         stream,
                       bool                  compact,
                       int                   statsId,
                       const bdlt::Datetime& datetime,
                       bool                  needValidJson);

    /// Dump the stats to the stat log file.
    void logStats();

    /// Print the stats to the stats log file at the appropriate time.
    void onSnapshot();

    // ACCESSORS

    /// Returns true if printing is enabled, false otherwise.
    bool isEnabled() const;

    /// Returns true if the next call to `onSnapshot` will perform the
    /// action (i.e., print).
    bool nextSnapshotWillPrint() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------
// class PrinterManager
// -------------

inline bool PrinterManager::isEnabled() const
{
    return (d_config.printer().printInterval() > 0 &&
            d_config.snapshotInterval() > 0);
}

inline bool PrinterManager::nextSnapshotWillPrint() const
{
    return d_actionCounter == 1;
}

}  // close package namespace
}  // close enterprise namespace

#endif
