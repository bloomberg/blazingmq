// Copyright 2019-2023 Bloomberg Finance L.P.
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

// bmqex_sequentialcontext.h                                          -*-C++-*-
#ifndef INCLUDED_BMQEX_SEQUENTIALCONTEXT
#define INCLUDED_BMQEX_SEQUENTIALCONTEXT

//@PURPOSE: Provides a single-threaded execution context.
//
//@CLASSES:
//  bmqex::SequentialContext:         a single-threaded execution context
//  bmqex::SequentialContextExecutor: an executor to submit work to the context
//
//@SEE ALSO:
//  bmqex_strand
//
//@DESCRIPTION:
// This component provides an execution context that provides guarantees
// of ordering and non-concurrency. Submitted function objects are executed in
// the context of a single thread of execution owned by the execution context.
//
/// Comparison to bmqex::Strand
///---------------------------
// While 'bmqex::Strand' is an execution context adapter created on top of an
// existing execution context, 'bmqex::SequentianContext' does not require an
// underlying context to work. Otherwise, they are very similar in their
// purpose and functions.
//
/// Thread safety
///-------------
// 'bmqex::SequentialContext' is fully thread-safe, meaning that multiple
// threads may use their own instances of the class or use a shared instance
// without further synchronization.
//
// With the exception of assignment operators, as well as the 'swap' member
// function, 'bmqex::SequentialContextExecutor' is fully thread-safe, meaning
// that multiple threads may use their own instances of the class or use a
// shared instance without further synchronization.
//
/// Usage
///-----
// Create an execution context, start it:
//..
//  bmqex::SequentialContext context;
//  int rc = context.start();
//  BSLS_ASSERT(rc == 0);
//..
// Obtain an executor object:
//..
//  bmqex::SequentialContext::ExecutorType executor = context.executor();
//..
// To execute a function object, call 'post' on the executor:
//..
//  // executes the lambda on the working thread
//  executor.post([](){ bsl::cout << "It Works!" << bsl::endl; });
//..
// Or call 'dispatch', if you may already be inside the working thread:
//..
//  // if called from inside the working thread, executes the lambda in-place
//  // otherwise, falls back to 'post'
//  executor.dispatch([](){ bsl::cout << "It Works!" << bsl::endl; });
//..
// When you are done, destroy the context. If you want to wait for all pending
// jobs to complete, call 'join' before that:
//..
// // waits for all outstanding jobs to complete and stop the context
// context.join();
//..

#include <bmqex_job.h>

// BDE
#include <bsl_deque.h>
#include <bslalg_constructorproxy.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_decay.h>
#include <bslmf_istriviallycopyable.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmf_util.h>
#include <bslmt_condition.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bslmt_threadutil.h>
#include <bsls_atomic.h>
#include <bsls_compilerfeatures.h>
#include <bsls_exceptionutil.h>  // BSLS_NOTHROW_SPEC
#include <bsls_keyword.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bslmt {
class ThreadAttributes;
}

namespace bmqex {

class SequentialContextExecutor;

// ===================================
// struct SequentialContext_ThreadUtil
// ===================================

/// Provides a namespace for utility functions for managing threads.
struct SequentialContext_ThreadUtil {
    // CLASS METHODS

    /// Return an unsigned 64-bit integer representing a thread id that is
    /// guaranteed never to be a valid thread id.
    static bsls::Types::Uint64 invalidThreadId() BSLS_KEYWORD_NOEXCEPT;
};

// =======================
// class SequentialContext
// =======================

/// Provides a single threaded execution context that provides guarantees of
/// ordering and non-concurrency.
class SequentialContext {
  public:
    // TYPES

    /// Provides an executor to submit function objects on the
    /// `SequentialContext`.
    typedef SequentialContextExecutor ExecutorType;

  private:
    // PRIVATE DATA

    // Used for general thread safety, and in conjunction with
    // `d_condition` to synchronize with the completion of the working
    // thread.
    mutable bslmt::Mutex d_mutex;

    // Used to notify the working thread about new jobs, or that it's time
    // to stop.
    bslmt::Condition d_condition;

    // Working thread handle, or an invalid handle, if the thread is not
    // running.
    bslmt::ThreadUtil::Handle d_workingThread;

