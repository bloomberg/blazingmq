// Copyright 2023 Bloomberg Finance L.P.
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

// mqbc_clusterstatemanager.t.cpp                                     -*-C++-*-
#include <mqbc_clusterstatemanager.h>

// MQB
#include <mqbc_clusterdata.h>
#include <mqbc_clusterfsm.h>
#include <mqbc_clusterstateledger.h>
#include <mqbc_clusterstatetable.h>
#include <mqbc_clusterutil.h>
#include <mqbc_electorinfo.h>
#include <mqbi_cluster.h>
#include <mqbmock_cluster.h>
#include <mqbmock_clusterstateledger.h>
#include <mqbmock_storagemanager.h>
#include <mqbnet_cluster.h>
#include <mqbnet_elector.h>

// BMQ
#include <bmqp_crc32c.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocolutil.h>

#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>
#include <bmqu_tempdirectory.h>

// BDE
#include <bdlf_bind.h>
#include <bsl_memory.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>
#include <bsls_systemtime.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {
// CONSTANTS
static const char* k_REFUSAL_MESSAGE = "Not a follower";

static const bsls::Types::Int64 k_WATCHDOG_TIMEOUT_DURATION = 5 * 60;
// 5 minutes

// TYPES
typedef mqbmock::Cluster::TestChannelMapCIter TestChannelMapCIter;

typedef mqbc::ClusterStateManager::NodeToLSNMap      NodeToLSNMap;
typedef mqbc::ClusterStateManager::NodeToLSNMapCIter NodeToLSNMapCIter;

typedef mqbc::ClusterStateManager::ClusterStateLedgerMp ClusterStateLedgerMp;
typedef mqbc::ClusterStateLedger::ClusterMessageCRefList
    ClusterMessageCRefList;

// CLASSES
// =============
// struct Tester
// =============

/// This class provides the mock cluster and other components necessary to
/// test the cluster state manager in isolation, as well as some helper
/// methods.
struct Tester {
  public:
    // PUBLIC DATA
    bool                                         d_isLeader;
    bmqu::TempDirectory                          d_tempDir;
    bslma::ManagedPtr<mqbmock::Cluster>          d_cluster_mp;
    mqbmock::ClusterStateLedger*                 d_clusterStateLedger_p;
    bslma::ManagedPtr<mqbc::ClusterStateManager> d_clusterStateManager_mp;
    mqbmock::StorageManager                      d_storageManager;

  public:
    // CREATORS
    Tester(bool isLeader)
    : d_isLeader(isLeader)
    , d_tempDir(bmqtst::TestHelperUtil::allocator())
    , d_cluster_mp(0)
    , d_clusterStateLedger_p(0)
    , d_clusterStateManager_mp(0)
    , d_storageManager()
    {
        // Create the cluster
        mqbmock::Cluster::ClusterNodeDefs clusterNodeDefs(
            bmqtst::TestHelperUtil::allocator());
        mqbc::ClusterUtil::appendClusterNode(
            &clusterNodeDefs,
            "E1",
            "US-EAST",
            41234,
            mqbmock::Cluster::k_LEADER_NODE_ID,
            bmqtst::TestHelperUtil::allocator());
        mqbc::ClusterUtil::appendClusterNode(
            &clusterNodeDefs,
            "E2",
            "US-EAST",
            41235,
            mqbmock::Cluster::k_LEADER_NODE_ID + 1,
            bmqtst::TestHelperUtil::allocator());
        mqbc::ClusterUtil::appendClusterNode(
            &clusterNodeDefs,
            "W1",
            "US-WEST",
            41236,
            mqbmock::Cluster::k_LEADER_NODE_ID + 2,
            bmqtst::TestHelperUtil::allocator());
        mqbc::ClusterUtil::appendClusterNode(
            &clusterNodeDefs,
            "W2",
            "US-WEST",
            41237,
            mqbmock::Cluster::k_LEADER_NODE_ID + 3,
            bmqtst::TestHelperUtil::allocator());

        d_cluster_mp.load(
            new (*bmqtst::TestHelperUtil::allocator())
                mqbmock::Cluster(bmqtst::TestHelperUtil::allocator(),
                                 true,  // isClusterMember
                                 d_isLeader,
                                 true,   // isCSLMode
                                 true,   // isFSMWorkflow
                                 false,  // doesFSMwriteQLIST
                                 clusterNodeDefs,
                                 "testCluster",
                                 d_tempDir.path()),
            bmqtst::TestHelperUtil::allocator());

        d_cluster_mp->_clusterData()->stats().setIsMember(true);
        BSLS_ASSERT_OPT(d_cluster_mp->_channels().size() > 0);

        bmqsys::Time::initialize(
            &bsls::SystemTime::nowRealtimeClock,
            bdlf::BindUtil::bind(&Tester::nowMonotonicClock, this),
            bdlf::BindUtil::bind(&Tester::highResolutionTimer, this),
            bmqtst::TestHelperUtil::allocator());

        // Create the cluster state ledger
        ClusterStateLedgerMp clusterStateLedger_mp(
            new (*bmqtst::TestHelperUtil::allocator())
                mqbmock::ClusterStateLedger(
                    d_cluster_mp->_clusterData(),
                    bmqtst::TestHelperUtil::allocator()),
            bmqtst::TestHelperUtil::allocator());
        d_clusterStateLedger_p = dynamic_cast<mqbmock::ClusterStateLedger*>(
            clusterStateLedger_mp.get());

        // Create the cluster state manager
        d_clusterStateManager_mp.load(
            new (*bmqtst::TestHelperUtil::allocator())
                mqbc::ClusterStateManager(d_cluster_mp->_clusterDefinition(),
                                          d_cluster_mp.get(),
                                          d_cluster_mp->_clusterData(),
                                          d_cluster_mp->_state(),
                                          clusterStateLedger_mp,
                                          k_WATCHDOG_TIMEOUT_DURATION,
                                          bmqtst::TestHelperUtil::allocator()),
            bmqtst::TestHelperUtil::allocator());
        d_clusterStateManager_mp->setStorageManager(&d_storageManager);

        // Start the cluster and the cluster state manager
        bmqu::MemOutStream errorDescription;
        int                rc = d_cluster_mp->start(errorDescription);
        BSLS_ASSERT_OPT(rc == 0);
        rc = d_clusterStateManager_mp->start(errorDescription);
        BSLS_ASSERT_OPT(rc == 0);

        // In unit test, we do *not* want to trigger cluster state observer
        // events.
        const_cast<mqbc::ClusterState*>(
            d_clusterStateManager_mp->clusterState())
            ->unregisterObserver(d_clusterStateManager_mp.get());
    }

    ~Tester()
    {
        // Stop the cluster
        d_clusterStateManager_mp->stop();
        d_cluster_mp->stop();

        bmqsys::Time::shutdown();
    }

    // MANIPULATORS
    void electLeader(bsls::Types::Uint64 electorTerm = 1U)
    {
        mqbnet::ClusterNode* leaderNode =
            d_cluster_mp->netCluster().lookupNode(
                mqbmock::Cluster::k_LEADER_NODE_ID);
        BSLS_ASSERT_OPT(leaderNode != 0);
        d_cluster_mp->_clusterData()->electorInfo().setElectorInfo(
            d_isLeader ? mqbnet::ElectorState::e_LEADER
                       : mqbnet::ElectorState::e_FOLLOWER,
            electorTerm,
            leaderNode,
            mqbc::ElectorInfoLeaderStatus::e_PASSIVE);
    }

    void transitionToNewLeader(mqbnet::ClusterNode* node,
                               bsls::Types::Uint64  term)
    {
        // Transition the specified 'node' to become the new leader having the
        // specified 'term'.

        d_cluster_mp->_clusterData()->electorInfo().setElectorInfo(
            mqbnet::ElectorState::e_FOLLOWER,
            term,  // term
            0,     // leaderNode
            mqbc::ElectorInfoLeaderStatus::e_UNDEFINED);
        d_cluster_mp->_clusterData()->electorInfo().setElectorInfo(
            mqbnet::ElectorState::e_FOLLOWER,
            term,  // term
            node,  // leaderNode
            mqbc::ElectorInfoLeaderStatus::e_PASSIVE);
    }

    void setSelfLedgerLSN(const bmqp_ctrlmsg::LeaderMessageSequence& value)
    {
        // Artificially set self LSN in the ledger to the specified 'value' by
        // commiting a *dummy* advisory.

        bmqp_ctrlmsg::LeaderAdvisory snapshot;
        snapshot.sequenceNumber() = value;
        // As a side effect of commiting a dummy advisory, the commit record
        // bumps up the sequence number by 1.  To counteract this side effect,
        // we bump it down by 1 for now.
        --snapshot.sequenceNumber().sequenceNumber();

        d_cluster_mp->_clusterData()->electorInfo().setLeaderMessageSequence(
            snapshot.sequenceNumber());

        // Whenever we commit an advisory, it will invoke commit callback which
        // will have side effects.  We temporarily pause commit callbacks here
        // to avoid side effects, since the only thing we want to do is to set
        // self LSN.
        d_clusterStateLedger_p->_setPauseCommitCb(true);
        d_clusterStateLedger_p->apply(snapshot);
        d_clusterStateLedger_p->_commitAdvisories(
            mqbc::ClusterStateLedgerCommitStatus::e_SUCCESS);
        d_clusterStateLedger_p->_setPauseCommitCb(false);
    }

    void clearChannels()
    {
        for (TestChannelMapCIter cit = d_cluster_mp->_channels().cbegin();
             cit != d_cluster_mp->_channels().cend();
             ++cit) {
            cit->second->reset();
        }
    }

    // ACCESSORS
    bsls::TimeInterval nowMonotonicClock() const
    {
        return d_cluster_mp->_scheduler().now();
    }

    bsls::Types::Int64 highResolutionTimer() const
    {
        return d_cluster_mp->_scheduler().now().totalNanoseconds();
    }

    void verifyFollowerLSNRequestsSent() const
    {
        // Verify that a same follower LSN request is sent to all followers.

        bmqp_ctrlmsg::ClusterMessage expectedMessage;
        expectedMessage.choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeFollowerLSNRequest();

        for (TestChannelMapCIter cit = d_cluster_mp->_channels().cbegin();
             cit != d_cluster_mp->_channels().cend();
             ++cit) {
            if (cit->first->nodeId() ==
                d_cluster_mp->netCluster().selfNodeId()) {
                BMQTST_ASSERT(!cit->second->waitFor(1, false));
                continue;  // CONTINUE
            }

            BMQTST_ASSERT(cit->second->waitFor(1, false));

            bmqp_ctrlmsg::ControlMessage message;
            mqbc::ClusterUtil::extractMessage(
                &message,
                cit->second->writeCalls()[0].d_blob,
                bmqtst::TestHelperUtil::allocator());
            BMQTST_ASSERT_EQ(message.choice().clusterMessage(),
                             expectedMessage);
        }
    }

