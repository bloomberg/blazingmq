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

// mqbraft_partitionraftlog.cpp -*-C++-*-
#include <mqbraft_partitionraftlog.h>

// MQB
#include <mqbs_filestore.h>

// BMQ
#include <bmqu_blobobjectproxy.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbraft {

// ======================
// class PartitionRaftLog
// ======================

// CREATORS
PartitionRaftLog::PartitionRaftLog(mqbs::FileStore*  fileStore,
                                   bslma::Allocator* allocator)
: d_fileStore_p(fileStore)
, d_index(allocator)
, d_snapshotIndex(0)
, d_snapshotTerm(0)
, d_allocator_p(bslma::Default::allocator(allocator))
, d_pendingWrite()
, d_cache()
, d_pendingRolloverBlob()
, d_pendingRolloverIndex(0)
{
    BSLS_ASSERT_SAFE(fileStore);
}

PartitionRaftLog::~PartitionRaftLog()
{
}

// MANIPULATORS
int PartitionRaftLog::open()
{
    int rc = d_fileStore_p->openForRaft(&d_index);
    if (rc != 0) {
        return rc;
    }

    const bmqp_ctrlmsg::PartitionSequenceNumber& snapshotPSN =
        d_fileStore_p->firstSyncPointAfterRolloverSeqNum();
    d_snapshotIndex = snapshotPSN.sequenceNumber();
    d_snapshotTerm  = snapshotPSN.primaryLeaseId();

    BALL_LOG_INFO << "PartitionRaftLog::open: recovered " << d_index.size()
                  << " entries, snapshotIndex=" << d_snapshotIndex
                  << ", snapshotTerm=" << d_snapshotTerm
                  << ", lastIndex=" << lastIndex()
                  << ", lastTerm=" << lastTerm();

    return 0;
}

void PartitionRaftLog::setPendingWrite(const PendingWrite& pw)
{
    d_pendingWrite = pw;
}

int PartitionRaftLog::append(bsls::Types::Uint64                 term,
                             const bsl::shared_ptr<bdlbb::Blob>& data,
                             bsls::Types::Uint64                 id)
{
    if (id != 0) {
        // Primary path: format directly in mmap via PendingWrite.
        BSLS_ASSERT_SAFE(id == d_pendingWrite.d_id);

        d_pendingWrite.d_primaryLeaseId = term;
        d_pendingWrite.d_sequenceNumber = lastIndex() + 1;

        int rc = -1;
        if (d_pendingWrite.d_recordType == mqbs::RecordType::e_MESSAGE) {
            rc = d_fileStore_p->formatMessageRecord(&d_pendingWrite);
        }
        else if (d_pendingWrite.d_recordType == mqbs::RecordType::e_QUEUE_OP) {
            rc = d_fileStore_p->formatQueueCreationRecord(&d_pendingWrite);
        }
        else {
            BSLS_ASSERT_SAFE(d_pendingWrite.d_recordType ==
                             mqbs::RecordType::e_JOURNAL_OP);
            rc = d_fileStore_p->formatSyncPointRecord(&d_pendingWrite);
        }
        if (rc != 0) {
            return rc;
        }

        d_index.push_back(EntryInfo(d_pendingWrite.d_sequenceNumber,
                                    term,
                                    d_pendingWrite.d_journalOffset,
                                    d_pendingWrite.d_dataOffset,
                                    d_pendingWrite.d_recordType,
                                    d_pendingWrite.d_handle));

        d_cache.d_blob   = d_pendingWrite.d_entryBlob;
        d_cache.d_handle = d_pendingWrite.d_handle;
        d_cache.d_index  = lastIndex();
        d_cache.d_valid  = true;

        d_pendingWrite = PendingWrite();
        return 0;
    }

    // Replica path: combined blob from AppendEntries.
    BSLS_ASSERT_SAFE(data && data->length() > 0);

    EntryInfo info;
    int       rc = d_fileStore_p->writeFormattedRecord(*data, &info);
    if (rc != 0) {
        return rc;
    }

    // 'writeFormattedRecord' fills the physical metadata; the Raft layer owns
    // the sequence number (index) and primary lease id (term).
    info.d_sequenceNum    = lastIndex() + 1;
    info.d_primaryLeaseId = term;
    d_index.push_back(info);

    d_cache.d_blob   = data;
    d_cache.d_handle = info.d_handle;
    d_cache.d_index  = lastIndex();
    d_cache.d_valid  = true;

    // If the replica just appended an 'e_ROLLOVER' sync point, apply the file
    // rollover now -- mirroring the leader (which rolls over in
    // 'PartitionRaft::proposeRollover') and legacy's replica
    // rollover-on-receipt.  Doing it here, at single-entry append granularity,
    // means the rollover happens exactly at 'e_ROLLOVER' even when a catch-up
    // AppendEntries batch continues past it (subsequent entries then land in
    // the new file set).  'commitIndex' is taken as 'd_snapshotIndex': the
    // resulting file bytes are identical regardless of the commit split point,
    // and the replica simply keeps the full tail in 'd_index'.  The marker is
    // stamped with the received 'e_ROLLOVER's own timestamp.
    if (mqbs::RecordType::e_JOURNAL_OP == info.d_recordType) {
        bmqu::BlobObjectProxy<mqbs::JournalOpRecord> jop(data.get(),
                                                         true,    // read
                                                         false);  // write
        if (jop.isSet() && mqbs::JournalOpType::e_SYNCPOINT == jop->type() &&
            mqbs::SyncPointType::e_ROLLOVER == jop->syncPointType()) {
            rollover(d_snapshotIndex, jop->header().timestamp());
        }
    }

    return 0;
}

