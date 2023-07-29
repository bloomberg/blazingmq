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

// mwcu_operationchain.t.cpp                                          -*-C++-*-
#include <mwcu_operationchain.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// BDE
#include <bdlb_randomdevice.h>
#include <bdlcc_deque.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlmt_threadpool.h>
#include <bsl_functional.h>
#include <bslmf_movableref.h>
#include <bslmt_semaphore.h>
#include <bslmt_threadutil.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_systemclocktype.h>
#include <bsls_systemtime.h>
#include <bsls_timeinterval.h>

// CONVENIENCE
using namespace BloombergLP;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// UTILITY FUNCTIONS

/// Invoke the specified completion `callback` on the specified
/// `threadPool`.
void asyncNullOperation(bdlmt::ThreadPool*           threadPool,
                        const bsl::function<void()>& callback)
{
    // PRECONDITIONS
    BSLS_ASSERT(threadPool);
    BSLS_ASSERT(callback);

    threadPool->enqueueJob(callback);
}

/// Invoke the specified completion `callback` on the specified `threadPool`
/// with the specified `operationId` parameter.
void asyncIdentifiableOperation(bdlmt::ThreadPool* threadPool,
                                unsigned           operationId,
                                const bsl::function<void(unsigned)>& callback)
{
    // PRECONDITIONS
    BSLS_ASSERT(threadPool);
    BSLS_ASSERT(callback);

    threadPool->enqueueJob(bdlf::BindUtil::bind(callback, operationId));
}

/// Return a `bsls::TimeInterval` object encoding a point in time the
/// specified number of `seconds` from now, according to the monotonic
/// clock.
bsls::TimeInterval secondsFromNow(double seconds)
{
    return bsls::SystemTime::nowMonotonicClock() + seconds;
}

// ==================
// struct Synchronize
// ==================

/// A functor providing syncronization.
struct Synchronize {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // ACCESSORS
    void operator()(bslmt::Semaphore* semaphore) const { semaphore->wait(); }
};

// ===============
// struct PushBack
// ===============

/// A functor to store a value in a container.
struct PushBack {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // ACCESSORS
    template <class CONTAINER, class VALUE>
    void operator()(CONTAINER* container, const VALUE& value) const
    {
        container->pushBack(value);
    }
};

// ==========================
// struct PushBackSynchronize
// ==========================

/// A functor to store a value in a container while providing
/// syncronization.
struct PushBackSynchronize {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // ACCESSORS
    template <class CONTAINER, class VALUE>
    void operator()(bslmt::Semaphore* semaphore,
                    CONTAINER*        container,
                    const VALUE&      value) const
    {
        semaphore->wait();
        container->pushBack(value);
    }
};

// =======================
// struct PushBackRndSleep
// =======================

/// A functor to store a value in a container while going to sleep for a
/// random (short) amount of time before that.
struct PushBackRndSleep {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // ACCESSORS
    template <class CONTAINER, class VALUE>
    void operator()(CONTAINER* container, const VALUE& value) const
    {
        unsigned nsec;
        bdlb::RandomDevice::getRandomBytesNonBlocking(
            reinterpret_cast<unsigned char*>(&nsec),
            sizeof(nsec));

        bsls::TimeInterval sleepTime(0, (nsec % 50000000) + 1);
        bslmt::ThreadUtil::sleep(sleepTime);  // from 1 to 50 ms

        container->pushBack(value);
    }
};

// =====================
// struct ThrowException
// =====================

/// A functor that throws an exception of type `int` with value 42.
struct ThrowException {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // ACCESSORS
    BSLS_ANNOTATION_NORETURN
    void operator()() const { throw int(42); }
};

// ====================
// struct NullOperation
// ====================

/// A operation callback that invokes a nullary completion callback
/// in-place.
struct NullOperation {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // ACCESSORS
    template <class CALLBACK>
    void operator()(const CALLBACK& callback) const
    {
        callback();
    }
};

// =================================
// struct ExceptionHandlingOperation
// =================================

/// A exception-handling operation callback.
struct ExceptionHandlingOperation {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // ACCESSORS
    template <class CALLBACK>
    void operator()(bool* exceptionThrown, const CALLBACK& callback) const
    {
        try {
            callback();
            *exceptionThrown = false;
        }
        catch (int) {
            *exceptionThrown = true;
        }
    }
};

}  // close anonymous namespace

// ============================================================================
//                            USAGE EXAMPLE
// ----------------------------------------------------------------------------

// ===================
// struct UsageExample
// ===================

/// A namespace for functions from the usage example.
struct UsageExample {
    // TYPES
    typedef bsl::function<void()> SendCallback;

    typedef bsl::function<void(int clientId, int payload)> ReceiveCallback;

    // CLASS FUNCTIONS

    /// Callback invoked on completion of a `send` operation.
    static void onDataSent();

    /// Callback invoked on completion of a `receive` operation.
    static void onDataReceived(int clientId, int payload);

    /// Send the specified `payload` to the client identified by the
    /// specified `clientId` and invoke the specified
    /// `completionCallback` when the payload is sent.
    static void
    send(int clientId, int payload, const SendCallback& completionCallback);

