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

// bmqp_pusheventbuilder.cpp                                          -*-C++-*-
#include <bmqp_pusheventbuilder.h>

#include <bmqscm_version.h>
// MWC
#include <mwcu_blob.h>

// BDE
#include <bdlbb_blobutil.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_algorithm.h>
#include <bsl_cstring.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslmf_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqp {

namespace {

bool isDefaultSubQueueInfo(const Protocol::SubQueueInfosArray& subQueueInfos)
{
    return subQueueInfos.size() == 1U &&
           subQueueInfos[0].id() == Protocol::k_DEFAULT_SUBSCRIPTION_ID;
}

}  // close anonymous namespace

// ----------------------
// class PushEventBuilder
// ----------------------

// PRIVATE MANIPULATORS
bmqt::EventBuilderResult::Enum PushEventBuilder::packMessageImp(
    const bdlbb::Blob&                   payload,
    int                                  queueId,
    const bmqt::MessageGUID&             msgId,
    int                                  flags,
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType,
    const MessagePropertiesInfo&         messagePropertiesInfo)
{
    // PRECONDITIONS
    BSLA_MAYBE_UNUSED const int optionsCount = d_options.optionsCount();
    const int                   optionsSize  = d_options.size();
    BSLS_ASSERT_SAFE((optionsCount > 0 && d_currPushHeader.isSet()) ||
                     (optionsCount == 0 && !d_currPushHeader.isSet()));
    BSLS_ASSERT_SAFE(optionsSize < PushHeader::k_MAX_OPTIONS_SIZE);

    const int payloadLen = payload.length();

    // Validate payload is not too big
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            payloadLen > PushHeader::k_MAX_PAYLOAD_SIZE_SOFT)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        eraseCurrentMessage();
        return bmqt::EventBuilderResult::e_PAYLOAD_TOO_BIG;  // RETURN
    }

    int numPaddingBytes = 0;
    int numWords = ProtocolUtil::calcNumWordsAndPadding(&numPaddingBytes,
                                                        payloadLen);

    // Ensure event has enough space left for this message
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            EventHeader::k_MAX_SIZE_SOFT <
            static_cast<int>((eventSize() + sizeof(PushHeader) + optionsSize +
                              payloadLen + numPaddingBytes)))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // The total size of this event would exceed the enforced maximum.
        eraseCurrentMessage();
        return bmqt::EventBuilderResult::e_EVENT_TOO_BIG;  // RETURN
    }

    ensurePushHeader();

    // Make sure memory is reset
    bsl::memset(static_cast<void*>(d_currPushHeader.object()),
                0,
                sizeof(bmqp::PushHeader));

    const int headerWords = sizeof(bmqp::PushHeader) / Protocol::k_WORD_SIZE;

    BSLS_ASSERT_SAFE(0 == optionsSize % Protocol::k_WORD_SIZE);
    const int optionsWords = optionsSize / Protocol::k_WORD_SIZE;

    (*d_currPushHeader)
        .setHeaderWords(headerWords)
        .setOptionsWords(optionsWords)
        .setQueueId(queueId)
        .setMessageWords(headerWords + optionsWords + numWords)
        .setMessageGUID(msgId)
        .setCompressionAlgorithmType(compressionAlgorithmType)
        .setFlags(flags);

    messagePropertiesInfo.applyTo(d_currPushHeader.object());

    d_currPushHeader.reset();  // i.e., flush writing to blob..

    // Add the payload
    bdlbb::BlobUtil::append(d_blob_sp.get(), payload);

    // Add padding
    ProtocolUtil::appendPadding(d_blob_sp.get(), payloadLen);

    d_options.reset();
    ++d_msgCount;

    return bmqt::EventBuilderResult::e_SUCCESS;
}

void PushEventBuilder::ensurePushHeader()
{
    // Check if PushHeader has not been written for current message
    if (d_options.size() != 0) {
        return;  // RETURN
    }

    // Add the PushHeader
    mwcu::BlobPosition phOffset;
    mwcu::BlobUtil::reserve(&phOffset, d_blob_sp.get(), sizeof(PushHeader));

    d_currPushHeader.reset(d_blob_sp.get(),
                           phOffset,
                           false,  // no read
                           true);  // write mode

    BSLS_ASSERT_SAFE(d_currPushHeader.isSet());
    // Should never fail because we just reserved enough space
}

// CREATORS
PushEventBuilder::PushEventBuilder(bdlbb::BlobBufferFactory* bufferFactory,
                                   bslma::Allocator*         allocator)
: d_allocator_p(bslma::Default::allocator(allocator))
, d_bufferFactory_p(bufferFactory)
, d_blob_sp(0, allocator)  // initialized in `reset()`
, d_msgCount(0)
, d_options()
, d_currPushHeader()
{
    reset();
}

