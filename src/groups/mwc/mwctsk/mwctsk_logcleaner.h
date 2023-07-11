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

// mwctsk_logcleaner.h                                                -*-C++-*-
#ifndef INCLUDED_MWCTSK_LOGCLEANER
#define INCLUDED_MWCTSK_LOGCLEANER

//@PURPOSE: Provide a mechanism to periodically clean up old logs.
//
//@CLASSES:
//  mwctsk::LogCleaner: Mechanism to periodically clean up old logs
//
//@DESCRIPTION: 'mwctsk::LogCleaner' is a mechanism to manage periodic
// cleaning of old log files.
//
/// Threading
///---------
// All IO operations (listing log files, deleting old files) are performed on a
// separate thread; the provided scheduler is only used to schedule recurring
// events.
//
/// Thread safety
///-------------
// Not thread-safe.
//
/// Usage Example
///-------------
// The following example illustrates how to typically instantiate and use a
// 'LogCleaner'.
//
// First, we create a 'LogCleaner' object, and pass it an event scheduler.
//..
//  mwctsk::LogCleaner logCleaner(&scheduler);
//..
//
// We can then start it, but first, we use the 'ball::LogFileCleanerUtil' to
// convert a 'ball' log pattern to a file pattern, and create a
// 'bsls::TimeInterval' representing the age of the oldest files to keep:
//..
//  // Convert log pattern to file pattern
//  bsl::string filePattern;
//  ball::LogFileCleanerUtil::logPatternToFilePattern(&filePatterm,
//                                                    config.logPattern());
//
//  // Configure the maxAge:
//  bsls::TimeInterval maxAge(0, 0);
//  maxAge.addDays(config.fileMaxAgeDays());
//
//  // Start the log cleaner: this will initiate a cleanup right away, and
//  // schedule daily recurring cleaning:
//  rc = logCleaner.start(filePattern, maxAge);
//  if (rc != 0) {
//      BALL_LOG_ERROR << "Failed to start logCleaner for '"
//                     << filePattern << "'";
//  }
//..
//
// Finally, before shutdown, we must stop the LogCleaner:
//..
//  logCleaner.stop();
//..

// MWC

#include <mwcex_strand.h>
#include <mwcex_systemexecutor.h>

// BDE
#include <bdlmt_eventscheduler.h>
#include <bdlt_timeunitratio.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {

namespace mwctsk {

// ================
// class LogCleaner
// ================

/// Mechanism to periodically clean up old logs.
class LogCleaner {
  private:
    // PRIVATE DATA

    // Used to schedule periodic logs cleaning events. Note that the log
    // cleaning is not performed in the scheduler thread, but rather in the
    // context of a separate thread, as this operation is quite heavy.
    bdlmt::EventScheduler* d_scheduler_p;

    bdlmt::EventScheduler::RecurringEventHandle d_logsCleaningEvent;
    // Recurring scheduler event handle for periodic log cleaning.

    // This strand is used to perform logs cleaning in the context of a
    // separate thread.
    mwcex::Strand<mwcex::SystemExecutor> d_logsCleaningContext;

    // Pattern of the files to cleanup.
    bsl::string d_filePattern;

    // Age of the maximum files to keep.
    bsls::TimeInterval d_maxAge;

  private:
    // PRIVATE MANIPULATORS

    /// Delete files matching `d_filePattern`, which are older than
    /// `d_maxAge`.
    void cleanLogs();

  private:
    // NOT IMPLEMENTED
    LogCleaner(const LogCleaner&) BSLS_KEYWORD_DELETED;
    LogCleaner& operator=(const LogCleaner&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `LogCleaner` object that uses the specified `scheduler` to
    /// schedule log cleaning operations. Optionally specify a `allocator`
    /// used to supply memory. If `allocator` is 0, the default memory
    /// allocator is used.
    explicit LogCleaner(bdlmt::EventScheduler* scheduler,
                        bslma::Allocator*      allocator = 0);

    /// Destroy this object. Call `stop()`.
    ~LogCleaner();

  public:
    // MANIPULATORS

    /// Initiate the log cleaning of files matching the specified `pattern`
    /// which are older than the specified `maxAge` every `cleanupPeriod`
    /// interval. Return 0 on success, and a non-zero value otherwise. If
    /// the log cleaner is already started, this function has no effects
    /// and 0 is returned.
    int start(bslstl::StringRef         pattern,
              bsls::TimeInterval        maxAge,
              const bsls::TimeInterval& cleanupPeriod =
                  bsls::TimeInterval(bdlt::TimeUnitRatio::k_S_PER_D));

    /// Stop cleaning logs. Synchronize with any ongoing log cleaning
    /// operation. If the log cleaner is already stopped, this function has
    /// no effects.
    void stop();

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(LogCleaner, bslma::UsesBslmaAllocator)
};

}  // close package namespace
}  // close enterprise namespace

#endif