    /// Receive a payload send to us by another client and invoke the
    /// specified `completionCallback` with the payload and the sender
    /// client ID.
    static void receive(const ReceiveCallback& completionCallback);
};

// -------------------
// struct UsageExample
// -------------------

// CLASS FUNCTIONS
inline void UsageExample::onDataSent()
{
    // NOTHING
}

inline void UsageExample::onDataReceived(int, int)
{
    // NOTHING
}

inline void
UsageExample::send(int, int, const SendCallback& completionCallback)
{
    completionCallback();
}

inline void UsageExample::receive(const ReceiveCallback& completionCallback)
{
    completionCallback(0, 0);
}

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_usageExample()
// ------------------------------------------------------------------------
// USAGE EXAMPLE
//
// Concerns:
//   Ensure the usage example compiles and runs.
//
// Plan:
//   Run the usage example.
// ------------------------------------------------------------------------
{
    // create a chain
    mwcu::OperationChain chain(s_allocator_p);

    static const int k_MY_PAYLOAD   = 42;
    static const int k_MY_CLIENT_ID = 42;

    // receive data from 10 clients
    mwcu::OperationChainLink link(chain.allocator());
    for (int i = 0; i < 10; ++i) {
        link.insert(bdlf::BindUtil::bind(&UsageExample::receive,
                                         bdlf::PlaceHolders::_1),
                    &UsageExample::onDataReceived);
    }

    chain.append(&link);

    // send data to another client
    chain.appendInplace(bdlf::BindUtil::bind(&UsageExample::send,
                                             k_MY_CLIENT_ID,
                                             k_MY_PAYLOAD,
                                             bdlf::PlaceHolders::_1),
                        &UsageExample::onDataSent);

    // execute operations in the chain
    chain.start();

    // for for completion of all executing operations
    chain.join();
}

static void test2_chain_creators(bdlmt::ThreadPool* threadPool)
// ------------------------------------------------------------------------
// CHAIN CREATORS
//
// Concerns:
//   Ensure proper behavior of creators.
//
// Plan:
//   Create a 'mwcu::OperationChain' object and check postconditions.
// ------------------------------------------------------------------------
{
    // PRECONDITIONS
    BSLS_ASSERT(threadPool);

    // constructor #1
    {
        // create chain
        mwcu::OperationChain chain(s_allocator_p);

        // check postconditions
        ASSERT_EQ(chain.isStarted(), false);
        ASSERT_EQ(chain.isRunning(), false);
        ASSERT_EQ(chain.numLinks(), 0u);
        ASSERT_EQ(chain.numOperations(), 0u);
        ASSERT_EQ(chain.allocator(), s_allocator_p);
    }

    // constructor #2
    {
        // create chain
        mwcu::OperationChain chain(true, s_allocator_p);

        // check postconditions
        ASSERT_EQ(chain.isStarted(), true);
        ASSERT_EQ(chain.isRunning(), false);
        ASSERT_EQ(chain.numLinks(), 0u);
        ASSERT_EQ(chain.numOperations(), 0u);
        ASSERT_EQ(chain.allocator(), s_allocator_p);
    }
}

static void test3_chain_startStop(bdlmt::ThreadPool* threadPool)
// ------------------------------------------------------------------------
// CHAIN START STOP
//
// Concerns:
//   Ensure proper behavior of 'start' and 'stop' methods.
//
// Plan:
//   1. Start the chain and make sure operations in the chain starts
//      executing.
//
//   2. Stop the chain and make sure operations in the chain stops
//      executing, while 'stop' does not block the calling thread.
// ------------------------------------------------------------------------
{
    // PRECONDITIONS
    BSLS_ASSERT(threadPool);

    // 1. start
    {
        // store completed operation IDs here
        bdlcc::Deque<unsigned> completionIds(
            bsls::SystemClockType::e_MONOTONIC);

        // create chain
        mwcu::OperationChain chain(s_allocator_p);

        // number of operations in the chain
        static const unsigned k_NUM_OPERATIONS = 3;

        // add several operations
        for (unsigned operationId = 0; operationId < k_NUM_OPERATIONS;
             ++operationId) {
            chain.appendInplace(
                bdlf::BindUtil::bind(&asyncIdentifiableOperation,
                                     threadPool,
                                     operationId,
                                     bdlf::PlaceHolders::_1),
                bdlf::BindUtil::bind(PushBack(),
                                     &completionIds,
                                     bdlf::PlaceHolders::_1));
        }

        // make sure operation haven't started executing
        unsigned completionId;
        ASSERT_EQ(chain.isRunning(), false);
        ASSERT_NE(completionIds.timedPopFront(&completionId,
                                              secondsFromNow(0.25)),
                  0);

        // start executing
        chain.start();

        // start again (should be a no-op)
        chain.start();

        // make sure all operations are executed
        for (unsigned i = 0; i < k_NUM_OPERATIONS; ++i) {
            completionIds.popFront();
        }
    }

    // 2. stop
    {
        // store completed operation IDs here
        bdlcc::Deque<unsigned> completionIds(
            bsls::SystemClockType::e_MONOTONIC);

        // create chain
        mwcu::OperationChain chain(s_allocator_p);

        // used for synchronization
        bslmt::Semaphore semaphore;

        // number of operations in the chain
        static const unsigned k_NUM_OPERATIONS = 3;

        // add several operations
        for (unsigned operationId = 0; operationId < k_NUM_OPERATIONS;
             ++operationId) {
            chain.appendInplace(
                bdlf::BindUtil::bind(&asyncIdentifiableOperation,
                                     threadPool,
                                     operationId,
                                     bdlf::PlaceHolders::_1),
                bdlf::BindUtil::bind(PushBackSynchronize(),
                                     &semaphore,
                                     &completionIds,
                                     bdlf::PlaceHolders::_1));
        }

        // no executing operations
        ASSERT_EQ(chain.isRunning(), false);

        // start executing
        chain.start();

        // stop executing
        chain.stop();

        // stop again (should be a no-op)
        chain.stop();

        // one operation is executing
        ASSERT_EQ(chain.numOperationsExecuting(), 1u);
        ASSERT_EQ(chain.numOperationsPending(), k_NUM_OPERATIONS - 1);

        // allow one executing operation to complete
        semaphore.post(1u);

        // make sure only one operation completed
        unsigned completionId;
        ASSERT_EQ(completionIds.popFront(), 0u);
        ASSERT_NE(completionIds.timedPopFront(&completionId,
                                              secondsFromNow(0.25)),
                  0);
    }
}

