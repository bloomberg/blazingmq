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

// mqbraft_partitionraft.cpp -*-C++-*-
#include <mqbraft_partitionraft.h>

// MQB
#include <mqbconfm_messages.h>
#include <mqbi_domain.h>
#include <mqbs_filebackedstorage.h>
#include <mqbs_filestore.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_filestoreprotocolutil.h>
#include <mqbs_filestoreutil.h>
#include <mqbs_inmemorystorage.h>
#include <mqbu_exit.h>

// BMQ
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqtsk_alarmlog.h>
#include <bmqu_blob.h>
#include <bmqu_blobobjectproxy.h>
#include <bmqu_memoutstream.h>
#include <bmqu_time.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blobutil.h>
#include <bdlf_bind.h>
#include <bdls_pathutil.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslmf_movableref.h>
#include <bsls_assert.h>
#include <bsls_systemtime.h>

// SYSTEM
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace BloombergLP {
namespace mqbraft {

namespace {

const int k_TICK_INTERVAL_MS = 100;

/// How long a StopResponse waits for the stopping peer to acknowledge its
/// sync point.  Well below the stopping peer's StopRequest timeout.
const int k_STOP_RESPONSE_WAIT_MS = 1000;
const int k_JREC_SIZE        = mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
const bsls::Types::Uint64 k_CHUNK_SIZE = 4ULL * 1024 * 1024;

RaftNodeConfig makeRaftConfig(mqbc::ClusterData& clusterData,
                              int                partitionId,
                              bslma::Allocator*  allocator)
{
    RaftNodeConfig config(partitionId,
                          true,  // broadcastHeartbeatOnCommit
                          allocator);

    const mqbnet::ClusterNode* selfNode = clusterData.membership().selfNode();

    config.setSelf(selfNode->nodeId(), selfNode->hostName());

    // 'd_peers' is the full membership, self included: 'RaftNode::quorum()'
    // is 'peers.size() / 2 + 1', and 'RaftNode' skips self where it iterates
    // peers.  'netCluster->nodes()' includes self.
    const mqbnet::Cluster::NodesList& nodes =
        clusterData.membership().netCluster()->nodes();
    for (mqbnet::Cluster::NodesList::const_iterator it = nodes.begin();
         it != nodes.end();
         ++it) {
        config.addNode((*it)->nodeId(), (*it)->hostName());
    }

    config.d_electionTimeoutMin = 10;
    config.d_electionTimeoutMax = 20;
    config.d_heartbeatInterval  = 3;

    return config;
}

/// Return the size of the entry at the specified `pos` in the specified
/// `blob`, or -1 if the entry cannot be read.  Every lookup is relative to
/// `pos`.  This is to avoid rescanning `blob` from its start for each entry.
int computeEntrySize(const bdlbb::Blob& blob, const bmqu::BlobPosition& pos)
{
    bmqu::BlobObjectProxy<mqbs::RecordHeader> rh(&blob, pos, true, false);
    if (!rh.isSet()) {
        return -1;
    }

    switch (rh->type()) {
    case mqbs::RecordType::e_MESSAGE: {
        bmqu::BlobPosition dhPos;
        if (0 !=
            bmqu::BlobUtil::findOffsetSafe(&dhPos, blob, pos, k_JREC_SIZE)) {
            return -1;
        }
        bmqu::BlobObjectProxy<mqbs::DataHeader> dh(
            &blob,
            dhPos,
            -mqbs::DataHeader::k_MIN_HEADER_SIZE,
            true,
            false);
        if (!dh.isSet()) {
            return -1;
        }
        return k_JREC_SIZE + dh->messageWords() * bmqp::Protocol::k_WORD_SIZE;
    }
    case mqbs::RecordType::e_QUEUE_OP: {
        // In 'blob', a QUEUE_OP entry starts with a journal record at 'pos'.
        // A qlist record comes right after that journal record only if the
        // journal record's 'queueUriRecordOffsetWords()' is non-zero
        // ('FileStore::readRecord' builds the entry this way).
        // 1. If the offset is zero, the entry size is the journal record size.
        // 2. Otherwise, add the qlist record size, read from the
        //    'QueueRecordHeader' right after the journal record.
        bmqu::BlobObjectProxy<mqbs::QueueOpRecord> qop(&blob,
                                                       pos,
                                                       true,
                                                       false);
        if (!qop.isSet() || 0 == qop->queueUriRecordOffsetWords()) {
            return k_JREC_SIZE;
        }

        bmqu::BlobPosition qrhPos;
        if (0 !=
            bmqu::BlobUtil::findOffsetSafe(&qrhPos, blob, pos, k_JREC_SIZE)) {
            return k_JREC_SIZE;
        }
        bmqu::BlobObjectProxy<mqbs::QueueRecordHeader> qrh(&blob,
                                                           qrhPos,
                                                           true,
                                                           false);
        if (!qrh.isSet()) {
            return k_JREC_SIZE;
        }
        int qlistLen = qrh->queueRecordWords() * bmqp::Protocol::k_WORD_SIZE;
        return k_JREC_SIZE + qlistLen;
    }
    case mqbs::RecordType::e_CONFIRM:
    case mqbs::RecordType::e_DELETION:
    case mqbs::RecordType::e_JOURNAL_OP: return k_JREC_SIZE;
    case mqbs::RecordType::e_UNDEFINED:
    default: return -1;
    }
}

}  // close unnamed namespace

// ====================
// class PartitionRaft
// ====================

// CREATORS
PartitionRaft::PartitionRaft(unsigned int partitionId,
                             const bsl::shared_ptr<mqbs::FileStore>& fileStore,
                             mqbc::ClusterData*           clusterData,
                             mqbs::StorageMonitor*        storageMonitor,
                             const PartitionLeadershipCb& leadershipCb,
                             bslma::Allocator*            allocator)
: d_partitionId(partitionId)
, d_fileStore_sp(fileStore)
, d_clusterData_p(clusterData)
, d_storageMonitor_p(storageMonitor)
, d_raftLog_mp()
, d_raftNode_mp()
, d_tickHandle()
, d_proposalPool(1024, bslma::Default::allocator(allocator))
, d_isStarted(false)
, d_allocator_p(bslma::Default::allocator(allocator))
, d_receivingSnapshot(false)
, d_snapshotJournalFd(-1)
, d_snapshotDataFd(-1)
, d_snapshotQlistFd(-1)
, d_snapshotFiles(d_allocator_p)
, d_snapshotLastIncludedIndex(0)
, d_snapshotLastIncludedTerm(0)
, d_isDispatchingOutput(false)
, d_deferred(d_allocator_p)
, d_isRolloverPending(false)
, d_reportedSyncPointTerm(0)
, d_needsBecomeLeaderSyncPoint(false)
, d_leadershipCb(leadershipCb)
, d_canShutdown(false)
, d_stopResponseWaits(d_allocator_p)
, d_persistentStatePath(d_allocator_p)
, d_isPersistentStateDirty(false)
{
    BSLS_ASSERT_SAFE(d_fileStore_sp);
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(storageMonitor);
    BSLS_ASSERT_SAFE(d_leadershipCb);

    d_raftLog_mp.load(new (*d_allocator_p)
                          PartitionRaftLog(d_fileStore_sp.get(),
                                           d_allocator_p),
                      d_allocator_p);

    d_raftNode_mp.load(
        new (*d_allocator_p) RaftNode(
            makeRaftConfig(*d_clusterData_p, d_partitionId, d_allocator_p),
            d_raftLog_mp.get(),
            d_allocator_p),
        d_allocator_p);

    BSLS_ASSERT_SAFE(d_raftLog_mp);
    BSLS_ASSERT_SAFE(d_raftNode_mp);
}

PartitionRaft::~PartitionRaft()
{
    BSLS_ASSERT_SAFE(!d_isStarted);
}

// PRIVATE MANIPULATORS
mqbnet::ClusterNode* PartitionRaft::peerNode(int nodeId)
{
    bsl::unordered_map<int, mqbnet::ClusterNode*>::const_iterator it =
        d_peerNodes.find(nodeId);
    if (it != d_peerNodes.end()) {
        return it->second;  // RETURN
    }

    mqbnet::ClusterNode* node =
        d_clusterData_p->membership().netCluster()->lookupNode(nodeId);
    if (node) {
        d_peerNodes[nodeId] = node;
    }
    return node;
}

void PartitionRaft::setPeerAvailabilityDispatched(int nodeId, bool isAvailable)
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(d_fileStore_sp->inDispatcherThread());

    d_raftNode_mp->setPeerAvailability(nodeId, isAvailable);
}

void PartitionRaft::onNodeStateChange(mqbnet::ClusterNode* node,
                                      bool                 isAvailable)
{
    // executed by the *IO* thread, holding 'ClusterImp::d_mutex': do no work
    // here beyond handing the change to this partition's dispatcher.
    BSLS_ASSERT_SAFE(node);

    execute(bdlf::BindUtil::bind(&PartitionRaft::setPeerAvailabilityDispatched,
                                 this,
                                 node->nodeId(),
                                 isAvailable));
}

void PartitionRaft::flush()
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(d_fileStore_sp->inDispatcherThread());

    if (!d_isStarted) {
        return;  // RETURN
    }

    RaftNodeOutput output(d_allocator_p);
    d_raftNode_mp->flushSends(&output);
    dispatchMessages(&output);

    // Runs after every event on this thread, including each tick and each
    // AppendEntries response.
    checkIfCanSendStopResponse();
}

bool PartitionRaft::savePersistentState(bool changed)
{
    // executed by the partition *DISPATCHER* thread

    if (changed) {
        d_isPersistentStateDirty = true;
    }
    if (!d_isPersistentStateDirty) {
        return true;  // RETURN
    }

    const PersistentState state = d_raftNode_mp->persistentState();
    bmqu::MemOutStream    errorDescription(d_allocator_p);
    const int             rc = PersistentStateUtil::save(errorDescription,
                                             d_persistentStatePath,
                                             state);
    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("FILE_IO")
            << "Partition [" << d_partitionId << "] failed to save " << state
            << ", rc: " << rc << ", " << errorDescription.str()
            << ".  Raft messages are not sent until it is saved."
            << BMQTSK_ALARMLOG_END;
        return false;  // RETURN
    }

    d_isPersistentStateDirty = false;
    return true;
}

void PartitionRaft::dispatchMessages(RaftNodeOutput* output)
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(output);

    // A message carries the term, and a vote reply the vote: neither may
    // leave before they are on disk.
    if (!savePersistentState(output->d_persistentStateChanged)) {
        return;  // RETURN
    }

    for (bsl::vector<RaftMessage>::size_type i = 0;
         i < output->d_messages.size();
         ++i) {
        const RaftMessage& msg = output->d_messages[i];
        if (msg.d_type == RaftMessageType::e_APPEND_ENTRIES) {
            sendAppendEntries(msg);
        }
        else if (msg.d_type == RaftMessageType::e_APPEND_ENTRIES_RESP) {
            sendAppendEntriesResponse(msg);
        }
        else if (msg.d_type == RaftMessageType::e_INSTALL_SNAPSHOT) {
            // The control message must precede the chunks: it is what puts
            // the peer into receiving mode ('onRaftControlMessage' ->
            // 'beginReceiveSnapshot'), and 'appendSnapshotChunk' drops any
            // chunk that arrives before that.  Both go to the same channel in
            // enqueue order, so this ordering holds on the wire.
            sendControlMessage(msg);
            sendSnapshot(msg.d_destinationNodeId, msg.d_lastLogIndex);
        }
        else {
            sendControlMessage(msg);
        }
    }
}

