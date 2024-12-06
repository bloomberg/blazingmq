// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqp_confirmeventbuilder.t.cpp                                     -*-C++-*-
#include <bmqp_confirmeventbuilder.h>

// BMQ
#include <bmqp_confirmmessageiterator.h>
#include <bmqp_event.h>
#include <bmqt_messageguid.h>
#include <bmqt_resultcode.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_guid.h>
#include <bdlb_guidutil.h>
#include <bdlb_scopeexit.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bsl_fstream.h>
#include <bsl_vector.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// struct representing the parameters that
/// `ConfirmEventBuilder::appendMessage` takes.
struct Data {
    int               d_queueId;
    bmqt::MessageGUID d_guid;
    int               d_subQueueId;
};

/// Append the specified `numMsgs` messages to the specified `builder` and
/// populate the specified `vec` with the messages that were added, so that
/// the caller can correlate the builder's data to what was added.
static void appendMessages(bmqp::ConfirmEventBuilder* builder,
                           bsl::vector<Data>*         vec,
                           size_t                     numMsgs)
{
    // Above hex string represents a valid guid with these values:
    //   TS = bdlb::BigEndianUint64::make(12345)
    //   IP = 98765
    //   ID = 9999
    static const char s_HEX_REP[] = "0000000000003039CD8101000000270F";

    vec->reserve(numMsgs);

    for (size_t i = 0; i < numMsgs; ++i) {
        Data data;
        data.d_queueId    = i * 3;
        data.d_subQueueId = i * 3 + 1;
        data.d_guid.fromHex(s_HEX_REP);
        int rc = builder->appendMessage(data.d_queueId,
                                        data.d_subQueueId,
                                        data.d_guid);
        BMQTST_ASSERT_EQ(rc, 0);
        vec->push_back(data);
    }
}

