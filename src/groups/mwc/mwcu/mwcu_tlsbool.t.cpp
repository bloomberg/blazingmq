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

// mwcu_tlsbool.t.cpp                                                 -*-C++-*-
#include <mwcu_tlsbool.h>

// BDE
#include <bdlf_bind.h>
#include <bsl_vector.h>
#include <bslmt_barrier.h>
#include <bslmt_threadgroup.h>
#include <bslmt_threadutil.h>
#include <bsls_assert.h>
#include <bsls_platform.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Thread function: for each of the values in the specified `values`, set
/// the specified `obj` to that value and wait on the specified `barrier`
/// for synchronization with the other thread.
static void threadFunction(bslmt::Barrier*          barrier,
                           const bsl::vector<bool>& values,
                           mwcu::TLSBool*           obj)
{
    // Make sure value is default initialized to 'false'
    ASSERT_EQ(*obj, false);

    for (size_t i = 0; i < values.size(); ++i) {
        PV_SAFE(bslmt::ThreadUtil::selfIdAsUint64()
                << "[" << i << "]: setting value to " << values[i]
                << ", current value: " << *obj);
        if (i != 0) {
            // Make sure the value is equal to the last one we set
            ASSERT_EQ(*obj, values[i - 1]);
        }

        // Set the new value
        *obj = values[i];

        // Validate the value was correctly set
        ASSERT_EQ(*obj, values[i]);

        // Wait on the barrier
        barrier->wait();
    }
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    {
        PV("Testing default constructor");

        mwcu::TLSBool obj;
        ASSERT_EQ(obj.isValid(), true);

        // Initialized to false by default
        ASSERT_EQ(obj, false);
        ASSERT_EQ(obj.getDefault(false), false);
        ASSERT_EQ(obj.getDefault(true), false);

        obj = true;
        ASSERT_EQ(obj, true);
        ASSERT_EQ(obj.getDefault(false), true);
        ASSERT_EQ(obj.getDefault(true), true);

        obj = false;
        ASSERT_EQ(obj, false);
        ASSERT_EQ(obj.getDefault(false), false);
        ASSERT_EQ(obj.getDefault(true), false);
    }

    {
        PV("Testing constructor with 'false' value");

        mwcu::TLSBool obj(false);
        ASSERT_EQ(obj.isValid(), true);

        // Initialized to false by default
        ASSERT_EQ(obj, false);
        ASSERT_EQ(obj.getDefault(false), false);
        ASSERT_EQ(obj.getDefault(true), false);

        obj = true;
        ASSERT_EQ(obj, true);
        ASSERT_EQ(obj.getDefault(false), true);
        ASSERT_EQ(obj.getDefault(true), true);

        obj = false;
        ASSERT_EQ(obj, false);
        ASSERT_EQ(obj.getDefault(false), false);
        ASSERT_EQ(obj.getDefault(true), false);
    }

    {
        PV("Testing constructor with 'true' value");

        mwcu::TLSBool obj(true);
        ASSERT_EQ(obj.isValid(), true);

        // Initialized to true by default
        ASSERT_EQ(obj, true);
        ASSERT_EQ(obj.getDefault(false), true);
        ASSERT_EQ(obj.getDefault(true), true);

        obj = false;
        ASSERT_EQ(obj, false);
        ASSERT_EQ(obj.getDefault(false), false);
        ASSERT_EQ(obj.getDefault(true), false);

        obj = true;
        ASSERT_EQ(obj, true);
        ASSERT_EQ(obj.getDefault(false), true);
        ASSERT_EQ(obj.getDefault(true), true);
    }
}

