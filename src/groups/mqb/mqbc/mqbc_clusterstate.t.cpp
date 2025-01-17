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

// mqbc_clusterstate.t.cpp                                            -*-C++-*-
#include <mqbc_clusterstate.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslmf_assert.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

namespace {
// TYPES
typedef bsl::unordered_map<bsl::string, int> TestData;
}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_partitionIdExtractor()
// ------------------------------------------------------------------------
// Testing:
//    PartitionIdExtractor
// ------------------------------------------------------------------------
{
    mqbc::ClusterState::PartitionIdExtractor extractor(
        bmqtst::TestHelperUtil::allocator());

    TestData testData(bmqtst::TestHelperUtil::allocator());
    testData.emplace(bsl::string("test", bmqtst::TestHelperUtil::allocator()),
                     -1);
    testData.emplace(bsl::string("123", bmqtst::TestHelperUtil::allocator()),
                     -1);
    testData.emplace(bsl::string("test.123.test",
                                 bmqtst::TestHelperUtil::allocator()),
                     -1);
    testData.emplace(bsl::string("test.123.test.test",
                                 bmqtst::TestHelperUtil::allocator()),
                     123);
    testData.emplace(bsl::string("test.-1.test.test",
                                 bmqtst::TestHelperUtil::allocator()),
                     -1);

    TestData::const_iterator cIt = testData.begin();
    for (; cIt != testData.end(); ++cIt) {
        int result = extractor.extract(cIt->first);
        BMQTST_ASSERT_EQ(result, cIt->second);
    }
}

static void test2_sanitizer() 
{
    bslma::Allocator *alloc = bmqtst::TestHelperUtil::allocator();

    const char     pattern[] = "^\\S+\\.([0-9]+)\\.\\S+\\.\\S+$";

    bdlpcre::RegEx regex(alloc);
    bsl::string    error(alloc);
    size_t         errorOffset;
    BSLA_MAYBE_UNUSED const int rc = regex.prepare(
        &error,
        &errorOffset,
        pattern,
        bdlpcre::RegEx::k_FLAG_JIT);
    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT(regex.isPrepared());

    bsl::string str("test.123.test.test", alloc);
    bsl::vector<bslstl::StringRef> result(alloc);
    regex.match(&result, str.data(), str.length());
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_sanitizer(); break;
    case 1: test1_partitionIdExtractor(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
