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

// mwcstu_printutil.t.cpp                                             -*-C++-*-

#include <mwcst_printutil.h>

#include <mwcst_testutil.h>

#include <bslma_testallocator.h>
#include <bsls_assert.h>

#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_set.h>
#include <bsl_sstream.h>

using namespace BloombergLP;
using namespace mwcstu;
using namespace bsl;

//=============================================================================
//                                  TEST PLAN
//-----------------------------------------------------------------------------
//                              *** Overview ***
//
// The component under test is a utility for printing data in a table.
//
// ----------------------------------------------------------------------------
// [ 1] PRINT MEMORY TEST
// [ 2] PRINT VALUE WITH SEPARATOR TEST
// [ 3] PRINT DOUBLE VALUE WITH SEPARATOR TEST
// [ 4] TIME INTERVAL NS TEST
// [ 5] PRINT ORDINAL TEST

//=============================================================================
//                      STANDARD BDE ASSERT TEST MACROS
//-----------------------------------------------------------------------------
static int testStatus = 0;

//=============================================================================
//                       STANDARD BDE TEST DRIVER MACROS
//-----------------------------------------------------------------------------

#define L_ BSLS_BSLTESTUTIL_L_  // current Line number

//=============================================================================
//                      GLOBAL HELPER FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------

// FUNCTIONS
template <class TYPE>
void checkStreamOutput(int line, const TYPE& obj, const char* expected)
{
    bsl::ostringstream ss;
    ss << mwcstu::Printer<TYPE>(&obj);

    LOOP_ASSERT_EQUALS(line, ss.str(), expected);
}

template <class TYPE>
void checkPrintOutput(int         line,
                      const TYPE& obj,
                      int         level,
                      int         spacesPerLevel,
                      const char* expected)
{
    bsl::ostringstream    ss;
    mwcstu::Printer<TYPE> printer(&obj);
    printer.print(ss, level, spacesPerLevel);
    LOOP_ASSERT_EQUALS(line, ss.str(), expected);
}

