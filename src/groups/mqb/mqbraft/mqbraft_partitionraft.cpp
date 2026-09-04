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
#include <bmqu_time.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blobutil.h>
#include <bdlf_bind.h>
#include <bsl_vector.h>
#include <bslmf_movableref.h>
#include <bsls_assert.h>

// SYSTEM
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace BloombergLP {
namespace mqbraft {

namespace {

const int k_TICK_INTERVAL_MS = 100;
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

    // 'd_peers' is the *full* membership including self:
    // 'RaftNode::quorum()' is 'peers.size()/2 + 1' (majority of the whole
    // cluster for both odd and even sizes) and
    // 'becomeCandidate'/'becomeLeader' skip self while iterating.
    // 'netCluster->nodes()' already includes self.
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

/// Every lookup here is relative to the specified `pos`, the record's own
/// position: resolving from the start of `blob` instead would rescan the
/// buffer list from the beginning, making a walk over the entries of one
/// event quadratic in their count.
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
        // A QUEUE_OP carries an inline qlist payload only when its journal
        // record references one -- i.e. 'queueUriRecordOffsetWords() > 0'
        // (CREATION/ADDITION when qlist-aware).  PURGE/DELETION ops, and any
        // op when not qlist-aware, have none.  This mirrors
        // 'FileStore::readRecord', which appends the qlist under the same
        // condition.  Without this check a qlist-less QUEUE_OP would blindly
        // read the *next* entry's journal record as a QueueRecordHeader and
        // compute a bogus (over-large) size.
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
PartitionRaft::PartitionRaft(int partitionId,
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
, d_pendingWritePool(1024, bslma::Default::allocator(allocator))
, d_isStarted(false)
, d_allocator_p(bslma::Default::allocator(allocator))
, d_receivingSnapshot(false)
, d_snapshotJournalFd(-1)
, d_snapshotDataFd(-1)
, d_snapshotQlistFd(-1)
, d_snapshotJournalPath(d_allocator_p)
, d_snapshotDataPath(d_allocator_p)
, d_snapshotQlistPath(d_allocator_p)
, d_snapshotLastIncludedIndex(0)
, d_snapshotLastIncludedTerm(0)
, d_isDispatchingOutput(false)
, d_deferred(d_allocator_p)
, d_isRolloverPending(false)
, d_isExpectingTermCommit(false)
, d_needsBecomeLeaderSyncPoint(false)
, d_leadershipCb(leadershipCb)
, d_canShutdown(false)
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
    // executed by the partition *DISPATCHER* thread, from 'FileStore::flush()'
    // once the dispatcher has drained this partition's queue.  Every
    // AppendEntries this leader owes is produced here, so a batch of proposals
    // and responses yields one round rather than one send apiece.
    BSLS_ASSERT_SAFE(d_fileStore_sp->inDispatcherThread());

    if (!d_isStarted) {
        return;  // RETURN
    }

    RaftNodeOutput output(d_allocator_p);
    d_raftNode_mp->flushSends(&output);
    dispatchMessages(&output);
}

void PartitionRaft::dispatchMessages(RaftNodeOutput* output)
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(output);

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

    // A node that is not the leader keeps no write the log will ever carry:
    // the buffered ones await a rollover that will not drain, and a
    // truncation has erased the entries of any appended above the log end
    // (that is where 'truncateFrom' leaves them; it releases only their
    // records, which it must do before 'truncateRecords').  Their producers
    // are still attached, so they are kept to be re-posted rather than
    // discarded.  Before applying committed entries below: a placeholder
    // belongs to no log entry, and the apply loop's deletion path would walk
    // it.  The appended writes below the end stay -- their entries can still
    // commit under the new leader, and 'applyCommittedEntry' needs them to
    // route those commits through the primary path.
    //
    // Not under 'd_lostLeadership': the new leader probes backwards, so the
    // conflicting AppendEntries usually arrives well after the one that
    // stepped this node down.
    if (!isLeader()) {
        d_raftLog_mp->dropWritesFrom(d_raftLog_mp->lastIndex() + 1,
                                     &d_writesToRepost);
    }

    if (output->d_lostLeadership) {
        d_isRolloverPending = false;

        // This term's sync point will not commit under this node; leaving the
        // expectation set would fire the 'haveCommit' callback for the next
        // term this node leads, on whatever entry commits first.
        d_isExpectingTermCommit = false;

        // The queues stay local for now.  Converting them is the cluster's
        // call, because the handles the peers opened here have to go first
        // and only it can take those out of their sessions; it drives both
        // from the leadership change this signals below.  Writes arriving
        // meanwhile are held by 'propose' rather than rejected, and the
        // conversion hands them on.
        //
        // The uncommitted tail needs no replay.  A committed entry survives
        // into the new leader's log, and an SC producer is ACKed only on
        // commit, so the tail holds nothing a producer was told was accepted
        // -- except under EC, which ACKs at propose by definition.
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
            // 'rollover()' also removes the committed 'e_ROLLOVER' from the
            // pending-write buffer on the primary (it is never popped via
            // 'applyCommittedEntryAsPrimary'), keeping 'drainPendingWrites'
            // from replaying it into the new file set.
            d_raftLog_mp->rollover(entry.d_index);
            hadRollover = true;
        }
        else {
            applyCommittedEntry(entry, commitTimepoint);
        }
    }

    d_isDispatchingOutput = false;

    postDispatch(output, hadRollover);

    // Note on leadership changes: becoming leader needs no action here.  The
    // become-leader sync point is NOT written here -- it is the first journal
    // record under the new leaseId (== term), and strict ordering requires the
    // CSL's artificial 'partitionPrimaryAdvisory' (carrying this leaseId) to
    // commit first.  See 'proposeDeferredSyncPoint' called by the orchestrator
    // once the advisory commits and the partition reaches E_ACTIVE.

    // On any leadership change record the primary identity on the FileStore
    // and signal the cluster.  'd_stateChanged' covers self becoming/losing
    // leader; 'd_leaderChanged' additionally covers a follower observing a new
    // leader identity.
    if (output->d_stateChanged || output->d_leaderChanged) {
        const int                 leaderNodeId = d_raftNode_mp->leaderId();
        const bsls::Types::Uint64 term         = d_raftNode_mp->currentTerm();

        // Record the primary identity on the FileStore so 'fs->primaryNode()',
        // 'primaryLeaseId()' and 'd_isPrimary' reflect Raft leadership (the
        // primary sets self, replicas set the remote leader).  This runs on
        // this partition's dispatcher thread, as 'setActivePrimary' requires,
        // so no dispatch is needed.  'isRaft=true' keeps only the identity
        // bookkeeping and skips the legacy sync-point machinery (Raft drives
        // sync points through its own log).
        if (RaftNode::k_INVALID_NODE_ID != leaderNodeId) {
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

        // Arm the become-leader sync point for this leadership.  It cannot be
        // inferred from 'lastTerm() == currentTerm()': the leader commits and
        // applies its recovered log on becoming leader, and those applies
        // propose queue records of their own, so a current-term entry can
        // exist without this sync point ever having been written.
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

    // After the conversion, so each write reaches a queue that can relay it,
    // and after the cluster's handle drops, which it scheduled ahead of this
    // on the same thread: a write from a peer whose handle is now gone is
    // that peer's to re-send.
    repostHeldWrites();
}

void PartitionRaft::repostHeldWrites()
{
    // executed by the partition *DISPATCHER* thread

    if (d_writesToRepost.empty()) {
        return;  // RETURN
    }

    bsl::vector<bsl::shared_ptr<mqbs::FileStore::PendingWrite> > held(
        d_allocator_p);
    held.swap(d_writesToRepost);

    size_t numReposted = 0;
    size_t numKept     = 0;

    for (size_t i = 0; i < held.size(); ++i) {
        const mqbs::FileStore::PendingWrite& pw = *held[i];

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
            d_writesToRepost.push_back(held[i]);
            ++numKept;
            continue;  // CONTINUE
        }

        // The message never entered this node's storage, so the capacity
        // 'put' set aside for it goes back whether or not it is re-posted.
        d_fileStore_sp->undoPropose(pw);

        if (pw.d_recordType != mqbs::RecordType::e_MESSAGE) {
            continue;  // CONTINUE
        }

        // Gone means either a cluster peer whose handle the demotion dropped,
        // or a client that has since disconnected.  Either way there is no
        // producer left here to answer, and a peer re-sends to the new
        // primary itself.
        if (!queue || !queue->hasHandle(handle)) {
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

        // Not 'QueueHandle::postMessage': that one hops from the producer's
        // dispatcher thread onto the queue's, and asserts it is on the
        // former.  This runs on the partition's dispatcher thread, which is
        // the queue's, so hand the message straight to the queue.
        queue->postMessage(header, pw.d_appData, pw.d_options, handle);
        ++numReposted;
    }

    if (numKept == held.size()) {
        // Nothing changed: every one of them is waiting on the conversion.
        return;  // RETURN
    }

    BALL_LOG_INFO << "Partition [" << d_partitionId << "] re-posted "
                  << numReposted << " of " << held.size()
                  << " write(s) held while not the leader; " << numKept
                  << " still awaiting the conversion, the rest had no "
                  << "producer left to answer.";
}

void PartitionRaft::postDispatch(RaftNodeOutput* output, bool hadRollover)
{
    // executed by the partition *DISPATCHER* thread

    // Everything here runs after the apply loop of 'dispatchOutput', never
    // inside it, because it proposes: a propose from within the loop re-enters
    // 'dispatchOutput' on a frame that is still walking 'd_committed'.

    // Writes the apply loop held back for that reason.  Swap first: 'propose'
    // runs a nested 'dispatchOutput', so this must not be re-entered on the
    // same entries.
    if (!d_deferred.empty()) {
        bsl::vector<bsl::shared_ptr<mqbs::FileStore::PendingWrite> > deferred(
            d_allocator_p);
        deferred.swap(d_deferred);

        for (size_t i = 0; i < deferred.size(); ++i) {
            const int rc = propose(deferred[i]);
            if (rc != 0) {
                BALL_LOG_ERROR << "Partition [" << d_partitionId
                               << "] failed to propose a deferred "
                               << deferred[i]->d_recordType
                               << " record, rc: " << rc;
            }
        }
    }

    // Resolve buffered writes once the rollover outcome is known.  Buffered
    // writes exist only during/after a rollover window, so during normal
    // writes the buffer is empty and this does not fire.
    if (hadRollover && isLeader()) {
        BSLS_ASSERT_SAFE(d_isRolloverPending);

        // The 'e_ROLLOVER' just committed and rolled over in the apply loop.
        // Clear the in-flight flag before the drain, not after:
        // 'drainPendingWrites' runs a nested 'dispatchOutput', which must not
        // see a rollover still in flight.  The drain loop itself does not read
        // the flag -- it calls 'setPendingWrite' and 'RaftNode::propose'
        // directly rather than going through 'PartitionRaft::propose'.
        d_isRolloverPending = false;

        drainPendingWrites();
    }

    // Writes a truncation took off the log on the way in ('truncateFrom' runs
    // inside 'handleAppendEntries', ahead of this).  Here rather than there
    // because re-posting proposes, and the conversion may already have run,
    // in which case nothing else would come back for them.
    if (!isLeader()) {
        repostHeldWrites();
    }

    // A committed whole-queue purge freed journal space; reclaim it now.
    // 'onPurgeComplete' proposes an 'e_ROLLOVER'.
    if (d_fileStore_sp->takePurgeCompleted()) {
        onPurgeComplete();
    }

    // The gather stopped at its cap; nothing else brings us back, since
    // 'commitTo' only runs when the commit index advances.  Re-post rather
    // than loop, so the dispatcher gets a turn between batches.
    if (output->d_hasMoreToApply) {
        execute(
            bdlf::BindUtil::bind(&PartitionRaft::applyCommittedBatchDispatched,
                                 this));
    }

    // Re-drive delivery once for the whole batch applied above.  Each
    // committed message noted its queue rather than notifying it, so a queue
    // that took several entries is walked once, and one deleted later in the
    // same batch is not walked at all.
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
        .setPartitionId(static_cast<unsigned int>(d_partitionId))
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

    // One event, every peer at this anchor.  Channels copy the blob's buffer
    // references on write and never modify it, so it is shared, not rebuilt.
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
        .setPartitionId(static_cast<unsigned int>(d_partitionId))
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

    // Use this partition's own 'FileStore::sendMessage' (backed by its
    // per-partition 'ControlMessageTransmitter'), NOT
    // 'd_clusterData_p->messageTransmitter()': that shared, cluster-level
    // transmitter's 'SchemaEventBuilder' is documented as usable only from the
    // cluster dispatcher thread, but this method runs on the partition
    // dispatcher thread.  With multiple partitions (each own thread) this
    // would race on the transmitter's shared blob-building state.
    d_fileStore_sp->sendMessage(controlMsg, destNode);
}

void PartitionRaft::applyCommittedEntry(const LogEntry&    entry,
                                        bsls::Types::Int64 commitTimepoint)
{
    // executed by the partition *DISPATCHER* thread

    // Entries this node appended through the primary write path keep their
    // pending write until they commit, and that -- not current leadership --
    // is what says the storage side is already done for them.  An entry
    // proposed as leader can commit after the term has moved on; applying it
    // as a replica would redo work propose time did, up to re-inserting a
    // handle to a record already erased.
    if (d_raftLog_mp->isOwnAppendedEntry(entry.d_index)) {
        d_raftLog_mp->applyCommittedEntryAsPrimary(entry.d_index,
                                                   commitTimepoint);

        if (isLeader() && d_isExpectingTermCommit) {
            d_isExpectingTermCommit = false;
            d_leadershipCb(d_partitionId,
                           d_raftNode_mp->leaderId(),
                           d_raftNode_mp->currentTerm(),
                           true);
        }
    }
    else {
        d_raftLog_mp->applyCommittedEntryAsReplica(entry.d_index,
                                                   *entry.d_data);
    }
}

void PartitionRaft::applyEntriesAsReplica(const RaftMessage&  msg,
                                          bsls::Types::Uint64 prevLastIndex,
                                          bsls::Types::Uint64 newLastIndex)
{
    // No-op.  Entries are written to journal during append() in
    // PartitionRaftLog.  Storage is populated on commit via
    // applyCommittedEntry → applyCommittedEntryAsReplica.
    (void)msg;
    (void)prevLastIndex;
    (void)newLastIndex;
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

    // The InstallSnapshot control message that puts the follower into
    // receiving mode is sent by the caller ('dispatchOutput'), immediately
    // before this.  Sending a second one here made the follower handle the
    // same message twice and answer twice.

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
                .setPartitionId(static_cast<unsigned int>(d_partitionId))
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
                // The channel went away mid-transfer.  Report it: 'RaftNode'
                // has this snapshot marked in flight, which suppresses
                // replication to the peer until it times out.
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

    // The done flag cannot ride on the last chunk of a file: the receiver
    // finalizes on it, and a file of size 0 emits no chunk to carry it.
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
            .setPartitionId(static_cast<unsigned int>(d_partitionId))
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
        d_storageMonitor_p->onStoragesCleared(d_partitionId);

        // Get file paths before wiping
        mqbs::FileStoreSet fileSet;
        d_fileStore_sp->loadCurrentFiles(&fileSet);
        d_snapshotJournalPath = fileSet.journalFile();
        d_snapshotDataPath    = fileSet.dataFile();
        d_snapshotQlistPath   = fileSet.qlistFile();

        // 'onStoragesCleared' above destroyed the partition's storage objects
        // (the monitor held the owning shared_ptrs).  The FileStore still
        // holds raw pointers to them in 'd_storages', now dangling; drop them
        // so no subsequent lookup (e.g. a committed-record apply before the
        // reopen re-registers fresh storages) touches freed memory.
        d_fileStore_sp->clearStorages();

        // This path closes the 'FileStore' directly rather than through
        // 'PartitionRaft::close', so it has to release what aliases the file
        // set itself: the cached entry blobs, and the pending writes a leader
        // stint left behind (stepdown keeps the appended ones, and only a
        // truncation drops them -- an InstallSnapshot never truncates).  Their
        // record handles are iterators into 'd_records', which the close
        // invalidates.  From index 0, because the snapshot replaces the log
        // wholesale: nothing this node appended can still commit.  'open()'
        // clears the cache too, but only after the wipe, and never touches the
        // pending writes.
        d_raftLog_mp->dropWritesFrom(0);
        d_raftLog_mp->clearCache();

        // Wipe current FileStore
        d_fileStore_sp->close(false, true);  // flush=false, archive=true
    }

    d_snapshotJournalFd = ::open(d_snapshotJournalPath.c_str(),
                                 O_WRONLY | O_CREAT | O_TRUNC,
                                 0644);
    d_snapshotDataFd    = ::open(d_snapshotDataPath.c_str(),
                              O_WRONLY | O_CREAT | O_TRUNC,
                              0644);
    d_snapshotQlistFd   = ::open(d_snapshotQlistPath.c_str(),
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
    // the log can serve the installed index.
    bsls::Types::Uint64 installedIndex = d_raftLog_mp->snapshotIndex();
    if (d_raftLog_mp->lastIndex() > installedIndex) {
        installedIndex = d_raftLog_mp->lastIndex();
    }

    if (installedIndex < d_snapshotLastIncludedIndex) {
        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] snapshot apply reached index " << installedIndex
                       << ", expected " << d_snapshotLastIncludedIndex
                       << "; not acknowledging InstallSnapshot";
        return;
    }

    // The installed files moved the log's floor, so raise the applied state
    // to match: it would otherwise ask for entries below that floor forever.
    // A snapshot carries only committed state, so both watermarks move to
    // what it installed.
    d_raftNode_mp->initRecoveredState(d_raftLog_mp->lastTerm(),
                                      installedIndex);

    // Answer the node that sent the chunks, not 'leaderId()': that is
    // 'k_INVALID_NODE_ID' whenever the term advanced during the transfer, and
    // the response is then dropped -- which the leader sees as a timeout and
    // answers with another full snapshot.
    RaftMessage resp(d_allocator_p);
    resp.d_type         = RaftMessageType::e_INSTALL_SNAPSHOT_RESP;
    resp.d_term         = d_raftNode_mp->currentTerm();
    resp.d_sourceNodeId = d_clusterData_p->membership().selfNode()->nodeId();
    resp.d_destinationNodeId = source->nodeId();
    resp.d_lastLogIndex      = d_snapshotLastIncludedIndex;
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

    // Legacy refreshes these whenever it issues a sync point; Raft issues none
    // periodically, so they would otherwise keep the values 'openForRaft' read
    // off an empty file set.
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
        // FileStore open/recovery failure is unrecoverable; ALARM and
        // terminate, matching legacy 'StorageManager::do_attemptOpenStorage'.
        BMQTSK_ALARMLOG_ALARM("FILE_IO")
            << d_clusterData_p->identity().description() << " Partition ["
            << d_partitionId
            << "]: failed to open/recover PartitionRaftLog, rc: " << rc << "."
            << BMQTSK_ALARMLOG_END;

        mqbu::ExitUtil::terminate(mqbu::ExitCode::e_RECOVERY_FAILURE);  // EXIT
    }

    // Seed the recovered term and applied state.  The term (or legacy-written
    // journal's leaseId, since the on-disk field is the same) must never
    // regress across a restart per Raft's persistent-state contract; the
    // applied state must be raised to the snapshot boundary -- a node that
    // ever rolled over has 'snapshotIndex > 0', and without this the
    // hardcoded 'currentTerm/commitIndex/lastAppliedCommit = 0' from the
    // RaftNode ctor would let this node re-propose a stale term and would
    // stall on indices at or below the snapshot floor. Both watermarks take
    // the snapshot boundary: recovery restores storage to exactly that point,
    // and the entries above it are applied by the normal commit path once a
    // leader confirms them.
    d_raftNode_mp->initRecoveredState(d_raftLog_mp->lastTerm(),
                                      d_raftLog_mp->snapshotIndex());

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

    // Every AppendEntries goes out from here, once the dispatcher has drained
    // this partition's queue.
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

int PartitionRaft::propose(
    const bsl::shared_ptr<mqbs::FileStore::PendingWrite>& pw)
{
    // executed by the partition *DISPATCHER* thread
    enum { rc_UNAVAILABLE = -1 };

    // Compute the rollover footprint (DATA and QLIST bytes) from the write.
    // The JOURNAL reserve is always checked by 'rolloverIfNeeded'; only
    // messages consume DATA and only queue creations consume QLIST.
    bsls::Types::Uint64 dataBytes  = 0;
    bsls::Types::Uint64 qlistBytes = 0;
    if (pw->d_recordType == mqbs::RecordType::e_MESSAGE) {
        dataBytes = mqbs::FileStoreProtocolUtil::messageDataFileSize(
            pw->d_appData,
            pw->d_options);
    }
    else if (pw->d_recordType == mqbs::RecordType::e_QUEUE_OP &&
             pw->d_queueOpType == mqbs::QueueOpType::e_CREATION) {
        qlistBytes = mqbs::FileStoreProtocolUtil::queueCreationQlistFileSize(
            pw->d_queueInfo->d_queueUri,
            pw->d_queueInfo->d_appIdKeyPairs);
    }

    // Skipped while a rollover of this node's own is in flight:
    // 'proposeRollover' marks the partition unavailable for its duration, so
    // the read-only branch below would reject every write of that window
    // instead of letting it buffer.
    if (!d_isRolloverPending) {
        if (!d_fileStore_sp->isFileSetAvailable()) {
            // The journal is read-only: a rollover could not reclaim enough
            // space (outstanding records exceed the policy threshold).  Mirror
            // legacy 'FileStore::writeQueueOpRecord': the ONLY write still
            // permitted is a full-queue PURGE, written into the reserved PURGE
            // area, which frees outstanding records and lets
            // 'onPurgeComplete' roll over to recover.  A per-appId purge
            // (non-null appKey) writes per-message deletion records, for which
            // there is no room -- reject it, like any other write, so the
            // partition stays read-only.
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
            int rc = rolloverIfNeeded(dataBytes, qlistBytes);
            if (0 != rc) {
                return rc;  // RETURN
            }
        }
    }

    // Buffer for replay into the new file set once the rollover commits; this
    // covers both the write that triggered it and every one that follows
    // while it is in flight.  'bufferPendingWrite' reserves 'pw->d_handle'.
    if (d_isRolloverPending) {
        return d_raftLog_mp->bufferPendingWrite(pw,
                                                d_raftNode_mp->currentTerm());
    }

    // Only a local queue proposes, and a local queue only exists on the
    // primary, so the sole way to arrive here without leadership is between
    // this node's stepdown and the conversion of its queues to remote.  Hold
    // the write rather than fail it: the caller would have to answer its
    // producer, and the only answer available would be a NACK for a message
    // the new primary is about to accept.
    //
    // Not 'bufferPendingWrite': that reserves an index and a record handle
    // for replay into this log, and this write is bound for the new primary
    // instead.  The new leader is meanwhile appending at real indices, and
    // 'invalidatePendingWriteHandle' locates a pending write by arithmetic on
    // them, so a reserved index that is never appended would make it address
    // the wrong entry.
    if (!isLeader()) {
        d_writesToRepost.push_back(pw);
        return 0;  // RETURN
    }

    // Otherwise enqueue it for 'append()'; the record's sequence number
    // (index) is stamped there.  'setPendingWrite' stores this same
    // shared_ptr, so 'append()' sets 'pw->d_handle' on the very object the
    // caller holds -- no separate step to surface the handle back is needed.
    d_raftLog_mp->setPendingWrite(pw);

    RaftNodeOutput output(d_allocator_p);
    int rc = d_raftNode_mp->propose(&output, bsl::shared_ptr<bdlbb::Blob>());
    if (rc != 0) {
        return rc;
    }

    dispatchOutput(&output);

    // The cache is NOT released here: the round that reads this entry runs at
    // 'flush()', which releases it once the entry has been served to the
    // peers.  Clearing it now would send every round to mmap instead.
    return 0;
}

void PartitionRaft::proposeDeferredSyncPoint()
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(d_fileStore_sp->inDispatcherThread());

    // Idempotent: a no-op unless this node is the leader and has not yet
    // written the sync point for this leadership.
    // 'd_needsBecomeLeaderSyncPoint' is armed on becoming leader and cleared
    // here, so it is self-correcting across leadership changes.
    if (!isLeader() || !d_needsBecomeLeaderSyncPoint) {
        return;  // RETURN
    }

    d_needsBecomeLeaderSyncPoint = false;

    BALL_LOG_INFO << "Partition [" << d_partitionId
                  << "] writing deferred become-leader sync point (partition "
                  << "activated; CSL advisory for its leaseId has committed).";

    // Set before proposing: in a single-node cluster 'propose' commits and
    // applies synchronously, so 'applyCommittedEntry' runs before this
    // returns.
    d_isExpectingTermCommit = true;

    proposeSyncPoint();

    // The sync point just appended is this term's first entry.  If the log
    // also holds an uncommitted 'e_ROLLOVER' inherited from a prior leader
    // (which proposed the rollover but lost leadership before it committed),
    // that sync point is the current-term entry that will carry the
    // 'e_ROLLOVER' to commit -- so it will commit and roll over regardless of
    // this node's own rollover configuration.  Enter the rollover-pending
    // state now so client writes buffer (keeping the post-'e_ROLLOVER' tail to
    // journal-op sync points) and no duplicate 'e_ROLLOVER' is proposed; the
    // buffered writes drain into the new file set once the inherited rollover
    // commits.  This must not rely on 'rolloverIfNeeded' (a
    // differently-configured new leader may not re-trigger), hence the direct
    // log check.
    if (!d_isRolloverPending &&
        d_raftLog_mp->hasUncommittedRollover(d_raftNode_mp->commitIndex())) {
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

    if (!isLeader()) {
        return;  // RETURN
    }

    BALL_LOG_INFO << "Partition [" << d_partitionId
                  << "] writing final sync point on shutdown.";

    proposeSyncPoint();
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

void PartitionRaft::proposeSyncPoint()
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(isLeader());

    // A sync point is journal-only; 'propose()' runs 'rolloverIfNeeded' and
    // skips issuing it if a rollover is required but blocked by an uncommitted
    // 'e_ROLLOVER' (it will be re-issued on a later tick).
    bsl::shared_ptr<mqbs::FileStore::PendingWrite> pw =
        d_pendingWritePool.getObject();
    pw->initSyncPoint(mqbs::SyncPointType::e_REGULAR);

    int rc = propose(pw);
    if (rc != 0) {
        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] failed to propose sync point upon becoming "
                       << "leader, rc: " << rc;
    }
}

void PartitionRaft::proposeRollover()
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(isLeader());

    bsl::shared_ptr<mqbs::FileStore::PendingWrite> pw =
        d_pendingWritePool.getObject();
    pw->initSyncPoint(mqbs::SyncPointType::e_ROLLOVER);
    // Not buffering here (this is what *starts* the rollover), so this simply
    // enqueues 'e_ROLLOVER' for 'append()'.
    d_raftLog_mp->setPendingWrite(pw);

    // Buffering state is now derived by the log: once 'e_ROLLOVER' is appended
    // it becomes the last log entry, so 'isBuffering()' returns true.  In a
    // single-node cluster 'propose()' commits (and applies, hence rolls over)
    // synchronously, at which point 'isBuffering()' flips back to false and
    // the triggering write appends straight into the new file.

    // Inline the 'propose()' sequence (rather than calling it) to keep the
    // AppendEntries dispatch and cache clearing local.  The physical rollover
    // is now driven by the apply hook when 'e_ROLLOVER' commits, not here.
    RaftNodeOutput output(d_allocator_p);
    int rc = d_raftNode_mp->propose(&output, bsl::shared_ptr<bdlbb::Blob>());
    if (rc != 0) {
        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] failed to propose e_ROLLOVER, rc: " << rc;
        return;
    }

    // Disable the (old) file set now that the 'e_ROLLOVER' marker has been
    // appended to it (the marker write itself must NOT be blocked, so this
    // cannot precede 'propose()' -- 'formatSyncPointRecord' rejects any write
    // to an unavailable set).  This must also precede 'dispatchOutput()':  in
    // a single-node cluster 'dispatchOutput()' applies the just-committed
    // 'e_ROLLOVER' and performs the physical rollover synchronously, swapping
    // in a fresh (available) 'FileSet'; disabling afterwards would wrongly
    // mark that new set unavailable.
    //
    // The flag is defensive here: every regular write during the pending-
    // rollover window is already buffered upstream ('d_isRolloverPending' gate
    // in 'propose()') and never reaches 'format*Record'.  It mirrors legacy's
    // "leave the partition unavailable rather than silently accept writes past
    // capacity" on rollover failure -- a guarantee also enforced by
    // 'PartitionRaftLog::rollover()' on its own failure path.
    d_fileStore_sp->setAvailabilityStatus(false);
    d_isRolloverPending = true;

    dispatchOutput(&output);
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
        // A rollover cannot reclaim enough space -- the partition is full.
        // 'primaryRolloverNeed' has already marked it read-only and panicked;
        // NACK the triggering write rather than roll over into a same-size
        // file and overflow it.  The partition recovers only via a full purge
        // (see 'onPurgeComplete').
        return rc_READONLY;  // RETURN
    }
    case mqbs::FileStore::e_ROLLOVER_NEEDED:
    default: {
        // A rollover is required and will reclaim enough space.  Rather than
        // NACK the triggering write, propose 'e_ROLLOVER' (unless one is
        // already in flight -- at most one uncommitted rollover at a time).
        // 'setPendingWrite()' then buffers this triggering write, and every
        // subsequent one, until the rollover commits.
        if (!d_isRolloverPending) {
            proposeRollover();
        }
        return rc_SUCCESS;  // RETURN
    }
    }
}

