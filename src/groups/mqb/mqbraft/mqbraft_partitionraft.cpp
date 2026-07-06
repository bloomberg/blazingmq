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
#include <mqbs_filebackedstorage.h>
#include <mqbs_filestore.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_filestoreprotocolutil.h>
#include <mqbs_filestoreutil.h>
#include <mqbs_inmemorystorage.h>

// BMQ
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqtsk_alarmlog.h>
#include <bmqu_blob.h>
#include <bmqu_blobobjectproxy.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blobutil.h>
#include <bdlf_bind.h>
#include <bdlt_currenttime.h>
#include <bdlt_epochutil.h>
#include <bsl_vector.h>
#include <bslmf_movableref.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbraft {

namespace {

const int k_TICK_INTERVAL_MS = 100;
const int k_JREC_SIZE        = mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
const bsls::Types::Uint64 k_CHUNK_SIZE = 4ULL * 1024 * 1024;

RaftNodeConfig makeRaftConfig(mqbc::ClusterData& clusterData,
                              bslma::Allocator*  allocator)
{
    RaftNodeConfig config(allocator);
    config.d_selfId = clusterData.membership().selfNode()->nodeId();

    const mqbnet::Cluster::NodesList& nodes =
        clusterData.membership().netCluster()->nodes();
    for (mqbnet::Cluster::NodesList::const_iterator it = nodes.begin();
         it != nodes.end();
         ++it) {
        if ((*it)->nodeId() != config.d_selfId) {
            config.d_peerIds.push_back((*it)->nodeId());
        }
    }

    config.d_electionTimeoutMin = 10;
    config.d_electionTimeoutMax = 20;
    config.d_heartbeatInterval  = 3;

    return config;
}

int computeEntrySize(const bdlbb::Blob& blob, int offset)
{
    bmqu::BlobPosition pos;
    if (0 != bmqu::BlobUtil::findOffsetSafe(&pos, blob, offset)) {
        return -1;
    }

    bmqu::BlobObjectProxy<mqbs::RecordHeader> rh(&blob, pos, true, false);
    if (!rh.isSet()) {
        return -1;
    }

    switch (rh->type()) {
    case mqbs::RecordType::e_MESSAGE: {
        bmqu::BlobPosition dhPos;
        if (0 != bmqu::BlobUtil::findOffsetSafe(&dhPos,
                                                blob,
                                                offset + k_JREC_SIZE)) {
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
        bmqu::BlobPosition qrhPos;
        if (0 != bmqu::BlobUtil::findOffsetSafe(&qrhPos,
                                                blob,
                                                offset + k_JREC_SIZE)) {
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
                             mqbc::ClusterData*     clusterData,
                             mqbs::StoragesMonitor* storagesMonitor,
                             bslma::Allocator*      allocator)
: d_partitionId(partitionId)
, d_fileStore_sp(fileStore)
, d_clusterData_p(clusterData)
, d_storagesMonitor_p(storagesMonitor)
, d_raftLog_mp()
, d_raftNode_mp()
, d_tickHandle()
, d_writeIdCounter(0)
, d_uncommittedRolloverIndex(0)
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
{
    BSLS_ASSERT_SAFE(d_fileStore_sp);
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(storagesMonitor);

    d_raftLog_mp.load(new (*d_allocator_p)
                          PartitionRaftLog(d_fileStore_sp.get(),
                                           d_allocator_p),
                      d_allocator_p);

    d_raftNode_mp.load(new (*d_allocator_p) RaftNode(
                           makeRaftConfig(*d_clusterData_p, d_allocator_p),
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
void PartitionRaft::dispatchOutput(RaftNodeOutput* output)
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
        else if (msg.d_type == RaftMessageType::e_INSTALL_SNAPSHOT) {
            sendSnapshot(msg.d_destinationNodeId,
                         msg.d_lastLogIndex,
                         msg.d_lastLogTerm);
        }
        else {
            sendControlMessage(msg);
        }
    }

    for (bsl::vector<LogEntry>::size_type i = 0;
         i < output->d_committed.size();
         ++i) {
        applyCommittedEntry(output->d_committed[i]);
    }

    if (output->d_stateChanged && isLeader()) {
        // Just became leader (RaftNode::propose() never sets
        // 'd_stateChanged', so this cannot recurse via 'proposeSyncPoint()'
        // below).
        proposeSyncPoint();
    }
}

void PartitionRaft::sendAppendEntries(const RaftMessage& msg)
{
    // executed by the partition *DISPATCHER* thread
    mqbnet::ClusterNode* destNode =
        d_clusterData_p->membership().netCluster()->lookupNode(
            msg.d_destinationNodeId);
    if (!destNode) {
        BALL_LOG_WARN << "Partition [" << d_partitionId
                      << "] cannot send AppendEntries to unknown node "
                      << msg.d_destinationNodeId;
        return;
    }

    bsl::shared_ptr<bdlbb::Blob> event_sp =
        d_clusterData_p->blobSpPool().getObject();
    bdlbb::Blob& event = *event_sp;

    event.setLength(sizeof(bmqp::EventHeader) + sizeof(bmqp::RaftHeader));

    bmqu::BlobObjectProxy<bmqp::RaftHeader> rh(&event,
                                               true,
                                               sizeof(bmqp::EventHeader));
    (*rh)
        .setTerm(msg.d_term)
        .setPrevLogIndex(msg.d_prevLogIndex)
        .setPrevLogTerm(msg.d_prevLogTerm)
        .setLeaderCommit(msg.d_leaderCommit)
        .setEntryCount(static_cast<unsigned int>(msg.d_entries.size()))
        .setPartitionId(static_cast<unsigned int>(d_partitionId));
    rh.reset();

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

    destNode->write(event_sp, bmqp::EventType::e_RAFT_PARTITION);
}

void PartitionRaft::sendControlMessage(const RaftMessage& msg)
{
    // executed by the partition *DISPATCHER* thread
    mqbnet::ClusterNode* destNode =
        d_clusterData_p->membership().netCluster()->lookupNode(
            msg.d_destinationNodeId);
    if (!destNode) {
        BALL_LOG_WARN << "Partition [" << d_partitionId
                      << "] cannot send Raft control to unknown node "
                      << msg.d_destinationNodeId;
        return;
    }

    bmqp_ctrlmsg::ControlMessage controlMsg;
    bmqp_ctrlmsg::RaftMessage& raftMsg = controlMsg.choice().makeRaftMessage();
    toCtrlMsg(&raftMsg, msg);

    d_clusterData_p->messageTransmitter().sendMessage(controlMsg, destNode);
}

void PartitionRaft::applyCommittedEntry(const LogEntry& entry)
{
    // executed by the partition *DISPATCHER* thread

    if (isLeader()) {
        d_raftLog_mp->applyCommittedEntryAsPrimary(entry.d_index);
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
                                 bsls::Types::Uint64 lastIncludedIndex,
                                 bsls::Types::Uint64 lastIncludedTerm)
{
    // executed by the partition *DISPATCHER* thread
    mqbnet::ClusterNode* destNode =
        d_clusterData_p->membership().netCluster()->lookupNode(destNodeId);
    if (!destNode) {
        BALL_LOG_WARN << "Partition [" << d_partitionId
                      << "] cannot send snapshot to unknown node "
                      << destNodeId;
        return;
    }

    BALL_LOG_INFO << "Partition [" << d_partitionId
                  << "] sending snapshot to node " << destNodeId
                  << ", lastIncludedIndex=" << lastIncludedIndex;

    mqbs::FileStoreSet fileSet;
    d_fileStore_sp->loadCurrentFiles(&fileSet);

    // Send order: data → qlist → journal.  isDone is set on the last chunk
    // of the journal.  Files with size 0 are skipped.
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

    bmqu::MemOutStream errorDesc;
    int                rc = mqbs::FileStoreUtil::openFileSetReadMode(errorDesc,
                                                      fileSet,
                                                      &allFiles[0].d_mfd,
                                                      &allFiles[1].d_mfd,
                                                      &allFiles[2].d_mfd);

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

    // Send XSD InstallSnapshot metadata first so the follower can prepare
    {
        RaftMessage metaMsg(d_allocator_p);
        metaMsg.d_type = RaftMessageType::e_INSTALL_SNAPSHOT;
        metaMsg.d_term = d_raftNode_mp->currentTerm();
        metaMsg.d_sourceNodeId =
            d_clusterData_p->membership().selfNode()->nodeId();
        metaMsg.d_destinationNodeId = destNodeId;
        metaMsg.d_lastLogIndex      = lastIncludedIndex;
        metaMsg.d_lastLogTerm       = lastIncludedTerm;
        sendControlMessage(metaMsg);
    }

    for (int f = 0; f < 3; ++f) {
        bsls::Types::Uint64 offset = 0;
        while (offset < allFiles[f].d_size) {
            bsls::Types::Uint64 remaining = allFiles[f].d_size - offset;
            bsls::Types::Uint64 chunkLen  = bsl::min(k_CHUNK_SIZE, remaining);

            bsl::shared_ptr<bdlbb::Blob> event_sp =
                d_clusterData_p->blobSpPool().getObject();
            bdlbb::Blob& event = *event_sp;

            int hdrSize = static_cast<int>(sizeof(bmqp::EventHeader) +
                                           sizeof(bmqp::SnapshotChunkHeader));
            event.setLength(hdrSize);

            bmqu::BlobObjectProxy<bmqp::SnapshotChunkHeader> hdr(
                &event,
                true,
                sizeof(bmqp::EventHeader));
            (*hdr)
                .setPartitionId(static_cast<unsigned int>(d_partitionId))
                .setFileType(allFiles[f].d_fileType)
                .setDone(chunkLen >= remaining)
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

            destNode->write(event_sp, bmqp::EventType::e_RAFT_SNAPSHOT);

            offset += chunkLen;
        }
    }

    rc = mqbs::FileStoreUtil::closePartitionSet(&allFiles[0].d_mfd,
                                                &allFiles[1].d_mfd,
                                                &allFiles[2].d_mfd);
    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("FILE_IO")
            << d_clusterData_p->identity().description() << " Partition ["
            << d_partitionId
            << "]: Failed to close one of JOURNAL/QLIST/DATA file, rc: " << rc
            << ", while sending snapshot: to node: "
            << destNode->nodeDescription() << BMQTSK_ALARMLOG_END;
    }

    BALL_LOG_INFO << "Partition [" << d_partitionId
                  << "] snapshot sent to node " << destNodeId;
}

void PartitionRaft::beginReceiveSnapshot(bsls::Types::Uint64 lastIncludedIndex,
                                         bsls::Types::Uint64 lastIncludedTerm)
{
    // executed by the partition *DISPATCHER* thread
    BALL_LOG_INFO << "Partition [" << d_partitionId
                  << "] beginning snapshot receive, lastIncludedIndex="
                  << lastIncludedIndex;

    d_storagesMonitor_p->onStoragesCleared(d_partitionId);

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

    // Get file paths before wiping
    mqbs::FileStoreSet fileSet;
    d_fileStore_sp->loadCurrentFiles(&fileSet);
    d_snapshotJournalPath = fileSet.journalFile();
    d_snapshotDataPath    = fileSet.dataFile();
    d_snapshotQlistPath   = fileSet.qlistFile();

    // Wipe current FileStore
    d_fileStore_sp->close(false, true);  // flush=false, archive=true

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

void PartitionRaft::applySnapshotChunk(const bdlbb::Blob& event)
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(d_receivingSnapshot);

    bmqu::BlobObjectProxy<bmqp::SnapshotChunkHeader> hdr(
        &event,
        true,
        sizeof(bmqp::EventHeader));
    if (!hdr.isSet()) {
        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] failed to read SnapshotChunkHeader";
        return;
    }

    unsigned int        fileType    = hdr->fileType();
    bsls::Types::Uint64 offset      = hdr->offset();
    unsigned int        chunkLength = hdr->chunkLength();
    bool                done        = hdr->done();

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
                       << "] no fd for snapshot chunk fileType=" << fileType;
        return;
    }

    if (::lseek(fd, static_cast<off_t>(offset), SEEK_SET) < 0) {
        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] lseek failed offset=" << offset;
        return;
    }

    int                dataOff = static_cast<int>(sizeof(bmqp::EventHeader) +
                                   sizeof(bmqp::SnapshotChunkHeader));
    bmqu::BlobPosition pos;
    if (0 != bmqu::BlobUtil::findOffsetSafe(&pos, event, dataOff)) {
        return;
    }
    for (int i = pos.buffer(); i < event.numDataBuffers() && chunkLength > 0;
         ++i) {
        const bdlbb::BlobBuffer& buf = event.buffer(i);
        int          bufStart        = (i == pos.buffer()) ? pos.byte() : 0;
        unsigned int available       = static_cast<unsigned int>(buf.size() -
                                                           bufStart);
        unsigned int toWrite         = bsl::min(available, chunkLength);
        ::write(fd, buf.data() + bufStart, toWrite);
        chunkLength -= toWrite;
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

    // Send response last — after files are fully applied
    RaftMessage resp(d_allocator_p);
    resp.d_type         = RaftMessageType::e_INSTALL_SNAPSHOT_RESP;
    resp.d_term         = d_raftNode_mp->currentTerm();
    resp.d_sourceNodeId = d_clusterData_p->membership().selfNode()->nodeId();
    resp.d_destinationNodeId = d_raftNode_mp->leaderId();
    resp.d_lastLogIndex      = d_snapshotLastIncludedIndex;
    sendControlMessage(resp);

    BALL_LOG_INFO << "Partition [" << d_partitionId
                  << "] snapshot applied, sent InstallSnapshot response";
}

void PartitionRaft::appendSnapshotChunk(const bdlbb::Blob&   event,
                                        mqbnet::ClusterNode* source)
{
    // executed by the partition *DISPATCHER* thread
    (void)source;

    if (!d_receivingSnapshot) {
        BALL_LOG_WARN << "Partition [" << d_partitionId
                      << "] received snapshot chunk but not in receiving mode";
        return;
    }

    applySnapshotChunk(event);
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
    case RaftMessageType::e_APPEND_ENTRIES_RESP: {
        bmqp_ctrlmsg::RaftAppendEntriesResponse& aer =
            out->choice().makeAppendEntriesResponse();
        aer.success()    = msg.d_success;
        aer.matchIndex() = msg.d_matchIndex;
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
    default: BSLS_ASSERT_SAFE(false); break;
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
    case Choice::SELECTION_ID_APPEND_ENTRIES_RESPONSE: {
        const bmqp_ctrlmsg::RaftAppendEntriesResponse& aer =
            msg.choice().appendEntriesResponse();
        out->d_type       = RaftMessageType::e_APPEND_ENTRIES_RESP;
        out->d_success    = aer.success();
        out->d_matchIndex = aer.matchIndex();
    } break;
    case Choice::SELECTION_ID_TIMEOUT_NOW: {
        out->d_type = RaftMessageType::e_TIMEOUT_NOW;
    } break;
    case Choice::SELECTION_ID_INSTALL_SNAPSHOT: {
        out->d_type = RaftMessageType::e_INSTALL_SNAPSHOT;
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

void PartitionRaft::tickDispatched()
{
    // executed by the partition *DISPATCHER* thread
    RaftNodeOutput output(d_allocator_p);
    d_raftNode_mp->tick(&output);
    dispatchOutput(&output);
}

// MANIPULATORS
int PartitionRaft::start(bsl::ostream& errorDescription)
{
    int rc = d_raftLog_mp->open();
    if (rc != 0) {
        errorDescription << "Failed to open PartitionRaftLog for partition "
                         << d_partitionId << ", rc: " << rc;
        return rc;
    }

    bsls::TimeInterval tickInterval;
    tickInterval.setTotalMilliseconds(k_TICK_INTERVAL_MS);

    d_clusterData_p->scheduler().scheduleRecurringEvent(
        &d_tickHandle,
        tickInterval,
        bdlf::BindUtil::bind(&PartitionRaft::tickCb, this));

    d_isStarted = true;

    BALL_LOG_INFO << "PartitionRaft started for partition " << d_partitionId
                  << ", node " << d_raftNode_mp->selfId();

    return 0;
}

void PartitionRaft::stop()
{
    if (!d_isStarted) {
        return;
    }

    d_clusterData_p->scheduler().cancelEventAndWait(&d_tickHandle);
    d_isStarted = false;

    BALL_LOG_INFO << "PartitionRaft stopped for partition " << d_partitionId;
}

int PartitionRaft::propose(mqbs::FileStore::PendingWrite& pw)
{
    // executed by the partition *DISPATCHER* thread

    // Compute the rollover footprint (DATA and QLIST bytes) from the write.
    // The JOURNAL reserve is always checked by 'rolloverIfNeeded'; only
    // messages consume DATA and only queue creations consume QLIST.
    bsls::Types::Uint64 dataBytes  = 0;
    bsls::Types::Uint64 qlistBytes = 0;
    if (pw.d_recordType == mqbs::RecordType::e_MESSAGE) {
        dataBytes = mqbs::FileStoreProtocolUtil::messageDataFileSize(
            pw.d_appData,
            pw.d_options);
    }
    else if (pw.d_recordType == mqbs::RecordType::e_QUEUE_OP &&
             pw.d_queueOpType == mqbs::QueueOpType::e_CREATION) {
        qlistBytes = mqbs::FileStoreProtocolUtil::queueCreationQlistFileSize(
            pw.d_queueUri,
            *pw.d_appIdKeyPairs_p);
    }

    int rc = rolloverIfNeeded(dataBytes, qlistBytes);
    if (0 != rc) {
        return rc;  // RETURN
    }

    // Assign the write id and hand the fully-populated 'pw' to the log; the
    // record's sequence number (index) is stamped in 'append()'.
    const bsls::Types::Uint64 writeId = ++d_writeIdCounter;
    pw.d_id                           = writeId;
    d_raftLog_mp->setPendingWrite(pw);

    RaftNodeOutput output(d_allocator_p);
    rc = d_raftNode_mp->propose(&output,
                                bsl::shared_ptr<bdlbb::Blob>(),
                                writeId);
    if (rc != 0) {
        return rc;
    }

    dispatchOutput(&output);
    d_raftLog_mp->clearCache();
    return 0;
}

void PartitionRaft::proposeSyncPoint()
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(isLeader());

    // A sync point is journal-only; 'propose()' runs 'rolloverIfNeeded' and
    // skips issuing it if a rollover is required but blocked by an uncommitted
    // 'e_ROLLOVER' (it will be re-issued on a later tick).
    mqbs::FileStore::PendingWrite pw(0);  // sync-point ctor; id set in propose

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

    // Capture the commit boundary *before* proposing 'e_ROLLOVER'.  In a
    // single-node cluster 'propose()' commits synchronously, which would
    // otherwise pull 'e_ROLLOVER' into the committed prefix and break the
    // uncommitted-tail invariant 'PartitionRaftLog::rollover()' relies on.
    const bsls::Types::Uint64 commitIndex = d_raftNode_mp->commitIndex();

    const bsls::Types::Uint64 timestamp = bdlt::EpochUtil::convertToTimeT64(
        bdlt::CurrentTime::utc());

    const bsls::Types::Uint64 writeId = ++d_writeIdCounter;

    mqbs::FileStore::PendingWrite pw(writeId);
    pw.d_syncPointType = mqbs::SyncPointType::e_ROLLOVER;
    d_raftLog_mp->setPendingWrite(pw);

    // Inline the 'propose()' sequence (rather than calling it) so that
    // 'rollover()' runs *before* 'clearCache()': 'rollover()' captures
    // 'e_ROLLOVER's blob from the log's single-entry cache for its resend
    // slot. The AppendEntries messages in 'output' already carry the
    // 'e_ROLLOVER' bytes (snapshotted during 'propose'), so dispatching them
    // after the rollover is correct.
    RaftNodeOutput output(d_allocator_p);
    int            rc = d_raftNode_mp->propose(&output,
                                    bsl::shared_ptr<bdlbb::Blob>(),
                                    writeId);
    if (rc != 0) {
        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] failed to propose e_ROLLOVER, rc: " << rc;
        return;
    }

    // 'e_ROLLOVER' is the just-appended entry; remember its index so a second
    // rollover is not proposed until this one commits (the single-scalar
    // resend slot holds only one).
    d_uncommittedRolloverIndex = d_raftLog_mp->lastIndex();

    d_raftLog_mp->rollover(commitIndex, timestamp);

    dispatchOutput(&output);
    d_raftLog_mp->clearCache();
}

int PartitionRaft::rolloverIfNeeded(bsls::Types::Uint64 dataBytes,
                                    bsls::Types::Uint64 qlistBytes)
{
    // executed by the partition *DISPATCHER* thread
    enum { rc_SUCCESS = 0, rc_ROLLOVER_PENDING = -1 };

    if (!isLeader() ||
        !d_fileStore_sp->primaryNeedsRollover(dataBytes, qlistBytes)) {
        return rc_SUCCESS;  // RETURN
    }

    // A rollover is required.  Enforce at-most-one uncommitted rollover: if
    // the previous 'e_ROLLOVER' has not committed yet, we cannot propose
    // another (nor write into the already-full file), so reject the write; the
    // caller retries once the outstanding rollover commits.
    if (0 != d_uncommittedRolloverIndex &&
        d_raftNode_mp->commitIndex() < d_uncommittedRolloverIndex) {
        BALL_LOG_WARN
            << "Partition [" << d_partitionId
            << "] rollover required but previous e_ROLLOVER at index "
            << d_uncommittedRolloverIndex
            << " is not yet committed (commitIndex="
            << d_raftNode_mp->commitIndex()
            << "); rejecting write until it commits.";
        return rc_ROLLOVER_PENDING;  // RETURN
    }

    proposeRollover();
    return rc_SUCCESS;
}

void PartitionRaft::execute(const mqbi::Dispatcher::VoidFunction& functor)
{
    // Delegate to the owned FileStore, which dispatches on this partition's
    // dispatcher thread.
    d_fileStore_sp->execute(functor);
}

int PartitionRaft::rollover()
{
    // executed by the partition *DISPATCHER* thread (admin 'rollover'
    // command). Route the admin rollover through the Raft mechanism instead of
    // legacy FileStore::rollover (which would write an e_ROLLOVER + trailing
    // sync point outside the Raft log).
    enum { rc_SUCCESS = 0, rc_NOT_LEADER = -1, rc_ROLLOVER_PENDING = -2 };

    if (!isLeader()) {
        BALL_LOG_WARN << "Partition [" << d_partitionId
                      << "] admin rollover rejected: not the Raft leader.";
        return rc_NOT_LEADER;  // RETURN
    }

    if (0 != d_uncommittedRolloverIndex &&
        d_raftNode_mp->commitIndex() < d_uncommittedRolloverIndex) {
        BALL_LOG_WARN << "Partition [" << d_partitionId
                      << "] admin rollover rejected: previous e_ROLLOVER at "
                      << "index " << d_uncommittedRolloverIndex
                      << " not yet committed.";
        return rc_ROLLOVER_PENDING;  // RETURN
    }

    proposeRollover();
    return rc_SUCCESS;
}

void PartitionRaft::appendEntries(const bdlbb::Blob&   event,
                                  mqbnet::ClusterNode* source)
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(source);

    bmqu::BlobObjectProxy<bmqp::RaftHeader> rh(&event,
                                               false,
                                               sizeof(bmqp::EventHeader));
    if (!rh.isSet()) {
        BALL_LOG_ERROR << "Partition [" << d_partitionId
                       << "] failed to read RaftHeader";
        return;
    }

    RaftMessage internalMsg(d_allocator_p);
    internalMsg.d_type         = RaftMessageType::e_APPEND_ENTRIES;
    internalMsg.d_term         = rh->term();
    internalMsg.d_sourceNodeId = source->nodeId();
    internalMsg.d_prevLogIndex = rh->prevLogIndex();
    internalMsg.d_prevLogTerm  = rh->prevLogTerm();
    internalMsg.d_leaderCommit = rh->leaderCommit();

    int          offset = sizeof(bmqp::EventHeader) + sizeof(bmqp::RaftHeader);
    int          remaining  = event.length() - offset;
    unsigned int entryCount = rh->entryCount();

    for (unsigned int i = 0; i < entryCount && remaining >= k_JREC_SIZE; ++i) {
        int entrySize = computeEntrySize(event, offset);
        if (entrySize <= 0 || entrySize > remaining) {
            BALL_LOG_ERROR << "Partition [" << d_partitionId
                           << "] bad entry size " << entrySize << " at offset "
                           << offset;
            break;
        }

        bmqu::BlobPosition recPos;
        if (0 != bmqu::BlobUtil::findOffsetSafe(&recPos, event, offset)) {
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

        offset += entrySize;
        remaining -= entrySize;
    }

    bsls::Types::Uint64 prevLastIndex = d_raftLog_mp->lastIndex();

    RaftNodeOutput output(d_allocator_p);
    d_raftNode_mp->step(&output, internalMsg);

    bsls::Types::Uint64 newLastIndex = d_raftLog_mp->lastIndex();

    applyEntriesAsReplica(internalMsg, prevLastIndex, newLastIndex);
    dispatchOutput(&output);
}

void PartitionRaft::onRaftControlMessage(
    const bmqp_ctrlmsg::RaftMessage& message,
    mqbnet::ClusterNode*             source)
{
    // executed by the partition *DISPATCHER* thread
    BSLS_ASSERT_SAFE(source);

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

    mqbs::FileStore::PendingWrite pw(0,  // id set in propose()
                                     0,  // primaryLeaseId set in append()
                                     0,  // sequenceNumber set in append()
                                     attributes,
                                     guid,
                                     appData,
                                     options,
                                     queueKey);

    int rc = propose(pw);
    if (rc != 0) {
        return rc;
    }
    *handle = d_raftLog_mp->cachedHandle();
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

    mqbs::FileStore::PendingWrite pw(0,  // id set in propose()
                                     guid,
                                     queueKey,
                                     appKey,
                                     timestamp,
                                     reason);

    int rc = propose(pw);
    if (rc != 0) {
        return rc;
    }
    *handle = d_raftLog_mp->cachedHandle();
    return 0;
}

int PartitionRaft::writeDeletionRecord(
    const bmqt::MessageGUID&       guid,
    const mqbu::StorageKey&        queueKey,
    mqbs::DeletionRecordFlag::Enum deletionFlag,
    bsls::Types::Uint64            timestamp)
{
    mqbs::FileStore::PendingWrite pw(0,  // id set in propose()
                                     guid,
                                     queueKey,
                                     deletionFlag,
                                     timestamp);

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

    mqbs::FileStore::PendingWrite pw(0,  // id set in propose()
                                     mqbs::QueueOpType::e_PURGE,
                                     queueKey,
                                     appKey,
                                     timestamp,
                                     startLeaseId,
                                     startSeqNo);

    int rc = propose(pw);
    if (rc != 0) {
        return rc;
    }
    *handle = d_raftLog_mp->cachedHandle();
    return 0;
}

int PartitionRaft::writeQueueDeletionRecord(
    mqbs::DataStoreRecordHandle* handle,
    const mqbu::StorageKey&      queueKey,
    const mqbu::StorageKey&      appKey,
    bsls::Types::Uint64          timestamp)
{
    BSLS_ASSERT_SAFE(handle);

    mqbs::FileStore::PendingWrite pw(0,  // id set in propose()
                                     mqbs::QueueOpType::e_DELETION,
                                     queueKey,
                                     appKey,
                                     timestamp,
                                     0,   // startPrimaryLeaseId
                                     0);  // startSequenceNumber

    int rc = propose(pw);
    if (rc != 0) {
        return rc;
    }
    *handle = d_raftLog_mp->cachedHandle();
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

    mqbs::FileStore::PendingWrite pw(0,  // id set in propose()
                                     queueUri,
                                     queueKey,
                                     &appIdKeyPairs,
                                     timestamp,
                                     isNewQueue);

    int rc = propose(pw);
    if (rc != 0) {
        return rc;
    }
    *handle = d_raftLog_mp->cachedHandle();
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

mqbs::StoragesMonitor* PartitionRaft::storagesMonitor()
{
    return d_storagesMonitor_p;
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
    d_fileStore_sp->onPurgeComplete();
}

void PartitionRaft::flushStorage()
{
    d_fileStore_sp->flushStorage();
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

unsigned int PartitionRaft::primaryLeaseId() const
{
    return static_cast<unsigned int>(d_raftNode_mp->currentTerm());
}

bool PartitionRaft::hasReceipt(const mqbs::DataStoreRecordHandle& handle) const
{
    return d_fileStore_sp->hasReceipt(handle);
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

bsls::Types::Uint64 PartitionRaft::currentTerm() const
{
    return d_raftNode_mp->currentTerm();
}

}  // close package namespace
}  // close enterprise namespace
