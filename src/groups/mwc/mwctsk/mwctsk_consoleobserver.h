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

// mwctsk_consoleobserver.h                                           -*-C++-*-
#ifndef INCLUDED_MWCTSK_CONSOLEOBSERVER
#define INCLUDED_MWCTSK_CONSOLEOBSERVER

//@PURPOSE: Provide a customizable colorized console output for BALL logging.
//
//@CLASSES:
//  mwctsk::ConsoleObserver: BALL observer printing to stdout
//
//@DESCRIPTION: 'mwctsk::ConsoleObserver' is a concrete implementation of the
// 'ball::ObserverAdapter' protocol which outputs to stdout and can be
// configured to use Unix terminal colors based on categories or severity.
//
/// NOTICE
///------
//: o WARN records and ERROR records are printed with respectively the
//:   'bold-yellow' and 'bold-red' color.
//: o Logs are always printed using the UTC and not the local time.
//: o Colors are automatically disabled if stdout is *NOT* a TTY.
//
/// Colors
///------
// The 'ConsoleObserver' can be set to use the following colors: black, red,
// green, yellow, blue, magenta, cyan, white and gray.  The background can be
// left unset or set to the same available colors.  In addition, the font can
// be set to bold or not.
//
// The names of the colors follow this syntax: [bold-]<fgcolor>[-on-<bgcolor>].
// For example the following color names are valid:
//      green
//      bold-gray
//      black-on-green
//      bold-white-on-red
// Note that the color names must be in *lowercase*.
//
// Colored categories configuration can dynamically be added/removed.
//
/// Thread-safety
///-------------
// This object is *thread* *enabled*, meaning that two threads can safely call
// any methods on the *same* *instance* without external synchronization.
//
/// Usage Example
///-------------
// The following example illustrates how to typically instantiate and use a
// 'ConsoleObserver'.
//
// First, we create a 'ConsoleObserver' object and configure it with the
// severity threshold beyond which to print to stdout, and the format to use.
//..
//  mwctsk::ConsoleObserver consoleObserver(allocator);
//
//  consoleObserver.setSeverityThreshold(ball::Severity::INFO)
//                 .setLogFromat("%d (%t) %s %F:%l %m\n");
//..
//
// Then, we can configure some colors to use for specific categories (note that
// this step can be done now or at any point in the future):
//..
//  consoleObserver.setCategoryColor("MWC*", "green")
//                 .setCategoryColor("MWCTSK.APPLICATION", "red-on-yellow");
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
//      ballObserver->registerObserver(&consoleObserver);
//  }
//..
//
// Similarly, before destruction we must unregister the observer.
//..
//  ball::MultiplexObserver *ballObserver =
//      dynamic_cast<ball::MultiplexObserver*>(
//          ball::LoggerManager::singleton().observer());
//  if (ballObserver) {
//      ballObserver->deregisterObserver(&consoleObserver);
//  }
//..
//

// MWC

// BDE
#include <ball_log.h>
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

namespace BloombergLP {

namespace mwctsk {

// =====================
// class ConsoleObserver
// =====================

/// BALL observer printing to stdout
class ConsoleObserver : public ball::ObserverAdapter {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MWCTSK.CONSOLEOBSERVER");

  private:
    // PRIVATE TYPES

    /// Map of color name to Unix color code
    typedef bsl::unordered_map<bsl::string, bsl::string> ColorMap;

    /// Vector of pair<CategoryExpression, associatedColor>.  Note that the
    /// second member of the pair (the associated color) is a raw pointer to
    /// the corresponding color code from the `ColorMap` (in order to save
    /// some memory by not repeating the color code over and over).  Also
    /// note that this is a vector and can't be a map because the lookup
    /// will need to do a match of the category with the category expression
    /// (due to `*` character meaning).
    typedef bsl::vector<bsl::pair<bsl::string, const char*> > CategoryColorVec;

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

    ColorMap d_colorMap;
    // Statically initialized map of available
    // colors to their corresponding code.

    CategoryColorVec d_categoryColorVec;
    // Dynamically modifiable map of registered
    // categories to their associated color.

  private:
    // PRIVATE MANIPULATORS

    /// Initialize the `d_colorMap` member.
    void initializeColorMap();

    // PRIVATE ACCESSORS

    /// Return the color code to use for the specified `record`, or a null
    /// pointer if no specific color should be used.
    const char* getColorCodeForRecord(const ball::Record& record) const;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    ConsoleObserver(const ConsoleObserver&);             // = delete;
    ConsoleObserver& operator=(const ConsoleObserver&);  // = delete;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ConsoleObserver, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object, using the specified `allocator`.
    explicit ConsoleObserver(bslma::Allocator* allocator);

    /// Destroy this object.
    ~ConsoleObserver() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Register the specified `color` to be used for the specified
    /// `category`.  `category` can be an expression ending with the special
    /// character `*`, in which case it will apply to all matching
    /// categories.  Note that an empty `color` clears any coloring
    /// associated to this `category`; and configuring an already configured
    /// `category` will replace its `color` with the new one.  Return a
    /// reference to this object offering modification access.
    ConsoleObserver& setCategoryColor(const bslstl::StringRef& category,
                                      const bslstl::StringRef& color);

    /// Set the format of the logs to the specified `value`.  Refer to the
    /// documentation of `ball::RecordStringFormatter` for a description of
    /// the accepted format.  Return a reference to this object offering
    /// modification access.
    ConsoleObserver& setLogFormat(const bsl::string& value);

    /// Set the severity threshold to the specified `value`: any log which
    /// severity is greater than or equal to `value` will be printed to
    /// stdout.  Use `ball::Severity::OFF` to completely disable this
    /// observer.  Return a reference to this object offering modification
    /// access.
    ConsoleObserver& setSeverityThreshold(ball::Severity::Level value);

    // MANIPULATORS
    //   (virtual: ball::ObserverAdapter)

    /// Process the specified log `record` having the specified publishing
    /// `context`.  If the severity associated to this `record` is greater
    /// than or equal to the configured `severityThreshold`, the record is
    /// printed to stdout according to the configured `format`, in the color
    /// associated to its corresponding category, if any.  Note that
    /// regardless of the category, warning and error severity are printed
    /// in a special color.
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
// class ConsoleObserver
// ---------------------

inline ConsoleObserver& ConsoleObserver::setLogFormat(const bsl::string& value)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    d_formatter.setFormat(value.data());

    return *this;
}

inline ConsoleObserver&
ConsoleObserver::setSeverityThreshold(ball::Severity::Level value)
{
    d_severityThreshold = value;
    return *this;
}

inline ball::Severity::Level ConsoleObserver::severityThreshold() const
{
    return d_severityThreshold;
}

inline bool ConsoleObserver::isEnabled() const
{
    return d_severityThreshold != ball::Severity::OFF;
}

}  // close package namespace
}  // close enterprise namespace

#endif