static void test4_chain_join(bdlmt::ThreadPool* threadPool)
// ------------------------------------------------------------------------
// CHAIN JOIN
//
// Concerns:
//   Ensure proper behavior of 'join' method.
//
// Plan:
//   Join the chain and make sure the calling thread is blocked until there
//   is no operations executing.
// ------------------------------------------------------------------------
{
    // PRECONDITIONS
    BSLS_ASSERT(threadPool);

    // store completed operation IDs here
    bdlcc::Deque<unsigned> completionIds(bsls::SystemClockType::e_MONOTONIC);

    // create chain
    mwcu::OperationChain chain(s_allocator_p);

    // number of operations in the chain
    static const unsigned k_NUM_OPERATIONS = 32;

    // add several operations
    for (unsigned operationId = 0; operationId < k_NUM_OPERATIONS;
         ++operationId) {
        chain.appendInplace(bdlf::BindUtil::bind(&asyncIdentifiableOperation,
                                                 threadPool,
                                                 operationId,
                                                 bdlf::PlaceHolders::_1),
                            bdlf::BindUtil::bind(PushBackRndSleep(),
                                                 &completionIds,
                                                 bdlf::PlaceHolders::_1));
    }

    // start executing
    chain.start();

    // join
    chain.join();

    // make sure all operations completed
    ASSERT_EQ(chain.isRunning(), false);
    ASSERT_EQ(chain.numOperations(), 0u);
    ASSERT_EQ(completionIds.length(), k_NUM_OPERATIONS);
}

static void test5_chain_append1(bdlmt::ThreadPool* threadPool)
// ------------------------------------------------------------------------
// CHAIN APPEND 1
//
// Concerns:
//   Ensure proper behavior of 'append' method when appending a single
//   link.
//
// Plan:
//   1. Append a single empty link and make sure this operation has no
//      effect.
//
//   2. Append a single non-empty link and make sure the link is inserted
//      to the end of the chain. Also make sure the input link is left
//      empty after the append.
//
//   3. Append a single non-empty link to a started chain and make sure the
//      chain start executing operations.
// ------------------------------------------------------------------------
{
    // PRECONDITIONS
    BSLS_ASSERT(threadPool);

    // 1. append single empty link
    {
        // create chain
        mwcu::OperationChain chain(s_allocator_p);

        // create empty link
        mwcu::OperationChainLink link(chain.allocator());

        // append link
        chain.append(&link);

        // the operation had no effect
        ASSERT_EQ(chain.numLinks(), 0u);
        ASSERT_EQ(chain.numOperations(), 0u);
    }

    // 2. append single non-empty link
    {
        // NOTE: The first link has 1 operation, and each next link has +1
        //       operations. The total number of operations in the chain is a
        //       function of the number of links (N), '(N(N + 1)) / 2'. This
        //       way we know which link is which.

        // number of links
        static const unsigned k_NUM_LINKS = 8;

        // create chain
        mwcu::OperationChain chain(s_allocator_p);

        // append several links
        mwcu::OperationChainLink link(chain.allocator());
        for (unsigned i = 0; i < k_NUM_LINKS; ++i) {
            // number of operations in the link
            const unsigned k_NUM_OPERATIONS = i + 1;

            // fill link
            for (unsigned j = 0; j < k_NUM_OPERATIONS; ++j) {
                link.insert(NullOperation());
            }

            // append link
            chain.append(&link);

            // appended
            ASSERT_EQ(chain.numLinks(), i + 1);
            ASSERT_EQ(chain.numOperations(), ((i + 1) * (i + 2)) / 2);

            // link left empty
            ASSERT_EQ(link.numOperations(), 0u);
        }
    }

    // 3. append non-empty link to a started chain
    {
        // create chain
        mwcu::OperationChain chain(s_allocator_p);

        // start executing operations
        chain.start();

        // no operations currently executing
        ASSERT_EQ(chain.numOperationsExecuting(), 0u);

        // used for synchronization
        bslmt::Semaphore semaphore;

        // append a link with one operation
        chain.appendInplace(bdlf::BindUtil::bind(&asyncNullOperation,
                                                 threadPool,
                                                 bdlf::PlaceHolders::_1),
                            bdlf::BindUtil::bind(Synchronize(), &semaphore));

        // the chain is executing one operation
        ASSERT_EQ(chain.numLinks(), 1u);
        ASSERT_EQ(chain.numOperations(), 1u);
        ASSERT_EQ(chain.numOperationsExecuting(), 1u);

        // append another link with one operation
        chain.appendInplace(bdlf::BindUtil::bind(&asyncNullOperation,
                                                 threadPool,
                                                 bdlf::PlaceHolders::_1),
                            bdlf::BindUtil::bind(Synchronize(), &semaphore));

        // the chain is still executing only one operation
        ASSERT_EQ(chain.numLinks(), 2u);
        ASSERT_EQ(chain.numOperations(), 2u);
        ASSERT_EQ(chain.numOperationsExecuting(), 1u);

        // allow both operations to complete and wait for all operations to
        // finish execution
        semaphore.post(2);
        chain.join();

        // the chain is empty
        ASSERT_EQ(chain.numLinks(), 0u);
        ASSERT_EQ(chain.numOperations(), 0u);
    }
}

