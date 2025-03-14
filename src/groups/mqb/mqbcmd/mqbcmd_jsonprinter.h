// Copyright 2020-2024 Bloomberg Finance L.P.
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

// mqbcmd_jsonprinter.h                                              -*-C++-*-
#ifndef INCLUDED_MQBCMD_JSONPRINTER
#define INCLUDED_MQBCMD_JSONPRINTER

/// @file mqbcmd_jsonprinter.h
///
/// @brief Provide a namespace of utilities to print results in JSON.
///
///
/// This component provides a namespace, @bbref{mqbcmd::JsonPrinter},
/// containing utilities to print results in JSON.

// MQB
#include <mqbcmd_messages.h>

// BDE
#include <bsl_iostream.h>

namespace BloombergLP {
namespace mqbcmd {

// ==================
// struct JsonPrinter
// ==================

/// This `struct` provides a namespace of utilities to print results in JSON.
struct JsonPrinter {
    /// Print the specified `result` to the specified `os` at the
    /// (absolute value of) the optionally specified indentation `level` and
    /// return a reference to `stream`.  If `level` is specified, optionally
    /// specify `spacesPerLevel`, the number of spaces per indentation level
    /// for this and all of its nested objects.  If the optionally specified
    /// 'pretty' flag is true, print JSON in a human-friendly format, if it is
    /// false, print the json in a compact format.
    static bsl::ostream& print(bsl::ostream& os,
                               const Result& result,
                               bool          pretty         = true,
                               int           level          = 0,
                               int           spacesPerLevel = 4);

    /// Print the specified `responses` to the specified `os` at the
    /// (absolute value of) the optionally specified indentation `level` and
    /// return a reference to `stream`.  If `level` is specified, optionally
    /// specify `spacesPerLevel`, the number of spaces per indentation level
    /// for this and all of its nested objects.  If the optionally specified
    /// 'pretty' flag is true, print JSON in a human-friendly format, if it is
    /// false, print the json in a compact format.
    static bsl::ostream& printResponses(bsl::ostream&            os,
                                        const RouteResponseList& responseList,
                                        bool                     pretty = true,
                                        int                      level  = 0,
                                        int spacesPerLevel              = 4);
};

}  // close package namespace
}  // close enterprise namespace

#endif
