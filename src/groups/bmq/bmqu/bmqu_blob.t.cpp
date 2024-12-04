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

// bmqu_blob.t.cpp                                                    -*-C++-*-
#include <bmqtst_blobtestutil.h>
#include <bmqu_blob.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdlbb_pooledblobbufferfactory.h>
#include <bslim_printer.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

struct TestObject {
    char d_field1;
    char d_field2;

    bool operator==(const TestObject& other) const
    {
        return d_field1 == other.d_field1 && d_field2 == other.d_field2;
    }
};

bsl::ostream& operator<<(bsl::ostream& stream, const TestObject& obj)
{
    stream << "[ " << obj.d_field1 << ", " << obj.d_field2 << " ]";
    return stream;
}

}  // close anonymous namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_positionBreathingTest()
// ------------------------------------------------------------------------
// POSITION BREATHING TEST
//
// Concerns:
//   Ensure 'BlobPosition' contains proper values
//
// Plan:
//   1. Test constructors and properties
//   2. Test printing
//
// Testing:
//   BlobPosition
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Position Breathing Test");

    {
        PVV("Default constructor");
        bmqu::BlobPosition obj;
        ASSERT_EQ(obj.buffer(), 0);
        ASSERT_EQ(obj.byte(), 0);
        obj.setBuffer(123);
        obj.setByte(987);
        ASSERT_EQ(obj.buffer(), 123);
        ASSERT_EQ(obj.byte(), 987);
    }

    {
        PVV("Initialized constructor");
        bmqu::BlobPosition obj(12, 98);
        ASSERT_EQ(obj.buffer(), 12);
        ASSERT_EQ(obj.byte(), 98);
        obj.setBuffer(123);
        obj.setByte(987);
        ASSERT_EQ(obj.buffer(), 123);
        ASSERT_EQ(obj.byte(), 987);
    }

    // Verifying printing
    bmqu::BlobPosition position(1, 2);
    {
        bmqu::MemOutStream os(bmqtst::TestHelperUtil::allocator());
        os << position;
        PV("position = " << os.str());
        ASSERT_EQ(os.str(), "[ buffer = 1 byte = 2 ]");
    }

    {
        bmqu::MemOutStream os(bmqtst::TestHelperUtil::allocator());
        bslim::Printer     printer(&os, 0, -1);
        printer.start();
        printer.printAttribute("position", position);
        printer.end();
        PV("position = " << os.str());
        ASSERT_EQ(os.str(), "[ position = [ buffer = 1 byte = 2 ] ]");
    }
}

static void test2_positionComparison()
// ------------------------------------------------------------------------
// POSITION COMPARISON TEST
//
// Concerns:
//   Ensure 'BlobPosition' comparison operators return proper values
//
// Plan:
//   Generate test data and compare results with expected values
//
// Testing:
//   '<', '==' and '!='
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Position Comparison Test");

    using namespace bmqu;
    struct Test {
        int          d_line;
        BlobPosition d_lhs;
        BlobPosition d_rhs;
        bool         d_less;
        bool         d_equal;
        bool         d_different;
    } k_DATA[] = {
        {L_, BlobPosition(0, 0), BlobPosition(0, 0), false, true, false},
        {L_, BlobPosition(2, 3), BlobPosition(2, 3), false, true, false},
        {L_, BlobPosition(0, 1), BlobPosition(0, 0), false, false, true},
        {L_, BlobPosition(0, 0), BlobPosition(0, 1), true, false, true},
        {L_, BlobPosition(0, 0), BlobPosition(1, 0), true, false, true},
        {L_, BlobPosition(0, 5), BlobPosition(1, 0), true, false, true},
        {L_, BlobPosition(0, 5), BlobPosition(0, 3), false, false, true},
        {L_, BlobPosition(1, 5), BlobPosition(1, 0), false, false, true},
        {L_, BlobPosition(2, 5), BlobPosition(1, 10), false, false, true},
        {L_, BlobPosition(2, 5), BlobPosition(2, 6), true, false, true},
        {L_, BlobPosition(2, 6), BlobPosition(2, 5), false, false, true},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": comparing " << test.d_lhs << " vs "
                        << test.d_rhs);

        ASSERT_EQ_D("line " << test.d_line,
                    test.d_lhs < test.d_rhs,
                    test.d_less);
        ASSERT_EQ_D("line " << test.d_line,
                    test.d_lhs == test.d_rhs,
                    test.d_equal);
        ASSERT_EQ_D("line " << test.d_line,
                    test.d_lhs != test.d_rhs,
                    test.d_different);
    }
}

static void test3_sectionBreathingTest()
// ------------------------------------------------------------------------
// SECTION BREATHING TEST
//
// Concerns:
//   Ensure 'BlobSection' contains proper values
//
// Plan:
//   1. Test constructors and properties
//   2. Test printing
//
// Testing:
//   BlobSection
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Section Breathing Test");

    {
        PVV("Default constructor");
        bmqu::BlobSection obj;
        ASSERT_EQ(obj.start(), bmqu::BlobPosition());
        ASSERT_EQ(obj.end(), bmqu::BlobPosition());
        obj.start() = bmqu::BlobPosition(1, 2);
        obj.end()   = bmqu::BlobPosition(9, 8);
        ASSERT_EQ(obj.start(), bmqu::BlobPosition(1, 2));
        ASSERT_EQ(obj.end(), bmqu::BlobPosition(9, 8));
    }

    {
        PVV("Initialized constructor");
        bmqu::BlobSection obj(bmqu::BlobPosition(1, 2),
                              bmqu::BlobPosition(9, 8));
        ASSERT_EQ(obj.start(), bmqu::BlobPosition(1, 2));
        ASSERT_EQ(obj.end(), bmqu::BlobPosition(9, 8));
        obj.start() = bmqu::BlobPosition(3, 4);
        obj.end()   = bmqu::BlobPosition(8, 9);
        ASSERT_EQ(obj.start(), bmqu::BlobPosition(3, 4));
        ASSERT_EQ(obj.end(), bmqu::BlobPosition(8, 9));
    }

    // Verifying printing
    bmqu::BlobSection obj(bmqu::BlobPosition(1, 2), bmqu::BlobPosition(9, 8));
    {
        bmqu::MemOutStream os(bmqtst::TestHelperUtil::allocator());
        os << obj;
        PV("section = " << os.str());
        ASSERT_EQ(os.str(),
                  "[ start = [ buffer = 1 byte = 2 ] "
                  "end = [ buffer = 9 byte = 8 ] ]");
    }

    {
        bmqu::MemOutStream os(bmqtst::TestHelperUtil::allocator());
        bslim::Printer     printer(&os, 0, -1);
        printer.start();
        printer.printAttribute("section", obj);
        printer.end();
        PV("section = " << os.str());
        ASSERT_EQ(os.str(),
                  "[ section = [ start = [ buffer = 1 byte = 2 ] "
                  "end = [ buffer = 9 byte = 8 ] ] ]");
    }
}

