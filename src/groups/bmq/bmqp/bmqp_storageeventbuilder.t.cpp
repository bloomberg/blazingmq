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

// bmqp_storageeventbuilder.t.cpp                                     -*-C++-*-
#include <bmqp_storageeventbuilder.h>

// BMQ
#include <bmqp_event.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_cstring.h>  // for bsl::strlen
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_memory.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

const int k_RECORD_SIZE = 48;  // Size of a journal record.  Must be 4byte
                               // aligned.

struct Data {
    // DATA
    bmqp::StorageMessageType::Enum d_messageType;

    int d_flags;

    int d_spv;
    // storage protocol version

    unsigned int d_pid;

    unsigned int d_journalOffsetWords;

    char d_recordBuffer[k_RECORD_SIZE];

    bsl::string d_payload;
    // empty unless d_messageType == e_DATA
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Data, bslma::UsesBslmaAllocator)

    // CREATORS
    Data(bslma::Allocator* allocator);
    Data(const Data& other, bslma::Allocator* allocator);
};

Data::Data(bslma::Allocator* allocator)
: d_payload(allocator)
{
    // NOTHING
}

Data::Data(const Data& other, bslma::Allocator* allocator)
: d_messageType(other.d_messageType)
, d_flags(other.d_flags)
, d_spv(other.d_spv)
, d_pid(other.d_pid)
, d_journalOffsetWords(other.d_journalOffsetWords)
, d_payload(other.d_payload, allocator)
{
    bsl::memcpy(d_recordBuffer, other.d_recordBuffer, k_RECORD_SIZE);
}

