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

// bmqex_bdlmtmultiqueuethreadpoolexecutor.h                          -*-C++-*-
#ifndef INCLUDED_BMQEX_BDLMTMULTIQUEUETHREADPOOLEXECUTOR
#define INCLUDED_BMQEX_BDLMTMULTIQUEUETHREADPOOLEXECUTOR

//@PURPOSE: Provides an executor adapter for 'bdlmt::MultiQueueThreadPool'.
//
//@CLASSES:
//  BdlmtMultiQueueThreadPoolExecutor: executor adapter for bdlmt m.q.t. pool
//
//@DESCRIPTION:
// This component provides a class, 'bmqex::BdlmtMultiQueueThreadPoolExecutor',
// that is an executor adapter for 'bdlmt::MultiQueueThreadPool'.
//
/// Thread safety
///-------------
// With the exception of assignment operators, as well as the 'swap' member
// function, 'bmqex::BdlmtMultiQueueThreadPoolExecutor' is fully thread-safe,
// meaning that multiple threads may use their own instances of the class or
// use a shared instance without further synchronization.
//
/// Usage
///-----
// Lets assume that you already have an instance of
// 'bdlmt::MultiQueueThreadPool', 'd_threadPool', and a queue id, 'd_queueId',
// that represents a queue on that thread pool. Then, you can create an
// executor adapter on top of the thread pool, and use the adapter with the
// 'bmqex' package. For example:
//..
//  // create an executor
//  bmqex::BdlmtMultiQueueThreadPoolExecutor ex(&d_threadPool, d_queueId);
//
//  // use it
//  bmqex::ExecutionUtil::execute(bmqex::ExecutionPolicyUtil::oneWay()
//                                                           .neverBlocking()
//                                                           .useExecutor(ex),
//                                [](){ bsl::cout << "Hello World!\n"; });
//..

// BDE
#include <bsl_algorithm.h>  // bsl::swap
#include <bsl_functional.h>
#include <bsl_type_traits.h>
#include <bslmf_istriviallycopyable.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class MultiQueueThreadPool;
}

namespace bmqex {

// =======================================
// class BdlmtMultiQueueThreadPoolExecutor
// =======================================

/// Provides an executor adapter for `bdlmt::MultiQueueThreadPool`.
class BdlmtMultiQueueThreadPoolExecutor {
  public:
    // TYPES

    /// Defines the type of the associated execution context.
    typedef bdlmt::MultiQueueThreadPool ContextType;

  private:
    // PRIVATE DATA
    ContextType* d_context_p;

    int d_queueId;

  public:
    // CREATORS

    /// Create a `BdlmtMultiQueueThreadPoolExecutor` object having the
    /// specified `context` as its associated execution context. Specify the
    /// `queueId` used when submitting function objects to the associated
    /// `bdlmt::MultiQueueThreadPool`.
    BdlmtMultiQueueThreadPoolExecutor(ContextType* context,
                                      int queueId) BSLS_KEYWORD_NOEXCEPT;

  public:
    // MANIPULATORS

    /// Submit the specified function object `f` to the associated
    /// `bdlmt::MultiQueueThreadPool` as if by 'context().enqueueJob(
    /// queueId(), f)'. The behavior is undefined unless the queuing is
    /// enabled on the associated `bdlmt::MultiQueueThreadPool` and
    /// `queueId()` is a valid queue id.
    void post(const bsl::function<void()>& f) const;

    /// Perform `post(f)`.
    void dispatch(const bsl::function<void()>& f) const;

