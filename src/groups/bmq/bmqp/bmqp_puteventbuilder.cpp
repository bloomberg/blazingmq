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

// bmqp_puteventbuilder.cpp                                           -*-C++-*-
#include <bmqp_puteventbuilder.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqp_compression.h>
#include <bmqp_crc32c.h>
#include <bmqp_optionutil.h>
#include <bmqp_protocolutil.h>

// MWC
#include <mwcu_blob.h>
#include <mwcu_blobobjectproxy.h>
#include <mwcu_memoutstream.h>

// BDE
#include <bdlb_scopeexit.h>
#include <bdlbb_blobutil.h>
#include <bdlf_bind.h>
#include <bsl_cstring.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_annotation.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqp {

namespace {
#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
bool isWordAligned(const bdlbb::Blob& blob)
{
    return (blob.length() % Protocol::k_WORD_SIZE) == 0;
}
#endif
}  // close unnamed namespace

// ---------------------
// class PutEventBuilder
// ---------------------

void PutEventBuilder::resetFields(void* ptr)
{
    PutEventBuilder* builder = static_cast<PutEventBuilder*>(ptr);
    builder->d_flags         = 0;
    builder->d_messageGUID   = bmqt::MessageGUID();
    builder->d_crc32c        = 0;
}

bmqt::EventBuilderResult::Enum
PutEventBuilder::packMessageInternal(const bdlbb::Blob& appData, int queueId)
{
    typedef bmqt::EventBuilderResult Result;
    typedef OptionUtil::OptionMeta   OptionMeta;

    int       appDataLength   = appData.length();
    int       numPaddingBytes = 0;
    const int numWords = ProtocolUtil::calcNumWordsAndPadding(&numPaddingBytes,
                                                              appDataLength);

    // Validate payload is not too big
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            appDataLength > PutHeader::k_MAX_PAYLOAD_SIZE_SOFT)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return Result::e_PAYLOAD_TOO_BIG;  // RETURN
    }

    const int sizeNoOptions = eventSize() + sizeof(PutHeader) + appDataLength +
                              numPaddingBytes;

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(sizeNoOptions >
                                              PutHeader::k_MAX_SIZE_SOFT)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return Result::e_EVENT_TOO_BIG;  // RETURN
    }

    // Add the PutHeader
    mwcu::BlobPosition offset;
    mwcu::BlobUtil::reserve(&offset, d_blob_sp.get(), sizeof(PutHeader));
    mwcu::BlobObjectProxy<PutHeader> putHeader(d_blob_sp.get(),
                                               offset,
                                               false,  // no read
                                               true);  // write mode
    // Make sure memory is reset
    bsl::memset(static_cast<void*>(putHeader.object()),
                0,
                sizeof(bmqp::PutHeader));

    // Add option(s) if they fit.
    OptionUtil::OptionsBox optionBox;
    if (!d_msgGroupId.isNull()) {
        const OptionMeta msgGroupId = OptionMeta::forOptionWithPadding(
            OptionType::e_MSG_GROUP_ID,
            d_msgGroupId.value().length());
        Result::Enum res = OptionUtil::isValidMsgGroupId(d_msgGroupId.value());
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(res != Result::e_SUCCESS)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            return res;  // RETURN
        }
        res = optionBox.canAdd(sizeNoOptions, msgGroupId);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(res != Result::e_SUCCESS)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            return res;  // RETURN
        }
        optionBox.add(
            d_blob_sp.get(),
            reinterpret_cast<const char*>(d_msgGroupId.value().data()),
            msgGroupId);
    }

    const int headerWords = sizeof(bmqp::PutHeader) / Protocol::k_WORD_SIZE;
    const int optionsSize = optionBox.size();
    BSLS_ASSERT_SAFE(0 == optionsSize % Protocol::k_WORD_SIZE);
    const int optionsWords = optionsSize / Protocol::k_WORD_SIZE;

    (*putHeader)
        .setHeaderWords(headerWords)
        .setOptionsWords(optionsWords)
        .setQueueId(queueId)
        .setMessageWords(headerWords + optionsWords + numWords)
        .setFlags(d_flags)
        .setCompressionAlgorithmType(d_compressionAlgorithmType)
        .setCrc32c(d_crc32c);

    d_messagePropertiesInfo.applyTo(putHeader.object());

    BSLS_ASSERT_SAFE(!d_messageGUID.isUnset());

    putHeader->setMessageGUID(d_messageGUID);
    putHeader.reset();  // i.e., flush writing to blob..

    // Just a sanity test.  Should still be word aligned.
    BSLS_ASSERT_SAFE(isWordAligned(*d_blob_sp));

    bdlbb::BlobUtil::append(d_blob_sp.get(), appData);

    // Add padding
    ProtocolUtil::appendPaddingRaw(d_blob_sp.get(), numPaddingBytes);

    ++d_msgCount;

    return Result::e_SUCCESS;
}

