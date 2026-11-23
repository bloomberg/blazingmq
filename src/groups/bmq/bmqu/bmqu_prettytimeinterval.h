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

#ifndef INCLUDED_BMQU_PRETTYTIMEINTERVAL
#define INCLUDED_BMQU_PRETTYTIMEINTERVAL

///@PURPOSE: Provide a stream manipulator to print a time interval.
///
///@CLASSES:
///  bmqu::PrettyTimeInterval: Manipulator printing a time interval
///
///@DESCRIPTION: 'bmqu::PrettyTimeInterval' writes a time interval, given in
/// nanoseconds, to a stream using the best unit (ns, us, ms, s, m, h, d, w).
///
/// Usage
///-----
///..
///  os << bmqu::PrettyTimeInterval(12432);     // 12.43 us
///  os << bmqu::PrettyTimeInterval(12432, 3);  // 12.432 us
///..

// BDE
#include <bsl_ostream.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bmqu {

// ========================
// class PrettyTimeInterval
// ========================

/// @brief Stream manipulator that writes a time interval in a readable form.
class PrettyTimeInterval {
  private:
    // DATA
    bsls::Types::Int64 d_timeNs;
    int                d_precision;

  public:
    // CREATORS

    /// @brief Create a manipulator for the specified time interval.
    ///
    /// @param timeNs The time interval, in nanoseconds.
    /// @param precision The number of digits after the decimal point, in
    ///                  the range `[0, 18]`.
    explicit PrettyTimeInterval(bsls::Types::Int64 timeNs, int precision = 2);

    // ACCESSORS

    /// @brief Return the time interval, in nanoseconds.
    bsls::Types::Int64 timeNs() const;

    /// @brief Return the number of digits after the decimal point.
    int precision() const;

    /// @brief Print this object to the specified `stream`, as per the
    ///        contract of the standard BDE print method.
    ///
    /// @param stream The destination output stream.
    /// @param level The indentation level.
    /// @param spacesPerLevel The number of spaces per indentation level.
    ///
    /// @return A reference to the modifiable `stream`.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// @brief Write the specified `value` to the specified `stream`, rounded to
///        the nearest value at its precision.
///
/// @param stream The destination output stream.
/// @param value The time interval to write.
///
/// @return A reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&             stream,
                         const PrettyTimeInterval& value);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------------
// class PrettyTimeInterval
// ------------------------

// CREATORS
inline PrettyTimeInterval::PrettyTimeInterval(bsls::Types::Int64 timeNs,
                                              int                precision)
: d_timeNs(timeNs)
, d_precision(precision)
{
    // NOTHING
}

// ACCESSORS
inline bsls::Types::Int64 PrettyTimeInterval::timeNs() const
{
    return d_timeNs;
}

inline int PrettyTimeInterval::precision() const
{
    return d_precision;
}

}  // close package namespace
}  // close enterprise namespace

#endif