void PartitionRaft::dispatchOutput(RaftNodeOutput* output)
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(output);

    dispatchMessages(output);

    if (!isLeader()) {
        // The proposals above the log (the current one and those buffered for
        // a rollover) can no longer be appended.  Hold them for
        // 'repostHeldProposals'.  Entries already in 'd_index' stay (hence
        // 'lastIndex() + 1'): they can still commit, and 'truncateFrom' drops
        // the ones that do not.
        d_raftLog_mp->dropProposalsFrom(d_raftLog_mp->lastIndex() + 1,
                                        true);  // hold to re-post
    }

    if (output->d_lostLeadership) {
        d_isRolloverPending = false;

        // The queues stay local for now.  The cluster will convert the queue
        // to remote after dropping all cluster node handles.  Until then,
        // 'propose' holds new writes.
        //
        // Each uncommitted entry this node appended holds the 'Proposal' this
        // node proposed ('EntryInfo::d_ownProposal'):
        // 1. If the entry commits under the new leader, this node applies the
        //    'Proposal' ('FileStore::onRecordCommittedPrimary'), which ACKs a
        //    strong consistency PUT.
        // 2. If a conflicting AppendEntries truncates the entry,
        //    'truncateFrom' holds the 'Proposal' for 'repostHeldProposals'.
        //    An eventual consistency PUT was ACKed at propose.
    }

    bool hadRollover = false;

    // Read once for the whole batch: every entry below is applied in this same
    // call, so they share an apply time to within the loop itself.
    const bsls::Types::Int64 commitTimepoint =
        bmqu::Time::highResolutionTimer();

    d_isDispatchingOutput = true;

    for (bsl::vector<LogEntry>::size_type i = 0;
         i < output->d_committed.size();
         ++i) {
        const LogEntry& entry = output->d_committed[i];

        if (d_raftLog_mp->isRollover(entry.d_index)) {
            // 'rollover()' erases the committed 'e_ROLLOVER' entry, which
            // takes with it the write this node proposed for it -- the one
            // entry 'applyCommittedEntry' never sees -- keeping
            // 'drainProposals' from replaying it into the new file set.
            d_raftLog_mp->rollover(entry.d_index);
            hadRollover = true;
        }
        else {
            applyCommittedEntry(entry, commitTimepoint);
        }
    }

    d_isDispatchingOutput = false;

    postDispatch(output, hadRollover);

    // 'd_leaderChanged' is set when '(leaderId, term)' changes, and on every
    // election timeout in 'e_NEVER' mode.
    if (output->d_leaderChanged) {
        const int                 leaderNodeId = d_raftNode_mp->leaderId();
        const bsls::Types::Uint64 term         = d_raftNode_mp->currentTerm();

        if (RaftNode::k_INVALID_NODE_ID != leaderNodeId) {
            // Record the primary identity on the FileStore so
            // 'fs->primaryNode()', 'primaryLeaseId()' and 'd_isPrimary'
            // reflect Raft leadership (the primary sets self, replicas set the
            // remote leader).  'isRaft=true' keeps only the identity
            // bookkeeping and skips the legacy sync-point machinery (Raft
            // drives sync points through its own log).
            mqbnet::ClusterNode* leaderNode =
                (leaderNodeId ==
                 d_clusterData_p->membership().selfNode()->nodeId())
                    ? d_clusterData_p->membership().selfNode()
                    : peerNode(leaderNodeId);
            if (leaderNode) {
                d_fileStore_sp->setActivePrimary(
                    leaderNode,
                    static_cast<unsigned int>(term),
                    /* isRaft */ true);
            }
        }

        // Set 'd_needsBecomeLeaderSyncPoint' if this node is the leader.  The
        // become-leader sync point is not proposed here: the sync point waits
        // until the CSL commits this leaseId (the term), and the cluster then
        // calls 'proposeDeferredSyncPoint'.  'd_needsBecomeLeaderSyncPoint'
        // cannot be inferred from 'lastTerm() == currentTerm()': applying
        // earlier entries can propose current-term records (per-app purge
        // DELETIONs) before this sync point.
        d_needsBecomeLeaderSyncPoint = isLeader();

        // Signal the cluster so it can (re)compute this partition's
        // primary/gate state.
        mqbnet::ClusterNode* leader = peerNode(leaderNodeId);

        BALL_LOG_INFO << "Partition [" << d_partitionId
                      << "] invoking d_leadershipCb with leader="
                      << (leader ? leader->hostName().c_str() : "none")
                      << ", term=" << term << ", haveCommit=false";
        d_leadershipCb(d_partitionId, leaderNodeId, term, false);
    }
}

void PartitionRaft::convertQueuesToRemote()
{
    // executed by the partition *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fileStore_sp->inDispatcherThread());

    d_fileStore_sp->convertQueuesToRemote(
        d_clusterData_p->clusterConfig().queueOperations().ackWindowSize());

    // After the conversion, so each PUT reaches a queue that can relay the
    // PUT.  After the cluster's handle drops, which the cluster scheduled
    // ahead of this on the same thread, so a PUT from a peer finds no
    // producer and is dropped.
    repostHeldProposals();
}

void PartitionRaft::repostHeldProposals()
{
    // executed by the partition *DISPATCHER* thread

    PartitionRaftLog::HeldProposals held(d_allocator_p);
    d_raftLog_mp->takeHeldProposals(&held);

    if (held.empty()) {
        return;  // RETURN
    }

    size_t numReposted   = 0;
    size_t numKept       = 0;
    size_t numNoProducer = 0;
    size_t numDropped    = 0;

    for (size_t i = 0; i < held.size(); ++i) {
        const mqbs::Proposal& pw = *held[i];

        mqbi::QueueHandle* handle = pw.d_recordType ==
                                            mqbs::RecordType::e_MESSAGE
                                        ? pw.d_attributes.queueHandle()
                                        : 0;
        mqbi::Queue*       queue  = handle ? handle->queue() : 0;

        // A truncation can land before the conversion the stepdown asked the
        // cluster for.  Re-posting into a queue that is still local would
        // only propose the write and hold it again, so leave it for the
        // conversion, which re-posts too.
        if (queue && queue->isLocal()) {
            d_raftLog_mp->holdProposal(held[i]);
            ++numKept;
            continue;  // CONTINUE
        }

        // The record never entered this node's storage, so what 'propose'
        // reserved for the record goes back, whether or not it is re-posted.
        d_fileStore_sp->undoPropose(pw);

        if (pw.d_recordType != mqbs::RecordType::e_MESSAGE) {
            ++numDropped;
            continue;  // CONTINUE
        }

        // Gone means either a cluster peer whose handle the demotion dropped,
        // or a client that has since disconnected.  Either way there is no
        // producer left here to answer.  A peer re-sends its unacknowledged
        // PUTs, except broadcast PUTs, when it reopens on the new primary.
        if (!queue || !queue->hasHandle(handle)) {
            ++numNoProducer;
            continue;  // CONTINUE
        }

        // 'Channel::pack' reads only these from the header and recomputes the
        // word counts from the payload, so the original need not be kept.
        bmqp::PutHeader header;
        header.setMessageGUID(pw.d_guid)
            .setQueueId(static_cast<int>(queue->id()))
            .setCompressionAlgorithmType(
                pw.d_attributes.compressionAlgorithmType())
            .setCrc32c(pw.d_attributes.crc32c());

        int flags = header.flags();
        bmqp::PutHeaderFlagUtil::setFlag(
            &flags,
            bmqp::PutHeaderFlags::e_ACK_REQUESTED);
        header.setFlags(flags);

        pw.d_attributes.messagePropertiesInfo().applyTo(&header);

        // This runs on the partition's dispatcher thread, which is the
        // queue's, so hand the message straight to the queue.
        queue->postMessage(header, pw.d_appData, pw.d_options, handle);
        ++numReposted;
    }

    if (numKept == held.size()) {
        // Nothing changed: every one of them is waiting on the conversion.
        return;  // RETURN
    }

    BALL_LOG_INFO << "Partition [" << d_partitionId << "] of " << held.size()
                  << " held record(s): re-posted " << numReposted
                  << " PUT(s) to the new primary, kept " << numKept
                  << " PUT(s) for 'convertQueuesToRemote', dropped "
                  << numNoProducer << " PUT(s) with no producer left to ACK.";

    if (numDropped) {
        BALL_LOG_WARN << "Partition [" << d_partitionId << "] dropped "
                      << numDropped << " held record(s) other than PUT.";
    }
}

void PartitionRaft::reproposeHeldProposals()
{
    // executed by the partition *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isLeader());
    BSLS_ASSERT_SAFE(!d_raftNode_mp->isTransferringLeadership());

    PartitionRaftLog::HeldProposals held(d_allocator_p);
    d_raftLog_mp->takeHeldProposals(&held);

    if (held.empty()) {
        return;  // RETURN
    }

    // Propose them into this node's log: the queues are still local.  No
    // 'undoPropose': the capacity 'put' reserved is for this write.
    BALL_LOG_INFO << "Partition [" << d_partitionId << "] proposing "
                  << held.size() << " write(s) held for a leadership "
                  << "transfer that was abandoned.";

    rePropose(held);
}

void PartitionRaft::postDispatch(RaftNodeOutput* output, bool hadRollover)
{
    // executed by the partition *DISPATCHER* thread

    if (!d_deferred.empty()) {
        // Propose the DELETIONs the apply loop deferred.  Swap first:
        // 'propose' runs a nested 'dispatchOutput', which reaches this code
        // again.
        bsl::vector<bsl::shared_ptr<mqbs::Proposal> > deferred(d_allocator_p);
        deferred.swap(d_deferred);

        for (size_t i = 0; i < deferred.size(); ++i) {
            const int rc = propose(deferred[i]);
            if (rc != 0) {
                BALL_LOG_ERROR << "Partition [" << d_partitionId
                               << "] failed to propose a deferred "
                               << deferred[i]->d_recordType
                               << " record, rc: " << rc;

                d_fileStore_sp->undoPropose(*deferred[i]);
            }
        }
    }

    if (hadRollover && isLeader()) {
        BSLS_ASSERT_SAFE(d_isRolloverPending);

        // The 'e_ROLLOVER' just committed and rolled over in the apply loop.
        // Propose the writes buffered during the rollover ('drainProposals').
        // Clear the flag before the drain: the drain goes through 'propose',
        // which buffers while the flag is set.
        d_isRolloverPending = false;

        drainProposals();
    }

    if (!isLeader()) {
        repostHeldProposals();
    }
    else if (!d_raftNode_mp->isTransferringLeadership()) {
        reproposeHeldProposals();
    }

    if (d_fileStore_sp->takePurgeCompleted()) {
        // A committed whole-queue purge freed journal space.
        // 'onPurgeComplete' proposes an 'e_ROLLOVER' if one is needed.
        onPurgeComplete();
    }

    if (output->d_hasMoreToApply) {
        // 'RaftNode::loadCommittedBatch' stopped at 'k_MAX_APPLY_PER_BATCH'.
        // 'commitTo' runs again only when the commit index advances, so post
        // the next batch here.  Posting gives the dispatcher a turn between
        // batches.
        execute(
            bdlf::BindUtil::bind(&PartitionRaft::applyCommittedBatchDispatched,
                                 this));
    }

    // Re-drive delivery once for the whole batch applied above.  Each
    // committed message only noted its queue.  So a queue that took several
    // entries is walked once, and a queue deleted later in the same batch is
    // not walked.
    d_fileStore_sp->notifyQueuesOnReplicatedBatch();
}

void PartitionRaft::sendAppendEntries(const RaftMessage& msg)
{
    // executed by the partition *DISPATCHER* thread
    bsl::shared_ptr<bdlbb::Blob> event_sp =
        d_clusterData_p->blobSpPool().getObject();
    bdlbb::Blob& event = *event_sp;

    event.setLength(sizeof(bmqp::EventHeader) + sizeof(bmqp::RaftHeader) +
                    sizeof(bmqp::RaftAppendEntriesHeader));

    bmqu::BlobObjectProxy<bmqp::RaftHeader> rh(
        &event,
        bmqu::BlobPosition(0, static_cast<int>(sizeof(bmqp::EventHeader))),
        true,   // read
        true);  // write
    *rh = bmqp::RaftHeader();
    (*rh)
        .setMsgType(bmqp::RaftHeader::k_MSG_TYPE_APPEND_ENTRIES)
        .setPartitionId(d_partitionId)
        .setTerm(msg.d_term);
    rh.reset();

    bmqu::BlobObjectProxy<bmqp::RaftAppendEntriesHeader> aeh(
        &event,
        bmqu::BlobPosition(0,
                           static_cast<int>(sizeof(bmqp::EventHeader) +
                                            sizeof(bmqp::RaftHeader))),
        true,   // read
        true);  // write
    *aeh = bmqp::RaftAppendEntriesHeader();
    (*aeh)
        .setPrevLogIndex(msg.d_prevLogIndex)
        .setPrevLogTerm(msg.d_prevLogTerm)
        .setLeaderCommit(msg.d_leaderCommit)
        .setEntryCount(static_cast<unsigned int>(msg.d_entries.size()));
    aeh.reset();

    for (bsl::vector<LogEntry>::size_type i = 0; i < msg.d_entries.size();
         ++i) {
        bmqu::BlobUtil::appendToBlob(&event,
                                     *msg.d_entries[i].d_data,
                                     bmqu::BlobPosition());
    }

    bmqu::BlobObjectProxy<bmqp::EventHeader> eh(&event);
    (*eh) = bmqp::EventHeader(bmqp::EventType::e_RAFT_PARTITION);
    (*eh).setLength(event.length());
    eh.reset();

    // One event for every destination at this 'prevLogIndex'.  Each channel
    // queues the same 'event_sp'.
    for (size_t i = 0; i < msg.destinationCount(); ++i) {
        const int            nodeId   = msg.destination(i);
        mqbnet::ClusterNode* destNode = peerNode(nodeId);
        if (!destNode) {
            BALL_LOG_WARN << "Partition [" << d_partitionId
                          << "] cannot send AppendEntries to unknown node "
                          << nodeId;
            continue;  // CONTINUE
        }
        destNode->write(event_sp, bmqp::EventType::e_RAFT_PARTITION);
    }
}

