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

// bmqex_strand.h                                                     -*-C++-*-
#ifndef INCLUDED_BMQEX_STRAND
#define INCLUDED_BMQEX_STRAND

//@PURPOSE: Provides a strand execution context.
//
//@CLASSES:
//  bmqex::Strand:         a strand execution context
//  bmqex::StrandExecutor: an executor to submit work to the strand
//
//@DESCRIPTION:
// This component provides a mechanism, 'bmqex::Strand', that is an execution
// context (see package documentation) that provides guarantees of ordering and
// non-concurrency.
//
/// Thread safety
///-------------
// 'bmqex::Strand' is fully thread-safe, meaning that multiple threads may use
// their own instances of the class or use a shared instance without further
// synchronization.
//
// With the exception of assignment operators, as well as the 'swap' member
// function, 'bmqex::StrandExecutor' is fully thread-safe, meaning that
// multiple threads may use their own instances of the class or use a shared
// instance without further synchronization.
//
/// Usage
///-----
// Given an executor 'ex' of type 'EX' associated with an arbitrary execution
// context (like, for example, a thread pool), we can ensure sequential non-
// concurrent execution of submitted functors using a strand context created on
// top of that executor:
//..
//  // create a strand
//  bmqex::Strand<EX> strand(ex);
//
//  // obtain an executor
//  bmqex::Strand<EX>::ExecutorType strandEx = strand.executor();
//
//  // start the strand
//  strand.start();
//
//  // submit functors
//  strandEx.post([]() { bsl::cout << "1"; });
//  strandEx.post([]() { bsl::cout << "2"; });
//  strandEx.post([]() { bsl::cout << "3"; });
//
//  // wait till all functors are executed
//  strand.join();
//
//  // the output is always "123"
//..

#include <bmqex_executortraits.h>
#include <bmqex_job.h>

// BDE
#include <bdlf_memfn.h>
#include <bsl_algorithm.h>  // bsl::swap
#include <bsl_deque.h>
#include <bslalg_constructorproxy.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_decay.h>
#include <bslmf_istriviallycopyable.h>
#include <bslmf_movableref.h>
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

namespace bmqex {

template <class>
class StrandExecutor;

// ========================
// struct Strand_ThreadUtil
// ========================

/// Provides a namespace for utility functions for managing threads.
struct Strand_ThreadUtil {
    // CLASS METHODS

    /// Return an unsigned 64-bit integer representing a thread id that is
    /// guaranteed never to be a valid thread id.
    static bsls::Types::Uint64 invalidThreadId() BSLS_KEYWORD_NOEXCEPT;
};

// ============
// class Strand
// ============

/// Provides an execution context that provides guarantees of ordering and
/// non-concurrency.
///
/// `EXECUTOR` must meet the requirements of Executor (see package
/// documentation).
template <class EXECUTOR>
class Strand {
  public:
    // TYPES

    /// Provides an executor to submit functors on the `Strand`.
    typedef StrandExecutor<EXECUTOR> ExecutorType;

    /// Defines the type of the underlying executor.
    typedef EXECUTOR InnerExecutorType;

  private:
    // PRIVATE DATA
    mutable bslmt::Mutex d_mutex;

    // Used to signal that `run` has completed.
    bslmt::Condition d_condition;

    // Underlying executor used to submit work.
    EXECUTOR d_innerExecutor;

    // The thread that is currently executing `run`, or an invalid id, if
    // `run` is not being executed.
    bsls::AtomicUint64 d_workingThreadId;

    // `true` if the strand is started, and `false` otherwise.
    bool d_isStarted;

    // `true` if a call to `run` has been initiated and is not yet
    // completed, and `false` otherwise.
    bool d_isRunning;

    // Functors to be executed.
    bsl::deque<Job> d_jobQueue;

    // FRIENDS
    template <class>
    friend class StrandExecutor;

  private:
    // PRIVATE MANIPULATORS

    /// Invoke functors from the job queue until either the queue is empty,
    /// or the context is stopped. Then, signal the condition.
    void run() BSLS_NOTHROW_SPEC;