static void test6_chain_append2(bdlmt::ThreadPool* threadPool)
// ------------------------------------------------------------------------
// CHAIN APPEND 2
//
// Concerns:
//   Ensure proper behavior of 'append' method when appending multiple
//   links at once.
//
// Plan:
//   1. Append an empty link list and make sure this operation has no
//      effect.
//
//   2. Append an non-empty link list containing empty links and make sure
//      this operation has no effect.
//
//   3. Append an non-empty link list containing non-empty links and make
//      sure the links are inserted to the end of the chain in the order
//      they appear in the list. Also make sure input links are left empty
//      after the append.
// ------------------------------------------------------------------------
{
    // PRECONDITIONS
    BSLS_ASSERT(threadPool);

    // 1. append empty link list
    {
        // create chain
        mwcu::OperationChain chain(s_allocator_p);

        // append links
        mwcu::OperationChainLink** links =
            reinterpret_cast<mwcu::OperationChainLink**>(0x01);

        chain.append(links, 0);

        // the operation had no effect
        ASSERT_EQ(chain.numLinks(), 0u);
        ASSERT_EQ(chain.numOperations(), 0u);
    }

    // 2. append non-empty link list containing empty links
    {
        // create chain
        mwcu::OperationChain chain(s_allocator_p);

        // create empty links
        mwcu::OperationChainLink link1(chain.allocator());
        mwcu::OperationChainLink link2(chain.allocator());
        mwcu::OperationChainLink link3(chain.allocator());

        // append links
        mwcu::OperationChainLink* links[] = {&link1, &link2, &link3};
        chain.append(links, 3);

        // the operation had no effect
        ASSERT_EQ(chain.numLinks(), 0u);
        ASSERT_EQ(chain.numOperations(), 0u);
    }

    // 3. append non-empty link list containing non-empty links
    {
        // NOTE: The first link has 1 operation, and each next link has +1
        //       operations. The total number of operations in the chain is a
        //       function of the number of links (N), '(N(N + 1)) / 2'. This
        //       way we know which link is which.

        // create chain
        mwcu::OperationChain chain(s_allocator_p);

        // create links
        mwcu::OperationChainLink link1(chain.allocator());
        link1.insert(NullOperation());

        mwcu::OperationChainLink link2(chain.allocator());
        link2.insert(NullOperation());
        link2.insert(NullOperation());

        mwcu::OperationChainLink link3(chain.allocator());
        link3.insert(NullOperation());
        link3.insert(NullOperation());
        link3.insert(NullOperation());

        // append links
        mwcu::OperationChainLink* links[] = {&link1, &link2, &link3};
        chain.append(links, 3);

        // make sure links are appended in the right order
        for (unsigned i = 3; i != 0; --i) {
            ASSERT_EQ(chain.numLinks(), i);
            ASSERT_EQ(chain.numOperations(), (i * (i + 1)) / 2);

            chain.popBack();
        }

        // all input links are left empty
        ASSERT_EQ(link1.numOperations(), 0u);
        ASSERT_EQ(link2.numOperations(), 0u);
        ASSERT_EQ(link3.numOperations(), 0u);
    }
}

