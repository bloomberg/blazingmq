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

// bmqp_protocol.t.cpp                                                -*-C++-*-
#include <bmqp_protocol.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_messageguidgenerator.h>
#include <bmqp_queueid.h>
#include <bmqt_messageguid.h>
#include <bmqt_queueoptions.h>

// MWC
#include <mwcu_memoutstream.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// BDE
#include <bsl_ios.h>
#include <bsl_limits.h>
#include <bsl_string.h>
#include <bslmf_assert.h>
#include <bsls_assert.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

const int k_INT_MAX = bsl::numeric_limits<int>::max();

const unsigned int k_UNSIGNED_INT_MAX =
    bsl::numeric_limits<unsigned int>::max();

const bsls::Types::Uint64 k_33_BITS_MASK = 0x1FFFFFFFF;

struct PrintTestData {
    int         d_line;
    int         d_type;
    const char* d_expected;
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
static void test1_breathingTest()
{
    // --------------------------------------------------------------------
    // BREATHING TEST  bmqp::EventHeader
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
    mwctst::TestHelper::printTestName("BREATHING TEST");

    {
        // -----------
        // Consistency
        // -----------

        // Consumer priority
        const int invalid = bmqp::Protocol::k_CONSUMER_PRIORITY_INVALID;
        const int min     = bmqp::Protocol::k_CONSUMER_PRIORITY_MIN;
        const int max     = bmqp::Protocol::k_CONSUMER_PRIORITY_MAX;

        ASSERT_EQ(invalid,
                  bmqp_ctrlmsg::QueueStreamParameters ::
                      DEFAULT_INITIALIZER_CONSUMER_PRIORITY);
        ASSERT_LT(invalid, min);
        ASSERT_LE(min, max);

        ASSERT_EQ(min, bmqt::QueueOptions::k_CONSUMER_PRIORITY_MIN);
        ASSERT_EQ(max, bmqt::QueueOptions::k_CONSUMER_PRIORITY_MAX);
    }

    {
        // ---------
        // Constants
        // ---------

        // Currently, value of 'StorageHeader::k_MAX_PAYLOAD_SIZE_SOFT' is
        // derived from 'PutHeader::k_MAX_PAYLOAD_SIZE_SOFT' (former must be
        // greater than later -- see notes in
        // 'StorageHeader::k_MAX_PAYLOAD_SIZE_SOFT' constant).  Ensure that its
        // true.
        ASSERT_GT(
            static_cast<int>(bmqp::StorageHeader::k_MAX_PAYLOAD_SIZE_SOFT),
            bmqp::PutHeader::k_MAX_PAYLOAD_SIZE_SOFT);

        // Similar to above, value of 'EventHeader::k_MAX_SIZE_SOFT' is derived
        // from 'StorageHeader::k_MAX_PAYLOAD_SIZE_SOFT' (former must be
        // greater than later -- see notes in 'EventHeader::k_MAX_SIZE_SOFT'
        // constant).  Ensure that its true.
        ASSERT_GT(
            static_cast<unsigned int>(bmqp::EventHeader::k_MAX_SIZE_SOFT),
            bmqp::StorageHeader::k_MAX_PAYLOAD_SIZE_SOFT);

        // Ensure that various MAX_OPTIONS_SIZE constants have the same value.
        ASSERT_EQ(bmqp::Protocol::k_MAX_OPTIONS_SIZE,
                  bmqp::PutHeader::k_MAX_OPTIONS_SIZE);
        ASSERT_EQ(bmqp::Protocol::k_MAX_OPTIONS_SIZE,
                  bmqp::PushHeader::k_MAX_OPTIONS_SIZE);
    }

    {
        // ----------------------
        // RdaInfo Breathing Test
        // ----------------------

        // Create default RdaInfo
        bmqp::RdaInfo info;
        ASSERT_EQ(info.isUnlimited(), true);
        ASSERT_EQ(info.isPotentiallyPoisonous(), false);
        ASSERT_GE(info.counter(), bmqp::RdaInfo::k_MAX_COUNTER_VALUE);

        // Going from unlimited redelivery to setting a counter value
        info.setCounter(16U);
        ASSERT_EQ(info.counter(), 16U);
        ASSERT_EQ(info.isUnlimited(), false);
        ASSERT_EQ(info.isPotentiallyPoisonous(), false);

        // Set the RdaInfo as being potentially poisonous
        info.setPotentiallyPoisonous(true);
        ASSERT_EQ(info.isPotentiallyPoisonous(), true);

        // Make sure nothing else was changed
        ASSERT_EQ(info.isUnlimited(), false);
        ASSERT_EQ(info.counter(), 16U);

        // Change counter value and make sure nothing else changed
        info.setCounter(31U);
        ASSERT_EQ(info.counter(), 31U);
        ASSERT_EQ(info.isUnlimited(), false);
        ASSERT_EQ(info.isPotentiallyPoisonous(), true);

        // Set the RdaInfo to unlimited
        info.setUnlimited();
        ASSERT_EQ(info.isUnlimited(), true);
        ASSERT_EQ(info.isPotentiallyPoisonous(), false);
        ASSERT_GE(info.counter(), bmqp::RdaInfo::k_MAX_COUNTER_VALUE);

        // Create RdaInfo using by populating the internal representation of
        // the counter with the potentially poisonous flag with a counter set
        // to 11
        unsigned int  internalRepresentation = 0x4B;
        bmqp::RdaInfo info2(internalRepresentation);
        ASSERT_EQ(info2.isUnlimited(), false);
        ASSERT_EQ(info2.isPotentiallyPoisonous(), true);
        ASSERT_EQ(info2.counter(), 11U);
    }

    {
        // ---------------------------
        // SubQueueInfo Breathing Test
        // ---------------------------

        // Create default SubQueueInfo
        bmqp::SubQueueInfo info;
        ASSERT_EQ(info.id(), bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);
        ASSERT_EQ(info.rdaInfo().isUnlimited(), true);

        // Set some values
        info.setId(96U);
        info.rdaInfo().setCounter(17U);
        ASSERT_EQ(info.id(), 96U);
        ASSERT_EQ(info.rdaInfo().counter(), 17U);
        ASSERT_EQ(info.rdaInfo().isUnlimited(), false);

        // Create SubQueueInfo with non-default ctor
        bmqp::SubQueueInfo info2(888U);
        info2.rdaInfo().setCounter(29U);
        ASSERT_EQ(info2.id(), 888U);
        ASSERT_EQ(info2.rdaInfo().counter(), 29U);
        ASSERT_EQ(info2.rdaInfo().isUnlimited(), false);
    }

    {
        // --------------------------
        // EventHeader Breathing Test
        // --------------------------

        // Create default EventHeader
        bmqp::EventHeader eh;
        const int         numWords = sizeof(eh) / 4;

        // Oddly, using directly 'bmqp::Protocol::k_VERSION' in the ASSERT_EQ
        // leads to a linker undefined symbol ..
        const int currentProtocolVersion = bmqp::Protocol::k_VERSION;

        ASSERT_EQ(static_cast<size_t>(eh.length()), sizeof(eh));
        ASSERT_EQ(eh.fragmentBit(), 0);
        ASSERT_EQ(eh.protocolVersion(), currentProtocolVersion);
        ASSERT_EQ(eh.headerWords(), numWords);
        ASSERT_EQ(eh.type(), bmqp::EventType::e_UNDEFINED);
        ASSERT_EQ(eh.typeSpecific(), 0);

        // Create EventHeader with non-default ctor
        bmqp::EventHeader              eh2(bmqp::EventType::e_CONTROL);
        const bmqp::EncodingType::Enum encodingType =
            bmqp::EventHeaderUtil::controlEventEncodingType(eh2);

        ASSERT_EQ(static_cast<size_t>(eh2.length()), sizeof(eh2));
        ASSERT_EQ(eh2.fragmentBit(), 0);
        ASSERT_EQ(eh2.protocolVersion(), currentProtocolVersion);
        ASSERT_EQ(eh2.headerWords(), numWords);
        ASSERT_EQ(eh2.type(), bmqp::EventType::e_CONTROL);
        ASSERT_EQ(encodingType, bmqp::EncodingType::e_BER);

        // Set min values on a default EventHeader
        bmqp::EventHeader           eh3;
        const int                   minLength          = 0;
        const unsigned char         minProtocolVersion = 0;
        const bmqp::EventType::Enum minEventType =
            bmqp::EventType::e_UNDEFINED;
        const unsigned char minHeaderWords  = 0;
        const unsigned char minTypeSpecific = 0;  // 0b00000000 in C++14

        eh3.setLength(minLength);
        eh3.setProtocolVersion(minProtocolVersion);
        eh3.setHeaderWords(minHeaderWords);
        eh3.setType(minEventType);
        eh3.setTypeSpecific(minTypeSpecific);

        ASSERT_EQ(eh3.length(), minLength);
        ASSERT_EQ(eh3.fragmentBit(), 0);
        ASSERT_EQ(eh3.protocolVersion(), minProtocolVersion);
        ASSERT_EQ(eh3.headerWords(), minHeaderWords);
        ASSERT_EQ(eh3.type(), minEventType);
        ASSERT_EQ(eh3.typeSpecific(), minTypeSpecific);

        // Set max values on a default EventHeader
        bmqp::EventHeader           eh4;
        const int                   maxLength          = k_INT_MAX;
        const unsigned char         maxProtocolVersion = 3;
        const bmqp::EventType::Enum maxEventType =
            bmqp::EventType::e_HEARTBEAT_RSP;
        const unsigned char maxHeaderWords  = 255;
        const unsigned char maxTypeSpecific = 255;  // 0b11111111 in C++14

        eh4.setLength(maxLength);
        eh4.setProtocolVersion(maxProtocolVersion);
        eh4.setHeaderWords(maxHeaderWords);
        eh4.setType(maxEventType);
        eh4.setTypeSpecific(maxTypeSpecific);

        ASSERT_EQ(eh4.length(), maxLength);
        ASSERT_EQ(eh4.fragmentBit(), 0);
        ASSERT_EQ(eh4.protocolVersion(), maxProtocolVersion);
        ASSERT_EQ(eh4.headerWords(), maxHeaderWords);
        ASSERT_EQ(eh4.type(), maxEventType);
        ASSERT_EQ(eh4.typeSpecific(), maxTypeSpecific);
    }

    {
        // ---------------------------
        // OptionHeader Breathing Test
        // ---------------------------

        bmqp::OptionHeader oh;
        const int          numWords = sizeof(bmqp::OptionHeader) / 4;

        ASSERT_EQ(oh.type(), bmqp::OptionType::e_UNDEFINED);
        ASSERT_EQ(oh.packed(), false);
        ASSERT_EQ(oh.typeSpecific(), 0);
        ASSERT_EQ(oh.words(), numWords);

        // Set some values
        oh.setType(bmqp::OptionType::e_SUB_QUEUE_IDS_OLD);
        oh.setPacked(true);
        oh.setTypeSpecific(13);
        oh.setWords(2097151);
        ASSERT_EQ(oh.type(), bmqp::OptionType::e_SUB_QUEUE_IDS_OLD);
        ASSERT_EQ(oh.packed(), true);
        ASSERT_EQ(oh.typeSpecific(), 13);
        ASSERT_EQ(oh.words(), 2097151);

        // Create non-packed OptionHeader with non-default ctor
        bmqp::OptionHeader oh2(bmqp::OptionType::e_MSG_GROUP_ID,
                               false);  // isPacked
        ASSERT_EQ(oh2.type(), bmqp::OptionType::e_MSG_GROUP_ID);
        ASSERT_EQ(oh2.packed(), false);
        ASSERT_EQ(oh2.typeSpecific(), 0);
        ASSERT_EQ(oh2.words(), numWords);

        // Create *packed* OptionHeader with non-default ctor
        bmqp::OptionHeader oh3(bmqp::OptionType::e_MSG_GROUP_ID,
                               true);  // isPacked
        ASSERT_EQ(oh3.type(), bmqp::OptionType::e_MSG_GROUP_ID);
        ASSERT_EQ(oh3.packed(), true);
        ASSERT_EQ(oh3.typeSpecific(), 0);
        ASSERT_EQ(oh3.words(), 0);
    }

    {
        // --------------------------------------
        // MessagePropertiesHeader Breathing Test
        // --------------------------------------

        bmqp::MessagePropertiesHeader mph;
        ASSERT_EQ(static_cast<size_t>(mph.headerSize()), sizeof(mph));
        ASSERT_EQ(mph.messagePropertiesAreaWords(), 2);
        // Entire msg property area is word aligned, so even though
        // sizeof(MessagePropertiesHeader) == 6 bytes, total area is
        // recorded as 8 bytes (2 words), because it is assumed that
        // padding will be added.

        ASSERT_EQ(static_cast<size_t>(mph.messagePropertyHeaderSize()),
                  sizeof(bmqp::MessagePropertyHeader));

        bmqp::MessagePropertiesHeader mph2;
        mph2.setHeaderSize(14);                 // max per protocol
        mph2.setMessagePropertyHeaderSize(14);  // max per protocol
        mph2.setMessagePropertiesAreaWords(
            bmqp::MessagePropertiesHeader::k_MAX_MESSAGE_PROPERTIES_SIZE /
            bmqp::Protocol::k_WORD_SIZE);  // max per protocol

        ASSERT_EQ(mph2.headerSize(), 14);
        ASSERT_EQ(mph2.messagePropertyHeaderSize(), 14);
        ASSERT_EQ(
            mph2.messagePropertiesAreaWords(),
            bmqp::MessagePropertiesHeader::k_MAX_MESSAGE_PROPERTIES_SIZE /
                bmqp::Protocol::k_WORD_SIZE);

        bmqp::MessagePropertiesHeader mph3;
        mph3.setHeaderSize(10);
        mph3.setMessagePropertyHeaderSize(12);
        mph3.setMessagePropertiesAreaWords(1024 * 123);
        mph3.setNumProperties(8);

        ASSERT_EQ(mph3.headerSize(), 10);
        ASSERT_EQ(mph3.messagePropertyHeaderSize(), 12);
        ASSERT_EQ(mph3.messagePropertiesAreaWords(), 1024 * 123);
        ASSERT_EQ(mph3.numProperties(), 8);
    }

    {
        // ------------------------------------
        // MessagePropertyHeader Breathing Test
        // ------------------------------------

        bmqp::MessagePropertyHeader mph;
        ASSERT_EQ(0, mph.propertyType());
        ASSERT_EQ(0, mph.propertyValueLength());
        ASSERT_EQ(0, mph.propertyNameLength());

        bmqp::MessagePropertyHeader mph2;
        mph2.setPropertyType(31);                    // max per protocol
        mph2.setPropertyValueLength((1 << 26) - 1);  // max per protocol
        mph2.setPropertyNameLength((1 << 12) - 1);   // max per protocol

        ASSERT_EQ(31, mph2.propertyType());
        ASSERT_EQ(((1 << 26) - 1), mph2.propertyValueLength());
        ASSERT_EQ(((1 << 12) - 1), mph2.propertyNameLength());

        bmqp::MessagePropertyHeader mph3;
        mph3.setPropertyType(17);
        mph3.setPropertyValueLength((1 << 19) - 1);
        mph3.setPropertyNameLength((1 << 8) - 1);

        ASSERT_EQ(17, mph3.propertyType());
        ASSERT_EQ(((1 << 19) - 1), mph3.propertyValueLength());
        ASSERT_EQ(((1 << 8) - 1), mph3.propertyNameLength());
    }

    {
        // ------------------------
        // PutHeader Breathing Test
        // ------------------------

        bmqp::PutHeader ph;
        const int       numWords = sizeof(ph) / 4;

        ASSERT_EQ(ph.flags(), 0);
        ASSERT_EQ(static_cast<size_t>(ph.messageWords()), sizeof(ph) / 4);
        ASSERT_EQ(0, ph.optionsWords());
        ASSERT_EQ(bmqt::CompressionAlgorithmType::e_NONE,
                  ph.compressionAlgorithmType());
        ASSERT_EQ(numWords, ph.headerWords());
        ASSERT_EQ(0, ph.queueId());

        ASSERT(ph.messageGUID().isUnset());

        // Set some values
        const int                                  msgNumWords     = 5;
        const int                                  optionsNumWords = 7;
        const int                                  queueId         = 9;
        const unsigned int                         crc32c          = 0x8;
        const bmqt::CompressionAlgorithmType::Enum catType =
            bmqt::CompressionAlgorithmType::e_NONE;
        bmqt::MessageGUID guid = bmqp::MessageGUIDGenerator::testGUID();

        ph.setMessageWords(msgNumWords)
            .setOptionsWords(optionsNumWords)
            .setCompressionAlgorithmType(catType)
            .setQueueId(queueId)
            .setMessageGUID(guid)
            .setCrc32c(crc32c);

        // Set the 'e_ACK_REQUESTED' flag
        int phFlags = ph.flags();
        bmqp::PutHeaderFlagUtil::setFlag(
            &phFlags,
            bmqp::PutHeaderFlags::e_ACK_REQUESTED);
        ph.setFlags(phFlags);

        ASSERT_EQ(msgNumWords, ph.messageWords());
        ASSERT_EQ(optionsNumWords, ph.optionsWords());
        ASSERT_EQ(catType, ph.compressionAlgorithmType());
        ASSERT_EQ(numWords, ph.headerWords());
        ASSERT_EQ(queueId, ph.queueId());
        ASSERT_EQ(guid, ph.messageGUID());
        ASSERT_EQ(crc32c, ph.crc32c());

        ASSERT_EQ(bmqp::PutHeaderFlagUtil::isSet(
                      ph.flags(),
                      bmqp::PutHeaderFlags::e_ACK_REQUESTED),
                  true);

        // Set empty GUID
        bmqt::MessageGUID emptyGUID;
        ph.setMessageGUID(emptyGUID);
        ASSERT_EQ(emptyGUID, ph.messageGUID());

        // Make sure the ACK flag was not erased
        ASSERT_EQ(bmqp::PutHeaderFlagUtil::isSet(
                      ph.flags(),
                      bmqp::PutHeaderFlags::e_ACK_REQUESTED),
                  true);
    }

    {
        // -------------------------
        // PushHeader Breathing Test
        // -------------------------

        bmqp::PushHeader ph;
        const int        numWords = sizeof(ph) / 4;

        ASSERT_EQ(0, ph.flags());
        ASSERT_EQ(sizeof(ph) / 4, static_cast<size_t>(ph.messageWords()));
        ASSERT_EQ(0, ph.optionsWords());
        ASSERT_EQ(bmqt::CompressionAlgorithmType::e_NONE,
                  ph.compressionAlgorithmType());
        ASSERT_EQ(numWords, ph.headerWords());
        ASSERT_EQ(0, ph.queueId());

        // Set some values
        const int                                  msgNumWords     = 5;
        const int                                  optionsNumWords = 7;
        const int                                  queueId         = 9;
        const bmqt::CompressionAlgorithmType::Enum catType =
            bmqt::CompressionAlgorithmType::e_ZLIB;
        bmqt::MessageGUID expectedGUID;

        ph.setMessageWords(msgNumWords)
            .setOptionsWords(optionsNumWords)
            .setCompressionAlgorithmType(catType)
            .setQueueId(queueId)
            .setMessageGUID(expectedGUID);

        // Set the 'e_ACK_REQUESTED' flag
        int phFlags = ph.flags();
        bmqp::PushHeaderFlagUtil::setFlag(
            &phFlags,
            bmqp::PushHeaderFlags::e_IMPLICIT_PAYLOAD);
        ph.setFlags(phFlags);

        ASSERT_EQ(msgNumWords, ph.messageWords());
        ASSERT_EQ(optionsNumWords, ph.optionsWords());
        ASSERT_EQ(catType, ph.compressionAlgorithmType());
        ASSERT_EQ(numWords, ph.headerWords());
        ASSERT_EQ(queueId, ph.queueId());
        ASSERT_EQ(expectedGUID, ph.messageGUID());

        // Check flag
        ASSERT_EQ(bmqp::PushHeaderFlagUtil::isSet(
                      ph.flags(),
                      bmqp::PushHeaderFlags::e_IMPLICIT_PAYLOAD),
                  true);
    }

    {
        // ------------------------
        // AckHeader Breathing Test
        // ------------------------
        bmqp::AckHeader ah;
        const int       numWords = sizeof(ah) / 4;

        ASSERT_EQ(ah.flags(), 0);
        ASSERT_EQ(ah.headerWords(), numWords);
        ASSERT_EQ(static_cast<size_t>(ah.perMessageWords()),
                  sizeof(bmqp::AckMessage) / 4);

        // Set some values
        const char flags       = 123;
        const int  msgNumWords = 5;

        ah.setFlags(flags);
        ah.setPerMessageWords(msgNumWords);

        ASSERT_EQ(ah.flags(), flags);
        ASSERT_EQ(ah.perMessageWords(), msgNumWords);
        ASSERT_EQ(ah.headerWords(), numWords);
    }

    {
        // -----------------------------
        // AckMessage Breathing Test
        // -----------------------------
        bmqp::AckMessage am;

        unsigned char zeroGuidBuf[bmqt::MessageGUID::e_SIZE_BINARY];
        bsl::memset(zeroGuidBuf, 0, bmqt::MessageGUID::e_SIZE_BINARY);
        bmqt::MessageGUID zeroGuid;
        zeroGuid.fromBinary(zeroGuidBuf);

        ASSERT_EQ(am.queueId(), 0);
        ASSERT_EQ(am.status(), 0);
        ASSERT_EQ(am.correlationId(), 0);
        ASSERT_EQ(am.messageGUID(), zeroGuid);

        // Set some values
        const int queueId = 5;
        const int status  = 2;
        const int corrId  = 11;

        unsigned char onesGuidBuf[bmqt::MessageGUID::e_SIZE_BINARY];
        bsl::memset(onesGuidBuf, 1, bmqt::MessageGUID::e_SIZE_BINARY);
        bmqt::MessageGUID onesGuid;
        onesGuid.fromBinary(onesGuidBuf);

        am.setQueueId(queueId)
            .setStatus(status)
            .setCorrelationId(corrId)
            .setMessageGUID(onesGuid);

        ASSERT_EQ(am.queueId(), queueId);
        ASSERT_EQ(am.status(), status);
        ASSERT_EQ(am.correlationId(), corrId);
        ASSERT_EQ(am.messageGUID(), onesGuid);

        // Custom constructor test
        bmqp::AckMessage am2(status, corrId, onesGuid, queueId);
        ASSERT_EQ(queueId, am2.queueId());
        ASSERT_EQ(status, am2.status());
        ASSERT_EQ(corrId, am2.correlationId());
        ASSERT_EQ(onesGuid, am2.messageGUID());
    }

    {
        // ----------------------------
        // ConfirmHeader Breathing Test
        // ----------------------------
        bmqp::ConfirmHeader ch;
        const int           numWords = sizeof(ch) / 4;

        ASSERT_EQ(ch.headerWords(), numWords);
        ASSERT_EQ(static_cast<size_t>(ch.perMessageWords()),
                  sizeof(bmqp::ConfirmMessage) / 4);

        // Set some values
        const int msgNumWords = 5;

        ch.setPerMessageWords(msgNumWords);

        ASSERT_EQ(ch.perMessageWords(), msgNumWords);
        ASSERT_EQ(ch.headerWords(), numWords);
    }

    {
        // -----------------------------
        // ConfirmMessage Breathing Test
        // -----------------------------
        bmqp::ConfirmMessage cm;

        unsigned char zeroGuidBuf[bmqt::MessageGUID::e_SIZE_BINARY];
        bsl::memset(zeroGuidBuf, 0, bmqt::MessageGUID::e_SIZE_BINARY);
        bmqt::MessageGUID zeroGuid;
        zeroGuid.fromBinary(zeroGuidBuf);

        ASSERT_EQ(cm.queueId(), 0);
        ASSERT_EQ(cm.messageGUID(), zeroGuid);
        ASSERT_EQ(cm.subQueueId(), 0);

        // Set some values
        const int queueId    = 5;
        const int subQueueId = 9;

        unsigned char onesGuidBuf[bmqt::MessageGUID::e_SIZE_BINARY];
        bsl::memset(onesGuidBuf, 1, bmqt::MessageGUID::e_SIZE_BINARY);
        bmqt::MessageGUID onesGuid;
        onesGuid.fromBinary(onesGuidBuf);

        cm.setQueueId(queueId);
        cm.setSubQueueId(subQueueId);
        cm.setMessageGUID(onesGuid);

        ASSERT_EQ(cm.queueId(), queueId);
        ASSERT_EQ(cm.messageGUID(), onesGuid);
        ASSERT_EQ(cm.subQueueId(), subQueueId);
    }

    {
        // ---------------------------
        // RejectHeader Breathing Test
        // ---------------------------
        bmqp::RejectHeader rh;
        const int          numWords = sizeof(rh) / 4;

        ASSERT_EQ(rh.headerWords(), numWords);
        ASSERT_EQ(static_cast<size_t>(rh.perMessageWords()),
                  sizeof(bmqp::RejectMessage) / 4);

        // Set some values
        const int msgNumWords = 5;

        rh.setPerMessageWords(msgNumWords);

        ASSERT_EQ(rh.perMessageWords(), msgNumWords);
        ASSERT_EQ(rh.headerWords(), numWords);
    }

    {
        // ----------------------------
        // RejectMessage Breathing Test
        // ----------------------------
        bmqp::RejectMessage rm;

        unsigned char zeroGuidBuf[bmqt::MessageGUID::e_SIZE_BINARY];
        bsl::memset(zeroGuidBuf, 0, bmqt::MessageGUID::e_SIZE_BINARY);
        bmqt::MessageGUID zeroGuid;
        zeroGuid.fromBinary(zeroGuidBuf);

        ASSERT_EQ(rm.queueId(), 0);
        ASSERT_EQ(rm.messageGUID(), zeroGuid);
        ASSERT_EQ(rm.subQueueId(), 0);

        // Set some values
        const int queueId    = 5;
        const int subQueueId = 9;

        unsigned char onesGuidBuf[bmqt::MessageGUID::e_SIZE_BINARY];
        bsl::memset(onesGuidBuf, 1, bmqt::MessageGUID::e_SIZE_BINARY);
        bmqt::MessageGUID onesGuid;
        onesGuid.fromBinary(onesGuidBuf);

        rm.setQueueId(queueId);
        rm.setMessageGUID(onesGuid);
        rm.setSubQueueId(subQueueId);

        ASSERT_EQ(rm.queueId(), queueId);
        ASSERT_EQ(rm.messageGUID(), onesGuid);
        ASSERT_EQ(rm.subQueueId(), subQueueId);
    }

    {
        // ----------------------------
        // StorageHeader Breathing Test
        // ----------------------------
        bmqp::StorageHeader sh;
        const int           numWords = sizeof(sh) / 4;

        ASSERT_EQ(0, sh.flags());
        ASSERT_EQ(numWords, sh.messageWords());
        ASSERT_EQ(0, sh.storageProtocolVersion());
        ASSERT_EQ(numWords, sh.headerWords());
        ASSERT_EQ(0u, sh.partitionId());
        ASSERT_EQ(0u, sh.journalOffsetWords());
        ASSERT_EQ(bmqp::StorageMessageType::e_UNDEFINED, sh.messageType());

        // Set fields
        const int flags = static_cast<int>(
            bmqp::StorageHeaderFlags::e_RECEIPT_REQUESTED);
        const int          words = 7;
        const int          spv   = 2;
        const unsigned int pid   = 11;
        const unsigned int jow   = 54;
        sh.setFlags(flags);
        sh.setMessageWords(words);
        sh.setStorageProtocolVersion(spv);
        sh.setPartitionId(pid);
        sh.setJournalOffsetWords(jow);
        sh.setMessageType(bmqp::StorageMessageType::e_DELETION);

        ASSERT_EQ(flags, sh.flags());
        ASSERT_EQ(words, sh.messageWords());
        ASSERT_EQ(spv, sh.storageProtocolVersion());
        ASSERT_EQ(numWords, sh.headerWords());
        ASSERT_EQ(pid, sh.partitionId());
        ASSERT_EQ(jow, sh.journalOffsetWords());
        ASSERT_EQ(bmqp::StorageMessageType::e_DELETION, sh.messageType());

        ASSERT_EQ(bmqp::StorageHeaderFlagUtil::isSet(
                      static_cast<unsigned char>(sh.flags()),
                      bmqp::StorageHeaderFlags::e_RECEIPT_REQUESTED),
                  true);
    }

    {
        // -----------------------------
        // RecoveryHeader Breathing Test
        // -----------------------------
        bmqp::RecoveryHeader rh;

        const int numWords = sizeof(rh) / 4;
        char      md5Digest[bmqp::RecoveryHeader::k_MD5_DIGEST_LEN];
        bsl::memset(md5Digest, 1, bmqp::RecoveryHeader::k_MD5_DIGEST_LEN);

        ASSERT_EQ(0, rh.isFinalChunk());
        ASSERT_EQ(numWords, rh.messageWords());
        ASSERT_EQ(numWords, rh.headerWords());
        ASSERT_EQ(0u, rh.partitionId());
        ASSERT_EQ(0u, rh.chunkSequenceNumber());
        ASSERT_EQ(bmqp::RecoveryFileChunkType::e_UNDEFINED,
                  rh.fileChunkType());

        // Set fields
        const int                 words = 7;
        const unsigned int        pid   = 11;
        const bsls::Types::Uint64 seqNum =
            bsl::numeric_limits<unsigned int>::max();

        rh.setFinalChunkBit();
        rh.setMessageWords(words);
        rh.setFileChunkType(bmqp::RecoveryFileChunkType::e_JOURNAL);
        rh.setPartitionId(pid);
        rh.setChunkSequenceNumber(seqNum);
        rh.setMd5Digest(md5Digest);

        ASSERT_EQ(true, rh.isFinalChunk());
        ASSERT_EQ(words, rh.messageWords());
        ASSERT_EQ(numWords, rh.headerWords());
        ASSERT_EQ(pid, rh.partitionId());
        ASSERT_EQ(seqNum, rh.chunkSequenceNumber());
        ASSERT_EQ(bmqp::RecoveryFileChunkType::e_JOURNAL, rh.fileChunkType());
        ASSERT_EQ(0,
                  bsl::memcmp(rh.md5Digest(),
                              md5Digest,
                              bmqp::RecoveryHeader::k_MD5_DIGEST_LEN));
    }
}

static void test2_bitManipulation()
{
    // --------------------------------------------------------------------
    // BIT MANIPULATION ROUTINES TEST
    //    bmqp::Protocol::combine()
    //    bmqp::Protocol::getUpper()
    //    bmqp::Protocol::getLower()
    //    bmqp::Protocol::split()
    //
    // Concerns:
    //   Test manipulators
    // --------------------------------------------------------------------
    mwctst::TestHelper::printTestName("BIT MANIPULATION ROUTINE TEST");

    {
        // combine
        unsigned int        U = 0;
        unsigned int        L = 0;
        bsls::Types::Uint64 E = 0;
        ASSERT_EQ(bmqp::Protocol::combine(U, L), E);

        U = 0;
        L = k_UNSIGNED_INT_MAX;
        E = k_UNSIGNED_INT_MAX;
        ASSERT_EQ(bmqp::Protocol::combine(U, L), E);

        U = 1;
        L = 0;
        E = 1ULL << 32;
        ASSERT_EQ(bmqp::Protocol::combine(U, L), E);

        U = 1;
        L = k_UNSIGNED_INT_MAX;
        E = k_33_BITS_MASK;
        ASSERT_EQ(bmqp::Protocol::combine(U, L), E);

        U = k_UNSIGNED_INT_MAX;
        L = k_UNSIGNED_INT_MAX;
        E = bsl::numeric_limits<bsls::Types::Uint64>::max();
        ASSERT_EQ(bmqp::Protocol::combine(U, L), E);

        U = k_UNSIGNED_INT_MAX;
        L = 0;
        E = bsl::numeric_limits<bsls::Types::Uint64>::max() << 32;
        ASSERT_EQ(bmqp::Protocol::combine(U, L), E);

        U = 0xC6E7C7CD;
        L = 0x01FE34E8;
        E = 0xC6E7C7CD01FE34E8;
        ASSERT_EQ(bmqp::Protocol::combine(U, L), E);

        U = 0x80808080;
        L = 0x80808080;
        E = 0x8080808080808080;
        ASSERT_EQ(bmqp::Protocol::combine(U, L), E);

        U = 3195984953;
        L = 4037236647;
        E = 13726650855680333735ULL;
        ASSERT_EQ(bmqp::Protocol::combine(U, L), E);
    }

    {
        // getUpper
        bsls::Types::Uint64 N = 0;
        unsigned int        E = 0;
        ASSERT_EQ(bmqp::Protocol::getUpper(N), E);

        N = 1ULL << 32;
        E = 1;
        ASSERT_EQ(bmqp::Protocol::getUpper(N), E);

        N = k_33_BITS_MASK;
        E = 1;
        ASSERT_EQ(bmqp::Protocol::getUpper(N), E);

        N = bsl::numeric_limits<bsls::Types::Uint64>::max();
        E = k_UNSIGNED_INT_MAX;
        ASSERT_EQ(bmqp::Protocol::getUpper(N), E);

        N = 0xC6E7C7CD01FE34E8;
        E = 0xC6E7C7CD;
        ASSERT_EQ(bmqp::Protocol::getUpper(N), E);

        N = 0x8080808080808080;
        E = 0x80808080;
        ASSERT_EQ(bmqp::Protocol::getUpper(N), E);

        N = 13726650855680333735ULL;
        E = 3195984953;
        ASSERT_EQ(bmqp::Protocol::getUpper(N), E);
    }

    {
        // getLower
        bsls::Types::Uint64 N = 0;
        unsigned int        E = 0;
        ASSERT_EQ(bmqp::Protocol::getLower(N), E);

        N = 1ULL << 32;
        E = 0;
        ASSERT_EQ(bmqp::Protocol::getLower(N), E);

        N = k_33_BITS_MASK;
        E = k_UNSIGNED_INT_MAX;
        ASSERT_EQ(bmqp::Protocol::getLower(N), E);

        N = bsl::numeric_limits<bsls::Types::Uint64>::max();
        E = k_UNSIGNED_INT_MAX;
        ASSERT_EQ(bmqp::Protocol::getLower(N), E);

        N = 1;
        E = 1;
        ASSERT_EQ(bmqp::Protocol::getLower(N), E);

        N = 0xC6E7C7CD01FE34E8;
        E = 0x01FE34E8;
        ASSERT_EQ(bmqp::Protocol::getLower(N), E);

        N = 0x8080808080808080;
        E = 0x80808080;
        ASSERT_EQ(bmqp::Protocol::getLower(N), E);

        N = 13726650855680333735ULL;
        E = 4037236647;
        ASSERT_EQ(bmqp::Protocol::getLower(N), E);
    }

    {
        // split(unsigned int *, unsigned int *)

        unsigned int U = 0;
        unsigned int L = 0;

        bsls::Types::Uint64 N = 0;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(U, 0u);
        ASSERT_EQ(L, 0u);

        N = 1;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(U, 0u);
        ASSERT_EQ(L, 1u);

        N = 1ULL << 32;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(U, 1u);
        ASSERT_EQ(L, 0u);

        N = k_33_BITS_MASK;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(U, 1u);
        ASSERT_EQ(L, k_UNSIGNED_INT_MAX);

        N = bsl::numeric_limits<bsls::Types::Uint64>::max();
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(U, k_UNSIGNED_INT_MAX);
        ASSERT_EQ(L, k_UNSIGNED_INT_MAX);

        N = bsl::numeric_limits<bsls::Types::Uint64>::max() << 32;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(U, k_UNSIGNED_INT_MAX);
        ASSERT_EQ(L, 0u);

        N = 0xC6E7C7CD01FE34E8;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(U, 0xC6E7C7CDu);
        ASSERT_EQ(L, 0x01FE34E8u);

        N = 0x8080808080808080;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(U, 0x80808080u);
        ASSERT_EQ(L, 0x80808080u);

        N = 13726650855680333735ULL;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(U, 3195984953u);
        ASSERT_EQ(L, 4037236647u);
    }

    {
        // split(bdlb::BigEndianUint32 *, bdlb::BigEndianUint32 *)

        bdlb::BigEndianUint32 U;
        bdlb::BigEndianUint32 L;

        bsls::Types::Uint64 N = 0;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(static_cast<unsigned int>(U), 0u);
        ASSERT_EQ(static_cast<unsigned int>(L), 0u);

        N = 1;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(U, 0u);
        ASSERT_EQ(L, 1u);

        N = 1ULL << 32;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(static_cast<unsigned int>(U), 1u);
        ASSERT_EQ(static_cast<unsigned int>(L), 0u);

        N = k_33_BITS_MASK;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(static_cast<unsigned int>(U), 1u);
        ASSERT_EQ(static_cast<unsigned int>(L), k_UNSIGNED_INT_MAX);

        N = bsl::numeric_limits<bsls::Types::Uint64>::max();
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(static_cast<unsigned int>(U), k_UNSIGNED_INT_MAX);
        ASSERT_EQ(static_cast<unsigned int>(L), k_UNSIGNED_INT_MAX);

        N = bsl::numeric_limits<bsls::Types::Uint64>::max() << 32;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(static_cast<unsigned int>(U), k_UNSIGNED_INT_MAX);
        ASSERT_EQ(static_cast<unsigned int>(L), 0u);

        N = 0xC6E7C7CD01FE34E8;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(static_cast<unsigned int>(U), 0xC6E7C7CDu);
        ASSERT_EQ(static_cast<unsigned int>(L), 0x01FE34E8u);

        N = 0x8080808080808080;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(static_cast<unsigned int>(U), 0x80808080u);
        ASSERT_EQ(static_cast<unsigned int>(L), 0x80808080u);

        N = 13726650855680333735ULL;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(static_cast<unsigned int>(U), 3195984953u);
        ASSERT_EQ(static_cast<unsigned int>(L), 4037236647u);
    }

    {
        // split(bdlb::BigEndianUint16 *, bdlb::BigEndianUint32 *)

        bdlb::BigEndianUint16 U;
        bdlb::BigEndianUint32 L;

        bsls::Types::Uint64 N = 0;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(static_cast<unsigned short>(U), 0);
        ASSERT_EQ(static_cast<unsigned int>(L), 0u);

        N = 1;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(static_cast<unsigned short>(U), 0);
        ASSERT_EQ(static_cast<unsigned int>(L), 1u);

        N = 1ULL << 32;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(U, 1u);
        ASSERT_EQ(L, 0u);

        N = k_33_BITS_MASK;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(U, 1u);
        ASSERT_EQ(k_UNSIGNED_INT_MAX, L);

        N = 0xFFFFFFFFFFFF;  // 48 bits set
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(static_cast<unsigned short>(U), 0xFFFF);
        ASSERT_EQ(static_cast<unsigned int>(L), k_UNSIGNED_INT_MAX);

        N = bsl::numeric_limits<bsls::Types::Uint64>::max() << 32;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(static_cast<unsigned short>(U), 0xFFFF);
        ASSERT_EQ(static_cast<unsigned int>(L), 0u);

        N = 0xC6E7C7CD01FE34E8;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(static_cast<unsigned short>(U), 0xC7CD);
        ASSERT_EQ(static_cast<unsigned int>(L), 0x01FE34E8u);

        N = 0x8080808080808080;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(static_cast<unsigned short>(U), 0x8080);
        ASSERT_EQ(static_cast<unsigned int>(L), 0x80808080u);

        N = 13726650855680333735ULL;
        bmqp::Protocol::split(&U, &L, N);
        ASSERT_EQ(static_cast<unsigned short>(U), 56377);
        ASSERT_EQ(static_cast<unsigned int>(L), 4037236647u);
    }
}

static void test3_flagUtils()
{
    // --------------------------------------------------------------------
    // FLAG UTILS
    //
    // Concerns:
    //   FlagsUtil access, manipulation, and validation routines.
    //
    // Plan:
    //   For every type of flag for which there is a corresponding FlagsUtil
    //   utility struct, start with empty flags and do the following:
    //     1. Check that the flag is not 'isSet'.
    //     2. Set the flag. Verify that it is set, and that no other flag is
    //        set.
    //     3. Verify that, with this flag set, the flags are correctly
    //        identified as 'isValid' or not 'isValid'.
    //     4. Unset the flag and verify that it is unset.
    //
    // Testing:
    //   PutHeaderFlagUtil::setFlag
    //   PutHeaderFlagUtil::unsetFlag
    //   PutHeaderFlagUtil::isSet
    //   PutHeaderFlagUtil::isValid
    //   PushHeaderFlagUtil::setFlag
    //   PushHeaderFlagUtil::unsetFlag
    //   PushHeaderFlagUtil::isSet
    //   PushHeaderFlagUtil::isValid
    //   StorageHeaderFlagUtil::setFlag
    //   StorageHeaderFlagUtil::unsetFlag
    //   StorageHeaderFlagUtil::isSet
    //   StorageHeaderFlagUtil::isValid
    // --------------------------------------------------------------------
    mwctst::TestHelper::printTestName("FLAG UTILS");

    {
        // --------------------------------
        // PutHeaderFlagUtil Breathing Test
        // --------------------------------
        struct Test {
            int                        d_line;
            bmqp::PutHeaderFlags::Enum d_value;
            bool                       d_isValid;
        } k_DATA[] = {{L_, bmqp::PutHeaderFlags::e_ACK_REQUESTED, true},
                      {L_, bmqp::PutHeaderFlags::e_MESSAGE_PROPERTIES, true},
                      {L_, bmqp::PutHeaderFlags::e_UNUSED3, false},
                      {L_, bmqp::PutHeaderFlags::e_UNUSED4, false}};

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test& test = k_DATA[idx];

            int flags = 0;

            // 1. Check that the flag is not 'isSet'.
            ASSERT(!bmqp::PutHeaderFlagUtil::isSet(flags, test.d_value));

            // 2. Set the flag.  Verify that it is set, and that no other
            // flag is set.
            PVV(test.d_line << ": Testing: PutHeaderFlagUtil::setFlag("
                            << test.d_value << ")");

            bmqp::PutHeaderFlagUtil::setFlag(&flags, test.d_value);
            ASSERT(bmqp::PutHeaderFlagUtil::isSet(flags, test.d_value));

            mwcu::MemOutStream out(s_allocator_p);
            bmqp::PutHeaderFlagUtil::prettyPrint(out, flags);
            const bslstl::StringRef& flagsString = out.str();
            for (int currFlagVal = 1;
                 currFlagVal < (1 << bmqp::PutHeaderFlags::k_VALUE_COUNT);
                 currFlagVal = currFlagVal << 1) {
                bmqp::PutHeaderFlags::Enum currFlag =
                    static_cast<bmqp::PutHeaderFlags::Enum>(currFlagVal);

                PVV(test.d_line << ": Testing:     PutHeaderFlagUtil::isSet('"
                                << flagsString << "', " << currFlag << ")");

                if (currFlag == test.d_value) {
                    const bool expectedIsSet = (currFlag == test.d_value);
                    ASSERT_EQ_D(test.d_line,
                                bmqp::PutHeaderFlagUtil::isSet(flags,
                                                               currFlag),
                                expectedIsSet);
                }
            }

            // 3. Verify that, with this flag set, the flags are correctly
            // identified as 'isValid' or not 'isValid'.
            mwcu::MemOutStream errDesc(s_allocator_p);
            ASSERT_EQ(bmqp::PutHeaderFlagUtil::isValid(errDesc, flags),
                      test.d_isValid);

            // 4. Unset flag and verify that it is unset.
            bmqp::PutHeaderFlagUtil::unsetFlag(&flags, test.d_value);
            ASSERT(!bmqp::PutHeaderFlagUtil::isSet(flags, test.d_value));
        }
    }

