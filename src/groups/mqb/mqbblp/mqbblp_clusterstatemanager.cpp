// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbblp_clusterstatemanager.cpp                                     -*-C++-*-
#include <mqbblp_clusterstatemanager.h>

#include <mqbscm_version.h>
// MQB
#include <mqbblp_storagemanager.h>
#include <mqbc_clusternodesession.h>
#include <mqbc_clusterstateledgeriterator.h>
#include <mqbi_cluster.h>
#include <mqbi_domain.h>
#include <mqbnet_cluster.h>
#include <mqbnet_elector.h>
#include <mqbs_datastore.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbu_exit.h>

// BMQ
#include <bmqt_uri.h>

#include <bmqio_status.h>
#include <bmqsys_time.h>
#include <bmqtsk_alarmlog.h>
#include <bmqu_printutil.h>

// BDE
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlt_timeunitratio.h>
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>  // size_t
#include <bsla_annotations.h>
#include <bsls_assert.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqbblp {
namespace {

// For convenience
const int k_KEY_LEN = mqbs::FileStoreProtocol::k_KEY_LENGTH;

}  // close unnamed namespace

// -------------------------
// class ClusterStateManager
// -------------------------

// PRIVATE MANIPULATORS
void ClusterStateManager::onCommit(
    const bmqp_ctrlmsg::ControlMessage&        advisory,
    mqbc::ClusterStateLedgerCommitStatus::Enum status)
{
    // executed by the cluster *DISPATCHER* thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(advisory.choice().isClusterMessageValue());

    if (status != mqbc::ClusterStateLedgerCommitStatus::e_SUCCESS) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << ": Failed to commit advisory: " << advisory
                       << ", with status '" << status << "'";
        return;  // RETURN
    }

    const bmqp_ctrlmsg::ClusterMessage& clusterMessage =
        advisory.choice().clusterMessage();

    // NOTE: Even when using old workflow, we still apply all advisories to the
    // CSL. We just don't invoke the commit callbacks.
    // Make an exception for QueueUpdateAdvisory, QueueAssignmentAdvisory, and
    // PartitionPrimaryAdvisory
    if (!d_clusterConfig.clusterAttributes().isCSLModeEnabled() &&
        !clusterMessage.choice().isQueueUpdateAdvisoryValue() &&
        !clusterMessage.choice().isQueueAssignmentAdvisoryValue() &&
        !clusterMessage.choice().isPartitionPrimaryAdvisoryValue() &&
        !clusterMessage.choice().isLeaderAdvisoryValue()) {
        return;  // RETURN
    }

    // Commenting out following 'if' check to fix an assert during node
    // shutdown.
    // if (   d_clusterData_p->membership().selfNodeStatus()
    //     == bmqp_ctrlmsg::NodeStatus::E_STOPPING) {
    //     return;                                                    // RETURN
    // }

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Committed advisory: " << advisory << ", with status '"
                  << status << "'";

    mqbc::ClusterUtil::apply(d_state_p, clusterMessage, *d_clusterData_p);
}

void ClusterStateManager::onSelfActiveLeader()
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(bmqp_ctrlmsg::NodeStatus::E_AVAILABLE ==
                     d_clusterData_p->membership().selfNodeStatus());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": setting self as active leader, will assign partitions "
                  << " and broadcast cluster state to peers.";

    d_clusterData_p->electorInfo().onSelfActiveLeader();

    bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo> partitions;
    assignPartitions(&partitions);

    sendClusterState(true,  // sendPartitionPrimaryInfo
                     true,  // sendQueuesInfo
                     0,
                     partitions);
}

void ClusterStateManager::leaderSyncCb()
{
    // executed by the *SCHEDULER* thread

    dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterStateManager::initiateLeaderSync,
                             this,
                             false),  // don't wait
        d_cluster_p);
}

void ClusterStateManager::onLeaderSyncStateQueryResponse(
    const RequestContextSp& requestContext,
    bsls::Types::Uint64     term)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    // This response is received by the passive leader for the multi-requests
    // that it sent to AVAILABLE followers.

    BSLS_ASSERT_SAFE(!isLocal());

    if (mqbnet::ElectorState::e_LEADER !=
        d_clusterData_p->electorInfo().electorState()) {
        // This node may not be the leader anymore.
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": ignoring leader sync state query response as this "
                      << "node is not the leader anymore. Current elector "
                      << "state: "
                      << d_clusterData_p->electorInfo().electorState()
                      << ". Current leader nodeId: "
                      << d_clusterData_p->electorInfo().leaderNodeId();
        return;  // RETURN
    }

    if (mqbc::ElectorInfoLeaderStatus::e_ACTIVE ==
        d_clusterData_p->electorInfo().leaderStatus()) {
        // Leader (self) has transitioned to ACTIVE, before even processing the
        // leader-sync-query response.  This could occur if there are multiple
        // occurrence of leader switch within a short span of time (due to
        // network issues, bugs etc), and this response belongs to a previous
        // 'instance' of the leader, while a new instance of leader has already
        // transitioned to ACTIVE.  We print current self term and the term
        // when this request was sent by this node.

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": received leader-sync-query response for term: "
                      << term << ", but self has already transitioned to "
                      << "ACTIVE leader. Self term: "
                      << d_clusterData_p->electorInfo().electorTerm()
                      << ". Ignoring this response.";
        return;  // RETURN
    }

    if (bmqp_ctrlmsg::NodeStatus::E_STOPPING ==
        d_clusterData_p->membership().selfNodeStatus()) {
        // Nothing to do if self is stopping
        return;  // RETURN
    }

    // If not stopping, leader must be available.

    BSLS_ASSERT_SAFE(bmqp_ctrlmsg::NodeStatus::E_AVAILABLE ==
                     d_clusterData_p->membership().selfNodeStatus());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": processing leader sync query response. Self's leader "
                  << "message sequence: "
                  << d_clusterData_p->electorInfo().leaderMessageSequence();

    mqbnet::ClusterNode* maxSeqNode = d_clusterData_p->membership().selfNode();
    bmqp_ctrlmsg::LeaderMessageSequence maxSeq =
        d_clusterData_p->electorInfo().leaderMessageSequence();
    const NodeResponsePairs& pairs = requestContext->response();

    // A passive leader never sends request to zero follower.

    BSLS_ASSERT_SAFE(!pairs.empty());

    for (NodeResponsePairsConstIter it = pairs.begin(); it != pairs.end();
         ++it) {
        BSLS_ASSERT_SAFE(it->first);

        if (it->second.choice().isStatusValue()) {
            BALL_LOG_WARN << d_clusterData_p->identity().description()
                          << ": received failed leader sync query response "
                          << it->second.choice().status() << " from "
                          << it->first->nodeDescription()
                          << ". Skipping this node's response.";
            continue;  // CONTINUE
        }

        mqbc::ClusterNodeSession* ns =
            d_clusterData_p->membership().getClusterNodeSession(it->first);
        BSLS_ASSERT_SAFE(ns);
        if (bmqp_ctrlmsg::NodeStatus::E_AVAILABLE != ns->nodeStatus()) {
            BALL_LOG_WARN << d_clusterData_p->identity().description()
                          << ": ignoring leader sync query response from "
                          << it->first->nodeDescription() << " because it is "
                          << "not AVAILABLE.";
            continue;  // CONTINUE
        }

        BSLS_ASSERT_SAFE(it->second.choice().isClusterMessageValue());
        BSLS_ASSERT_SAFE(it->second.choice()
                             .clusterMessage()
                             .choice()
                             .isLeaderSyncStateQueryResponseValue());

        const bmqp_ctrlmsg::LeaderSyncStateQueryResponse& r =
            it->second.choice()
                .clusterMessage()
                .choice()
                .leaderSyncStateQueryResponse();

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": received leader sync query response " << r
                      << " from " << it->first->nodeDescription();

        if (maxSeq < r.leaderMessageSequence()) {
            maxSeq     = r.leaderMessageSequence();
            maxSeqNode = it->first;
        }
    }

    if (maxSeqNode == d_clusterData_p->membership().selfNode()) {
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": leader has latest view during leader sync step. "
                      << "Leader message sequence: " << maxSeq;

        onSelfActiveLeader();

        return;  // RETURN
    }

    // Send request to 'maxSeqNode', requesting its partition/primary mapping
    // and (QueueUri, QueueKey, PartitionId)

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": follower node " << maxSeqNode->nodeDescription()
                  << " has latest view during leader sync step.  Leader "
                  << "message sequence of that node: " << maxSeq
                  << ". Self's leader message sequence: "
                  << d_clusterData_p->electorInfo().leaderMessageSequence()
                  << ".";

    RequestManagerType::RequestSp request =
        d_clusterData_p->requestManager().createRequest();
    request->request()
        .choice()
        .makeClusterMessage()
        .choice()
        .makeLeaderSyncDataQuery();

    request->setResponseCb(bdlf::BindUtil::bind(
        &ClusterStateManager::onLeaderSyncDataQueryResponse,
        this,
        bdlf::PlaceHolders::_1,
        maxSeqNode));

    bmqt::GenericResult::Enum status =
        d_cluster_p->sendRequest(request, maxSeqNode, bsls::TimeInterval(10));

    if (bmqt::GenericResult::e_SUCCESS != status) {
        // Request failed to encode/be sent; process error handling (note that
        // 'onLeaderSyncDataQueryResponse' won't be invoked in this case)

        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << d_clusterData_p->identity().description()
            << ": failed to send leader sync data query request to follower "
            << "node " << maxSeqNode->nodeDescription() << ", rc: " << status
            << ". Attempting to send leader sync state "
            << "query to AVAILABLE followers again." << BMQTSK_ALARMLOG_END;

        // Attempt to send leader sync state query to AVAILABLE followers
        // again (with wait=true flag).

        initiateLeaderSync(true);
    }
}

