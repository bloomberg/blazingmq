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

#include <bmqu_prettytimeinterval.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bsl_limits.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>
#include <bsl_iostream.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_print()
// ------------------------------------------------------------------------
// PRINT
//
// Concerns:
//   Ensure proper output of 'PrettyTimeInterval'.
//
// Plan:
//   Test various inputs.
//
// Testing:
//   operator<<(bsl::ostream&, const PrettyTimeInterval&)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PRINT");

    const bsls::Types::Int64 k_INTMAX =
        bsl::numeric_limits<bsls::Types::Int64>::max();
    const bsls::Types::Int64 k_INTMIN =
        bsl::numeric_limits<bsls::Types::Int64>::min();

    struct Test {
        int                d_line;
        bsls::Types::Int64 d_value;
        int                d_precision;
        const char*        d_expected;
    } k_DATA[] = {
        {L_, 10, 3, "10 ns"},
        {L_, -10, 3, "-10 ns"},
        {L_, 1000, 3, "1.000 us"},
        {L_, 12000, 3, "12.000 us"},
        {L_, 12432, 2, "12.43 us"},
        {L_, 12436, 2, "12.44 us"},
        {L_, 12435, 2, "12.44 us"},
        {L_, 12432, 3, "12.432 us"},
        {L_, 12678123, 0, "13 ms"},
        {L_, 12678123, 1, "12.7 ms"},
        {L_, 12678123, 2, "12.68 ms"},
        {L_, 12678123, 3, "12.678 ms"},
        {L_, 12678123, 4, "12.6781 ms"},
        {L_, 1000000000LL, 3, "1.000 s"},
        {L_, 9999123400000LL, 3, "2.778 h"},
        {L_, 5LL * 60 * 1000 * 1000 * 1000, 2, "5.00 m"},
        {L_, -5LL * 60 * 1000 * 1000 * 1000, 2, "-5.00 m"},
        {L_, 1000LL * 1000 * 1000 * 60 * 60 * 24 * 20, 2, "2.86 w"},
        {L_, k_INTMAX, 3, "15250.284 w"},
        {L_, k_INTMIN, 2, "-15250.28 w"},
        {L_, 923456789123456LL, 0, "2 w"},
        {L_, 923456789123456LL, 1, "1.5 w"},   // 1.5268w
        {L_, 923456789123456LL, 2, "1.53 w"},  // 1.5268w
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": checking value '" << test.d_value << "' "
                        << " (precision: " << test.d_precision << ")");

        bmqu::MemOutStream buf(bmqtst::TestHelperUtil::allocator());
        buf << bmqu::PrettyTimeInterval(test.d_value, test.d_precision);
        BMQTST_ASSERT_EQ_D(test.d_line, buf.str(), test.d_expected);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_print(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << "\n";
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
