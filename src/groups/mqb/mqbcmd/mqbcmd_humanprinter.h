// Copyright 2020-2023 Bloomberg Finance L.P.
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

// mqbcmd_humanprinter.h                                              -*-C++-*-
#ifndef INCLUDED_MQBCMD_HUMANPRINTER
#define INCLUDED_MQBCMD_HUMANPRINTER

//@PURPOSE: Provide a namespace of utilities to human-friendly print results.
//
//@CLASSES:
//  HumanPrinter: Utilities to print results in a human-friendly way.
//
//@DESCRIPTION:
// This component provides a namespace, 'mqbcmd::HumanPrinter', containing
// utilities to print results in a human-friendly way.
//

// MQB

#include <mqbcmd_messages.h>

// BDE
#include <bsl_iostream.h>

namespace BloombergLP {
namespace mqbcmd {

// ===================
// struct HumanPrinter
// ===================

/// This `struct` provides a namespace of utilities to print results in a
/// human-friendly way.
struct HumanPrinter {
    /// Pretty-print the specified `result` to the specified `os` at the
    /// (absolute value of) the optionally specified indentation `level` and
    /// return a reference to `stream`.  If `level` is specified, optionally
    /// specify `spacesPerLevel`, the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    static bsl::ostream& print(bsl::ostream& os,
                               const Result& result,
                               int           level          = 0,
                               int           spacesPerLevel = 4);

    /// Pretty-print the specified `responses` to the specified `os` at the
    /// (absolute value of) the optionally specified indentation `level` and
    /// return a reference to `stream`.  If `level` is specified, optionally
    /// specify `spacesPerLevel`, the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    static bsl::ostream& printResponses(bsl::ostream&            os,
                                        const RouteResponseList& responseList);
};

}  // close package namespace
}  // close enterprise namespace

#endif