static void test7_chain_appendInplace(bdlmt::ThreadPool* threadPool)
// ------------------------------------------------------------------------
// CHAIN APPEND INPLACE
//
// Concerns:
//   Ensure proper behavior of 'appendInplace' method.
//
// Plan:
//   1. Append-inplace a link to the chain and make sure this operation is
//      equivalent to appending a single link containing a single
//      operation. Test both overloads of 'appendInplace'.
// ------------------------------------------------------------------------
{
    // PRECONDITIONS
    BSLS_ASSERT(threadPool);

    // 1. append-inplace a link
    {
        // create chain
        mwcu::OperationChain chain(s_allocator_p);

        // append-inplace a link (use first overload)
        chain.appendInplace(NullOperation());

        // one link appended with one operation in it
        ASSERT_EQ(chain.numLinks(), 1u);
        ASSERT_EQ(chain.numOperations(), 1u);

        // clear the chain
        chain.removeAll();

        // append-inplace a link (use second overload)
        chain.appendInplace(NullOperation(), mwcu::NoOp());

        // one link appended with one operation in it
        ASSERT_EQ(chain.numLinks(), 1u);
        ASSERT_EQ(chain.numOperations(), 1u);
    }
}

static void test8_chain_popBack(bdlmt::ThreadPool* threadPool)
// ------------------------------------------------------------------------
// CHAIN POP BACK
//
// Concerns:
//   Ensure proper behavior of 'popBack' method.
//
// Plan:
//   1. Extract all links, one by one, from the back of the chain and make
//      sure all associated operations are removed from the chain. Also
//      make sure the ownership of associated operations is transffered to
//      the receiving link, if such is specified.
//
//   2. Try extract the last link from an empty chain. Make sure this
//      operation has no effect and fails with a non-zero error code. Also
//      make sure the receiving link is unmodified, if such is specified.
//
//   3. Try extract the last link from the chain while associated
//      operations are executing and make sure this operation has no effect
//      and fails with a non-zero error code. Also make sure the receiving
//      link is unmodified, if such is specified.
// ------------------------------------------------------------------------
{
    // PRECONDITIONS
    BSLS_ASSERT(threadPool);

    // 1. extract all links
    {
        // NOTE: The first link has 1 operation, and each next link has +1
        //       operations. The total number of operations in the chain is a
        //       function of the number of links (N), '(N(N + 1)) / 2'. This
        //       way we know which link is which.

        // number of links
        static const unsigned k_NUM_LINKS = 8;

        // create chain
        mwcu::OperationChain chain(s_allocator_p);

        // append several links
        mwcu::OperationChainLink link(chain.allocator());
        for (unsigned i = 0; i < k_NUM_LINKS; ++i) {
            // number of operations in the link
            const unsigned k_NUM_OPERATIONS = i + 1;

            // fill link
            for (unsigned j = 0; j < k_NUM_OPERATIONS; ++j) {
                link.insert(NullOperation());
            }

            // append link
            chain.append(&link);
        }

        // extract links from the back of the chain one by one
        for (unsigned i = k_NUM_LINKS; i > 0; --i) {
            // extract last link
            chain.popBack(&link);

            // link removed from the chain
            ASSERT_EQ(chain.numLinks(), i - 1);
            ASSERT_EQ(chain.numOperations(), ((i - 1) * i) / 2);

            // link contains extracted operations
            ASSERT_EQ(link.numOperations(), i);
        }

        // the chain is empty
        ASSERT_EQ(chain.numLinks(), 0u);
        ASSERT_EQ(chain.numOperations(), 0u);
    }

    // 2. try extract last link from an empty chain
    {
        // create chain
        mwcu::OperationChain chain(s_allocator_p);

        // try extract link
        int rc = chain.popBack();
        ASSERT(rc != 0);

        // the operation had no effect
        ASSERT_EQ(chain.numLinks(), 0u);
        ASSERT_EQ(chain.numOperations(), 0u);

        // try extract link again, this time transfer operations ownership
        mwcu::OperationChainLink link(chain.allocator());
        link.insert(NullOperation());

        rc = chain.popBack(&link);
        ASSERT(rc != 0);

        // the operation had no effect
        ASSERT_EQ(chain.numLinks(), 0u);
        ASSERT_EQ(chain.numOperations(), 0u);

        // receiving link unmodified
        ASSERT_EQ(link.numOperations(), 1u);
    }

    // 3. try extract last link while associated operations are executing
    {
        // create chain
        mwcu::OperationChain chain(s_allocator_p);

        // start executing operations
        chain.start();

        // used for synchronization
        bslmt::Semaphore semaphore;

        // append a link with one operation
        chain.appendInplace(bdlf::BindUtil::bind(&asyncNullOperation,
                                                 threadPool,
                                                 bdlf::PlaceHolders::_1),
                            bdlf::BindUtil::bind(Synchronize(), &semaphore));

        // the chain is executing one operation
        ASSERT_EQ(chain.numLinks(), 1u);
        ASSERT_EQ(chain.numOperations(), 1u);
        ASSERT_EQ(chain.numOperationsExecuting(), 1u);

        // try extract link
        int rc = chain.popBack();
        ASSERT(rc != 0);

        // link not removed from the chain
        ASSERT_EQ(chain.numLinks(), 1u);
        ASSERT_EQ(chain.numOperations(), 1u);
        ASSERT_EQ(chain.numOperationsExecuting(), 1u);

        // try extract link again, this time transfer operations ownership
        mwcu::OperationChainLink link(chain.allocator());
        link.insert(NullOperation());
        rc = chain.popBack(&link);
        ASSERT(rc != 0);

        // link not removed from the chain
        ASSERT_EQ(chain.numLinks(), 1u);
        ASSERT_EQ(chain.numOperations(), 1u);
        ASSERT_EQ(chain.numOperationsExecuting(), 1u);

        // receiving link unmodified
        ASSERT_EQ(link.numOperations(), 1u);

        // allow executing operations to complete and synchronize with the
        // chain
        semaphore.post(1);
        chain.join();
    }
}

