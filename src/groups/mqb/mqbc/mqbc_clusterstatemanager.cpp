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

// mqbc_clusterstatemanager.cpp                                       -*-C++-*-
#include <mqbc_clusterstatemanager.h>

#include <mqbscm_version.h>
// MQB
#include <mqbc_clusterstateledgeriterator.h>
#include <mqbc_clusterutil.h>
#include <mqbi_cluster.h>
#include <mqbi_storagemanager.h>
#include <mqbnet_cluster.h>
#include <mqbu_exit.h>

// BMQ
#include <bmqp_event.h>
#include <bmqt_resultcode.h>

// BDE
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bmqp_protocol.h>
#include <bsl_vector.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbc {

// -------------------------
// class ClusterStateManager
// -------------------------

// CREATORS
ClusterStateManager::ClusterStateManager(
    const mqbcfg::ClusterDefinition& clusterConfig,
    mqbi::Cluster*                   cluster,
    mqbc::ClusterData*               clusterData,
    mqbc::ClusterState*              clusterState,
    ClusterStateLedgerMp             clusterStateLedger,
    bsls::Types::Int64               watchDogTimeoutDuration,
    bslma::Allocator*                allocator)
: d_allocator_p(allocator)
, d_allocators(d_allocator_p)
, d_watchDogEventHandle()
, d_watchDogTimeoutInterval(watchDogTimeoutDuration)
, d_clusterConfig(clusterConfig)
, d_cluster_p(cluster)
, d_clusterData_p(clusterData)
, d_state_p(clusterState)
, d_clusterFSM(*this)
, d_nodeToLedgerLSNMap(allocator)
, d_lsnQuorum((clusterConfig.nodes().size() / 2) + 1)
// TODO Add cluster config to determine Eventual vs Strong
, d_clusterStateLedger_mp(clusterStateLedger)
, d_storageManager_p(0)
, d_queueAssigningCb()
, d_afterPartitionPrimaryAssignmentCb()
{
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
    // NOTHING
}

// PRIVATE MANIPULATORS
//   (virtual: mqbc::ClusterStateTableActions)
void ClusterStateManager::do_abort(const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!args->empty());

    const ClusterFSM::Event::Enum& event = args->front().first;

    BALL_LOG_ERROR << d_clusterData_p->identity().description()
                   << "Encountered nonsensical input event [" << event
                   << "] to the current state [" << d_clusterFSM.state()
                   << "] in the Cluster FSM! Aborting.";

    mqbu::ExitUtil::terminate(mqbu::ExitCode::e_REQUESTED);  // EXIT
}

void ClusterStateManager::do_startWatchDog(
    BSLS_ANNOTATION_UNUSED const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    d_clusterData_p->scheduler()->scheduleEvent(
        &d_watchDogEventHandle,
        d_clusterData_p->scheduler()->now() + d_watchDogTimeoutInterval,
        bdlf::BindUtil::bind(&ClusterStateManager::onWatchDog, this));
}

void ClusterStateManager::do_stopWatchDog(
    BSLS_ANNOTATION_UNUSED const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    if (d_clusterData_p->scheduler()->cancelEvent(d_watchDogEventHandle) !=
        0) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << ": Failed to cancel WatchDog.";
    }
}

void ClusterStateManager::do_triggerWatchDog(
    BSLS_ANNOTATION_UNUSED const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    if (d_clusterData_p->scheduler()->rescheduleEvent(
            d_watchDogEventHandle,
            d_clusterData_p->scheduler()->now()) != 0) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << ": Failed to trigger WatchDog.";
    }
}

void ClusterStateManager::do_applyCSLSelf(const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(d_clusterData_p->electorInfo().isSelfLeader() &&
                     d_clusterFSM.isSelfLeader());

    const ClusterFSMEventMetadata& metadata = args->front().second;
    BSLS_ASSERT_SAFE(metadata.highestLSNNode());

    bmqp_ctrlmsg::LeaderAdvisory clusterStateSnapshot;
    ClusterState                 tempState(
        d_cluster_p,
        d_cluster_p->clusterConfig()->partitionConfig().numPartitions(),
        d_allocator_p);

    const bool selfHighestLSN =
        metadata.highestLSNNode()->nodeId() ==
        d_clusterData_p->membership().selfNode()->nodeId();
    if (selfHighestLSN) {
        // Verify that self ledger's term is less than or equal to the current
        // elector term
        bmqp_ctrlmsg::LeaderMessageSequence selfLedgerLSN;
        latestLedgerLSN(&selfLedgerLSN);
        BSLS_ASSERT_SAFE(selfLedgerLSN.electorTerm() <=
                         d_clusterData_p->electorInfo().electorTerm());

        BALL_LOG_INFO << "#TEMP_STATE "
                      << d_clusterData_p->identity().description()
                      << ": Loading snapshot into temporary cluster state from"
                      << " CSL iterator.";

        const int rc = loadClusterStateSnapshot(&tempState);
        if (rc != 0) {
            BALL_LOG_ERROR << d_clusterData_p->identity().description()
                           << ": Failed to load cluster state from CSL "
                           << "iterator, rc: " << rc;

            applyFSMEvent(ClusterFSM::Event::e_CSL_CMT_FAIL,
                          ClusterFSMEventMetadata(d_allocator_p));
            return;  // RETURN
        }

        ClusterUtil::loadQueuesInfo(&clusterStateSnapshot.queues(),
                                    tempState,
                                    true);  // includeAppIds
    }
    else {
        // Verify that elector term in follower snapshot is less than the
        // current elector term
        BSLS_ASSERT_SAFE(
            metadata.clusterStateSnapshot().sequenceNumber().electorTerm() <
            d_clusterData_p->electorInfo().electorTerm());

        clusterStateSnapshot = metadata.clusterStateSnapshot();

        BALL_LOG_INFO << "#TEMP_STATE "
                      << d_clusterData_p->identity().description()
                      << ": Loading follower snapshot into temporary cluster "
                      << "state from " << metadata.highestLSNNode() << ".";

        bmqp_ctrlmsg::ClusterMessage clusterMessage;
        clusterMessage.choice().makeLeaderAdvisory(clusterStateSnapshot);
        ClusterUtil::apply(&tempState, clusterMessage, *d_clusterData_p);

        // Input partition vector must be empty when calling
        // 'ClusterUtil::assignPartitions'
        clusterStateSnapshot.partitions().clear();
    }

    d_clusterData_p->electorInfo().nextLeaderMessageSequence(
        &clusterStateSnapshot.sequenceNumber());

    if (d_clusterFSM.isSelfLeader() && d_clusterFSM.isSelfHealed()) {
        // Self is healed leader. We are simply broadcasting cluster state
        // snapshot to newcoming follower nodes. No need to re-assign
        // partitions.
        ClusterUtil::loadPartitionsInfo(&clusterStateSnapshot.partitions(),
                                        tempState);
    }
    else {
        BALL_LOG_INFO << "#TEMP_STATE "
                      << d_clusterData_p->identity().description()
                      << ": Assigning partitions into temporary cluster "
                      << "state.";

        ClusterUtil::assignPartitions(&clusterStateSnapshot.partitions(),
                                      &tempState,
                                      d_clusterConfig.masterAssignment(),
                                      *d_clusterData_p,
                                      true);  // isCSLMode
    }

    // Apply the new leader's first advisory
    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Applying cluster state snapshot from "
                  << (selfHighestLSN
                          ? "self"
                          : metadata.highestLSNNode()->nodeDescription())
                  << " to self: " << clusterStateSnapshot;
    d_clusterStateLedger_mp->apply(clusterStateSnapshot);
}

