// Copyright 2026 Bloomberg Finance L.P.
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

#include <mqbi_storage.h>

// BMQ
#include <bmqp_messageproperties.h>
#include <bmqt_compressionalgorithmtype.h>

// BDE
#include <bsl_cstdlib.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_storageMessageAttributes()
// ------------------------------------------------------------------------
// STORAGE MESSAGE ATTRIBUTES
//
// Concerns:
//   Exercise value construction, accessors, reset, setters, and copy
//   assignment of StorageMessageAttributes.
//
// Plan:
//  1) Construct with specific non-default values, verify every getter.
//  2) Reset, verify all fields match default-constructed values.
//  3) Set every field back via setters, verify values again.
//  4) Copy-assign to another object, verify values.
//  5) Assign a default-constructed object, verify default values.
//
// Testing:
//   Value constructor, accessors, setters, reset(), operator=,
//   operator==, operator!=
// ------------------------------------------------------------------------
// NOLINTBEGIN(performance-avoid-endl)
{
    bmqtst::TestHelper::printTestName("STORAGE MESSAGE ATTRIBUTES");

    const bsls::Types::Uint64 k_TIMESTAMP    = 1234567890ULL;
    const bsls::Types::Int64  k_TIMEPOINT    = 9876543210LL;
    const unsigned int        k_REF_COUNT    = 7;
    const unsigned int        k_APP_DATA_LEN = 4096;
    const unsigned int        k_CRC32C       = 0xDEADBEEF;
    const bool                k_HAS_RECEIPT  = false;

    const bmqp::MessagePropertiesInfo k_MSG_PROPS_INFO =
        bmqp::MessagePropertiesInfo::makeInvalidSchema();

    const bmqt::CompressionAlgorithmType::Enum k_COMPRESSION =
        bmqt::CompressionAlgorithmType::e_ZLIB;

    // 1) Value constructor + getters
    PV("Step 1: value constructor");

    mqbi::StorageMessageAttributes obj(k_TIMESTAMP,
                                       k_REF_COUNT,
                                       k_APP_DATA_LEN,
                                       k_MSG_PROPS_INFO,
                                       k_COMPRESSION,
                                       k_HAS_RECEIPT,
                                       0,
                                       k_CRC32C,
                                       k_TIMEPOINT);

    BMQTST_ASSERT_EQ(obj.arrivalTimestamp(), k_TIMESTAMP);
    BMQTST_ASSERT_EQ(obj.arrivalTimepoint(), k_TIMEPOINT);
    BMQTST_ASSERT_EQ(obj.refCount(), k_REF_COUNT);
    BMQTST_ASSERT_EQ(obj.appDataLen(), k_APP_DATA_LEN);
    BMQTST_ASSERT_EQ(obj.messagePropertiesInfo().isPresent(),
                     k_MSG_PROPS_INFO.isPresent());
    BMQTST_ASSERT_EQ(obj.hasReceipt(), k_HAS_RECEIPT);
    BMQTST_ASSERT_EQ(obj.queueHandle(), static_cast<mqbi::QueueHandle*>(0));
    BMQTST_ASSERT_EQ(obj.crc32c(), k_CRC32C);
    BMQTST_ASSERT_EQ(obj.compressionAlgorithmType(), k_COMPRESSION);

    // 2) Reset
    PV("Step 2: reset");

    obj.reset();

    const mqbi::StorageMessageAttributes dflt;
    BMQTST_ASSERT_EQ(obj.arrivalTimestamp(), dflt.arrivalTimestamp());
    BMQTST_ASSERT_EQ(obj.arrivalTimepoint(), dflt.arrivalTimepoint());
    BMQTST_ASSERT_EQ(obj.refCount(), dflt.refCount());
    BMQTST_ASSERT_EQ(obj.appDataLen(), dflt.appDataLen());
    BMQTST_ASSERT_EQ(obj.messagePropertiesInfo().isPresent(),
                     dflt.messagePropertiesInfo().isPresent());
    BMQTST_ASSERT_EQ(obj.hasReceipt(), dflt.hasReceipt());
    BMQTST_ASSERT_EQ(obj.queueHandle(), dflt.queueHandle());
    BMQTST_ASSERT_EQ(obj.crc32c(), dflt.crc32c());
    BMQTST_ASSERT_EQ(obj.compressionAlgorithmType(),
                     dflt.compressionAlgorithmType());

    // 3) Setters
    PV("Step 3: setters");

    obj.setArrivalTimestamp(k_TIMESTAMP);
    obj.setArrivalTimepoint(k_TIMEPOINT);
    obj.setRefCount(k_REF_COUNT);
    obj.setAppDataLen(k_APP_DATA_LEN);
    obj.setMessagePropertiesInfo(k_MSG_PROPS_INFO);
    obj.setReceipt(k_HAS_RECEIPT);
    obj.setCrc32c(k_CRC32C);
    obj.setCompressionAlgorithmType(k_COMPRESSION);

    BMQTST_ASSERT_EQ(obj.arrivalTimestamp(), k_TIMESTAMP);
    BMQTST_ASSERT_EQ(obj.arrivalTimepoint(), k_TIMEPOINT);
    BMQTST_ASSERT_EQ(obj.refCount(), k_REF_COUNT);
    BMQTST_ASSERT_EQ(obj.appDataLen(), k_APP_DATA_LEN);
    BMQTST_ASSERT_EQ(obj.messagePropertiesInfo().isPresent(),
                     k_MSG_PROPS_INFO.isPresent());
    BMQTST_ASSERT_EQ(obj.hasReceipt(), k_HAS_RECEIPT);
    BMQTST_ASSERT_EQ(obj.crc32c(), k_CRC32C);
    BMQTST_ASSERT_EQ(obj.compressionAlgorithmType(), k_COMPRESSION);

    // 4) Copy assignment
    PV("Step 4: copy assignment");

    mqbi::StorageMessageAttributes copy;
    copy = obj;

    BMQTST_ASSERT_EQ(copy.arrivalTimestamp(), k_TIMESTAMP);
    BMQTST_ASSERT_EQ(copy.arrivalTimepoint(), k_TIMEPOINT);
    BMQTST_ASSERT_EQ(copy.refCount(), k_REF_COUNT);
    BMQTST_ASSERT_EQ(copy.appDataLen(), k_APP_DATA_LEN);
    BMQTST_ASSERT_EQ(copy.messagePropertiesInfo().isPresent(),
                     k_MSG_PROPS_INFO.isPresent());
    BMQTST_ASSERT_EQ(copy.hasReceipt(), k_HAS_RECEIPT);
    BMQTST_ASSERT_EQ(copy.queueHandle(), static_cast<mqbi::QueueHandle*>(0));
    BMQTST_ASSERT_EQ(copy.crc32c(), k_CRC32C);
    BMQTST_ASSERT_EQ(copy.compressionAlgorithmType(), k_COMPRESSION);

    // Also check "==" and "!="
    BMQTST_ASSERT(copy == obj);
    BMQTST_ASSERT(!(copy != obj));
    BMQTST_ASSERT(obj != dflt);
    BMQTST_ASSERT(!(obj == dflt));

    // 5) Assign default-constructed
    PV("Step 5: assign default-constructed");

    obj = mqbi::StorageMessageAttributes();

    BMQTST_ASSERT_EQ(obj.arrivalTimestamp(), dflt.arrivalTimestamp());
    BMQTST_ASSERT_EQ(obj.arrivalTimepoint(), dflt.arrivalTimepoint());
    BMQTST_ASSERT_EQ(obj.refCount(), dflt.refCount());
    BMQTST_ASSERT_EQ(obj.appDataLen(), dflt.appDataLen());
    BMQTST_ASSERT_EQ(obj.messagePropertiesInfo().isPresent(),
                     dflt.messagePropertiesInfo().isPresent());
    BMQTST_ASSERT_EQ(obj.hasReceipt(), dflt.hasReceipt());
    BMQTST_ASSERT_EQ(obj.queueHandle(), dflt.queueHandle());
    BMQTST_ASSERT_EQ(obj.crc32c(), dflt.crc32c());
    BMQTST_ASSERT_EQ(obj.compressionAlgorithmType(),
                     dflt.compressionAlgorithmType());

    // Also check "==" and "!="
    BMQTST_ASSERT(obj == dflt);
    BMQTST_ASSERT(!(obj != dflt));
    BMQTST_ASSERT(copy != obj);
    BMQTST_ASSERT(!(copy == obj));
}
// NOLINTEND(performance-avoid-endl)

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
// NOLINTBEGIN(cert-err34-c,cppcoreguidelines-pro-bounds-pointer-arithmetic,performance-avoid-endl)
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_storageMessageAttributes(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
// NOLINTEND(cert-err34-c,cppcoreguidelines-pro-bounds-pointer-arithmetic,performance-avoid-endl)