    void verifyFollowerLSNResponseSent(
        const bmqp_ctrlmsg::LeaderMessageSequence& sequenceNumber) const
    {
        // Verify that a follower LSN response having the specified
        // 'sequenceNumber' is replied *only* to the leader.
        bmqp_ctrlmsg::ControlMessage expectedMessage;
        expectedMessage.rId() = 1;
        bmqp_ctrlmsg::FollowerLSNResponse& response =
            expectedMessage.choice()
                .makeClusterMessage()
                .choice()
                .makeClusterStateFSMMessage()
                .choice()
                .makeFollowerLSNResponse();
        response.sequenceNumber() = sequenceNumber;

        for (TestChannelMapCIter cit = d_cluster_mp->_channels().cbegin();
             cit != d_cluster_mp->_channels().cend();
             ++cit) {
            if (cit->first->nodeId() ==
                d_cluster_mp->_clusterData()->electorInfo().leaderNodeId()) {
                BMQTST_ASSERT(cit->second->waitFor(1, false));

                bmqp_ctrlmsg::ControlMessage message;
                mqbc::ClusterUtil::extractMessage(
                    &message,
                    cit->second->writeCalls()[0].d_blob,
                    bmqtst::TestHelperUtil::allocator());
                BMQTST_ASSERT_EQ(message, expectedMessage);
            }
            else {
                BMQTST_ASSERT(!cit->second->waitFor(1, false));
            }
        }
    }

    void verifyFailureFollowerLSNResponseSent(
        const mqbnet::ClusterNode& destination) const
    {
        // Verify that a failure follower LSN response is replied *only* to
        // the specified 'destination'.
        bmqp_ctrlmsg::ControlMessage expectedMessage;
        expectedMessage.rId()          = 1;
        bmqp_ctrlmsg::Status& response = expectedMessage.choice().makeStatus();
        response.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        response.code()     = mqbi::ClusterErrorCode::e_NOT_FOLLOWER;
        response.message()  = k_REFUSAL_MESSAGE;

        for (TestChannelMapCIter cit = d_cluster_mp->_channels().cbegin();
             cit != d_cluster_mp->_channels().cend();
             ++cit) {
            if (cit->first->nodeId() == destination.nodeId()) {
                BMQTST_ASSERT(cit->second->waitFor(1, false));

                bmqp_ctrlmsg::ControlMessage message;
                mqbc::ClusterUtil::extractMessage(
                    &message,
                    cit->second->writeCalls()[0].d_blob,
                    bmqtst::TestHelperUtil::allocator());
                BMQTST_ASSERT_EQ(message, expectedMessage);
            }
            else {
                BMQTST_ASSERT(!cit->second->waitFor(1, false));
            }
        }
    }

    void verifyRegistrationRequestSent(
        const bmqp_ctrlmsg::LeaderMessageSequence& sequenceNumber) const
    {
        // Verify that a registration request having the specified
        // 'sequenceNumber' is sent *only* to the leader.

        bmqp_ctrlmsg::ClusterMessage expectedMessage;
        expectedMessage.choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeRegistrationRequest();
        expectedMessage.choice()
            .clusterStateFSMMessage()
            .choice()
            .registrationRequest()
            .sequenceNumber() = sequenceNumber;

        for (TestChannelMapCIter cit = d_cluster_mp->_channels().cbegin();
             cit != d_cluster_mp->_channels().cend();
             ++cit) {
            if (cit->first->nodeId() ==
                d_cluster_mp->_clusterData()->electorInfo().leaderNodeId()) {
                BMQTST_ASSERT(cit->second->waitFor(1, false));

                bmqp_ctrlmsg::ControlMessage message;
                mqbc::ClusterUtil::extractMessage(
                    &message,
                    cit->second->writeCalls()[0].d_blob,
                    bmqtst::TestHelperUtil::allocator());
                BMQTST_ASSERT_EQ(message.choice().clusterMessage(),
                                 expectedMessage);
            }
            else {
                BMQTST_ASSERT(!cit->second->waitFor(1, false));
            }
        }
    }

    void verifyRegistrationResponseSent(
        const mqbnet::ClusterNode& destination) const
    {
        // Verify that a registration response is replied to the specified
        // 'destination'.
        bmqp_ctrlmsg::ControlMessage expectedMessage;
        expectedMessage.rId() = 1;
        expectedMessage.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeRegistrationResponse();

        for (TestChannelMapCIter cit = d_cluster_mp->_channels().cbegin();
             cit != d_cluster_mp->_channels().cend();
             ++cit) {
            if (cit->first->nodeId() == destination.nodeId()) {
                BMQTST_ASSERT(cit->second->waitFor(1, false));

                bmqp_ctrlmsg::ControlMessage message;
                mqbc::ClusterUtil::extractMessage(
                    &message,
                    cit->second->writeCalls()[0].d_blob,
                    bmqtst::TestHelperUtil::allocator());
                BMQTST_ASSERT_EQ(message, expectedMessage);
            }
            else {
                BMQTST_ASSERT(!cit->second->waitFor(1, false));
            }
        }
    }

    void verifyFollowerClusterStateRequestSent() const
    {
        // Verify that a follower cluster state request is sent to the highest
        // LSN follower.

        // Verify that the highest LSN node is not self i.e. Leader but a
        // follower node
        const NodeToLSNMap& lsnMap = d_clusterStateManager_mp->nodeToLSNMap();

        mqbnet::ClusterNode*                highestLSNFollower = 0;
        bmqp_ctrlmsg::LeaderMessageSequence highestLSN;
        for (NodeToLSNMapCIter cit = lsnMap.cbegin(); cit != lsnMap.cend();
             ++cit) {
            if (!highestLSNFollower || cit->second > highestLSN) {
                highestLSNFollower = cit->first;
                highestLSN         = cit->second;
            }
        }

        BSLS_ASSERT_OPT(highestLSNFollower);
        BSLS_ASSERT_OPT(highestLSNFollower->nodeId() !=
                        d_cluster_mp->netCluster().selfNodeId());

        PV("Highest LSN follower is " << highestLSNFollower->nodeDescription()
                                      << " with a LSN of " << highestLSN);

        // Verify that the follower cluster state request is sent
        bmqp_ctrlmsg::ClusterMessage expectedFollowerClusterStateRequest;
        expectedFollowerClusterStateRequest.choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeFollowerClusterStateRequest();

        for (TestChannelMapCIter cit = d_cluster_mp->_channels().cbegin();
             cit != d_cluster_mp->_channels().cend();
             ++cit) {
            if (cit->first->nodeId() == highestLSNFollower->nodeId()) {
                BSLS_ASSERT_OPT(cit->second->waitFor(1, false));

                bmqp_ctrlmsg::ControlMessage message;
                mqbc::ClusterUtil::extractMessage(
                    &message,
                    cit->second->writeCalls()[0].d_blob,
                    bmqtst::TestHelperUtil::allocator());
                BSLS_ASSERT_OPT(message.choice().clusterMessage() ==
                                expectedFollowerClusterStateRequest);
            }
            else {
                BSLS_ASSERT_OPT(!cit->second->waitFor(1, false));
            }
        }
    }

    void verifyFollowerClusterStateResponseSent(
        const bmqp_ctrlmsg::LeaderMessageSequence& sequenceNumber) const
    {
        // Verify that a follower cluster state response having the specified
        // 'sequenceNumber' is replied *only* to the leader.
        bmqp_ctrlmsg::ControlMessage expectedMessage;
        expectedMessage.rId() = 1;
        bmqp_ctrlmsg::FollowerClusterStateResponse& response =
            expectedMessage.choice()
                .makeClusterMessage()
                .choice()
                .makeClusterStateFSMMessage()
                .choice()
                .makeFollowerClusterStateResponse();
        response.clusterStateSnapshot().sequenceNumber() = sequenceNumber;
        // Load (empty) cluster state into the response
        mqbc::ClusterUtil::loadPartitionsInfo(
            &response.clusterStateSnapshot().partitions(),
            *d_cluster_mp->_state());
        mqbc::ClusterUtil::loadQueuesInfo(
            &response.clusterStateSnapshot().queues(),
            *d_cluster_mp->_state());

        for (TestChannelMapCIter cit = d_cluster_mp->_channels().cbegin();
             cit != d_cluster_mp->_channels().cend();
             ++cit) {
            if (cit->first->nodeId() ==
                d_cluster_mp->_clusterData()->electorInfo().leaderNodeId()) {
                BMQTST_ASSERT(cit->second->waitFor(1, false));

                bmqp_ctrlmsg::ControlMessage message;
                mqbc::ClusterUtil::extractMessage(
                    &message,
                    cit->second->writeCalls()[0].d_blob,
                    bmqtst::TestHelperUtil::allocator());
                BMQTST_ASSERT_EQ(message, expectedMessage);
            }
            else {
                BMQTST_ASSERT(!cit->second->waitFor(1, false));
            }
        }
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTestLeader()
// ------------------------------------------------------------------------
// BREATHING TEST LEADER
//
// Concerns:
//   Exercise basic functionality before beginning testing in earnest.
//   Elector elects self as leader.  Verify that we transition to Leader
//   Healing Stage 1.
//
// Testing:
//   Basic functionality as new leader
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "BREATHING TEST LEADER");

    Tester tester(true);  // isLeader

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_UNKNOWN);

    // 1. Elector elects self as leader
    tester.electLeader();

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);

    // Verify that self ledger LSN is stored, and is less than current
    // elector term
    const NodeToLSNMap& lsnMap =
        tester.d_clusterStateManager_mp->nodeToLSNMap();
    BMQTST_ASSERT_EQ(lsnMap.size(), 1U);
    NodeToLSNMapCIter citer = lsnMap.find(const_cast<mqbnet::ClusterNode*>(
        tester.d_cluster_mp->netCluster().selfNode()));
    BMQTST_ASSERT(citer != lsnMap.cend());
    BMQTST_ASSERT_LT(citer->second,
                     tester.d_cluster_mp->_clusterData()
                         ->electorInfo()
                         .leaderMessageSequence());

    tester.verifyFollowerLSNRequestsSent();
}

