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

// bmqp_protocolutil.t.cpp                                            -*-C++-*-
#include <bmqp_protocolutil.h>

#include <bmqu_memoutstream.h>

// BMQ
#include <bmqp_crc32c.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqp_messageguidgenerator.h>
#include <bmqp_messageproperties.h>
#include <bmqp_protocol.h>
#include <bmqp_puteventbuilder.h>
#include <bmqp_putmessageiterator.h>
#include <bmqp_queueid.h>
#include <bmqt_resultcode.h>

// BDE
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_limits.h>
#include <bsl_string.h>
#include <bsl_vector.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_initializeShutdown()
// ------------------------------------------------------------------------
// INITIALIZE / SHUTDOWN
//
// Concerns:
//   1. Calling 'shutdown' without a call to 'initialize' should assert.
//   2. Should be able to call 'initialize()' after the instance has
//      already started and have no effect.
//   3. Should be able to call 'shutdown' after already calling 'shutdown'
//      and have no effect, provided that the number of calls to 'shutdown'
//      does not exceed to number of calls to 'initialize' without a
//      corresponding call to 'shutdown'.
//   4. It is safe to call 'initialize' after calling 'shutdown'.
//
// Plan:
//   1. Assert failure of calling 'shutdown' without prior call to
//      'initialize'.
//   2. Assert pass of multiple calls to 'initialize'.
//   3. Assert pass of multiple calls to 'shutdown' and then assert fail of
//      one too many calls to 'shutdown'.
//   4. Assert pass a call to 'initialize' and then a call to 'shutdown'.
//
// Testing:
//   initialize
//   shutdown
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("INITIALIZE / SHUTDOWN");

    // 1. Calling 'shutdown' without a call to 'initialize' should assert.
    ASSERT_SAFE_FAIL(bmqp::ProtocolUtil::shutdown());

    // 2. Should be able to call 'initialize()' after the instance has already
    //    started and have no effect.
    // Initialize the 'ProtocolUtil'
    ASSERT_SAFE_PASS(
        bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator()));

    // 'initialize' should be a no-op
    ASSERT_SAFE_PASS(
        bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator()));

    // 3. Should be able to call 'shutdown' after already calling 'shutdown'
    //    and have no effect, provided that the number of calls to 'shutdown'
    //    does not exceed to number of calls to 'initialize' without a
    //    corresponding call to 'shutdown'.
    // 'shutdown' should be a no-op
    ASSERT_SAFE_PASS(bmqp::ProtocolUtil::shutdown());

    // Shut down the 'ProtocolUtil'
    ASSERT_SAFE_PASS(bmqp::ProtocolUtil::shutdown());

    // 'shutdown' again should assert
    ASSERT_SAFE_FAIL(bmqp::ProtocolUtil::shutdown());

    // 4. It is safe to call 'initialize' after calling 'shutdown'.
    ASSERT_SAFE_PASS(
        bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator()));

    // Finally, shutdown the 'ProtocolUtil'
    ASSERT_SAFE_PASS(bmqp::ProtocolUtil::shutdown());
}

