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

#ifndef INCLUDED_MQBSTAT_STATSFILELOGGER
#define INCLUDED_MQBSTAT_STATSFILELOGGER

//@PURPOSE: Provide a mechanism to log statistics to a file.
//
//@CLASSES:
//  mqbstat::StatsFileLogger: statistics file logger
//
//@DESCRIPTION: 'mqbstat::StatsFileLogger' handles writing formatted statistics
// to a log file with support for file rotation and cleanup of old logs.

// MQB
#include <bmqtsk_logcleaner.h>

// BDE
#include <ball_fileobserver2.h>
#include <ball_log.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_functional.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>

namespace BloombergLP {

namespace mqbstat {

// ====================
// class StatsFileLogger
// ====================

class StatsFileLogger {
  public:
    // PUBLIC TYPES
    typedef bsl::function<void(bsl::ostream&)> PrinterCb;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBSTAT.STATSFILELOGGER");

    // DATA

    /// Log file pattern.
    bsl::string d_filePattern;

    /// FileObserver for the stats log dump.
    ball::FileObserver2 d_statsLogFile;

    /// Mechanism to clean up old stat logs.
    bmqtsk::LogCleaner d_statLogCleaner;

    // NOT IMPLEMENTED
    StatsFileLogger(const StatsFileLogger& other) BSLS_KEYWORD_DELETED;
    StatsFileLogger&
    operator=(const StatsFileLogger& other) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StatsFileLogger, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `StatsFileLogger` object, using the specified
    /// `filePattern` for the log file path, the specified
    /// `eventScheduler` and the specified `allocator` for memory allocation.
    explicit StatsFileLogger(bsl::string_view       filePattern,
                             bdlmt::EventScheduler* eventScheduler,
                             bslma::Allocator*      allocator);

    // MANIPULATORS

    /// Start the StatsFileLogger.
    void start();

    /// Stop the logger.
    void stop();

    /// Dump the stats to the stat log file, using the specified `printerCb`
    /// to format the output.
    void logStats(const PrinterCb& printerCb);
};

}  // close package namespace
}  // close enterprise namespace

#endif
