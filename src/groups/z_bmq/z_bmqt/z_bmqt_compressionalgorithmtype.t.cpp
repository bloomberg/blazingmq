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

// bmqt_compressionalgorithmtype.t.cpp
#include <z_bmqt_compressionalgorithmtype.h>

// MWC
#include <mwcu_memoutstream.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// BDE
#include <bsl_string.h>
#include <bslmf_assert.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

struct ToAsciiTestData {
    int         d_type;
    const char* d_expected;
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

template <typename ENUM_TYPE, typename ARRAY, int SIZE>
static void enumToAsciiHelper(ARRAY (&data)[SIZE])
{
    for (size_t idx = 0; idx < SIZE; ++idx) {
        const ToAsciiTestData&               test = data[idx];
        z_bmqt_CompressionAlgorithmType::Enum type =
            static_cast<z_bmqt_CompressionAlgorithmType::Enum>(test.d_type);
        const char* result = z_bmqt_CompressionAlgorithmType::toAscii(type);
        ASSERT_EQ(strcmp(result, test.d_expected), 0);
    }
}

static void test1_enumToAscii()
// ------------------------------------------------------------------------
// ENUM LAYOUT
//
// Concerns:
//   Check that enums toAscii methods work correct
//
// Plan:
//   1. For every type of enum for which there is a corresponding toAscii
//      function check that output of the print function.
//      equal to expected value
//   2. Check that the result for invalid value is equal to expected.
//
// Testing:
//   z_bmqt_CompressionAlgorithmType::toAscii
//   operator<<(bsl::ostream&, CompressionAlgorithmType::Enum)
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ENUM LAYOUT");

    PV("Test bmqt::CompressionAlgorithmType printing");
    {
        BSLMF_ASSERT(
            z_bmqt_CompressionAlgorithmType::k_LOWEST_SUPPORTED_TYPE ==
            z_bmqt_CompressionAlgorithmType::ec_NONE);

        BSLMF_ASSERT(
            z_bmqt_CompressionAlgorithmType::k_HIGHEST_SUPPORTED_TYPE ==
            z_bmqt_CompressionAlgorithmType::ec_ZLIB);

        ToAsciiTestData k_DATA[] = {
            {z_bmqt_CompressionAlgorithmType::ec_UNKNOWN, "UNKNOWN"},
            {z_bmqt_CompressionAlgorithmType::ec_NONE, "NONE"},
            {z_bmqt_CompressionAlgorithmType::ec_ZLIB, "ZLIB"},
            {z_bmqt_CompressionAlgorithmType::k_HIGHEST_SUPPORTED_TYPE + 1,
             "(* UNKNOWN *)"}};

        enumToAsciiHelper<z_bmqt_CompressionAlgorithmType>(k_DATA);
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
    case 1: test1_enumToAscii(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
