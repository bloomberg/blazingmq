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

// bmqex_bdlmtthreadpoolexecutor.h                                    -*-C++-*-
#ifndef INCLUDED_BMQEX_BDLMTTHREADPOOLEXECUTOR
#define INCLUDED_BMQEX_BDLMTTHREADPOOLEXECUTOR

//@PURPOSE: Provides an executor adapter for 'bdlmt::ThreadPool'.
//
//@CLASSES:
//  BdlmtThreadPoolExecutor: executor adapter for bdlmt thread pool
//
//@DESCRIPTION:
// This component provides a class, 'bmqex::BdlmtThreadPoolExecutor', that is
// an executor adapter for 'bdlmt::ThreadPool'.
//
/// Thread safety
///-------------
// With the exception of assignment operators, as well as the 'swap' member
// function, 'bmqex::BdlmtThreadPoolExecutor' is fully thread-safe, meaning
// that multiple threads may use their own instances of the class or use a
// shared instance without further synchronization.
//
/// Usage
///-----
// Lets assume that you already have an instance of 'bdlmt::ThreadPool',
// 'd_threadPool'. Then, you can create an executor adapter on top of it, and
// use the adapter with the 'bmqex' package. For example:
//..
//  // create an executor
//  bmqex::BdlmtThreadPoolExecutor ex(&d_threadPool);
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
class ThreadPool;
}

namespace bmqex {

// =============================
// class BdlmtThreadPoolExecutor
// =============================

/// Provides an executor adapter for `bdlmt::ThreadPool`.
class BdlmtThreadPoolExecutor {
  public:
    // TYPES

    /// Defines the type of the associated execution context.
    typedef bdlmt::ThreadPool ContextType;

  private:
    // PRIVATE DATA
    ContextType* d_context_p;

  public:
    // CREATORS
    BdlmtThreadPoolExecutor(ContextType* context) BSLS_KEYWORD_NOEXCEPT;
    // IMPLICIT
    // Create a 'BdlmtThreadPoolExecutor' object having the specified
    // 'context' as its associated execution context.

  public:
    // MANIPULATORS

    /// Submit the specified function object `f` to the associated
    /// `bdlmt::ThreadPool` as if by `context().enqueueJob(f)`. The behavior
    /// is undefined unless the queuing is enabled on the associated
    /// `bdlmt::ThreadPool`.
    void post(const bsl::function<void()>& f) const;

    /// Swap the contents of `*this` and `other`.
    void swap(BdlmtThreadPoolExecutor& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Return a reference to the associated `bdlmt::ThreadPool` object.
    ContextType& context() const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(BdlmtThreadPoolExecutor,
                                   bsl::is_trivially_copyable)
};

// FREE OPERATORS

/// Return `&lhs.context() == &rhs.context()`.
bool operator==(const BdlmtThreadPoolExecutor& lhs,
                const BdlmtThreadPoolExecutor& rhs) BSLS_KEYWORD_NOEXCEPT;

/// Return `!(lhs == rhs)`.
bool operator!=(const BdlmtThreadPoolExecutor& lhs,
                const BdlmtThreadPoolExecutor& rhs) BSLS_KEYWORD_NOEXCEPT;

/// Swap the contents of `lhs` and `rhs`.
void swap(BdlmtThreadPoolExecutor& lhs,
          BdlmtThreadPoolExecutor& rhs) BSLS_KEYWORD_NOEXCEPT;

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// -----------------------------
// class BdlmtThreadPoolExecutor
// -----------------------------

// CREATORS
inline BdlmtThreadPoolExecutor::BdlmtThreadPoolExecutor(ContextType* context)
    BSLS_KEYWORD_NOEXCEPT : d_context_p(context)
{
    // PRECONDITIONS
    BSLS_ASSERT(context);
}

// MANIPULATORS
inline void BdlmtThreadPoolExecutor::swap(BdlmtThreadPoolExecutor& other)
    BSLS_KEYWORD_NOEXCEPT
{
    using bsl::swap;

    swap(d_context_p, other.d_context_p);
}

// ACCESSORS
inline BdlmtThreadPoolExecutor::ContextType&
BdlmtThreadPoolExecutor::context() const BSLS_KEYWORD_NOEXCEPT
{
    return *d_context_p;
}

}  // close package namespace

// FREE OPERATORS
inline bool
bmqex::operator==(const BdlmtThreadPoolExecutor& lhs,
                  const BdlmtThreadPoolExecutor& rhs) BSLS_KEYWORD_NOEXCEPT
{
    return &lhs.context() == &rhs.context();
}

inline bool
bmqex::operator!=(const BdlmtThreadPoolExecutor& lhs,
                  const BdlmtThreadPoolExecutor& rhs) BSLS_KEYWORD_NOEXCEPT
{
    return !(lhs == rhs);
}

inline void bmqex::swap(BdlmtThreadPoolExecutor& lhs,
                        BdlmtThreadPoolExecutor& rhs) BSLS_KEYWORD_NOEXCEPT
{
    lhs.swap(rhs);
}

}  // close enterprise namespace

#endif