static void test2_breathingTestFollower()
// ------------------------------------------------------------------------
// BREATHING TEST FOLLOWER
//
// Concerns:
//   Exercise basic functionality before beginning testing in earnest.
//   Elector elects a leader; self becomes follower.  Verify that we
//   transition to Follower Healing.
//
// Testing:
//   Basic functionality as new follower
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "BREATHING TEST FOLLOWER");

    Tester tester(false);  // isLeader
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_UNKNOWN);

    bmqp_ctrlmsg::LeaderMessageSequence selfLSN;
    selfLSN.electorTerm()    = 1U;
    selfLSN.sequenceNumber() = 3U;
    tester.setSelfLedgerLSN(selfLSN);

    // 1. Elector elects a leader; self becomes follower
    tester.electLeader(2U);

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_FOL_HEALING);
    BMQTST_ASSERT(tester.d_clusterStateManager_mp->nodeToLSNMap().empty());

    tester.verifyRegistrationRequestSent(selfLSN);
}

static void test3_invalidMessagesUnknownState()
// ------------------------------------------------------------------------
// INVALID MESSAGES UNKNOWN STATE
//
// Concerns:
//   Verify that invalid requests and responses are rejected when we are
//   in the Unknown state.
//
// Testing:
//   Invalid message handling when in Unknown state
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "INVALID MESSAGES UNKNOWN STATE");

    Tester tester(false);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    // 1. Receives a follower LSN request in Unknown state
    bmqp_ctrlmsg::ControlMessage message;
    message.rId() = 1;
    message.choice()
        .makeClusterMessage()
        .choice()
        .makeClusterStateFSMMessage()
        .choice()
        .makeFollowerLSNRequest();

    mqbnet::ClusterNode* source = tester.d_cluster_mp->netCluster().lookupNode(
        mqbmock::Cluster::k_LEADER_NODE_ID);
    tester.d_clusterStateManager_mp->processFollowerLSNRequest(message,
                                                               source);

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_UNKNOWN);

    tester.verifyFailureFollowerLSNResponseSent(*source);
}

static void test4_invalidMessagesLeader()
// ------------------------------------------------------------------------
// INVALID MESSAGES LEADER
//
// Concerns:
//   Verify that invalid requests and responses are rejected when we are
//   in any of the leader states.
//
// Testing:
//   Invalid message handling when in any leader state
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "INVALID MESSAGES LEADER");

    Tester tester(true);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    tester.electLeader();
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);
    tester.verifyFollowerLSNRequestsSent();
    tester.clearChannels();

    // 1. Receives a follower LSN request in leader healing stage 1
    bmqp_ctrlmsg::ControlMessage message;
    message.rId() = 1;
    message.choice()
        .makeClusterMessage()
        .choice()
        .makeClusterStateFSMMessage()
        .choice()
        .makeFollowerLSNRequest();

    mqbnet::ClusterNode* source = tester.d_cluster_mp->netCluster().lookupNode(
        mqbmock::Cluster::k_LEADER_NODE_ID + 1);
    tester.d_clusterStateManager_mp->processFollowerLSNRequest(message,
                                                               source);

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);

    tester.verifyFailureFollowerLSNResponseSent(*source);
    tester.clearChannels();

    // 2. Artificially transition self to leader healing stage 2
    bmqp_ctrlmsg::ControlMessage       followerMessage;
    bmqp_ctrlmsg::FollowerLSNResponse& followerResponse =
        followerMessage.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeFollowerLSNResponse();
    followerResponse.sequenceNumber().electorTerm()    = 1U;
    followerResponse.sequenceNumber().sequenceNumber() = 2U;

    followerMessage.rId() = 1;
    tester.d_cluster_mp->requestManager().processResponse(followerMessage);
    followerMessage.rId() = 2;
    tester.d_cluster_mp->requestManager().processResponse(followerMessage);

    // This is the highest LSN follower
    followerMessage.rId()                              = 3;
    followerResponse.sequenceNumber().sequenceNumber() = 5U;
    tester.d_cluster_mp->requestManager().processResponse(followerMessage);

    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);
    tester.verifyFollowerClusterStateRequestSent();
    tester.clearChannels();

    // 3. Receives a follower LSN request (in leader healing stage 2)
    tester.d_clusterStateManager_mp->processFollowerLSNRequest(message,
                                                               source);

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);

    tester.verifyFailureFollowerLSNResponseSent(*source);
}

static void test5_followerLSNRequestHandlingFollower()
// ------------------------------------------------------------------------
// FOLLOWER LSN REQUEST HANDLING FOLLOWER
//
// Concerns:
//   Verify that a follower replies to follower LSN request from leader.
//
// Testing:
//   Follower response to follower LSN request
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "CLUSTER STATE MANAGER - "
        "FOLLOWER LSN REQUEST HANDLING FOLLOWER");

    Tester tester(false);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    bmqp_ctrlmsg::LeaderMessageSequence selfLSN;
    selfLSN.electorTerm()    = 1U;
    selfLSN.sequenceNumber() = 3U;
    tester.setSelfLedgerLSN(selfLSN);

    tester.electLeader(2U);
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_FOL_HEALING);
    tester.verifyRegistrationRequestSent(selfLSN);
    tester.clearChannels();

    // 1. Receives a follower LSN request from leader
    bmqp_ctrlmsg::ControlMessage message;
    message.rId() = 1;
    message.choice()
        .makeClusterMessage()
        .choice()
        .makeClusterStateFSMMessage()
        .choice()
        .makeFollowerLSNRequest();

    mqbnet::ClusterNode* source = tester.d_cluster_mp->netCluster().lookupNode(
        mqbmock::Cluster::k_LEADER_NODE_ID);
    tester.d_clusterStateManager_mp->processFollowerLSNRequest(message,
                                                               source);

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_FOL_HEALING);

    tester.verifyFollowerLSNResponseSent(selfLSN);
}

static void test6_registrationRequestHandlingLeader()
// ------------------------------------------------------------------------
// REGISTRATION REQUEST HANDLING LEADER
//
// Concerns:
//   Verify that the leader replies to registration request from a
//   follower, and store the follower's LSN.
//
// Testing:
//   Leader response to registration request
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "REGISTRATION REQUEST HANDLING LEADER");

    Tester tester(true);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    tester.electLeader();
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);
    tester.verifyFollowerLSNRequestsSent();
    tester.clearChannels();

    // 1. Receives registration request from a follower
    bmqp_ctrlmsg::ControlMessage message;
    message.rId() = 1;
    bmqp_ctrlmsg::LeaderMessageSequence& lms =
        message.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeRegistrationRequest()
            .sequenceNumber();
    lms.electorTerm()    = 2U;
    lms.sequenceNumber() = 5U;

    mqbnet::ClusterNode* source = tester.d_cluster_mp->netCluster().lookupNode(
        mqbmock::Cluster::k_LEADER_NODE_ID + 1);
    tester.d_clusterStateManager_mp->processRegistrationRequest(message,
                                                                source);

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);

    tester.verifyRegistrationResponseSent(*source);

    // Verify that the follower's LSN is stored
    const NodeToLSNMap& lsnMap =
        tester.d_clusterStateManager_mp->nodeToLSNMap();
    BMQTST_ASSERT_EQ(lsnMap.size(), 2U);  // 2 including self LSN
    NodeToLSNMapCIter citer = lsnMap.find(source);
    BMQTST_ASSERT(citer != lsnMap.cend());
    BMQTST_ASSERT_EQ(citer->second.electorTerm(), 2U);
    BMQTST_ASSERT_EQ(citer->second.sequenceNumber(), 5U);
}

static void test7_followerLSNResponseQuorum()
// ------------------------------------------------------------------------
// FOLLOWER LSN RESPONSE QUORUM
//
// Concerns:
//   Verify that the leader transitions to healing stage 2 upon reaching
//   follower LSN quorum through a series of follower LSN responses.
//
// Testing:
//   Leader transition upon follower LSN response quorum
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "FOLLOWER LSN RESPONSE QUORUM");

    Tester tester(true);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    // Self is elected as leader; a follower LSN request is sent to all
    // followers
    tester.electLeader();
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);
    tester.verifyFollowerLSNRequestsSent();

    const NodeToLSNMap& lsnMap =
        tester.d_clusterStateManager_mp->nodeToLSNMap();
    BSLS_ASSERT_OPT(lsnMap.size() == 1U);

    // 1. Receives follower LSN response from a majority of (two) followers,
    //    but failure response from the last follower
    bmqp_ctrlmsg::ControlMessage         message;
    bmqp_ctrlmsg::LeaderMessageSequence& lms =
        message.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeFollowerLSNResponse()
            .sequenceNumber();

    message.rId()        = 1;
    lms.electorTerm()    = 1U;
    lms.sequenceNumber() = 2U;
    tester.d_cluster_mp->requestManager().processResponse(message);

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);

    message.rId()        = 2;
    lms.electorTerm()    = 1U;
    lms.sequenceNumber() = 5U;
    tester.d_cluster_mp->requestManager().processResponse(message);

    bmqp_ctrlmsg::ControlMessage failureMessage;
    failureMessage.rId() = 3;
    bmqp_ctrlmsg::Status& failureResponse =
        failureMessage.choice().makeStatus();
    failureResponse.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
    failureResponse.code()     = mqbi::ClusterErrorCode::e_NOT_FOLLOWER;
    failureResponse.message()  = k_REFUSAL_MESSAGE;
    tester.d_cluster_mp->requestManager().processResponse(failureMessage);

    // Verify that the majority of follower LSNs are stored
    BMQTST_ASSERT_EQ(lsnMap.size(), 3U);

    // Quorum is achieved.  Verify that self has transitioned to leader
    // healing stage 2.
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);
}

