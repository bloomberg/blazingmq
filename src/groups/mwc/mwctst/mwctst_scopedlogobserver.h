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

// mwctst_scopedlogobserver.h                                         -*-C++-*-
#ifndef INCLUDED_MWCTST_SCOPEDLOGOBSERVER
#define INCLUDED_MWCTST_SCOPEDLOGOBSERVER

//@PURPOSE: provide a scoped implementation of the log observer protocol.
//
//@CLASSES:
//  mwctst::ScopedLogObserver:     a scoped log observer for testing purposes
//  mwctst::ScopedLogObserverUtil: utilities to check log records
//
//@DESCRIPTION: This component defines a mechanism,
// 'mwctst::ScopedLogObserver', providing an RAII-like implementation of the
// BALL observer protocol, automatically registering/unregistering itself with
// the installed singleton logger manager instance and keeping track of log
// records.  It additionally provides utilities,
// 'mwctst::ScopedLogObserverUtil', to simplify checking that log records
// satisfy certain criteria.
//
/// Thread Safety
///-------------
// NOT Thread-safe.
//
/// Usage
///-----
// This section illustrates intended usage of this component.
//
/// Example 1: Basic Usage
///  - - - - - - - - - - -
// The following snippets of code illustrate the basic usage of the scoped log
// observer object for testing purposes.
//
// First, create a 'mwctst::ScopedLogObserver' object having a desired severity
// threshold for capturing log records (in this case, 'ball::Severity::ERROR').
//..
//  BALL_LOG_SET_CATEGORY("TEST");
//  mwctst::ScopedLogObserver observer(ball::Severity::ERROR, s_allocator_p);
//..
// Next, publish a log using the BALL logging infrastructure.  Note that the
// severity threshold should be at or above that of the observer for the log
// record to be captured.
//..
//  BALL_LOG_ERROR << "MySampleError";
//..
// Finally, retrieve the log records captured by the observer and leverage the
// associated utilities to verify that the expected log record was emitted.
//..
//  BSLS_ASSERT(observer.records().size() == 1U);
//  BSLS_ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
//                                                       observer.records()[0],
//                                                       ".*Sample.*",
//                                                       s_allocator_p));
//..

// MWC

// BDE
#include <ball_observeradapter.h>
#include <ball_record.h>
#include <ball_severity.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace ball {
class Context;
}

namespace mwctst {

// =======================
// class ScopedLogObserver
// =======================

/// This mechanism implements a scoped implementation of the log observer
/// protocol used for testing purposes.
class ScopedLogObserver : public ball::ObserverAdapter {
  private:
    // PRIVATE DATA

    // Severity threshold beyond which log records should be captured.
    ball::Severity::Level d_severityThreshold;

    // Unique name of this log observer used to registered in the logger
    // manager.
    bsl::string d_observerName;

    // Records captured by this observer.
    bsl::vector<ball::Record> d_records;

  private:
    // NOT IMPLEMENTED
    ScopedLogObserver(const ScopedLogObserver&) BSLS_KEYWORD_DELETED;
    ScopedLogObserver&
    operator=(const ScopedLogObserver&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `ScopedLogObserver` object capturing log records published
    /// with at least the specified `severity` and using the specified
    /// `allocator` to provide memory.  The behavior is undefined unless the
    /// BALL logger manager singleton is initialized, and stays initialized
    /// for the entire lifetime of this object.
    ScopedLogObserver(ball::Severity::Level severity,
                      bslma::Allocator*     allocator);

    /// Destroy this object.
    ~ScopedLogObserver() BSLS_KEYWORD_OVERRIDE;

  public:
    // MANIPULATORS

    /// Set the severity threshold to the specified `value`: any log which
    /// severity is greater than or equal to `value` will be recorded.  Use
    /// `ball::Severity::OFF` to completely disable this observer.  Return a
    /// reference to this object.
    ScopedLogObserver& setSeverityThreshold(ball::Severity::Level value);

    // ---------------------
    // ball::ObserverAdapter
    // ---------------------

    /// Process the specified log `record` having the specified publishing
    /// `context`.
    void publish(const ball::Record&  record,
                 const ball::Context& context) BSLS_KEYWORD_OVERRIDE;

  public:
    // ACCESSORS

    /// Return the currently set severity threshold of this object.
    ball::Severity::Level severityThreshold() const;

    /// Return whether this observer is enabled or not.  Note that this is
    /// just a convenience shortcut for comparing the `severityThreshold`
    /// with a value of `ball::Severity::OFF`.
    bool isEnabled() const;

    /// Return a reference not offering modifiable access to the list of
    /// records captured by this observer.
    const bsl::vector<ball::Record>& records() const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ScopedLogObserver,
                                   bslma::UsesBslmaAllocator)
};

// ============================
// struct ScopedLogObserverUtil
// ============================

/// This class provides utilities to check that log records specify certain
/// criteria (e.g. the log's message matches a regular expression).
struct ScopedLogObserverUtil {
    // PUBLIC CLASS METHODS

    /// Match the message contained in the specified `record` against the
    /// specified `pattern` and return true on success, false otherwise.
    /// The matching is performed with the flags `k_FLAG_CASELESS`,
    /// `k_FLAG_DOTMATCHESALL` (as described in `bdlpcre_regex.h`).  The
    /// behavior is undefined unless `pattern` represents a valid pattern.
    static bool recordMessageMatch(const ball::Record& record,
                                   const char*         pattern,
                                   bslma::Allocator*   allocator);
};

}  // close package namespace
}  // close enterprise namespace

#endif
