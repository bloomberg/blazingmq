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

// mwcu_printutil.t.cpp                                               -*-C++-*-
#include <mwcu_printutil.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <bsl_limits.h>
#include <bsls_types.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
static void test1_prettyNumberInt64()
// ------------------------------------------------------------------------
// prettyNumber (Int64)
//
// Concerns:
//   Ensure proper behavior of 'prettyNumber' (Int64) method.
//
// Plan:
//   Test various inputs.
//
// Testing:
//   Proper behavior of
//   'prettyNumber(stream, value, groupSize, separator)'
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("prettyNumber (Int64)");

    const bsls::Types::Int64 k_INTMAX =
        bsl::numeric_limits<bsls::Types::Int64>::max();
    const bsls::Types::Int64 k_INTMIN =
        bsl::numeric_limits<bsls::Types::Int64>::min();

    struct Test {
        int                d_line;
        bsls::Types::Int64 d_value;
        int                d_groupSize;
        char               d_separator;
        const char*        d_expected;
    } k_DATA[] = {
        {L_, 0, 3, ' ', "0"},
        {L_, k_INTMAX, 2, '.', "9.22.33.72.03.68.54.77.58.07"},
        {L_, k_INTMIN, 4, '#', "-922#3372#0368#5477#5808"},
        {L_, -1000, 1, '_', "-1_0_0_0"},
        {L_, 12345, 1, ',', "1,2,3,4,5"},
        {L_, 12345, 2, ',', "1,23,45"},
        {L_, 12345, 3, ',', "12,345"},
        {L_, 12345, 4, ',', "1,2345"},
        {L_, 12345, 5, ',', "12345"},
        {L_, 12345, 6, ',', "12345"},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        // function
        {
            PVV(test.d_line << ": checking value '" << test.d_value << "' "
                            << "(groupSize: " << test.d_groupSize
                            << ", separator: '" << test.d_separator
                            << "', function)");

            mwcu::MemOutStream buf(s_allocator_p);
            mwcu::PrintUtil::prettyNumber(buf,
                                          test.d_value,
                                          test.d_groupSize,
                                          test.d_separator);
            ASSERT_EQ_D(test.d_line, buf.str(), test.d_expected);
        }

        // manipulator
        {
            PVV(test.d_line << ": checking value '" << test.d_value << "' "
                            << "(groupSize: " << test.d_groupSize
                            << ", separator: '" << test.d_separator
                            << "', manipulator)");

            mwcu::MemOutStream buf(s_allocator_p);
            buf << mwcu::PrintUtil::prettyNumber(test.d_value,
                                                 test.d_groupSize,
                                                 test.d_separator);
            ASSERT_EQ_D(test.d_line, buf.str(), test.d_expected);
        }
    }

    PV("Ensure assertion when group size <= 0");
    {
        ASSERT_SAFE_FAIL(
            mwcu::PrintUtil::prettyNumber(bsl::cout, 123, 0, ','));
        ASSERT_SAFE_FAIL(
            mwcu::PrintUtil::prettyNumber(bsl::cout, 123, -2, ','));
    }
}

static void test2_prettyNumberDouble()
// ------------------------------------------------------------------------
// prettyNumber (double)
//
// Concerns:
//   Ensure proper behavior of 'prettyNumber' (double) method.
//
// Plan:
//   Test various inputs.
//
// Testing:
//   Proper behavior of
//   'prettyNumber(stream, value, precision, groupSize, sep)'
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("prettyNumber (Double)");

    struct Test {
        int         d_line;
        double      d_value;
        int         d_precision;
        int         d_groupSize;
        char        d_separator;
        const char* d_expected;
    } k_DATA[] = {
        {L_, 0.0, 2, 3, ' ', "0.00"},
        {L_, 1234567.342324, 3, 3, ',', "1,234,567.342"},
        {L_, 1234556.342324, 0, 1, ',', "1,2,3,4,5,5,6"},
        {L_, -3432.11223, 4, 3, ',', "-3,432.1122"},
        {L_, 12345.9876, 0, 3, ',', "12,345"},
        {L_, 12345.9876, 1, 3, ',', "12,345.9"},
        {L_, 12345.9876, 2, 3, ',', "12,345.98"},
        {L_, 12345.9876, 3, 3, ',', "12,345.987"},
        {L_, 12345.9876, 4, 3, ',', "12,345.9876"},
        {L_, 12345.9876, 5, 3, ',', "12,345.98760"},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        // function
        {
            PVV(test.d_line << ": checking value '" << test.d_value << "' "
                            << " (precision: " << test.d_precision
                            << ", groupSize: " << test.d_groupSize
                            << ", separator: '" << test.d_separator
                            << "', function)");

            mwcu::MemOutStream buf(s_allocator_p);
            mwcu::PrintUtil::prettyNumber(buf,
                                          test.d_value,
                                          test.d_precision,
                                          test.d_groupSize,
                                          test.d_separator);
            ASSERT_EQ_D(test.d_line, buf.str(), test.d_expected);
        }

        // manipulator
        {
            PVV(test.d_line << ": checking value '" << test.d_value << "' "
                            << " (precision: " << test.d_precision
                            << ", groupSize: " << test.d_groupSize
                            << ", separator: '" << test.d_separator
                            << "', manipulator)");

            mwcu::MemOutStream buf(s_allocator_p);
            buf << mwcu::PrintUtil::prettyNumber(test.d_value,
                                                 test.d_precision,
                                                 test.d_groupSize,
                                                 test.d_separator);
            ASSERT_EQ_D(test.d_line, buf.str(), test.d_expected);
        }
    }

    PV("Ensure assertion when group size <= 0");
    {
        ASSERT_SAFE_FAIL(
            mwcu::PrintUtil::prettyNumber(bsl::cout, 123.0, 0, 0, ','));
        ASSERT_SAFE_FAIL(
            mwcu::PrintUtil::prettyNumber(bsl::cout, 123.0, 0, -2, ','));
    }
}

