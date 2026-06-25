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
                       bdlbb::BlobBufferFactory*          bufferFactory,
                       bslma::Allocator*                  allocator)
: d_log_sp(log)
, d_index(allocator)
, d_snapshotIndex(0)
, d_snapshotTerm(0)
, d_bufferFactory_p(bufferFactory)
, d_allocator_p(bslma::Default::allocator(allocator))
{
    BSLS_ASSERT_SAFE(log);
    BSLS_ASSERT_SAFE(bufferFactory);
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

    const bsls::Types::Int64 numBytes = d_log_sp->outstandingNumBytes();
    bsls::Types::Int64       offset   = k_FILE_HEADER_SIZE;

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
            RecordInfo info;
            info.d_offset = offset;
            info.d_term   = header->electorTerm();
            d_index.push_back(info);
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

int CslRaftLog::append(bsls::Types::Uint64 term, const bdlbb::Blob& data)
{
    mqbsi::Log::Offset writeOffset = d_log_sp->write(data,
                                                     bmqu::BlobPosition(),
                                                     data.length());

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
    return d_index[static_cast<int>(vectorIdx)].d_term;
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

        bdlbb::Blob entryBlob(d_bufferFactory_p, d_allocator_p);
        entryBlob.setLength(recSize);

        rc = d_log_sp->read(entryBlob.buffer(0).data(), recSize, offset);
        if (rc != 0) {
            return rc;
        }

        out->push_back(LogEntry(d_index[static_cast<int>(vectorIdx)].d_term,
                                entryBlob,
                                d_allocator_p));
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

}  // close package namespace
}  // close enterprise namespace