void ClusterStateManager::onLeaderSyncDataQueryResponse(
    const RequestManagerType::RequestSp& context,
    const mqbnet::ClusterNode*           responder)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(context->request().choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(context->request()
                         .choice()
                         .clusterMessage()
                         .choice()
                         .isLeaderSyncDataQueryValue());
    BSLS_ASSERT_SAFE(responder);

    // This response is received by the passive leader (this node) from the
    // follower ('responder') with the leader sync data.

    if (mqbnet::ElectorState::e_LEADER !=
        d_clusterData_p->electorInfo().electorState()) {
        // This node may not be the leader anymore.
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": ignoring leader-sync data query response from "
                      << responder->nodeDescription() << " as self is not the "
                      << "leader anymore. Current elector state: "
                      << d_clusterData_p->electorInfo().electorState()
                      << ". Current leader nodeId: "
                      << d_clusterData_p->electorInfo().leaderNodeId();
        return;  // RETURN
    }

    const bmqp_ctrlmsg::LeaderSyncDataQuery& req = context->request()
                                                       .choice()
                                                       .clusterMessage()
                                                       .choice()
                                                       .leaderSyncDataQuery();

    if (context->response().choice().isStatusValue()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": received failure response: "
                      << context->response().choice().status()
                      << " for leader-sync data query request: " << req
                      << " from " << responder->nodeDescription()
                      << ". Attempting to send leader sync state query to "
                      << "AVAILABLE followers.";

        initiateLeaderSync(true);
        return;  // RETURN
    }

    // This node is the leader.  It must be a passive leader.
    BSLS_ASSERT(mqbc::ElectorInfoLeaderStatus::e_PASSIVE ==
                d_clusterData_p->electorInfo().leaderStatus());

    // Success
    BSLS_ASSERT(context->response().choice().isClusterMessageValue());
    BSLS_ASSERT(context->response()
                    .choice()
                    .clusterMessage()
                    .choice()
                    .isLeaderSyncDataQueryResponseValue());

    const bmqp_ctrlmsg::LeaderAdvisory& leaderSyncData =
        context->response()
            .choice()
            .clusterMessage()
            .choice()
            .leaderSyncDataQueryResponse()
            .leaderSyncData();

    if (bmqp_ctrlmsg::NodeStatus::E_STOPPING ==
        d_clusterData_p->membership().selfNodeStatus()) {
        // Nothing to do if self is stopping.
        return;  // RETURN
    }

    // If not stopping, leader must be available.

    BSLS_ASSERT_SAFE(bmqp_ctrlmsg::NodeStatus::E_AVAILABLE ==
                     d_clusterData_p->membership().selfNodeStatus());

    // Self's leader message sequence must be smaller than the one contained in
    // the response from follower.

    if (!(d_clusterData_p->electorInfo().leaderMessageSequence() <
          leaderSyncData.sequenceNumber())) {
        // This should not occur.

        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << d_clusterData_p->identity().description()
            << ": Received a smaller or equal leader-msg-sequence number in "
            << "leader-sync data query response from  follower node "
            << responder->nodeDescription()
            << ". Received sequence: " << leaderSyncData.sequenceNumber()
            << ", self sequence:"
            << d_clusterData_p->electorInfo().leaderMessageSequence()
            << ". Attempting to send leader-sync state query to AVAILABLE "
            << "followers again." << BMQTSK_ALARMLOG_END;

        initiateLeaderSync(true);
        return;  // RETURN
    }

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": processing leader-sync data response from "
                  << responder->nodeDescription()
                  << ". Self's leader message sequence: "
                  << d_clusterData_p->electorInfo().leaderMessageSequence()
                  << ", received sequence: " << leaderSyncData.sequenceNumber()
                  << ". Leader sync data: " << leaderSyncData << ".";

    // Converge the partitions.  First step is to update self's partition info
    // based on the provided partition info.

    const bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>& partitionsInfo =
        leaderSyncData.partitions();

    for (unsigned int i = 0; i < partitionsInfo.size(); ++i) {
        const bmqp_ctrlmsg::PartitionPrimaryInfo& peerPinfo =
            partitionsInfo[i];

        if (peerPinfo.partitionId() < 0 ||
            peerPinfo.partitionId() >=
                static_cast<int>(d_state_p->partitions().size())) {
            BMQTSK_ALARMLOG_ALARM("CLUSTER")
                << d_clusterData_p->identity().description()
                << ": Invalid partitionId: " << peerPinfo
                << " received from follower node "
                << responder->nodeDescription()
                << " in leader-sync data query response. "
                << "Skipping this partition info." << BMQTSK_ALARMLOG_END;
            continue;  // CONTINUE
        }

        mqbnet::ClusterNode* proposedPrimaryNode =
            d_clusterData_p->membership().netCluster()->lookupNode(
                peerPinfo.primaryNodeId());
        if (0 == proposedPrimaryNode) {
            BMQTSK_ALARMLOG_ALARM("CLUSTER")
                << d_clusterData_p->identity().description()
                << ": Invalid primaryNodeId: " << peerPinfo
                << " received from follower node "
                << responder->nodeDescription()
                << " in leader-sync data query response. "
                << "Skipping this partition info." << BMQTSK_ALARMLOG_END;
            continue;  // CONTINUE
        }

        if (0 == peerPinfo.primaryLeaseId()) {
            BMQTSK_ALARMLOG_ALARM("CLUSTER")
                << d_clusterData_p->identity().description()
                << ": Invalid primaryLeaseId: " << peerPinfo
                << " received from follower node "
                << responder->nodeDescription()
                << " in leader-sync data query response. "
                << "Skipping this partition info." << BMQTSK_ALARMLOG_END;
            continue;  // CONTINUE
        }

        const mqbc::ClusterStatePartitionInfo& selfPinfo =
            d_state_p->partition(peerPinfo.partitionId());
        if (selfPinfo.primaryLeaseId() > peerPinfo.primaryLeaseId()) {
            // This is most certainly a bug.  If a follower has high leader
            // message sequence, it should never have a lower primaryLeaseId
            // for a given partition.

            BMQTSK_ALARMLOG_ALARM("CLUSTER")
                << d_clusterData_p->identity().description()
                << ": stale primaryLeaseId: " << peerPinfo
                << " received from follower node "
                << responder->nodeDescription()
                << " in leader-sync data query response. Primary leaseId as "
                << "perceived by self: " << selfPinfo.primaryLeaseId()
                << BMQTSK_ALARMLOG_END;
            continue;  // CONTINUE
        }

        unsigned int         effectiveLeaseId     = peerPinfo.primaryLeaseId();
        mqbnet::ClusterNode* effectivePrimaryNode = proposedPrimaryNode;

        // Ensure that 'primaryNode' is perceived as AVAILABLE by this node.

        mqbc::ClusterNodeSession* proposedPrimaryNs =
            d_clusterData_p->membership().getClusterNodeSession(
                proposedPrimaryNode);
        BSLS_ASSERT_SAFE(proposedPrimaryNs);

        if (bmqp_ctrlmsg::NodeStatus::E_AVAILABLE !=
            proposedPrimaryNs->nodeStatus()) {
            // Self node does not perceive proposed primary node as AVAILABLE,
            // but it still needs to use the leaseId specified in the message
            // from the peer as it could be higher than what self has.
            // 'effectiveLeaseId' already contains peer's leaseId.  We just
            // clear out 'effectivePrimaryNode' and proceed ahead, instead of
            // continuing with next iteration.

            effectivePrimaryNode = 0;

            BALL_LOG_WARN << d_clusterData_p->identity().description()
                          << ": specified primary node "
                          << proposedPrimaryNode->nodeDescription()
                          << " is not perceived as AVAILABLE. Node status: "
                          << proposedPrimaryNs->nodeStatus();
        }

        if (d_clusterData_p->membership().selfNode() != effectivePrimaryNode &&
            d_clusterData_p->membership().selfNode() ==
                selfPinfo.primaryNode() &&
            bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE ==
                selfPinfo.primaryStatus() &&
            bmqp_ctrlmsg::NodeStatus::E_AVAILABLE ==
                d_clusterData_p->membership().selfNodeStatus()) {
            // Self node is available, and views self as active primary of this
            // partition, but during leader-sync, a follower with more later
            // view of the cluster has informed self node that a different node
            // is the primary for this partition.  This downgrade scenario
            // (primary -> replica) is currently not supported, so self node
            // will exit.  Note that this scnenario can be witnessed in a bad
            // network where some nodes cannot see other nodes intermittently.
            // See 'processPartitionPrimaryAdvisoryRaw' for similar check.

            BMQTSK_ALARMLOG_ALARM("CLUSTER")
                << d_clusterData_p->identity().description() << " Partition ["
                << peerPinfo.partitionId()
                << "]: self node views self as active/available primary, but a"
                << " different node is proposed as primary in the leader-sync "
                << "step: " << peerPinfo
                << ". This downgrade from primary to replica is currently not "
                << "supported, and self node will exit."
                << BMQTSK_ALARMLOG_END;

            mqbu::ExitUtil::terminate(mqbu::ExitCode::e_UNSUPPORTED_SCENARIO);
            // EXIT
        }

        // All good with this partition info.  Note that the provided partition
        // info could be *exactly* same as that contained by this node.  We
        // update the state anyways.

        // TODO CSL Please review this code path during node startup sequence.
        d_state_p->setPartitionPrimary(peerPinfo.partitionId(),
                                       effectiveLeaseId,
                                       effectivePrimaryNode);  // Could be null

        // Update primary node-specific list of assigned partitions only if it
        // needs to be.

        if (effectivePrimaryNode) {
            BSLS_ASSERT_SAFE(effectivePrimaryNode == proposedPrimaryNode);
            proposedPrimaryNs->addPartitionSafe(peerPinfo.partitionId());
        }
    }

    const bsl::vector<bmqp_ctrlmsg::QueueInfo>& queuesInfo =
        leaderSyncData.queues();

    for (unsigned int i = 0; i < queuesInfo.size(); ++i) {
        const bmqp_ctrlmsg::QueueInfo& queueInfo = queuesInfo[i];

        // It is assumed that uri present in 'queueInfo' is canonical.

        if (queueInfo.partitionId() < 0 ||
            queueInfo.partitionId() >=
                static_cast<int>(d_state_p->partitions().size())) {
            BMQTSK_ALARMLOG_ALARM("CLUSTER")
                << d_clusterData_p->identity().description()
                << ": Invalid partitionId specified for queueUri ["
                << queueInfo.uri() << "]: " << queueInfo.partitionId()
                << " by follower node " << responder->nodeDescription()
                << " in leader-sync data query response."
                << BMQTSK_ALARMLOG_END;
            continue;  // CONTINUE
        }

        if (queueInfo.key().size() != static_cast<unsigned int>(k_KEY_LEN)) {
            BMQTSK_ALARMLOG_ALARM("CLUSTER")
                << d_clusterData_p->identity().description()
                << ": Invalid queue key length for queueUri ["
                << queueInfo.uri() << "]: " << queueInfo.key().size()
                << " by follower node " << responder->nodeDescription()
                << " in leader-sync data query response."
                << BMQTSK_ALARMLOG_END;

            continue;  // CONTINUE
        }

        // This node may or may not be aware of the queue.  There are 3
        // scenarios if self node is aware of the queue:
        // 1) Queue metadata is same (all good).
        // 2) Queue metadata is different and self's queue metadata is
        //    null/empty.  Self should update with what peer is saying and
        //    continue.
        // 3) Queue metadata is different, and self's queue metadata is not
        //    null/empty.  Self and peer have conflicting views... most
        //    likely because self has stale view of the cluster state.  We
        //    force-update self's view.
        //
        // (2) can occur in this scenario: before self was leader, it sent
        // the old leader a queue-assignment request.  Old leader assigned
        // the queue, and broadcast the queue-assignment advisory, but
        // crashed at a point such that the advisory could reach to all but
        // self node.  Now self ended up with null/empty queue key and
        // partitionId for that queue.  Then self got promoted to leader,
        // initiated the sync and ended up here.

        registerQueueInfo(queueInfo, true);  // Force update?
        // Note that we don't inform the storage manager or the partition about
        // this queue, because writes to the partition are issued only by the
        // primary of that partition, and self may or may not be the primary.
        // Also, leader-sync != primary-sync.  If self's partition is not aware
        // of this queue, we can rely on the primary eventually syncing it.
    }

    // Mark the leader as active, assign orphan partitions and broadcast
    // leader advisory.
    onSelfActiveLeader();
}

