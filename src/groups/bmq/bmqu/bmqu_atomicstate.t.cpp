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

// bmqu_atomicstate.t.cpp                                             -*-C++-*-
#include <bmqu_atomicstate.h>

// BDE
#include <bdlf_bind.h>
#include <bslmt_semaphore.h>
#include <bslmt_threadutil.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

namespace {

void threadFn(bool*              result,
              bmqu::AtomicState* state,
              bslmt::Semaphore*  semaphore)
{
    semaphore->post();
    *result = state->process();
    semaphore->post();
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_cancelThenProcess()
{
    // ------------------------------------------------------------------------
    //
    // Call cancel followed by process
    //
    // ------------------------------------------------------------------------

    bmqu::AtomicState state;
    bool              result;

    result = state.cancel();
    ASSERT_EQ(result, true);

    result = state.process();
    ASSERT_EQ(result, false);
}

static void test2_processThenCancel()
{
    // ------------------------------------------------------------------------
    //
    // Call process followed by cancel
    //
    // ------------------------------------------------------------------------

    bmqu::AtomicState state;
    bool              result;

    result = state.process();
    ASSERT_EQ(result, true);

    result = state.cancel();
    ASSERT_EQ(result, false);
}

static void test3_lockThenProcess()
{
    // ------------------------------------------------------------------------
    //
    // Call tryLock followed by process followed by unlock
    //
    // ------------------------------------------------------------------------

    bmqu::AtomicState         state;
    bool                      result;
    bool                      result2;
    bslmt::Semaphore          semaphore;
    bslmt::ThreadUtil::Handle threadHandle;

    result = state.tryLock();
    ASSERT_EQ(result, true);

    bslmt::ThreadUtil::createWithAllocator(
        &threadHandle,
        bdlf::BindUtil::bind(&threadFn, &result2, &state, &semaphore),
        bmqtst::TestHelperUtil::allocator());
    // wait for the thread to arrive at the start
    semaphore.wait();

    // make sure the thread is not moving
    bslmt::ThreadUtil::microSleep(0, 1);

    int rc = semaphore.tryWait();
    ASSERT_NE(rc, 0);

    state.unlock();

    bslmt::ThreadUtil::join(threadHandle);

    ASSERT_EQ(result2, true);
}

static void test4_lockThenCancelThenProcess()
{
    // ------------------------------------------------------------------------
    //
    // Call tryLock followed by cancel followed by process followed by unlock
    //
    // ------------------------------------------------------------------------

    bmqu::AtomicState state;
    bool              result;

    result = state.tryLock();
    ASSERT_EQ(result, true);

    result = state.cancel();
    ASSERT_EQ(result, true);

    result = state.process();
    ASSERT_EQ(result, false);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);

    switch (_testCase) {
    case 0:
    case 1: test1_cancelThenProcess(); break;
    case 2: test2_processThenCancel(); break;
    case 3: test3_lockThenProcess(); break;
    case 4: test4_lockThenCancelThenProcess(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
