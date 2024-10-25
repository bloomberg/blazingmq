// Copyright 2023 Bloomberg Finance L.P.
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

// bmqp_rejecteventbuilder.t.cpp                                      -*-C++-*-
#include <bmqp_rejecteventbuilder.h>

// BMQ
#include <bmqp_event.h>
#include <bmqp_rejectmessageiterator.h>
#include <bmqt_messageguid.h>
#include <bmqt_resultcode.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_guid.h>
#include <bdlb_guidutil.h>
#include <bdlb_scopeexit.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bsl_fstream.h>
#include <bsl_ios.h>
#include <bsl_memory.h>
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
/// `RejectEventBuilder::appendMessage` takes.
struct Data {
    int               d_queueId;
    bmqt::MessageGUID d_guid;
    int               d_subQueueId;
};

/// Append the specified `numMsgs` messages to the specified `builder` and
/// populate the specified `vec` with the messages that were added, so that
/// the caller can correlate the builder's data to what was added.
static void appendMessages(bmqp::RejectEventBuilder* builder,
                           bsl::vector<Data>*        vec,
                           size_t                    numMsgs)
{
    // Above hex string represents a valid guid
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
        ASSERT_EQ(rc, 0);
        vec->push_back(data);
    }
}

/// Use a RejectMessageIterator to verify that the specified `builder`
/// contains the messages represented in the specified `data`.  Note that
/// bmqp::Event and bmqp::RejectMessageIterator are lower than
/// bmqp::RejectEventBuilder and thus, can be used to test it.
static void verifyContent(const bmqp::RejectEventBuilder& builder,
                          const bsl::vector<Data>&        data)
{
    PVV("Verifying accessors");
    size_t expectedSize = sizeof(bmqp::EventHeader) +
                          sizeof(bmqp::RejectHeader) +
                          (data.size() * sizeof(bmqp::RejectMessage));
    ASSERT_EQ(static_cast<size_t>(builder.messageCount()), data.size());
    ASSERT_EQ(static_cast<size_t>(builder.eventSize()), expectedSize);
    ASSERT_EQ(static_cast<size_t>(builder.blob().length()), expectedSize);

    PVV("Iterating over messages");
    bmqp::Event event(&builder.blob(), s_allocator_p);

    ASSERT(event.isValid());
    ASSERT(event.isRejectEvent());

    bmqp::RejectMessageIterator iter;
    event.loadRejectMessageIterator(&iter);

    ASSERT(iter.isValid());

    size_t idx = 0;
    while (iter.next() == 1 && idx < data.size()) {
        const Data& d = data[idx];

        ASSERT_EQ_D(idx, iter.isValid(), true);
        ASSERT_EQ_D(idx, d.d_queueId, iter.message().queueId());
        ASSERT_EQ_D(idx, d.d_subQueueId, iter.message().subQueueId());
        ASSERT_EQ_D(idx, d.d_guid, iter.message().messageGUID());

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

    bdlbb::PooledBlobBufferFactory bufferFactory(256, s_allocator_p);
    bmqp::RejectEventBuilder       obj(&bufferFactory, s_allocator_p);
    bsl::vector<Data>              messages(s_allocator_p);

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
    // Create a RejectEvent with multiple messages.  Iterate and verify.

    const int k_NUM_MSGS = 1000;

    bdlbb::PooledBlobBufferFactory bufferFactory(256, s_allocator_p);
    bmqp::RejectEventBuilder       obj(&bufferFactory, s_allocator_p);
    bsl::vector<Data>              messages(s_allocator_p);

    PVV("Appending messages");
    appendMessages(&obj, &messages, k_NUM_MSGS);

    PVV("Verifying content");
    verifyContent(obj, messages);
}

static void test3_reset()
{
    bmqtst::TestHelper::printTestName("RESET");
    // Verifying reset: add three messages, reset, and add another message.

    bdlbb::PooledBlobBufferFactory bufferFactory(256, s_allocator_p);
    bmqp::RejectEventBuilder       obj(&bufferFactory, s_allocator_p);
    bsl::vector<Data>              messages(s_allocator_p);

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
    bdlbb::PooledBlobBufferFactory bufferFactory(256, s_allocator_p);
    bmqp::RejectEventBuilder       obj(&bufferFactory, s_allocator_p);

    PVV("Computing max message");
    // Compute max message using a different logic than how it is done in
    // 'RejectEventBuilder'.
    int maxMsgCount      = 0;
    int currentEventSize = sizeof(bmqp::EventHeader) +
                           sizeof(bmqp::RejectHeader);
    while ((currentEventSize + sizeof(bmqp::RejectMessage)) <=
           bmqp::EventHeader::k_MAX_SIZE_SOFT) {
        ++maxMsgCount;
        currentEventSize += sizeof(bmqp::RejectMessage);
    }
    PV("MaxMessageCount: " << maxMsgCount);
    ASSERT_EQ(obj.maxMessageCount(), maxMsgCount);

    PVV("Filling up RejectEventBuilder");
    // Fill the builder with that maximum number of messages, all additions
    // should succeed.
    while (maxMsgCount--) {
        rc = obj.appendMessage(0, 0, bmqt::MessageGUID());
        ASSERT_EQ(rc, 0);
        ASSERT(obj.eventSize() <= bmqp::EventHeader::k_MAX_SIZE_SOFT);
    }

    ASSERT_EQ(obj.messageCount(), obj.maxMessageCount());

    PVV("Append a one-too-much message to the RejectEventBuilder");
    // This message should fail since the builder is full.
    rc = obj.appendMessage(0, 0, bmqt::MessageGUID());
    ASSERT_EQ(rc, static_cast<int>(bmqt::EventBuilderResult::e_EVENT_TOO_BIG));
    ASSERT(obj.eventSize() <= bmqp::EventHeader::k_MAX_SIZE_SOFT);
}

static void testN1_decodeFromFile()
// --------------------------------------------------------------------
// DECODE FROM FILE
//
// Concerns:
//   bmqp::RejectEventBuilder encodes bmqp::Event so that binary data
//   can be stored into a file and then restored and decoded back.
//
// Plan:
//   1. Using bmqp::RejectEventBuilder encode
//      bmqp::EventType::e_REJECT event.
//   2. Store binary representation of this event into a file.
//   3. Read this file, decode event and verify that it contains messages
//      with expected properties.
// --------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("DECODE FROM FILE");

    bdlbb::PooledBlobBufferFactory bufferFactory(256, s_allocator_p);
    bmqp::RejectEventBuilder       obj(&bufferFactory, s_allocator_p);
    bsl::vector<Data>              messages(s_allocator_p);
    bdlbb::Blob                    outBlob(&bufferFactory, s_allocator_p);
    bmqu::MemOutStream             os(s_allocator_p);
    bdlb::Guid                     guid       = bdlb::GuidUtil::generate();
    const int                      k_NUM_MSGS = 10;

    PVV("Appending messages");
    appendMessages(&obj, &messages, k_NUM_MSGS);

    ASSERT_NE(obj.blob().length(), 0);

    os << "msg_reject_" << guid << ".bin" << bsl::ends;

    const int blobLen = obj.blob().length();
    char*     buf     = new char[blobLen];

    /// Functor invoked to delete the file at the specified `filePath`
    /// and the specified `buf`.
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
    bsl::ofstream ofile(os.str().data(), bsl::ios::binary);
    BSLS_ASSERT(ofile.good());

    bdlbb::BlobUtil::copy(buf, obj.blob(), 0, blobLen);
    ofile.write(buf, blobLen);
    ofile.close();
    bsl::memset(buf, 0, blobLen);

    // Read blob from file
    bsl::ifstream ifile(os.str().data(), bsl::ios::binary);
    BSLS_ASSERT(ifile.good());

    ifile.read(buf, blobLen);
    ifile.close();

    bsl::shared_ptr<char> dataBufferSp(buf,
                                       bslstl::SharedPtrNilDeleter(),
                                       s_allocator_p);
    bdlbb::BlobBuffer     dataBlobBuffer(dataBufferSp, blobLen);

    outBlob.appendDataBuffer(dataBlobBuffer);
    outBlob.setLength(blobLen);

    ASSERT_EQ(bdlbb::BlobUtil::compare(obj.blob(), outBlob), 0);

    // Decode event
    bmqp::Event event(&outBlob, s_allocator_p);

    ASSERT(event.isValid());
    ASSERT(event.isRejectEvent());

    PVV("Iterating over messages");

    bmqp::RejectMessageIterator iter;
    event.loadRejectMessageIterator(&iter);

    ASSERT(iter.isValid());

    size_t idx = 0;
    while (iter.next() == 1 && idx < messages.size()) {
        const Data& d = messages[idx];

        ASSERT_EQ_D(idx, iter.isValid(), true);
        ASSERT_EQ_D(idx, d.d_guid, iter.message().messageGUID());
        ASSERT_EQ_D(idx, d.d_queueId, iter.message().queueId());
        ASSERT_EQ_D(idx, d.d_subQueueId, iter.message().subQueueId());

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
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
