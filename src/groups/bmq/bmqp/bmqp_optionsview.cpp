// Copyright 2017-2023 Bloomberg Finance L.P.
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

// bmqp_optionsview.cpp                                               -*-C++-*-
#include <bmqp_optionsview.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqp_protocolutil.h>
#include <bmqp_queueid.h>

// MWC
#include <mwcu_blobiterator.h>
#include <mwcu_blobobjectproxy.h>

// BDE
#include <bsl_iostream.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqp {

// -----------------
// class OptionsView
// -----------------

// PRIVATE MANIPULATORS
int OptionsView::resetImpl(const bdlbb::Blob*        blob,
                           const mwcu::BlobPosition& optionsAreaPos,
                           int                       optionsAreaSize)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(blob);
    BSLS_ASSERT_SAFE(optionsAreaSize >= 0);
    BSLS_ASSERT_SAFE(d_optionPositions.size() ==
                     (OptionHeader::k_MAX_TYPE + 1));

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0  // Success
        ,
        rc_INVALID_OPTIONS_AREA = -1  // The blob does not contain a valid
                                      // options area at the specified
                                      // 'optionsAreaPos'.
        ,
        rc_INVALID_OPTIONS_AREA_SIZE = -2  // The blob does not contain an
                                           // options area of the specified
                                           // size 'optionsAreaSize'.
        ,
        rc_DUPLICATE_OPTION_TYPE = -3  // The blob contains a duplicate
                                       // option type.

    };

    mwcu::BlobIterator blobIter(blob, optionsAreaPos, optionsAreaSize, true);
    while (!blobIter.atEnd()) {
        mwcu::BlobObjectProxy<OptionHeader> oh(blobIter.blob(),
                                               blobIter.position());

        if (!oh.isSet()) {
            // We could not read an OptionHeader. For example,
            // 'blobIter.position() + sizeof(OptionHeader)' extends past the
            // end of the blob.
            clear();
            return rc_INVALID_OPTIONS_AREA_SIZE;  // RETURN
        }

        const int optionSize = oh->packed()
                                   ? sizeof(OptionHeader)
                                   : (oh->words() * Protocol::k_WORD_SIZE);
        if (!oh->packed() && optionSize == sizeof(OptionHeader)) {
            // At this time we don't support an non-packed option header that
            // is not followed by an option payload.
            clear();
            return rc_INVALID_OPTIONS_AREA;  // RETURN
        }

        if (optionSize > blobIter.remaining()) {
            // The OptionHeader 'Words' field disagrees with the blob iterator
            // on the number of bytes remaining (we side with iter because it
            // expects the specified 'optionsAreaSize' to be correct
            clear();
            return rc_INVALID_OPTIONS_AREA;  // RETURN
        }

        const OptionType::Enum& optionType = oh->type();
        // NOTE: This OptionHeader might be corrupt and yet still be in a valid
        // range.  This check is not a safeguard against that possibility.
        // Also, it is possible that, due to invalid formatting of the options
        // area, the chunk of memory we consider to be an OptionHeader is not
        // in fact an OptionHeader (for instance, if we are reading a word that
        // was intended to be a sub-queue id but because of an incorrect
        // 'words' field in a previous OptionHeader we advanced to it).
        const int optionTypeAsInt = static_cast<int>(optionType);
        if (optionTypeAsInt < OptionType::k_LOWEST_SUPPORTED_TYPE ||
            optionTypeAsInt > OptionType::k_HIGHEST_SUPPORTED_TYPE) {
            // An option type is not valid unless it is in the range
            // (OptionType::e_UNDEFINED, OptionType::e_HIGHEST_SUPPORTED_TYPE]
            clear();
            return rc_INVALID_OPTIONS_AREA;  // RETURN
        }

        // Insert BlobPosition for this OptionType
        if (d_optionPositions[optionType] != d_NullBlobPosition) {
            // We already have an option of this type (we processed it at a
            // previous iteration of this loop), which should not occur.
            clear();
            return rc_DUPLICATE_OPTION_TYPE;  // RETURN
        }
        // We don't need bounds-checking because the vector is always of size
        // 'OptionHeader::k_MAX_TYPE' + 1.
        d_optionPositions[optionType] = blobIter.position();

        // At this point we have validated this OptionHeader (but not the
        // option itself).
        optionsAreaSize -= optionSize;
        blobIter.advance(optionSize);
    }

    // NOTE: Not sure if this scenario is even possible.
    if (optionsAreaSize != 0) {
        // We've reached the end of the iterator while not advancing by exactly
        // the 'optionsAreaSize' number of bytes.
        BSLS_ASSERT_SAFE(false &&
                         "'optionsAreaSize' should have been exhausted");
        clear();
        return rc_INVALID_OPTIONS_AREA_SIZE;  // RETURN
    }

    // Everything went fine, so let's set a valid state
    d_blob_p  = blob;
    d_isValid = true;

    return rc_SUCCESS;
}

