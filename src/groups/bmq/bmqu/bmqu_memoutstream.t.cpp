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

// bmqu_memoutstream.t.cpp                                            -*-C++-*-
#include <bmqu_memoutstream.h>

// BDE
#include <bslma_testallocator.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    // In this test case we do some very strict/precise assertions on
    // allocations/allocator use.  This is to ensure that the stream
    // remains working as efficiently as possible (over time as code
    // evolves).  However these strict numbers here are not the artifacts
    // of how this stream works, but how the BDE provided
    // `bdesb_MemOutStreamBuf` works.  So it is possible that changes in
    // `bdesb_MemOutStreamBuf` will break this test driver.  Said changes
    // will need to be reviewed at that time to determine if they are
    // acceptable (and this test driver needs fixing) or BDE will need to
    // make a new component with the changes and keep
    // `bdesb_MemOutStreamBuf` as it is.
    bslma::TestAllocator alloc(s_allocator_p);

    bmqu::MemOutStream obj(&alloc);

    {
        PV("Test printing");
        obj << 2;
        obj << ":hello world";

        ASSERT_EQ(obj.str(), "2:hello world");
        ASSERT_EQ(alloc.numBlocksInUse(), 1);
        ASSERT_EQ(obj.isEmpty(), false);
        ASSERT_EQ(obj.length(), static_cast<bsl::size_t>(13));
    }

    {
        PV("Test reset");
        obj.reset();
        ASSERT(obj.str().isEmpty());
        ASSERT_EQ(alloc.numBlocksInUse(), 0);
        ASSERT_EQ(obj.isEmpty(), true);
        ASSERT_EQ(obj.length(), static_cast<bsl::size_t>(0));
    }

    {
        PV("Test reserving");

        obj.reserveCapacity(3);
        ASSERT_EQ(alloc.numBlocksInUse(), 1);
        ASSERT_EQ(alloc.numBytesInUse(), 3);
        ASSERT_EQ(obj.isEmpty(), true);
        ASSERT_EQ(obj.length(), static_cast<bsl::size_t>(0));

        obj << "123";  // This should take 3 chars, we don't do C-strings
        ASSERT_EQ(obj.str(), "123");
        ASSERT_EQ(alloc.numBlocksInUse(), 1);
        ASSERT_EQ(alloc.numBytesInUse(), 3);
        ASSERT_EQ(obj.isEmpty(), false);
        ASSERT_EQ(obj.length(), static_cast<bsl::size_t>(3));

        // Did not allocate more than 2 blocks at any time
        ASSERT_LE(alloc.numBlocksMax(), 2);

        // Make it allocate some more memory
        obj << "456";
        ASSERT_EQ(obj.str(), "123456");
        ASSERT_EQ(alloc.numBlocksInUse(), 1);
        ASSERT_EQ(alloc.numBytesInUse(), 6);
        ASSERT_LE(alloc.numBlocksMax(), 2);
        ASSERT_EQ(obj.isEmpty(), false);
        ASSERT_EQ(obj.length(), static_cast<bsl::size_t>(6));
    }
}

static void test2_usageExample()
// ------------------------------------------------------------------------
// Testing:
//   This is the test case from the usage example.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("USAGE EXAMPLE");

    bmqu::MemOutStream obj(30, s_allocator_p);
    obj << "hello world";
    ASSERT_EQ(obj.str(), "hello world");
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_usageExample(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
