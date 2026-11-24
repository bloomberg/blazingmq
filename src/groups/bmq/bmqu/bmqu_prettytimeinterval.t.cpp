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
#include <bsl_string_view.h>
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

    struct Local {
        static void verifyPrint(int                line,
                                bsls::Types::Int64 value,
                                int                precision,
                                bsl::string_view   expected)
        {
            bmqu::MemOutStream buf(bmqtst::TestHelperUtil::allocator());
            buf << bmqu::PrettyTimeInterval(value, precision);
            BMQTST_ASSERT_EQ_D(line, buf.str(), expected);
        }
    };

    PV("Nanoseconds are printed without decimals");
    {
        Local::verifyPrint(L_, 10, 3, "10 ns");
    }

    PV("Unit selection");
    {
        Local::verifyPrint(L_, 1000, 3, "1.000 us");
        Local::verifyPrint(L_, 12000, 3, "12.000 us");
        Local::verifyPrint(L_, 12678123, 3, "12.678 ms");
        Local::verifyPrint(L_, 1000000000LL, 3, "1.000 s");
        Local::verifyPrint(L_, 5LL * 60 * 1000 * 1000 * 1000, 2, "5.00 m");
        Local::verifyPrint(L_, 9999123400000LL, 3, "2.778 h");
        Local::verifyPrint(L_,
                           1000LL * 1000 * 1000 * 60 * 60 * 24 * 20,
                           2,
                           "2.86 w");
    }

    PV("Precision");
    {
        Local::verifyPrint(L_, 12678123, 0, "13 ms");
        Local::verifyPrint(L_, 12678123, 1, "12.7 ms");
        Local::verifyPrint(L_, 12678123, 2, "12.68 ms");
        Local::verifyPrint(L_, 12678123, 4, "12.6781 ms");
        Local::verifyPrint(L_, 12432, 3, "12.432 us");
    }

    PV("Rounding to the nearest");
    {
        Local::verifyPrint(L_, 12432, 2, "12.43 us");
        Local::verifyPrint(L_, 12435, 2, "12.44 us");
        Local::verifyPrint(L_, 12436, 2, "12.44 us");
        Local::verifyPrint(L_, 923456789123456LL, 0, "2 w");     // 1.5268w
        Local::verifyPrint(L_, 923456789123456LL, 1, "1.5 w");   // 1.5268w
        Local::verifyPrint(L_, 923456789123456LL, 2, "1.53 w");  // 1.5268w
    }

    PV("Rounding carries into the whole part and the next unit");
    {
        Local::verifyPrint(L_, 1999999, 2, "2.00 ms");
        Local::verifyPrint(L_, 999999, 2, "1.00 ms");
        Local::verifyPrint(L_, 999999, 0, "1 ms");
        Local::verifyPrint(L_, -999999, 2, "-1.00 ms");
        Local::verifyPrint(L_, 59999999999LL, 1, "1.0 m");
    }

    PV("Negative values");
    {
        Local::verifyPrint(L_, -10, 3, "-10 ns");
        Local::verifyPrint(L_, -5LL * 60 * 1000 * 1000 * 1000, 2, "-5.00 m");
    }

    PV("Limits");
    {
        const bsls::Types::Int64 k_INTMAX =
            bsl::numeric_limits<bsls::Types::Int64>::max();
        const bsls::Types::Int64 k_INTMIN =
            bsl::numeric_limits<bsls::Types::Int64>::min();

        Local::verifyPrint(L_, k_INTMAX, 3, "15250.284 w");
        Local::verifyPrint(L_, k_INTMIN, 2, "-15250.28 w");
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    bmqtst::TestGuard guard(argc, argv);

    switch (guard.testCase()) {
    case 0:
    case 1: test1_print(); break;
    default: guard.caseNotFound(); break;
    }

    return guard.finish();
}
