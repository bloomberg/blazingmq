// Copyright 2025 Bloomberg Finance L.P.
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

// mqbcfg_clusterquorummanager.h -*-C++-*-
#ifndef INCLUDED_MQBCFG_CLUSTERQUORUMMANAGER
#define INCLUDED_MQBCFG_CLUSTERQUORUMMANAGER

/// @file mqbcfg_clusterquorummanager.h
///
/// @brief Provide a thread safe mechanism to manage cluster quorum.
///
/// @bbref{mqbcfg::ClusterQuorumManager} is a thread safe class that
/// encapsulates the quorum value for a cluster, allowing it to be set and
/// retrieved safely across multiple threads.

// MQB
#include <mqbcfg_messages.h>

// BDE
#include <bsls_atomic.h>

namespace BloombergLP {
namespace mqbcfg {

// =========================
// class ClusterQuorumManager
// =========================

/// This class provides a mechanism to manage the quorum for a cluster.
class ClusterQuorumManager {
  private:
    // DATA
    bsls::AtomicUint d_quorum;

  private:
    // NOT IMPLEMENTED
    ClusterQuorumManager(const ClusterQuorumManager&) BSLS_KEYWORD_DELETED;
    ClusterQuorumManager&
    operator=(const ClusterQuorumManager&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `ClusterQuorumManager` object using the specified
    /// `clusterConfig`. If the quorum is not specified in the config,
    /// calculate it as the majority of nodes.
    explicit ClusterQuorumManager(int quorum, size_t nodeCount);

    // MANIPULATORS

    /// Set the quorum to the specified `value`.
    void setQuorum(int value);

    // ACCESSORS

    /// Return the current quorum value.
    size_t quorum() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------------
// class ClusterQuorumManager
// ------------------------

// CREATORS
inline ClusterQuorumManager::ClusterQuorumManager(int quorum, size_t nodeCount)
{
    if (0 == quorum) {
        d_quorum.store(static_cast<int>(nodeCount) / 2 + 1);
    }
    else {
        if (quorum > static_cast<int>(nodeCount)) {
            quorum = static_cast<int>(nodeCount);
        }
        else if (quorum < 1) {
            quorum = 0;
        }

        d_quorum.store(quorum);
    }
}

// MANIPULATORS
inline void ClusterQuorumManager::setQuorum(int quorum)
{
    if (quorum < 1) {
        quorum = 0;
    }

    d_quorum.store(quorum);
}

// ACCESSORS
inline size_t ClusterQuorumManager::quorum() const
{
    return d_quorum.load();
}

}  // close package namespace
}  // close enterprise namespace

#endif