void ClusterStateManager::processBufferedQueueAdvisories()
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());

    if (bmqp_ctrlmsg::NodeStatus::E_STOPPING ==
        d_clusterData_p->membership().selfNodeStatus()) {
        return;  // RETURN
    }

    // There is no need to validate the leader, leader sequence number, primary
    // or primary leaseId-sequenceNum associated with the buffered queue
    // assignment advisories.  This is because they were validated at the time
    // of their reception (and the advisories were buffered only after that).
    // Additionally, there may be a different leader/primary at this time.  So
    // if we validate, we will fail to apply (ie, replay) these then-valid
    // advisories, which means cluster state at this node will go out of sync.
    for (size_t i = 0; i < d_bufferedQueueAdvisories.size(); ++i) {
        const QueueAdvisoryAndSource& advSourcePair =
            d_bufferedQueueAdvisories[i];

        BSLS_ASSERT_SAFE(advSourcePair.second);  // source

        mqbnet::ClusterNode*                source = advSourcePair.second;
        const bmqp_ctrlmsg::ControlMessage& msg    = advSourcePair.first;

        BSLS_ASSERT_SAFE(msg.choice().isClusterMessageValue());
        BSLS_ASSERT_SAFE(msg.choice()
                             .clusterMessage()
                             .choice()
                             .isQueueUnAssignmentAdvisoryValue());

        processQueueUnAssignmentAdvisory(msg, source, true /* delayed */);
    }

    d_bufferedQueueAdvisories.clear();
}

