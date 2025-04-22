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

// mqbs_qlistfileiterator.cpp                                         -*-C++-*-
#include <ball_log.h>
#include <mqbs_qlistfileiterator.h>

#include <mqbscm_version.h>
// MQB
#include <mqbs_offsetptr.h>

// BMQ
#include <bmqp_protocol.h>

// BDE
#include <bsl_cstddef.h>
#include <bsl_utility.h>

namespace BloombergLP {
namespace mqbs {

// -----------------------
// class QlistFileIterator
// -----------------------

// MANIPULATORS
int QlistFileIterator::reset(const MappedFileDescriptor* mfd,
                             const FileHeader&           fileHeader)
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
        rc_NO_QLIST_HEADER = -3  // Not enough bytes for QlistFileHeader
        ,
        rc_CORRUPT_QLIST_HEADER = -4  // Corrupt QlistFileHeader
        ,
        rc_NOT_ENOUGH_BYTES = -5  // Not enough bytes
    };

    // Clear earlier state
    clear();

    d_mfd_p            = mfd;
    d_qlistRecordIndex = 0;
    d_blockIter.reset(&d_mfd_p->block(), 0, d_mfd_p->fileSize(), true);

    if (0 == fileHeader.headerWords()) {
        // We don't have magic words in headers (FileHeader, JournalFileHeader,
        // QueueRecordHeader, etc). This means that the header size could be ok
        // but it may contain invalid (null, zero etc) values. This check takes
        // care of this scenario.
        clear();
        return rc_CORRUPT_FILE_HEADER;  // RETURN
    }

    // Skip the FileHeader to point to the QlistFileHeader
    bool rc = d_blockIter.advance(fileHeader.headerWords() *
                                  bmqp::Protocol::k_WORD_SIZE);
    if (!rc) {
        // Not enough space for FileHeader
        clear();
        return rc_NO_FILE_HEADER;  // RETURN
    }

    if (static_cast<unsigned int>(QlistFileHeader::k_MIN_HEADER_SIZE) >
        d_blockIter.remaining()) {
        // Not enough space for minimum QlistFileHeader
        clear();
        return rc_NO_QLIST_HEADER;  // RETURN
    }

    OffsetPtr<const QlistFileHeader> qh(d_mfd_p->block(),
                                        d_blockIter.position());

    const unsigned int qfhSize = qh->headerWords() *
                                 bmqp::Protocol::k_WORD_SIZE;
    if (0 == qfhSize) {
        // We don't have magic words in headers (FileHeader, JournalFileHeader,
        // QueueRecordHeader, etc). This means that the header size could be ok
        // but it may contain invalid (null, zero etc) values. This check takes
        // care of this scenario.
        clear();
        return rc_CORRUPT_QLIST_HEADER;  // RETURN
    }

    if (d_blockIter.remaining() < qfhSize) {
        // File not big enough to contain number of bytes declared by the
        // QlistFileHeader
        clear();
        return rc_NOT_ENOUGH_BYTES;  // RETURN
    }
    // Complete QlistFileHeader is present

    d_qlistHeaderOffset = d_blockIter.position();
    BALL_LOG_ERROR << "d_qlistHeaderOffset = " << d_qlistHeaderOffset
                   << ", d_blockIter.position() = " << d_blockIter.position();

    // Reset 'd_blockIter' accordingly with appropriate 'remaining' bytes.

    const unsigned int fhSize = fileHeader.headerWords() *
                                bmqp::Protocol::k_WORD_SIZE;

    // Attempt to iterate over as much file as possible.

    d_blockIter.reset(&d_mfd_p->block(),
                      fhSize,
                      d_mfd_p->fileSize() - fhSize,
                      true);

    // Below code snippet is needed so that 'nextRecord()' works seamlessly
    // during first invocation too, by skipping over 'QlistFileHeader'.
    d_advanceLength = qfhSize;

    return rc_SUCCESS;
}

