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

// mqbcmd_commandlist.h                                               -*-C++-*-
#ifndef INCLUDED_MQBCMD_COMMANDLIST
#define INCLUDED_MQBCMD_COMMANDLIST

/// @file mqbcmd_commandlist.h
///
/// @brief Provide a list of all commands the broker can respond to.
///
/// This component provides a namespace, @bbref{mqbcmd::CommandList},
/// containing all of the broker commands and their description.

namespace BloombergLP {
namespace mqbcmd {

// FORWARD DECLARATION
class Help;

// ==================
// struct CommandList
// ==================

/// This `struct` provides a namespace of utilities to retrieve all commands
/// the broker recognizes.
struct CommandList {
  public:
    // PUBLIC CLASS METHODS

    /// Load into the specified `out` the broker commands and their
    /// description.  If the specified `isPlumbing` is true, print the
    /// output in a machine friendly format.
    static void loadCommands(mqbcmd::Help* out, const bool isPlumbing);
};

}  // close package namespace
}  // close enterprise namespace

#endif