static void test8_registrationRequestQuorum()
// ------------------------------------------------------------------------
// REGISTRATION REQUEST QUORUM
//
// Concerns:
//   Verify that the leader transitions to healing stage 2 upon reaching
//   follower LSN quorum through a series of registration requests.
//
// Testing:
//   Leader transition upon registration request quorum
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "REGISTRATION REQUEST QUORUM");

    Tester tester(true);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    // Self is elected as leader
    tester.electLeader();
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);
    tester.verifyFollowerLSNRequestsSent();

    const NodeToLSNMap& lsnMap =
        tester.d_clusterStateManager_mp->nodeToLSNMap();
    BSLS_ASSERT_OPT(lsnMap.size() == 1U);

    // 1. Receives registration request from a majority of (two) followers,
    //    but nothing from the last follower
    bmqp_ctrlmsg::ControlMessage         message;
    bmqp_ctrlmsg::LeaderMessageSequence& lms =
        message.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeRegistrationRequest()
            .sequenceNumber();

    message.rId()        = 1;
    lms.electorTerm()    = 1U;
    lms.sequenceNumber() = 2U;

    mqbnet::ClusterNode* source1 =
        tester.d_cluster_mp->netCluster().lookupNode(
            mqbmock::Cluster::k_LEADER_NODE_ID + 1);
    tester.d_clusterStateManager_mp->processRegistrationRequest(message,
                                                                source1);

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);

    message.rId()        = 2;
    lms.electorTerm()    = 1U;
    lms.sequenceNumber() = 5U;

    mqbnet::ClusterNode* source2 =
        tester.d_cluster_mp->netCluster().lookupNode(
            mqbmock::Cluster::k_LEADER_NODE_ID + 2);
    tester.d_clusterStateManager_mp->processRegistrationRequest(message,
                                                                source2);

    // Verify that the majority of follower LSNs are stored
    BMQTST_ASSERT_EQ(lsnMap.size(), 3U);
    BMQTST_ASSERT_EQ(lsnMap.at(source1).electorTerm(), 1U);
    BMQTST_ASSERT_EQ(lsnMap.at(source1).sequenceNumber(), 2U);
    BMQTST_ASSERT_EQ(lsnMap.at(source2).electorTerm(), 1U);
    BMQTST_ASSERT_EQ(lsnMap.at(source2).sequenceNumber(), 5U);

    // Quorum is achieved.  Verify that self has transitioned to leader
    // healing stage 2.
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);
}

static void test9_followerLSNNoQuorum()
// ------------------------------------------------------------------------
// FOLLOWER LSN NO QUORUM
//
// Concerns:
//   Verify that the leader stays in healing stage 1 when failing to reach
//   follower LSN quorum.
//
// Testing:
//   Leader transition upon follower LSN no quorum
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "FOLLOWER LSN NO QUORUM");

    Tester tester(true);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    // Self is elected as leader; a follower LSN request is sent to all
    // followers
    tester.electLeader();
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);
    tester.verifyFollowerLSNRequestsSent();

    const NodeToLSNMap& lsnMap =
        tester.d_clusterStateManager_mp->nodeToLSNMap();
    BSLS_ASSERT_OPT(lsnMap.size() == 1U);

    // 1. Receives follower LSN response from one follower, but failure
    //    response from other followers, thus failing to reach quorum
    bmqp_ctrlmsg::ControlMessage message;
    message.rId() = 1;
    bmqp_ctrlmsg::LeaderMessageSequence& lms =
        message.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeFollowerLSNResponse()
            .sequenceNumber();
    lms.electorTerm()    = 1U;
    lms.sequenceNumber() = 2U;
    tester.d_cluster_mp->requestManager().processResponse(message);

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);

    bmqp_ctrlmsg::ControlMessage failureMessage;
    bmqp_ctrlmsg::Status&        failureResponse =
        failureMessage.choice().makeStatus();
    failureResponse.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
    failureResponse.code()     = mqbi::ClusterErrorCode::e_NOT_FOLLOWER;
    failureResponse.message()  = k_REFUSAL_MESSAGE;

    failureMessage.rId() = 2;
    tester.d_cluster_mp->requestManager().processResponse(failureMessage);

    failureMessage.rId() = 3;
    tester.d_cluster_mp->requestManager().processResponse(failureMessage);

    // Verify that we only have knowledge of two LSNs
    BMQTST_ASSERT_EQ(lsnMap.size(), 2U);

    // Quorum is not achieved.  Verify that self is staying in leader
    // healing stage 1.
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);
}

static void test10_followerClusterStateRequestHandlingFollower()
// ------------------------------------------------------------------------
// FOLLOWER CLUSTER STATE REQUEST HANDLING FOLLOWER
//
// Concerns:
//   Verify that a follower replies to follower cluster state request from
//   leader.
//
// Testing:
//   Follower response to follower cluster state request
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "CLUSTER STATE MANAGER - "
        "FOLLOWER CLUSTER STATE REQUEST HANDLING FOLLOWER");

    Tester tester(false);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    // Set some initial self LSN in the ledger
    const bsls::Types::Uint64 k_INITIAL_ELECTOR_TERM    = 3U;
    const bsls::Types::Uint64 k_INITIAL_SEQUENCE_NUMBER = 7U;

    bmqp_ctrlmsg::LeaderMessageSequence selfLSN;
    selfLSN.electorTerm()    = k_INITIAL_ELECTOR_TERM;
    selfLSN.sequenceNumber() = k_INITIAL_SEQUENCE_NUMBER;
    tester.setSelfLedgerLSN(selfLSN);

    tester.electLeader();
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_FOL_HEALING);
    tester.verifyRegistrationRequestSent(selfLSN);
    tester.clearChannels();

    // 1. Receives a follower cluster state request from leader
    bmqp_ctrlmsg::ControlMessage message;
    message.rId() = 1;
    message.choice()
        .makeClusterMessage()
        .choice()
        .makeClusterStateFSMMessage()
        .choice()
        .makeFollowerClusterStateRequest();

    mqbnet::ClusterNode* leader = tester.d_cluster_mp->netCluster().lookupNode(
        mqbmock::Cluster::k_LEADER_NODE_ID);
    tester.d_clusterStateManager_mp->processFollowerClusterStateRequest(
        message,
        leader);

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_FOL_HEALING);

    tester.verifyFollowerClusterStateResponseSent(selfLSN);
}

static void test11_leaderHighestLeaderHealed()
// ------------------------------------------------------------------------
// LEADER HIGHEST LEADER HEALED
//
// Concerns:
//   Verify that when the leader has highest LSN, it transitions from
//   healing stage 2 to healed upon successful CSL commit callback.
//
// Testing:
//   Leader transition to healed upon successful CSL commit callback
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "LEADER HIGHEST LEADER HEALED");

    Tester tester(true);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    // Artificially set self LSN in the ledger to be higher than all followers
    bmqp_ctrlmsg::LeaderMessageSequence selfLSN;
    selfLSN.electorTerm()    = 1U;
    selfLSN.sequenceNumber() = 8U;
    tester.setSelfLedgerLSN(selfLSN);

    tester.electLeader(2U);
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);
    tester.verifyFollowerLSNRequestsSent();

    // 1. Receives follower LSN response from all followers
    bmqp_ctrlmsg::ControlMessage         followerLSNResponse;
    bmqp_ctrlmsg::LeaderMessageSequence& lms =
        followerLSNResponse.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeFollowerLSNResponse()
            .sequenceNumber();

    followerLSNResponse.rId() = 1;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 2U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    followerLSNResponse.rId() = 2;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 5U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    followerLSNResponse.rId() = 3;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 5U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    // Verify that all LSNs are stored
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->nodeToLSNMap().size() ==
                    4U);

    // Self has transitioned to leader healing stage 2
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);

    // Verify that a cluster state snapshot is applied to the CSL
    ClusterMessageCRefList advisories;
    tester.d_clusterStateLedger_p->uncommittedAdvisories(&advisories);
    BMQTST_ASSERT_EQ(advisories.size(), 1U);
    BMQTST_ASSERT(advisories.front().get().choice().isLeaderAdvisoryValue());

    const bmqp_ctrlmsg::LeaderAdvisory& advisory =
        advisories.front().get().choice().leaderAdvisory();
    BMQTST_ASSERT_EQ(advisory.sequenceNumber().electorTerm(), 2U);
    BMQTST_ASSERT_EQ(advisory.sequenceNumber().sequenceNumber(), 1U);

    // 2. Invoke success commit callback at the CSL
    tester.d_clusterStateLedger_p->_commitAdvisories(
        mqbc::ClusterStateLedgerCommitStatus::e_SUCCESS);

    // Verify that leader (self) has healed
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALED);

    const bmqp_ctrlmsg::LeaderMessageSequence& latestLSN =
        tester.d_cluster_mp->_clusterData()
            ->electorInfo()
            .leaderMessageSequence();
    BMQTST_ASSERT_EQ(latestLSN.electorTerm(), 2U);
    BMQTST_ASSERT_EQ(latestLSN.sequenceNumber(), 2U);
}

static void test12_followerHighestLeaderHealed()
// ------------------------------------------------------------------------
// FOLLOWER HIGHEST LEADER HEALED
//
// Concerns:
//   Verify that when a follower has highest LSN, the leader transitions
//   from healing stage 2 to healed upon follower cluster state response
//   and successful CSL commit callback.
//
// Testing:
//   Leader transition to healed upon follower cluster state response and
//   successful CSL commit callback
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "FOLLOWER HIGHEST LEADER HEALED");

    Tester tester(true);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    // Artificially set self LSN in the ledger to be lower than some followers
    bmqp_ctrlmsg::LeaderMessageSequence selfLSN;
    selfLSN.electorTerm()    = 1U;
    selfLSN.sequenceNumber() = 5U;
    tester.setSelfLedgerLSN(selfLSN);

    tester.electLeader(2U);
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);
    tester.verifyFollowerLSNRequestsSent();
    tester.clearChannels();

    // 1. Receives follower LSN response from all followers
    bmqp_ctrlmsg::ControlMessage         followerLSNResponse;
    bmqp_ctrlmsg::LeaderMessageSequence& lms =
        followerLSNResponse.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeFollowerLSNResponse()
            .sequenceNumber();

    followerLSNResponse.rId() = 1;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 2U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    followerLSNResponse.rId() = 2;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 8U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    followerLSNResponse.rId() = 3;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 5U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    // Verify that all LSNs are stored
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->nodeToLSNMap().size() ==
                    4U);

    // Self has transitioned to leader healing stage 2
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);

    // Verify that a follower cluster state request is sent to the highest LSN
    // follower
    tester.verifyFollowerClusterStateRequestSent();

    // 2. Receive a follower cluster state response from the highest LSN
    // follower
    bmqp_ctrlmsg::ControlMessage  followerClusterStateResponse;
    bmqp_ctrlmsg::LeaderAdvisory& clusterStateSnapshot =
        followerClusterStateResponse.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeFollowerClusterStateResponse()
            .clusterStateSnapshot();

    followerClusterStateResponse.rId() = 4;  // This is the 4th request so far

    clusterStateSnapshot.sequenceNumber().electorTerm()    = 1U;
    clusterStateSnapshot.sequenceNumber().sequenceNumber() = 8U;
    clusterStateSnapshot.partitions().resize(
        tester.d_cluster_mp->_state()->partitions().size());
    for (size_t i = 0; i < clusterStateSnapshot.partitions().size(); ++i) {
        clusterStateSnapshot.partitions()[i].partitionId() = i;
    }

    tester.d_cluster_mp->requestManager().processResponse(
        followerClusterStateResponse);
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);

    // Verify that a cluster state snapshot is applied to the CSL
    ClusterMessageCRefList advisories;
    tester.d_clusterStateLedger_p->uncommittedAdvisories(&advisories);
    BMQTST_ASSERT_EQ(advisories.size(), 1U);
    BMQTST_ASSERT(advisories.front().get().choice().isLeaderAdvisoryValue());

    const bmqp_ctrlmsg::LeaderAdvisory& advisory =
        advisories.front().get().choice().leaderAdvisory();
    BMQTST_ASSERT_EQ(advisory.sequenceNumber().electorTerm(), 2U);
    BMQTST_ASSERT_EQ(advisory.sequenceNumber().sequenceNumber(), 1U);

    // 3. Invoke success commit callback at the CSL
    tester.d_clusterStateLedger_p->_commitAdvisories(
        mqbc::ClusterStateLedgerCommitStatus::e_SUCCESS);

    // Verify that leader (self) has healed
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALED);

    const bmqp_ctrlmsg::LeaderMessageSequence& latestLSN =
        tester.d_cluster_mp->_clusterData()
            ->electorInfo()
            .leaderMessageSequence();
    BMQTST_ASSERT_EQ(latestLSN.electorTerm(), 2U);
    BMQTST_ASSERT_EQ(latestLSN.sequenceNumber(), 2U);
}