    // Working thread id, or an invalid id, if the thread is not running.
    // Used to check if we are inside the working thread.
    bsls::AtomicUint64 d_workingThreadId;

    // Used to indicate that a job is currently been executed, so that job
    // won't get removed from the job queue by `dropPendingJobs`.
    bool d_isExecuting;

    // Used to notify the working thread it's time to stop.
    bool d_doStop;

    // Used to notify the working thread it's time to stop once the
    // outstanding jobs is 0.
    bool d_doJoin;

    // Jobs to be executed by the working thread.
    bsl::deque<Job> d_jobQueue;

    // FRIENDS
    friend class SequentialContextExecutor;

  private:
    // PRIVATE MANIPULATORS
    void run() BSLS_NOTHROW_SPEC;

  private:
    // NOT IMPLEMENTED
    SequentialContext(const SequentialContext&) BSLS_KEYWORD_DELETED;
    SequentialContext&
    operator=(const SequentialContext&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `SequentialContext` object without spawning its working
    /// thread. Optionally specify a `basicAllocator` used to supply memory.
    /// If `basicAllocator` is 0, the default memory allocator is used.
    explicit SequentialContext(bslma::Allocator* basicAllocator = 0);

    /// Destroy this object. Perform `stop()` followed by `join()`.
    ~SequentialContext();

  public:
    // MANIPULATORS

    /// Spawn a thread of execution (the working thread) and begin executing
    /// function objects on this execution context. Optionally specify a
    /// `threadAttributes` used to configure the thread. If
    /// `threadAttributes` is not provided, use the value of
    /// `bmqsys::ThreadUtil::defaultAttributes()`. Return 0 on success, and
    /// a non-zero value otherwise. On failure this function has no effect.
    /// The behavior is undefined unless 'threadAttributes.detachedState()
    /// == bslmt::ThreadAttributes::e_CREATE_JOINABLE)', or if the working
    /// thread is currently running.
    ///
    /// This function meets the strong exception guarantee. If an exception
    /// is thrown, this function has no effect.
    int start();
    int start(const bslmt::ThreadAttributes& threadAttributes);

    /// Stop executing function objects on this execution context and signal
    /// the working thread to complete as soon as possible. If the thread is
    /// currently executing a function object, the thread will exit only
    /// after completion of that function object. Invocation of `stop`
    /// returns without waiting for the thread to complete.
    void stop() BSLS_KEYWORD_NOEXCEPT;

    /// If not already stopped, signal the working thread to complete once
    /// `outstandingJobs()` is 0. Blocks the calling thread until the
    /// working thread has completed, without executing submitted function
    /// objects in the calling thread. The behavior is undefined if this
    /// function is invoked from the thread owned by `*this`.
    ///
    /// Note that subsequent invocations of `join` complete immediately.
    void join() BSLS_KEYWORD_NOEXCEPT;

    /// Remove all functions objects but the one that is currently being
    /// invoked (if any) from this execution context. Return the number of
    /// removed function objects.
    size_t dropPendingJobs() BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Return the number of outstanding jobs for this execution context,
    /// that is defined as the number of function objects that have been
    /// added to the context via its associated executor, but not yet
    /// invoked, plus the number of function objects that are currently
    /// being invoked within the context, which is either 0 or 1.
    size_t outstandingJobs() const BSLS_KEYWORD_NOEXCEPT;

    /// Return an executor that may be used for submitting function
    /// objects to this execution context.
    ExecutorType executor() const BSLS_KEYWORD_NOEXCEPT;

    /// Return the allocator used by this execution context to supply
    /// memory.
    bslma::Allocator* allocator() const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(SequentialContext,
                                   bslma::UsesBslmaAllocator)
};

// ===============================
// class SequentialContextExecutor
// ===============================

/// Provides an executor to submit function objects on the
/// `SequentialContext`.
class SequentialContextExecutor {
  public:
    // TYPES
    typedef SequentialContext ContextType;

  private:
    // PRIVATE DATA
    ContextType* d_context_p;

    // FRIENDS
    friend class SequentialContext;

  private:
    // PRIVATE CREATORS
    SequentialContextExecutor(ContextType* context) BSLS_KEYWORD_NOEXCEPT;
    // IMPLICIT
    // Create a 'SequentialContextExecutor' object having the specified
    // 'context' as its associated execution context.
    //
    // Note that this is a private constructor reserved to be used by the
    // 'SequentialContext'.