static void test4_bufferSize()
// ------------------------------------------------------------------------
// BUFFER SIZE TEST
//
// Concerns:
//   Ensure 'bufferSize' returns proper value
//
// Plan:
//   1. Generate test data
//   2. For each case generate a blob, call `bufferSize` and compare return
//      value with expected one.
//
// Testing:
//   bufferSize
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Buffer Size Test");

    struct Test {
        int         d_line;         // Line
        const char* d_blobFormat;   // Format for the blob
        int         d_bufSizes[5];  // size of buffers, -1 for last buffer
    } k_DATA[] = {
        {L_, "", {-1}},
        {L_, "a", {1, -1}},
        {L_, "ab", {2, -1}},
        {L_, "ab|c", {2, 1, -1}},
        {L_, "ab|cde|fghi", {2, 3, 4, -1}},
        {L_, "ab|cde|fghiXXX", {2, 3, 4, -1}},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": checking blob '" << test.d_blobFormat << "'");

        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&blob,
                                         test.d_blobFormat,
                                         bmqtst::TestHelperUtil::allocator());

        for (int bufIdx = 0; test.d_bufSizes[bufIdx] != -1; ++bufIdx) {
            ASSERT_EQ_D("line " << test.d_line << ", bufIdx " << bufIdx,
                        bmqu::BlobUtil::bufferSize(blob, bufIdx),
                        test.d_bufSizes[bufIdx]);
        }
    }
}

static void test5_isValidPos()
// ------------------------------------------------------------------------
// IS VALID POS BREATHING TEST
//
// Concerns:
//   Ensure 'isValidPos' returns proper values
//
// Plan:
//   Generate test blob, test data, call 'isValidPos' and compare with
//   expected values.
//
// Testing:
//   isValidPos
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Is Valid Pos Test");

    bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
    bmqtst::BlobTestUtil::fromString(&blob,
                                     "ab|cde|f|ghiXX",
                                     bmqtst::TestHelperUtil::allocator());

    struct Test {
        int                d_line;
        bmqu::BlobPosition d_position;
        bool               d_expected;  // expected result
    } k_DATA[] = {
        // All valid positions
        {L_, bmqu::BlobPosition(0, 0), true},
        {L_, bmqu::BlobPosition(0, 1), true},
        {L_, bmqu::BlobPosition(1, 0), true},
        {L_, bmqu::BlobPosition(1, 1), true},
        {L_, bmqu::BlobPosition(1, 2), true},
        {L_, bmqu::BlobPosition(2, 0), true},
        {L_, bmqu::BlobPosition(3, 0), true},
        {L_, bmqu::BlobPosition(3, 1), true},
        {L_, bmqu::BlobPosition(3, 2), true},
        {L_, bmqu::BlobPosition(4, 0), true},  // One past the end
        // Invalid positions
        {L_, bmqu::BlobPosition(0, 2), false},
        {L_, bmqu::BlobPosition(0, 3), false},
        {L_, bmqu::BlobPosition(1, 3), false},
        {L_, bmqu::BlobPosition(3, 3), false},
        {L_, bmqu::BlobPosition(4, 1), false},
        {L_, bmqu::BlobPosition(5, 0), false},
        {L_, bmqu::BlobPosition(5, 1), false},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": checking validity of " << test.d_position);
        ASSERT_EQ_D("line " << test.d_line,
                    bmqu::BlobUtil::isValidPos(blob, test.d_position),
                    test.d_expected);
    }
}

static void test6_positionToOffset()
// ------------------------------------------------------------------------
// POSITION TO OFFSET TEST
//
// Concerns:
//   Ensure 'positionToOffsetSafe' and 'positionToOffset' calculate proper
//   offset based on provided blob position.
//
// Plan:
//   Generate test data, call 'positionToOffsetSafe' and 'positionToOffset'
//   and check output value.
//
// Testing:
//   positionToOffsetSafe
//   positionToOffset
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Position To Offset Test");

    bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
    bmqtst::BlobTestUtil::fromString(&blob,
                                     "ab|cde|f|ghiXX",
                                     bmqtst::TestHelperUtil::allocator());

    struct Test {
        int                d_line;
        bmqu::BlobPosition d_position;
        int                d_expected;
    } k_DATA[] = {
        // All valid positions
        {L_, bmqu::BlobPosition(0, 0), 0},
        {L_, bmqu::BlobPosition(0, 1), 1},
        {L_, bmqu::BlobPosition(1, 0), 2},
        {L_, bmqu::BlobPosition(1, 1), 3},
        {L_, bmqu::BlobPosition(1, 2), 4},
        {L_, bmqu::BlobPosition(2, 0), 5},
        {L_, bmqu::BlobPosition(3, 0), 6},
        {L_, bmqu::BlobPosition(3, 1), 7},
        {L_, bmqu::BlobPosition(3, 2), 8},
        {L_, bmqu::BlobPosition(4, 0), 9},  // One past the end
        // Invalid positions
        {L_, bmqu::BlobPosition(0, 2), -1},
        {L_, bmqu::BlobPosition(0, 3), -1},
        {L_, bmqu::BlobPosition(1, 3), -1},
        {L_, bmqu::BlobPosition(3, 3), -1},
        {L_, bmqu::BlobPosition(4, 1), -1},
        {L_, bmqu::BlobPosition(5, 0), -1},
        {L_, bmqu::BlobPosition(5, 1), -1},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": checking offset of " << test.d_position);

        int offset = -1;
        int rc     = bmqu::BlobUtil::positionToOffsetSafe(&offset,
                                                      blob,
                                                      test.d_position);
        ASSERT_EQ_D("line " << test.d_line, offset, test.d_expected);

        if (test.d_expected >= 0) {
            ASSERT_EQ_D("line " << test.d_line, rc, 0);

            // If valid, also ensure 'raw' isOffset works
            offset = -1;
            bmqu::BlobUtil::positionToOffset(&offset, blob, test.d_position);
            ASSERT_EQ_D("line " << test.d_line, offset, test.d_expected);
        }
        else {
            // if invalid, just check return code
            ASSERT_EQ_D("line " << test.d_line, rc, -1);
        }
    }
}