static void test13_followerHealed()
// FOLLOWER HEALED
//
// Concerns:
//   Verify that the follower transitions from healing to healed upon
//   successful CSL commit callback.
//
// Testing:
//   Follower transition to healed upon successful CSL commit callback
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "FOLLOWER HEALED");

    Tester tester(false);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    tester.electLeader();
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_FOL_HEALING);

    // 1. Receive CSL advisory from leader.  Apply it.
    bmqp_ctrlmsg::LeaderAdvisory cslAdvisory;
    cslAdvisory.sequenceNumber().electorTerm()    = 3U;
    cslAdvisory.sequenceNumber().sequenceNumber() = 7U;

    tester.d_clusterStateLedger_p->apply(cslAdvisory);
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_FOL_HEALING);

    // 2. CSL advisory is committed successfully.
    tester.d_clusterStateLedger_p->_commitAdvisories(
        mqbc::ClusterStateLedgerCommitStatus::e_SUCCESS);

    // Verify that self (a follower) has healed
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_FOL_HEALED);
}

static void test14_leaderCSLCommitFailure()
// ------------------------------------------------------------------------
// LEADER CSL COMMIT FAILURE
//
// Concerns:
//   Verify that the leader transitions back to unknown upon CSL commit
//   callback failure.
//
// Testing:
//   Leader upon CSL commit callback failure
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "LEADER CSL COMMIT FAILURE");

    Tester tester(true);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    // Set self LSN in the ledger
    bmqp_ctrlmsg::LeaderMessageSequence selfLSN;
    selfLSN.electorTerm()    = 1U;
    selfLSN.sequenceNumber() = 8U;
    tester.setSelfLedgerLSN(selfLSN);

    tester.electLeader(2U);
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);
    tester.verifyFollowerLSNRequestsSent();

    // 1. Receives follower LSN response from all followers
    bmqp_ctrlmsg::ControlMessage         followerLSNResponse;
    bmqp_ctrlmsg::LeaderMessageSequence& lms =
        followerLSNResponse.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeFollowerLSNResponse()
            .sequenceNumber();

    followerLSNResponse.rId() = 1;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 2U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    followerLSNResponse.rId() = 2;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 5U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    followerLSNResponse.rId() = 3;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 5U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    // Self has transitioned to leader healing stage 2
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);

    // 2. Invoke failure commit callback at the CSL
    tester.d_clusterStateLedger_p->_commitAdvisories(
        mqbc::ClusterStateLedgerCommitStatus::e_CANCELED);

    // Verify that self transitions back to unknown
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_UNKNOWN);
}

static void test15_followerCSLCommitFailure()
// FOLLOWER CSL COMMIT FAILURE
//
// Concerns:
//   Verify that the follower transitions back to unknown upon CSL commit
//   callback failure.
//
// Testing:
//   Follower upon CSL commit callback failure
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "FOLLOWER CSL COMMIT FAILURE");

    Tester tester(false);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    tester.electLeader(4U);
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_FOL_HEALING);

    // 1. Receive CSL advisory from leader.  Apply it.
    bmqp_ctrlmsg::LeaderAdvisory cslAdvisory;
    cslAdvisory.sequenceNumber().electorTerm()    = 3U;
    cslAdvisory.sequenceNumber().sequenceNumber() = 7U;

    tester.d_clusterStateLedger_p->apply(cslAdvisory);
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_FOL_HEALING);

    // 2. Invoke failure commit callback at the CSL
    tester.d_clusterStateLedger_p->_commitAdvisories(
        mqbc::ClusterStateLedgerCommitStatus::e_CANCELED);

    // Verify that self transitions back to unknown
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_UNKNOWN);
}

static void test16_followerClusterStateRespFailureLeaderNext()
// ------------------------------------------------------------------------
// FOLLOWER CLUSTER STATE RESP FAILURE LEADER NEXT
//
// Concerns:
//   Verify that after a failure follower cluster state response, when the
//   leader has highest LSN, it transitions from healing stage 2 to healed
//   upon successful CSL commit callback.
//
// Testing:
//   After failure follower cluster state response, leader transition to
//   healed upon successful CSL commit callback
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "CLUSTER STATE MANAGER - "
        "FOLLOWER CLUSTER STATE RESP FAILURE LEADER NEXT");

    Tester tester(true);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    // Set self LSN in the ledger to be the 2nd highest amongst all nodes
    bmqp_ctrlmsg::LeaderMessageSequence selfLSN;
    selfLSN.electorTerm()    = 1U;
    selfLSN.sequenceNumber() = 7U;
    tester.setSelfLedgerLSN(selfLSN);

    tester.electLeader(2U);
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);
    tester.verifyFollowerLSNRequestsSent();
    tester.clearChannels();

    // 1. Receives follower LSN response from all followers
    bmqp_ctrlmsg::ControlMessage         followerLSNResponse;
    bmqp_ctrlmsg::LeaderMessageSequence& lms =
        followerLSNResponse.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeFollowerLSNResponse()
            .sequenceNumber();

    followerLSNResponse.rId() = 1;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 2U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    followerLSNResponse.rId() = 2;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 8U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    followerLSNResponse.rId() = 3;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 5U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    // Verify that all LSNs are stored
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->nodeToLSNMap().size() ==
                    4U);

    // Self has transitioned to leader healing stage 2.  A follower cluster
    // state request has been sent to the highest LSN follower
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);
    tester.verifyFollowerClusterStateRequestSent();
    tester.clearChannels();

    // 2. Receive a failure follower cluster state response from the highest
    //    LSN follower
    bmqp_ctrlmsg::ControlMessage failureFollowerClusterStateResponse;
    failureFollowerClusterStateResponse.rId() = 4;  // 4th request so far :)
    bmqp_ctrlmsg::Status& status =
        failureFollowerClusterStateResponse.choice().makeStatus();
    status.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
    status.code()     = mqbi::ClusterErrorCode::e_NOT_FOLLOWER;
    status.message()  = k_REFUSAL_MESSAGE;

    tester.d_cluster_mp->requestManager().processResponse(
        failureFollowerClusterStateResponse);
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->nodeToLSNMap().size(),
                     3U);

    // 3. Since self is the next highest LSN node, it can directly transition
    //    to healed via applying to CSL
    //
    // Verify that a cluster state snapshot is applied to the CSL
    ClusterMessageCRefList advisories;
    tester.d_clusterStateLedger_p->uncommittedAdvisories(&advisories);
    BMQTST_ASSERT_EQ(advisories.size(), 1U);
    BMQTST_ASSERT(advisories.front().get().choice().isLeaderAdvisoryValue());

    const bmqp_ctrlmsg::LeaderAdvisory& advisory =
        advisories.front().get().choice().leaderAdvisory();
    BMQTST_ASSERT_EQ(advisory.sequenceNumber().electorTerm(), 2U);
    BMQTST_ASSERT_EQ(advisory.sequenceNumber().sequenceNumber(), 1U);

    // 4. Invoke success commit callback at the CSL
    tester.d_clusterStateLedger_p->_commitAdvisories(
        mqbc::ClusterStateLedgerCommitStatus::e_SUCCESS);

    // Verify that leader (self) has healed
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALED);

    const bmqp_ctrlmsg::LeaderMessageSequence& latestLSN =
        tester.d_cluster_mp->_clusterData()
            ->electorInfo()
            .leaderMessageSequence();
    BMQTST_ASSERT_EQ(latestLSN.electorTerm(), 2U);
    BMQTST_ASSERT_EQ(latestLSN.sequenceNumber(), 2U);
}

