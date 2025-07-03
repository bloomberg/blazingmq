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

// bmqp_recoverymessageiterator.t.cpp                                 -*-C++-*-
#include <bmqp_recoverymessageiterator.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>
#include <bsl_cstring.h>
#include <bsl_vector.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

struct Data {
    // DATA
    bool d_isFinalChunk;

    bmqp::RecoveryFileChunkType::Enum d_chunkFileType;

    unsigned int d_pid;

    unsigned int d_seqNum;

    bsl::string d_chunk;
};

/// Populate specified `blob` with a RECOVERY event which has specified
/// `numMsgs` recovery messages, update specified `eh` with corresponding
/// EventHeader, and update specified `data` with the expected values.
void populateBlob(bdlbb::Blob*       blob,
                  bmqp::EventHeader* eh,
                  bsl::vector<Data>* vec,
                  size_t             numMsgs)
{
    int eventLength = 0;

    eh->setType(bmqp::EventType::e_RECOVERY);
    eh->setHeaderWords(sizeof(bmqp::EventHeader) /
                       bmqp::Protocol::k_WORD_SIZE);
    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(eh),
                            sizeof(bmqp::EventHeader));
    eventLength += sizeof(bmqp::EventHeader);

    for (size_t i = 0; i < numMsgs; ++i) {
        int  msgSize = 0;
        Data data;
        data.d_isFinalChunk  = (0 == i % 100) ? true : false;
        data.d_chunkFileType = bmqp::RecoveryFileChunkType::e_JOURNAL;
        data.d_pid           = i % 100;
        data.d_seqNum        = i * i;
        data.d_chunk.resize(4 * i, i);  // payload is 4-byte aligned
        msgSize += 4 * i;

        // RecoveryHeader
        bmqp::RecoveryHeader rhdr;
        rhdr.setHeaderWords(sizeof(bmqp::RecoveryHeader) /
                            bmqp::Protocol::k_WORD_SIZE)
            .setPartitionId(data.d_pid)
            .setChunkSequenceNumber(data.d_seqNum)
            .setFileChunkType(data.d_chunkFileType)
            .setMessageWords(rhdr.headerWords() +
                             (msgSize / bmqp::Protocol::k_WORD_SIZE));

        if (data.d_isFinalChunk) {
            rhdr.setFinalChunkBit();
        }

        // Append RecoveryHeader
        bdlbb::BlobUtil::append(blob,
                                reinterpret_cast<const char*>(&rhdr),
                                rhdr.headerWords() *
                                    bmqp::Protocol::k_WORD_SIZE);

        // Append record
        bdlbb::BlobUtil::append(blob,
                                data.d_chunk.c_str(),
                                data.d_chunk.size());

        eventLength += (rhdr.messageWords() * bmqp::Protocol::k_WORD_SIZE);

        vec->push_back(data);
    }

    // Set EventHeader length
    bmqp::EventHeader* e = reinterpret_cast<bmqp::EventHeader*>(
        blob->buffer(0).data());
    e->setLength(eventLength);
}

