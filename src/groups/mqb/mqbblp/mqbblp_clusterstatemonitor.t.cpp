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

// mqbblp_clusterstatemonitor.t.cpp                                   -*-C++-*-
#include <mqbblp_clusterstatemonitor.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// MQB
#include <mqbc_clusterstate.h>
#include <mqbc_clusterutil.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbmock_cluster.h>
#include <mqbnet_cluster.h>
#include <mqbnet_mockcluster.h>
#include <mqbscm_version.h>
#include <mqbstat_brokerstats.h>

#include <bmqio_testchannel.h>
#include <bmqst_statcontext.h>
#include <bmqsys_time.h>
#include <bmqtst_scopedlogobserver.h>
#include <bmqu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <ball_severity.h>
#include <bdlb_nullablevalue.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bsl_utility.h>
#include <bsl_vector.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>
#include <bmqu_tempdirectory.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Object to aid in evaluating threshold notifications being emitted by the
/// ClusterStateMonitor.  Records the notifications via various members so
/// that the tester can verify the order and content of the notifications
/// emitted .
struct NotificationEvaluator : public mqbc::ClusterStateObserver {
    bsl::vector<size_t> d_partitionOrphanThresholds;
    // Vector of partition Ids that have
    // been reported to be orphan via a
    // threshold notification, in the order
    // in which they were reported.

    bsl::vector<mqbnet::ClusterNode*> d_nodeUnavailableThresholds;
    // Vector of nodes that have been
    // reported to be unavailable via a
    // threshold notification, in the order
    // in which they were reported.

    size_t d_numLeaderPassiveThresholds;
    // Number of times the leader passive
    // threshold notification was observed
    // by this object.

    size_t d_numFailoverThresholds;
    // Number of times the failover
    // threshold notification was observed
    // by this object.

    NotificationEvaluator(bslma::Allocator* allocator)
    : d_partitionOrphanThresholds(allocator)
    , d_nodeUnavailableThresholds(allocator)
    , d_numLeaderPassiveThresholds(0)
    , d_numFailoverThresholds(0)
    {
        // NOTHING
    }

    void onPartitionOrphanThreshold(size_t partitionId) BSLS_KEYWORD_OVERRIDE
    {
        d_partitionOrphanThresholds.push_back(partitionId);
    }

    void
    onNodeUnavailableThreshold(mqbnet::ClusterNode* node) BSLS_KEYWORD_OVERRIDE
    {
        d_nodeUnavailableThresholds.push_back(node);
    }

    void onLeaderPassiveThreshold() BSLS_KEYWORD_OVERRIDE
    {
        ++d_numLeaderPassiveThresholds;
    }

    void onFailoverThreshold() BSLS_KEYWORD_OVERRIDE
    {
        ++d_numFailoverThresholds;
    }
};

struct TestHelper {
    // Provide helper methods to abstract away the code needed to modify the
    // cluster state.

  public:
    // PUBLIC DATA
    bslma::ManagedPtr<mqbmock::Cluster> d_cluster_mp;

    bsl::vector<mqbnet::MockClusterNode*> d_nodes;

    bmqu::TempDirectory d_tempDir;

