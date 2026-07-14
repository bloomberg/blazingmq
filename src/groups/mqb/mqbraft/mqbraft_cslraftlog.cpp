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

// mqbraft_cslraftlog.cpp -*-C++-*-
#include <mqbraft_cslraftlog.h>

// MQB
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbc_clusterstateledgerutil.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbraft {

namespace {

const int k_RECORD_HEADER_SIZE = static_cast<int>(
    sizeof(mqbc::ClusterStateRecordHeader));

const int k_FILE_HEADER_SIZE = static_cast<int>(
    sizeof(mqbc::ClusterStateFileHeader));

}  // close unnamed namespace

// ================
// class CslRaftLog
// ================

// CREATORS
CslRaftLog::CslRaftLog(const bsl::shared_ptr<mqbsi::Log>& log,
                       BlobSpPool*                       blobSpPool,
                       bslma::Allocator*                 allocator)
: d_log_sp(log)
, d_index(allocator)
, d_snapshotIndex(0)
, d_snapshotTerm(0)
, d_blobSpPool_p(blobSpPool)
, d_allocator_p(bslma::Default::allocator(allocator))
{
    BSLS_ASSERT_SAFE(log);
    BSLS_ASSERT_SAFE(blobSpPool);
}

CslRaftLog::~CslRaftLog()
{
}

// MANIPULATORS
int CslRaftLog::open()
{
    if (!d_log_sp->isOpened()) {
        return -1;
    }

    d_index.clear();
    d_snapshotIndex = 0;
    d_snapshotTerm  = 0;

    const bsls::Types::Int64 numBytes   = d_log_sp->outstandingNumBytes();
    bsls::Types::Int64       offset     = k_FILE_HEADER_SIZE;
    bool                     firstEntry = true;

    while (offset + k_RECORD_HEADER_SIZE <= numBytes) {
        mqbc::ClusterStateRecordHeader* header = 0;
        int rc = d_log_sp->alias(reinterpret_cast<void**>(&header),
                                 k_RECORD_HEADER_SIZE,
                                 offset);
        if (rc != 0) {
            break;
        }

        bsls::Types::Int64 recSize = mqbc::ClusterStateLedgerUtil::recordSize(
            *header);
        if (recSize <= 0 || offset + recSize > numBytes) {
            break;
        }

        mqbc::ClusterStateRecordType::Enum recType = header->recordType();
        if (recType == mqbc::ClusterStateRecordType::e_UPDATE ||
            recType == mqbc::ClusterStateRecordType::e_SNAPSHOT) {
            if (firstEntry &&
                recType == mqbc::ClusterStateRecordType::e_SNAPSHOT &&
                header->sequenceNumber() > 0) {
                // The first record of a rolled-over (or migration-seeded) CSL
                // file is the base snapshot: it captures the full committed
                // cluster state at its sequence number, so the log resumes at
                // that index.  Set the snapshot boundary to 'seq - 1' but
                // STILL index the record: the committed base snapshot is then
                // applied by the normal commit drain (which runs after the
                // cluster's 'ClusterState' observer is registered), keeping
                // 'ClusterState' and the queue helper's 'd_queues' consistent
                // -- rather than by a special early apply during 'start()'.
                // A plain (never-rolled) file whose first record is an
                // 'e_UPDATE' leaves 'd_snapshotIndex == 0'.
                d_snapshotIndex = header->sequenceNumber() - 1;
                d_snapshotTerm  = header->electorTerm();
            }
            RecordInfo info;
            info.d_offset = offset;
            info.d_term   = header->electorTerm();
            d_index.push_back(info);
            firstEntry = false;
        }

        offset += recSize;
    }

    return 0;
}

int CslRaftLog::close()
{
    d_index.clear();
    if (d_log_sp->isOpened()) {
        return d_log_sp->close();
    }
    return 0;
}

int CslRaftLog::append(bsls::Types::Uint64                  term,
                       const bsl::shared_ptr<bdlbb::Blob>&  data,
                       bsls::Types::Uint64                  id)
{
    BSLS_ASSERT_SAFE(id == 0);
    mqbsi::Log::Offset writeOffset = d_log_sp->write(*data,
                                                      bmqu::BlobPosition(),
                                                      data->length());

    if (writeOffset < 0) {
        return static_cast<int>(writeOffset);
    }

    RecordInfo info;
    info.d_offset = writeOffset;
    info.d_term   = term;
    d_index.push_back(info);

    return 0;
}

int CslRaftLog::truncateFrom(bsls::Types::Uint64 index)
{
    if (index <= d_snapshotIndex || index > lastIndex()) {
        return -1;
    }

    bsls::Types::Uint64 vectorIdx = index - d_snapshotIndex - 1;
    mqbsi::Log::Offset  offset = d_index[static_cast<int>(vectorIdx)].d_offset;

    int rc = d_log_sp->truncate(offset);
    if (rc != 0) {
        return rc;
    }

    d_index.erase(d_index.begin() + static_cast<int>(vectorIdx),
                  d_index.end());

    return 0;
}

int CslRaftLog::rollover(bsl::shared_ptr<mqbsi::Log>*        oldLog,
                         const bsl::shared_ptr<mqbsi::Log>&  newLog,
                         const mqbu::StorageKey&             newLogId,
                         const bsl::shared_ptr<bdlbb::Blob>& snapshotRecord,
                         bsls::Types::Uint64                 compactIndex,
                         bsls::Types::Uint64                 compactTerm)
{
    BSLS_ASSERT_SAFE(oldLog);
    BSLS_ASSERT_SAFE(newLog && newLog->isOpened());
    BSLS_ASSERT_SAFE(snapshotRecord);
    BSLS_ASSERT_SAFE(d_snapshotIndex <= compactIndex);
    BSLS_ASSERT_SAFE(compactIndex <= lastIndex());

    // Read the uncommitted tail -- entries with index in
    // '(compactIndex, lastIndex]' -- from the CURRENT log before switching.
    // These are proposals not yet folded into the snapshot and must survive.
    const bsls::Types::Uint64 oldLastIndex = lastIndex();
    bsl::vector<LogEntry>     tail(d_allocator_p);
    if (compactIndex < oldLastIndex) {
        int rc = entries(compactIndex + 1, oldLastIndex + 1, &tail);
        if (rc != 0) {
            return rc;  // RETURN
        }
    }

    // Seed 'newLog' with the file header.
    mqbc::ClusterStateFileHeader fh;
    fh.setProtocolVersion(mqbc::ClusterStateLedgerProtocol::k_VERSION)
        .setHeaderWords(mqbc::ClusterStateFileHeader::k_HEADER_NUM_WORDS)
        .setFileKey(newLogId);
    if (newLog->write(&fh, 0, k_FILE_HEADER_SIZE) < 0) {
        return -1;  // RETURN
    }

    // Write the base snapshot record (the compacted committed state).
    if (newLog->write(*snapshotRecord,
                      bmqu::BlobPosition(),
                      snapshotRecord->length()) < 0) {
        return -2;  // RETURN
    }

    // Copy the tail into 'newLog', recording its new offsets.
    bsl::vector<RecordInfo> newIndex(d_allocator_p);
    newIndex.reserve(tail.size());
    for (bsl::vector<LogEntry>::const_iterator it = tail.begin();
         it != tail.end();
         ++it) {
        mqbsi::Log::Offset newOffset = newLog->write(*it->d_data,
                                                     bmqu::BlobPosition(),
                                                     it->d_data->length());
        if (newOffset < 0) {
            return -3;  // RETURN
        }
        RecordInfo info;
        info.d_offset = newOffset;
        info.d_term   = it->d_term;
        newIndex.push_back(info);
    }

    // Switch to the new log and advance the snapshot boundary.  The base
    // snapshot at the head of 'newLog' is intentionally NOT in 'd_index': it
    // represents the compacted prefix '<= compactIndex'.
    *oldLog         = d_log_sp;
    d_log_sp        = newLog;
    d_index         = newIndex;
    d_snapshotIndex = compactIndex;
    d_snapshotTerm  = compactTerm;

    return 0;
}

// ACCESSORS
bsls::Types::Uint64 CslRaftLog::lastIndex() const
{
    return d_snapshotIndex + d_index.size();
}

bsls::Types::Uint64 CslRaftLog::lastTerm() const
{
    if (d_index.empty()) {
        return d_snapshotTerm;
    }
    return d_index.back().d_term;
}

bsls::Types::Uint64 CslRaftLog::term(bsls::Types::Uint64 index) const
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
    return d_index[vectorIdx].d_term;
}