void appendMessage(bdlbb::Blob*                      blob,
                   bmqp::EventHeader*                eh,
                   unsigned int                      partitionId,
                   bmqp::RecoveryFileChunkType::Enum chunkFileType,
                   unsigned int                      chunkSeqNum,
                   bool                              isFinal)
{
    int eventLength = 0;

    eh->setType(bmqp::EventType::e_RECOVERY);
    eh->setHeaderWords(sizeof(bmqp::EventHeader) /
                       bmqp::Protocol::k_WORD_SIZE);
    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(eh),
                            sizeof(bmqp::EventHeader));
    eventLength += sizeof(bmqp::EventHeader);

    const char*  p    = "abcdefgh";
    unsigned int pLen = bsl::strlen(p);  // Must be word aligned

    // RecoveryHeader
    bmqp::RecoveryHeader rh;
    rh.setHeaderWords(sizeof(bmqp::RecoveryHeader) /
                      bmqp::Protocol::k_WORD_SIZE)
        .setMessageWords(rh.headerWords() + pLen / bmqp::Protocol::k_WORD_SIZE)
        .setFileChunkType(chunkFileType)
        .setPartitionId(partitionId)
        .setChunkSequenceNumber(chunkSeqNum);

    if (isFinal) {
        rh.setFinalChunkBit();
    }

    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(&rh),
                            rh.headerWords() * bmqp::Protocol::k_WORD_SIZE);
    bdlbb::BlobUtil::append(blob, p, pLen);

    eventLength += (rh.messageWords() * bmqp::Protocol::k_WORD_SIZE);

    // set EventHeader length
    bmqp::EventHeader* e = reinterpret_cast<bmqp::EventHeader*>(
        blob->buffer(0).data());
    e->setLength(eventLength);
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the component.
//
// Plan:
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());

    {
        // Create invalid iter
        bmqp::RecoveryMessageIterator iter;
        BMQTST_ASSERT_EQ(false, iter.isValid());
    }

    {
        // Create invalid iter from another invalid iter
        bmqp::RecoveryMessageIterator iter1;
        bmqp::RecoveryMessageIterator iter2(iter1);

        BMQTST_ASSERT_EQ(false, iter1.isValid());
        BMQTST_ASSERT_EQ(false, iter2.isValid());
    }

    {
        // Assigning invalid iter
        bmqp::RecoveryMessageIterator iter1, iter2;
        BMQTST_ASSERT_EQ(false, iter1.isValid());
        BMQTST_ASSERT_EQ(false, iter2.isValid());

        iter1 = iter2;
        BMQTST_ASSERT_EQ(false, iter1.isValid());
        BMQTST_ASSERT_EQ(false, iter2.isValid());
    }

    {
        // Create valid iter
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

        // Populate blob
        const bool                        isFinal = true;
        const unsigned int                pid     = 3;
        bmqp::RecoveryFileChunkType::Enum chunkFileType =
            bmqp::RecoveryFileChunkType::e_JOURNAL;
        const unsigned int chunkSeqNum = 987654321;
        bmqp::EventHeader  eventHeader;

        appendMessage(&blob,
                      &eventHeader,
                      pid,
                      chunkFileType,
                      chunkSeqNum,
                      isFinal);

        // Iterate and verify
        bmqp::RecoveryMessageIterator iter(&blob, eventHeader);

        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(1, iter.next());
        BMQTST_ASSERT_EQ(true, iter.header().isFinalChunk());
        BMQTST_ASSERT_EQ(pid, iter.header().partitionId());
        BMQTST_ASSERT_EQ(chunkSeqNum, iter.header().chunkSequenceNumber());
        BMQTST_ASSERT_EQ(bmqp::RecoveryFileChunkType::e_JOURNAL,
                         iter.header().fileChunkType());

        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(0, iter.next());
        BMQTST_ASSERT_EQ(false, iter.isValid());

        // Copy

        bmqp::RecoveryMessageIterator iter2(iter);
        BMQTST_ASSERT_EQ(false, iter2.isValid());

        // Clear
        iter.clear();
        BMQTST_ASSERT_EQ(false, iter.isValid());
        BMQTST_ASSERT_EQ(false, iter2.isValid());

        // Assign
        iter = iter2;
        BMQTST_ASSERT_EQ(false, iter.isValid());
        BMQTST_ASSERT_EQ(false, iter2.isValid());

        // Reset, iterate and verify again
        iter.reset(&blob, eventHeader);

        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(1, iter.next());
        BMQTST_ASSERT_EQ(true, iter.header().isFinalChunk());
        BMQTST_ASSERT_EQ(pid, iter.header().partitionId());
        BMQTST_ASSERT_EQ(chunkSeqNum, iter.header().chunkSequenceNumber());
        BMQTST_ASSERT_EQ(bmqp::RecoveryFileChunkType::e_JOURNAL,
                         iter.header().fileChunkType());

        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(0, iter.next());
        BMQTST_ASSERT_EQ(false, iter.isValid());

        // Reset, iterate and verify again
        iter2.reset(&blob, eventHeader);
        iter.reset(&blob, iter2);

        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(1, iter.next());
        BMQTST_ASSERT_EQ(true, iter.header().isFinalChunk());
        BMQTST_ASSERT_EQ(pid, iter.header().partitionId());
        BMQTST_ASSERT_EQ(chunkSeqNum, iter.header().chunkSequenceNumber());
        BMQTST_ASSERT_EQ(bmqp::RecoveryFileChunkType::e_JOURNAL,
                         iter.header().fileChunkType());

        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(0, iter.next());
        BMQTST_ASSERT_EQ(false, iter.isValid());
    }
}