static void test9_chain_removeAll(bdlmt::ThreadPool* threadPool)
// ------------------------------------------------------------------------
// CHAIN REMOVE ALL
//
// Concerns:
//   Ensure proper behavior of 'removeAll' method.
//
// Plan:
//   1. Remove all links from the chain and make sure they are removed.
//      Also make sure the correct number of removed operation is returned.
//
//   2. Try remove all operations from an empty chain and make sure this
//      operation has no effect and 0 is returned.
//
//   3. Try remove all operations from the chain while some operations are
//      executing and make sure only non-executing operations are removed.
//      Also make sure the correct number of removed operation is reported.
// ------------------------------------------------------------------------
{
    // PRECONDITIONS
    BSLS_ASSERT(threadPool);

    // 1. remove all links
    {
        // create chain
        mwcu::OperationChain chain(s_allocator_p);

        // number of links and operations in each link
        static const unsigned k_NUM_LINKS      = 3;
        static const unsigned k_NUM_OPERATIONS = 5;

        // fill the chain
        mwcu::OperationChainLink link(chain.allocator());
        for (unsigned i = 0; i < k_NUM_LINKS; ++i) {
            for (unsigned j = 0; j < k_NUM_OPERATIONS; ++j) {
                link.insert(NullOperation());
            }

            chain.append(&link);
        }

        // chain not empty
        ASSERT_EQ(chain.numLinks(), k_NUM_LINKS);
        ASSERT_EQ(chain.numOperations(), k_NUM_LINKS * k_NUM_OPERATIONS);

        // remove all links
        unsigned count = chain.removeAll();
        ASSERT_EQ(count, k_NUM_LINKS * k_NUM_OPERATIONS);

        // all links removed from the chain
        ASSERT_EQ(chain.numLinks(), 0u);
        ASSERT_EQ(chain.numOperations(), 0u);
    }

    // 2. try remove all operations from an empty chain
    {
        // create chain
        mwcu::OperationChain chain(s_allocator_p);

        // try remove all links
        unsigned count = chain.removeAll();
        ASSERT_EQ(count, 0u);
    }

    // 3. try remove all operations while some of them are executing
    {
        // create chain
        mwcu::OperationChain chain(s_allocator_p);

        // start executing operations
        chain.start();

        // used for synchronization
        bslmt::Semaphore semaphore;

        // number of links
        static const unsigned k_NUM_LINKS = 3;

        // append several links with one operation each
        for (unsigned i = 0; i < k_NUM_LINKS; ++i) {
            chain.appendInplace(bdlf::BindUtil::bind(&asyncNullOperation,
                                                     threadPool,
                                                     bdlf::PlaceHolders::_1),
                                bdlf::BindUtil::bind(Synchronize(),
                                                     &semaphore));
        }

        // the chain is executing one operation
        ASSERT_EQ(chain.numLinks(), k_NUM_LINKS);
        ASSERT_EQ(chain.numOperations(), k_NUM_LINKS);
        ASSERT_EQ(chain.numOperationsExecuting(), 1u);

        // remove all links
        unsigned count = chain.removeAll();
        ASSERT_EQ(count, k_NUM_LINKS - 1);

        // not all links were removed from the chain
        ASSERT_EQ(chain.numLinks(), 1u);
        ASSERT_EQ(chain.numOperations(), 1u);
        ASSERT_EQ(chain.numOperationsExecuting(), 1u);

        // allow executing operations to complete and synchronize with the
        // chain
        semaphore.post(1);
        chain.join();
    }
}

static void test10_chain_serialization(bdlmt::ThreadPool* threadPool)
// ------------------------------------------------------------------------
// CHAIN SERIALIZATION
//
// Concerns:
//   Ensure proper behavior of the operation chain in regards to operation
//   serialization.
//
// Plan:
//   Run a chain containing several links with several operations in each
//   link and make sure all operations are executed in the link order.
// ------------------------------------------------------------------------
{
    // PRECONDITIONS
    BSLS_ASSERT(threadPool);

    // store completed operation IDs here
    bdlcc::Deque<unsigned> completionIds(bsls::SystemClockType::e_MONOTONIC);

    // create chain
    mwcu::OperationChain chain(s_allocator_p);

    // number of links and operations in each link
    static const unsigned k_NUM_LINKS      = 8;
    static const unsigned k_NUM_OPERATIONS = 32;

    // fill the chain
    mwcu::OperationChainLink link(chain.allocator());
    for (unsigned i = 0; i < k_NUM_LINKS; ++i) {
        for (unsigned j = 0; j < k_NUM_OPERATIONS; ++j) {
            unsigned operationId = i * k_NUM_OPERATIONS + j;
            link.insert(bdlf::BindUtil::bind(&asyncIdentifiableOperation,
                                             threadPool,
                                             operationId,
                                             bdlf::PlaceHolders::_1),
                        bdlf::BindUtil::bind(PushBackRndSleep(),
                                             &completionIds,
                                             bdlf::PlaceHolders::_1));
        }

        chain.append(&link);
    }

    // start executing operations and wait for all opertions to complete
    chain.start();
    chain.join();

    // all operation executed
    ASSERT_EQ(chain.numOperations(), 0u);
    ASSERT_EQ(completionIds.length(), k_NUM_LINKS * k_NUM_OPERATIONS);

    // make sure operations were executed in the right order
    for (unsigned i = 0; i < k_NUM_LINKS; ++i) {
        for (unsigned j = 0; j < k_NUM_OPERATIONS; ++j) {
            unsigned operationId = completionIds.popFront();

            ASSERT_GE(operationId, i * k_NUM_OPERATIONS);
            ASSERT_LT(operationId, (i + 1) * k_NUM_OPERATIONS);
        }
    }
}