void ClusterStateManager::do_initializeQueueKeyInfoMap(
    BSLS_ANNOTATION_UNUSED const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(d_clusterFSM.isSelfHealed());

    d_storageManager_p->initializeQueueKeyInfoMap(*d_state_p);
}

void ClusterStateManager::do_sendFollowerLSNRequests(
    BSLS_ANNOTATION_UNUSED const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(d_clusterData_p->electorInfo().isSelfLeader() &&
                     d_clusterFSM.isSelfLeader());

    if (d_clusterData_p->cluster()->isLocal()) {
        return;  // RETURN
    }

    bsl::vector<mqbnet::ClusterNode*> followers;
    ClusterUtil::loadPeerNodes(&followers, *d_clusterData_p);
    BSLS_ASSERT_SAFE(!followers.empty());

    MultiRequestContextSp contextSp =
        d_clusterData_p->multiRequestManager().createRequestContext();

    contextSp->request()
        .choice()
        .makeClusterMessage()
        .choice()
        .makeClusterStateFSMMessage()
        .choice()
        .makeFollowerLSNRequest();

    contextSp->setDestinationNodes(followers);
    contextSp->setResponseCb(
        bdlf::BindUtil::bind(&ClusterStateManager::onFollowerLSNResponse,
                             this,
                             bdlf::PlaceHolders::_1));

    d_clusterData_p->multiRequestManager().sendRequest(contextSp,
                                                       bsls::TimeInterval(10));
}

void ClusterStateManager::do_sendFollowerLSNResponse(
    const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());
    BSLS_ASSERT_SAFE(!d_clusterData_p->electorInfo().isSelfLeader() &&
                     d_clusterFSM.isSelfFollower());

    const ClusterFSMEventMetadata& metadata = args->front().second;
    BSLS_ASSERT_SAFE(metadata.inputMessages().size() == 1);
    const InputMessage& inputMessage = metadata.inputMessages().at(0);

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = inputMessage.requestId();

    bmqp_ctrlmsg::ClusterMessage& clusterMsg =
        controlMsg.choice().makeClusterMessage();
    bmqp_ctrlmsg::ClusterStateFSMMessage& clusterFSMMessage =
        clusterMsg.choice().makeClusterStateFSMMessage();
    bmqp_ctrlmsg::FollowerLSNResponse& response =
        clusterFSMMessage.choice().makeFollowerLSNResponse();
    latestLedgerLSN(&response.sequenceNumber());

    d_clusterData_p->messageTransmitter().sendMessage(controlMsg,
                                                      inputMessage.source());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": sent response " << controlMsg
                  << " to follower LSN request from "
                  << inputMessage.source()->nodeDescription();
}

void ClusterStateManager::do_sendFailureFollowerLSNResponse(
    const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());

    const ClusterFSMEventMetadata& metadata = args->front().second;
    BSLS_ASSERT_SAFE(metadata.inputMessages().size() == 1);
    const InputMessage& inputMessage = metadata.inputMessages().at(0);

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = inputMessage.requestId();

    bmqp_ctrlmsg::Status& response = controlMsg.choice().makeStatus();
    response.category()            = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
    response.code()                = mqbi::ClusterErrorCode::e_NOT_FOLLOWER;
    response.message()             = "Not a follower";

    d_clusterData_p->messageTransmitter().sendMessage(controlMsg,
                                                      inputMessage.source());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": sent failure response " << controlMsg
                  << " to follower LSN request from "
                  << inputMessage.source()->nodeDescription();
}

void ClusterStateManager::do_findHighestLSN(
    BSLS_ANNOTATION_UNUSED const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(d_clusterData_p->electorInfo().isSelfLeader() &&
                     d_clusterFSM.isSelfLeader());

    mqbnet::ClusterNode* selfNode = d_clusterData_p->membership().selfNode();
    mqbnet::ClusterNode* highestLSNNode            = selfNode;
    bmqp_ctrlmsg::LeaderMessageSequence highestLSN = d_nodeToLedgerLSNMap.at(
        selfNode);
    for (NodeToLSNMapCIter cit = d_nodeToLedgerLSNMap.cbegin();
         cit != d_nodeToLedgerLSNMap.cend();
         ++cit) {
        if (cit->second > highestLSN) {
            highestLSNNode = cit->first;
            highestLSN     = cit->second;
        }
    }

    ClusterFSMEventMetadata metadata(highestLSNNode);
    if (highestLSNNode->nodeId() ==
        d_clusterData_p->membership().selfNode()->nodeId()) {
        applyFSMEvent(ClusterFSM::Event::e_SELF_HIGHEST_LSN, metadata);
    }
    else {
        applyFSMEvent(ClusterFSM::Event::e_FOL_HIGHEST_LSN, metadata);
    }
}