static void test7_findOffset()
// ------------------------------------------------------------------------
// FIND OFFSET TEST
//
// Concerns:
//   Ensure "findOffset" calculates proper position based on provided
//   offset
//
// Plan:
//   Generate test data, call 'findOffset' and check output value.
//
// Testing:
//   findOffsetSafe
//   findOffset
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Find Offset Test");

    bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
    bmqtst::BlobTestUtil::fromString(&blob,
                                     "ab|cde|f|ghiXX",
                                     bmqtst::TestHelperUtil::allocator());

    PV("Without a start offset");
    {
        struct Test {
            int                d_line;
            int                d_offset;
            int                d_expectedRc;
            bmqu::BlobPosition d_expected;
        } k_DATA[] = {
            // Valid offsets
            {L_, 0, 0, bmqu::BlobPosition(0, 0)},
            {L_, 1, 0, bmqu::BlobPosition(0, 1)},
            {L_, 2, 0, bmqu::BlobPosition(1, 0)},
            {L_, 3, 0, bmqu::BlobPosition(1, 1)},
            {L_, 4, 0, bmqu::BlobPosition(1, 2)},
            {L_, 5, 0, bmqu::BlobPosition(2, 0)},
            {L_, 6, 0, bmqu::BlobPosition(3, 0)},
            {L_, 7, 0, bmqu::BlobPosition(3, 1)},
            {L_, 8, 0, bmqu::BlobPosition(3, 2)},
            {L_, 9, 0, bmqu::BlobPosition(4, 0)},
            // Invalid offsets
            {L_, 10, -3, bmqu::BlobPosition(0, 0)},
            {L_, 11, -3, bmqu::BlobPosition(0, 0)},
        };

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test& test = k_DATA[idx];

            PVV(test.d_line << ": checking position of offset "
                            << test.d_offset);

            bmqu::BlobPosition position;
            int                rc = bmqu::BlobUtil::findOffsetSafe(&position,
                                                    blob,
                                                    test.d_offset);
            ASSERT_EQ_D("line " << test.d_line, rc, test.d_expectedRc);

            if (test.d_expectedRc == 0) {
                ASSERT_EQ_D("line " << test.d_line, position, test.d_expected);

                // Verify the non-safe method
                rc = bmqu::BlobUtil::findOffset(&position,
                                                blob,
                                                test.d_offset);
                ASSERT_EQ_D("line " << test.d_line, rc, test.d_expectedRc);
                ASSERT_EQ_D("line " << test.d_line, position, test.d_expected);
            }
        }
    }

    PV("Using a start offset");
    {
        struct Test {
            int                d_line;
            bmqu::BlobPosition d_start;
            int                d_offset;
            int                d_expectedRc;
            bmqu::BlobPosition d_expected;
        } k_DATA[] = {
            // Start of (0, 1)
            {L_, bmqu::BlobPosition(0, 1), 0, 0, bmqu::BlobPosition(0, 1)},
            {L_, bmqu::BlobPosition(0, 1), 1, 0, bmqu::BlobPosition(1, 0)},
            {L_, bmqu::BlobPosition(0, 1), 2, 0, bmqu::BlobPosition(1, 1)},
            {L_, bmqu::BlobPosition(0, 1), 3, 0, bmqu::BlobPosition(1, 2)},
            {L_, bmqu::BlobPosition(0, 1), 4, 0, bmqu::BlobPosition(2, 0)},
            {L_, bmqu::BlobPosition(0, 1), 5, 0, bmqu::BlobPosition(3, 0)},
            {L_, bmqu::BlobPosition(0, 1), 6, 0, bmqu::BlobPosition(3, 1)},
            {L_, bmqu::BlobPosition(0, 1), 7, 0, bmqu::BlobPosition(3, 2)},
            {L_, bmqu::BlobPosition(0, 1), 8, 0, bmqu::BlobPosition(4, 0)},
            {L_, bmqu::BlobPosition(0, 1), 9, -3, bmqu::BlobPosition(0, 0)},
            {L_, bmqu::BlobPosition(0, 1), 10, -3, bmqu::BlobPosition(0, 0)},
            // Start at (1,0)
            {L_, bmqu::BlobPosition(1, 0), 0, 0, bmqu::BlobPosition(1, 0)},
            {L_, bmqu::BlobPosition(1, 0), 1, 0, bmqu::BlobPosition(1, 1)},
            {L_, bmqu::BlobPosition(1, 0), 2, 0, bmqu::BlobPosition(1, 2)},
            {L_, bmqu::BlobPosition(1, 0), 3, 0, bmqu::BlobPosition(2, 0)},
            {L_, bmqu::BlobPosition(1, 0), 4, 0, bmqu::BlobPosition(3, 0)},
            {L_, bmqu::BlobPosition(1, 0), 5, 0, bmqu::BlobPosition(3, 1)},
            {L_, bmqu::BlobPosition(1, 0), 6, 0, bmqu::BlobPosition(3, 2)},
            {L_, bmqu::BlobPosition(1, 0), 7, 0, bmqu::BlobPosition(4, 0)},
            {L_, bmqu::BlobPosition(1, 0), 8, -3, bmqu::BlobPosition(0, 0)},
            {L_, bmqu::BlobPosition(1, 0), 9, -3, bmqu::BlobPosition(0, 0)},
            // Start at (1, 2)
            {L_, bmqu::BlobPosition(1, 2), 0, 0, bmqu::BlobPosition(1, 2)},
            {L_, bmqu::BlobPosition(1, 2), 1, 0, bmqu::BlobPosition(2, 0)},
            {L_, bmqu::BlobPosition(1, 2), 2, 0, bmqu::BlobPosition(3, 0)},
            {L_, bmqu::BlobPosition(1, 2), 3, 0, bmqu::BlobPosition(3, 1)},
            {L_, bmqu::BlobPosition(1, 2), 4, 0, bmqu::BlobPosition(3, 2)},
            {L_, bmqu::BlobPosition(1, 2), 5, 0, bmqu::BlobPosition(4, 0)},
            {L_, bmqu::BlobPosition(1, 2), 6, -3, bmqu::BlobPosition(0, 0)},
            {L_, bmqu::BlobPosition(1, 2), 7, -3, bmqu::BlobPosition(0, 0)},
            // Start at (3, 2)
            {L_, bmqu::BlobPosition(3, 2), 0, 0, bmqu::BlobPosition(3, 2)},
            {L_, bmqu::BlobPosition(3, 2), 1, 0, bmqu::BlobPosition(4, 0)},
            {L_, bmqu::BlobPosition(3, 2), 2, -3, bmqu::BlobPosition(0, 0)},
            {L_, bmqu::BlobPosition(3, 2), 3, -3, bmqu::BlobPosition(0, 0)},
            // Start at (4, 0) (i.e. one past the end)
            {L_, bmqu::BlobPosition(4, 0), 0, 0, bmqu::BlobPosition(4, 0)},
            {L_, bmqu::BlobPosition(4, 0), 1, -2, bmqu::BlobPosition(0, 0)},
            // Invalid start
            {L_, bmqu::BlobPosition(5, 2), 0, -1, bmqu::BlobPosition(0, 0)},
            {L_, bmqu::BlobPosition(5, 2), 1, -1, bmqu::BlobPosition(0, 0)}};

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test& test = k_DATA[idx];

            PVV(test.d_line << ": checking position of offset "
                            << test.d_offset << " from " << test.d_start);

            bmqu::BlobPosition position;
            int                rc = bmqu::BlobUtil::findOffsetSafe(&position,
                                                    blob,
                                                    test.d_start,
                                                    test.d_offset);
            ASSERT_EQ_D("line " << test.d_line, rc, test.d_expectedRc);

            if (test.d_expectedRc == 0) {
                ASSERT_EQ_D("line " << test.d_line, position, test.d_expected);

                // Verify the non-safe method
                rc = bmqu::BlobUtil::findOffset(&position,
                                                blob,
                                                test.d_start,
                                                test.d_offset);
                ASSERT_EQ_D("line " << test.d_line, rc, test.d_expectedRc);
                ASSERT_EQ_D("line " << test.d_line, position, test.d_expected);
            }
        }
    }
}