bmqt::EventBuilderResult::Enum appendMessage(unsigned int               index,
                                             bmqp::StorageEventBuilder* seb,
                                             bsl::vector<Data>* dataVec,
                                             bslma::Allocator*  allocator)
{
    Data D(allocator);

    unsigned int remainder = index % 6;
    if (0 == remainder) {
        D.d_messageType = bmqp::StorageMessageType::e_DATA;
    }
    else if (1 == remainder) {
        D.d_messageType = bmqp::StorageMessageType::e_QLIST;
    }
    else if (2 == remainder) {
        D.d_messageType = bmqp::StorageMessageType::e_CONFIRM;
    }
    else if (3 == remainder) {
        D.d_messageType = bmqp::StorageMessageType::e_DELETION;
    }
    else if (4 == remainder) {
        D.d_messageType = bmqp::StorageMessageType::e_QUEUE_OP;
    }
    else {
        D.d_messageType = bmqp::StorageMessageType::e_JOURNAL_OP;
    }

    D.d_flags              = index % 4;
    D.d_pid                = index;
    D.d_journalOffsetWords = index * 100 + 1;

    bsl::string recordBuffer(allocator);
    recordBuffer.resize(k_RECORD_SIZE, index % 256);
    bsl::memcpy(D.d_recordBuffer, recordBuffer.c_str(), k_RECORD_SIZE);

    if (bmqp::StorageMessageType::e_DATA == D.d_messageType ||
        bmqp::StorageMessageType::e_QLIST == D.d_messageType) {
        // Payload's size must be non zero and word aligned
        D.d_payload.resize(4 * (index + 1), 'A');
    }

    dataVec->push_back(D);

    Data& Dref = dataVec->back();

    bsl::shared_ptr<char> journalRecordBufferSp(Dref.d_recordBuffer,
                                                bslstl::SharedPtrNilDeleter(),
                                                allocator);
    bdlbb::BlobBuffer     journalRecordBlobBuffer(journalRecordBufferSp,
                                              k_RECORD_SIZE);

    if (bmqp::StorageMessageType::e_DATA == Dref.d_messageType ||
        bmqp::StorageMessageType::e_QLIST == Dref.d_messageType) {
        bsl::shared_ptr<char> dataBufferSp(
            const_cast<char*>(Dref.d_payload.c_str()),
            bslstl::SharedPtrNilDeleter(),
            allocator);
        bdlbb::BlobBuffer dataBlobBuffer(dataBufferSp,
                                         Dref.d_payload.length());

        return seb->packMessage(D.d_messageType,
                                D.d_pid,
                                D.d_flags,
                                D.d_journalOffsetWords,
                                journalRecordBlobBuffer,
                                dataBlobBuffer);
    }

    return seb->packMessage(D.d_messageType,
                            D.d_pid,
                            D.d_flags,
                            D.d_journalOffsetWords,
                            journalRecordBlobBuffer);
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
//   TODO
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    const char*                    PAYLOAD     = "abcdefghijklmnopqrstuvwx";
    const unsigned int             PAYLOAD_LEN = bsl::strlen(PAYLOAD);  // 24
    const char*                    JOURNAL_REC = "12345678";
    const unsigned int JOURNAL_REC_LEN = bsl::strlen(JOURNAL_REC);  // 8

    // Note that payload and journal record must be word aligned (per
    // StorageEventBuilder's contract)

    // Create StorageEventBuilder
    bmqp::StorageEventBuilder seb(1,  // storage protocol version
                                  bmqp::EventType::e_STORAGE,
                                  &blobSpPool,
                                  bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(seb.eventSize(), static_cast<int>(sizeof(bmqp::EventHeader)));
    ASSERT_EQ(seb.messageCount(), 0);

    bsl::shared_ptr<char> journalRecordBufferSp(
        const_cast<char*>(JOURNAL_REC),
        bslstl::SharedPtrNilDeleter(),
        bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobBuffer     journalRecordBlobBuffer(journalRecordBufferSp,
                                              JOURNAL_REC_LEN);

    bsl::shared_ptr<char> dataBufferSp(const_cast<char*>(PAYLOAD),
                                       bslstl::SharedPtrNilDeleter(),
                                       bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobBuffer     dataBlobBuffer(dataBufferSp, PAYLOAD_LEN);

    bmqt::EventBuilderResult::Enum rc = seb.packMessage(
        bmqp::StorageMessageType::e_DATA,
        1,     // partitionId
        1,     // flags
        5000,  // journalOffsetW
        journalRecordBlobBuffer,
        dataBlobBuffer);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
    ASSERT_LT(PAYLOAD_LEN, static_cast<unsigned int>(seb.eventSize()));
    ASSERT_EQ(seb.messageCount(), 1);

    // Get blob and use bmqp iterator to test
    // Note that bmqp event and bmqp iterators are lower than bmqp builders,
    // and thus, can be used to test them.
    const bdlbb::Blob& eventBlob = *seb.blob();
    bmqp::Event rawEvent(&eventBlob, bmqtst::TestHelperUtil::allocator());

    BSLS_ASSERT(true == rawEvent.isValid());
    BSLS_ASSERT(true == rawEvent.isStorageEvent());

    bmqp::StorageMessageIterator storageIter;
    rawEvent.loadStorageMessageIterator(&storageIter);

    ASSERT_EQ(storageIter.isValid(), true);
    ASSERT_EQ(storageIter.next(), 1);

    ASSERT_EQ(storageIter.header().flags(), 1);
    ASSERT_EQ(storageIter.header().storageProtocolVersion(), 1);
    ASSERT_EQ(storageIter.header().partitionId(), 1U);
    ASSERT_EQ(storageIter.header().journalOffsetWords(), 5000U);
    ASSERT_EQ(storageIter.header().messageType(),
              bmqp::StorageMessageType::e_DATA);

    bmqu::BlobPosition position;
    ASSERT_EQ(0, storageIter.loadDataPosition(&position));
    int res, compareResult;
    res = bmqu::BlobUtil::compareSection(&compareResult,
                                         eventBlob,
                                         position,
                                         JOURNAL_REC,
                                         JOURNAL_REC_LEN);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(compareResult, 0);

    bmqu::BlobPosition dataPos;
    ASSERT_EQ(0,
              bmqu::BlobUtil::findOffsetSafe(&dataPos,
                                             eventBlob,
                                             position,
                                             JOURNAL_REC_LEN));

    res = bmqu::BlobUtil::compareSection(&compareResult,
                                         eventBlob,
                                         dataPos,
                                         PAYLOAD,
                                         PAYLOAD_LEN);

    ASSERT_EQ(0, storageIter.next());  // we added only 1 msg
}

static void test2_storageEventHavingMultipleMessages()
// ------------------------------------------------------------------------
// STORAGE EVENT HAVING MULTIPLE MESSAGES
//
// Build an event with multiple STORAGE msgs. Iterate and verify
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("STORAGE EVENT HAVING MULTIPLE"
                                      " MESSAGES");

    const int                      k_SPV = 2;  // Storage protocol version
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));

    bmqp::StorageEventBuilder      seb(k_SPV,
                                  bmqp::EventType::e_STORAGE,
                                  &blobSpPool,
                                  bmqtst::TestHelperUtil::allocator());
    bsl::vector<Data>              data(bmqtst::TestHelperUtil::allocator());
    const size_t                   NUM_MSGS = 1000;
    data.reserve(NUM_MSGS);

    for (size_t dataIdx = 0; dataIdx < NUM_MSGS; ++dataIdx) {
        bmqt::EventBuilderResult::Enum rc = appendMessage(
            dataIdx,
            &seb,
            &data,
            bmqtst::TestHelperUtil::allocator());
        ASSERT_EQ_D(dataIdx, rc, bmqt::EventBuilderResult::e_SUCCESS);
    }

    // Iterate and check
    const bdlbb::Blob& eventBlob = *seb.blob();
    bmqp::Event rawEvent(&eventBlob, bmqtst::TestHelperUtil::allocator());

    BSLS_ASSERT(true == rawEvent.isValid());
    BSLS_ASSERT(true == rawEvent.isStorageEvent());

    bmqp::StorageMessageIterator iter;
    rawEvent.loadStorageMessageIterator(&iter);
    ASSERT_EQ(iter.isValid(), true);

    size_t dataIndex = 0;

    while (1 == iter.next() && dataIndex < NUM_MSGS) {
        const Data& D = data[dataIndex];

        ASSERT_EQ_D(dataIndex, true, iter.isValid());

        ASSERT_EQ_D(dataIndex, k_SPV, iter.header().storageProtocolVersion());

        ASSERT_EQ_D(dataIndex, D.d_flags, iter.header().flags());
        ASSERT_EQ_D(dataIndex, D.d_pid, iter.header().partitionId());
        ASSERT_EQ_D(dataIndex, D.d_messageType, iter.header().messageType());

        bmqu::BlobPosition recordPosition;
        ASSERT_EQ_D(dataIndex, 0, iter.loadDataPosition(&recordPosition));

        int res, compareResult;
        res = bmqu::BlobUtil::compareSection(&compareResult,
                                             eventBlob,
                                             recordPosition,
                                             D.d_recordBuffer,
                                             k_RECORD_SIZE);
        ASSERT_EQ_D(dataIndex, 0, res);
        ASSERT_EQ_D(dataIndex, 0, compareResult);

        if (bmqp::StorageMessageType::e_DATA == D.d_messageType ||
            bmqp::StorageMessageType::e_QLIST == D.d_messageType) {
            bmqu::BlobPosition payloadPos;
            ASSERT_EQ(0,
                      bmqu::BlobUtil::findOffsetSafe(&payloadPos,
                                                     eventBlob,
                                                     recordPosition,
                                                     k_RECORD_SIZE));
            res = bmqu::BlobUtil::compareSection(&compareResult,
                                                 eventBlob,
                                                 payloadPos,
                                                 D.d_payload.c_str(),
                                                 D.d_payload.size());
            ASSERT_EQ_D(dataIndex, 0, res);
            ASSERT_EQ_D(dataIndex, 0, compareResult);
        }

        ++dataIndex;
    }

    ASSERT_EQ(data.size(), dataIndex);
    ASSERT_EQ(iter.isValid(), false);
}

