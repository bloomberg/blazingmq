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

// mqbcfg_clusterquorummanager.cpp -*-C++-*-

// MQB
#include <mqbcfg_clusterquorummanager.h>

// BDE
#include <bslmt_lockguard.h>

namespace BloombergLP {
namespace mqbcfg {

// MANIPULATORS
void ClusterQuorumManager::setQuorum(unsigned int quorum,
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

void ClusterQuorumManager::setCallback(const UpdateQuorumCallback& callback)
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);
    d_callback = callback;
}

}  // close package namespace
}  // close enterprise namespace
