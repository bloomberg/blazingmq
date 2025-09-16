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

// mqbs_journalfileiterator.cpp                                       -*-C++-*-
#include <mqbs_journalfileiterator.h>

#include <mqbscm_version.h>
// MQB
#include <mqbs_filestoreprotocolutil.h>
#include <mqbs_offsetptr.h>

// BMQ
#include <bmqp_protocol.h>

// BDE
#include <bsl_limits.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace mqbs {

// -------------------------
// class JournalFileIterator
// -------------------------

// MANIPULATORS
int JournalFileIterator::reset(const MappedFileDescriptor* mfd,
                               const FileHeader&           fileHeader,
                               bool                        isReverse)
{
    BSLS_ASSERT_SAFE(mfd);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0  // Success
        ,
        rc_NO_FILE_HEADER = -1  // Not enough bytes for FileHeader
        ,
        rc_CORRUPT_FILE_HEADER = -2  // Corrupt FileHeader
        ,
        rc_NO_JOURNAL_HEADER = -3  // Not enough bytes for JournalHeader
        ,
        rc_CORRUPT_JOURNAL_HEADER = -4  // Corrupt JournalHeader
        ,
        rc_NOT_ENOUGH_BYTES = -5  // Not enough bytes
        ,
        rc_NO_VALID_RECORD = -6  // No valid record in the journal
    };

    // Clear earlier state
    clear();

    d_mfd_p              = mfd;
    d_isReverseMode      = isReverse;
    d_journalRecordIndex = 0;
    d_blockIter.reset(&d_mfd_p->block(), 0, d_mfd_p->fileSize(), true);

    if (0 == fileHeader.headerWords()) {
        // We don't have magic words in headers (FileHeader, JournalFileHeader,
        // QueueRecordHeader, etc). This means that the header size could be ok
        // but it may contain invalid (null, zero etc) values. This check takes
        // care of this scenario.
        clear();
        return rc_CORRUPT_FILE_HEADER;  // RETURN
    }

    // Skip the FileHeader to point to the JournalFileHeader
    bool rc = d_blockIter.advance(fileHeader.headerWords() *
                                  bmqp::Protocol::k_WORD_SIZE);
    if (!rc) {
        // Not enough space for FileHeader
        clear();
        return rc_NO_FILE_HEADER;  // RETURN
    }

    if (static_cast<unsigned int>(JournalFileHeader::k_MIN_HEADER_SIZE) >
        d_blockIter.remaining()) {
        // Not enough space for minimum JournalFileHeader
        clear();
        return rc_NO_JOURNAL_HEADER;  // RETURN
    }

    OffsetPtr<const JournalFileHeader> jh(d_mfd_p->block(),
                                          d_blockIter.position());

    const unsigned int journalHeaderSize = jh->headerWords() *
                                           bmqp::Protocol::k_WORD_SIZE;

    if (0 == journalHeaderSize) {
        // We don't have magic words in headers (FileHeader, JournalFileHeader,
        // QueueRecordHeader, etc). This means that the header size could be ok
        // but it may contain invalid (null, zero etc) values. This check takes
        // care of this scenario.
        clear();
        return rc_CORRUPT_JOURNAL_HEADER;  // RETURN
    }

    if (d_blockIter.remaining() < journalHeaderSize) {
        // File not big enough to contain number of bytes declared by the
        // JournalFileHeader
        clear();
        return rc_NOT_ENOUGH_BYTES;  // RETURN
    }
    // Complete JournalFileHeader is present

    const unsigned int fhSize = fileHeader.headerWords() *
                                bmqp::Protocol::k_WORD_SIZE;

    d_journalHeaderOffset = fhSize;
    d_recordSize          = jh->recordWords() * bmqp::Protocol::k_WORD_SIZE;

    if (0 == d_recordSize) {
        // We don't have magic words in headers (FileHeader, JournalFileHeader,
        // QueueRecordHeader, etc). This means that the header size could be ok
        // but it may contain invalid (null, zero etc) values. This check takes
        // care of this scenario.
        clear();
        return rc_CORRUPT_JOURNAL_HEADER;  // RETURN
    }

    d_lastSyncPointOffset =
        FileStoreProtocolUtil::lastJournalSyncPoint(*d_mfd_p, fileHeader, *jh);

    d_lastRecordOffset = FileStoreProtocolUtil::lastJournalRecord(
        *d_mfd_p,
        fileHeader,
        *jh,
        d_lastSyncPointOffset);

    if (0 == d_lastRecordOffset) {
        // This journal has no records, but reset() and isValid() both should
        // return true, however nextRecord() should fail.

        // This will cause isValid() to succeed
        d_advanceLength = bsl::numeric_limits<int>::max();

        // This will cause nextRecord() to fail
        d_blockIter.clear();
    }
    else {
        // This journal is not empty. Reset 'd_blockIter' appropriately with
        // offset position and remaining length.

        if (!d_isReverseMode) {
            // Reset 'd_blockIter' for forward iteration
            d_blockIter.reset(&d_mfd_p->block(),
                              fhSize,
                              d_lastRecordOffset + d_recordSize - fhSize,
                              true);

            BSLS_ASSERT(true == rc);

            d_advanceLength = journalHeaderSize;
        }
        else {
            // Reset 'd_blockIter' for backward iteration
            d_blockIter.reset(
                &d_mfd_p->block(),
                d_lastRecordOffset + d_recordSize,  // position
                d_lastRecordOffset + d_recordSize -
                    (fileHeader.headerWords() + jh->headerWords()) *
                        bmqp::Protocol::k_WORD_SIZE,
                false);

            d_journalRecordIndex = d_blockIter.remaining() / d_recordSize + 1;
            d_advanceLength      = d_recordSize;
        }
    }

    return rc_SUCCESS;
}