    {
        // ---------------------------------
        // PushHeaderFlagUtil Breathing Test
        // ---------------------------------

        struct Test {
            int                         d_line;
            bmqp::PushHeaderFlags::Enum d_value;
            bool                        d_isValid;
        } k_DATA[] = {{L_, bmqp::PushHeaderFlags::e_IMPLICIT_PAYLOAD, true},
                      {L_, bmqp::PushHeaderFlags::e_MESSAGE_PROPERTIES, true},
                      {L_, bmqp::PushHeaderFlags::e_UNUSED3, false},
                      {L_, bmqp::PushHeaderFlags::e_UNUSED4, false}};

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test& test = k_DATA[idx];

            int flags = 0;

            // 1. Check that the flag is not 'isSet'.
            ASSERT(!bmqp::PushHeaderFlagUtil::isSet(flags, test.d_value));

            // 2. Set the flag.  Verify that it is set, and that no other
            // flag is set.
            PVV(test.d_line << ": Testing: PushHeaderFlagUtil::setFlag("
                            << test.d_value << ")");

            bmqp::PushHeaderFlagUtil::setFlag(&flags, test.d_value);
            ASSERT(bmqp::PushHeaderFlagUtil::isSet(flags, test.d_value));

            mwcu::MemOutStream out(s_allocator_p);
            bmqp::PushHeaderFlagUtil::prettyPrint(out, flags);
            const bslstl::StringRef& flagsString = out.str();
            for (int currFlagVal = 1;
                 currFlagVal < (1 << bmqp::PushHeaderFlags::k_VALUE_COUNT);
                 currFlagVal = currFlagVal << 1) {
                bmqp::PushHeaderFlags::Enum currFlag =
                    static_cast<bmqp::PushHeaderFlags::Enum>(currFlagVal);

                PVV(test.d_line << ": Testing:     PushHeaderFlagUtil::isSet('"
                                << flagsString << "', " << currFlag << ")");

                if (currFlag == test.d_value) {
                    const bool expectedIsSet = (currFlag == test.d_value);
                    ASSERT_EQ_D(test.d_line,
                                bmqp::PushHeaderFlagUtil::isSet(flags,
                                                                currFlag),
                                expectedIsSet);
                }
            }

            // 3. Verify that, with this flag set, the flags are correctly
            // identified as 'isValid' or not 'isValid'.
            mwcu::MemOutStream errDesc(s_allocator_p);
            ASSERT_EQ(bmqp::PushHeaderFlagUtil::isValid(errDesc, flags),
                      test.d_isValid);

            // 4. Unset flag and verify that it is unset.
            bmqp::PushHeaderFlagUtil::unsetFlag(&flags, test.d_value);
            ASSERT(!bmqp::PushHeaderFlagUtil::isSet(flags, test.d_value));
        }
    }

    {
        // ------------------------------------
        // StorageHeaderFlagUtil Breathing Test
        // ------------------------------------

        struct Test {
            int                            d_line;
            bmqp::StorageHeaderFlags::Enum d_value;
            bool                           d_isValid;
        } k_DATA[] = {
            {L_, bmqp::StorageHeaderFlags::e_RECEIPT_REQUESTED, true},
            {L_, bmqp::StorageHeaderFlags::e_UNUSED2, false},
            {L_, bmqp::StorageHeaderFlags::e_UNUSED3, false},
            {L_, bmqp::StorageHeaderFlags::e_UNUSED4, false}};

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test& test = k_DATA[idx];

            int flags = 0;

            // 1. Check that the flag is not 'isSet'.
            ASSERT(!bmqp::StorageHeaderFlagUtil::isSet(
                static_cast<unsigned char>(flags),
                test.d_value));

            // 2. Set the flag.  Verify that it is set, and that no other flag
            // is set.
            PVV(test.d_line << ": Testing: StorageHeaderFlagUtil::setFlag("
                            << test.d_value << ")");

            bmqp::StorageHeaderFlagUtil::setFlag(&flags, test.d_value);
            ASSERT(bmqp::StorageHeaderFlagUtil::isSet(
                static_cast<unsigned char>(flags),
                test.d_value));

            mwcu::MemOutStream out(s_allocator_p);
            bmqp::StorageHeaderFlagUtil::prettyPrint(
                out,
                static_cast<unsigned char>(flags));
            const bslstl::StringRef& flagsString = out.str();
            for (int currFlagVal = 1;
                 currFlagVal < (1 << bmqp::StorageHeaderFlags::k_VALUE_COUNT);
                 currFlagVal = currFlagVal << 1) {
                bmqp::StorageHeaderFlags::Enum currFlag =
                    static_cast<bmqp::StorageHeaderFlags::Enum>(currFlagVal);

                PVV(test.d_line << ": Testing: "
                                << "    StorageHeaderFlagUtil::isSet('"
                                << flagsString << "', " << currFlag << ")");

                if (currFlag == test.d_value) {
                    const bool expectedIsSet = (currFlag == test.d_value);
                    ASSERT_EQ_D(test.d_line,
                                bmqp::StorageHeaderFlagUtil::isSet(
                                    static_cast<unsigned char>(flags),
                                    currFlag),
                                expectedIsSet);
                }
            }

            // 3. Verify that, with this flag set, the flags are correctly
            //    identified as 'isValid' or not 'isValid'.
            mwcu::MemOutStream errDesc(s_allocator_p);
            ASSERT_EQ(bmqp::StorageHeaderFlagUtil::isValid(
                          errDesc,
                          static_cast<unsigned char>(flags)),
                      test.d_isValid);

            // 4. Unset flag and verify that it is unset.
            bmqp::StorageHeaderFlagUtil::unsetFlag(&flags, test.d_value);
            ASSERT(!bmqp::StorageHeaderFlagUtil::isSet(
                static_cast<unsigned char>(flags),
                test.d_value));
        }
    }
}

