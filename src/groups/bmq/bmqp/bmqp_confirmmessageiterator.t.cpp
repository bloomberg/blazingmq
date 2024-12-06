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

// bmqp_confirmmessageiterator.t.cpp                                  -*-C++-*-
#include <bmqp_confirmmessageiterator.h>

// BMQ
#include <bmqp_protocol.h>
#include <bmqt_messageguid.h>

// BDE
#include <bdlb_bigendian.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_vector.h>

#include <bmqu_memoutstream.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Struct representing the attributes of a ConfirmMessage.
struct Data {
    int               d_queueId;
    bmqt::MessageGUID d_guid;
    int               d_subQueueId;
};

/// Append a confirm message having the attributes from the specified `data`
/// to the specified `blob`.
static void appendConfirmMessage(bdlbb::Blob* blob, const Data& data)
{
    // QueueId
    bdlb::BigEndianInt32 queueId = bdlb::BigEndianInt32::make(data.d_queueId);

    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(&queueId),
                            sizeof(queueId));

    // GUID
    unsigned char guidBinBuffer[bmqt::MessageGUID::e_SIZE_BINARY];
    data.d_guid.toBinary(guidBinBuffer);
    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(guidBinBuffer),
                            bmqt::MessageGUID::e_SIZE_BINARY);

    // SubQueueId
    bdlb::BigEndianInt32 subQueueId = bdlb::BigEndianInt32::make(
        data.d_subQueueId);

    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(&subQueueId),
                            sizeof(subQueueId));
}

/// Populate the specified `blob` with the specified `numMsgs` confirm
/// messages, and store them also in the specified `vec`.  Populate the
/// specified `eh` with the corresponding event header.
static void populateBlob(bdlbb::Blob*       blob,
                         bmqp::EventHeader* eh,
                         bsl::vector<Data>* vec,
                         size_t             numMsgs)
{
    // Create a GUID from valid hex rep

    // Above hex string represents a valid guid with these values:
    //   TS = bdlb::BigEndianUint64::make(12345)
    //   IP = 98765
    //   ID = 9999
    static const char s_GUID_HEX[] = "0000000000003039CD8101000000270F";

    vec->reserve(numMsgs);
    int eventLength = 0;

    // Event Header
    (*eh)
        .setType(bmqp::EventType::e_CONFIRM)
        .setHeaderWords(sizeof(bmqp::EventHeader) /
                        bmqp::Protocol::k_WORD_SIZE);

    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(eh),
                            sizeof(bmqp::EventHeader));
    eventLength += sizeof(bmqp::EventHeader);

    // ConfirmHeader
    bmqp::ConfirmHeader ch;
    ch.setHeaderWords(sizeof(bmqp::ConfirmHeader) /
                      bmqp::Protocol::k_WORD_SIZE);
    ch.setPerMessageWords(sizeof(bmqp::ConfirmMessage) /
                          bmqp::Protocol::k_WORD_SIZE);

    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(&ch),
                            sizeof(bmqp::ConfirmHeader));
    eventLength += sizeof(bmqp::ConfirmHeader);

    // ConfirmMessages
    for (size_t i = 0; i < numMsgs; ++i) {
        Data data;
        data.d_guid.fromHex(s_GUID_HEX);  // TBD: use new guid every time
        data.d_queueId    = i;
        data.d_subQueueId = 2 * i;

        appendConfirmMessage(blob, data);

        eventLength += sizeof(bmqp::ConfirmMessage);

        vec->push_back(data);
    }

    // Set the final EventHeader length
    bmqp::EventHeader* e = reinterpret_cast<bmqp::EventHeader*>(
        blob->buffer(0).data());
    e->setLength(eventLength);
}

