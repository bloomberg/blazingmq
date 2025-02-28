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

// mqbnet_dummysession.t.cpp                                          -*-C++-*-
#include <mqbnet_clusteractivenodemanager.h>

// MQB
#include <mqbcfg_messages.h>
#include <mqbnet_cluster.h>
#include <mqbnet_mockcluster.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_string.h>

// TEST DRIVER
#include <bmqtst_scopedlogobserver.h>
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bmqtst::ScopedLogObserver logObserver(ball::Severity::ERROR,
                                          bmqtst::TestHelperUtil::allocator());

    mqbnet::Cluster::NodesList nodes;
    bsl::string                description = "dummy";
    bsl::string                dataCenter  = "east";

    mqbnet::ClusterActiveNodeManager mgr =
        mqbnet::ClusterActiveNodeManager(nodes, description, dataCenter);

    BMQTST_ASSERT(!mgr.activeNode());
    BMQTST_ASSERT(logObserver.records().empty());
}

static void test2_activeNodeWithinDC()
// Validate that an available node in the same data center is promptly
// selected as the active node. Nodes outside of the data center will not be
// selected until the selection criteria is explicitly extended.
{
    bmqtst::TestHelper::printTestName("ACTIVE NODE IN SAME DC");

    bmqtst::ScopedLogObserver logObserver(ball::Severity::ERROR,
                                          bmqtst::TestHelperUtil::allocator());

    // Set up mock cluster
    mqbcfg::ClusterDefinition clusterConfig(
        bmqtst::TestHelperUtil::allocator());
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    mqbnet::MockCluster mockCluster(clusterConfig,
                                    &bufferFactory,
                                    bmqtst::TestHelperUtil::allocator());

    // Populate cluster nodes
    // - 1 node in "east" data center
    // - 1 node in "west" data center
    mqbnet::Cluster::NodesList nodes;
    mqbcfg::ClusterNode clusterNodeConfig(bmqtst::TestHelperUtil::allocator());

    clusterNodeConfig.dataCenter() = "east";
    clusterNodeConfig.name()       = "east-1";
    mqbnet::MockClusterNode east1(&mockCluster,
                                  clusterNodeConfig,
                                  &bufferFactory,
                                  bmqtst::TestHelperUtil::allocator());
    nodes.push_back(&east1);

    clusterNodeConfig.dataCenter() = "west";
    clusterNodeConfig.name()       = "west-1";
    mqbnet::MockClusterNode west1(&mockCluster,
                                  clusterNodeConfig,
                                  &bufferFactory,
                                  bmqtst::TestHelperUtil::allocator());
    nodes.push_back(&west1);

    // Create ClusterActiveNodeManager
    bsl::string                      description = "dummy";
    bsl::string                      dataCenter  = "east";
    mqbnet::ClusterActiveNodeManager mgr =
        mqbnet::ClusterActiveNodeManager(nodes, description, dataCenter);
    BMQTST_ASSERT(!mgr.activeNode());

    bmqp_ctrlmsg::NegotiationMessage negotiationMessage(
        bmqtst::TestHelperUtil::allocator());
    negotiationMessage.makeClientIdentity().hostName() = "dummyIdentity";

    // "west" node up, it should not become active
    {
        int rc = mgr.onNodeUp(&west1, negotiationMessage.clientIdentity());
        BMQTST_ASSERT_EQ(rc, mqbnet::ClusterActiveNodeManager::e_NO_CHANGE);
        BMQTST_ASSERT(!mgr.activeNode());
    }

    // "east" node up, it should become active
    {
        int rc = mgr.onNodeUp(&east1, negotiationMessage.clientIdentity());
        BMQTST_ASSERT_EQ(rc, mqbnet::ClusterActiveNodeManager::e_NEW_ACTIVE);
        BMQTST_ASSERT_EQ(mgr.activeNode(), &east1);
    }

    // "east" node down, no active node
    {
        int rc = mgr.onNodeDown(&east1);
        BMQTST_ASSERT_EQ(rc, mqbnet::ClusterActiveNodeManager::e_LOST_ACTIVE);
        BMQTST_ASSERT(!mgr.activeNode());
    }

    // Refresh should not change active node
    {
        int rc = mgr.refresh();
        BMQTST_ASSERT_EQ(rc, mqbnet::ClusterActiveNodeManager::e_NO_CHANGE);
        BMQTST_ASSERT(!mgr.activeNode());
    }

    // Relax DC filter logic, "west" should become active
    {
        mgr.enableExtendedSelection();
        int rc = mgr.refresh();
        BMQTST_ASSERT_EQ(rc, mqbnet::ClusterActiveNodeManager::e_NEW_ACTIVE);
        BMQTST_ASSERT_EQ(mgr.activeNode(), &west1);
    }

    BMQTST_ASSERT(logObserver.records().empty());
}

