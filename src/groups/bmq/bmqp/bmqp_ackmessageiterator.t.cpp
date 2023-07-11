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

// bmqp_ackmessageiterator.t.cpp                                      -*-C++-*-
#include <bmqp_ackmessageiterator.h>

// BMQ
#include <bmqp_protocol.h>
#include <bmqt_messageguid.h>

// BDE
#include <bdlb_bigendian.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_cstring.h>  // for strlen, strncmp
#include <bsl_iostream.h>
#include <bslma_default.h>

// MWC
#include <mwcu_memoutstream.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

struct Data {
    // DATA
    int               d_status;
    int               d_corrId;
    bmqt::MessageGUID d_guid;
    int               d_queueId;
};

void populateBlob(bdlbb::Blob*       blob,
                  bmqp::EventHeader* eh,
                  bsl::vector<Data>* vec,
                  size_t             numMsgs)
{
    // Create guid from valid hex rep

    // Above hex string represents a valid guid with these values:
    //   TS = bdlb::BigEndianUint64::make(12345)
    //   IP = 98765
    //   ID = 9999
    const char k_HEX_REP[] = "0000000000003039CD8101000000270F";

    const int k_FLAGS = 1;

    int eventLength = 0;

    (*eh)
        .setType(bmqp::EventType::e_ACK)
        .setHeaderWords(sizeof(bmqp::EventHeader) /
                        bmqp::Protocol::k_WORD_SIZE);
    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(eh),
                            sizeof(bmqp::EventHeader));
    eventLength += sizeof(bmqp::EventHeader);

    // AckHeader
    bmqp::AckHeader ah;
    ah.setFlags(k_FLAGS);
    ah.setHeaderWords(sizeof(bmqp::AckHeader) / bmqp::Protocol::k_WORD_SIZE);
    ah.setPerMessageWords(sizeof(bmqp::AckMessage) /
                          bmqp::Protocol::k_WORD_SIZE);

    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(&ah),
                            sizeof(bmqp::AckHeader));
    eventLength += sizeof(bmqp::AckHeader);

    for (size_t i = 0; i < numMsgs; ++i) {
        Data data;
        data.d_status = i % 3;
        data.d_guid.fromHex(k_HEX_REP);  // TBD use new guid every time
        data.d_corrId  = i;
        data.d_queueId = numMsgs - i;

        bmqp::AckMessage ackMessage;
        ackMessage.setStatus(data.d_status)
            .setCorrelationId(data.d_corrId)
            .setMessageGUID(data.d_guid)
            .setQueueId(data.d_queueId);

        bdlbb::BlobUtil::append(blob,
                                reinterpret_cast<const char*>(&ackMessage),
                                sizeof(bmqp::AckMessage));
        eventLength += sizeof(bmqp::AckMessage);

        vec->push_back(data);
    }

    // set EventHeader length
    bmqp::EventHeader* e = reinterpret_cast<bmqp::EventHeader*>(
        blob->buffer(0).data());
    e->setLength(eventLength);
}

