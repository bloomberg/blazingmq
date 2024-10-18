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

// bmqu_atomicvalidator.t.cpp                                         -*-C++-*-
#include <bmqu_atomicvalidator.h>

#include <bmqsys_threadutil.h>

// BDE
#include <bdlf_bind.h>
#include <bsl_iostream.h>
#include <bslmt_threadutil.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

/// TBD:
///----
//: o AtomicValidatorGuard
//: o Usage Example

//=============================================================================
//                             TEST PLAN
//-----------------------------------------------------------------------------
//                              *** Overview ***
// TODO

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

void acquireReleaseThread(bmqu::AtomicValidator* validator,
                          bsls::AtomicInt*       runningThreads);

// FUNCTIONS
void acquireReleaseThread(bmqu::AtomicValidator* validator,
                          bsls::AtomicInt*       runningThreads)
{
    bool reported = false;
    while (validator->acquire()) {
        validator->release();

        if (!reported) {
            (*runningThreads)++;
            reported = true;
        }
    }

    if (reported) {
        (*runningThreads)--;
    }
}

}  // close unnamed namespace

//=============================================================================
//                              TEST CASES
//-----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the component.
//
// Plan:
//  1. AtomicValidator
//     1. Verify acquire
//     2. Verify acquire after release
//     3. Verify failure to acquire after invalidate
//     4. Reset and verify acquire
//  2. AtomicValidatorGuard
//     1. Guard on null validator -> valid guard
//     2. Guard on valid validator -> valid guard, acquires in constructor,
//        and successfully releases
//     3. Guard on invalidated validator -> invalid guard
//     4. Guard on valid validator with previous acquire -> valid guard,
//        no acquire in constructor, and releases in destructor.
//     5. Guard on valid validator with *no* previous acquire -> valid
//        guard, acquires in constructor, and releases in destructor.
//
// Testing:
//  Basic Functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    {
        PV("ATOMIC VALIDATOR");

        bmqu::AtomicValidator obj;

        // Acquire
        ASSERT(obj.acquire());

        // Release and acquire
        obj.release();
        ASSERT(obj.acquire());

        // Release and invalidate -> cannot acquire
        ASSERT_PASS(obj.release());
        ASSERT_PASS(obj.invalidate());
        ASSERT(!obj.acquire());

        // Reset -> can acquire again
        ASSERT_PASS(obj.reset());
        ASSERT(obj.acquire());
        ASSERT_PASS(obj.release())
    }

    {
        PV("ATOMIC VALIDATOR GUARD");

        PVV("- VALIDATOR NULL");
        {
            bmqu::AtomicValidatorGuard obj0(0);
            ASSERT(obj0.isValid());
            ASSERT(obj0.release() == 0);
            ASSERT(obj0.isValid());
        }

        bmqu::AtomicValidator validator;

        PVV("- VALIDATOR VALID");
        {
            bmqu::AtomicValidatorGuard obj(&validator);  // ACQUIRE #1

            ASSERT(obj.isValid());

            bmqu::AtomicValidator* releaseRC = obj.release();
            ASSERT(releaseRC == &validator);
            ASSERT(obj.isValid());

            ASSERT_PASS(releaseRC->release());  // RELEASE #1
        }

        PVV("- VALIDATOR INVALID");
        {
            validator.invalidate();

            bmqu::AtomicValidatorGuard obj(&validator);

            ASSERT(!obj.isValid());
            ASSERT(obj.release() == 0);
            ASSERT(obj.isValid());
        }

        PVV("- VALIDATOR VALID, PRE-ACQUIRED");
        {
            validator.reset();
            validator.acquire();  // ACQUIRE #2

            bmqu::AtomicValidatorGuard obj(&validator, 1);  // no acquisition
            ASSERT(obj.isValid());
            // obj.~AtomicValidatorGuard()                        // RELEASE #2
        }
        ASSERT_SAFE_FAIL(validator.release());
        // Verifies release occurred in the destructor above

        PVV("- VALIDATOR VALID, NOT PRE-ACQUIRED");
        {
            bmqu::AtomicValidatorGuard obj(&validator, 0);  // ACQUIRE #3
            ASSERT(obj.isValid());
            // obj.~AtomicValidatorGuard()                        // RELEASE #3
        }
        ASSERT_SAFE_FAIL(validator.release());
        // Verifies release occurred in the destructor above
    }
}

static void test2_atomicValidatorMultiThreaded()
// ------------------------------------------------------------------------
// ATOMIC VALIDATOR - MULTITHREADED
//
// Concerns:
//  a. That the component works and is thread safe
//
// Plan:
//  1. Start several threads just doing acquire/release
//  2. Invalidate the AtomicValidator, and make sure all threads
//     detect it and exit
//
// Testing:
//  AtomicValidator
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ATOMIC VALIDATOR - MULTI-THREADED");

    const int k_NUM_THREADS = 16;

    bmqu::AtomicValidator     validator;
    bsls::AtomicInt           numRunningThreads;
    bslmt::ThreadUtil::Handle handles[k_NUM_THREADS];

    // 1. Start several threads just doing acquire/release
    for (int i = 0; i < k_NUM_THREADS; ++i) {
        int ret = bslmt::ThreadUtil::createWithAllocator(
            &handles[i],
            bmqsys::ThreadUtil::defaultAttributes(),
            bdlf::BindUtil::bindS(s_allocator_p,
                                  &acquireReleaseThread,
                                  &validator,
                                  &numRunningThreads),
            s_allocator_p);
        ASSERT_EQ(ret, 0);
    }

    PV("Waiting for threads to start...");

    while (numRunningThreads != k_NUM_THREADS) {
        bslmt::ThreadUtil::yield();
    }

    // 2. Invalidate the AtomicValidator, and make sure all threads detect it
    //     and exit
    PV("Invalidating...");

    validator.invalidate();

    PV("Waiting for threads to exit...");

    for (int i = 0; i < k_NUM_THREADS; ++i) {
        bslmt::ThreadUtil::join(handles[i]);
    }

    ASSERT_EQ(numRunningThreads, 0);

    // Any further 'acquire' fails
    ASSERT(!validator.acquire());

    // After we reset, acquire starts working again
    validator.reset();

    ASSERT(validator.acquire());
}

//=============================================================================
//                                MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_atomicValidatorMultiThreaded(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
