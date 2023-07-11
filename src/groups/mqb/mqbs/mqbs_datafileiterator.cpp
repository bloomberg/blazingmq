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

// mqbs_datafileiterator.cpp                                          -*-C++-*-
#include <mqbs_datafileiterator.h>

#include <mqbscm_version.h>
// MQB
#include <mqbs_offsetptr.h>

// BMQ
#include <bmqp_protocol.h>

namespace BloombergLP {
namespace mqbs {

// ----------------------
// class DataFileIterator
// ----------------------

// MANIPULATORS
int DataFileIterator::reset(const MappedFileDescriptor* mfd,
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
        rc_NO_DATA_HEADER = -3  // Not enough bytes for DataFileHeader
        ,
        rc_CORRUPT_DATA_HEADER = -4  // Corrupt DataFileHeader
        ,
        rc_NOT_ENOUGH_BYTES = -5  // Not enough bytes
    };

    // Clear earlier state.

    clear();

    d_mfd_p           = mfd;
    d_dataRecordIndex = 0;
    d_blockIter.reset(&d_mfd_p->block(), 0, d_mfd_p->fileSize(), true);

    if (0 == fileHeader.headerWords()) {
        // We don't have magic words in headers (FileHeader, JournalFileHeader,
        // QueueRecordHeader, etc). This means that the header size could be ok
        // but it may contain invalid (null, zero etc) values. This check takes
        // care of this scenario.

        clear();
        return rc_CORRUPT_FILE_HEADER;  // RETURN
    }

    // Skip the FileHeader to point to the DataFileHeader.

    bool rc = d_blockIter.advance(fileHeader.headerWords() *
                                  bmqp::Protocol::k_WORD_SIZE);
    if (!rc) {
        // Not enough space for FileHeader.

        clear();
        return rc_NO_FILE_HEADER;  // RETURN
    }

    if (static_cast<unsigned int>(DataFileHeader::k_MIN_HEADER_SIZE) >
        d_blockIter.remaining()) {
        // Not enough space for minimum DataFileHeader.

        clear();
        return rc_NO_DATA_HEADER;  // RETURN
    }

    OffsetPtr<const DataFileHeader> dfh(d_mfd_p->block(),
                                        d_blockIter.position());

    const unsigned int dfhSize = dfh->headerWords() *
                                 bmqp::Protocol::k_WORD_SIZE;

    if (0 == dfhSize) {
        // We don't have magic words in headers (FileHeader, JournalFileHeader,
        // QueueRecordHeader, etc). This means that the header size could be ok
        // but it may contain invalid (null, zero etc) values. This check takes
        // care of this scenario.

        clear();
        return rc_CORRUPT_DATA_HEADER;  // RETURN
    }

    if (d_blockIter.remaining() < dfhSize) {
        // File not big enough to contain number of bytes declared by the
        // DataFileHeader.

        clear();
        return rc_NOT_ENOUGH_BYTES;  // RETURN
    }
    // Complete DataFileHeader is present.

    d_dataFileHeaderOffset = d_blockIter.position();

    // Reset 'd_blockIter' accordingly with appropriate 'remaining' bytes.

    const unsigned int fhSize = fileHeader.headerWords() *
                                bmqp::Protocol::k_WORD_SIZE;

    // Attempt to iterate over as much file as possible.

    d_blockIter.reset(&d_mfd_p->block(),
                      fhSize,
                      d_mfd_p->fileSize() - fhSize,
                      true);

    // Below code snippet is needed so that 'nextRecord()' works seamlessly
    // during first invocation too, by skipping over 'DataFileHeader'.

    d_advanceLength = dfhSize;