static void test2_multipleMessages()
// ------------------------------------------------------------------------
// MULTIPLE MESSAGES
//
// Concerns:
//
// Plan:
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob eventBlob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

    bsl::vector<Data> data(bmqtst::TestHelperUtil::allocator());
    bmqp::EventHeader eventHeader;
    const size_t      NUM_MSGS = 5000;

    populateBlob(&eventBlob, &eventHeader, &data, NUM_MSGS);

    // Iterate and verify
    bmqp::RecoveryMessageIterator iter(&eventBlob, eventHeader);
    BMQTST_ASSERT_EQ(true, iter.isValid());

    size_t index  = 0;
    int    nextRc = -1;
    while (1 == (nextRc = iter.next()) && index < data.size()) {
        const Data& D = data[index];
        BMQTST_ASSERT_EQ_D(index,
                           D.d_isFinalChunk,
                           iter.header().isFinalChunk());
        BMQTST_ASSERT_EQ_D(index,
                           D.d_chunkFileType,
                           iter.header().fileChunkType());
        BMQTST_ASSERT_EQ_D(index, D.d_pid, iter.header().partitionId());

        BMQTST_ASSERT_EQ_D(index,
                           D.d_seqNum,
                           iter.header().chunkSequenceNumber());

        // Compare chunk record
        bmqu::BlobPosition position;
        BMQTST_ASSERT_EQ_D(index, 0, iter.loadChunkPosition(&position));

        int compareRc = 0;
        BMQTST_ASSERT_EQ_D(index,
                           0,
                           bmqu::BlobUtil::compareSection(&compareRc,
                                                          eventBlob,
                                                          position,
                                                          D.d_chunk.c_str(),
                                                          D.d_chunk.length()));
        BMQTST_ASSERT_EQ_D(index, 0, compareRc);

        ++index;
    }

    BMQTST_ASSERT_EQ(nextRc, 0);
    BMQTST_ASSERT_EQ(index, data.size());
    BMQTST_ASSERT_LT(iter.next(), 0);
    BMQTST_ASSERT_EQ(false, iter.isValid());
}

static void test3_corruptHeader()
// ------------------------------------------------------------------------
// CORRUPT HEADER
//
// Concerns:
//
// Plan:
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob eventBlob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

    const char*  p    = "abcdefgh";
    unsigned int pLen = bsl::strlen(p);

    // BUILD the event blob ...
    // EventHeader
    bmqp::EventHeader eh(bmqp::EventType::e_RECOVERY);
    eh.setLength(sizeof(bmqp::EventHeader) +
                 (2 * sizeof(bmqp::RecoveryHeader)));
    bdlbb::BlobUtil::append(&eventBlob,
                            reinterpret_cast<const char*>(&eh),
                            sizeof(bmqp::EventHeader));
    // RecoveryHeader (valid)
    bmqp::RecoveryHeader rhd1;
    rhd1.setHeaderWords(sizeof(bmqp::RecoveryHeader) /
                        bmqp::Protocol::k_WORD_SIZE)
        .setFileChunkType(bmqp::RecoveryFileChunkType::e_JOURNAL)
        .setPartitionId(0)
        .setChunkSequenceNumber(10000)
        .setMessageWords(rhd1.headerWords() +
                         pLen / bmqp::Protocol::k_WORD_SIZE);

    bdlbb::BlobUtil::append(&eventBlob,
                            reinterpret_cast<const char*>(&rhd1),
                            sizeof(bmqp::RecoveryHeader));
    bdlbb::BlobUtil::append(&eventBlob, "abcdefgh", 8);

    // StorageHeader (invalid: missing a byte from the RecoveryHeader)
    bmqp::RecoveryHeader rhd2;
    rhd2.setHeaderWords(sizeof(bmqp::RecoveryHeader) /
                        bmqp::Protocol::k_WORD_SIZE)
        .setFileChunkType(bmqp::RecoveryFileChunkType::e_DATA)
        .setPartitionId(1)
        .setChunkSequenceNumber(10001)
        .setMessageWords(rhd2.headerWords() - 1);

    bdlbb::BlobUtil::append(&eventBlob,
                            reinterpret_cast<const char*>(&rhd2),
                            sizeof(bmqp::RecoveryHeader) - 1);

    // Iterate and check
    bmqp::RecoveryMessageIterator iter(&eventBlob, eh);

    // First message
    BMQTST_ASSERT_EQ(true, iter.isValid());
    BMQTST_ASSERT_EQ(1, iter.next());
    BMQTST_ASSERT_EQ(true, iter.isValid());
    BMQTST_ASSERT_EQ(false, iter.header().isFinalChunk());
    BMQTST_ASSERT_EQ(0U, iter.header().partitionId());
    BMQTST_ASSERT_EQ(10000U, iter.header().chunkSequenceNumber());
    BMQTST_ASSERT_EQ(bmqp::RecoveryFileChunkType::e_JOURNAL,
                     iter.header().fileChunkType());

    // Second message
    BMQTST_ASSERT_EQ(iter.isValid(), true);
    BMQTST_ASSERT_LT(iter.next(), 0);
    BMQTST_ASSERT_LT(iter.next(), 0);  // make sure successive call to next
                                       // keep returning invalid
    BMQTST_ASSERT_EQ(false, iter.isValid());
}