    // CREATORS
    TestHelper()
    : d_cluster_mp(0)
    , d_nodes(bmqtst::TestHelperUtil::allocator())
    , d_tempDir(bmqtst::TestHelperUtil::allocator())
    {
        mqbmock::Cluster::ClusterNodeDefs clusterNodeDefs(
            bmqtst::TestHelperUtil::allocator());

        mqbc::ClusterUtil::appendClusterNode(
            &clusterNodeDefs,
            "testNode1",
            "US-EAST",
            41234,
            mqbmock::Cluster::k_LEADER_NODE_ID,
            bmqtst::TestHelperUtil::allocator());
        mqbc::ClusterUtil::appendClusterNode(
            &clusterNodeDefs,
            "testNode2",
            "US-EAST",
            41235,
            mqbmock::Cluster::k_LEADER_NODE_ID + 1,
            bmqtst::TestHelperUtil::allocator());
        mqbc::ClusterUtil::appendClusterNode(
            &clusterNodeDefs,
            "testNode3",
            "US-WEST",
            41236,
            mqbmock::Cluster::k_LEADER_NODE_ID + 2,
            bmqtst::TestHelperUtil::allocator());
        mqbc::ClusterUtil::appendClusterNode(
            &clusterNodeDefs,
            "testNode4",
            "US-WEST",
            41237,
            mqbmock::Cluster::k_LEADER_NODE_ID + 3,
            bmqtst::TestHelperUtil::allocator());
        mqbc::ClusterUtil::appendClusterNode(
            &clusterNodeDefs,
            "testNode5",
            "US-WEST",
            41238,
            mqbmock::Cluster::k_LEADER_NODE_ID + 4,
            bmqtst::TestHelperUtil::allocator());

        d_cluster_mp.load(
            new (*bmqtst::TestHelperUtil::allocator())
                mqbmock::Cluster(bmqtst::TestHelperUtil::allocator(),
                                 true,   // isClusterMember
                                 false,  // isLeader
                                 false,  // isCSLMode
                                 false,  // isFSMWorkflow
                                 false,  // doesFSMwriteQLIST
                                 clusterNodeDefs,
                                 "testCluster",
                                 d_tempDir.path()),
            bmqtst::TestHelperUtil::allocator());

        // In some UTs, operations with cluster might be executed either
        // from the main thread or from the scheduler thread.
        // To pass `inDispatcherThread` checks (allow ANY thread):
        d_cluster_mp->setThreadId(mqbi::DispatcherClient::k_ANY_THREAD_ID);

        bmqsys::Time::initialize(
            bdlf::BindUtil::bind(&mqbmock::Cluster::getTime,
                                 d_cluster_mp.get()),
            bdlf::BindUtil::bind(&mqbmock::Cluster::getTime,
                                 d_cluster_mp.get()),
            bdlf::BindUtil::bind(&mqbmock::Cluster::getTimeInt64,
                                 d_cluster_mp.get()));

        for (mqbnet::Cluster::NodesList::iterator iter =
                 d_cluster_mp->netCluster().nodes().begin();
             iter != d_cluster_mp->netCluster().nodes().end();
             ++iter) {
            d_nodes.push_back(dynamic_cast<mqbnet::MockClusterNode*>(*iter));
        }
    }

    ~TestHelper() { bmqsys::Time::shutdown(); }

    // MANIPULATORS

    /// Set leader to specified `isActive` state.
    void setLeader(mqbnet::ClusterNode* node, bool isActive)
    {
        mqbc::ElectorInfoLeaderStatus::Enum status =
            isActive ? mqbc::ElectorInfoLeaderStatus::e_ACTIVE
                     : mqbc::ElectorInfoLeaderStatus::e_PASSIVE;

        if (d_cluster_mp->_clusterData()->electorInfo().leaderStatus() ==
                mqbc::ElectorInfoLeaderStatus::e_UNDEFINED &&
            status == mqbc::ElectorInfoLeaderStatus::e_ACTIVE) {
            // It is **prohibited** to set leader status directly from
            // e_UNDEFINED to e_ACTIVE, so we set to e_PASSIVE first then
            // immediately to e_ACTIVE
            d_cluster_mp->_clusterData()->electorInfo().setElectorInfo(
                mqbnet::ElectorState::e_LEADER,
                1,
                node,
                mqbc::ElectorInfoLeaderStatus::e_PASSIVE);
        }

        d_cluster_mp->_clusterData()->electorInfo().setElectorInfo(
            mqbnet::ElectorState::e_LEADER,
            1,
            node,
            status);
    }

    /// Set primary of specified `partition` to specified `isActive` state.
    void setPartition(int partition, mqbnet::ClusterNode* node, bool isActive)
    {
        bmqp_ctrlmsg::PrimaryStatus::Value status =
            isActive ? bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE
                     : bmqp_ctrlmsg::PrimaryStatus::E_PASSIVE;

        mqbc::ClusterNodeSession* ns =
            d_cluster_mp->_clusterData()->membership().getClusterNodeSession(
                node);
        BSLS_ASSERT_OPT(ns);

        d_cluster_mp->_state()
            ->setPartitionPrimary(partition, 1, ns)
            .setPartitionPrimaryStatus(partition, status);
    }

