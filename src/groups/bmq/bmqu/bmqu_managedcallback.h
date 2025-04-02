// Copyright 2025 Bloomberg Finance L.P.
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

// bmqu_managedcallback.h                                             -*-C++-*-
#ifndef INCLUDED_BMQU_MANAGEDCALLBACK
#define INCLUDED_BMQU_MANAGEDCALLBACK

//@PURPOSE: Provide a mechanism to build functors in a reusable buffer.
//
//@CLASSES:
//  bmqu::ManagedCallback: Mechanism to build functors in its buffer.
//  bmqu::ManagedCallback::CallbackFunctor:
//             Base interface for functors acceptable by bmqu::ManagedCallback.
//
//
//@DESCRIPTION: 'bmqu::ManagedCallback' provides a mechanism to build functors
// in its buffer.  The objects of this type are intended to be reused to avoid
// buffer reallocations for better performance on critical paths.
//
/// Usage
///-----
// First, need to declare a functor class, and inherit it from the acceptable
// callback type.
//..
//  struct ExampleCallback : public bmqu::ManagedCallback::CallbackFunctor {
//      size_t* d_calls_p;
//
//      explicit ExampleCallback(size_t* calls)
//      : d_calls_p(calls)
//      {
//          // PRECONDITIONS
//          BSLS_ASSERT_SAFE(calls);
//      }
//
//      ~ExampleCallback() BSLS_KEYWORD_OVERRIDE
//      {
//          // NOTHING
//      }
//
//      void operator()() const BSLS_KEYWORD_OVERRIDE { ++(*d_calls_p); }
// };
//..
//
// Next, you can use the defined callback type with bmqu::ManagedCallback:
//..
//  bmqu::ManagedCallback callback(allocator);
//  size_t                counter = 0;
//
//  // A buffer will be reallocated here once to store the callback:
//  callback.createInplace<ExampleCallback>(&counter);
//  for (int i = 0; i < 5; ++i) {
//      callback();
//  }
//  // Has to reset `bmqu::ManagedCallback` object before reusing it:
//  callback.reset();
//
//  for (int i = 0; i < 100; i++) {
//      // There will be no buffer reallocations on these calls, because we
//      // reuse the same `bmqu::ManagedCallback` object, and it has sufficient
//      // buffer capacity to store `ExampleCallback`:
//      callback.createInplace<ExampleCallback>(&counter);
//      callback();
//      callback.reset();
//  }
//..
//
// NOTE: the need to support cpp03 for Solaris makes it impossible to call
//       `createInplace` with classes that expect non-const references in
//       constructor arguments.  This is due to cpp03 compatibility code
//       passing arguments as (const &) when perfect forwarding with `&&`
//       is not available.
//       Consider passing output arguments with equivalent code using pointers
//       to mutable variables instead.
//

// BDE
#include <bsl_functional.h>
#include <bsl_utility.h>  // bsl::forward
#include <bsl_vector.h>
#include <bslmf_isaccessiblebaseof.h>
#include <bsls_assert.h>
#include <bsls_compilerfeatures.h>

#if BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES
// clang-format off
// Include version that can be compiled with C++03
// Generated on Wed Apr  2 14:54:56 2025
// Command line: sim_cpp11_features.pl bmqu_managedcallback.h

# define COMPILING_BMQU_MANAGEDCALLBACK_H
# include <bmqu_managedcallback_cpp03.h>
# undef COMPILING_BMQU_MANAGEDCALLBACK_H

// clang-format on
#else

namespace BloombergLP {
namespace bmqu {

// ===============
// ManagedCallback
// ===============

class ManagedCallback BSLS_KEYWORD_FINAL {
    /// The class useful for in-place construction and passing of functors
    /// between different actors.
  private:
    // DATA
    /// Reusable buffer holding the stored callback.
    bsl::vector<char> d_callbackBuffer;

    /// The flag indicating if `d_callbackBuffer` is empty now.
    bool d_empty;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ManagedCallback, bslma::UsesBslmaAllocator)

    // PUBLIC TYPES
    /// Signature of a `void` functor method.
    typedef bsl::function<void(void)> VoidFunctor;

    // ===============
    // CallbackFunctor
    // ===============
    /// The interface for all callback functors passed to ManagedCallback.
    struct CallbackFunctor {
        // CREATORS
        virtual ~CallbackFunctor();

        // ACCESSORS
        virtual void operator()() const = 0;
    };