static void test11_chain_exceptionHandling(bdlmt::ThreadPool* threadPool)
// ------------------------------------------------------------------------
// CHAIN EXCEPTION HANDLING
//
// Concerns:
//   Ensure proper behavior of the operation chain in regards to exception
//   handling.
//
// Plan:
//   Execute several operations that handling exceptions thrown by their
//   respective completion callbacks and make sure that those exceptions
//   are propagated to the caller.
// ------------------------------------------------------------------------
{
    // PRECONDITIONS
    BSLS_ASSERT(threadPool);

    // create chain
    mwcu::OperationChain chain(s_allocator_p);

    // add several operations, each handling an exception thrown from the
    // completion callback
    bool exception1Thrown = false;
    chain.appendInplace(bdlf::BindUtil::bind(ExceptionHandlingOperation(),
                                             &exception1Thrown,
                                             bdlf::PlaceHolders::_1),
                        ThrowException());

    bool exception2Thrown = false;
    chain.appendInplace(bdlf::BindUtil::bind(ExceptionHandlingOperation(),
                                             &exception2Thrown,
                                             bdlf::PlaceHolders::_1),
                        ThrowException());

    // execute operations
    chain.start();
    chain.join();

    // make sure all operations were executed
    ASSERT_EQ(exception1Thrown, true);
    ASSERT_EQ(exception2Thrown, true);
}

static void test12_link_creators()
// ------------------------------------------------------------------------
// LINK CREATORS
//
// Concerns:
//   Ensure proper behavior of creators.
//
// Plan:
//   1. Default-construct a link and check postconditions.
//
//   2. Move-construct a link from another link and make sure that the
//      contents of the moved-from link is moved to the new link.
// ------------------------------------------------------------------------
{
    // 1. default constructor
    {
        // create link
        mwcu::OperationChainLink link(s_allocator_p);

        // check postconditions
        ASSERT_EQ(link.numOperations(), 0u);
        ASSERT_EQ(link.allocator(), s_allocator_p);
    }

    // 2. move constructor
    {
        // create link containing several operations
        mwcu::OperationChainLink original(s_allocator_p);
        original.insert(NullOperation());
        original.insert(NullOperation());

        // move-construct another link
        mwcu::OperationChainLink copy(bslmf::MovableRefUtil::move(original));

        // the copy now contains the state of the original
        ASSERT_EQ(copy.numOperations(), 2u);
        ASSERT_EQ(original.allocator(), s_allocator_p);

        // the original is left empty
        ASSERT_EQ(original.numOperations(), 0u);
        ASSERT_EQ(original.allocator(), s_allocator_p);
    }
}

static void test13_link_assignment()
// ------------------------------------------------------------------------
// LINK ASSIGNMENT
//
// Concerns:
//   Ensure proper behavior of the assignment operator.
//
// Plan:
//   1. Move assign one link to another and check that the contents of the
//      moved-to link are replaced with the contents of the moved-from
//      link.
//
//   2. Move-assign a link to itself and make sure that the link stays in a
//      valid state.
// ------------------------------------------------------------------------
{
    // 1. regular move-assignment
    {
        // create link containing one operation
        mwcu::OperationChainLink link1(s_allocator_p);

        link1.insert(NullOperation());

        // create another link containing two operations
        mwcu::OperationChainLink link2(s_allocator_p);
        link2.insert(NullOperation());
        link2.insert(NullOperation());

        // move-assign the second link to the first
        link1 = bslmf::MovableRefUtil::move(link2);

        // the first link now contains the state of the second one
        ASSERT_EQ(link1.numOperations(), 2u);
        ASSERT_EQ(link1.allocator(), s_allocator_p);

        // the second link is left empty
        ASSERT_EQ(link2.numOperations(), 0u);
        ASSERT_EQ(link2.allocator(), s_allocator_p);
    }

    // 2. move-assign to self
    {
        // create link containing several operations
        mwcu::OperationChainLink link(s_allocator_p);
        link.insert(NullOperation());
        link.insert(NullOperation());

        // move-assign to self
        link = bslmf::MovableRefUtil::move(link);

        // the link is in a valid state
        ASSERT_EQ(link.allocator(), s_allocator_p);
    }
}