void PartitionRaft::drainPendingWrites()
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(isLeader());

    // Take ownership of the buffered writes from the log, emptying its queue.
    // Each 'setPendingWrite' below then leaves the write it pushes at
    // 'd_appendedCount', which is where 'append' looks; nothing is popped
    // until the entry commits, so the queue refills to 'toReplay.size()'.
    PartitionRaftLog::PendingWrites toReplay(d_allocator_p);
    d_raftLog_mp->takePendingWrites(&toReplay);

    if (toReplay.empty()) {
        return;  // RETURN
    }

    BALL_LOG_INFO << "Partition [" << d_partitionId << "] draining "
                  << toReplay.size() << " buffered write(s) after rollover.";

    // Feed each buffered write through the normal append path into a single
    // shared output, so each gets the next contiguous log index in the new
    // file, then dispatch the accumulated output once.
    RaftNodeOutput output(d_allocator_p);

    for (PartitionRaftLog::PendingWrites::iterator it = toReplay.begin();
         it != toReplay.end();
         ++it) {
        const bsl::shared_ptr<mqbs::FileStore::PendingWrite>& sp = *it;

        // The index this write will now receive must equal the one reserved
        // when it was buffered (the term is unchanged for the whole window).
        const bsls::Types::Uint64 expectedIndex = d_raftLog_mp->lastIndex() +
                                                  1;
        if (sp->d_handle.isValid()) {
            BSLS_ASSERT_SAFE(sp->d_handle.sequenceNum() == expectedIndex);
            BSLS_ASSERT_SAFE(
                sp->d_handle.primaryLeaseId() ==
                static_cast<unsigned int>(d_raftNode_mp->currentTerm()));
        }
        (void)expectedIndex;

        // Not buffering during drain (the rollover already committed), so this
        // enqueues the write for 'append()' into the new file set.
        d_raftLog_mp->setPendingWrite(sp);

        int rc = d_raftNode_mp->propose(&output,
                                        bsl::shared_ptr<bdlbb::Blob>());
        if (rc != 0) {
            BALL_LOG_ERROR << "Partition [" << d_partitionId
                           << "] failed to drain buffered write, rc: " << rc;
        }
    }

    dispatchOutput(&output);
}

