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
#ifndef INCLUDED_MQBSTAT_PRINTER
#define INCLUDED_MQBSTAT_PRINTER

//@PURPOSE: Provide a mechanism to print statistics to streams
//
//@CLASSES:
//  mqbstat::TablePrinter: bmqbrkr statistics printer
//
//@DESCRIPTION: 'mqbstat::TablePrinter' handles the printing of all the
// statistics.
// It holds the tables and table info providers which can be printed.

// MQB
#include <mqbcfg_messages.h>

#include <bmqst_basictableinfoprovider.h>
#include <bmqst_statcontext.h>
#include <bmqst_table.h>
#include <bmqtsk_logcleaner.h>
#include <mqbstat_jsonprinter.h>

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
// class TablePrinter
// =============

class TablePrinter {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBSTAT.PRINTER");

  private:
    // PRIVATE TYPES

    /// Context including table and tip for printing and statcontext for
    /// stats.
    struct Context {
        /// Stat Context pointer
        bmqst::StatContext* d_statContext_p;

        /// Table
        bmqst::Table d_table;

        /// tip
        bmqst::BasicTableInfoProvider d_tip;
    };

    typedef bsl::shared_ptr<Context>                   ContextSp;
    typedef bsl::unordered_map<bsl::string, ContextSp> ContextsMap;
    typedef bsl::unordered_map<bsl::string, bmqst::StatContext*>
        StatContextsMap;

  private:
    // DATA

    /// Config to use.
    const mqbcfg::StatsConfig& d_config;

    /// FileObserver for the stats log dump.
    ball::FileObserver2 d_statsLogFile;

    /// HiRes timer value of the last time
    /// the Counting Allocators snapshot
    /// happened on the context.
    bsls::Types::Int64 d_lastAllocatorSnapshot;

    /// Contexts map
    ContextsMap d_contexts;

    /// Mechanism to clean up old stat logs.
    bmqtsk::LogCleaner d_statLogCleaner;

  private:
    // NOT IMPLEMENTED
    TablePrinter(const TablePrinter& other) BSLS_CPP11_DELETED;
    TablePrinter& operator=(const TablePrinter& other) BSLS_CPP11_DELETED;

    /// Initialize table and tips.
    void initializeTablesAndTips();

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TablePrinter, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `TablePrinter` object, using the specified `config`,
    /// `eventScheduler`, `statContextsMap` and the specified `allocator`
    /// for memory allocation.
    explicit TablePrinter(const mqbcfg::StatsConfig& config,
                          bdlmt::EventScheduler*     eventScheduler,
                          const StatContextsMap&     statContextsMap,
                          bslma::Allocator*          allocator);

    // MANIPULATORS

    /// Start the Printer.  Return 0 on success, or a non-zero return
    /// code on error and fill in the specified `errorDescription` stream
    /// with the description of the error.
    int start(bsl::ostream& errorDescription);

    /// Stop the printer.
    void stop();

    /// Print the stats to the specified `stream`.
    ///
    /// THREAD: This method is called in the `snapshot` thread.
    int printStats(bsl::ostream&         stream,
                   int                   statsId,
                   const bdlt::Datetime& datetime);

    /// Dump the stats to the stat log file.
    void logStats(int lastStatId);

    // ACCESSORS

    /// Returns true if printing is enabled, false otherwise.
    bool isEnabled() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------
// class TablePrinter
// -------------

// ACCESSORS

inline bool TablePrinter::isEnabled() const
{
    return d_config.printer().encoding() &
           mqbcfg::StatsPrinterEncodingFormat::TABLE;
}

}  // close package namespace
}  // close enterprise namespace

#endif
