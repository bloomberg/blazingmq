// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mwcex_systemexecutor.h                                             -*-C++-*-
#ifndef INCLUDED_MWCEX_SYSTEMEXECUTOR
#define INCLUDED_MWCEX_SYSTEMEXECUTOR

//@PURPOSE: Provides an executor allowing functions to execute on any thread.
//
//@CLASSES:
//  mwcex::SystemExecutor:             system executor
//  mwcex::SystemExecutorContextGuard: scoped system execution context guard
//
//@DESCRIPTION:
// This component provides a mechanism 'mwcex::SystemExecutor', that is an
// executor allowed to spawn an arbitrary number of threads to execute
// submitted functors. Those threads are collectively referred to as system
// threads. All functors submitted via a call to the executor's 'post' function
// are executed on a system thread, while all functors submitted via a call to
// the 'dispatch' function are executed in the calling thread, i.e. in-place.
//
// This executor is better suited for the execution of unfrequent but heavy
// background tasks, or to be used in test drivers as the default go-to
// executor.
//
/// Thread attributes
///-----------------
// Each executor object may be initialized with custom thread attributes,
// 'bslmt::ThreadAttributes', defining the attributes of the system thread
// submitted functors are to be executed on. By default, new threads are
// spawned with implementation-defined attributes.
//
/// Execution context
///-----------------
// All 'mwcex::SystemExecutor' objects refer to the same execution context
// mechanism, that is a singleton. While the execution context itself is an
// implementation details, a guard class, 'mwcex::SystemExecutorContextGuard',
// is provided, allowing to explicitly initialized the context with a custom
// allocator.
//
/// Usage
///-----
// Here is a straightforward usage example demonstrating how to submit a
// functor for asynchronous execution.
//..
//  mwcex::SystemExecutor ex;
//  ex.post([](){ bsl::cout << "I'm executed!"; });
//..
// And here is another one. This time we explicitly specify desired attributes
// of the system thread.
//..
//  bslmt::ThreadAttributes threadAttr;
//  threadAttr.setStackSize(1024 * 1024);
//
//  mwcex::SystemExecutor ex(threadAttr);
//  ex.post([](){ bsl::cout << "I'm executed!"; });
//..
// And this example demonstrates the usage of the execution context guard with
// a custom allocator.
//..
//  // a test allocator
//  bslma::TestAllocator testAlloc;
//
//  // initialize system execution context with test allocator
//  // mwcex::SystemExecutorContextGuard sysCtxGuard(&testAlloc);
//
//  // use the executor ...
//..

// MWC

#include <mwcsys_threadutil.h>

// BDE
#include <bdlma_concurrentpoolallocator.h>
#include <bsl_memory.h>
#include <bslalg_constructorproxy.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslmf_decay.h>
#include <bslmf_isbitwisemoveable.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmf_util.h>
#include <bslmt_condition.h>
#include <bslmt_mutex.h>
#include <bslmt_threadutil.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_compilerfeatures.h>
#include <bsls_exceptionutil.h>  // BSLS_NOTHROW_SPEC
#include <bsls_keyword.h>
#include <bsls_spinlock.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bslmt {
class ThreadAttributes;
}

namespace mwcex {

class SystemExecutor_Context;

// ================================
// struct SystemExecutor_ThreadData
// ================================

/// Data for a spawned thread.
///
/// `FUNCTION` must meet the requirements of Destructible as specified in
/// the C++ standard.
template <class FUNCTION>
struct SystemExecutor_ThreadData {
    // DATA

    /// The associated execution context.
    SystemExecutor_Context* d_context_p;

    /// Spinlock to be used by the thread to synchronize with postconditions
    /// needed to be established by the spawning thread before this thread's
    /// main code starts running.
    bsls::SpinLock d_spinLock;

    /// Thread handle to be used by the thread to mark itself as completed,
    /// so another thread could call `bslmt::ThreadUtil::join` on that
    /// handle. Note that the thread cannot just obtain self handle using
    /// `bslmt::ThreadUtil::self`, as such handle can not be used for join.
    bslmt::ThreadUtil::Handle d_thread;

