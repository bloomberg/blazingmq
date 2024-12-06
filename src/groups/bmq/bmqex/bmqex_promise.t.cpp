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

// bmqex_promise.t.cpp                                                -*-C++-*-
#include <bmqex_promise.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// BDE
#include <bsl_cstring.h>  // bsl::strcmp
#include <bslma_testallocator.h>
#include <bslmf_movableref.h>
#include <bsls_systemclocktype.h>

// CONVENIENCE
using namespace BloombergLP;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_promise_creators()
// ------------------------------------------------------------------------
// PROMISE CREATORS
//
// Concerns:
//   Ensure proper behavior of creators methods.
//
// Plan:
//   1. Default-construct a 'bmqex::Promise'. Check that it has a shared
//      state.
//
//   2. Clock-construct a 'bmqex::Promise'. Check that it has a shared
//      state.
//
//   3. Move-construct a 'bmqex::Promise'. Check that the moved-to object
//      refers to the shared state of the moved-from object.
//
//   4. Destroy a promise object without making the shared state ready.
//      Check that the shared state contains an 'bmqex::PromiseBroken'
//      exception.
//
// Testing:
//   bmqex::Promise's default constructor
//   bmqex::Promise's clock constructor
//   bmqex::Promise's move constructor
//   bmqex::Promise's destructor
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. default constructor
    {
        bmqex::Promise<int> promise(&alloc);

        // has a shared state
        BMQTST_ASSERT(promise.future().isValid());
    }

    // 2. clock constructor
    {
        bmqex::Promise<int> promise(bsls::SystemClockType::e_REALTIME, &alloc);

        // has a shared state
        BMQTST_ASSERT(promise.future().isValid());
    }

    // 3. move constructor
    {
        // create a moved-from promise
        bmqex::Promise<int> movedFromPromise(&alloc);
        movedFromPromise.setValue(42);

        // create a moved-to promise
        bmqex::Promise<int> movedToPromise(
            bslmf::MovableRefUtil::move(movedFromPromise));

        // moved-to promise contains the shared state of the moved-from promise
        BMQTST_ASSERT_EQ(movedToPromise.future().isValid(), true);
        BMQTST_ASSERT_EQ(movedToPromise.future().get(), 42);
    }

    // 4. destructor
    {
        // create a promise, obtain a future, destroy the promise
        bmqex::Future<int> future;
        {
            bmqex::Promise<int> promise(&alloc);
            future = promise.future();
        }

        // the shared state is ready
        BMQTST_ASSERT(future.isReady());

        // the shared state contains a 'bmqex::PromiseBroken' exception
        bool exceptionThrown = false;
        try {
            future.get();
        }
        catch (const bmqex::PromiseBroken&) {
            exceptionThrown = true;
        }

        BMQTST_ASSERT(exceptionThrown);
    }
}

static void test2_promise_cassignment()
// ------------------------------------------------------------------------
// PROMISE ASSIGNMENT
//
// Concerns:
//   Ensure proper behavior of assignment operators.
//
// Plan:
//   Move-assign a 'bmqex::Promise'. Check that the moved-to object refers
//   to the shared state of the moved-from object.
//
// Testing:
//   bmqex::Promise's move assignment operator
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // create a moved-from promise
    bmqex::Promise<int> movedFromPromise(&alloc);
    movedFromPromise.setValue(42);

    // create a moved-to promise
    bmqex::Promise<int> movedToPromise(&alloc);
    movedToPromise = bslmf::MovableRefUtil::move(movedFromPromise);

    // moved-to promise contains the shared state of the moved-from promise
    BMQTST_ASSERT_EQ(movedToPromise.future().isValid(), true);
    BMQTST_ASSERT_EQ(movedToPromise.future().get(), 42);
}

static void test3_promise_cswap()
// ------------------------------------------------------------------------
// PROMISE SWAP
//
// Concerns:
//   Ensure proper behavior of the 'swap' method.
//
// Plan:
//   Swap two promises and check that their shared state is swapped.
//
// Testing:
//   bmqex::Promise::swap
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // create promises
    bmqex::Promise<int> promise1(&alloc);
    bmqex::Promise<int> promise2(&alloc);

    promise1.setValue(1);
    promise2.setValue(2);

    // do swap
    promise1.swap(promise2);

    // check
    BMQTST_ASSERT_EQ(promise1.future().get(), 2);
    BMQTST_ASSERT_EQ(promise2.future().get(), 1);
}

static void test4_promise_cfuture()
// ------------------------------------------------------------------------
// PROMISE FUTURE
//
// Concerns:
//   Ensure proper behavior of the 'future' method.
//
// Plan:
//   Check that 'future()' returns a future object refers to the promise's
//   associated shared state.
//
// Testing:
//   bmqex::Promise::future
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // create promises
    bmqex::Promise<int> promise(&alloc);
    promise.setValue(42);

    // obtain a future
    bmqex::Future<int> future = promise.future();

    // the future refers to the promise's shared state
    BMQTST_ASSERT_EQ(future.isValid(), true);
    BMQTST_ASSERT_EQ(future.get(), 42);
}

static void test5_promiseBroken_what()
// ------------------------------------------------------------------------
// PROMISEBROKEN WHAT
//
// Concerns:
//   Ensure proper behavior of the 'what' function.
//
// Plan:
//   Check that 'what()' returns a "PromiseBroken" string literal.
//
// Testing:
//   bmqex::PromiseBroken::what
// ------------------------------------------------------------------------
{
    bmqex::PromiseBroken promiseBroken;

    BMQTST_ASSERT(bsl::strcmp(promiseBroken.what(), "PromiseBroken") == 0);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 1: test1_promise_creators(); break;
    case 2: test2_promise_cassignment(); break;
    case 3: test3_promise_cswap(); break;
    case 4: test4_promise_cfuture(); break;
    case 5: test5_promiseBroken_what(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