void PartitionRaft::sendAppendEntriesResponse(const RaftMessage& msg)
{
    // executed by the partition *DISPATCHER* thread
    mqbnet::ClusterNode* destNode = peerNode(msg.d_destinationNodeId);
    if (!destNode) {
        BALL_LOG_WARN
            << "Partition [" << d_partitionId
            << "] cannot send AppendEntries response to unknown node "
            << msg.d_destinationNodeId;
        return;  // RETURN
    }

    bsl::shared_ptr<bdlbb::Blob> event_sp =
        d_clusterData_p->blobSpPool().getObject();
    bdlbb::Blob& event = *event_sp;

    event.setLength(sizeof(bmqp::EventHeader) + sizeof(bmqp::RaftHeader) +
                    sizeof(bmqp::RaftResponseHeader));

    bmqu::BlobObjectProxy<bmqp::RaftHeader> rh(
        &event,
        bmqu::BlobPosition(0, static_cast<int>(sizeof(bmqp::EventHeader))),
        true,   // read
        true);  // write
    *rh = bmqp::RaftHeader();
    (*rh)
        .setMsgType(bmqp::RaftHeader::k_MSG_TYPE_APPEND_ENTRIES_RESP)
        .setPartitionId(d_partitionId)
        .setTerm(msg.d_term);
    rh.reset();

    bmqu::BlobObjectProxy<bmqp::RaftResponseHeader> resp(
        &event,
        bmqu::BlobPosition(0,
                           static_cast<int>(sizeof(bmqp::EventHeader) +
                                            sizeof(bmqp::RaftHeader))),
        true,   // read
        true);  // write
    *resp = bmqp::RaftResponseHeader();
    (*resp)
        .setSuccess(msg.d_success)
        .setMatchIndex(msg.d_matchIndex)
        .setRejectedIndex(msg.d_rejectedIndex);
    resp.reset();

    bmqu::BlobObjectProxy<bmqp::EventHeader> eh(&event);
    (*eh) = bmqp::EventHeader(bmqp::EventType::e_RAFT_PARTITION);
    (*eh).setLength(event.length());
    eh.reset();

    destNode->write(event_sp, bmqp::EventType::e_RAFT_PARTITION);
}

void PartitionRaft::sendControlMessage(const RaftMessage& msg)
{
    // executed by the partition *DISPATCHER* thread
    mqbnet::ClusterNode* destNode = peerNode(msg.d_destinationNodeId);
    if (!destNode) {
        BALL_LOG_WARN << "Partition [" << d_partitionId
                      << "] cannot send Raft control to unknown node "
                      << msg.d_destinationNodeId;
        return;
    }

    bmqp_ctrlmsg::ControlMessage controlMsg;
    bmqp_ctrlmsg::RaftMessage& raftMsg = controlMsg.choice().makeRaftMessage();
    toCtrlMsg(&raftMsg, msg);

    // Use this partition's own transmitter ('FileStore::sendMessage').  This
    // is to avoid using the cluster's transmitter, which is usable only on the
    // cluster dispatcher thread.
    d_fileStore_sp->sendMessage(controlMsg, destNode);
}

void PartitionRaft::applyCommittedEntry(const LogEntry&    entry,
                                        bsls::Types::Int64 commitTimepoint)
{
    // executed by the partition *DISPATCHER* thread

    // Read before applying: the entry leaves the log's index below.
    const bool isSyncPoint = d_raftLog_mp->isRegularSyncPoint(entry.d_index);

    // The log routes by the entry.  An entry holding this node's own write is
    // applied from that write ('FileStore::onRecordCommittedPrimary'),
    // whatever this node's leadership is now: only the write holds what that
    // apply needs.  Any other entry is applied from its blob
    // ('FileStore::onRecordCommittedReplica').
    d_raftLog_mp->applyCommittedEntry(entry.d_index,
                                      entry.d_data,
                                      commitTimepoint);

    if (isSyncPoint && entry.d_term == d_raftNode_mp->currentTerm() &&
        entry.d_term > d_reportedSyncPointTerm) {
        // Report the first sync point committed under the current term.
        // Applying it means this node, leader or replica, has applied every
        // entry before it, including those inherited from earlier terms.
        // 'ClusterOrchestrator::maybeTransitionToAvailable' waits for this
        // report.
        d_reportedSyncPointTerm = entry.d_term;

        d_leadershipCb(d_partitionId,
                       d_raftNode_mp->leaderId(),
                       entry.d_term,
                       true);
    }
}

void PartitionRaft::sendSnapshot(int                 destNodeId,
                                 bsls::Types::Uint64 lastIncludedIndex)
{
    // executed by the partition *DISPATCHER* thread
    mqbnet::ClusterNode* destNode = peerNode(destNodeId);
    if (!destNode) {
        BALL_LOG_WARN << "Partition [" << d_partitionId
                      << "] cannot send snapshot to unknown node "
                      << destNodeId;
        return;
    }

    BALL_LOG_INFO << "Partition [" << d_partitionId
                  << "] sending snapshot to node " << destNode->hostName()
                  << ", lastIncludedIndex=" << lastIncludedIndex;

    mqbs::FileStoreSet fileSet;
    d_fileStore_sp->loadCurrentFiles(&fileSet);

    // Send order: data → qlist → journal, followed by a payload-less terminal
    // chunk carrying the done flag.  Files with size 0 are skipped.
    struct FileDesc {
        unsigned int               d_fileType;
        mqbs::MappedFileDescriptor d_mfd;
        bsls::Types::Uint64        d_size;
    };

    FileDesc allFiles[3] = {{bmqp::SnapshotChunkHeader::k_FILE_TYPE_DATA,
                             mqbs::MappedFileDescriptor(),
                             fileSet.dataFileSize()},
                            {bmqp::SnapshotChunkHeader::k_FILE_TYPE_QLIST,
                             mqbs::MappedFileDescriptor(),
                             fileSet.qlistFileSize()},
                            {bmqp::SnapshotChunkHeader::k_FILE_TYPE_JOURNAL,
                             mqbs::MappedFileDescriptor(),
                             fileSet.journalFileSize()}};

    // 'allFiles' is in send order; 'openFileSetReadMode' takes journal, data,
    // qlist.
    bmqu::MemOutStream errorDesc;
    int                rc = mqbs::FileStoreUtil::openFileSetReadMode(errorDesc,
                                                      fileSet,
                                                      &allFiles[2].d_mfd,
                                                      &allFiles[0].d_mfd,
                                                      &allFiles[1].d_mfd);

    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("FILE_IO")
            << d_clusterData_p->identity().description() << " Partition ["
            << d_partitionId
            << "]: Failed to open one of JOURNAL/QLIST/DATA file, rc: " << rc
            << ", reason [" << errorDesc.str()
            << "] while sending snapshot: to node: "
            << destNode->nodeDescription() << BMQTSK_ALARMLOG_END;

        return;  // RETURN
    }

    // The caller ('dispatchMessages') sends the InstallSnapshot control
    // message that puts the follower into receiving mode, just before this.

    const int hdrSize = static_cast<int>(sizeof(bmqp::EventHeader) +
                                         sizeof(bmqp::SnapshotChunkHeader));

    for (int f = 0; f < 3; ++f) {
        bsls::Types::Uint64 offset = 0;
        while (offset < allFiles[f].d_size) {
            bsls::Types::Uint64 remaining = allFiles[f].d_size - offset;
            bsls::Types::Uint64 chunkLen  = bsl::min(k_CHUNK_SIZE, remaining);

            bsl::shared_ptr<bdlbb::Blob> event_sp =
                d_clusterData_p->blobSpPool().getObject();
            bdlbb::Blob& event = *event_sp;

            event.setLength(hdrSize);

            bmqu::BlobObjectProxy<bmqp::SnapshotChunkHeader> hdr(
                &event,
                bmqu::BlobPosition(
                    0,
                    static_cast<int>(sizeof(bmqp::EventHeader))),
                true,   // read
                true);  // write
            (*hdr)
                .setPartitionId(d_partitionId)
                .setFileType(allFiles[f].d_fileType)
                .setDone(false)
                .setLastIncludedIndex(lastIncludedIndex)
                .setOffset(offset)
                .setTotalSize(allFiles[f].d_size)
                .setChunkLength(static_cast<unsigned int>(chunkLen));
            hdr.reset();

            bdlbb::BlobUtil::append(&event,
                                    allFiles[f].d_mfd.block().base() + offset,
                                    chunkLen);

            bmqu::BlobObjectProxy<bmqp::EventHeader> eh(&event);
            (*eh) = bmqp::EventHeader(bmqp::EventType::e_RAFT_SNAPSHOT);
            eh->setLength(event.length());
            eh.reset();

            const bmqt::GenericResult::Enum writeRc =
                destNode->write(event_sp, bmqp::EventType::e_RAFT_SNAPSHOT);
            if (bmqt::GenericResult::e_NOT_CONNECTED == writeRc) {
                // The channel went away mid-transfer.  Mark the peer
                // unavailable, which resets its state in 'RaftNode'.  This is
                // to avoid the snapshot staying marked in flight, which
                // suppresses replication to the peer until a timeout.
                BALL_LOG_WARN << "Partition [" << d_partitionId
                              << "] snapshot to node " << destNode->hostName()
                              << " aborted: channel is gone";
                d_raftNode_mp->setPeerAvailability(destNodeId, false);

                mqbs::FileStoreUtil::closePartitionSet(&allFiles[0].d_mfd,
                                                       &allFiles[2].d_mfd,
                                                       &allFiles[1].d_mfd);
                return;  // RETURN
            }

            offset += chunkLen;
        }
    }

    // Send the done flag in its own chunk: the receiver finalizes on the flag,
    // and a file of size 0 sends no chunk that could carry the flag.
    {
        bsl::shared_ptr<bdlbb::Blob> event_sp =
            d_clusterData_p->blobSpPool().getObject();
        bdlbb::Blob& event = *event_sp;

        event.setLength(hdrSize);

        bmqu::BlobObjectProxy<bmqp::SnapshotChunkHeader> hdr(
            &event,
            bmqu::BlobPosition(0, static_cast<int>(sizeof(bmqp::EventHeader))),
            true,   // read
            true);  // write
        (*hdr)
            .setPartitionId(d_partitionId)
            .setFileType(bmqp::SnapshotChunkHeader::k_FILE_TYPE_JOURNAL)
            .setDone(true)
            .setLastIncludedIndex(lastIncludedIndex)
            .setOffset(0)
            .setTotalSize(0)
            .setChunkLength(0);
        hdr.reset();

        bmqu::BlobObjectProxy<bmqp::EventHeader> eh(&event);
        (*eh) = bmqp::EventHeader(bmqp::EventType::e_RAFT_SNAPSHOT);
        eh->setLength(event.length());
        eh.reset();

        destNode->write(event_sp, bmqp::EventType::e_RAFT_SNAPSHOT);
    }

    // 'closePartitionSet' takes data, journal, qlist.
    rc = mqbs::FileStoreUtil::closePartitionSet(&allFiles[0].d_mfd,
                                                &allFiles[2].d_mfd,
                                                &allFiles[1].d_mfd);
    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("FILE_IO")
            << d_clusterData_p->identity().description() << " Partition ["
            << d_partitionId
            << "]: Failed to close one of JOURNAL/QLIST/DATA file, rc: " << rc
            << ", while sending snapshot: to node: "
            << destNode->nodeDescription() << BMQTSK_ALARMLOG_END;
    }

    BALL_LOG_INFO << "Partition [" << d_partitionId
                  << "] snapshot sent to node " << destNode->hostName();
}