PutEventBuilder::PutEventBuilder(bdlbb::BlobBufferFactory* bufferFactory,
                                 bslma::Allocator*         allocator)
: d_allocator_p(bslma::Default::allocator(allocator))
, d_bufferFactory_p(bufferFactory)
, d_blob_sp(0, allocator)  // initialized in `reset()`
, d_msgStarted(false)
, d_blobPayload_p(0)
, d_rawPayload_p(0)
, d_rawPayloadLength(0)
, d_properties_p(0)
, d_flags(0)
, d_messageGUID()
, d_msgGroupId(allocator)
, d_msgCount(0)
, d_crc32c(0)
, d_compressionAlgorithmType(bmqt::CompressionAlgorithmType::e_NONE)
, d_lastPackedMessageCompressionRatio(-1)
, d_messagePropertiesInfo()
{
    reset();
}

int PutEventBuilder::reset()
{
    d_blob_sp.createInplace(d_allocator_p, d_bufferFactory_p, d_allocator_p);
    d_msgStarted       = false;
    d_blobPayload_p    = 0;
    d_rawPayload_p     = 0;
    d_rawPayloadLength = 0;
    d_properties_p     = 0;
    d_flags            = 0;
    d_msgGroupId.reset();
    d_messageGUID                       = bmqt::MessageGUID();
    d_msgCount                          = 0;
    d_crc32c                            = 0;
    d_lastPackedMessageCompressionRatio = -1;
    d_messagePropertiesInfo             = MessagePropertiesInfo();

    // NOTE: Since PutEventBuilder owns the blob and we just reset it, we have
    //       guarantee that buffer(0) will contain the entire header (unless
    //       the bufferFactory has blobs of ridiculously small size !!)

    // Ensure blob has enough space for an EventHeader
    //
    // Use placement new to create the object directly in the blob buffer,
    // while still calling it's constructor (to memset memory and initialize
    // some fields)
    d_blob_sp->setLength(sizeof(EventHeader));
    new (d_blob_sp->buffer(0).data()) EventHeader(EventType::e_PUT);

    return 0;
}