void PartitionRaft::execute(const mqbi::Dispatcher::VoidFunction& functor)
{
    // Delegate to the owned FileStore, which dispatches on this partition's
    // dispatcher thread.
    d_fileStore_sp->execute(functor);
}

void PartitionRaft::synchronize()
{
    d_fileStore_sp->synchronize();
}

int PartitionRaft::close(bool flush, bool archive)
{
    // executed by the partition *DISPATCHER* thread

    // Any pending write still sitting in the Raft log (e.g. a shutdown sync
    // point proposed moments ago that never got a chance to commit) holds a
    // blob aliased into the active file set.  Drop it now, while this
    // partition's dispatcher is still alive, so 'FileStore::close' below
    // does not leave that alias for a deferred 'FileStore::gc' to release
    // later -- by which time the Dispatcher may already be stopped.  From
    // index 0: this node is closing, so no entry of its own commits again.
    d_raftLog_mp->dropWritesFrom(0);

    // Same reason: the cached entry blobs alias the active file set too.
    d_raftLog_mp->clearCache();

    return d_fileStore_sp->close(flush, archive);
}

int PartitionRaft::rollover()
{
    // executed by the partition *DISPATCHER* thread (admin 'rollover'
    // command). Route the admin rollover through the Raft mechanism instead of
    // legacy FileStore::rollover (which would write an e_ROLLOVER + trailing
    // sync point outside the Raft log).
    enum { rc_SUCCESS = 0, rc_NOT_LEADER = -1, rc_ROLLOVER_PENDING = -2 };

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

    proposeRollover();
    return rc_SUCCESS;
}

