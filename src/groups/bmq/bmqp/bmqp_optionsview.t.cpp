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

// bmqp_optionsview.t.cpp                                             -*-C++-*-
#include <bmqp_optionsview.h>

// BMQ
#include <bmqp_optionutil.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqp_queueid.h>
#include <bmqu_memoutstream.h>

#include <bmqu_blobobjectproxy.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// BDE
#include <bdlb_bigendian.h>
#include <bdlb_nullablevalue.h>
#include <bdlb_random.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_limits.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Verify that the specified `view` is valid and contains the specified
/// `subQueueInfos`.
void verifySubQueueInfos(const bmqp::OptionsView&               view,
                         const bsl::vector<bmqp::SubQueueInfo>& subQueueInfos)
{
    BMQTST_ASSERT(view.isValid());
    BMQTST_ASSERT(view.find(bmqp::OptionType::e_SUB_QUEUE_INFOS) !=
                  view.end());

    bmqp::Protocol::SubQueueInfosArray retrievedSubQueueInfos(
        bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(0, view.loadSubQueueInfosOption(&retrievedSubQueueInfos));
    BMQTST_ASSERT_EQ(retrievedSubQueueInfos.size(), subQueueInfos.size());
    for (size_t i = 0; i < subQueueInfos.size(); ++i) {
        BMQTST_ASSERT_EQ_D(i, retrievedSubQueueInfos[i], subQueueInfos[i]);
    }
}

/// Verify that the specified `view` is valid and contains the specified
/// `subQueueIdsOld`.  Load that the `subQueueIdsOld` will be loaded as
/// both Protocol::SubQueueInfosArray and Protocol::SubQueueIdsArrayOld to
/// ensure that it can be decoded as both.
void verifySubQueueIdsOld(
    const bmqp::OptionsView&                  view,
    const bsl::vector<bdlb::BigEndianUint32>& subQueueIdsOld)
{
    BMQTST_ASSERT(view.isValid());
    BMQTST_ASSERT(view.find(bmqp::OptionType::e_SUB_QUEUE_IDS_OLD) !=
                  view.end());

    const size_t numSubQueueIds = subQueueIdsOld.size();

    bmqp::Protocol::SubQueueInfosArray retrievedSubQueueInfos(
        bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(0, view.loadSubQueueInfosOption(&retrievedSubQueueInfos));
    BMQTST_ASSERT_EQ(numSubQueueIds, retrievedSubQueueInfos.size());
    for (size_t i = 0; i < numSubQueueIds; ++i) {
        BMQTST_ASSERT_EQ_D(i,
                           subQueueIdsOld[i],
                           retrievedSubQueueInfos[i].id());
        BMQTST_ASSERT_EQ_D(i,
                           true,
                           retrievedSubQueueInfos[i].rdaInfo().isUnlimited());
    }

    bmqp::Protocol::SubQueueIdsArrayOld retrievedSubQueueIdsOld(
        bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(0, view.loadSubQueueIdsOption(&retrievedSubQueueIdsOld));
    BMQTST_ASSERT_EQ(numSubQueueIds, retrievedSubQueueIdsOld.size());
    for (size_t i = 0; i < numSubQueueIds; ++i) {
        BMQTST_ASSERT_EQ_D(i, subQueueIdsOld[i], retrievedSubQueueIdsOld[i]);
    }
}

/// Populate the specified `subQueueInfos` with the specified
/// `numSubQueueInfos` number of randomly generated pairs of sub-queue id
/// and RDA counter.  If `numSubQueueInfos` equals 1, use default sub-queue
/// id instead.  Note that `subQueueInfos` will be cleared prior to being
/// populated.
void generateSubQueueInfos(bsl::vector<bmqp::SubQueueInfo>* subQueueInfos,
                           int                              numSubQueueInfos)
{
    BSLS_ASSERT_SAFE(subQueueInfos);
    BSLS_ASSERT_SAFE(numSubQueueInfos >= 0);

    subQueueInfos->clear();

    int seed  = bsl::numeric_limits<int>::max();
    int seed2 = bsl::numeric_limits<int>::max() / 2;

    for (int i = 0; i < numSubQueueInfos; ++i) {
        bmqp::SubQueueInfo info;
        if (numSubQueueInfos == 1) {
            info.setId(bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);
        }
        else {
            info.setId(
                bdlb::BigEndianUint32::make(bdlb::Random::generate15(&seed)));
        }
        info.rdaInfo().setCounter(static_cast<unsigned char>(
            bdlb::Random::generate15(&seed2) %
            (bmqp::RdaInfo::k_MAX_COUNTER_VALUE + 1)));
        subQueueInfos->push_back(info);
    }

    BSLS_ASSERT_SAFE(subQueueInfos->size() ==
                     static_cast<unsigned int>(numSubQueueInfos));
}

/// Populate the specified `subQueueIdsOld` with the specified
/// `numSubQueueIds` number of randomly generated SubQueueIds.  Note that
/// `subQueueIdsOld` will be cleared prior to being populated.
void generateSubQueueIdsOld(bsl::vector<bdlb::BigEndianUint32>* subQueueIdsOld,
                            int                                 numSubQueueIds)
{
    BSLS_ASSERT_SAFE(subQueueIdsOld);
    BSLS_ASSERT_SAFE(numSubQueueIds >= 0);

    subQueueIdsOld->clear();

    int seed = bsl::numeric_limits<int>::max();

    for (int i = 0; i < numSubQueueIds; ++i) {
        bdlb::BigEndianUint32 curr = bdlb::BigEndianUint32::make(
            bdlb::Random::generate15(&seed));
        subQueueIdsOld->push_back(curr);
    }

    BSLS_ASSERT_SAFE(subQueueIdsOld->size() ==
                     static_cast<unsigned int>(numSubQueueIds));
}

typedef bdlb::NullableValue<bmqp::Protocol::MsgGroupId> NullableMsgGroupId;

/// Populate the specified `msgGroupId` with a random Group Id.
static void generateMsgGroupId(bmqp::Protocol::MsgGroupId* msgGroupId)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(msgGroupId);

    int seed = bsl::numeric_limits<int>::max();

    bmqu::MemOutStream oss(bmqtst::TestHelperUtil::allocator());
    oss << "gid:" << bdlb::Random::generate15(&seed);
    *msgGroupId = oss.str();
}

/// Populate the specified `blob` with a PUSH event consisting of an event
/// header, a push header, an options area containing an options header
/// followed by the specified `numSubQueueInfos` number of randomly
/// generated sub-queue infos, and a payload.  Load the generated sub-queue
/// infos into the specified `subQueueInfos`.  If the specified
/// `useSubQueueIdsOld` is true, the generated sub-queue infos will be of
/// the old version and they will be loaded into the specified
/// `subQueueIdsOld` instead of `subQueueInfos`.  If the specified
/// `hasMsgGroupId` is true, append a message Id to the options area and
/// load it into the specified `msgGroupId`.  Load into the specified
/// `optionsAreaPosition` the position of the options area (or sets it to
/// default value if there is no option payload), and loads into the
/// specified `optionsAreaSize` the size of the options area in bytes.
void populateBlob(bdlbb::Blob*                        blob,
                  bmqu::BlobPosition*                 optionsAreaPosition,
                  int*                                optionsAreaSize,
                  bsl::vector<bmqp::SubQueueInfo>*    subQueueInfos,
                  bsl::vector<bdlb::BigEndianUint32>* subQueueIdsOld,
                  bool                                useSubQueueIdsOld,
                  size_t                              numSubQueueInfos,
                  NullableMsgGroupId*                 msgGroupId,
                  bool                                hasMsgGroupId)
{
    BSLS_ASSERT_SAFE(blob);
    BSLS_ASSERT_SAFE(optionsAreaPosition);
    BSLS_ASSERT_SAFE(optionsAreaSize);
    BSLS_ASSERT_SAFE((useSubQueueIdsOld && subQueueIdsOld) ||
                     (!useSubQueueIdsOld && subQueueInfos));
    BSLS_ASSERT_SAFE(msgGroupId);
    BSLS_ASSERT_SAFE(msgGroupId->isNull());

    // OptionHeader
    int    optionsWords             = 0;
    int    subQueueInfosOptionWords = 0;
    size_t itemSize                 = 0;

    const bool hasSubQueues = (numSubQueueInfos > 0);
    if (hasSubQueues) {
        itemSize = useSubQueueIdsOld ? sizeof(bdlb::BigEndianUint32)
                                     : sizeof(bmqp::SubQueueInfo);

        subQueueInfosOptionWords = (sizeof(bmqp::OptionHeader) +
                                    numSubQueueInfos * itemSize) /
                                   bmqp::Protocol::k_WORD_SIZE;
        optionsWords += subQueueInfosOptionWords;
    }

    if (hasMsgGroupId) {
        generateMsgGroupId(&msgGroupId->makeValue());
    }
    typedef bmqp::OptionUtil::OptionMeta OptionMeta;
    const OptionMeta                     msgGroupIdOption =
        hasMsgGroupId ? OptionMeta::forOptionWithPadding(
                            bmqp::OptionType::e_MSG_GROUP_ID,
                            msgGroupId->value().length())
                                          : OptionMeta::forNullOption();
    if (hasMsgGroupId) {
        optionsWords += msgGroupIdOption.size() / bmqp::Protocol::k_WORD_SIZE;
    }

    *optionsAreaSize = optionsWords * bmqp::Protocol::k_WORD_SIZE;

    // PushHeader
    bmqp::PushHeader        ph;
    const int               queueId = 123;
    const bmqt::MessageGUID guid;

    ph.setFlags(bmqp::PushHeaderFlags::e_OUT_OF_ORDER)
        .setOptionsWords(optionsWords)
        .setHeaderWords(sizeof(bmqp::PushHeader) / bmqp::Protocol::k_WORD_SIZE)
        .setQueueId(queueId)
        .setMessageGUID(guid);
    ph.setMessageWords(ph.optionsWords() + ph.headerWords());

    const int eventLength = sizeof(bmqp::EventHeader) +
                            ph.messageWords() * bmqp::Protocol::k_WORD_SIZE;

    // Append EventHeader
    bmqp::EventHeader eh;
    eh.setLength(eventLength)
        .setType(bmqp::EventType::e_PUSH)
        .setHeaderWords(sizeof(bmqp::EventHeader) /
                        bmqp::Protocol::k_WORD_SIZE);

    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(&eh),
                            eh.headerWords() * bmqp::Protocol::k_WORD_SIZE);

    // Append PushHeader
    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(&ph),
                            ph.headerWords() * bmqp::Protocol::k_WORD_SIZE);

    if (hasSubQueues || hasMsgGroupId) {
        // Capture options area position (Position of 1st OptionHeader)
        bmqu::BlobUtil::reserve(optionsAreaPosition,
                                blob,
                                sizeof(bmqp::OptionHeader));
    }
    else {
        // We don't write an option header when there is no option to write
        *optionsAreaPosition = bmqu::BlobPosition();
    }

    if (hasSubQueues) {
        bmqp::OptionHeader oh;
        oh.setType(useSubQueueIdsOld ? bmqp::OptionType::e_SUB_QUEUE_IDS_OLD
                                     : bmqp::OptionType::e_SUB_QUEUE_INFOS)
            .setWords(subQueueInfosOptionWords);
        if (!useSubQueueIdsOld) {
            oh.setTypeSpecific(sizeof(bmqp::SubQueueInfo) /
                               bmqp::Protocol::k_WORD_SIZE);
        }

        // Append OptionHeader
        bmqu::BlobUtil::writeBytes(blob,
                                   *optionsAreaPosition,
                                   reinterpret_cast<const char*>(&oh),
                                   sizeof(bmqp::OptionHeader));

        if (useSubQueueIdsOld) {
            generateSubQueueIdsOld(subQueueIdsOld, numSubQueueInfos);
            bdlbb::BlobUtil::append(
                blob,
                reinterpret_cast<const char*>(subQueueIdsOld->data()),
                numSubQueueInfos * itemSize);
        }
        else {
            generateSubQueueInfos(subQueueInfos, numSubQueueInfos);
            bdlbb::BlobUtil::append(
                blob,
                reinterpret_cast<const char*>(subQueueInfos->data()),
                numSubQueueInfos * itemSize);
        }
    }
    if (hasMsgGroupId) {
        bmqp::OptionUtil::OptionsBox options;
        options.add(blob, msgGroupId->value().data(), msgGroupIdOption);
    }
}