static void test8_reserve()
// ------------------------------------------------------------------------
// RESERVE TEST
//
// Concerns:
//   Ensure 'reserve' has proper behavior
//
// Plan:
//   Generate test data, call 'reserve' and check the result
//
// Testing:
//   reserve
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Reserve Test");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        4,
        bmqtst::TestHelperUtil::allocator());

    struct Test {
        int                d_line;
        const char*        d_blobPattern;
        int                d_reserveLength;
        bmqu::BlobPosition d_expected;
    } k_DATA[] = {
        {L_, "", 1, bmqu::BlobPosition(0, 0)},
        {L_, "a", 3, bmqu::BlobPosition(1, 0)},
        {L_, "abcd", 3, bmqu::BlobPosition(1, 0)},
        {L_, "abcd|e", 3, bmqu::BlobPosition(2, 0)},
        {L_, "abcd|efg", 3, bmqu::BlobPosition(2, 0)},
        {L_, "abcd|efgX", 1, bmqu::BlobPosition(1, 3)},
        {L_, "abcd|efgX", 2, bmqu::BlobPosition(1, 3)},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": reserve in blob '" << test.d_blobPattern << "'");

        PVVV("reserve");
        {
            bdlbb::Blob blob(&bufferFactory,
                             bmqtst::TestHelperUtil::allocator());
            bmqtst::BlobTestUtil::fromString(
                &blob,
                test.d_blobPattern,
                bmqtst::TestHelperUtil::allocator());

            int length = blob.length();
            bmqu::BlobUtil::reserve(&blob, test.d_reserveLength);
            ASSERT_EQ_D("line " << test.d_line,
                        blob.length(),
                        length + test.d_reserveLength);
        }
        PVVV("reserve and load the position");
        {
            bdlbb::Blob blob(&bufferFactory,
                             bmqtst::TestHelperUtil::allocator());
            bmqtst::BlobTestUtil::fromString(
                &blob,
                test.d_blobPattern,
                bmqtst::TestHelperUtil::allocator());

            int                length = blob.length();
            bmqu::BlobPosition pos;
            bmqu::BlobUtil::reserve(&pos, &blob, test.d_reserveLength);
            ASSERT_EQ_D("line " << test.d_line,
                        blob.length(),
                        length + test.d_reserveLength);
            ASSERT_EQ_D("line " << test.d_line, pos, test.d_expected);
        }
    }
}

static void test9_writeBytes()
// ------------------------------------------------------------------------
// WRITE BYTES TEST
//
// Concerns:
//   Ensure 'writeBytes' proper overwrites data in the blob.
//
// Plan:
//   Generate test data, call `writeBytes` and compare results with the
//   blob data.
//
// Testing:
//   writeBytes
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Write Bytes Test");

    using namespace bmqu;

    struct Test {
        int                d_line;
        const char*        d_blobPattern;
        bmqu::BlobPosition d_position;
        const char*        d_data;
        const char*        d_resultBlob;
        int                d_expectedRc;
    } k_DATA[] = {
        {L_, "", BlobPosition(0, 0), "", "", 0},
        {L_, "", BlobPosition(0, 0), "a", "", -21},
        {L_, "", BlobPosition(0, 1), "", "", -11},
        {L_, "a", BlobPosition(0, 0), "", "a", 0},
        {L_, "a", BlobPosition(0, 0), "b", "b", 0},
        {L_, "a", BlobPosition(0, 1), "", "a", -11},
        {L_, "a", BlobPosition(0, 1), "b", "a", -11},
        {L_, "ab|cde", BlobPosition(0, 0), "1", "1bcde", 0},
        {L_, "ab|cde", BlobPosition(0, 0), "12", "12cde", 0},
        {L_, "ab|cde", BlobPosition(0, 0), "123", "123de", 0},
        {L_, "ab|cde", BlobPosition(0, 0), "1234", "1234e", 0},
        {L_, "ab|cde", BlobPosition(0, 0), "12345", "12345", 0},
        {L_, "ab|cde", BlobPosition(0, 0), "123456", "abcde", -31},
        {L_, "ab|cde", BlobPosition(0, 1), "1", "a1cde", 0},
        {L_, "ab|cde", BlobPosition(0, 1), "12", "a12de", 0},
        {L_, "ab|cde", BlobPosition(0, 1), "123", "a123e", 0},
        {L_, "ab|cde", BlobPosition(0, 1), "1234", "a1234", 0},
        {L_, "ab|cde", BlobPosition(0, 1), "12345", "abcde", -31},
        {L_, "ab|cde", BlobPosition(1, 0), "1", "ab1de", 0},
        {L_, "ab|cde", BlobPosition(1, 0), "12", "ab12e", 0},
        {L_, "ab|cde", BlobPosition(1, 0), "123", "ab123", 0},
        {L_, "ab|cde", BlobPosition(1, 0), "1234", "abcde", -31},
        {L_, "ab|cde", BlobPosition(1, 1), "1", "abc1e", 0},
        {L_, "ab|cde", BlobPosition(1, 1), "12", "abc12", 0},
        {L_, "ab|cde", BlobPosition(1, 1), "123", "abcde", -31},
        {L_, "ab|cde|fg|h|ij", BlobPosition(1, 1), "123456", "abc123456j", 0}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << " in blob '" << test.d_blobPattern << "':  write '"
                        << test.d_data << "'  bytes starting from  "
                        << test.d_position);

        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&blob,
                                         test.d_blobPattern,
                                         bmqtst::TestHelperUtil::allocator());

        int rc = bmqu::BlobUtil::writeBytes(&blob,
                                            test.d_position,
                                            test.d_data,
                                            bsl::strlen(test.d_data));
        ASSERT_EQ_D("line " << test.d_line, rc, test.d_expectedRc);

        bsl::string resultBlob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::toString(&resultBlob, blob);
        ASSERT_EQ_D("line " << test.d_line,
                    resultBlob,
                    bsl::string(test.d_resultBlob,
                                bmqtst::TestHelperUtil::allocator()));
    }
}

static void test10_appendBlobFromIndex()
// ------------------------------------------------------------------------
// APPEND BLOB FROM INDEX TEST
//
// Concerns:
//   Ensure 'appendBlobFromIndex' proper writes data in the blob.
//
// Plan:
//   Generate test data, call `appendBlobFromIndex` and compare results
//   with the blob data.
//
// Testing:
//   appendBlobFromIndex
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Append Blob From Index Test");

    using namespace bmqu;

    bdlbb::PooledBlobBufferFactory bufferFactory(
        4,
        bmqtst::TestHelperUtil::allocator());

    struct Test {
        int         d_line;
        const char* d_blobPattern;
        const int   d_index;
        const int   d_byte;
        const int   d_length;
        const char* d_data;
        const char* d_resultBlob;
    } k_DATA[] = {{L_, "", 0, 0, 0, "ab|cdeXX", ""},
                  {L_, "", 0, 1, 0, "ab|cdeXX", ""},
                  {L_, "", 1, 0, 0, "ab|cdeXX", ""},
                  {L_, "", 1, 2, 0, "ab|cdeXX", ""},
                  {L_, "", 1, 5, 0, "ab|cdeXX", ""},
                  {L_, "", 0, 0, 1, "ab|cdeXX", "aXXX"},
                  {L_, "", 0, 1, 2, "ab|cdeXX", "bcXX"},
                  {L_, "", 1, 0, 3, "ab|cdeXX", "cdeX"},
                  {L_, "", 1, 2, 1, "ab|cdeXX", "eXXX"},
                  {L_, "wx|yzXX", 0, 0, 1, "ab|cdeXX", "wx|yzaX"},
                  {L_, "wx|yzXX", 0, 1, 2, "ab|cdeXX", "wx|yzbc"},
                  {L_, "wx|yzXX", 1, 0, 3, "ab|cdeXX", "wx|yzcd|eXXX"},
                  {L_, "wx|yzXX", 1, 2, 1, "ab|cdeXX", "wx|yzeX"}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << " in blob '" << test.d_blobPattern << "':  append "
                        << test.d_length << " bytes of '" << test.d_data
                        << "' blob starting from  (" << test.d_index << ", "
                        << test.d_byte << ") position");

        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&blob,
                                         test.d_blobPattern,
                                         bmqtst::TestHelperUtil::allocator());

        bdlbb::Blob src(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&src,
                                         test.d_data,
                                         bmqtst::TestHelperUtil::allocator());

        bmqu::BlobUtil::appendBlobFromIndex(&blob,
                                            src,
                                            test.d_index,
                                            test.d_byte,
                                            test.d_length);

        bsl::string resultBlob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::toString(&resultBlob, blob, true);
        ASSERT_EQ_D("line " << test.d_line,
                    resultBlob,
                    bsl::string(test.d_resultBlob,
                                bmqtst::TestHelperUtil::allocator()));
    }
}