void ClusterStateManager::processPartitionPrimaryAdvisoryRaw(
    const bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>& partitions,
    const mqbnet::ClusterNode*                             source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(source == d_clusterData_p->electorInfo().leaderNode());

    BALL_LOG_INFO
        << d_clusterData_p->identity().description()
        << ": processing partition-primary mapping: "
        << bmqu::Printer<bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo> >(
               &partitions)
        << " from leader node " << source->nodeDescription();

    // Validate the notification.  If *any* part of notification is found
    // invalid, we reject the *entire* notification.

    for (int i = 0; i < static_cast<int>(partitions.size()); ++i) {
        const bmqp_ctrlmsg::PartitionPrimaryInfo& info = partitions[i];

        if (info.partitionId() >=
            static_cast<int>(d_state_p->partitions().size())) {
            BMQTSK_ALARMLOG_ALARM("CLUSTER")
                << d_clusterData_p->identity().description()
                << ": Invalid partitionId: " << info
                << " specified in partition-primary advisory. "
                << "Ignoring this *ENTIRE* advisory message."
                << BMQTSK_ALARMLOG_END;
            return;  // RETURN
        }

        mqbnet::ClusterNode* proposedPrimaryNode =
            d_clusterData_p->membership().netCluster()->lookupNode(
                info.primaryNodeId());

        if (0 == proposedPrimaryNode) {
            BMQTSK_ALARMLOG_ALARM("CLUSTER")
                << d_clusterData_p->identity().description()
                << ": Invalid primaryNodeId: " << info
                << " specified in partition-primary advisory."
                << " Ignoring this *ENTIRE* advisory." << BMQTSK_ALARMLOG_END;
            return;  // RETURN
        }

        if (proposedPrimaryNode == d_clusterData_p->membership().selfNode() &&
            bmqp_ctrlmsg::NodeStatus::E_STARTING ==
                d_clusterData_p->membership().selfNodeStatus()) {
            // Self is the proposed primary but self is STARTING.  This is a
            // bug because if this node perceives self as STARTING, any other
            // node (including the leader) *cannot* perceive this node as
            // AVAILABLE.  This node might be STOPPING, but that's ok since its
            // possible that it transitioned from AVAILABLE to other state
            // immediately after leader broadcast the advisory.  Lower layers
            // will take care of that scenario.

            BMQTSK_ALARMLOG_ALARM("CLUSTER")
                << d_clusterData_p->identity().description()
                << ": proposed primary specified in partition/primary "
                   "mapping: "
                << info << " is self but self is STARTING. "
                << "Ignoring this *ENTIRE* advisory." << BMQTSK_ALARMLOG_END;
            return;  // RETURN
        }

        const mqbc::ClusterStatePartitionInfo& pi = d_state_p->partition(
            info.partitionId());

        if (d_clusterData_p->membership().selfNode() != proposedPrimaryNode &&
            d_clusterData_p->membership().selfNode() == pi.primaryNode() &&
            bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE == pi.primaryStatus() &&
            bmqp_ctrlmsg::NodeStatus::E_AVAILABLE ==
                d_clusterData_p->membership().selfNodeStatus()) {
            // Self node is available, and views self as active primary of this
            // partition, but has received an advisory from the leader
            // indicating that a different node is the primary for this
            // partition.  This downgrade scenario (primary -> replica) is
            // currently not supported, so self node will exit.  Note that this
            // scnenario can be witnessed in a bad network where some nodes
            // cannot see other nodes intermittently.  See
            // 'onLeaderSyncDataQueryResponse' for similar check.

            BMQTSK_ALARMLOG_ALARM("CLUSTER")
                << d_clusterData_p->identity().description() << " Partition ["
                << info.partitionId()
                << "]: self node views self as active/available primary, but a"
                << " different node is proposed as primary in the "
                << "partition/primary mapping: " << info << ". This downgrade "
                << "from primary to replica is currently not supported, and "
                << "self node will exit." << BMQTSK_ALARMLOG_END;

            mqbu::ExitUtil::terminate(mqbu::ExitCode::e_UNSUPPORTED_SCENARIO);
            // EXIT
        }

        if (d_isFirstLeaderAdvisory) {
            // If this node just started and recovered from a peer, it will
            // already by aware of the *current* primaryLeaseId for each
            // partition, but not its primary node (this is because leaseId is
            // retrieved from the storage, but not the primary nodeId, because
            // we don't persist the primary nodeId).  When this node becomes
            // AVAILABLE, the leader simply sends the partition/primary
            // advisory without bumping up the leaseId.
            // 'd_isFirstLeaderAdvisory' flag takes care of this scenario.

            // Note that 'pi.primaryNode()' may not be zero because currently,
            // we update the cluster state even upon receiving primary status
            // advisory from a primary node, so self node may or may not have
            // received a primary-status advisory from the primary node.

            if (info.primaryLeaseId() < pi.primaryLeaseId()) {
                BMQTSK_ALARMLOG_ALARM("CLUSTER")
                    << d_clusterData_p->identity().description()
                    << ": Stale primaryLeaseId specified in: " << info
                    << ", current primaryLeaseId: " << pi.primaryLeaseId()
                    << ". Ignoring this *ENTIRE* advisory."
                    << BMQTSK_ALARMLOG_END;
                return;  // RETURN
            }
        }
        else {
            if ((pi.primaryNode() == proposedPrimaryNode) ||
                (pi.primaryNode() == 0)) {
                // Proposed primary node is same as self's primary node, or
                // self views this partition as orphan.  In either case,
                // leaseId cannot be smaller.  It can, however, be equal.  The
                // case in which 'pi.primaryNode() == proposedPrimaryNode' and
                // leaseId is same, is obvious -- the leader simply re-sent the
                // partition primary mapping advisory.  But the case where
                // pi.primaryNode() is null (and 'proposedPrimaryNode' is
                // valid) *and* leaseId is same can be explained in this way:
                // this node (replica) was aware of the primary and leaseId,
                // but then at some point, lost connection to the primary, and
                // marked this partition as orphan.  Note that primary node did
                // not crash.  After some time, connection was re-established,
                // and leader/primary resent the primary mapping again -- with
                // same leaseId.  This scenario was seen when cluster was
                // running on VM boxes.  Also note that above scenario is
                // different from the case where 'd_isFirstLeaderAdvisory' flag
                // is used -- that flag is used when a node has not heard from
                // leader even once.

                BSLS_ASSERT_SAFE(0 != proposedPrimaryNode);

                if (info.primaryLeaseId() < pi.primaryLeaseId()) {
                    BMQTSK_ALARMLOG_ALARM("CLUSTER")
                        << d_clusterData_p->identity().description()
                        << ": Stale primaryLeaseId specified in: " << info
                        << ", current primaryLeaseId: " << pi.primaryLeaseId()
                        << ". Primary node viewed by self: "
                        << (pi.primaryNode() != 0
                                ? pi.primaryNode()->nodeDescription()
                                : "** null **")
                        << ", proposed primary node: "
                        << proposedPrimaryNode->nodeDescription()
                        << ". Ignoring this *ENTIRE* advisory."
                        << BMQTSK_ALARMLOG_END;
                    return;  // RETURN
                }
            }
            else {
                // Different (non-zero) primary nodes.  Proposed leaseId must
                // be greater.

                if (info.primaryLeaseId() <= pi.primaryLeaseId()) {
                    BMQTSK_ALARMLOG_ALARM("CLUSTER")
                        << d_clusterData_p->identity().description()
                        << ": Stale primaryLeaseId specified in: " << info
                        << ", current primaryLeaseId: " << pi.primaryLeaseId()
                        << ". Ignoring this *ENTIRE* advisory."
                        << BMQTSK_ALARMLOG_END;
                    return;  // RETURN
                }
            }
        }
    }

    if (d_clusterData_p->membership().selfNodeStatus() ==
        bmqp_ctrlmsg::NodeStatus::E_STOPPING) {
        // No need to process the advisory since self is stopping.
        BALL_LOG_INFO << d_cluster_p->description()
                      << ": Not processing partition primary advisory since"
                      << " self is stopping.";
        return;  // RETURN
    }

    // All valid.

    for (unsigned int i = 0; i < partitions.size(); ++i) {
        const bmqp_ctrlmsg::PartitionPrimaryInfo& info = partitions[i];
        const mqbc::ClusterStatePartitionInfo&    pi   = d_state_p->partition(
            info.partitionId());

        mqbnet::ClusterNode* proposedPrimaryNode =
            d_clusterData_p->membership().netCluster()->lookupNode(
                info.primaryNodeId());

        mqbc::ClusterNodeSession* ns =
            d_clusterData_p->membership().getClusterNodeSession(
                proposedPrimaryNode);
        BSLS_ASSERT_SAFE(ns);

        if (proposedPrimaryNode == pi.primaryNode()) {
            if (pi.primaryLeaseId() == info.primaryLeaseId()) {
                // Leader has re-sent the primary info for this partition.  We
                // must continue with the logic (like notifying StorageMgr etc)
                // because certain peers may not be aware that this node is
                // (active) primary, and will need to be notified by StorageMgr
                // of the same.
                BSLS_ASSERT_SAFE(
                    ns->isPrimaryForPartition(info.partitionId()));
            }
            else {
                // We do support the scenario where proposed primary node is
                // same as the current one, but only leaseId has been bumped
                // up.
                BSLS_ASSERT_SAFE(pi.primaryLeaseId() < info.primaryLeaseId());
            }
        }

        // Remove the old partition<->primary mapping (below logic works even
        // if proposed primary node is same as existing one).

        if (0 != pi.primaryNode()) {
            mqbc::ClusterNodeSession* currPrimaryNs =
                d_clusterData_p->membership().getClusterNodeSession(
                    pi.primaryNode());
            BSLS_ASSERT_SAFE(currPrimaryNs);
            currPrimaryNs->removePartitionSafe(info.partitionId());
        }

        ns->addPartitionRaw(info.partitionId());

        // Notify the storage about (potentially same) mapping.  This must be
        // done before updating cluster state, because ClusterQueueHelper, an
        // observer of cluster state, assumes that storage is aware of the
        // mapping.
        d_storageManager_p->setPrimaryForPartition(info.partitionId(),
                                                   proposedPrimaryNode,
                                                   info.primaryLeaseId());

        d_state_p->setPartitionPrimary(info.partitionId(),
                                       info.primaryLeaseId(),
                                       proposedPrimaryNode);
    }

    d_isFirstLeaderAdvisory = false;
}

