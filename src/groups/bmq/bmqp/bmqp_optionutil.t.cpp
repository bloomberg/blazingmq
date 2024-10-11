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

// bmqp_optionutil.t.cpp                                              -*-C++-*-
#include <bmqp_optionutil.h>

// BMQ
#include <bmqp_protocolutil.h>
#include <bmqt_resultcode.h>

// BDE
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

typedef bsl::pair<int, bmqt::EventBuilderResult::Enum> LimitT;

LimitT maxCanBeAdded(const int contentSize, const int payloadSize)
{
    typedef bmqp::OptionUtil::OptionMeta OptionMeta;
    typedef bmqp::OptionUtil::OptionsBox OptionsBox;
    typedef bmqt::EventBuilderResult     Result;

    // Arbitrary value
    const bmqp::OptionType::Enum type = bmqp::OptionType::e_SUB_QUEUE_IDS_OLD;

    bdlbb::PooledBlobBufferFactory bufferFactory(
        bmqp::EventHeader::k_MAX_SIZE_SOFT,
        s_allocator_p);
    bdlbb::Blob      blob(&bufferFactory, s_allocator_p);
    bsl::string      payload(payloadSize, 'a', s_allocator_p);
    const OptionMeta meta = OptionMeta::forOption(type, payloadSize);
    OptionsBox       box;
    LimitT           limit(0, Result::e_SUCCESS);
    while (true) {
        // Check if I can add it
        limit.second = box.canAdd(contentSize, meta);
        if (limit.second != Result::e_SUCCESS) {
            break;
        }

        // Add it.
        box.add(&blob, payload.c_str(), meta);

        ++limit.first;
    }

    return limit;
}

bmqp::OptionUtil::OptionMeta appendOption(bmqp::OptionUtil::OptionsBox* box,
                                          bdlbb::Blob*                  blob,
                                          const bmqp::OptionType::Enum  type,
                                          const bsl::string& payload,
                                          const int          currentSize,
                                          const bool         padding)
{
    typedef bmqp::OptionUtil::OptionMeta OptionMeta;
    typedef bmqt::EventBuilderResult     Result;

    const int payloadSize = payload.length();
    if (!padding) {
        BSLS_ASSERT_SAFE(0 == payloadSize % bmqp::Protocol::k_WORD_SIZE);
    }
    const OptionMeta   meta   = padding
                                    ? OptionMeta::forOptionWithPadding(type,
                                                                   payloadSize)
                                    : OptionMeta::forOption(type, payloadSize);
    const Result::Enum result = box->canAdd(currentSize, meta);
    ASSERT_EQ(result, Result::e_SUCCESS);
    box->add(blob, payload.c_str(), meta);
    return meta;
}

