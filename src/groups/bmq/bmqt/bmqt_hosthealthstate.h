// Copyright 2021-2023 Bloomberg Finance L.P.
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

// bmqt_hosthealthstate.h                                             -*-C++-*-
#ifndef INCLUDED_BMQT_HOSTHEALTHSTATE
#define INCLUDED_BMQT_HOSTHEALTHSTATE

/// @file bmqt_hosthealthstate.h
///
/// @brief Provide an enumeration for different host health states.
///
/// Provide an enumeration, @bbref{bmqt::HostHealthState} for different
/// representations of the health of a host.
///
///   - *UNKNOWN*: Host health could not be ascertained
///   - *HEALTHY*: Host is considered to be healthy
///   - *UNHEALTHY*: Host is not considered to be healthy

// BMQ

// BDE
#include <bsl_iosfwd.h>
#include <bsl_string.h>

namespace BloombergLP {
namespace bmqt {

// ======================
// struct HostHealthState
// ======================

/// Enumeration for host health states.
struct HostHealthState {
    // TYPES
    enum Enum { e_UNKNOWN = 0, e_HEALTHY = 1, e_UNHEALTHY = 2 };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// `HostHealthState::Value` value.
    static bsl::ostream& print(bsl::ostream&         stream,
                               HostHealthState::Enum value,
                               int                   level          = 0,
                               int                   spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `BMQT_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(HostHealthState::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(HostHealthState::Enum*   out,
                          const bslstl::StringRef& str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, HostHealthState::Enum value);

}  // close package namespace

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------------
// struct HostHealthState
// ----------------------

// FREE OPERATORS
inline bsl::ostream& bmqt::operator<<(bsl::ostream&               stream,
                                      bmqt::HostHealthState::Enum value)
{
    return HostHealthState::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