static void test3_packMessage_payloadTooBig()
// ------------------------------------------------------------------------
// PACK MESSAGE - PAYLOAD TOO BIG
//
// Concerns:
//   Test behavior when trying to build *one* big message.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PACK MESSAGE - PAYLOAD TOO BIG");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));

    const int          k_SPV               = 1;  // Storage proto version
    const char*        PAYLOAD             = "abcdefghijklmnopqrstuvwx";
    const unsigned int PAYLOAD_LEN         = bsl::strlen(PAYLOAD);  // 24
    const char*        JOURNAL_REC         = "12345678";
    const unsigned int JOURNAL_REC_LEN     = bsl::strlen(JOURNAL_REC);  // 8
    const char*        IGNORED_JOURNAL_REC = "abcedfgh";                // 8

    bmqp::StorageEventBuilder seb(k_SPV,
                                  bmqp::EventType::e_STORAGE,
                                  &blobSpPool,
                                  bmqtst::TestHelperUtil::allocator());
    bsl::string               bigPayload(bmqtst::TestHelperUtil::allocator());
    bigPayload.resize(bmqp::StorageHeader::k_MAX_PAYLOAD_SIZE_SOFT + 4, 'a');
    // Note that payload's size must be word aligned.

    bsl::shared_ptr<char> journalRecordBufferSp(
        const_cast<char*>(IGNORED_JOURNAL_REC),
        bslstl::SharedPtrNilDeleter(),
        bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobBuffer journalRecordBlobBuffer(
        journalRecordBufferSp,
        bsl::strlen(IGNORED_JOURNAL_REC));

    bsl::shared_ptr<char> dataBufferSp(const_cast<char*>(bigPayload.c_str()),
                                       bslstl::SharedPtrNilDeleter(),
                                       bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobBuffer     dataBlobBuffer(dataBufferSp, bigPayload.length());

    bmqt::EventBuilderResult::Enum rc = seb.packMessage(
        bmqp::StorageMessageType::e_DATA,
        1,    // partitionId
        2,    // flags
        450,  // journalOffsetWords
        journalRecordBlobBuffer,
        dataBlobBuffer);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_PAYLOAD_TOO_BIG);
    ASSERT_EQ(seb.eventSize(), static_cast<int>(sizeof(bmqp::EventHeader)));
    ASSERT_EQ(seb.messageCount(), 0);

    journalRecordBufferSp.reset(const_cast<char*>(JOURNAL_REC),
                                bslstl::SharedPtrNilDeleter(),
                                bmqtst::TestHelperUtil::allocator());
    journalRecordBlobBuffer.reset(journalRecordBufferSp, JOURNAL_REC_LEN);

    dataBufferSp.reset(const_cast<char*>(PAYLOAD),
                       bslstl::SharedPtrNilDeleter(),
                       bmqtst::TestHelperUtil::allocator());
    dataBlobBuffer.reset(dataBufferSp, PAYLOAD_LEN);

    // Now append a "regular"-sized message and make sure event builder
    // behaves as expected
    rc = seb.packMessage(bmqp::StorageMessageType::e_DATA,
                         1,    // partitionId
                         2,    // flags
                         290,  // journalOffsetWords
                         journalRecordBlobBuffer,
                         dataBlobBuffer);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
    ASSERT_EQ(sizeof(bmqp::EventHeader) + sizeof(bmqp::StorageHeader) +
                  JOURNAL_REC_LEN + PAYLOAD_LEN,
              static_cast<unsigned int>(seb.eventSize()));
    ASSERT_EQ(seb.messageCount(), 1);

    const bdlbb::Blob& eventBlob = *seb.blob();
    bmqp::Event rawEvent(&eventBlob, bmqtst::TestHelperUtil::allocator());

    BSLS_ASSERT(true == rawEvent.isValid());
    BSLS_ASSERT(true == rawEvent.isStorageEvent());

    bmqp::StorageMessageIterator storageIter;
    rawEvent.loadStorageMessageIterator(&storageIter);

    ASSERT_EQ(storageIter.isValid(), true);
    ASSERT_EQ(storageIter.next(), 1);

    ASSERT_EQ(storageIter.header().flags(), 2);
    ASSERT_EQ(storageIter.header().storageProtocolVersion(), 1);
    ASSERT_EQ(storageIter.header().partitionId(), 1U);
    ASSERT_EQ(storageIter.header().journalOffsetWords(), 290U);
    ASSERT_EQ(storageIter.header().messageType(),
              bmqp::StorageMessageType::e_DATA);

    bmqu::BlobPosition position;
    ASSERT_EQ(storageIter.loadDataPosition(&position), 0);
    int res, compareResult;
    res = bmqu::BlobUtil::compareSection(&compareResult,
                                         eventBlob,
                                         position,
                                         JOURNAL_REC,
                                         JOURNAL_REC_LEN);
    ASSERT_EQ(0, res);
    ASSERT_EQ(0, compareResult);

    bmqu::BlobPosition dataPos;
    ASSERT_EQ(0,
              bmqu::BlobUtil::findOffsetSafe(&dataPos,
                                             eventBlob,
                                             position,
                                             JOURNAL_REC_LEN));

    res = bmqu::BlobUtil::compareSection(&compareResult,
                                         eventBlob,
                                         dataPos,
                                         PAYLOAD,
                                         PAYLOAD_LEN);

    ASSERT_EQ(0, res);
    ASSERT_EQ(false, storageIter.next());  // we added only 1 msg
}