static void test2_hexBinaryConversions()
// ------------------------------------------------------------------------
//                       HEX/BINARY CONVERSIONS
// ------------------------------------------------------------------------
//
// Concerns:
//   Verify the correctness of binary to hex and hex to binary conversion.
//
// Testing:
//   - void hexToBinary(char *buffer, int length, const char *hex)
//   - void binaryToHex(char *buffer, char *binary, int binaryBufferLength)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HEX/BINARY CONVERSIONS");

    PV("hexToBinary");
    {
        struct Test {
            int                 d_line;
            const char*         d_hex;
            const unsigned char d_expected[8];
        } k_DATA[] = {
            {L_, "0000000000000000", {0, 0, 0, 0, 0, 0, 0, 0}},
            {L_, "FFFFFFFFFFFFFFFF", {255, 255, 255, 255, 255, 255, 255, 255}},
            {L_,
             "0011223344556677",
             {0x0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77}},
            {L_,
             "8899AABBCCDDEEFF",
             {0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}},
            {L_,
             "0123456789ABCDEF",
             {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF}},
        };

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test& test = k_DATA[idx];

            PVV(test.d_line << ": converting '" << test.d_hex << "' to bin");
            char buffer[8] = {0};
            bmqp::ProtocolUtil::hexToBinary(buffer, 8, test.d_hex);
            ASSERT_EQ_D("line " << test.d_line,
                        0,
                        bsl::memcmp(test.d_expected, buffer, 8));
        }
    }

    PV("binaryToHex");
    {
        struct Test {
            int                 d_line;
            const unsigned char d_binary[8];
            const char*         d_expected;
        } k_DATA[] = {
            {L_, {0, 0, 0, 0, 0, 0, 0, 0}, "0000000000000000"},
            {L_, {255, 255, 255, 255, 255, 255, 255, 255}, "FFFFFFFFFFFFFFFF"},
            {L_,
             {0x0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77},
             "0011223344556677"},
            {L_,
             {0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
             "8899AABBCCDDEEFF"},
            {L_,
             {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF},
             "0123456789ABCDEF"},
            {L_, {0x01, 0x02, 0x03}, "010203"},
        };

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test& test = k_DATA[idx];

            PVV(test.d_line << ": converting to hex (expected: "
                            << test.d_expected << ")");
            char   buffer[16] = {0};
            size_t l          = strlen(test.d_expected) / 2;
            bmqp::ProtocolUtil::binaryToHex(
                buffer,
                reinterpret_cast<const char*>(test.d_binary),
                l);
            ASSERT_EQ_D("line " << test.d_line,
                        0,
                        bsl::memcmp(test.d_expected, buffer, l * 2));
        }
    }

    PV("Isomorphism");
    {
        const char k_HEX[]               = "0123456789ABCDEF";
        const char k_BINARY[]            = {4, 'a', '?', '1', 0, 22, 127, '*'};
        const int  k_SIZE                = 8;
        char       binaryToFill[k_SIZE]  = {0};
        char       hexToFill[2 * k_SIZE] = {0};

        PVV("binaryToHex(hexToBinary(x))");
        bmqp::ProtocolUtil::hexToBinary(binaryToFill, k_SIZE, k_HEX);
        bmqp::ProtocolUtil::binaryToHex(hexToFill, binaryToFill, k_SIZE);
        ASSERT_EQ(0, bsl::memcmp(k_HEX, hexToFill, k_SIZE * 2));

        PVV("hexToBinary(binaryToHex(x))");
        bmqp::ProtocolUtil::binaryToHex(hexToFill, k_BINARY, k_SIZE);
        bmqp::ProtocolUtil::hexToBinary(binaryToFill, k_SIZE, hexToFill);
        ASSERT_EQ(0, bsl::memcmp(k_BINARY, binaryToFill, k_SIZE));
    }
}

static void test3_calcNumWordsAndPadding()
// ------------------------------------------------------------------------
//                     CALC NUM WORDS AND PADDING
// ------------------------------------------------------------------------
//
// Concerns:
//   Verify the correctness of num words and padding bytes computation for
//   both words and dwords.
//
// Testing:
//   - int calcNumWordsAndPadding(int *padding, int length)
//   - int calcNumDwordsAndPAdding(int *padding, int length)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CALC NUM WORDS AND PADDING");

    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    struct Test {
        int d_line;
        int d_length;
        int d_wordPadding;
        int d_wordNumWords;
        int d_dwordPadding;
        int d_dwordNumWords;
    } k_DATA[] = {
        {L_, 0, 4, 1, 8, 1},
        {L_, 1, 3, 1, 7, 1},
        {L_, 2, 2, 1, 6, 1},
        {L_, 3, 1, 1, 5, 1},
        {L_, 4, 4, 2, 4, 1},
        {L_, 5, 3, 2, 3, 1},
        {L_, 6, 2, 2, 2, 1},
        {L_, 7, 1, 2, 1, 1},
        {L_, 8, 4, 3, 8, 2},
        {L_, 9, 3, 3, 7, 2},
        {L_, 123, 1, 31, 5, 16},
        {L_, 1023, 1, 256, 1, 128},
        {L_, 1024, 4, 257, 8, 129},
        {L_, 1025, 3, 257, 7, 129},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        int padding = 0;
        int words   = 0;

        // WORDS
        words = bmqp::ProtocolUtil::calcNumWordsAndPadding(&padding,
                                                           test.d_length);
        ASSERT_EQ_D("line " << test.d_line, padding, test.d_wordPadding);
        ASSERT_EQ_D("line " << test.d_line, words, test.d_wordNumWords);

        // DWORD
        words = bmqp::ProtocolUtil::calcNumDwordsAndPadding(&padding,
                                                            test.d_length);
        ASSERT_EQ_D("line " << test.d_line, padding, test.d_dwordPadding);
        ASSERT_EQ_D("line " << test.d_line, words, test.d_dwordNumWords);
    }

    bmqp::ProtocolUtil::shutdown();
}