static void test3_activeNodeOutsideDC()
// Validate that any available node can be promptly selected as the active
// node if the cluster does not have any nodes in the same data center as the
// local machine.
{
    bmqtst::TestHelper::printTestName("ACTIVE NDOE OUTSIDE DC");

    bmqtst::ScopedLogObserver logObserver(ball::Severity::ERROR,
                                          bmqtst::TestHelperUtil::allocator());

    // Set up mock cluster
    mqbcfg::ClusterDefinition clusterConfig(
        bmqtst::TestHelperUtil::allocator());
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    mqbnet::MockCluster mockCluster(clusterConfig,
                                    &bufferFactory,
                                    bmqtst::TestHelperUtil::allocator());

    // Populate cluster nodes
    // - 1 node in "east" data center
    // - 1 node in "west" data center
    mqbnet::Cluster::NodesList nodes;
    mqbcfg::ClusterNode clusterNodeConfig(bmqtst::TestHelperUtil::allocator());

    clusterNodeConfig.dataCenter() = "east";
    clusterNodeConfig.name()       = "east-1";
    mqbnet::MockClusterNode east1(&mockCluster,
                                  clusterNodeConfig,
                                  &bufferFactory,
                                  bmqtst::TestHelperUtil::allocator());
    nodes.push_back(&east1);

    clusterNodeConfig.dataCenter() = "west";
    clusterNodeConfig.name()       = "west-1";
    mqbnet::MockClusterNode west1(&mockCluster,
                                  clusterNodeConfig,
                                  &bufferFactory,
                                  bmqtst::TestHelperUtil::allocator());
    nodes.push_back(&west1);

    // Create ClusterActiveNodeManager
    bsl::string                      description = "dummy";
    bsl::string                      dataCenter  = "south";
    mqbnet::ClusterActiveNodeManager mgr =
        mqbnet::ClusterActiveNodeManager(nodes, description, dataCenter);
    BMQTST_ASSERT(!mgr.activeNode());

    bmqp_ctrlmsg::NegotiationMessage negotiationMessage(
        bmqtst::TestHelperUtil::allocator());
    negotiationMessage.makeClientIdentity().hostName() = "dummyIdentity";

    // "west" node up, it should become active
    {
        int rc = mgr.onNodeUp(&west1, negotiationMessage.clientIdentity());
        BMQTST_ASSERT_EQ(rc, mqbnet::ClusterActiveNodeManager::e_NEW_ACTIVE);
        BMQTST_ASSERT_EQ(mgr.activeNode(), &west1);
    }

    // "east" node up, it should become active
    {
        int rc = mgr.onNodeUp(&east1, negotiationMessage.clientIdentity());
        BMQTST_ASSERT_EQ(rc, mqbnet::ClusterActiveNodeManager::e_NO_CHANGE);
        BMQTST_ASSERT_EQ(mgr.activeNode(), &west1);
    }

    // "west" node down, "east" should become active
    {
        int rc = mgr.onNodeDown(&west1);
        BMQTST_ASSERT_EQ(rc,
                         mqbnet::ClusterActiveNodeManager::e_LOST_ACTIVE |
                             mqbnet::ClusterActiveNodeManager::e_NEW_ACTIVE);
        BMQTST_ASSERT_EQ(mgr.activeNode(), &east1);
    }

    BMQTST_ASSERT(logObserver.records().empty());
}