void ClusterStateManager::do_sendFollowerClusterStateRequest(
    const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());
    BSLS_ASSERT_SAFE(d_clusterData_p->electorInfo().isSelfLeader() &&
                     d_clusterFSM.isSelfLeader());

    const ClusterFSMEventMetadata& metadata = args->front().second;
    BSLS_ASSERT_SAFE(metadata.highestLSNNode());
    BSLS_ASSERT_SAFE(metadata.highestLSNNode()->nodeId() !=
                     d_clusterData_p->membership().selfNode()->nodeId());

    RequestContextSp request =
        d_clusterData_p->requestManager().createRequest();
    request->request()
        .choice()
        .makeClusterMessage()
        .choice()
        .makeClusterStateFSMMessage()
        .choice()
        .makeFollowerClusterStateRequest();

    request->setResponseCb(bdlf::BindUtil::bind(
        &ClusterStateManager::onFollowerClusterStateResponse,
        this,
        metadata.highestLSNNode(),
        bdlf::PlaceHolders::_1));

    bmqt::GenericResult::Enum rc = d_cluster_p->sendRequest(
        request,
        metadata.highestLSNNode(),
        bsls::TimeInterval(10));

    if (rc != bmqt::GenericResult::e_SUCCESS) {
        InputMessages inputMessages(1, d_allocator_p);
        inputMessages.at(0).setSource(
            d_clusterData_p->membership().selfNode());

        args->emplace(ClusterFSM::Event::e_FAIL_FOL_CSL_RSPN,
                      ClusterFSMEventMetadata(inputMessages,
                                              metadata.highestLSNNode()));
    }
}

void ClusterStateManager::do_sendFollowerClusterStateResponse(
    const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());
    BSLS_ASSERT_SAFE(!d_clusterData_p->electorInfo().isSelfLeader() &&
                     d_clusterFSM.isSelfFollower());

    const ClusterFSMEventMetadata& metadata = args->front().second;
    BSLS_ASSERT_SAFE(metadata.inputMessages().size() == 1);
    const InputMessage& inputMessage = metadata.inputMessages().at(0);

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = inputMessage.requestId();

    bmqp_ctrlmsg::FollowerClusterStateResponse& response =
        controlMsg.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeFollowerClusterStateResponse();
    bmqp_ctrlmsg::LeaderAdvisory& leaderAdvisory =
        response.clusterStateSnapshot();
    latestLedgerLSN(&leaderAdvisory.sequenceNumber());

    const int rc = loadClusterStateSnapshot(&leaderAdvisory);
    if (rc != 0) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << ": Failed to load cluster state from CSL iterator, "
                       << "rc: " << rc;
        return;  // RETURN
    }

    d_clusterData_p->messageTransmitter().sendMessage(controlMsg,
                                                      inputMessage.source());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": sent response " << controlMsg
                  << " to follower cluster state request from "
                  << inputMessage.source()->nodeDescription();
}

void ClusterStateManager::do_sendFailureFollowerClusterStateResponse(
    const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());

    const ClusterFSMEventMetadata& metadata = args->front().second;
    BSLS_ASSERT_SAFE(metadata.inputMessages().size() == 1);
    const InputMessage& inputMessage = metadata.inputMessages().at(0);

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = inputMessage.requestId();

    bmqp_ctrlmsg::Status& response = controlMsg.choice().makeStatus();
    response.category()            = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
    response.code()                = mqbi::ClusterErrorCode::e_NOT_FOLLOWER;
    response.message()             = "Not a follower";

    d_clusterData_p->messageTransmitter().sendMessage(controlMsg,
                                                      inputMessage.source());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": sent failure response " << controlMsg
                  << " to follower CSL request from "
                  << inputMessage.source()->nodeDescription();
}

void ClusterStateManager::do_storeSelfLSN(const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(d_clusterData_p->electorInfo().isSelfLeader() &&
                     d_clusterFSM.isSelfLeader());

    const ClusterFSMEventMetadata& metadata = args->front().second;
    BSLS_ASSERT_SAFE(metadata.inputMessages().size() == 1);
    const InputMessage& inputMessage = metadata.inputMessages().at(0);
    BSLS_ASSERT_SAFE(inputMessage.source()->nodeId() ==
                     d_clusterData_p->membership().selfNode()->nodeId());

    bmqp_ctrlmsg::LeaderMessageSequence selfLSN;
    latestLedgerLSN(&selfLSN);
    d_nodeToLedgerLSNMap[inputMessage.source()] = selfLSN;

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": In the Cluster FSM, storing self's LSN as " << selfLSN;
}

void ClusterStateManager::do_storeFollowerLSNs(const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());
    BSLS_ASSERT_SAFE(d_clusterData_p->electorInfo().isSelfLeader() &&
                     d_clusterFSM.isSelfLeader());

    const ClusterFSMEventMetadata& metadata = args->front().second;

    for (InputMessagesCIter cit = metadata.inputMessages().cbegin();
         cit != metadata.inputMessages().cend();
         ++cit) {
        d_nodeToLedgerLSNMap[cit->source()] = cit->leaderSequenceNumber();

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": In the Cluster FSM, storing the LSN of "
                      << cit->source()->nodeDescription() << " as "
                      << cit->leaderSequenceNumber();
    }
}

void ClusterStateManager::do_removeFollowerLSN(const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());
    BSLS_ASSERT_SAFE(d_clusterData_p->electorInfo().isSelfLeader() &&
                     d_clusterFSM.isSelfLeader());

    const ClusterFSMEventMetadata& metadata  = args->front().second;
    mqbnet::ClusterNode* crashedFollowerNode = metadata.crashedFollowerNode();

    NodeToLSNMapCIter cit = d_nodeToLedgerLSNMap.find(crashedFollowerNode);
    if (cit == d_nodeToLedgerLSNMap.cend()) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << " In the cluster FSM, failed to remove the LSN of "
                       << crashedFollowerNode->nodeDescription()
                       << " because it was not recorded.";
    }

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": In the Cluster FSM, removing the LSN of "
                  << crashedFollowerNode->nodeDescription() << ", which was "
                  << cit->second;

    d_nodeToLedgerLSNMap.erase(cit);
}

void ClusterStateManager::do_checkLSNQuorum(const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(d_clusterData_p->electorInfo().isSelfLeader() &&
                     d_clusterFSM.isSelfLeader());

    if (d_nodeToLedgerLSNMap.size() >= d_lsnQuorum) {
        // If we have a quorum of LSNs (including self LSN)

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": Achieved a quorum of LSNs with a count of "
                      << d_nodeToLedgerLSNMap.size();

        args->emplace(ClusterFSM::Event::e_QUORUM_LSN,
                      ClusterFSMEventMetadata(d_allocator_p));
    }
    else if (d_clusterFSM.state() == ClusterFSM::State::e_LDR_HEALING_STG2) {
        // Lost Quorum

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": Lost quorum of LSNs. New count is "
                      << d_nodeToLedgerLSNMap.size();

        args->emplace(ClusterFSM::Event::e_LOST_QUORUM_LSN,
                      ClusterFSMEventMetadata(d_allocator_p));
    }
}