    /// Set specified `node` to specified `isActive` state.
    void setNode(mqbnet::ClusterNode* node, bool isActive)
    {
        bmqp_ctrlmsg::NodeStatus::Value status =
            isActive ? bmqp_ctrlmsg::NodeStatus::E_AVAILABLE
                     : bmqp_ctrlmsg::NodeStatus::E_UNKNOWN;
        d_cluster_mp->_clusterData()
            ->membership()
            .clusterNodeSessionMap()
            .find(node)
            ->second->setNodeStatus(status, status);
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure proper building and starting of the ClusterStateMonitor
//
// Plan:
//  1) Create a ClusterStateMonitor on the stack
//  2) Invoke start.
//  3) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bmqtst::ScopedLogObserver logObserver(ball::Severity::e_ERROR,
                                          bmqtst::TestHelperUtil::allocator());
    NotificationEvaluator notifications(bmqtst::TestHelperUtil::allocator());

    TestHelper helper;

    const bool k_IS_ACTIVE = true;

    helper.setLeader(helper.d_nodes[0], k_IS_ACTIVE);
    helper.setPartition(0, helper.d_nodes[0], k_IS_ACTIVE);
    helper.setPartition(1, helper.d_nodes[1], k_IS_ACTIVE);
    helper.setPartition(2, helper.d_nodes[2], k_IS_ACTIVE);
    helper.setPartition(3, helper.d_nodes[3], k_IS_ACTIVE);

    mqbblp::ClusterStateMonitor monitor(helper.d_cluster_mp->_clusterData(),
                                        helper.d_cluster_mp->_state(),
                                        bmqtst::TestHelperUtil::allocator());
    monitor.registerObserver(&notifications);

    bmqu::MemOutStream dummy;

    helper.d_cluster_mp->start(dummy);
    monitor.start();

    // False by default
    BMQTST_ASSERT_EQ(monitor.isHealthy(), false);
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 0U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 0U);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 0U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 0U);

    helper.d_cluster_mp->advanceTime(1);
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 0U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 0U);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 0U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 0U);

    helper.d_cluster_mp->stop();
    monitor.stop();
    monitor.unregisterObserver(&notifications);

    // No alarms emitted
    BMQTST_ASSERT_EQ(logObserver.records().size(), 0U);
}