// PRIVATE ACCESSORS
int OptionsView::loadOptionPositionAndSize(
    mwcu::BlobPosition*          payloadPosition,
    int*                         payloadSizeBytes,
    const bmqp::OptionType::Enum optionType,
    const bool                   hasPadding) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(payloadPosition);
    BSLS_ASSERT_SAFE(payloadSizeBytes);
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(find(optionType) != end());

    const mwcu::BlobPosition& headerPos = d_optionPositions[optionType];
    mwcu::BlobObjectProxy<OptionHeader> oh(d_blob_p, headerPos);

    BSLS_ASSERT_SAFE(oh.isSet() && "Option header not set");
    // should be ok, as in the constructor

    int rc = mwcu::BlobUtil::findOffsetSafe(payloadPosition,
                                            *d_blob_p,
                                            headerPos,
                                            sizeof(OptionHeader));
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc;  // RETURN
    }

    if (oh->packed()) {
        // *Packed* options
        *payloadSizeBytes = 0;
        return rc;  // RETURN
    }

    // 1 word for header, rest for payload.
    const int words = oh->words();
    BSLS_ASSERT_SAFE((words > 1) && "Expect positive number of words");
    const int optionTotalSizeBytes = words * Protocol::k_WORD_SIZE;
    const int optionLenBytes       = (oh->words() - 1) * Protocol::k_WORD_SIZE;

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!hasPadding)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        *payloadSizeBytes = optionLenBytes;
        return rc;  // RETURN
    }

    mwcu::BlobPosition lastBytePos;
    rc = mwcu::BlobUtil::findOffsetSafe(&lastBytePos,
                                        *d_blob_p,
                                        headerPos,
                                        optionTotalSizeBytes - 1);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc;  // RETURN
    }

    const char lastByte =
        (*d_blob_p).buffer(lastBytePos.buffer()).data()[lastBytePos.byte()];

    *payloadSizeBytes = optionLenBytes - lastByte;

    return rc;
}

// MANIPULATORS
void OptionsView::dumpBlob(bsl::ostream& stream)
{
    // PRECONDITIONS
    static const int k_MAX_BYTES_DUMP = 128;

    // For now, print only the beginning of the blob.. we may later on print
    // also the bytes around the current position.
    if (d_blob_p) {
        stream << mwcu::BlobStartHexDumper(d_blob_p, k_MAX_BYTES_DUMP);
    }
    else {
        stream << "/no blob/";
    }
}

// ACCESSORS
int OptionsView::loadSubQueueIdsOption(
    Protocol::SubQueueIdsArrayOld* subQueueIdsOld) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(subQueueIdsOld);
    BSLS_ASSERT_SAFE(subQueueIdsOld->empty());
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(find(OptionType::e_SUB_QUEUE_IDS_OLD) != end());

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS      = 0,
        rc_LOAD_FAILURE = -1
    };

    // Read in the SubQueueIds as BigEndianUint32 and then convert them to
    // unsigned integers.
    bdlma::LocalSequentialAllocator<16 * sizeof(bdlb::BigEndianUint32)>
                                       bufferedAllocator(d_allocator_p);
    bsl::vector<bdlb::BigEndianUint32> tempVec(&bufferedAllocator);

    const int rc = loadSubQueueInfosOptionHelper(
        &tempVec,
        Protocol::k_WORD_SIZE,
        OptionType::e_SUB_QUEUE_IDS_OLD);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_LOAD_FAILURE;  // RETURN
    }

    // Populate SubQueueIds
    subQueueIdsOld->assign(tempVec.begin(), tempVec.end());

    return rc_SUCCESS;
}