bmqt::EventBuilderResult::Enum
PutEventBuilder::packMessageInOldStyle(int queueId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_msgStarted);

    typedef bmqt::EventBuilderResult Result;

    // Guid and flags need to be reset after this method (irrespective of its
    // success or failure).  Create a proctor to auto reset them.
    const bsl::function<void()> f =
        bdlf::BindUtil::bind(&PutEventBuilder::resetFields, this);
    bdlb::ScopeExitAny resetter(f);

    // Calculate length of entire application data (includes payload, message
    // properties and padding, if any).
    bdlbb::Blob applicationData(d_bufferFactory_p, d_allocator_p);

    const bdlbb::Blob* propertiesBlob = 0;
    if (d_properties_p && 0 != d_properties_p->numProperties()) {
        // Note that '0 != d_properties_p->numProperties()' check is required
        // because application can set an empty instance of properties with a
        // message.

        // k_NO_SCHEMA (0) communicates the old style of compressing MPs
        d_messagePropertiesInfo = MessagePropertiesInfo::makeNoSchema();

        // propertiesBlob include 6 byte mph along with properties
        propertiesBlob = &(d_properties_p->streamOut(d_bufferFactory_p,
                                                     d_messagePropertiesInfo));

        bdlbb::BlobUtil::append(&applicationData, *propertiesBlob);
    }
    else {
        BSLS_ASSERT_SAFE(!d_messagePropertiesInfo.isPresent());
    }

    // Add the payload
    if (d_rawPayload_p) {
        bdlbb::BlobUtil::append(&applicationData,
                                d_rawPayload_p,
                                d_rawPayloadLength);
    }
    else {
        bdlbb::BlobUtil::append(&applicationData, *d_blobPayload_p);
    }

    // Compress
    if (applicationData.length() >= Protocol::k_COMPRESSION_MIN_APPDATA_SIZE &&
        d_compressionAlgorithmType != bmqt::CompressionAlgorithmType::e_NONE) {
        bdlbb::Blob        compressedApplicationData(d_bufferFactory_p,
                                              d_allocator_p);
        mwcu::MemOutStream error(d_allocator_p);

        int rc = Compression::compress(&compressedApplicationData,
                                       d_bufferFactory_p,
                                       d_compressionAlgorithmType,
                                       applicationData,
                                       &error,
                                       d_allocator_p);
        if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(
                rc == Result::e_SUCCESS && compressedApplicationData.length() <
                                               applicationData.length())) {
            // Compression is successful and is worth using!
            d_crc32c = Crc32c::calculate(compressedApplicationData);

            // Keep track of the compression ratio.
            d_lastPackedMessageCompressionRatio =
                static_cast<double>(applicationData.length()) /
                compressedApplicationData.length();

            return packMessageInternal(compressedApplicationData,
                                       queueId);  // RETURN
        }
    }

    BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

    // Either the compression failed, or the resulting blob was bigger and not
    // worth using. In either way, we fall back to using the original blob.
    // Explicitly set the 'd_compressionAlgorithmType' to 'NONE'.
    d_compressionAlgorithmType = bmqt::CompressionAlgorithmType::e_NONE;
    d_crc32c                   = Crc32c::calculate(applicationData);
    d_lastPackedMessageCompressionRatio = 1;

    return packMessageInternal(applicationData, queueId);
}