void ClusterStateManager::do_sendRegistrationRequest(
    const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());
    BSLS_ASSERT_SAFE(!d_clusterData_p->electorInfo().isSelfLeader() &&
                     d_clusterFSM.isSelfFollower());

    RequestContextSp request =
        d_clusterData_p->requestManager().createRequest();

    bmqp_ctrlmsg::RegistrationRequest& registrationRequest =
        request->request()
            .choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeRegistrationRequest();
    latestLedgerLSN(&registrationRequest.sequenceNumber());

    mqbnet::ClusterNode* destNode =
        d_clusterData_p->electorInfo().leaderNode();
    BSLS_ASSERT_SAFE(destNode);

    request->setResponseCb(
        bdlf::BindUtil::bind(&ClusterStateManager::onRegistrationResponse,
                             this,
                             destNode,
                             bdlf::PlaceHolders::_1));

    bmqt::GenericResult::Enum rc =
        d_cluster_p->sendRequest(request, destNode, bsls::TimeInterval(10));

    if (rc != bmqt::GenericResult::e_SUCCESS) {
        InputMessages inputMessages(1, d_allocator_p);
        inputMessages.at(0)
            .setSource(d_clusterData_p->membership().selfNode())
            .setLeaderSequenceNumber(registrationRequest.sequenceNumber());

        args->emplace(ClusterFSM::Event::e_FAIL_REGISTRATION_RSPN,
                      ClusterFSMEventMetadata(inputMessages));
    }
}

void ClusterStateManager::do_sendRegistrationResponse(
    const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());
    BSLS_ASSERT_SAFE(d_clusterData_p->electorInfo().isSelfLeader() &&
                     d_clusterFSM.isSelfLeader());

    const ClusterFSMEventMetadata& metadata = args->front().second;
    BSLS_ASSERT_SAFE(metadata.inputMessages().size() == 1);
    const InputMessage& inputMessage = metadata.inputMessages().at(0);

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = inputMessage.requestId();
    controlMsg.choice()
        .makeClusterMessage()
        .choice()
        .makeClusterStateFSMMessage()
        .choice()
        .makeRegistrationResponse();

    d_clusterData_p->messageTransmitter().sendMessage(controlMsg,
                                                      inputMessage.source());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": sent response " << controlMsg
                  << " to registration request from "
                  << inputMessage.source()->nodeDescription();
}

void ClusterStateManager::do_sendFailureRegistrationResponse(
    const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());

    const ClusterFSMEventMetadata& metadata = args->front().second;
    BSLS_ASSERT_SAFE(metadata.inputMessages().size() == 1);
    const InputMessage& inputMessage = metadata.inputMessages().at(0);

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = inputMessage.requestId();

    bmqp_ctrlmsg::Status& response = controlMsg.choice().makeStatus();
    response.category()            = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
    response.code()                = mqbi::ClusterErrorCode::e_NOT_LEADER;
    response.message()             = "Not a leader";

    d_clusterData_p->messageTransmitter().sendMessage(controlMsg,
                                                      inputMessage.source());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": sent failure response " << controlMsg
                  << " to registration request from "
                  << inputMessage.source()->nodeDescription();
}

void ClusterStateManager::do_logStaleFollowerLSNResponse(
    const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());
    BSLS_ASSERT_SAFE(!d_clusterData_p->electorInfo().isSelfLeader() &&
                     !d_clusterFSM.isSelfLeader());
    // Response is not stale if self is leader

    const ClusterFSMEventMetadata& metadata = args->front().second;

    for (InputMessagesCIter cit = metadata.inputMessages().cbegin();
         cit != metadata.inputMessages().cend();
         ++cit) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": Follower LSN response from "
                      << cit->source()->nodeDescription()
                      << " with rId = " << cit->requestId() << " and a LSN of "
                      << cit->leaderSequenceNumber()
                      << " is stale.  Self is no longer leader.";
    }
}

void ClusterStateManager::do_logStaleFollowerClusterStateResponse(
    const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());
    BSLS_ASSERT_SAFE(d_clusterFSM.state() !=
                     ClusterFSM::State::e_LDR_HEALING_STG2);

    const ClusterFSMEventMetadata& metadata = args->front().second;
    BSLS_ASSERT_SAFE(metadata.inputMessages().size() == 1);
    const InputMessage& inputMessage = metadata.inputMessages().at(0);

    BALL_LOG_WARN << d_clusterData_p->identity().description()
                  << ": Follower CSL response from "
                  << inputMessage.source()->nodeDescription()
                  << " with rId = " << inputMessage.requestId()
                  << " is stale.  Contained snapshot: "
                  << metadata.clusterStateSnapshot();
}

void ClusterStateManager::do_logErrorLeaderNotHealed(
    const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());
    BSLS_ASSERT_SAFE(!d_clusterData_p->electorInfo().isSelfLeader());
    BSLS_ASSERT_SAFE(d_clusterFSM.state() == ClusterFSM::State::e_FOL_HEALING);

    const ClusterFSMEventMetadata& metadata = args->front().second;
    BSLS_ASSERT_SAFE(metadata.inputMessages().size() == 1);
    const InputMessage& inputMessage = metadata.inputMessages().at(0);
    BSLS_ASSERT_SAFE(inputMessage.source()->nodeId() ==
                     d_clusterData_p->electorInfo().leaderNodeId());

    BALL_LOG_ERROR << d_clusterData_p->identity().description()
                   << ": Self detecting leader: "
                   << inputMessage.source()->nodeDescription()
                   << " as not healed.  Transitioning self from healed "
                   << " follower to healing follower.";
}

void ClusterStateManager::do_logFailFollowerLSNResponses(
    const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());
    BSLS_ASSERT_SAFE(d_clusterData_p->electorInfo().isSelfLeader() &&
                     d_clusterFSM.isSelfLeader());

    const ClusterFSMEventMetadata& metadata = args->front().second;
    for (InputMessagesCIter cit = metadata.inputMessages().cbegin();
         cit != metadata.inputMessages().cend();
         ++cit) {
        BSLS_ASSERT_SAFE(cit->source());

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": Received failure follower LSN response from "
                      << cit->source()->nodeDescription();
    }
}

void ClusterStateManager::do_logFailFollowerClusterStateResponse(
    const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());
    BSLS_ASSERT_SAFE(d_clusterData_p->electorInfo().isSelfLeader() &&
                     d_clusterFSM.isSelfLeader());

    const ClusterFSMEventMetadata& metadata = args->front().second;
    BSLS_ASSERT_SAFE(metadata.inputMessages().size() == 1);

    const InputMessage& inputMessage = metadata.inputMessages().at(0);

    BSLS_ASSERT_SAFE(inputMessage.source());
    if (inputMessage.source()->nodeId() ==
        d_clusterData_p->membership().selfNode()->nodeId()) {
        // We failed to send the request

        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << ": Failed to send follower CSL request to "
                       << metadata.highestLSNNode();
    }
    else {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": Received failed registration response from "
                      << inputMessage.source()->nodeDescription()
                      << " for request with rId = "
                      << inputMessage.requestId();
    }
}

