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

#include <mqbc_clusterstate.h>

// MQB
#include <mqbc_clusterutil.h>
#include <mqbmock_cluster.h>
#include <mqbu_storagekey.h>

#include <bmqu_tempdirectory.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_managedptr.h>
#include <bslmf_assert.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

namespace {
// TYPES
typedef bsl::unordered_map<bsl::string, int> TestData;

/// Assign a queue with the specified `uri` and `partitionId` into the
/// specified `state`.
void assign(mqbc::ClusterState* state, const char* uri, int partitionId)
{
    bmqp_ctrlmsg::QueueInfo info(bmqtst::TestHelperUtil::allocator());
    info.uri()         = uri;
    info.partitionId() = partitionId;
    mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(), "ABCDEF1234")
        .loadBinary(&info.key());

    state->assignQueue(info);
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_partitionIdExtractor()
// ------------------------------------------------------------------------
// Testing:
//    PartitionIdExtractor
// ------------------------------------------------------------------------
{
    mqbc::ClusterState::PartitionIdExtractor extractor(
        bmqtst::TestHelperUtil::allocator());

    TestData testData(bmqtst::TestHelperUtil::allocator());
    testData.emplace(bsl::string("test", bmqtst::TestHelperUtil::allocator()),
                     -1);
    testData.emplace(bsl::string("123", bmqtst::TestHelperUtil::allocator()),
                     -1);
    testData.emplace(bsl::string("test.123.test",
                                 bmqtst::TestHelperUtil::allocator()),
                     -1);
    testData.emplace(bsl::string("test.123.test.test",
                                 bmqtst::TestHelperUtil::allocator()),
                     123);
    testData.emplace(bsl::string("test.-1.test.test",
                                 bmqtst::TestHelperUtil::allocator()),
                     -1);

    TestData::const_iterator cIt = testData.begin();
    for (; cIt != testData.end(); ++cIt) {
        int result = extractor.extract(cIt->first);
        BMQTST_ASSERT_EQ(result, cIt->second);
    }
}

static void test2_clearQueues()
// ------------------------------------------------------------------------
// CLEAR QUEUES
//
// Concerns:
//   'clearQueues' empties the state when several domains hold queues.
//   'unassignQueue' erases a domain once its last queue goes, so every
//   iterator into the domain map dies under it.
//
// Testing:
//    ClusterState::clearQueues
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLEAR QUEUES");

    bmqu::TempDirectory tempDir(bmqtst::TestHelperUtil::allocator());

    mqbmock::Cluster::ClusterNodeDefs nodeDefs(
        bmqtst::TestHelperUtil::allocator());
    mqbc::ClusterUtil::appendClusterNode(&nodeDefs,
                                         "E1",
                                         "US-EAST",
                                         41234,
                                         mqbmock::Cluster::k_LEADER_NODE_ID,
                                         bmqtst::TestHelperUtil::allocator());
    mqbc::ClusterUtil::appendClusterNode(&nodeDefs,
                                         "E2",
                                         "US-EAST",
                                         41235,
                                         mqbmock::Cluster::k_LEADER_NODE_ID +
                                             1,
                                         bmqtst::TestHelperUtil::allocator());

    mqbmock::Cluster cluster(bmqtst::TestHelperUtil::allocator(),
                             true,   // isClusterMember
                             true,   // isLeader
                             true,   // isFSMWorkflow
                             false,  // doesFSMwriteQLIST
                             nodeDefs,
                             "testCluster",
                             tempDir.path());

    mqbc::ClusterState* state = cluster._state();

    // Two domains, one with several queues and one with a single queue: the
    // single-queue domain is erased by 'unassignQueue' itself.
    assign(state, "bmq://bmq.test.domain1/q1", 0);
    assign(state, "bmq://bmq.test.domain1/q2", 1);
    assign(state, "bmq://bmq.test.domain1/q3", 0);
    assign(state, "bmq://bmq.test.domain2/q1", 1);
    BMQTST_ASSERT_EQ(state->domainStates().size(), 2u);

    state->clearQueues();

    BMQTST_ASSERT(state->domainStates().empty());
    for (int pid = 0; pid < 2; ++pid) {
        BMQTST_ASSERT_EQ(state->partitions()[pid].numQueuesMapped(), 0);
    }

    // Idempotent on an already empty state.
    state->clearQueues();
    BMQTST_ASSERT(state->domainStates().empty());
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_clearQueues(); break;
    case 1: test1_partitionIdExtractor(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    // 'e_CHECK_GBL_ALLOC', not 'e_CHECK_DEF_GBL_ALLOC': 'mqbmock::Cluster'
    // allocates from the default allocator, as in every other test in this
    // package that builds one.
    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
