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
///
/// Thread Safety
/// =============
///
/// This component is thread safe.

// BDE
#include <bsl_functional.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace mqbcfg {

// =========================
// class ClusterQuorumManager
// =========================

/// This class provides a mechanism to manage the quorum for a cluster.
class ClusterQuorumManager {
  public:
    // TYPES

    /// Invoked on every quorum update
    typedef bsl::function<void(const unsigned int)> UpdateQuorumCallback;

  private:
    // DATA
    bsls::AtomicUint d_quorum;

    /// Mutex for the callback
    bslmt::Mutex d_mutex;

    /// Callback to be called after every quorum change.
    UpdateQuorumCallback d_callback;

  private:
    // NOT IMPLEMENTED
    ClusterQuorumManager(const ClusterQuorumManager&) BSLS_KEYWORD_DELETED;
    ClusterQuorumManager&
    operator=(const ClusterQuorumManager&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `ClusterQuorumManager` object using the specified `quorum`
    /// and `nodeCount`.
    explicit ClusterQuorumManager(unsigned int quorum, unsigned int nodeCount);
    // MANIPULATORS

    /// Set the quorum to the specified value. If the `quorum` is 0,
    /// it is calculated as the majority of `nodeCount`.
    void setQuorum(unsigned int quorum, unsigned int nodeCount);

    /// Set the quorum change callback
    void setCallback(const UpdateQuorumCallback& callback);

    // ACCESSORS

    /// Return the current quorum value.
    unsigned int quorum() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------------
// class ClusterQuorumManager
// ------------------------

// CREATORS
inline ClusterQuorumManager::ClusterQuorumManager(unsigned int quorum,
                                                  unsigned int nodeCount)
: d_mutex()
, d_callback(0)
{
    setQuorum(quorum, nodeCount);
}

// MANIPULATORS
inline void ClusterQuorumManager::setQuorum(unsigned int quorum,
                                            unsigned int nodeCount)
{
    if (0 == quorum) {
        quorum = nodeCount / 2 + 1;
    }

    // It is permissible for 'quorum' to be greater than 'nodeCount'. This
    // is useful in testing scenarios to prevent a leader from being
    // elected.

    d_quorum.store(quorum);

    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);
    if (d_callback != 0) {
        d_callback(quorum);
    }
}

inline void
ClusterQuorumManager::setCallback(const UpdateQuorumCallback& callback)
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);
    d_callback = callback;
}

// ACCESSORS
inline unsigned int ClusterQuorumManager::quorum() const
{
    unsigned int quorum = d_quorum.load();
    BSLS_ASSERT_SAFE(0 < quorum);
    return quorum;
}

}  // close package namespace
}  // close enterprise namespace

#endif
