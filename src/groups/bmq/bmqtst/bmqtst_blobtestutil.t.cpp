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

// bmqtst_blobtestutil.t.cpp                                          -*-C++-*-
#include <bmqtst_blobtestutil.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_algorithm.h>
#include <bsl_iterator.h>
#include <bsl_string.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_fromString()
// ------------------------------------------------------------------------
// FROM STRING
//
// Concerns:
//   Verify that the 'fromString' method works.
//
// Testing:
//   fromString
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("FROM STRING");

    {
        PVV("FROM STRING - ''");

        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&blob,
                                         "",
                                         bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(blob.length(), 0);
        BMQTST_ASSERT_EQ(blob.numDataBuffers(), 0);
    }

    {
        PVV("FROM STRING - 'a|b'");

        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&blob,
                                         "a|b",
                                         bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(blob.length(), 2);
        BMQTST_ASSERT_EQ(blob.numDataBuffers(), 2);

        // First buffer
        bsl::string buf1(blob.buffer(0).data(), 1U);
        BMQTST_ASSERT_EQ(blob.buffer(0).size(), 1);
        BMQTST_ASSERT_EQ(buf1, "a");

        // Second buffer
        bsl::string buf2(blob.buffer(1).data(), 1U);
        BMQTST_ASSERT_EQ(blob.buffer(1).size(), 1);
        BMQTST_ASSERT_EQ(buf2, "b");
    }

    {
        PVV("FROM STRING - 'ab'");

        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&blob,
                                         "ab",
                                         bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(blob.length(), 2);
        BMQTST_ASSERT_EQ(blob.numDataBuffers(), 1);

        bsl::string buf(blob.buffer(0).data(), 2U);
        BMQTST_ASSERT_EQ(blob.buffer(0).size(), 2);
        BMQTST_ASSERT_EQ(buf, "ab");
    }

    {
        PVV("FROM STRING - 'aXX'");
        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&blob,
                                         "aXX",
                                         bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(blob.length(), 1);
        BMQTST_ASSERT_EQ(blob.numDataBuffers(), 1);
        BMQTST_ASSERT_EQ(blob.buffer(0).size(), 3);

        bsl::string buf(blob.buffer(0).data(), 1U);
        BMQTST_ASSERT_EQ(buf, "a");
    }
}

static void test2_toString()
// ------------------------------------------------------------------------
// TO STRING
//
// Concerns:
//   Verify that the 'toString' method works.
//
// Testing:
//   toString
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("TO STRING");

    {
        PVV("TO STRING - 'abcdefg'");

        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&blob,
                                         "abcdefg",
                                         bmqtst::TestHelperUtil::allocator());
        BSLS_ASSERT_OPT(blob.length() == 7);
        BSLS_ASSERT_OPT(blob.numDataBuffers() == 1);

        bsl::string out(bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ("abcdefg",
                         bmqtst::BlobTestUtil::toString(&out, blob));
    }

    {
        PVV("TO STRING - 'a|b'");

        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&blob,
                                         "a|b",
                                         bmqtst::TestHelperUtil::allocator());
        BSLS_ASSERT_OPT(blob.length() == 2);
        BSLS_ASSERT_OPT(blob.numDataBuffers() == 2);

        bsl::string out(bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ("a|b",
                         bmqtst::BlobTestUtil::toString(&out, blob, true));
    }

    {
        PVV("TO STRING - 'a|bXXX'");

        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&blob,
                                         "a|bXXX",
                                         bmqtst::TestHelperUtil::allocator());
        BSLS_ASSERT_OPT(blob.length() == 2);
        BSLS_ASSERT_OPT(blob.numDataBuffers() == 2);
        BSLS_ASSERT_OPT(blob.totalSize() == 5);

        bsl::string out(bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ("a|bXXX",
                         bmqtst::BlobTestUtil::toString(&out, blob, true));
    }

    {
        PVV("TO STRING - 'abc|def|g'");

        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());
        bmqtst::BlobTestUtil::fromString(&blob,
                                         "abc|def|g",
                                         bmqtst::TestHelperUtil::allocator());
        BSLS_ASSERT_OPT(blob.length() == 7);
        BSLS_ASSERT_OPT(blob.numDataBuffers() == 3);

        bsl::string out(bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ("abcdefg",
                         bmqtst::BlobTestUtil::toString(&out, blob));
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
    case 2: test2_toString(); break;
    case 1: test1_fromString(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
