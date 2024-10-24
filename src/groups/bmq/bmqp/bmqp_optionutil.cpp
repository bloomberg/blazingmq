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

// bmqp_optionutil.cpp                                                -*-C++-*-
#include <bmqp_optionutil.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqp_protocolutil.h>

#include <bmqu_blobobjectproxy.h>

// BDE
#include <bdlbb_blobutil.h>
#include <bsl_cstring.h>  // for bsl::memset
#include <bslmf_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqp {

// -----------------------------
// struct OptionUtil::OptionMeta
// -----------------------------

// CLASS METHODS
OptionUtil::OptionMeta
OptionUtil::OptionMeta::forOption(const OptionType::Enum type,
                                  const int              size,
                                  bool                   packed,
                                  int                    packedValue,
                                  unsigned char          typeSpecific)
{
    // This isn't strictly necessary out of the protocol, but it's true with
    // whatever we use up to now.  I keep it here so that if/when this changes
    // it becomes consciously.  It's possible that extra padding will need to
    // be added at the end of the Options section if non-aligned option payload
    // is allowed.
    BSLS_ASSERT_SAFE(packed || (0 == size % Protocol::k_WORD_SIZE));

    return OptionMeta(type, size, 0, packed, packedValue, typeSpecific);
}

OptionUtil::OptionMeta
OptionUtil::OptionMeta::forOptionWithPadding(const OptionType::Enum type,
                                             const int              size)
{
    BSLS_ASSERT_SAFE(type != OptionType::e_UNDEFINED);

    int padding = 0;
    ProtocolUtil::calcNumWordsAndPadding(&padding, size);
    return OptionMeta(type, size, padding);
}

OptionUtil::OptionMeta OptionUtil::OptionMeta::forNullOption()
{
    return OptionMeta(OptionType::e_UNDEFINED, 0, 0);
}

// CREATORS
OptionUtil::OptionMeta::OptionMeta(const OptionType::Enum type,
                                   const int              payloadSize,
                                   const int              padding,
                                   bool                   packed,
                                   int                    packedValue,
                                   unsigned char          typeSpecific)
: d_type(type)
, d_payloadSize(payloadSize)
, d_padding(padding)
, d_packed(packed)
, d_packedValue(packedValue)
, d_typeSpecific(typeSpecific)
{
}

// -----------------------------
// struct OptionUtil::OptionsBox
// -----------------------------

// CREATORS
OptionUtil::OptionsBox::OptionsBox()
: d_optionsSize(0)
, d_optionsCount(0)
{
}

// MANIPULATORS
void OptionUtil::OptionsBox::add(bdlbb::Blob*      blob,
                                 const char*       payload,
                                 const OptionMeta& option)
{
    // Write Option Header
    bmqu::BlobPosition ohOffset;
    bmqu::BlobUtil::reserve(&ohOffset, blob, sizeof(OptionHeader));

    bmqu::BlobObjectProxy<OptionHeader> optionHeader(blob,
                                                     ohOffset,
                                                     false,  // no read
                                                     true);  // write mode

    BSLS_ASSERT_SAFE(optionHeader.isSet());
    // Should never fail because we just reserved enough space

    BSLS_ASSERT_SAFE(0 == option.size() % Protocol::k_WORD_SIZE);
    // Sanity test

    bsl::memset(static_cast<void*>(optionHeader.object()),
                0,
                sizeof(OptionHeader));
    optionHeader->setType(option.type());
    optionHeader->setPacked(option.packed());

    const int optionSize  = option.size();
    const int payloadSize = option.payloadSize();
    if (option.packed()) {
        // If the option is packed, then there is no payload and the 'words'
        // field is used for a type-specific purpose and is directly set to the
        // 'packedValue' attribute.
        optionHeader->setWords(option.packedValue());
    }
    else {
        optionHeader->setWords(option.size() / Protocol::k_WORD_SIZE);
        optionHeader->setTypeSpecific(option.typeSpecific());
    }

    // Write option payload
    if (payloadSize > 0) {
        bdlbb::BlobUtil::append(blob, payload, payloadSize);
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(option.padding())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Write padding
        ProtocolUtil::appendPadding(blob, payloadSize);
    }

    ++d_optionsCount;
    d_optionsSize += optionSize;
}