//=============================================================================
//                                MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    int test    = argc > 1 ? atoi(argv[1]) : 0;
    int verbose = argc > 2;
    // int veryVerbose = argc > 3;
    // int veryVeryVerbose = argc > 4;

    cout << "TEST " << __FILE__ << " CASE " << test << endl;

    // Use test allocator
    bslma::TestAllocator testAllocator;
    testAllocator.setNoAbort(true);
    bslma::Default::setDefaultAllocatorRaw(&testAllocator);

    typedef bsls::Types::Int64 Int64;

    switch (test) {
    case 0:
    case 6: {
        // --------------------------------------------------------------------
        // PRINTER TEST
        //
        // Concerns:
        //   That the Printer correctly supports both 'operator<<' and
        //   'print' for the parameterized type
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl << "PRINTER TEST" << endl << "============" << endl;

        {
            bsl::map<int, double> obj;
            obj[1] = .5;
            obj[2] = .6;
            checkStreamOutput(L_, obj, "{1:0.5, 2:0.6}");
            checkPrintOutput(L_, obj, 0, -1, "{1:0.5, 2:0.6}");
        }
    } break;
    case 5: {
        // --------------------------------------------------------------------
        // PRINT ORDINAL TEST
        //
        // Concerns:
        //   That the value is printed correctly with the right suffix.
        //
        // Plan:
        //   Table of values and expected output
        //
        // Testing:
        //   printOrdinal()
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl << "PRINT ORDINAL" << endl << "=============" << endl;

        const struct TestData {
            int         d_line;
            Int64       d_num;
            const char* d_expected_p;
        } DATA[] = {
            {L_, 0, "0th"},
            {L_, 1, "1st"},
            {L_, 2, "2nd"},
            {L_, 3, "3rd"},
            {L_, 4, "4th"},
            {L_, 5, "5th"},
            {L_, 10, "10th"},
            {L_, 11, "11th"},
            {L_, 101, "101st"},
            {L_, 111, "111th"},
        };
        const int NUM_DATA = sizeof(DATA) / sizeof(*DATA);

        for (int dataIdx = 0; dataIdx < NUM_DATA; ++dataIdx) {
            const TestData& data = DATA[dataIdx];
            const int       LINE = data.d_line;

            // P(LINE);

            bsl::ostringstream ss;
            PrintUtil::printOrdinal(ss, data.d_num);
            LOOP_ASSERT_EQUALS(LINE, ss.str(), data.d_expected_p);
        }

    } break;

    case 4: {
        // --------------------------------------------------------------------
        // TIME INTERVAL NS TEST
        //
        // Concerns:
        //   That the value prints correctly and 'printedTimeIntervalNsLength'
        //   returns the correct length
        //
        // Plan:
        //   Table of values to print and expected output
        //
        // Testing:
        //   printTimeIntervalNs();
        //   printedTimeIntervalNsLength();
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl
                 << "PRINT TIME INTERVAL NS TEST" << endl
                 << "===========================" << endl;

        const struct TestData {
            int         d_line;
            Int64       d_interval;
            int         d_precision;
            const char* d_expected_p;
        } DATA[] = {
            {L_, 10, 3, "10 ns"},
            {L_, 1000, 3, "1.000 us"},
            {L_, 12000, 3, "12.000 us"},
            {L_, 12678123, 3, "12.678 ms"},
            {L_, 1000000000LL, 3, "1.000 s"},
            {L_, 9999123400000LL, 3, "2.777 h"},
            {L_, 5LL * 60 * 1000 * 1000 * 1000, 2, "5.00 m"},
            {L_, 1000LL * 1000 * 1000 * 60 * 60 * 24 * 20, 2, "2.85 w"}};
        const int NUM_DATA = sizeof(DATA) / sizeof(*DATA);

        for (int dataIdx = 0; dataIdx < NUM_DATA; ++dataIdx) {
            const TestData& data = DATA[dataIdx];
            const int       LINE = data.d_line;

            // P(LINE);

            bsl::ostringstream ss;
            PrintUtil::printTimeIntervalNs(ss,
                                           data.d_interval,
                                           data.d_precision);
            size_t printedLength  = ss.str().length();
            size_t expectedLength = PrintUtil::printedTimeIntervalNsLength(
                data.d_interval,
                data.d_precision);

            LOOP_ASSERT_EQUALS(LINE, ss.str(), data.d_expected_p);
            LOOP_ASSERT_EQUALS(LINE, printedLength, expectedLength);
        }
    } break;

    case 3: {
        // --------------------------------------------------------------------
        // PRINT DOUBLE VALUE WITH SEPARATOR TEST
        //
        // Concerns:
        //   That the value prints correctly
        //
        // Plan:
        //   Table of values to print and expected output
        //
        // Testing:
        //   printValueWithSeparator();
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl
                 << "PRINT DOUBLE VALUE WITH SEPARATOR TEST" << endl
                 << "======================================" << endl;

        const struct TestData {
            int         d_line;
            double      d_arg;
            int         d_precision;
            int         d_groupSize;
            char        d_separator;
            const char* d_expected_p;
        } DATA[] = {
            {L_, 0, 2, 3, ' ', "0.00"},
            {L_, 1234556.342324, 3, 3, ',', "1,234,556.342"},
            {L_, 1234556.342324, 0, 1, ',', "1,2,3,4,5,5,6"},
            {L_, -3432.11223, 4, 3, ',', "-3,432.1122"},
        };
        const int NUM_DATA = sizeof(DATA) / sizeof(*DATA);

        for (int dataIdx = 0; dataIdx < NUM_DATA; ++dataIdx) {
            const TestData& data = DATA[dataIdx];
            const int       LINE = data.d_line;

            // P(LINE);

            bsl::ostringstream ss;
            PrintUtil::printValueWithSeparator(ss,
                                               data.d_arg,
                                               data.d_precision,
                                               data.d_groupSize,
                                               data.d_separator);

            int expectedLength   = ss.str().length();
            int calculatedLength = PrintUtil::printedValueLengthWithSeparator(
                data.d_arg,
                data.d_precision,
                data.d_groupSize);

            LOOP_ASSERT_EQUALS(LINE, ss.str(), data.d_expected_p);
            LOOP_ASSERT_EQUALS(LINE, expectedLength, calculatedLength);
        }
    } break;

    case 2: {
        // --------------------------------------------------------------------
        // PRINT VALUE WITH SEPARATOR TEST
        //
        // Concerns:
        //   That the value prints correctly
        //
        // Plan:
        //   Table of values to print and expected output
        //
        // Testing:
        //   printValueWithSeparator();
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl
                 << "PRINT VALUE WITH SEPARATOR TEST" << endl
                 << "===============================" << endl;

        const Int64 maxInt = bsl::numeric_limits<Int64>::max();
        const Int64 minInt = bsl::numeric_limits<Int64>::min();

        const struct TestData {
            int         d_line;
            Int64       d_arg;
            int         d_groupSize;
            char        d_separator;
            const char* d_expected_p;
        } DATA[] = {
            {L_, 0, 3, ' ', "0"},
            {L_, maxInt, 2, '.', "9.22.33.72.03.68.54.77.58.07"},
            {L_, minInt, 4, '-', "-922-3372-0368-5477-5808"},
        };
        const int NUM_DATA = sizeof(DATA) / sizeof(*DATA);

        for (int dataIdx = 0; dataIdx < NUM_DATA; ++dataIdx) {
            const TestData& data = DATA[dataIdx];
            const int       LINE = data.d_line;

            // P(LINE);

            bsl::ostringstream ss;
            PrintUtil::printValueWithSeparator(ss,
                                               data.d_arg,
                                               data.d_groupSize,
                                               data.d_separator);

            LOOP_ASSERT_EQUALS(LINE, ss.str(), data.d_expected_p);
        }
    } break;

    case 1: {
        // --------------------------------------------------------------------
        // PRINT MEMORY TEST
        //
        // Concerns:
        //   That 'printMemory' correctly prints its output
        //
        // Plan:
        //   Table of values to print and expected output
        //
        // Testing:
        //   printMemory()
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl
                 << "PRINT MEMORY TEST" << endl
                 << "=================" << endl;

#define TO_I64(x) (Int64)(x)

        const struct TestData {
            int         d_line;
            Int64       d_arg;
            int         d_precision;
            const char* d_expected_p;
        } DATA[] = {
            {L_, 0, 3, "0 B"},
            {L_, 500, 0, "500 B"},
            {L_, 500, 2, "500 B"},
            {L_, 1023, 0, "1023 B"},
            {L_, 1024, 0, "1 KB"},
            {L_, 1024, 2, "1.00 KB"},
            {L_, 1025, 3, "1.000 KB"},
            {L_, 1025, 2, "1.00 KB"},
            {L_, TO_I64(1.461 * 1024 * 1024), 2, "1.46 MB"},
            {L_, TO_I64(1.951 * 1024 * 1024), 2, "1.95 MB"},
            {L_, TO_I64(1.5 * 1024 * 1024), 3, "1.500 MB"},
            {L_, 2 * 1024 * 1024, 1, "2.0 MB"},
            {L_, 2 * 1024 * 1024, 0, "2 MB"},
            {L_, 1024 * 1024 * 1024, 1, "1.0 GB"},
            {L_, 1024LL * 1024 * 1024 * 1024, 1, "1.0 TB"},
            {L_, 1024LL * 1024 * 1024 * 1024 * 1024, 1, "1.0 PB"},
            {L_, 2048LL * 1024 * 1024 * 1024 * 1024 * 1024, 1, "2.0 EB"},
            {L_, -500, 2, "-500 B"},
            {L_, TO_I64(-1.951 * 1024 * 1024), 2, "-1.95 MB"},
        };
        const int NUM_DATA = sizeof(DATA) / sizeof(*DATA);

        for (int dataIdx = 0; dataIdx < NUM_DATA; ++dataIdx) {
            const TestData& data = DATA[dataIdx];
            const int       LINE = data.d_line;

            // P(LINE);

            bsl::ostringstream ss;
            PrintUtil::printMemory(ss, data.d_arg, data.d_precision);

            int expectedLength =
                PrintUtil::printedMemoryLength(data.d_arg, data.d_precision);

            LOOP_ASSERT_EQUALS(LINE, ss.str(), data.d_expected_p);
            LOOP_ASSERT_EQUALS(LINE,
                               static_cast<int>(ss.str().length()),
                               expectedLength);
        }

#undef TO_I64

    } break;

    default: {
        cerr << "WARNING: CASE '" << test << "' NOT FOUND." << endl;
        testStatus = -1;
    } break;
    }

    if (testStatus != 255) {
        if ((testAllocator.numMismatches() != 0) ||
            (testAllocator.numBytesInUse() != 0)) {
            bsl::cout << "*** Error " << __FILE__ << "(" << __LINE__
                      << "): test allocator: " << '\n';
            testAllocator.print();
            testStatus++;
        }
    }

    if (testStatus > 0) {
        cerr << "Error, non-zero test status = " << testStatus << "." << endl;
    }
    return testStatus;
}
