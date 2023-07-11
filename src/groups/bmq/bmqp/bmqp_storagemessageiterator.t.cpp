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

// bmqp_storagemessageiterator.t.cpp                                  -*-C++-*-
#include <bmqp_storagemessageiterator.h>

// BMQ
#include <bmqp_protocol.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_cstring.h>
#include <bsl_iostream.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

const int k_RECORD_SIZE = 48;  // Size of a journal record.  Must be 4-byte
                               // aligned.

struct Data {
    // DATA
    bmqp::StorageMessageType::Enum d_messageType;

    int d_flags;

    int d_spv;  // storage protocol version

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

/// Populate specified `blob` with a STORAGE event which has specified
/// `numMsgs` storage messages, update specified `eh` with corresponding
/// EventHeader, and update specified `data` with the expected values.
void populateBlob(bdlbb::Blob*       blob,
                  bmqp::EventHeader* eh,
                  bsl::vector<Data>* vec,
                  size_t             numMsgs)
{
    int eventLength = 0;

    eh->setType(bmqp::EventType::e_STORAGE);
    eh->setHeaderWords(sizeof(bmqp::EventHeader) /
                       bmqp::Protocol::k_WORD_SIZE);
    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(eh),
                            sizeof(bmqp::EventHeader));
    eventLength += sizeof(bmqp::EventHeader);

    for (size_t i = 0; i < numMsgs; ++i) {
        int  msgSize = 0;
        Data data(s_allocator_p);
        data.d_flags              = 1;
        data.d_spv                = 1;
        data.d_pid                = i % 100;
        data.d_journalOffsetWords = i * 100 + 5;

        size_t remainder = i % 6;

        if (0 == remainder) {
            data.d_messageType = bmqp::StorageMessageType::e_DATA;
            bsl::memset(data.d_recordBuffer, i, k_RECORD_SIZE);
            data.d_payload.resize(4 * i, i);  // payload is 4byte aligned
            msgSize += k_RECORD_SIZE + 4 * i;
        }
        else if (1 == remainder) {
            data.d_messageType = bmqp::StorageMessageType::e_QLIST;
            bsl::memset(data.d_recordBuffer, i, k_RECORD_SIZE);
            data.d_payload.resize(4 * i, i);  // payload is 4byte aligned
            msgSize += k_RECORD_SIZE + 4 * i;
        }
        else if (2 == remainder) {
            data.d_messageType = bmqp::StorageMessageType::e_CONFIRM;
            bsl::memset(data.d_recordBuffer, i, k_RECORD_SIZE);
            msgSize += k_RECORD_SIZE;
        }
        else if (3 == remainder) {
            data.d_messageType = bmqp::StorageMessageType::e_DELETION;
            bsl::memset(data.d_recordBuffer, i, k_RECORD_SIZE);
            msgSize += k_RECORD_SIZE;
        }
        else if (4 == remainder) {
            data.d_messageType = bmqp::StorageMessageType::e_QUEUE_OP;
            bsl::memset(data.d_recordBuffer, i, k_RECORD_SIZE);
            msgSize += k_RECORD_SIZE;
        }
        else {
            data.d_messageType = bmqp::StorageMessageType::e_JOURNAL_OP;
            bsl::memset(data.d_recordBuffer, i, k_RECORD_SIZE);
            msgSize += k_RECORD_SIZE;
        }

        // StorageHeader
        bmqp::StorageHeader shdr;
        shdr.setFlags(data.d_flags)
            .setStorageProtocolVersion(data.d_spv)
            .setHeaderWords(sizeof(bmqp::StorageHeader) /
                            bmqp::Protocol::k_WORD_SIZE)
            .setMessageType(data.d_messageType)
            .setPartitionId(data.d_pid)
            .setJournalOffsetWords(data.d_journalOffsetWords)
            .setMessageWords(shdr.headerWords() +
                             (msgSize / bmqp::Protocol::k_WORD_SIZE));

        bdlbb::BlobUtil::append(blob,
                                reinterpret_cast<const char*>(&shdr),
                                shdr.headerWords() *
                                    bmqp::Protocol::k_WORD_SIZE);

        // Append record
        bdlbb::BlobUtil::append(blob, data.d_recordBuffer, k_RECORD_SIZE);

        if (bmqp::StorageMessageType::e_DATA == data.d_messageType ||
            bmqp::StorageMessageType::e_QLIST == data.d_messageType) {
            bdlbb::BlobUtil::append(blob,
                                    data.d_payload.c_str(),
                                    data.d_payload.size());
        }

        eventLength += (shdr.messageWords() * bmqp::Protocol::k_WORD_SIZE);

        vec->push_back(data);
    }

