// Copyright 2024 Bloomberg Finance L.P.
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

// mqbstat_jsonprinter.h                                              -*-C++-*-
#ifndef INCLUDED_MQBSTAT_JSONPRINTER
#define INCLUDED_MQBSTAT_JSONPRINTER

//@PURPOSE: Provide a mechanism to print statistics as a JSON
//
//@CLASSES:
//  mqbstat::JsonPrinter: statistics printer to JSON
//
//@DESCRIPTION: 'mqbstat::JsonPrinter' handles the printing of the statistics
// as a compact or pretty JSON.  It is responsible solely for printing, so any
// statistics updates (e.g. making a new snapshot of the used StatContexts)
// must be done before calling to this component.

// MQB
#include <mqbcfg_messages.h>

// BMQ
#include <bmqst_statcontext.h>
#include <bmqtsk_logcleaner.h>

// BDE
#include <ball_fileobserver2.h>
#include <ball_log.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>

namespace BloombergLP {

namespace mqbstat {

// =================
// class JsonPrinter
// =================

class JsonPrinter {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBSTAT.JSONPRINTER");

  public:
    // PUBLIC TYPES
    typedef bsl::unordered_map<bsl::string, bmqst::StatContext*>
        StatContextsMap;

  private:
    // PRIVATE TYPES
    /// Forward declaration of the printer implementation type.
    class JsonPrinterImpl;

    // DATA
    /// Managed pointer to the printer implementation.
    bslma::ManagedPtr<JsonPrinterImpl> d_impl_mp;

    /// Config to use.
    const mqbcfg::StatsConfig& d_config;

    /// Log file pattern
    bsl::string d_logfile_pattern;

    /// Contexts map
    const StatContextsMap& d_statContextsMap;

    /// FileObserver for the stats log dump.
    ball::FileObserver2 d_statsLogFile;

    /// Mechanism to clean up old stat logs.
    bmqtsk::LogCleaner d_statLogCleaner;

  private:
    // NOT IMPLEMENTED
    JsonPrinter(const JsonPrinter& other) BSLS_CPP11_DELETED;
    JsonPrinter& operator=(const JsonPrinter& other) BSLS_CPP11_DELETED;

  public:
    // CREATORS

    /// Create a new `JsonPrinter` object, using the specified `config`,
    /// `eventScheduler`, `statContextsMap` and the specified `allocator`
    /// for memory allocation.

    explicit JsonPrinter(const mqbcfg::StatsConfig& config,
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

    // ACCESSORS

    /// Print the JSON-encoded stats to the specified `out`.
    /// If the specified `compact` flag is `true`, the JSON is printed in
    /// compact form, otherwise the JSON is printed in pretty form.
    /// Return `0` on success, and non-zero return code on failure.
    ///
    /// THREAD: This method is called in the *StatController scheduler* thread.
    int printStats(bsl::ostream&         os,
                   bool                  compact,
                   int                   statsId,
                   const bdlt::Datetime& datetime,
                   bool                  needValidJson = false);

    /// Dump the stats to the stat log file.
    void logStats(int lastStatId);

    /// Returns true if printing is enabled, false otherwise.
    bool isEnabled() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------
// class JsonPrinter
// -------------

inline bool JsonPrinter::isEnabled() const
{
    return d_config.printer().encoding() &
           mqbcfg::StatsPrinterEncodingFormat::Value::JSON;
}

}  // close package namespace
}  // close enterprise namespace

#endif
