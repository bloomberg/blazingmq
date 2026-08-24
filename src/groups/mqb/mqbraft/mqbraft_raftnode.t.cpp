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

    int append(bsls::Types::Uint64                 term,
               const bsl::shared_ptr<bdlbb::Blob>& data) BSLS_KEYWORD_OVERRIDE
    {
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

    void entries(bsls::Types::Uint64    lo,
                 bsls::Types::Uint64    hi,
                 bsl::vector<LogEntry>* out,
                 bsls::Types::Uint64    maxCount,
                 bsls::Types::Uint64    maxBytes,
                 bool                   forApply) const BSLS_KEYWORD_OVERRIDE
    {
        (void)forApply;
        BSLS_ASSERT_SAFE(out);
        BSLS_ASSERT_SAFE(lo <= hi);
        BSLS_ASSERT_SAFE(lo > d_snapshotIndex);
        BSLS_ASSERT_SAFE(hi <= lastIndex() + 1);
        const bsl::vector<LogEntry>::size_type loaded = out->size();
        bsls::Types::Uint64                    bytes  = 0;
        for (bsls::Types::Uint64 i = lo; i < hi; ++i) {
            bsls::Types::Uint64 offset = i - d_snapshotIndex - 1;
            out->push_back(d_entries[static_cast<int>(offset)]);
            bytes += out->back().d_data ? out->back().d_data->length() : 0;
            if ((maxCount != 0 && out->size() - loaded >= maxCount) ||
                (maxBytes != 0 && bytes >= maxBytes)) {
                break;
            }
        }
    }

    bsls::Types::Uint64 snapshotIndex() const BSLS_KEYWORD_OVERRIDE
    {
        return d_snapshotIndex;
    }

    bsls::Types::Uint64 snapshotTerm() const BSLS_KEYWORD_OVERRIDE
    {
        return d_snapshotTerm;
    }

    /// Compact at the specified `index` as a rollover does: drop the entries
    /// at or below it and move the snapshot boundary to `index` / `term`.
    void setSnapshot(bsls::Types::Uint64 index, bsls::Types::Uint64 term)
    {
        BSLS_ASSERT_SAFE(index >= d_snapshotIndex && index <= lastIndex());

        d_entries.erase(d_entries.begin(),
                        d_entries.begin() +
                            static_cast<int>(index - d_snapshotIndex));
        d_snapshotIndex = index;
        d_snapshotTerm  = term;
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
    explicit TestCluster(int                 numNodes,
                         bool                preVote           = true,
                         bslma::Allocator*   allocator         = 0,
                         bool                broadcastOnCommit = false,
                         bsls::Types::Uint64 maxUnacked        = 0)
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

            RaftNodeConfig config(RaftNodeConfig::k_CSL_PARTITION_ID,
                                  broadcastOnCommit,
                                  d_allocator_p);
            config.d_selfId             = i;
            config.d_peerIds            = peerIds;
            config.d_electionTimeoutMin = 10;
            config.d_electionTimeoutMax = 20;
            config.d_heartbeatInterval  = 3;
            config.d_preVote            = preVote;
            if (maxUnacked != 0) {
                config.d_maxUnackedEntries = maxUnacked;
            }

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
    /// collecting responses into 'responses'.  A message may carry several
    /// destinations: a round sends one message to every peer at the same log
    /// position.  Each node is flushed after it steps, the way
    /// 'ClusterStateRaft' runs a round on every event, since 'step' only moves
    /// peer state and never sends.
    void deliverMessages(const RaftNodeOutput& output,
                         RaftNodeOutput*       responses)
    {
        for (bsl::vector<RaftMessage>::size_type i = 0;
             i < output.d_messages.size();
             ++i) {
            const RaftMessage& msg = output.d_messages[i];
            for (size_t j = 0; j < msg.destinationCount(); ++j) {
                const int dest = msg.destination(j);
                if (dest >= 0 && dest < d_numNodes) {
                    d_nodes[dest]->step(responses, msg);
                    d_nodes[dest]->flushSends(responses);
                }
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
            d_nodes[i]->flushSends(&output);
            runUntilQuiet(&output);
        }
    }

    /// Tick every node except the specified 'skip', delivering messages after
    /// each tick.  Models 'skip' being down (it emits no heartbeats).
    void tickAllExcept(int skip)
    {
        for (int i = 0; i < d_numNodes; ++i) {
            if (i == skip) {
                continue;  // CONTINUE
            }
            RaftNodeOutput output(d_allocator_p);
            d_nodes[i]->tick(&output);
            d_nodes[i]->flushSends(&output);
            runUntilQuiet(&output);
        }
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
};

/// Tick the cluster until a leader emerges or maxTicks is reached.
/// Return `true` if the specified `msg` is addressed to the specified `node`.
/// A round sends one message to every peer at the same log position, so a
/// destination is not always `d_destinationNodeId`.
bool goesTo(const RaftMessage& msg, int node)
{
    for (size_t i = 0; i < msg.destinationCount(); ++i) {
        if (msg.destination(i) == node) {
            return true;
        }
    }
    return false;
}

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
        cluster.node(0)->flushSends(&output);
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
    RaftNodeOutput               proposeOutput(&alloc);
    int rc = cluster.node(leader)->propose(&proposeOutput, data);
    BMQTST_ASSERT_EQ(rc, 0);
    cluster.node(leader)->flushSends(&proposeOutput);

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

    MemoryRaftLog                log(&alloc);
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

    MemoryRaftLog                log(&alloc);
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
    cluster.node(leader)->flushSends(&out1);
    cluster.runUntilQuiet(&out1);

    BMQTST_ASSERT_EQ(cluster.node(leader)->commitIndex(), 1ULL);

    RaftNodeOutput out2(&alloc);
    cluster.node(leader)->propose(&out2, data2);
    cluster.node(leader)->flushSends(&out2);
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

static void test13_electionMode()
// ELECTION MODE (FORCE / NEVER)
//
// Verify that setElectionMode(e_NEVER) leaves an incumbent leader in place but
// keeps that node from winning any later election, and that
// setElectionMode(e_FORCE) makes a specific node win leadership once the
// incumbent is gone.  This backs the legacy 'set_quorum' primary-pinning knob
// (see 'ClusterOrchestrator::processCommand').
{
    bmqtst::TestHelper::printTestName("ELECTION MODE");

    bslma::TestAllocator alloc("test", false);
    TestCluster          cluster(3, false, &alloc);

    const int leader = electLeader(&cluster);
    BMQTST_ASSERT_GE(leader, 0);

    const int target = (leader + 1) % 3;
    const int other  = (leader + 2) % 3;

    // Exclude the third node and the current leader, then force 'target'.
    // Deliver each mode change's resulting messages before the next.
    {
        RaftNodeOutput output(&alloc);
        cluster.node(other)->setElectionMode(&output, ElectionMode::e_NEVER);
        cluster.node(other)->flushSends(&output);
        cluster.runUntilQuiet(&output);
    }
    {
        RaftNodeOutput output(&alloc);
        cluster.node(leader)->setElectionMode(&output, ElectionMode::e_NEVER);
        cluster.node(leader)->flushSends(&output);
        cluster.runUntilQuiet(&output);
    }
    // Excluding the incumbent does not depose it.
    BMQTST_ASSERT_EQ(cluster.node(leader)->state(), RaftState::e_LEADER);
    for (int t = 0; t < 50; ++t) {
        cluster.tickAll();
    }
    BMQTST_ASSERT_EQ(cluster.findLeader(), leader);

    {
        RaftNodeOutput output(&alloc);
        cluster.node(target)->setElectionMode(&output, ElectionMode::e_FORCE);
        cluster.node(target)->flushSends(&output);
        cluster.runUntilQuiet(&output);
    }

    // Once the incumbent is gone, the forced node is the only eligible
    // candidate and takes leadership on its election timeout.
    for (int t = 0;
         t < 100 && cluster.node(target)->state() != RaftState::e_LEADER;
         ++t) {
        cluster.tickAllExcept(leader);
    }
    BMQTST_ASSERT_EQ(cluster.node(target)->state(), RaftState::e_LEADER);

    // Even after many ticks, an excluded node never becomes leader and the
    // forced node keeps leadership.
    for (int t = 0; t < 100; ++t) {
        cluster.tickAll();
    }
    BMQTST_ASSERT_EQ(cluster.node(other)->state(), RaftState::e_FOLLOWER);
    BMQTST_ASSERT_EQ(cluster.node(leader)->state(), RaftState::e_FOLLOWER);
    BMQTST_ASSERT_EQ(cluster.findLeader(), target);
}

static void test14_appendEntriesBatching()
// APPEND ENTRIES BATCHING AND SHARING
//
// Nothing is sent until the caller runs a round, so a burst of proposals
// leaves the log with one round's worth of entries owed and sends them as one
// message.  Peers at the same log position are owed identical bytes, so that
// one message carries both of them as destinations rather than being built
// and read from the log twice.
{
    bmqtst::TestHelper::printTestName("APPEND ENTRIES BATCHING");

    bslma::TestAllocator alloc("test", false);
    // 'broadcastOnCommit' as production configures it for both the CSL and
    // per-partition Raft groups.
    TestCluster cluster(3, false, &alloc, true);

    const int leader = electLeader(&cluster);
    BMQTST_ASSERT_GE(leader, 0);

    const int peer  = (leader + 1) % 3;
    const int other = (leader + 2) % 3;

    // One proposal, one round: a single message addressed to both peers.
    RaftNodeOutput first(&alloc);
    BMQTST_ASSERT_EQ(cluster.node(leader)->propose(&first,
                                                   cluster.makeBlob("m1")),
                     0);
    BMQTST_ASSERT_EQ(first.d_messages.size(), 0u);  // deferred

    cluster.node(leader)->flushSends(&first);
    BMQTST_ASSERT_EQ(first.d_messages.size(), 1u);
    BMQTST_ASSERT_EQ(first.d_messages[0].d_type,
                     RaftMessageType::e_APPEND_ENTRIES);
    BMQTST_ASSERT_EQ(first.d_messages[0].d_entries.size(), 1u);
    BMQTST_ASSERT_EQ(first.d_messages[0].destinationCount(), 2u);
    BMQTST_ASSERT(goesTo(first.d_messages[0], peer));
    BMQTST_ASSERT(goesTo(first.d_messages[0], other));

    const RaftMessage firstToPeer(first.d_messages[0], &alloc);

    // Ten more proposals with no round in between send nothing; the round that
    // follows carries all ten in one message, still shared by both peers.
    for (int i = 0; i < 10; ++i) {
        RaftNodeOutput burst(&alloc);
        BMQTST_ASSERT_EQ(cluster.node(leader)->propose(&burst,
                                                       cluster.makeBlob("m")),
                         0);
        BMQTST_ASSERT_EQ(burst.d_messages.size(), 0u);
    }
    BMQTST_ASSERT_EQ(cluster.log(leader)->lastIndex(), 11ULL);

    RaftNodeOutput second(&alloc);
    cluster.node(leader)->flushSends(&second);
    BMQTST_ASSERT_EQ(second.d_messages.size(), 1u);
    BMQTST_ASSERT_EQ(second.d_messages[0].d_entries.size(), 10u);
    BMQTST_ASSERT_EQ(second.d_messages[0].destinationCount(), 2u);

    // The peer answers the first message only; the leader must not treat that
    // as an ack of the ten sent optimistically after it.
    RaftNodeOutput peerResp(&alloc);
    cluster.node(peer)->step(&peerResp, firstToPeer);
    BMQTST_ASSERT_EQ(peerResp.d_messages.size(), 1u);
    BMQTST_ASSERT_EQ(peerResp.d_messages[0].d_type,
                     RaftMessageType::e_APPEND_ENTRIES_RESP);
    BMQTST_ASSERT_EQ(peerResp.d_messages[0].d_success, true);
    BMQTST_ASSERT_EQ(peerResp.d_messages[0].d_matchIndex, 1ULL);

    RaftNodeOutput afterResp(&alloc);
    cluster.node(leader)->step(&afterResp, peerResp.d_messages[0]);
    cluster.node(leader)->flushSends(&afterResp);

    // Everything through index 11 already went out, so the peer is owed no
    // further entries -- only the commit index this response moved.
    for (size_t i = 0; i < afterResp.d_messages.size(); ++i) {
        if (afterResp.d_messages[i].d_type ==
            RaftMessageType::e_APPEND_ENTRIES) {
            BMQTST_ASSERT_EQ(afterResp.d_messages[i].d_entries.size(), 0u);
        }
    }
}

static void test15_unackedEntriesBound()
// UNACKED ENTRIES BOUND
//
// Sends are optimistic and the channel buffer neither blocks nor drops, so a
// peer that stops answering has to stop being sent to.  The bound is on
// entries outstanding past the peer's own match index, and it lifts as soon as
// the peer acks.
{
    bmqtst::TestHelper::printTestName("UNACKED ENTRIES BOUND");

    bslma::TestAllocator alloc("test", false);
    TestCluster          cluster(3, false, &alloc, true, 3 /* maxUnacked */);

    const int leader = electLeader(&cluster);
    BMQTST_ASSERT_GE(leader, 0);

    const int peer = (leader + 1) % 3;

    // Neither peer answers, so each round pushes 'nextIndex' further past a
    // match index stuck at 0.  Past the bound the rounds send nothing.
    size_t rounds = 0;
    for (int i = 0; i < 8; ++i) {
        RaftNodeOutput out(&alloc);
        cluster.node(leader)->propose(&out, cluster.makeBlob("m"));
        cluster.node(leader)->flushSends(&out);
        if (!out.d_messages.empty()) {
            ++rounds;
        }
    }
    BMQTST_ASSERT_EQ(cluster.log(leader)->lastIndex(), 8ULL);

    // Four rounds get through -- indices 1 through 4, the fourth of which
    // leaves 4 outstanding, one past the bound of 3.
    BMQTST_ASSERT_EQ(rounds, 4u);

    // The peer acks everything it was sent, and the rest goes out.
    RaftMessage ack(&alloc);
    ack.d_type              = RaftMessageType::e_APPEND_ENTRIES_RESP;
    ack.d_term              = cluster.node(leader)->currentTerm();
    ack.d_sourceNodeId      = peer;
    ack.d_destinationNodeId = leader;
    ack.d_success           = true;
    ack.d_matchIndex        = 4;

    RaftNodeOutput resumed(&alloc);
    cluster.node(leader)->step(&resumed, ack);
    cluster.node(leader)->flushSends(&resumed);

    size_t entriesToPeer = 0;
    for (size_t i = 0; i < resumed.d_messages.size(); ++i) {
        if (goesTo(resumed.d_messages[i], peer)) {
            entriesToPeer += resumed.d_messages[i].d_entries.size();
        }
    }
    BMQTST_ASSERT_EQ(entriesToPeer, 4u);  // indices 5 through 8
}

static void test16_rejectionUsesPeerLastIndex()
// REJECTION USES THE PEER'S LAST INDEX
//
// A rejection carries the peer's own last index, so the leader resumes from
// there in one step rather than walking back one index per round trip.  That
// also makes the several rejections one divergence draws idempotent.
{
    bmqtst::TestHelper::printTestName("STALE REJECTION");

    bslma::TestAllocator alloc("test", false);
    TestCluster          cluster(3, false, &alloc, true);

    const int leader = electLeader(&cluster);
    BMQTST_ASSERT_GE(leader, 0);

    const int peer = (leader + 1) % 3;

    // Four rounds, none answered, so four messages of one entry each went to
    // the peer and 'nextIndex' ran to 5.
    bsl::vector<RaftMessage> toPeer(&alloc);
    for (int i = 0; i < 4; ++i) {
        RaftNodeOutput out(&alloc);
        cluster.node(leader)->propose(&out, cluster.makeBlob("m"));
        cluster.node(leader)->flushSends(&out);
        for (size_t j = 0; j < out.d_messages.size(); ++j) {
            if (goesTo(out.d_messages[j], peer)) {
                toPeer.push_back(out.d_messages[j]);
            }
        }
    }
    BMQTST_ASSERT_EQ(toPeer.size(), 4u);

    // Hand-build the rejections rather than driving the peer, so several are
    // outstanding against one divergence.  'matchIndex' is what a follower
    // whose files were wiped reports: its log is empty.
    RaftMessage rej(&alloc);
    rej.d_type              = RaftMessageType::e_APPEND_ENTRIES_RESP;
    rej.d_term              = cluster.node(leader)->currentTerm();
    rej.d_sourceNodeId      = peer;
    rej.d_destinationNodeId = leader;
    rej.d_success           = false;
    rej.d_matchIndex        = 0;
    rej.d_rejectedIndex     = toPeer[1].d_prevLogIndex;

    // The leader resumes from the peer's own last index, so one rejection is
    // enough to rewind all the way -- not one index per round trip.
    RaftNodeOutput first(&alloc);
    cluster.node(leader)->step(&first, rej);
    cluster.node(leader)->flushSends(&first);
    BMQTST_ASSERT_EQ(first.d_messages.size(), 1u);
    BMQTST_ASSERT_EQ(first.d_messages[0].d_type,
                     RaftMessageType::e_APPEND_ENTRIES);
    BMQTST_ASSERT_EQ(first.d_messages[0].d_prevLogIndex, 0ULL);
    BMQTST_ASSERT_EQ(first.d_messages[0].d_entries.size(), 4u);

    // A diverged peer is sent one message at a time: the second rejection has
    // not arrived, so the round that follows sends it nothing more.
    RaftNodeOutput quiet(&alloc);
    cluster.node(leader)->flushSends(&quiet);
    BMQTST_ASSERT_EQ(quiet.d_messages.size(), 0u);

    // The other rejections of the same divergence are acted on too, and land
    // in the same place: the backoff reads 'matchIndex', not a step count.
    rej.d_rejectedIndex = toPeer[2].d_prevLogIndex;
    RaftNodeOutput second(&alloc);
    cluster.node(leader)->step(&second, rej);
    cluster.node(leader)->flushSends(&second);
    BMQTST_ASSERT_EQ(second.d_messages.size(), 1u);
    BMQTST_ASSERT_EQ(second.d_messages[0].d_prevLogIndex, 0ULL);

    rej.d_rejectedIndex = toPeer[3].d_prevLogIndex;
    RaftNodeOutput third(&alloc);
    cluster.node(leader)->step(&third, rej);
    cluster.node(leader)->flushSends(&third);
    BMQTST_ASSERT_EQ(third.d_messages.size(), 1u);
    BMQTST_ASSERT_EQ(third.d_messages[0].d_prevLogIndex, 0ULL);
}

static void test17_rolloverCommitPrecedesEntries()
// ROLLOVER COMMIT PRECEDES ENTRIES
//
// A peer that has not been told the compaction at 'snapshotIndex' committed
// must apply it before receiving anything past it: a replica appends entries
// before it applies commits, so entries carried in that same message would
// land in the file set the rollover is about to replace.  The round sends the
// commit by itself, and the entries follow in the next one.
{
    bmqtst::TestHelper::printTestName("ROLLOVER COMMIT PRECEDES ENTRIES");

    bslma::TestAllocator alloc("test", false);
    TestCluster          cluster(3, false, &alloc, true);

    const int leader = electLeader(&cluster);
    BMQTST_ASSERT_GE(leader, 0);

    const int peer  = (leader + 1) % 3;
    const int other = (leader + 2) % 3;

    // Two entries, delivered and acked by hand so the leader commits them
    // without a round running afterwards: that leaves the peers' 'sentCommit'
    // behind the commit index, which is the state a rollover has to survive.
    RaftNodeOutput sent(&alloc);
    cluster.node(leader)->propose(&sent, cluster.makeBlob("m1"));
    cluster.node(leader)->propose(&sent, cluster.makeBlob("m2"));
    cluster.node(leader)->flushSends(&sent);

    RaftNodeOutput acks(&alloc);
    for (size_t i = 0; i < sent.d_messages.size(); ++i) {
        const RaftMessage& m = sent.d_messages[i];
        for (size_t j = 0; j < m.destinationCount(); ++j) {
            cluster.node(m.destination(j))->step(&acks, m);
        }
    }
    for (size_t i = 0; i < acks.d_messages.size(); ++i) {
        RaftNodeOutput ignored(&alloc);
        cluster.node(leader)->step(&ignored, acks.d_messages[i]);
    }
    BMQTST_ASSERT_EQ(cluster.node(leader)->commitIndex(), 2ULL);

    // The leader rolls over at the commit point.
    const bsls::Types::Uint64 term = cluster.node(leader)->currentTerm();
    cluster.log(leader)->setSnapshot(2, term);

    // A third entry is owed, but the peers have not heard that 2 committed,
    // so this round carries the commit alone -- shared, since both are held.
    RaftNodeOutput held(&alloc);
    cluster.node(leader)->propose(&held, cluster.makeBlob("m3"));
    cluster.node(leader)->flushSends(&held);

    BMQTST_ASSERT_EQ(held.d_messages.size(), 1u);
    BMQTST_ASSERT_EQ(held.d_messages[0].d_type,
                     RaftMessageType::e_APPEND_ENTRIES);
    BMQTST_ASSERT_EQ(held.d_messages[0].d_entries.size(), 0u);
    BMQTST_ASSERT_EQ(held.d_messages[0].d_leaderCommit, 2ULL);
    BMQTST_ASSERT_EQ(held.d_messages[0].destinationCount(), 2u);
    BMQTST_ASSERT(goesTo(held.d_messages[0], peer));
    BMQTST_ASSERT(goesTo(held.d_messages[0], other));

    // The hold is released by that message, so the entry goes out next round.
    RaftNodeOutput released(&alloc);
    cluster.node(leader)->flushSends(&released);

    BMQTST_ASSERT_EQ(released.d_messages.size(), 1u);
    BMQTST_ASSERT_EQ(released.d_messages[0].d_entries.size(), 1u);
    BMQTST_ASSERT_EQ(released.d_messages[0].d_entries[0].d_index, 3ULL);
    BMQTST_ASSERT_EQ(released.d_messages[0].destinationCount(), 2u);

    // And it does not repeat once acked.
    RaftNodeOutput quiet(&alloc);
    cluster.node(leader)->flushSends(&quiet);
    BMQTST_ASSERT_EQ(quiet.d_messages.size(), 0u);
}

static void test18_singleNodeCommitsOnPropose()
// SINGLE NODE COMMITS ON PROPOSE
//
// A node that is its own quorum commits and applies each proposal as it is
// made, without a round of messages.
{
    bmqtst::TestHelper::printTestName("SINGLE NODE COMMITS ON PROPOSE");

    bslma::TestAllocator alloc("test", false);
    TestCluster          cluster(1, false, &alloc);

    int leader = electLeader(&cluster);
    BMQTST_ASSERT_EQ(leader, 0);

    RaftNodeOutput out(&alloc);
    cluster.node(0)->propose(&out, cluster.makeBlob("entry1"));

    BMQTST_ASSERT_EQ(cluster.node(0)->commitIndex(), 1ULL);
    BMQTST_ASSERT_EQ(out.d_committed.size(), 1u);
    BMQTST_ASSERT_EQ(out.d_committed[0].d_index, 1ULL);

    // The second proposal appends to the same 'output', so the first entry is
    // still there and the new one follows it.
    cluster.node(0)->propose(&out, cluster.makeBlob("entry2"));

    BMQTST_ASSERT_EQ(cluster.node(0)->commitIndex(), 2ULL);
    BMQTST_ASSERT_EQ(out.d_committed.size(), 2u);
    BMQTST_ASSERT_EQ(out.d_committed[1].d_index, 2ULL);

    // Nothing to replicate: a lone node owes no peer anything.
    RaftNodeOutput sends(&alloc);
    cluster.node(0)->flushSends(&sends);
    BMQTST_ASSERT_EQ(sends.d_messages.size(), 0u);
}

static void test19_installSnapshotAlreadyHeld()
// INSTALL SNAPSHOT ALREADY HELD
//
// A follower offered a snapshot whose last included entry it already holds
// acknowledges it instead of reinstalling.  A leader whose retry timer is far
// shorter than the install otherwise re-sends the whole file set every
// timeout, and each retry wipes and re-indexes the follower's partition.
{
    bmqtst::TestHelper::printTestName("INSTALL SNAPSHOT ALREADY HELD");

    bslma::TestAllocator alloc("test", false);
    TestCluster          cluster(3, false, &alloc);

    // Follower 1 holds entries 1..5, compacted through 3 at term 1 -- the
    // state a node is in once an install has completed.
    for (int i = 0; i < 5; ++i) {
        cluster.log(1)->append(1, cluster.makeBlob("e"));
    }
    cluster.log(1)->setSnapshot(3, 1);
    BMQTST_ASSERT_EQ(cluster.log(1)->lastIndex(), 5ULL);

    RaftMessage snap(&alloc);
    snap.d_type              = RaftMessageType::e_INSTALL_SNAPSHOT;
    snap.d_term              = 1;
    snap.d_sourceNodeId      = cluster.node(0)->selfId();
    snap.d_destinationNodeId = cluster.node(1)->selfId();
    snap.d_lastLogIndex      = 3;
    snap.d_lastLogTerm       = 1;

    RaftNodeOutput out(&alloc);
    cluster.node(1)->step(&out, snap);

    // Acknowledged, not installed.
    BMQTST_ASSERT(!out.d_hasInstallSnapshot);
    BMQTST_ASSERT_EQ(out.d_messages.size(), 1u);
    BMQTST_ASSERT_EQ(out.d_messages[0].d_type,
                     RaftMessageType::e_INSTALL_SNAPSHOT_RESP);
    BMQTST_ASSERT_EQ(out.d_messages[0].d_destinationNodeId,
                     snap.d_sourceNodeId);

    // The log is untouched.
    BMQTST_ASSERT_EQ(cluster.log(1)->lastIndex(), 5ULL);
    BMQTST_ASSERT_EQ(cluster.log(1)->snapshotIndex(), 3ULL);

    // Same index at a term this node does not have there: it must install,
    // since the entry it holds is not the one the snapshot covers.
    RaftMessage other(snap, &alloc);
    other.d_term        = 2;
    other.d_lastLogTerm = 2;

    RaftNodeOutput out2(&alloc);
    cluster.node(1)->step(&out2, other);
    BMQTST_ASSERT(out2.d_hasInstallSnapshot);

    // An index past what this node holds must install too.  Carries the term
    // the step above advanced this node to; a lower one is stale and dropped.
    RaftMessage ahead(snap, &alloc);
    ahead.d_term         = 2;
    ahead.d_lastLogIndex = 9;

    RaftNodeOutput out3(&alloc);
    cluster.node(1)->step(&out3, ahead);
    BMQTST_ASSERT(out3.d_hasInstallSnapshot);
}

// ============================================================================
//                                 MAIN PROGRAM
// ============================================================================

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 19: test19_installSnapshotAlreadyHeld(); break;
    case 18: test18_singleNodeCommitsOnPropose(); break;
    case 17: test17_rolloverCommitPrecedesEntries(); break;
    case 16: test16_rejectionUsesPeerLastIndex(); break;
    case 15: test15_unackedEntriesBound(); break;
    case 14: test14_appendEntriesBatching(); break;
    case 13: test13_electionMode(); break;
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