    // Set EventHeader length
    bmqp::EventHeader* e = reinterpret_cast<bmqp::EventHeader*>(
        blob->buffer(0).data());
    e->setLength(eventLength);
}

void appendMessage(bdlbb::Blob*                   blob,
                   bmqp::EventHeader*             eh,
                   bmqp::StorageMessageType::Enum type,
                   unsigned int                   partitionId,
                   unsigned int                   journalOffsetWords,
                   int                            flags)
{
    int eventLength = 0;

    eh->setType(bmqp::EventType::e_STORAGE);
    eh->setHeaderWords(sizeof(bmqp::EventHeader) /
                       bmqp::Protocol::k_WORD_SIZE);
    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(eh),
                            sizeof(bmqp::EventHeader));
    eventLength += sizeof(bmqp::EventHeader);

    const char*  p    = "abcdefgh";
    unsigned int pLen = bsl::strlen(p);

    // StorageHeader
    bmqp::StorageHeader sh;
    sh.setFlags(flags)
        .setHeaderWords(sizeof(bmqp::StorageHeader) /
                        bmqp::Protocol::k_WORD_SIZE)
        .setMessageType(type)
        .setPartitionId(partitionId)
        .setJournalOffsetWords(journalOffsetWords)
        .setMessageWords(sh.headerWords() +
                         pLen / bmqp::Protocol::k_WORD_SIZE);

    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(&sh),
                            sh.headerWords() * bmqp::Protocol::k_WORD_SIZE);
    eventLength += (sh.messageWords() * bmqp::Protocol::k_WORD_SIZE);

    bdlbb::BlobUtil::append(blob, p, pLen);
    eventLength += pLen;

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
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);

    {
        // Create invalid iter
        bmqp::StorageMessageIterator iter;
        ASSERT_EQ(false, iter.isValid());
    }

    {
        // Create invalid iter from another invalid iter
        bmqp::StorageMessageIterator iter1;
        bmqp::StorageMessageIterator iter2(iter1);

        ASSERT_EQ(false, iter1.isValid());
        ASSERT_EQ(false, iter2.isValid());
    }

    {
        // Assigning invalid iter
        bmqp::StorageMessageIterator iter1, iter2;
        ASSERT_EQ(false, iter1.isValid());
        ASSERT_EQ(false, iter2.isValid());

        iter1 = iter2;
        ASSERT_EQ(false, iter1.isValid());
        ASSERT_EQ(false, iter2.isValid());
    }

    {
        // Create valid iter
        bdlbb::Blob blob(&bufferFactory, s_allocator_p);

        // Populate blob
        const int          flags = 2;
        const unsigned int pid   = 3;
        const unsigned int jow   = 400;
        bmqp::EventHeader  eventHeader;

        appendMessage(&blob,
                      &eventHeader,
                      bmqp::StorageMessageType::e_CONFIRM,
                      pid,
                      jow,
                      flags);

        // Iterate and verify
        bmqp::StorageMessageIterator iter(&blob, eventHeader);

        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(1, iter.next());
        ASSERT_EQ(flags, iter.header().flags());
        ASSERT_EQ(pid, iter.header().partitionId());
        ASSERT_EQ(jow, iter.header().journalOffsetWords());

        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(0, iter.next());
        ASSERT_EQ(false, iter.isValid());

        // Copy
        bmqp::StorageMessageIterator iter2(iter);
        ASSERT_EQ(false, iter2.isValid());

        // Clear
        iter.clear();
        ASSERT_EQ(false, iter.isValid());
        ASSERT_EQ(false, iter2.isValid());

        // Assign
        iter = iter2;
        ASSERT_EQ(false, iter.isValid());
        ASSERT_EQ(false, iter2.isValid());

        // Reset, iterate and verify again
        iter.reset(&blob, eventHeader);

        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(1, iter.next());
        ASSERT_EQ(flags, iter.header().flags());
        ASSERT_EQ(pid, iter.header().partitionId());
        ASSERT_EQ(jow, iter.header().journalOffsetWords());

        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(0, iter.next());
        ASSERT_EQ(false, iter.isValid());

        // Reset, iterate and verify again
        iter2.reset(&blob, eventHeader);
        iter.reset(&blob, iter2);

        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(1, iter.next());
        ASSERT_EQ(flags, iter.header().flags());
        ASSERT_EQ(pid, iter.header().partitionId());
        ASSERT_EQ(jow, iter.header().journalOffsetWords());

        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(0, iter.next());
        ASSERT_EQ(false, iter.isValid());
    }
}