static void test4_panicInExtendedMode()
// Validate that a PANIC log is emitted if in extended mode and unable to
// select an active node.
{
    bmqtst::TestHelper::printTestName("ACTIVE NDOE OUTSIDE DC");

    bmqtst::ScopedLogObserver logObserver(ball::Severity::ERROR,
                                          bmqtst::TestHelperUtil::allocator());

    // Set up mock cluster
    mqbcfg::ClusterDefinition clusterConfig(
        bmqtst::TestHelperUtil::allocator());
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    mqbnet::MockCluster mockCluster(clusterConfig,
                                    &bufferFactory,
                                    bmqtst::TestHelperUtil::allocator());

    // Populate cluster nodes
    // - 1 node in "east" data center
    // - 1 node in "west" data center
    mqbnet::Cluster::NodesList nodes;
    mqbcfg::ClusterNode clusterNodeConfig(bmqtst::TestHelperUtil::allocator());

    clusterNodeConfig.dataCenter() = "east";
    clusterNodeConfig.name()       = "east-1";
    mqbnet::MockClusterNode east1(&mockCluster,
                                  clusterNodeConfig,
                                  &bufferFactory,
                                  bmqtst::TestHelperUtil::allocator());
    nodes.push_back(&east1);

    clusterNodeConfig.dataCenter() = "west";
    clusterNodeConfig.name()       = "west-1";
    mqbnet::MockClusterNode west1(&mockCluster,
                                  clusterNodeConfig,
                                  &bufferFactory,
                                  bmqtst::TestHelperUtil::allocator());
    nodes.push_back(&west1);

    // Create ClusterActiveNodeManager
    bsl::string                      description = "dummy";
    bsl::string                      dataCenter  = "east";
    mqbnet::ClusterActiveNodeManager mgr =
        mqbnet::ClusterActiveNodeManager(nodes, description, dataCenter);
    BMQTST_ASSERT(!mgr.activeNode());

    bmqp_ctrlmsg::NegotiationMessage negotiationMessage(
        bmqtst::TestHelperUtil::allocator());
    negotiationMessage.makeClientIdentity().hostName() = "dummyIdentity";

    // Refresh should not panic in normal mode
    {
        int rc = mgr.refresh();
        BMQTST_ASSERT_EQ(rc, mqbnet::ClusterActiveNodeManager::e_NO_CHANGE);
        BMQTST_ASSERT(logObserver.records().empty());
    }

    // Refresh with extended selection should panic
    {
        mgr.enableExtendedSelection();
        int rc = mgr.refresh();
        BMQTST_ASSERT_EQ(rc, mqbnet::ClusterActiveNodeManager::e_NO_CHANGE);
        BMQTST_ASSERT_EQ(1L, logObserver.records().size());
        BMQTST_ASSERT(
            logObserver.records().back().fixedFields().messageRef().find(
                "PANIC [CLUSTER_ACTIVE_NODE]") != bsl::string::npos);
    }

    // "west" node up, new active node
    {
        int rc = mgr.onNodeUp(&west1, negotiationMessage.clientIdentity());
        BMQTST_ASSERT_EQ(rc, mqbnet::ClusterActiveNodeManager::e_NEW_ACTIVE);
        BMQTST_ASSERT_EQ(mgr.activeNode(), &west1);
    }

    // "west" node down, should panic again
    {
        int rc = mgr.onNodeDown(&west1);
        BMQTST_ASSERT_EQ(rc, mqbnet::ClusterActiveNodeManager::e_LOST_ACTIVE);
        BMQTST_ASSERT_EQ(2L, logObserver.records().size());
        BMQTST_ASSERT(
            logObserver.records().back().fixedFields().messageRef().find(
                "PANIC [CLUSTER_ACTIVE_NODE]") != bsl::string::npos);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 4: test4_panicInExtendedMode(); break;
    case 3: test3_activeNodeOutsideDC(); break;
    case 2: test2_activeNodeWithinDC(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
