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

// bmqp_recoveryeventbuilder.t.cpp                                    -*-C++-*-
#include <bmqp_recoveryeventbuilder.h>

// BMQ
#include <bmqp_event.h>

// MWC
#include <mwcu_blob.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlde_md5.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

const int k_MD5_DIGEST_LEN = 16;

void populateMd5Digest(char* digest, const char* data, int length)
{
    bdlde::Md5::Md5Digest md5Digest;
    bdlde::Md5            md5Hasher(data, length);
    md5Hasher.loadDigest(&md5Digest);
    bsl::memcpy(digest, md5Digest.buffer(), k_MD5_DIGEST_LEN);
}

struct Data {
    // DATA
    unsigned int d_pid;

    bmqp::RecoveryFileChunkType::Enum d_chunkFileType;

    unsigned int d_seqNum;

    bsl::string d_chunk;

    bool d_isFinalChunk;

    char d_md5Digest[k_MD5_DIGEST_LEN];
};

bmqt::EventBuilderResult::Enum appendMessage(unsigned int                index,
                                             bmqp::RecoveryEventBuilder* reb,
                                             bsl::vector<Data>* dataVec,
                                             bool               isFinalChunk)
{
    Data D;

    D.d_pid           = index % 5;
    D.d_chunkFileType = bmqp::RecoveryFileChunkType::e_DATA;
    D.d_seqNum        = index;
    D.d_isFinalChunk  = isFinalChunk;

    // Chunk's size must be non zero and word aligned
    D.d_chunk.resize(4 * (index + 1), 'A');

    populateMd5Digest(D.d_md5Digest, D.d_chunk.c_str(), D.d_chunk.length());

    dataVec->push_back(D);

    Data&                 Dref = dataVec->back();
    bsl::shared_ptr<char> chunkBufferSp(
        const_cast<char*>(Dref.d_chunk.c_str()),
        bslstl::SharedPtrNilDeleter());
    bdlbb::BlobBuffer chunkBlobBuffer(chunkBufferSp, Dref.d_chunk.length());

    return reb->packMessage(D.d_pid,
                            D.d_chunkFileType,
                            D.d_seqNum,
                            chunkBlobBuffer,
                            D.d_isFinalChunk);
}

}  // close unnamed namespace

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
    mwctst::TestHelper::printTestName("BREATHING TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    const char*                    CHUNK = "abcdefghijklmnopqrstuvwx";

    // Note that chunk must be word aligned per RecoveryEventBuilder's
    // contract.
    const unsigned int CHUNK_LEN = bsl::strlen(CHUNK);  // 24

    char md5Digest[k_MD5_DIGEST_LEN];
    populateMd5Digest(md5Digest, CHUNK, CHUNK_LEN);

    // Create RecoveryEventBuilder.

    bmqp::RecoveryEventBuilder reb(&bufferFactory, s_allocator_p);
    ASSERT_EQ(sizeof(bmqp::EventHeader), static_cast<size_t>(reb.eventSize()));
    ASSERT_EQ(reb.messageCount(), 0);

    bsl::shared_ptr<char> chunkBufferSp(const_cast<char*>(CHUNK),
                                        bslstl::SharedPtrNilDeleter());
    bdlbb::BlobBuffer     chunkBlobBuffer(chunkBufferSp, CHUNK_LEN);

    bmqt::EventBuilderResult::Enum rc = reb.packMessage(
        1,  // partitionId
        bmqp::RecoveryFileChunkType::e_DATA,
        1000,  // sequenceNum
        chunkBlobBuffer,
        false);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
    ASSERT_LT(CHUNK_LEN, static_cast<unsigned int>(reb.eventSize()));
    ASSERT_EQ(reb.messageCount(), 1);

    // Get blob and use bmqp iterator to test.  Note that bmqp event and bmqp
    // iterators are lower than bmqp builders, and thus, can be used to test
    // them.

    const bdlbb::Blob& eventBlob = reb.blob();
    bmqp::Event        rawEvent(&eventBlob, s_allocator_p);

    BSLS_ASSERT(true == rawEvent.isValid());
    BSLS_ASSERT(true == rawEvent.isRecoveryEvent());

    bmqp::RecoveryMessageIterator recoveryIter;
    rawEvent.loadRecoveryMessageIterator(&recoveryIter);

    ASSERT_EQ(recoveryIter.isValid(), true);
    ASSERT_EQ(recoveryIter.next(), 1);

    ASSERT_EQ(recoveryIter.header().isFinalChunk(), false);
    ASSERT_EQ(recoveryIter.header().partitionId(), 1U);
    ASSERT_EQ(recoveryIter.header().chunkSequenceNumber(), 1000U);

    ASSERT_EQ(0,
              bsl::memcmp(md5Digest,
                          recoveryIter.header().md5Digest(),
                          k_MD5_DIGEST_LEN));
    ASSERT_EQ(bmqp::RecoveryFileChunkType::e_DATA,
              recoveryIter.header().fileChunkType());
    ASSERT_EQ((sizeof(bmqp::RecoveryHeader) / bmqp::Protocol::k_WORD_SIZE),
              static_cast<size_t>(recoveryIter.header().headerWords()));

    mwcu::BlobPosition position;
    ASSERT_EQ(recoveryIter.loadChunkPosition(&position), 0);
    int res, compareResult;
    res = mwcu::BlobUtil::compareSection(&compareResult,
                                         eventBlob,
                                         position,
                                         CHUNK,
                                         CHUNK_LEN);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(compareResult, 0);
    ASSERT_EQ(recoveryIter.next(), 0);  // we added only 1 msg
}