static void test17_followerClusterStateRespFailureFollowerNext()
// ------------------------------------------------------------------------
// FOLLOWER CLUSTER STATE RESP FAILURE FOLLOWER NEXT
//
// Concerns:
//   Verify that after a failure follower cluster state response, when
//   another follower has the highest LSN, the leader transitions from
//   healing stage 2 to healed upon follower cluster state response and
//   successful CSL commit callback.
//
// Testing:
//   After failure follower cluster state response, leader transition to
//   healed upon follower cluster state response and successful CSL commit
//   callback
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "CLUSTER STATE MANAGER - "
        "FOLLOWER CLUSTER STATE RESP FAILURE FOLLOWER NEXT");

    Tester tester(true);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    // Set self LSN in the ledger to be the lowest amongst all nodes
    bmqp_ctrlmsg::LeaderMessageSequence selfLSN;
    selfLSN.electorTerm()    = 1U;
    selfLSN.sequenceNumber() = 1U;
    tester.setSelfLedgerLSN(selfLSN);

    tester.electLeader(2U);
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);
    tester.verifyFollowerLSNRequestsSent();
    tester.clearChannels();

    // 1. Receives follower LSN response from all followers
    bmqp_ctrlmsg::ControlMessage         followerLSNResponse;
    bmqp_ctrlmsg::LeaderMessageSequence& lms =
        followerLSNResponse.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeFollowerLSNResponse()
            .sequenceNumber();

    followerLSNResponse.rId() = 1;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 2U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    followerLSNResponse.rId() = 2;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 8U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    followerLSNResponse.rId() = 3;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 5U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    // Verify that all LSNs are stored
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->nodeToLSNMap().size() ==
                    4U);

    // Self has transitioned to leader healing stage 2.
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);

    // Verify that a follower cluster state request is sent to the highest LSN
    // follower
    tester.verifyFollowerClusterStateRequestSent();
    tester.clearChannels();

    // 2. Receive a failure follower cluster state response from the highest
    //    LSN follower
    bmqp_ctrlmsg::ControlMessage failureFollowerClusterStateResponse;
    failureFollowerClusterStateResponse.rId() = 4;  // 4th request so far :)
    bmqp_ctrlmsg::Status& status =
        failureFollowerClusterStateResponse.choice().makeStatus();
    status.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
    status.code()     = mqbi::ClusterErrorCode::e_NOT_FOLLOWER;
    status.message()  = k_REFUSAL_MESSAGE;

    tester.d_cluster_mp->requestManager().processResponse(
        failureFollowerClusterStateResponse);
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->nodeToLSNMap().size(),
                     3U);

    // Verify that a follower cluster state request is sent to the next highest
    // LSN follower
    tester.verifyFollowerClusterStateRequestSent();

    // 3. Receive a follower cluster state response from the next highest LSN
    // follower
    bmqp_ctrlmsg::ControlMessage                followerClusterStateRespMsg;
    bmqp_ctrlmsg::FollowerClusterStateResponse& followerClusterStateResp =
        followerClusterStateRespMsg.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeFollowerClusterStateResponse();
    bmqp_ctrlmsg::LeaderAdvisory& clusterStateSnapshot =
        followerClusterStateResp.clusterStateSnapshot();

    followerClusterStateRespMsg.rId() = 5;

    clusterStateSnapshot.sequenceNumber().electorTerm()    = 1U;
    clusterStateSnapshot.sequenceNumber().sequenceNumber() = 5U;
    clusterStateSnapshot.partitions().resize(
        tester.d_cluster_mp->_state()->partitions().size());
    for (size_t i = 0; i < clusterStateSnapshot.partitions().size(); ++i) {
        clusterStateSnapshot.partitions()[i].partitionId() = i;
    }

    tester.d_cluster_mp->requestManager().processResponse(
        followerClusterStateRespMsg);
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);

    // Verify that a cluster state snapshot is applied to the CSL
    ClusterMessageCRefList advisories;
    tester.d_clusterStateLedger_p->uncommittedAdvisories(&advisories);
    BMQTST_ASSERT_EQ(advisories.size(), 1U);
    BMQTST_ASSERT(advisories.front().get().choice().isLeaderAdvisoryValue());

    const bmqp_ctrlmsg::LeaderAdvisory& advisory =
        advisories.front().get().choice().leaderAdvisory();
    BMQTST_ASSERT_EQ(advisory.sequenceNumber().electorTerm(), 2U);
    BMQTST_ASSERT_EQ(advisory.sequenceNumber().sequenceNumber(), 1U);

    // 4. Invoke success commit callback at the CSL
    tester.d_clusterStateLedger_p->_commitAdvisories(
        mqbc::ClusterStateLedgerCommitStatus::e_SUCCESS);

    // Verify that leader (self) has healed
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALED);

    const bmqp_ctrlmsg::LeaderMessageSequence& latestLSN =
        tester.d_cluster_mp->_clusterData()
            ->electorInfo()
            .leaderMessageSequence();
    BMQTST_ASSERT_EQ(latestLSN.electorTerm(), 2U);
    BMQTST_ASSERT_EQ(latestLSN.sequenceNumber(), 2U);
}

static void test18_followerClusterStateRespFailureLostQuorum()
// ------------------------------------------------------------------------
// FOLLOWER CLUSTER STATE RESP FAILURE LOST QUORUM
//
// Concerns:
//   Verify that after a failure follower cluster state response, if the
//   leader loses quorum, it transitions back from healing stage 2 to
//   healing stage 1 and re-sends follower LSN requests.
//
// Testing:
//   After losing quorum, leader transition back to healing stage 1
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "CLUSTER STATE MANAGER - "
        "FOLLOWER CLUSTER STATE RESP FAILURE LOST QUORUM");

    Tester tester(true);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    // Set self LSN in the ledger to be the lowest amongst all nodes
    bmqp_ctrlmsg::LeaderMessageSequence selfLSN;
    selfLSN.electorTerm()    = 1U;
    selfLSN.sequenceNumber() = 1U;
    tester.setSelfLedgerLSN(selfLSN);

    tester.electLeader(2U);
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);
    tester.verifyFollowerLSNRequestsSent();
    tester.clearChannels();

    // 1. Receives follower LSN response from a majority of (two) followers,
    //    but failure response from the last follower
    bmqp_ctrlmsg::ControlMessage         followerLSNResponse;
    bmqp_ctrlmsg::LeaderMessageSequence& lms =
        followerLSNResponse.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeFollowerLSNResponse()
            .sequenceNumber();

    followerLSNResponse.rId() = 1;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 5U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    followerLSNResponse.rId() = 2;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 8U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    bmqp_ctrlmsg::ControlMessage failureFollowerLSNResponse;
    failureFollowerLSNResponse.rId() = 3;
    bmqp_ctrlmsg::Status& status =
        failureFollowerLSNResponse.choice().makeStatus();
    status.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
    status.code()     = mqbi::ClusterErrorCode::e_NOT_FOLLOWER;
    status.message()  = k_REFUSAL_MESSAGE;
    tester.d_cluster_mp->requestManager().processResponse(
        failureFollowerLSNResponse);

    // Verify that the majority of follower LSNs are stored
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->nodeToLSNMap().size() ==
                    3U);

    // Self has transitioned to leader healing stage 2.
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);

    // Verify that a follower cluster state request is sent to the highest LSN
    // follower
    tester.verifyFollowerClusterStateRequestSent();
    tester.clearChannels();

    // 2. Receive a failure follower cluster state response from the highest
    //    LSN follower
    bmqp_ctrlmsg::ControlMessage failureFollowerClusterStateResponse;
    failureFollowerClusterStateResponse.rId() = 4;  // 4th request so far :)
    bmqp_ctrlmsg::Status& status2 =
        failureFollowerClusterStateResponse.choice().makeStatus();
    status2.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
    status2.code()     = mqbi::ClusterErrorCode::e_NOT_FOLLOWER;
    status2.message()  = k_REFUSAL_MESSAGE;

    tester.d_cluster_mp->requestManager().processResponse(
        failureFollowerClusterStateResponse);

    // Self has lost quorum.  Verify that self transitions back to leader
    // healing stage 1
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->nodeToLSNMap().size(),
                     2U);

    // Verify that self re-sends follower LSN requests to all followers
    tester.verifyFollowerLSNRequestsSent();
}