    // CREATORS
    /// Create a ManagedCallback object using the optionally specified
    /// `allocator`.
    explicit ManagedCallback(bslma::Allocator* allocator = 0);

    /// Destroy this object.
    ~ManagedCallback();

    // MANIPULATORS
    /// Reset the state of this object.  If this object stores a callback,
    /// call a destructor for it and set this object empty.
    void reset();

#if !BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES  // $var-args=9
    /// Construct a callback object of the specified CALLBACK_TYPE in this
    /// objects' reusable buffer, with the specified `args` passed to its
    /// constructor.  The buffer must be empty before construction, it is
    /// the user's responsibility to call `reset()` to destroy the callback.
    template <class CALLBACK_TYPE, class... ARGS>
    void createInplace(ARGS&&... args);
#endif

    /// Store the specified `callback` in this object.
    /// Note: these setters are slow because they performs function copy, going
    ///       against the basic idea of ManagedCallback.  They exist solely for
    ///       the compatibility with other code where this copy is acceptable.
    ///       In performance-critical paths, `createInplace` should always be
    ///       used together with reusable ManagedCallback object(s).
    void set(const VoidFunctor& callback);
    void set(bslmf::MovableRef<VoidFunctor> callback);

    // ACCESSORS
    /// Is this object empty or not.
    bool empty() const;

    /// Call the stored callback.
    /// The object must be `!empty()` before calling `operator()`.
    void operator()() const;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are removed because we cannot
    /// guarantee that the internal buffer copy is safe.
    /// TODO: use is_trivially_copyable trait to ensure this on a standard
    ///       upgrade.  Alternatively keep copy operators and store the buffer
    ///       under a shared pointer.
    ManagedCallback(const ManagedCallback&) BSLS_KEYWORD_DELETED;
    ManagedCallback& operator=(const ManagedCallback&) BSLS_KEYWORD_DELETED;

    // PRIVATE MANIPULATORS

    /// Book and return the allocated memory to store the specified
    /// `CALLBACK_TYPE` that has to be inherited from `CallbackFunctor`.
    /// Note that it's the user's responsibility to construct a functor object
    /// at the provided address.
    /// The object must be `empty()` before calling `place()`.
    template <class CALLBACK_TYPE>
    char* place();
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------
// ManagedCallback
// ---------------

inline ManagedCallback::ManagedCallback(bslma::Allocator* allocator)
: d_callbackBuffer(allocator)
, d_empty(true)
{
    // NOTHING
}

inline ManagedCallback::~ManagedCallback()
{
    reset();
}

inline void ManagedCallback::reset()
{
    if (!d_empty) {
        // Not necessary to resize the vector or memset its elements to 0,
        // we just call the virtual destructor, and `d_empty` flag
        // prevents us from calling outdated callback.
        reinterpret_cast<CallbackFunctor*>(d_callbackBuffer.data())
            ->~CallbackFunctor();
        d_empty = true;
    }
}

template <class CALLBACK_TYPE>
inline char* ManagedCallback::place()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_empty);

    /// Ensure that CALLBACK_TYPE inherits from CallbackFunctor interface.
#if __cplusplus >= 201103L
    static_assert(
        bslmf::IsAccessibleBaseOf<CallbackFunctor, CALLBACK_TYPE>::value);
#else
    typedef bslmf::IsAccessibleBaseOf<CallbackFunctor, CALLBACK_TYPE> IsBase;
    BSLS_ASSERT_SAFE(IsBase::value);
#endif

    d_callbackBuffer.resize(
        bsls::AlignmentUtil::roundUpToMaximalAlignment(sizeof(CALLBACK_TYPE)));
    d_empty = false;
    return d_callbackBuffer.data();
}

inline bool ManagedCallback::empty() const
{
    return d_empty;
}

inline void ManagedCallback::operator()() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_empty);

    (*reinterpret_cast<const CallbackFunctor*>(d_callbackBuffer.data()))();
}

#if !BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES  // $var-args=9
template <class CALLBACK_TYPE, class... ARGS>
inline void ManagedCallback::createInplace(ARGS&&... args)
{
    // Preconditions for placement are checked in `place()`.
    // Destructor for the built object will be called in `reset()`.
    new (place<CALLBACK_TYPE>()) CALLBACK_TYPE(bsl::forward<ARGS>(args)...);
}
#endif

}  // close package namespace
}  // close enterprise namespace

#endif  // End C++11 code

#endif
