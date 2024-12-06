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

// mqbi_queue.t.cpp                                                   -*-C++-*-
#include <mqbi_queue.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bsl_cstdlib.h>
#include <bsl_ctime.h>
#include <bsl_functional.h>
#include <bsl_string.h>
#include <bsl_unordered_set.h>
#include <bsl_utility.h>
#include <bslh_defaulthashalgorithm.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Return a 15-bit random number between the specified `min` and the
/// specified `max`, inclusive.  The behavior is undefined unless `min >= 0`
/// and `max >= min`.
int generateRandomInteger(int min, int max)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(min >= 0);
    BSLS_ASSERT_OPT(max >= min);

    return min + (bsl::rand() % (max - min + 1));
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_hashAppendSubQueueIdInfo()
// ------------------------------------------------------------------------
// HASH APPEND SUBQUEUEIDINFO
//
// Concerns:
//   Ensure that 'hashAppend' on 'SubQueueIdInfo' is functional.
//
// Plan:
//  1) Generate a 'SubQueueIdInfo', compute its hash, and verify that
//     'hashAppend' on this object is deterministic by comparing the hash
//     value over many iterations.
//  2) Use 'SubQueueIdInfo' as the key type in a 'bsl::unordered_set<KEY>'
//     and verify basic insertion and removal.
//
// Testing:
//   template <class HASH_ALGORITHM>
//   void
//   hashAppend(HASH_ALGORITHM&                     hashAlgo,
//              const bmqp_ctrlmsg::SubQueueIdInfo& subQueueIdInfo)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HASH APPEND SUBQUEUEIDINFO");

    {
        PV("HASH FUNCTION DETERMINISTIC");

        const size_t k_NUM_ITERATIONS = 1000;

        bmqp_ctrlmsg::SubQueueIdInfo obj(bmqtst::TestHelperUtil::allocator());
        obj.appId() = "foobar";
        obj.subId() = generateRandomInteger(1, 100);
        // same as: bslh::Hash<> hasher;
        bsl::hash<bmqp_ctrlmsg::SubQueueIdInfo> hasher;

        bsl::hash<bmqp_ctrlmsg::SubQueueIdInfo>::result_type firstHash =
            hasher(obj);
        for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
            bslh::DefaultHashAlgorithm algo;
            hashAppend(algo, obj);
            bsl::hash<bmqp_ctrlmsg::SubQueueIdInfo>::result_type currHash =
                algo.computeHash();
            PVV("[" << i << "] hash: " << currHash);
            BMQTST_ASSERT_EQ_D(i, currHash, firstHash);
        }
    }

    {
        PV("SUBQUEUE-ID-INFO IS A HASHABLE KEY");

        // obj1 == obj2 != obj3
        bmqp_ctrlmsg::SubQueueIdInfo obj1(bmqtst::TestHelperUtil::allocator());
        obj1.appId() = "foo";
        obj1.subId() = 1;

        bmqp_ctrlmsg::SubQueueIdInfo obj2(obj1,
                                          bmqtst::TestHelperUtil::allocator());

        bmqp_ctrlmsg::SubQueueIdInfo obj3(bmqtst::TestHelperUtil::allocator());
        obj3.appId() = "bar";
        obj3.subId() = 2;

        // Verify
        bsl::unordered_set<bmqp_ctrlmsg::SubQueueIdInfo> infos(
            bmqtst::TestHelperUtil::allocator());

        infos.insert(obj1);  // success
        BMQTST_ASSERT_EQ(infos.count(obj1), static_cast<size_t>(1));
        BMQTST_ASSERT_EQ(infos.count(obj2), static_cast<size_t>(1));
        BMQTST_ASSERT_EQ(infos.count(obj3), static_cast<size_t>(0));

        infos.insert(obj2);  // failure
        BMQTST_ASSERT_EQ(infos.count(obj1), static_cast<size_t>(1));
        BMQTST_ASSERT_EQ(infos.count(obj2), static_cast<size_t>(1));
        BMQTST_ASSERT_EQ(infos.count(obj3), static_cast<size_t>(0));

        infos.insert(obj3);  // success
        BMQTST_ASSERT_EQ(infos.count(obj1), static_cast<size_t>(1));
        BMQTST_ASSERT_EQ(infos.count(obj2), static_cast<size_t>(1));
        BMQTST_ASSERT_EQ(infos.count(obj3), static_cast<size_t>(1));

        infos.erase(obj2);  // success
        BMQTST_ASSERT_EQ(infos.count(obj1), static_cast<size_t>(0));
        BMQTST_ASSERT_EQ(infos.count(obj2), static_cast<size_t>(0));
        BMQTST_ASSERT_EQ(infos.count(obj3), static_cast<size_t>(1));
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    unsigned int seed = bsl::time(NULL);
    bsl::srand(seed);
    PV("Seed: " << seed);

    switch (_testCase) {
    case 0:
    case 1: test1_hashAppendSubQueueIdInfo(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