static void test4_paddingChar()
// ------------------------------------------------------------------------
//                        APPEND PADDING (CHAR)
// ------------------------------------------------------------------------
//
// Concerns:
//   Verify the correctness of append padding, the char variants for both
//   words and dwords.
//
// Testing:
//   - void appendPaddingRaw(char *destination, int numPaddingBytes)
//   - void appendPaddingDwordRaw(char *destination, int numPaddingBytes)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("APPEND PADDING (char)");

    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    PV("WORD");
    {
        struct Test {
            int  d_line;
            int  d_numPaddingBytes;
            char d_expectedBuffer[5];
        } k_DATA[] = {
            {L_, 1, {1, 0x0B, 0x0C, 0x0D, 0x0E}},
            {L_, 2, {2, 2, 0x0C, 0x0D, 0x0E}},
            {L_, 3, {3, 3, 3, 0x0D, 0x0E}},
            {L_, 4, {4, 4, 4, 4, 0x0E}},
        };

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test& test = k_DATA[idx];

            PVV(test.d_line << ": word padding of " << test.d_numPaddingBytes
                            << " bytes");
            char buffer[5] = {0x0A, 0x0B, 0x0C, 0x0D, 0x0E};
            bmqp::ProtocolUtil::appendPaddingRaw(buffer,
                                                 test.d_numPaddingBytes);
            ASSERT_EQ_D("line " << test.d_line,
                        0,
                        bsl::memcmp(test.d_expectedBuffer, buffer, 5));
        }
    }

    PV("DWORD");
    {
        struct Test {
            int  d_line;
            int  d_numPaddingBytes;
            char d_expectedBuffer[9];
        } k_DATA[] = {
            {L_, 1, {1, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x12, 0x34}},
            {L_, 2, {2, 2, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x12, 0x34}},
            {L_, 3, {3, 3, 3, 0x0D, 0x0E, 0x0F, 0x10, 0x12, 0x34}},
            {L_, 4, {4, 4, 4, 4, 0x0E, 0x0F, 0x10, 0x12, 0x34}},
            {L_, 5, {5, 5, 5, 5, 5, 0x0F, 0x10, 0x12, 0x34}},
            {L_, 6, {6, 6, 6, 6, 6, 6, 0x10, 0x12, 0x34}},
            {L_, 7, {7, 7, 7, 7, 7, 7, 7, 0x12, 0x34}},
            {L_, 8, {8, 8, 8, 8, 8, 8, 8, 8, 0x34}},
        };

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test& test = k_DATA[idx];

            PVV(test.d_line << ": word padding of " << test.d_numPaddingBytes
                            << " bytes");
            char buffer[9] =
                {0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x12, 0x34};
            bmqp::ProtocolUtil::appendPaddingDwordRaw(buffer,
                                                      test.d_numPaddingBytes);
            ASSERT_EQ_D("line " << test.d_line,
                        0,
                        bsl::memcmp(test.d_expectedBuffer, buffer, 9));
        }
    }

    bmqp::ProtocolUtil::shutdown();
}

