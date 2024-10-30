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

// bmqp_ackeventbuilder.t.cpp                                         -*-C++-*-
#include <bmqp_ackeventbuilder.h>

// BMQ
#include <bmqp_ackmessageiterator.h>
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

/// struct representing the parameters that `AckEventBuilder::appendMessage`
/// takes.
struct Data {
    int               d_status;
    int               d_corrId;
    bmqt::MessageGUID d_guid;
    int               d_queueId;
};

/// Append the specified `numMsgs` messages to the specified `builder` and
/// populate the specified `vec` with the messages that were added, so that
/// the caller can corelate the builder's data to what was added.
static void appendMessages(bmqp::AckEventBuilder* builder,
                           bsl::vector<Data>*     vec,
                           size_t                 numMsgs)
{
    // Above hex string represents a valid guid with these values:
    //   TS = bdlb::BigEndianUint64::make(12345)
    //   IP = 98765
    //   ID = 9999
    static const char s_HEX_REP[] = "0000000000003039CD8101000000270F";

    vec->reserve(numMsgs);

    for (size_t i = 0; i < numMsgs; ++i) {
        Data data;
        data.d_status = i % 3;
        data.d_corrId = i * i;
        data.d_guid.fromHex(s_HEX_REP);
        data.d_queueId = i;
        int rc         = builder->appendMessage(data.d_status,
                                        data.d_corrId,
                                        data.d_guid,
                                        data.d_queueId);
        ASSERT_EQ(rc, 0);
        vec->push_back(data);
    }
}

/// Use an AckMessageIterator to verify that the specified `builder`
/// contains the messages represented in the specified `data`.  Note that
/// bmqp::Event and bmqp::AckMessageIterator are lower than
/// bmqp::AckEventBuilder and thus, can be used to test it.
static void verifyContent(const bmqp::AckEventBuilder& builder,
                          const bsl::vector<Data>&     data)
{
    PVV("Verifying accessors");
    size_t expectedSize = sizeof(bmqp::EventHeader) + sizeof(bmqp::AckHeader) +
                          (data.size() * sizeof(bmqp::AckMessage));
    ASSERT_EQ(static_cast<size_t>(builder.messageCount()), data.size());
    ASSERT_EQ(static_cast<size_t>(builder.eventSize()), expectedSize);
    ASSERT_EQ(static_cast<size_t>(builder.blob().length()), expectedSize);

    PVV("Iterating over messages");
    bmqp::Event event(&builder.blob(), bmqtst::TestHelperUtil::allocator());

    ASSERT_EQ(event.isValid(), true);
    ASSERT_EQ(event.isAckEvent(), true);

    bmqp::AckMessageIterator iter;
    event.loadAckMessageIterator(&iter);

    ASSERT_EQ(iter.isValid(), true);

    size_t idx = 0;
    while (iter.next() == 1 && idx < data.size()) {
        const Data& d = data[idx];

        ASSERT_EQ_D(idx, iter.isValid(), true);
        ASSERT_EQ_D(idx, d.d_guid, iter.message().messageGUID());
        ASSERT_EQ_D(idx, d.d_corrId, iter.message().correlationId());
        ASSERT_EQ_D(idx, d.d_status, iter.message().status());
        ASSERT_EQ_D(idx, d.d_queueId, iter.message().queueId());

        ++idx;
    }

    ASSERT_EQ(idx, data.size());
    ASSERT_EQ(iter.isValid(), false);
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
    bmqp::AckEventBuilder obj(&blobSpPool,
                              bmqtst::TestHelperUtil::allocator());
    bsl::vector<Data>     messages(bmqtst::TestHelperUtil::allocator());

    PVV("Verifying accessors");
    ASSERT_EQ(obj.messageCount(), 0);
    ASSERT_NE(obj.maxMessageCount(), 0);
    ASSERT_EQ(obj.eventSize(), 0);
    ASSERT_EQ(obj.blob().length(), 0);

    PVV("Appending one message");
    appendMessages(&obj, &messages, 1);

    PVV("Verifying content");
    verifyContent(obj, messages);
}