const char* validateOption(const char*                         p,
                           const bmqp::OptionUtil::OptionMeta& meta,
                           const bsl::string&                  payload)
{
    // TODO_POISON_PILL Test for *packed* option when OptionsBox::add() is
    //                  updated

    // Now we're a the beginning of 'OptionHeader' and we should get the
    // 'Type' of the option shifted by two reserved bits.
    ASSERT_EQ(static_cast<char>(meta.type() << 2), *p);
    ++p;

    const int sizeWords = meta.size() / bmqp::Protocol::k_WORD_SIZE;

    // for the 1st size byte, the top 3 bits are reserved to zero.  For
    // subsequent bytes, they hold the appropriate part of the size value.
    ASSERT_EQ((sizeWords >> 16) & 0x1f, *p);
    ++p;
    ASSERT_EQ((sizeWords >> 8) & 0xff, *p);
    ++p;
    ASSERT_EQ(sizeWords, *p);
    ++p;

    // Payload follows. Optional padding follows
    ASSERT_EQ(0, bsl::memcmp(p, payload.c_str(), payload.length()));

    p += payload.length();

    // The optional padding bytes should all be equal to the size of padding.
    for (int i = 0; i < meta.padding(); ++i) {
        ASSERT_EQ(meta.padding(), *p);
        ++p;
    }
    return p;
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_basicOptionMetaProperties()
// ------------------------------------------------------------------------
//                       BASIC OPTIONMETA PROPERTIES
// ------------------------------------------------------------------------
//
// Concerns:
//   Verify that 'OptionMeta' for different types of Options have their
//   properties set as expected
//
// Testing:
//   -static OptionMeta OptionUtil::OptionMeta::forOption(
//                                      const bmqp::OptionType::Enum type,
//                                      const int                    size);
//   -static OptionMeta OptionUtil::OptionMeta::forOptionWithPadding(
//                                      const bmqp::OptionType::Enum type,
//                                      const int                    size);
//   -static OptionMeta OptionUtil::OptionMeta::forNullOption();
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("BASIC OPTIONMETA PROPERTIES");

    typedef bmqp::OptionUtil::OptionMeta OptionMeta;

    // Arbitrary value
    const bmqp::OptionType::Enum type = bmqp::OptionType::e_SUB_QUEUE_IDS_OLD;
    const int                    headerSize = sizeof(bmqp::OptionHeader);

    PV("A payload without padding")
    {
        for (int size = 0; size < 10; ++size) {
            if (0 != size % bmqp::Protocol::k_WORD_SIZE) {
                ASSERT_SAFE_FAIL(OptionMeta::forOption(type, size));
            }
            else {
                const OptionMeta meta = OptionMeta::forOption(type, size);

                ASSERT_EQ(false, meta.isNull());
                ASSERT_EQ(size, meta.payloadSize());
                ASSERT_EQ(size, meta.payloadEffectiveSize());
                ASSERT_EQ(size + headerSize, meta.size());
                ASSERT_EQ(0, meta.padding());
                ASSERT_EQ(type, meta.type());
            }
        }
    }

    PV("A payload with zero with padding")
    {
        for (int size = 0; size < 10; ++size) {
            const OptionMeta meta = OptionMeta::forOptionWithPadding(type,
                                                                     size);

            const int padding = 1 + (3 - size % 4);
            ASSERT_EQ(false, meta.isNull());
            ASSERT_EQ(size, meta.payloadSize());
            ASSERT_EQ(size + padding, meta.payloadEffectiveSize());
            ASSERT_EQ(size + padding + headerSize, meta.size());
            ASSERT_EQ(padding, meta.padding());
            ASSERT_EQ(type, meta.type());
        }
    }

    PV("A null option")
    {
        const OptionMeta meta = OptionMeta::forNullOption();

        ASSERT_EQ(true, meta.isNull());
        ASSERT_SAFE_FAIL(meta.payloadSize());
        ASSERT_SAFE_FAIL(meta.payloadEffectiveSize());
        ASSERT_SAFE_FAIL(meta.size());
        ASSERT_SAFE_FAIL(meta.padding());
        ASSERT_SAFE_FAIL(meta.type());
    }
}