static void test19_stopNode()
// ------------------------------------------------------------------------
// STOP NODE
//
// Concerns:
//   Verify that self transitions to STOPPING state upon STOP_NODE event,
//   regardless of current state.
//
// Testing:
//   Self transition to STOPPING state
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - STOP NODE");

    // 1.) Stopping from Unknown state
    Tester tester1(true);  // isLeader
    BSLS_ASSERT_OPT(tester1.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    tester1.d_clusterStateManager_mp->stop();

    BMQTST_ASSERT_EQ(tester1.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_STOPPED);

    // 2.) Stopping from Follower Healing state
    Tester tester2(false);  // isLeader
    BSLS_ASSERT_OPT(tester2.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    tester2.electLeader();
    BSLS_ASSERT_OPT(tester2.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_FOL_HEALING);

    tester2.d_clusterStateManager_mp->stop();

    BMQTST_ASSERT_EQ(tester2.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_STOPPED);

    // 3.) Stopping from Follower Healed state
    Tester tester3(false);  // isLeader
    BSLS_ASSERT_OPT(tester3.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    tester3.electLeader(3U);

    bmqp_ctrlmsg::LeaderAdvisory cslAdvisory;
    cslAdvisory.sequenceNumber().electorTerm()    = 2U;
    cslAdvisory.sequenceNumber().sequenceNumber() = 1U;

    tester3.d_clusterStateLedger_p->apply(cslAdvisory);
    tester3.d_clusterStateLedger_p->_commitAdvisories(
        mqbc::ClusterStateLedgerCommitStatus::e_SUCCESS);

    BMQTST_ASSERT_EQ(tester3.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_FOL_HEALED);

    tester3.d_clusterStateManager_mp->stop();

    BMQTST_ASSERT_EQ(tester3.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_STOPPED);

    // 4.) Stopping from Leader Healing Stage 1 state
    Tester tester4(true);  // isLeader
    BSLS_ASSERT_OPT(tester4.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    tester4.electLeader();
    BSLS_ASSERT_OPT(tester4.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);

    tester4.d_clusterStateManager_mp->stop();

    BMQTST_ASSERT_EQ(tester4.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_STOPPED);

    // 5.) Stopping from Leader Healing Stage 2 state
    Tester tester5(true);  // isLeader
    BSLS_ASSERT_OPT(tester5.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    tester5.electLeader(2U);

    bmqp_ctrlmsg::ControlMessage         followerLSNResponse;
    bmqp_ctrlmsg::LeaderMessageSequence& lms =
        followerLSNResponse.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeFollowerLSNResponse()
            .sequenceNumber();

    followerLSNResponse.rId() = 1;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 2U;
    tester5.d_cluster_mp->requestManager().processResponse(
        followerLSNResponse);
    followerLSNResponse.rId() = 2;
    tester5.d_cluster_mp->requestManager().processResponse(
        followerLSNResponse);
    followerLSNResponse.rId() = 3;
    tester5.d_cluster_mp->requestManager().processResponse(
        followerLSNResponse);

    BSLS_ASSERT_OPT(tester5.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);

    tester5.d_clusterStateManager_mp->stop();

    BMQTST_ASSERT_EQ(tester5.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_STOPPED);

    // 6.) Stopping from Leader Healed state
    Tester tester6(true);  // isLeader
    BSLS_ASSERT_OPT(tester6.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    // Set self LSN in the ledger to be the highest amongst all nodes
    bmqp_ctrlmsg::LeaderMessageSequence selfLSN;
    selfLSN.electorTerm()    = 1U;
    selfLSN.sequenceNumber() = 8U;
    tester6.setSelfLedgerLSN(selfLSN);

    tester6.electLeader(2U);

    followerLSNResponse.rId() = 1;
    tester6.d_cluster_mp->requestManager().processResponse(
        followerLSNResponse);
    followerLSNResponse.rId() = 2;
    tester6.d_cluster_mp->requestManager().processResponse(
        followerLSNResponse);
    followerLSNResponse.rId() = 3;
    tester6.d_cluster_mp->requestManager().processResponse(
        followerLSNResponse);

    tester6.d_clusterStateLedger_p->_commitAdvisories(
        mqbc::ClusterStateLedgerCommitStatus::e_SUCCESS);

    BSLS_ASSERT_OPT(tester6.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALED);

    tester6.d_clusterStateManager_mp->stop();

    BMQTST_ASSERT_EQ(tester6.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_STOPPED);
}

static void test20_resetUnknownLeader()
// ------------------------------------------------------------------------
// RESET UNKNOWN LEADER
//
// Concerns:
//   Verify that upon RST_UNKNOWN event from elector, the leader goes back
//   to UNKNOWN state, and clears all its LSNs.
//
// Testing:
//   Upon RST_UNKNOWN event, the leader goes back to UNKNOWN state
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "RESET UNKNOWN LEADER");

    Tester tester(true);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    // Artificially set self LSN in the ledger to be higher than all followers
    bmqp_ctrlmsg::LeaderMessageSequence selfLSN;
    selfLSN.electorTerm()    = 1U;
    selfLSN.sequenceNumber() = 8U;
    tester.setSelfLedgerLSN(selfLSN);

    // 1.a.) Self transitions to Leader Healing Stage 1
    tester.electLeader(2U);
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);

    // 1.b.) Receives one follower LSN response
    bmqp_ctrlmsg::ControlMessage         followerLSNResponse;
    bmqp_ctrlmsg::LeaderMessageSequence& lms =
        followerLSNResponse.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeFollowerLSNResponse()
            .sequenceNumber();

    followerLSNResponse.rId() = 1;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 2U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    // 1.c.) Upon leader loss, which leads to RST_UNKNOWN event from elector,
    //       verify that self goes back to UNKNOWN state, and clears all its
    //       LSNs
    tester.d_cluster_mp->_clusterData()->electorInfo().setElectorInfo(
        mqbnet::ElectorState::e_FOLLOWER,
        2U,  // term
        0,   // leaderNode
        mqbc::ElectorInfoLeaderStatus::e_UNDEFINED);

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_UNKNOWN);

    const NodeToLSNMap& lsnMap =
        tester.d_clusterStateManager_mp->nodeToLSNMap();
    BMQTST_ASSERT(lsnMap.empty());

    // 2.a.) Self transitions to Leader Healing Stage 2, after receiving
    //       follower LSN responses from all followers
    tester.electLeader(3U);

    followerLSNResponse.rId() = 4;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    followerLSNResponse.rId() = 5;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    followerLSNResponse.rId() = 6;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);
    BSLS_ASSERT_OPT(lsnMap.size() == 4U);

    // 2.b.) Upon leader loss, which leads to RST_UNKNOWN event from elector,
    //       verify that self goes back to UNKNOWN state, and clears all its
    //       LSNs
    tester.d_cluster_mp->_clusterData()->electorInfo().setElectorInfo(
        mqbnet::ElectorState::e_FOLLOWER,
        3U,  // term
        0,   // leaderNode
        mqbc::ElectorInfoLeaderStatus::e_UNDEFINED);

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_UNKNOWN);
    BMQTST_ASSERT(lsnMap.empty());

    // 3.a.) Self transitions to Leader Healed, after receiving follower LSN
    //       responses from all followers and then upon CSL commit success
    tester.electLeader(4U);

    followerLSNResponse.rId() = 7;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    followerLSNResponse.rId() = 8;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    followerLSNResponse.rId() = 9;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);
    BSLS_ASSERT_OPT(lsnMap.size() == 4U);

    tester.d_clusterStateLedger_p->_commitAdvisories(
        mqbc::ClusterStateLedgerCommitStatus::e_SUCCESS);

    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALED);

    // 3.b.) Upon leader loss, which leads to RST_UNKNOWN event from elector,
    //       verify that self goes back to UNKNOWN state, and clears all its
    //       LSNs
    tester.d_cluster_mp->_clusterData()->electorInfo().setElectorInfo(
        mqbnet::ElectorState::e_FOLLOWER,
        4U,  // term
        0,   // leaderNode
        mqbc::ElectorInfoLeaderStatus::e_UNDEFINED);

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_UNKNOWN);
    BMQTST_ASSERT(lsnMap.empty());
}

static void test21_resetUnknownFollower()
// ------------------------------------------------------------------------
// RESET UNKNOWN FOLLOWER
//
// Concerns:
//   Verify that upon leader loss, the follower goes back to UNKNOWN state.
//
// Testing:
//   Upon leader loss, the follower goes back to UNKNOWN state
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "RESET UNKNOWN FOLLOWER");

    Tester tester(false);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    // 1.a.) Self transitions to Follower Healing
    tester.electLeader(2U);
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_FOL_HEALING);

    // 1.b.) Upon leader loss, verify that self goes back to UNKNOWN state
    tester.d_cluster_mp->_clusterData()->electorInfo().setElectorInfo(
        mqbnet::ElectorState::e_FOLLOWER,
        2U,  // term
        0,   // leaderNode
        mqbc::ElectorInfoLeaderStatus::e_UNDEFINED);

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_UNKNOWN);

    // 2.a.) Self transitions to Follower Healed, upon CSL commit success
    tester.electLeader(3U);

    bmqp_ctrlmsg::LeaderAdvisory cslAdvisory;
    cslAdvisory.sequenceNumber().electorTerm()    = 3U;
    cslAdvisory.sequenceNumber().sequenceNumber() = 1U;

    tester.d_clusterStateLedger_p->apply(cslAdvisory);
    tester.d_clusterStateLedger_p->_commitAdvisories(
        mqbc::ClusterStateLedgerCommitStatus::e_SUCCESS);

    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_FOL_HEALED);

    // 2.b.) Upon leader loss, verify that self goes back to UNKNOWN state
    tester.d_cluster_mp->_clusterData()->electorInfo().setElectorInfo(
        mqbnet::ElectorState::e_FOLLOWER,
        3U,  // term
        0,   // leaderNode
        mqbc::ElectorInfoLeaderStatus::e_UNDEFINED);

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_UNKNOWN);
}

static void test22_selectFollowerFromLeader()
// ------------------------------------------------------------------------
// SELECT FOLLOWER FROM LEADER
//
// Concerns:
//   Verify that when a new leader is elected, the existing leader
//   transitions to Follower Healing.
//
// Testing:
//   Upon new leader, existing leader transition to Follower Healing
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "SELECT FOLLOWER FROM LEADER");

    bmqp_ctrlmsg::LeaderMessageSequence selfLSN;
    selfLSN.electorTerm()    = 1U;
    selfLSN.sequenceNumber() = 5U;

    // 1.) Transition to follower from Leader Healing Stage 1
    Tester tester(true);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    tester.setSelfLedgerLSN(selfLSN);

    tester.electLeader(2U);
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);
    tester.verifyFollowerLSNRequestsSent();
    tester.clearChannels();

    mqbnet::ClusterNode* newLeader =
        tester.d_cluster_mp->netCluster().lookupNode(
            tester.d_cluster_mp->netCluster().selfNodeId() + 1);
    BSLS_ASSERT_OPT(newLeader != 0);
    tester.transitionToNewLeader(newLeader, 3U);

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_FOL_HEALING);

    tester.verifyRegistrationRequestSent(selfLSN);

    // 2.) Transition to follower from Leader Healing Stage 2
    Tester tester2(true);  // isLeader
    BSLS_ASSERT_OPT(tester2.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    tester2.setSelfLedgerLSN(selfLSN);

    tester2.electLeader(2U);
    tester2.verifyFollowerLSNRequestsSent();
    tester2.clearChannels();

    bmqp_ctrlmsg::ControlMessage         followerLSNResponse;
    bmqp_ctrlmsg::LeaderMessageSequence& lms =
        followerLSNResponse.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeFollowerLSNResponse()
            .sequenceNumber();

    followerLSNResponse.rId() = 1;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 8U;  // Follower LSNs are higher than leader's
    tester2.d_cluster_mp->requestManager().processResponse(
        followerLSNResponse);
    followerLSNResponse.rId() = 2;
    tester2.d_cluster_mp->requestManager().processResponse(
        followerLSNResponse);
    followerLSNResponse.rId() = 3;
    tester2.d_cluster_mp->requestManager().processResponse(
        followerLSNResponse);

    BSLS_ASSERT_OPT(tester2.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);
    tester2.verifyFollowerClusterStateRequestSent();
    tester2.clearChannels();

    newLeader = tester2.d_cluster_mp->netCluster().lookupNode(
        tester2.d_cluster_mp->netCluster().selfNodeId() + 1);
    BSLS_ASSERT_OPT(newLeader != 0);
    tester2.transitionToNewLeader(newLeader, 3U);

    BMQTST_ASSERT_EQ(tester2.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_FOL_HEALING);

    tester2.verifyRegistrationRequestSent(selfLSN);

    // 3.) Transition to follower from Leader Healed
    Tester tester3(true);  // isLeader
    BSLS_ASSERT_OPT(tester3.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    tester3.setSelfLedgerLSN(selfLSN);
    lms.sequenceNumber() = 2U;  // Follower LSNs are lower than leader's

    tester3.electLeader(2U);
    tester3.verifyFollowerLSNRequestsSent();
    tester3.clearChannels();

    followerLSNResponse.rId() = 1;
    tester3.d_cluster_mp->requestManager().processResponse(
        followerLSNResponse);
    followerLSNResponse.rId() = 2;
    tester3.d_cluster_mp->requestManager().processResponse(
        followerLSNResponse);
    followerLSNResponse.rId() = 3;
    tester3.d_cluster_mp->requestManager().processResponse(
        followerLSNResponse);

    tester3.d_clusterStateLedger_p->_commitAdvisories(
        mqbc::ClusterStateLedgerCommitStatus::e_SUCCESS);

    BSLS_ASSERT_OPT(tester3.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALED);
    tester3.clearChannels();

    newLeader = tester3.d_cluster_mp->netCluster().lookupNode(
        tester3.d_cluster_mp->netCluster().selfNodeId() + 1);
    BSLS_ASSERT_OPT(newLeader != 0);
    tester3.transitionToNewLeader(newLeader, 3U);

    BMQTST_ASSERT_EQ(tester3.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_FOL_HEALING);

    // Current self LSN should be (2,2) after becoming healed leader earlier
    bmqp_ctrlmsg::LeaderMessageSequence currentSelfLSN;
    currentSelfLSN.electorTerm()    = 2U;
    currentSelfLSN.sequenceNumber() = 2U;
    tester3.verifyRegistrationRequestSent(currentSelfLSN);
}

static void test23_selectLeaderFromFollower()
// ------------------------------------------------------------------------
// SELECT LEADER FROM FOLLOWER
//
// Concerns:
//   Verify that a follower successfully transitions to a leader when
//   elected.
//
// Testing:
//   Follower transition to leader
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "SELECT LEADER FROM FOLLOWER");

    bmqp_ctrlmsg::LeaderMessageSequence selfLSN;
    selfLSN.electorTerm()    = 1U;
    selfLSN.sequenceNumber() = 3U;

    // 1.) Transition to leader from Follower Healing
    Tester tester(false);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    tester.setSelfLedgerLSN(selfLSN);
    tester.electLeader(2U);
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_FOL_HEALING);
    tester.verifyRegistrationRequestSent(selfLSN);
    tester.clearChannels();

    tester.transitionToNewLeader(
        const_cast<mqbnet::ClusterNode*>(
            tester.d_cluster_mp->netCluster().selfNode()),
        3U);

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);

    tester.verifyFollowerLSNRequestsSent();

    // 2.) Transition to leader from Follower Healed
    Tester tester2(false);  // isLeader
    BSLS_ASSERT_OPT(tester2.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    tester2.setSelfLedgerLSN(selfLSN);
    tester2.electLeader(2U);
    tester2.verifyRegistrationRequestSent(selfLSN);

    bmqp_ctrlmsg::LeaderAdvisory cslAdvisory;
    cslAdvisory.sequenceNumber().electorTerm()    = 2U;
    cslAdvisory.sequenceNumber().sequenceNumber() = 1U;

    tester2.d_clusterStateLedger_p->apply(cslAdvisory);
    tester2.d_clusterStateLedger_p->_commitAdvisories(
        mqbc::ClusterStateLedgerCommitStatus::e_SUCCESS);

    BMQTST_ASSERT_EQ(tester2.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_FOL_HEALED);
    tester2.clearChannels();

    tester.transitionToNewLeader(
        const_cast<mqbnet::ClusterNode*>(
            tester.d_cluster_mp->netCluster().selfNode()),
        3U);

    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);

    tester.verifyFollowerLSNRequestsSent();
}