int PartitionRaft::transferLeadership(const bsl::string& targetHostName)
{
    // executed by the partition *DISPATCHER* thread (admin
    // 'transferLeadership' command)
    enum {
        rc_SUCCESS        = 0,
        rc_NOT_LEADER     = -1,
        rc_UNKNOWN_TARGET = -2,
        rc_REJECTED       = -3
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
        // The FileStore is closed for the duration of an InstallSnapshot
        // transfer ('beginReceiveSnapshot'), so there is nowhere to append
        // this entry.  Drop it: the leader learns this node's real
        // 'matchIndex' from the InstallSnapshot response and re-sends
        // anything past it on its next heartbeat/propose broadcast, so
        // nothing is lost -- same as a dropped packet in ordinary Raft.
        BALL_LOG_INFO << "Partition [" << d_partitionId
                      << "] ignoring AppendEntries while receiving snapshot";
        return;
    }

    if (!d_fileStore_sp->isOpen()) {
        // Shutdown/teardown: 'StorageUtil::shutdown' -> 'FileStore::close'
        // runs on this same partition dispatcher thread, so a queued
        // AppendEntries can be drained *after* the partition has closed.  The
        // FileStore then has no journal to append to and no storage to apply
        // committed entries against; applying would touch already-freed
        // storage (use-after-free in 'onRecordCommittedReplica' ->
        // 'processMessageRecord').  Drop it -- the leader resends on its next
        // heartbeat if this node returns.
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

    // Applying another partition's entries would write its records into this
    // partition's files.  Routing already selected this object by the same
    // field, so a mismatch means the id changed underneath or the event was
    // misrouted; either way, drop it rather than corrupt the file set.
    if (rh->partitionId() != static_cast<unsigned int>(d_partitionId)) {
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

    // 'skip' hops from one record to the next, starting past the headers.
    // Resolving each record by its offset from the start of 'event' would
    // rescan the buffer list every time, and an event carrying a peer's
    // backlog holds a buffer or two per entry.
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

    bsls::Types::Uint64 prevLastIndex = d_raftLog_mp->lastIndex();

    RaftNodeOutput output(d_allocator_p);
    d_raftNode_mp->step(&output, internalMsg);

    bsls::Types::Uint64 newLastIndex = d_raftLog_mp->lastIndex();

    applyEntriesAsReplica(internalMsg, prevLastIndex, newLastIndex);
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

    // See 'appendEntries': a response for another partition would advance the
    // wrong peer state.
    if (rh->partitionId() != static_cast<unsigned int>(d_partitionId)) {
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
        // As in 'appendEntries' and 'onAppendEntriesResponse': the FileStore
        // is closed for the whole of an InstallSnapshot transfer, and again
        // from 'FileStore::close' at shutdown, both on this same thread. Every
        // message handled below reaches the log -- 'handleRequestVote' asks it
        // for 'lastTerm()' -- and a closed store has no file set to read. Drop
        // it: the sender retries, as it would for a dropped packet.
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

    bsl::shared_ptr<mqbs::FileStore::PendingWrite> pw =
        d_pendingWritePool.getObject();
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

    bsl::shared_ptr<mqbs::FileStore::PendingWrite> pw =
        d_pendingWritePool.getObject();
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
    bsl::shared_ptr<mqbs::FileStore::PendingWrite> pw =
        d_pendingWritePool.getObject();
    pw->initDeletion(guid, queueKey, deletionFlag, timestamp);

    if (d_isDispatchingOutput) {
        // A purge applying at commit writes a DELETION for every message
        // whose refCount it drops to zero, and 'dispatchOutput' is still
        // walking its committed entries.  'postDispatch' proposes these.  The
        // caller needs no handle back, which is what makes deferring safe.
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

    bsl::shared_ptr<mqbs::FileStore::PendingWrite> pw =
        d_pendingWritePool.getObject();
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

    bsl::shared_ptr<mqbs::FileStore::PendingWrite> pw =
        d_pendingWritePool.getObject();
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

    bsl::shared_ptr<mqbs::FileStore::PendingWrite> pw =
        d_pendingWritePool.getObject();
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
    // If this handle belongs to a pending write still in the buffer,
    // invalidate it so application becomes a no-op.  Otherwise proceed with
    // normal removal.
    d_raftLog_mp->invalidatePendingWriteHandle(handle);
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

    // Reclaim the space a purge freed through the Raft rollover mechanism
    // (propose 'e_ROLLOVER'; the physical rollover happens deterministically
    // on commit), NOT the legacy 'FileStore::onPurgeComplete' path, which
    // would drive 'rolloverImpl' directly -- outside the Raft log.  Only the
    // leader proposes, and only if the file set is at capacity;
    // 'rolloverIfNeeded' is a no-op otherwise, so this is safe on replicas too
    // (they roll over via the committed 'e_ROLLOVER' apply hook).
    //
    // If a full purge just recovered a read-only partition, the journal is
    // marked unavailable.  Re-enable it (mirrors legacy
    // 'FileStore::onPurgeComplete') so the reclaiming rollover's sync-point
    // marker can be written -- 'writeSyncPointRecord' refuses on an
    // unavailable journal.  'rolloverIfNeeded' re-disables it if the freed
    // space is still insufficient (a rollover would still overflow).
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
    return d_raftLog_mp->writeHeadIndex();
}

bool PartitionRaft::isApplied(bsls::Types::Uint64 sequenceNumber) const
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

int PartitionRaft::leaderId() const
{
    return d_raftNode_mp->leaderId();
}

bool PartitionRaft::isPendingReplication(
    mqbi::Storage::DeliveryProbe* probe) const
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

    if (0 == probe->d_u0) {
        // First look at this message.  The leader sends the PUSH only once
        // the record has committed, but what is known here may predate that
        // commit, so it is no upper bound on the record's index yet.  Note it
        // and wait for a higher figure: the leader can only have reported
        // that after sending the PUSH, so that one does cover the record.
        probe->d_u0 = leaderCommit;
        return true;  // RETURN
    }

    if (leaderCommit <= probe->d_u0) {
        // Nothing newer heard from the leader yet.
        return true;  // RETURN
    }

    // Applying through a position that covers the record settles it: had it
    // ever been written here it would be in the storage, so it is either gone
    // or was never committed.
    return d_raftNode_mp->lastAppliedCommit() < leaderCommit;
}

bsls::Types::Uint64 PartitionRaft::currentTerm() const
{
    return d_raftNode_mp->currentTerm();
}

}  // close package namespace
}  // close enterprise namespace