  public:
    // MANIPULATORS

    /// Submit the function object `f` for execution on the
    /// `SequentialContext` and return immediately, without waiting for the
    /// submitted function object to complete. If `f` exits via an
    /// exception, the `SequentialContext` calls `bsl::terminate()`.
    ///
    /// This function meets the strong exception guarantee. If an exception
    /// is thrown, this function has no effect.
    ///
    /// `bsl::decay_t<FUNCTION>` must meet the requirements of Destructible
    /// and CopyConstructible as specified in the C++ standard.
    /// `DECAY_COPY(bsl::forward<FUNCTION>(f))()` shall be a valid
    /// expression.
    template <class FUNCTION>
    void post(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f) const;

    /// If `runningInThisThread()` is `true`, call 'DECAY_COPY(
    /// bsl::forward<FUNCTION>(f))()`. If `f' exits via an exception, the
    /// exception is propagated to the caller. Otherwise, call
    /// `post(bsl::forward<FUNCTION>(f))`.
    ///
    /// `bsl::decay_t<FUNCTION>` must meet the requirements of Destructible
    /// and CopyConstructible as specified in the C++ standard.
    /// `DECAY_COPY(bsl::forward<FUNCTION>(f))()` shall be a valid
    /// expression.
    template <class FUNCTION>
    void dispatch(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f) const;

    /// Swap the contents of `*this` and `other`.
    void swap(SequentialContextExecutor& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// If the current thread of execution is the thread owned by the
    /// associated `SequentialContext` object, return `true`. Otherwise,
    /// return `false`.
    bool runningInThisThread() const BSLS_KEYWORD_NOEXCEPT;

    /// Return a reference to the associated `SequentialContext` object.
    ContextType& context() const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(SequentialContextExecutor,
                                   bsl::is_trivially_copyable)
};

// FREE OPERATORS

/// Return `&lhs.context() == &rhs.context()`.
bool operator==(const SequentialContextExecutor& lhs,
                const SequentialContextExecutor& rhs) BSLS_KEYWORD_NOEXCEPT;

/// Return `!(lhs == rhs)`.
bool operator!=(const SequentialContextExecutor& lhs,
                const SequentialContextExecutor& rhs) BSLS_KEYWORD_NOEXCEPT;

/// Swap the contents of `lhs` and `rhs`.
void swap(SequentialContextExecutor& lhs,
          SequentialContextExecutor& rhs) BSLS_KEYWORD_NOEXCEPT;

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// -------------------------------
// class SequentialContextExecutor
// -------------------------------

// MANIPULATORS
template <class FUNCTION>
inline void
SequentialContextExecutor::post(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION)
                                    f) const
{
    // add 'f' to the job queue
    {
        bslmt::LockGuard<bslmt::Mutex> lock(&d_context_p->d_mutex);  // LOCK

        d_context_p->d_jobQueue.emplace_back(
            BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f));
    }  // UNLOCK

    // wake the working thread
    d_context_p->d_condition.signal();
}

template <class FUNCTION>
inline void
SequentialContextExecutor::dispatch(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION)
                                        f) const
{
    if (runningInThisThread()) {
        // make a local, non-const copy of the function
        bslalg::ConstructorProxy<typename bsl::decay<FUNCTION>::type> f2(
            BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f),
            d_context_p->allocator());

        // invoke in-place
        bslmf::Util::moveIfSupported(f2.object())();
    }
    else {
        // fallback to 'post()'
        post(BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f));
    }
}

}  // close package namespace

// FREE OPERATORS
inline bool
bmqex::operator==(const SequentialContextExecutor& lhs,
                  const SequentialContextExecutor& rhs) BSLS_KEYWORD_NOEXCEPT
{
    return &lhs.context() == &rhs.context();
}

inline bool
bmqex::operator!=(const SequentialContextExecutor& lhs,
                  const SequentialContextExecutor& rhs) BSLS_KEYWORD_NOEXCEPT
{
    return !(lhs == rhs);
}

inline void bmqex::swap(SequentialContextExecutor& lhs,
                        SequentialContextExecutor& rhs) BSLS_KEYWORD_NOEXCEPT
{
    lhs.swap(rhs);
}

}  // close enterprise namespace

#endif
