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

// bmqp_putmessageiterator.t.cpp                                      -*-C++-*-
#include <bmqp_putmessageiterator.h>

// BMQ
#include <bmqp_messageguidgenerator.h>
#include <bmqp_messageproperties.h>
#include <bmqp_optionutil.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqp_puttester.h>
#include <bmqt_messageguid.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bslma_default.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

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
//   - Create invalid iterator and assert validity.
//   - Create invalid iterator from an existing invalid iterator and check
//     the validity for both.
//   - Assign an invalid iterator and check the validity.
//   - Create a valid iterator which does not allow decompression i.e. the
//     decompress flag is set to false. The iterator has messages which
//     have not been compressed.
//   - Create a valid iterator which does not allow decompression i.e. the
//     decompress flag is set to false. The iterator has messages which
//     have been compressed using zlib algorithm.
//   - Create a valid iterator which allows decompression i.e. the
//     decompress flag is set to true. The iterator has messages which
//     have not been compressed.
//   - Create a valid iterator which allows decompression i.e. the
//     decompress flag is set to true. The iterator has messages which
//     have been compressed using ZLIB compression algorithm.
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
        PVV("CREATE INVALID ITER");
        bmqp::PutMessageIterator iter(&bufferFactory,
                                      bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(false, iter.isValid());
    }

    {
        PVV("CREATE INVALID ITER FROM ANOTHER INVALID ITER");
        bmqp::PutMessageIterator iter1(&bufferFactory,
                                       bmqtst::TestHelperUtil::allocator());
        bmqp::PutMessageIterator iter2(iter1,
                                       bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(false, iter1.isValid());
        BMQTST_ASSERT_EQ(false, iter2.isValid());
    }

    {
        PVV("ASSIGNING INVALID ITER");
        bmqp::PutMessageIterator iter1(&bufferFactory,
                                       bmqtst::TestHelperUtil::allocator()),
            iter2(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(false, iter1.isValid());
        BMQTST_ASSERT_EQ(false, iter2.isValid());

        iter1 = iter2;
        BMQTST_ASSERT_EQ(false, iter1.isValid());
        BMQTST_ASSERT_EQ(false, iter2.isValid());
    }

    {
        PVV("CREATE VALID ITER DECOMPRESSFLAG FALSE, E_NONE COMPRESSION");
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob expectedBlob(&bufferFactory,
                                 bmqtst::TestHelperUtil::allocator());
        int                expectedBlobLength = 0;
        bmqu::BlobPosition expectedHeaderPos;
        bmqu::BlobPosition expectedPayloadPos;
        bmqu::BlobPosition retrievedPayloadPos;
        bdlbb::Blob retrievedPayloadBlob(bmqtst::TestHelperUtil::allocator());

        // Populate blob
        const int         queueId = 123;
        bmqp::EventHeader eventHeader;
        bmqt::MessageGUID guid = bmqp::MessageGUIDGenerator::testGUID();

        bmqp::PutTester::populateBlob(&blob,
                                      &eventHeader,
                                      &expectedBlob,
                                      &expectedBlobLength,
                                      &expectedHeaderPos,
                                      &expectedPayloadPos,
                                      queueId,
                                      guid);

        // Iterate and verify
        bmqp::PutMessageIterator iter(&blob,
                                      eventHeader,
                                      false,  // decompress flag
                                      &bufferFactory,
                                      bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(true, iter.next());
        BMQTST_ASSERT_EQ(queueId, iter.header().queueId());
        BMQTST_ASSERT_EQ(guid, iter.header().messageGUID());
        BMQTST_ASSERT_EQ(expectedBlobLength, iter.applicationDataSize());
        BMQTST_ASSERT_EQ(false, iter.hasMessageProperties());

        bmqu::BlobPosition emptyPos;
        BMQTST_ASSERT_EQ(false,
                         0 == iter.loadMessagePropertiesPosition(&emptyPos));
        BMQTST_ASSERT_EQ(bmqu::BlobPosition(), emptyPos);

        BMQTST_ASSERT_EQ(
            0,
            iter.loadApplicationDataPosition(&retrievedPayloadPos));
        BMQTST_ASSERT_EQ(retrievedPayloadPos, expectedPayloadPos);

        retrievedPayloadBlob.removeAll();
        BMQTST_ASSERT_EQ(0, iter.loadApplicationData(&retrievedPayloadBlob));
        BMQTST_ASSERT_EQ(0,
                         bdlbb::BlobUtil::compare(retrievedPayloadBlob,
                                                  expectedBlob));

        BMQTST_ASSERT_EQ(false, iter.hasMsgGroupId());
        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(false, iter.next());
        BMQTST_ASSERT_EQ(false, iter.isValid());

        // Copy
        bmqp::PutMessageIterator iter2(iter,
                                       bmqtst::TestHelperUtil::allocator());
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
        iter.reset(&blob, eventHeader, true);
        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(true, iter.next());
        BMQTST_ASSERT_EQ(queueId, iter.header().queueId());
        BMQTST_ASSERT_EQ(guid, iter.header().messageGUID());
        BMQTST_ASSERT_EQ(expectedBlobLength, iter.applicationDataSize());
        BMQTST_ASSERT_EQ(false, iter.hasMessageProperties());

        BMQTST_ASSERT_EQ(false,
                         0 == iter.loadMessagePropertiesPosition(&emptyPos));
        BMQTST_ASSERT_EQ(bmqu::BlobPosition(), emptyPos);

        bdlbb::Blob retrievedPayloadBlob2(bmqtst::TestHelperUtil::allocator());
        bmqu::BlobPosition retrievedPayloadPos2;

        BMQTST_ASSERT_EQ(
            0,
            iter.loadApplicationDataPosition(&retrievedPayloadPos2));
        BMQTST_ASSERT_EQ(retrievedPayloadPos2, expectedPayloadPos);

        retrievedPayloadBlob2.removeAll();
        BMQTST_ASSERT_EQ(0, iter.loadApplicationData(&retrievedPayloadBlob2));
        BMQTST_ASSERT_EQ(0,
                         bdlbb::BlobUtil::compare(retrievedPayloadBlob2,
                                                  expectedBlob));

        BMQTST_ASSERT_EQ(false, iter.hasMsgGroupId());
        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(false, iter.next());
        BMQTST_ASSERT_EQ(false, iter.isValid());
    }

    {
        PVV("CREATE VALID ITER DECOMPRESSFLAG FALSE, E_ZLIB COMPRESSION");
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob expectedBlob(&bufferFactory,
                                 bmqtst::TestHelperUtil::allocator());
        int                expectedBlobLength = 0;
        bmqu::BlobPosition expectedHeaderPos;
        bmqu::BlobPosition expectedPayloadPos;
        bmqu::BlobPosition retrievedPayloadPos;
        bdlbb::Blob retrievedPayloadBlob(bmqtst::TestHelperUtil::allocator());

        // Populate blob
        const int         queueId = 123;
        bmqp::EventHeader eventHeader;
        bmqt::MessageGUID guid = bmqp::MessageGUIDGenerator::testGUID();

        bmqp::PutTester::populateBlob(&blob,
                                      &eventHeader,
                                      &expectedBlob,
                                      &expectedBlobLength,
                                      &expectedHeaderPos,
                                      &expectedPayloadPos,
                                      queueId,
                                      guid,
                                      bmqt::CompressionAlgorithmType::e_ZLIB,
                                      &bufferFactory,
                                      bmqtst::TestHelperUtil::allocator());

        // Iterate and verify
        bmqp::PutMessageIterator iter(&blob,
                                      eventHeader,
                                      false,  // decompress flag
                                      &bufferFactory,
                                      bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(true, iter.next());
        BMQTST_ASSERT_EQ(queueId, iter.header().queueId());
        BMQTST_ASSERT_EQ(guid, iter.header().messageGUID());
        BMQTST_ASSERT_EQ(false, iter.hasMessageProperties());

        bmqu::BlobPosition emptyPos;
        BMQTST_ASSERT_EQ(false,
                         0 == iter.loadMessagePropertiesPosition(&emptyPos));
        BMQTST_ASSERT_EQ(bmqu::BlobPosition(), emptyPos);

        BMQTST_ASSERT_EQ(
            0,
            iter.loadApplicationDataPosition(&retrievedPayloadPos));
        BMQTST_ASSERT_EQ(retrievedPayloadPos, expectedPayloadPos);

        retrievedPayloadBlob.removeAll();
        BMQTST_ASSERT_EQ(0, iter.loadApplicationData(&retrievedPayloadBlob));
        BMQTST_ASSERT(retrievedPayloadBlob.length() > 0);

        BMQTST_ASSERT_EQ(false, iter.hasMsgGroupId());
        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(false, iter.next());
        BMQTST_ASSERT_EQ(false, iter.isValid());

        // Copy
        bmqp::PutMessageIterator iter2(iter,
                                       bmqtst::TestHelperUtil::allocator());
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
        iter.reset(&blob, eventHeader, true);
        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(true, iter.next());
        BMQTST_ASSERT_EQ(queueId, iter.header().queueId());
        BMQTST_ASSERT_EQ(guid, iter.header().messageGUID());
        BMQTST_ASSERT_EQ(expectedBlobLength, iter.applicationDataSize());
        BMQTST_ASSERT_EQ(false, iter.hasMessageProperties());

        BMQTST_ASSERT_EQ(false,
                         0 == iter.loadMessagePropertiesPosition(&emptyPos));
        BMQTST_ASSERT_EQ(bmqu::BlobPosition(), emptyPos);

        bdlbb::Blob retrievedPayloadBlob2(bmqtst::TestHelperUtil::allocator());
        bmqu::BlobPosition retrievedPayloadPos2;

        BMQTST_ASSERT_EQ(
            0,
            iter.loadApplicationDataPosition(&retrievedPayloadPos2));
        BMQTST_ASSERT_EQ(retrievedPayloadPos2, expectedPayloadPos);

        retrievedPayloadBlob2.removeAll();
        BMQTST_ASSERT_EQ(0, iter.loadApplicationData(&retrievedPayloadBlob2));
        BMQTST_ASSERT_EQ(0,
                         bdlbb::BlobUtil::compare(retrievedPayloadBlob2,
                                                  expectedBlob));

        BMQTST_ASSERT_EQ(false, iter.hasMsgGroupId());
        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(false, iter.next());
        BMQTST_ASSERT_EQ(false, iter.isValid());
    }

    {
        PVV("CREATE VALID ITER DECOMPRESSFLAG TRUE, E_NONE COMPRESSION");
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob expectedBlob(&bufferFactory,
                                 bmqtst::TestHelperUtil::allocator());
        int                expectedBlobLength = 0;
        bmqu::BlobPosition expectedHeaderPos;
        bmqu::BlobPosition expectedPayloadPos;
        bmqu::BlobPosition retrievedPayloadPos;
        bdlbb::Blob        retrievedPayloadBlob(&bufferFactory,
                                         bmqtst::TestHelperUtil::allocator());

        // Populate blob
        const int         queueId = 123;
        bmqp::EventHeader eventHeader;
        bmqt::MessageGUID guid = bmqp::MessageGUIDGenerator::testGUID();

        bmqp::PutTester::populateBlob(&blob,
                                      &eventHeader,
                                      &expectedBlob,
                                      &expectedBlobLength,
                                      &expectedHeaderPos,
                                      &expectedPayloadPos,
                                      queueId,
                                      guid);

        // Iterate and verify
        bmqp::PutMessageIterator iter(&blob,
                                      eventHeader,
                                      true,  // decompress flag
                                      &bufferFactory,
                                      bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(true, iter.next());
        BMQTST_ASSERT_EQ(queueId, iter.header().queueId());
        BMQTST_ASSERT_EQ(guid, iter.header().messageGUID());
        BMQTST_ASSERT_EQ(expectedBlobLength, iter.messagePayloadSize());
        BMQTST_ASSERT_EQ(expectedBlobLength, iter.applicationDataSize());
        BMQTST_ASSERT_EQ(false, iter.hasMessageProperties());
        BMQTST_ASSERT_EQ(0, iter.messagePropertiesSize());

        bdlbb::Blob emptyBlob(bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(0, iter.loadMessageProperties(&emptyBlob));
        BMQTST_ASSERT_EQ(0, emptyBlob.length());

        bmqu::BlobPosition emptyPos;
        BMQTST_ASSERT_EQ(false,
                         0 == iter.loadMessagePropertiesPosition(&emptyPos));
        BMQTST_ASSERT_EQ(bmqu::BlobPosition(), emptyPos);

        bmqp::MessageProperties emptyProps(
            bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(0, iter.loadMessageProperties(&emptyProps));
        BMQTST_ASSERT_EQ(0, emptyProps.numProperties());

        BMQTST_ASSERT_EQ(0, iter.loadMessagePayload(&retrievedPayloadBlob));
        BMQTST_ASSERT_EQ(0,
                         bdlbb::BlobUtil::compare(retrievedPayloadBlob,
                                                  expectedBlob));

        BMQTST_ASSERT_EQ(
            0,
            iter.loadApplicationDataPosition(&retrievedPayloadPos));
        BMQTST_ASSERT_EQ(retrievedPayloadPos, expectedPayloadPos);

        retrievedPayloadBlob.removeAll();
        BMQTST_ASSERT_EQ(0, iter.loadApplicationData(&retrievedPayloadBlob));
        BMQTST_ASSERT_EQ(0,
                         bdlbb::BlobUtil::compare(retrievedPayloadBlob,
                                                  expectedBlob));

        BMQTST_ASSERT_EQ(false, iter.hasMsgGroupId());
        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(false, iter.next());
        BMQTST_ASSERT_EQ(false, iter.isValid());

        // Copy
        bmqp::PutMessageIterator iter2(iter,
                                       bmqtst::TestHelperUtil::allocator());
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
        iter.reset(&blob, eventHeader, true);
        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(true, iter.next());
        BMQTST_ASSERT_EQ(queueId, iter.header().queueId());
        BMQTST_ASSERT_EQ(guid, iter.header().messageGUID());
        BMQTST_ASSERT_EQ(expectedBlobLength, iter.messagePayloadSize());
        BMQTST_ASSERT_EQ(expectedBlobLength, iter.applicationDataSize());
        BMQTST_ASSERT_EQ(false, iter.hasMessageProperties());
        BMQTST_ASSERT_EQ(0, iter.messagePropertiesSize());
        BMQTST_ASSERT_EQ(0, iter.loadMessageProperties(&emptyBlob));
        BMQTST_ASSERT_EQ(0, emptyBlob.length());
        BMQTST_ASSERT_EQ(false,
                         0 == iter.loadMessagePropertiesPosition(&emptyPos));
        BMQTST_ASSERT_EQ(bmqu::BlobPosition(), emptyPos);
        BMQTST_ASSERT_EQ(0, iter.loadMessageProperties(&emptyProps));
        BMQTST_ASSERT_EQ(0, emptyProps.numProperties());

        bdlbb::Blob        retrievedPayloadBlob2(&bufferFactory,
                                          bmqtst::TestHelperUtil::allocator());
        bmqu::BlobPosition retrievedPayloadPos2;

        BMQTST_ASSERT_EQ(0, iter.loadMessagePayload(&retrievedPayloadBlob2));
        BMQTST_ASSERT_EQ(0,
                         bdlbb::BlobUtil::compare(retrievedPayloadBlob2,
                                                  expectedBlob));

        BMQTST_ASSERT_EQ(
            0,
            iter.loadApplicationDataPosition(&retrievedPayloadPos2));
        BMQTST_ASSERT_EQ(retrievedPayloadPos2, expectedPayloadPos);

        retrievedPayloadBlob2.removeAll();
        BMQTST_ASSERT_EQ(0, iter.loadApplicationData(&retrievedPayloadBlob2));
        BMQTST_ASSERT_EQ(0,
                         bdlbb::BlobUtil::compare(retrievedPayloadBlob2,
                                                  expectedBlob));

        BMQTST_ASSERT_EQ(false, iter.hasMsgGroupId());
        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(false, iter.next());
        BMQTST_ASSERT_EQ(false, iter.isValid());
    }

    {
        PVV("CREATE VALID ITER DECOMPRESSFLAG TRUE, E_ZLIB COMPRESSION");
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob expectedBlob(&bufferFactory,
                                 bmqtst::TestHelperUtil::allocator());
        int                expectedBlobLength = 0;
        bmqu::BlobPosition expectedHeaderPos;
        bmqu::BlobPosition expectedPayloadPos;
        bmqu::BlobPosition retrievedPayloadPos;
        bdlbb::Blob        retrievedPayloadBlob(&bufferFactory,
                                         bmqtst::TestHelperUtil::allocator());

        // Populate blob
        const int         queueId = 123;
        bmqp::EventHeader eventHeader;
        bmqt::MessageGUID guid = bmqp::MessageGUIDGenerator::testGUID();

        bmqp::PutTester::populateBlob(&blob,
                                      &eventHeader,
                                      &expectedBlob,
                                      &expectedBlobLength,
                                      &expectedHeaderPos,
                                      &expectedPayloadPos,
                                      queueId,
                                      guid,
                                      bmqt::CompressionAlgorithmType::e_ZLIB,
                                      &bufferFactory,
                                      bmqtst::TestHelperUtil::allocator());

        // Iterate and verify
        bmqp::PutMessageIterator iter(&blob,
                                      eventHeader,
                                      true,  // decompress flag
                                      &bufferFactory,
                                      bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(true, iter.next());
        BMQTST_ASSERT_EQ(queueId, iter.header().queueId());
        BMQTST_ASSERT_EQ(guid, iter.header().messageGUID());
        BMQTST_ASSERT_EQ(expectedBlobLength, iter.messagePayloadSize());
        BMQTST_ASSERT_EQ(expectedBlobLength, iter.applicationDataSize());
        BMQTST_ASSERT_EQ(false, iter.hasMessageProperties());
        BMQTST_ASSERT_EQ(0, iter.messagePropertiesSize());

        bdlbb::Blob emptyBlob(bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(0, iter.loadMessageProperties(&emptyBlob));
        BMQTST_ASSERT_EQ(0, emptyBlob.length());

        bmqu::BlobPosition emptyPos;
        BMQTST_ASSERT_EQ(false,
                         0 == iter.loadMessagePropertiesPosition(&emptyPos));
        BMQTST_ASSERT_EQ(bmqu::BlobPosition(), emptyPos);

        bmqp::MessageProperties emptyProps(
            bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(0, iter.loadMessageProperties(&emptyProps));
        BMQTST_ASSERT_EQ(0, emptyProps.numProperties());

        BMQTST_ASSERT_EQ(0, iter.loadMessagePayload(&retrievedPayloadBlob));
        BMQTST_ASSERT_EQ(0,
                         bdlbb::BlobUtil::compare(retrievedPayloadBlob,
                                                  expectedBlob));

        BMQTST_ASSERT_EQ(
            0,
            iter.loadApplicationDataPosition(&retrievedPayloadPos));
        BMQTST_ASSERT_EQ(retrievedPayloadPos, expectedPayloadPos);

        retrievedPayloadBlob.removeAll();
        BMQTST_ASSERT_EQ(0, iter.loadApplicationData(&retrievedPayloadBlob));
        BMQTST_ASSERT_EQ(0,
                         bdlbb::BlobUtil::compare(retrievedPayloadBlob,
                                                  expectedBlob));

        BMQTST_ASSERT_EQ(false, iter.hasMsgGroupId());
        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(false, iter.next());
        BMQTST_ASSERT_EQ(false, iter.isValid());

        // Copy
        bmqp::PutMessageIterator iter2(iter,
                                       bmqtst::TestHelperUtil::allocator());
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
        iter.reset(&blob, eventHeader, true);
        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(true, iter.next());
        BMQTST_ASSERT_EQ(queueId, iter.header().queueId());
        BMQTST_ASSERT_EQ(guid, iter.header().messageGUID());
        BMQTST_ASSERT_EQ(expectedBlobLength, iter.messagePayloadSize());
        BMQTST_ASSERT_EQ(expectedBlobLength, iter.applicationDataSize());
        BMQTST_ASSERT_EQ(false, iter.hasMessageProperties());
        BMQTST_ASSERT_EQ(0, iter.messagePropertiesSize());
        BMQTST_ASSERT_EQ(0, iter.loadMessageProperties(&emptyBlob));
        BMQTST_ASSERT_EQ(0, emptyBlob.length());
        BMQTST_ASSERT_EQ(false,
                         0 == iter.loadMessagePropertiesPosition(&emptyPos));
        BMQTST_ASSERT_EQ(bmqu::BlobPosition(), emptyPos);
        BMQTST_ASSERT_EQ(0, iter.loadMessageProperties(&emptyProps));
        BMQTST_ASSERT_EQ(0, emptyProps.numProperties());

        bdlbb::Blob        retrievedPayloadBlob2(&bufferFactory,
                                          bmqtst::TestHelperUtil::allocator());
        bmqu::BlobPosition retrievedPayloadPos2;

        BMQTST_ASSERT_EQ(0, iter.loadMessagePayload(&retrievedPayloadBlob2));
        BMQTST_ASSERT_EQ(0,
                         bdlbb::BlobUtil::compare(retrievedPayloadBlob2,
                                                  expectedBlob));

        BMQTST_ASSERT_EQ(
            0,
            iter.loadApplicationDataPosition(&retrievedPayloadPos2));
        BMQTST_ASSERT_EQ(retrievedPayloadPos2, expectedPayloadPos);

        retrievedPayloadBlob2.removeAll();
        BMQTST_ASSERT_EQ(0, iter.loadApplicationData(&retrievedPayloadBlob2));
        BMQTST_ASSERT_EQ(0,
                         bdlbb::BlobUtil::compare(retrievedPayloadBlob2,
                                                  expectedBlob));

        BMQTST_ASSERT_EQ(false, iter.hasMsgGroupId());
        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(false, iter.next());
        BMQTST_ASSERT_EQ(false, iter.isValid());
    }
}

static void test2_reset()
// ------------------------------------------------------------------------
// RESET
//
// Concerns:
//   Ensure correct behavior of the reset method.
//
// Testing:
//   reset
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("RESET");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());

    {
        PVV("NO COMPRESSION, DECOMPRESS FLAG FALSE");
        bmqp::PutMessageIterator pmt(&bufferFactory,
                                     bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob copiedBlob(bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob expectedBlob(&bufferFactory,
                                 bmqtst::TestHelperUtil::allocator());
        int                      expectedBlobLength = 0;
        bmqu::BlobPosition       headerPosition;
        bmqu::BlobPosition       payloadPosition;
        const int                queueId = 123;
        bmqp::EventHeader        eventHeader;
        bdlbb::Blob payloadBlob(bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob appDataBlob(bmqtst::TestHelperUtil::allocator());
        bmqt::MessageGUID        guid = bmqp::MessageGUIDGenerator::testGUID();

        {
            bdlbb::Blob blob(&bufferFactory,
                             bmqtst::TestHelperUtil::allocator());

            // Populate blob
            bmqp::PutTester::populateBlob(&blob,
                                          &eventHeader,
                                          &expectedBlob,
                                          &expectedBlobLength,
                                          &headerPosition,
                                          &payloadPosition,
                                          queueId,
                                          guid);

            bmqp::PutMessageIterator iter(&blob,
                                          eventHeader,
                                          false,
                                          &bufferFactory,
                                          bmqtst::TestHelperUtil::allocator());

            BMQTST_ASSERT_EQ(true, iter.isValid());

            // Copy 'blob' into 'copiedBlob'. Reset 'pmt' with 'copiedBlob'
            // and 'iter'

            copiedBlob = blob;
            pmt.reset(&copiedBlob, iter);
        }

        // Iterate and verify
        BMQTST_ASSERT_EQ(true, pmt.isValid());
        BMQTST_ASSERT_EQ(true, pmt.next());
        BMQTST_ASSERT_EQ(queueId, pmt.header().queueId());
        BMQTST_ASSERT_EQ(guid, pmt.header().messageGUID());
        BMQTST_ASSERT_EQ(expectedBlobLength, pmt.applicationDataSize());

        BMQTST_ASSERT_EQ(0, pmt.loadApplicationData(&appDataBlob));
        BMQTST_ASSERT_EQ(0,
                         bdlbb::BlobUtil::compare(appDataBlob, expectedBlob));

        BMQTST_ASSERT_EQ(false, pmt.hasMsgGroupId());
        BMQTST_ASSERT_EQ(true, pmt.isValid());
        BMQTST_ASSERT_EQ(false, pmt.next());
        BMQTST_ASSERT_EQ(false, pmt.isValid());
    }

    {
        PVV("ZLIB COMPRESSION, DECOMPRESS FLAG TRUE");
        bmqp::PutMessageIterator pmt(&bufferFactory,
                                     bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob copiedBlob(bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob expectedBlob(&bufferFactory,
                                 bmqtst::TestHelperUtil::allocator());
        int                      expectedBlobLength = 0;
        bmqu::BlobPosition       headerPosition;
        bmqu::BlobPosition       payloadPosition;
        const int                queueId = 123;
        bmqp::EventHeader        eventHeader;
        bdlbb::Blob payloadBlob(bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob appDataBlob(bmqtst::TestHelperUtil::allocator());
        bmqt::MessageGUID        guid = bmqp::MessageGUIDGenerator::testGUID();

        {
            bdlbb::Blob blob(&bufferFactory,
                             bmqtst::TestHelperUtil::allocator());

            // Populate blob
            bmqp::PutTester::populateBlob(
                &blob,
                &eventHeader,
                &expectedBlob,
                &expectedBlobLength,
                &headerPosition,
                &payloadPosition,
                queueId,
                guid,
                bmqt::CompressionAlgorithmType::e_ZLIB,
                &bufferFactory,
                bmqtst::TestHelperUtil::allocator());

            bmqp::PutMessageIterator iter(&blob,
                                          eventHeader,
                                          true,
                                          &bufferFactory,
                                          bmqtst::TestHelperUtil::allocator());

            BMQTST_ASSERT_EQ(true, iter.isValid());

            // Copy 'blob' into 'copiedBlob'. Reset 'pmt' with 'copiedBlob'
            // and 'iter'

            copiedBlob = blob;
            pmt.reset(&copiedBlob, iter);
        }

        // Iterate and verify
        BMQTST_ASSERT_EQ(true, pmt.isValid());
        BMQTST_ASSERT_EQ(true, pmt.next());
        BMQTST_ASSERT_EQ(queueId, pmt.header().queueId());
        BMQTST_ASSERT_EQ(guid, pmt.header().messageGUID());
        BMQTST_ASSERT_EQ(expectedBlobLength, pmt.messagePayloadSize());
        BMQTST_ASSERT_EQ(expectedBlobLength, pmt.applicationDataSize());

        BMQTST_ASSERT_EQ(0, pmt.loadMessagePayload(&payloadBlob));
        BMQTST_ASSERT_EQ(0,
                         bdlbb::BlobUtil::compare(payloadBlob, expectedBlob));

        BMQTST_ASSERT_EQ(0, pmt.loadApplicationData(&appDataBlob));
        BMQTST_ASSERT_EQ(0,
                         bdlbb::BlobUtil::compare(appDataBlob, expectedBlob));

        BMQTST_ASSERT_EQ(false, pmt.hasMsgGroupId());
        BMQTST_ASSERT_EQ(true, pmt.isValid());
        BMQTST_ASSERT_EQ(false, pmt.next());
        BMQTST_ASSERT_EQ(false, pmt.isValid());
    }
}

static void test3_putEventWithNoMessages()
// ------------------------------------------------------------------------
// PUT EVENT WITH NO MESSAGES
//
// Concerns:
//   Iterating over PUT event having *NO* PUT messages
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PUT EVENT WITH NO MESSAGES");

    // Test
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob eventBlob(&bufferFactory, bmqtst::TestHelperUtil::allocator());

    bsl::vector<bmqp::PutTester::Data> data(
        bmqtst::TestHelperUtil::allocator());
    bmqp::EventHeader                  eventHeader;

    bmqp::PutTester::populateBlob(&eventBlob,
                                  &eventHeader,
                                  &data,
                                  0,
                                  &bufferFactory,
                                  false,  // No zero-length msgs
                                  bmqtst::TestHelperUtil::allocator());

    // Verify non-validity
    bmqp::PutMessageIterator iter(&bufferFactory,
                                  bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_LT(iter.reset(&eventBlob, eventHeader, true), 0);
    BMQTST_ASSERT_EQ(iter.isValid(), false);
}

static void test4_invalidPutEvent()
// ------------------------------------------------------------------------
// INVALID PUT EVENT
//
// Concerns:
//   - iterating over invalid PUT event (having a PUT message, but not
//     enough bytes in the blob)
//   - attempting to load options from blob having not enough bytes
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("INVALID PUT EVENT");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    {
        PVV("NEXT METHOD OVER INVALID BLOB");
        bdlbb::Blob                        eventBlob(&bufferFactory,
                              bmqtst::TestHelperUtil::allocator());
        bsl::vector<bmqp::PutTester::Data> data(
            bmqtst::TestHelperUtil::allocator());
        bmqp::EventHeader                  eventHeader;

        bmqp::PutTester::populateBlob(&eventBlob,
                                      &eventHeader,
                                      &data,
                                      2,
                                      &bufferFactory,
                                      false,  // No zero-length msgs
                                      bmqtst::TestHelperUtil::allocator());

        // Render the blob invalid by removing it's last byte
        bdlbb::BlobUtil::erase(&eventBlob, eventBlob.length() - 1, 1);

        // Verify
        bmqp::PutMessageIterator iter(&eventBlob,
                                      eventHeader,
                                      false,
                                      &bufferFactory,
                                      bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(true, iter.isValid());  // First message is valid..
        BMQTST_ASSERT_EQ(1, iter.next());        // Second message is error.

        int rc = iter.next();
        BMQTST_ASSERT_LT(rc, 0);

        PVV("Error returned: " << rc);
        iter.dumpBlob(bsl::cout);
    }

    {
        PVV("LOAD OPTIONS METHOD OVER INVALID BLOB");
        bdlbb::Blob                        eventBlob(&bufferFactory,
                              bmqtst::TestHelperUtil::allocator());
        bsl::vector<bmqp::PutTester::Data> data(
            bmqtst::TestHelperUtil::allocator());
        bmqp::EventHeader                  eventHeader;

        bmqp::PutTester::populateBlob(&eventBlob,
                                      &eventHeader,
                                      &data,
                                      3,
                                      &bufferFactory,
                                      true,  // Include zero-length msgs
                                      bmqtst::TestHelperUtil::allocator());

        // Verify
        bmqp::PutMessageIterator iter(&eventBlob,
                                      eventHeader,
                                      false,
                                      &bufferFactory,
                                      bmqtst::TestHelperUtil::allocator());

        // First 3 messages are valid..
        BMQTST_ASSERT_EQ(true, iter.isValid());
        BMQTST_ASSERT_EQ(1, iter.next());
        BMQTST_ASSERT_EQ(1, iter.next());

        bdlbb::Blob options;
        eventBlob.setLength(1);
        int rc = iter.loadOptions(&options);
        BMQTST_ASSERT_LT(rc, 0);

        PVV("Error returned: " << rc);
        iter.dumpBlob(bsl::cout);
    }
}

static void test5_putEventWithMultipleMessages()
// ------------------------------------------------------------------------
// PUT EVENT WITH MULTIPLE MESSAGES
//
// Concerns:
//   Iterating over PUT event having multiple PUT messages.
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PUT EVENT WITH MULTIPLE MESSAGES");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());

    {
        PVV("WITH ZLIB COMPRESSION, DECOMPRESS FLAG TRUE");
        bdlbb::Blob                        eventBlob(&bufferFactory,
                              bmqtst::TestHelperUtil::allocator());
        bsl::vector<bmqp::PutTester::Data> data(
            bmqtst::TestHelperUtil::allocator());
        bmqp::EventHeader                  eventHeader;
        const size_t                       k_NUM_MSGS = 1000;

        bmqp::PutTester::populateBlob(&eventBlob,
                                      &eventHeader,
                                      &data,
                                      k_NUM_MSGS,
                                      &bufferFactory,
                                      false,  // No zero-length PUT msgs.
                                      bmqtst::TestHelperUtil::allocator(),
                                      bmqt::CompressionAlgorithmType::e_ZLIB);

        // Iterate and verify
        bmqp::PutMessageIterator iter(&eventBlob,
                                      eventHeader,
                                      true,
                                      &bufferFactory,
                                      bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(true, iter.isValid());

        size_t index = 0;

        while (iter.next() == 1 && index < data.size()) {
            const bmqp::PutTester::Data& D = data[index];

            BMQTST_ASSERT_EQ_D(index, D.d_queueId, iter.header().queueId());

            const bool hasMsgProps = bmqp::PutHeaderFlagUtil::isSet(
                iter.header().flags(),
                bmqp::PutHeaderFlags::e_MESSAGE_PROPERTIES);
            BMQTST_ASSERT_EQ_D(index, true, hasMsgProps);
            BMQTST_ASSERT_EQ_D(index,
                               hasMsgProps,
                               iter.hasMessageProperties());
            BMQTST_ASSERT_EQ_D(index,
                               D.d_propLen,
                               iter.messagePropertiesSize());
            BMQTST_ASSERT_EQ_D(index, D.d_msgLen, iter.messagePayloadSize());
            BMQTST_ASSERT_EQ_D(index,
                               (D.d_msgLen + D.d_propLen),
                               iter.applicationDataSize());

            bdlbb::Blob        options(bmqtst::TestHelperUtil::allocator());
            bdlbb::Blob        props(bmqtst::TestHelperUtil::allocator());
            bdlbb::Blob        payload(bmqtst::TestHelperUtil::allocator());
            bdlbb::Blob        appData(bmqtst::TestHelperUtil::allocator());
            bmqu::BlobPosition propsPos;
            bmqu::BlobPosition payloadPos;
            bmqu::BlobPosition appDataPos;

            BMQTST_ASSERT_EQ_D(index,
                               0,
                               iter.loadMessagePropertiesPosition(&propsPos));

            BMQTST_ASSERT_EQ_D(index,
                               0,
                               iter.loadApplicationDataPosition(&appDataPos));

            BMQTST_ASSERT_EQ_D(index, 0, iter.loadMessageProperties(&props));
            BMQTST_ASSERT_EQ_D(index,
                               0,
                               bdlbb::BlobUtil::compare(props,
                                                        D.d_properties));

            BMQTST_ASSERT_EQ_D(index, 0, iter.loadMessagePayload(&payload));
            BMQTST_ASSERT_EQ_D(index,
                               0,
                               bdlbb::BlobUtil::compare(payload, D.d_payload));

            BMQTST_ASSERT_EQ_D(index, 0, iter.loadApplicationData(&appData));
            BMQTST_ASSERT_EQ_D(index,
                               0,
                               bdlbb::BlobUtil::compare(appData, D.d_appData));

            const bool hasMsgGroupId = iter.hasMsgGroupId();
            BMQTST_ASSERT_EQ_D(index, hasMsgGroupId, !D.d_msgGroupId.isNull());
            if (hasMsgGroupId) {
                bmqp::Protocol::MsgGroupId msgGroupId(
                    bmqtst::TestHelperUtil::allocator());
                BMQTST_ASSERT_EQ_D(index,
                                   true,
                                   iter.extractMsgGroupId(&msgGroupId));

                BMQTST_ASSERT_EQ_D(index,
                                   msgGroupId,
                                   D.d_msgGroupId.valueOr(""));
            }
            typedef bmqp::OptionUtil::OptionMeta OptionMeta;
            const OptionMeta                     msgGroupIdOption =
                hasMsgGroupId ? OptionMeta::forOptionWithPadding(
                                    bmqp::OptionType::e_MSG_GROUP_ID,
                                    D.d_msgGroupId.value().length())
                                                  : OptionMeta::forNullOption();

            const int optionsSize = hasMsgGroupId ? msgGroupIdOption.size()
                                                  : 0;

            BMQTST_ASSERT_EQ_D(index, 0, iter.loadOptions(&options));

            BMQTST_ASSERT_EQ_D(index, optionsSize, options.length());

            ++index;
        }

        BMQTST_ASSERT_EQ(index, data.size());
        BMQTST_ASSERT_EQ(false, iter.isValid());
    }

    {
        PVV("WITH NO COMPRESSION, DECOMPRESS FLAG TRUE");
        bdlbb::Blob                        eventBlob(&bufferFactory,
                              bmqtst::TestHelperUtil::allocator());
        bsl::vector<bmqp::PutTester::Data> data(
            bmqtst::TestHelperUtil::allocator());
        bmqp::EventHeader                  eventHeader;
        const size_t                       k_NUM_MSGS = 1000;

        bmqp::PutTester::populateBlob(&eventBlob,
                                      &eventHeader,
                                      &data,
                                      k_NUM_MSGS,
                                      &bufferFactory,
                                      false,  // No zero-length PUT msgs.
                                      bmqtst::TestHelperUtil::allocator());

        // Iterate and verify
        bmqp::PutMessageIterator iter(&eventBlob,
                                      eventHeader,
                                      true,
                                      &bufferFactory,
                                      bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(true, iter.isValid());

        size_t index = 0;

        while (iter.next() == 1 && index < data.size()) {
            const bmqp::PutTester::Data& D = data[index];

            BMQTST_ASSERT_EQ_D(index, D.d_queueId, iter.header().queueId());

            const bool hasMsgProps = bmqp::PutHeaderFlagUtil::isSet(
                iter.header().flags(),
                bmqp::PutHeaderFlags::e_MESSAGE_PROPERTIES);
            BMQTST_ASSERT_EQ_D(index, true, hasMsgProps);
            BMQTST_ASSERT_EQ_D(index,
                               hasMsgProps,
                               iter.hasMessageProperties());
            BMQTST_ASSERT_EQ_D(index,
                               D.d_propLen,
                               iter.messagePropertiesSize());
            BMQTST_ASSERT_EQ_D(index, D.d_msgLen, iter.messagePayloadSize());
            BMQTST_ASSERT_EQ_D(index,
                               (D.d_msgLen + D.d_propLen),
                               iter.applicationDataSize());

            bdlbb::Blob        options(bmqtst::TestHelperUtil::allocator());
            bdlbb::Blob        props(bmqtst::TestHelperUtil::allocator());
            bdlbb::Blob        payload(bmqtst::TestHelperUtil::allocator());
            bdlbb::Blob        appData(bmqtst::TestHelperUtil::allocator());
            bmqu::BlobPosition propsPos;
            bmqu::BlobPosition payloadPos;
            bmqu::BlobPosition appDataPos;

            BMQTST_ASSERT_EQ_D(index,
                               0,
                               iter.loadMessagePropertiesPosition(&propsPos));

            BMQTST_ASSERT_EQ_D(index,
                               0,
                               iter.loadApplicationDataPosition(&appDataPos));

            BMQTST_ASSERT_EQ_D(index, 0, iter.loadMessageProperties(&props));
            BMQTST_ASSERT_EQ_D(index,
                               0,
                               bdlbb::BlobUtil::compare(props,
                                                        D.d_properties));

            BMQTST_ASSERT_EQ_D(index, 0, iter.loadMessagePayload(&payload));
            BMQTST_ASSERT_EQ_D(index,
                               0,
                               bdlbb::BlobUtil::compare(payload, D.d_payload));

            BMQTST_ASSERT_EQ_D(index, 0, iter.loadApplicationData(&appData));
            BMQTST_ASSERT_EQ_D(index,
                               0,
                               bdlbb::BlobUtil::compare(appData, D.d_appData));

            const bool hasMsgGroupId = iter.hasMsgGroupId();
            BMQTST_ASSERT_EQ_D(index, hasMsgGroupId, !D.d_msgGroupId.isNull());
            if (hasMsgGroupId) {
                bmqp::Protocol::MsgGroupId msgGroupId(
                    bmqtst::TestHelperUtil::allocator());
                BMQTST_ASSERT_EQ_D(index,
                                   true,
                                   iter.extractMsgGroupId(&msgGroupId));

                BMQTST_ASSERT_EQ_D(index,
                                   msgGroupId,
                                   D.d_msgGroupId.valueOr(""));
            }
            typedef bmqp::OptionUtil::OptionMeta OptionMeta;
            const OptionMeta                     msgGroupIdOption =
                hasMsgGroupId ? OptionMeta::forOptionWithPadding(
                                    bmqp::OptionType::e_MSG_GROUP_ID,
                                    D.d_msgGroupId.value().length())
                                                  : OptionMeta::forNullOption();

            const int optionsSize = hasMsgGroupId ? msgGroupIdOption.size()
                                                  : 0;

            BMQTST_ASSERT_EQ_D(index, 0, iter.loadOptions(&options));

            BMQTST_ASSERT_EQ_D(index, optionsSize, options.length());

            ++index;
        }

        BMQTST_ASSERT_EQ(index, data.size());
        BMQTST_ASSERT_EQ(false, iter.isValid());
    }
}

static void test6_putEventWithZeroLengthPutMessages()
// ------------------------------------------------------------------------
// PUT EVENT WITH ZERO-LENGTH MESSAGES
//
// Concerns:
//   Iterating over PUT event containing one or more zero-length PUT
//   messages.
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PUT EVENT WITH ZERO-LENGTH MESSAGES");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());

    {
        PVV("WITH NO COMPRESSION, DECOMPRESS FLAG TRUE");
        bdlbb::Blob                        eventBlob(&bufferFactory,
                              bmqtst::TestHelperUtil::allocator());
        bsl::vector<bmqp::PutTester::Data> data(
            bmqtst::TestHelperUtil::allocator());
        bmqp::EventHeader                  eventHeader;
        const size_t                       k_NUM_MSGS = 1000;

        bmqp::PutTester::populateBlob(&eventBlob,
                                      &eventHeader,
                                      &data,
                                      k_NUM_MSGS,
                                      &bufferFactory,
                                      true,  // make some PUT msgs zero-length
                                      bmqtst::TestHelperUtil::allocator());

        // Iterate and verify
        bmqp::PutMessageIterator iter(&eventBlob,
                                      eventHeader,
                                      true,
                                      &bufferFactory,
                                      bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(true, iter.isValid());

        size_t index = 0;

        while (iter.next() == 1 && index < data.size()) {
            const bmqp::PutTester::Data& D = data[index];
            BMQTST_ASSERT_EQ_D(index, D.d_queueId, iter.header().queueId());

            const bool hasMsgProps = bmqp::PutHeaderFlagUtil::isSet(
                iter.header().flags(),
                bmqp::PutHeaderFlags::e_MESSAGE_PROPERTIES);
            BMQTST_ASSERT_EQ_D(index, true, hasMsgProps);
            BMQTST_ASSERT_EQ_D(index,
                               hasMsgProps,
                               iter.hasMessageProperties());
            BMQTST_ASSERT_EQ_D(index,
                               D.d_propLen,
                               iter.messagePropertiesSize());
            BMQTST_ASSERT_EQ_D(index, D.d_msgLen, iter.messagePayloadSize());
            BMQTST_ASSERT_EQ_D(index,
                               (D.d_msgLen + D.d_propLen),
                               iter.applicationDataSize());

            bdlbb::Blob        props(bmqtst::TestHelperUtil::allocator());
            bdlbb::Blob        payload(bmqtst::TestHelperUtil::allocator());
            bdlbb::Blob        appData(bmqtst::TestHelperUtil::allocator());
            bmqu::BlobPosition propsPos;
            bmqu::BlobPosition payloadPos;
            bmqu::BlobPosition appDataPos;

            BMQTST_ASSERT_EQ_D(index,
                               0,
                               iter.loadMessagePropertiesPosition(&propsPos));

            BMQTST_ASSERT_EQ_D(index,
                               0,
                               iter.loadApplicationDataPosition(&appDataPos));

            BMQTST_ASSERT_EQ_D(index, 0, iter.loadMessageProperties(&props));
            BMQTST_ASSERT_EQ_D(index,
                               0,
                               bdlbb::BlobUtil::compare(props,
                                                        D.d_properties));

            BMQTST_ASSERT_EQ_D(index, 0, iter.loadMessagePayload(&payload));
            BMQTST_ASSERT_EQ_D(index,
                               0,
                               bdlbb::BlobUtil::compare(payload, D.d_payload));

            BMQTST_ASSERT_EQ_D(index, 0, iter.loadApplicationData(&appData));
            BMQTST_ASSERT_EQ_D(index,
                               0,
                               bdlbb::BlobUtil::compare(appData, D.d_appData));

            const bool hasMsgGroupId = iter.hasMsgGroupId();
            BMQTST_ASSERT_EQ_D(index, hasMsgGroupId, !D.d_msgGroupId.isNull());
            if (hasMsgGroupId) {
                bmqp::Protocol::MsgGroupId msgGroupId(
                    bmqtst::TestHelperUtil::allocator());
                BMQTST_ASSERT_EQ_D(index,
                                   true,
                                   iter.extractMsgGroupId(&msgGroupId));
                BMQTST_ASSERT_EQ_D(index,
                                   msgGroupId,
                                   D.d_msgGroupId.valueOr(""));
            }

            ++index;
        }

        BMQTST_ASSERT_EQ(index, data.size());
        BMQTST_ASSERT_EQ(false, iter.isValid());
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    switch (_testCase) {
    case 0:
    case 6: test6_putEventWithZeroLengthPutMessages(); break;
    case 5: test5_putEventWithMultipleMessages(); break;
    case 4: test4_invalidPutEvent(); break;
    case 3: test3_putEventWithNoMessages(); break;
    case 2: test2_reset(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