void populateBlob(bdlbb::Blob*             blob,
                  bmqp::EventHeader*       eh,
                  int                      status,
                  int                      corrId,
                  const bmqt::MessageGUID& guid,
                  int                      queueId,
                  unsigned char            flags)
{
    int eventLength = sizeof(bmqp::EventHeader) + sizeof(bmqp::AckHeader) +
                      sizeof(bmqp::AckMessage);

    // EventHeader
    (*eh)
        .setLength(eventLength)
        .setType(bmqp::EventType::e_ACK)
        .setHeaderWords(sizeof(bmqp::EventHeader) /
                        bmqp::Protocol::k_WORD_SIZE);
    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(eh),
                            sizeof(bmqp::EventHeader));

    // AckHeader
    bmqp::AckHeader ah;
    ah.setFlags(flags)
        .setHeaderWords(sizeof(bmqp::AckHeader) / bmqp::Protocol::k_WORD_SIZE)
        .setPerMessageWords(sizeof(bmqp::AckMessage) /
                            bmqp::Protocol::k_WORD_SIZE);

    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(&ah),
                            sizeof(bmqp::AckHeader));

    bmqp::AckMessage ackMessage;
    ackMessage.setStatus(status)
        .setCorrelationId(corrId)
        .setMessageGUID(guid)
        .setQueueId(queueId);

    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(&ackMessage),
                            sizeof(bmqp::AckMessage));
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
// TODO: - Test case with corrupted message header
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// --------------------------------------------------------------------
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
// --------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);

    {
        PVV("Create invalid iter");
        bmqp::AckMessageIterator iter;
        ASSERT_EQ(iter.isValid(), false);
        ASSERT_LT(iter.next(), 0);
    }

    {
        PVV("Create invalid iter from another invalid iter");
        bmqp::AckMessageIterator iter1;
        bmqp::AckMessageIterator iter2(iter1);

        ASSERT_EQ(false, iter1.isValid());
        ASSERT_EQ(false, iter2.isValid());
    }

    {
        PVV("Assigning invalid iter");
        bmqp::AckMessageIterator iter1, iter2;
        ASSERT_EQ(false, iter1.isValid());
        ASSERT_EQ(false, iter2.isValid());

        iter1 = iter2;
        ASSERT_EQ(false, iter1.isValid());
        ASSERT_EQ(false, iter2.isValid());
    }

    {
        PVV("Valid iter");

        // Create valid iter
        bdlbb::Blob blob(&bufferFactory, s_allocator_p);

        // Populate blob
        const bmqt::MessageGUID GUID;
        bmqp::EventHeader       eventHeader;
        unsigned char           FLAGS    = 62;
        const int               CORR_ID  = 54321;
        const int               STATUS   = 3;
        const int               QUEUE_ID = 9876;

        populateBlob(&blob,
                     &eventHeader,
                     STATUS,
                     CORR_ID,
                     GUID,
                     QUEUE_ID,
                     FLAGS);

        // Iterate and verify
        bmqp::AckMessageIterator iter(&blob, eventHeader);

        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(1, iter.next());
        ASSERT_EQ(FLAGS, iter.header().flags());

        ASSERT_EQ(STATUS, iter.message().status());
        ASSERT_EQ(CORR_ID, iter.message().correlationId());
        ASSERT_EQ(GUID, iter.message().messageGUID());
        ASSERT_EQ(QUEUE_ID, iter.message().queueId());

        ASSERT_EQ(false, iter.next());
        ASSERT_EQ(false, iter.isValid());

        // Copy
        bmqp::AckMessageIterator iter2(iter);
        ASSERT_EQ(false, iter2.isValid());
        // ASSERT_EQ(0, iter2.next());

        // Clear
        iter.clear();
        ASSERT_EQ(false, iter.isValid());
        // ASSERT_EQ(0, iter.next());
        ASSERT_EQ(false, iter2.isValid());
        // ASSERT_EQ(0, iter2.next());

        // Assign
        iter = iter2;
        ASSERT_EQ(false, iter.isValid());
        // ASSERT_EQ(0, iter.next());
        ASSERT_EQ(false, iter2.isValid());
        // ASSERT_EQ(0, iter2.next());

        // Reset, iterate and verify again
        iter.reset(&blob, eventHeader);
        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(1, iter.next());
        ASSERT_EQ(FLAGS, iter.header().flags());

        ASSERT_EQ(STATUS, iter.message().status());
        ASSERT_EQ(CORR_ID, iter.message().correlationId());
        ASSERT_EQ(GUID, iter.message().messageGUID());
        ASSERT_EQ(QUEUE_ID, iter.message().queueId());

        ASSERT_EQ(0, iter.next());
        ASSERT_EQ(false, iter.isValid());

        // Reset, assign and iterate other
        iter.reset(&blob, eventHeader);
        iter2 = iter;
        ASSERT_EQ(true, iter2.isValid());
        ASSERT_EQ(1, iter2.next());
        ASSERT_EQ(FLAGS, iter2.header().flags());

        ASSERT_EQ(STATUS, iter2.message().status());
        ASSERT_EQ(CORR_ID, iter2.message().correlationId());
        ASSERT_EQ(GUID, iter2.message().messageGUID());
        ASSERT_EQ(QUEUE_ID, iter2.message().queueId());

        ASSERT_EQ(0, iter2.next());
        ASSERT_EQ(false, iter2.isValid());

        // Reset other from copy of original (did not advance) and iterate
        ASSERT_EQ(iter2.reset(&blob, iter), 0);
        ASSERT_EQ(true, iter2.isValid());
        ASSERT_EQ(1, iter2.next());
        ASSERT_EQ(FLAGS, iter2.header().flags());

        ASSERT_EQ(STATUS, iter2.message().status());
        ASSERT_EQ(CORR_ID, iter2.message().correlationId());
        ASSERT_EQ(GUID, iter2.message().messageGUID());
        ASSERT_EQ(QUEUE_ID, iter2.message().queueId());

        ASSERT_EQ(0, iter2.next());
        ASSERT_EQ(false, iter2.isValid());

        // Verify dumping the blob
        const char*        k_NO_BLOB_STR = "/no blob/";
        mwcu::MemOutStream out(s_allocator_p);

        iter2.dumpBlob(out);
        ASSERT_NE(out.str(), k_NO_BLOB_STR);

        // Dumping an empty blob
        iter2.clear();
        out.reset();

        iter2.dumpBlob(out);
        ASSERT_EQ(out.str(), k_NO_BLOB_STR);

        // Copy valid iterator
        iter.reset(&blob, eventHeader);
        ASSERT_EQ(iter.next(), 1);
        bmqp::AckMessageIterator iter3(iter);
        ASSERT_EQ(iter3.isValid(), true);

        // Provoke next method to return rc_INVALID value
        ASSERT_EQ(iter.next(), 0);
        ASSERT_EQ(iter.next(), -1);
    }
}