void PartitionRaft::beginReceiveSnapshot(bsls::Types::Uint64 lastIncludedIndex,
                                         bsls::Types::Uint64 lastIncludedTerm)
{
    // executed by the partition *DISPATCHER* thread
    BALL_LOG_INFO << "Partition [" << d_partitionId
                  << "] beginning snapshot receive, lastIncludedIndex="
                  << lastIncludedIndex;

    // Close any in-progress snapshot fds
    if (d_snapshotJournalFd >= 0) {
        ::close(d_snapshotJournalFd);
        d_snapshotJournalFd = -1;
    }
    if (d_snapshotDataFd >= 0) {
        ::close(d_snapshotDataFd);
        d_snapshotDataFd = -1;
    }
    if (d_snapshotQlistFd >= 0) {
        ::close(d_snapshotQlistFd);
        d_snapshotQlistFd = -1;
    }

    // A retry can arrive while the FileStore is still closed from an earlier
    // attempt, in which case the paths below are already the ones to write.
    if (d_fileStore_sp->isOpen()) {
        // Get file paths before wiping
        d_fileStore_sp->loadCurrentFiles(&d_snapshotFiles);

        // Empty the storages but keep the objects: their contents are handles
        // into 'd_records', which the close below clears, and the queues hold
        // the objects.  This is to avoid leaving a queue on a storage that
        // recovery has replaced (delivery for the queue would stop on this
        // node).
        d_fileStore_sp->clearStorages();

        // This path closes the 'FileStore' directly, so release here what
        // aliases the file set, as 'PartitionRaft::close' does:
        // 1. The proposals still in the log.  Their record handles are
        //    iterators into 'd_records', which the close invalidates.  From
        //    index 0: the snapshot replaces the whole log, so nothing this
        //    node appended can still commit.
        // 2. The cached entry blobs.  'open()' clears the cache only after the
        //    close.
        d_raftLog_mp->dropProposalsFrom(0, false);  // discard
        d_raftLog_mp->clearCache();

        // Wipe current FileStore
        d_fileStore_sp->close(false, true);  // flush=false, archive=true
    }

    d_snapshotJournalFd = ::open(d_snapshotFiles.journalFile().c_str(),
                                 O_WRONLY | O_CREAT | O_TRUNC,
                                 0644);
    d_snapshotDataFd    = ::open(d_snapshotFiles.dataFile().c_str(),
                              O_WRONLY | O_CREAT | O_TRUNC,
                              0644);
    d_snapshotQlistFd   = ::open(d_snapshotFiles.qlistFile().c_str(),
                               O_WRONLY | O_CREAT | O_TRUNC,
                               0644);

    d_snapshotLastIncludedIndex = lastIncludedIndex;
    d_snapshotLastIncludedTerm  = lastIncludedTerm;
    d_receivingSnapshot         = true;
}

void PartitionRaft::applySnapshotChunk(const bdlbb::Blob&   event,
                                       mqbnet::ClusterNode* source)
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(d_receivingSnapshot);

    bmqu::BlobPosition position;

    if (0 != bmqu::BlobUtil::findOffsetSafe(&position,
                                            event,
                                            sizeof(bmqp::EventHeader))) {
        BALL_LOG_ERROR
            << "Failed to locate RaftHeader in e_RAFT_PARTITION event";
        return;
    }

    bmqu::BlobObjectProxy<bmqp::SnapshotChunkHeader> hdr(&event,
                                                         position,
                                                         true,    // read
                                                         false);  // write

    if (!hdr.isSet()) {
        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] failed to read SnapshotChunkHeader";
        return;
    }

    unsigned int        fileType    = hdr->fileType();
    bsls::Types::Uint64 offset      = hdr->offset();
    unsigned int        chunkLength = hdr->chunkLength();
    bool                done        = hdr->done();

    // The terminal chunk carries no payload; it only marks the end of the
    // snapshot.
    if (chunkLength > 0) {
        int fd = -1;
        if (fileType == bmqp::SnapshotChunkHeader::k_FILE_TYPE_JOURNAL) {
            fd = d_snapshotJournalFd;
        }
        else if (fileType == bmqp::SnapshotChunkHeader::k_FILE_TYPE_DATA) {
            fd = d_snapshotDataFd;
        }
        else if (fileType == bmqp::SnapshotChunkHeader::k_FILE_TYPE_QLIST) {
            fd = d_snapshotQlistFd;
        }

        if (fd < 0) {
            BALL_LOG_ERROR << "Partition [" << d_partitionId
                           << "] no fd for snapshot chunk fileType="
                           << fileType;
            return;
        }

        if (::lseek(fd, static_cast<off_t>(offset), SEEK_SET) < 0) {
            BALL_LOG_ERROR << "Partition [" << d_partitionId
                           << "] lseek failed offset=" << offset;
            return;
        }

        int dataOff = static_cast<int>(sizeof(bmqp::EventHeader) +
                                       sizeof(bmqp::SnapshotChunkHeader));
        bmqu::BlobPosition pos;
        if (0 != bmqu::BlobUtil::findOffsetSafe(&pos, event, dataOff)) {
            return;
        }
        for (int i = pos.buffer();
             i < event.numDataBuffers() && chunkLength > 0;
             ++i) {
            const bdlbb::BlobBuffer& buf = event.buffer(i);
            int          bufStart  = (i == pos.buffer()) ? pos.byte() : 0;
            unsigned int available = static_cast<unsigned int>(buf.size() -
                                                               bufStart);
            unsigned int toWrite   = bsl::min(available, chunkLength);
            ::write(fd, buf.data() + bufStart, toWrite);
            chunkLength -= toWrite;
        }
    }

    if (!done) {
        return;
    }

    ::close(d_snapshotJournalFd);
    ::close(d_snapshotDataFd);
    ::close(d_snapshotQlistFd);
    d_snapshotJournalFd = -1;
    d_snapshotDataFd    = -1;
    d_snapshotQlistFd   = -1;
    d_receivingSnapshot = false;

    BALL_LOG_INFO << "Partition [" << d_partitionId
                  << "] snapshot received, reopening FileStore"
                  << ", lastIncludedIndex=" << d_snapshotLastIncludedIndex;

    // Reopen via full recovery path: fires queueCreationCb, populates
    // d_records, sets d_snapshotIndex from firstSyncPointAfterRolloverSeqNum
    int rc = d_raftLog_mp->open();

    if (rc != 0) {
        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] failed to reopen FileStore after snapshot, rc="
                       << rc;
        return;
    }

    // 'open' also succeeds having recovered nothing, so acknowledge only once
    // the log can serve the installed index.  The log serves it either from
    // the restored files or from the entries above them, so this is the end
    // of the log -- not what the files restored.
    bsls::Types::Uint64 servedIndex = d_raftLog_mp->snapshotIndex();
    if (d_raftLog_mp->lastIndex() > servedIndex) {
        servedIndex = d_raftLog_mp->lastIndex();
    }

    if (servedIndex < d_snapshotLastIncludedIndex) {
        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] snapshot apply reached index " << servedIndex
                       << ", expected " << d_snapshotLastIncludedIndex
                       << "; not acknowledging InstallSnapshot";
        return;
    }

    // The installed files moved the log's floor ('snapshotIndex()'), so raise
    // the applied state to the floor.  This is to avoid asking for entries
    // below the floor.  The files restore storage only up to the floor.
    // 'open' recovered the entries above it as unapplied, and the commit path
    // applies them, as after a restart ('start' seeds the same way).
    d_raftNode_mp->initRecoveredState(d_raftLog_mp->lastTerm(),
                                      d_raftLog_mp->snapshotIndex());

    // For each queue, re-seat it onto its refilled storage and re-drive
    // delivery:
    // 1. The queues kept their storage objects across the close, but the
    //    contents were emptied, so their iterators point at contents that
    //    are gone.
    // 2. The install refilled the storages outside the commit path, which is
    //    the only path that re-drives delivery.  PUSHes that arrived during
    //    the transfer wait in the relay push stream until then.
    d_fileStore_sp->notifyAllQueuesOnRestore();

    // Answer the node that sent the chunks.  This is to avoid answering
    // 'leaderId()', which is 'k_INVALID_NODE_ID' if the term advanced during
    // the transfer.
    RaftMessage resp(d_allocator_p);
    resp.d_type         = RaftMessageType::e_INSTALL_SNAPSHOT_RESP;
    resp.d_term         = d_raftNode_mp->currentTerm();
    resp.d_sourceNodeId = d_clusterData_p->membership().selfNode()->nodeId();
    resp.d_destinationNodeId = source->nodeId();
    resp.d_lastLogIndex      = d_snapshotLastIncludedIndex;
    if (!savePersistentState(false)) {
        return;  // RETURN
    }
    sendControlMessage(resp);

    BALL_LOG_INFO << "Partition [" << d_partitionId
                  << "] snapshot applied, sent InstallSnapshot response";
}

void PartitionRaft::appendSnapshotChunk(
    const bsl::shared_ptr<const bdlbb::Blob>& event,
    mqbnet::ClusterNode*                      source)
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(source);

    if (!d_receivingSnapshot) {
        BALL_LOG_WARN << "Partition [" << d_partitionId
                      << "] received snapshot chunk but not in receiving mode";
        return;
    }

    applySnapshotChunk(*event, source);
}

void PartitionRaft::toCtrlMsg(bmqp_ctrlmsg::RaftMessage* out,
                              const RaftMessage&         msg) const
{
    BSLS_ASSERT_SAFE(out);

    out->term()        = msg.d_term;
    out->partitionId() = d_partitionId + 1;  // 0 reserved for CSL

    switch (msg.d_type) {
    case RaftMessageType::e_REQUEST_VOTE: {
        bmqp_ctrlmsg::RaftRequestVote& rv = out->choice().makeRequestVote();
        rv.lastLogIndex()                 = msg.d_lastLogIndex;
        rv.lastLogTerm()                  = msg.d_lastLogTerm;
        rv.preVote()                      = msg.d_preVote;
    } break;
    case RaftMessageType::e_REQUEST_VOTE_RESP: {
        bmqp_ctrlmsg::RaftRequestVoteResponse& rvr =
            out->choice().makeRequestVoteResponse();
        rvr.voteGranted() = msg.d_success;
        rvr.preVote()     = msg.d_preVote;
    } break;
    case RaftMessageType::e_TIMEOUT_NOW: {
        out->choice().makeTimeoutNow();
    } break;
    case RaftMessageType::e_INSTALL_SNAPSHOT: {
        bmqp_ctrlmsg::RaftInstallSnapshot& is =
            out->choice().makeInstallSnapshot();
        is.lastIncludedIndex() = msg.d_lastLogIndex;
        is.lastIncludedTerm()  = msg.d_lastLogTerm;
        is.offset()            = 0;
        is.done()              = true;
    } break;
    case RaftMessageType::e_INSTALL_SNAPSHOT_RESP: {
        out->choice().makeInstallSnapshotResponse();
    } break;
    case RaftMessageType::e_APPEND_ENTRIES:
    case RaftMessageType::e_APPEND_ENTRIES_RESP:
    default:
        // Both go through the binary path ('sendAppendEntries',
        // 'sendAppendEntriesResponse').
        BSLS_ASSERT_SAFE(false);
        break;
    }
}

void PartitionRaft::fromCtrlMsg(RaftMessage*                     out,
                                const bmqp_ctrlmsg::RaftMessage& msg,
                                int sourceNodeId) const
{
    BSLS_ASSERT_SAFE(out);

    typedef bmqp_ctrlmsg::RaftMessageChoice Choice;

    out->d_term         = msg.term();
    out->d_sourceNodeId = sourceNodeId;

    switch (msg.choice().selectionId()) {
    case Choice::SELECTION_ID_REQUEST_VOTE: {
        const bmqp_ctrlmsg::RaftRequestVote& rv = msg.choice().requestVote();
        out->d_type         = RaftMessageType::e_REQUEST_VOTE;
        out->d_lastLogIndex = rv.lastLogIndex();
        out->d_lastLogTerm  = rv.lastLogTerm();
        out->d_preVote      = rv.preVote();
    } break;
    case Choice::SELECTION_ID_REQUEST_VOTE_RESPONSE: {
        const bmqp_ctrlmsg::RaftRequestVoteResponse& rvr =
            msg.choice().requestVoteResponse();
        out->d_type    = RaftMessageType::e_REQUEST_VOTE_RESP;
        out->d_success = rvr.voteGranted();
        out->d_preVote = rvr.preVote();
    } break;
    case Choice::SELECTION_ID_TIMEOUT_NOW: {
        out->d_type = RaftMessageType::e_TIMEOUT_NOW;
    } break;
    case Choice::SELECTION_ID_INSTALL_SNAPSHOT: {
        const bmqp_ctrlmsg::RaftInstallSnapshot& is =
            msg.choice().installSnapshot();
        out->d_type         = RaftMessageType::e_INSTALL_SNAPSHOT;
        out->d_lastLogIndex = is.lastIncludedIndex();
        out->d_lastLogTerm  = is.lastIncludedTerm();
    } break;
    case Choice::SELECTION_ID_INSTALL_SNAPSHOT_RESPONSE: {
        out->d_type = RaftMessageType::e_INSTALL_SNAPSHOT_RESP;
    } break;
    default: break;
    }
}