static void test2_checkAlarmsWithResetTest()
// ------------------------------------------------------------------------
// CHECK MULTIPLE ALARMS WITH RESET TEST
//
// Concerns:
//   Ensure proper building and starting of the ClusterStateMonitor
//
// Plan:
//  1) Create a ClusterStateMonitor on the stack
//  2) Trigger and reset various alarms and ensure correctness.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CHECK ALARMS WITH RESET");

    bmqtst::ScopedLogObserver logObserver(ball::Severity::e_ERROR,
                                          bmqtst::TestHelperUtil::allocator());
    NotificationEvaluator notifications(bmqtst::TestHelperUtil::allocator());

    TestHelper helper;

    const bool k_IS_ACTIVE = true;

    const mqbcfg::ClusterMonitorConfig& config =
        helper.d_cluster_mp->_clusterDefinition().clusterMonitorConfig();

    helper.setLeader(helper.d_nodes[0], k_IS_ACTIVE);
    helper.setPartition(0, helper.d_nodes[0], k_IS_ACTIVE);
    helper.setPartition(1, helper.d_nodes[1], k_IS_ACTIVE);
    helper.setPartition(2, helper.d_nodes[2], k_IS_ACTIVE);
    helper.setPartition(3, helper.d_nodes[3], k_IS_ACTIVE);

    mqbblp::ClusterStateMonitor monitor(helper.d_cluster_mp->_clusterData(),
                                        helper.d_cluster_mp->_state(),
                                        bmqtst::TestHelperUtil::allocator());

    monitor.registerObserver(&notifications);

    bmqu::MemOutStream dummy;

    helper.d_cluster_mp->start(dummy);
    monitor.start();

    // False by default
    BMQTST_ASSERT_EQ(monitor.isHealthy(), false);

    // T: 1
    helper.d_cluster_mp->advanceTime(1);
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 0U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 0U);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 0U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 0U);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 0U);

    // [T = 1]: Set 1 partition(3) to inactive primary and wait for alarm
    helper.setPartition(3, helper.d_nodes[3], !k_IS_ACTIVE);

    // T: 121
    // - 1 partition orphan notification
    // - 1 bad state alarm
    helper.d_cluster_mp->advanceTime(config.maxTimeMaster());
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 1U);
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds[0], 3U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 0U);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 0U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 0U);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 1U);

    // False if any state goes bad
    BMQTST_ASSERT_EQ(monitor.isHealthy(), false);

    // [T = 121]: Set leader to inactive
    helper.setLeader(helper.d_nodes[0], !k_IS_ACTIVE);

    // T: 151
    // - 1 leader passive notification
    helper.d_cluster_mp->advanceTime(config.thresholdLeader());
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 1U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 0U);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 1U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 0U);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 1U);

    helper.setLeader(helper.d_nodes[0], !k_IS_ACTIVE);

    // T: 181
    // - 1 leader passive notification
    // - 1 partition orphan notification
    helper.d_cluster_mp->advanceTime(config.thresholdLeader());
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 2U);
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds[1], 3U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 0U);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 2U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 0U);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 1U);

    // [T = 181] set node to false
    helper.setNode(helper.d_nodes[0], !k_IS_ACTIVE);

    // T: 241
    // - 1 partition orphan notification
    // - 1 node unavailable notification
    // - 1 leader passive notifications
    // - 1 bad state alarm
    helper.d_cluster_mp->advanceTime(config.thresholdNode());
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 3U);
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds[2], 3U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 1U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds[0],
                     helper.d_nodes[0]);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 3U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 0U);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 2U);

    // T: 301
    // - 1 partition orphan notification
    // - 1 node unavailable notification
    // - 1 leader passive notifications
    // - 1 bad state alarm
    helper.d_cluster_mp->advanceTime(config.thresholdNode());
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 4U);
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds[3], 3U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 2U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds[1],
                     helper.d_nodes[0]);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 4U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 0U);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 3U);

    // [T = 301] Reset leader, partition, failover state, and node to good
    //           state and things should go back to good state immediately
    helper.setPartition(3, helper.d_nodes[3], k_IS_ACTIVE);
    helper.setLeader(helper.d_nodes[0], k_IS_ACTIVE);
    helper.d_cluster_mp->_setIsRestoringState(false);
    helper.setNode(helper.d_nodes[0], k_IS_ACTIVE);

    // T: 421
    // - No more thresholds or bad state notifications emitted
    helper.d_cluster_mp->advanceTime(config.maxTimeMaster());
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 4U);
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds[3], 3U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 2U);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 4U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 0U);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 3U);

    // Back to healthy
    BMQTST_ASSERT_EQ(monitor.isHealthy(), true);

    // [T = 421] We are in clean state here, change a partition primary to only
    // a short time and assert that a threshold notification is emitted
    helper.setPartition(2, helper.d_nodes[1], !k_IS_ACTIVE);

    // T: 481
    // - 1 partition orphan notification
    helper.d_cluster_mp->advanceTime(config.thresholdMaster());
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 5U);
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds[4], 2U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 2U);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 4U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 0U);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 3U);

    // [T = 481] Restore partition to valid
    helper.setPartition(2, helper.d_nodes[1], k_IS_ACTIVE);

    // [T: 481] Set 'isRestoring' to true
    helper.d_cluster_mp->_setIsRestoringState(true);

    // T: 601
    // - 1 failover threshold notification
    helper.d_cluster_mp->advanceTime(config.thresholdFailover());
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 5U);
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds[4], 2U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 2U);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 4U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 1U);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 3U);

    // T: 721
    // - 1 failover threshold notification
    // - 1 alarm
    helper.d_cluster_mp->advanceTime(config.maxTimeFailover() -
                                     config.thresholdFailover());
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 5U);
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds[4], 2U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 2U);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 4U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 2U);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 4U);

    // [T = 721] Restore 'isRestoring' to false (good state)
    helper.d_cluster_mp->_setIsRestoringState(false);

    // - No more thresholds or bad state notifications emitted
    helper.d_cluster_mp->advanceTime(config.maxTimeMaster());
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 5U);
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds[4], 2U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 2U);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 4U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 2U);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 4U);

    helper.d_cluster_mp->stop();
    monitor.stop();
    monitor.unregisterObserver(&notifications);
}

