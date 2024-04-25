// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqt_messageeventtype.h                                            -*-C++-*-
#ifndef INCLUDED_BMQT_MESSAGEEVENTTYPE
#define INCLUDED_BMQT_MESSAGEEVENTTYPE

/// @file bmqt_messageeventtype.h
///
/// @brief Provide an enumeration for the different types of message events.
///
/// Provide an enumeration, @bbref{bmqt::MessageEventType}, for the different
/// types of message events.
///
///   - *UNDEFINED*: Unknown message type
///   - *ACK*:       Message is an ack
///   - *PUSH*:      Message is a push
///   - *PUT*:       Message is a put

// BMQ

// BDE
#include <bsl_iosfwd.h>
#include <bsl_string.h>

namespace BloombergLP {
namespace bmqt {

// =======================
// struct MessageEventType
// =======================

/// Enumeration for the types of message events
struct MessageEventType {
    // TYPES
    enum Enum { e_UNDEFINED = 0, e_PUT = 1, e_PUSH = 2, e_ACK = 3 };

    // PUBLIC CONSTANTS

    /// NOTE: This value must always be equal to the lowest type in the
    /// enum because it is being used as a lower bound to verify that an
    /// MessageEventType field is a supported type.
    static const int k_LOWEST_SUPPORTED_EVENT_TYPE = e_PUT;

    /// NOTE: This value must always be equal to the highest type in the
    /// enum because it is being used as an upper bound to verify an
    /// MessageEventType field is a supported type.
    static const int k_HIGHEST_SUPPORTED_EVENT_TYPE = e_ACK;

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
    /// `MessageEventType::Value` value.
    static bsl::ostream& print(bsl::ostream&          stream,
                               MessageEventType::Enum value,
                               int                    level          = 0,
                               int                    spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `BMQT_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(MessageEventType::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(MessageEventType::Enum*  out,
                          const bslstl::StringRef& str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, MessageEventType::Enum value);

}  // close package namespace

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------
// struct MessageEventType
// -----------------------

// FREE OPERATORS
inline bsl::ostream& bmqt::operator<<(bsl::ostream&                stream,
                                      bmqt::MessageEventType::Enum value)
{
    return MessageEventType::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