static void test5_paddingBlob()
// ------------------------------------------------------------------------
//                        APPEND PADDING (BLOB)
// ------------------------------------------------------------------------
//
// Concerns:
//   Verify the correctness of append padding, the blob variant.
//
// Testing:
//   - void appendPaddingRaw(bdlbb::Blob *destination, int numPaddingBytes)
//   - void appendPadding(bdlbb::Blob *destination, int payloadLength)
//
// NOTE: This test case mostly only focuses on 'appendPaddingRaw', because
//       'appendPadding' is simply a basic wrapper on 'appendPaddingRaw',
//       but is being called for coverage purposes.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("APPEND PADDING (blob)");

    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    PV("blob with enough capacity in last buffer");
    {
        bdlbb::PooledBlobBufferFactory bufferFactory(
            5,
            bmqtst::TestHelperUtil::allocator());

        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

        // Initialize the blob: it will have one buffer of capacity 5, and a
        // total size of 3.
        bdlbb::BlobUtil::append(&blob, "ABC", 3);
        ASSERT_EQ(blob.length(), 3);
        ASSERT_EQ(blob.numDataBuffers(), 1);

        bmqp::ProtocolUtil::appendPaddingRaw(&blob, 2);
        // Appending two bytes of padding to this blob: this will fit in
        // the already existing buffer.
        ASSERT_EQ(blob.length(), 5);
        ASSERT_EQ(blob.numDataBuffers(), 1);
        const char expectedBufferContent[] = {'A', 'B', 'C', 2, 2};
        ASSERT_EQ(
            0,
            bsl::memcmp(blob.buffer(0).data(), expectedBufferContent, 5));
    }

    PV("blob without enough capacity in last buffer");
    {
        bdlbb::PooledBlobBufferFactory bufferFactory(
            5,
            bmqtst::TestHelperUtil::allocator());

        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

        // Initialize the blob: it will have one buffer of capacity 5, and a
        // total size of 3.
        bdlbb::BlobUtil::append(&blob, "ABC", 3);
        ASSERT_EQ(blob.length(), 3);
        ASSERT_EQ(blob.numDataBuffers(), 1);

        bmqp::ProtocolUtil::appendPaddingRaw(&blob, 4);
        // Appending four bytes of padding to this blob: this will not fit
        // in the already existing buffer, and will end up appending a new
        // buffer with just the padding bytes.
        ASSERT_EQ(blob.length(), 7);
        ASSERT_EQ(blob.numDataBuffers(), 2);
        ASSERT_EQ(blob.buffer(0).size(), 3);
        ASSERT_EQ(blob.buffer(1).size(), 4);
        ASSERT_EQ(blob.lastDataBufferLength(), 4);

        const char expectedBuffer1Content[] = {'A', 'B', 'C'};
        const char expectedBuffer2Content[] = {4, 4, 4, 4};

        ASSERT_EQ(
            0,
            bsl::memcmp(blob.buffer(0).data(), expectedBuffer1Content, 3));
        ASSERT_EQ(
            0,
            bsl::memcmp(blob.buffer(1).data(), expectedBuffer2Content, 4));
    }

    PV("append padding");
    {
        bdlbb::PooledBlobBufferFactory bufferFactory(
            5,
            bmqtst::TestHelperUtil::allocator());

        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

        // Initialize the blob: it will have one buffer of capacity 5, and a
        // total size of 3.
        bdlbb::BlobUtil::append(&blob, "ABC", 3);
        ASSERT_EQ(blob.length(), 3);
        ASSERT_EQ(blob.numDataBuffers(), 1);

        bmqp::ProtocolUtil::appendPadding(&blob, 3);
        // Expecting it to add 1 byte of padding
        ASSERT_EQ(blob.length(), 4);
        ASSERT_EQ(blob.numDataBuffers(), 1);
        const char expectedBufferContent[] = {'A', 'B', 'C', 1};
        ASSERT_EQ(
            0,
            bsl::memcmp(blob.buffer(0).data(), expectedBufferContent, 4));
    }

    bmqp::ProtocolUtil::shutdown();
}