// PRIVATE MANIPULATORS
//   (virtual: mqbc::ElectorInfoObserver)
void ClusterStateManager::onClusterLeader(
    mqbnet::ClusterNode*                node,
    mqbc::ElectorInfoLeaderStatus::Enum status)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    if (!node) {
        BSLS_ASSERT_SAFE(mqbc::ElectorInfoLeaderStatus::e_UNDEFINED == status);

        d_isFirstLeaderAdvisory = true;
    }
}

// PRIVATE MANIPULATORS
//   (virtual: mqbc::ClusterStateObserver)
void ClusterStateManager::onPartitionPrimaryAssignment(
    int                                partitionId,
    mqbnet::ClusterNode*               primary,
    unsigned int                       leaseId,
    bmqp_ctrlmsg::PrimaryStatus::Value status,
    mqbnet::ClusterNode*               oldPrimary,
    unsigned int                       oldLeaseId)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());

    // This method will notify the storage about (potentially same)
    // mapping.  This must be done before calling
    // 'ClusterQueueHelper::afterPartitionPrimaryAssignment' (via
    // d_afterPartitionPrimaryAssignmentCb), because ClusterQueueHelper
    // assumes that storage is aware of the mapping.
    mqbc::ClusterUtil::onPartitionPrimaryAssignment(d_clusterData_p,
                                                    d_storageManager_p,
                                                    partitionId,
                                                    primary,
                                                    leaseId,
                                                    status,
                                                    oldPrimary,
                                                    oldLeaseId);

    d_isFirstLeaderAdvisory = false;

    d_afterPartitionPrimaryAssignmentCb(partitionId, primary, status);
}

// CREATORS
ClusterStateManager::ClusterStateManager(
    const mqbcfg::ClusterDefinition& clusterConfig,
    mqbi::Cluster*                   cluster,
    mqbc::ClusterData*               clusterData,
    mqbc::ClusterState*              clusterState,
    ClusterStateLedgerMp             clusterStateLedger,
    bslma::Allocator*                allocator)
: d_allocator_p(allocator)
, d_allocators(d_allocator_p)
, d_isStarted(false)
, d_clusterConfig(clusterConfig)
, d_cluster_p(cluster)
, d_clusterData_p(clusterData)
, d_state_p(clusterState)
, d_clusterStateLedger_mp(clusterStateLedger)
, d_storageManager_p(0)
, d_isFirstLeaderAdvisory(true)
, d_bufferedQueueAdvisories(allocator)
{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(allocator);
    BSLS_ASSERT_SAFE(d_cluster_p);
    BSLS_ASSERT_SAFE(d_clusterData_p);
    BSLS_ASSERT_SAFE(d_state_p);
    BSLS_ASSERT_SAFE(d_clusterStateLedger_mp);

    d_clusterStateLedger_mp->setCommitCb(
        bdlf::BindUtil::bind(&ClusterStateManager::onCommit,
                             this,
                             bdlf::PlaceHolders::_1,    // advisory
                             bdlf::PlaceHolders::_2));  // status
}

ClusterStateManager::~ClusterStateManager()
{
    // executed by *ANY* thread

    BSLS_ASSERT(!d_isStarted && "stop() must be called");
}

// MANIPULATORS
//   (virtual: mqbi::ClusterStateManager)
int ClusterStateManager::start(bsl::ostream& errorDescription)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    enum { rc_SUCCESS = 0, rc_CLUSTER_STATE_LEDGER_FAILURE = -1 };

    if (d_isStarted) {
        return rc_SUCCESS;  // RETURN
    }

    // Start the cluster state ledger
    const int rc = d_clusterStateLedger_mp->open();
    if (rc != 0) {
        errorDescription << d_clusterData_p->identity().description()
                         << ": Failed to open cluster state ledger [rc: " << rc
                         << "]";
        return rc * 10 + rc_CLUSTER_STATE_LEDGER_FAILURE;  // RETURN
    }

    d_state_p->registerObserver(this);

    d_isStarted = true;
    return rc_SUCCESS;
}

void ClusterStateManager::stop()
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    if (!d_isStarted) {
        return;  // RETURN
    }

    d_isStarted = false;

    const int rc = d_clusterStateLedger_mp->close();
    if (rc != 0) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << ": Failed to close cluster state ledger [rc: " << rc
                       << "]";
    }
}

void ClusterStateManager::setPrimary(int                  partitionId,
                                     unsigned int         leaseId,
                                     mqbnet::ClusterNode* primary)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(static_cast<size_t>(partitionId) <
                     d_state_p->partitions().size());
    BSLS_ASSERT_SAFE(primary);

    d_state_p->setPartitionPrimary(partitionId, leaseId, primary);
}

void ClusterStateManager::setPrimaryStatus(
    int                                partitionId,
    bmqp_ctrlmsg::PrimaryStatus::Value status)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(static_cast<size_t>(partitionId) <
                     d_state_p->partitions().size());

    d_state_p->setPartitionPrimaryStatus(partitionId, status);
}

void ClusterStateManager::markOrphan(const bsl::vector<int>& partitions,
                                     mqbnet::ClusterNode*    primary)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(primary);

    for (int i = 0; i < static_cast<int>(partitions.size()); ++i) {
        const mqbc::ClusterStatePartitionInfo& pinfo = d_state_p->partition(
            partitions[i]);
        d_state_p->setPartitionPrimary(partitions[i],
                                       pinfo.primaryLeaseId(),
                                       0);  // no primary node
    }
}

void ClusterStateManager::assignPartitions(
    bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>* partitions)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(d_clusterData_p->electorInfo().isSelfActiveLeader());
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());
    BSLS_ASSERT_SAFE(partitions && partitions->empty());

    mqbc::ClusterUtil::assignPartitions(
        partitions,
        d_state_p,
        d_clusterConfig.masterAssignment(),
        *d_clusterData_p,
        d_clusterConfig.clusterAttributes().isCSLModeEnabled());
}

ClusterStateManager::QueueAssignmentResult::Enum
ClusterStateManager::assignQueue(const bmqt::Uri&      uri,
                                 bmqp_ctrlmsg::Status* status)
{
    // executed by the cluster *DISPATCHER* thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    return mqbc::ClusterUtil::assignQueue(d_state_p,
                                          d_clusterData_p,
                                          d_clusterStateLedger_mp.get(),
                                          d_cluster_p,
                                          uri,
                                          d_allocator_p,
                                          status);
}

void ClusterStateManager::registerQueueInfo(
    const bmqp_ctrlmsg::QueueInfo& advisory,
    bool                           forceUpdate)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    mqbc::ClusterUtil::registerQueueInfo(d_state_p,
                                         d_cluster_p,
                                         advisory,
                                         forceUpdate);
}