static void test4_packMessageRaw()
// ------------------------------------------------------------------------
// PACK MESSAGE RAW
//
// Concerns:
//   Test behavior when trying to build an event containing multiple
//   messages, all added via 'packMessageRaw'.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PACK MESSAGE RAW");

    // We will build an event (event 'A') contain multiple messages using
    // 'packMessage' which has been tested in case 2.  We will then iterate
    // over this event and create a new event (event 'B') using
    // 'packMessageRaw', leveraging the header position and length of each
    // message. Finally, we will iterate event 'B' and verify it against the
    // messages packed in event 'A'.

    const int                      k_SPV = 2;  // Storage protocol version
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::StorageEventBuilder      sebA(k_SPV,
                                   bmqp::EventType::e_STORAGE,
                                   &blobSpPool,
                                   bmqtst::TestHelperUtil::allocator());
    bsl::vector<Data>              data(bmqtst::TestHelperUtil::allocator());
    const size_t                   NUM_MSGS = 1000;
    data.reserve(NUM_MSGS);

    for (size_t dataIdx = 0; dataIdx < NUM_MSGS; ++dataIdx) {
        bmqt::EventBuilderResult::Enum rc = appendMessage(
            dataIdx,
            &sebA,
            &data,
            bmqtst::TestHelperUtil::allocator());
        ASSERT_EQ_D(dataIdx, rc, bmqt::EventBuilderResult::e_SUCCESS);
    }

    const bdlbb::Blob& eventA = *sebA.blob();
    bmqp::Event        rawEventA(&eventA, bmqtst::TestHelperUtil::allocator());
    BSLS_ASSERT(rawEventA.isValid() == true);
    BSLS_ASSERT(rawEventA.isStorageEvent() == true);

    bmqp::StorageMessageIterator iterA;
    rawEventA.loadStorageMessageIterator(&iterA);
    ASSERT_EQ(iterA.isValid(), true);

    size_t dataIndex = 0;

    // Iterate over event 'A', and create event 'B' using 'packMessageRaw'.
    bmqp::StorageEventBuilder sebB(k_SPV,
                                   bmqp::EventType::e_STORAGE,
                                   &blobSpPool,
                                   bmqtst::TestHelperUtil::allocator());

    while (1 == iterA.next() && dataIndex < NUM_MSGS) {
        ASSERT_EQ_D(dataIndex, true, iterA.isValid());

        const bmqp::StorageHeader& header = iterA.header();

        bmqt::EventBuilderResult::Enum buildRc = sebB.packMessageRaw(
            eventA,
            iterA.headerPosition(),
            header.messageWords() * bmqp::Protocol::k_WORD_SIZE);

        ASSERT_EQ_D(dataIndex, buildRc, bmqt::EventBuilderResult::e_SUCCESS);

        ++dataIndex;
    }

    ASSERT_EQ(data.size(), dataIndex);
    ASSERT_EQ(iterA.isValid(), false);

    // Finally, iterate over event 'B' and verify.
    const bdlbb::Blob& eventB = *sebB.blob();
    bmqp::Event        rawEventB(&eventB, bmqtst::TestHelperUtil::allocator());

    BSLS_ASSERT(true == rawEventB.isValid());
    BSLS_ASSERT(true == rawEventB.isStorageEvent());

    bmqp::StorageMessageIterator iterB;
    rawEventB.loadStorageMessageIterator(&iterB);
    ASSERT_EQ(iterB.isValid(), true);

    dataIndex = 0;

    while (1 == iterB.next() && dataIndex < NUM_MSGS) {
        const Data& D = data[dataIndex];

        ASSERT_EQ_D(dataIndex, true, iterB.isValid());
        ASSERT_EQ_D(dataIndex, k_SPV, iterB.header().storageProtocolVersion());

        ASSERT_EQ_D(dataIndex, D.d_flags, iterB.header().flags());
        ASSERT_EQ_D(dataIndex, D.d_pid, iterB.header().partitionId());

        ASSERT_EQ_D(dataIndex, D.d_messageType, iterB.header().messageType());

        bmqu::BlobPosition recordPosition;
        ASSERT_EQ_D(dataIndex, 0, iterB.loadDataPosition(&recordPosition));

        int res, compareResult;
        res = bmqu::BlobUtil::compareSection(&compareResult,
                                             eventB,
                                             recordPosition,
                                             D.d_recordBuffer,
                                             k_RECORD_SIZE);
        ASSERT_EQ_D(dataIndex, 0, res);
        ASSERT_EQ_D(dataIndex, 0, compareResult);

        if (bmqp::StorageMessageType::e_DATA == D.d_messageType ||
            bmqp::StorageMessageType::e_QLIST == D.d_messageType) {
            bmqu::BlobPosition payloadPos;
            ASSERT_EQ(0,
                      bmqu::BlobUtil::findOffsetSafe(&payloadPos,
                                                     eventB,
                                                     recordPosition,
                                                     k_RECORD_SIZE));
            res = bmqu::BlobUtil::compareSection(&compareResult,
                                                 eventB,
                                                 payloadPos,
                                                 D.d_payload.c_str(),
                                                 D.d_payload.size());
            ASSERT_EQ_D(dataIndex, 0, res);
            ASSERT_EQ_D(dataIndex, 0, compareResult);
        }

        ++dataIndex;
    }

    ASSERT_EQ(data.size(), dataIndex);
    ASSERT_EQ(iterB.isValid(), false);
}