    return rc_SUCCESS;
}

int DataFileIterator::nextRecord()
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_HAS_NEXT = 1  // Another message exists
        ,
        rc_AT_END = 0  // This is the last message
        ,
        rc_INVALID = -1  // The Iterator is an invalid state
        ,
        rc_NOT_ENOUGH_BYTES_FOR_HDR = -2  // Not enough bytes for DataHeader
        ,
        rc_NOT_ENOUGH_BYTES_FOR_MSG = -3  // Not enough bytes for the payload
        ,
        rc_CORRUPT_HEADER = -4  // Corrupt DataHeader
    };

    if (!isValid()) {
        return rc_INVALID;  // RETURN
    }

    if (false == d_blockIter.advance(d_advanceLength)) {
        clear();
        return rc_AT_END;  // RETURN
    }

    if (d_blockIter.isForwardIterator() &&
        d_blockIter.remaining() < DataHeader::k_MIN_HEADER_SIZE) {
        // Not enough bytes for DataHeader
        clear();
        return rc_NOT_ENOUGH_BYTES_FOR_HDR;  // RETURN
    }

    OffsetPtr<const DataHeader> dh(*d_blockIter.block(),
                                   d_blockIter.position());

    // `dataLen` represents len(DataHeader + ApplicationData)
    unsigned int dataLen = dh->messageWords() * bmqp::Protocol::k_WORD_SIZE;

    if (0 == dataLen) {
        clear();
        return rc_CORRUPT_HEADER;  // RETURN
    }

    if (d_blockIter.isForwardIterator()) {
        if (d_blockIter.remaining() < dataLen) {
            // Not enough bytes for the payload
            clear();
            return rc_NOT_ENOUGH_BYTES_FOR_MSG;  // RETURN
        }
        d_dataRecordIndex++;
    }
    else {
        if (firstRecordPosition() > d_blockIter.position()) {
            clear();
            return rc_NOT_ENOUGH_BYTES_FOR_MSG;  // RETURN
        }
        else {
            // We don't have footers for our records and since they are
            // variable-sized, we can't navigate backwards safely, so we have
            // to navigate from the first record.
            unsigned int position = firstRecordPosition();
            while (position < d_blockIter.position()) {
                OffsetPtr<const DataHeader> header(*d_blockIter.block(),
                                                   position);
                dataLen = header->messageWords() * bmqp::Protocol::k_WORD_SIZE;
                position += dataLen;
            }
            --d_dataRecordIndex;
        }
    }

    // Update 'advanceLength' for next message
    d_advanceLength = dataLen;

    return rc_HAS_NEXT;
}

void DataFileIterator::flipDirection()
{
    d_blockIter.flipDirection();

    unsigned int firstRecordPos = firstRecordPosition();
    if (d_blockIter.position() < firstRecordPos) {
        return;  // RETURN
    }
    if (d_blockIter.isForwardIterator()) {
        // We have to reset the advance length to the length of the current
        // record.
        OffsetPtr<const DataHeader> dh(*d_blockIter.block(),
                                       d_blockIter.position());
        d_advanceLength = dh->messageWords() * bmqp::Protocol::k_WORD_SIZE;
    }
    else {
        // We have to reset the advance length to the length of the previous
        // record.  Since our records don't have footers and since they are
        // variable-sized, we have to navigate from the first record.
        while (firstRecordPos < d_blockIter.position()) {
            OffsetPtr<const DataHeader> dh(*d_blockIter.block(),
                                           firstRecordPos);
            d_advanceLength = dh->messageWords() * bmqp::Protocol::k_WORD_SIZE;
            firstRecordPos += d_advanceLength;
        }
    }
}

// ACCESSORS
void DataFileIterator::loadApplicationData(const char**  data,
                                           unsigned int* length) const
{
    const DataHeader& dh    = dataHeader();
    const int   headerSize  = dh.headerWords() * bmqp::Protocol::k_WORD_SIZE;
    const int   optionsSize = dh.optionsWords() * bmqp::Protocol::k_WORD_SIZE;
    const char* begin = d_blockIter.block()->base() + d_blockIter.position() +
                        headerSize + optionsSize;
    const unsigned int paddedLen = (dh.messageWords() *
                                    bmqp::Protocol::k_WORD_SIZE) -
                                   headerSize - optionsSize;

    BSLS_ASSERT_SAFE(0 < paddedLen);

    *data   = begin;
    *length = paddedLen - begin[paddedLen - 1];  // length == unpadded len
}

void DataFileIterator::loadOptions(const char**  data,
                                   unsigned int* length) const
{
    const DataHeader& dh  = dataHeader();
    const int headerSize  = dh.headerWords() * bmqp::Protocol::k_WORD_SIZE;
    const int optionsSize = dh.optionsWords() * bmqp::Protocol::k_WORD_SIZE;

    if (0 != optionsSize) {
        *data = d_blockIter.block()->base() + d_blockIter.position() +
                headerSize;
    }
    else {
        *data = 0;
    }

    *length = static_cast<unsigned int>(optionsSize);
}

}  // close package namespace
}  // close enterprise namespace