void ClusterStateManager::unassignQueue(
    const bmqp_ctrlmsg::QueueUnassignedAdvisory& advisory)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(d_clusterData_p->electorInfo().isSelfActiveLeader());
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());

    BALL_LOG_DEBUG << d_clusterData_p->identity().description()
                   << ": 'QueueUnAssignmentAdvisory' will be applied to "
                   << "cluster state ledger: " << advisory;

    const int rc = d_clusterStateLedger_mp->apply(advisory);
    if (rc != 0) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << ": Failed to apply queue unassignment advisory: "
                       << advisory << ", rc: " << rc;
    }
    else {
        // In non-CSL mode this is the shortcut to call Primary CQH instead of
        // waiting for the quorum of acks in the ledger.
        for (bsl::vector<bmqp_ctrlmsg::QueueInfo>::const_iterator cit =
                 advisory.queues().begin();
             cit != advisory.queues().end();
             ++cit) {
            const bmqp_ctrlmsg::QueueInfo& queueInfo = *cit;

            if (d_state_p->unassignQueue(queueInfo.uri())) {
                BALL_LOG_INFO << d_clusterData_p->identity().description()
                              << ": Queue unassigned: " << queueInfo;
            }
            else {
                BALL_LOG_INFO << d_clusterData_p->identity().description()
                              << ": Failed to unassign Queue: " << queueInfo;
            }
        }
    }
}

void ClusterStateManager::sendClusterState(
    bool                 sendPartitionPrimaryInfo,
    bool                 sendQueuesInfo,
    mqbnet::ClusterNode* node,
    const bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>& partitions)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(mqbnet::ElectorState::e_LEADER ==
                     d_clusterData_p->electorInfo().electorState());

    // Self is leader and has published advisory above, so update it.
    d_isFirstLeaderAdvisory = false;

    mqbc::ClusterUtil::sendClusterState(d_clusterData_p,
                                        d_clusterStateLedger_mp.get(),
                                        d_storageManager_p,
                                        *d_state_p,
                                        sendPartitionPrimaryInfo,
                                        sendQueuesInfo,
                                        node,
                                        partitions);
}

void ClusterStateManager::updateAppIds(const bsl::vector<bsl::string>& added,
                                       const bsl::vector<bsl::string>& removed,
                                       const bsl::string& domainName,
                                       const bsl::string& uri)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());
    BSLS_ASSERT_SAFE(!domainName.empty());

    mqbc::ClusterUtil::updateAppIds(d_clusterData_p,
                                    d_clusterStateLedger_mp.get(),
                                    *d_state_p,
                                    added,
                                    removed,
                                    domainName,
                                    uri,
                                    d_allocator_p);
}

void ClusterStateManager::initiateLeaderSync(bool wait)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    if (mqbnet::ElectorState::e_LEADER !=
        d_clusterData_p->electorInfo().electorState()) {
        // This node may have gone from leader to non-leader during the time
        // it waited to perform leader sync.

        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(d_clusterData_p->membership().selfNode()->nodeId() ==
                     d_clusterData_p->electorInfo().leaderNodeId());

    if (d_clusterData_p->membership().selfNodeStatus() !=
        bmqp_ctrlmsg::NodeStatus::E_AVAILABLE) {
        // Node is not AVAILABLE at the moment.  It could be STARTING or
        // STOPPING.  Leader sync will occur if/when this node transitions to
        // AVAILABLE, and it is found to be a passive leader.  This is
        // applicable to a local cluster as well.

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": node is not AVAILABLE. Current status ["
                      << d_clusterData_p->membership().selfNodeStatus()
                      << "]. Not initiating leader sync step at this time.";
        return;  // RETURN
    }

    // Node is AVAILABLE.  Perform leader sync with all AVAILABLE nodes.  Do
    // not update self's leader message sequence yet.

    bsl::vector<mqbnet::ClusterNode*> availableFollowers;
    for (ClusterNodeSessionMapIter nit =
             d_clusterData_p->membership().clusterNodeSessionMap().begin();
         nit != d_clusterData_p->membership().clusterNodeSessionMap().end();
         ++nit) {
        if (nit->first != d_clusterData_p->membership().selfNode() &&
            nit->second->nodeStatus() ==
                bmqp_ctrlmsg::NodeStatus::E_AVAILABLE) {
            availableFollowers.push_back(nit->first);
        }
    }

    if (availableFollowers.empty()) {
        // None of the followers are AVAILABLE.  This means that all or
        // majority of the nodes in the cluster are coming up.

        if (wait && !isLocal()) {
            // Wait for a few seconds before performing leader sync, in order
            // to give a chance to all nodes to come up and declare themselves
            // AVAILABLE.  Note that this should be done only in case of
            // cluster of size > 1.

            const int leaderSyncDelayMs =
                d_clusterConfig.elector().leaderSyncDelayMs();
            const bsls::Types::Int64 leaderSyncDelayNs =
                leaderSyncDelayMs * bdlt::TimeUnitRatio::k_NS_PER_MS;
            BALL_LOG_INFO << d_clusterData_p->identity().description()
                          << ": none of the followers AVAILABLE. Waiting for "
                          << bmqu::PrintUtil::prettyTimeInterval(
                                 leaderSyncDelayNs)
                          << " before initiating leader sync.";

            bsls::TimeInterval after(bmqsys::Time::nowMonotonicClock());
            after.addMilliseconds(leaderSyncDelayMs);
            d_clusterData_p->scheduler().scheduleEvent(
                d_clusterData_p->electorInfo().leaderSyncEventHandle(),
                after,
                bdlf::BindUtil::bind(&ClusterStateManager::leaderSyncCb,
                                     this));

            return;  // RETURN
        }

        // Since none of the followers are AVAILABLE and (1) no-wait option is
        // specified or (2) its a local cluster, no need to sync with them.
        // Send leader advisory now.

        if (!isLocal()) {
            BALL_LOG_INFO << d_clusterData_p->identity().description()
                          << ": none of the followers are AVAILABLE. No need "
                          << "for leader sync.";
        }

        onSelfActiveLeader();

        return;  // RETURN
    }

    // One or more followers are AVAILABLE (applicable only in case of cluster
    // of size > 1).  Send a leader sync request.

    BSLS_ASSERT_SAFE(!isLocal());

    MultiRequestManagerType::RequestContextSp contextSp =
        d_clusterData_p->multiRequestManager().createRequestContext();

    contextSp->request()
        .choice()
        .makeClusterMessage()
        .choice()
        .makeLeaderSyncStateQuery();

    contextSp->setDestinationNodes(availableFollowers);
    contextSp->setResponseCb(bdlf::BindUtil::bind(
        &ClusterStateManager::onLeaderSyncStateQueryResponse,
        this,
        bdlf::PlaceHolders::_1,
        d_clusterData_p->electorInfo().electorTerm()));

    d_clusterData_p->multiRequestManager().sendRequest(contextSp,
                                                       bsls::TimeInterval(10));
}

void ClusterStateManager::processLeaderSyncStateQuery(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(message.choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(message.choice()
                         .clusterMessage()
                         .choice()
                         .isLeaderSyncStateQueryValue());

    // This query is sent by a passive leader ('source') to all AVAILABLE
    // followers to find out the latest leader state (leader message sequence
    // to be specific).  We don't check to see if 'source' is really perceived
    // as the leader by this node, because it really doesn't matter.  If
    // 'source' is not the leader anymore, it will simply not process this
    // response.  We do however check self status, and if self is not
    // AVAILABLE, we do not respond so as not to send any incomplete info.

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = message.rId();

    if (bmqp_ctrlmsg::NodeStatus::E_AVAILABLE ==
        d_clusterData_p->membership().selfNodeStatus()) {
        bmqp_ctrlmsg::ClusterMessage& clusterMsg =
            controlMsg.choice().makeClusterMessage();
        bmqp_ctrlmsg::LeaderSyncStateQueryResponse& response =
            clusterMsg.choice().makeLeaderSyncStateQueryResponse();

        response.leaderMessageSequence() =
            d_clusterData_p->electorInfo().leaderMessageSequence();
    }
    else {
        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category() = bmqp_ctrlmsg::StatusCategory::E_NOT_READY;
        status.code()     = -1;
        status.message()  = "Peer is not available.";
    }

    d_clusterData_p->messageTransmitter().sendMessage(controlMsg, source);

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": sent response " << controlMsg
                  << " to leader sync query from "
                  << source->nodeDescription();
}

void ClusterStateManager::processQueueAssignmentRequest(
    const bmqp_ctrlmsg::ControlMessage& request,
    mqbnet::ClusterNode*                requester)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    mqbc::ClusterUtil::processQueueAssignmentRequest(
        d_state_p,
        d_clusterData_p,
        d_clusterStateLedger_mp.get(),
        d_cluster_p,
        request,
        requester,
        d_allocator_p);
}

