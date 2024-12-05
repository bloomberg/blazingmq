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

// bmqu_blobiterator.t.cpp -*-C++-*-
#include <bmqu_blobiterator.h>

#include <bmqtst_blobtestutil.h>
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

}  // close anonymous namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   a) Default ctor
//   b) Non-default ctor
//
// Plan:
//   1) Create iterators and check properties
//
// Testing:
//   BlobPosition()
//   BlobPosition(const bdlbb::Blob *,
//                const bmqu::BlobPosition&,
//                int,
//                bool)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Breathing Test");

    {
        PVV("Default ctor");
        bmqu::BlobIterator obj;

        PVVV("iterator = " << obj);

        ASSERT_EQ(true, obj.atEnd());
        ASSERT_EQ(0, obj.remaining());
        ASSERT_EQ(bmqu::BlobPosition(), obj.position());
        ASSERT(!obj.blob());
    }

    {
        PVV("Non-default ctor");

        const int BUFFER_SIZE = 10;
        const int BLOB_LENGTH = 124;

        bdlbb::PooledBlobBufferFactory factory(
            BUFFER_SIZE,
            bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob blob(&factory, bmqtst::TestHelperUtil::allocator());
        blob.setLength(BLOB_LENGTH);

        bmqu::BlobIterator obj(&blob, bmqu::BlobPosition(12, 0), 4, true);

        PVVV("iterator = " << obj);

        ASSERT_EQ(false, obj.atEnd());
        ASSERT_EQ(4, obj.remaining());
        ASSERT_EQ(bmqu::BlobPosition(12, 0), obj.position());
        ASSERT_EQ(&blob, obj.blob());
    }
}

static void test2_advance()
// ------------------------------------------------------------------------
// ADVANCE TEST
//
// Concerns:
//   a) Starting with 0 length returns correct position and atEnd
//   b) Using length works correctly
//   c) Using number of advancements works correctly
//   d) Position is correctly updated after each move
//
// Plan:
//   1) Create a table of tests
//   2) For each test check 'advance' behavior
//
// Testing:
//   advance();
//   atEnd();
//   position();
//   blob();
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Advance Test");

    const int BUFFER_SIZE = 10;
    const int BLOB_LENGTH = 124;

    const struct TestData {
        int  d_line;
        int  d_startBuffer;       // buf of BlobPosition used to init iter
        int  d_startByte;         // byte of BlobPosition used to init iter
        int  d_lengthOrSteps;     // arg to constructor
        bool d_advancingLength;   // arg to constructor
        int  d_advancements[10];  // advancements to make. 0 terminates
        bool d_expectedAtEnd;     // after all advancements
        int  d_expectedBuffer;    // after all advancements
        int  d_expectedByte;      // after all advancements
    } DATA[] = {
        // Move not until the end
        {L_, 0, 0, 2, false, {1}, false, 0, 1},

        // Move until the end
        {L_, 0, 0, 2, false, {1, 2}, true, 0, 3},

        // Initialize with no length
        {L_, 0, 0, 0, true, {}, true, 0, 0},

        // Move not until the end, advancing length
        {L_, 2, 3, 7, true, {2, 2, 2}, false, 2, 9},

        // Move until the end, advancing length
        {L_, 2, 3, 7, true, {2, 2, 3}, true, 3, 0},

        // Move to end of blob, advancing length
        {L_, 12, 0, 4, true, {1, 3}, true, 13, 0},
    };
    const int NUM_DATA = sizeof(DATA) / sizeof(*DATA);

    for (int dataIdx = 0; dataIdx < NUM_DATA; ++dataIdx) {
        const TestData& data = DATA[dataIdx];

        bdlbb::PooledBlobBufferFactory factory(
            BUFFER_SIZE,
            bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob blob(&factory, bmqtst::TestHelperUtil::allocator());
        blob.setLength(BLOB_LENGTH);

        bmqu::BlobPosition startPos(data.d_startBuffer, data.d_startByte);
        bmqu::BlobIterator iter(&blob,
                                startPos,
                                data.d_lengthOrSteps,
                                data.d_advancingLength);

        for (int i = 0; data.d_advancements[i]; ++i) {
            iter.advance(data.d_advancements[i]);
        }

        ASSERT_EQ_D("line " << data.d_line, iter.blob(), &blob);
        ASSERT_EQ_D("line " << data.d_line,
                    iter.atEnd(),
                    data.d_expectedAtEnd);

        bmqu::BlobPosition expectedPos(data.d_expectedBuffer,
                                       data.d_expectedByte);
        ASSERT_EQ_D("line " << data.d_line, expectedPos, iter.position());
    }
}