static void test2_basicOptionsBoxCanAdd()
// ------------------------------------------------------------------------
//                            OPTIONSBOX CAN ADD
// ------------------------------------------------------------------------
//
// Concerns:
//   Verify that 'OptionsBox::canAdd()' returns the expected value for
//   various types of 'OptionMeta' and content sizes.
//
// Testing:
//   -bmqt::EventBuilderResult::Enum canAdd(
//                                         const int         currentSize,
//                                         const OptionMeta& option) const;
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("OPTIONSBOX CAN ADD");

    // Arbitrary value
    const bmqp::OptionType::Enum type = bmqp::OptionType::e_SUB_QUEUE_IDS_OLD;

    typedef bmqp::OptionUtil::OptionMeta OptionMeta;
    typedef bmqp::OptionUtil::OptionsBox OptionsBox;
    typedef bmqt::EventBuilderResult     Result;

    const int headerSize = sizeof(bmqp::OptionHeader);
    // Size step to support 32 iterations
    const int k_MAX_SIZE      = bmqp::OptionHeader::k_MAX_SIZE;
    const int k_MAX_TYPE      = bmqp::OptionHeader::k_MAX_TYPE;
    const int k_MAX_SIZE_SOFT = bmqp::EventHeader::k_MAX_SIZE_SOFT;
    const int k_WORD          = bmqp::Protocol::k_WORD_SIZE;
    // Size increment to support ~16 iterations rounded to a multiple of k_WORD
    const int sizeStep16 = ((k_MAX_SIZE / 16) / k_WORD) * k_WORD;

    PV("Rule 1. Fail with e_OPTION_TOO_BIG"
       "when payload + header > OptionHeader::k_MAX_SIZE")
    {
        const int contentSize = 4;

        for (int size = 0; size < k_MAX_SIZE + sizeStep16 + 1;
             size += sizeStep16) {
            OptionsBox         box;
            const OptionMeta   meta     = OptionMeta::forOption(type, size);
            const Result::Enum expected = (size + headerSize > k_MAX_SIZE)
                                              ? Result::e_OPTION_TOO_BIG
                                              : Result::e_SUCCESS;
            ASSERT_EQ(expected, box.canAdd(contentSize, meta));
        }
    }

    PV("Rule 2. Fail with e_TOO_MANY_OPTIONS"
       "when payload + header > OptionHeader::k_MAX_SIZE")
    {
        // Regardless of the content size, the problem is going to be that
        // we have too many small options in this case.
        const int smallPayload      = 12;  // Just 12 bytes
        const int maxAllowedContent = k_MAX_SIZE_SOFT -
                                      k_MAX_TYPE * (smallPayload + headerSize);
        const int oneTooMany         = maxAllowedContent + 1;
        const int testContentSizes[] = {4, maxAllowedContent, oneTooMany};
        for (unsigned int i = 0;
             i < (sizeof(testContentSizes) / sizeof(testContentSizes[0]));
             ++i) {
            const int    contentSize = testContentSizes[i];
            const LimitT limit = maxCanBeAdded(contentSize, smallPayload);
            if (contentSize == oneTooMany) {
                ASSERT_EQ(Result::e_OPTION_TOO_BIG, limit.second);
                ASSERT_EQ(k_MAX_TYPE - 1, limit.first);
            }
            else {
                ASSERT_EQ(Result::e_UNKNOWN, limit.second);
                ASSERT_EQ(k_MAX_TYPE, limit.first);
            }
        }
    }

    // This rule is no longer necessary (covered by Rule 3):
    //
    // PV("Rule 3. Fail with e_OPTION_TOO_BIG when payload + header +"
    // "the same for all other options > OptionHeader::k_MAX_OPTIONS_SIZE")

    PV("Rule 4. Fail with e_OPTION_TOO_BIG when size excluding options + size"
       "of all options > EventHeader::k_MAX_SIZE_SOFT")
    {
        const int testContentSizes[] = {4, 2 * k_MAX_SIZE};
        for (unsigned int i = 0;
             i < (sizeof(testContentSizes) / sizeof(testContentSizes[0]));
             ++i) {
            const int    contentSize = testContentSizes[i];
            const int    maxPayload  = k_MAX_SIZE - headerSize;
            const LimitT limit       = maxCanBeAdded(contentSize, maxPayload);
            ASSERT_EQ(Result::e_OPTION_TOO_BIG, limit.second);
        }
    }
}