static void test2_storageEventHavingMultipleMessages()
// ------------------------------------------------------------------------
// STORAGE EVENT HAVING MULTIPLE MESSAGES
//
// Concerns:
//   Test iterating over STORAGE event having multiple STORAGE messages.
//
// Testing:
//   Iterating over STORAGE event having multiple STORAGE messages.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("STORAGE EVENT HAVING MULTIPLE"
                                      " MESSAGES");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    bdlbb::Blob                    eventBlob(&bufferFactory, s_allocator_p);

    bsl::vector<Data> data(s_allocator_p);
    bmqp::EventHeader eventHeader;
    const size_t      NUM_MSGS = 5000;

    populateBlob(&eventBlob, &eventHeader, &data, NUM_MSGS);

    // Iterate and verify
    bmqp::StorageMessageIterator iter(&eventBlob, eventHeader);
    ASSERT_EQ(true, iter.isValid());

    size_t index  = 0;
    int    nextRc = -1;
    while (1 == (nextRc = iter.next()) && index < data.size()) {
        const Data& D = data[index];
        ASSERT_EQ_D(index, D.d_flags, iter.header().flags());
        ASSERT_EQ_D(index, D.d_messageType, iter.header().messageType());

        ASSERT_EQ_D(index, D.d_spv, iter.header().storageProtocolVersion());

        ASSERT_EQ_D(index, D.d_pid, iter.header().partitionId());

        // Compare journal record
        mwcu::BlobPosition position;
        ASSERT_EQ_D(index, 0, iter.loadDataPosition(&position));

        int compareRc = 0;
        ASSERT_EQ_D(index,
                    0,
                    mwcu::BlobUtil::compareSection(&compareRc,
                                                   eventBlob,
                                                   position,
                                                   D.d_recordBuffer,
                                                   k_RECORD_SIZE));
        ASSERT_EQ_D(index, 0, compareRc);

        if (bmqp::StorageMessageType::e_DATA == iter.header().messageType() ||
            bmqp::StorageMessageType::e_QLIST == iter.header().messageType()) {
            // Compare data payload as well.  Need to find data payload
            // position first.
            mwcu::BlobPosition dataPos;
            ASSERT_EQ_D(index,
                        0,
                        mwcu::BlobUtil::findOffsetSafe(&dataPos,
                                                       eventBlob,
                                                       position,
                                                       k_RECORD_SIZE));
            int compRc = 0;
            ASSERT_EQ_D(index,
                        0,
                        mwcu::BlobUtil::compareSection(&compRc,
                                                       eventBlob,
                                                       position,
                                                       D.d_payload.c_str(),
                                                       D.d_payload.length()));
            ASSERT_EQ_D(index, 0, compRc);
        }

        ++index;
    }

    ASSERT_EQ(nextRc, 0);
    ASSERT_EQ(data.size(), index);
    ASSERT_LT(iter.next(), 0);
    ASSERT_EQ(iter.isValid(), false);
}