    /// Functor to be invoked by the thread. May or may not be an
    /// allocator-aware object, hence the `bslalg::ConstructorProxy`
    /// wrapper.
    bslalg::ConstructorProxy<FUNCTION> d_function;

    // NOT IMPLEMENTED
    SystemExecutor_ThreadData(const SystemExecutor_ThreadData&)
        BSLS_KEYWORD_DELETED;
    SystemExecutor_ThreadData&
    operator=(const SystemExecutor_ThreadData&) BSLS_KEYWORD_DELETED;

    // CREATORS

    /// Create a `SystemExecutor_ThreadData` object initialized with the
    /// specified `context` and `function`. Specify an `allocator` used to
    /// supply memory.
    template <class FUNCTION_PARAM>
    SystemExecutor_ThreadData(SystemExecutor_Context* context,
                              BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION_PARAM)
                                  function,
                              bslma::Allocator* allocator);
};

// ============================
// class SystemExecutor_Context
// ============================

/// Provides a singleton execution context for `SystemExecutor`.
class SystemExecutor_Context {
  private:
    // PRIVATE DATA

    // Used to efficiently allocate thread data for spawned threads.
    bdlma::ConcurrentPoolAllocator d_threadDataAllocator;

    // Used in conjunction with `d_threadExitCondition` to synchronize with
    // the completion of all spawned threads.
    bslmt::Mutex d_threadExitMutex;

    // Used by spawned threads to notify the context about completion.
    bslmt::Condition d_threadExitCondition;

    // The number of threads that had not yet been joined.
    bsls::AtomicUint d_unjoinedThreads;

    // Handle of the last completed thread, to be joined by the next
    // completing thread, or by the destructor.
    bslmt::ThreadUtil::Handle d_lastCompletedThread;

    // Used to supply memory.
    bslma::Allocator* d_allocator_p;

  private:
    // PRIVATE CLASS METHODS

    /// This function is executed when a spawned thread starts. All it does
    /// is a call to `onThreadRun` on the associated execution context.
    template <class THREAD_DATA>
    static void* threadRunHandler(void* threadData) BSLS_KEYWORD_NOEXCEPT;

  private:
    // PRIVATE MANIPULATORS

    /// This function is executed when a spawned thread starts.
    template <class THREAD_DATA>
    void onThreadRun(THREAD_DATA* threadData) BSLS_NOTHROW_SPEC;

    /// This function is called by a spawned thread before completion to
    /// establish necessary postconditions.
    void finalizeThread(const bslmt::ThreadUtil::Handle& thread)
        BSLS_KEYWORD_NOEXCEPT;

  private:
    // NOT IMPLEMENTED
    SystemExecutor_Context(const SystemExecutor_Context&) BSLS_KEYWORD_DELETED;
    SystemExecutor_Context&
    operator&=(const SystemExecutor_Context&) BSLS_KEYWORD_DELETED;

  public:
    // CLASS METHODS

    /// Return a reference to a process-wide unique object of this class.
    /// The lifetime of this object is guaranteed to extend from the first
    /// call of this function (or of `initSingleton`), and until either a
    /// call to `shutdownSingleton`, or the program termination. The
    /// behavior is undefined if the object has already been destroyed via a
    /// call to `shutdownSingleton`.
    static SystemExecutor_Context& singleton();

    /// Return a reference to a process-wide unique object of this class
    /// initialized with the optionally specified `globalAllocator` or, if
    /// `globalAllocator` is 0, with the currently installed global
    /// allocator. The lifetime of this object is guaranteed to extend from
    /// the first call of this function, and until either a call to
    /// `shutdownSingleton`, or the program termination. The behavior is
    /// undefined if the object has already been initialized via a call to
    /// `singleton` / `initSingleton`, or has already been destroyed via a
    /// call to `shutdownSingleton`.
    static SystemExecutor_Context&
    initSingleton(bslma::Allocator* globalAllocator = 0);