static void test3_checkOptionsBlobSegment()
// ------------------------------------------------------------------------
//                        CHECK OPTIONS BLOB SEGMENT
// ------------------------------------------------------------------------
//
// Concerns:
//   Verify that 'OptionsBox::add()' builds a blob as expected or various
//   types of 'OptionMeta' and content sizes.
//
// Testing:
//   -void OptionUtil::OptionsBox::add(bdlbb::Blob        *blob,
//                                     const char        *payload,
//                                     const OptionMeta&  option);
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("CHECK OPTIONS BLOB SEGMENT");

    typedef bmqp::OptionUtil::OptionMeta OptionMeta;
    typedef bmqp::OptionUtil::OptionsBox OptionsBox;

    // Initialize the blob and add header
    bdlbb::PooledBlobBufferFactory bufferFactory(
        bmqp::EventHeader::k_MAX_SIZE_SOFT,
        s_allocator_p);
    bdlbb::Blob blob(&bufferFactory, s_allocator_p);
    bsl::string header     = "....";  // Fake headers
    int         headerSize = header.size();
    bdlbb::BlobUtil::append(&blob, header.c_str(), headerSize);

    // Add the first option (that needs padding)
    bsl::string      p1 = "1234";
    OptionsBox       box;
    const OptionMeta meta1 = appendOption(
        &box,
        &blob,
        bmqp::OptionType::e_SUB_QUEUE_IDS_OLD,
        p1,
        headerSize,
        true);

    // Add the second option (that needs no padding)
    bsl::string      p2    = " world!!";
    const OptionMeta meta2 = appendOption(
        &box,
        &blob,
        bmqp::OptionType::e_SUB_QUEUE_IDS_OLD,
        p2,
        headerSize + p1.size(),
        false);

    bsl::string canary = "canary";
    bdlbb::BlobUtil::append(&blob, canary.c_str(), canary.size());

    const int expectedSize = headerSize + meta1.size() + meta2.size() +
                             canary.size();
    ASSERT_EQ(expectedSize, blob.length());
    ASSERT_EQ(1, blob.numDataBuffers());
    ASSERT_EQ(0,
              bsl::memcmp(blob.buffer(0).data(), header.c_str(), headerSize));

    PV("The underlying header is as expected by the protocol");
    {
        // I validate directly using 'protocol.h' information.
        const char* p = blob.buffer(0).data();

        p += headerSize;  // Skip the fake header

        // --------------------------------------------------------------------
        // validating the first option

        p = validateOption(p, meta1, p1);
        p = validateOption(p, meta2, p2);

        ASSERT_EQ(0, bsl::memcmp(p, canary.c_str(), canary.size()));
    }
}

#ifdef BMQ_ENABLE_MSG_GROUPID
static void test4_isValidMsgGroupId()
// ------------------------------------------------------------------------
//                        VALIDATE GROUPID LENGTH
// ------------------------------------------------------------------------
//
// Concerns:
//   Validate that 'isValidMsgGroupId()' returns success or error codes as
//   expected for given 'length's.
//
// Testing:
//   -static
//    bmqt::EventBuilderResult::Enum OptionUtil::isValidMsgGroupId(
//                                         const bslstl::StringRef& group);
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("VALIDATE GROUPID LENGTH");

    typedef bmqt::EventBuilderResult Result;

    const bmqp::Protocol::MsgGroupId maxLength(
        "1234567890123456789012345678901",
        s_allocator_p);
    const bmqp::Protocol::MsgGroupId overMaxLength(
        "12345678901234567890123456789012",
        s_allocator_p);

#ifdef BMQ_ENABLE_MSG_GROUPID
    ASSERT_EQ(Result::e_INVALID_MSG_GROUP_ID,
              bmqp::OptionUtil::isValidMsgGroupId(""));
    ASSERT_EQ(Result::e_INVALID_MSG_GROUP_ID,
              bmqp::OptionUtil::isValidMsgGroupId(overMaxLength));
#endif

    ASSERT_EQ(Result::e_SUCCESS, bmqp::OptionUtil::isValidMsgGroupId(" "));
    ASSERT_EQ(Result::e_SUCCESS,
              bmqp::OptionUtil::isValidMsgGroupId(maxLength));
}
#endif

// ============================================================================
//                                MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    bmqp::ProtocolUtil::initialize(s_allocator_p);

    switch (_testCase) {
    case 0:
#ifdef BMQ_ENABLE_MSG_GROUPID
    case 4: test4_isValidMsgGroupId(); break;
#endif
    case 3: test3_checkOptionsBlobSegment(); break;
    case 2: test2_basicOptionsBoxCanAdd(); break;
    case 1: test1_basicOptionMetaProperties(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