int CslRaftLog::entries(bsls::Types::Uint64    lo,
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
        mqbsi::Log::Offset  offset =
            d_index[static_cast<int>(vectorIdx)].d_offset;

        mqbc::ClusterStateRecordHeader* header = 0;
        int rc = d_log_sp->alias(reinterpret_cast<void**>(&header),
                                 k_RECORD_HEADER_SIZE,
                                 offset);
        if (rc != 0) {
            return rc;
        }

        int recSize = static_cast<int>(
            mqbc::ClusterStateLedgerUtil::recordSize(*header));

        bsl::shared_ptr<bdlbb::Blob> entryBlob = d_blobSpPool_p->getObject();

        rc = d_log_sp->read(entryBlob.get(), recSize, offset);
        if (rc != 0) {
            return rc;
        }

        out->push_back(LogEntry(d_index[static_cast<int>(vectorIdx)].d_term,
                                i,
                                entryBlob));
    }

    return 0;
}

bsls::Types::Uint64 CslRaftLog::snapshotIndex() const
{
    return d_snapshotIndex;
}

bsls::Types::Uint64 CslRaftLog::snapshotTerm() const
{
    return d_snapshotTerm;
}

void CslRaftLog::applySnapshot(bsls::Types::Uint64 lastIncludedIndex,
                                bsls::Types::Uint64 lastIncludedTerm)
{
    d_snapshotIndex = lastIncludedIndex;
    d_snapshotTerm  = lastIncludedTerm;
    d_index.clear();
}