/// Populate the specified `blob` with full ConfirmMessage event for the
/// message having the specified `queueId`, `guid` and `subQueueId`; and
/// populate the specified `eh` with the corresponding event header.
static void populateBlob(bdlbb::Blob*             blob,
                         bmqp::EventHeader*       eh,
                         int                      queueId,
                         const bmqt::MessageGUID& guid,
                         int                      subQueueId)
{
    Data data;
    data.d_queueId    = queueId;
    data.d_guid       = guid;
    data.d_subQueueId = subQueueId;

    int eventLength = sizeof(bmqp::EventHeader) + sizeof(bmqp::ConfirmHeader) +
                      sizeof(bmqp::ConfirmMessage);

    // Event Header
    (*eh)
        .setType(bmqp::EventType::e_CONFIRM)
        .setLength(eventLength)
        .setHeaderWords(sizeof(bmqp::EventHeader) /
                        bmqp::Protocol::k_WORD_SIZE);

    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(eh),
                            eh->headerWords() * bmqp::Protocol::k_WORD_SIZE);

    // Confirm Header
    bmqp::ConfirmHeader ch;
    ch.setHeaderWords(sizeof(bmqp::ConfirmHeader) /
                      bmqp::Protocol::k_WORD_SIZE);
    ch.setPerMessageWords(sizeof(bmqp::ConfirmMessage) /
                          bmqp::Protocol::k_WORD_SIZE);

    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(&ch),
                            sizeof(bmqp::ConfirmHeader));

    // Confirm Message
    appendConfirmMessage(blob, data);
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());

    {
        // Create invalid iter
        bmqp::ConfirmMessageIterator iter;
        BMQTST_ASSERT_EQ(iter.isValid(), false);
    }

    {
        // Create invalid iter from another invalid iter
        bmqp::ConfirmMessageIterator iter1;
        bmqp::ConfirmMessageIterator iter2(iter1);

        BMQTST_ASSERT_EQ(iter1.isValid(), false);
        BMQTST_ASSERT_EQ(iter2.isValid(), false);
    }

    {
        // Assigning invalid iter
        bmqp::ConfirmMessageIterator iter1, iter2;
        BMQTST_ASSERT_EQ(iter1.isValid(), false);
        BMQTST_ASSERT_EQ(iter2.isValid(), false);

        iter1 = iter2;
        BMQTST_ASSERT_EQ(iter1.isValid(), false);
        BMQTST_ASSERT_EQ(iter2.isValid(), false);
    }

    {
        // Create valid iter
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

        // Populate blob
        bmqp::EventHeader       eventHeader;
        const int               qId = 54321;
        const bmqt::MessageGUID guid;
        const int               sQId = 123;

        populateBlob(&blob, &eventHeader, qId, guid, sQId);

        // Iterate and verify
        bmqp::ConfirmMessageIterator iter(&blob, eventHeader);

        BMQTST_ASSERT_EQ(iter.isValid(), true);
        BMQTST_ASSERT_EQ(iter.next(), 1);

        BMQTST_ASSERT_EQ(iter.message().queueId(), qId);
        BMQTST_ASSERT_EQ(iter.message().messageGUID(), guid);
        BMQTST_ASSERT_EQ(iter.message().subQueueId(), sQId);

        BMQTST_ASSERT_EQ(iter.next(), 0);
        BMQTST_ASSERT_EQ(iter.isValid(), false);

        // Copy invalid iterator
        bmqp::ConfirmMessageIterator iter2(iter);
        BMQTST_ASSERT_EQ(iter2.isValid(), false);

        // Clear
        iter.clear();
        BMQTST_ASSERT_EQ(iter.isValid(), false);
        BMQTST_ASSERT_EQ(iter2.isValid(), false);

        // Assign
        iter = iter2;
        BMQTST_ASSERT_EQ(iter.isValid(), false);
        BMQTST_ASSERT_EQ(iter2.isValid(), false);

        // Reset, iterate and verify again
        iter.reset(&blob, eventHeader);
        BMQTST_ASSERT_EQ(iter.isValid(), true);
        BMQTST_ASSERT_EQ(iter.next(), 1);
        BMQTST_ASSERT_EQ(iter.message().queueId(), qId);
        BMQTST_ASSERT_EQ(iter.message().messageGUID(), guid);
        BMQTST_ASSERT_EQ(iter.message().subQueueId(), sQId);

        BMQTST_ASSERT_EQ(iter.next(), 0);
        BMQTST_ASSERT_EQ(iter.isValid(), false);

        // Reset, assign and iterate other
        iter.reset(&blob, eventHeader);
        iter2 = iter;
        BMQTST_ASSERT_EQ(iter2.isValid(), true);
        BMQTST_ASSERT_EQ(iter2.next(), 1);
        BMQTST_ASSERT_EQ(iter2.message().queueId(), qId);
        BMQTST_ASSERT_EQ(iter2.message().messageGUID(), guid);
        BMQTST_ASSERT_EQ(iter2.message().subQueueId(), sQId);

        BMQTST_ASSERT_EQ(iter2.next(), 0);
        BMQTST_ASSERT_EQ(iter2.isValid(), false);

        // Copy valid iterator
        iter.reset(&blob, eventHeader);
        BMQTST_ASSERT_EQ(iter.next(), 1);
        bmqp::ConfirmMessageIterator iter3(iter);
        BMQTST_ASSERT_EQ(iter3.isValid(), true);

        // Provoke next method to return rc_INVALID value
        BMQTST_ASSERT_EQ(iter.next(), 0);
        BMQTST_ASSERT_LT(iter.next(), 0);  // rc_INVALID
    }
}