void ClusterStateManager::processQueueUnassignedAdvisory(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(message.choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(message.choice()
                         .clusterMessage()
                         .choice()
                         .isQueueUnassignedAdvisoryValue());
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());

    // TODO: For now, the leader sequence number in the message is not
    //       validated because unassignment advisories are sent by the primary.
    //       In the future, they will be sent by the leader, and should be
    //       validated then.

    const bmqp_ctrlmsg::QueueUnassignedAdvisory& adv =
        message.choice().clusterMessage().choice().queueUnassignedAdvisory();

    if (d_clusterConfig.clusterAttributes().isCSLModeEnabled()) {
        BALL_LOG_ERROR << "#CSL_MODE_MIX "
                       << "Received legacy queueUnassignedAdvisory: " << adv
                       << " from: " << source << " in CSL mode.";

        return;  // RETURN
    }

    bmqp_ctrlmsg::ControlMessage             legacyMsg;
    bmqp_ctrlmsg::QueueUnAssignmentAdvisory& legacyAdv =
        legacyMsg.choice()
            .makeClusterMessage()
            .choice()
            .makeQueueUnAssignmentAdvisory();

    legacyAdv.partitionId()    = adv.partitionId();
    legacyAdv.primaryLeaseId() = adv.primaryLeaseId();
    legacyAdv.primaryNodeId()  = adv.primaryNodeId();
    legacyAdv.queues()         = adv.queues();

    processQueueUnAssignmentAdvisory(legacyMsg, source);
}

void ClusterStateManager::processQueueUnAssignmentAdvisory(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source,
    bool                                delayed)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(message.choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(message.choice()
                         .clusterMessage()
                         .choice()
                         .isQueueUnAssignmentAdvisoryValue());
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());

    const bmqp_ctrlmsg::QueueUnAssignmentAdvisory& advisory =
        message.choice().clusterMessage().choice().queueUnAssignmentAdvisory();

    if (d_clusterConfig.clusterAttributes().isCSLModeEnabled()) {
        BALL_LOG_ERROR << "#CSL_MODE_MIX "
                       << "Received legacy " << (delayed ? "buffered " : "")
                       << "queueUnAssignmentAdvisory: " << advisory
                       << " from: " << source << " in CSL mode.";

        return;  // RETURN
    }

    BALL_LOG_INFO << d_cluster_p->description() << ": Processing"
                  << (delayed ? " buffered " : " ")
                  << "queueUnAssignmentAdvisory message: " << message
                  << ", from " << source->nodeDescription();

    const mqbc::ClusterStatePartitionInfo& pi = d_state_p->partition(
        advisory.partitionId());
    if (!delayed) {
        // Source (primary) and leaseId should not be validated for delayed
        // (aka buffered) advisories.  Those attributes were validated when
        // buffered advisories were received.
        if (!pi.primaryNode() ||
            advisory.primaryNodeId() != pi.primaryNode()->nodeId()) {
            // Different primary.  Ignore message.
            BALL_LOG_WARN << d_cluster_p->description()
                          << ": ignoring queueUnAssignmentAdvisory: "
                          << advisory << " from: " << source->nodeDescription()
                          << ", because the primary mismatch [currentPrimary: "
                          << (pi.primaryNode()
                                  ? pi.primaryNode()->nodeDescription()
                                  : "** none **")
                          << ", leaseId: " << pi.primaryLeaseId() << "]";
            return;  // RETURN
        }

        // Verify 'stale' primary leaseId
        if (advisory.primaryLeaseId() < pi.primaryLeaseId()) {
            BMQTSK_ALARMLOG_ALARM("CLUSTER_STATE")
                << d_cluster_p->description()
                << ": got queueUnAssignmentAdvisory: " << advisory
                << " from current primary: " << source->nodeDescription()
                << ", with smaller leaseId: " << advisory.primaryLeaseId()
                << ", current: " << pi.primaryLeaseId()
                << ". Ignoring this advisory." << BMQTSK_ALARMLOG_END;
            return;  // RETURN
        }
    }

    if (d_clusterData_p->membership().selfNodeStatus() ==
        bmqp_ctrlmsg::NodeStatus::E_STOPPING) {
        // No need to process the advisory since self is stopping.
        BALL_LOG_INFO << d_cluster_p->description()
                      << ": Not processing queue un-asssignment advisory since"
                      << " self is stopping.";
        return;  // RETURN
    }

    // Advisory and source have been validated.  If self is starting and this
    // is a "live" advisory, buffer the advisory and it will be applied later,
    // else apply it right away.
    if (!delayed && bmqp_ctrlmsg::NodeStatus::E_STARTING ==
                        d_clusterData_p->membership().selfNodeStatus()) {
        d_bufferedQueueAdvisories.push_back(bsl::make_pair(message, source));
        return;  // RETURN
    }

    for (bsl::vector<bmqp_ctrlmsg::QueueInfo>::const_iterator it =
             advisory.queues().begin();
         it != advisory.queues().end();
         ++it) {
        const bmqp_ctrlmsg::QueueInfo& queueInfo = *it;

        // Ensure the partitionId of the QueueInfo matches the one at the top
        // level advisory message.
        BSLS_ASSERT_SAFE(advisory.partitionId() == queueInfo.partitionId());

        bmqt::Uri        uri(queueInfo.uri());
        mqbu::StorageKey key(mqbu::StorageKey::BinaryRepresentation(),
                             queueInfo.key().data());

        mqbc::ClusterStateQueueInfo* assigned = d_state_p->getAssigned(uri);
        // Only Replica can `processQueueAssignmentAdvisory`.  Therefore, the
        // state cannot be `k_UNASSIGNING`

        if (assigned == 0) {
            // Queue is not assigned.  Error because it should not occur.

            BALL_LOG_ERROR << d_cluster_p->description()
                           << " Ignoring queueUnAssignementAdvisory: "
                           << queueInfo << ", from "
                           << source->nodeDescription() << ", for queue ["
                           << uri
                           << "] because self node sees queue as unassigned.";
            continue;  // CONTINUE
        }

        // Self node sees queue as assigned.  Validate that the key/partition
        // from the unassignment match the internal state.

        if ((assigned->partitionId() != advisory.partitionId()) ||
            (assigned->key() != key)) {
            // This can occur if a queue is deleted by the primary and created
            // immediately by the client.  Primary broadcasts queue
            // unassignment advisory upon deleting old instance of the queue,
            // then leader broadcasts queue assignment advisory for new
            // instance of the queue, but these 2 events come out of order at
            // self node (queue assignment followed by queue unassignment).
            // Self node updates queue info upon receiving queue assignment
            // advisory from the leader, which causes this mismatch.

            // Not completely sure if this could occur in a scenario other than
            // above (ie, if this is a buffered advisory).

            BALL_LOG_ERROR << d_cluster_p->description() << " Ignoring"
                           << (delayed ? " buffered " : " ")
                           << "queueUnAssignmentAdvisory for queue [" << uri
                           << "] from '" << source->nodeDescription()
                           << "' for mismatched "
                           << "queueInfo: [advisoryPartitionId: "
                           << advisory.partitionId()
                           << ", advisoryKey: " << key
                           << ", internalPartitionId: "
                           << assigned->partitionId()
                           << ", internalKey: " << assigned->key() << "]";
            continue;  // CONTINUE
        }
        d_state_p->unassignQueue(uri);
    }
}