static void test2_multiMessage()
{
    bmqtst::TestHelper::printTestName("MULTI MESSAGE");
    // Create an ACK event with multiple ACK messages.  Iterate and verify.

    const int k_NUM_MSGS = 1000;

    bdlbb::PooledBlobBufferFactory bufferFactory(
        256,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::AckEventBuilder obj(&blobSpPool,
                              bmqtst::TestHelperUtil::allocator());
    bsl::vector<Data>     messages(bmqtst::TestHelperUtil::allocator());

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
    bmqp::AckEventBuilder obj(&blobSpPool,
                              bmqtst::TestHelperUtil::allocator());
    bsl::vector<Data>     messages(bmqtst::TestHelperUtil::allocator());

    PV("Appending 3 messages");
    appendMessages(&obj, &messages, 3);

    PV("Resetting the builder");
    obj.reset();

    PV("Verifying accessors");
    ASSERT_EQ(obj.messageCount(), 0);
    ASSERT_EQ(obj.eventSize(), 0);
    ASSERT_EQ(obj.blob().length(), 0);

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
    bmqp::AckEventBuilder obj(&blobSpPool,
                              bmqtst::TestHelperUtil::allocator());

    PVV("Computing max message");
    // Compute max message using a different logic than how it is done in
    // 'AckEventBuilder'.
    int maxMsgCount      = 0;
    int currentEventSize = sizeof(bmqp::EventHeader) + sizeof(bmqp::AckHeader);
    while ((currentEventSize + sizeof(bmqp::AckMessage)) <=
           bmqp::EventHeader::k_MAX_SIZE_SOFT) {
        ++maxMsgCount;
        currentEventSize += sizeof(bmqp::AckMessage);
    }
    PV("MaxMessageCount: " << maxMsgCount);
    ASSERT_EQ(obj.maxMessageCount(), maxMsgCount);

    PVV("Filling up AckEventBuilder");
    // Fill the builder with that maximum number of messages, all additions
    // should succeed.
    while (maxMsgCount--) {
        rc = obj.appendMessage(0, 0, bmqt::MessageGUID(), 0);
        ASSERT_EQ(rc, 0);
        ASSERT(obj.eventSize() <= bmqp::EventHeader::k_MAX_SIZE_SOFT);
    }

    ASSERT_EQ(obj.messageCount(), obj.maxMessageCount());

    PVV("Append a one-too-much message to the AckEventBuilder");
    // This message should fail since the builder is full.
    rc = obj.appendMessage(0, 0, bmqt::MessageGUID(), 0);
    ASSERT_EQ(rc, static_cast<int>(bmqt::EventBuilderResult::e_EVENT_TOO_BIG));
    ASSERT(obj.eventSize() <= bmqp::EventHeader::k_MAX_SIZE_SOFT);
}

static void testN1_decodeFromFile()
// --------------------------------------------------------------------
// DECODE FROM FILE
//
// Concerns:
//   bmqp::AckEventBuilder encodes bmqp::Event so that binary data
//   can be stored into a file and then restored and decoded back.
//
// Plan:
//   1. Using bmqp::AckEventBuilder encode bmqp::EventType::e_ACK event.
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
    bmqp::AckEventBuilder obj(&blobSpPool,
                              bmqtst::TestHelperUtil::allocator());
    bsl::vector<Data>     messages(bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob outBlob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqu::MemOutStream             os(bmqtst::TestHelperUtil::allocator());
    bdlb::Guid                     guid        = bdlb::GuidUtil::generate();
    const int                      k_NUM_MSGS  = 10;
    const int                      k_SIZE      = 256;
    char                           buf[k_SIZE] = {0};

    PVV("Appending messages");
    appendMessages(&obj, &messages, k_NUM_MSGS);

    os << "msg_ack_" << guid << ".bin" << bsl::ends;

    /// Functor invoked to delete the file at the specified `filePath`
    struct local {
        static void deleteFile(const char* filePath)
        {
            BSLS_ASSERT_OPT(bsl::remove(filePath) == 0);
        }
    };

    bdlb::ScopeExitAny guard(
        bdlf::BindUtil::bind(local::deleteFile, os.str().data()));

    // Dump blob into file
    bsl::ofstream ofile(os.str().data(), bsl::ios_base::binary);

    BSLS_ASSERT(ofile.good() == true);

    bdlbb::BlobUtil::copy(buf, obj.blob(), 0, obj.blob().length());
    ofile.write(buf, k_SIZE);
    ofile.close();
    bsl::memset(buf, 0, k_SIZE);

    // Read blob from file
    bsl::ifstream ifile(os.str().data(), bsl::ios_base::binary);

    BSLS_ASSERT(ifile.good() == true);

    ifile.read(buf, k_SIZE);
    ifile.close();

    bsl::shared_ptr<char> dataBufferSp(buf,
                                       bslstl::SharedPtrNilDeleter(),
                                       bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobBuffer     dataBlobBuffer(dataBufferSp, k_SIZE);

    outBlob.appendDataBuffer(dataBlobBuffer);
    outBlob.setLength(obj.blob().length());

    ASSERT_EQ(bdlbb::BlobUtil::compare(obj.blob(), outBlob), 0);

    // Decode event
    bmqp::Event event(&outBlob, bmqtst::TestHelperUtil::allocator());

    ASSERT_EQ(event.isValid(), true);
    ASSERT_EQ(event.isAckEvent(), true);

    PVV("Iterating over messages");

    bmqp::AckMessageIterator iter;
    event.loadAckMessageIterator(&iter);

    ASSERT_EQ(iter.isValid(), true);

    size_t idx = 0;
    while (iter.next() == 1 && idx < messages.size()) {
        const Data& d = messages[idx];

        ASSERT_EQ_D(idx, iter.isValid(), true);
        ASSERT_EQ_D(idx, d.d_guid, iter.message().messageGUID());
        ASSERT_EQ_D(idx, d.d_corrId, iter.message().correlationId());
        ASSERT_EQ_D(idx, d.d_status, iter.message().status());
        ASSERT_EQ_D(idx, d.d_queueId, iter.message().queueId());

        ++idx;
    }

    ASSERT_EQ(idx, messages.size());
    ASSERT_EQ(iter.isValid(), false);
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
