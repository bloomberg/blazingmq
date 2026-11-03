// Copyright 2026 Bloomberg Finance L.P.
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

// mqbraft_persistentstate.h -*-C++-*-
#ifndef INCLUDED_MQBRAFT_PERSISTENTSTATE
#define INCLUDED_MQBRAFT_PERSISTENTSTATE

//@PURPOSE: Provide the file holding a Raft group's `currentTerm` and vote.
//
//@CLASSES:
//  mqbraft::PersistentState:     VST for `currentTerm` and the vote
//  mqbraft::PersistentStateUtil: load and save a `PersistentState` file
//
//@DESCRIPTION: The Raft paper's "persistent state on all servers" (Figure 2)
// other than the log: `currentTerm` and `votedFor`.  A vote is stored with
// the term it was cast in, and counts only while that term is current.
//
// One file per Raft group.  `save` replaces the file atomically: it writes a
// temporary file, syncs it, renames it over the old one and syncs the
// directory.
//
/// Threading
///----------
// This component is NOT thread-safe.

// BDE
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbraft {

// ======================
// struct PersistentState
// ======================

/// VST for the state of a Raft group saved outside its log.
struct PersistentState {
    // DATA
    bsls::Types::Uint64 d_currentTerm;

    /// Term `d_votedFor` was cast in; 0 if no vote.
    bsls::Types::Uint64 d_voteTerm;

    int d_votedFor;

    // CREATORS

    /// Create an object with term 0 and no vote.
    PersistentState();
};

// FREE OPERATORS
bsl::ostream& operator<<(bsl::ostream& stream, const PersistentState& value);

// ==========================
// struct PersistentStateUtil
// ==========================

/// Load and save a `PersistentState` file.
struct PersistentStateUtil {
    // CLASS METHODS

    /// Load into the specified `state` the contents of the file at the
    /// specified `path`.  Return 0 on success, including when the file does
    /// not exist, in which case `state` is set to term 0 and no vote.
    /// Return non-zero, leaving `state` unchanged and writing the reason to
    /// the specified `errorDescription`, if the file cannot be read or fails
    /// its checks.
    static int load(PersistentState*   state,
                    bsl::ostream&      errorDescription,
                    const bsl::string& path);

    /// Replace the file at the specified `path` with the specified `state`,
    /// and sync it to disk.  Return 0 on success.  Return non-zero, writing
    /// the reason to the specified `errorDescription`, otherwise; the file
    /// then holds either the old or the new contents.
    static int save(bsl::ostream&          errorDescription,
                    const bsl::string&     path,
                    const PersistentState& state);
};

}  // close package namespace
}  // close enterprise namespace

#endif