static void test2_isValid()
{
    mwctst::TestHelper::printTestName("TESTING IS VALID");

    // MemorySanitizer links with libc++, which allocates a pthread-key at time
    // of first exception. Forcing an exception here prevents a fatal error
    // when our assertion machinery later tries to throw (after this test has
    // intentionally exhausted all available pthread-keys).
    try {
        throw 1;
    }
    catch (int) {
    };

        // Test isValid and default values.  There is a limit of
        // PTHREAD_KEYS_MAX (1024 on Linux) keys that can be created.

#if !defined PTHREAD_KEYS_MAX
    // Somehow, some platforms (e.g., SunOs) don't define that variable so skip
    // this test.
    const int k_PTHREAD_KEYS_MAX = -1;

    PV("PTHREAD_KEYS_MAX undefined, skipping test");
    return;  // RETURN
#else
    const int k_PTHREAD_KEYS_MAX = PTHREAD_KEYS_MAX;
#endif

    if (k_PTHREAD_KEYS_MAX > 4096) {
        // Skip the test if the limit is too high and would render the test
        // potentially too slow.
        PV("PTHREAD_KEYS_MAX too big (" << k_PTHREAD_KEYS_MAX
                                        << "), skipping test");
        return;  // RETURN
    }

    // There is a maximum number of keys that can be created by a process,
    // defined as PTHREAD_KEYS_MAX.  However, it seems that on some platforms,
    // some keys are already used and the full range is therefore not available
    // to the application (e.g. on AIX PTHREAD_KEYS_MAX is 450, but application
    // can only create 445 keys, on Darwin limit is 512 but only 509 or 510 are
    // available, ...).  Figure out what that limit is, by creating keys until
    // creation returns failure.
    bsl::vector<bslmt::ThreadUtil::Key> keys(s_allocator_p);
    keys.resize(k_PTHREAD_KEYS_MAX + 1);

    int createdKeysCount = 0;
    while (createdKeysCount <= k_PTHREAD_KEYS_MAX + 1) {
        int rc = bslmt::ThreadUtil::createKey(&keys[createdKeysCount], 0);
        if (rc != 0) {
            break;  // BREAK
        }
        ++createdKeysCount;
    }

    if (createdKeysCount >= (k_PTHREAD_KEYS_MAX + 1)) {
        // We should not have succeeded in creating '> PTHREAD_KEYS_MAX' keys,
        // looks like the constant and implementation are not consistent.
        BSLS_ASSERT_OPT(false && "Able to create more keys than authorized");
    }

    PV("PTHREAD_KEYS_MAX " << k_PTHREAD_KEYS_MAX
                           << ", available to application: "
                           << createdKeysCount);

    // At this point, we created the max limit number of keys the system can
    // handle, creating any one more should assert, unless providing a default
    // value in the constructor.
#if !defined(BSLS_PLATFORM_OS_AIX) && !defined(BSLS_PLATFORM_OS_DARWIN)
    // It doesn't look like AIX or Darwin handle constructor throwing an
    // ASSERT, so skip that check on those platforms.
    {
        // Constructor without a value, should assert
        ASSERT_OPT_FAIL(mwcu::TLSBool obj);
        ASSERT_OPT_FAIL(mwcu::TLSBool obj(true));
    }
#endif

    {
        PV("Invalid TLSBool with Initial default value of 'true'");
        mwcu::TLSBool obj(true, true);
        ASSERT_EQ(obj.isValid(), false);

        // Initialized to true
        ASSERT_EQ(obj.getDefault(false), false);
        ASSERT_EQ(obj.getDefault(true), true);

        obj = false;  // Should have no effect since invalid
        ASSERT_EQ(obj.getDefault(false), false);
        ASSERT_EQ(obj.getDefault(true), true);

        if (!obj) {
            ASSERT(false && "operator bool should have said 'true'");
        }
    }

    {
        PV("Invalid TLSBool with Initial default value of 'false'");
        mwcu::TLSBool obj(true, false);
        ASSERT_EQ(obj.isValid(), false);

        // Initialized to true
        ASSERT_EQ(obj.getDefault(false), false);
        ASSERT_EQ(obj.getDefault(true), true);

        obj = true;  // Should have no effect since invalid
        ASSERT_EQ(obj.getDefault(false), false);
        ASSERT_EQ(obj.getDefault(true), true);

        if (obj) {
            ASSERT(false && "operator bool should have said 'false'");
        }
    }

    // Properly clean up created keys
    for (int i = 0; i < createdKeysCount; ++i) {
        bslmt::ThreadUtil::deleteKey(keys[i]);
    }
}

static void test3_multithread()
{
    s_ignoreCheckDefAlloc = true;
    // TBD: Despite using bindA, the binding uses default allocator to
    //      allocate.
    s_ignoreCheckGblAlloc = true;
    // Can't ensure no global memory is allocated because
    // 'bslmt::ThreadUtil::create()' uses the global allocator to allocate
    // memory.

    mwctst::TestHelper::printTestName("MULTITHREAD");

    bslmt::Barrier barrier(2);
    mwcu::TLSBool  obj;

    bslmt::ThreadGroup threadGroup(s_allocator_p);

    bool t1Val[] = {false, true, true, true, false, false, true};
    bool t2Val[] = {true, true, true, false, true, false, true};

    bsl::vector<bool> t1Values(t1Val,
                               t1Val + sizeof(t1Val) / sizeof(bool),
                               s_allocator_p);
    bsl::vector<bool> t2Values(t2Val,
                               t2Val + sizeof(t2Val) / sizeof(bool),
                               s_allocator_p);

    // Create threads
    threadGroup.addThread(bdlf::BindUtil::bindS(s_allocator_p,
                                                &threadFunction,
                                                &barrier,
                                                t1Values,
                                                &obj));
    threadGroup.addThread(bdlf::BindUtil::bindS(s_allocator_p,
                                                &threadFunction,
                                                &barrier,
                                                t2Values,
                                                &obj));
    threadGroup.joinAll();
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_multithread(); break;
    case 2: test2_isValid(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
