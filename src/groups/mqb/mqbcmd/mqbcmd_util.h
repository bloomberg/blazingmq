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

// mqbcmd_util.h                                                      -*-C++-*-
#ifndef INCLUDED_MQBCMD_UTIL
#define INCLUDED_MQBCMD_UTIL

/// @file mqbcmd_util.h
///
/// @brief Provide a namespace of command utilities.
///
/// This component provides a namespace, @bbref{mqbcmd::Util}, containing
/// utility functions for commands.

// MQB
#include <mqbcmd_messages.h>

// BDE
#include <bsl_iostream.h>

namespace BloombergLP {
namespace mqbcmd {

// ===========
// struct Util
// ===========

/// This `struct` provides a namespace command utilities.
struct Util {
    /// Load into the specified `result` a flat result obtained from the
    /// specified `cmdResult` (hierarchial).  That is, convert a command
    /// result from it's internal type representation, to the public one.
    static void flatten(Result* result, const InternalResult& cmdResult);

    /// Print the given `responseList` to the given output stream, `os`, using
    /// the specified `encoding`.
    static void
    printCommandResponses(const mqbcmd::RouteResponseList&    responseList,
                          const mqbcmd::EncodingFormat::Value encoding,
                          bsl::ostream&                       os);

    /// Print the given `result` to the given output stream, `os`, using the
    /// specified `encoding`.
    static void printCommandResult(const mqbcmd::InternalResult& result,
                                   mqbcmd::EncodingFormat::Value encoding,
                                   bsl::ostream&                 os);
};

}  // close package namespace
}  // close enterprise namespace

#endif
