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

// mqbcmd_parseutil.h                                                 -*-C++-*-
#ifndef INCLUDED_MQBCMD_PARSEUTIL
#define INCLUDED_MQBCMD_PARSEUTIL

//@PURPOSE: Provide a namespace of command parsing functions.
//
//@CLASSES:
//  ParseUtil: namespace of command parsing functions
//
//@DESCRIPTION:
// This component provides a namespace, 'mqbcmd::ParseUtil', containing a
// utility function for parsing "trap-like" broker commands.  The resulting
// object is an instance of a class generated from 'mqbcmd.xsd'.
//
/// Command Grammar
///---------------
// The structure of the parsed command is defined by 'mqbcmd.xsd'.  The input
// language, however, does not have a formal grammar.  Its structure is defined
// by this parser component.  An informal description of the language is
// returned by the broker in response to the "HELP" command.
//
/// Usage Example
///-------------
// This section illustrates intended use of this component.
//
/// Example 1: Parsing a Command in a "Trap" Handler
///  - - - - - - - - - - - - - - - - - - - - - - - -
// Suppose that a line of text is read by a command-line like interface and is
// then to be interpreted as a command (e.g. in an "m-trap handler").  The text
// can be parsed into a 'mqbcmd::Command' object using
// 'mqbcmd::ParseUtil::parse':
//..
// void handleTrap(const bslstl::StringRef& line)
// {
//     mqbcmd::Command command;
//     bsl::string     error;
//
//     if (mqbcmd::ParseUtil::parse(&command, &error, line)) {
//         BALL_LOG_ERROR << "Unable to parse command string: " << line
//                        << " due to the error: " << error;
//         return;                                                    // RETURN
//     }
//
//     // The command has been successfully parsed.
//     dispatch(command);
// }
//..

// MQB

// BDE
#include <bsl_string.h>

namespace BloombergLP {
namespace mqbcmd {

// FORWARD DECLARATIONS
class Command;

// ================
// struct ParseUtil
// ================

/// This `struct` provides a namespace for functions used to parse
/// `mqbcmd_messages` types from their "trap" style format, e.g. "CLUSTERS
/// LIST".
struct ParseUtil {
    /// Load into the specified `command` a command parsed from the
    /// specified `input`.  Return zero on success or a nonzero value if an
    /// error occurs.  If an error occurs, load a diagnostic message into
    /// the specified `error`.  In the input starts by a `{`, it is
    /// attempted to be decoded as JSON string; otherwise, it falls back to
    /// the command grammar (refer to the corresponding section in the
    /// component level documentation).
    static int parse(Command*                 command,
                     bsl::string*             error,
                     const bslstl::StringRef& input);
};

}  // close package namespace
}  // close enterprise namespace

#endif