static void test3_corruptedStorageEvent_part1()
// ------------------------------------------------------------------------
// CORRUPTED STORAGE EVENT - PART 1
//
// Concerns:
//   Test iterating over corrupted STORAGE event (not having enough bytes
//   for a storageHeader)
//
// Testing:
//   Iterating over corrupted STORAGE event (not having bytes for a full
//   storageHeader).
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("CORRUPTED STORAGE EVENT - PART 1");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    bdlbb::Blob                    eventBlob(&bufferFactory, s_allocator_p);
    const char*                    p    = "abcdefgh";
    unsigned int                   pLen = bsl::strlen(p);

    // BUILD the event blob ...
    // EventHeader
    bmqp::EventHeader eh(bmqp::EventType::e_STORAGE);
    eh.setLength(sizeof(bmqp::EventHeader) +
                 (2 * sizeof(bmqp::StorageHeader)));
    bdlbb::BlobUtil::append(&eventBlob,
                            reinterpret_cast<const char*>(&eh),
                            sizeof(bmqp::EventHeader));
    // StorageHeader (valid)
    bmqp::StorageHeader shd1;
    shd1.setFlags(1)
        .setHeaderWords(sizeof(bmqp::StorageHeader) /
                        bmqp::Protocol::k_WORD_SIZE)
        .setMessageType(bmqp::StorageMessageType::e_JOURNAL_OP)
        .setPartitionId(0)
        .setMessageWords(shd1.headerWords() +
                         pLen / bmqp::Protocol::k_WORD_SIZE);

    bdlbb::BlobUtil::append(&eventBlob,
                            reinterpret_cast<const char*>(&shd1),
                            sizeof(bmqp::StorageHeader));
    bdlbb::BlobUtil::append(&eventBlob, "abcdefgh", 8);

    // StorageHeader (invalid: missing a byte from the StorageHeader)
    bmqp::StorageHeader shd2;
    shd2.setFlags(2)
        .setHeaderWords(sizeof(bmqp::StorageHeader) /
                        bmqp::Protocol::k_WORD_SIZE)
        .setMessageType(bmqp::StorageMessageType::e_JOURNAL_OP)
        .setPartitionId(1)
        .setMessageWords(shd2.headerWords() - 1);

    bdlbb::BlobUtil::append(&eventBlob,
                            reinterpret_cast<const char*>(&shd2),
                            sizeof(bmqp::StorageHeader) - 1);

    // Iterate and check
    bmqp::StorageMessageIterator iter(&eventBlob, eh);

    // First message
    ASSERT_EQ(true, iter.isValid());
    ASSERT_EQ(1, iter.next());
    ASSERT_EQ(true, iter.isValid());
    ASSERT_EQ(1, iter.header().flags());
    ASSERT_EQ(0U, iter.header().partitionId());
    ASSERT_EQ(bmqp::StorageMessageType::e_JOURNAL_OP,
              iter.header().messageType());

    // Second message
    ASSERT_EQ(iter.isValid(), true);
    ASSERT_LT(iter.next(), 0);
    ASSERT_LT(iter.next(), 0);  // make sure successive
                                // call to next keep
                                // returning invalid
    ASSERT_EQ(iter.isValid(), false);
}

