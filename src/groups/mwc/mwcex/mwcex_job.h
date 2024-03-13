// Copyright 2021-2023 Bloomberg Finance L.P.
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

// mwcex_job.h                                                        -*-C++-*-
#ifndef INCLUDED_MWCEX_JOB
#define INCLUDED_MWCEX_JOB

//@PURPOSE: Provides a polymorphic function object wrapper.
//
//@CLASSES:
//  mwcex::Job: a polymorphic function object wrapper.
//
//@DESCRIPTION:
// THIS IS A PRIVATE COMPONENT NOT TO BE USED OUTSIDE OF THIS PACKAGE.
//
// This component provides a polymorphic wrapper for nullary function objects,
// 'mwcex::Job', that is similar to 'bsl::function', but optimized for a narrow
// purpose.
//
// 'mwcex::Job' is designed to be memory-efficient, it cannot be copied, moved
// or swapped, and only supports the absolute minimum amount of operations
// required, that is construction, destruction and invocation. Unlike
// 'bsl::function' that invokes the target functor 'f' as if by 'f()',
// 'mwcex::Job' does it as as if by 'bsl::move(f)()'.

// MWC

#include <mwcu_objectplaceholder.h>

// BDE
#include <bslalg_constructorproxy.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_decay.h>
#include <bslmf_movableref.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmf_util.h>
#include <bsls_compilerfeatures.h>
#include <bsls_keyword.h>

namespace BloombergLP {

namespace mwcex {

// ====================
// class Job_TargetBase
// ====================

/// An interface used to implement the type erasure technique. When creating
/// a polymorphic wrapper with a target of type `F`, an instance of derived
/// class template `Job_Target<F>` is instantiated and stored via a pointer
/// to its base class (this one). Then, calls to `mwcex::Job`s public
/// methods are forwarded to this class.
class Job_TargetBase {
  public:
    // CREATORS

    /// Destroy this object and the contained function object with it.
    virtual ~Job_TargetBase();

  public:
    // MANIPULATORS

    /// Invoke the contained function object `f` as if by `bsl::move(f)()`.
    virtual void invoke() = 0;
};

// ================
// class Job_Target
// ================

/// An implementation of the `Job_TargetBase` interface containing the
/// function object.
template <class FUNCTION>
class Job_Target : public Job_TargetBase {
  private:
    // PRIVATE DATA

    // Function object to be invoked. May or may not be an allocator-aware
    // object, hence the `bslalg::ConstructorProxy` wrapper.
    bslalg::ConstructorProxy<FUNCTION> d_function;

  private:
    // NOT IMPLEMENTED
    Job_Target(const Job_Target&) BSLS_KEYWORD_DELETED;
    Job_Target& operator=(const Job_Target&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `Job_Target` object containing a function object of type
    /// `FUNCTION` direct-non-list-initialized by 'bsl::forward<
    /// FUNCTION_PARAM>(function)`. Specify an `allocator' used to
    /// supply memory.
    template <class FUNCTION_PARAM>
    Job_Target(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION_PARAM) function,
               bslma::Allocator* allocator);

  public:
    // MANIPULATORS

    /// Invoke the contained function object `f` as if by `bsl::move(f)()`.
    void invoke() BSLS_KEYWORD_OVERRIDE;
};

// =========
// class Job
// =========

/// A polymorphic function object wrapper with small buffer optimization.
class Job {
  private:
    // PRIVATE TYPES

    /// A "small" dummy object used to help calculate the size of the
    /// on-stack buffer.
    struct Dummy {
        void* d_padding[5];
    };

  private:
    // PRIVATE DATA

    // Uses an on-stack buffer to allocate memory for "small" objects, and
    // falls back to requesting memory from the supplied allocator if
    // the buffer is not large enough. Note that the size of the on-stack
    // buffer is an arbitrary value.
    mwcu::ObjectPlaceHolder<sizeof(Job_Target<Dummy>)> d_target;

  private:
    // NOT IMPLEMENTED
    Job(const Job&) BSLS_KEYWORD_DELETED;
    Job& operator=(const Job&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `Job` object containing a function object of type
    /// `bsl::decay_t<FUNCTION>` direct-non-list-initialized by
    /// `bsl::forward<FUNCTION>(function)`. Optionally specify a
    /// `basicAllocator` used to supply memory. If `basicAllocator` is 0,
    /// the default memory allocator is used.
    ///
    /// `bsl::decay_t<FUNCTION>` must meet the requirements of Destructible
    /// and MoveConstructible as specified in the C++ standard. Given an
    /// object `f` of type `bsl::decay_t<FUNCTION>`, `f()` shall be a valid
    /// expression.
    template <class FUNCTION>
    explicit Job(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) function,
                 bslma::Allocator* basicAllocator = 0);

    /// Destroy this object and the contained function object with it.
    ~Job();

  public:
    // MANIPULATORS

    /// Invoke the contained function object `f` as if by `bsl::move(f)()`.
    void operator()();

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Job, bslma::UsesBslmaAllocator)
};

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// ---------
// class Job
// ---------

// CREATORS
template <class FUNCTION>
inline Job::Job(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) function,
                bslma::Allocator* basicAllocator)
{
    typedef Job_Target<typename bsl::decay<FUNCTION>::type> Target;

    d_target.createObject<Target>(bslma::Default::allocator(basicAllocator),
                                  BSLS_COMPILERFEATURES_FORWARD(FUNCTION,
                                                                function),
                                  bslma::Default::allocator(basicAllocator));
}

// ----------------
// class Job_Target
// ----------------

// CREATORS
template <class FUNCTION>
template <class FUNCTION_PARAM>
inline Job_Target<FUNCTION>::Job_Target(
    BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION_PARAM) function,
    bslma::Allocator* allocator)
: d_function(BSLS_COMPILERFEATURES_FORWARD(FUNCTION_PARAM, function),
             allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);
}

// MANIPULATORS
template <class FUNCTION>
inline void Job_Target<FUNCTION>::invoke()
{
    bslmf::Util::moveIfSupported(d_function.object())();
}

}  // close package namespace
}  // close enterprise namespace

#endif