    /// Swap the contents of `*this` and `other`.
    void swap(BdlmtMultiQueueThreadPoolExecutor& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Return a reference to the associated `bdlmt::MultiQueueThreadPool`
    /// object.
    ContextType& context() const BSLS_KEYWORD_NOEXCEPT;

    /// Return the associated queue id.
    int queueId() const BSLS_KEYWORD_NOEXCEPT;

    /// Return a `BdlmtMultiQueueThreadPoolExecutor` object having the same
    /// associated execution context as `*this` and the specified `queueId`,
    /// as if by 'return BdlmtMultiQueueThreadPoolExecutor(&context(),
    /// queueId)'.
    BdlmtMultiQueueThreadPoolExecutor
    rebindQueueId(int queueId) const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(BdlmtMultiQueueThreadPoolExecutor,
                                   bsl::is_trivially_copyable)
};

// FREE OPERATORS

/// Return '&lhs.context() == &rhs.context() &&
///          lhs.queueId() ==  rhs.queueId()'.
bool operator==(const BdlmtMultiQueueThreadPoolExecutor& lhs,
                const BdlmtMultiQueueThreadPoolExecutor& rhs)
    BSLS_KEYWORD_NOEXCEPT;

/// Return `!(lhs == rhs)`.
bool operator!=(const BdlmtMultiQueueThreadPoolExecutor& lhs,
                const BdlmtMultiQueueThreadPoolExecutor& rhs)
    BSLS_KEYWORD_NOEXCEPT;

/// Swap the contents of `lhs` and `rhs`.
void swap(BdlmtMultiQueueThreadPoolExecutor& lhs,
          BdlmtMultiQueueThreadPoolExecutor& rhs) BSLS_KEYWORD_NOEXCEPT;

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// ---------------------------------------
// class BdlmtMultiQueueThreadPoolExecutor
// ---------------------------------------

// CREATORS
inline BdlmtMultiQueueThreadPoolExecutor::BdlmtMultiQueueThreadPoolExecutor(
    ContextType* context,
    int          queueId) BSLS_KEYWORD_NOEXCEPT : d_context_p(context),
                                         d_queueId(queueId)
{
    // PRECONDITIONS
    BSLS_ASSERT(context);
}

// MANIPULATORS
inline void BdlmtMultiQueueThreadPoolExecutor::swap(
    BdlmtMultiQueueThreadPoolExecutor& other) BSLS_KEYWORD_NOEXCEPT
{
    using bsl::swap;

    swap(d_context_p, other.d_context_p);
    swap(d_queueId, other.d_queueId);
}

// ACCESSORS
inline BdlmtMultiQueueThreadPoolExecutor::ContextType&
BdlmtMultiQueueThreadPoolExecutor::context() const BSLS_KEYWORD_NOEXCEPT
{
    return *d_context_p;
}

inline int
BdlmtMultiQueueThreadPoolExecutor::queueId() const BSLS_KEYWORD_NOEXCEPT
{
    return d_queueId;
}

inline BdlmtMultiQueueThreadPoolExecutor
BdlmtMultiQueueThreadPoolExecutor::rebindQueueId(int queueId) const
    BSLS_KEYWORD_NOEXCEPT
{
    return BdlmtMultiQueueThreadPoolExecutor(d_context_p, queueId);
}

}  // close package namespace

// FREE OPERATORS
inline bool bmqex::operator==(const BdlmtMultiQueueThreadPoolExecutor& lhs,
                              const BdlmtMultiQueueThreadPoolExecutor& rhs)
    BSLS_KEYWORD_NOEXCEPT
{
    return &lhs.context() == &rhs.context() && lhs.queueId() == rhs.queueId();
}

inline bool bmqex::operator!=(const BdlmtMultiQueueThreadPoolExecutor& lhs,
                              const BdlmtMultiQueueThreadPoolExecutor& rhs)
    BSLS_KEYWORD_NOEXCEPT
{
    return !(lhs == rhs);
}

inline void
bmqex::swap(BdlmtMultiQueueThreadPoolExecutor& lhs,
            BdlmtMultiQueueThreadPoolExecutor& rhs) BSLS_KEYWORD_NOEXCEPT
{
    lhs.swap(rhs);
}

}  // close enterprise namespace

#endif