static void test14_link_insert()
// ------------------------------------------------------------------------
// LINK INSERT
//
// Concerns:
//   Ensure proper behavior of the 'insert' method.
//
// Plan:
//   Insert several operations into a link. Test both 'insert' overloads.
// ------------------------------------------------------------------------
{
    // create an empty link
    mwcu::OperationChainLink link(s_allocator_p);
    ASSERT_EQ(link.numOperations(), 0u);

    static const unsigned k_NUM_OPERATIONS = 10;

    // insert several operations (use first overload)
    for (unsigned i = 0; i < k_NUM_OPERATIONS; ++i) {
        link.insert(NullOperation());
        ASSERT_EQ(link.numOperations(), i + 1);
    }

    // remove them all
    link.removeAll();

    // insert again (use second overload)
    for (unsigned i = 0; i < k_NUM_OPERATIONS; ++i) {
        link.insert(NullOperation(), mwcu::NoOp());
        ASSERT_EQ(link.numOperations(), i + 1);
    }
}

static void test15_link_removeAll()
// ------------------------------------------------------------------------
// LINK REMOVE ALL
//
// Concerns:
//   Ensure proper behavior of the 'removeAll' method.
//
// Plan:
//   Remove all operations from a link and make sure that the link is left
//   empty. Also make sure that the correct number of removed operation is
//   reported.
// ------------------------------------------------------------------------
{
    // create link containing several operations
    mwcu::OperationChainLink link(s_allocator_p);
    link.insert(NullOperation());
    link.insert(NullOperation());

    // remove all operations
    size_t count = link.removeAll();
    ASSERT_EQ(count, 2u);
    ASSERT_EQ(link.numOperations(), 0u);

    // remove all operations again
    count = link.removeAll();
    ASSERT_EQ(count, 0u);
    ASSERT_EQ(link.numOperations(), 0u);
}

static void test16_link_swap()
// ------------------------------------------------------------------------
// LINK SWAP
//
// Concerns:
//   Ensure proper behavior of the 'swap' method.
//
// Plan:
//   Swap two links and check that their contents are swapped. Test cases
//   where one or both links are empty, and where none of them are.
// ------------------------------------------------------------------------
{
    // both empty
    {
        mwcu::OperationChainLink link1(s_allocator_p);
        mwcu::OperationChainLink link2(s_allocator_p);

        link1.swap(link2);
        ASSERT_EQ(link1.numOperations(), 0u);
        ASSERT_EQ(link2.numOperations(), 0u);
    }

    // first empty
    {
        mwcu::OperationChainLink link1(s_allocator_p);

        mwcu::OperationChainLink link2(s_allocator_p);
        link2.insert(NullOperation());
        link2.insert(NullOperation());

        link1.swap(link2);
        ASSERT_EQ(link1.numOperations(), 2u);
        ASSERT_EQ(link2.numOperations(), 0u);
    }

    // second empty
    {
        mwcu::OperationChainLink link1(s_allocator_p);
        link1.insert(NullOperation());
        link1.insert(NullOperation());

        mwcu::OperationChainLink link2(s_allocator_p);

        link1.swap(link2);
        ASSERT_EQ(link1.numOperations(), 0u);
        ASSERT_EQ(link2.numOperations(), 2u);
    }

    // none empty
    {
        mwcu::OperationChainLink link1(s_allocator_p);
        link1.insert(NullOperation());
        link1.insert(NullOperation());

        mwcu::OperationChainLink link2(s_allocator_p);
        link2.insert(NullOperation());
        link2.insert(NullOperation());
        link2.insert(NullOperation());

        link1.swap(link2);
        ASSERT_EQ(link1.numOperations(), 3u);
        ASSERT_EQ(link2.numOperations(), 2u);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    // utility mechanisms
    bdlmt::ThreadPool threadPool(bslmt::ThreadAttributes(),
                                 4,         // minThreads
                                 4,         // maxThreads
                                 INT_MAX);  // maxIdleTime
    int               rc = threadPool.start();
    BSLS_ASSERT_OPT(rc == 0);

    switch (_testCase) {
    // usage example
    case 1: test1_usageExample(); break;

    // 'mwcu::OperationChain'
    case 2: test2_chain_creators(&threadPool); break;
    case 3: test3_chain_startStop(&threadPool); break;
    case 4: test4_chain_join(&threadPool); break;
    case 5: test5_chain_append1(&threadPool); break;
    case 6: test6_chain_append2(&threadPool); break;
    case 7: test7_chain_appendInplace(&threadPool); break;
    case 8: test8_chain_popBack(&threadPool); break;
    case 9: test9_chain_removeAll(&threadPool); break;
    case 10: test10_chain_serialization(&threadPool); break;
    case 11: test11_chain_exceptionHandling(&threadPool); break;

    // 'mwcu::OperationChainLink'
    case 12: test12_link_creators(); break;
    case 13: test13_link_assignment(); break;
    case 14: test14_link_insert(); break;
    case 15: test15_link_removeAll(); break;
    case 16: test16_link_swap(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        s_testStatus = -1;
    } break;
    }

    // shutdown
    threadPool.stop();

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_GBL_ALLOC);
}