static void test4_corruptedStorageEvent_part2()
// ------------------------------------------------------------------------
// CORRUPTED STORAGE EVENT - PART 2
//
// Concerns:
//   Test iterating over corrupted STORAGE event (not having enough payload
//   bytes in the blob)
//
// Testing:
//   Iterating over corrupted STORAGE event (not having enough payload
//   bytes in the blob).
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("CORRUPTED STORAGE EVENT - PART 2");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    bdlbb::Blob                    eventBlob(&bufferFactory, s_allocator_p);

    // BUILD the event blob ...
    // EventHeader
    bmqp::EventHeader eh(bmqp::EventType::e_STORAGE);
    eh.setLength(sizeof(bmqp::EventHeader) +
                 (2 * sizeof(bmqp::StorageHeader)));
    bdlbb::BlobUtil::append(&eventBlob,
                            reinterpret_cast<const char*>(&eh),
                            sizeof(bmqp::EventHeader));
    // StorageHeader (valid)
    bmqp::StorageHeader shd1;
    shd1.setFlags(1)
        .setHeaderWords(sizeof(bmqp::StorageHeader) /
                        bmqp::Protocol::k_WORD_SIZE)
        .setMessageType(bmqp::StorageMessageType::e_CONFIRM)
        .setPartitionId(0)
        .setMessageWords(shd1.headerWords() +
                         (k_RECORD_SIZE / bmqp::Protocol::k_WORD_SIZE));

    bdlbb::BlobUtil::append(&eventBlob,
                            reinterpret_cast<const char*>(&shd1),
                            sizeof(bmqp::StorageHeader));

    bsl::string confirmRecord(s_allocator_p);
    confirmRecord.resize(k_RECORD_SIZE, 'a');

    bdlbb::BlobUtil::append(&eventBlob, confirmRecord.c_str(), k_RECORD_SIZE);

    // StorageHeader (invalid: missing a byte from the StorageHeader)
    bmqp::StorageHeader shd2;
    shd2.setFlags(2)
        .setHeaderWords(sizeof(bmqp::StorageHeader) /
                        bmqp::Protocol::k_WORD_SIZE)
        .setMessageType(bmqp::StorageMessageType::e_DELETION)
        .setPartitionId(1)
        .setMessageWords(shd2.headerWords() +
                         (k_RECORD_SIZE / bmqp::Protocol::k_WORD_SIZE));
    bdlbb::BlobUtil::append(&eventBlob,
                            reinterpret_cast<const char*>(&shd2),
                            sizeof(bmqp::StorageHeader));

    bdlbb::BlobUtil::append(&eventBlob,
                            confirmRecord.c_str(),
                            k_RECORD_SIZE - 2);  // incomplete

    // Iterate and check
    bmqp::StorageMessageIterator iter(&eventBlob, eh);

    // First message
    ASSERT_EQ(true, iter.isValid());
    ASSERT_EQ(1, iter.next());
    ASSERT_EQ(true, iter.isValid());
    ASSERT_EQ(1, iter.header().flags());
    ASSERT_EQ(0U, iter.header().partitionId());
    ASSERT_EQ(bmqp::StorageMessageType::e_CONFIRM,
              iter.header().messageType());

    // Second message
    ASSERT_EQ(iter.isValid(), true);
    ASSERT_LT(iter.next(), 0);
    ASSERT_LT(iter.next(), 0);  // make sure successive call to next keep
                                // returning invalid
    ASSERT_EQ(iter.isValid(), false);
}