template <typename ENUM_TYPE, typename ARRAY, int SIZE>
static void printEnumHelper(ARRAY (&data)[SIZE])
{
    for (size_t idx = 0; idx < SIZE; ++idx) {
        const PrintTestData& test = data[idx];

        PVVV("Line [" << test.d_line << "]");

        mwcu::MemOutStream out(s_allocator_p);
        mwcu::MemOutStream expected(s_allocator_p);

        typedef typename ENUM_TYPE::Enum T;

        T obj = static_cast<T>(test.d_type);

        expected << test.d_expected;

        out.setstate(bsl::ios_base::badbit);
        ENUM_TYPE::print(out, obj, 0, -1);

        ASSERT_EQ(out.str(), "");

        out.clear();
        ENUM_TYPE::print(out, obj, 0, -1);

        ASSERT_EQ(out.str(), expected.str());

        out.reset();
        out << obj;

        ASSERT_EQ(out.str(), expected.str());
    }
}

static void test4_enumPrint()
{
    // --------------------------------------------------------------------
    // ENUM LAYOUT
    //
    // Concerns:
    //   Check that enums print methods work correct
    //
    // Plan:
    //   1. For every type of enum for which there is a corresponding stream
    //      operator and print method check that layout of each enum value.
    //      equal to expected value:
    //      1.1. Check layout of stream operator
    //      1.2. Check layout of print method
    //   2. Check that layout for invalid value is equal to expected.
    //
    // Testing:
    //   EventType::print
    //   EncodingType::print
    //   OptionType::print
    //   PutHeaderFlags::print
    //   PushHeaderFlags::print
    //   StorageMessageType::print
    //   StorageHeaderFlags::print
    //   StorageHeaderFlags::print
    //   operator<<(bsl::ostream&, EventType::Enum)
    //   operator<<(bsl::ostream&, EncodingType::Enum)
    //   operator<<(bsl::ostream&, OptionType::Enum)
    //   operator<<(bsl::ostream&, PutHeaderFlags::Enum)
    //   operator<<(bsl::ostream&, PushHeaderFlags::Enum)
    //   operator<<(bsl::ostream&, StorageMessageType::Enum)
    //   operator<<(bsl::ostream&, StorageHeaderFlags::Enum)
    //   operator<<(bsl::ostream&, RecoveryFileChunkType::Enum)
    // --------------------------------------------------------------------

    mwctst::TestHelper::printTestName("ENUM LAYOUT");

    PV("Test bmqp::EventType printing");
    {
        BSLMF_ASSERT(bmqp::EventType::e_CONTROL ==
                     bmqp::EventType::k_LOWEST_SUPPORTED_EVENT_TYPE);

        BSLMF_ASSERT(bmqp::EventType::e_REPLICATION_RECEIPT ==
                     bmqp::EventType::k_HIGHEST_SUPPORTED_EVENT_TYPE);

        PrintTestData k_DATA[] = {
            {L_, bmqp::EventType::e_UNDEFINED, "UNDEFINED"},
            {L_, bmqp::EventType::e_CONTROL, "CONTROL"},
            {L_, bmqp::EventType::e_PUT, "PUT"},
            {L_, bmqp::EventType::e_CONFIRM, "CONFIRM"},
            {L_, bmqp::EventType::e_PUSH, "PUSH"},
            {L_, bmqp::EventType::e_ACK, "ACK"},
            {L_, bmqp::EventType::e_CLUSTER_STATE, "CLUSTER_STATE"},
            {L_, bmqp::EventType::e_ELECTOR, "ELECTOR"},
            {L_, bmqp::EventType::e_STORAGE, "STORAGE"},
            {L_, bmqp::EventType::e_RECOVERY, "RECOVERY"},
            {L_, bmqp::EventType::e_PARTITION_SYNC, "PARTITION_SYNC"},
            {L_, bmqp::EventType::e_HEARTBEAT_REQ, "HEARTBEAT_REQ"},
            {L_, bmqp::EventType::e_HEARTBEAT_RSP, "HEARTBEAT_RSP"},
            {L_, bmqp::EventType::e_REJECT, "REJECT"},
            {L_,
             bmqp::EventType::e_REPLICATION_RECEIPT,
             "REPLICATION_RECEIPT"},
            {L_, -1, "(* UNKNOWN *)"}};

        printEnumHelper<bmqp::EventType>(k_DATA);
    }

    PV("Test bmqp::EncodingType printing");
    {
        BSLMF_ASSERT(bmqp::EncodingType::e_BER ==
                     bmqp::EncodingType::k_LOWEST_SUPPORTED_ENCODING_TYPE);

        BSLMF_ASSERT(bmqp::EncodingType::e_JSON ==
                     bmqp::EncodingType::k_HIGHEST_SUPPORTED_ENCODING_TYPE);

        PrintTestData k_DATA[] = {
            {L_, bmqp::EncodingType::e_UNKNOWN, "UNKNOWN"},
            {L_, bmqp::EncodingType::e_BER, "BER"},
            {L_, bmqp::EncodingType::e_JSON, "JSON"},
            {L_, -2, "(* UNKNOWN *)"}};

        printEnumHelper<bmqp::EncodingType>(k_DATA);
    }

    PV("Test bmqp::OptionType printing");
    {
        BSLMF_ASSERT(bmqp::OptionType::k_LOWEST_SUPPORTED_TYPE ==
                     bmqp::OptionType::e_SUB_QUEUE_IDS_OLD);

        BSLMF_ASSERT(bmqp::OptionType::k_HIGHEST_SUPPORTED_TYPE ==
                     bmqp::OptionType::e_SUB_QUEUE_INFOS);

        PrintTestData k_DATA[] = {
            {L_, bmqp::OptionType::e_UNDEFINED, "UNDEFINED"},
            {L_, bmqp::OptionType::e_SUB_QUEUE_IDS_OLD, "SUB_QUEUE_IDS_OLD"},
            {L_, bmqp::OptionType::e_MSG_GROUP_ID, "MSG_GROUP_ID"},
            {L_, bmqp::OptionType::e_SUB_QUEUE_INFOS, "SUB_QUEUE_INFOS"},
            {L_, -1, "(* UNKNOWN *)"}};

        printEnumHelper<bmqp::OptionType>(k_DATA);
    }

    PV("Test bmqp::PutHeaderFlags printing");
    {
        BSLMF_ASSERT(bmqp::PutHeaderFlags::k_LOWEST_SUPPORTED_PUT_FLAG ==
                     bmqp::PutHeaderFlags::e_ACK_REQUESTED);

        BSLMF_ASSERT(bmqp::PutHeaderFlags::k_HIGHEST_SUPPORTED_PUT_FLAG ==
                     bmqp::PutHeaderFlags::e_MESSAGE_PROPERTIES);

        BSLMF_ASSERT(bmqp::PutHeaderFlags::k_HIGHEST_PUT_FLAG ==
                     bmqp::PutHeaderFlags::e_UNUSED4);

        PrintTestData k_DATA[] = {
            {L_, bmqp::PutHeaderFlags::e_ACK_REQUESTED, "ACK_REQUESTED"},
            {L_,
             bmqp::PutHeaderFlags::e_MESSAGE_PROPERTIES,
             "MESSAGE_PROPERTIES"},
            {L_, bmqp::PutHeaderFlags::e_UNUSED3, "UNUSED3"},
            {L_, bmqp::PutHeaderFlags::e_UNUSED4, "UNUSED4"},
            {L_, -1, "(* UNKNOWN *)"}};

        printEnumHelper<bmqp::PutHeaderFlags>(k_DATA);
    }

    PV("Test bmqp::PushHeaderFlags printing");
    {
        BSLMF_ASSERT(bmqp::PushHeaderFlags::k_LOWEST_SUPPORTED_PUSH_FLAG ==
                     bmqp::PushHeaderFlags::e_IMPLICIT_PAYLOAD);

        BSLMF_ASSERT(bmqp::PushHeaderFlags::k_HIGHEST_SUPPORTED_PUSH_FLAG ==
                     bmqp::PushHeaderFlags::e_MESSAGE_PROPERTIES);

        BSLMF_ASSERT(bmqp::PushHeaderFlags::k_HIGHEST_PUSH_FLAG ==
                     bmqp::PushHeaderFlags::e_UNUSED4);

        PrintTestData k_DATA[] = {
            {L_,
             bmqp::PushHeaderFlags::e_IMPLICIT_PAYLOAD,
             "IMPLICIT_PAYLOAD"},
            {L_,
             bmqp::PushHeaderFlags::e_MESSAGE_PROPERTIES,
             "MESSAGE_PROPERTIES"},
            {L_, bmqp::PushHeaderFlags::e_UNUSED3, "UNUSED3"},
            {L_, bmqp::PushHeaderFlags::e_UNUSED4, "UNUSED4"},
            {L_, -1, "(* UNKNOWN *)"}};

        printEnumHelper<bmqp::PushHeaderFlags>(k_DATA);
    }

    PV("Test bmqp::StorageMessageType printing");
    {
        BSLMF_ASSERT(
            bmqp::StorageMessageType::k_LOWEST_SUPPORTED_STORAGE_MSG_TYPE ==
            bmqp::StorageMessageType::e_DATA);

        BSLMF_ASSERT(
            bmqp::StorageMessageType::k_HIGHEST_SUPPORTED_STORAGE_MSG_TYPE ==
            bmqp::StorageMessageType::e_QUEUE_OP);

        PrintTestData k_DATA[] = {
            {L_, bmqp::StorageMessageType::e_UNDEFINED, "UNDEFINED"},
            {L_, bmqp::StorageMessageType::e_DATA, "DATA"},
            {L_, bmqp::StorageMessageType::e_QLIST, "QLIST"},
            {L_, bmqp::StorageMessageType::e_CONFIRM, "CONFIRM"},
            {L_, bmqp::StorageMessageType::e_DELETION, "DELETION"},
            {L_, bmqp::StorageMessageType::e_JOURNAL_OP, "JOURNAL_OP"},
            {L_, bmqp::StorageMessageType::e_QUEUE_OP, "QUEUE_OP"},
            {L_, -1, "(* UNKNOWN *)"}};

        printEnumHelper<bmqp::StorageMessageType>(k_DATA);
    }

    PV("Test bmqp::StorageHeaderFlags printing");
    {
        BSLMF_ASSERT(
            bmqp::StorageHeaderFlags::k_LOWEST_SUPPORTED_STORAGE_FLAG ==
            bmqp::StorageHeaderFlags::e_RECEIPT_REQUESTED);

        BSLMF_ASSERT(
            bmqp::StorageHeaderFlags::k_HIGHEST_SUPPORTED_STORAGE_FLAG ==
            bmqp::StorageHeaderFlags::e_RECEIPT_REQUESTED);

        BSLMF_ASSERT(bmqp::StorageHeaderFlags::k_HIGHEST_STORAGE_FLAG ==
                     bmqp::StorageHeaderFlags::e_UNUSED4);

        PrintTestData k_DATA[] = {
            {L_,
             bmqp::StorageHeaderFlags::e_RECEIPT_REQUESTED,
             "RECEIPT_REQUESTED"},
            {L_, bmqp::StorageHeaderFlags::e_UNUSED2, "UNUSED2"},
            {L_, bmqp::StorageHeaderFlags::e_UNUSED3, "UNUSED3"},
            {L_, bmqp::StorageHeaderFlags::e_UNUSED4, "UNUSED4"},
            {L_, -1, "(* UNKNOWN *)"}};

        printEnumHelper<bmqp::StorageHeaderFlags>(k_DATA);
    }

    PV("Test bmqp::RecoveryFileChunkType printing");
    {
        BSLMF_ASSERT(bmqp::RecoveryFileChunkType::
                         k_LOWEST_SUPPORTED_RECOVERY_CHUNK_TYPE ==
                     bmqp::RecoveryFileChunkType::e_DATA);

        BSLMF_ASSERT(bmqp::RecoveryFileChunkType::
                         k_HIGHEST_SUPPORTED_RECOVERY_CHUNK_TYPE ==
                     bmqp::RecoveryFileChunkType::e_QLIST);

        PrintTestData k_DATA[] = {
            {L_, bmqp::RecoveryFileChunkType::e_UNDEFINED, "UNDEFINED"},
            {L_, bmqp::RecoveryFileChunkType::e_DATA, "DATA"},
            {L_, bmqp::RecoveryFileChunkType::e_JOURNAL, "JOURNAL"},
            {L_, bmqp::RecoveryFileChunkType::e_QLIST, "QLIST"},
            {L_, -1, "(* UNKNOWN *)"}};

        printEnumHelper<bmqp::RecoveryFileChunkType>(k_DATA);
    }
}

