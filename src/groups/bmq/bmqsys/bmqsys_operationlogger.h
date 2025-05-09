// Copyright 2025 Bloomberg Finance L.P.
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

// bmqsys_operationlogger.h                                           -*-C++-*-
#ifndef INCLUDED_BMQSYS_OPERATIONLOGGER
#define INCLUDED_BMQSYS_OPERATIONLOGGER

//@PURPOSE: Provide utility to measure and log operation time.
//
//@CLASSES:
// bmqsys::OperationLogger
//
//@DESCRIPTION:
// This component provides a set of utility functions and stream manipulators
// for writing numbers, time intervals, and bytes amounts in a human-readable
// format.
// The 'pretty*' functions exist in two styles:
//: o Procedural: The argument list consists of the destination output stream,
//:   followed by the value to write, possibly followed by formatting
//:   parameters (group size, group separator, etc). The function writes the
//:   value to the stream and returns void.
//: o Manipulator: The argument list consists of the value to write, possibly
//:   followed by formatting parameters (group size, group separator, etc).
//:   The function returns an object which, when inserted into an output
//:   stream, writes the value to the stream. This is similar to the standard
//:   library stream manipulators ('std::setw', 'std::setprecision', etc).
//
// The '*indent' functions exist only as manipulators that invoke the
// corresponding functions in the 'bdlb_print' component.
//
// All the functions provided by this component have sensible default values
// for the formatting parameters. Numbers are written following the US American
// convention (i.e. group digits by thousands, using the comma as separator).
// Bytes amounts and time intervals are written with two decimals.
//
/// Usage
///-----
// Write values using the procedural interface:
//..
//  // os is a bsl::ostream                  output:
//  prettyNumber(os, 1234567, 2, '.'); // 1.23.45.67
//  prettyNumber(os, 1234567);         // 1,234,567
//  prettyBytes(os, 1025);             // 1.00 KB
//  prettyTimeInterval(os, 12432);     // 12.43 us
//..
//
// Write the same values using manipulators:
//..
//  os << prettyNumber(1234567, 2, '.'); // 1.23.45.67
//  os << prettyNumber(1234567);         // 1,234,567
//  os << prettyBytes(1025);             // 1.00 KB
//  os << prettyTimeInterval(12432);     // 12.43 us
//..

// BDE
#include <ball_log.h>
#include <bsl_ostream.h>
#include <bslma_allocator.h>
#include <bslmf_nestedtraitdeclaration.h>

// BMQ
#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

namespace BloombergLP {
namespace bmqsys {

// =====================
// class OperationLogger
// =====================

/// Printer for arbitrary types, with output operator specialized for some
/// types without an output operator
class OperationLogger {
  private:
    // PRIVATE DATA

    /// High resolution time of the current operation start.
    bsls::Types::Int64 d_beginTimestamp;

    /// Stream holding current operation description.
    bmqu::MemOutStream d_currentOpDescription;

    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQSYS.OPERATIONLOGGER");

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(OperationLogger, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create this object using the optionally specified `allocator`.
    explicit OperationLogger(bslma::Allocator* allocator = 0);

    // Destroy this object.
    ~OperationLogger();

    // MANIPULATORS
    bsl::ostream& start();

    void stop();

    /// The same as `stop()`, defined to be used with
    /// `bdlcc::ObjectPoolFunctors::Clear<OperationLogger>`.
    void clear();
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class OperationLogger
// ---------------------

inline bsl::ostream& OperationLogger::start()
{
    if (d_beginTimestamp != 0 || !d_currentOpDescription.isEmpty()) {
        BALL_LOG_WARN << "Trying to start OperationLogger with non-stopped "
                      << "operation: d_beginTimestamp = " << d_beginTimestamp
                      << ", d_currentOpDescription = \""
                      << d_currentOpDescription.str() << "\"";
        d_currentOpDescription.reset();
    }

    d_beginTimestamp = bmqsys::Time::highResolutionTimer();
    return d_currentOpDescription;
}

inline void OperationLogger::stop()
{
    if (d_beginTimestamp == 0) {
        BALL_LOG_WARN << "Calling `OperationLogger::stop` multiple times";
        return;  // RETURN
    }
    if (d_currentOpDescription.isEmpty()) {
        BALL_LOG_WARN << "Empty operation description in OperationLogger: "
                      << "operation was not set on `start` call.";
        return;  // RETURN
    }

    BALL_LOG_INFO_BLOCK
    {
        const bsls::Types::Int64 elapsed =
            bmqsys::Time::highResolutionTimer() - d_beginTimestamp;
        BALL_LOG_OUTPUT_STREAM
            << d_currentOpDescription.str()
            << " took: " << bmqu::PrintUtil::prettyTimeInterval(elapsed)
            << " (" << elapsed << " nanoseconds)";
    }

    d_beginTimestamp = 0;
    d_currentOpDescription.reset();
}

inline void OperationLogger::clear()
{
    stop();
}

}  // close package namespace
}  // close enterprise namespace

#endif