void PartitionRaft::tickCb()
{
    // executed by the *SCHEDULER* thread
    d_fileStore_sp->execute(
        bdlf::BindUtil::bind(&PartitionRaft::tickDispatched, this));
}

void PartitionRaft::applyCommittedBatchDispatched()
{
    // executed by the partition *DISPATCHER* thread
    if (!d_isStarted || !d_fileStore_sp->isOpen()) {
        return;  // RETURN
    }

    RaftNodeOutput output(d_allocator_p);
    d_raftNode_mp->loadCommittedBatch(&output);
    dispatchOutput(&output);
}

void PartitionRaft::tickDispatched()
{
    // executed by the partition *DISPATCHER* thread
    RaftNodeOutput output(d_allocator_p);
    d_raftNode_mp->tick(&output);
    dispatchOutput(&output);

    // Refresh the partition stats on each tick.  Raft has no periodic sync
    // point to refresh the stats.
    d_fileStore_sp->updatePartitionStats();
}

void PartitionRaft::setElectionMode(ElectionMode::Enum mode)
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(d_fileStore_sp->inDispatcherThread());

    RaftNodeOutput output(d_allocator_p);
    d_raftNode_mp->setElectionMode(&output, mode);
    dispatchOutput(&output);
}

// MANIPULATORS
void PartitionRaft::start()
{
    // executed by this partition's dispatcher thread
    BSLS_ASSERT_SAFE(d_fileStore_sp->inDispatcherThread());

    const int rc = d_raftLog_mp->open();
    if (0 != rc) {
        // A FileStore open or recovery failure is unrecoverable: ALARM and
        // terminate.
        BMQTSK_ALARMLOG_ALARM("FILE_IO")
            << d_clusterData_p->identity().description() << " Partition ["
            << d_partitionId
            << "]: failed to open/recover PartitionRaftLog, rc: " << rc << "."
            << BMQTSK_ALARMLOG_END;

        mqbu::ExitUtil::terminate(mqbu::ExitCode::e_RECOVERY_FAILURE);  // EXIT
    }

    // Seed the recovered term and applied state ('initRecoveredState' only
    // raises them).  The term is the log's last term, which for a journal
    // written by legacy is its leaseId.  The term must never decrease across a
    // restart.  The commit index and the applied state take the snapshot
    // boundary ('snapshotIndex()', above 0 once the partition has rolled
    // over).  Recovery restores storage up to that point, and the commit path
    // applies the entries above it once a leader confirms them.
    d_raftNode_mp->initRecoveredState(d_raftLog_mp->lastTerm(),
                                      d_raftLog_mp->snapshotIndex());

    // The term and vote, which the log does not hold.
    d_persistentStatePath =
        d_clusterData_p->clusterConfig().partitionConfig().location();
    bdls::PathUtil::appendRaw(&d_persistentStatePath, "bmq_raft_");
    d_persistentStatePath.append(bsl::to_string(d_partitionId));
    d_persistentStatePath.append(".bmq_raft");

    PersistentState    persistentState;
    bmqu::MemOutStream errorDescription(d_allocator_p);
    const int          loadRc = PersistentStateUtil::load(&persistentState,
                                                 errorDescription,
                                                 d_persistentStatePath);
    if (0 != loadRc) {
        BMQTSK_ALARMLOG_ALARM("FILE_IO")
            << d_clusterData_p->identity().description() << " Partition ["
            << d_partitionId
            << "]: failed to load Raft term and vote, rc: " << loadRc << ", "
            << errorDescription.str() << "." << BMQTSK_ALARMLOG_END;

        mqbu::ExitUtil::terminate(mqbu::ExitCode::e_RECOVERY_FAILURE);  // EXIT
    }
    d_raftNode_mp->restorePersistentState(persistentState);

    // Register before seeding: 'onNodeStateChange' reports transitions only,
    // and it dispatches to this same thread, so an event raised during the
    // seed below queues behind it and applies after.
    mqbnet::Cluster* netCluster = d_clusterData_p->membership().netCluster();
    netCluster->registerObserver(this);

    const mqbnet::Cluster::NodesList& nodes = netCluster->nodes();
    for (mqbnet::Cluster::NodesList::const_iterator it = nodes.begin();
         it != nodes.end();
         ++it) {
        d_raftNode_mp->setPeerAvailability((*it)->nodeId(),
                                           (*it)->isAvailable());
    }

    bsls::TimeInterval tickInterval;
    tickInterval.setTotalMilliseconds(k_TICK_INTERVAL_MS);

    d_clusterData_p->scheduler().scheduleRecurringEvent(
        &d_tickHandle,
        tickInterval,
        bdlf::BindUtil::bind(&PartitionRaft::tickCb, this));

    d_isStarted = true;

    // The dispatcher calls 'flush' once it has drained this partition's queue.
    d_fileStore_sp->setFlushCallback(
        bdlf::BindUtil::bind(&PartitionRaft::flush, this));

    BALL_LOG_INFO << "PartitionRaft started for partition " << d_partitionId
                  << ", node " << d_raftNode_mp->selfId();
}

void PartitionRaft::stop()
{
    if (!d_isStarted) {
        return;
    }

    d_fileStore_sp->setFlushCallback(bsl::function<void()>());

    d_clusterData_p->membership().netCluster()->unregisterObserver(this);

    d_clusterData_p->scheduler().cancelEventAndWait(&d_tickHandle);
    d_isStarted = false;

    BALL_LOG_INFO << "PartitionRaft stopped for partition " << d_partitionId;
}

int PartitionRaft::propose(const bsl::shared_ptr<mqbs::Proposal>& pw)
{
    // executed by the partition *DISPATCHER* thread
    enum { rc_UNAVAILABLE = -1 };

    if (d_raftNode_mp->isTransferringLeadership() &&
        pw->d_recordType == mqbs::RecordType::e_MESSAGE) {
        // Hold the PUT.  'repostHeldProposals' re-posts the PUT to the new
        // primary, or 'reproposeHeldProposals' proposes the PUT here if the
        // transfer is abandoned.  Every other record is proposed.
        d_raftLog_mp->holdProposal(pw);
        return 0;  // RETURN
    }

    // Skipped while a rollover is pending: 'proposeRollover' marks the
    // journal unavailable for the rollover's duration.  This is to avoid the
    // read-only branch below rejecting the writes that should buffer.
    if (!d_isRolloverPending) {
        if (!d_fileStore_sp->isFileSetAvailable()) {
            // The journal is read-only: a rollover could not reclaim enough
            // space (outstanding records exceed the policy threshold).  The
            // only write allowed is a full-queue PURGE, into the reserved
            // PURGE area ('primaryHasPurgeReserve').  The purge frees
            // outstanding records, and 'onPurgeComplete' then proposes a
            // rollover.  A per-app purge (non-null appKey) is rejected: its
            // per-message DELETION records have no room.
            const bool isPurge = pw->d_recordType ==
                                     mqbs::RecordType::e_QUEUE_OP &&
                                 pw->d_queueOpType ==
                                     mqbs::QueueOpType::e_PURGE &&
                                 pw->d_appKey.isNull();
            if (!isPurge || !d_fileStore_sp->primaryHasPurgeReserve()) {
                return rc_UNAVAILABLE;  // RETURN
            }

            BALL_LOG_WARN << "Partition [" << d_partitionId
                          << "] Writing PURGE record for queueKey ["
                          << pw->d_queueKey
                          << "] into the reserved journal area despite the "
                          << "partition being read-only (unavailable).";
            // Fall through: append the PURGE directly (no rollover); the
            // reserved area guarantees room.
        }
        else {
            // Compute the rollover footprint (DATA and QLIST bytes) from the
            // write. The JOURNAL reserve is always checked by
            // 'rolloverIfNeeded'; only messages consume DATA and only queue
            // creations consume QLIST.
            bsls::Types::Uint64 dataBytes  = 0;
            bsls::Types::Uint64 qlistBytes = 0;
            if (pw->d_recordType == mqbs::RecordType::e_MESSAGE) {
                dataBytes = mqbs::FileStoreProtocolUtil::messageDataFileSize(
                    pw->d_appData,
                    pw->d_options);
            }
            else if (pw->d_recordType == mqbs::RecordType::e_QUEUE_OP &&
                     pw->d_queueOpType == mqbs::QueueOpType::e_CREATION) {
                qlistBytes =
                    mqbs::FileStoreProtocolUtil::queueCreationQlistFileSize(
                        pw->d_queueInfo->d_queueUri,
                        pw->d_queueInfo->d_appIdKeyPairs);
            }

            int rc = rolloverIfNeeded(dataBytes, qlistBytes);
            if (0 != rc) {
                return rc;  // RETURN
            }
        }
    }

    if (d_isRolloverPending) {
        // Buffer the write for replay into the new file set once the rollover
        // commits.  This covers the write that triggered the rollover and
        // every write after it until the commit.
        return d_raftLog_mp->bufferProposal(pw);
    }

    if (!isLeader()) {
        // A local queue is between this node's stepdown and the conversion of
        // its queues to remote.  Hold the write for 'repostHeldProposals',
        // which re-posts a PUT to the new primary.  This is to avoid NACKing a
        // PUT that the new primary can accept.
        d_raftLog_mp->holdProposal(pw);
        return 0;  // RETURN
    }

    // Hand the write to 'append()', which stamps the record's sequence number
    // (the log index).  'setProposal' stores this same shared_ptr, so
    // 'append()' sets 'pw->d_handle' on the object the caller holds.
    d_raftLog_mp->setProposal(pw);

    RaftNodeOutput output(d_allocator_p);
    int rc = d_raftNode_mp->propose(&output, bsl::shared_ptr<bdlbb::Blob>());
    if (rc != 0) {
        return rc;
    }

    dispatchOutput(&output);

    return 0;
}

void PartitionRaft::proposeDeferredSyncPoint()
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(d_fileStore_sp->inDispatcherThread());

    // 'd_needsBecomeLeaderSyncPoint' is set on each leadership change and
    // cleared here.
    if (!isLeader() || !d_needsBecomeLeaderSyncPoint) {
        return;  // RETURN
    }

    d_needsBecomeLeaderSyncPoint = false;

    BALL_LOG_INFO << "Partition [" << d_partitionId
                  << "] writing deferred become-leader sync point (partition "
                  << "activated; CSL advisory for its leaseId has committed).";

    proposeSyncPoint();

    if (!d_isRolloverPending &&
        d_raftLog_mp->hasUncommittedRollover(d_raftNode_mp->commitIndex())) {
        // The log holds an uncommitted 'e_ROLLOVER' from a prior leader.
        // This current-term sync point's commit also commits that
        // 'e_ROLLOVER', and every node rolls over.  Enter the rollover-pending
        // state now:
        // 1. Writes buffer until the rollover, then drain into the new file
        //    set.
        // 2. No second 'e_ROLLOVER' is proposed.
        // The log is checked directly: 'rolloverIfNeeded' depends on this
        // node's configuration, which can differ from the prior leader's.
        d_isRolloverPending = true;
        BALL_LOG_INFO << "Partition [" << d_partitionId
                      << "] inherited an uncommitted e_ROLLOVER; buffering "
                      << "writes until it commits and rolls over.";
    }
}

void PartitionRaft::proposeShutdownSyncPoint()
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(d_fileStore_sp->inDispatcherThread());

    // Skip before the become-leader sync point: the CSL may not have this
    // leaseId yet.  This is to avoid a sync point that stops a restart into
    // legacy ('e_STORAGE_OUT_OF_SYNC').
    if (!isLeader() || d_needsBecomeLeaderSyncPoint) {
        return;  // RETURN
    }

    BALL_LOG_INFO << "Partition [" << d_partitionId
                  << "] writing final sync point on shutdown.";

    proposeSyncPoint();
}