static void test4_corruptPayload()
// ------------------------------------------------------------------------
// CORRUPT PAYLOAD
//
// Concerns:
//
// Plan:
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob eventBlob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

    const int chunkLen = 1024 * 1024;  // valid, under max limit.

    // BUILD the event blob ...
    // EventHeader
    bmqp::EventHeader eh(bmqp::EventType::e_RECOVERY);
    eh.setLength(sizeof(bmqp::EventHeader) +
                 (2 * sizeof(bmqp::RecoveryHeader)));
    bdlbb::BlobUtil::append(&eventBlob,
                            reinterpret_cast<const char*>(&eh),
                            sizeof(bmqp::EventHeader));
    // RecoveryHeader (valid)
    bmqp::RecoveryHeader rhd1;
    rhd1.setHeaderWords(sizeof(bmqp::RecoveryHeader) /
                        bmqp::Protocol::k_WORD_SIZE)
        .setFileChunkType(bmqp::RecoveryFileChunkType::e_DATA)
        .setPartitionId(0)
        .setChunkSequenceNumber(10000)
        .setMessageWords(rhd1.headerWords() +
                         (chunkLen / bmqp::Protocol::k_WORD_SIZE));

    bdlbb::BlobUtil::append(&eventBlob,
                            reinterpret_cast<const char*>(&rhd1),
                            sizeof(bmqp::RecoveryHeader));

    bsl::string chunkA(bmqtst::TestHelperUtil::allocator());
    chunkA.resize(chunkLen, 'a');  // length under max limit

    bdlbb::BlobUtil::append(&eventBlob, chunkA.c_str(), chunkLen);

    // RecoveryHeader (invalid: missing a byte from the RecoveryHeader)
    bmqp::RecoveryHeader rhd2;
    rhd2.setHeaderWords(sizeof(bmqp::RecoveryHeader) /
                        bmqp::Protocol::k_WORD_SIZE)
        .setFileChunkType(bmqp::RecoveryFileChunkType::e_JOURNAL)
        .setPartitionId(1)
        .setChunkSequenceNumber(10001)
        .setMessageWords(rhd2.headerWords() +
                         (chunkLen / bmqp::Protocol::k_WORD_SIZE));

    bdlbb::BlobUtil::append(&eventBlob,
                            reinterpret_cast<const char*>(&rhd2),
                            sizeof(bmqp::RecoveryHeader));

    bdlbb::BlobUtil::append(&eventBlob,
                            chunkA.c_str(),
                            chunkLen - 2);  // incomplete

    // Iterate and check
    bmqp::RecoveryMessageIterator iter(&eventBlob, eh);

    // First message
    BMQTST_ASSERT_EQ(true, iter.isValid());
    BMQTST_ASSERT_EQ(1, iter.next());
    BMQTST_ASSERT_EQ(true, iter.isValid());
    BMQTST_ASSERT_EQ(0U, iter.header().partitionId());
    BMQTST_ASSERT_EQ(10000U, iter.header().chunkSequenceNumber());
    BMQTST_ASSERT_EQ(false, iter.header().isFinalChunk());
    BMQTST_ASSERT_EQ(bmqp::RecoveryFileChunkType::e_DATA,
                     iter.header().fileChunkType());

    // Second message
    BMQTST_ASSERT_EQ(iter.isValid(), true);
    BMQTST_ASSERT_LT(iter.next(), 0);
    BMQTST_ASSERT_LT(iter.next(), 0);  // make sure successive call to next
                                       // keep returning invalid
    BMQTST_ASSERT_EQ(iter.isValid(), false);
}

