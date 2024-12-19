// Copyright 2014-2023 Bloomberg Finance L.P.
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

// mqba_dispatcher.t.cpp                                              -*-C++-*-
#include <mqba_dispatcher.h>

// MQB
#include <mqbcfg_messages.h>
#include <mqbmock_dispatcher.h>

#include <bmqex_bindutil.h>
#include <bmqex_executionpolicy.h>
#include <bmqex_executionutil.h>
#include <bmqex_executor.h>
#include <bmqsys_time.h>

// BDE
#include <bdlf_bind.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_sstream.h>
#include <bslmt_semaphore.h>
#include <bslmt_threadutil.h>
#include <bsls_assert.h>
#include <bsls_systemclocktype.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// ==================
// struct Synchronize
// ==================

/// Provides a functor to synchronize with. First calls `post()` on the
/// specified `startedSignal` semaphore and then, calls `wait()` on the
/// specified `continueSignal` semaphore.
struct Synchronize {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // ACCESSORS
    void operator()(bslmt::Semaphore* startedSignal,
                    bslmt::Semaphore* continueSignal) const
    {
        startedSignal->post();
        continueSignal->wait();
    }
};

// =======================
// struct LoadSelfThreadId
// =======================

/// Provides a functor that loads the id of the current thread into the
/// specified output value.
struct LoadSelfThreadId {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // ACCESSORS
    void operator()(bslmt::ThreadUtil::Id* threadId) const
    {
        *threadId = bslmt::ThreadUtil::selfId();
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the component.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    // Create Dispatcher
    mqbcfg::DispatcherConfig dispatcherConfig;
    dispatcherConfig.sessions().numProcessors() = 1;
    dispatcherConfig.queues().numProcessors()   = 1;
    dispatcherConfig.clusters().numProcessors() = 1;

    bdlmt::EventScheduler eventScheduler(bsls::SystemClockType::e_MONOTONIC,
                                         bmqtst::TestHelperUtil::allocator());
    eventScheduler.start();

    {
        mqba::Dispatcher obj(dispatcherConfig,
                             &eventScheduler,
                             bmqtst::TestHelperUtil::allocator());
    }

    eventScheduler.stop();
}

static void test2_clientTypeEnumValues()
// ------------------------------------------------------------------------
// CLIENT TYPE ENUM VALUES
//
// Concerns:
//   Because we create a 'bsl::vector<DispatcherContext>' which index
//   corresponds to the 'mqbi::DispatcherClientType::Enum' values, make
//   sure those are starting from 0 and consecutive.
//
// Plan:
//
// Testing:
//   mqbi::DispatcherClientType::Enum
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLIENT TYPE ENUM VALUES");

    BMQTST_ASSERT_EQ(mqbi::DispatcherClientType::e_SESSION, 0);
    BMQTST_ASSERT_EQ(mqbi::DispatcherClientType::e_QUEUE, 1);
    BMQTST_ASSERT_EQ(mqbi::DispatcherClientType::e_CLUSTER, 2);

    // For some reason, this doesn't compile to put the constant value directly
    // in the ASSERT, so make an alias for it.
    int count = mqbi::DispatcherClientType::k_COUNT;
    BMQTST_ASSERT_EQ(count, 3);
}