void ClusterStateManager::do_logFailRegistrationResponse(
    const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());
    BSLS_ASSERT_SAFE(!d_clusterData_p->electorInfo().isSelfLeader() &&
                     d_clusterFSM.isSelfFollower());

    const ClusterFSMEventMetadata& metadata = args->front().second;
    BSLS_ASSERT_SAFE(metadata.inputMessages().size() == 1);

    const InputMessage& inputMessage = metadata.inputMessages().at(0);

    BSLS_ASSERT_SAFE(inputMessage.source());
    if (inputMessage.source()->nodeId() ==
        d_clusterData_p->membership().selfNode()->nodeId()) {
        // We failed to send the request

        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << ": Failed to send registration request with LSN = "
                       << inputMessage.leaderSequenceNumber() << " to "
                       << d_clusterData_p->electorInfo().leaderNode();
    }
    else {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": Received failed registration response from "
                      << inputMessage.source() << " for request with rId = "
                      << inputMessage.requestId();
    }
}

void ClusterStateManager::do_reapplyEvent(const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!args->empty());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Re-apply event: " << args->front().first
                  << " in the Cluster FSM.";

    args->emplace(args->front());
}

void ClusterStateManager::do_reapplySelectLeader(const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(d_clusterData_p->electorInfo().isSelfLeader());
    BSLS_ASSERT_SAFE(d_clusterFSM.state() == ClusterFSM::State::e_UNKNOWN);

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Re-apply transition to leader in the Cluster FSM.";

    InputMessages inputMessages(1, d_allocator_p);
    inputMessages.at(0).setSource(d_clusterData_p->membership().selfNode());

    args->emplace(ClusterFSM::Event::e_SLCT_LDR,
                  ClusterFSMEventMetadata(inputMessages));
}

void ClusterStateManager::do_reapplySelectFollower(
    const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->electorInfo().isSelfLeader());
    BSLS_ASSERT_SAFE(d_clusterFSM.state() == ClusterFSM::State::e_UNKNOWN);

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Re-apply transition to follower in the Cluster FSM.";

    InputMessages inputMessages(1, d_allocator_p);
    inputMessages.at(0).setSource(d_clusterData_p->electorInfo().leaderNode());

    args->emplace(ClusterFSM::Event::e_SLCT_FOL,
                  ClusterFSMEventMetadata(inputMessages));
}

void ClusterStateManager::do_cleanupLSNs(
    BSLS_ANNOTATION_UNUSED const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    d_nodeToLedgerLSNMap.clear();
}

void ClusterStateManager::do_cancelRequests(
    BSLS_ANNOTATION_UNUSED const ClusterFSMArgsSp& args)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    bmqp_ctrlmsg::ControlMessage response;
    bmqp_ctrlmsg::Status&        failure = response.choice().makeStatus();
    failure.category() = bmqp_ctrlmsg::StatusCategory::E_CANCELED;
    failure.code()     = mqbi::ClusterErrorCode::e_UNKNOWN;
    failure.message()  = "Cancellation by Cluster FSM";

    d_clusterData_p->requestManager().cancelAllRequests(response);
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

    // This method will notify the storage about (potentially same) mapping.
    // This must be done before calling
    // 'ClusterQueueHelper::afterPartitionPrimaryAssignment' (via
    // d_afterPartitionPrimaryAssignmentCb), because ClusterQueueHelper assumes
    // that storage is aware of the mapping.
    ClusterUtil::onPartitionPrimaryAssignment(d_clusterData_p,
                                              d_storageManager_p,
                                              partitionId,
                                              primary,
                                              leaseId,
                                              status,
                                              oldPrimary,
                                              oldLeaseId);

    d_afterPartitionPrimaryAssignmentCb(partitionId, primary, status);
}

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

        applyFSMEvent(ClusterFSM::Event::e_CSL_CMT_FAIL,
                      ClusterFSMEventMetadata(d_allocator_p));

        return;  // RETURN
    }

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Committed advisory: " << advisory << ", with status '"
                  << status << "'";

    const bmqp_ctrlmsg::ClusterMessage& clusterMessage =
        advisory.choice().clusterMessage();
    mqbc::ClusterUtil::apply(d_state_p, clusterMessage, *d_clusterData_p);

    applyFSMEvent(ClusterFSM::Event::e_CSL_CMT_SUCCESS,
                  ClusterFSMEventMetadata(d_allocator_p));
}

void ClusterStateManager::applyFSMEvent(
    ClusterFSM::Event::Enum        event,
    const ClusterFSMEventMetadata& metadata)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    ClusterFSMArgsSp eventsQueueSp(new (*d_allocator_p)
                                       ClusterFSMArgs(d_allocator_p),
                                   d_allocator_p);
    eventsQueueSp->emplace(event, metadata);
    d_clusterFSM.applyEvent(eventsQueueSp);
}

int ClusterStateManager::loadClusterStateSnapshot(ClusterState* out)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(out);

    bslma::ManagedPtr<mqbc::ClusterStateLedgerIterator> cslIter =
        d_clusterStateLedger_mp->getIterator();

    return mqbc::ClusterUtil::load(out,
                                   cslIter.get(),
                                   *d_clusterData_p,
                                   d_allocator_p);
}

int ClusterStateManager::loadClusterStateSnapshot(
    bmqp_ctrlmsg::LeaderAdvisory* out)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(out);

    ClusterState tempState(
        d_cluster_p,
        d_cluster_p->clusterConfig()->partitionConfig().numPartitions(),
        d_allocator_p);

    BALL_LOG_INFO << "#TEMP_STATE "
                  << d_clusterData_p->identity().description()
                  << ": Loading snapshot into temporary cluster state from"
                  << " CSL iterator.";

    const int rc = loadClusterStateSnapshot(&tempState);
    if (rc != 0) {
        return rc;  // RETURN
    }

    mqbc::ClusterUtil::loadPartitionsInfo(&out->partitions(), tempState);
    mqbc::ClusterUtil::loadQueuesInfo(&out->queues(),
                                      tempState,
                                      true);  // includeAppIds

    return 0;
}

void ClusterStateManager::onWatchDog()
{
    // executed by the *SCHEDULER* thread

    dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterStateManager::onWatchDogDispatched, this),
        d_cluster_p);
}

