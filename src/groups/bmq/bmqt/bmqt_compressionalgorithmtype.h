// Copyright 2019-2023 Bloomberg Finance L.P.
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

// bmqt_compressionalgorithmtype.h                                    -*-C++-*-
#ifndef INCLUDED_BMQT_COMPRESSIONALGORITHMTYPE
#define INCLUDED_BMQT_COMPRESSIONALGORITHMTYPE

//@PURPOSE: Provide an enumeration for different compression algorithm types.
//
//@CLASSES:
//  bmqt::CompressionAlgorithmType: Type of compression algorithm
//
//@DESCRIPTION: Provide an enumeration, 'bmqt::CompressionAlgorithmType' for
// different types of compression algorithm.
//
//: o !NONE!: No compression algorithm was specified
//: o !ZLIB!: The compression algorithm is ZLIB

// BMQ

// BDE
#include <bsl_ostream.h>
#include <bsl_string.h>

namespace BloombergLP {
namespace bmqt {

// ===============================
// struct CompressionAlgorithmType
// ===============================

/// This struct defines various types of compression algorithms.
struct CompressionAlgorithmType {
    // TYPES
    enum Enum { e_UNKNOWN = -1, e_NONE = 0, e_ZLIB = 1 };

    // CONSTANTS

    /// NOTE: This value must always be equal to the lowest type in the
    /// enum because it is being used as a lower bound to verify that a
    /// header's `CompressionAlgorithmType` field is a supported type.
    static const int k_LOWEST_SUPPORTED_TYPE = e_NONE;

    /// NOTE: This value must always be equal to the highest type in the
    /// enum because it is being used as an upper bound to verify that a
    /// header's `CompressionAlgorithmType` field is a supported type.
    static const int k_HIGHEST_SUPPORTED_TYPE = e_ZLIB;

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
    /// `CompressionAlgorithmType::Enum` value.
    static bsl::ostream& print(bsl::ostream&                  stream,
                               CompressionAlgorithmType::Enum value,
                               int                            level = 0,
                               int spacesPerLevel                   = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << CompressionAlgorithmType::toAscii(
    ///                             CompressionAlgorithmType::e_NONE);
    /// ```
    /// will print the following on standard output:
    /// ```
    /// NONE
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(CompressionAlgorithmType::Enum value);

    /// Update the specified `out` with correct enum corresponding to the
    /// specified string `str`, if it exists, and return true.  Otherwise in
    /// case of an error or unidentified string, return false.  The expected
    /// `str` is the enumerator name with the "e_" prefix excluded.
    /// For example:
    /// ```
    /// CompressionAlgorithmType::fromAscii(out, NONE);
    /// ```
    ///  will return true and the value of `out` will be:
    /// ```
    /// bmqt::CompresssionAlgorithmType::e_NONE
    /// ```
    /// Note that specifying a `str` that does not match any of the
    /// enumerators excluding "e_" prefix will result in the function
    /// returning false and the specified `out` will not be touched.
    static bool fromAscii(CompressionAlgorithmType::Enum* out,
                          const bsl::string&              str);

    /// Return true incase of valid specified `str` i.e. a enumerator name
    /// with the "e_" prefix excluded.  Otherwise in case of invalid `str`
    /// return false and populate the specified `stream` with error message.
    static bool isValid(const bsl::string* str, bsl::ostream& stream);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                  stream,
                         CompressionAlgorithmType::Enum value);

}  // close package namespace

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ===============================
// struct CompressionAlgorithmType
// ===============================

// FREE OPERATORS
inline bsl::ostream&
bmqt::operator<<(bsl::ostream&                        stream,
                 bmqt::CompressionAlgorithmType::Enum value)
{
    return bmqt::CompressionAlgorithmType::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