bmqt::EventBuilderResult::Enum PutEventBuilder::packMessage(int queueId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_msgStarted);

    typedef bmqt::EventBuilderResult Result;

    // CorrelationId, guid and flags need to be reset after this method
    // (irrespective of its success or failure).  Create a proctor to auto
    // reset them.
    const bsl::function<void()> f =
        bdlf::BindUtil::bind(&PutEventBuilder::resetFields, this);
    bdlb::ScopeExitAny resetter(f);

    // Calculate length of entire application data (includes payload, message
    // properties and padding, if any).
    bdlbb::Blob        bufferBlob(d_bufferFactory_p, d_allocator_p);
    bdlbb::Blob        resultBlob(d_bufferFactory_p, d_allocator_p);
    const bdlbb::Blob* payloadBlob = d_blobPayload_p;

    if (d_properties_p && 0 != d_properties_p->numProperties()) {
        // Note that '0 != d_properties_p->numProperties()' check is required
        // because application can set an empty instance of properties with a
        // message.

        if (!d_messagePropertiesInfo.isExtended()) {
            // Did not 'setMessagePropertiesLogic'.
            d_messagePropertiesInfo =
                MessagePropertiesInfo::makeInvalidSchema();
        }

        // propertiesBlob include 6 byte mph along with properties
        const bdlbb::Blob& propertiesBlob = d_properties_p->streamOut(
            d_bufferFactory_p,
            d_messagePropertiesInfo);

        bdlbb::BlobUtil::append(&resultBlob, propertiesBlob);
    }
    else {
        BSLS_ASSERT_SAFE(!d_messagePropertiesInfo.isPresent());
    }

    // Add the payload
    if (d_rawPayload_p) {
        bdlbb::BlobUtil::append(&bufferBlob,
                                d_rawPayload_p,
                                d_rawPayloadLength);
        payloadBlob = &bufferBlob;
    }
    else {
        payloadBlob = d_blobPayload_p;
    }

    // Compress
    if (payloadBlob->length() >= Protocol::k_COMPRESSION_MIN_APPDATA_SIZE &&
        d_compressionAlgorithmType != bmqt::CompressionAlgorithmType::e_NONE) {
        bdlbb::Blob compressedPayloadBlob(d_bufferFactory_p, d_allocator_p);
        mwcu::MemOutStream error(d_allocator_p);

        int rc = Compression::compress(&compressedPayloadBlob,
                                       d_bufferFactory_p,
                                       d_compressionAlgorithmType,
                                       *payloadBlob,
                                       &error,
                                       d_allocator_p);
        if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(
                rc == Result::e_SUCCESS &&
                compressedPayloadBlob.length() < payloadBlob->length())) {
            // Compression is successful and is worth using!

            // Keep track of the compression ratio.
            d_lastPackedMessageCompressionRatio =
                static_cast<double>(payloadBlob->length()) /
                compressedPayloadBlob.length();
            bdlbb::BlobUtil::append(&resultBlob, compressedPayloadBlob);
            d_crc32c = Crc32c::calculate(resultBlob);

            return packMessageInternal(resultBlob, queueId);  // RETURN
        }
    }

    // Either no compression, or the compression failed, or the resulting blob
    // was bigger and not worth using. In either way, we fall back to using the
    // original blob. Explicitly set the 'd_compressionAlgorithmType' to
    // 'NONE'.
    bdlbb::BlobUtil::append(&resultBlob, *payloadBlob);

    d_compressionAlgorithmType = bmqt::CompressionAlgorithmType::e_NONE;
    d_crc32c                   = Crc32c::calculate(resultBlob);
    d_lastPackedMessageCompressionRatio = 1;

    return packMessageInternal(resultBlob, queueId);
}

bmqt::EventBuilderResult::Enum PutEventBuilder::packMessageRaw(int queueId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_msgStarted);

    // Guid and flags need to be reset after this method (irrespective of its
    // success or failure).  Create a proctor to auto reset them.
    const bsl::function<void()> f =
        bdlf::BindUtil::bind(&PutEventBuilder::resetFields, this);
    bdlb::ScopeExitAny resetter(f);

    // Note that the 'd_blobPayload_p' has the entire application data.
    return packMessageInternal(*d_blobPayload_p, queueId);
}

const bdlbb::Blob& PutEventBuilder::blob() const
{
    // Fix packet's length in header now that we know it ..  Following is valid
    // (see comment in reset)
    EventHeader& eh = *reinterpret_cast<EventHeader*>(
        d_blob_sp->buffer(0).data());
    eh.setLength(d_blob_sp->length());

    return *d_blob_sp;
}

bsl::shared_ptr<bdlbb::Blob> PutEventBuilder::blob_sp() const
{
    // Fix packet's length in header now that we know it ..  Following is valid
    // (see comment in reset)
    EventHeader& eh = *reinterpret_cast<EventHeader*>(
        d_blob_sp->buffer(0).data());
    eh.setLength(d_blob_sp->length());

    return d_blob_sp;
}

const bmqp::MessageProperties* PutEventBuilder::messageProperties() const
{
    return d_properties_p;
}

}  // close package namespace
}  // close enterprise namespace