void ClusterStateManager::onWatchDogDispatched()
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    if (d_clusterFSM.isSelfHealed()) {
        return;  // RETURN
    }

    BALL_LOG_WARN << d_clusterData_p->identity().description()
                  << ": Watch dog triggered because node startup healing "
                  << "sequence was not completed in the configured time of "
                  << d_watchDogTimeoutInterval.totalSeconds() << " seconds.";

    applyFSMEvent(ClusterFSM::Event::e_WATCH_DOG, ClusterFSMEventMetadata());
}

void ClusterStateManager::onFollowerLSNResponse(
    const MultiRequestContextSp& requestContext)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());
    BSLS_ASSERT_SAFE(d_clusterFSM.isSelfLeader() ||
                     (d_clusterFSM.state() == ClusterFSM::State::e_STOPPING) ||
                     (d_clusterFSM.state() == ClusterFSM::State::e_UNKNOWN));

    const NodeResponsePairs& pairs = requestContext->response();
    BSLS_ASSERT_SAFE(!pairs.empty());

    InputMessages failureResponses(d_allocator_p);
    InputMessages successResponses(d_allocator_p);
    for (NodeResponsePairsCIter cit = pairs.cbegin(); cit != pairs.cend();
         ++cit) {
        BSLS_ASSERT_SAFE(cit->first);

        if (cit->second.choice().isStatusValue()) {
            BALL_LOG_WARN << d_clusterData_p->identity().description()
                          << ": Received failed follower LSN response "
                          << cit->second.choice().status() << " from "
                          << cit->first->nodeDescription()
                          << ". Skipping this node's response.";

            InputMessage inputMessage;
            inputMessage.setSource(cit->first);

            failureResponses.emplace_back(inputMessage);

            continue;  // CONTINUE
        }

        BSLS_ASSERT_SAFE(cit->second.choice().isClusterMessageValue());
        BSLS_ASSERT_SAFE(cit->second.choice()
                             .clusterMessage()
                             .choice()
                             .isClusterStateFSMMessageValue());
        BSLS_ASSERT_SAFE(cit->second.choice()
                             .clusterMessage()
                             .choice()
                             .clusterStateFSMMessage()
                             .choice()
                             .isFollowerLSNResponseValue());

        const bmqp_ctrlmsg::FollowerLSNResponse& resp =
            cit->second.choice()
                .clusterMessage()
                .choice()
                .clusterStateFSMMessage()
                .choice()
                .followerLSNResponse();

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": Received follower LSN response " << resp
                      << " from " << cit->first->nodeDescription();

        InputMessage inputMessage;
        inputMessage.setSource(cit->first)
            .setLeaderSequenceNumber(resp.sequenceNumber());

        successResponses.emplace_back(inputMessage);
    }

    if (!failureResponses.empty()) {
        applyFSMEvent(ClusterFSM::Event::e_FAIL_FOL_LSN_RSPN,
                      ClusterFSMEventMetadata(failureResponses));
    }

    if (!successResponses.empty()) {
        mqbnet::ClusterNode* highestLSNNode =
            (d_clusterFSM.isSelfLeader() && d_clusterFSM.isSelfHealed())
                ? d_clusterData_p->membership().selfNode()
                : 0;
        applyFSMEvent(ClusterFSM::Event::e_FOL_LSN_RSPN,
                      ClusterFSMEventMetadata(successResponses,
                                              highestLSNNode));
    }
}

void ClusterStateManager::onRegistrationResponse(
    mqbnet::ClusterNode*    source,
    const RequestContextSp& requestContext)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());
    BSLS_ASSERT_SAFE(!d_clusterData_p->electorInfo().isSelfLeader());
    BSLS_ASSERT_SAFE(d_clusterFSM.isSelfFollower() ||
                     (d_clusterFSM.state() == ClusterFSM::State::e_STOPPING) ||
                     (d_clusterFSM.state() == ClusterFSM::State::e_UNKNOWN));
    BSLS_ASSERT_SAFE(source);

    InputMessages inputMessages(1, d_allocator_p);
    inputMessages.at(0).setSource(source).setRequestId(
        requestContext->request().rId().value());

    ClusterFSMEventMetadata metadata(inputMessages);

    if (requestContext->result() != bmqt::GenericResult::e_SUCCESS) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": Received failed registration response from "
                      << requestContext->nodeDescription()
                      << " for request with rId = "
                      << requestContext->request().rId().value()
                      << ", rc = " << requestContext->result();

        applyFSMEvent(ClusterFSM::Event::e_FAIL_REGISTRATION_RSPN, metadata);

        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(
        requestContext->response().choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(requestContext->response()
                         .choice()
                         .clusterMessage()
                         .choice()
                         .isClusterStateFSMMessageValue());
    BSLS_ASSERT_SAFE(requestContext->response()
                         .choice()
                         .clusterMessage()
                         .choice()
                         .clusterStateFSMMessage()
                         .choice()
                         .isRegistrationResponseValue());

    const bmqp_ctrlmsg::RegistrationResponse& resp =
        requestContext->response()
            .choice()
            .clusterMessage()
            .choice()
            .clusterStateFSMMessage()
            .choice()
            .registrationResponse();

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Received registration response " << resp << "from "
                  << requestContext->nodeDescription()
                  << " for request with rId = "
                  << requestContext->request().rId().value();

    applyFSMEvent(ClusterFSM::Event::e_REGISTRATION_RSPN, metadata);
}