static void test11_sectionSize()
// ------------------------------------------------------------------------
// SECTION SIZE TEST
//
// Concerns:
//   Ensure 'sectionSize' proper calculates in the blob.
//
// Plan:
//   Generate test data, call `sectionSize` and compare results
//   with the expected values.
//
// Testing:
//   sectionSize
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Section Size Test");

    using namespace bmqu;

    struct Test {
        int                d_line;
        const char*        d_blobPattern;
        const BlobPosition d_start;
        const BlobPosition d_end;
        const int          d_rc;
        const int          d_size;
    } k_DATA[] = {
        {L_, "", BlobPosition(0, 1), BlobPosition(0, 0), -1, 0},
        {L_, "", BlobPosition(1, 0), BlobPosition(1, 1), -1, 0},
        {L_, "", BlobPosition(0, 0), BlobPosition(1, 1), -1, 0},
        {L_, "", BlobPosition(0, 0), BlobPosition(0, 0), 0, 0},
        {L_, "ab|cdXX", BlobPosition(2, 0), BlobPosition(1, 2), -1, 0},
        {L_, "ab|cdXX", BlobPosition(2, 0), BlobPosition(2, 2), -1, 0},
        {L_, "ab|cdXX", BlobPosition(1, 0), BlobPosition(2, 2), -1, 0},
        {L_, "ab|cdXX", BlobPosition(0, 1), BlobPosition(2, 0), 0, 3}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << " in blob '" << test.d_blobPattern << "':  section "
                        << "size from " << test.d_start << " to "
                        << test.d_end);

        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&blob,
                                         test.d_blobPattern,
                                         bmqtst::TestHelperUtil::allocator());

        const BlobSection section(test.d_start, test.d_end);
        int               size = 0;
        const int rc = bmqu::BlobUtil::sectionSize(&size, blob, section);

        ASSERT_EQ_D("line " << test.d_line, rc, test.d_rc);
        ASSERT_EQ_D("line " << test.d_line, size, test.d_size);
    }
}

static void test12_compareSection()
// ------------------------------------------------------------------------
// COMPARE SECTION TEST
//
// Concerns:
//   Ensure 'compareSection' return proper resuls.
//
// Plan:
//   Generate test data, call `compareSection` and compare results
//   with the expected values.
//
// Testing:
//   compareSection
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Compare Section Test");

    using namespace bmqu;

    struct Test {
        int                d_line;
        const char*        d_blobPattern;
        const BlobPosition d_pos;
        const char*        d_data;
        const int          d_length;
        const int          d_rc;
        const int          d_cmpResult;
    } k_DATA[] = {{L_, "abcd|efXX", BlobPosition(0, 0), "cdef", 0, 0, 0},
                  {L_, "abcd|efXX", BlobPosition(2, 2), "cdef", 0, 0, 0},
                  {L_, "abcd|efXX", BlobPosition(0, 2), "cdef", 4, 0, 0},
                  {L_, "abcd|efXX", BlobPosition(0, 2), "ddef", 4, 0, 1},
                  {L_, "abcd|efXX", BlobPosition(0, 2), "cdee", 4, 0, -1},
                  {L_, "abcd|efXX", BlobPosition(0, 3), "def_", 4, -1, 0}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << " in blob '" << test.d_blobPattern << "':  compare "
                        << test.d_length << " bytes with '" << test.d_data
                        << "' starting from " << test.d_pos);

        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&blob,
                                         test.d_blobPattern,
                                         bmqtst::TestHelperUtil::allocator());

        int       cmpResult = 0;
        const int rc        = bmqu::BlobUtil::compareSection(&cmpResult,
                                                      blob,
                                                      test.d_pos,
                                                      test.d_data,
                                                      test.d_length);

        ASSERT_EQ_D("line " << test.d_line, rc, test.d_rc);
        ASSERT_EQ_D("line " << test.d_line, cmpResult, test.d_cmpResult);
    }
}

static void test13_copyToRawBufferFromIndex()
// ------------------------------------------------------------------------
// COPY TO RAW BUFFER FROM INDEX TEST
//
// Concerns:
//   Ensure 'copyToRawBufferFromIndex' proper writes data in
//   the raw  buffer.
//
// Plan:
//   Generate test data, call `copyToRawBufferFromIndex` and compare
//   results with the expected data.
//
// Testing:
//   copyToRawBufferFromIndex
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Copy To Raw Buffer From Index Test");

    using namespace bmqu;

    struct Test {
        int         d_line;
        const int   d_rawBufferOffset;
        const char* d_blobPattern;
        const int   d_index;
        const int   d_byte;
        const int   d_length;
        const char* d_result;
    } k_DATA[] = {{L_, 0, "ab|cdeXX", 0, 0, 0, "1234567"},
                  {L_, 0, "ab|cdeXX", 0, 1, 0, "1234567"},
                  {L_, 0, "ab|cdeXX", 1, 0, 0, "1234567"},
                  {L_, 0, "ab|cdeXX", 1, 2, 0, "1234567"},
                  {L_, 0, "ab|cdeXX", 1, 5, 0, "1234567"},
                  {L_, 0, "ab|cdeXX", 0, 0, 1, "a234567"},
                  {L_, 1, "ab|cdeXX", 0, 1, 2, "1bc4567"},
                  {L_, 2, "ab|cdeXX", 1, 0, 3, "12cde67"},
                  {L_, 4, "ab|cdeXX", 1, 2, 1, "1234e67"},
                  {L_, 6, "ab|cdeXX", 0, 0, 1, "123456a"},
                  {L_, 5, "ab|cdeXX", 0, 1, 2, "12345bc"},
                  {L_, 4, "ab|cdeXX", 1, 0, 3, "1234cde"},
                  {L_, 1, "ab|cdeXX", 0, 0, 5, "1abcde7"}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        const size_t NUM = 7;
        char         dst[NUM];
        for (size_t i = 0; i < NUM; ++i) {
            dst[i] = '1' + i;
        }

        PVV(test.d_line << " with offset of " << test.d_rawBufferOffset
                        << " bytes in destination buffer copy "
                        << test.d_length << " bytes of '" << test.d_blobPattern
                        << "' blob starting from  (" << test.d_index << ", "
                        << test.d_byte << ") position");

        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&blob,
                                         test.d_blobPattern,
                                         bmqtst::TestHelperUtil::allocator());

        bmqu::BlobUtil::copyToRawBufferFromIndex(dst + test.d_rawBufferOffset,
                                                 blob,
                                                 test.d_index,
                                                 test.d_byte,
                                                 test.d_length);

        ASSERT_EQ_D("line " << test.d_line,
                    bsl::string(dst, NUM, bmqtst::TestHelperUtil::allocator()),
                    bsl::string(test.d_result,
                                bmqtst::TestHelperUtil::allocator()));
    }
}