void PartitionRaft::onPeerNodeStopping(int                      peerId,
                                       const StopContextHolder& stopContext)
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(d_fileStore_sp->inDispatcherThread());

    // Skip before the become-leader sync point, as in
    // 'proposeShutdownSyncPoint'.  Releasing 'stopContext' lets the
    // StopResponse go out.
    if (!isLeader() || d_needsBecomeLeaderSyncPoint) {
        return;  // RETURN
    }

    BALL_LOG_INFO << "Partition [" << d_partitionId
                  << "] writing sync point for stopping node " << peerId;

    const bsls::Types::Uint64 lastIndex = d_raftLog_mp->lastIndex();

    proposeSyncPoint();

    // Not appended: 'propose' failed, or buffered it behind a pending
    // rollover.
    if (d_isRolloverPending || d_raftLog_mp->lastIndex() == lastIndex) {
        return;  // RETURN
    }

    StopResponseWait wait;
    wait.d_peerId     = peerId;
    wait.d_index      = d_raftLog_mp->lastIndex();
    wait.d_whenToStop = bsls::SystemTime::now(
        bsls::SystemClockType::e_MONOTONIC);
    wait.d_whenToStop.addMilliseconds(k_STOP_RESPONSE_WAIT_MS);
    wait.d_stopContext = stopContext;

    d_stopResponseWaits.push_back(wait);

    // Send the sync point now.
    flush();
}

bool PartitionRaft::checkIfCanShutdown()
{
    // executed by the *CLUSTER DISPATCHER* thread

    if (d_canShutdown) {
        return true;  // RETURN
    }

    execute(bdlf::BindUtil::bind(&PartitionRaft::checkIfCanShutdownDispatched,
                                 this));

    return false;
}

void PartitionRaft::checkIfCanShutdownDispatched()
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(d_fileStore_sp->inDispatcherThread());

    const bsls::Types::Uint64 lastIndex = d_raftLog_mp->lastIndex();

    const mqbnet::Cluster::NodesList& nodes =
        d_clusterData_p->membership().netCluster()->nodes();

    for (mqbnet::Cluster::NodesList::const_iterator it = nodes.begin();
         it != nodes.end();
         ++it) {
        mqbnet::ClusterNode* node = *it;

        bsls::Types::Uint64 matchIndex = 0;
        if (!d_raftNode_mp->matchIndex(&matchIndex, node->nodeId())) {
            // Self, or no peer state (this node is not the leader).
            continue;  // CONTINUE
        }

        // A peer with no channel cannot be reached before the channels
        // close, so it does not hold the shutdown back.
        if (node->isAvailable() && matchIndex < lastIndex) {
            d_canShutdown = false;
            return;  // RETURN
        }
    }

    d_canShutdown = true;
}

void PartitionRaft::checkIfCanSendStopResponse()
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(d_fileStore_sp->inDispatcherThread());

    if (d_stopResponseWaits.empty()) {
        return;  // RETURN
    }

    const bsls::TimeInterval now = bsls::SystemTime::now(
        bsls::SystemClockType::e_MONOTONIC);

    bsl::vector<StopResponseWait>::iterator it = d_stopResponseWaits.begin();
    while (it != d_stopResponseWaits.end()) {
        mqbnet::ClusterNode* node       = peerNode(it->d_peerId);
        bsls::Types::Uint64  matchIndex = 0;

        // 'matchIndex' returns false once this node is not the leader.
        const bool isLeader     = d_raftNode_mp->matchIndex(&matchIndex,
                                                        it->d_peerId);
        const bool hasSyncPoint = isLeader && matchIndex >= it->d_index;

        if (!isLeader || hasSyncPoint || !node || !node->isAvailable() ||
            now >= it->d_whenToStop) {
            if (!hasSyncPoint) {
                BALL_LOG_WARN << "Partition [" << d_partitionId
                              << "] sending StopResponse to node "
                              << it->d_peerId << " without its sync point "
                              << "at index " << it->d_index;
            }

            // Destroying the holder releases the stop context.
            it = d_stopResponseWaits.erase(it);
        }
        else {
            ++it;
        }
    }
}

void PartitionRaft::proposeSyncPoint()
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(isLeader());

    // A sync point writes only to the journal.  While a rollover is pending,
    // 'propose' buffers the sync point.
    bsl::shared_ptr<mqbs::Proposal> pw = d_proposalPool.getObject();
    pw->initSyncPoint(mqbs::SyncPointType::e_REGULAR);

    int rc = propose(pw);
    if (rc != 0) {
        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] failed to propose sync point upon becoming "
                       << "leader, rc: " << rc;
    }
}

int PartitionRaft::proposeRollover()
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(isLeader());

    bsl::shared_ptr<mqbs::Proposal> pw = d_proposalPool.getObject();
    pw->initSyncPoint(mqbs::SyncPointType::e_ROLLOVER);
    // 'e_ROLLOVER' goes to 'append()' directly, not through 'propose'.
    d_raftLog_mp->setProposal(pw);

    RaftNodeOutput output(d_allocator_p);
    int rc = d_raftNode_mp->propose(&output, bsl::shared_ptr<bdlbb::Blob>());
    if (rc != 0) {
        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] failed to propose e_ROLLOVER, rc: " << rc;
        return rc;  // RETURN
    }

    // Mark the old file set unavailable.  The order matters:
    // 1. After 'propose': the 'e_ROLLOVER' record must be written, and
    //    'formatSyncPointRecord' rejects any write to an unavailable set.
    // 2. Before 'dispatchOutput': in a single-node cluster 'dispatchOutput'
    //    commits and rolls over at once, and the new file set must stay
    //    available.
    // The flag is defensive: 'propose' buffers every write while
    // 'd_isRolloverPending' is set, so none reaches 'format*Record'.
    d_fileStore_sp->setAvailabilityStatus(false);
    d_isRolloverPending = true;

    dispatchOutput(&output);

    return 0;
}

int PartitionRaft::rolloverIfNeeded(bsls::Types::Uint64 dataBytes,
                                    bsls::Types::Uint64 qlistBytes)
{
    // executed by the partition *DISPATCHER* thread
    enum { rc_SUCCESS = 0, rc_READONLY = -1 };

    if (!isLeader()) {
        return rc_SUCCESS;  // RETURN
    }

    switch (d_fileStore_sp->primaryRolloverNeed(dataBytes, qlistBytes)) {
    case mqbs::FileStore::e_ROLLOVER_NONE: {
        return rc_SUCCESS;  // RETURN
    }
    case mqbs::FileStore::e_ROLLOVER_READONLY: {
        // A rollover cannot reclaim enough space: the partition is full.
        // 'primaryRolloverNeed' has already marked the partition read-only and
        // panicked.  NACK the triggering write.  The partition recovers only
        // through a full purge (see 'onPurgeComplete').
        return rc_READONLY;  // RETURN
    }
    case mqbs::FileStore::e_ROLLOVER_NEEDED:
    default: {
        // A rollover is required and will reclaim enough space.  Propose
        // 'e_ROLLOVER', unless one is already pending.  'propose' then
        // buffers this triggering write, and every later one, until the
        // rollover commits.
        if (!d_isRolloverPending) {
            // A failed proposal is only logged: the triggering write is not
            // NACKed for the failure.
            proposeRollover();
        }
        return rc_SUCCESS;  // RETURN
    }
    }
}

void PartitionRaft::drainProposals()
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(isLeader());

    // Take ownership of the buffered writes from the log, emptying its queue.
    PartitionRaftLog::Proposals toReplay(d_allocator_p);
    d_raftLog_mp->takeBufferedProposals(&toReplay);

    if (toReplay.empty()) {
        return;  // RETURN
    }

    BALL_LOG_INFO << "Partition [" << d_partitionId << "] draining "
                  << toReplay.size() << " buffered write(s) after rollover.";

    // Each write appends into the new file set, unless the write does not
    // fit.  'propose' then proposes the next rollover and buffers the
    // remaining writes.
    rePropose(toReplay);
}

void PartitionRaft::rePropose(const PartitionRaftLog::Proposals& proposals)
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(isLeader());

    // Each of these was set aside by an earlier 'propose' (for a rollover or a
    // leadership transfer), or by a truncation that removed its entry.  Its
    // caller was told it succeeded, so a failure here is answered here.
    for (PartitionRaftLog::Proposals::const_iterator it = proposals.begin();
         it != proposals.end();
         ++it) {
        const mqbs::Proposal& pw = **it;

        const int rc = propose(*it);
        if (rc == 0) {
            continue;  // CONTINUE
        }

        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] failed to propose a " << pw.d_recordType
                       << " record, rc: " << rc;

        d_fileStore_sp->undoPropose(pw);

        if (pw.d_recordType != mqbs::RecordType::e_MESSAGE) {
            continue;  // CONTINUE
        }

        mqbi::QueueHandle* handle = pw.d_attributes.queueHandle();
        mqbi::Queue*       queue  = handle ? handle->queue() : 0;
        if (queue) {
            // NACK, as 'LocalQueue::postMessage' does for a failed 'put'.
            queue->onRemoval(pw.d_guid,
                             handle,
                             mqbi::StorageResult::toAckResult(
                                 mqbi::StorageResult::e_WRITE_FAILURE));
        }
    }
}

void PartitionRaft::execute(const mqbi::Dispatcher::VoidFunction& functor)
{
    // Delegate to the owned FileStore, which dispatches on this partition's
    // dispatcher thread.
    d_fileStore_sp->execute(functor);
}

int PartitionRaft::close(bool flush, bool archive)
{
    // executed by the partition *DISPATCHER* thread

    // A proposal still in the Raft log (e.g. an uncommitted shutdown sync
    // point) holds a blob aliased into the active file set.  Drop the
    // proposals now.  This is to avoid leaving the release to a deferred
    // 'FileStore::gc', which can run after the dispatcher stops.  From index
    // 0: this node is closing, so no entry of its own commits again.
    d_raftLog_mp->dropProposalsFrom(0, false);  // discard

    // Same reason: the cached entry blobs alias the active file set too.
    d_raftLog_mp->clearCache();

    // 'stop' removed the flush callback, so nothing checks these again.
    // Release them so each StopResponse goes out.
    d_stopResponseWaits.clear();

    return d_fileStore_sp->close(flush, archive);
}

int PartitionRaft::rollover()
{
    // executed by the partition *DISPATCHER* thread (admin 'rollover'
    // command).  The rollover goes through the Raft log ('proposeRollover').
    enum {
        rc_SUCCESS          = 0,
        rc_NOT_LEADER       = -1,
        rc_ROLLOVER_PENDING = -2,
        rc_PROPOSE_FAILED   = -3
    };

    if (!isLeader()) {
        // Same wording as legacy 'FileStore::rollover'.
        BALL_LOG_WARN << "Partition [" << d_partitionId
                      << "] Rollover rejected: node is not primary for this "
                      << "partition.";
        return rc_NOT_LEADER;  // RETURN
    }

    if (d_isRolloverPending) {
        BALL_LOG_WARN << "Partition [" << d_partitionId
                      << "] admin rollover rejected: a previous e_ROLLOVER is "
                      << "not yet committed.";
        return rc_ROLLOVER_PENDING;  // RETURN
    }

    if (0 != proposeRollover()) {
        // Return a failure to the caller: with no 'e_ROLLOVER' entry appended,
        // no rollover will happen.
        return rc_PROPOSE_FAILED;  // RETURN
    }

    return rc_SUCCESS;
}

int PartitionRaft::transferLeadership(const bsl::string& targetHostName)
{
    // executed by the partition *DISPATCHER* thread (admin
    // 'transferLeadership' command)
    enum {
        rc_SUCCESS          = 0,
        rc_NOT_LEADER       = -1,
        rc_UNKNOWN_TARGET   = -2,
        rc_REJECTED         = -3,
        rc_ROLLOVER_PENDING = -4
    };

    if (!isLeader()) {
        BALL_LOG_WARN << "Partition [" << d_partitionId
                      << "] Transfer leadership rejected: node is not primary "
                      << "for this partition.";
        return rc_NOT_LEADER;  // RETURN
    }

    const mqbnet::ClusterNode* target =
        mqbnet::ClusterUtil::lookupNodeByHostName(
            d_clusterData_p->membership().netCluster(),
            targetHostName);
    if (!target) {
        BALL_LOG_WARN << "Partition [" << d_partitionId
                      << "] Transfer leadership rejected: unknown node '"
                      << targetHostName << "'.";
        return rc_UNKNOWN_TARGET;  // RETURN
    }

    if (target->nodeId() == d_raftNode_mp->selfId()) {
        // Already in the requested state.
        return rc_SUCCESS;  // RETURN
    }

    if (d_isRolloverPending) {
        // This is to avoid re-posting the writes buffered for the rollover
        // after the PUTs held during the transfer, which arrived later.
        BALL_LOG_WARN << "Partition [" << d_partitionId
                      << "] Transfer leadership rejected: a rollover is "
                      << "pending.";
        return rc_ROLLOVER_PENDING;  // RETURN
    }

    RaftNodeOutput output(d_allocator_p);
    const int      rc = d_raftNode_mp->transferLeadership(&output,
                                                     target->nodeId());
    if (rc != 0) {
        BALL_LOG_WARN << "Partition [" << d_partitionId
                      << "] Transfer leadership to '" << targetHostName
                      << "' rejected, rc: " << rc;
        return rc_REJECTED;  // RETURN
    }

    BALL_LOG_INFO << "Partition [" << d_partitionId
                  << "] initiating leadership transfer to '" << targetHostName
                  << "'";

    dispatchOutput(&output);
    return rc_SUCCESS;
}

