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

// mqbc_clustermembership.cpp                                         -*-C++-*-
#include <mqbc_clustermembership.h>

#include <mqbscm_version.h>
// MQB
#include <mqbc_clusternodesession.h>
#include <mqbnet_cluster.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bsl_map.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsla_annotations.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbc {

// -------------------------------
// class ClusterMembershipObserver
// -------------------------------

// CREATORS
ClusterMembershipObserver::~ClusterMembershipObserver()
{
    // NOTHING
}

void ClusterMembershipObserver::onSelfNodeStatus(
    BSLA_UNUSED bmqp_ctrlmsg::NodeStatus::Value value)
{
    // NOTHING
}

// -----------------------
// class ClusterMembership
// -----------------------

// MANIPULATORS
ClusterMembership&
ClusterMembership::registerObserver(ClusterMembershipObserver* observer)
{
    d_observers.insert(observer);
    return *this;
}

ClusterMembership&
ClusterMembership::unregisterObserver(ClusterMembershipObserver* observer)
{
    d_observers.erase(observer);
    return *this;
}

ClusterMembership&
ClusterMembership::setSelfNodeStatus(bmqp_ctrlmsg::NodeStatus::Value value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(selfNodeSession());

    mqbc::ClusterNodeSession* selfSession = selfNodeSession();

    bmqp_ctrlmsg::NodeStatus::Value oldVal = selfSession->nodeStatus();

    selfSession->setNodeStatus(value, value);

    for (ClusterNodeSessionMapConstIter cit = clusterNodeSessionMap().begin();
         cit != clusterNodeSessionMap().end();
         ++cit) {
        mqbc::ClusterNodeSession* session = cit->second.get();
        if (session != selfSession) {
            session->setNodeStatus(session->nodeStatus(), value);
        }
    }

    if (oldVal == value) {
        return *this;  // RETURN
    }

    // Self status changed; notify observers.
    for (ObserversSet::iterator it = d_observers.begin();
         it != d_observers.end();
         ++it) {
        (*it)->onSelfNodeStatus(value);
    }

    return *this;
}

}  // close package namespace
}  // close enterprise namespace
