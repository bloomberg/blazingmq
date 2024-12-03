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

// mqbs_offsetptr.t.cpp                                               -*-C++-*-
#include <mqbs_offsetptr.h>

// MQB
#include <mqbs_memoryblock.h>

// BDE
#include <bsls_alignedbuffer.h>
#include <bsls_alignmentutil.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

struct TestObj {
    int d_dummy;
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Testing:
//   Verifies the parameterized constructor of 'mqbs::OffsetPtr'.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Breathing Test");

    char                    base[5] = {0};
    bsl::size_t             offset  = 3;
    const mqbs::MemoryBlock memoryBlockObj(&base[0], sizeof(base));
    mqbs::OffsetPtr<char>   offsetPtrObj(memoryBlockObj, offset);

    // Verify the correctness of offset.
    ASSERT_EQ(offsetPtrObj.get(), memoryBlockObj.base() + offset);
}

static void test2_operations()
// ------------------------------------------------------------------------
// Operations Test
//
// Testing:
//   Verifies the accessors of 'mqbs::OffsetPtr'
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Operations Test");

    bsl::size_t offset       = 0;
    const int   value        = 3;
    const int   objAlignment = bsls::AlignmentFromType<TestObj>::VALUE;
    bsls::AlignedBuffer<sizeof(TestObj), objAlignment> alignedBuffer;
    char* baseBuffer = alignedBuffer.buffer();

    const mqbs::MemoryBlock memoryBlockObj(baseBuffer, sizeof(TestObj));

    TestObj testObj;
    testObj.d_dummy = value;

    mqbs::OffsetPtr<TestObj> offsetPtrObj(memoryBlockObj, offset);
    new (offsetPtrObj.get()) TestObj();
    *offsetPtrObj = testObj;
    ASSERT_EQ(offsetPtrObj->d_dummy, value);
}

static void test3_reset()
// ------------------------------------------------------------------------
// Reset Test
//
// Testing:
//   Verifies the manipulator 'reset()' of a 'mqbs::OffsetPtr'.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Reset Test");

    char                    base[64]  = {0};
    bsl::size_t             offset    = 3;
    bsl::size_t             newOffset = 13;
    const mqbs::MemoryBlock memoryBlockObj(&base[0], sizeof(base));

    mqbs::OffsetPtr<char> offsetPtrObj(memoryBlockObj, offset);
    ASSERT_EQ(offsetPtrObj.get(), memoryBlockObj.base() + offset);

    // Verify correctness of offset after a 'reset()'
    offsetPtrObj.reset(memoryBlockObj, newOffset);
    ASSERT_EQ(offsetPtrObj.get(), memoryBlockObj.base() + newOffset);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_reset(); break;
    case 2: test2_operations(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