static void test5_emptyPayload()
// ------------------------------------------------------------------------
// EMPTY PAYLOAD
//
// Concerns:
//
// Plan:
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob eventBlob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

    bmqp::EventHeader eh;

    eh.setType(bmqp::EventType::e_RECOVERY);
    eh.setHeaderWords(sizeof(bmqp::EventHeader) / bmqp::Protocol::k_WORD_SIZE);
    bdlbb::BlobUtil::append(&eventBlob,
                            reinterpret_cast<const char*>(&eh),
                            eh.headerWords() * bmqp::Protocol::k_WORD_SIZE);

    // RecoveryHeader
    bmqp::RecoveryHeader rhdr;
    rhdr.setHeaderWords(sizeof(bmqp::RecoveryHeader) /
                        bmqp::Protocol::k_WORD_SIZE)
        .setPartitionId(1000)
        .setChunkSequenceNumber(1)
        .setFileChunkType(bmqp::RecoveryFileChunkType::e_JOURNAL)
        .setMessageWords(rhdr.headerWords());

    bdlbb::BlobUtil::append(&eventBlob,
                            reinterpret_cast<const char*>(&rhdr),
                            rhdr.headerWords() * bmqp::Protocol::k_WORD_SIZE);

    // Set EventHeader length
    bmqp::EventHeader* e = reinterpret_cast<bmqp::EventHeader*>(
        eventBlob.buffer(0).data());
    e->setLength(sizeof(bmqp::EventHeader) +
                 (rhdr.messageWords() * bmqp::Protocol::k_WORD_SIZE));

    // Iterate and check
    bmqp::RecoveryMessageIterator iter(&eventBlob, eh);

    // First message
    BMQTST_ASSERT_EQ(true, iter.isValid());
    BMQTST_ASSERT_EQ(1, iter.next());
    BMQTST_ASSERT_EQ(true, iter.isValid());
    BMQTST_ASSERT_EQ(1000U, iter.header().partitionId());
    BMQTST_ASSERT_EQ(1U, iter.header().chunkSequenceNumber());
    BMQTST_ASSERT_EQ(false, iter.header().isFinalChunk());
    BMQTST_ASSERT_EQ(bmqp::RecoveryFileChunkType::e_JOURNAL,
                     iter.header().fileChunkType());
    BMQTST_ASSERT_EQ(0,
                     iter.header().messageWords() -
                         iter.header().headerWords());

    bmqu::BlobPosition chunkPosition;
    BMQTST_ASSERT_EQ(iter.loadChunkPosition(&chunkPosition), 0);
    BMQTST_ASSERT_EQ(iter.next(), 0);
    BMQTST_ASSERT_EQ(iter.isValid(), false);
}

