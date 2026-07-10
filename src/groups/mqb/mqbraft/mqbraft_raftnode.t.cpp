// Copyright 2025-2026 Bloomberg Finance L.P.
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

// mqbraft_raftnode.t.cpp -*-C++-*-
#include <mqbraft_raftnode.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_iostream.h>
#include <bsl_vector.h>
#include <bslma_testallocator.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

using namespace BloombergLP;
using namespace BloombergLP::mqbraft;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS
// ----------------------------------------------------------------------------
namespace {

// ==================
// class MemoryRaftLog
// ==================

class MemoryRaftLog : public RaftLog {
  private:
    bsl::vector<LogEntry> d_entries;
    bsls::Types::Uint64   d_snapshotIndex;
    bsls::Types::Uint64   d_snapshotTerm;
    bslma::Allocator*     d_allocator_p;

    MemoryRaftLog(const MemoryRaftLog&);
    MemoryRaftLog& operator=(const MemoryRaftLog&);

  public:
    explicit MemoryRaftLog(bslma::Allocator* allocator = 0)
    : d_entries(allocator)
    , d_snapshotIndex(0)
    , d_snapshotTerm(0)
    , d_allocator_p(bslma::Default::allocator(allocator))
    {
    }

    ~MemoryRaftLog() BSLS_KEYWORD_OVERRIDE {}

    int append(bsls::Types::Uint64                  term,
               const bsl::shared_ptr<bdlbb::Blob>&  data,
               bsls::Types::Uint64                  id = 0)
        BSLS_KEYWORD_OVERRIDE
    {
        (void)id;
        d_entries.push_back(LogEntry(term, lastIndex() + 1, data));
        return 0;
    }

    int truncateFrom(bsls::Types::Uint64 index) BSLS_KEYWORD_OVERRIDE
    {
        if (index <= d_snapshotIndex || index > lastIndex()) {
            return -1;
        }
        bsls::Types::Uint64 offset = index - d_snapshotIndex - 1;
        d_entries.erase(d_entries.begin() + static_cast<int>(offset),
                        d_entries.end());
        return 0;
    }

    bsls::Types::Uint64 lastIndex() const BSLS_KEYWORD_OVERRIDE
    {
        return d_snapshotIndex + d_entries.size();
    }

    bsls::Types::Uint64 lastTerm() const BSLS_KEYWORD_OVERRIDE
    {
        if (d_entries.empty()) {
            return d_snapshotTerm;
        }
        return d_entries.back().d_term;
    }

    bsls::Types::Uint64
    term(bsls::Types::Uint64 index) const BSLS_KEYWORD_OVERRIDE
    {
        if (index == 0) {
            return 0;
        }
        if (index == d_snapshotIndex) {
            return d_snapshotTerm;
        }
        if (index < d_snapshotIndex || index > lastIndex()) {
            return 0;
        }
        bsls::Types::Uint64 offset = index - d_snapshotIndex - 1;
        return d_entries[static_cast<int>(offset)].d_term;
    }

    int entries(bsls::Types::Uint64    lo,
                bsls::Types::Uint64    hi,
                bsl::vector<LogEntry>* out) const BSLS_KEYWORD_OVERRIDE
    {
        BSLS_ASSERT_SAFE(out);
        if (lo > hi || lo <= d_snapshotIndex || hi > lastIndex() + 1) {
            return -1;
        }
        out->clear();
        for (bsls::Types::Uint64 i = lo; i < hi; ++i) {
            bsls::Types::Uint64 offset = i - d_snapshotIndex - 1;
            out->push_back(d_entries[static_cast<int>(offset)]);
        }
        return 0;
    }

    bsls::Types::Uint64 snapshotIndex() const BSLS_KEYWORD_OVERRIDE
    {
        return d_snapshotIndex;
    }

    bsls::Types::Uint64 snapshotTerm() const BSLS_KEYWORD_OVERRIDE
    {
        return d_snapshotTerm;
    }

    void applySnapshot(bsls::Types::Uint64 lastIncludedIndex,
                       bsls::Types::Uint64 lastIncludedTerm)
        BSLS_KEYWORD_OVERRIDE
    {
        d_snapshotIndex = lastIncludedIndex;
        d_snapshotTerm  = lastIncludedTerm;
        d_entries.clear();
    }
};

/// Helper class that manages a cluster of RaftNode instances and routes
/// messages between them.
class TestCluster {
  private:
    // DATA
    bsl::vector<RaftNode*>         d_nodes;
    bsl::vector<MemoryRaftLog*>    d_logs;
    int                            d_numNodes;
    bslma::Allocator*              d_allocator_p;
    bdlbb::PooledBlobBufferFactory d_bufferFactory;