static void test2_multipleMessagesTest()
// ------------------------------------------------------------------------
// MULTIPLE MESSAGES TEST
//
// Concerns:
//
// Plan:
//    Build an event with multiple STORAGE msgs.  Iterate and verify.
//
// Testing:
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("MULTIPLE MESSAGES TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    bmqp::RecoveryEventBuilder     reb(&bufferFactory, s_allocator_p);
    bsl::vector<Data>              data(s_allocator_p);
    const size_t                   NUM_MSGS = 1000;
    data.reserve(NUM_MSGS);

    for (size_t dataIdx = 0; dataIdx < NUM_MSGS; ++dataIdx) {
        bool isFinalChunk = (dataIdx == (NUM_MSGS - 1)) ? true : false;

        bmqt::EventBuilderResult::Enum rc =
            appendMessage(dataIdx, &reb, &data, isFinalChunk);
        ASSERT_EQ_D(dataIdx, rc, bmqt::EventBuilderResult::e_SUCCESS);
    }

    // Iterate and check
    const bdlbb::Blob& eventBlob = reb.blob();
    bmqp::Event        rawEvent(&eventBlob, s_allocator_p);

    BSLS_ASSERT(true == rawEvent.isValid());
    BSLS_ASSERT(true == rawEvent.isRecoveryEvent());

    bmqp::RecoveryMessageIterator iter;
    rawEvent.loadRecoveryMessageIterator(&iter);
    ASSERT_EQ(true, iter.isValid());

    size_t dataIndex = 0;

    while (1 == iter.next() && dataIndex < NUM_MSGS) {
        const Data& D = data[dataIndex];

        ASSERT_EQ_D(dataIndex, true, iter.isValid());
        ASSERT_EQ_D(dataIndex, D.d_isFinalChunk, iter.header().isFinalChunk());
        ASSERT_EQ_D(dataIndex, D.d_pid, iter.header().partitionId());

        ASSERT_EQ_D(dataIndex,
                    D.d_seqNum,
                    iter.header().chunkSequenceNumber());

        ASSERT_EQ_D(dataIndex,
                    D.d_chunkFileType,
                    iter.header().fileChunkType());

        ASSERT_EQ_D(dataIndex,
                    0,
                    bsl::memcmp(D.d_md5Digest,
                                iter.header().md5Digest(),
                                k_MD5_DIGEST_LEN));

        mwcu::BlobPosition chunkPosition;
        ASSERT_EQ_D(dataIndex, 0, iter.loadChunkPosition(&chunkPosition));

        int res, compareResult;
        res = mwcu::BlobUtil::compareSection(&compareResult,
                                             eventBlob,
                                             chunkPosition,
                                             D.d_chunk.c_str(),
                                             D.d_chunk.size());
        ASSERT_EQ_D(dataIndex, 0, res);
        ASSERT_EQ_D(dataIndex, 0, compareResult);

        ++dataIndex;
    }

    ASSERT_EQ(dataIndex, data.size());
    ASSERT_EQ(false, iter.isValid());
}

