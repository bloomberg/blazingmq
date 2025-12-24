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

#include <bmqio_status.h>

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
    mqbi::DispatcherClient*                    cluster,
    mqbnet::ClusterNode*                       netNode,
    const bsl::string&                         clusterName,
    const bmqp_ctrlmsg::ClientIdentity&        identity,
    const bsl::shared_ptr<bmqst::StatContext>& statContext,
    bslma::Allocator*                          allocator)
: d_cluster_p(cluster)
, d_clusterNode_p(netNode)
, d_peerInstanceId(0)  // There is no invalid value for this field
, d_queueHandleRequesterContext_sp(
      new(*allocator) mqbi::QueueHandleRequesterContext(allocator),
      allocator)
, d_nodeStatus(bmqp_ctrlmsg::NodeStatus::E_UNAVAILABLE)  // See note in ctor
, d_statContext_sp(statContext)
, d_primaryPartitions(allocator)
, d_queueHandles(allocator)
, d_gatePush()
, d_gateAck()
, d_gatePut()
, d_gateConfirm()
{
    // Note regarding 'd_nodeStatus': it must be initialized with E_UNAVAILABLE
    // because this value indicates that self node is not connected to this
    // peer, and some startup logic depends on this value being E_UNAVAILABLE.

    d_queueHandleRequesterContext_sp->setClient(this)
        .setIdentity(identity)
        .setDescription(description())
        .setIsClusterMember(true)
        .setRequesterId(
            mqbi::QueueHandleRequesterContext ::generateUniqueRequesterId())
        .setInlineClient(this)
        .setStatContext(statContext);
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
    QueueHandleMapIter qit = d_queueHandles.begin();

    while (qit != d_queueHandles.end()) {
        mqbi::QueueHandle* handle_p = qit->second.d_handle_p;
        BSLS_ASSERT_SAFE(handle_p);

        handle_p->drop();
        qit = d_queueHandles.erase(qit);
    }

    d_gatePush.close();
    d_gateAck.close();
    d_gatePut.close();
    d_gateConfirm.close();

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
    BSLS_ASSERT_SAFE(inDispatcherThread());

    BSLS_ASSERT_SAFE(d_primaryPartitions.end() ==
                     bsl::find(d_primaryPartitions.begin(),
                               d_primaryPartitions.end(),
                               partitionId));

    d_primaryPartitions.push_back(partitionId);

    BALL_LOG_INFO << d_clusterNode_p->nodeDescription() << " Partition ["
                  << partitionId
                  << "]: added self as primary in ClusterNodeSession";
}

bool ClusterNodeSession::addPartitionSafe(int partitionId)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(inDispatcherThread());

    if (d_primaryPartitions.end() != bsl::find(d_primaryPartitions.begin(),
                                               d_primaryPartitions.end(),
                                               partitionId)) {
        return false;  // RETURN
    }

    addPartitionRaw(partitionId);

    return true;
}

bool ClusterNodeSession::removePartitionSafe(int partitionId)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(inDispatcherThread());

    bsl::vector<int>::iterator it = bsl::find(d_primaryPartitions.begin(),
                                              d_primaryPartitions.end(),
                                              partitionId);
    if (it == d_primaryPartitions.end()) {
        return false;  // RETURN
    }

    d_primaryPartitions.erase(it);

    BALL_LOG_INFO << d_clusterNode_p->nodeDescription() << " Partition ["
                  << partitionId
                  << "]: removed self as primary in ClusterNodeSession";

    return true;
}

bool ClusterNodeSession::isPrimaryForPartition(int partitionId) const
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(inDispatcherThread());

    return d_primaryPartitions.end() != bsl::find(d_primaryPartitions.begin(),
                                                  d_primaryPartitions.end(),
                                                  partitionId);
}

void ClusterNodeSession::removeAllPartitions()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(inDispatcherThread());

    d_primaryPartitions.clear();
}

mqbi::InlineResult::Enum ClusterNodeSession::sendPush(
    const bmqt::MessageGUID&                  msgGUID,
    int                                       queueId,
    const bsl::shared_ptr<bdlbb::Blob>&       message,
    const mqbi::StorageMessageAttributes&     attributes,
    const bmqp::MessagePropertiesInfo&        mps,
    const bmqp::Protocol::SubQueueInfosArray& subQueueInfos)
{
    // executed by the *QUEUE DISPATCHER* thread
    // This PUSH message is enqueued by mqbblp::Queue/QueueHandle on this node,
    // and needs to be forwarded to 'event.clusterNode()' (the replica node,
    // which is the client).  Note that replica is already expected to have the
    // payload, and so, primary (this node) sends only the guid and, if
    // applicable, the associated subQueueIds.

    GateKeeper::Status status(d_gatePush);

    if (!status.isOpen()) {
        // Target node (or self) is not AVAILABLE, so we don't send this PUSH
        // to it. Note that this PUSH msg was dispatched by the queue handle
        // representing the target node, and will be in its 'pending list'.

        return mqbi::InlineResult::e_UNAVAILABLE;  // RETURN
    }

    bmqt::GenericResult::Enum rc = bmqt::GenericResult::e_SUCCESS;
    // TBD: groupId: also pass options to the 'PushEventBuilder::packMessage'
    // routine below.

    if (message) {
        // If it's at most once, then we explicitly send the payload since it's
        // in-mem mode and there's been no replication (i.e. no preceding
        // STORAGE message).
        rc = clusterNode()->channel().writePush(
            message,
            queueId,
            msgGUID,
            0,
            attributes.compressionAlgorithmType(),
            mps,
            subQueueInfos);
    }
    else {
        rc = clusterNode()->channel().writePush(
            queueId,
            msgGUID,
            0,
            attributes.compressionAlgorithmType(),
            mps,
            subQueueInfos);
    }

    return rc == bmqt::GenericResult::e_SUCCESS
               ? mqbi::InlineResult::e_SUCCESS
               : mqbi::InlineResult::e_CHANNEL_ERROR;
}

mqbi::InlineResult::Enum
ClusterNodeSession::sendAck(int queueId, const bmqp::AckMessage& ackMessage)
{
    // executed by the *QUEUE DISPATCHER* thread

    // This ACK message is enqueued by mqbblp::Queue on this node, and needs to
    // be forwarded to 'clusterNode()' (the replica node).

    GateKeeper::Status status(d_gateAck);

    if (!status.isOpen()) {
        // Drop the ACK because downstream node (or self) is either starting,
        // or shut down.

        return mqbi::InlineResult::e_UNAVAILABLE;  // RETURN
    }

    bmqt::GenericResult::Enum rc = clusterNode()->channel().writeAck(
        ackMessage.status(),
        ackMessage.correlationId(),
        ackMessage.messageGUID(),
        queueId);

    return rc == bmqt::GenericResult::e_SUCCESS
               ? mqbi::InlineResult::e_SUCCESS
               : mqbi::InlineResult::e_CHANNEL_ERROR;
}

}  // close package namespace
}  // close enterprise namespace