static void test5_packMessageRaw_emptyMessage()
// ------------------------------------------------------------------------
// PACK MESSAGE RAW - EMPTY MESSAGE
//
// Concerns:
//   1. Test behavior when trying to build an event by packing an empty
//      message using 'packMessageRaw'.
//
// Plan:
//   1. Attempt to pack an empty message and verify success.
//
// Testing:
//   packMessageRaw
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PACK MESSAGE RAW - EMPTY MESSAGE");

    const int                      k_SPV = 2;  // Storage protocol version
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::StorageEventBuilder seb(k_SPV,
                                  bmqp::EventType::e_STORAGE,
                                  &blobSpPool,
                                  bmqtst::TestHelperUtil::allocator());

    bdlbb::Blob emptyBlob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    BSLS_ASSERT_OPT(emptyBlob.length() == 0);
    ASSERT_SAFE_FAIL(
        seb.packMessageRaw(emptyBlob, bmqu::BlobPosition(0, 0), 0));
    ASSERT_EQ(seb.packMessageRaw(emptyBlob, bmqu::BlobPosition(0, 0), 10),
              bmqt::EventBuilderResult::e_SUCCESS);
    // Above we packMessageRaw with 'length' of 10 because we need an
    // arbitrary 'length > 0' to not trigger an assert and at the same time
    // ensure that packing an empty blob succeeds.
    ASSERT_EQ(bdlbb::BlobUtil::compare(*seb.blob(), emptyBlob), 0);
    ASSERT_EQ(seb.messageCount(), 0);
    ASSERT_EQ(seb.eventSize(), static_cast<int>(sizeof(bmqp::EventHeader)));
}