static void test5_corruptedStorageEvent_part3()
{
    // ------------------------------------------------------------------------
    // CORRUPTED STORAGE EVENT - PART 3
    //
    // Concerns:
    //   Test iterating over corrupted STORAGE event (not having enough payload
    //   bytes in the blob)
    //
    // Testing:
    //   Iterating over corrupted STORAGE event (not having enough payload
    //   bytes in the blob).
    // ------------------------------------------------------------------------
    mwctst::TestHelper::printTestName("CORRUPTED STORAGE EVENT - PART 3");

    // Populate blob
    bsl::vector<Data> data(s_allocator_p);
    bmqp::EventHeader eventHeader;
    // Next method. No recovery header. Less then min header size.
    {
        // Min buf size not to reproduce given rc
        const size_t enoughSize = bmqp::StorageHeader::k_MIN_HEADER_SIZE +
                                  sizeof(bmqp::EventHeader);

        // Create buffer factory and blob
        bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
        bdlbb::Blob                    blob(&bufferFactory, s_allocator_p);

        // Populate blob
        populateBlob(&blob, &eventHeader, &data, 1);
        blob.setLength(enoughSize - 1);

        // Create iterator
        bmqp::StorageMessageIterator iter(&blob, eventHeader);
        ASSERT(iter.isValid());
        ASSERT_LT(iter.next(), 0);  // rc_NO_RECOVERYHEADER
        ASSERT(!iter.isValid());
    }
}
static void test6_resetMethod()
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
    mwctst::TestHelper::printTestName("RESET METHOD");

    // Populate blob
    bsl::vector<Data> data(s_allocator_p);
    bmqp::EventHeader eventHeader;

    // Reset method. Invalid event header case.
    {
        // Min buf size not to reproduce given rc
        const size_t enoughSize = sizeof(bmqp::EventHeader) + 1;
        bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);

        // Create buffer factory and blob
        bdlbb::Blob blob(&bufferFactory, s_allocator_p);

        // Populate blob and manually set length
        populateBlob(&blob, &eventHeader, &data, 1);

        // Create iterator
        bmqp::StorageMessageIterator iter(&blob, eventHeader);
        ASSERT(iter.isValid());

        // Invalidate the blob
        blob.setLength(enoughSize - 1);

        // Reset and verify
        ASSERT_LT(iter.reset(&blob, eventHeader),
                  0);  // rc_INVALID_EVENTHEADER
        ASSERT(!iter.isValid());
    }
}

static void test7_dumpBlob()
{
    // --------------------------------------------------------------------
    // DUMB BLOB
    //
    // Concerns:
    //   Check blob layout if:
    //     1. The underlying blob contain single storage message
    //     2. The underlying blob is not setted
    //
    // Plan:
    //   1. Create and populate blob with a single storage message
    //   2. Create and init iterator by given blob
    //   3. Check output of dumpBlob method
    //   4. Create and init iterator without setting underlying blob
    //   5. Check output of dumpBlob method
    //
    // Testing:
    //   void dumpBlob(bsl::ostream& stream);
    // --------------------------------------------------------------------
    mwctst::TestHelper::printTestName("DUMP BLOB");

    // Test iterator dump contains expected value
    bmqp::EventHeader              eventHeader;
    bsl::vector<Data>              data(s_allocator_p);
    mwcu::MemOutStream             stream(s_allocator_p);
    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    bdlbb::Blob                    blob(&bufferFactory, s_allocator_p);

    // Populate blob
    populateBlob(&blob, &eventHeader, &data, 1);

    // Create iterator over blob
    {
        bmqp::StorageMessageIterator iter(&blob, eventHeader);
        ASSERT(iter.isValid());

        // Dump blob
        iter.dumpBlob(stream);
        bsl::string str1(stream.str(), s_allocator_p);
        bsl::string str2("     0:   00000044 48020000 0800000F 13010000     "
                         "|...DH...........|\n"
                         "    16:   00000005 00000000 00000000 00000000     "
                         "|................|\n"
                         "    32:   00000000 00000000 00000000 00000000     "
                         "|................|\n"
                         "    48:   00000000 00000000 00000000 00000000     "
                         "|................|\n"
                         "    64:   00000000                                "
                         "|....            |\n",
                         s_allocator_p);

        // Verify that dump contains expected value
        ASSERT_EQ(str1, str2);
        stream.reset();
    }

    // Create iterator without blob
    {
        bmqp::StorageMessageIterator iter;
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

    // Temporary workaround to suppress the 'unused operator
    // NestedTraitDeclaration' warning/error generated by clang.  TBD: figure
    // out the right way to "fix" this.
    Data dummy(s_allocator_p);
    static_cast<void>(
        static_cast<
            bslmf::NestedTraitDeclaration<Data, bslma::UsesBslmaAllocator> >(
            dummy));

    switch (_testCase) {
    case 0:
    case 7: test7_dumpBlob(); break;
    case 6: test6_resetMethod(); break;
    case 5: test5_corruptedStorageEvent_part3(); break;
    case 4: test4_corruptedStorageEvent_part2(); break;
    case 3: test3_corruptedStorageEvent_part1(); break;
    case 2: test2_storageEventHavingMultipleMessages(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