static void test14_readUpToNBytes()
// ------------------------------------------------------------------------
// READ UP TO N BYTES TEST
//
// Concerns:
//   Ensure 'readUpToNBytes' proper reads data into raw buffer from source
//   blob.
//
// Plan:
//   Generate test data, call `readUpToNBytes` and compare
//   results with the expected data.
//
// Testing:
//   readUpToNBytes
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Read Up To N Bytes Test");

    using namespace bmqu;

    bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
    const char* blobPattern = "ab|cde|f|ghiXX";
    bmqtst::BlobTestUtil::fromString(&blob,
                                     blobPattern,
                                     bmqtst::TestHelperUtil::allocator());

    struct Test {
        int                d_line;
        bmqu::BlobPosition d_start;
        int                d_length;
        int                d_expectedRc;
        const char*        d_expected;
    } k_DATA[] = {
        {L_, bmqu::BlobPosition(0, 0), 0, 0, ""},
        {L_, bmqu::BlobPosition(0, 0), 1, 1, "a"},
        {L_, bmqu::BlobPosition(0, 0), 2, 2, "ab"},
        {L_, bmqu::BlobPosition(0, 0), 3, 3, "abc"},
        {L_, bmqu::BlobPosition(0, 0), 4, 4, "abcd"},
        {L_, bmqu::BlobPosition(0, 0), 5, 5, "abcde"},
        {L_, bmqu::BlobPosition(0, 0), 6, 6, "abcdef"},
        {L_, bmqu::BlobPosition(0, 0), 7, 7, "abcdefg"},
        {L_, bmqu::BlobPosition(0, 0), 8, 8, "abcdefgh"},
        {L_, bmqu::BlobPosition(0, 0), 9, 9, "abcdefghi"},
        {L_, bmqu::BlobPosition(0, 0), 10, 9, "abcdefghi"},
        {L_, bmqu::BlobPosition(0, 1), 1, 1, "b"},
        {L_, bmqu::BlobPosition(0, 1), 5, 5, "bcdef"},
        {L_, bmqu::BlobPosition(2, 0), 3, 3, "fgh"},
        {L_, bmqu::BlobPosition(3, 0), 0, 0, ""},
        {L_, bmqu::BlobPosition(3, 0), 1, 1, "g"},
        {L_, bmqu::BlobPosition(3, 0), 2, 2, "gh"},
        {L_, bmqu::BlobPosition(3, 0), 3, 3, "ghi"},
        {L_, bmqu::BlobPosition(3, 0), 12, 3, "ghi"},
        {L_, bmqu::BlobPosition(4, 0), 0, 0, ""},
        {L_, bmqu::BlobPosition(4, 0), 1, -2, ""},  // invalid length
        {L_, bmqu::BlobPosition(5, 1), 0, -1, ""},  // invalid start
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << " in blob pattern '" << blobPattern << "': read "
                        << test.d_length << " bytes starting from "
                        << test.d_start << " position.");

        char buffer[32];

        int rc = bmqu::BlobUtil::readUpToNBytes(buffer,
                                                blob,
                                                test.d_start,
                                                test.d_length);

        ASSERT_EQ_D("line " << test.d_line, rc, test.d_expectedRc);

        if (test.d_expectedRc == 0) {
            ASSERT_EQ_D("line " << test.d_line,
                        bsl::string(buffer,
                                    test.d_length,
                                    bmqtst::TestHelperUtil::allocator()),
                        bsl::string(test.d_expected,
                                    bmqtst::TestHelperUtil::allocator()));
        }
    }
}

static void test15_readNBytes()
// ------------------------------------------------------------------------
// READ N BYTES TEST
//
// Concerns:
//   Ensure 'readNBytes' proper reads data into raw buffer from source
//   blob.
//
// Plan:
//   Generate test data, call `readNBytes` and compare
//   results with the expected data.
//
// Testing:
//   readNBytes
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Read N Bytes Test");

    using namespace bmqu;

    bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
    const char* blobPattern = "ab|cde|f|ghiXX";
    bmqtst::BlobTestUtil::fromString(&blob,
                                     blobPattern,
                                     bmqtst::TestHelperUtil::allocator());

    struct Test {
        int                d_line;
        bmqu::BlobPosition d_start;
        int                d_length;
        int                d_expectedRc;
        const char*        d_expected;
    } k_DATA[] = {
        {L_, bmqu::BlobPosition(0, 0), 0, 0, ""},
        {L_, bmqu::BlobPosition(0, 0), 1, 0, "a"},
        {L_, bmqu::BlobPosition(0, 0), 2, 0, "ab"},
        {L_, bmqu::BlobPosition(0, 0), 3, 0, "abc"},
        {L_, bmqu::BlobPosition(0, 0), 4, 0, "abcd"},
        {L_, bmqu::BlobPosition(0, 0), 5, 0, "abcde"},
        {L_, bmqu::BlobPosition(0, 0), 6, 0, "abcdef"},
        {L_, bmqu::BlobPosition(0, 0), 7, 0, "abcdefg"},
        {L_, bmqu::BlobPosition(0, 0), 8, 0, "abcdefgh"},
        {L_, bmqu::BlobPosition(0, 0), 9, 0, "abcdefghi"},
        {L_, bmqu::BlobPosition(0, 0), 10, -2, "abcdefghi"},  // invalid end
        {L_, bmqu::BlobPosition(0, 1), 1, 0, "b"},
        {L_, bmqu::BlobPosition(0, 1), 5, 0, "bcdef"},
        {L_, bmqu::BlobPosition(2, 0), 3, 0, "fgh"},
        {L_, bmqu::BlobPosition(3, 0), 0, 0, ""},
        {L_, bmqu::BlobPosition(3, 0), 1, 0, "g"},
        {L_, bmqu::BlobPosition(3, 0), 2, 0, "gh"},
        {L_, bmqu::BlobPosition(3, 0), 3, 0, "ghi"},
        {L_, bmqu::BlobPosition(3, 0), 12, -2, "ghi"},
        {L_, bmqu::BlobPosition(4, 0), 0, 0, ""},
        {L_, bmqu::BlobPosition(4, 0), 1, -21, ""},
        // invalid length
        {L_, bmqu::BlobPosition(5, 1), 0, -11, ""},
        // invalid start
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << " in blob pattern '" << blobPattern << "': read "
                        << test.d_length << " bytes starting from "
                        << test.d_start << " position.");

        char buffer[32];

        int rc = bmqu::BlobUtil::readNBytes(buffer,
                                            blob,
                                            test.d_start,
                                            test.d_length);

        ASSERT_EQ_D("line " << test.d_line, rc, test.d_expectedRc);

        if (test.d_expectedRc == 0) {
            ASSERT_EQ_D("line " << test.d_line,
                        bsl::string(buffer,
                                    test.d_length,
                                    bmqtst::TestHelperUtil::allocator()),
                        bsl::string(test.d_expected,
                                    bmqtst::TestHelperUtil::allocator()));
        }
    }
}