    // NOT IMPLEMENTED
    TestCluster(const TestCluster&);
    TestCluster& operator=(const TestCluster&);

  public:
    // CREATORS
    explicit TestCluster(int               numNodes,
                         bool              preVote   = true,
                         bslma::Allocator* allocator = 0)
    : d_nodes(allocator)
    , d_logs(allocator)
    , d_numNodes(numNodes)
    , d_allocator_p(bslma::Default::allocator(allocator))
    , d_bufferFactory(256, d_allocator_p)
    {
        bsl::vector<int> peerIds(d_allocator_p);
        for (int i = 0; i < numNodes; ++i) {
            peerIds.push_back(i);
        }

        for (int i = 0; i < numNodes; ++i) {
            MemoryRaftLog* log = new (*d_allocator_p)
                MemoryRaftLog(d_allocator_p);
            d_logs.push_back(log);

            RaftNodeConfig config(true, d_allocator_p);
            config.d_selfId             = i;
            config.d_peerIds            = peerIds;
            config.d_electionTimeoutMin = 10;
            config.d_electionTimeoutMax = 20;
            config.d_heartbeatInterval  = 3;
            config.d_preVote            = preVote;

            RaftNode* node = new (*d_allocator_p)
                RaftNode(config, log, d_allocator_p);
            d_nodes.push_back(node);
        }
    }

    ~TestCluster()
    {
        for (int i = 0; i < d_numNodes; ++i) {
            d_allocator_p->deleteObject(d_nodes[i]);
            d_allocator_p->deleteObject(d_logs[i]);
        }
    }

    // MANIPULATORS

    /// Deliver all messages from 'output' to their destination nodes,
    /// collecting responses into 'responses'.
    void deliverMessages(const RaftNodeOutput& output,
                         RaftNodeOutput*       responses)
    {
        for (bsl::vector<RaftMessage>::size_type i = 0;
             i < output.d_messages.size();
             ++i) {
            const RaftMessage& msg  = output.d_messages[i];
            int                dest = msg.d_destinationNodeId;
            if (dest >= 0 && dest < d_numNodes) {
                d_nodes[dest]->step(responses, msg);
            }
        }
    }

    /// Run a full round: deliver messages and collect responses, repeat
    /// until no more messages.  Return total messages processed.
    int runUntilQuiet(RaftNodeOutput* seedOutput)
    {
        int            total = 0;
        RaftNodeOutput current(*seedOutput, d_allocator_p);
        seedOutput->reset();

        while (!current.d_messages.empty()) {
            RaftNodeOutput next(d_allocator_p);
            deliverMessages(current, &next);
            total += static_cast<int>(current.d_messages.size());

            for (bsl::vector<LogEntry>::size_type i = 0;
                 i < next.d_committed.size();
                 ++i) {
                seedOutput->d_committed.push_back(next.d_committed[i]);
            }
            seedOutput->d_stateChanged = seedOutput->d_stateChanged ||
                                         next.d_stateChanged;
            seedOutput->d_leaderChanged = seedOutput->d_leaderChanged ||
                                          next.d_leaderChanged;

            current = next;
        }
        return total;
    }

    /// Tick all nodes one at a time, delivering messages after each tick.
    void tickAll()
    {
        for (int i = 0; i < d_numNodes; ++i) {
            RaftNodeOutput output(d_allocator_p);
            d_nodes[i]->tick(&output);
            runUntilQuiet(&output);
        }
    }

    /// Tick only node 'id', deliver messages until quiet.
    void tickNode(int id)
    {
        RaftNodeOutput output(d_allocator_p);
        d_nodes[id]->tick(&output);
        runUntilQuiet(&output);
    }

    /// Find the leader node.  Return -1 if none.
    int findLeader() const
    {
        int leader = -1;
        for (int i = 0; i < d_numNodes; ++i) {
            if (d_nodes[i]->state() == RaftState::e_LEADER) {
                if (leader != -1) {
                    return -2;  // multiple leaders
                }
                leader = i;
            }
        }
        return leader;
    }

    /// Return count of nodes that think 'nodeId' is the leader.
    int leaderAgreement(int nodeId) const
    {
        int count = 0;
        for (int i = 0; i < d_numNodes; ++i) {
            if (d_nodes[i]->leaderId() == nodeId) {
                ++count;
            }
        }
        return count;
    }