int PartitionRaftLog::truncateFrom(bsls::Types::Uint64 index)
{
    if (index <= d_snapshotIndex || index > lastIndex()) {
        return -1;
    }

    bsls::Types::Uint64 vectorIdx     = index - d_snapshotIndex - 1;
    bsls::Types::Uint64 journalOffset = d_index[vectorIdx].d_journalOffset;
    bsls::Types::Uint64 dataOffset    = d_index[vectorIdx].d_dataOffset;

    BALL_LOG_WARN << "Raft log truncation from index " << index
                  << " (lastIndex=" << lastIndex()
                  << ", journalOffset=" << journalOffset
                  << ", dataOffset=" << dataOffset << "). Removing "
                  << (d_index.size() - vectorIdx) << " entries.";

    d_fileStore_p->truncateRecords(journalOffset);

    int rc = d_fileStore_p->truncateJournal(journalOffset);
    if (rc != 0) {
        return rc;
    }

    if (dataOffset > 0) {
        rc = d_fileStore_p->truncateData(dataOffset);
        if (rc != 0) {
            return rc;
        }
    }

    d_index.erase(d_index.begin() + vectorIdx, d_index.end());

    // If truncation reached the outstanding 'e_ROLLOVER' entry, drop its
    // resend slot too (marker (i) is discarded by the journal truncate above).
    if (d_pendingRolloverIndex != 0 && index <= d_pendingRolloverIndex) {
        d_pendingRolloverBlob.reset();
        d_pendingRolloverIndex = 0;
    }

    clearCache();

    return 0;
}

void PartitionRaftLog::clearCache()
{
    d_cache.d_valid = false;
    d_cache.d_blob.reset();
}