/// Test iterating over ACK event having multiple ACK messages
static void test2_ackEventWithMultipleMessages()
{
    mwctst::TestHelper::printTestName("ACK EVENT WITH MULTIPLE MESSAGES");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    bdlbb::Blob                    eventBlob(&bufferFactory, s_allocator_p);

    bsl::vector<Data> data(s_allocator_p);
    bmqp::EventHeader eventHeader;

    const size_t k_NUM_MSGS = 1000;

    populateBlob(&eventBlob, &eventHeader, &data, k_NUM_MSGS);

    // Iterate and verify
    bmqp::AckMessageIterator iter(&eventBlob, eventHeader);
    ASSERT_EQ(true, iter.isValid());

    size_t index = 0;
    while ((iter.next() == 1)) {
        const Data& D = data[index];
        // ASSERT_EQ_D(index, D.d_flags, iter.flags());

        ASSERT_EQ_D(index, D.d_status, iter.message().status());
        ASSERT_EQ_D(index, D.d_corrId, iter.message().correlationId());
        ASSERT_EQ_D(index, D.d_guid, iter.message().messageGUID());
        ASSERT_EQ_D(index, D.d_queueId, iter.message().queueId());
        ++index;
    }

    ASSERT_EQ(index, data.size());
    ASSERT_EQ(false, iter.isValid());
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
    //      event header, message header and ack message
    //   5. Expect that next method will return error code
    //
    // Testing:
    //   int next();
    // --------------------------------------------------------------------
    mwctst::TestHelper::printTestName("NEXT METHOD");

    // Test iterating over ACK event having multiple ACK messages

    // Default values for event header
    // Populate blob
    const bmqt::MessageGUID k_GUID;
    unsigned char           k_FLAGS    = 62;
    const int               k_CORR_ID  = 54321;
    const int               k_STATUS   = 3;
    const int               k_QUEUE_ID = 9876;

    bmqp::EventHeader eventHeader;

    // Next method. Iterator is in invalid state case.
    {
        // Create buffer factory and blob
        bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
        bdlbb::Blob                    blob(&bufferFactory, s_allocator_p);

        // Populate blob
        populateBlob(&blob,
                     &eventHeader,
                     k_STATUS,
                     k_CORR_ID,
                     k_GUID,
                     k_QUEUE_ID,
                     k_FLAGS);

        // Create valid iterator
        bmqp::AckMessageIterator iter(&blob, eventHeader);
        ASSERT(iter.isValid());

        // Iterate and verify
        ASSERT_EQ(iter.next(), 1);
        ASSERT_EQ(iter.next(), 0);
        ASSERT_LT(iter.next(), 0);  // rc_INVALID
    }

    // Next method. Not enough bytes case.
    {
        // Create buffer factory and blob
        bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
        bdlbb::Blob                    blob(&bufferFactory, s_allocator_p);

        // Populate blob
        populateBlob(&blob,
                     &eventHeader,
                     k_STATUS,
                     k_CORR_ID,
                     k_GUID,
                     k_QUEUE_ID,
                     k_FLAGS);

        // Min buf size not to reproduce given rc
        const size_t enoughSize = sizeof(bmqp::EventHeader) +
                                  sizeof(bmqp::AckHeader) +
                                  sizeof(bmqp::AckMessage);

        // Create iterator
        bmqp::AckMessageIterator iter(&blob, eventHeader);
        blob.setLength(enoughSize - 1);
        ASSERT(iter.isValid());

        // Iterate and verify
        ASSERT_LT(iter.next(), 0);  // rc_NOT_ENOUGH_BYTES
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
    mwctst::TestHelper::printTestName("RESET METHOD");

    // Default values for event header
    // Populate blob
    const bmqt::MessageGUID k_GUID;
    unsigned char           k_FLAGS    = 62;
    const int               k_CORR_ID  = 54321;
    const int               k_STATUS   = 3;
    const int               k_QUEUE_ID = 9876;

    bmqp::EventHeader eventHeader;

    // Reset method. Invalid event header case.
    {
        bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);

        // Create buffer factory and blob
        bdlbb::Blob blob(&bufferFactory, s_allocator_p);

        // Populate blob and manually set length
        populateBlob(&blob,
                     &eventHeader,
                     k_STATUS,
                     k_CORR_ID,
                     k_GUID,
                     k_QUEUE_ID,
                     k_FLAGS);

        // Min buf size not to reproduce given rc
        const size_t enoughSize = sizeof(bmqp::EventHeader) + 1;

        // Create iterator
        bmqp::AckMessageIterator iter(&blob, eventHeader);
        blob.setLength(enoughSize - 1);
        ASSERT(iter.isValid());

        // Reset and verify
        ASSERT_LT(iter.reset(&blob, eventHeader),
                  0);  // rc_INVALID_EVENTHEADER
        ASSERT(!iter.isValid());
    }

    // NOTE: as long as AckHeader::k_MIN_HEADER_SIZE = 1, there is no possible
    //       size of buffer that both would not trigger invalid event header
    //       case and also can't allocate header of minimum size.

    // Reset method. Not enough bytes case.
    {
        // Create buffer factory and blob
        bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
        bdlbb::Blob                    blob(&bufferFactory, s_allocator_p);

        // Populate blob and manually set length
        populateBlob(&blob,
                     &eventHeader,
                     k_STATUS,
                     k_CORR_ID,
                     k_GUID,
                     k_QUEUE_ID,
                     k_FLAGS);

        // Min buf size not to reproduce given rc
        const size_t enoughSize = sizeof(bmqp::EventHeader) +
                                  sizeof(bmqp::AckHeader);

        // Create iterator
        bmqp::AckMessageIterator iter(&blob, eventHeader);
        blob.setLength(enoughSize - 1);
        ASSERT(iter.isValid());

        // Reset and verify
        ASSERT_LT(iter.reset(&blob, eventHeader), 0);  // rc_NOT_ENOUGH_BYTES
        ASSERT(!iter.isValid());
    }
}