  private:
    // NOT IMPLEMENTED
    Strand(const Strand&) BSLS_KEYWORD_DELETED;
    Strand& operator=(const Strand&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `Strand` object having its inner executor object default-
    /// constructed. Optionally specify a `basicAllocator` used to supply
    /// memory. If `basicAllocator` is 0, the default memory allocator is
    /// used.
    ///
    /// `EXECUTOR` must meet the requirements of DefaultConstructible as
    /// specified in the C++ standard.
    explicit Strand(bslma::Allocator* basicAllocator = 0);

    /// Create a `Strand` object having its inner executor object direct-
    /// non-list-initialized by `bsl::move(executor)`. Optionally specify a
    /// `basicAllocator` used to supply memory. If `basicAllocator` is 0,
    /// the default memory allocator is used.
    explicit Strand(EXECUTOR executor, bslma::Allocator* basicAllocator = 0);

    /// Destroy this object. Perform `stop()` followed by `join()`.
    ~Strand();

  public:
    // MANIPULATORS

    /// Begin executing functors on this execution context.
    ///
    /// This function meets the strong exception guarantee. If an exception
    /// is thrown, this function has no effect.
    void start();

    /// Stop executing functors on this execution context. Invocation of
    /// `stop` returns without waiting for the currently executing functor
    /// (if any) to complete.
    void stop() BSLS_KEYWORD_NOEXCEPT;

    /// Block the calling thread pending completion of the currently
    /// executing functor. Then, continue to block the calling thread until
    /// either `outstandingJobs()` is 0, or the execution context is stopped
    /// (if not already). The behavior is undefined if this function is
    /// invoked from a functor executed on this strand.
    void join() BSLS_KEYWORD_NOEXCEPT;

    /// Remove all functors but the one that is currently being invoked (if
    /// any) from this strand. Return the number of removed functors.
    size_t dropPendingJobs() BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Return the number of outstanding jobs for this execution context,
    /// that is defined as the number of functors that have been added to
    /// the strand via its associated executor, but not yet invoked, plus
    /// the number of functors that are currently being invoked within
    /// the strand, which is either 0 or 1.
    size_t outstandingJobs() const BSLS_KEYWORD_NOEXCEPT;

    /// Return an executor that may be used for submitting functors to this
    /// execution context.
    ExecutorType executor() const BSLS_KEYWORD_NOEXCEPT;

    /// Return the underlying executor.
    InnerExecutorType innerExecutor() const BSLS_KEYWORD_NOEXCEPT;

    /// Return the allocator used by this execution context to supply
    /// memory.
    bslma::Allocator* allocator() const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Strand, bslma::UsesBslmaAllocator)
};

// ====================
// class StrandExecutor
// ====================

/// Provides an executor to submit functors on the `Strand`.
template <class EXECUTOR>
class StrandExecutor {
  public:
    // TYPES
    typedef Strand<EXECUTOR> ContextType;

    /// Defines the type of the underlying executor.
    typedef EXECUTOR InnerExecutorType;

  private:
    // PRIVATE DATA
    ContextType* d_context_p;

    // FRIENDS
    template <class>
    friend class Strand;

  private:
    // PRIVATE CREATORS

    /// Create a `StrandExecutor` object having the specified `context` as
    /// its associated execution context.
    ///
    /// Note that this is a private constructor reserved to be used by the
    /// `Strand`.
    StrandExecutor(ContextType* context) BSLS_KEYWORD_NOEXCEPT;  // IMPLICIT

  public:
    // MANIPULATORS

    /// Submit the functor `f` for execution on the inner executor as if by
    /// 'ExecutorTraits<InnerExecutorType>::post(context().
    /// innerExecutor(), bsl::forward<FUNCTION>(f))', such that the
    /// guarantees of ordering and non-concurrency are met. If `f` exits via
    /// an exception, the `Strand` calls `bsl::terminate()`.
    ///
    /// This function meets the strong exception guarantee. If an exception
    /// is thrown, this function has no effect.
    ///
    /// `bsl::decay_t<FUNCTION>` must meet the requirements of Destructible
    /// and MoveConstructible as specified in the C++ standard.
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
    /// and MoveConstructible as specified in the C++ standard.
    /// `DECAY_COPY(bsl::forward<FUNCTION>(f))()` shall be a valid
    /// expression.
    template <class FUNCTION>
    void dispatch(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f) const;