// MANIPULATORS
int PushEventBuilder::reset()
{
    d_currPushHeader.reset();
    // Flush any buffered changes if necessary, and make this object not
    // refer to any valid blob object.

    d_blob_sp.createInplace(d_allocator_p, d_bufferFactory_p, d_allocator_p);

    d_msgCount = 0;
    d_options.reset();

    // NOTE: Since PushEventBuilder owns the blob and we just reset it, we have
    //       guarantee that buffer(0) will contain the entire header (unless
    //       the bufferFactory has blobs of ridiculously small size !!)

    // Ensure blob has enough space for an EventHeader
    //
    // Use placement new to create the object directly in the blob buffer,
    // while still calling it's constructor (to memset memory and initialize
    // some fields)
    d_blob_sp->setLength(sizeof(EventHeader));
    new (d_blob_sp->buffer(0).data()) EventHeader(EventType::e_PUSH);

    return 0;
}

bmqt::EventBuilderResult::Enum PushEventBuilder::addSubQueueIdsOption(
    const Protocol::SubQueueIdsArrayOld& subQueueIds)
{
    // PRECONDITIONS
    const int optionsSize = d_options.size();
    BSLS_ASSERT_SAFE((optionsSize > 0 && d_currPushHeader.isSet()) ||
                     (optionsSize == 0 && !d_currPushHeader.isSet()));
    (void)optionsSize;  // compiler happiness

    typedef bmqt::EventBuilderResult Result;
    typedef OptionUtil::OptionMeta   OptionMeta;

    const unsigned int numSubQueueIds = subQueueIds.size();

    // We do not write an OptionHeader when there is no option payload
    if (numSubQueueIds == 0) {
        return bmqt::EventBuilderResult::e_SUCCESS;  // RETURN
    }

    const int  currentSize = eventSize() + sizeof(PushHeader);
    const int  size        = numSubQueueIds * Protocol::k_WORD_SIZE;
    OptionMeta option = OptionMeta::forOption(OptionType::e_SUB_QUEUE_IDS_OLD,
                                              size);

    const Result::Enum rc = d_options.canAdd(currentSize, option);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != Result::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        eraseCurrentMessage();
        return rc;  // RETURN
    }

    // Make sure PushHeader is written for current message
    ensurePushHeader();

    // We need to convert the SubQueueIds to a vector of BigEndianUint32 and
    // then write them to the blob.
    bdlma::LocalSequentialAllocator<16 * sizeof(unsigned int)> lsa(
        d_allocator_p);
    bsl::vector<bdlb::BigEndianUint32> tempVec(&lsa);
    tempVec.reserve(numSubQueueIds);

    bsl::transform(subQueueIds.begin(),
                   subQueueIds.end(),
                   bsl::back_inserter(tempVec),
                   bdlb::BigEndianUint32::make);

    // TODO_POISON_PILL Make sure that adding *packed* option also works
    d_options.add(d_blob_sp.get(),
                  reinterpret_cast<const char*>(tempVec.data()),
                  option);

    return bmqt::EventBuilderResult::e_SUCCESS;
}

bmqt::EventBuilderResult::Enum PushEventBuilder::addSubQueueInfosOption(
    const Protocol::SubQueueInfosArray& subQueueInfos,
    bool                                packRdaCounter)
{
    // PRECONDITIONS
    const int optionsSize = d_options.size();
    BSLS_ASSERT_SAFE((optionsSize > 0 && d_currPushHeader.isSet()) ||
                     (optionsSize == 0 && !d_currPushHeader.isSet()));
    (void)optionsSize;  // compiler happiness

    typedef bmqt::EventBuilderResult Result;
    typedef OptionUtil::OptionMeta   OptionMeta;

    const unsigned int numSubQueueIds = subQueueInfos.size();

    // We do not write an OptionHeader when there is no option payload
    if (numSubQueueIds == 0) {
        return bmqt::EventBuilderResult::e_SUCCESS;  // RETURN
    }

    // Old option only includes subQueueId
    int        size   = numSubQueueIds * Protocol::k_WORD_SIZE;
    OptionMeta option = OptionMeta::forOption(OptionType::e_SUB_QUEUE_IDS_OLD,
                                              size);
    if (packRdaCounter) {
        // New option includes subQueueId + rdaCounter, or a special "packed"
        // option header if there is only the default subQueueId.
        size = numSubQueueIds * sizeof(bmqp::SubQueueInfo);

        bool          packed       = false;
        int           packedValue  = 0;
        unsigned char typeSpecific = sizeof(bmqp::SubQueueInfo) /
                                     bmqp::Protocol::k_WORD_SIZE;
        if (isDefaultSubQueueInfo(subQueueInfos)) {
            // Special case: default subQueueId. Pack RDA counter into header.
            packed       = true;
            packedValue  = subQueueInfos[0].rdaInfo().internalRepresentation();
            typeSpecific = 0;
            size         = 0;
        }
        option = OptionMeta::forOption(OptionType::e_SUB_QUEUE_INFOS,
                                       size,
                                       packed,
                                       packedValue,
                                       typeSpecific);
    }

    const int          currentSize = eventSize() + sizeof(PushHeader);
    const Result::Enum rc          = d_options.canAdd(currentSize, option);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != Result::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        eraseCurrentMessage();
        return rc;  // RETURN
    }

    // Make sure PushHeader is written for current message
    ensurePushHeader();

    if (packRdaCounter && isDefaultSubQueueInfo(subQueueInfos)) {
        // The header is packed and there is no payload, so simply add the
        // option header.
        d_options.add(d_blob_sp.get(), 0, option);
        return bmqt::EventBuilderResult::e_SUCCESS;  // RETURN
    }

    if (packRdaCounter) {
        // New options: Can directly add the subQueueInfos (subQueueId,
        // rdaCounter). Need contiguous memory, so first copy into a vector.
        bdlma::LocalSequentialAllocator<32 * sizeof(bmqp::SubQueueInfo)> lsa(
            d_allocator_p);
        bsl::vector<bmqp::SubQueueInfo> tempVec(&lsa);
        bsl::copy(subQueueInfos.begin(),
                  subQueueInfos.end(),
                  bsl::back_inserter(tempVec));
        d_options.add(d_blob_sp.get(),
                      reinterpret_cast<const char*>(tempVec.data()),
                      option);
        return bmqt::EventBuilderResult::e_SUCCESS;  // RETURN
    }

    // Old options: We need to convert the SubQueueInfos to a vector of
    // subQueueIds (BigEndianUint32) and then write them to the blob.
    bdlma::LocalSequentialAllocator<32 * sizeof(bdlb::BigEndianUint32)> lsa(
        d_allocator_p);
    bsl::vector<bdlb::BigEndianUint32> tempVec(&lsa);

    // Only pack in the subQueueIds
    tempVec.reserve(numSubQueueIds);
    for (Protocol::SubQueueInfosArray::const_iterator citer =
             subQueueInfos.begin();
         citer != subQueueInfos.end();
         ++citer) {
        tempVec.push_back(bdlb::BigEndianUint32::make(citer->id()));
    }

    d_options.add(d_blob_sp.get(),
                  reinterpret_cast<const char*>(tempVec.data()),
                  option);

    return bmqt::EventBuilderResult::e_SUCCESS;
}