static void test3_eventTooBigTest()
// ------------------------------------------------------------------------
// EVENT TOO BIG TEST
//
// Concerns:
//   Test behavior when trying to build *one* big event.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("EVENT TOO BIG TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    bmqp::RecoveryEventBuilder     reb(&bufferFactory, s_allocator_p);
    bsl::string                    bigChunk(s_allocator_p);
    bigChunk.resize(bmqp::RecoveryHeader::k_MAX_PAYLOAD_SIZE_SOFT + 4, 'a');
    // Note that chunk's size must be word aligned.

    const char*  k_SMALL_CHUNK     = "abcdefghijkl";
    const size_t k_SMALL_CHUNK_LEN = bsl::strlen(k_SMALL_CHUNK);
    char         md5Digest[k_MD5_DIGEST_LEN];
    populateMd5Digest(md5Digest, k_SMALL_CHUNK, k_SMALL_CHUNK_LEN);

    bsl::shared_ptr<char> chunkBufferSp(const_cast<char*>(bigChunk.c_str()),
                                        bslstl::SharedPtrNilDeleter());
    bdlbb::BlobBuffer     chunkBlobBuffer(chunkBufferSp, bigChunk.size());

    bmqt::EventBuilderResult::Enum rc = reb.packMessage(
        1,  // PartitionId
        bmqp::RecoveryFileChunkType::e_DATA,
        1000,  // SequenceNum
        chunkBlobBuffer,
        true);  // IsFinalChunk

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_PAYLOAD_TOO_BIG);
    ASSERT_EQ(static_cast<int>(sizeof(bmqp::EventHeader)), reb.eventSize());
    ASSERT_EQ(0, reb.messageCount());

    chunkBufferSp.reset(const_cast<char*>(k_SMALL_CHUNK),
                        bslstl::SharedPtrNilDeleter());
    chunkBlobBuffer.reset(chunkBufferSp, k_SMALL_CHUNK_LEN);

    // Now append a "regular"-sized message and make sure event builder
    // behaves as expected
    rc = reb.packMessage(1,  // PartitionId
                         bmqp::RecoveryFileChunkType::e_DATA,
                         1000,  // SequenceNum
                         chunkBlobBuffer,
                         true);  // IsFinalChunk

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
    ASSERT_EQ(sizeof(bmqp::EventHeader) + sizeof(bmqp::RecoveryHeader) +
                  k_SMALL_CHUNK_LEN,
              static_cast<unsigned int>(reb.eventSize()));
    ASSERT_EQ(reb.messageCount(), 1);

    const bdlbb::Blob& eventBlob = reb.blob();
    bmqp::Event        rawEvent(&eventBlob, s_allocator_p);

    BSLS_ASSERT(true == rawEvent.isValid());
    BSLS_ASSERT(true == rawEvent.isRecoveryEvent());

    bmqp::RecoveryMessageIterator recoveryIter;
    rawEvent.loadRecoveryMessageIterator(&recoveryIter);

    ASSERT_EQ(recoveryIter.isValid(), true);
    ASSERT_EQ(recoveryIter.next(), 1);

    ASSERT_EQ(recoveryIter.header().isFinalChunk(), true);
    ASSERT_EQ(recoveryIter.header().partitionId(), 1U);
    ASSERT_EQ(recoveryIter.header().chunkSequenceNumber(), 1000U);
    ASSERT_EQ(bsl::memcmp(md5Digest,
                          recoveryIter.header().md5Digest(),
                          k_MD5_DIGEST_LEN),
              0);
    ASSERT_EQ(bmqp::RecoveryFileChunkType::e_DATA,
              recoveryIter.header().fileChunkType());

    mwcu::BlobPosition position;
    ASSERT_EQ(recoveryIter.loadChunkPosition(&position), 0);
    int res, compareResult;
    res = mwcu::BlobUtil::compareSection(&compareResult,
                                         eventBlob,
                                         position,
                                         k_SMALL_CHUNK,
                                         k_SMALL_CHUNK_LEN);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(compareResult, 0);
    ASSERT_EQ(recoveryIter.next(), false);  // we added only 1 msg
}

static void test4_emptyPayloadTest()
// ------------------------------------------------------------------------
// EMPTY PAYLOAD TEST
//
// Concerns:
//   Trying to build an event where the message has empty payload.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("EMPTY PAYLOAD TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    bmqp::RecoveryEventBuilder     reb(&bufferFactory, s_allocator_p);

    bsl::shared_ptr<char> chunkBufferSp(
        reinterpret_cast<char*>(&reb),  // dummy
        bslstl::SharedPtrNilDeleter());
    bdlbb::BlobBuffer chunkBlobBuffer(chunkBufferSp, 0);

    bmqt::EventBuilderResult::Enum rc = reb.packMessage(
        1,  // PartitionId
        bmqp::RecoveryFileChunkType::e_DATA,
        1000,  // SequenceNum
        chunkBlobBuffer,
        false);  // IsNotFinalChunk

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
    // ASSERT_EQ(sizeof(bmqp::EventHeader) +
    //           sizeof(bmqp::RecoveryHeader),
    //           static_cast<unsigned int>(reb.eventSize()));
    ASSERT_EQ(reb.messageCount(), 1);

    const bdlbb::Blob& eventBlob = reb.blob();
    bmqp::Event        rawEvent(&eventBlob, s_allocator_p);

    BSLS_ASSERT(true == rawEvent.isValid());
    BSLS_ASSERT(true == rawEvent.isRecoveryEvent());

    bmqp::RecoveryMessageIterator recoveryIter;
    rawEvent.loadRecoveryMessageIterator(&recoveryIter);

    ASSERT_EQ(recoveryIter.isValid(), true);
    ASSERT_EQ(recoveryIter.next(), 1);

    ASSERT_EQ(recoveryIter.header().isFinalChunk(), false);
    ASSERT_EQ(recoveryIter.header().partitionId(), 1U);
    ASSERT_EQ(recoveryIter.header().chunkSequenceNumber(), 1000U);
    ASSERT_EQ(recoveryIter.header().fileChunkType(),
              bmqp::RecoveryFileChunkType::e_DATA);

    mwcu::BlobPosition position;
    ASSERT_EQ(recoveryIter.loadChunkPosition(&position), 0);

    ASSERT_EQ(recoveryIter.next(), 0);  // we added only 1 msg
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 4: test4_emptyPayloadTest(); break;
    case 3: test3_eventTooBigTest(); break;
    case 2: test2_multipleMessagesTest(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_GBL_ALLOC);
}