void PartitionRaftLog::rollover(bsls::Types::Uint64 commitIndex,
                                bsls::Types::Uint64 timestamp)
{
    BSLS_ASSERT_SAFE(commitIndex >= d_snapshotIndex);
    BSLS_ASSERT_SAFE(commitIndex <= lastIndex());

    // 'e_ROLLOVER' is the entry just appended, hence the current last index.
    const bsls::Types::Uint64 rolloverIndex = lastIndex();
    BSLS_ASSERT_SAFE(rolloverIndex > d_snapshotIndex);

    // Capture 'e_ROLLOVER's blob into the single-scalar resend slot before the
    // rollover: it is never written to the new file and so cannot be served
    // from mmap by 'entries()' afterwards.  It is the last-appended entry,
    // held in the cache.
    BSLS_ASSERT_SAFE(d_cache.d_valid && d_cache.d_index == rolloverIndex);
    d_pendingRolloverBlob  = d_cache.d_blob;
    d_pendingRolloverIndex = rolloverIndex;

    // Read the term of the new snapshot boundary before 'd_index' and
    // 'd_snapshotIndex' change below.
    const bsls::Types::Uint64 newSnapshotTerm = term(commitIndex);

    // Prepare the new file set (FileStore creates the files and headers).
    mqbs::FileStore::FileSetSp newFileSetSp;
    int rc = d_fileStore_p->prepareRolloverFileSet(&newFileSetSp);
    BSLS_ASSERT_SAFE(rc == 0);
    (void)rc;
    mqbs::FileSet* newFileSet = newFileSetSp.get();

    mqbs::FileStore::QueueKeyCounterMap queueKeyCounterMap;

    // 1. Committed prefix: copy every outstanding record up to 'commitIndex'
    //    from 'd_records'.  A committed journal-op has already been compacted
    //    out of the log and needs no representation in the new file, so this
    //    straight 'd_records' walk (which never contains journal-ops) is
    //    sufficient and correct for this portion.
    d_fileStore_p->writeRolledOverRecords(newFileSet,
                                          &queueKeyCounterMap,
                                          commitIndex);

    // 2. Uncommitted tail [commitIndex+1 .. lastIndex()], replayed in strict
    //    index order -- this is what preserves the monotonic
    //    offset<->sequenceNumber relationship that 'truncateRecords' relies
    //    on. Offsets are patched in place; the committed prefix is erased
    //    below.
    const bsls::Types::Uint64 tailStart = commitIndex - d_snapshotIndex;
    bsls::Types::Uint64       rolloverSyncPointOldOffset = 0;
    bool                      foundRolloverEntry         = false;

    for (bsl::size_t i = tailStart; i < d_index.size(); ++i) {
        EntryInfo& entry = d_index[i];

        if (mqbs::RecordType::e_JOURNAL_OP != entry.d_recordType) {
            // Normal record (message/queue-op/confirm/deletion): copy it and
            // record its new offsets.  A record can't be both uncommitted and
            // deleted, so its 'd_records' entry is guaranteed to exist.
            mqbs::DataStoreRecord* record = d_fileStore_p->recordForKey(
                entry.d_sequenceNum,
                static_cast<unsigned int>(entry.d_primaryLeaseId));

            d_fileStore_p->writeRolledOverRecord(record,
                                                 &queueKeyCounterMap,
                                                 newFileSet);

            entry.d_journalOffset = record->d_recordOffset;
            entry.d_dataOffset    = record->d_messageOffset;
        }
        else if (entry.d_sequenceNum == rolloverIndex) {
            // The triggering 'e_ROLLOVER': never copied into the new file (so
            // a legacy binary never sees it reappear).  Remember its old-file
            // offset for marker (i), and point its log entry at the current
            // (pre-marker) new-file position so that a later 'truncateFrom' at
            // its index uniformly discards marker (i) too.
            rolloverSyncPointOldOffset = entry.d_journalOffset;
            foundRolloverEntry         = true;

            entry.d_journalOffset = newFileSet->d_journalFilePosition;
            entry.d_dataOffset    = 0;
        }
        else {
            // Any other journal-op in the uncommitted tail (e.g. a regular
            // sync point): construct a fresh verbatim copy into the new file.
            entry.d_journalOffset =
                d_fileStore_p->writeRolledOverJournalOpRecord(
                    newFileSet,
                    entry.d_journalOffset);
            entry.d_dataOffset = 0;
        }
    }

    BSLS_ASSERT_SAFE(foundRolloverEntry &&
                     "e_ROLLOVER must be present in the uncommitted tail");
    (void)foundRolloverEntry;

    d_fileStore_p->logRolloverQueueSummary(queueKeyCounterMap);

    // 3. Marker (i): built from 'e_ROLLOVER' at its old-file offset.
    d_fileStore_p->writeFirstSyncPointAfterRollover(newFileSet,
                                                    rolloverSyncPointOldOffset,
                                                    timestamp);

    // 4. Truncate/gc the old file set, swap the new one in, schedule archive.
    d_fileStore_p->finalizeRolloverFileSet(newFileSetSp);

    // Drop the committed prefix from 'd_index' and advance the snapshot
    // boundary: the committed prefix now lives compacted in the rolled-over
    // file and is served to lagging peers via snapshot.  The surviving tail
    // entries retain the new-file offsets patched above.
    d_index.erase(d_index.begin(), d_index.begin() + tailStart);
    d_snapshotIndex = commitIndex;
    d_snapshotTerm  = newSnapshotTerm;

    clearCache();
}