static void test6_heartbeatAndEmptyBlobs()
// ------------------------------------------------------------------------
//                           HEARTBEAT AND EMPTY BLOBS
// ------------------------------------------------------------------------
//
// Concerns:
//   Verify the statically created blobs for the heartbeat request and
//   response, and empty blobs are correct.
//
// Testing:
//   - const bdlbb::Blob& heartbeatReqBlob();
//   - const bdlbb::Blob& heartbeatRspBlob();
//   - const bdlbb::Blob& emptyBlob();
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HEARTBEAT BLOBS");

    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    PV("Verifying the HeartbeatReq blob")
    {
        bmqp::EventHeader expectedHeader(bmqp::EventType::e_HEARTBEAT_REQ);

        const bdlbb::Blob& blob = bmqp::ProtocolUtil::heartbeatReqBlob();

        ASSERT_EQ(blob.length(), static_cast<int>(sizeof(bmqp::EventHeader)));
        ASSERT_EQ(blob.numDataBuffers(), 1);
        ASSERT_EQ(0,
                  memcmp(blob.buffer(0).data(),
                         &expectedHeader,
                         sizeof(bmqp::EventHeader)));
    }

    PV("Verifying the HeartbeatRsp blob")
    {
        bmqp::EventHeader expectedHeader(bmqp::EventType::e_HEARTBEAT_RSP);

        const bdlbb::Blob& blob = bmqp::ProtocolUtil::heartbeatRspBlob();

        ASSERT_EQ(blob.length(), static_cast<int>(sizeof(bmqp::EventHeader)));
        ASSERT_EQ(blob.numDataBuffers(), 1);
        ASSERT_EQ(0,
                  memcmp(blob.buffer(0).data(),
                         &expectedHeader,
                         sizeof(bmqp::EventHeader)));
    }

    PV("Verifying emtpy blob")
    {
        const bdlbb::Blob& blob = bmqp::ProtocolUtil::emptyBlob();
        ASSERT_EQ(0, blob.length());
        ASSERT_EQ(0, blob.numDataBuffers());
    }

    bmqp::ProtocolUtil::shutdown();
}

static void test7_ackResultToCode()
// ------------------------------------------------------------------------
// ACK RESULT TO CODE
//
// Concerns:
//   Proper behavior of the 'ackResultToCode' method.
//
// Plan:
//   Verify that the 'ackResultToCode' method returns the correct code of
//   every applicable enum value of 'bmqt::AckResult'.
//
// Testing:
//   ackResultToCode
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ACK RESULT TO CODE");

    struct Test {
        int                   d_line;
        bmqt::AckResult::Enum d_ackResult;
        int                   d_expectedCode;
    } k_DATA[] = {{L_, bmqt::AckResult::e_SUCCESS, 0},
                  {L_, bmqt::AckResult::e_LIMIT_MESSAGES, 1},
                  {L_, bmqt::AckResult::e_LIMIT_BYTES, 2},
                  {L_, bmqt::AckResult::e_STORAGE_FAILURE, 6},
                  {L_, bmqt::AckResult::e_UNKNOWN, 5},
                  {L_, bmqt::AckResult::e_TIMEOUT, 5},
                  {L_, bmqt::AckResult::e_NOT_CONNECTED, 5},
                  {L_, bmqt::AckResult::e_CANCELED, 5},
                  {L_, bmqt::AckResult::e_NOT_SUPPORTED, 5},
                  {L_, bmqt::AckResult::e_REFUSED, 5},
                  {L_, bmqt::AckResult::e_INVALID_ARGUMENT, 5},
                  {L_, bmqt::AckResult::e_NOT_READY, 7}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": Testing: bmqp::ProtocolUti::ackResultToCode("
                        << test.d_ackResult << ") == " << test.d_expectedCode);

        ASSERT_EQ_D(test.d_line,
                    bmqp::ProtocolUtil::ackResultToCode(test.d_ackResult),
                    test.d_expectedCode);
    }
}