void PartitionRaft::appendEntries(
    const bsl::shared_ptr<const bdlbb::Blob>& event_sp,
    mqbnet::ClusterNode*                      source)
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(source);

    const bdlbb::Blob& event = *event_sp;

    if (d_receivingSnapshot) {
        // The FileStore is closed during an InstallSnapshot transfer
        // ('beginReceiveSnapshot'), so drop the event.  The leader learns this
        // node's 'matchIndex' from the InstallSnapshot response and re-sends
        // the entries past that index, as for a dropped packet.
        BALL_LOG_INFO << "Partition [" << d_partitionId
                      << "] ignoring AppendEntries while receiving snapshot";
        return;
    }

    if (!d_fileStore_sp->isOpen()) {
        // At shutdown, 'FileStore::close' runs on this same thread, so a
        // queued AppendEntries can arrive after the close.  Drop the event.
        // This is to avoid applying entries to freed storage.
        BALL_LOG_INFO << "Partition [" << d_partitionId
                      << "] ignoring AppendEntries; FileStore is closed.";
        return;
    }

    bmqu::BlobPosition position;

    if (0 != bmqu::BlobUtil::findOffsetSafe(&position,
                                            event,
                                            sizeof(bmqp::EventHeader))) {
        BALL_LOG_ERROR
            << "Failed to locate RaftHeader in e_RAFT_PARTITION event";
        return;
    }

    bmqu::BlobObjectProxy<bmqp::RaftHeader> rh(&event,
                                               position,
                                               true,    // read
                                               false);  // write

    if (!rh.isSet()) {
        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] failed to read RaftHeader";
        return;
    }

    if (rh->partitionId() != d_partitionId) {
        // Drop the event.  This is to avoid writing another partition's
        // records into this partition's files.
        BMQTSK_ALARMLOG_ALARM("RAFT_MISROUTE")
            << "Partition [" << d_partitionId
            << "] received an AppendEntries addressed to partition "
            << rh->partitionId() << " from node "
            << (source ? source->hostName().c_str() : "unknown")
            << "; dropping it." << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    bmqu::BlobPosition aehPosition;
    if (0 != bmqu::BlobUtil::findOffsetSafe(&aehPosition,
                                            event,
                                            sizeof(bmqp::EventHeader) +
                                                sizeof(bmqp::RaftHeader))) {
        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] failed to locate RaftAppendEntriesHeader";
        return;
    }

    bmqu::BlobObjectProxy<bmqp::RaftAppendEntriesHeader> aeh(&event,
                                                             aehPosition,
                                                             true,    // read
                                                             false);  // write

    if (!aeh.isSet()) {
        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] failed to read RaftAppendEntriesHeader";
        return;
    }

    RaftMessage internalMsg(d_allocator_p);
    internalMsg.d_type         = RaftMessageType::e_APPEND_ENTRIES;
    internalMsg.d_term         = rh->term();
    internalMsg.d_sourceNodeId = source->nodeId();
    internalMsg.d_prevLogIndex = aeh->prevLogIndex();
    internalMsg.d_prevLogTerm  = aeh->prevLogTerm();
    internalMsg.d_leaderCommit = aeh->leaderCommit();

    // 'skip' moves from one record to the next, starting past the headers.
    // This is to avoid rescanning 'event' from its start for each record.
    int skip = sizeof(bmqp::EventHeader) + sizeof(bmqp::RaftHeader) +
               sizeof(bmqp::RaftAppendEntriesHeader);
    int          remaining  = event.length() - skip;
    unsigned int entryCount = aeh->entryCount();

    bmqu::BlobPosition recPos;  // start of 'event'

    for (unsigned int i = 0; i < entryCount && remaining >= k_JREC_SIZE; ++i) {
        bmqu::BlobPosition nextPos;
        if (0 !=
            bmqu::BlobUtil::findOffsetSafe(&nextPos, event, recPos, skip)) {
            break;
        }
        recPos = nextPos;

        int entrySize = computeEntrySize(event, recPos);
        if (entrySize <= 0 || entrySize > remaining) {
            BALL_LOG_ERROR << "Partition [" << d_partitionId
                           << "] bad entry size " << entrySize << " for entry "
                           << i;
            break;
        }

        bmqu::BlobObjectProxy<mqbs::RecordHeader> recHeader(&event,
                                                            recPos,
                                                            true,
                                                            false);
        if (!recHeader.isSet()) {
            break;
        }

        bsl::shared_ptr<bdlbb::Blob> entryBlob =
            d_clusterData_p->blobSpPool().getObject();
        bmqu::BlobUtil::appendToBlob(entryBlob.get(),
                                     event,
                                     recPos,
                                     entrySize);

        internalMsg.d_entries.push_back(
            LogEntry(recHeader->primaryLeaseId(),
                     internalMsg.d_prevLogIndex + 1 + i,
                     entryBlob));

        skip = entrySize;
        remaining -= entrySize;
    }

    RaftNodeOutput output(d_allocator_p);
    d_raftNode_mp->step(&output, internalMsg);

    dispatchOutput(&output);
}

void PartitionRaft::onAppendEntriesResponse(
    const bsl::shared_ptr<const bdlbb::Blob>& event_sp,
    mqbnet::ClusterNode*                      source)
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(source);

    const bdlbb::Blob& event = *event_sp;

    if (!d_fileStore_sp->isOpen()) {
        // Shutdown/teardown, as in 'appendEntries' above: the partition can
        // close before a queued event is drained.
        BALL_LOG_INFO << "Partition [" << d_partitionId
                      << "] ignoring AppendEntries response; FileStore is "
                      << "closed.";
        return;  // RETURN
    }

    bmqu::BlobPosition position;

    if (0 != bmqu::BlobUtil::findOffsetSafe(&position,
                                            event,
                                            sizeof(bmqp::EventHeader))) {
        BALL_LOG_ERROR
            << "Failed to locate RaftHeader in e_RAFT_PARTITION event";
        return;  // RETURN
    }

    bmqu::BlobObjectProxy<bmqp::RaftHeader> rh(&event,
                                               position,
                                               true,    // read
                                               false);  // write

    if (!rh.isSet()) {
        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] failed to read RaftHeader";
        return;  // RETURN
    }

    if (rh->partitionId() != d_partitionId) {
        // Drop the response.  This is to avoid advancing this partition's
        // peer state with another partition's response.
        BMQTSK_ALARMLOG_ALARM("RAFT_MISROUTE")
            << "Partition [" << d_partitionId
            << "] received an AppendEntries response addressed to partition "
            << rh->partitionId() << " from node " << source->hostName()
            << "; dropping it." << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    bmqu::BlobPosition respPosition;
    if (0 != bmqu::BlobUtil::findOffsetSafe(&respPosition,
                                            event,
                                            sizeof(bmqp::EventHeader) +
                                                sizeof(bmqp::RaftHeader))) {
        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] failed to locate RaftResponseHeader";
        return;  // RETURN
    }

    bmqu::BlobObjectProxy<bmqp::RaftResponseHeader> resp(&event,
                                                         respPosition,
                                                         true,    // read
                                                         false);  // write

    if (!resp.isSet()) {
        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] failed to read RaftResponseHeader";
        return;  // RETURN
    }

    RaftMessage internalMsg(d_allocator_p);
    internalMsg.d_type          = RaftMessageType::e_APPEND_ENTRIES_RESP;
    internalMsg.d_term          = rh->term();
    internalMsg.d_sourceNodeId  = source->nodeId();
    internalMsg.d_success       = resp->success();
    internalMsg.d_matchIndex    = resp->matchIndex();
    internalMsg.d_rejectedIndex = resp->rejectedIndex();

    RaftNodeOutput output(d_allocator_p);
    d_raftNode_mp->step(&output, internalMsg);

    dispatchOutput(&output);
}

void PartitionRaft::onRaftControlMessage(
    const bmqp_ctrlmsg::RaftMessage& message,
    mqbnet::ClusterNode*             source)
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(source);

    if (!d_fileStore_sp->isOpen()) {
        // As in 'appendEntries': the FileStore is closed during an
        // InstallSnapshot transfer and after 'FileStore::close' at shutdown.
        // Every message handled below reads the log (e.g. 'handleRequestVote'
        // reads 'lastTerm()'), and a closed store has no file set.  Drop the
        // message.  The sender retries, as for a dropped packet.
        BALL_LOG_INFO << "Partition [" << d_partitionId
                      << "] ignoring Raft control message; FileStore is "
                      << "closed.";
        return;  // RETURN
    }

    RaftMessage internalMsg(d_allocator_p);
    fromCtrlMsg(&internalMsg, message, source->nodeId());

    RaftNodeOutput output(d_allocator_p);
    d_raftNode_mp->step(&output, internalMsg);

    if (output.d_hasInstallSnapshot) {
        const RaftMessage& snap = output.d_installSnapshot;
        beginReceiveSnapshot(snap.d_lastLogIndex, snap.d_lastLogTerm);
    }

    dispatchOutput(&output);
}

// RecordStore OVERRIDES
int PartitionRaft::writeMessageRecord(
    mqbi::StorageMessageAttributes*     attributes,
    mqbs::DataStoreRecordHandle*        handle,
    const bmqt::MessageGUID&            guid,
    const bsl::shared_ptr<bdlbb::Blob>& appData,
    const bsl::shared_ptr<bdlbb::Blob>& options,
    const mqbu::StorageKey&             queueKey)
{
    BSLS_ASSERT_SAFE(attributes);
    BSLS_ASSERT_SAFE(handle);
    BSLS_ASSERT_SAFE(appData);

    bsl::shared_ptr<mqbs::Proposal> pw = d_proposalPool.getObject();
    pw->initMessage(attributes, guid, appData, options, queueKey);

    int rc = propose(pw);
    if (rc != 0) {
        return rc;
    }
    *handle = pw->d_handle;
    return 0;
}

int PartitionRaft::writeConfirmRecord(mqbs::DataStoreRecordHandle* handle,
                                      const bmqt::MessageGUID&     guid,
                                      const mqbu::StorageKey&      queueKey,
                                      const mqbu::StorageKey&      appKey,
                                      bsls::Types::Uint64          timestamp,
                                      mqbs::ConfirmReason::Enum    reason)
{
    BSLS_ASSERT_SAFE(handle);

    bsl::shared_ptr<mqbs::Proposal> pw = d_proposalPool.getObject();
    pw->initConfirm(guid, queueKey, appKey, timestamp, reason);

    int rc = propose(pw);
    if (rc != 0) {
        return rc;
    }
    *handle = pw->d_handle;
    return 0;
}

int PartitionRaft::writeDeletionRecord(
    const bmqt::MessageGUID&       guid,
    const mqbu::StorageKey&        queueKey,
    mqbs::DeletionRecordFlag::Enum deletionFlag,
    bsls::Types::Uint64            timestamp)
{
    bsl::shared_ptr<mqbs::Proposal> pw = d_proposalPool.getObject();
    pw->initDeletion(guid, queueKey, deletionFlag, timestamp);

    if (d_isDispatchingOutput) {
        // A per-app purge applying at commit writes a DELETION for each
        // message whose refCount it drops to zero.  This DELETION goes to
        // 'd_deferred', and 'postDispatch' proposes it.
        d_deferred.push_back(pw);
        return 0;  // RETURN
    }

    return propose(pw);
}