int QlistFileIterator::nextRecord()
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_HAS_NEXT = 1  // There is another message after this
                         // one
        ,
        rc_AT_END = 0  // This is the last message
        ,
        rc_INVALID = -1  // The Iterator is an invalid state
        ,
        rc_MAGIC_MISMATCH = -2  // Magic mismatch
        ,
        rc_NOT_ENOUGH_BYTES = -3  // Not enough bytes for QueueRecordHeader
                                  // and QueueRecord
        ,
        rc_CORRUPT_RECORD_HEADER = -4  // Queue record header is corrupt
    };

    if (!isValid()) {
        return rc_INVALID;  // RETURN
    }

    if (false == d_blockIter.advance(d_advanceLength)) {
        clear();
        return rc_AT_END;  // RETURN
    }

    if (d_blockIter.isForwardIterator() &&
        d_blockIter.remaining() < QueueRecordHeader::k_MIN_HEADER_SIZE) {
        // Not enough bytes for QueueRecordHeader
        clear();
        return rc_NOT_ENOUGH_BYTES;  // RETURN
    }

    OffsetPtr<const QueueRecordHeader> qrh(*d_blockIter.block(),
                                           d_blockIter.position());

    unsigned int recordSize = qrh->queueRecordWords() *
                              bmqp::Protocol::k_WORD_SIZE;
    // Note that 'recordSize' == sizeof(QueueRecordHeader + QueueRecord)

    if (0 == recordSize) {
        // We don't have magic words in headers (FileHeader, JournalFileHeader,
        // QueueRecordHeader, etc). This means that the header size could be ok
        // but it may contain invalid (null, zero etc) values. This check takes
        // care of this scenario.
        clear();
        return rc_CORRUPT_RECORD_HEADER;  // RETURN
    }

    if (d_blockIter.isForwardIterator()) {
        if (d_blockIter.remaining() < recordSize) {
            // Not enough bytes for (QueueRecordHeader + QueueRecord)
            clear();
            return rc_NOT_ENOUGH_BYTES;  // RETURN
        }

        d_advanceLength = recordSize;
        d_qlistRecordIndex++;
    }
    else {
        if (firstRecordPosition() > d_blockIter.position()) {
            clear();
            return rc_NOT_ENOUGH_BYTES;  // RETURN
        }
        else {
            // We don't have footers for our records and since they are
            // variable-sized, we can't navigate backwards safely, so we have
            // to navigate from the first record to find 'advance length' for
            // next iteration.

            bsls::Types::Uint64 position = firstRecordPosition();

            // It is important to initialize `recSize` with unsigned int
            // max value, so that if below `while` loop doesn't execute
            // because we are at the last record, we still want
            // `d_advanceLength` to be initialized with a non-zero value so
            // that this iterator remains valid, as well as
            // `hasRecordSizeRemaining` works correctly in reverse
            // iteration mode.
            bsls::Types::Uint64 recSize =
                bsl::numeric_limits<unsigned int>::max();

            while (position < d_blockIter.position()) {
                OffsetPtr<const QueueRecordHeader> header(*d_blockIter.block(),
                                                          position);
                recSize = header->queueRecordWords() *
                          bmqp::Protocol::k_WORD_SIZE;
                position += recSize;
            }

            d_advanceLength = recSize;
            --d_qlistRecordIndex;
        }
    }

    // Check magic bits
    OffsetPtr<const bdlb::BigEndianUint32> magic(
        *d_blockIter.block(),
        d_blockIter.position() + recordSize - sizeof(bdlb::BigEndianUint32));

    if (*magic != QueueRecordHeader::k_MAGIC) {
        clear();
        return rc_MAGIC_MISMATCH;  // RETURN
    }

    // 'advanceLength' has already been updated above.

    return rc_HAS_NEXT;
}

void QlistFileIterator::flipDirection()
{
    d_blockIter.flipDirection();

    unsigned int firstRecordPos = firstRecordPosition();
    if (d_blockIter.position() < firstRecordPos) {
        return;  // RETURN
    }
    if (d_blockIter.isForwardIterator()) {
        // We have to reset the advance length to the length of the current
        // record.
        OffsetPtr<const QueueRecordHeader> qrh(*d_blockIter.block(),
                                               d_blockIter.position());
        d_advanceLength = qrh->queueRecordWords() *
                          bmqp::Protocol::k_WORD_SIZE;
    }
    else {
        // We have to reset the advance length to the length of the previous
        // record.  Since our records do not have footers and since they are
        // variable-sized, we have to navigate from the first record.
        while (firstRecordPos < d_blockIter.position()) {
            OffsetPtr<const QueueRecordHeader> qrh(*d_blockIter.block(),
                                                   firstRecordPos);
            d_advanceLength = qrh->queueRecordWords() *
                              bmqp::Protocol::k_WORD_SIZE;
            firstRecordPos += d_advanceLength;
        }
    }
}