static void test24_watchDogLeader()
// ------------------------------------------------------------------------
// WATCHDOG LEADER
//
// Concerns:
//   Verify that the watchdog triggers upon timeout when the leader is
//   healing.
//
// Testing:
//   Watchdog for healing leader
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "WATCHDOG LEADER");

    Tester tester(true);  // isLeader
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_UNKNOWN);

    // Artificially set self LSN in the ledger to be higher than all followers
    bmqp_ctrlmsg::LeaderMessageSequence selfLSN;
    selfLSN.electorTerm()    = 1U;
    selfLSN.sequenceNumber() = 8U;
    tester.setSelfLedgerLSN(selfLSN);

    // 1.a.) Transition to Leader Healing Stage 1
    tester.electLeader(2U);
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);
    tester.verifyFollowerLSNRequestsSent();
    tester.clearChannels();

    // 1.b.) Trigger watchdog via timeout
    tester.d_cluster_mp->advanceTime(k_WATCHDOG_TIMEOUT_DURATION);
    tester.d_cluster_mp->waitForScheduler();

    // Verify that the watchdog triggers re-transition to Leader Healing
    // Stage 1, where we send follower LSN requests again.
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);
    tester.verifyFollowerLSNRequestsSent();
    tester.clearChannels();

    const NodeToLSNMap& lsnMap =
        tester.d_clusterStateManager_mp->nodeToLSNMap();
    BMQTST_ASSERT_EQ(lsnMap.size(), 1U);

    // 2.a.) Transition to Leader Healing Stage 2
    bmqp_ctrlmsg::ControlMessage         followerLSNResponse;
    bmqp_ctrlmsg::LeaderMessageSequence& lms =
        followerLSNResponse.choice()
            .makeClusterMessage()
            .choice()
            .makeClusterStateFSMMessage()
            .choice()
            .makeFollowerLSNResponse()
            .sequenceNumber();

    followerLSNResponse.rId() = 4;
    lms.electorTerm()         = 1U;
    lms.sequenceNumber()      = 2U;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);
    followerLSNResponse.rId() = 5;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);
    followerLSNResponse.rId() = 6;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALING_STG2);

    // 2.b.) Trigger watchdog via timeout
    tester.d_cluster_mp->advanceTime(k_WATCHDOG_TIMEOUT_DURATION);
    tester.d_cluster_mp->waitForScheduler();

    // Verify that the watchdog triggers re-transition to Leader Healing
    // Stage 1, where we send follower LSN requests again.
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALING_STG1);
    tester.verifyFollowerLSNRequestsSent();
    tester.clearChannels();
    BMQTST_ASSERT_EQ(lsnMap.size(), 1U);

    // 3.a.) Transition to Leader Healed
    followerLSNResponse.rId() = 7;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);
    followerLSNResponse.rId() = 8;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);
    followerLSNResponse.rId() = 9;
    tester.d_cluster_mp->requestManager().processResponse(followerLSNResponse);

    tester.d_clusterStateLedger_p->_commitAdvisories(
        mqbc::ClusterStateLedgerCommitStatus::e_SUCCESS);

    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_LDR_HEALED);

    // 3.b.) Attempt to trigger watchdog via timeout, but should fail
    tester.d_cluster_mp->advanceTime(k_WATCHDOG_TIMEOUT_DURATION);
    tester.d_cluster_mp->waitForScheduler();

    // Verify that watchdog did not trigger
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_LDR_HEALED);
}

static void test25_watchDogFollower()
// ------------------------------------------------------------------------
// WATCHDOG FOLLOWER
//
// Concerns:
//   Verify that the watchdog triggers upon timeout when the follower is
//   healing.
//
// Testing:
//   Watchdog for healing follower
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLUSTER STATE MANAGER - "
                                      "WATCHDOG FOLLOWER");

    Tester tester(false);  // isLeader

    bmqp_ctrlmsg::LeaderMessageSequence selfLSN;
    selfLSN.electorTerm()    = 1U;
    selfLSN.sequenceNumber() = 3U;
    tester.setSelfLedgerLSN(selfLSN);

    // 1.a.) Transition to Follower Healing
    tester.electLeader(2U);
    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_FOL_HEALING);
    tester.verifyRegistrationRequestSent(selfLSN);
    tester.clearChannels();

    // 1.b.) Trigger watch dog via timeout
    tester.d_cluster_mp->advanceTime(k_WATCHDOG_TIMEOUT_DURATION);
    tester.d_cluster_mp->waitForScheduler();

    // Verify that the watch dog triggers re-transition to Follower Healing.
    // where we send registration request again.
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_FOL_HEALING);
    tester.verifyRegistrationRequestSent(selfLSN);
    tester.clearChannels();

    // 2.a.) Transition to Follower Healed
    bmqp_ctrlmsg::LeaderAdvisory cslAdvisory;
    cslAdvisory.sequenceNumber().electorTerm()    = 2U;
    cslAdvisory.sequenceNumber().sequenceNumber() = 1U;

    tester.d_clusterStateLedger_p->apply(cslAdvisory);
    tester.d_clusterStateLedger_p->_commitAdvisories(
        mqbc::ClusterStateLedgerCommitStatus::e_SUCCESS);

    BSLS_ASSERT_OPT(tester.d_clusterStateManager_mp->healthState() ==
                    mqbc::ClusterStateTableState::e_FOL_HEALED);

    // 2.b.) Attempt to trigger watch dog via timeout, but should fail
    tester.d_cluster_mp->advanceTime(k_WATCHDOG_TIMEOUT_DURATION);
    tester.d_cluster_mp->waitForScheduler();

    // Verify that watch dog did not trigger
    BMQTST_ASSERT_EQ(tester.d_clusterStateManager_mp->healthState(),
                     mqbc::ClusterStateTableState::e_FOL_HEALED);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    switch (_testCase) {
    case 0:
    case 25: test25_watchDogFollower(); break;
    case 24: test24_watchDogLeader(); break;
    case 23: test23_selectLeaderFromFollower(); break;
    case 22: test22_selectFollowerFromLeader(); break;
    case 21: test21_resetUnknownFollower(); break;
    case 20: test20_resetUnknownLeader(); break;
    case 19: test19_stopNode(); break;
    case 18: test18_followerClusterStateRespFailureLostQuorum(); break;
    case 17: test17_followerClusterStateRespFailureFollowerNext(); break;
    case 16: test16_followerClusterStateRespFailureLeaderNext(); break;
    case 15: test15_followerCSLCommitFailure(); break;
    case 14: test14_leaderCSLCommitFailure(); break;
    case 13: test13_followerHealed(); break;
    case 12: test12_followerHighestLeaderHealed(); break;
    case 11: test11_leaderHighestLeaderHealed(); break;
    case 10: test10_followerClusterStateRequestHandlingFollower(); break;
    case 9: test9_followerLSNNoQuorum(); break;
    case 8: test8_registrationRequestQuorum(); break;
    case 7: test7_followerLSNResponseQuorum(); break;
    case 6: test6_registrationRequestHandlingLeader(); break;
    case 5: test5_followerLSNRequestHandlingFollower(); break;
    case 4: test4_invalidMessagesLeader(); break;
    case 3: test3_invalidMessagesUnknownState(); break;
    case 2: test2_breathingTestFollower(); break;
    case 1: test1_breathingTestLeader(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
    // Can't ensure no default memory is allocated because
    // 'bdlmt::EventSchedulerTestTimeSource' inside 'mqbmock::Cluster' uses
    // the default allocator in its constructor.
}