void ClusterStateManager::processLeaderSyncDataQuery(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(message.choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(message.choice()
                         .clusterMessage()
                         .choice()
                         .isLeaderSyncDataQueryValue());

    // This query is sent by a passive leader ('source') to the follower having
    // latest view of the cluster (ie, having highest leader-message-sequence).
    // This query is preceded by a leaderSyncStateQuery request to all
    // AVAILABLE followers and their responses.  We don't check to see if
    // 'source' is really perceived as the leader by this node, because it
    // really doesn't matter.  If 'source' is not the leader anymore, it will
    // simply not process this response.

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = message.rId();

    if (bmqp_ctrlmsg::NodeStatus::E_AVAILABLE ==
        d_clusterData_p->membership().selfNodeStatus()) {
        bmqp_ctrlmsg::ClusterMessage& clusterMsg =
            controlMsg.choice().makeClusterMessage();
        bmqp_ctrlmsg::LeaderSyncDataQueryResponse& response =
            clusterMsg.choice().makeLeaderSyncDataQueryResponse();
        bmqp_ctrlmsg::LeaderAdvisory& leaderAdvisory =
            response.leaderSyncData();

        leaderAdvisory.sequenceNumber() =
            d_clusterData_p->electorInfo().leaderMessageSequence();

        // Populate partitions info.

        bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>& partitions =
            leaderAdvisory.partitions();

        for (size_t pid = 0; pid < d_state_p->partitions().size(); ++pid) {
            const mqbc::ClusterStatePartitionInfo& pinfo =
                d_state_p->partition(pid);

            // Only send those partitions which are not orphan.

            if (0 == pinfo.primaryNode()) {
                continue;  // CONTINUE
            }

            bmqp_ctrlmsg::PartitionPrimaryInfo pmi;
            pmi.partitionId()    = static_cast<int>(pid);
            pmi.primaryNodeId()  = pinfo.primaryNode()->nodeId();
            pmi.primaryLeaseId() = pinfo.primaryLeaseId();

            partitions.push_back(pmi);
        }

        // Populate queues info
        mqbc::ClusterUtil::loadQueuesInfo(&leaderAdvisory.queues(),
                                          *d_state_p);
    }
    else {
        // Self is not available.

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category() = bmqp_ctrlmsg::StatusCategory::E_NOT_READY;
        status.code()     = -1;
        status.message()  = "Peer is not available.";
    }

    d_clusterData_p->messageTransmitter().sendMessage(controlMsg, source);

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": sent response to leader sync data query from "
                  << source->nodeDescription();
}

void ClusterStateManager::processFollowerLSNRequest(
    BSLA_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLA_UNUSED mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_OPT(false && "This method should only be invoked in CSL mode");
}

void ClusterStateManager::processFollowerClusterStateRequest(
    BSLA_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLA_UNUSED mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_OPT(false && "This method should only be invoked in CSL mode");
}

void ClusterStateManager::processRegistrationRequest(
    BSLA_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLA_UNUSED mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_OPT(false && "This method should only be invoked in CSL mode");
}

void ClusterStateManager::processClusterStateEvent(
    const mqbi::DispatcherClusterStateEvent& event)
{
    // executed by *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    mqbnet::ClusterNode* source = event.clusterNode();
    bmqp::Event          rawEvent(event.blob().get(), d_allocator_p);
    BSLS_ASSERT_SAFE(rawEvent.isClusterStateEvent());

    const int rc = d_clusterStateLedger_mp->apply(*rawEvent.blob(), source);
    if (rc != 0) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << ": Failed to apply cluster state event, rc: " << rc;
    }
}

void ClusterStateManager::processPartitionPrimaryAdvisory(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(message.choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(message.choice()
                         .clusterMessage()
                         .choice()
                         .isPartitionPrimaryAdvisoryValue());

    const bmqp_ctrlmsg::PartitionPrimaryAdvisory& advisory =
        message.choice().clusterMessage().choice().partitionPrimaryAdvisory();

    if (d_clusterConfig.clusterAttributes().isCSLModeEnabled()) {
        BALL_LOG_ERROR << "#CSL_MODE_MIX "
                       << "Received legacy partitionPrimaryAdvisory: "
                       << advisory << " from: " << source << " in CSL mode.";

        return;  // RETURN
    }

    if (source != d_clusterData_p->electorInfo().leaderNode()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": ignoring partition-primary advisory: " << advisory
                      << " from cluster node " << source->nodeDescription()
                      << " as this node is not the current perceived leader. "
                      << "Current leader: "
                      << d_clusterData_p->electorInfo().leaderNodeId();
        return;  // RETURN
    }
    // 'source' is the perceived leader

    const bmqp_ctrlmsg::LeaderMessageSequence& leaderMsgSeq =
        advisory.sequenceNumber();

    if (d_clusterData_p->electorInfo().leaderMessageSequence() >
        leaderMsgSeq) {
        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << d_clusterData_p->identity().description()
            << ": Got partition-primary advisory: " << advisory
            << " from leader node " << source->nodeDescription()
            << " with smaller leader message sequence: " << leaderMsgSeq
            << ". Current value: "
            << d_clusterData_p->electorInfo().leaderMessageSequence()
            << ". Ignoring this advisory." << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    processPartitionPrimaryAdvisoryRaw(advisory.partitions(), source);

    // Leader status and sequence number are updated unconditionally.
    d_clusterData_p->electorInfo().setLeaderMessageSequence(leaderMsgSeq);
    d_clusterData_p->electorInfo().setLeaderStatus(
        mqbc::ElectorInfoLeaderStatus::e_ACTIVE);
}

void ClusterStateManager::processLeaderAdvisory(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT(message.choice().isClusterMessageValue());
    BSLS_ASSERT(
        message.choice().clusterMessage().choice().isLeaderAdvisoryValue());

    BALL_LOG_INFO << d_cluster_p->description()
                  << ": Processing leaderAdvisory message: " << message
                  << ", from '" << source->nodeDescription() << "'";

    const bmqp_ctrlmsg::LeaderAdvisory& advisory =
        message.choice().clusterMessage().choice().leaderAdvisory();

    if (d_clusterConfig.clusterAttributes().isCSLModeEnabled()) {
        BALL_LOG_ERROR << "#CSL_MODE_MIX "
                       << "Received legacy leaderAdvisory: " << advisory
                       << " from: " << source << " in CSL mode.";

        return;  // RETURN
    }

    if (d_clusterData_p->electorInfo().leaderNode() != source) {
        // Different leader.  Ignore message.
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": ignoring leader advisory: " << advisory << " from "
                      << source->nodeDescription()
                      << " because there is a different leader: "
                      << d_clusterData_p->electorInfo().leaderNodeId()
                      << " with term: "
                      << d_clusterData_p->electorInfo().electorTerm();
        return;  // RETURN
    }

    const bmqp_ctrlmsg::LeaderMessageSequence& leaderMsgSeq =
        advisory.sequenceNumber();
    if (d_clusterData_p->electorInfo().leaderMessageSequence() >
        leaderMsgSeq) {
        BMQTSK_ALARMLOG_ALARM("CLUSTER_STATE")
            << d_clusterData_p->identity().description()
            << ": got leader advisory: " << advisory << " from leader node ["
            << source->nodeDescription()
            << " with smaller leader message sequence: " << leaderMsgSeq
            << ". Current value: "
            << d_clusterData_p->electorInfo().leaderMessageSequence()
            << ". Ignoring this advisory." << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    // Process partition-primary mapping.  It's ok to process partition-primary
    // mapping advisory irrespective of self's status.

    processPartitionPrimaryAdvisoryRaw(advisory.partitions(), source);

    // Process (QueueUri, QueueKey, PartitionId) mapping (removed)
    // this has been done through CSL

    // Leader status and sequence number are updated unconditionally.  It may
    // have been updated by one of the routines called earlier in this method,
    // but there is no harm in setting these values again.

    d_clusterData_p->electorInfo().setLeaderMessageSequence(leaderMsgSeq);
    d_clusterData_p->electorInfo().setLeaderStatus(
        mqbc::ElectorInfoLeaderStatus::e_ACTIVE);
}

void ClusterStateManager::processShutdownEvent()
{
    // executed by *ANY* thread

    // NOTHING
}

void ClusterStateManager::onNodeUnavailable(
    BSLA_UNUSED mqbnet::ClusterNode* node)
{
    BSLS_ASSERT_OPT(false && "This method should only be invoked in CSL mode");
}

void ClusterStateManager::onNodeStopping()
{
    BSLS_ASSERT_OPT(false && "This method should only be invoked in CSL mode");
}

void ClusterStateManager::onNodeStopped()
{
    BSLS_ASSERT_OPT(false && "This method should only be invoked in CSL mode");
}

// ACCESSORS
//   (virtual: mqbi::ClusterStateManager)
void ClusterStateManager::validateClusterStateLedger() const
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    mqbc::ClusterUtil::validateClusterStateLedger(d_cluster_p,
                                                  *d_clusterStateLedger_mp,
                                                  *d_state_p,
                                                  *d_clusterData_p,
                                                  d_allocator_p);
}

int ClusterStateManager::latestLedgerLSN(
    bmqp_ctrlmsg::LeaderMessageSequence* out) const
{
    return mqbc::ClusterUtil::latestLedgerLSN(out,
                                              *d_clusterStateLedger_mp,
                                              *d_clusterData_p);
}

}  // close package namespace
}  // close enterprise namespace