static void test6_packMessageRaw_invalidPosition()
// ------------------------------------------------------------------------
// PACK MESSAGE RAW - INVALID POSITION
//
// Concerns:
//   1. Test behavior when trying to build an event by packing a message
//      and specifying an invalid start position.
//
// Plan:
//   1. Attempt to pack a message starting at an invalid start position and
//      verify failure.
//
// Testing:
//   packMessageRaw
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PACK MESSAGE RAW - INVALID POSITION");

    const int                      k_SPV = 2;  // Storage protocol version
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bdlbb::Blob               k_EMPTY_BLOB(&bufferFactory,
                             bmqtst::TestHelperUtil::allocator());
    bmqp::StorageEventBuilder seb(k_SPV,
                                  bmqp::EventType::e_STORAGE,
                                  &blobSpPool,
                                  bmqtst::TestHelperUtil::allocator());

    // 1.
    bsl::string payloadStr(1024U, 'x', bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<char> dataBufferSp(const_cast<char*>(payloadStr.c_str()),
                                       bslstl::SharedPtrNilDeleter(),
                                       bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobBuffer     dataBlobBuffer(dataBufferSp, payloadStr.length());

    bdlbb::Blob message(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    message.appendDataBuffer(dataBlobBuffer);

    bmqu::BlobPosition invalidPosition(-1, -1);
    ASSERT_NE(seb.packMessageRaw(message, invalidPosition, message.length()),
              bmqt::EventBuilderResult::e_SUCCESS);
    ASSERT_EQ(bdlbb::BlobUtil::compare(*seb.blob(), k_EMPTY_BLOB), 0);
    ASSERT_EQ(seb.messageCount(), 0);
    ASSERT_EQ(seb.eventSize(), static_cast<int>(sizeof(bmqp::EventHeader)));
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    // Temporary workaround to suppress the 'unused operator
    // NestedTraitDeclaration' warning/error generated by clang.  TBD: figure
    // out the right way to "fix" this.
    Data dummy(bmqtst::TestHelperUtil::allocator());
    static_cast<void>(
        static_cast<
            bslmf::NestedTraitDeclaration<Data, bslma::UsesBslmaAllocator> >(
            dummy));

    switch (_testCase) {
    case 0:
    case 6: test6_packMessageRaw_invalidPosition(); break;
    case 5: test5_packMessageRaw_emptyMessage(); break;
    case 4: test4_packMessageRaw(); break;
    case 3: test3_packMessage_payloadTooBig(); break;
    case 2: test2_storageEventHavingMultipleMessages(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