    /// Destroy the singleton object. The behavior is undefined if the
    /// object has not been initialized, or has already been destroyed.
    static void shutdownSingleton() BSLS_KEYWORD_NOEXCEPT;

  public:
    // CREATORS

    /// Create a `SystemExecutor_Context` object. Specify an `allocator`
    /// used to supply memory.
    ///
    /// Note that this constructor shall not be used directly.
    explicit SystemExecutor_Context(bslma::Allocator* allocator);

    /// Destroy this object. Block the calling thread pending completion of
    /// all threads spawned by this execution context, if any.
    ~SystemExecutor_Context();

  public:
    // MANIPULATORS

    /// Invoke `DECAY_COPY(bsl::forward<FUNCTION>(f))()` as if by launching
    /// a new thread having the specified `threadAttributes`, with the call
    /// to `DECAY_COPY` being evaluated in the calling thread. Any exception
    /// propagated from the execution of the submitted functor results in a
    /// call to `bsl::terminate`. The behavior is undefined unless
    /// 'threadAttributes.detachedState() ==
    ///                        bslmt::ThreadAttributes::e_CREATE_JOINABLE)'.
    ///
    /// `bsl::decay_t<FUNCTION>` must meet the requirements of Destructible
    /// and MoveConstructible as specified in the C++ standard.
    /// `DECAY_COPY(bsl::forward<FUNCTION>(f))()` shall be a valid
    /// expression.
    ///
    /// This function meets the strong exception guarantee. If an exception
    /// is thrown, this function has no effect.
    template <class FUNCTION>
    void executeAsync(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f,
                      const bslmt::ThreadAttributes& threadAttributes);

    /// Equivalent to `DECAY_COPY(bsl::forward<FUNCTION>(f))()`.
    ///
    /// `bsl::decay_t<FUNCTION>` must meet the requirements of Destructible
    /// and MoveConstructible as specified in the C++ standard.
    /// `DECAY_COPY(bsl::forward<FUNCTION>(f))()` shall be a valid
    /// expression.
    template <class FUNCTION>
    void executeInPlace(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f);

  public:
    // ACCESSORS

    /// Return the allocator used by this execution context to supply
    /// memory.
    bslma::Allocator* allocator() const BSLS_KEYWORD_NOEXCEPT;
};

// ====================
// class SystemExecutor
// ====================

/// Provides an executor that allows functors to execute on any thread.
class SystemExecutor {
  private:
    // PRIVATE DATA

    // Optional thread attributes used when spawning threads to execute
    // submitted functors. Note that to meet the `noexcept` requirements
    // for the executor copy and move constructors, this object is shared
    // between instances.
    bsl::shared_ptr<bslmt::ThreadAttributes> d_threadAttributes;

  public:
    // CREATORS

    /// Create a `SystemExecutor` object with no associated thread
    /// attributes.
    SystemExecutor();

    /// Create a `SystemExecutor` object with the specified associated
    /// `threadAttributes`. Optionally specify a `basicAllocator` used to
    /// supply memory. If `basicAllocator` is 0, the currently installed
    /// default allocator is used. The behavior is undefined unless
    /// 'threadAttributes.detachedState() ==
    ///                         bslmt::ThreadAttributes::e_CREATE_JOINABLE'.
    explicit SystemExecutor(const bslmt::ThreadAttributes& threadAttributes,
                            bslma::Allocator*              basicAllocator = 0);

  public:
    // MANIPULATORS

    /// Invoke `DECAY_COPY(bsl::forward<FUNCTION>(f))()` as if by launching
    /// a new thread having this executor's associated thread attributes,
    /// with the call to `DECAY_COPY` being evaluated in the calling thread.
    /// If this executor does not have associated thread attributes, the
    /// attributes used are unspecified. Any exception propagated from the
    /// execution of the submitted functor results in a call to
    /// `bsl::terminate`.
    ///
    /// `bsl::decay_t<FUNCTION>` must meet the requirements of Destructible
    /// and MoveConstructible as specified in the C++ standard.
    /// `DECAY_COPY(bsl::forward<FUNCTION>(f))()` shall be a valid
    /// expression.
    ///
    /// This function meets the strong exception guarantee. If an exception
    /// is thrown, this function has no effect.
    template <class FUNCTION>
    void post(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f) const;