/// Populate the specified `blob` with a PUSH event consisting of an event
/// header, a push header, and a *packed* option header containing a random
/// generated RDA counter for the default sub-queue id, with no option
/// payload.  Load the default sub-queue id and the RDA counter into the
/// specified `subQueueInfos`.  Load into the specified
/// `optionsAreaPosition` the position of the options area, and load into
/// the specified `optionsAreaSize` the size of the options area in bytes.
void populatePackedBlob(bdlbb::Blob*                     blob,
                        bmqu::BlobPosition*              optionsAreaPosition,
                        int*                             optionsAreaSize,
                        bsl::vector<bmqp::SubQueueInfo>* subQueueInfos)
{
    BSLS_ASSERT_SAFE(blob);
    BSLS_ASSERT_SAFE(optionsAreaPosition);
    BSLS_ASSERT_SAFE(optionsAreaSize);
    BSLS_ASSERT_SAFE(subQueueInfos);

    // OptionHeader
    *optionsAreaSize       = sizeof(bmqp::OptionHeader);
    const int optionsWords = *optionsAreaSize / bmqp::Protocol::k_WORD_SIZE;

    // PushHeader
    bmqp::PushHeader        ph;
    const int               queueId = 123;
    const bmqt::MessageGUID guid;

    ph.setFlags(bmqp::PushHeaderFlags::e_OUT_OF_ORDER)
        .setOptionsWords(optionsWords)
        .setHeaderWords(sizeof(bmqp::PushHeader) / bmqp::Protocol::k_WORD_SIZE)
        .setQueueId(queueId)
        .setMessageGUID(guid)
        .setMessageWords(ph.optionsWords() + ph.headerWords());

    const int eventLength = sizeof(bmqp::EventHeader) +
                            ph.messageWords() * bmqp::Protocol::k_WORD_SIZE;

    // Append EventHeader
    bmqp::EventHeader eh;
    eh.setLength(eventLength)
        .setType(bmqp::EventType::e_PUSH)
        .setHeaderWords(sizeof(bmqp::EventHeader) /
                        bmqp::Protocol::k_WORD_SIZE);

    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(&eh),
                            eh.headerWords() * bmqp::Protocol::k_WORD_SIZE);

    // Append PushHeader
    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(&ph),
                            ph.headerWords() * bmqp::Protocol::k_WORD_SIZE);

    // Append *packed* OptionHeader
    bmqu::BlobUtil::reserve(optionsAreaPosition,
                            blob,
                            sizeof(bmqp::OptionHeader));

    bmqp::OptionHeader oh;
    oh.setType(bmqp::OptionType::e_SUB_QUEUE_INFOS).setPacked(true);

    generateSubQueueInfos(subQueueInfos, 1);
    // Reinterpret 'words' field as RDA counter
    oh.setWords(subQueueInfos->front().rdaInfo().internalRepresentation());

    bmqu::BlobUtil::writeBytes(blob,
                               *optionsAreaPosition,
                               reinterpret_cast<const char*>(&oh),
                               sizeof(bmqp::OptionHeader));
}

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the component.
//
// Plan:
//   - Default constructor (invalid instance)
//   - Copy constructor with invalid instance (invalid instance)
//   - Basic functionality with zero options
//   - Basic functionality with one option header and one option value
//   - Basic functionality with one option header and three option values
//   - Basic functionality with one option header and three option values
//     including group id
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());

    PV(endl << "BREATHING TEST" << endl << "==============" << endl);

    {
        // [1]
        PV("DEFAULT CONSTRUCTOR: INVALID INSTANCE");

        // Create invalid instance
        bmqp::OptionsView view(bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(false, view.isValid());
    }

    {
        // [2]
        PV("COPY CONSTRUCTOR: INVALID INSTANCE");

        // Create invalid instance from another invalid instance
        bmqp::OptionsView view1(bmqtst::TestHelperUtil::allocator());
        bmqp::OptionsView view2(view1, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(false, view2.isValid());
    }

    {
        // [3]
        PV("BASIC FUNCTIONALITY: ZERO OPTIONS");

        // Create valid view, PUSH event with 1 message, zero options
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        bmqu::BlobPosition optionsAreaPosition;
        int                optionsAreaSize = 0;

        // Populate blob, 0 options
        bsl::vector<bmqp::SubQueueInfo> subQueueInfos(
            bmqtst::TestHelperUtil::allocator());
        size_t                          numSubQueueIds = 0;
        NullableMsgGroupId msgGroupId(bmqtst::TestHelperUtil::allocator());
        bool                            hasMsgGroupId = false;

        populateBlob(&blob,
                     &optionsAreaPosition,
                     &optionsAreaSize,
                     &subQueueInfos,
                     0,
                     false,
                     numSubQueueIds,
                     &msgGroupId,
                     hasMsgGroupId);

        // Iterate and verify
        bmqp::OptionsView view(&blob,
                               optionsAreaPosition,
                               optionsAreaSize,
                               bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(true, view.isValid());
        BMQTST_ASSERT_EQ(true,
                         view.find(bmqp::OptionType::e_SUB_QUEUE_INFOS) ==
                             view.end());

        // Copy valid instance
        bmqp::OptionsView view2(view, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(true, view2.isValid());
        BMQTST_ASSERT_EQ(true,
                         view2.find(bmqp::OptionType::e_SUB_QUEUE_INFOS) ==
                             view2.end());

        // Clear original
        view.clear();
        BMQTST_ASSERT_EQ(false, view.isValid());

        // Verify second instance remains valid
        BMQTST_ASSERT_EQ(true, view2.isValid());
        BMQTST_ASSERT_EQ(true,
                         view2.find(bmqp::OptionType::e_SUB_QUEUE_INFOS) ==
                             view2.end());

        // Copy invalid instance (original)
        bmqp::OptionsView view3(view, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(false, view3.isValid());

        // Reset original, iterate and verify again
        BMQTST_ASSERT_EQ(
            0,
            view.reset(&blob, optionsAreaPosition, optionsAreaSize));

        BMQTST_ASSERT_EQ(true, view.isValid());
        BMQTST_ASSERT_EQ(true,
                         view.find(bmqp::OptionType::e_SUB_QUEUE_INFOS) ==
                             view.end());
    }

    {
        // [4]
        PV("BASIC FUNCTIONALITY: ONE OPTION HEADER, ONE OPTION VALUE");

        // Create valid view, PUSH event with 1 message, 1 option header and
        // 1 sub-queue id
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        bmqu::BlobPosition optionsAreaPosition;
        int                optionsAreaSize = 0;

        // Populate blob, 1 option header and 1 sub-queue id
        bsl::vector<bmqp::SubQueueInfo> subQueueInfos(
            bmqtst::TestHelperUtil::allocator());
        size_t                          numSubQueueIds = 1;
        NullableMsgGroupId msgGroupId(bmqtst::TestHelperUtil::allocator());
        bool                            hasMsgGroupId = false;

        populateBlob(&blob,
                     &optionsAreaPosition,
                     &optionsAreaSize,
                     &subQueueInfos,
                     0,
                     false,
                     numSubQueueIds,
                     &msgGroupId,
                     hasMsgGroupId);

        // Sanity test: First verify that populateBlob gave us back valid
        //              parameters to pass to OptionsView
        bmqu::BlobObjectProxy<bmqp::OptionHeader> oh(&blob,
                                                     optionsAreaPosition,
                                                     optionsAreaSize);

        const unsigned int itemSize = oh->typeSpecific();
        BMQTST_ASSERT_EQ(numSubQueueIds * itemSize + 1,
                         static_cast<size_t>(oh->words()));

        BMQTST_ASSERT_EQ(bmqp::OptionType::e_SUB_QUEUE_INFOS, oh->type());

        // Iterate and verify
        bmqp::OptionsView view(&blob,
                               optionsAreaPosition,
                               optionsAreaSize,
                               bmqtst::TestHelperUtil::allocator());
        verifySubQueueInfos(view, subQueueInfos);

        // Copy valid instance
        bmqp::OptionsView view2(view, bmqtst::TestHelperUtil::allocator());
        verifySubQueueInfos(view2, subQueueInfos);

        // Clear original
        view.clear();
        BMQTST_ASSERT_EQ(false, view.isValid());

        // Verify second instance remains valid
        verifySubQueueInfos(view2, subQueueInfos);

        // Copy invalid instance (original)
        bmqp::OptionsView view3(view, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(false, view3.isValid());

        // Reset original, iterate and verify again
        BMQTST_ASSERT_EQ(
            0,
            view.reset(&blob, optionsAreaPosition, optionsAreaSize));
        verifySubQueueInfos(view, subQueueInfos);
    }

    {
        // [5]
        PV("BASIC FUNCTIONALITY: ONE OPTION HEADER, MULTIPLE OPTION VALUES");

        // Create valid view, PUSH event with 3 sub-queue ids
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        bmqu::BlobPosition optionsAreaPosition;
        int                optionsAreaSize = 0;

        // Populate blob, 1 option header, multiple sub-queue ids)
        bsl::vector<bmqp::SubQueueInfo> subQueueInfos(
            bmqtst::TestHelperUtil::allocator());
        size_t                          numSubQueueIds = 3;
        NullableMsgGroupId msgGroupId(bmqtst::TestHelperUtil::allocator());
        bool                            hasMsgGroupId = false;

        populateBlob(&blob,
                     &optionsAreaPosition,
                     &optionsAreaSize,
                     &subQueueInfos,
                     0,
                     false,
                     numSubQueueIds,
                     &msgGroupId,
                     hasMsgGroupId);

        // Iterate and verify
        bmqp::OptionsView view(&blob,
                               optionsAreaPosition,
                               optionsAreaSize,
                               bmqtst::TestHelperUtil::allocator());
        verifySubQueueInfos(view, subQueueInfos);

        // Copy valid instance, iterate and verify
        bmqp::OptionsView view2(view, bmqtst::TestHelperUtil::allocator());
        verifySubQueueInfos(view2, subQueueInfos);

        // Clear original
        view.clear();
        BMQTST_ASSERT_EQ(false, view.isValid());

        // Verify second instance remains valid
        BMQTST_ASSERT_EQ(true, view2.isValid());
        verifySubQueueInfos(view2, subQueueInfos);

        // Copy invalid instance (original)
        bmqp::OptionsView view3(view, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(false, view3.isValid());

        // Reset original, iterate and verify again
        BMQTST_ASSERT_EQ(
            0,
            view.reset(&blob, optionsAreaPosition, optionsAreaSize));
        verifySubQueueInfos(view, subQueueInfos);
    }

    {
        // [6]
        PV("BASIC FUNCTIONALITY: ONE OPTION HEADER, MULTIPLE OPTION "
           "VALUES INCLUDING GROUP ID");

        // Create valid view, PUSH event with 3 sub-queue ids
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        bmqu::BlobPosition optionsAreaPosition;
        int                optionsAreaSize = 0;

        // Populate blob, 1 option header, multiple sub-queue ids)
        bsl::vector<bmqp::SubQueueInfo> subQueueInfos(
            bmqtst::TestHelperUtil::allocator());
        size_t                          numSubQueueIds = 3;
        NullableMsgGroupId msgGroupId(bmqtst::TestHelperUtil::allocator());
        bool                            hasMsgGroupId = true;

        populateBlob(&blob,
                     &optionsAreaPosition,
                     &optionsAreaSize,
                     &subQueueInfos,
                     0,
                     false,
                     numSubQueueIds,
                     &msgGroupId,
                     hasMsgGroupId);

        // Iterate and verify
        bmqp::OptionsView view(&blob,
                               optionsAreaPosition,
                               optionsAreaSize,
                               bmqtst::TestHelperUtil::allocator());
        verifySubQueueInfos(view, subQueueInfos);
        BMQTST_ASSERT_EQ(true, !msgGroupId.isNull());
        BMQTST_ASSERT_EQ(true,
                         view.find(bmqp::OptionType::e_MSG_GROUP_ID) !=
                             view.end());
        bmqp::Protocol::MsgGroupId readGroupId(
            bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(0, view.loadMsgGroupIdOption(&readGroupId));
        BMQTST_ASSERT_EQ(readGroupId, msgGroupId.value());

        // Copy valid instance, iterate and verify
        bmqp::OptionsView view2(view, bmqtst::TestHelperUtil::allocator());
        verifySubQueueInfos(view2, subQueueInfos);
        BMQTST_ASSERT_EQ(true,
                         view2.find(bmqp::OptionType::e_MSG_GROUP_ID) !=
                             view2.end());
        readGroupId.clear();
        BMQTST_ASSERT_EQ(0, view2.loadMsgGroupIdOption(&readGroupId));
        BMQTST_ASSERT_EQ(readGroupId, msgGroupId.value());

        // Clear original
        view.clear();
        BMQTST_ASSERT_EQ(false, view.isValid());

        // Verify second instance remains valid
        verifySubQueueInfos(view2, subQueueInfos);
        BMQTST_ASSERT_EQ(true,
                         view2.find(bmqp::OptionType::e_MSG_GROUP_ID) !=
                             view2.end());
        readGroupId.clear();
        BMQTST_ASSERT_EQ(0, view2.loadMsgGroupIdOption(&readGroupId));
        BMQTST_ASSERT_EQ(readGroupId, msgGroupId.value());

        // Copy invalid instance (original)
        bmqp::OptionsView view3(view, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(false, view3.isValid());

        // Reset original, iterate and verify again
        BMQTST_ASSERT_EQ(
            0,
            view.reset(&blob, optionsAreaPosition, optionsAreaSize));
        verifySubQueueInfos(view, subQueueInfos);
        BMQTST_ASSERT_EQ(true,
                         view.find(bmqp::OptionType::e_MSG_GROUP_ID) !=
                             view.end());
        readGroupId.clear();
        BMQTST_ASSERT_EQ(0, view.loadMsgGroupIdOption(&readGroupId));
        BMQTST_ASSERT_EQ(readGroupId, msgGroupId.value());
    }
}

void test2_subQueueIdsOld()
// ------------------------------------------------------------------------
// SUB QUEUE IDS OLD
//
// Concerns:
//   Load old version of sub-queue ids to both Protocol::SubQueueInfosArray
//   and Protocol::SubQueueIdsArrayOld using this component.
//
// Plan:
//   - Init blob and populate it via push event and three old sub-queue id
//     option values
//   - Create OptionView instance to observe created options
//   - Iterate over options. Check iteration is correct
//   - Copy to other OptionView instances and verify expected behavior
//
// Testing:
//   loadSubQueueInfosOption(...)
//   loadSubQueueIdsOption(...)
// ------------------------------------------------------------------------
{
    PV(endl << "SUB QUEUE IDS OLD" << endl << "=================" << endl);

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());

    // Create valid view, PUSH event with 3 old sub-queue ids
    bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqu::BlobPosition optionsAreaPosition;
    int                optionsAreaSize = 0;

    // Populate blob, 1 option header, multiple old sub-queue ids)
    bsl::vector<bdlb::BigEndianUint32> subQueueIdsOld(
        bmqtst::TestHelperUtil::allocator());
    size_t                             numSubQueueIds = 3;
    NullableMsgGroupId msgGroupId(bmqtst::TestHelperUtil::allocator());
    bool                               hasMsgGroupId = false;

    populateBlob(&blob,
                 &optionsAreaPosition,
                 &optionsAreaSize,
                 0,
                 &subQueueIdsOld,
                 true,
                 numSubQueueIds,
                 &msgGroupId,
                 hasMsgGroupId);

    // Iterate and verify
    bmqp::OptionsView view(&blob,
                           optionsAreaPosition,
                           optionsAreaSize,
                           bmqtst::TestHelperUtil::allocator());
    verifySubQueueIdsOld(view, subQueueIdsOld);

    // Copy valid instance, iterate and verify
    bmqp::OptionsView view2(view, bmqtst::TestHelperUtil::allocator());
    verifySubQueueIdsOld(view2, subQueueIdsOld);

    // Clear original
    view.clear();
    BMQTST_ASSERT_EQ(false, view.isValid());

    // Verify second instance remains valid
    verifySubQueueIdsOld(view2, subQueueIdsOld);

    // Copy invalid instance (original)
    bmqp::OptionsView view3(view, bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(false, view3.isValid());

    // Reset original, iterate and verify again
    BMQTST_ASSERT_EQ(0,
                     view.reset(&blob, optionsAreaPosition, optionsAreaSize));
    verifySubQueueIdsOld(view, subQueueIdsOld);
}

void test3_packedOptions()
// ------------------------------------------------------------------------
// PACKED OPTIONS
//
// Concerns:
//   Load a *packed* option containing a RDA counter for the default
//   sub-queue id into a Protocol::SubQueueInfosArray using this component.
//
// Plan:
//   - Init blob and populate it via push event and a *packed* option
//     containing a RDA counter for the default sub-queue id
//   - Create OptionView instance to observe created options
//   - Iterate over options. Check iteration is correct
//   - Copy to other OptionView instances and verify expected behavior
//
// Testing:
//   loadSubQueueIdsOption(...)
// ------------------------------------------------------------------------
{
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());

    // Create valid view, PUSH event with a *packed* sub-queue id option
    bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqu::BlobPosition optionsAreaPosition;
    int                optionsAreaSize = 0;

    bsl::vector<bmqp::SubQueueInfo> subQueueInfos(
        bmqtst::TestHelperUtil::allocator());
    populatePackedBlob(&blob,
                       &optionsAreaPosition,
                       &optionsAreaSize,
                       &subQueueInfos);

    // Iterate and verify
    bmqp::OptionsView view(&blob,
                           optionsAreaPosition,
                           optionsAreaSize,
                           bmqtst::TestHelperUtil::allocator());
    verifySubQueueInfos(view, subQueueInfos);

    // Copy valid instance, iterate and verify
    bmqp::OptionsView view2(view, bmqtst::TestHelperUtil::allocator());
    verifySubQueueInfos(view2, subQueueInfos);

    // Clear original
    view.clear();
    BMQTST_ASSERT_EQ(false, view.isValid());

    // Verify second instance remains valid
    verifySubQueueInfos(view2, subQueueInfos);

    // Copy invalid instance (original)
    bmqp::OptionsView view3(view, bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(false, view3.isValid());

    // Reset original, iterate and verify again
    BMQTST_ASSERT_EQ(0,
                     view.reset(&blob, optionsAreaPosition, optionsAreaSize));
    verifySubQueueInfos(view, subQueueInfos);
}

void test4_invalidOptionsArea()
// ------------------------------------------------------------------------
// INVALID OPTIONS AREA
//
// Concerns:
//   Iterating over an invalid options area.
//
// Plan:
//   - One less sub-queue info than specified in OptionHeader
//   - An OptionsHeader but no option payload (sub-queue info)
//   - An OptionHeader with type 'OptionType::e_UNDEFINED'
//   - An OptionHeader with an unsupported type
//   - Multiple OptionHeader of the same type (duplicate)
//   - Empty blob used for initialization
//   - Blob with not enough size to locate OptionHeader
//
// Testing:
//   Construction of OptionsView with an invalid options area results in an
//   invalid OptionsView instance ('isValid()' returns false) or inability
//   to load an option's payload as determined by the return code.
// ------------------------------------------------------------------------
{
    PV(endl
       << "INVALID OPTIONS AREA" << endl
       << "====================" << endl);

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());

    {
        // [1]
        PV("LESS OPTION VALUES THAN SPECIFIED IN OPTION HEADER");

        // One less sub-queue info than specified in OptionHeader. Note that
        // this will NOT be detected because we decided that validating that
        // the blob is of sufficient length to contain the specified size is
        // the responsibility of the user. This means that an option header
        // could lie to us that it has bigger payload than it actually has in
        // the blob and the view will still be valid.
        bdlbb::Blob        optionsBlob(&bufferFactory,
                                bmqtst::TestHelperUtil::allocator());
        bmqu::BlobPosition optionsAreaPosition;  // start of blob
        int                numSubQueueInfos = 6;

        const int optionPayloadSize = numSubQueueInfos *
                                      sizeof(bmqp::SubQueueInfo);
        const int optionsAreaSize = sizeof(bmqp::OptionHeader) +
                                    optionPayloadSize;

        bmqp::OptionHeader oh;
        oh.setType(bmqp::OptionType::e_SUB_QUEUE_INFOS);
        oh.setWords(optionsAreaSize / bmqp::Protocol::k_WORD_SIZE);

        // Write OptionHeader
        bdlbb::BlobUtil::append(&optionsBlob,
                                reinterpret_cast<const char*>(&oh),
                                sizeof(bmqp::OptionHeader));

        // Write option payload minus one subQueueInfo
        bsl::vector<bmqp::SubQueueInfo> subQueueInfos(
            bmqtst::TestHelperUtil::allocator());
        generateSubQueueInfos(&subQueueInfos, numSubQueueInfos - 1);
        // one less than expected by OptionHeader's 'words()'

        bdlbb::BlobUtil::append(
            &optionsBlob,
            reinterpret_cast<const char*>(subQueueInfos.data()),
            (numSubQueueInfos - 1) * sizeof(bmqp::SubQueueInfo));

        // Construct invalid view with invalid option payload
        bmqp::OptionsView view(&optionsBlob,
                               optionsAreaPosition,
                               optionsAreaSize,
                               bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(true, view.isValid());
        // The view is valid despite the inconsistency.
    }

    {
        // [2]
        PV("OPTION HEADER INDICATES ZERO OPTION VALUES");

        // An OptionsHeader but no option payload (sub-queue info)
        bdlbb::Blob              optionsBlob(&bufferFactory,
                                bmqtst::TestHelperUtil::allocator());
        const bmqu::BlobPosition optionsAreaPosition;  // start of blob
        const int                optionsAreaSize = sizeof(bmqp::OptionHeader);

        bmqp::OptionHeader oh;
        oh.setType(bmqp::OptionType::e_SUB_QUEUE_INFOS);
        oh.setWords(sizeof(bmqp::OptionHeader) / bmqp::Protocol::k_WORD_SIZE);

        // Write OptionHeader
        bdlbb::BlobUtil::append(&optionsBlob,
                                reinterpret_cast<const char*>(&oh),
                                sizeof(bmqp::OptionHeader));

        // Construct invalid view with invalid options area
        bmqp::OptionsView view(&optionsBlob,
                               optionsAreaPosition,
                               optionsAreaSize,
                               bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(false, view.isValid());
    }

    {
        // [3]
        PV("OPTION HEADER WITH TYPE UNDEFINED");

        // An OptionHeader with type 'OptionType::e_UNDEFINED'
        bdlbb::Blob              optionsBlob(&bufferFactory,
                                bmqtst::TestHelperUtil::allocator());
        const bmqu::BlobPosition optionsAreaPosition;  // start of blob
        const int                numSubQueueInfos = 1;

        const int optionPayloadSize = numSubQueueInfos *
                                      sizeof(bmqp::SubQueueInfo);
        const int optionsAreaSize = sizeof(bmqp::OptionHeader) +
                                    optionPayloadSize;

        bmqp::OptionHeader oh;
        oh.setType(bmqp::OptionType::e_UNDEFINED);
        oh.setWords(optionsAreaSize / bmqp::Protocol::k_WORD_SIZE);

        // Write OptionHeader
        bdlbb::BlobUtil::append(&optionsBlob,
                                reinterpret_cast<const char*>(&oh),
                                sizeof(bmqp::OptionHeader));

        // Write option payload (not really necessary but just to differentiate
        // it from the test case where there's only an OptionHeader).
        bsl::vector<bmqp::SubQueueInfo> subQueueInfos(
            bmqtst::TestHelperUtil::allocator());
        generateSubQueueInfos(&subQueueInfos, numSubQueueInfos);

        bdlbb::BlobUtil::append(
            &optionsBlob,
            reinterpret_cast<const char*>(subQueueInfos.data()),
            optionPayloadSize);

        // Construct invalid view with invalid options area
        bmqp::OptionsView view(&optionsBlob,
                               optionsAreaPosition,
                               optionsAreaSize,
                               bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(false, view.isValid());
    }

    {
        // [4]
        PV("OPTION HEADER WITH UNSUPPORTED TYPES");

        // #1: An OptionHeader with an unsupported type
        bdlbb::Blob              optionsBlob(&bufferFactory,
                                bmqtst::TestHelperUtil::allocator());
        const bmqu::BlobPosition optionsAreaPosition;  // start of blob
        const int                numSubQueueInfos = 2;

        const int optionPayloadSize = numSubQueueInfos *
                                      sizeof(bmqp::SubQueueInfo);
        const int optionsAreaSize = sizeof(bmqp::OptionHeader) +
                                    optionPayloadSize;

        bmqp::OptionHeader     oh;
        bmqp::OptionType::Enum optionType =
            static_cast<bmqp::OptionType::Enum>(
                bmqp::OptionType::k_LOWEST_SUPPORTED_TYPE - 1);
        oh.setType(optionType);
        oh.setWords(optionsAreaSize / bmqp::Protocol::k_WORD_SIZE);

        // Write OptionHeader
        bdlbb::BlobUtil::append(&optionsBlob,
                                reinterpret_cast<const char*>(&oh),
                                sizeof(bmqp::OptionHeader));

        // Write option payload
        bsl::vector<bmqp::SubQueueInfo> subQueueInfos(
            bmqtst::TestHelperUtil::allocator());
        generateSubQueueInfos(&subQueueInfos, numSubQueueInfos);

        bdlbb::BlobUtil::append(
            &optionsBlob,
            reinterpret_cast<const char*>(subQueueInfos.data()),
            optionPayloadSize);

        // Construct invalid view with an invalid type in an OptionHeader
        bmqp::OptionsView view1(&optionsBlob,
                                optionsAreaPosition,
                                optionsAreaSize,
                                bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(false, view1.isValid());

        // #2: An OptionHeader with an unsupported type
        optionType = static_cast<bmqp::OptionType::Enum>(
            bmqp::OptionType::k_HIGHEST_SUPPORTED_TYPE + 1);
        oh.setType(optionType);

        // Overwrite OptionHeader
        bmqu::BlobUtil::writeBytes(&optionsBlob,
                                   optionsAreaPosition,
                                   reinterpret_cast<const char*>(&oh),
                                   sizeof(bmqp::OptionHeader));

        // Construct invalid view with an invalid type in an OptionHeader
        bmqp::OptionsView view2(&optionsBlob,
                                optionsAreaPosition,
                                optionsAreaSize,
                                bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(false, view2.isValid());
    }

    {
        // [5]
        PV("MULTIPLE OPTION HEADERS OF THE SAME TYPE (DUPLICATE)");

        // Multiple OptionHeader of the same type (duplicate)
        bdlbb::Blob              optionsBlob(&bufferFactory,
                                bmqtst::TestHelperUtil::allocator());
        const bmqu::BlobPosition optionsAreaPosition;  // start of blob
        const int                numSubQueueInfos = 2;

        const int optionPayloadSize = numSubQueueInfos *
                                      sizeof(bmqp::SubQueueInfo);
        int optionsAreaSize = sizeof(bmqp::OptionHeader) + optionPayloadSize;

        // First option
        bmqp::OptionHeader oh1;
        oh1.setType(bmqp::OptionType::e_SUB_QUEUE_INFOS);
        oh1.setWords(optionsAreaSize / bmqp::Protocol::k_WORD_SIZE);

        // Write OptionHeader
        bdlbb::BlobUtil::append(&optionsBlob,
                                reinterpret_cast<const char*>(&oh1),
                                sizeof(bmqp::OptionHeader));

        // Write option payload
        bsl::vector<bmqp::SubQueueInfo> subQueueInfos(
            bmqtst::TestHelperUtil::allocator());
        generateSubQueueInfos(&subQueueInfos, numSubQueueInfos);

        bdlbb::BlobUtil::append(
            &optionsBlob,
            reinterpret_cast<const char*>(subQueueInfos.data()),
            optionPayloadSize);

        // Second option (of the same type as the first option)
        const int numSubQueueInfos2  = 3;
        const int optionPayloadSize2 = numSubQueueInfos2 *
                                       sizeof(bmqp::SubQueueInfo);
        optionsAreaSize += sizeof(bmqp::OptionHeader) + optionPayloadSize2;

        bmqp::OptionHeader oh2;
        oh2.setType(bmqp::OptionType::e_SUB_QUEUE_INFOS);
        // This is a duplicate
        oh2.setWords((sizeof(bmqp::OptionHeader) + optionPayloadSize2) /
                     bmqp::Protocol::k_WORD_SIZE);

        // Write OptionHeader
        bdlbb::BlobUtil::append(&optionsBlob,
                                reinterpret_cast<const char*>(&oh2),
                                sizeof(bmqp::OptionHeader));

        // Write option payload
        generateSubQueueInfos(&subQueueInfos, numSubQueueInfos2);

        bdlbb::BlobUtil::append(
            &optionsBlob,
            reinterpret_cast<const char*>(subQueueInfos.data()),
            optionPayloadSize2);

        // Construct invalid view with invalid options area
        bmqp::OptionsView view(&optionsBlob,
                               optionsAreaPosition,
                               optionsAreaSize,
                               bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(false, view.isValid());
    }
    {
        // [6]
        PV("EMPTY BLOB");

        bdlbb::Blob              optionsBlob(&bufferFactory,
                                bmqtst::TestHelperUtil::allocator());
        const bmqu::BlobPosition optionsAreaPosition;  // start of blob
        const int                optionsAreaSize = sizeof(bmqp::OptionHeader);

        bmqp::OptionHeader oh;
        oh.setType(bmqp::OptionType::e_SUB_QUEUE_INFOS);
        oh.setWords(sizeof(bmqp::OptionHeader) / bmqp::Protocol::k_WORD_SIZE);

        bmqp::OptionsView view(&optionsBlob,
                               optionsAreaPosition,
                               optionsAreaSize,
                               bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(false, view.isValid());
    }
    {
        // [7]
        PV("WRONG AREA SIZE FOR OPTION VIEW");

        bdlbb::Blob              optionsBlob(&bufferFactory,
                                bmqtst::TestHelperUtil::allocator());
        const bmqu::BlobPosition optionsAreaPosition;  // start of blob
        const int                numSubQueueInfos = 6;

        const int optionPayloadSize = numSubQueueInfos *
                                      sizeof(bmqp::SubQueueInfo);
        const int optionsAreaSize = sizeof(bmqp::OptionHeader) +
                                    optionPayloadSize;

        bmqp::OptionHeader oh;
        oh.setType(bmqp::OptionType::e_SUB_QUEUE_INFOS);
        oh.setWords(optionsAreaSize / bmqp::Protocol::k_WORD_SIZE);

        // Write OptionHeader
        bdlbb::BlobUtil::append(&optionsBlob,
                                reinterpret_cast<const char*>(&oh),
                                sizeof(bmqp::OptionHeader));

        // Write option payload
        bsl::vector<bmqp::SubQueueInfo> subQueueInfos(
            bmqtst::TestHelperUtil::allocator());
        generateSubQueueInfos(&subQueueInfos, numSubQueueInfos);

        bdlbb::BlobUtil::append(
            &optionsBlob,
            reinterpret_cast<const char*>(subQueueInfos.data()),
            optionPayloadSize);

        // Construct invalid view with invalid option payload
        bmqp::OptionsView view(&optionsBlob,
                               optionsAreaPosition,
                               optionsAreaSize - 1,
                               bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(false, view.isValid());
    }
}

void test5_iteratorTest()
{
    // ------------------------------------------------------------------------
    // ITERATOR TEST TEST
    //
    // Concerns:
    //   Exercise the basic functionality of the options view iterator.
    //
    // Plan:
    //   - Init blob and populate it via push event and two options
    //   - Create OptionView instance to observe created options
    //   - Iterate over options. Check iteration is correct
    //
    // Testing:
    //   OptionsView::Iterator::operator++
    //   OptionsView::Iterator::operator*
    //   OptionsView::Iterator::operator==
    //
    // ------------------------------------------------------------------------

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());

    PV("OPTIONS VIEW ITERATOR FUNCTIONALITY TEST");

    // Create valid view, PUSH event with 3 sub-queue ids
    bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqu::BlobPosition optionsAreaPosition;
    int                optionsAreaSize = 0;

    // Populate blob, 1 option header, multiple sub-queue ids
    bsl::vector<bmqp::SubQueueInfo> subQueueIds(
        bmqtst::TestHelperUtil::allocator());
    size_t                          numSubQueueIds = 3;
    NullableMsgGroupId msgGroupId(bmqtst::TestHelperUtil::allocator());
    bool                            hasMsgGroupId = true;

    populateBlob(&blob,
                 &optionsAreaPosition,
                 &optionsAreaSize,
                 &subQueueIds,
                 0,
                 false,
                 numSubQueueIds,
                 &msgGroupId,
                 hasMsgGroupId);

    // Iterate and verify
    bmqp::OptionsView view(&blob,
                           optionsAreaPosition,
                           optionsAreaSize,
                           bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT(view.isValid());

    bmqp::OptionsView::const_iterator iter  = view.begin();
    bmqp::OptionsView::const_iterator iter2 = iter;
    BMQTST_ASSERT(iter2 != view.end());
    BMQTST_ASSERT(iter == iter2);
    BMQTST_ASSERT(*iter == bmqp::OptionType::e_MSG_GROUP_ID);
    BMQTST_ASSERT(*iter2 == bmqp::OptionType::e_MSG_GROUP_ID);

    ++iter2;
    BMQTST_ASSERT(iter2 != view.end());
    BMQTST_ASSERT(iter2 != iter);
    BMQTST_ASSERT(*iter2 == bmqp::OptionType::e_SUB_QUEUE_INFOS);

    ++(iter.imp());
    BMQTST_ASSERT(iter == iter2);
    BMQTST_ASSERT(*iter == bmqp::OptionType::e_SUB_QUEUE_INFOS);
    BMQTST_ASSERT(iter != view.end());

    ++iter2;
    BMQTST_ASSERT(iter2 == view.end());

    ++(iter.imp());
    BMQTST_ASSERT(iter == view.end());
}

void test6_iteratorTestSubQueueIdsOld()
{
    // ------------------------------------------------------------------------
    // ITERATOR TEST TEST SUB QUEUE IDS OLD
    //
    // Concerns:
    //   Exercise the basic functionality of the options view iterator, when
    //   the encoded sub-queue ids are of old version.
    //
    // Plan:
    //   - Init blob and populate it via push event and two options
    //   - Create OptionView instance to observe created options
    //   - Iterate over options. Check iteration is correct
    //
    // Testing:
    //   OptionsView::Iterator::operator++
    //   OptionsView::Iterator::operator*
    //   OptionsView::Iterator::operator==
    //
    // ------------------------------------------------------------------------

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());

    PV("OPTIONS VIEW ITERATOR FUNCTIONALITY TEST (SUB QUEUE IDS OLD)");

    // Create valid view, PUSH event with 3 old version sub-queue ids
    bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqu::BlobPosition optionsAreaPosition;
    int                optionsAreaSize = 0;

    // Populate blob, 1 option header, multiple sub-queue ids
    bsl::vector<bdlb::BigEndianUint32> subQueueIdsOld(
        bmqtst::TestHelperUtil::allocator());
    size_t                             numSubQueueIds = 3;
    NullableMsgGroupId msgGroupId(bmqtst::TestHelperUtil::allocator());
    bool                               hasMsgGroupId = true;

    populateBlob(&blob,
                 &optionsAreaPosition,
                 &optionsAreaSize,
                 0,
                 &subQueueIdsOld,
                 true,
                 numSubQueueIds,
                 &msgGroupId,
                 hasMsgGroupId);

    // Iterate and verify
    bmqp::OptionsView view(&blob,
                           optionsAreaPosition,
                           optionsAreaSize,
                           bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT(view.isValid());

    bmqp::OptionsView::const_iterator iter  = view.begin();
    bmqp::OptionsView::const_iterator iter2 = iter;
    BMQTST_ASSERT(iter2 != view.end());
    BMQTST_ASSERT(iter == iter2);
    BMQTST_ASSERT(*iter == bmqp::OptionType::e_SUB_QUEUE_IDS_OLD);
    BMQTST_ASSERT(*iter2 == bmqp::OptionType::e_SUB_QUEUE_IDS_OLD);

    ++iter2;
    BMQTST_ASSERT(iter2 != view.end());
    BMQTST_ASSERT(iter2 != iter);
    BMQTST_ASSERT(*iter2 == bmqp::OptionType::e_MSG_GROUP_ID);

    ++(iter.imp());
    BMQTST_ASSERT(iter == iter2);
    BMQTST_ASSERT(*iter == bmqp::OptionType::e_MSG_GROUP_ID);
    BMQTST_ASSERT(iter != view.end());

    ++iter2;
    BMQTST_ASSERT(iter2 == view.end());

    ++(iter.imp());
    BMQTST_ASSERT(iter == view.end());
}

void test7_dumpBlob()
{
    // --------------------------------------------------------------------
    // DUMP BLOB
    //
    // Concerns:
    //   Check blob layout if:
    //     1. The underlying blob contain PUSH message with 2 options
    //     2. The underlying blob is not setted
    //
    // Plan:
    //   1. Create and populate blob with a single PUSH message with 2 options
    //   2. Create and init options view by given blob
    //   3. Check output of dumpBlob method
    //   4. Create and init options view without setting underlying blob
    //   5. Check output of dumpBlob method
    //
    // Testing:
    //   void dumpBlob(bsl::ostream& stream);
    // --------------------------------------------------------------------
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());

    PV("DUMP BLOB: ONE OPTION HEADER, MULTIPLE OPTION VALUES");

    // Create valid view, PUSH event with 3 sub-queue ids
    bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqu::BlobPosition optionsAreaPosition;
    int                optionsAreaSize = 0;
    bmqu::MemOutStream stream(bmqtst::TestHelperUtil::allocator());

    // Populate blob, 1 option header, multiple sub-queue ids)
    bsl::vector<bmqp::SubQueueInfo> subQueueIds(
        bmqtst::TestHelperUtil::allocator());
    size_t                          numSubQueueIds = 3;
    NullableMsgGroupId msgGroupId(bmqtst::TestHelperUtil::allocator());
    bool                            hasMsgGroupId = true;

    populateBlob(&blob,
                 &optionsAreaPosition,
                 &optionsAreaSize,
                 &subQueueIds,
                 0,
                 false,
                 numSubQueueIds,
                 &msgGroupId,
                 hasMsgGroupId);

    // Create view to observe multiple options
    {
        bmqp::OptionsView view(&blob,
                               optionsAreaPosition,
                               optionsAreaSize,
                               bmqtst::TestHelperUtil::allocator());
        BSLS_ASSERT_OPT(view.isValid());
        // Dump blob
        view.dumpBlob(stream);
        bsl::string str1(stream.str(), bmqtst::TestHelperUtil::allocator());
        bsl::string str2("     0:   00000054 44020000 40000013 00000B08     "
                         "|...TD...@.......|\n"
                         "    16:   0000007B 00000000 00000000 00000000     "
                         "|...{............|\n"
                         "    32:   00000000 00000000 0C400007 00003E39     "
                         "|.........@....>9|\n"
                         "    48:   39000000 00001139 39000000 00002686     "
                         "|9......99.....&.|\n"
                         "    64:   06000000 08000004 6769643A 31353932     "
                         "|........gid:1592|\n"
                         "    80:   39030303                                "
                         "|9...            |\n",
                         bmqtst::TestHelperUtil::allocator());
        // Verify that dump contains expected value
        BMQTST_ASSERT_EQ(str1, str2);
        stream.reset();
    }

    // Create view without blob
    {
        bmqp::OptionsView view(bmqtst::TestHelperUtil::allocator());

        // Dump blob
        view.dumpBlob(stream);
        bsl::string str1(stream.str(), bmqtst::TestHelperUtil::allocator());
        bsl::string str2("/no blob/", bmqtst::TestHelperUtil::allocator());

        // Verify that dump contains expected value
        BMQTST_ASSERT_EQ(str1, str2);
    }
}

void test8_dumpBlobSubQueueIdsOld()
{
    // --------------------------------------------------------------------
    // DUMP BLOB SUB QUEUE IDS OLD
    //
    // Concerns:
    //   Check blob layout if the underlying blob contain PUSH message with 2
    //   options, including sub-queue ids old.
    //
    // Plan:
    //   1. Create and populate blob with a single PUSH message with 2 options,
    //      including sub-queue ids old
    //   2. Create and init options view by given blob
    //   3. Check output of dumpBlob method
    //
    // Testing:
    //   void dumpBlob(bsl::ostream& stream);
    // --------------------------------------------------------------------
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());

    PV("DUMP BLOB: ONE OPTION HEADER, MULTIPLE OPTION VALUES "
       "(SUB QUEUE IDS OLD)");

    // Create valid view, PUSH event with 3 old version sub-queue ids
    bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqu::BlobPosition optionsAreaPosition;
    int                optionsAreaSize = 0;
    bmqu::MemOutStream stream(bmqtst::TestHelperUtil::allocator());

    // Populate blob, 1 option header, multiple sub-queue ids)
    bsl::vector<bdlb::BigEndianUint32> subQueueIdsOld(
        bmqtst::TestHelperUtil::allocator());
    size_t                             numSubQueueIds = 3;
    NullableMsgGroupId msgGroupId(bmqtst::TestHelperUtil::allocator());
    bool                               hasMsgGroupId = true;

    populateBlob(&blob,
                 &optionsAreaPosition,
                 &optionsAreaSize,
                 0,
                 &subQueueIdsOld,
                 true,
                 numSubQueueIds,
                 &msgGroupId,
                 hasMsgGroupId);

    // Create view to observe multiple options
    {
        bmqp::OptionsView view(&blob,
                               optionsAreaPosition,
                               optionsAreaSize,
                               bmqtst::TestHelperUtil::allocator());

        // Dump blob
        view.dumpBlob(stream);
        bsl::string str1(stream.str(), bmqtst::TestHelperUtil::allocator());
        bsl::string str2("     0:   00000048 44020000 40000010 00000808     "
                         "|...HD...@.......|\n"
                         "    16:   0000007B 00000000 00000000 00000000     "
                         "|...{............|\n"
                         "    32:   00000000 00000000 04000004 00003E39     "
                         "|..............>9|\n"
                         "    48:   00001139 00002686 08000004 6769643A     "
                         "|...9..&.....gid:|\n"
                         "    64:   31353932 39030303                       "
                         "|15929...        |\n",
                         bmqtst::TestHelperUtil::allocator());

        // Verify that dump contains expected value
        BMQTST_ASSERT_EQ(str1, str2);
        stream.reset();
    }
}

}  // close unnamed namespace

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    switch (_testCase) {
    case 0:
    case 8: test8_dumpBlobSubQueueIdsOld(); break;
    case 7: test7_dumpBlob(); break;
    case 6: test6_iteratorTestSubQueueIdsOld(); break;
    case 5: test5_iteratorTest(); break;
    case 4: test4_invalidOptionsArea(); break;
    case 3: test3_packedOptions(); break;
    case 2: test2_subQueueIdsOld(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