void PartitionRaftLog::applyCommittedEntryAsPrimary(bsls::Types::Uint64 index)
{
    const EntryInfo& entryInfo = d_index[index - d_snapshotIndex - 1];

    // TODO (raft): do we need entryInfo.d_sequenceNum?
    BSLS_ASSERT_SAFE(entryInfo.d_sequenceNum == index);

    d_fileStore_p->onRecordCommittedPrimary(entryInfo.d_primaryLeaseId, index);
}

void PartitionRaftLog::applyCommittedEntryAsReplica(bsls::Types::Uint64 index,
                                                    const bdlbb::Blob&  data)
{
    const EntryInfo& entryInfo = d_index[index - d_snapshotIndex - 1];

    // TODO (raft): do we need entryInfo.d_sequenceNum?
    BSLS_ASSERT_SAFE(entryInfo.d_sequenceNum == index);
    d_fileStore_p->onRecordCommittedReplica(data, entryInfo.d_handle);
}

void PartitionRaftLog::applySnapshot(bsls::Types::Uint64 lastIncludedIndex,
                                     bsls::Types::Uint64 lastIncludedTerm)
{
    d_snapshotIndex = lastIncludedIndex;
    d_snapshotTerm  = lastIncludedTerm;
    d_index.clear();
    d_pendingRolloverBlob.reset();
    d_pendingRolloverIndex = 0;
    clearCache();
}

// ACCESSORS
bsls::Types::Uint64 PartitionRaftLog::lastIndex() const
{
    return d_snapshotIndex + d_index.size();
}

bsls::Types::Uint64 PartitionRaftLog::lastTerm() const
{
    if (d_index.empty()) {
        return d_snapshotTerm;
    }
    return d_index.back().d_primaryLeaseId;
}

bsls::Types::Uint64 PartitionRaftLog::term(bsls::Types::Uint64 index) const
{
    if (index == 0) {
        return 0;
    }
    if (index == d_snapshotIndex) {
        return d_snapshotTerm;
    }
    if (index <= d_snapshotIndex || index > lastIndex()) {
        return 0;
    }

    bsls::Types::Uint64 vectorIdx = index - d_snapshotIndex - 1;
    return d_index[vectorIdx].d_primaryLeaseId;
}

int PartitionRaftLog::entries(bsls::Types::Uint64    lo,
                              bsls::Types::Uint64    hi,
                              bsl::vector<LogEntry>* out) const
{
    BSLS_ASSERT_SAFE(out);

    if (lo > hi || lo <= d_snapshotIndex || hi > lastIndex() + 1) {
        return -1;
    }

    out->clear();

    for (bsls::Types::Uint64 i = lo; i < hi; ++i) {
        bsls::Types::Uint64 vectorIdx = i - d_snapshotIndex - 1;
        bsls::Types::Uint64 entryTerm = d_index[vectorIdx].d_primaryLeaseId;

        if (d_cache.d_valid && i == d_cache.d_index) {
            out->push_back(LogEntry(entryTerm, i, d_cache.d_blob));
            continue;
        }

        // 'e_ROLLOVER' lives only in the single-scalar resend slot -- it was
        // deliberately excluded from the new file set, so it cannot be read
        // from mmap.
        if (d_pendingRolloverBlob && i == d_pendingRolloverIndex) {
            out->push_back(LogEntry(entryTerm, i, d_pendingRolloverBlob));
            continue;
        }

        // Cache miss: read from mmap'd files
        bsls::Types::Uint64 journalOffset = d_index[vectorIdx].d_journalOffset;
        bsl::shared_ptr<bdlbb::Blob> entryBlob;
        int rc = d_fileStore_p->readRecord(&entryBlob, journalOffset);
        if (rc != 0) {
            return rc;
        }

        out->push_back(LogEntry(entryTerm, i, entryBlob));
    }

    return 0;
}

bsls::Types::Uint64 PartitionRaftLog::snapshotIndex() const
{
    return d_snapshotIndex;
}

bsls::Types::Uint64 PartitionRaftLog::snapshotTerm() const
{
    return d_snapshotTerm;
}

const mqbs::DataStoreRecordHandle& PartitionRaftLog::cachedHandle() const
{
    BSLS_ASSERT_SAFE(d_cache.d_valid);
    return d_cache.d_handle;
}

}  // close package namespace
}  // close enterprise namespace
