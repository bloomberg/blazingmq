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

#include <bmqt_compressionalgorithmtype.h>

#include <bmqu_memoutstream.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// BDE
#include <bslmf_assert.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

struct PrintTestData {
    int         d_line;
    int         d_type;
    const char* d_expected;
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

template <typename ENUM_TYPE, typename ARRAY, int SIZE>
// NOLINTBEGIN(*-avoid-c-arrays)
static void printEnumHelper(ARRAY (&data)[SIZE])
{
    // NOLINTBEGIN(performance-avoid-endl)
    for (size_t idx = 0; idx < SIZE; ++idx) {
        const PrintTestData& test = data[idx];

        PVVV("Line [" << test.d_line << "]");

        bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expected(bmqtst::TestHelperUtil::allocator());

        typedef typename ENUM_TYPE::Enum T;

        T obj = static_cast<T>(test.d_type);

        expected << test.d_expected;

        out.setstate(bsl::ios_base::badbit);
        ENUM_TYPE::print(out, obj, 0, -1);

        BMQTST_ASSERT_EQ(out.str(), "");

        out.clear();
        ENUM_TYPE::print(out, obj, 0, -1);

        BMQTST_ASSERT_EQ(out.str(), expected.str());

        out.reset();
        out << obj;

        BMQTST_ASSERT_EQ(out.str(), expected.str());
    }
    // NOLINTEND(performance-avoid-endl)
}
// NOLINTEND(*-avoid-c-arrays)

static void test1_enumPrint()
// ------------------------------------------------------------------------
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
//   CompressionAlgorithmType::print
//   operator<<(bsl::ostream&, CompressionAlgorithmType::Enum)
// ------------------------------------------------------------------------
// NOLINTBEGIN(performance-avoid-endl)
{
    bmqtst::TestHelper::printTestName("ENUM LAYOUT");

    PV("Test bmqt::CompressionAlgorithmType printing");
    {
        BSLMF_ASSERT(bmqt::CompressionAlgorithmType::k_LOWEST_SUPPORTED_TYPE ==
                     bmqt::CompressionAlgorithmType::e_NONE);

        BSLMF_ASSERT(
            bmqt::CompressionAlgorithmType::k_HIGHEST_SUPPORTED_TYPE ==
            bmqt::CompressionAlgorithmType::e_ZLIB);

        // NOLINTBEGIN(*-avoid-c-arrays)
        PrintTestData k_DATA[] = {
            {L_, bmqt::CompressionAlgorithmType::e_UNKNOWN, "UNKNOWN"},
            {L_, bmqt::CompressionAlgorithmType::e_NONE, "NONE"},
            {L_, bmqt::CompressionAlgorithmType::e_ZLIB, "ZLIB"}};
        // NOLINTEND(*-avoid-c-arrays)

        printEnumHelper<bmqt::CompressionAlgorithmType>(k_DATA);
    }
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
    case 1: test1_enumPrint(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
// NOLINTEND(cert-err34-c,cppcoreguidelines-pro-bounds-pointer-arithmetic,performance-avoid-endl)
