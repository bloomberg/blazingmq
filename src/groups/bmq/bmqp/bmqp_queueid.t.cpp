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

// bmqp_queueid.t.cpp                                                 -*-C++-*-
#include <bmqp_queueid.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bsl_cstdlib.h>
#include <bsl_ctime.h>
#include <bsl_functional.h>
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

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    {
        // -----------
        // Consistency
        // -----------
        PV("CONSISTENCY");

        BMQTST_ASSERT_EQ(
            bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID,
            bmqp_ctrlmsg::SubQueueIdInfo::DEFAULT_INITIALIZER_SUB_ID);
        // The default subQueueId as specified in this class should equal
        // the default subQueueId as specified for an instance of
        // SubQueueIdInfo because various components in 'mqbblp'
        // (specifically, the queue engines) rely on this assumption.

        // DEFAULT < RESERVED
        BMQTST_ASSERT_LT(bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID,
                         bmqp::QueueId::k_RESERVED_SUBQUEUE_ID);

        // RESERVED < UNASSIGNED
        BMQTST_ASSERT_LT(bmqp::QueueId::k_RESERVED_SUBQUEUE_ID,
                         bmqp::QueueId::k_UNASSIGNED_SUBQUEUE_ID);
    }

    {
        // -------------------
        // Basic Functionality
        // -------------------
        PV("BASIC FUNCTIONALITY");

        int          id    = 5;
        unsigned int subId = 10;

        bmqp::QueueId obj1(id, subId);

        // Accessors
        BMQTST_ASSERT_EQ(obj1.id(), id);
        BMQTST_ASSERT_EQ(obj1.subId(), subId);

        // Manipulators
        id    = 6;
        subId = 11;

        obj1.setId(id).setSubId(subId);

        BMQTST_ASSERT_EQ(obj1.id(), id);
        BMQTST_ASSERT_EQ(obj1.subId(), subId);

        // Equality
        bmqp::QueueId obj2(obj1);

        BMQTST_ASSERT_EQ(obj1, obj2);

        // Constructor with default argument 'subId'
        bmqp::QueueId obj3(id);

        const unsigned int k_DEFAULT_SUBQUEUE_ID =
            bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID;
        BMQTST_ASSERT_EQ(obj3.id(), id);
        BMQTST_ASSERT_EQ(obj3.subId(), k_DEFAULT_SUBQUEUE_ID);
    }
}

static void test2_print()
{
    bmqtst::TestHelper::printTestName("PRINT");

    int          id    = 5;
    unsigned int subId = 10;

    bmqp::QueueId obj(id, subId);

    // Print
    bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
    out.setstate(bsl::ios_base::badbit);
    obj.print(out, 0, -1);

    BMQTST_ASSERT_EQ(out.str(), "");

    out.clear();
    obj.print(out, 0, -1);

    const char* expected = "[ qId = 5 subId = 10 ]";
    BMQTST_ASSERT_EQ(out.str(), expected);

    // operator<<
    out.reset();

    out << obj;

    BMQTST_ASSERT_EQ(out.str(), expected);
}

static void test3_hashAppend()
// ------------------------------------------------------------------------
// HASH APPEND
//
// Concerns:
//   Ensure that 'hashAppend' on 'bmqp::QueueId' is functional.
//
// Plan:
//  1) Generate a 'bmqp::QueueId' object, compute its hash, and verify that
//     'hashAppend' on this object is deterministic by comparing the hash
//     value over many iterations.
//
// Testing:
//   template <class HASH_ALGORITHM>
//   void
//   hashAppend(HASH_ALGORITHM&      hashAlgo,
//              const bmqp::QueueId& queueId)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HASH APPEND");

    {
        PV("HASH FUNCTION DETERMINISTIC");

        const size_t k_NUM_ITERATIONS = 1000;

        bmqp::QueueId obj(0);
        obj.setId(generateRandomInteger(1, 5000))
            .setSubId(generateRandomInteger(1, 5000));

        bsl::hash<bmqp::QueueId>              hasher;
        bsl::hash<bmqp::QueueId>::result_type firstHash = hasher(obj);
        for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
            bslh::DefaultHashAlgorithm algo;
            hashAppend(algo, obj);
            bsl::hash<bmqp::QueueId>::result_type currHash =
                algo.computeHash();
            PVV("[" << i << "] hash: " << currHash);
            BMQTST_ASSERT_EQ_D(i, currHash, firstHash);
        }
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
    case 3: test3_hashAppend(); break;
    case 2: test2_print(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