int PartitionRaft::writeQueuePurgeRecord(
    mqbs::DataStoreRecordHandle*       handle,
    const mqbu::StorageKey&            queueKey,
    const mqbu::StorageKey&            appKey,
    bsls::Types::Uint64                timestamp,
    const mqbs::DataStoreRecordHandle& start)
{
    BSLS_ASSERT_SAFE(handle);

    unsigned int        startLeaseId = 0;
    bsls::Types::Uint64 startSeqNo   = 0;
    if (!appKey.isNull()) {
        BSLS_ASSERT_SAFE(start.isValid());
        startLeaseId = start.primaryLeaseId();
        startSeqNo   = start.sequenceNum();
    }

    bsl::shared_ptr<mqbs::Proposal> pw = d_proposalPool.getObject();
    pw->initQueueOp(mqbs::QueueOpType::e_PURGE,
                    queueKey,
                    appKey,
                    timestamp,
                    startLeaseId,
                    startSeqNo);

    int rc = propose(pw);
    if (rc != 0) {
        return rc;
    }
    *handle = pw->d_handle;
    return 0;
}

int PartitionRaft::writeQueueDeletionRecord(
    mqbs::DataStoreRecordHandle* handle,
    const mqbu::StorageKey&      queueKey,
    const mqbu::StorageKey&      appKey,
    bsls::Types::Uint64          timestamp)
{
    BSLS_ASSERT_SAFE(handle);

    bsl::shared_ptr<mqbs::Proposal> pw = d_proposalPool.getObject();
    pw->initQueueOp(mqbs::QueueOpType::e_DELETION,
                    queueKey,
                    appKey,
                    timestamp,
                    0,   // startPrimaryLeaseId
                    0);  // startSequenceNumber

    int rc = propose(pw);
    if (rc != 0) {
        return rc;
    }
    *handle = pw->d_handle;
    return 0;
}

int PartitionRaft::writeQueueCreationRecord(
    mqbs::DataStoreRecordHandle* handle,
    const bmqt::Uri&             queueUri,
    const mqbu::StorageKey&      queueKey,
    const AppInfos&              appIdKeyPairs,
    bsls::Types::Uint64          timestamp,
    bool                         isNewQueue)
{
    BSLS_ASSERT_SAFE(handle);

    bsl::shared_ptr<mqbs::Proposal> pw = d_proposalPool.getObject();
    pw->initQueueCreation(queueUri,
                          queueKey,
                          appIdKeyPairs,
                          timestamp,
                          isNewQueue);

    int rc = propose(pw);
    if (rc != 0) {
        return rc;
    }
    *handle = pw->d_handle;
    return 0;
}

void PartitionRaft::registerStorage(mqbs::ReplicatedStorage* storage)
{
    d_fileStore_sp->registerStorage(storage);
}

void PartitionRaft::unregisterStorage(const mqbs::ReplicatedStorage* storage)
{
    d_fileStore_sp->unregisterStorage(storage);
}

mqbs::StorageMonitor* PartitionRaft::storageMonitor()
{
    return d_storageMonitor_p;
}

const mqbs::DataStoreConfig::Records& PartitionRaft::records() const
{
    return d_fileStore_sp->records();
}

bsls::Types::Uint64 PartitionRaft::numRecords() const
{
    return d_fileStore_sp->numRecords();
}

void PartitionRaft::loadMessageRecord(
    mqbs::MessageRecord*                                  buffer,
    const mqbs::DataStoreConfig::Records::const_iterator& it) const
{
    d_fileStore_sp->loadMessageRecord(buffer, it);
}

void PartitionRaft::loadConfirmRecord(
    mqbs::ConfirmRecord*                                  buffer,
    const mqbs::DataStoreConfig::Records::const_iterator& it) const
{
    d_fileStore_sp->loadConfirmRecord(buffer, it);
}

void PartitionRaft::loadQueueOpRecord(
    mqbs::QueueOpRecord*                                  buffer,
    const mqbs::DataStoreConfig::Records::const_iterator& it) const
{
    d_fileStore_sp->loadQueueOpRecord(buffer, it);
}

void PartitionRaft::recordIteratorToHandle(
    mqbs::DataStoreRecordHandle*                          handle,
    const mqbs::DataStoreConfig::Records::const_iterator& it) const
{
    d_fileStore_sp->recordIteratorToHandle(handle, it);
}

void PartitionRaft::createStorage(
    bsl::shared_ptr<mqbs::ReplicatedStorage>* storageSp,
    const bmqt::Uri&                          queueUri,
    const mqbu::StorageKey&                   queueKey,
    mqbi::Domain*                             domain)
{
    BSLS_ASSERT_SAFE(storageSp);
    BSLS_ASSERT_SAFE(domain);

    bsl::shared_ptr<const mqbconfm::Domain> domainCfg  = domain->config();
    const mqbconfm::StorageDefinition&      storageDef = domainCfg->storage();
    const mqbconfm::Storage&                storageCfg = storageDef.config();

    BSLS_ASSERT_SAFE(!storageCfg.isUndefinedValue());

    if (storageCfg.isInMemoryValue()) {
        storageSp->reset(new (*d_allocator_p)
                             mqbs::InMemoryStorage(this,
                                                   queueUri,
                                                   queueKey,
                                                   domain,
                                                   d_partitionId,
                                                   *domainCfg,
                                                   domain->capacityMeter(),
                                                   d_allocator_p),
                         d_allocator_p);
    }
    else {
        BSLS_ASSERT_SAFE(storageCfg.isFileBackedValue());
        storageSp->reset(new (*d_allocator_p)
                             mqbs::FileBackedStorage(this,
                                                     queueUri,
                                                     queueKey,
                                                     domain,
                                                     *domainCfg,
                                                     d_allocator_p),
                         d_allocator_p);
    }
}

void PartitionRaft::removeRecordRaw(const mqbs::DataStoreRecordHandle& handle)
{
    // Every handle that reaches here came from the storage, which is
    // populated on commit, so it names a record no proposal still owns.
    d_fileStore_sp->removeRecordRaw(handle);
}

void PartitionRaft::setAvailabilityStatus(bool enable)
{
    d_fileStore_sp->setAvailabilityStatus(enable);
}

void PartitionRaft::setReplicationFactor(int factor)
{
    d_fileStore_sp->setReplicationFactor(factor);
}

void PartitionRaft::onPurgeComplete()
{
    // executed by the partition *DISPATCHER* thread

    // Reclaim the space the purge freed with a Raft rollover
    // ('rolloverIfNeeded').  Only the leader proposes, and only if the file
    // set needs a rollover.  Replicas roll over when the 'e_ROLLOVER'
    // commits.
    //
    // After a full purge of a read-only partition, the journal is still
    // marked unavailable.  Make the journal writable again, so the
    // 'e_ROLLOVER' record can be written ('writeSyncPointRecord' rejects an
    // unavailable journal).  'rolloverIfNeeded' marks the journal read-only
    // again if the freed space is still not enough.
    if (isLeader() && !d_fileStore_sp->isFileSetAvailable()) {
        d_fileStore_sp->setAvailabilityStatus(true);
    }

    rolloverIfNeeded(0, 0);
}

void PartitionRaft::flushStorage()
{
    d_fileStore_sp->flushStorage();
    flush();
}

void PartitionRaft::setLastStrongConsistency(unsigned int primaryLeaseId,
                                             bsls::Types::Uint64 sequenceNum)
{
    // No-op for Raft partitions; consistency is managed by Raft protocol
    (void)primaryLeaseId;
    (void)sequenceNum;
}

void PartitionRaft::loadSummary(mqbcmd::FileStore* summary) const
{
    d_fileStore_sp->loadSummary(summary);
}

void PartitionRaft::getStorages(
    mqbs::RecordStore::StorageList*          storages,
    const mqbs::RecordStore::StorageFilters& filters) const
{
    d_fileStore_sp->getStorages(storages, filters);
}

void PartitionRaft::loadMessageRaw(
    bsl::shared_ptr<bdlbb::Blob>*      appData,
    bsl::shared_ptr<bdlbb::Blob>*      options,
    mqbi::StorageMessageAttributes*    attributes,
    const mqbs::DataStoreRecordHandle& handle) const
{
    d_fileStore_sp->loadMessageRaw(appData, options, attributes, handle);
}

void PartitionRaft::loadMessageAttributesRaw(
    mqbi::StorageMessageAttributes*    buffer,
    const mqbs::DataStoreRecordHandle& handle) const
{
    d_fileStore_sp->loadMessageAttributesRaw(buffer, handle);
}

void PartitionRaft::loadQueueOpRecordRaw(
    mqbs::QueueOpRecord*               buffer,
    const mqbs::DataStoreRecordHandle& handle) const
{
    d_fileStore_sp->loadQueueOpRecordRaw(buffer, handle);
}

unsigned int PartitionRaft::getMessageLenRaw(
    const mqbs::DataStoreRecordHandle& handle) const
{
    return d_fileStore_sp->getMessageLenRaw(handle);
}

unsigned int PartitionRaft::writeHeadLeaseId() const
{
    return static_cast<unsigned int>(d_raftNode_mp->currentTerm());
}

bsls::Types::Uint64 PartitionRaft::writeHeadSeqNum() const
{
    return d_raftLog_mp->proposalHeadIndex();
}

bool PartitionRaft::hasApplied(bsls::Types::Uint64 sequenceNumber) const
{
    return d_raftNode_mp->lastAppliedCommit() >= sequenceNumber;
}

bool PartitionRaft::hasReceipt(const mqbs::DataStoreRecordHandle& handle) const
{
    return d_fileStore_sp->hasReceipt(handle);
}

bool PartitionRaft::isFileSetAvailable() const
{
    return d_fileStore_sp->isFileSetAvailable();
}

bsl::string_view PartitionRaft::description() const
{
    return d_fileStore_sp->description();
}

// ACCESSORS
bool PartitionRaft::isLeader() const
{
    return d_raftNode_mp->state() == RaftState::e_LEADER;
}

bool PartitionRaft::isTransferringLeadership() const
{
    return d_raftNode_mp->isTransferringLeadership();
}

int PartitionRaft::leaderId() const
{
    return d_raftNode_mp->leaderId();
}

bool PartitionRaft::isPendingReplication(mqbi::Storage::DeliveryProbe* probe,
                                         BSLA_MAYBE_UNUSED bool isAbsent) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(probe);

    if (isLeader()) {
        // The leader holds every committed record, so nothing is on its way:
        // one that is absent is one that was removed.
        return false;  // RETURN
    }

    const bsls::Types::Uint64 leaderCommit =
        d_raftNode_mp->lastKnownLeaderCommit();

    if (0 == leaderCommit) {
        // Nothing heard from a leader yet, so there is no bound to wait on.
        return true;  // RETURN
    }

    const bsls::Types::Uint64 generation =
        d_raftNode_mp->leaderCommitGeneration();

    if (0 == probe->d_u0 || generation != probe->d_u1) {
        // 'probe->d_u0' holds the 'leaderCommit' recorded for this message,
        // and 'probe->d_u1' holds the 'leaderCommitGeneration' at that time.
        // On the first call for this message, or after the generation changed
        // (a new leader, the channel to the leader came back, a rejection, or
        // a snapshot):
        // 1. Raise 'd_u0' to 'leaderCommit', and store the generation.
        // 2. Return true: the leader sends the PUSH only after the record
        //    commits, but 'leaderCommit' here may predate that commit.
        // A later 'leaderCommit' above 'd_u0' in the same generation was sent
        // after the PUSH, so it is at or above the record's index.  That needs
        // 'leaderCommit' to be the leader's whole commit index (see
        // 'd_leaderCommit' in 'RaftNode::sendAppendEntries').
        //
        // 'd_u0' is never lowered: a new leader's commit index starts below
        // what is already committed, and a small rise from it can stay below
        // the record's index.
        if (leaderCommit > probe->d_u0) {
            probe->d_u0 = leaderCommit;
        }
        probe->d_u1 = generation;
        return true;  // RETURN
    }

    if (leaderCommit <= probe->d_u0) {
        // Nothing newer heard from the leader yet.
        return true;  // RETURN
    }

    // Applying through a position that covers the record settles it: had it
    // ever been written here it would be in the storage, so it is either gone
    // or was never committed.
    const bsls::Types::Uint64 applied = d_raftNode_mp->lastAppliedCommit();

    if (applied < leaderCommit) {
        return true;  // RETURN
    }

    // Also require the applied watermark to reach the log's end.  After an
    // InstallSnapshot, the watermark is at the snapshot boundary while the
    // entries recovered above that boundary are still unapplied, and the
    // record may be among them.
    return applied < d_raftLog_mp->lastIndex();
}

bsls::Types::Uint64 PartitionRaft::currentTerm() const
{
    return d_raftNode_mp->currentTerm();
}

}  // close package namespace
}  // close enterprise namespace