static void test16_appendToBlob()
// ------------------------------------------------------------------------
// APPEND TO BLOB TEST
//
// Concerns:
//   Ensure 'appendToBlob' proper appends data to the blob.
//
// Plan:
//   Generate test data, call `appendToBlob` and compare results
//   with the expected data.
//
// Testing:
//   appendToBlob
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Append To Blob Test");

    using namespace bmqu;

    bdlbb::PooledBlobBufferFactory bufferFactory(
        4,
        bmqtst::TestHelperUtil::allocator());

    {
        PVV("appendToBlob with length");

        struct Test {
            int         d_line;
            const char* d_destPattern;
            const char* d_srcPattern;
            const int   d_index;
            const int   d_byte;
            const int   d_length;
            const int   d_rc;
            const char* d_resultPattern;
        } k_DATA[] = {
            {L_, "ab|cdXX", "ef|ghXX", 1, 2, 1, -1, "ab|cdXX"},
            // invalid start
            {L_, "ab|cdXX", "ef|ghXX", 1, 1, 4, -32, "ab|cdXX"},
            // invalid length
            {L_, "ab|cdXX", "ef|ghXX", 0, 1, 0, 0, "ab|cd"},
            {L_, "ab|cdXX", "ef|ghXX", 0, 1, 1, 0, "ab|cd|f"},
            {L_, "ab|cdXX", "ef|ghXX", 0, 1, 2, 0, "ab|cd|f|g"},
            {L_, "ab|cdXX", "ef|ghXX", 0, 1, 3, 0, "ab|cd|f|gh"},
            {L_, "ab|cdXX", "ef|ghXX", 0, 0, 4, 0, "ab|cd|ef|gh"},
        };

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test& test = k_DATA[idx];

            PVV(test.d_line << " in blob '" << test.d_destPattern
                            << "':  append " << test.d_length << " bytes of '"
                            << test.d_srcPattern << "' blob starting from  ("
                            << test.d_index << ", " << test.d_byte
                            << ") position");

            bdlbb::Blob blob(&bufferFactory,
                             bmqtst::TestHelperUtil::allocator());
            bmqtst::BlobTestUtil::fromString(
                &blob,
                test.d_destPattern,
                bmqtst::TestHelperUtil::allocator());

            bdlbb::Blob src(bmqtst::TestHelperUtil::allocator());
            bmqtst::BlobTestUtil::fromString(
                &src,
                test.d_srcPattern,
                bmqtst::TestHelperUtil::allocator());

            const BlobPosition start(test.d_index, test.d_byte);

            const int rc =
                bmqu::BlobUtil::appendToBlob(&blob, src, start, test.d_length);

            bsl::string resultBlob(bmqtst::TestHelperUtil::allocator());
            bmqtst::BlobTestUtil::toString(&resultBlob, blob, true);

            ASSERT_EQ_D("line " << test.d_line, rc, test.d_rc);
            ASSERT_EQ_D("line " << test.d_line,
                        resultBlob,
                        bsl::string(test.d_resultPattern,
                                    bmqtst::TestHelperUtil::allocator()));
        }
    }

    {
        PVV("appendToBlob without length");

        struct Test {
            int         d_line;
            const char* d_destPattern;
            const char* d_srcPattern;
            const int   d_index;
            const int   d_byte;
            const int   d_rc;
            const char* d_resultPattern;
        } k_DATA[] = {
            {L_, "ab|cdXX", "ef|ghXX", 1, 2, -1, "ab|cdXX"},
            // invalid start
            {L_, "ab|cdXX", "ef|ghXX", 0, 0, 0, "ab|cd|ef|gh"},
            {L_, "ab|cdXX", "ef|ghXX", 0, 1, 0, "ab|cd|f|gh"},
            {L_, "ab|cdXX", "ef|ghXX", 1, 0, 0, "ab|cd|gh"},
            {L_, "ab|cdXX", "ef|ghXX", 1, 1, 0, "ab|cd|h"},
            {L_, "ab|cdXX", "ef|ghXX", 2, 0, 0, "ab|cd"},
        };

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test& test = k_DATA[idx];

            PVV(test.d_line << " in blob '" << test.d_destPattern
                            << "':  append " << test.d_srcPattern
                            << "' blob starting from  (" << test.d_index
                            << ", " << test.d_byte << ") position");

            bdlbb::Blob blob(&bufferFactory,
                             bmqtst::TestHelperUtil::allocator());
            bmqtst::BlobTestUtil::fromString(
                &blob,
                test.d_destPattern,
                bmqtst::TestHelperUtil::allocator());

            bdlbb::Blob src(bmqtst::TestHelperUtil::allocator());
            bmqtst::BlobTestUtil::fromString(
                &src,
                test.d_srcPattern,
                bmqtst::TestHelperUtil::allocator());

            const BlobPosition start(test.d_index, test.d_byte);

            const int rc = bmqu::BlobUtil::appendToBlob(&blob, src, start);

            bsl::string resultBlob(bmqtst::TestHelperUtil::allocator());
            bmqtst::BlobTestUtil::toString(&resultBlob, blob, true);

            ASSERT_EQ_D("line " << test.d_line, rc, test.d_rc);
            ASSERT_EQ_D("line " << test.d_line,
                        resultBlob,
                        bsl::string(test.d_resultPattern,
                                    bmqtst::TestHelperUtil::allocator()));
        }
    }
}

static void test17_getAlignedSection()
// ------------------------------------------------------------------------
// GET ALIGNED SECTION TEST
//
// Concerns:
//   Ensure 'getAlignedSectionSafe' and 'getAlignedSection' return proper
//   pointer to buffer containing requested section of blob data.
//
// Plan:
//   Generate test data, call `getAlignedSectionSafe`/`getAlignedSection`
//   and compare results with the expected data.
//
// Testing:
//   getAlignedSectionSafe
//   getAlignedSection
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Get Aligned Section Test");

    using namespace bmqu;

    const int k_ALIGNMENT = 1;

    struct Test {
        enum Res { e_NULL, e_BUFFER, e_STORAGE };

        int         d_line;
        const char* d_blobPattern;
        const int   d_index;
        const int   d_byte;
        const int   d_length;
        const bool  d_copyFromBlob;
        const Res   d_res;
    } k_DATA[] = {
        {L_, "ab|cdXX", 3, 0, 0, false, Test::e_NULL},  // invalid start
        {L_, "ab|cdXX", 3, 0, 0, true, Test::e_NULL},   // invalid start
        {L_, "ab|cdXX", 0, 0, 0, false, Test::e_BUFFER},
        {L_, "ab|cdXX", 0, 0, 0, true, Test::e_BUFFER},
        {L_, "ab|cdXX", 0, 0, 1, false, Test::e_BUFFER},
        {L_, "ab|cdXX", 0, 0, 1, true, Test::e_BUFFER},
        {L_, "ab|cdXX", 0, 0, 2, false, Test::e_BUFFER},
        {L_, "ab|cdXX", 0, 0, 2, true, Test::e_BUFFER},
        {L_, "ab|cdXX", 0, 0, 3, false, Test::e_STORAGE},
        {L_, "ab|cdXX", 0, 0, 3, true, Test::e_STORAGE},
        {L_, "ab|cdXX", 0, 0, 4, false, Test::e_STORAGE},
        {L_, "ab|cdXX", 0, 0, 4, true, Test::e_STORAGE},
        {L_, "ab|cdXX", 0, 0, 5, false, Test::e_NULL},  // invalid length
        {L_, "ab|cdXX", 0, 0, 5, true, Test::e_NULL},   // invalid length
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << " in blob '" << test.d_blobPattern
                        << "':  get section of " << test.d_length
                        << " bytes starting from  (" << test.d_index << ", "
                        << test.d_byte
                        << ") position with copy: " << test.d_copyFromBlob);

        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&blob,
                                         test.d_blobPattern,
                                         bmqtst::TestHelperUtil::allocator());

        const BlobPosition start(test.d_index, test.d_byte);

        char        storageSafe[10];
        const char* resSafe = bmqu::BlobUtil::getAlignedSectionSafe(
            storageSafe,
            blob,
            start,
            test.d_length,
            k_ALIGNMENT,
            test.d_copyFromBlob);

        if (test.d_res == Test::e_NULL) {
            PVVV("null result")
            ASSERT_D("line " << test.d_line, !resSafe);
            continue;
        }

        // Also call "unsafe" version
        char        storage[10];
        const char* res = bmqu::BlobUtil::getAlignedSection(
            storage,
            blob,
            start,
            test.d_length,
            k_ALIGNMENT,
            test.d_copyFromBlob);

        char      expected[10];
        const int rc =
            bmqu::BlobUtil::readNBytes(expected, blob, start, test.d_length);
        ASSERT_EQ_D("line " << test.d_line, rc, 0);

        if (test.d_res == Test::e_BUFFER) {
            PVVV("pointer to blob buffer data")
            ASSERT_EQ_D("line " << test.d_line,
                        resSafe,
                        blob.buffer(start.buffer()).data() + start.byte());
            ASSERT_EQ_D("line " << test.d_line,
                        bsl::string(resSafe, test.d_length),
                        bsl::string(expected, test.d_length));
            ASSERT_EQ_D("line " << test.d_line, resSafe, res);
        }
        else {
            PVVV("pointer to storage")
            ASSERT_EQ_D("line " << test.d_line, resSafe, storageSafe);
            ASSERT_EQ_D("line " << test.d_line, res, storage);

            if (test.d_copyFromBlob) {
                PVVV("copy to storage")
                ASSERT_EQ_D("line " << test.d_line,
                            bsl::string(resSafe, test.d_length),
                            bsl::string(expected, test.d_length));
                ASSERT_EQ_D("line " << test.d_line,
                            bsl::string(res, test.d_length),
                            bsl::string(expected, test.d_length));
            }
        }
    }
}

