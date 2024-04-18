// Copyright 2016-2023 Bloomberg Finance L.P.
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

// bmqt_propertytype.h                                                -*-C++-*-
#ifndef INCLUDED_BMQT_PROPERTYTYPE
#define INCLUDED_BMQT_PROPERTYTYPE

/// @file bmqt_propertytype.h
///
/// @brief Provide enum for the supported data types for a message property.
///
/// This component contains @bbref{bmqt::PropertyType} which describes various
/// data types that are supported for message properties.
///
/// Data Types and Size                              {#bmqt_propertytype_types}
/// ===================
///
/// This section describes the size of each data type:
///
/// | Data Type                      | Size (in bytes)   |
/// | ------------------------------ | ----------------- |
/// | BOOL                           | 1                 |
/// | CHAR                           | 1                 |
/// | SHORT                          | 2                 |
/// | INT                            | 4                 |
/// | INT64                          | 8                 |
/// | STRING                         | variable          |
/// | BINARY                         | variable          |
///
/// Note that the difference between `BINARY` and `STRING` data types is that
/// the former allows null (`'\0'`) character while the later does not.

// BMQ

// BDE
#include <bsl_iosfwd.h>
#include <bsl_string.h>

namespace BloombergLP {
namespace bmqt {

// ===================
// struct PropertyType
// ===================

/// This enum represents the supported data types for a message property.
struct PropertyType {
    // TYPES
    enum Enum {
        e_UNDEFINED = 0,
        e_BOOL      = 1,
        e_CHAR      = 2,
        e_SHORT     = 3,
        e_INT32     = 4,
        e_INT64     = 5,
        e_STRING    = 6,
        e_BINARY    = 7
    };

    // CONSTANTS

    /// NOTE: This value must always be equal to the lowest type in the
    /// enum because it is being used as a lower bound to verify that a
    /// Property's `type` field is a supported type.
    static const int k_LOWEST_SUPPORTED_PROPERTY_TYPE = e_BOOL;

    /// NOTE: This value must always be equal to the highest type in the
    /// enum because it is being used as an upper bound to verify a
    /// Property's `type` field is a supported type.
    static const int k_HIGHEST_SUPPORTED_PROPERTY_TYPE = e_BINARY;

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
    /// `PropertyType::Enum` value.
    static bsl::ostream& print(bsl::ostream&      stream,
                               PropertyType::Enum value,
                               int                level          = 0,
                               int                spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(PropertyType::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(PropertyType::Enum*      out,
                          const bslstl::StringRef& str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, PropertyType::Enum value);

}  // close package namespace

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------
// struct PropertyType
// -------------------

inline bsl::ostream& bmqt::operator<<(bsl::ostream&            stream,
                                      bmqt::PropertyType::Enum value)
{
    return bmqt::PropertyType::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