static void test8_ackResultFromCode()
// ------------------------------------------------------------------------
// ACK RESULT FROM CODE
//
// Concerns:
//   Proper behavior of the 'ackResultFromCode' method.
//
// Plan:
//   Verify that the 'ackResultFromCode' method returns the correct
//   AckResult of every applicable code.
//
// Testing:
//   ackResultFromCode
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ACK RESULT FROM CODE");

    struct Test {
        int                   d_line;
        int                   d_code;
        bmqt::AckResult::Enum d_expectedAckResult;
    } k_DATA[] = {
        {L_, 0, bmqt::AckResult::e_SUCCESS},
        {L_, 1, bmqt::AckResult::e_LIMIT_MESSAGES},
        {L_, 2, bmqt::AckResult::e_LIMIT_BYTES},
        {L_, 6, bmqt::AckResult::e_STORAGE_FAILURE},
        {L_, 5, bmqt::AckResult::e_UNKNOWN},
        {L_, -1, bmqt::AckResult::e_UNKNOWN},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": Testing: bmqp::ProtocolUti::ackResultFromCode("
                        << test.d_code << ") == " << test.d_expectedAckResult);

        ASSERT_EQ_D(test.d_line,
                    bmqp::ProtocolUtil::ackResultFromCode(test.d_code),
                    test.d_expectedAckResult);
    }
}

static void test9_loadFieldValues()
// ------------------------------------------------------------------------
// LOAD FIELD VALUES
//
// Concerns:
//   Proper behavior of the 'loadFieldValues' method.
//
// Plan:
//   Verify that:
//     1. If the input field name is valid, load the corresponding values
//        into the output vector and return true.
//     2. If the input field name is invalid, do not modify the output
//        vector and return false.
//
// Testing:
//   loadFieldValues
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LOAD FIELD VALUES");
    // Disable check that no memory was allocated from the default allocator
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    const bsl::string  field1("f1", bmqtst::TestHelperUtil::allocator());
    const bsl::string  val1("v1", bmqtst::TestHelperUtil::allocator());
    const bsl::string  val2("v2", bmqtst::TestHelperUtil::allocator());
    const bsl::string  val3("v3", bmqtst::TestHelperUtil::allocator());
    const unsigned int field1NumVal = 3;

    const bsl::string  field2("f2", bmqtst::TestHelperUtil::allocator());
    const bsl::string  val4("v4", bmqtst::TestHelperUtil::allocator());
    const unsigned int field2NumVal = 1;

    const bsl::string emptyField("fEmpty",
                                 bmqtst::TestHelperUtil::allocator());

    const bsl::string featureSet(field1 + ":" + val1 + "," + val2 + "," +
                                     val3 + ";" + field2 + ":" + val4 + ";" +
                                     emptyField,
                                 bmqtst::TestHelperUtil::allocator());

    PV("Load valid fields");
    {
        bsl::vector<bsl::string> field1Values(
            bmqtst::TestHelperUtil::allocator());

        ASSERT(bmqp::ProtocolUtil::loadFieldValues(&field1Values,
                                                   field1,
                                                   featureSet));
        ASSERT_EQ(field1NumVal, field1Values.size());
        ASSERT_EQ(val1, field1Values[0]);
        ASSERT_EQ(val2, field1Values[1]);
        ASSERT_EQ(val3, field1Values[2]);

        bsl::vector<bsl::string> field2Values(
            bmqtst::TestHelperUtil::allocator());

        ASSERT(bmqp::ProtocolUtil::loadFieldValues(&field2Values,
                                                   field2,
                                                   featureSet));
        ASSERT_EQ(field2NumVal, field2Values.size());
        ASSERT_EQ(val4, field2Values[0]);

        PVV("Load field with no specified value");
        bsl::vector<bsl::string> emptyFieldValues(
            bmqtst::TestHelperUtil::allocator());

        ASSERT(bmqp::ProtocolUtil::loadFieldValues(&emptyFieldValues,
                                                   emptyField,
                                                   featureSet));
        ASSERT(emptyFieldValues.empty());
    }

    PV("Load invalid field");
    {
        bsl::vector<bsl::string> invalidFieldValues(
            bmqtst::TestHelperUtil::allocator());
        const bsl::string invalidField("invalidField",
                                       bmqtst::TestHelperUtil::allocator());

        ASSERT(!bmqp::ProtocolUtil::loadFieldValues(&invalidFieldValues,
                                                    invalidField,
                                                    featureSet));
        ASSERT(invalidFieldValues.empty());
    }

    PV("Load from malformed feature set");
    {
        bsl::vector<bsl::string> field1Values(
            bmqtst::TestHelperUtil::allocator());
        const bsl::string malformedFeatureSet(
            field1 + ":",
            bmqtst::TestHelperUtil::allocator());

        ASSERT(!bmqp::ProtocolUtil::loadFieldValues(&field1Values,
                                                    field1,
                                                    malformedFeatureSet));
        ASSERT(field1Values.empty());
    }
}