void ClusterStateManager::onFollowerClusterStateResponse(
    mqbnet::ClusterNode*    source,
    const RequestContextSp& requestContext)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());
    BSLS_ASSERT_SAFE(d_clusterFSM.isSelfLeader() ||
                     (d_clusterFSM.state() == ClusterFSM::State::e_STOPPING) ||
                     (d_clusterFSM.state() == ClusterFSM::State::e_UNKNOWN));

    if (requestContext->result() != bmqt::GenericResult::e_SUCCESS) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": Received failed follower cluster state response "
                      << "from " << requestContext->nodeDescription()
                      << " for request with rId = "
                      << requestContext->request().rId().value()
                      << ", rc = " << requestContext->result();

        InputMessages inputMessages(1, d_allocator_p);
        inputMessages.at(0).setSource(source).setRequestId(
            requestContext->request().rId().value());

        applyFSMEvent(ClusterFSM::Event::e_FAIL_FOL_CSL_RSPN,
                      ClusterFSMEventMetadata(inputMessages, source, source));

        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(
        requestContext->response().choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(requestContext->response()
                         .choice()
                         .clusterMessage()
                         .choice()
                         .isClusterStateFSMMessageValue());
    BSLS_ASSERT_SAFE(requestContext->response()
                         .choice()
                         .clusterMessage()
                         .choice()
                         .clusterStateFSMMessage()
                         .choice()
                         .isFollowerClusterStateResponseValue());

    const bmqp_ctrlmsg::FollowerClusterStateResponse& resp =
        requestContext->response()
            .choice()
            .clusterMessage()
            .choice()
            .clusterStateFSMMessage()
            .choice()
            .followerClusterStateResponse();

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Received follower cluster state response " << resp
                  << " from " << requestContext->nodeDescription()
                  << " for request with rId = "
                  << requestContext->request().rId().value();

    InputMessages inputMessages(1, d_allocator_p);
    inputMessages.at(0).setSource(source).setRequestId(
        requestContext->request().rId().value());

    applyFSMEvent(ClusterFSM::Event::e_FOL_CSL_RSPN,
                  ClusterFSMEventMetadata(inputMessages,
                                          source,
                                          0,  // crashedFollowerNode
                                          resp.clusterStateSnapshot()));
}

// MANIPULATORS
//   (virtual: mqbi::ClusterStateManager)
int ClusterStateManager::start(bsl::ostream& errorDescription)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    d_clusterData_p->electorInfo().registerObserver(this);
    d_clusterFSM.registerObserver(&d_clusterData_p->electorInfo());

    // Start the cluster state ledger
    const int rc = d_clusterStateLedger_mp->open();
    if (rc != 0) {
        errorDescription << d_clusterData_p->identity().description()
                         << ": Failed to open cluster state ledger [rc: " << rc
                         << "]";
        return rc;  // RETURN
    }

    d_state_p->registerObserver(this);

    return 0;
}

void ClusterStateManager::stop()
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    d_clusterData_p->electorInfo().unregisterObserver(this);
    d_clusterFSM.unregisterObserver(&d_clusterData_p->electorInfo());

    const int rc = d_clusterStateLedger_mp->close();
    if (rc != 0) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << ": Failed to close cluster state ledger [rc: " << rc
                       << "]";
    }

    d_state_p->unregisterObserver(this);

    applyFSMEvent(ClusterFSM::Event::e_STOP_NODE, ClusterFSMEventMetadata());
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
    BSLS_ASSERT_SAFE(d_clusterFSM.isSelfLeader());
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());
    BSLS_ASSERT_SAFE(partitions && partitions->empty());

    ClusterUtil::assignPartitions(partitions,
                                  d_state_p,
                                  d_clusterConfig.masterAssignment(),
                                  *d_clusterData_p,
                                  true);  // isCSLMode
}

ClusterStateManager::QueueAssignmentResult::Enum
ClusterStateManager::assignQueue(const bmqt::Uri&      uri,
                                 bmqp_ctrlmsg::Status* status)
{
    // executed by the cluster *DISPATCHER* thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    return mqbc::ClusterUtil::assignQueue(d_state_p,
                                          d_clusterData_p,
                                          d_clusterStateLedger_mp.get(),
                                          d_cluster_p,
                                          uri,
                                          d_queueAssigningCb,
                                          d_allocator_p,
                                          status);
}

void ClusterStateManager::registerQueueInfo(const bmqt::Uri& uri,
                                            int              partitionId,
                                            const mqbu::StorageKey& queueKey,
                                            const AppIdInfos&       appIdInfos,
                                            bool forceUpdate)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    mqbc::ClusterUtil::registerQueueInfo(d_state_p,
                                         d_cluster_p,
                                         uri,
                                         partitionId,
                                         queueKey,
                                         appIdInfos,
                                         d_queueAssigningCb,
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
    BSLS_ASSERT_SAFE(d_clusterData_p->electorInfo().isSelfLeader() &&
                     d_clusterFSM.isSelfLeader());

    mqbc::ClusterUtil::sendClusterState(d_clusterData_p,
                                        d_clusterStateLedger_mp.get(),
                                        0,  // storageManager
                                        *d_state_p,
                                        sendPartitionPrimaryInfo,
                                        sendQueuesInfo,
                                        node,
                                        partitions);
}

void ClusterStateManager::registerAppId(const bsl::string&  appId,
                                        const mqbi::Domain* domain)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(domain);

    mqbc::ClusterUtil::registerAppId(d_clusterData_p,
                                     d_clusterStateLedger_mp.get(),
                                     *d_state_p,
                                     appId,
                                     domain,
                                     d_allocator_p);
}

void ClusterStateManager::unregisterAppId(const bsl::string&  appId,
                                          const mqbi::Domain* domain)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(domain);

    mqbc::ClusterUtil::unregisterAppId(d_clusterData_p,
                                       d_clusterStateLedger_mp.get(),
                                       *d_state_p,
                                       appId,
                                       domain,
                                       d_allocator_p);
}

void ClusterStateManager::initiateLeaderSync(BSLS_ANNOTATION_UNUSED bool wait)
{
    // While this method could be invoked by ClusterOrchestrator as part of
    // pre-CSL workflow, we can do a no-op here because the Cluster FSM logic
    // will be the replacement for syncing cluster state at startup.

    // NOTHING
}

void ClusterStateManager::processLeaderSyncStateQuery(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_SAFE(false &&
                     "This method should only be invoked in non-CSL mode");
}

void ClusterStateManager::processLeaderSyncDataQuery(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_SAFE(false &&
                     "This method should only be invoked in non-CSL mode");
}

void ClusterStateManager::processFollowerLSNRequest(
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
                         .isClusterStateFSMMessageValue());
    BSLS_ASSERT_SAFE(message.choice()
                         .clusterMessage()
                         .choice()
                         .clusterStateFSMMessage()
                         .choice()
                         .isFollowerLSNRequestValue());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Received Follower LSN Request: " << message << " from "
                  << source->nodeDescription();

    InputMessages inputMessages(1, d_allocator_p);
    inputMessages.at(0).setSource(source).setRequestId(message.rId().value());

    applyFSMEvent(ClusterFSM::Event::e_FOL_LSN_RQST,
                  ClusterFSMEventMetadata(inputMessages));
}

void ClusterStateManager::processFollowerClusterStateRequest(
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
                         .isClusterStateFSMMessageValue());
    BSLS_ASSERT_SAFE(message.choice()
                         .clusterMessage()
                         .choice()
                         .clusterStateFSMMessage()
                         .choice()
                         .isFollowerClusterStateRequestValue());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Received follower cluster state request: " << message
                  << " from " << source->nodeDescription();

    InputMessages inputMessages(1, d_allocator_p);
    inputMessages.at(0).setSource(source).setRequestId(message.rId().value());

    applyFSMEvent(ClusterFSM::Event::e_FOL_CSL_RQST,
                  ClusterFSMEventMetadata(inputMessages));
}

