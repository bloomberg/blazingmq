// Copyright 2026 Bloomberg Finance L.P.
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

// bmqu_atomicgate.t.cpp                                              -*-C++-*-
#include <bmqu_atomicgate.h>

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

void enterThreadFn(bmqu::AtomicGate* gate,
                   bslmt::Semaphore* startSemaphore,
                   bslmt::Semaphore* doneSemaphore)
{
    const int k_NUM_ITERATIONS = 1000;

    startSemaphore->post();

    for (int i = 0; i < k_NUM_ITERATIONS; ++i) {
        BMQTST_ASSERT(gate->tryEnter());
        gate->leave();
    }

    doneSemaphore->post();

    // After this point, the gate might be closed at any moment.
    // Continue execution until we see that the gate is closed:
    while (true) {
        if (gate->tryEnter()) {
            gate->leave();
        }
        else {
            break;
        }
    }
}

void gateKeeperThreadFn(bmqu::GateKeeper* gateKeeper,
                        bslmt::Semaphore* startSemaphore,
                        bslmt::Semaphore* doneSemaphore)
{
    const int k_NUM_ITERATIONS = 1000;

    startSemaphore->post();

    for (int i = 0; i < k_NUM_ITERATIONS; ++i) {
        bmqu::GateKeeper::Status status(*gateKeeper);
        BMQTST_ASSERT(status.isOpen());
    }

    doneSemaphore->post();

    // After this point, the gate might be closed at any moment.
    // Continue execution until we see that the gate is closed:
    while (true) {
        bmqu::GateKeeper::Status status(*gateKeeper);
        if (!status.isOpen()) {
            break;
        }
    }
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_atomicGate_breathingTest()
{
    // ------------------------------------------------------------------------
    //
    // BREATHING TEST - AtomicGate
    //
    // ------------------------------------------------------------------------

    bmqtst::TestHelper::printTestName("BREATHING TEST - AtomicGate");

    // Test open gate
    {
        bmqu::AtomicGate gate(true);  // open
        BMQTST_ASSERT_EQ(gate.tryEnter(), true);
        gate.leave();
    }

    // Test closed gate
    {
        bmqu::AtomicGate gate(false);  // closed
        BMQTST_ASSERT_EQ(gate.tryEnter(), false);
    }
}

static void test2_atomicGate_openCloseSequence()
{
    // ------------------------------------------------------------------------
    //
    // Test open/close sequence
    //
    // ------------------------------------------------------------------------

    bmqtst::TestHelper::printTestName("OPEN/CLOSE SEQUENCE");

    bmqu::AtomicGate gate(false);  // closed

    // Try to enter closed gate
    BMQTST_ASSERT_EQ(gate.tryEnter(), false);

    // Open the gate
    gate.open();

    // Now should be able to enter
    BMQTST_ASSERT_EQ(gate.tryEnter(), true);
    gate.leave();

    // Close and drain
    gate.closeAndDrain();

    // Should not be able to enter
    BMQTST_ASSERT_EQ(gate.tryEnter(), false);
}

static void test3_atomicGate_closeAndDrain()
{
    // ------------------------------------------------------------------------
    //
    // Test closeAndDrain waits for leave
    //
    // ------------------------------------------------------------------------

    bmqtst::TestHelper::printTestName("CLOSE AND DRAIN");

    bmqu::AtomicGate          gate(true);  // open
    bslmt::Semaphore          startSemaphore;
    bslmt::Semaphore          doneSemaphore;
    bslmt::ThreadUtil::Handle threadHandle;

    // Enter the gate
    BMQTST_ASSERT_EQ(gate.tryEnter(), true);

    // Start a thread that will also try to enter
    bslmt::ThreadUtil::createWithAllocator(
        &threadHandle,
        bdlf::BindUtil::bind(&enterThreadFn,
                             &gate,
                             &startSemaphore,
                             &doneSemaphore),
        bmqtst::TestHelperUtil::allocator());

    // Wait for thread to start
    startSemaphore.wait();

    // Wait until thread performs the minimum work
    doneSemaphore.wait();

    // Release this thread's gate usage
    gate.leave();

    // This should break the loop in the thread function
    gate.closeAndDrain();

    // Join thread
    bslmt::ThreadUtil::join(threadHandle);
}

static void test4_gateKeeper_breathingTest()
{
    // ------------------------------------------------------------------------
    //
    // BREATHING TEST - GateKeeper
    //
    // ------------------------------------------------------------------------

    bmqtst::TestHelper::printTestName("BREATHING TEST - GateKeeper");

    bmqu::GateKeeper gateKeeper;

    // Gate starts closed
    {
        bmqu::GateKeeper::Status status(gateKeeper);
        BMQTST_ASSERT_EQ(status.isOpen(), false);
    }

    // Open the gate
    gateKeeper.open();
    {
        bmqu::GateKeeper::Status status(gateKeeper);
        BMQTST_ASSERT_EQ(status.isOpen(), true);
    }

    // Close the gate
    gateKeeper.close();
    {
        bmqu::GateKeeper::Status status(gateKeeper);
        BMQTST_ASSERT_EQ(status.isOpen(), false);
    }
}

static void test5_gateKeeper_multipleOpen()
{
    // ------------------------------------------------------------------------
    //
    // Test multiple open/close calls
    //
    // ------------------------------------------------------------------------

    bmqtst::TestHelper::printTestName("MULTIPLE OPEN/CLOSE");

    bmqu::GateKeeper gateKeeper;

    // Multiple open calls should be safe
    gateKeeper.open();
    gateKeeper.open();  // no-op

    {
        bmqu::GateKeeper::Status status(gateKeeper);
        BMQTST_ASSERT_EQ(status.isOpen(), true);
    }

    // Multiple close calls should be safe
    gateKeeper.close();
    gateKeeper.close();  // no-op

    {
        bmqu::GateKeeper::Status status(gateKeeper);
        BMQTST_ASSERT_EQ(status.isOpen(), false);
    }
}

static void test6_gateKeeper_threadSafety()
{
    // ------------------------------------------------------------------------
    //
    // Test GateKeeper thread safety
    //
    // ------------------------------------------------------------------------

    bmqtst::TestHelper::printTestName("GATEKEEPER THREAD SAFETY");

    bmqu::GateKeeper          gateKeeper;
    bslmt::Semaphore          startSemaphore;
    bslmt::Semaphore          doneSemaphore;
    bslmt::ThreadUtil::Handle threadHandle;

    gateKeeper.open();

    // Start a thread that will try to enter
    bslmt::ThreadUtil::createWithAllocator(
        &threadHandle,
        bdlf::BindUtil::bind(&gateKeeperThreadFn,
                             &gateKeeper,
                             &startSemaphore,
                             &doneSemaphore),
        bmqtst::TestHelperUtil::allocator());

    // Wait for thread to start
    startSemaphore.wait();

    // Wait until thread performs the minimum work
    doneSemaphore.wait();

    // This should break the loop in the thread function
    gateKeeper.close();

    bslmt::ThreadUtil::join(threadHandle);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);

    switch (_testCase) {
    case 0:
    case 1: test1_atomicGate_breathingTest(); break;
    case 2: test2_atomicGate_openCloseSequence(); break;
    case 3: test3_atomicGate_closeAndDrain(); break;
    case 4: test4_gateKeeper_breathingTest(); break;
    case 5: test5_gateKeeper_multipleOpen(); break;
    case 6: test6_gateKeeper_threadSafety(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