template <typename E>
static void encodeDecodeHelper(E encodingType)
{
    bmqu::MemOutStream                 ms;
    bdlbb::PooledBlobBufferFactory     bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqp_ctrlmsg::ClusterMessage       clusterMessage;
    bmqp_ctrlmsg::LeaderAdvisoryCommit commit;

    // Prepend a dummy buffer to force the message to start at an offset (for
    // the decode)
    bdlbb::BlobUtil::append(&blob, "dummyBuf", 8);

    // 1. Create an update message
    bmqp_ctrlmsg::LeaderMessageSequence lms;
    lms.electorTerm()    = 3U;
    lms.sequenceNumber() = 8U;

    commit.sequenceNumber()                           = lms;
    commit.sequenceNumberCommitted().electorTerm()    = 2U;
    commit.sequenceNumberCommitted().sequenceNumber() = 99U;

    clusterMessage.choice().makeLeaderAdvisoryCommit(commit);

    int rc = bmqp::ProtocolUtil::encodeMessage(ms,
                                               &blob,
                                               clusterMessage,
                                               encodingType);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(ms.str(), "");
    ASSERT_NE(blob.length(), 0);

    // Decode and verify
    bmqp_ctrlmsg::ClusterMessage decodedClusterMessage;
    ASSERT_NE(decodedClusterMessage, clusterMessage);

    rc = bmqp::ProtocolUtil::decodeMessage(ms,
                                           &decodedClusterMessage,
                                           blob,
                                           8,  // offset
                                           encodingType);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(ms.str(), "");
    ASSERT_EQ(decodedClusterMessage, clusterMessage);
}

static void test10_encodeDecodeMessage()
// ------------------------------------------------------------------------
// ENCODE DECODE MESSAGE
//
// Concerns:
//   Proper behavior of the 'encodeMessage', 'decodeMessage' methods.
//
// Plan:
//   1 Verify that can successfully 'encodeMessage'
//   2 Verify that can successfully 'decodeMessage' on previously encoded
//     message
//   3 Verify that original message equals decoded message
//
// Testing:
//   encodeMessage
//   decodeMessage
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // The default allocator check fails in this test case because the
    // 'loggedMessages' methods of Encoder returns a memory-aware object
    // without utilizing the parameter allocator.

    bmqtst::TestHelper::printTestName("ENCODE DECODE MESSAGE");

    PV("Test using JSON encoding");
    {
        encodeDecodeHelper(bmqp::EncodingType::e_JSON);
    }

    PV("Test using BER encoding");
    {
        encodeDecodeHelper(bmqp::EncodingType::e_BER);
    }
}

static void encode(bmqp::MessageProperties* properties)
{
    ASSERT_EQ(0, properties->setPropertyAsInt32("encoding", 3));
    ASSERT_EQ(0, properties->setPropertyAsString("id", "3"));
    ASSERT_EQ(0, properties->setPropertyAsInt64("timestamp", 3LL));
    ASSERT_EQ(3, properties->numProperties());
}

static void verify(const bmqp::MessageProperties& properties)
{
    ASSERT_EQ(3, properties.numProperties());
    ASSERT_EQ(properties.getPropertyAsInt32("encoding"), 3);
    ASSERT_EQ(properties.getPropertyAsString("id"), "3");
    ASSERT_EQ(properties.getPropertyAsInt64("timestamp"), 3LL);
}

