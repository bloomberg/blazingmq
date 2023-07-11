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
#include <mwctst_testhelper.h>

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
//     the decompress flag is set to true. The iterator has messages which
//     have been compressed using ZLIB compression algorithm.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);

    {
        PVV("CREATE INVALID ITER");
        bmqp::PutMessageIterator iter(&bufferFactory, s_allocator_p);
        ASSERT_EQ(false, iter.isValid());
    }

    {
        PVV("CREATE INVALID ITER FROM ANOTHER INVALID ITER");
        bmqp::PutMessageIterator iter1(&bufferFactory, s_allocator_p);
        bmqp::PutMessageIterator iter2(iter1, s_allocator_p);

        ASSERT_EQ(false, iter1.isValid());
        ASSERT_EQ(false, iter2.isValid());
    }

    {
        PVV("ASSIGNING INVALID ITER");
        bmqp::PutMessageIterator iter1(&bufferFactory, s_allocator_p),
            iter2(&bufferFactory, s_allocator_p);
        ASSERT_EQ(false, iter1.isValid());
        ASSERT_EQ(false, iter2.isValid());

        iter1 = iter2;
        ASSERT_EQ(false, iter1.isValid());
        ASSERT_EQ(false, iter2.isValid());
    }

    {
        PVV("CREATE VALID ITER DECOMPRESSFLAG FALSE, E_NONE COMPRESSION");
        bdlbb::Blob        blob(&bufferFactory, s_allocator_p);
        bdlbb::Blob        expectedBlob(&bufferFactory, s_allocator_p);
        int                expectedBlobLength = 0;
        mwcu::BlobPosition expectedHeaderPos;
        mwcu::BlobPosition expectedPayloadPos;
        mwcu::BlobPosition retrievedPayloadPos;
        bdlbb::Blob        retrievedPayloadBlob(s_allocator_p);

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
                                      s_allocator_p);

        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(true, iter.next());
        ASSERT_EQ(queueId, iter.header().queueId());
        ASSERT_EQ(guid, iter.header().messageGUID());
        ASSERT_EQ(expectedBlobLength, iter.applicationDataSize());
        ASSERT_EQ(false, iter.hasMessageProperties());

        mwcu::BlobPosition emptyPos;
        ASSERT_EQ(false, 0 == iter.loadMessagePropertiesPosition(&emptyPos));
        ASSERT_EQ(mwcu::BlobPosition(), emptyPos);

        ASSERT_EQ(0, iter.loadApplicationDataPosition(&retrievedPayloadPos));
        ASSERT_EQ(retrievedPayloadPos, expectedPayloadPos);

        retrievedPayloadBlob.removeAll();
        ASSERT_EQ(0, iter.loadApplicationData(&retrievedPayloadBlob));
        ASSERT_EQ(0,
                  bdlbb::BlobUtil::compare(retrievedPayloadBlob,
                                           expectedBlob));

        ASSERT_EQ(false, iter.hasMsgGroupId());
        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(false, iter.next());
        ASSERT_EQ(false, iter.isValid());

        // Copy
        bmqp::PutMessageIterator iter2(iter, s_allocator_p);
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
        iter.reset(&blob, eventHeader, true);
        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(true, iter.next());
        ASSERT_EQ(queueId, iter.header().queueId());
        ASSERT_EQ(guid, iter.header().messageGUID());
        ASSERT_EQ(expectedBlobLength, iter.applicationDataSize());
        ASSERT_EQ(false, iter.hasMessageProperties());

        ASSERT_EQ(false, 0 == iter.loadMessagePropertiesPosition(&emptyPos));
        ASSERT_EQ(mwcu::BlobPosition(), emptyPos);

        bdlbb::Blob        retrievedPayloadBlob2(s_allocator_p);
        mwcu::BlobPosition retrievedPayloadPos2;

        ASSERT_EQ(0, iter.loadApplicationDataPosition(&retrievedPayloadPos2));
        ASSERT_EQ(retrievedPayloadPos2, expectedPayloadPos);

        retrievedPayloadBlob2.removeAll();
        ASSERT_EQ(0, iter.loadApplicationData(&retrievedPayloadBlob2));
        ASSERT_EQ(0,
                  bdlbb::BlobUtil::compare(retrievedPayloadBlob2,
                                           expectedBlob));

        ASSERT_EQ(false, iter.hasMsgGroupId());
        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(false, iter.next());
        ASSERT_EQ(false, iter.isValid());
    }

    {
        PVV("CREATE VALID ITER DECOMPRESSFLAG FALSE, E_ZLIB COMPRESSION");
        bdlbb::Blob        blob(&bufferFactory, s_allocator_p);
        bdlbb::Blob        expectedBlob(&bufferFactory, s_allocator_p);
        int                expectedBlobLength = 0;
        mwcu::BlobPosition expectedHeaderPos;
        mwcu::BlobPosition expectedPayloadPos;
        mwcu::BlobPosition retrievedPayloadPos;
        bdlbb::Blob        retrievedPayloadBlob(s_allocator_p);

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
                                      s_allocator_p);

        // Iterate and verify
        bmqp::PutMessageIterator iter(&blob,
                                      eventHeader,
                                      false,  // decompress flag
                                      &bufferFactory,
                                      s_allocator_p);

        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(true, iter.next());
        ASSERT_EQ(queueId, iter.header().queueId());
        ASSERT_EQ(guid, iter.header().messageGUID());
        ASSERT_EQ(false, iter.hasMessageProperties());

        mwcu::BlobPosition emptyPos;
        ASSERT_EQ(false, 0 == iter.loadMessagePropertiesPosition(&emptyPos));
        ASSERT_EQ(mwcu::BlobPosition(), emptyPos);

        ASSERT_EQ(0, iter.loadApplicationDataPosition(&retrievedPayloadPos));
        ASSERT_EQ(retrievedPayloadPos, expectedPayloadPos);

        retrievedPayloadBlob.removeAll();
        ASSERT_EQ(0, iter.loadApplicationData(&retrievedPayloadBlob));
        ASSERT(retrievedPayloadBlob.length() > 0);

        ASSERT_EQ(false, iter.hasMsgGroupId());
        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(false, iter.next());
        ASSERT_EQ(false, iter.isValid());

        // Copy
        bmqp::PutMessageIterator iter2(iter, s_allocator_p);
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
        iter.reset(&blob, eventHeader, true);
        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(true, iter.next());
        ASSERT_EQ(queueId, iter.header().queueId());
        ASSERT_EQ(guid, iter.header().messageGUID());
        ASSERT_EQ(expectedBlobLength, iter.applicationDataSize());
        ASSERT_EQ(false, iter.hasMessageProperties());

        ASSERT_EQ(false, 0 == iter.loadMessagePropertiesPosition(&emptyPos));
        ASSERT_EQ(mwcu::BlobPosition(), emptyPos);

        bdlbb::Blob        retrievedPayloadBlob2(s_allocator_p);
        mwcu::BlobPosition retrievedPayloadPos2;

        ASSERT_EQ(0, iter.loadApplicationDataPosition(&retrievedPayloadPos2));
        ASSERT_EQ(retrievedPayloadPos2, expectedPayloadPos);

        retrievedPayloadBlob2.removeAll();
        ASSERT_EQ(0, iter.loadApplicationData(&retrievedPayloadBlob2));
        ASSERT_EQ(0,
                  bdlbb::BlobUtil::compare(retrievedPayloadBlob2,
                                           expectedBlob));

        ASSERT_EQ(false, iter.hasMsgGroupId());
        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(false, iter.next());
        ASSERT_EQ(false, iter.isValid());
    }

    {
        PVV("CREATE VALID ITER DECOMPRESSFLAG TRUE, E_NONE COMPRESSION");
        bdlbb::Blob        blob(&bufferFactory, s_allocator_p);
        bdlbb::Blob        expectedBlob(&bufferFactory, s_allocator_p);
        int                expectedBlobLength = 0;
        mwcu::BlobPosition expectedHeaderPos;
        mwcu::BlobPosition expectedPayloadPos;
        mwcu::BlobPosition retrievedPayloadPos;
        bdlbb::Blob        retrievedPayloadBlob(&bufferFactory, s_allocator_p);

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
                                      s_allocator_p);

        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(true, iter.next());
        ASSERT_EQ(queueId, iter.header().queueId());
        ASSERT_EQ(guid, iter.header().messageGUID());
        ASSERT_EQ(expectedBlobLength, iter.messagePayloadSize());
        ASSERT_EQ(expectedBlobLength, iter.applicationDataSize());
        ASSERT_EQ(false, iter.hasMessageProperties());
        ASSERT_EQ(0, iter.messagePropertiesSize());

        bdlbb::Blob emptyBlob(s_allocator_p);
        ASSERT_EQ(0, iter.loadMessageProperties(&emptyBlob));
        ASSERT_EQ(0, emptyBlob.length());

        mwcu::BlobPosition emptyPos;
        ASSERT_EQ(false, 0 == iter.loadMessagePropertiesPosition(&emptyPos));
        ASSERT_EQ(mwcu::BlobPosition(), emptyPos);

        bmqp::MessageProperties emptyProps(s_allocator_p);
        ASSERT_EQ(0, iter.loadMessageProperties(&emptyProps));
        ASSERT_EQ(0, emptyProps.numProperties());

        ASSERT_EQ(0, iter.loadMessagePayload(&retrievedPayloadBlob));
        ASSERT_EQ(0,
                  bdlbb::BlobUtil::compare(retrievedPayloadBlob,
                                           expectedBlob));

        ASSERT_EQ(0, iter.loadApplicationDataPosition(&retrievedPayloadPos));
        ASSERT_EQ(retrievedPayloadPos, expectedPayloadPos);

        retrievedPayloadBlob.removeAll();
        ASSERT_EQ(0, iter.loadApplicationData(&retrievedPayloadBlob));
        ASSERT_EQ(0,
                  bdlbb::BlobUtil::compare(retrievedPayloadBlob,
                                           expectedBlob));

        ASSERT_EQ(false, iter.hasMsgGroupId());
        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(false, iter.next());
        ASSERT_EQ(false, iter.isValid());

        // Copy
        bmqp::PutMessageIterator iter2(iter, s_allocator_p);
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
        iter.reset(&blob, eventHeader, true);
        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(true, iter.next());
        ASSERT_EQ(queueId, iter.header().queueId());
        ASSERT_EQ(guid, iter.header().messageGUID());
        ASSERT_EQ(expectedBlobLength, iter.messagePayloadSize());
        ASSERT_EQ(expectedBlobLength, iter.applicationDataSize());
        ASSERT_EQ(false, iter.hasMessageProperties());
        ASSERT_EQ(0, iter.messagePropertiesSize());
        ASSERT_EQ(0, iter.loadMessageProperties(&emptyBlob));
        ASSERT_EQ(0, emptyBlob.length());
        ASSERT_EQ(false, 0 == iter.loadMessagePropertiesPosition(&emptyPos));
        ASSERT_EQ(mwcu::BlobPosition(), emptyPos);
        ASSERT_EQ(0, iter.loadMessageProperties(&emptyProps));
        ASSERT_EQ(0, emptyProps.numProperties());

        bdlbb::Blob retrievedPayloadBlob2(&bufferFactory, s_allocator_p);
        mwcu::BlobPosition retrievedPayloadPos2;

        ASSERT_EQ(0, iter.loadMessagePayload(&retrievedPayloadBlob2));
        ASSERT_EQ(0,
                  bdlbb::BlobUtil::compare(retrievedPayloadBlob2,
                                           expectedBlob));

        ASSERT_EQ(0, iter.loadApplicationDataPosition(&retrievedPayloadPos2));
        ASSERT_EQ(retrievedPayloadPos2, expectedPayloadPos);

        retrievedPayloadBlob2.removeAll();
        ASSERT_EQ(0, iter.loadApplicationData(&retrievedPayloadBlob2));
        ASSERT_EQ(0,
                  bdlbb::BlobUtil::compare(retrievedPayloadBlob2,
                                           expectedBlob));

        ASSERT_EQ(false, iter.hasMsgGroupId());
        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(false, iter.next());
        ASSERT_EQ(false, iter.isValid());
    }

    {
        PVV("CREATE VALID ITER DECOMPRESSFLAG TRUE, E_ZLIB COMPRESSION");
        bdlbb::Blob        blob(&bufferFactory, s_allocator_p);
        bdlbb::Blob        expectedBlob(&bufferFactory, s_allocator_p);
        int                expectedBlobLength = 0;
        mwcu::BlobPosition expectedHeaderPos;
        mwcu::BlobPosition expectedPayloadPos;
        mwcu::BlobPosition retrievedPayloadPos;
        bdlbb::Blob        retrievedPayloadBlob(&bufferFactory, s_allocator_p);

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
                                      s_allocator_p);

        // Iterate and verify
        bmqp::PutMessageIterator iter(&blob,
                                      eventHeader,
                                      true,  // decompress flag
                                      &bufferFactory,
                                      s_allocator_p);

        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(true, iter.next());
        ASSERT_EQ(queueId, iter.header().queueId());
        ASSERT_EQ(guid, iter.header().messageGUID());
        ASSERT_EQ(expectedBlobLength, iter.messagePayloadSize());
        ASSERT_EQ(expectedBlobLength, iter.applicationDataSize());
        ASSERT_EQ(false, iter.hasMessageProperties());
        ASSERT_EQ(0, iter.messagePropertiesSize());

        bdlbb::Blob emptyBlob(s_allocator_p);
        ASSERT_EQ(0, iter.loadMessageProperties(&emptyBlob));
        ASSERT_EQ(0, emptyBlob.length());

        mwcu::BlobPosition emptyPos;
        ASSERT_EQ(false, 0 == iter.loadMessagePropertiesPosition(&emptyPos));
        ASSERT_EQ(mwcu::BlobPosition(), emptyPos);

        bmqp::MessageProperties emptyProps(s_allocator_p);
        ASSERT_EQ(0, iter.loadMessageProperties(&emptyProps));
        ASSERT_EQ(0, emptyProps.numProperties());

        ASSERT_EQ(0, iter.loadMessagePayload(&retrievedPayloadBlob));
        ASSERT_EQ(0,
                  bdlbb::BlobUtil::compare(retrievedPayloadBlob,
                                           expectedBlob));

        ASSERT_EQ(0, iter.loadApplicationDataPosition(&retrievedPayloadPos));
        ASSERT_EQ(retrievedPayloadPos, expectedPayloadPos);

        retrievedPayloadBlob.removeAll();
        ASSERT_EQ(0, iter.loadApplicationData(&retrievedPayloadBlob));
        ASSERT_EQ(0,
                  bdlbb::BlobUtil::compare(retrievedPayloadBlob,
                                           expectedBlob));

        ASSERT_EQ(false, iter.hasMsgGroupId());
        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(false, iter.next());
        ASSERT_EQ(false, iter.isValid());

        // Copy
        bmqp::PutMessageIterator iter2(iter, s_allocator_p);
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
        iter.reset(&blob, eventHeader, true);
        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(true, iter.next());
        ASSERT_EQ(queueId, iter.header().queueId());
        ASSERT_EQ(guid, iter.header().messageGUID());
        ASSERT_EQ(expectedBlobLength, iter.messagePayloadSize());
        ASSERT_EQ(expectedBlobLength, iter.applicationDataSize());
        ASSERT_EQ(false, iter.hasMessageProperties());
        ASSERT_EQ(0, iter.messagePropertiesSize());
        ASSERT_EQ(0, iter.loadMessageProperties(&emptyBlob));
        ASSERT_EQ(0, emptyBlob.length());
        ASSERT_EQ(false, 0 == iter.loadMessagePropertiesPosition(&emptyPos));
        ASSERT_EQ(mwcu::BlobPosition(), emptyPos);
        ASSERT_EQ(0, iter.loadMessageProperties(&emptyProps));
        ASSERT_EQ(0, emptyProps.numProperties());

        bdlbb::Blob retrievedPayloadBlob2(&bufferFactory, s_allocator_p);
        mwcu::BlobPosition retrievedPayloadPos2;

        ASSERT_EQ(0, iter.loadMessagePayload(&retrievedPayloadBlob2));
        ASSERT_EQ(0,
                  bdlbb::BlobUtil::compare(retrievedPayloadBlob2,
                                           expectedBlob));

        ASSERT_EQ(0, iter.loadApplicationDataPosition(&retrievedPayloadPos2));
        ASSERT_EQ(retrievedPayloadPos2, expectedPayloadPos);

        retrievedPayloadBlob2.removeAll();
        ASSERT_EQ(0, iter.loadApplicationData(&retrievedPayloadBlob2));
        ASSERT_EQ(0,
                  bdlbb::BlobUtil::compare(retrievedPayloadBlob2,
                                           expectedBlob));

        ASSERT_EQ(false, iter.hasMsgGroupId());
        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(false, iter.next());
        ASSERT_EQ(false, iter.isValid());
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
    mwctst::TestHelper::printTestName("RESET");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);

    {
        PVV("NO COMPRESSION, DECOMPRESS FLAG FALSE");
        bmqp::PutMessageIterator pmt(&bufferFactory, s_allocator_p);
        bdlbb::Blob              copiedBlob(s_allocator_p);
        bdlbb::Blob              expectedBlob(&bufferFactory, s_allocator_p);
        int                      expectedBlobLength = 0;
        mwcu::BlobPosition       headerPosition;
        mwcu::BlobPosition       payloadPosition;
        const int                queueId = 123;
        bmqp::EventHeader        eventHeader;
        bdlbb::Blob              payloadBlob(s_allocator_p);
        bdlbb::Blob              appDataBlob(s_allocator_p);
        bmqt::MessageGUID        guid = bmqp::MessageGUIDGenerator::testGUID();

        {
            bdlbb::Blob blob(&bufferFactory, s_allocator_p);

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
                                          s_allocator_p);

            ASSERT_EQ(true, iter.isValid());

            // Copy 'blob' into 'copiedBlob'. Reset 'pmt' with 'copiedBlob'
            // and 'iter'

            copiedBlob = blob;
            pmt.reset(&copiedBlob, iter);
        }

        // Iterate and verify
        ASSERT_EQ(true, pmt.isValid());
        ASSERT_EQ(true, pmt.next());
        ASSERT_EQ(queueId, pmt.header().queueId());
        ASSERT_EQ(guid, pmt.header().messageGUID());
        ASSERT_EQ(expectedBlobLength, pmt.applicationDataSize());

        ASSERT_EQ(0, pmt.loadApplicationData(&appDataBlob));
        ASSERT_EQ(0, bdlbb::BlobUtil::compare(appDataBlob, expectedBlob));

        ASSERT_EQ(false, pmt.hasMsgGroupId());
        ASSERT_EQ(true, pmt.isValid());
        ASSERT_EQ(false, pmt.next());
        ASSERT_EQ(false, pmt.isValid());
    }

    {
        PVV("ZLIB COMPRESSION, DECOMPRESS FLAG TRUE");
        bmqp::PutMessageIterator pmt(&bufferFactory, s_allocator_p);
        bdlbb::Blob              copiedBlob(s_allocator_p);
        bdlbb::Blob              expectedBlob(&bufferFactory, s_allocator_p);
        int                      expectedBlobLength = 0;
        mwcu::BlobPosition       headerPosition;
        mwcu::BlobPosition       payloadPosition;
        const int                queueId = 123;
        bmqp::EventHeader        eventHeader;
        bdlbb::Blob              payloadBlob(s_allocator_p);
        bdlbb::Blob              appDataBlob(s_allocator_p);
        bmqt::MessageGUID        guid = bmqp::MessageGUIDGenerator::testGUID();

        {
            bdlbb::Blob blob(&bufferFactory, s_allocator_p);

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
                s_allocator_p);

            bmqp::PutMessageIterator iter(&blob,
                                          eventHeader,
                                          true,
                                          &bufferFactory,
                                          s_allocator_p);

            ASSERT_EQ(true, iter.isValid());

            // Copy 'blob' into 'copiedBlob'. Reset 'pmt' with 'copiedBlob'
            // and 'iter'

            copiedBlob = blob;
            pmt.reset(&copiedBlob, iter);
        }

        // Iterate and verify
        ASSERT_EQ(true, pmt.isValid());
        ASSERT_EQ(true, pmt.next());
        ASSERT_EQ(queueId, pmt.header().queueId());
        ASSERT_EQ(guid, pmt.header().messageGUID());
        ASSERT_EQ(expectedBlobLength, pmt.messagePayloadSize());
        ASSERT_EQ(expectedBlobLength, pmt.applicationDataSize());

        ASSERT_EQ(0, pmt.loadMessagePayload(&payloadBlob));
        ASSERT_EQ(0, bdlbb::BlobUtil::compare(payloadBlob, expectedBlob));

        ASSERT_EQ(0, pmt.loadApplicationData(&appDataBlob));
        ASSERT_EQ(0, bdlbb::BlobUtil::compare(appDataBlob, expectedBlob));

        ASSERT_EQ(false, pmt.hasMsgGroupId());
        ASSERT_EQ(true, pmt.isValid());
        ASSERT_EQ(false, pmt.next());
        ASSERT_EQ(false, pmt.isValid());
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
    mwctst::TestHelper::printTestName("PUT EVENT WITH NO MESSAGES");

    // Test
    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    bdlbb::Blob                    eventBlob(&bufferFactory, s_allocator_p);

    bsl::vector<bmqp::PutTester::Data> data(s_allocator_p);
    bmqp::EventHeader                  eventHeader;

    bmqp::PutTester::populateBlob(&eventBlob,
                                  &eventHeader,
                                  &data,
                                  0,
                                  &bufferFactory,
                                  false,  // No zero-length msgs
                                  s_allocator_p);

    // Verify non-validity
    bmqp::PutMessageIterator iter(&bufferFactory, s_allocator_p);
    ASSERT_LT(iter.reset(&eventBlob, eventHeader, true), 0);
    ASSERT_EQ(iter.isValid(), false);
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
    mwctst::TestHelper::printTestName("INVALID PUT EVENT");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    {
        PVV("NEXT METHOD OVER INVALID BLOB");
        bdlbb::Blob eventBlob(&bufferFactory, s_allocator_p);
        bsl::vector<bmqp::PutTester::Data> data(s_allocator_p);
        bmqp::EventHeader                  eventHeader;

        bmqp::PutTester::populateBlob(&eventBlob,
                                      &eventHeader,
                                      &data,
                                      2,
                                      &bufferFactory,
                                      false,  // No zero-length msgs
                                      s_allocator_p);

        // Render the blob invalid by removing it's last byte
        bdlbb::BlobUtil::erase(&eventBlob, eventBlob.length() - 1, 1);

        // Verify
        bmqp::PutMessageIterator iter(&eventBlob,
                                      eventHeader,
                                      false,
                                      &bufferFactory,
                                      s_allocator_p);

        ASSERT_EQ(true, iter.isValid());  // First message is valid..
        ASSERT_EQ(1, iter.next());        // Second message is error.

        int rc = iter.next();
        ASSERT_LT(rc, 0);

        PVV("Error returned: " << rc);
        iter.dumpBlob(bsl::cout);
    }

    {
        PVV("LOAD OPTIONS METHOD OVER INVALID BLOB");
        bdlbb::Blob eventBlob(&bufferFactory, s_allocator_p);
        bsl::vector<bmqp::PutTester::Data> data(s_allocator_p);
        bmqp::EventHeader                  eventHeader;

        bmqp::PutTester::populateBlob(&eventBlob,
                                      &eventHeader,
                                      &data,
                                      3,
                                      &bufferFactory,
                                      true,  // Include zero-length msgs
                                      s_allocator_p);

        // Verify
        bmqp::PutMessageIterator iter(&eventBlob,
                                      eventHeader,
                                      false,
                                      &bufferFactory,
                                      s_allocator_p);

        // First 3 messages are valid..
        ASSERT_EQ(true, iter.isValid());
        ASSERT_EQ(1, iter.next());
        ASSERT_EQ(1, iter.next());

        bdlbb::Blob options;
        eventBlob.setLength(1);
        int rc = iter.loadOptions(&options);
        ASSERT_LT(rc, 0);

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
    mwctst::TestHelper::printTestName("PUT EVENT WITH MULTIPLE MESSAGES");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);

    {
        PVV("WITH ZLIB COMPRESSION, DECOMPRESS FLAG TRUE");
        bdlbb::Blob eventBlob(&bufferFactory, s_allocator_p);
        bsl::vector<bmqp::PutTester::Data> data(s_allocator_p);
        bmqp::EventHeader                  eventHeader;
        const size_t                       k_NUM_MSGS = 1000;

        bmqp::PutTester::populateBlob(&eventBlob,
                                      &eventHeader,
                                      &data,
                                      k_NUM_MSGS,
                                      &bufferFactory,
                                      false,  // No zero-length PUT msgs.
                                      s_allocator_p,
                                      bmqt::CompressionAlgorithmType::e_ZLIB);

        // Iterate and verify
        bmqp::PutMessageIterator iter(&eventBlob,
                                      eventHeader,
                                      true,
                                      &bufferFactory,
                                      s_allocator_p);
        ASSERT_EQ(true, iter.isValid());

        size_t index = 0;

        while (iter.next() == 1 && index < data.size()) {
            const bmqp::PutTester::Data& D = data[index];

            ASSERT_EQ_D(index, D.d_queueId, iter.header().queueId());

            const bool hasMsgProps = bmqp::PutHeaderFlagUtil::isSet(
                iter.header().flags(),
                bmqp::PutHeaderFlags::e_MESSAGE_PROPERTIES);
            ASSERT_EQ_D(index, true, hasMsgProps);
            ASSERT_EQ_D(index, hasMsgProps, iter.hasMessageProperties());
            ASSERT_EQ_D(index, D.d_propLen, iter.messagePropertiesSize());
            ASSERT_EQ_D(index, D.d_msgLen, iter.messagePayloadSize());
            ASSERT_EQ_D(index,
                        (D.d_msgLen + D.d_propLen),
                        iter.applicationDataSize());

            bdlbb::Blob        options(s_allocator_p);
            bdlbb::Blob        props(s_allocator_p);
            bdlbb::Blob        payload(s_allocator_p);
            bdlbb::Blob        appData(s_allocator_p);
            mwcu::BlobPosition propsPos;
            mwcu::BlobPosition payloadPos;
            mwcu::BlobPosition appDataPos;

            ASSERT_EQ_D(index,
                        0,
                        iter.loadMessagePropertiesPosition(&propsPos));

            ASSERT_EQ_D(index,
                        0,
                        iter.loadApplicationDataPosition(&appDataPos));

            ASSERT_EQ_D(index, 0, iter.loadMessageProperties(&props));
            ASSERT_EQ_D(index,
                        0,
                        bdlbb::BlobUtil::compare(props, D.d_properties));

            ASSERT_EQ_D(index, 0, iter.loadMessagePayload(&payload));
            ASSERT_EQ_D(index,
                        0,
                        bdlbb::BlobUtil::compare(payload, D.d_payload));

            ASSERT_EQ_D(index, 0, iter.loadApplicationData(&appData));
            ASSERT_EQ_D(index,
                        0,
                        bdlbb::BlobUtil::compare(appData, D.d_appData));

            const bool hasMsgGroupId = iter.hasMsgGroupId();
            ASSERT_EQ_D(index, hasMsgGroupId, !D.d_msgGroupId.isNull());
            if (hasMsgGroupId) {
                bmqp::Protocol::MsgGroupId msgGroupId(s_allocator_p);
                ASSERT_EQ_D(index, true, iter.extractMsgGroupId(&msgGroupId));

                ASSERT_EQ_D(index, msgGroupId, D.d_msgGroupId.valueOr(""));
            }
            typedef bmqp::OptionUtil::OptionMeta OptionMeta;
            const OptionMeta                     msgGroupIdOption =
                hasMsgGroupId ? OptionMeta::forOptionWithPadding(
                                    bmqp::OptionType::e_MSG_GROUP_ID,
                                    D.d_msgGroupId.value().length())
                                                  : OptionMeta::forNullOption();

            const int optionsSize = hasMsgGroupId ? msgGroupIdOption.size()
                                                  : 0;

            ASSERT_EQ_D(index, 0, iter.loadOptions(&options));

            ASSERT_EQ_D(index, optionsSize, options.length());

            ++index;
        }

        ASSERT_EQ(index, data.size());
        ASSERT_EQ(false, iter.isValid());
    }

    {
        PVV("WITH NO COMPRESSION, DECOMPRESS FLAG TRUE");
        bdlbb::Blob eventBlob(&bufferFactory, s_allocator_p);
        bsl::vector<bmqp::PutTester::Data> data(s_allocator_p);
        bmqp::EventHeader                  eventHeader;
        const size_t                       k_NUM_MSGS = 1000;

        bmqp::PutTester::populateBlob(&eventBlob,
                                      &eventHeader,
                                      &data,
                                      k_NUM_MSGS,
                                      &bufferFactory,
                                      false,  // No zero-length PUT msgs.
                                      s_allocator_p);

        // Iterate and verify
        bmqp::PutMessageIterator iter(&eventBlob,
                                      eventHeader,
                                      true,
                                      &bufferFactory,
                                      s_allocator_p);
        ASSERT_EQ(true, iter.isValid());

        size_t index = 0;

        while (iter.next() == 1 && index < data.size()) {
            const bmqp::PutTester::Data& D = data[index];

            ASSERT_EQ_D(index, D.d_queueId, iter.header().queueId());

            const bool hasMsgProps = bmqp::PutHeaderFlagUtil::isSet(
                iter.header().flags(),
                bmqp::PutHeaderFlags::e_MESSAGE_PROPERTIES);
            ASSERT_EQ_D(index, true, hasMsgProps);
            ASSERT_EQ_D(index, hasMsgProps, iter.hasMessageProperties());
            ASSERT_EQ_D(index, D.d_propLen, iter.messagePropertiesSize());
            ASSERT_EQ_D(index, D.d_msgLen, iter.messagePayloadSize());
            ASSERT_EQ_D(index,
                        (D.d_msgLen + D.d_propLen),
                        iter.applicationDataSize());

            bdlbb::Blob        options(s_allocator_p);
            bdlbb::Blob        props(s_allocator_p);
            bdlbb::Blob        payload(s_allocator_p);
            bdlbb::Blob        appData(s_allocator_p);
            mwcu::BlobPosition propsPos;
            mwcu::BlobPosition payloadPos;
            mwcu::BlobPosition appDataPos;

            ASSERT_EQ_D(index,
                        0,
                        iter.loadMessagePropertiesPosition(&propsPos));

            ASSERT_EQ_D(index,
                        0,
                        iter.loadApplicationDataPosition(&appDataPos));

            ASSERT_EQ_D(index, 0, iter.loadMessageProperties(&props));
            ASSERT_EQ_D(index,
                        0,
                        bdlbb::BlobUtil::compare(props, D.d_properties));

            ASSERT_EQ_D(index, 0, iter.loadMessagePayload(&payload));
            ASSERT_EQ_D(index,
                        0,
                        bdlbb::BlobUtil::compare(payload, D.d_payload));

            ASSERT_EQ_D(index, 0, iter.loadApplicationData(&appData));
            ASSERT_EQ_D(index,
                        0,
                        bdlbb::BlobUtil::compare(appData, D.d_appData));

            const bool hasMsgGroupId = iter.hasMsgGroupId();
            ASSERT_EQ_D(index, hasMsgGroupId, !D.d_msgGroupId.isNull());
            if (hasMsgGroupId) {
                bmqp::Protocol::MsgGroupId msgGroupId(s_allocator_p);
                ASSERT_EQ_D(index, true, iter.extractMsgGroupId(&msgGroupId));

                ASSERT_EQ_D(index, msgGroupId, D.d_msgGroupId.valueOr(""));
            }
            typedef bmqp::OptionUtil::OptionMeta OptionMeta;
            const OptionMeta                     msgGroupIdOption =
                hasMsgGroupId ? OptionMeta::forOptionWithPadding(
                                    bmqp::OptionType::e_MSG_GROUP_ID,
                                    D.d_msgGroupId.value().length())
                                                  : OptionMeta::forNullOption();

            const int optionsSize = hasMsgGroupId ? msgGroupIdOption.size()
                                                  : 0;

            ASSERT_EQ_D(index, 0, iter.loadOptions(&options));

            ASSERT_EQ_D(index, optionsSize, options.length());

            ++index;
        }

        ASSERT_EQ(index, data.size());
        ASSERT_EQ(false, iter.isValid());
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
    mwctst::TestHelper::printTestName("PUT EVENT WITH ZERO-LENGTH MESSAGES");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);

    {
        PVV("WITH NO COMPRESSION, DECOMPRESS FLAG TRUE");
        bdlbb::Blob eventBlob(&bufferFactory, s_allocator_p);
        bsl::vector<bmqp::PutTester::Data> data(s_allocator_p);
        bmqp::EventHeader                  eventHeader;
        const size_t                       k_NUM_MSGS = 1000;

        bmqp::PutTester::populateBlob(&eventBlob,
                                      &eventHeader,
                                      &data,
                                      k_NUM_MSGS,
                                      &bufferFactory,
                                      true,  // make some PUT msgs zero-length
                                      s_allocator_p);

        // Iterate and verify
        bmqp::PutMessageIterator iter(&eventBlob,
                                      eventHeader,
                                      true,
                                      &bufferFactory,
                                      s_allocator_p);
        ASSERT_EQ(true, iter.isValid());

        size_t index = 0;

        while (iter.next() == 1 && index < data.size()) {
            const bmqp::PutTester::Data& D = data[index];
            ASSERT_EQ_D(index, D.d_queueId, iter.header().queueId());

            const bool hasMsgProps = bmqp::PutHeaderFlagUtil::isSet(
                iter.header().flags(),
                bmqp::PutHeaderFlags::e_MESSAGE_PROPERTIES);
            ASSERT_EQ_D(index, true, hasMsgProps);
            ASSERT_EQ_D(index, hasMsgProps, iter.hasMessageProperties());
            ASSERT_EQ_D(index, D.d_propLen, iter.messagePropertiesSize());
            ASSERT_EQ_D(index, D.d_msgLen, iter.messagePayloadSize());
            ASSERT_EQ_D(index,
                        (D.d_msgLen + D.d_propLen),
                        iter.applicationDataSize());

            bdlbb::Blob        props(s_allocator_p);
            bdlbb::Blob        payload(s_allocator_p);
            bdlbb::Blob        appData(s_allocator_p);
            mwcu::BlobPosition propsPos;
            mwcu::BlobPosition payloadPos;
            mwcu::BlobPosition appDataPos;

            ASSERT_EQ_D(index,
                        0,
                        iter.loadMessagePropertiesPosition(&propsPos));

            ASSERT_EQ_D(index,
                        0,
                        iter.loadApplicationDataPosition(&appDataPos));

            ASSERT_EQ_D(index, 0, iter.loadMessageProperties(&props));
            ASSERT_EQ_D(index,
                        0,
                        bdlbb::BlobUtil::compare(props, D.d_properties));

            ASSERT_EQ_D(index, 0, iter.loadMessagePayload(&payload));
            ASSERT_EQ_D(index,
                        0,
                        bdlbb::BlobUtil::compare(payload, D.d_payload));

            ASSERT_EQ_D(index, 0, iter.loadApplicationData(&appData));
            ASSERT_EQ_D(index,
                        0,
                        bdlbb::BlobUtil::compare(appData, D.d_appData));

            const bool hasMsgGroupId = iter.hasMsgGroupId();
            ASSERT_EQ_D(index, hasMsgGroupId, !D.d_msgGroupId.isNull());
            if (hasMsgGroupId) {
                bmqp::Protocol::MsgGroupId msgGroupId(s_allocator_p);
                ASSERT_EQ_D(index, true, iter.extractMsgGroupId(&msgGroupId));
                ASSERT_EQ_D(index, msgGroupId, D.d_msgGroupId.valueOr(""));
            }

            ++index;
        }

        ASSERT_EQ(index, data.size());
        ASSERT_EQ(false, iter.isValid());
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    bmqp::ProtocolUtil::initialize(s_allocator_p);

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
        s_testStatus = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
