// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbs_filestoreprotocolutil.cpp                                     -*-C++-*-
#include <mqbs_filestoreprotocolutil.h>

#include <mqbscm_version.h>
// MQB
#include <mqbs_memoryblock.h>
#include <mqbs_offsetptr.h>

// BMQ
#include <bmqp_protocol.h>  // for bmqp::Protocol::k_WORD_SIZE

// BDE
#include <bdlb_bigendian.h>
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbs {

// ----------------------------
// struct FileStoreProtocolUtil
// ----------------------------

int FileStoreProtocolUtil::hasBmqHeader(const MappedFileDescriptor& mfd)
{
    enum {
        rc_SUCCESS              = 0,
        rc_NOT_ENOUGH_MIN_BYTES = -1,
        rc_MAGIC_MISMATCH       = -2,
        rc_INVALID_HEADER_SIZE  = -3,
        rc_NOT_ENOUGH_BYTES     = -4,
        rc_INVALID_FILE_TYPE    = -5,
        rc_INVALID_PARTITION_ID = -6
    };

    if (static_cast<unsigned int>(FileHeader::k_MIN_HEADER_SIZE) >
        mfd.fileSize()) {
        return rc_NOT_ENOUGH_MIN_BYTES;  // RETURN
    }

    OffsetPtr<const FileHeader> fh(mfd.block(), 0);

    if (FileHeader::k_MAGIC1 != fh->magic1() ||
        FileHeader::k_MAGIC2 != fh->magic2()) {
        return rc_MAGIC_MISMATCH;  // RETURN
    }

    const size_t headerSize = static_cast<size_t>(fh->headerWords() *
                                                  bmqp::Protocol::k_WORD_SIZE);

    if (0 == headerSize) {
        return rc_INVALID_HEADER_SIZE;  // RETURN
    }

    if (headerSize > mfd.fileSize()) {
        return rc_NOT_ENOUGH_BYTES;  // RETURN
    }

    if (FileType::e_UNDEFINED == fh->fileType()) {
        return rc_INVALID_FILE_TYPE;  // RETURN
    }

    if (0 > fh->partitionId()) {
        return rc_INVALID_PARTITION_ID;  // RETURN
    }

    return rc_SUCCESS;
}

const FileHeader&
FileStoreProtocolUtil::bmqHeader(const MappedFileDescriptor& mfd)
{
    BSLS_ASSERT_SAFE(0 == hasBmqHeader(mfd));
    return reinterpret_cast<const FileHeader&>(*(mfd.block().base()));
}

int FileStoreProtocolUtil::hasValidFirstSyncPointRecord(
    const MappedFileDescriptor& journalFd)
{
    enum {
        rc_SUCCESS                          = 0,
        rc_MISSING_BMQ_HEADER               = -1,
        rc_NOT_JOURNAL_FILE                 = -2,
        rc_NOT_ENOUGH_BYTES                 = -3,
        rc_NO_SYNC_POINT_RECORD             = -4,
        rc_CORRUPT_SYNC_POINT_RECORD        = -5,
        rc_SYNC_POINT_INVALID_SEQ_NUM       = -6,
        rc_SYNC_POINT_RECORD_MAGIC_MISMATCH = -7
    };

    if (0 != hasBmqHeader(journalFd)) {
        return rc_MISSING_BMQ_HEADER;  // RETURN
    }

    const FileHeader& fh = bmqHeader(journalFd);

    const unsigned int fileHeaderSize = fh.headerWords() *
                                        bmqp::Protocol::k_WORD_SIZE;

    BSLS_ASSERT_SAFE(journalFd.fileSize() >= fileHeaderSize);

    if (FileType::e_JOURNAL != fh.fileType()) {
        return rc_NOT_JOURNAL_FILE;  // RETURN
    }

    if (journalFd.fileSize() <
        (JournalFileHeader::k_MIN_HEADER_SIZE + fileHeaderSize)) {
        return rc_NOT_ENOUGH_BYTES;  // RETURN
    }

    OffsetPtr<const JournalFileHeader> jfh(journalFd.block(), fileHeaderSize);

    const unsigned int journalHeaderSize = jfh->headerWords() *
                                           bmqp::Protocol::k_WORD_SIZE;

    if (journalFd.fileSize() < journalHeaderSize) {
        return rc_NOT_ENOUGH_BYTES;  // RETURN
    }

    bsls::Types::Uint64 firstSyncPointOffset =
        jfh->firstSyncPointOffsetWords() * bmqp::Protocol::k_WORD_SIZE;
    if (0 == firstSyncPointOffset) {
        return rc_NO_SYNC_POINT_RECORD;  // RETURN
    }

    if (journalFd.fileSize() < firstSyncPointOffset) {
        return rc_NOT_ENOUGH_BYTES;  // RETURN
    }

    OffsetPtr<const JournalOpRecord> rec(journalFd.block(),
                                         firstSyncPointOffset);
    if (JournalOpType::e_SYNCPOINT != rec->type()) {
        return rc_CORRUPT_SYNC_POINT_RECORD;  // RETURN
    }

    const RecordHeader& recHeader = rec->header();

    if (0 == recHeader.primaryLeaseId() || 0 == recHeader.sequenceNumber()) {
        return rc_SYNC_POINT_INVALID_SEQ_NUM;  // RETURN
    }

    if (RecordHeader::k_MAGIC != rec->magic()) {
        return rc_SYNC_POINT_RECORD_MAGIC_MISMATCH;  // RETURN
    }

    return rc_SUCCESS;
}

bsls::Types::Uint64 FileStoreProtocolUtil::lastJournalSyncPoint(
    const MappedFileDescriptor& mfd,
    const FileHeader&           fileHeader,
    const JournalFileHeader&    journalHeader)
{
    bsls::Types::Uint64 recordStartOffset = (fileHeader.headerWords() +
                                             journalHeader.headerWords()) *
                                            bmqp::Protocol::k_WORD_SIZE;

    if (mfd.fileSize() <= recordStartOffset) {
        return 0;  // RETURN
    }

    const bsls::Types::Uint64 totalRecordsSize = mfd.fileSize() -
                                                 recordStartOffset;
    const unsigned int recordSize = journalHeader.recordWords() *
                                    bmqp::Protocol::k_WORD_SIZE;

    // Note that (totalRecordsSize % recordSize) may not be zero because
    // broker might have crashed in the middle of appending a record to the
    // journal.
    const bsls::Types::Uint64 numRecords = totalRecordsSize / recordSize;

    // Scan backwards for first valid journal sync point.

    for (unsigned int i = 1; i <= numRecords; ++i) {
        bsls::Types::Uint64 recordPos = recordStartOffset +
                                        ((numRecords - i) * recordSize);

        BSLS_ASSERT_SAFE(recordPos < mfd.fileSize());

        OffsetPtr<const RecordHeader> rh(mfd.block(), recordPos);
        if (RecordType::e_JOURNAL_OP != rh->type()) {
            continue;  // CONTINUE
        }

        OffsetPtr<const JournalOpRecord> rec(mfd.block(), recordPos);
        if (JournalOpType::e_SYNCPOINT != rec->type()) {
            // Corrupted sync point record.

            continue;  // CONTINUE
        }

        const RecordHeader& recHeader = rec->header();
        if (0 == recHeader.primaryLeaseId() ||
            0 == recHeader.sequenceNumber()) {
            continue;  // CONTINUE
        }

        if (RecordHeader::k_MAGIC != rec->magic()) {
            // Corrupted sync point record.

            continue;  // CONTINUE
        }

        // Found a valid journal sync point.

        return recordPos;  // RETURN
    }

    // No sync point found.

    return 0;
}

bsls::Types::Uint64 FileStoreProtocolUtil::lastJournalRecord(
    const MappedFileDescriptor& mfd,
    const FileHeader&           fileHeader,
    const JournalFileHeader&    journalHeader,
    bsls::Types::Uint64         lastJournalSyncPoint)
{
    // Need to scan forward from the 'lastJournalSyncPoint' until we encounter
    // an incomplete/corrupted record.  The record preceding this
    // incomplete/corrupted record is the last valid record.

    // We bail out as soon as we encounter first invalid record during this
    // forward scan.  Assuming that OS or filesystem writes to a file
    // sequentially, OR there was no OS or machine crash, this assumption is
    // ok.

    unsigned int minFileSize = (fileHeader.headerWords() +
                                journalHeader.headerWords()) *
                               bmqp::Protocol::k_WORD_SIZE;

    BSLS_ASSERT_SAFE(mfd.fileSize() >= minFileSize);

    if (mfd.fileSize() == minFileSize) {
        // This journal has no records, only FileHeader and JournalFileHeader.
        // The specified 'lastJournalSyncPoint' must be zero.

        BSLS_ASSERT_SAFE(0 == lastJournalSyncPoint);
        return 0;  // RETURN
    }

    const unsigned int recordSize = journalHeader.recordWords() *
                                    bmqp::Protocol::k_WORD_SIZE;

    BSLS_ASSERT_SAFE((lastJournalSyncPoint + recordSize) <= mfd.fileSize());

    bsls::Types::Uint64 currRecordPos = lastJournalSyncPoint + recordSize;
    bsls::Types::Uint64 prevRecordPos = lastJournalSyncPoint;

    if (0 == lastJournalSyncPoint) {
        // The journal has no sync points.  Update 'currRecordPos' and
        // 'prevRecordPos' accordingly.

        currRecordPos = (fileHeader.headerWords() +
                         journalHeader.headerWords()) *
                        bmqp::Protocol::k_WORD_SIZE;
        prevRecordPos = 0;
    }

    while ((currRecordPos + recordSize) <= mfd.fileSize()) {
        OffsetPtr<const RecordHeader> recHeader(mfd.block(), currRecordPos);
        if (RecordType::e_UNDEFINED == recHeader->type()) {
            return prevRecordPos;  // RETURN
        }

        OffsetPtr<const bdlb::BigEndianUint32> magic(
            mfd.block(),
            currRecordPos + recordSize - sizeof(bdlb::BigEndianUint32));

        if (*magic != RecordHeader::k_MAGIC) {
            return prevRecordPos;  // RETURN
        }

        prevRecordPos = currRecordPos;
        currRecordPos += recordSize;
    }

    return prevRecordPos;
}

int FileStoreProtocolUtil::calculateMd5Digest(
    bdlde::Md5::Md5Digest*    buffer,
    const bdlbb::Blob&        blob,
    const bmqu::BlobPosition& startPos,
    unsigned int              length)
{
    BSLS_ASSERT_SAFE(buffer);
    BSLS_ASSERT_SAFE(0 < length);
    BSLS_ASSERT_SAFE(bmqu::BlobUtil::isValidPos(blob, startPos));

    bmqu::BlobPosition endPos;
    int                rc =
        bmqu::BlobUtil::findOffsetSafe(&endPos, blob, startPos, length - 1);
    if (0 != rc) {
        return rc;  // RETURN
    }

    bdlde::Md5         hasher;
    bmqu::BlobPosition pos(startPos);
    while (length) {
        unsigned int len = bsl::min(
            static_cast<int>(length),
            bmqu::BlobUtil::bufferSize(blob, pos.buffer()) - pos.byte());

        hasher.update(blob.buffer(pos.buffer()).data() + pos.byte(), len);

        pos.setBuffer(pos.buffer() + 1);
        pos.setByte(0);
        length -= len;
    }

    hasher.loadDigest(buffer);
    return 0;
}

void FileStoreProtocolUtil::loadAppIdKeyPairs(
    bsl::vector<bsl::pair<bsl::string, mqbu::StorageKey> >* appIdKeyPairs,
    const MemoryBlock&                                      appIdsBlock,
    unsigned int                                            numAppIds)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(appIdKeyPairs);

    appIdKeyPairs->reserve(numAppIds);

    // 'appIdsAreaBegin' should point to the beginning of 1st 'AppIdHeader'.
    unsigned int offset = 0;

    for (size_t i = 0; i < numAppIds; ++i) {
        OffsetPtr<const AppIdHeader> appIdHeader(appIdsBlock, offset);

        unsigned int paddedLen = appIdHeader->appIdLengthWords() *
                                 bmqp::Protocol::k_WORD_SIZE;
        const char* appIdBegin = appIdsBlock.base() + offset +
                                 sizeof(AppIdHeader);

        appIdKeyPairs->emplace_back(
            bsl::string(appIdBegin,
                        paddedLen - appIdBegin[paddedLen - 1],
                        appIdKeyPairs->get_allocator()),
            mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                             appIdBegin + paddedLen));

        // Move to beginning of next AppIdHeader.
        offset += sizeof(AppIdHeader) + paddedLen +
                  FileStoreProtocol::k_HASH_LENGTH;  // AppKey
    }
}

}  // close package namespace
}  // close enterprise namespace