static void test2_multiConfirm()
{
    bmqtst::TestHelper::printTestName("MULTI CONFIRM");

    // Test iterating over CONFIRM event having multiple CONFIRM messages

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob eventBlob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

    bsl::vector<Data> data(bmqtst::TestHelperUtil::allocator());
    bmqp::EventHeader eventHeader;

    const size_t k_NUM_MSGS = 1000;

    populateBlob(&eventBlob, &eventHeader, &data, k_NUM_MSGS);

    // Iterate and verify
    bmqp::ConfirmMessageIterator iter(&eventBlob, eventHeader);
    BMQTST_ASSERT_EQ(iter.isValid(), true);

    size_t index = 0;

    while ((iter.next() == 1) && index < data.size()) {
        const Data& D = data[index];
        BMQTST_ASSERT_EQ_D(index, D.d_queueId, iter.message().queueId());
        BMQTST_ASSERT_EQ_D(index, D.d_guid, iter.message().messageGUID());
        BMQTST_ASSERT_EQ_D(index, D.d_subQueueId, iter.message().subQueueId());
        ++index;
    }

    BMQTST_ASSERT_EQ(data.size(), index);
    BMQTST_ASSERT_EQ(iter.isValid(), false);
}

static void test3_nextMethod()
{
    // --------------------------------------------------------------------
    // NEXT
    //
    // Concerns:
    //   Attempting to advance to the next message fails if:
    //     1. Iteration has already reached the end of the event.
    //     2. The underlying message is corrupted (e.g., number of bytes in the
    //        blob is less than the payload size of the message declared in the
    //        header).
    //
    // Plan:
    //   1. Create and init iterator by blob.
    //   2. Iterate and reach the end of blob
    //   3. Expect that next method will return error code
    //   4. Create and init iterator by blob of size one byte less then sum of
    //      event header, message header and confirm message
    //   5. Expect that next method will return error code
    //
    // Testing:
    //   int next();
    // --------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("NEXT METHOD");

    // Test iterating over CONFIRM event having multiple CONFIRM messages

    // Default values for event header
    bmqp::EventHeader       eventHeader;
    const int               qId = 54321;
    const bmqt::MessageGUID guid;
    const int               sQId = 123;

    // Next method. Iterator is in invalid state case.
    {
        // Create buffer factory and blob
        bdlbb::PooledBlobBufferFactory bufferFactory(
            1024,
            bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

        // Populate blob
        populateBlob(&blob, &eventHeader, qId, guid, sQId);

        // Create valid iterator
        bmqp::ConfirmMessageIterator iter(&blob, eventHeader);
        BMQTST_ASSERT(iter.isValid());

        // Iterate and verify
        BMQTST_ASSERT_EQ(iter.next(), 1);
        BMQTST_ASSERT_EQ(iter.next(), 0);
        BMQTST_ASSERT_LT(iter.next(), 0);  // rc_INVALID
    }

    // Next method. Not enough bytes case.
    {
        // Min buf size not to reproduce given rc
        const size_t enoughSize = sizeof(bmqp::EventHeader) +
                                  sizeof(bmqp::ConfirmHeader) +
                                  sizeof(bmqp::ConfirmMessage);

        // Create buffer factory and blob
        bdlbb::PooledBlobBufferFactory bufferFactory(
            1024,
            bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

        // Populate blob
        populateBlob(&blob, &eventHeader, qId, guid, sQId);

        // Create iterator
        bmqp::ConfirmMessageIterator iter(&blob, eventHeader);
        blob.setLength(enoughSize - 1);
        BMQTST_ASSERT(iter.isValid());

        // Iterate and verify
        BMQTST_ASSERT_LT(iter.next(), 0);  // rc_NOT_ENOUGH_BYTES
    }
}

static void test4_resetMethod()
{
    // --------------------------------------------------------------------
    // RESET
    //
    // Concerns:
    //   Attempting to reset iterator fails if:
    //     1. The underlying event doesn't contain message header
    //     2. The underlying message is corrupted (e.g., number of bytes in the
    //        blob is less than the payload size needed to allocate the message
    //        header).
    //
    // Plan:
    //   1. Create and init iterator by blob of event header size.
    //   2. Expect that reset method will return error code
    //   3. Create and init iterator by blob of size one byte less then sum of
    //      event header and message header
    //   4. Expect that reset method will return error code
    //
    // Testing:
    //   int reset();
    // --------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("RESET METHOD");

    // Default values for event header
    bmqp::EventHeader       eventHeader;
    const int               qId = 54321;
    const bmqt::MessageGUID guid;
    const int               sQId = 123;

    // Reset method. Invalid event header case.
    {
        // Min buf size not to reproduce given rc
        const size_t enoughSize = sizeof(bmqp::EventHeader) + 1;
        bdlbb::PooledBlobBufferFactory bufferFactory(
            enoughSize,
            bmqtst::TestHelperUtil::allocator());

        // Create buffer factory and blob
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

        // Populate blob and manually set length
        populateBlob(&blob, &eventHeader, qId, guid, sQId);

        // Create iterator
        bmqp::ConfirmMessageIterator iter(&blob, eventHeader);
        blob.setLength(enoughSize - 1);
        BMQTST_ASSERT(iter.isValid());

        // Reset and verify
        BMQTST_ASSERT_LT(iter.reset(&blob, eventHeader),
                         0);  // rc_INVALID_EVENTHEADER
        BMQTST_ASSERT(!iter.isValid());
    }

    // NOTE: as far as ConfirmHeader::k_MIN_HEADER_SIZE = 1, there is no
    //       possible size of buffer that both would not trigger invalid event
    //       header case and also can't allocate header of minimum size.

    // Reset method. Not enough bytes case.
    {
        // Min buf size not to reproduce given rc
        const size_t enoughSize = sizeof(bmqp::EventHeader) +
                                  sizeof(bmqp::ConfirmHeader);

        // Create buffer factory and blob
        bdlbb::PooledBlobBufferFactory bufferFactory(
            enoughSize,
            bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

        // Populate blob and manually set length
        populateBlob(&blob, &eventHeader, qId, guid, sQId);

        // Create iterator
        bmqp::ConfirmMessageIterator iter(&blob, eventHeader);
        blob.setLength(enoughSize - 1);
        BMQTST_ASSERT(iter.isValid());

        // Reset and verify
        BMQTST_ASSERT_LT(iter.reset(&blob, eventHeader),
                         0);  // rc_NOT_ENOUGH_BYTES
        BMQTST_ASSERT(!iter.isValid());
    }
}

static void test5_dumpBlob()
{
    // --------------------------------------------------------------------
    // DUMB BLOB
    //
    // Concerns:
    //   Check blob layout if:
    //     1. The underlying blob contain single confirm message
    //     2. The underlying blob is not setted
    //
    // Plan:
    //   1. Create and populate blob with a single confirm message
    //   2. Create and init iterator by given blob
    //   3. Check output of dumpBlob method
    //   4. Create and init iterator without setting underlying blob
    //   5. Check output of dumpBlob method
    //
    // Testing:
    //   void dumpBlob(bsl::ostream& stream);
    // --------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("DUMB BLOB");

    // Test iterator dump contains expected value

    bmqp::EventHeader              eventHeader;
    bmqu::MemOutStream             stream(bmqtst::TestHelperUtil::allocator());
    const int                      qId = 54321;
    const bmqt::MessageGUID        guid;
    const int                      sQId = 123;
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

    // Populate blob
    populateBlob(&blob, &eventHeader, qId, guid, sQId);

    //  Create and check iterator blob layout
    {
        bmqp::ConfirmMessageIterator iter(&blob, eventHeader);
        BMQTST_ASSERT(iter.isValid());

        // Dump blob
        iter.dumpBlob(stream);
        bsl::string str1(stream.str(), bmqtst::TestHelperUtil::allocator());
        bsl::string str2("     0:   00000024 43020000 16000000 0000D431     "
                         "|...$C..........1|\n"
                         "    16:   00000000 00000000 00000000 00000000     "
                         "|................|\n"
                         "    32:   0000007B                                "
                         "|...{            |\n",
                         bmqtst::TestHelperUtil::allocator());

        // Verify that dump contains expected value
        BMQTST_ASSERT_EQ(str1, str2);
        stream.reset();
    }

    // Iterator without blob
    {
        bmqp::ConfirmMessageIterator iter;

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
    case 5: test5_dumpBlob(); break;
    case 4: test4_resetMethod(); break;
    case 3: test3_nextMethod(); break;
    case 2: test2_multiConfirm(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
