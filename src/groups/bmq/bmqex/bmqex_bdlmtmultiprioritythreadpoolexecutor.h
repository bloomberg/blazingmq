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

// bmqex_bdlmtmultiprioritythreadpoolexecutor.h                       -*-C++-*-
#ifndef INCLUDED_BMQEX_BDLMTMULTIPRIORITYTHREADPOOLEXECUTOR
#define INCLUDED_BMQEX_BDLMTMULTIPRIORITYTHREADPOOLEXECUTOR

//@PURPOSE: Provides an executor adapter for 'bdlmt::MultipriorityThreadPool'.
//
//@CLASSES:
//  BdlmtMultipriorityThreadPoolExecutor: executor adapter for bdlmt m.p.t.p.
//
//@DESCRIPTION:
// This component provides a class,
// 'bmqex::BdlmtMultipriorityThreadPoolExecutor', that is an executor adapter
// for 'bdlmt::MultipriorityThreadPool'.
//
/// Thread safety
///-------------
// With the exception of assignment operators, as well as the 'swap' member
// function, 'bmqex::BdlmtMultipriorityThreadPoolExecutor' is fully thread-
// safe, meaning that multiple threads may use their own instances of the class
// or use a shared instance without further synchronization.
//
/// Usage
///-----
// Lets assume that you already have an instance of
// 'bdlmt::MultiQueueThreadPool', 'd_threadPool', and a priority, 'd_priority'.
// Then, you can create an executor adapter on top of the thread pool, and use
// the adapter with the 'bmqex' package. For example:
//..
//  // create an executor
//  bmqex::BdlmtMultipriorityThreadPoolExecutor ex(&d_threadPool, d_priority);
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
#include <bslmf_istriviallycopyable.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class MultipriorityThreadPool;
}

namespace bmqex {

// ==========================================
// class BdlmtMultipriorityThreadPoolExecutor
// ==========================================

/// Provides an executor adapter for `bdlmt::MultipriorityThreadPool`.
class BdlmtMultipriorityThreadPoolExecutor {
  public:
    // TYPES

    /// Defines the type of the associated execution context.
    typedef bdlmt::MultipriorityThreadPool ContextType;

  private:
    // PRIVATE DATA
    ContextType* d_context_p;

    int d_priority;

  public:
    // CREATORS
    BdlmtMultipriorityThreadPoolExecutor(ContextType* context,
                                         int          priority = 0)
        BSLS_KEYWORD_NOEXCEPT;
    // IMPLICIT
    // Create a 'BdlmtMultipriorityThreadPoolExecutor' object having the
    // specified 'context' as its associated execution context. Optionally
    // specify the 'priority' used when submitting function objects to the
    // associated 'bdlmt::MultipriorityThreadPool'.

  public:
    // MANIPULATORS

    /// Submit the specified function object `f` to the associated
    /// `bdlmt::MultipriorityThreadPool` as if by 'context().enqueueJob(f,
    /// priority())'. The behavior is undefined unless the queuing is
    /// enabled on the associated `bdlmt::MultipriorityThreadPool` and
    /// `0 <= priority < context().numPriorities()`.
    void post(const bsl::function<void()>& f) const;

    /// Swap the contents of `*this` and `other`.
    void
    swap(BdlmtMultipriorityThreadPoolExecutor& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Return a reference to the associated
    /// `bdlmt::MultipriorityThreadPool` object.
    ContextType& context() const BSLS_KEYWORD_NOEXCEPT;

    /// Return the associated priority.
    int priority() const BSLS_KEYWORD_NOEXCEPT;

    /// Return a `BdlmtMultipriorityThreadPoolExecutor` object having the
    /// same associated execution context as `*this` and the specified
    /// `priority`, as if by
    /// `return BdlmtMultipriorityThreadPoolExecutor(&context(), priority)`.
    BdlmtMultipriorityThreadPoolExecutor
    rebindPriority(int priority) const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(BdlmtMultipriorityThreadPoolExecutor,
                                   bsl::is_trivially_copyable)
};

// FREE OPERATORS

/// Return '&lhs.context()  == &rhs.context() &&
///          lhs.priority() ==  rhs.priority()'.
bool operator==(const BdlmtMultipriorityThreadPoolExecutor& lhs,
                const BdlmtMultipriorityThreadPoolExecutor& rhs)
    BSLS_KEYWORD_NOEXCEPT;

/// Return `!(lhs == rhs)`.
bool operator!=(const BdlmtMultipriorityThreadPoolExecutor& lhs,
                const BdlmtMultipriorityThreadPoolExecutor& rhs)
    BSLS_KEYWORD_NOEXCEPT;

/// Swap the contents of `lhs` and `rhs`.
void swap(BdlmtMultipriorityThreadPoolExecutor& lhs,
          BdlmtMultipriorityThreadPoolExecutor& rhs) BSLS_KEYWORD_NOEXCEPT;

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// ------------------------------------------
// class BdlmtMultipriorityThreadPoolExecutor
// ------------------------------------------

// CREATORS
inline BdlmtMultipriorityThreadPoolExecutor::
    BdlmtMultipriorityThreadPoolExecutor(ContextType* context,
                                         int priority) BSLS_KEYWORD_NOEXCEPT
: d_context_p(context),
  d_priority(priority)
{
    // PRECONDITIONS
    BSLS_ASSERT(context);
}

// MANIPULATORS
inline void BdlmtMultipriorityThreadPoolExecutor::swap(
    BdlmtMultipriorityThreadPoolExecutor& other) BSLS_KEYWORD_NOEXCEPT
{
    using bsl::swap;

    swap(d_context_p, other.d_context_p);
    swap(d_priority, other.d_priority);
}

// ACCESSORS
inline BdlmtMultipriorityThreadPoolExecutor::ContextType&
BdlmtMultipriorityThreadPoolExecutor::context() const BSLS_KEYWORD_NOEXCEPT
{
    return *d_context_p;
}

inline int
BdlmtMultipriorityThreadPoolExecutor::priority() const BSLS_KEYWORD_NOEXCEPT
{
    return d_priority;
}

inline BdlmtMultipriorityThreadPoolExecutor
BdlmtMultipriorityThreadPoolExecutor::rebindPriority(int priority) const
    BSLS_KEYWORD_NOEXCEPT
{
    return BdlmtMultipriorityThreadPoolExecutor(d_context_p, priority);
}

}  // close package namespace

// FREE OPERATORS
inline bool bmqex::operator==(const BdlmtMultipriorityThreadPoolExecutor& lhs,
                              const BdlmtMultipriorityThreadPoolExecutor& rhs)
    BSLS_KEYWORD_NOEXCEPT
{
    return &lhs.context() == &rhs.context() &&
           lhs.priority() == rhs.priority();
}

inline bool bmqex::operator!=(const BdlmtMultipriorityThreadPoolExecutor& lhs,
                              const BdlmtMultipriorityThreadPoolExecutor& rhs)
    BSLS_KEYWORD_NOEXCEPT
{
    return !(lhs == rhs);
}

inline void
bmqex::swap(BdlmtMultipriorityThreadPoolExecutor& lhs,
            BdlmtMultipriorityThreadPoolExecutor& rhs) BSLS_KEYWORD_NOEXCEPT
{
    lhs.swap(rhs);
}

}  // close enterprise namespace

#endif