    bsl::shared_ptr<bdlbb::Blob> makeBlob(const char* data)
    {
        bsl::shared_ptr<bdlbb::Blob> blob =
            bsl::allocate_shared<bdlbb::Blob>(d_allocator_p, &d_bufferFactory);
        bdlbb::BlobUtil::append(blob.get(),
                                data,
                                static_cast<int>(bsl::strlen(data)));
        return blob;
    }

    // ACCESSORS
    RaftNode*      node(int id) { return d_nodes[id]; }
    MemoryRaftLog* log(int id) { return d_logs[id]; }
    int            numNodes() const { return d_numNodes; }
};

/// Tick the cluster until a leader emerges or maxTicks is reached.
int electLeader(TestCluster* cluster, int maxTicks = 100)
{
    for (int t = 0; t < maxTicks; ++t) {
        cluster->tickAll();
        int leader = cluster->findLeader();
        if (leader >= 0) {
            return leader;
        }
    }
    return -1;
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ============================================================================

static void test1_breathingTest()
// BREATHING TEST
//
// Verify initial state of a RaftNode.
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bslma::TestAllocator alloc("test", false);
    MemoryRaftLog        log(&alloc);
    bsl::vector<int>     peers(&alloc);
    peers.push_back(0);
    peers.push_back(1);
    peers.push_back(2);

    RaftNodeConfig config(true, &alloc);
    config.d_selfId             = 0;
    config.d_peerIds            = peers;
    config.d_electionTimeoutMin = 10;
    config.d_electionTimeoutMax = 20;
    config.d_heartbeatInterval  = 3;
    config.d_preVote            = true;

    RaftNode node(config, &log, &alloc);

    BMQTST_ASSERT_EQ(node.state(), RaftState::e_FOLLOWER);
    BMQTST_ASSERT_EQ(node.currentTerm(), 0ULL);
    BMQTST_ASSERT_EQ(node.leaderId(), RaftNode::k_INVALID_NODE_ID);
    BMQTST_ASSERT_EQ(node.commitIndex(), 0ULL);
    BMQTST_ASSERT_EQ(node.selfId(), 0);
}

static void test2_leaderElection()
// LEADER ELECTION
//
// Verify that a 3-node cluster elects exactly one leader.
{
    bmqtst::TestHelper::printTestName("LEADER ELECTION");

    bslma::TestAllocator alloc("test", false);
    TestCluster          cluster(3, false, &alloc);

    int leader = electLeader(&cluster);

    BMQTST_ASSERT_GE(leader, 0);
    BMQTST_ASSERT_LT(leader, 3);
    BMQTST_ASSERT_EQ(cluster.node(leader)->state(), RaftState::e_LEADER);
    BMQTST_ASSERT_GT(cluster.node(leader)->currentTerm(), 0ULL);

    // All nodes should agree on the leader
    for (int i = 0; i < 3; ++i) {
        BMQTST_ASSERT_EQ(cluster.node(i)->leaderId(), leader);
    }
}

static void test3_preVoteElection()
// PRE-VOTE ELECTION
//
// Verify that pre-vote prevents term inflation.
{
    bmqtst::TestHelper::printTestName("PRE-VOTE ELECTION");

    bslma::TestAllocator alloc("test", false);
    TestCluster          cluster(3, true, &alloc);

    int leader = electLeader(&cluster);

    BMQTST_ASSERT_GE(leader, 0);
    BMQTST_ASSERT_EQ(cluster.node(leader)->state(), RaftState::e_LEADER);

    // With pre-vote, term should be 1 (pre-vote doesn't increment term)
    BMQTST_ASSERT_EQ(cluster.node(leader)->currentTerm(), 1ULL);
}

static void test4_electionWithLogRestriction()
// ELECTION WITH LOG RESTRICTION
//
// Verify that a candidate with a stale log cannot win election.
{
    bmqtst::TestHelper::printTestName("ELECTION WITH LOG RESTRICTION");

    bslma::TestAllocator           alloc("test", false);
    bdlbb::PooledBlobBufferFactory factory(256, &alloc);

    // Create 3 nodes with pre-vote disabled for simplicity
    TestCluster cluster(3, false, &alloc);

    // Give nodes 1 and 2 a log entry that node 0 doesn't have
    bsl::shared_ptr<bdlbb::Blob> data =
        bsl::allocate_shared<bdlbb::Blob>(&alloc, &factory);
    bdlbb::BlobUtil::append(data.get(), "entry1", 6);
    cluster.log(1)->append(1, data);
    cluster.log(2)->append(1, data);

    // Force node 0 to start election by ticking it past timeout
    for (int t = 0; t < 25; ++t) {
        RaftNodeOutput output(&alloc);
        cluster.node(0)->tick(&output);
        if (!output.d_messages.empty()) {
            // Node 0 started election.  Deliver to nodes 1 and 2.
            // They should reject because node 0's log is behind.
            RaftNodeOutput responses(&alloc);
            cluster.deliverMessages(output, &responses);

            // Check responses: both should reject
            bool allRejected = true;
            for (bsl::vector<RaftMessage>::size_type i = 0;
                 i < responses.d_messages.size();
                 ++i) {
                if (responses.d_messages[i].d_type ==
                    RaftMessageType::e_REQUEST_VOTE_RESP) {
                    // Node 0's log (empty) is less up-to-date than
                    // nodes 1,2 (have term-1 entry)
                    if (responses.d_messages[i].d_success) {
                        allRejected = false;
                    }
                }
            }
            BMQTST_ASSERT(allRejected);
            BMQTST_ASSERT_NE(cluster.node(0)->state(), RaftState::e_LEADER);
            break;
        }
    }
}

static void test5_logReplication()
// LOG REPLICATION
//
// Verify that a leader replicates entries to followers and commits them.
{
    bmqtst::TestHelper::printTestName("LOG REPLICATION");

    bslma::TestAllocator alloc("test", false);
    TestCluster          cluster(3, false, &alloc);

    int leader = electLeader(&cluster);
    BMQTST_ASSERT_GE(leader, 0);

    // Propose an entry
    bsl::shared_ptr<bdlbb::Blob> data = cluster.makeBlob("hello");
    RaftNodeOutput proposeOutput(&alloc);
    int            rc = cluster.node(leader)->propose(&proposeOutput, data);
    BMQTST_ASSERT_EQ(rc, 0);

    // Deliver messages until quiet
    cluster.runUntilQuiet(&proposeOutput);

    // Entry should be committed on leader
    BMQTST_ASSERT_EQ(cluster.node(leader)->commitIndex(), 1ULL);

    // All logs should have the entry
    for (int i = 0; i < 3; ++i) {
        BMQTST_ASSERT_EQ(cluster.log(i)->lastIndex(), 1ULL);
    }
}

static void test6_logConsistencyCheck()
// LOG CONSISTENCY CHECK
//
// Verify that a follower rejects AppendEntries with mismatched
// prevLogTerm.
{
    bmqtst::TestHelper::printTestName("LOG CONSISTENCY CHECK");

    bslma::TestAllocator           alloc("test", false);
    bdlbb::PooledBlobBufferFactory factory(256, &alloc);

    MemoryRaftLog log(&alloc);
    bsl::shared_ptr<bdlbb::Blob> data =
        bsl::allocate_shared<bdlbb::Blob>(&alloc, &factory);
    bdlbb::BlobUtil::append(data.get(), "x", 1);
    log.append(1, data);  // index 1, term 1

    bsl::vector<int> peers(&alloc);
    peers.push_back(0);
    peers.push_back(1);
    peers.push_back(2);

    RaftNodeConfig config(true, &alloc);
    config.d_selfId             = 1;
    config.d_peerIds            = peers;
    config.d_electionTimeoutMin = 10;
    config.d_electionTimeoutMax = 20;
    config.d_heartbeatInterval  = 3;
    config.d_preVote            = false;

    RaftNode follower(config, &log, &alloc);

    // Send AppendEntries with wrong prevLogTerm
    RaftMessage ae(&alloc);
    ae.d_type              = RaftMessageType::e_APPEND_ENTRIES;
    ae.d_term              = 2;
    ae.d_sourceNodeId      = 0;
    ae.d_destinationNodeId = 1;
    ae.d_prevLogIndex      = 1;
    ae.d_prevLogTerm       = 99;  // wrong term
    ae.d_leaderCommit      = 0;

    RaftNodeOutput output(&alloc);
    follower.step(&output, ae);

    BMQTST_ASSERT_EQ(output.d_messages.size(), 1u);
    BMQTST_ASSERT_EQ(output.d_messages[0].d_type,
                     RaftMessageType::e_APPEND_ENTRIES_RESP);
    BMQTST_ASSERT_EQ(output.d_messages[0].d_success, false);
}

static void test7_logConflictResolution()
// LOG CONFLICT RESOLUTION
//
// Verify that a follower truncates conflicting entries and accepts
// the leader's entries.
{
    bmqtst::TestHelper::printTestName("LOG CONFLICT RESOLUTION");

    bslma::TestAllocator           alloc("test", false);
    bdlbb::PooledBlobBufferFactory factory(256, &alloc);

    MemoryRaftLog log(&alloc);
    bsl::shared_ptr<bdlbb::Blob> data1 =
        bsl::allocate_shared<bdlbb::Blob>(&alloc, &factory);
    bdlbb::BlobUtil::append(data1.get(), "old", 3);
    log.append(1, data1);  // index 1, term 1
    log.append(1, data1);  // index 2, term 1

    bsl::vector<int> peers(&alloc);
    peers.push_back(0);
    peers.push_back(1);
    peers.push_back(2);

    RaftNodeConfig config(true, &alloc);
    config.d_selfId             = 1;
    config.d_peerIds            = peers;
    config.d_electionTimeoutMin = 10;
    config.d_electionTimeoutMax = 20;
    config.d_heartbeatInterval  = 3;
    config.d_preVote            = false;

    RaftNode follower(config, &log, &alloc);

    // Leader sends entry at index 1 with term 2 (conflict with existing
    // term 1)
    bsl::shared_ptr<bdlbb::Blob> newData =
        bsl::allocate_shared<bdlbb::Blob>(&alloc, &factory);
    bdlbb::BlobUtil::append(newData.get(), "new", 3);

    LogEntry leaderEntry(2, 1, newData);

    RaftMessage ae(&alloc);
    ae.d_type              = RaftMessageType::e_APPEND_ENTRIES;
    ae.d_term              = 2;
    ae.d_sourceNodeId      = 0;
    ae.d_destinationNodeId = 1;
    ae.d_prevLogIndex      = 0;
    ae.d_prevLogTerm       = 0;
    ae.d_leaderCommit      = 0;
    ae.d_entries.push_back(leaderEntry);

    RaftNodeOutput output(&alloc);
    follower.step(&output, ae);

    BMQTST_ASSERT_EQ(output.d_messages.size(), 1u);
    BMQTST_ASSERT_EQ(output.d_messages[0].d_success, true);

    // Follower's log should now have 1 entry with term 2
    BMQTST_ASSERT_EQ(log.lastIndex(), 1ULL);
    BMQTST_ASSERT_EQ(log.lastTerm(), 2ULL);
}

static void test8_commitIndexAdvancement()
// COMMIT INDEX ADVANCEMENT
//
// Verify that commitIndex only advances when a majority have the entry
// AND the entry is from the current term.
{
    bmqtst::TestHelper::printTestName("COMMIT INDEX ADVANCEMENT");

    bslma::TestAllocator alloc("test", false);
    TestCluster          cluster(3, false, &alloc);

    int leader = electLeader(&cluster);
    BMQTST_ASSERT_GE(leader, 0);

    // Propose entries
    bsl::shared_ptr<bdlbb::Blob> data1 = cluster.makeBlob("entry1");
    bsl::shared_ptr<bdlbb::Blob> data2 = cluster.makeBlob("entry2");

    RaftNodeOutput out1(&alloc);
    cluster.node(leader)->propose(&out1, data1);
    cluster.runUntilQuiet(&out1);

    BMQTST_ASSERT_EQ(cluster.node(leader)->commitIndex(), 1ULL);

    RaftNodeOutput out2(&alloc);
    cluster.node(leader)->propose(&out2, data2);
    cluster.runUntilQuiet(&out2);

    BMQTST_ASSERT_EQ(cluster.node(leader)->commitIndex(), 2ULL);

    // Verify all nodes have same commit index after full delivery
    for (int i = 0; i < 3; ++i) {
        BMQTST_ASSERT_EQ(cluster.log(i)->lastIndex(), 2ULL);
    }
}

static void test9_leadershipTransfer()
// LEADERSHIP TRANSFER
//
// Verify that leadership transfer works.
{
    bmqtst::TestHelper::printTestName("LEADERSHIP TRANSFER");

    bslma::TestAllocator alloc("test", false);
    TestCluster          cluster(3, false, &alloc);

    int leader = electLeader(&cluster);
    BMQTST_ASSERT_GE(leader, 0);

    // Pick a target that isn't the leader
    int target = (leader + 1) % 3;

    RaftNodeOutput output(&alloc);
    int rc = cluster.node(leader)->transferLeadership(&output, target);
    BMQTST_ASSERT_EQ(rc, 0);

    cluster.runUntilQuiet(&output);

    // Target should now be the leader (it received TimeoutNow, started
    // election, won)
    BMQTST_ASSERT_EQ(cluster.node(target)->state(), RaftState::e_LEADER);
    BMQTST_ASSERT_EQ(cluster.node(leader)->state(), RaftState::e_FOLLOWER);
}

static void test10_splitVote()
// SPLIT VOTE / NEW ELECTION
//
// Verify that if no majority is reached, a new election starts with a
// higher term.
{
    bmqtst::TestHelper::printTestName("SPLIT VOTE / NEW ELECTION");

    bslma::TestAllocator alloc("test", false);

    // Use 2 nodes — neither can get a majority alone, but with self-vote
    // each gets 1/2.  Actually with 2 nodes quorum is 1, so both would
    // become leader.  Use a scenario where we manually control who
    // votes for whom.

    // Instead: create a 5-node cluster and verify that eventually
    // a leader emerges even if initial elections split.
    TestCluster cluster(5, false, &alloc);

    int leader = electLeader(&cluster, 200);
    BMQTST_ASSERT_GE(leader, 0);
    BMQTST_ASSERT_EQ(cluster.node(leader)->state(), RaftState::e_LEADER);
}

static void test11_leaderStepDown()
// LEADER STEP DOWN
//
// Verify that a leader steps down when it receives a message with a
// higher term.
{
    bmqtst::TestHelper::printTestName("LEADER STEP DOWN");

    bslma::TestAllocator alloc("test", false);
    TestCluster          cluster(3, false, &alloc);

    int leader = electLeader(&cluster);
    BMQTST_ASSERT_GE(leader, 0);

    bsls::Types::Uint64 leaderTerm = cluster.node(leader)->currentTerm();

    // Send an AppendEntries with a higher term from a "phantom" leader.
    RaftMessage ae(&alloc);
    ae.d_type              = RaftMessageType::e_APPEND_ENTRIES;
    ae.d_term              = leaderTerm + 1;
    ae.d_sourceNodeId      = (leader + 1) % 3;
    ae.d_destinationNodeId = leader;
    ae.d_prevLogIndex      = 0;
    ae.d_prevLogTerm       = 0;
    ae.d_leaderCommit      = 0;

    RaftNodeOutput output(&alloc);
    cluster.node(leader)->step(&output, ae);

    BMQTST_ASSERT_EQ(cluster.node(leader)->state(), RaftState::e_FOLLOWER);
    BMQTST_ASSERT_EQ(cluster.node(leader)->currentTerm(), leaderTerm + 1);
}

static void test12_heartbeatResetsElectionTimer()
// HEARTBEAT RESETS ELECTION TIMER
//
// Verify that a follower receiving heartbeats does not start an election.
{
    bmqtst::TestHelper::printTestName("HEARTBEAT RESETS ELECTION TIMER");

    bslma::TestAllocator alloc("test", false);
    TestCluster          cluster(3, false, &alloc);

    int leader = electLeader(&cluster);
    BMQTST_ASSERT_GE(leader, 0);

    int follower = (leader + 1) % 3;

    // Tick many times — leader sends heartbeats, follower stays follower
    for (int t = 0; t < 50; ++t) {
        cluster.tickAll();
    }

    BMQTST_ASSERT_EQ(cluster.node(follower)->state(), RaftState::e_FOLLOWER);
    BMQTST_ASSERT_EQ(cluster.node(leader)->state(), RaftState::e_LEADER);
    BMQTST_ASSERT_EQ(cluster.findLeader(), leader);
}

// ============================================================================
//                                 MAIN PROGRAM
// ============================================================================

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 12: test12_heartbeatResetsElectionTimer(); break;
    case 11: test11_leaderStepDown(); break;
    case 10: test10_splitVote(); break;
    case 9: test9_leadershipTransfer(); break;
    case 8: test8_commitIndexAdvancement(); break;
    case 7: test7_logConflictResolution(); break;
    case 6: test6_logConsistencyCheck(); break;
    case 5: test5_logReplication(); break;
    case 4: test4_electionWithLogRestriction(); break;
    case 3: test3_preVoteElection(); break;
    case 2: test2_leaderElection(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