bool CslRaftLog::canAppend(int recordSize) const
{
    return d_log_sp->currentOffset() + recordSize <=
           d_log_sp->logConfig().maxSize();
}

int CslRaftLog::loadSnapshotRecord(bsl::shared_ptr<bdlbb::Blob>* out) const
{
    BSLS_ASSERT_SAFE(out);

    if (d_snapshotIndex == 0) {
        // No base snapshot: a plain (never-rolled, non-seeded) file.
        return 1;  // RETURN
    }

    // The base snapshot is the first record, right after the file header.
    const mqbsi::Log::Offset offset = k_FILE_HEADER_SIZE;

    mqbc::ClusterStateRecordHeader* header = 0;
    int rc = d_log_sp->alias(reinterpret_cast<void**>(&header),
                             k_RECORD_HEADER_SIZE,
                             offset);
    if (rc != 0) {
        return rc < 0 ? rc : -1;  // RETURN
    }
    if (header->recordType() != mqbc::ClusterStateRecordType::e_SNAPSHOT) {
        return -1;  // RETURN
    }

    const int recSize = static_cast<int>(
        mqbc::ClusterStateLedgerUtil::recordSize(*header));

    bsl::shared_ptr<bdlbb::Blob> blob = d_blobSpPool_p->getObject();
    rc = d_log_sp->read(blob.get(), recSize, offset);
    if (rc != 0) {
        return rc < 0 ? rc : -1;  // RETURN
    }

    *out = blob;
    return 0;
}

}  // close package namespace
}  // close enterprise namespace