static void test6_nextMethod()
{
    // --------------------------------------------------------------------
    // NEXT
    //
    // Concerns:
    //   Attempting to advance to the next message fails if:
    //     1. Iteration has already reached the end of the event.
    //     2. The underlying message is corrupted:
    //       2.1. number of bytes in the blob is less than event header plus
    //            minimum size of message header.
    //       2.2. number of bytes in the blob is less than event header plus
    //            full size of message header.
    //       2.3. number of bytes in the blob not enough for expected payload.
    //
    // Plan:
    //   1. Create and init iterator by blob.
    //   2. Iterate and reach the end of blob
    //   3. Expect that next method will return error code
    //   4. Create and init iterator by blob of size as described in 2.1.
    //   5. Expect that next method will return error code
    //   6. Create and init iterator by blob of size as described in 2.2.
    //   7. Expect that next method will return error code
    //   8. Create and init iterator by blob of size as described in 2.3.( Use
    //      populateBlob helper method to put 2 messages into blob, and then
    //      reduce blob length to size 1 byte less then needed to allocate
    //      second message payload.
    //   9. First time iterate successfully, then expect that second
    //      invocation of next method will return error code
    //
    // Testing:
    //   int next();
    // --------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("NEXT METHOD");

    // Populate blob
    bsl::vector<Data> data(bmqtst::TestHelperUtil::allocator());
    bmqp::EventHeader eventHeader;

    // Next method. Iterator is in invalid state case.
    {
        PVV("ITERATOR INVALID STATE");

        // Create buffer factory and blob
        bdlbb::PooledBlobBufferFactory bufferFactory(
            1024,
            bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

        // Populate blob
        populateBlob(&blob, &eventHeader, &data, 1);

        // Create valid iterator
        bmqp::RecoveryMessageIterator iter(&blob, eventHeader);
        BMQTST_ASSERT(iter.isValid());

        // Iterate and verify
        BMQTST_ASSERT_EQ(iter.next(), 1);
        BMQTST_ASSERT_EQ(iter.next(), 0);
        BMQTST_ASSERT_LT(iter.next(), 0);  // rc_INVALID
    }

    // Next method. No recovery header. Less then min header size.
    {
        PVV("NO RECOVERY HEADER - LESS THAN MINIMUM HEADER SIZE");

        // Min buf size not to reproduce given rc
        const size_t enoughSize = bmqp::RecoveryHeader::k_MIN_HEADER_SIZE +
                                  sizeof(bmqp::EventHeader);

        // Create buffer factory and blob
        bdlbb::PooledBlobBufferFactory bufferFactory(
            1024,
            bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

        // Populate blob
        populateBlob(&blob, &eventHeader, &data, 1);
        blob.setLength(enoughSize - 1);

        // Create iterator
        bmqp::RecoveryMessageIterator iter(&blob, eventHeader);
        BMQTST_ASSERT(iter.isValid());
        BMQTST_ASSERT_LT(iter.next(), 0);  // rc_NO_RECOVERYHEADER
        BMQTST_ASSERT(!iter.isValid());
    }

    // Next method. No recovery header.
    {
        PVV("NO RECOVERY HEADER - LESS THAN SPECIFIED HEADER SIZE");

        // Min buf size not to reproduce given rc
        const size_t enoughSize = sizeof(bmqp::RecoveryHeader) +
                                  sizeof(bmqp::EventHeader);

        // Create buffer factory and blob
        bdlbb::PooledBlobBufferFactory bufferFactory(
            1024,
            bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

        // Populate blob
        populateBlob(&blob, &eventHeader, &data, 1);
        blob.setLength(enoughSize - 1);

        // Create iterator
        bmqp::RecoveryMessageIterator iter(&blob, eventHeader);
        BMQTST_ASSERT(iter.isValid());
        BMQTST_ASSERT_LT(iter.next(), 0);  // rc_NO_RECOVERYHEADER
        BMQTST_ASSERT(!iter.isValid());
    }

    // Next method. Not enough bytes case.
    {
        PVVV("NOT ENOUGH BYTES");

        const size_t k_NUM_MSGS = 2;

        // Min buf size not to reproduce given rc
        const size_t enoughSize = sizeof(bmqp::RecoveryHeader) * k_NUM_MSGS +
                                  sizeof(bmqp::EventHeader) +
                                  4;  // 4 is payload size due to populateBlob

        // Create buffer factory and blob
        bdlbb::PooledBlobBufferFactory bufferFactory(
            1024,
            bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

        // Populate blob
        populateBlob(&blob, &eventHeader, &data, k_NUM_MSGS);
        blob.setLength(enoughSize - 1);

        // Create iterator
        bmqp::RecoveryMessageIterator iter(&blob, eventHeader);
        BMQTST_ASSERT(iter.isValid());
        BMQTST_ASSERT_EQ(iter.next(), 1);
        BMQTST_ASSERT(iter.isValid());
        BMQTST_ASSERT_LT(iter.next(), 0);  // rc_NOT_ENOUGH_BYTES
        BMQTST_ASSERT(!iter.isValid());
    }
}

static void test7_resetMethod()
{
    // --------------------------------------------------------------------
    // RESET
    //
    // Concerns:
    //   Attempting to reset iterator fails if:
    //     1. The underlying event doesn't contain message header
    //
    // Plan:
    //   1. Create and init iterator by blob of event header size.
    //   2. Expect that reset method will return error code
    //
    // Testing:
    //   int reset();
    // --------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("RESET METHOD");

    // Populate blob
    bsl::vector<Data> data(bmqtst::TestHelperUtil::allocator());
    bmqp::EventHeader eventHeader;
    BMQTST_ASSERT_EQ(eventHeader.headerWords(),
                     sizeof(bmqp::EventHeader) / bmqp::Protocol::k_WORD_SIZE);

    // Reset method. Invalid event header case.
    {
        PVV("INVALID EVENT HEADER");
        // Min buf size not to reproduce given rc
        const size_t enoughSize = sizeof(bmqp::EventHeader) + 1;
        bdlbb::PooledBlobBufferFactory bufferFactory(
            1024,
            bmqtst::TestHelperUtil::allocator());

        // Create buffer factory and blob
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

        // Populate blob and manually set length
        populateBlob(&blob, &eventHeader, &data, 1);

        // Create iterator
        bmqp::RecoveryMessageIterator iter(&blob, eventHeader);
        blob.setLength(enoughSize - 1);
        BMQTST_ASSERT(iter.isValid());

        // Reset and verify
        BMQTST_ASSERT_LT(iter.reset(&blob, eventHeader),
                         0);  // rc_INVALID_EVENTHEADER
        BMQTST_ASSERT(!iter.isValid());
    }
}

static void test8_dumpBlob()
{
    // --------------------------------------------------------------------
    // DUMP BLOB
    //
    // Concerns:
    //   Check blob layout if:
    //     1. The underlying blob contain single recovery message
    //     2. The underlying blob is not setted
    //
    // Plan:
    //   1. Create and populate blob with a single recovery message
    //   2. Create and init iterator by given blob
    //   3. Check output of dumpBlob method
    //   4. Create and init iterator without setting underlying blob
    //   5. Check output of dumpBlob method
    //
    // Testing:
    //   void dumpBlob(bsl::ostream& stream);
    // --------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("DUMP BLOB");

    // Test iterator dump contains expected value
    bmqp::EventHeader              eventHeader;
    bsl::vector<Data>              data(bmqtst::TestHelperUtil::allocator());
    bmqu::MemOutStream             stream(bmqtst::TestHelperUtil::allocator());
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

    // Populate blob
    populateBlob(&blob, &eventHeader, &data, 1);

    // Create iterator over blob
    {
        bmqp::RecoveryMessageIterator iter(&blob, eventHeader);
        BMQTST_ASSERT(iter.isValid());

        // Dump blob
        iter.dumpBlob(stream);
        bsl::string str1(stream.str(), bmqtst::TestHelperUtil::allocator());
        bsl::string str2("     0:   00000024 49020000 80000007 00720000     "
                         "|...$I........r..|\n"
                         "    16:   00000000 00000000 00000000 00000000     "
                         "|................|\n"
                         "    32:   00000000                                "
                         "|....            |\n",
                         bmqtst::TestHelperUtil::allocator());

        // Verify that dump contains expected value
        BMQTST_ASSERT_EQ(str1, str2);
        stream.reset();
    }

    // Create iterator without blob
    {
        bmqp::RecoveryMessageIterator iter;

        // Dump blob
        iter.dumpBlob(stream);
        bsl::string str1(stream.str(), bmqtst::TestHelperUtil::allocator());
        bsl::string str2("/no blob/", bmqtst::TestHelperUtil::allocator());

        // Verify that dump contains expected value
        BMQTST_ASSERT_EQ(str1, str2);
    }
}
// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 8: test8_dumpBlob(); break;
    case 7: test7_resetMethod(); break;
    case 6: test6_nextMethod(); break;
    case 5: test5_emptyPayload(); break;
    case 4: test4_corruptPayload(); break;
    case 3: test3_corruptHeader(); break;
    case 2: test2_multipleMessages(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