static void test3_executorsSupport()
// ------------------------------------------------------------------------
// EXECUTORS SUPPORT
//
// Concerns:
//   Test that the dispatcher provides executors suitable for executing
//   function objects on a dispatcher's associated processor.
//
// Plan:
//   - Create and start a dispatcher having one processor per client type.
//   - Register several clients.
//   - Check that the 'executor' and 'clientExecutor' functions both return
//     a valid executor object, given a valid client registered on the
//     dispatcher.
//   - Check comparison operations on returned executor objects,
//     specifically that two executors compare equal only if both of them
//     refer to the same processor (for executors returned by the
//     'executor' function), or if they both refer to the same client (for
//     executors returned by the 'clientExecutor' function).
//   - Check the correct behavior of 'post' and 'dispatch' functions on
//     returned executors, specifically that 'post' does not block the
//     calling thread pending completion of the submitted functor, and
//     'dispatch' does invoke the submitted functor in-place, if called
//     from within the processor's thread.
//
// Testing:
//   Executors support
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("EXECUTORS SUPPORT");

    // create / start a scheduler
    bdlmt::EventScheduler eventScheduler(bsls::SystemClockType::e_MONOTONIC,
                                         bmqtst::TestHelperUtil::allocator());
    int                   rc = eventScheduler.start();
    BSLS_ASSERT_OPT(rc == 0);

    // create the dispatcher
    mqbcfg::DispatcherConfig dispatcherConfig;

    // configure the dispatched in a way that there is only one processor for
    // client of each type
    dispatcherConfig.sessions().numProcessors()               = 1;
    dispatcherConfig.sessions().processorConfig().queueSize() = 100;
    dispatcherConfig.sessions().processorConfig().queueSizeLowWatermark() = 0;
    dispatcherConfig.sessions().processorConfig().queueSizeHighWatermark() =
        100;

    dispatcherConfig.queues().numProcessors()                            = 1;
    dispatcherConfig.queues().processorConfig().queueSize()              = 100;
    dispatcherConfig.queues().processorConfig().queueSizeLowWatermark()  = 0;
    dispatcherConfig.queues().processorConfig().queueSizeHighWatermark() = 100;

    dispatcherConfig.clusters().numProcessors()               = 1;
    dispatcherConfig.clusters().processorConfig().queueSize() = 100;
    dispatcherConfig.clusters().processorConfig().queueSizeLowWatermark() = 0;
    dispatcherConfig.clusters().processorConfig().queueSizeHighWatermark() =
        100;

    mqba::Dispatcher dispatcher(dispatcherConfig,
                                &eventScheduler,
                                bmqtst::TestHelperUtil::allocator());

    // start the dispatcher
    bsl::stringstream startErr(bmqtst::TestHelperUtil::allocator());
    rc = dispatcher.start(startErr);
    BMQTST_ASSERT(rc == 0);

    // register first client (of type 'e_SESSION')
    mqbmock::DispatcherClient client1(bmqtst::TestHelperUtil::allocator());
    dispatcher.registerClient(&client1, mqbi::DispatcherClientType::e_SESSION);

    // register second client (of type 'e_SESSION')
    mqbmock::DispatcherClient client2(bmqtst::TestHelperUtil::allocator());
    dispatcher.registerClient(&client2, mqbi::DispatcherClientType::e_SESSION);

    // register third client (of type 'e_QUEUE')
    mqbmock::DispatcherClient client3(bmqtst::TestHelperUtil::allocator());
    dispatcher.registerClient(&client3, mqbi::DispatcherClientType::e_QUEUE);

    // test regular executor
    {
        // obtain an executor for first client's processor
        bmqex::Executor executor1 = dispatcher.executor(&client1);
        BMQTST_ASSERT(static_cast<bool>(executor1));

        // obtain executor for second client's processor
        bmqex::Executor executor2 = dispatcher.executor(&client2);
        BMQTST_ASSERT(static_cast<bool>(executor2));

        // executors for the first and the second client do compare equal as
        // the clients used to obtain them have the same types, and therefore
        // the same associated processors
        BMQTST_ASSERT(executor1 == executor2);

        // obtain executor for third client's processor
        bmqex::Executor executor3 = dispatcher.executor(&client3);
        BMQTST_ASSERT(static_cast<bool>(executor3));

        // executors for the second and the third clients do not compare equal
        // as the clients used to obtain them have different types, and
        // therefore different associated processors
        BMQTST_ASSERT(executor2 != executor3);

        // create utility semaphores
        bslmt::Semaphore startedSignal,  // used to sync. with async op.
            continueSignal;              // used to sync. with async op.

        // submit a functor on a processor using the executor's 'post'
        // function, check that 'post' does not block the calling thread
        executor1.post(bdlf::BindUtil::bind(Synchronize(),
                                            &startedSignal,
                                            &continueSignal));

        startedSignal.wait();   // wait for the job to start executing
        continueSignal.post();  // allow the job to complete

        // storages to save thread ids in
        bslmt::ThreadUtil::Id threadId1 = bslmt::ThreadUtil::selfId();
        bslmt::ThreadUtil::Id threadId2 = bslmt::ThreadUtil::selfId();

        // submit two functors to be executed on the same processor using the
        // executor's 'post' function, and wait for the completion of submitted
        // functors
        bmqex::ExecutionUtil::execute(
            bmqex::ExecutionPolicyUtil::twoWay()
                .neverBlocking()
                .useExecutor(executor1)
                .useAllocator(bmqtst::TestHelperUtil::allocator()),
            bdlf::BindUtil::bind(LoadSelfThreadId(), &threadId1))
            .wait();

        bmqex::ExecutionUtil::execute(
            bmqex::ExecutionPolicyUtil::twoWay()
                .neverBlocking()
                .useExecutor(executor1)
                .useAllocator(bmqtst::TestHelperUtil::allocator()),
            bdlf::BindUtil::bind(LoadSelfThreadId(), &threadId2))
            .wait();

        // both functors were invoked in the same thread that is not this
        // thread
        BMQTST_ASSERT(threadId1 == threadId2);
        BMQTST_ASSERT(threadId1 != bslmt::ThreadUtil::selfId());

        // submit a functor on a processor using the executor's 'dispatch'
        // function, check that 'dispatch' does not block the calling thread
        executor2.dispatch(bdlf::BindUtil::bind(Synchronize(),
                                                &startedSignal,
                                                &continueSignal));

        startedSignal.wait();   // wait for the job to start executing
        continueSignal.post();  // allow the job to complete

        // reset thread ids
        threadId1 = bslmt::ThreadUtil::selfId();
        threadId2 = bslmt::ThreadUtil::selfId();

        // submit two functors to be executed on the same processor using the
        // executor's 'dispatch' function, and wait for the completion of
        // submitted functors
        bmqex::ExecutionUtil::execute(
            bmqex::ExecutionPolicyUtil::twoWay()
                .possiblyBlocking()
                .useExecutor(executor2)
                .useAllocator(bmqtst::TestHelperUtil::allocator()),
            bdlf::BindUtil::bind(LoadSelfThreadId(), &threadId1))
            .wait();

        bmqex::ExecutionUtil::execute(
            bmqex::ExecutionPolicyUtil::twoWay()
                .possiblyBlocking()
                .useExecutor(executor2)
                .useAllocator(bmqtst::TestHelperUtil::allocator()),
            bdlf::BindUtil::bind(LoadSelfThreadId(), &threadId2))
            .wait();

        // both functors were invoked in the same thread that is not this
        // thread
        BMQTST_ASSERT(threadId1 == threadId2);
        BMQTST_ASSERT(threadId1 != bslmt::ThreadUtil::selfId());

        // reset thread ids
        threadId1 = bslmt::ThreadUtil::selfId();
        threadId2 = bslmt::ThreadUtil::selfId();

        // submit a functor that, when invoked, will submit another functor
        // via the executor's 'dispatch' function and block the calling thread
        // until the nested functor completes
        bmqex::ExecutionUtil::execute(
            bmqex::ExecutionPolicyUtil::oneWay()
                .alwaysBlocking()
                .useExecutor(executor1)
                .useAllocator(bmqtst::TestHelperUtil::allocator()),
            bmqex::BindUtil::bindExecute(
                bmqex::ExecutionPolicyUtil::oneWay()
                    .alwaysBlocking()
                    .useExecutor(executor3)
                    .useAllocator(bmqtst::TestHelperUtil::allocator()),
                bdlf::BindUtil::bind(LoadSelfThreadId(), &threadId1)));

        // the nested functor was invoked in-place (we know that because
        // otherwise the operation above would not complete)
        BMQTST_ASSERT(threadId1 != bslmt::ThreadUtil::selfId());
    }

    // test client executor
    {
        // obtain executor for first client
        bmqex::Executor executor1 = dispatcher.clientExecutor(&client1);
        BMQTST_ASSERT(static_cast<bool>(executor1));

        // obtain executor for second client
        bmqex::Executor executor2 = dispatcher.clientExecutor(&client2);
        BMQTST_ASSERT(static_cast<bool>(executor2));

        // executors for the first and the second client do not compare equal
        // as ther refer to different clients
        BMQTST_ASSERT(executor1 != executor2);

        // obtain executor for second client again
        bmqex::Executor executor3 = dispatcher.clientExecutor(&client2);
        BMQTST_ASSERT(static_cast<bool>(executor3));

        // executors for the same (second) client do compare equal
        BMQTST_ASSERT(executor2 == executor3);

        // create utility semaphores
        bslmt::Semaphore startedSignal,  // used to sync. with async op.
            continueSignal;              // used to sync. with async op.

        // submit a functor on a processor using the executor's 'post'
        // function, check that 'post' does not block the calling thread
        executor1.post(bdlf::BindUtil::bind(Synchronize(),
                                            &startedSignal,
                                            &continueSignal));

        startedSignal.wait();   // wait for the job to start executing
        continueSignal.post();  // allow the job to complete

        // storages to save thread ids in
        bslmt::ThreadUtil::Id threadId1 = bslmt::ThreadUtil::selfId();
        bslmt::ThreadUtil::Id threadId2 = bslmt::ThreadUtil::selfId();

        // submit two functors to be executed on the same processor and by the
        // same client using the executor's 'post' function, and wait for the
        // completion of submitted functors
        bmqex::ExecutionUtil::execute(
            bmqex::ExecutionPolicyUtil::twoWay()
                .neverBlocking()
                .useExecutor(executor1)
                .useAllocator(bmqtst::TestHelperUtil::allocator()),
            bdlf::BindUtil::bind(LoadSelfThreadId(), &threadId1))
            .wait();

        bmqex::ExecutionUtil::execute(
            bmqex::ExecutionPolicyUtil::twoWay()
                .neverBlocking()
                .useExecutor(executor1)
                .useAllocator(bmqtst::TestHelperUtil::allocator()),
            bdlf::BindUtil::bind(LoadSelfThreadId(), &threadId2))
            .wait();

        // both functors were invoked in the same thread that is not this
        // thread
        BMQTST_ASSERT(threadId1 == threadId2);
        BMQTST_ASSERT(threadId1 != bslmt::ThreadUtil::selfId());

        // submit a functor on a processor using the executor's 'dispatch'
        // function, check that 'dispatch' does not block the calling thread
        executor1.dispatch(bdlf::BindUtil::bind(Synchronize(),
                                                &startedSignal,
                                                &continueSignal));

        startedSignal.wait();   // wait for the job to start executing
        continueSignal.post();  // allow the job to complete

        // reset thread ids
        threadId1 = bslmt::ThreadUtil::selfId();
        threadId2 = bslmt::ThreadUtil::selfId();

        // submit two functors to be executed on the same processor and by the
        // same client using the executor's 'dispatch' function, and wait for
        // the completion of submitted functors
        bmqex::ExecutionUtil::execute(
            bmqex::ExecutionPolicyUtil::twoWay()
                .possiblyBlocking()
                .useExecutor(executor2)
                .useAllocator(bmqtst::TestHelperUtil::allocator()),
            bdlf::BindUtil::bind(LoadSelfThreadId(), &threadId1))
            .wait();

        bmqex::ExecutionUtil::execute(
            bmqex::ExecutionPolicyUtil::twoWay()
                .possiblyBlocking()
                .useExecutor(executor2)
                .useAllocator(bmqtst::TestHelperUtil::allocator()),
            bdlf::BindUtil::bind(LoadSelfThreadId(), &threadId2))
            .wait();

        // both functors were invoked in the same thread that is not this
        // thread
        BMQTST_ASSERT(threadId1 == threadId2);
        BMQTST_ASSERT(threadId1 != bslmt::ThreadUtil::selfId());

        // submit a functor that, when invoked, will submit another functor
        // via the executor's 'dispatch' function and block the calling thread
        // until the nested functor completes
        bmqex::ExecutionUtil::execute(
            bmqex::ExecutionPolicyUtil::oneWay()
                .alwaysBlocking()
                .useExecutor(executor1)
                .useAllocator(bmqtst::TestHelperUtil::allocator()),
            bmqex::BindUtil::bindExecute(
                bmqex::ExecutionPolicyUtil::oneWay()
                    .alwaysBlocking()
                    .useExecutor(executor2)
                    .useAllocator(bmqtst::TestHelperUtil::allocator()),
                bdlf::BindUtil::bind(LoadSelfThreadId(), &threadId1)));

        // the nested functor was invoked in-place (we know that because
        // otherwise the operation above would not complete)
        BMQTST_ASSERT(threadId1 != bslmt::ThreadUtil::selfId());
    }

    // stop the dispatcher
    dispatcher.stop();

    // stop the scheduler
    eventScheduler.stop();
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqsys::Time::initialize();

    switch (_testCase) {
    case 0:
    case 3: test3_executorsSupport(); break;
    case 2: test2_clientTypeEnumValues(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqsys::Time::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