    /// Equivalent to `DECAY_COPY(bsl::forward<FUNCTION>(f))()`.
    ///
    /// `bsl::decay_t<FUNCTION>` must meet the requirements of Destructible
    /// and MoveConstructible as specified in the C++ standard.
    /// `DECAY_COPY(bsl::forward<FUNCTION>(f))()` shall be a valid
    /// expression.
    template <class FUNCTION>
    void dispatch(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f) const;

  public:
    // ACCESSORS

    /// Return a pointer to the executor's associated tread attributes, or a
    /// null pointer if the executor has no associated thread attributes.
    const bslmt::ThreadAttributes*
    threadAttributes() const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(SystemExecutor, bslmf::IsBitwiseMoveable)
};

// ================================
// class SystemExecutorContextGuard
// ================================

/// Provides a scoped guard for the system execution context singleton, that
/// initializes the singleton object on construction and destroy the singleton
/// object on destruction.
class SystemExecutorContextGuard {
  private:
    // NOT IMPLEMENTED
    SystemExecutorContextGuard(const SystemExecutorContextGuard&)
        BSLS_KEYWORD_DELETED;
    SystemExecutorContextGuard&
    operator=(const SystemExecutorContextGuard&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Initialize the system execution context singleton with the specified
    /// `globalAllocator`. The behavior is undefined if the singleton has
    /// already been initialized or has already been destroyed.
    explicit SystemExecutorContextGuard(bslma::Allocator* globalAllocator = 0);

    /// Destroy the system execution context singleton.
    ~SystemExecutorContextGuard();
};

// FREE OPERATORS

/// - If `lhs` and `rhs` have no associated thread attributes, return
///   `true`;
/// - Otherwise, if `lhs` or `rhs` has no associated thread attributes,
///   return `false`;
/// - Otherwise, if `lhs` and `rhs` contain identical thread attributes,
///   return `true`;
/// - Otherwise, return `false`.
bool operator==(const SystemExecutor& lhs,
                const SystemExecutor& rhs) BSLS_KEYWORD_NOEXCEPT;

/// Return !(lhs == rhs).
bool operator!=(const SystemExecutor& lhs,
                const SystemExecutor& rhs) BSLS_KEYWORD_NOEXCEPT;

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// --------------------------------
// struct SystemExecutor_ThreadData
// --------------------------------

// CREATORS
template <class FUNCTION>
template <class FUNCTION_PARAM>
inline SystemExecutor_ThreadData<FUNCTION>::SystemExecutor_ThreadData(
    SystemExecutor_Context* context,
    BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION_PARAM) function,
    bslma::Allocator* allocator)
: d_context_p(context)
, d_spinLock()
, d_thread()
, d_function(BSLS_COMPILERFEATURES_FORWARD(FUNCTION_PARAM, function),
             allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(context);
    BSLS_ASSERT(allocator);
}

// ----------------------------
// class SystemExecutor_Context
// ----------------------------

// PRIVATE CLASS METHODS
template <class THREAD_DATA>
inline void* SystemExecutor_Context::threadRunHandler(void* threadData)
    BSLS_KEYWORD_NOEXCEPT
{
    THREAD_DATA* typedThreadData = reinterpret_cast<THREAD_DATA*>(threadData);

    // PRECONDITIONS
    BSLS_ASSERT(typedThreadData);
    BSLS_ASSERT(typedThreadData->d_context_p);

    typedThreadData->d_context_p->onThreadRun(typedThreadData);
    return 0;
}