static void populateBlob(bdlbb::Blob* blob, int atLeastLen)
{
    const char* k_FIXED_PAYLOAD =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdef";

    const int k_FIXED_PAYLOAD_LEN = bsl::strlen(k_FIXED_PAYLOAD);

    int numIters = atLeastLen / k_FIXED_PAYLOAD_LEN + 1;

    for (int i = 0; i < numIters; ++i) {
        bdlbb::BlobUtil::append(blob, k_FIXED_PAYLOAD, k_FIXED_PAYLOAD_LEN);
    }
}

static void test11_parseMessageProperties()
// ------------------------------------------------------------------------
// TESTS PARSING AS IT IS USED IN QueueEngineUtil::logRejectMessage
//
// Concerns:
//   - Verify ProtocolUtil::parse.
//
// Plan:
//   Call ProtocolUtil::parse.
//
// ------------------------------------------------------------------------
{
    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    bmqtst::TestHelper::printTestName("TEST PARSING");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::MessageProperties in(bmqtst::TestHelperUtil::allocator());
    encode(&in);
    const int             queueId = 4;
    bmqp::PutEventBuilder peb(&bufferFactory,
                              bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob payload(&bufferFactory, bmqtst::TestHelperUtil::allocator());

    populateBlob(&payload, 2 * bmqp::Protocol::k_COMPRESSION_MIN_APPDATA_SIZE);

    peb.startMessage();
    peb.setMessagePayload(&payload);
    peb.setMessageProperties(&in);
    peb.setCompressionAlgorithmType(bmqt::CompressionAlgorithmType::e_ZLIB);
    peb.setMessageGUID(bmqp::MessageGUIDGenerator::testGUID());

    bmqt::EventBuilderResult::Enum builderResult = peb.packMessage(queueId);

    ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, builderResult);

    bmqp::PutMessageIterator putIt(&bufferFactory,
                                   bmqtst::TestHelperUtil::allocator(),
                                   true);
    bmqp::Event rawEvent(&peb.blob(), bmqtst::TestHelperUtil::allocator());

    BSLS_ASSERT_SAFE(rawEvent.isPutEvent());
    rawEvent.loadPutMessageIterator(&putIt);

    ASSERT_EQ(1, putIt.next());

    bdlbb::Blob payloadIn(&bufferFactory, bmqtst::TestHelperUtil::allocator());

    putIt.loadApplicationData(&payloadIn);

    bdlbb::Blob msgPropertiesBlob(&bufferFactory,
                                  bmqtst::TestHelperUtil::allocator());
    int         messagePropertiesSize = 0;
    bdlbb::Blob payloadOut(&bufferFactory,
                           bmqtst::TestHelperUtil::allocator());
    int         rc = bmqp::ProtocolUtil::parse(&msgPropertiesBlob,
                                       &messagePropertiesSize,
                                       &payloadOut,
                                       payloadIn,
                                       payloadIn.length(),
                                       true,  // decompress
                                       bmqu::BlobPosition(),
                                       true,  // MPs
                                       true,  // new style
                                       peb.compressionAlgorithmType(),
                                       &bufferFactory,
                                       bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(0, rc);
    bmqp::MessageProperties out(bmqtst::TestHelperUtil::allocator());
    out.streamIn(msgPropertiesBlob, true);

    verify(out);

    ASSERT_EQ(0, bdlbb::BlobUtil::compare(payloadOut, payload));

    bmqp::ProtocolUtil::shutdown();
}

// ============================================================================
//                                MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    bmqp::Crc32c::initialize();

    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 11: test11_parseMessageProperties(); break;
    case 10: test10_encodeDecodeMessage(); break;
    case 9: test9_loadFieldValues(); break;
    case 8: test8_ackResultFromCode(); break;
    case 7: test7_ackResultToCode(); break;
    case 6: test6_heartbeatAndEmptyBlobs(); break;
    case 5: test5_paddingBlob(); break;
    case 4: test4_paddingChar(); break;
    case 3: test3_calcNumWordsAndPadding(); break;
    case 2: test2_hexBinaryConversions(); break;
    case 1: test1_initializeShutdown(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