static void test5_enumIsomorphism()
{
    // --------------------------------------------------------------------
    // ENUM LAYOUT
    //
    // Concerns:
    //   Check isomorphism of toAscii, fromAscii methods:
    //     1. x = toAscii(fromAscii(x)), where x is valid string enum
    //            representation
    //     2. x = fromAscii(toAscii(x)), where x is valid enum value
    //
    // Plan:
    //   For every type of enum for which there is a corresponding toAscii and
    //   fromAscii methods are defined check isomorphism of given methods.
    //
    // Testing:
    //   PutHeaderFlags::toAscii
    //   PushHeaderFlags::toAscii
    //   StorageHeaderFlags::toAscii
    //   PutHeaderFlags::fromAscii
    //   PushHeaderFlags::fromAscii
    //   StorageHeaderFlags::fromAscii
    // --------------------------------------------------------------------
#define TEST_ISOMORPHISM(ENUM_TYPE, ENUM_VAL)                                 \
    {                                                                         \
        PV("Check isomorphism for " #ENUM_VAL " value of " #ENUM_TYPE         \
           " enum");                                                          \
        typedef bmqp::ENUM_TYPE T;                                            \
        mwcu::MemOutStream      stream(s_allocator_p);                        \
                                                                              \
        T::Enum     obj;                                                      \
        const char* str;                                                      \
        bool        res;                                                      \
        res = T::fromAscii(&obj, "IMPOSSIBLE VALUE");                         \
        ASSERT_EQ(res, false);                                                \
        res = T::fromAscii(&obj, #ENUM_VAL);                                  \
        ASSERT_EQ(res, true);                                                 \
        ASSERT_EQ(obj, T::e_##ENUM_VAL);                                      \
        str = T::toAscii(obj);                                                \
        ASSERT_EQ(bsl::strncmp(str, #ENUM_VAL, sizeof(#ENUM_VAL)), 0);        \
    }

    mwctst::TestHelper::printTestName("ENUM ISOMORPHISM");

    // Enum PutHeaderFlags
    TEST_ISOMORPHISM(PutHeaderFlags, ACK_REQUESTED)
    TEST_ISOMORPHISM(PutHeaderFlags, MESSAGE_PROPERTIES)
    TEST_ISOMORPHISM(PutHeaderFlags, UNUSED3)
    TEST_ISOMORPHISM(PutHeaderFlags, UNUSED4)

    // Enum PushHeaderFlags
    TEST_ISOMORPHISM(PushHeaderFlags, IMPLICIT_PAYLOAD)
    TEST_ISOMORPHISM(PushHeaderFlags, MESSAGE_PROPERTIES)
    TEST_ISOMORPHISM(PushHeaderFlags, UNUSED3)
    TEST_ISOMORPHISM(PushHeaderFlags, UNUSED4)

    // Enum StorageHeaderFlags
    TEST_ISOMORPHISM(StorageHeaderFlags, RECEIPT_REQUESTED)
    TEST_ISOMORPHISM(StorageHeaderFlags, UNUSED2)
    TEST_ISOMORPHISM(StorageHeaderFlags, UNUSED3)
    TEST_ISOMORPHISM(StorageHeaderFlags, UNUSED4)

#undef TEST_ISOMORPHISM
}

template <typename ENUM_UTIL_TYPE, typename FLAG_TYPE>
static void enumFromStringHelper(FLAG_TYPE          expectedFlags,
                                 const bsl::string& corrStr,
                                 const bsl::string& incorrStr,
                                 const bsl::string& errOutput)
{
    mwcu::MemOutStream errStream(s_allocator_p);
    FLAG_TYPE          outFlags;
    int                rc;
    rc = ENUM_UTIL_TYPE::fromString(errStream, &outFlags, corrStr);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(outFlags, expectedFlags);
    rc = ENUM_UTIL_TYPE::fromString(errStream, &outFlags, incorrStr);
    ASSERT_EQ(rc, -1);
    ASSERT_EQ(errStream.str(), errOutput);
}

static void test6_enumFromString()
{
    // --------------------------------------------------------------------
    // ENUM UTIL fromString
    //
    // Concerns:
    //   Check that fromString method for various flag utils correctly parse
    //   string representation of flags.
    //
    // Plan:
    //   For every type of util for which there is a corresponding fromString
    //   method is defined check that:
    //     1. flags output is correct and reflect flags enumeration in input
    //        string representation.
    //     2. in case if input string contains wrong flags, error output should
    //        contain all wrong flags, return code should reflect error.
    //
    // Testing:
    //   PushHeaderFlagUtil::fromString
    //   StorageHeaderFlagUtil::fromString
    //   StorageHeaderFlagUtil::fromString
    // --------------------------------------------------------------------

    mwctst::TestHelper::printTestName("ENUM UTIL toString");

    // Util PushHeaderFlagUtil
    PV("Testing PushHeaderFlagUtil toString method");
    int expectedFlags = bmqp::PushHeaderFlags::e_IMPLICIT_PAYLOAD |
                        bmqp::PushHeaderFlags::e_MESSAGE_PROPERTIES |
                        bmqp::PushHeaderFlags::e_UNUSED3 |
                        bmqp::PushHeaderFlags::e_UNUSED4;
    bsl::string corrStr("IMPLICIT_PAYLOAD,MESSAGE_PROPERTIES,UNUSED3,UNUSED4",
                        s_allocator_p);
    bsl::string incorrStr(
        "IMPLICIT_PAYLOAD,MESSAGE_PROPERTIES,UNUSED3,INVLD1,INVLD2",
        s_allocator_p);
    bsl::string errOutput("Invalid flag(s) 'INVLD1','INVLD2'", s_allocator_p);
    enumFromStringHelper<bmqp::PushHeaderFlagUtil>(expectedFlags,
                                                   corrStr,
                                                   incorrStr,
                                                   errOutput);

    // Util StorageHeaderFlagUtil
    PV("Testing StorageHeaderFlagUtil fromString method");
    expectedFlags = bmqp::PutHeaderFlags::e_ACK_REQUESTED |
                    bmqp::PutHeaderFlags::e_MESSAGE_PROPERTIES |
                    bmqp::PutHeaderFlags::e_UNUSED3 |
                    bmqp::PutHeaderFlags::e_UNUSED4;
    corrStr   = "ACK_REQUESTED,MESSAGE_PROPERTIES,UNUSED3,UNUSED4";
    incorrStr = "ACK_REQUESTED,MESSAGE_PROPERTIES,UNUSED3,INVLD1,INVLD2";
    enumFromStringHelper<bmqp::PutHeaderFlagUtil>(expectedFlags,
                                                  corrStr,
                                                  incorrStr,
                                                  errOutput);

    // Util StorageHeaderFlagUtil
    PV("Testing StorageHeaderFlagUtil fromString method");
    expectedFlags = bmqp::StorageHeaderFlags::e_RECEIPT_REQUESTED |
                    bmqp::StorageHeaderFlags::e_UNUSED2 |
                    bmqp::StorageHeaderFlags::e_UNUSED3 |
                    bmqp::StorageHeaderFlags::e_UNUSED4;
    corrStr   = "RECEIPT_REQUESTED,UNUSED2,UNUSED3,UNUSED4";
    incorrStr = "RECEIPT_REQUESTED,UNUSED2,UNUSED3,INVLD1,INVLD2";
    unsigned char charExpectedFlags = static_cast<unsigned char>(
        expectedFlags);
    enumFromStringHelper<bmqp::StorageHeaderFlagUtil>(charExpectedFlags,
                                                      corrStr,
                                                      incorrStr,
                                                      errOutput);
}

static void test7_eventHeaderUtil()
// --------------------------------------------------------------------
// EVENT HEADER UTIL
//
// Concerns:
//   EventHeaderUtil access and manipulation routines.
//
// Plan:
//   For every type-specific option or flag which can be accessed or
//   manipulated by the EventHeaderUtil utility struct, start with empty
//   bits and do the following:
//     1. Set the option or flag
//     2. Verify that the intended option or flag is set
//
// Testing:
//   EventHeaderUtil::setControlEventEncodingType
//   EventHeaderUtil::controlEventEncodingType
// --------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("EVENT HEADER UTIL");

    PV("Test bmqp::EventHeaderUtil setControlEventEncodingType");
    {
        struct Test {
            int                      d_line;
            bmqp::EncodingType::Enum d_value;
        } k_DATA[] = {
            {L_, bmqp::EncodingType::e_BER},
            {L_, bmqp::EncodingType::e_JSON},
        };

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        bmqp::EventHeader eventHeader(bmqp::EventType::e_CONTROL);
        // Set each encoding type in succession, and ensure that the encoding
        // returned is always the one last set
        for (size_t idx = 0; idx != k_NUM_DATA; ++idx) {
            const Test& test = k_DATA[idx];

            // 1. Set the encoding type
            PVV(test.d_line
                << ": Testing: EventHeaderUtil::setControlEventEncodingType("
                << test.d_value << ")");
            bmqp::EventHeaderUtil::setControlEventEncodingType(&eventHeader,
                                                               test.d_value);

            // 2. Verify that the intended encoding type is set
            ASSERT_EQ(
                test.d_value,
                bmqp::EventHeaderUtil::controlEventEncodingType(eventHeader));
        }
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
    case 7: test7_eventHeaderUtil(); break;
    case 6: test6_enumFromString(); break;
    case 5: test5_enumIsomorphism(); break;
    case 4: test4_enumPrint(); break;
    case 3: test3_flagUtils(); break;
    case 2: test2_bitManipulation(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
