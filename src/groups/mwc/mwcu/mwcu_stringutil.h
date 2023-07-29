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

// mwcu_stringutil.h                                                  -*-C++-*-
#ifndef INCLUDED_MWCU_STRINGUTIL
#define INCLUDED_MWCU_STRINGUTIL

//@PURPOSE: Provide utility functions for string manipulation.
//
//@CLASSES:
//  mwcu::StringUtil: namespace for string manipulation utility functions.
//
//@DESCRIPTION: 'mwcu::StringUtil' provides a utility namespace for string
// manipulation functions.

// BDE
#include <bsl_cstddef.h>
#include <bsl_string.h>
#include <bsl_vector.h>

namespace BloombergLP {
namespace mwcu {

// =================
// struct StringUtil
// =================

/// Utility namespace for string manipulation.
struct StringUtil {
    // CLASS METHODS

    /// Return `true` if the specified string `str` contains the specified
    /// substring `substr`, return false otherwise.
    static bool contains(const bsl::string&       str,
                         const bslstl::StringRef& substr);

    /// Return `true` if the specified string `str` contains the specified
    /// `prefix` substring starting from the optionally specified `offset`,
    /// return `false` otherwise.
    static bool startsWith(const bslstl::StringRef& str,
                           const bslstl::StringRef& prefix,
                           size_t                   offset = 0);

    /// Return `true` if the specified string `str` ends with the specified
    /// `suffix` string, return `false` otherwise.
    static bool endsWith(const bslstl::StringRef& str,
                         const bslstl::StringRef& suffix);

    /// Perform an in-place white spaces trimming at the beginning and the
    /// end of the specified string `str` and return a reference offering
    /// modifiable access to it.
    static bsl::string& trim(bsl::string* str);

    /// Perform an in-place white spaces trimming at the beginning of the
    /// specified string `str` and return a reference offering modifiable
    /// access to it.
    static bsl::string& ltrim(bsl::string* str);

    /// Perform an in-place white spaces trimming at the end of the
    /// specified string `str` and return a reference offering modifiable
    /// access to it.
    static bsl::string& rtrim(bsl::string* str);

    /// Split the specified string `str` in an array of StringRef using the
    /// specified `delims` as item separator.  Empty tokens will be
    /// represented by the empty string "" in the resulting vector.  Note
    /// that the returning StringRefs are reference to the char in the
    /// provided `str` and therefore are valid only within the scope of
    /// `str` validity.
    static bsl::vector<bslstl::StringRef>
    strTokenizeRef(const bsl::string& str, const bslstl::StringRef& delims);

    /// Return true if the specified string `str` matches the specified
    /// `pattern`.  `pattern` may include Unix file matching wildcards `*`
    /// and `?`, representing respectively 0 to n characters and exactly 1
    /// character.
    static bool match(const bslstl::StringRef& str,
                      const bslstl::StringRef& pattern);

    /// Remove from the specified `str` all contiguous repeats of a
    /// character, where the character is equal to one of the specified
    /// `characters`, and return a reference providing modifiable access to
    /// `*str`.  For example,
    /// ```
    /// bsl::string str;
    /// str = "hello   there spaces";
    /// assert(squeeze(&str, " ") == "hello there spaces");
    /// str = "mississippi";
    /// assert(squeeze(&str, "ps") == "misisipi");
    /// str = "wakka";
    /// assert(squeeze(&str, "") == "wakka");
    /// ```
    static bsl::string& squeeze(bsl::string*             str,
                                const bslstl::StringRef& characters);
};

}  // close package namespace
}  // close enterprise namespace

#endif