    /// Swap the contents of `*this` and `other`.
    void swap(StrandExecutor& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Return `true` if the current thread of execution is running a
    /// functor that was submitted to the associated `Strand` using `post`
    /// or `dispatch`. Otherwise, return `false`.
    ///
    /// Note that is, the current thread of execution's call chain includes
    /// a function that was submitted to the associated `Strand`.
    bool runningInThisThread() const BSLS_KEYWORD_NOEXCEPT;

    /// Return a reference to the associated `Strand` object.
    ContextType& context() const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StrandExecutor, bsl::is_trivially_copyable)
};

// FREE OPERATORS

/// Return `&lhs.context() == &rhs.context()`.
template <class EXECUTOR>
bool operator==(const StrandExecutor<EXECUTOR>& lhs,
                const StrandExecutor<EXECUTOR>& rhs) BSLS_KEYWORD_NOEXCEPT;

/// Return `!(lhs == rhs)`.
template <class EXECUTOR>
bool operator!=(const StrandExecutor<EXECUTOR>& lhs,
                const StrandExecutor<EXECUTOR>& rhs) BSLS_KEYWORD_NOEXCEPT;

/// Swap the contents of `lhs` and `rhs`.
template <class EXECUTOR>
void swap(StrandExecutor<EXECUTOR>& lhs,
          StrandExecutor<EXECUTOR>& rhs) BSLS_KEYWORD_NOEXCEPT;

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// ------------
// class Strand
// ------------

// PRIVATE MANIPULATORS
template <class EXECUTOR>
inline void Strand<EXECUTOR>::run() BSLS_NOTHROW_SPEC
{
    // NOTE: 'BSLS_NOTHROW_SPEC' specification ensures that 'bsl::terminate' is
    //       called in case of exception in C++03.

    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

    // set the working thread to this one
    d_workingThreadId.storeRelease(bslmt::ThreadUtil::selfIdAsUint64());

    while (d_isStarted && !d_jobQueue.empty()) {
        // Execute all jobs in the queue.

        // execute job outside the lock
        Job& job = d_jobQueue.front();
        d_mutex.unlock();  // UNLOCK
        job();
        d_mutex.lock();  // LOCK

        // remove job from the queue
        d_jobQueue.pop_front();
    }

    // unset the working thread
    d_workingThreadId.storeRelease(Strand_ThreadUtil::invalidThreadId());

    // unset the 'isRunning' flag
    d_isRunning = false;

    // signal the function has completed
    d_condition.broadcast();
}

// CREATORS
template <class EXECUTOR>
inline Strand<EXECUTOR>::Strand(bslma::Allocator* basicAllocator)
: d_mutex()
, d_condition()
, d_innerExecutor()
, d_workingThreadId(Strand_ThreadUtil::invalidThreadId())
, d_isStarted(false)
, d_isRunning(false)
, d_jobQueue(basicAllocator)
{
    // NOTHING
}

template <class EXECUTOR>
inline Strand<EXECUTOR>::Strand(EXECUTOR          executor,
                                bslma::Allocator* basicAllocator)
: d_mutex()
, d_condition()
, d_innerExecutor(bslmf::MovableRefUtil::move(executor))
, d_workingThreadId(Strand_ThreadUtil::invalidThreadId())
, d_isStarted(false)
, d_isRunning(false)
, d_jobQueue(basicAllocator)
{
    // NOTHING
}

template <class EXECUTOR>
inline Strand<EXECUTOR>::~Strand()
{
    stop();
    join();
}

// MANIPULATORS
template <class EXECUTOR>
inline void Strand<EXECUTOR>::start()
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

    if (d_isStarted) {
        // Already started. Do nothing.
        return;  // RETURN
    }

    if (!d_jobQueue.empty() && !d_isRunning) {
        // The job queue isn't empty but no async operation is in progress.
        // Initiate one.
        ExecutorTraits<EXECUTOR>::post(d_innerExecutor,
                                       bdlf::MemFnUtil::memFn(&Strand::run,
                                                              this));
        d_isRunning = true;
    }

    d_isStarted = true;
}

template <class EXECUTOR>
inline void Strand<EXECUTOR>::stop() BSLS_KEYWORD_NOEXCEPT
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

    d_isStarted = false;
}

template <class EXECUTOR>
inline void Strand<EXECUTOR>::join() BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(d_workingThreadId.loadAcquire() !=
                bslmt::ThreadUtil::selfIdAsUint64());

    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

    while (d_isRunning) {
        d_condition.wait(&d_mutex);  // UNLOCK / LOCK
    }
}