int JournalFileIterator::nextRecord()
{
    return advance(1);
}

int JournalFileIterator::advance(const bsls::Types::Uint64 distance)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_HAS_NEXT = 1  // There is another message after this one
        ,
        rc_AT_END = 0  // This is the last message
        ,
        rc_INVALID = -1  // The Iterator is an invalid state
        ,
        rc_INVALID_RECORD_TYPE = -2  // Invalid record type
        ,
        rc_INVALID_SEQ_NUM = -3  // Invalid sequence number
        ,
        rc_MAGIC_MISMATCH = -4  // Magic mismatch
    };

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isValid())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_INVALID;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            d_blockIter.advance(distance == 1
                                    ? d_advanceLength
                                    : distance * d_recordSize) == false)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        clear();
        return rc_AT_END;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            d_blockIter.isForwardIterator() &&
            d_blockIter.remaining() < d_recordSize)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // At the end (if math was correct, d_blockIter.remaining() == 0)
        clear();
        return rc_AT_END;  // RETURN
    }

    // Validate RecordHeader.
    OffsetPtr<const RecordHeader> recHeader(*d_blockIter.block(),
                                            d_blockIter.position());
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(recHeader->type() ==
                                              RecordType::e_UNDEFINED)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        clear();
        return rc_INVALID_RECORD_TYPE;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            recHeader->primaryLeaseId() == 0 ||
            recHeader->sequenceNumber() == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        clear();
        return rc_INVALID_SEQ_NUM;  // RETURN
    }

    // Check magic
    OffsetPtr<const bdlb::BigEndianUint32> magic(
        *d_blockIter.block(),
        d_blockIter.position() + d_recordSize - sizeof(bdlb::BigEndianUint32));

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(*magic !=
                                              RecordHeader::k_MAGIC)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        clear();
        return rc_MAGIC_MISMATCH;  // RETURN
    }

    // Update advance length. This is needed only if we are iterating in
    // forward direction because 'd_advanceLength' is set to a different value
    // in reset for forward iteration.  But updating its value even in backward
    // iteration has no side effect.
    d_advanceLength = d_recordSize;

    if (d_blockIter.isForwardIterator()) {
        d_journalRecordIndex += distance;
    }
    else {
        d_journalRecordIndex -= distance;
    }

    return rc_HAS_NEXT;
}

void JournalFileIterator::flipDirection()
{
    d_blockIter.flipDirection();
    d_isReverseMode = !d_blockIter.isForwardIterator();
}

}  // close package namespace
}  // close enterprise namespace
