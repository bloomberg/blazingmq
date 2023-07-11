// Copyright 2022-2023 Bloomberg Finance L.P.
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

// mwctsk_syslogobserver.h                                            -*-C++-*-
#ifndef INCLUDED_MWCTSK_SYSLOGOBSERVER
#define INCLUDED_MWCTSK_SYSLOGOBSERVER

//@PURPOSE: Provide a system log output for BALL logging.
//
//@CLASSES:
//  mwctsk::SyslogObserver: BALL observer printing to a system log
//
//@DESCRIPTION: 'mwctsk::SyslogObserver' is a concrete implementation of the
// 'ball::ObserverAdapter' protocol which outputs to a system log.
//
/// Thread-safety
///-------------
// This object is *thread* *enabled*, meaning that two threads can safely call
// any methods on the *same* *instance* without external synchronization.
//
/// Usage Example
///-------------
// The following example illustrates how to typically instantiate and use a
// 'SyslogObserver'.
//
// First, we create a 'SyslogObserver' object and configure it with the
// severity threshold beyond which to print to a system log, and the format to
// use.
//..
//  mwctsk::SyslogObserver syslogObserver(allocator);
//
//  syslogObserver.setSeverityThreshold(ball::Severity::INFO)
//                .setLogFromat("%d (%t) %s %F:%l %m\n");
//  syslogObserver.enableLogging();
//..
//
// Finally, we need to register this Observer to the BALL logging
// infrastructure.  Note that the following code assumes the
// 'ball::LoggerManager' singleton was initialized with a
// 'ball::MultiplexObserver'.
//..
//  ball::MultiplexObserver *ballObserver =
//      dynamic_cast<ball::MultiplexObserver*>(
//          ball::LoggerManager::singleton().observer());
//  if (!ballObserver) {
//      BALL_LOG_ERROR << "Unable to register consoleObserver [reason: "
//                     << "'ball::LoggerManager::observer()' is NOT a "
//                     << "multiplexObserver]";
//  } else {
//      ballObserver->registerObserver(&syslogObserver);
//  }
//..
//
// Similarly, before destruction we must unregister the observer.
//..
//  ball::MultiplexObserver *ballObserver =
//      dynamic_cast<ball::MultiplexObserver*>(
//          ball::LoggerManager::singleton().observer());
//  if (ballObserver) {
//      ballObserver->deregisterObserver(&syslogObserver);
//  }
//..
//

// MWC

// BDE
#include <ball_observeradapter.h>
#include <ball_recordstringformatter.h>
#include <ball_severity.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bsls_cpp11.h>

namespace BloombergLP {

namespace mwctsk {

// =====================
// class SyslogObserver
// =====================

/// BALL observer printing to system log
class SyslogObserver : public ball::ObserverAdapter {
  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use.

    mutable bslmt::Mutex d_mutex;
    // Mutex for synchronizing usage of this
    // object.

    ball::RecordStringFormatter d_formatter;
    // Formatter for record printing.

    ball::Severity::Level d_severityThreshold;
    // Severity threshold beyond which log records
    // should be printed.

  private:
    // NOT IMPLEMENTED
    SyslogObserver(const SyslogObserver&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    SyslogObserver& operator=(const SyslogObserver&) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(SyslogObserver, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object, using the specified `allocator`.
    explicit SyslogObserver(bslma::Allocator* allocator);

    /// Destroy this object.
    ~SyslogObserver() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Enable logging of all records published to this observer to the
    /// syslog for the specified `appName`.
    void enableLogging(const bsl::string& appName);

    /// Disable file logging for this file observer.
    void disableLogging();

    /// Set the format of the logs to the specified `value`.  Refer to the
    /// documentation of `ball::RecordStringFormatter` for a description of
    /// the accepted format.  Return a reference to this object offering
    /// modification access.
    SyslogObserver& setLogFormat(const bsl::string& value);

    /// Set the severity threshold to the specified `value`: any log which
    /// severity is greater than or equal to `value` will be printed to
    /// stdout.  Use `ball::Severity::OFF` to completely disable this
    /// observer.  Return a reference to this object offering modification
    /// access.
    SyslogObserver& setSeverityThreshold(ball::Severity::Level value);

    // MANIPULATORS
    //   (virtual: ball::ObserverAdapter)

    /// Process the specified log `record` having the specified publishing
    /// `context`.  If the severity associated to this `record` is greater
    /// than or equal to the configured `severityThreshold`, the record is
    /// printed to stdout according to the configured `format`, in the color
    /// associated to its corresponding category, if any
    void publish(const ball::Record&  record,
                 const ball::Context& context) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the currently set severity threshold of this object.
    ball::Severity::Level severityThreshold() const;

    /// Return whether this observer is enabled or not.  Note that this is
    /// just a convenience shortcut for comparing the `severityThreshold`
    /// with a value of `ball::Severity::OFF`.
    bool isEnabled() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class SyslogObserver
// ---------------------

inline SyslogObserver& SyslogObserver::setLogFormat(const bsl::string& value)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    d_formatter.setFormat(value.data());

    return *this;
}

inline SyslogObserver&
SyslogObserver::setSeverityThreshold(ball::Severity::Level value)
{
    d_severityThreshold = value;
    return *this;
}

inline ball::Severity::Level SyslogObserver::severityThreshold() const
{
    return d_severityThreshold;
}

inline bool SyslogObserver::isEnabled() const
{
    return d_severityThreshold != ball::Severity::OFF;
}

}  // close package namespace
}  // close enterprise namespace

#endif