/// Use a ConfirmMessageIterator to verify that the specified `builder`
/// contains the messages represented in the specified `data`.  Note that
/// bmqp::Event and bmqp::ConfirmMessageIterator are lower than
/// bmqp::ConfirmEventBuilder and thus, can be used to test it.
static void verifyContent(const bmqp::ConfirmEventBuilder& builder,
                          const bsl::vector<Data>&         data)
{
    PVV("Verifying accessors");
    size_t expectedSize = sizeof(bmqp::EventHeader) +
                          sizeof(bmqp::ConfirmHeader) +
                          (data.size() * sizeof(bmqp::ConfirmMessage));
    BMQTST_ASSERT_EQ(static_cast<size_t>(builder.messageCount()), data.size());
    BMQTST_ASSERT_EQ(static_cast<size_t>(builder.eventSize()), expectedSize);
    BMQTST_ASSERT_EQ(static_cast<size_t>(builder.blob()->length()),
                     expectedSize);

    PVV("Iterating over messages");
    bmqp::Event event(builder.blob().get(),
                      bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(event.isValid(), true);
    BMQTST_ASSERT_EQ(event.isConfirmEvent(), true);

    bmqp::ConfirmMessageIterator iter;
    event.loadConfirmMessageIterator(&iter);

    BMQTST_ASSERT_EQ(iter.isValid(), true);

    size_t idx = 0;
    while (iter.next() == 1 && idx < data.size()) {
        const Data& d = data[idx];

        BMQTST_ASSERT_EQ_D(idx, iter.isValid(), true);
        BMQTST_ASSERT_EQ_D(idx, d.d_queueId, iter.message().queueId());
        BMQTST_ASSERT_EQ_D(idx, d.d_subQueueId, iter.message().subQueueId());
        BMQTST_ASSERT_EQ_D(idx, d.d_guid, iter.message().messageGUID());

        ++idx;
    }

    BMQTST_ASSERT_EQ(idx, data.size());
    BMQTST_ASSERT_EQ(iter.isValid(), false);
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        256,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::ConfirmEventBuilder obj(&blobSpPool,
                                  bmqtst::TestHelperUtil::allocator());
    bsl::vector<Data>         messages(bmqtst::TestHelperUtil::allocator());

    PVV("Verifying accessors");
    BMQTST_ASSERT_EQ(obj.messageCount(), 0);
    BMQTST_ASSERT_NE(obj.maxMessageCount(), 0);
    BMQTST_ASSERT_EQ(obj.eventSize(), 0);
    BMQTST_ASSERT_EQ(obj.blob()->length(), 0);

    PVV("Appending one message");
    appendMessages(&obj, &messages, 1);

    PVV("Verifying content");
    verifyContent(obj, messages);
}

static void test2_multiMessage()
{
    bmqtst::TestHelper::printTestName("MULTI MESSAGE");
    // Create a ConfirmEvent with multiple messages.  Iterate and verify.

    const int k_NUM_MSGS = 1000;

    bdlbb::PooledBlobBufferFactory bufferFactory(
        256,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::ConfirmEventBuilder obj(&blobSpPool,
                                  bmqtst::TestHelperUtil::allocator());
    bsl::vector<Data>         messages(bmqtst::TestHelperUtil::allocator());

    PVV("Appending messages");
    appendMessages(&obj, &messages, k_NUM_MSGS);

    PVV("Verifying content");
    verifyContent(obj, messages);
}

static void test3_reset()
{
    bmqtst::TestHelper::printTestName("RESET");
    // Verifying reset: add two messages, reset, and add another message.

    bdlbb::PooledBlobBufferFactory bufferFactory(
        256,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::ConfirmEventBuilder obj(&blobSpPool,
                                  bmqtst::TestHelperUtil::allocator());
    bsl::vector<Data>         messages(bmqtst::TestHelperUtil::allocator());

    PV("Appending 3 messages");
    appendMessages(&obj, &messages, 3);

    PV("Resetting the builder");
    obj.reset();

    PV("Verifying accessors");
    BMQTST_ASSERT_EQ(obj.messageCount(), 0);
    BMQTST_ASSERT_EQ(obj.eventSize(), 0);
    BMQTST_ASSERT_EQ(obj.blob()->length(), 0);

    PV("Appending another message");
    messages.clear();
    appendMessages(&obj, &messages, 1);

    PV("Verifying content");
    verifyContent(obj, messages);
}

static void test4_capacity()
{
    bmqtst::TestHelper::printTestName("CAPACITY");
    // Verify that once the event is full, AppendMessage returns error.

    int                            rc;
    bdlbb::PooledBlobBufferFactory bufferFactory(
        256,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::ConfirmEventBuilder obj(&blobSpPool,
                                  bmqtst::TestHelperUtil::allocator());

    PVV("Computing max message");
    // Compute max message using a different logic than how it is done in
    // 'ConfirmEventBuilder'.
    int maxMsgCount      = 0;
    int currentEventSize = sizeof(bmqp::EventHeader) +
                           sizeof(bmqp::ConfirmHeader);
    while ((currentEventSize + sizeof(bmqp::ConfirmMessage)) <=
           bmqp::EventHeader::k_MAX_SIZE_SOFT) {
        ++maxMsgCount;
        currentEventSize += sizeof(bmqp::ConfirmMessage);
    }
    PV("MaxMessageCount: " << maxMsgCount);
    BMQTST_ASSERT_EQ(obj.maxMessageCount(), maxMsgCount);

    PVV("Filling up ConfirmEventBuilder");
    // Fill the builder with that maximum number of messages, all additions
    // should succeed.
    while (maxMsgCount--) {
        rc = obj.appendMessage(0, 0, bmqt::MessageGUID());
        BMQTST_ASSERT_EQ(rc, 0);
        BMQTST_ASSERT(obj.eventSize() <= bmqp::EventHeader::k_MAX_SIZE_SOFT);
    }

    BMQTST_ASSERT_EQ(obj.messageCount(), obj.maxMessageCount());

    PVV("Append a one-too-much message to the ConfirmEventBuilder");
    // This message should fail since the builder is full.
    rc = obj.appendMessage(0, 0, bmqt::MessageGUID());
    BMQTST_ASSERT_EQ(
        rc,
        static_cast<int>(bmqt::EventBuilderResult::e_EVENT_TOO_BIG));
    BMQTST_ASSERT(obj.eventSize() <= bmqp::EventHeader::k_MAX_SIZE_SOFT);
}

static void testN1_decodeFromFile()
// --------------------------------------------------------------------
// DECODE FROM FILE
//
// Concerns:
//   bmqp::ConfirmEventBuilder encodes bmqp::Event so that binary data
//   can be stored into a file and then restored and decoded back.
//
// Plan:
//   1. Using bmqp::COnfirmEventBuilder encode
//      bmqp::EventType::e_CONFIRM event.
//   2. Store binary representation of this event into a file.
//   3. Read this file, decode event and verify that it contains messages
//      with expected properties.
// --------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("DECODE FROM FILE");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        256,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::ConfirmEventBuilder obj(&blobSpPool,
                                  bmqtst::TestHelperUtil::allocator());
    bsl::vector<Data>         messages(bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob outBlob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqu::MemOutStream             os(bmqtst::TestHelperUtil::allocator());
    bdlb::Guid                     guid       = bdlb::GuidUtil::generate();
    const int                      k_NUM_MSGS = 10;

    PVV("Appending messages");
    appendMessages(&obj, &messages, k_NUM_MSGS);

    BMQTST_ASSERT_NE(obj.blob()->length(), 0);

    os << "msg_confirm_" << guid << ".bin" << bsl::ends;

    const int blobLen = obj.blob()->length();
    char*     buf     = new char[blobLen];

    /// Functor invoked to delete the file at the specified `filePath`
    struct local {
        static void deleteOnExit(const char* filePath, char* buf)
        {
            BSLS_ASSERT_OPT(bsl::remove(filePath) == 0);
            delete[] buf;
        }
    };

    bdlb::ScopeExitAny guard(
        bdlf::BindUtil::bind(local::deleteOnExit, os.str().data(), buf));

    // Dump blob into file
    bsl::ofstream ofile(os.str().data(), bsl::ios_base::binary);

    BSLS_ASSERT(ofile.good() == true);

    bdlbb::BlobUtil::copy(buf, *obj.blob(), 0, blobLen);
    ofile.write(buf, blobLen);
    ofile.close();
    bsl::memset(buf, 0, blobLen);

    // Read blob from file
    bsl::ifstream ifile(os.str().data(), bsl::ios_base::binary);

    BSLS_ASSERT(ifile.good() == true);

    ifile.read(buf, blobLen);
    ifile.close();

    bsl::shared_ptr<char> dataBufferSp(buf,
                                       bslstl::SharedPtrNilDeleter(),
                                       bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobBuffer     dataBlobBuffer(dataBufferSp, blobLen);

    outBlob.appendDataBuffer(dataBlobBuffer);
    outBlob.setLength(blobLen);

    BMQTST_ASSERT_EQ(bdlbb::BlobUtil::compare(*obj.blob(), outBlob), 0);

    // Decode event
    bmqp::Event event(&outBlob, bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(event.isValid(), true);
    BMQTST_ASSERT_EQ(event.isConfirmEvent(), true);

    PVV("Iterating over messages");

    bmqp::ConfirmMessageIterator iter;
    event.loadConfirmMessageIterator(&iter);

    BMQTST_ASSERT_EQ(iter.isValid(), true);

    size_t idx = 0;
    while (iter.next() == 1 && idx < messages.size()) {
        const Data& d = messages[idx];

        BMQTST_ASSERT_EQ_D(idx, iter.isValid(), true);
        BMQTST_ASSERT_EQ_D(idx, d.d_guid, iter.message().messageGUID());
        BMQTST_ASSERT_EQ_D(idx, d.d_queueId, iter.message().queueId());
        BMQTST_ASSERT_EQ_D(idx, d.d_subQueueId, iter.message().subQueueId());

        ++idx;
    }

    BMQTST_ASSERT_EQ(idx, messages.size());
    BMQTST_ASSERT_EQ(iter.isValid(), false);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 4: test4_capacity(); break;
    case 3: test3_reset(); break;
    case 2: test2_multiMessage(); break;
    case 1: test1_breathingTest(); break;
    case -1: testN1_decodeFromFile(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