static void test3_alwaysInvalidStateTest()
// ------------------------------------------------------------------------
// ALWAYS INVALID STATE TEST
//
// Concerns:
//   Ensure triggering of alarms is correct even if always in invalid state
//
// Plan:
//  1) Create a ClusterStateMonitor on the stack
//  2) Provide an invalid state, then ensure alarms are not triggered
//     before the correct time has passed.
//
// Testing:
//   Edge case.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ALWAYS INVALID STATE");

    bmqtst::ScopedLogObserver logObserver(ball::Severity::e_ERROR,
                                          bmqtst::TestHelperUtil::allocator());
    NotificationEvaluator notifications(bmqtst::TestHelperUtil::allocator());

    TestHelper helper;

    const mqbcfg::ClusterMonitorConfig& config =
        helper.d_cluster_mp->_clusterDefinition().clusterMonitorConfig();

    const bool k_IS_ACTIVE = true;

    // [T = 0]: Set all individual state constituents to invalid
    helper.setLeader(helper.d_nodes[0], !k_IS_ACTIVE);
    helper.setPartition(0, helper.d_nodes[0], !k_IS_ACTIVE);
    helper.setPartition(1, helper.d_nodes[1], !k_IS_ACTIVE);
    helper.setPartition(2, helper.d_nodes[2], !k_IS_ACTIVE);
    helper.setPartition(3, helper.d_nodes[3], !k_IS_ACTIVE);
    helper.setNode(helper.d_nodes[0], !k_IS_ACTIVE);
    helper.setNode(helper.d_nodes[1], !k_IS_ACTIVE);
    helper.setNode(helper.d_nodes[2], !k_IS_ACTIVE);
    helper.setNode(helper.d_nodes[3], !k_IS_ACTIVE);
    helper.setNode(helper.d_nodes[4], !k_IS_ACTIVE);
    helper.d_cluster_mp->_setIsRestoringState(true);

    mqbblp::ClusterStateMonitor monitor(helper.d_cluster_mp->_clusterData(),
                                        helper.d_cluster_mp->_state(),
                                        bmqtst::TestHelperUtil::allocator());
    monitor.registerObserver(&notifications);
    bmqu::MemOutStream dummy;

    // No state is ever valid so always not healthy
    BMQTST_ASSERT_EQ(monitor.isHealthy(), false);

    helper.d_cluster_mp->start(dummy);
    monitor.start();

    // T: 30
    // - 1 leader passive notification
    helper.d_cluster_mp->advanceTime(config.thresholdLeader());
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 0U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 0U);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 1U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 0U);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 0U);

    // T: 59
    // - Nothing (advancing by a time interval which is just smaller than the
    //   interval needed to transition from threshold to alarming)
    helper.d_cluster_mp->advanceTime(config.maxTimeLeader() -
                                     config.thresholdLeader() - 1);
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 0U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 0U);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 1U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 0U);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 0U);

    // T: 61
    // - 4 partition orphan notifications (1 for each partition)
    // - 5 node unavailable notifications (1 for each partition)
    // - 1 leader passive notification
    // - 1 bad state alarm (triggered by leader's bad state)
    helper.d_cluster_mp->advanceTime(2);
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 4U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 5U);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 2U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 0U);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 1U);

    // T: 121
    // - 4 partition orphan notifications (1 for each partition)
    // - 5 node unavailable notifications (1 for each partition)
    // - 1 leader passive notification
    // - 1 failover threshold notification
    // - 1 bad state alarm (triggered by leader's bad state)
    helper.d_cluster_mp->advanceTime(config.maxTimeLeader());
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 8U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 10U);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 3U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 1U);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 2U);

    // T: 181
    // - 4 partition orphan notifications (1 for each partition)
    // - 5 node unavailable notifications (1 for each partition)
    // - 1 leader passive notification
    // - 1 bad state alarm (triggered by leader's bad state)
    helper.d_cluster_mp->advanceTime(config.maxTimeLeader());
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 12U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 15U);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 4U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 1U);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 3U);

    // T: 241
    // - 4 partition orphan notifications (1 for each partition)
    // - 5 node unavailable notifications (1 for each partition)
    // - 1 leader passive notification
    // - 1 failover threshold notification
    // - 1 bad state alarm (triggered by leader's bad state)
    helper.d_cluster_mp->advanceTime(config.maxTimeLeader());
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 16U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 20U);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 5U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 2U);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 4U);

    // All partitions are in bad state
    BMQTST_ASSERT_EQ(monitor.isHealthy(), false);

    // set all to true and ensure resets are invoked
    helper.setPartition(0, helper.d_nodes[0], k_IS_ACTIVE);
    helper.setPartition(1, helper.d_nodes[1], k_IS_ACTIVE);
    helper.setPartition(2, helper.d_nodes[2], k_IS_ACTIVE);
    helper.setPartition(3, helper.d_nodes[3], k_IS_ACTIVE);
    helper.setLeader(helper.d_nodes[0], k_IS_ACTIVE);
    helper.setNode(helper.d_nodes[0], k_IS_ACTIVE);
    helper.setNode(helper.d_nodes[1], k_IS_ACTIVE);
    helper.setNode(helper.d_nodes[2], k_IS_ACTIVE);
    helper.setNode(helper.d_nodes[3], k_IS_ACTIVE);
    helper.setNode(helper.d_nodes[4], k_IS_ACTIVE);
    helper.d_cluster_mp->_setIsRestoringState(false);

    // Cluster should still be viewed as unhealthy
    BMQTST_ASSERT_EQ(monitor.isHealthy(), false);

    // T: 481
    // - nothing new
    helper.d_cluster_mp->advanceTime(config.maxTimeFailover());
    helper.d_cluster_mp->waitForScheduler();
    BMQTST_ASSERT_EQ(notifications.d_partitionOrphanThresholds.size(), 16U);
    BMQTST_ASSERT_EQ(notifications.d_nodeUnavailableThresholds.size(), 20U);
    BMQTST_ASSERT_EQ(notifications.d_numLeaderPassiveThresholds, 5U);
    BMQTST_ASSERT_EQ(notifications.d_numFailoverThresholds, 2U);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 4U);

    // All valid states so should be good
    BMQTST_ASSERT_EQ(monitor.isHealthy(), true);

    helper.d_cluster_mp->stop();
    monitor.stop();
    monitor.unregisterObserver(&notifications);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    {
        bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

        mqbcfg::AppConfig brokerConfig(bmqtst::TestHelperUtil::allocator());
        mqbcfg::BrokerConfig::set(brokerConfig);

        bsl::shared_ptr<bmqst::StatContext> statContext =
            mqbstat::BrokerStatsUtil::initializeStatContext(
                30,
                bmqtst::TestHelperUtil::allocator());

        switch (_testCase) {
        case 0:
        case 3: test3_alwaysInvalidStateTest(); break;
        case 2: test2_checkAlarmsWithResetTest(); break;
        case 1: test1_breathingTest(); break;
        default: {
            cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
            bmqtst::TestHelperUtil::testStatus() = -1;
        } break;
        }

        bmqp::ProtocolUtil::shutdown();
    }

    TEST_EPILOG(bmqtst::TestHelper::e_DEFAULT);
    // Can't ensure no global memory is allocated because
    // 'bslmt::ThreadUtil::create()' uses the global allocator to allocate
    // memory.

    // Can't ensure no default memory is allocated because
    // 'mqbnet_multirequestmanager' constructor uses the default allocator to
    // allocate memory
}