// PRIVATE MANIPULATORS
template <class THREAD_DATA>
inline void
SystemExecutor_Context::onThreadRun(THREAD_DATA* threadData) BSLS_NOTHROW_SPEC
{
    // NOTE: 'BSLS_NOTHROW_SPEC' specification ensures that 'bsl::terminate' is
    //       called in case of exception in C++03.

    // PRECONDITIONS
    BSLS_ASSERT(threadData);

    // Synchronize with postconditions established by the spawning thread
    // before running this thread's code. Note that we don't expect contention,
    // and the "backoff" is just a precaution for the very unlikely scenario
    // when the spawning thread goes to sleep before releasing the lock.
    threadData->d_spinLock.lockWithBackoff();  //   LOCK
    threadData->d_spinLock.unlock();           // UNLOCK

    // invoke the functor
    bslmf::Util::moveIfSupported(threadData->d_function.object())();

    // read this thread's handle
    bslmt::ThreadUtil::Handle thisThread = threadData->d_thread;

    // release thread data destroying the contained functor
    d_threadDataAllocator.deleteObject(threadData);

    // establish necessary postconditions before thread completion
    finalizeThread(thisThread);
}

// MANIPULATORS
template <class FUNCTION>
inline void SystemExecutor_Context::executeAsync(
    BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f,
    const bslmt::ThreadAttributes& threadAttributes)
{
    // PRECONDITIONS
    BSLS_ASSERT(threadAttributes.detachedState() ==
                bslmt::ThreadAttributes::e_CREATE_JOINABLE);

    // allocate thread data containing the functor 'f'
    typedef SystemExecutor_ThreadData<typename bsl::decay<FUNCTION>::type>
        ThreadData;

    bslma::ManagedPtr<ThreadData> threadData =
        bslma::ManagedPtrUtil::allocateManaged<ThreadData>(
            &d_threadDataAllocator,
            this,  // context
            BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f),
            d_allocator_p);

    // ensure the spawned thread synchronizes with postconditions established
    // by this function, specifically assignment of the thread handle and the
    // increment of the unjoined threads counter
    bsls::SpinLockGuard lock(&threadData->d_spinLock);  // LOCK

    // spawn new thread and save the thread handle into thread data
    int rc = bslmt::ThreadUtil::createWithAllocator(
        &threadData->d_thread,
        threadAttributes,
        &threadRunHandler<ThreadData>,
        threadData.get(),
        d_allocator_p);
    BSLS_ASSERT_OPT(rc == 0);

    // increment unjoined threads counter
    d_unjoinedThreads.addRelaxed(1);

    // transfer ownership of thread data to the spawned thread
    threadData.release();
}

template <class FUNCTION>
inline void SystemExecutor_Context::executeInPlace(
    BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f)
{
    // make a local, non-const copy of the function
    bslalg::ConstructorProxy<typename bsl::decay<FUNCTION>::type> f2(
        BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f),
        d_allocator_p);

    // invoke in-place
    bslmf::Util::moveIfSupported(f2.object())();
}

// --------------------
// class SystemExecutor
// --------------------

// MANIPULATORS
template <class FUNCTION>
inline void SystemExecutor::post(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION)
                                     f) const
{
    SystemExecutor_Context::singleton().executeAsync(
        BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f),
        d_threadAttributes ? *d_threadAttributes
                           : mwcsys::ThreadUtil::defaultAttributes());
}

template <class FUNCTION>
inline void
SystemExecutor::dispatch(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f) const
{
    SystemExecutor_Context::singleton().executeInPlace(
        BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f));
}

// --------------------------------
// class SystemExecutorContextGuard
// --------------------------------

// CREATORS
inline SystemExecutorContextGuard::SystemExecutorContextGuard(
    bslma::Allocator* globalAllocator)
{
    SystemExecutor_Context::initSingleton(globalAllocator);
}

inline SystemExecutorContextGuard::~SystemExecutorContextGuard()
{
    SystemExecutor_Context::shutdownSingleton();
}

}  // close package namespace
}  // close enterprise namespace

#endif