template <class EXECUTOR>
inline size_t Strand<EXECUTOR>::dropPendingJobs() BSLS_KEYWORD_NOEXCEPT
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

    // obtain the queue size
    const size_t jobQueueSize = d_jobQueue.size();

    // do remove
    if (d_isRunning) {
        // A job is currently executing. Remove all jobs from the queue but
        // that one.

        // If we are here, the job queue shall not be empty.
        BSLS_ASSERT(jobQueueSize != 0);

        for (size_t i = 0; i < jobQueueSize - 1; ++i) {
            // NOTE: Can't call 'erase' because it requires elements to be
            //       moveable, so we 'pop_back'.
            d_jobQueue.pop_back();
        }

        return jobQueueSize - 1;  // RETURN
    }
    else {
        // No job is currently executing. Remove all jobs from the queue.
        d_jobQueue.clear();
        return jobQueueSize;  // RETURN
    }
}

// ACCESSORS
template <class EXECUTOR>
inline size_t Strand<EXECUTOR>::outstandingJobs() const BSLS_KEYWORD_NOEXCEPT
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

    return d_jobQueue.size();
}

template <class EXECUTOR>
inline typename Strand<EXECUTOR>::ExecutorType
Strand<EXECUTOR>::executor() const BSLS_KEYWORD_NOEXCEPT
{
    return const_cast<Strand*>(this);
}

template <class EXECUTOR>
inline typename Strand<EXECUTOR>::InnerExecutorType
Strand<EXECUTOR>::innerExecutor() const BSLS_KEYWORD_NOEXCEPT
{
    return d_innerExecutor;
}

template <class EXECUTOR>
inline bslma::Allocator*
Strand<EXECUTOR>::allocator() const BSLS_KEYWORD_NOEXCEPT
{
    return d_jobQueue.get_allocator().mechanism();
}

// --------------------
// class StrandExecutor
// --------------------

// PRIVATE CREATORS
template <class EXECUTOR>
inline StrandExecutor<EXECUTOR>::StrandExecutor(ContextType* context)
    BSLS_KEYWORD_NOEXCEPT : d_context_p(context)
{
    // PRECONDITIONS
    BSLS_ASSERT(context);
}

// MANIPULATORS
template <class EXECUTOR>
template <class FUNCTION>
inline void
StrandExecutor<EXECUTOR>::post(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION)
                                   f) const
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_context_p->d_mutex);  // LOCK

    // enqueue the functor
    d_context_p->d_jobQueue.emplace_back(
        BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f));

    if (d_context_p->d_isStarted && !d_context_p->d_isRunning) {
        // The strand is started but no async operation is in progress.
        // Initiate one.

        try {
            ExecutorTraits<EXECUTOR>::post(
                d_context_p->d_innerExecutor,
                bdlf::MemFnUtil::memFn(&ContextType::run, d_context_p));
            d_context_p->d_isRunning = true;
        }
        catch (...) {
            d_context_p->d_jobQueue.pop_back();  // rollback
            throw;                               // rethrow exception
        }
    }
}

template <class EXECUTOR>
template <class FUNCTION>
inline void
StrandExecutor<EXECUTOR>::dispatch(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION)
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

template <class EXECUTOR>
inline void
StrandExecutor<EXECUTOR>::swap(StrandExecutor& other) BSLS_KEYWORD_NOEXCEPT
{
    bsl::swap(d_context_p, other.d_context_p);
}

// ACCESSORS
template <class EXECUTOR>
inline bool
StrandExecutor<EXECUTOR>::runningInThisThread() const BSLS_KEYWORD_NOEXCEPT
{
    return d_context_p->d_workingThreadId.loadAcquire() ==
           bslmt::ThreadUtil::selfIdAsUint64();
}

template <class EXECUTOR>
inline Strand<EXECUTOR>&
StrandExecutor<EXECUTOR>::context() const BSLS_KEYWORD_NOEXCEPT
{
    return *d_context_p;
}

}  // close package namespace

// FREE OPERATORS
template <class EXECUTOR>
inline bool
bmqex::operator==(const StrandExecutor<EXECUTOR>& lhs,
                  const StrandExecutor<EXECUTOR>& rhs) BSLS_KEYWORD_NOEXCEPT
{
    return &lhs.context() == &rhs.context();
}

template <class EXECUTOR>
inline bool
bmqex::operator!=(const StrandExecutor<EXECUTOR>& lhs,
                  const StrandExecutor<EXECUTOR>& rhs) BSLS_KEYWORD_NOEXCEPT
{
    return !(lhs == rhs);
}

template <class EXECUTOR>
inline void bmqex::swap(StrandExecutor<EXECUTOR>& lhs,
                        StrandExecutor<EXECUTOR>& rhs) BSLS_KEYWORD_NOEXCEPT
{
    lhs.swap(rhs);
}

}  // close enterprise namespace

#endif