// ACCESSORS
bmqt::EventBuilderResult::Enum
OptionUtil::OptionsBox::canAdd(const int         currentSize,
                               const OptionMeta& option) const
{
    // At minimum, a header is expected
    BSLS_ASSERT_SAFE(currentSize >= bmqp::Protocol::k_WORD_SIZE);

    const int size = option.size();
    // Validate option size vs. maximum allowed per option
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(size >
                                              OptionHeader::k_MAX_SIZE)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // The total size of this option would exceed the maximum allowed.
        return bmqt::EventBuilderResult::e_OPTION_TOO_BIG;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_optionsCount + 1 >
                                              OptionHeader::k_MAX_TYPE)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // More options than the maximum allowed.
        return bmqt::EventBuilderResult::e_UNKNOWN;  // RETURN
    }

    const int allOptionsSize = d_optionsSize + size;

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(allOptionsSize >
                                              Protocol::k_MAX_OPTIONS_SIZE)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // The total options size would exceed the enforced maximum.
        return bmqt::EventBuilderResult::e_OPTION_TOO_BIG;  // RETURN
    }

    const int evtSize = currentSize + allOptionsSize;

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(evtSize >
                                              EventHeader::k_MAX_SIZE_SOFT)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // The total size of this event would exceed the enforced maximum.
        return bmqt::EventBuilderResult::e_OPTION_TOO_BIG;  // RETURN
    }

    // We can safely add this option.
    return bmqt::EventBuilderResult::e_SUCCESS;
}

bool OptionUtil::loadOptionsPosition(int*                      optionsSize,
                                     bmqu::BlobPosition*       optionsPosition,
                                     const bdlbb::Blob&        blob,
                                     const int                 headerWords,
                                     const int                 optionsWords,
                                     const bmqu::BlobPosition& startPosition)
{
    const int headerSize    = headerWords * Protocol::k_WORD_SIZE;
    const int myOptionsSize = optionsWords * Protocol::k_WORD_SIZE;

    // At the moment, this is called always, so expected that no options exist
    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(myOptionsSize == 0)) {
        return true;  // RETURN
    }

    // Options exist.

    // Find start position of options area
    bmqu::BlobPosition myOptionsPosition;
    int                rc = bmqu::BlobUtil::findOffsetSafe(&myOptionsPosition,
                                            blob,
                                            startPosition,
                                            headerSize);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return false;  // RETURN
    }

    // Ensure that 'blob' contains 'myOptionsSize' bytes starting at
    // 'myOptionsPosition'.
    bmqu::BlobPosition optionsAreaEndPos;
    rc = bmqu::BlobUtil::findOffsetSafe(&optionsAreaEndPos,
                                        blob,
                                        myOptionsPosition,
                                        myOptionsSize);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return false;  // RETURN
    }

    // Validation was performed; set the state.
    *optionsPosition = myOptionsPosition;
    *optionsSize     = myOptionsSize;

    return true;
}

bmqt::EventBuilderResult::Enum
OptionUtil::isValidMsgGroupId(const Protocol::MsgGroupId& msgGroupId)
{
#ifdef BMQ_ENABLE_MSG_GROUPID
    const int length = msgGroupId.length();
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(length == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::EventBuilderResult::e_INVALID_MSG_GROUP_ID;  // RETURN
    }
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            length > Protocol::k_MSG_GROUP_ID_MAX_LENGTH)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::EventBuilderResult::e_INVALID_MSG_GROUP_ID;  // RETURN
    }
    return bmqt::EventBuilderResult::e_SUCCESS;
#else
    (void)msgGroupId;
    return bmqt::EventBuilderResult::e_UNKNOWN;
#endif
}

}  // close package namespace
}  // close enterprise namespace