static void test3_prettyBytes()
// ------------------------------------------------------------------------
// prettyBytes
//
// Concerns:
//   Ensure proper behavior of 'prettyBytes' method.
//
// Plan:
//   Test various inputs.
//
// Testing:
//   Proper behavior of 'prettyBytes(stream, bytes, precision)'
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("prettyBytes");

    const bsls::Types::Int64 k_INTMAX =
        bsl::numeric_limits<bsls::Types::Int64>::max();
    const bsls::Types::Int64 k_INTMIN =
        bsl::numeric_limits<bsls::Types::Int64>::min();

    PVV("k_INTMAX=" << k_INTMAX);

#define TO_I64(X) static_cast<bsls::Types::Int64>(X)

    struct Test {
        int                d_line;
        bsls::Types::Int64 d_value;
        int                d_precision;
        const char*        d_expected;
    } k_DATA[] = {
        {L_, 0, 3, "0  B"},
        {L_, 500, 0, "500  B"},
        {L_, 500, 2, "500  B"},
        {L_, 1023, 0, "1023  B"},
        {L_, 1024, 0, "1 KB"},
        {L_, 1024, 2, "1.00 KB"},
        {L_, 1025, 3, "1.001 KB"},
        {L_, 1025, 2, "1.00 KB"},
        {L_, TO_I64(1.461 * 1024 * 1024), 2, "1.46 MB"},
        {L_, TO_I64(1.951 * 1024 * 1024), 2, "1.95 MB"},
        {L_, TO_I64(1.5 * 1024 * 1024), 3, "1.500 MB"},
        {L_, 2 * 1024 * 1024, 1, "2.0 MB"},
        {L_, 2 * 1024 * 1024, 0, "2 MB"},
        {L_, 1024 * 1024 * 1024 - 1, 1, "1024.0 MB"},
        {L_, 1024 * 1024 * 1024 - 1, 2, "1024.00 MB"},
        {L_, 1024 * 1024 * 1024, 1, "1.0 GB"},
        {L_, 1024LL * 1024 * 1024 * 1024, 1, "1.0 TB"},
        {L_, 1024LL * 1024 * 1024 * 1024 * 1024, 1, "1.0 PB"},
        {L_, 1024LL * 1024 * 1024 * 1024 * 1024 - 1, 0, "1 PB"},
        {L_, 2048LL * 1024 * 1024 * 1024 * 1024 * 1024, 1, "2048.0 PB"},
        {L_, -500, 2, "-500  B"},
        {L_, TO_I64(-1.951 * 1024 * 1024), 2, "-1.95 MB"},
        {L_, k_INTMIN, 2, "-8192.00 PB"},
        {L_, k_INTMAX, 2, "8192.00 PB"},
    };

#undef TO_I64

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        // function
        {
            PVV(test.d_line << ": checking value '" << test.d_value << "' "
                            << " (precision: " << test.d_precision
                            << ", function)");

            mwcu::MemOutStream buf(s_allocator_p);
            mwcu::PrintUtil::prettyBytes(buf, test.d_value, test.d_precision);
            ASSERT_EQ_D(test.d_line, buf.str(), test.d_expected);
        }

        // manipulator
        {
            PVV(test.d_line << ": checking value '" << test.d_value << "' "
                            << " (precision: " << test.d_precision
                            << ", function)");

            mwcu::MemOutStream buf(s_allocator_p);
            buf << mwcu::PrintUtil::prettyBytes(test.d_value,
                                                test.d_precision);
            ASSERT_EQ_D(test.d_line, buf.str(), test.d_expected);
        }
    }
}

static void test4_prettyTimeInterval()
// ------------------------------------------------------------------------
// prettyTimeInterval
//
// Concerns:
//   Ensure proper behavior of 'prettyTimeInterval' method.
//
// Plan:
//   Test various inputs.
//
// Testing:
//   Proper behavior of 'prettyTimeInterval(stream, timeNs, precision)'
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("prettyTimeInterval");

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

        // function
        {
            PVV(test.d_line << ": checking value '" << test.d_value << "' "
                            << " (precision: " << test.d_precision
                            << ", function)");

            mwcu::MemOutStream buf(s_allocator_p);
            mwcu::PrintUtil::prettyTimeInterval(buf,
                                                test.d_value,
                                                test.d_precision);
            ASSERT_EQ_D(test.d_line, buf.str(), test.d_expected);
        }

        // manipulator
        {
            PVV(test.d_line << ": checking value '" << test.d_value << "' "
                            << " (precision: " << test.d_precision
                            << ", function)");

            mwcu::MemOutStream buf(s_allocator_p);
            buf << mwcu::PrintUtil::prettyTimeInterval(test.d_value,
                                                       test.d_precision);
            ASSERT_EQ_D(test.d_line, buf.str(), test.d_expected);
        }
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
    case 4: test4_prettyTimeInterval(); break;
    case 3: test3_prettyBytes(); break;
    case 2: test2_prettyNumberDouble(); break;
    case 1: test1_prettyNumberInt64(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