static void test3_forwardIterator()
// --------------------------------------------------------------------
// FORWARD ITERATOR TEST
//
// Concerns:
//   a) Comparison (== and !=) works correctly
//   b) Dereferencing (* and ->) works correctly
//   c) Incrementing (pre and post) works correctly
//   d) Default-initialized is a universal end iterator
//   e) Swappable
//   f) Destructible
//   g) Required typedefs are present
//   h) Multipass is supported
//
// Plan:
//   1) Create a table of tests
//   2) For each test check iterator behavior
//
// Testing:
//   operator==
//   operator!=
//   operator*
//   operator->
//   operator++()
//   operator++(int)
//   BlobIterator()
//   ~BlobIterator()
//   BlobIterator(const BlobIterator&)
//   BlobIterator& operator=(const BlobIterator&)
//   bsl::swap(BlobIterator&, BlobIterator&)
//   bsl::iterator_traits<BlobIterator>::reference
// --------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Forward Iterator Test");

    const int BUFFER_SIZE = 10;
    const int BLOB_LENGTH = 124;

    const struct TestData {
        int d_line;
        int d_startBuffer;     // buf of BlobPosition used to init iter
        int d_startByte;       // byte of BlobPosition used to init iter
        int d_length;          // arg to constructor
        int d_expectedBuffer;  // after all advancements
        int d_expectedByte;    // after all advancements
    } DATA[] = {
        // Move until the end
        {L_, 0, 0, 2, 0, 3},

        // Initialize with no length
        {L_, 0, 0, 0, 0, 0},

        // Move until the end, advancing length
        {L_, 2, 3, 7, 3, 0},

        // Move to end of blob, advancing length
        {L_, 12, 0, 4, 13, 0},
    };
    const int NUM_DATA = sizeof(DATA) / sizeof(*DATA);

    for (int dataIdx = 0; dataIdx < NUM_DATA; ++dataIdx) {
        const TestData& data = DATA[dataIdx];

        bdlbb::PooledBlobBufferFactory factory(
            BUFFER_SIZE,
            bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob blob(&factory, bmqtst::TestHelperUtil::allocator());
        blob.setLength(BLOB_LENGTH);
        bmqu::BlobIterator iter(&blob,
                                bmqu::BlobPosition(data.d_startBuffer,
                                                   data.d_startByte),
                                data.d_length,
                                true);
        bmqu::BlobIterator iter2(iter), iter3(iter), iter4, end;
        iter4 = iter;
        ASSERT_EQ_D("line " << data.d_line, iter, iter4);

        for (int i = 0; i < data.d_length; ++i) {
            ASSERT_NE_D("line " << data.d_line, iter, end);
            const bmqu::BlobPosition p(iter.position());
            char                     c;
            const int rc = bmqu::BlobUtil::readNBytes(&c, blob, p, 1);
            ASSERT_EQ_D("line " << data.d_line, 0, rc);
            ASSERT_EQ_D("line " << data.d_line, c, *iter);
            ++iter;
        }
        ASSERT_EQ_D("line " << data.d_line, iter, end);

        for (int i = 0; i < data.d_length; ++i, ++iter3) {
            ASSERT_EQ_D("line " << data.d_line, iter2, iter3);
            ASSERT_NE_D("line " << data.d_line, iter2, end);
            const bmqu::BlobPosition p(iter2.position());
            const bmqu::BlobIterator it(iter2++);
            bsl::iterator_traits<bmqu::BlobIterator>::reference r(*it);
            char                                                c;
            bmqu::BlobUtil::readNBytes(&c, blob, p, 1);
            ASSERT_EQ_D("line " << data.d_line, c, r);
        }
        ASSERT_EQ_D("line " << data.d_line, iter2, iter3);
        ASSERT_EQ_D("line " << data.d_line, iter2, end);
        ASSERT_EQ_D("line " << data.d_line, iter3, end);

        bsl::swap(iter4, end);
        for (int i = 0; i < data.d_length; ++i) {
            ASSERT_NE_D("line " << data.d_line, iter4, end);
            ++end;
        }
        ASSERT_EQ_D("line " << data.d_line, iter4, end);
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
    case 3: test3_forwardIterator(); break;
    case 2: test2_advance(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
