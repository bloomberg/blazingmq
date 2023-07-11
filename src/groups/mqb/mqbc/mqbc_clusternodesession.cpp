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

// mqbc_clusternodesession.cpp                                        -*-C++-*-
#include <mqbc_clusternodesession.h>

#include <mqbscm_version.h>
// MWC
#include <mwcio_status.h>

// BDE
#include <bsl_algorithm.h>
#include <bsl_iostream.h>

namespace BloombergLP {
namespace mqbc {

// ------------------------
// class ClusterNodeSession
// ------------------------

// CREATORS
ClusterNodeSession::ClusterNodeSession(
    mqbi::DispatcherClient*             cluster,
    mqbnet::ClusterNode*                netNode,
    const bsl::string&                  clusterName,
    const bmqp_ctrlmsg::ClientIdentity& identity,
    bslma::Allocator*                   allocator)
: d_cluster_p(cluster)
, d_clusterNode_p(netNode)
, d_peerInstanceId(0)  // There is no invalid value for this field
, d_queueHandleRequesterContext_sp(
      new(*allocator) mqbi::QueueHandleRequesterContext(allocator),
      allocator)
, d_nodeStatus(bmqp_ctrlmsg::NodeStatus::E_UNAVAILABLE)  // See note in ctor
, d_statContext_sp()
, d_primaryPartitions(allocator)
, d_queueHandles(allocator)
{
    // Note regarding 'd_nodeStatus': it must be initialized with E_UNAVAILABLE
    // because this value indicates that self node is not connected to this
    // peer, and some startup logic depends on this value being E_UNAVAILABLE.

    d_queueHandleRequesterContext_sp->setClient(this)
        .setIdentity(identity)
        .setDescription(description())
        .setIsClusterMember(true)
        .setRequesterId(
            mqbi::QueueHandleRequesterContext ::generateUniqueRequesterId());
    // TBD: The passed in 'queueHandleRequesterIdentity' is currently the
    //      'clusterState->identity()' (representing the identity of self node
    //      in the cluster); and it should instead represent the identity of
    //      the requester, (i.e., node->negotiationMessage().identity()) but
    //      this information is currently not available.

    BALL_LOG_INFO << clusterName << ": created cluster node ["
                  << netNode->nodeDescription() << "], ptr [" << this
                  << "], queueHandleRequesterId: "
                  << d_queueHandleRequesterContext_sp->requesterId();
}

ClusterNodeSession::~ClusterNodeSession()
{
    // NOTHING
}

// MANIPULATORS
void ClusterNodeSession::teardown()
{
    // executed by the *DISPATCHER* thread

    // Release all queue handles that were associated with this session
    for (QueueHandleMapIter it = d_queueHandles.begin();
         it != d_queueHandles.end();
         ++it) {
        mqbi::QueueHandle* handle = it->second.d_handle_p;
        handle->drop();
    }

    // TBD: Synchronize on the dispatcher ?
}

void ClusterNodeSession::flush()
{
    d_cluster_p->flush();
}

void ClusterNodeSession::addPartitionRaw(int partitionId)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    BSLS_ASSERT_SAFE(d_primaryPartitions.end() ==
                     bsl::find(d_primaryPartitions.begin(),
                               d_primaryPartitions.end(),
                               partitionId));

    d_primaryPartitions.push_back(partitionId);
}

bool ClusterNodeSession::addPartitionSafe(int partitionId)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (d_primaryPartitions.end() != bsl::find(d_primaryPartitions.begin(),
                                               d_primaryPartitions.end(),
                                               partitionId)) {
        return false;  // RETURN
    }

    addPartitionRaw(partitionId);
    return true;
}

void ClusterNodeSession::removePartitionRaw(int partitionId)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    BSLS_ASSERT_SAFE(d_primaryPartitions.end() !=
                     bsl::find(d_primaryPartitions.begin(),
                               d_primaryPartitions.end(),
                               partitionId));

    d_primaryPartitions.erase(bsl::remove(d_primaryPartitions.begin(),
                                          d_primaryPartitions.end(),
                                          partitionId),
                              d_primaryPartitions.end());
}

bool ClusterNodeSession::removePartitionSafe(int partitionId)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (d_primaryPartitions.end() == bsl::find(d_primaryPartitions.begin(),
                                               d_primaryPartitions.end(),
                                               partitionId)) {
        return false;  // RETURN
    }

    removePartitionRaw(partitionId);
    return true;
}

bool ClusterNodeSession::isPrimaryForPartition(int partitionId) const
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    return d_primaryPartitions.end() != bsl::find(d_primaryPartitions.begin(),
                                                  d_primaryPartitions.end(),
                                                  partitionId);
}

void ClusterNodeSession::removeAllPartitions()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    d_primaryPartitions.clear();
}

}  // close package namespace
}  // close enterprise namespace