// ACCESSORS
void QlistFileIterator::loadQueueUri(const char**  data,
                                     unsigned int* length) const
{
    BSLS_ASSERT_SAFE(data);
    BSLS_ASSERT_SAFE(length);

    const QueueRecordHeader* qrh = queueRecordHeader();
    const char* begin = d_blockIter.block()->base() + d_blockIter.position() +
                        (qrh->headerWords() * bmqp::Protocol::k_WORD_SIZE);
    unsigned int paddedLen = qrh->queueUriLengthWords() *
                             bmqp::Protocol::k_WORD_SIZE;

    BSLS_ASSERT_SAFE(0 < paddedLen);

    *data   = begin;
    *length = paddedLen - begin[paddedLen - 1];
}

void QlistFileIterator::loadQueueUriHash(const char** data) const
{
    BSLS_ASSERT_SAFE(data);

    const QueueRecordHeader* qrh = queueRecordHeader();

    *data = d_blockIter.block()->base() + d_blockIter.position() +
            (qrh->headerWords() * bmqp::Protocol::k_WORD_SIZE) +
            (qrh->queueUriLengthWords() * bmqp::Protocol::k_WORD_SIZE);
}

void QlistFileIterator::loadAppIds(bsl::vector<AppIdLengthPair>* appIds) const
{
    BSLS_ASSERT_SAFE(appIds);

    const QueueRecordHeader* qrh       = queueRecordHeader();
    const size_t             numAppIds = qrh->numAppIds();
    appIds->reserve(numAppIds);

    // Find location of first AppIdHeader.

    size_t pos = d_blockIter.position() +
                 (qrh->headerWords() * bmqp::Protocol::k_WORD_SIZE) +
                 (qrh->queueUriLengthWords() * bmqp::Protocol::k_WORD_SIZE) +
                 FileStoreProtocol::k_HASH_LENGTH;

    for (size_t i = 0; i < numAppIds; ++i) {
        OffsetPtr<const AppIdHeader> appIdHeader(*d_blockIter.block(), pos);

        unsigned int paddedLen = appIdHeader->appIdLengthWords() *
                                 bmqp::Protocol::k_WORD_SIZE;
        const char* begin = d_blockIter.block()->base() + pos +
                            sizeof(AppIdHeader);

        appIds->push_back(
            bsl::make_pair(begin, (paddedLen - begin[paddedLen - 1])));

        // Move to beginning of next AppIdHeader.

        pos += sizeof(AppIdHeader) + paddedLen +
               FileStoreProtocol::k_HASH_LENGTH;
    }

    BSLS_ASSERT_SAFE(numAppIds == appIds->size());
}

void QlistFileIterator::loadAppIdHashes(
    bsl::vector<const char*>* appIdHashes) const
{
    BSLS_ASSERT_SAFE(appIdHashes);

    const QueueRecordHeader* qrh       = queueRecordHeader();
    const size_t             numAppIds = qrh->numAppIds();
    appIdHashes->reserve(numAppIds);

    // Find location of first AppIdHeader.

    size_t pos = d_blockIter.position() +
                 (qrh->headerWords() * bmqp::Protocol::k_WORD_SIZE) +
                 (qrh->queueUriLengthWords() * bmqp::Protocol::k_WORD_SIZE) +
                 FileStoreProtocol::k_HASH_LENGTH;

    for (size_t i = 0; i < numAppIds; ++i) {
        OffsetPtr<const AppIdHeader> appIdHeader(*d_blockIter.block(), pos);

        unsigned int paddedLen = appIdHeader->appIdLengthWords() *
                                 bmqp::Protocol::k_WORD_SIZE;
        const char* begin = d_blockIter.block()->base() + pos +
                            sizeof(AppIdHeader) + paddedLen;

        appIdHashes->push_back(begin);

        // Move to beginning of next AppIdHeader.
        pos += sizeof(AppIdHeader) + paddedLen +
               FileStoreProtocol::k_HASH_LENGTH;
    }

    BSLS_ASSERT_SAFE(numAppIds == appIdHashes->size());
}

}  // close package namespace
}  // close enterprise namespace