bmqt::EventBuilderResult::Enum
PushEventBuilder::addMsgGroupIdOption(const Protocol::MsgGroupId& msgGroupId)
{
    // PRECONDITIONS
    BSLMF_ASSERT(OptionHeader::k_MAX_SIZE >
                 Protocol::k_MSG_GROUP_ID_MAX_LENGTH);
    // There are two checks below to validate option size.  One deals with
    // 'OptionHeader::k_MAX_SIZE' limit, see
    // 'OptionUtil::OptionsBox::canAdd'.  Another one checks that the
    // specified 'msgGroupId' doesn't exceed
    // 'Protocol::k_MSG_GROUP_ID_MAX_LENGTH', see
    // 'OptionUtil::isValidMsgGroupId'.  The order of the validation checks
    // is meant to ensure that both error scenarios are captured by this
    // method.

    const int optionsSize = d_options.size();
    BSLS_ASSERT_SAFE((optionsSize > 0 && d_currPushHeader.isSet()) ||
                     (optionsSize == 0 && !d_currPushHeader.isSet()));
    (void)optionsSize;  // compiler happiness

    typedef bmqt::EventBuilderResult Result;
    typedef OptionUtil::OptionMeta   OptionMeta;

    const int  currentSize = eventSize() + sizeof(PushHeader);
    const int  size        = msgGroupId.length();
    OptionMeta option =
        OptionMeta::forOptionWithPadding(OptionType::e_MSG_GROUP_ID, size);
    Result::Enum rc = d_options.canAdd(currentSize, option);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != Result::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        eraseCurrentMessage();
        return rc;  // RETURN
    }

    rc = OptionUtil::isValidMsgGroupId(msgGroupId);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != Result::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        eraseCurrentMessage();
        return rc;  // RETURN
    }

    // Make sure PushHeader is written for current message
    ensurePushHeader();

    d_options.add(d_blob_sp.get(),
                  reinterpret_cast<const char*>(msgGroupId.data()),
                  option);

    return bmqt::EventBuilderResult::e_SUCCESS;
}

// ACCESSORS
const bdlbb::Blob& PushEventBuilder::blob() const
{
    // Fix packet's length in header now that we know it ..  Following is valid
    // (see comment in reset)
    EventHeader& eh = *reinterpret_cast<EventHeader*>(
        d_blob_sp->buffer(0).data());
    eh.setLength(d_blob_sp->length());

    return *d_blob_sp;
}

bsl::shared_ptr<bdlbb::Blob> PushEventBuilder::blob_sp() const
{
    // Fix packet's length in header now that we know it ..  Following is valid
    // (see comment in reset)
    EventHeader& eh = *reinterpret_cast<EventHeader*>(
        d_blob_sp->buffer(0).data());
    eh.setLength(d_blob_sp->length());

    return d_blob_sp;
}

}  // close package namespace
}  // close enterprise namespace