static void test5_dumpBlob()
{
    // --------------------------------------------------------------------
    // DUMB BLOB
    //
    // Concerns:
    //   Check blob layout if:
    //     1. The underlying blob contain single ack message
    //     2. The underlying blob is not setted
    //
    // Plan:
    //   1. Create and populate blob with a single ack message
    //   2. Create and init iterator by given blob
    //   3. Check output of dumpBlob method
    //   4. Create and init iterator without setting underlying blob
    //   5. Check output of dumpBlob method
    //
    // Testing:
    //   void dumpBlob(bsl::ostream& stream);
    // --------------------------------------------------------------------
    mwctst::TestHelper::printTestName("DUMB BLOB");

    // Test iterator dump contains expected value

    const bmqt::MessageGUID k_GUID;
    unsigned char           k_FLAGS    = 62;
    const int               k_CORR_ID  = 54321;
    const int               k_STATUS   = 3;
    const int               k_QUEUE_ID = 9876;

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    bdlbb::Blob                    blob(&bufferFactory, s_allocator_p);
    bmqp::EventHeader              eventHeader;
    mwcu::MemOutStream             stream(s_allocator_p);

    // Populate blob
    populateBlob(&blob,
                 &eventHeader,
                 k_STATUS,
                 k_CORR_ID,
                 k_GUID,
                 k_QUEUE_ID,
                 k_FLAGS);

    //  Create and check iterator blob layout
    {
        bmqp::AckMessageIterator iter(&blob, eventHeader);
        ASSERT(iter.isValid());

        // Dump blob
        iter.dumpBlob(stream);
        bsl::string str1(stream.str(), s_allocator_p);
        bsl::string str2("     0:   00000024 45020000 163E0000 0300D431     "
                         "|...$E....>.....1|\n"
                         "    16:   00000000 00000000 00000000 00000000     "
                         "|................|\n"
                         "    32:   00002694                                "
                         "|..&.            |\n",
                         s_allocator_p);

        // Verify that dump contains expected value
        ASSERT_EQ(str1, str2);
        stream.reset();
    }

    // Iterator without blob
    {
        bmqp::AckMessageIterator iter;

        // Dump blob
        iter.dumpBlob(stream);
        bsl::string str1(stream.str(), s_allocator_p);
        bsl::string str2("/no blob/", s_allocator_p);

        // Verify that dump contains expected value
        ASSERT_EQ(str1, str2);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 5: test5_dumpBlob(); break;
    case 4: test4_resetMethod(); break;
    case 3: test3_nextMethod(); break;
    case 2: test2_ackEventWithMultipleMessages(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