void ClusterStateManager::processRegistrationRequest(
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
                         .isClusterStateFSMMessageValue());
    BSLS_ASSERT_SAFE(message.choice()
                         .clusterMessage()
                         .choice()
                         .clusterStateFSMMessage()
                         .choice()
                         .isRegistrationRequestValue());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Received Registration Request: " << message << " from "
                  << source->nodeDescription();

    InputMessages inputMessages(1, d_allocator_p);
    inputMessages.at(0)
        .setSource(source)
        .setRequestId(message.rId().value())
        .setLeaderSequenceNumber(message.choice()
                                     .clusterMessage()
                                     .choice()
                                     .clusterStateFSMMessage()
                                     .choice()
                                     .registrationRequest()
                                     .sequenceNumber());

    mqbnet::ClusterNode* highestLSNNode =
        (d_clusterFSM.isSelfLeader() && d_clusterFSM.isSelfHealed())
            ? d_clusterData_p->membership().selfNode()
            : 0;
    applyFSMEvent(ClusterFSM::Event::e_REGISTRATION_RQST,
                  ClusterFSMEventMetadata(inputMessages, highestLSNNode));
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

    // NOTE: Any validation of the event would go here.
    if (source != d_clusterData_p->electorInfo().leaderNode()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": ignoring cluster state event from cluster node "
                      << source->nodeDescription() << " as this node is not "
                      << "the current perceived leader. Current leader: ["
                      << d_clusterData_p->electorInfo().leaderNodeId() << ": "
                      << (d_clusterData_p->electorInfo().leaderNode()
                              ? d_clusterData_p->electorInfo()
                                    .leaderNode()
                                    ->nodeDescription()
                              : "* UNKNOWN *")
                      << "]";
        return;  // RETURN
    }
    // 'source' is the perceived leader

    // TBD: Suppress the following check for now, which will help some
    // integration tests to pass.  At this point, it is not clear if it is safe
    // to process cluster state events while self is stopping.
    //
    // if (   bmqp_ctrlmsg::NodeStatus::E_STOPPING
    //     == d_clusterData_p->membership().selfNodeStatus()) {
    //     return;                                                    // RETURN
    // }

    // TODO: Validate the incoming advisory and potentially buffer it for later
    //       if the node is currently starting.

    const int rc = d_clusterStateLedger_mp->apply(*rawEvent.blob(), source);
    if (rc != 0) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << ": Failed to apply cluster state event, rc: " << rc;
    }
}

void ClusterStateManager::processBufferedQueueAdvisories()
{
    // While this method could be invoked by mqbblp::Cluster as part of
    // pre-CSL workflow, we can do a no-op here because there won't be any
    // buffered queue advisories anymore under the new CSL logic.

    // NOTHING
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
        d_queueAssigningCb,
        d_allocator_p);
}

void ClusterStateManager::processQueueAssignmentAdvisory(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source,
    BSLS_ANNOTATION_UNUSED bool                 delayed)
{
    BSLS_ASSERT_SAFE(false &&
                     "This method should only be invoked in non-CSL mode");
}

void ClusterStateManager::processQueueUnassignedAdvisory(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_SAFE(false &&
                     "This method should only be invoked in non-CSL mode");
}

void ClusterStateManager::processQueueUnAssignmentAdvisory(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source,
    BSLS_ANNOTATION_UNUSED bool                 delayed)
{
    BSLS_ASSERT_SAFE(false &&
                     "This method should only be invoked in non-CSL mode");
}

void ClusterStateManager::processPartitionPrimaryAdvisory(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_SAFE(false &&
                     "This method should only be invoked in non-CSL mode");
}

void ClusterStateManager::processLeaderAdvisory(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_SAFE(false &&
                     "This method should only be invoked in non-CSL mode");
}

void ClusterStateManager::processShutdownEvent()
{
    // executed by *ANY* thread

    applyFSMEvent(ClusterFSM::Event::e_STOP_NODE, ClusterFSMEventMetadata());
}

void ClusterStateManager::onNodeUnavailable(mqbnet::ClusterNode* node)
{
    (void)node;

    BSLS_ASSERT_SAFE(false && "NOT IMPLEMENTED!");
}

void ClusterStateManager::onNodeStopping()
{
    BSLS_ASSERT_SAFE(false && "NOT IMPLEMENTED!");
}

void ClusterStateManager::onNodeStopped()
{
    BSLS_ASSERT_SAFE(false && "NOT IMPLEMENTED!");
}

// MANIPULATORS
//   (virtual: mqbc::ElectorInfoObserver)
void ClusterStateManager::onClusterLeader(
    mqbnet::ClusterNode*   node,
    BSLS_ANNOTATION_UNUSED ElectorInfoLeaderStatus::Enum status)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    if (!node) {
        // Leader lost
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": Transitioning to **UNKNOWN** state in the Cluster "
                      << "FSM.";

        applyFSMEvent(ClusterFSM::Event::e_RST_UNKNOWN,
                      ClusterFSMEventMetadata());

        return;  // RETURN
    }

    InputMessages inputMessages(1, d_allocator_p);
    inputMessages.at(0).setSource(node);  // leader node

    if (d_clusterData_p->membership().selfNode()->nodeId() == node->nodeId()) {
        BALL_LOG_INFO << d_clusterData_p->identity().description() << ": "
                      << (d_clusterFSM.isSelfLeader() ? "Re-" : "")
                      << "Transitioning to leader in the Cluster FSM.";

        applyFSMEvent(ClusterFSM::Event::e_SLCT_LDR,
                      ClusterFSMEventMetadata(inputMessages));
    }
    else {
        BALL_LOG_INFO << d_clusterData_p->identity().description() << ": "
                      << (d_clusterFSM.isSelfFollower() ? "Re-" : "")
                      << "Transitioning to follower in the Cluster FSM.";

        applyFSMEvent(ClusterFSM::Event::e_SLCT_FOL,
                      ClusterFSMEventMetadata(inputMessages));
    }
}

// ACCESSORS
//   (virtual: mqbi::ClusterStateManager)
void ClusterStateManager::validateClusterStateLedger() const
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_clusterData_p->dispatcherClientData()
                         .dispatcher()
                         ->inDispatcherThread(d_cluster_p));

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