int OptionsView::loadSubQueueInfosOption(
    Protocol::SubQueueInfosArray* subQueueInfos) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(subQueueInfos);
    BSLS_ASSERT_SAFE(subQueueInfos->empty());
    BSLS_ASSERT_SAFE(isValid());

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS      = 0,
        rc_LOAD_FAILURE = -1,
        rc_UNREACHABLE  = -2
    };

    if (find(OptionType::e_SUB_QUEUE_INFOS) != end()) {
        const mwcu::BlobPosition& headerPos =
            d_optionPositions[OptionType::e_SUB_QUEUE_INFOS];
        mwcu::BlobObjectProxy<OptionHeader> oh(d_blob_p, headerPos);
        BSLS_ASSERT_SAFE(oh.isSet() && "Option header not set");
        BSLS_ASSERT_SAFE(oh->type() == OptionType::e_SUB_QUEUE_INFOS);

        if (oh->packed()) {
            // *Packed* options => reinterpret 'words' field as RDA counter for
            //                     the default Subscription Id
            const unsigned int rdaCounter = static_cast<unsigned int>(
                oh->words());
            subQueueInfos->push_back(
                SubQueueInfo(bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID,
                             rdaCounter));

            return rc_SUCCESS;  // RETURN
        }

        // Read in the encoded SubQueueInfos and then decode them into the
        // output.
        bdlma::LocalSequentialAllocator<16 * sizeof(SubQueueInfo)>
                                  bufferedAllocator(d_allocator_p);
        bsl::vector<SubQueueInfo> tempVec(&bufferedAllocator);

        const unsigned int itemSize = static_cast<unsigned int>(
                                          oh->typeSpecific()) *
                                      Protocol::k_WORD_SIZE;

        int rc = loadSubQueueInfosOptionHelper(&tempVec,
                                               itemSize,
                                               OptionType::e_SUB_QUEUE_INFOS);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            return rc_LOAD_FAILURE;  // RETURN
        }

        // Populate SubQueueInfos
        subQueueInfos->assign(tempVec.cbegin(), tempVec.cend());

        return rc_SUCCESS;  // RETURN
    }
    else if (find(OptionType::e_SUB_QUEUE_IDS_OLD) != end()) {
        // Read in the SubQueueIds as BigEndianUint32 and then convert them to
        // unsigned integers.
        bdlma::LocalSequentialAllocator<16 * sizeof(bdlb::BigEndianUint32)>
                                           bufferedAllocator(d_allocator_p);
        bsl::vector<bdlb::BigEndianUint32> tempVec(&bufferedAllocator);

        int rc = loadSubQueueInfosOptionHelper(
            &tempVec,
            Protocol::k_WORD_SIZE,
            OptionType::e_SUB_QUEUE_IDS_OLD);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            return rc_LOAD_FAILURE;  // RETURN
        }

        // Populate SubQueueInfos
        for (bsl::vector<bdlb::BigEndianUint32>::const_iterator cit =
                 tempVec.cbegin();
             cit != tempVec.cend();
             ++cit) {
            subQueueInfos->push_back(SubQueueInfo(*cit));
        }

        return rc_SUCCESS;  // RETURN
    }

    BSLS_ASSERT_SAFE(false && "SubQueueInfos not found");
    return rc_UNREACHABLE;
}

int OptionsView::loadMsgGroupIdOption(Protocol::MsgGroupId* msgGroupId) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(msgGroupId);
    BSLS_ASSERT_SAFE(msgGroupId->empty());
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(find(OptionType::e_MSG_GROUP_ID) != end());

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS           = 0,
        rc_INVALID_BLOB      = -1,
        rc_INSUFFICIENT_DATA = -2,
        rc_INVALID_LENGTH    = -3
    };

    mwcu::BlobPosition msgGroupIdStartPos;
    int                msgGroupIdSizeBytes;
    int                rc = loadOptionPositionAndSize(&msgGroupIdStartPos,
                                       &msgGroupIdSizeBytes,
                                       OptionType::e_MSG_GROUP_ID,
                                       true);  // hasPadding
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Will happen if the range
        // [headerPos, headerPos + sizeof(OptionHeader)] does not fall within
        // the blob.
        return rc_INSUFFICIENT_DATA;  // RETURN
    }

    // If there's no Group Id information, the OptionHeader shouldn't be set.
    // Empty Group Ids are currently invalid.
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(msgGroupIdSizeBytes == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_INVALID_LENGTH;  // RETURN
    }

    // The maximum allowed value is Protocol::k_MSG_GROUP_ID_MAX_LENGTH.
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            msgGroupIdSizeBytes > Protocol::k_MSG_GROUP_ID_MAX_LENGTH)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_INVALID_LENGTH;  // RETURN
    }

    // Read msgGroupId
    msgGroupId->resize(msgGroupIdSizeBytes);
    rc = mwcu::BlobUtil::readNBytes(&(*msgGroupId)[0],
                                    *d_blob_p,
                                    msgGroupIdStartPos,
                                    msgGroupIdSizeBytes);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        msgGroupId->clear();
        // The range
        // [msgGroupIdStartPos, msgGroupIdStartPos + msgGroupIdSizeBytes]
        // doesn't fall within the blob.
        return rc_INVALID_BLOB;  // RETURN
    }

    return rc_SUCCESS;
}

}  // close package namespace
}  // close enterprise namespace