static void test18_getAlignedObject()
// ------------------------------------------------------------------------
// GET ALIGNED OBJECT TEST
//
// Concerns:
//   Ensure 'getAlignedObjectSafe' and 'getAlignedObject' return proper
//   pointer to requested object.
//
// Plan:
//   Generate test data, call `getAlignedObjectSafe`/`getAlignedObject`
//   and compare results with the expected data.
//
// Testing:
//   getAlignedObjectSafe
//   getAlignedObject
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Get Aligned Object Test");

    // It is assumed that TestObject alignment is 1.
    size_t alignment = bsls::AlignmentFromType<TestObject>::VALUE;
    ASSERT_EQ_D("TestObject alignment", size_t(1), alignment);

    using namespace bmqu;

    struct Test {
        int         d_line;
        const char* d_blobPattern;
        const int   d_index;
        const int   d_byte;
        const bool  d_copyFromBlob;
        const char* d_resData;
    } k_DATA[] = {
        {L_, "ab|cdXX", 3, 0, false, 0},  // invalid start
        {L_, "ab|cdXX", 3, 0, true, 0},   // invalid start
        {L_, "ab|cdXX", 0, 0, false, "ab"},
        {L_, "ab|cdXX", 0, 0, true, "ab"},
        {L_, "ab|cdXX", 0, 1, false, "__"},  // no copy to storage
        {L_, "ab|cdXX", 0, 1, true, "bc"},
        {L_, "ab|cdXX", 1, 0, false, "cd"},
        {L_, "ab|cdXX", 1, 0, true, "cd"},
        {L_, "ab|cdXX", 1, 1, false, 0},  // blob too short
        {L_, "ab|cdXX", 1, 1, true, 0},   // blob too short
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << " in blob '" << test.d_blobPattern
                        << "':  get aligned object of two chars starting "
                        << "from  (" << test.d_index << ", " << test.d_byte
                        << ") position with copy: " << test.d_copyFromBlob);

        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&blob,
                                         test.d_blobPattern,
                                         bmqtst::TestHelperUtil::allocator());

        const BlobPosition start(test.d_index, test.d_byte);

        TestObject  storageSafe = {'_', '_'};
        TestObject* resSafe     = bmqu::BlobUtil::getAlignedObjectSafe(
            &storageSafe,
            blob,
            start,
            test.d_copyFromBlob);

        if (!test.d_resData) {
            PVVV("null result")
            ASSERT_D("line " << test.d_line, !resSafe);
            continue;
        }

        TestObject expected = {test.d_resData[0], test.d_resData[1]};
        ASSERT_EQ_D("line " << test.d_line, *resSafe, expected);

        // Also call "unsafe" version
        TestObject  storage = {'_', '_'};
        TestObject* res     = bmqu::BlobUtil::getAlignedObject(
            &storage,
            blob,
            start,
            test.d_copyFromBlob);

        ASSERT_EQ_D("line " << test.d_line, *res, expected);
    }
}

static void test19_blobStartHexDumper()
// ------------------------------------------------------------------------
// BLOB START HEX DUMPER TEST
//
// Concerns:
//   Ensure 'BlobStartHexDumper' properly prints blob data
//
// Plan:
//   1. Print zero length blob
//   2. Print the whole blob when its size less than the printer length
//   3. Print first n bytes of the blob
//
// Testing:
//   BlobStartHexDumper
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Blob Start Hex Dumper Test");

    {
        PVV("Print zero length blob");
        bdlbb::Blob        blob(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream os(bmqtst::TestHelperUtil::allocator());

        os << bmqu::BlobStartHexDumper(&blob);
        ASSERT_EQ(os.str(), "");
    }

    {
        PVV("Print the whole blob (size <= limit)");
        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&blob,
                                         "abcd|efghXXXX",
                                         bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream os(bmqtst::TestHelperUtil::allocator());

        os << bmqu::BlobStartHexDumper(&blob, 8);
        ASSERT_EQ(os.str(),
                  "     0:   "
                  "61626364 65666768                       "
                  "|abcdefgh        |\n");
    }

    {
        PVV("Print the whole blob (size > limit)");
        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&blob,
                                         "abcd|efghXXXX",
                                         bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream os(bmqtst::TestHelperUtil::allocator());

        os << bmqu::BlobStartHexDumper(&blob, 6);
        ASSERT_EQ(os.str(),
                  "     0:   "
                  "61626364 6566                           "
                  "|abcdef          |\n"
                  "\t+ 2 more bytes. (8 total)");
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
    case 19: test19_blobStartHexDumper(); break;
    case 18: test18_getAlignedObject(); break;
    case 17: test17_getAlignedSection(); break;
    case 16: test16_appendToBlob(); break;
    case 15: test15_readNBytes(); break;
    case 14: test14_readUpToNBytes(); break;
    case 13: test13_copyToRawBufferFromIndex(); break;
    case 12: test12_compareSection(); break;
    case 11: test11_sectionSize(); break;
    case 10: test10_appendBlobFromIndex(); break;
    case 9: test9_writeBytes(); break;
    case 8: test8_reserve(); break;
    case 7: test7_findOffset(); break;
    case 6: test6_positionToOffset(); break;
    case 5: test5_isValidPos(); break;
    case 4: test4_bufferSize(); break;
    case 3: test3_sectionBreathingTest(); break;
    case 2: test2_positionComparison(); break;
    case 1: test1_positionBreathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
