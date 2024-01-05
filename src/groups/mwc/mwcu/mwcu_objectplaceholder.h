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

// mwcu_objectplaceholder.h                                           -*-C++-*-
#ifndef INCLUDED_MWCU_OBJECTPLACEHOLDER
#define INCLUDED_MWCU_OBJECTPLACEHOLDER

//@PURPOSE: Provides a placeholder for any object.
//
//@CLASSES:
//  mwcu::ObjectPlaceHolder: a placeholder for any object.
//
//@DESCRIPTION:
// This component provides a mechanism, 'mwcu::ObjectPlaceHolder', that may
// hold an object of any type, either in the internal (on-stack), or in an
// an external (dynamically allocated) buffer, depending on whether or not the
// object fits in the fixed-size internal buffer.
//
// The main purpose of this component is to facilitate the implementation of
// type-erasure with small buffer optimization.  For an example of how this
// component is intended to be used, see 'mwcex_job'.

// MWC

// BDE
#include <bsl_utility.h>  // bsl::forward
#include <bslma_allocator.h>
#include <bslma_constructionutil.h>
#include <bslma_destructionutil.h>
#include <bsls_alignedbuffer.h>
#include <bsls_assert.h>
#include <bsls_compilerfeatures.h>
#include <bsls_keyword.h>
#include <bsls_performancehint.h>

// clang-format off

#if BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES
// Include version that can be compiled with C++03
// Generated on Fri Jan  5 17:19:16 2024
// Command line: sim_cpp11_features.pl mwcu_objectplaceholder.h
# define COMPILING_MWCU_OBJECTPLACEHOLDER_H
# include <mwcu_objectplaceholder_cpp03.h>
# undef COMPILING_MWCU_OBJECTPLACEHOLDER_H
#else

// clang-format on

namespace BloombergLP {
namespace mwcu {

// ===================================
// class ObjectPlaceHolder_ObjectGuard
// ===================================

/// A guard to deallocate an object in case of exception.
template <class PLACEHOLDER>
class ObjectPlaceHolder_ObjectGuard {
  private:
    // PRIVATE DATA
    PLACEHOLDER* d_placeholder_p;

  public:
    // CREATORS

    /// Create a `ObjectPlaceHolder_ObjectGuard` object initialized with
    /// the specified `placeholder`.
    explicit ObjectPlaceHolder_ObjectGuard(PLACEHOLDER* placeholder)
        BSLS_KEYWORD_NOEXCEPT;

    /// Destroy this object.  Deallocate the object unless the guard was
    /// released.
    ~ObjectPlaceHolder_ObjectGuard();

  public:
    // MANIPULATORS

    /// Release this guard.
    void release() BSLS_KEYWORD_NOEXCEPT;
};

// =======================
// class ObjectPlaceHolder
// =======================

/// A placeholder for any object with a local buffer of (at least) the
/// specified `SIZE`.
template <size_t SIZE>
class ObjectPlaceHolder {
  private:
    // PRIVATE TYPES
    enum State {
        // State of the placeholder.

        e_EMPTY = 0  // The placeholder is empty.
        ,
        e_FULL_INT = 1  // The placeholder contains an internal object.
        ,
        e_FULL_EXT = 2  // The placeholder contains an external object.
    };

    /// An aligned buffer large enough to hold two pointers - the allocator
    /// pointer and the object pointer.
    typedef bsls::AlignedBuffer<(
        SIZE > (sizeof(void*) * 2) ? SIZE : (sizeof(void*) * 2))>
        Buffer;

    /// A guard to deallocate an object in case of exception.
    typedef ObjectPlaceHolder_ObjectGuard<ObjectPlaceHolder<SIZE> >
        ObjectGuard;

    // FRIENDS
    friend class ObjectPlaceHolder_ObjectGuard<ObjectPlaceHolder<SIZE> >;

  private:
    // PRIVATE CLASS DATA

    // The effective buffer size.
    static const size_t k_BUFFER_SIZE = sizeof(Buffer);

  private:
    // PRIVATE DATA

    // Unless the placeholder is empty, contains either a pair of pointers
    // - the allocator pointer and the object pointer, or the object itself.
    Buffer d_buffer;

    // Placeholder state. Use `char` instead of `State` to reduce memory
    // footprint.
    char d_state;

  private:
    // PRIVATE MANIPULATORS

    /// Store the specified `allocator` used to allocate the contained
    /// external object into the local buffer.
    void storeAllocator(bslma::Allocator* allocator) BSLS_KEYWORD_NOEXCEPT;

    /// Save the specified `address` of the contained external object into
    /// the local buffer.
    void storeObjectAddress(void* address) BSLS_KEYWORD_NOEXCEPT;

    /// Return the address of a contiguous block of memory large enough to
    /// accommodate an object of the specified `TYPE`.  If the allocation
    /// request exceeds the local buffer capacity, use memory obtained
    /// from the specified `allocator`.  On exception, this function has no
    /// effect.  The behavior is undefined if memory was already allocated,
    /// and has not already been released.
    template <class TYPE>
    TYPE* allocateObject(bslma::Allocator* allocator);

    /// Release memory allocated during the last successful call to
    /// `allocateObject`.  The behavior is undefined if no memory was
    /// allocated, or if the memory has already been released.
    void deallocateObject() BSLS_KEYWORD_NOEXCEPT;

  private:
    // PRIVATE ACCESSORS

    /// Load the allocator used to allocate the contained object from the
    /// local buffer, and return it.
    bslma::Allocator* loadAllocator() const BSLS_KEYWORD_NOEXCEPT;

    /// Load the address of the contained object from the local buffer, and
    /// return it.
    void* loadObjectAddress() const BSLS_KEYWORD_NOEXCEPT;

  private:
    // NOT IMPLEMENTED
    ObjectPlaceHolder(const ObjectPlaceHolder&) BSLS_KEYWORD_DELETED;
    ObjectPlaceHolder&
    operator=(const ObjectPlaceHolder&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `ObjectPlaceHolder` object containing nothing.
    ObjectPlaceHolder() BSLS_KEYWORD_NOEXCEPT;

    /// Destroy this object.  The behavior is undefined unless the
    /// placeholder is empty.
    ~ObjectPlaceHolder();

  public:
    // MANIPULATORS
    // clang-format off
#if !BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES // $var-args=9
    // clang-format on

    /// Initialize this placeholder with an object of the specified `TYPE`
    /// constructed from the specified `args` arguments.  Specify an
    /// `allocator` used to supply memory in case the size of the local
    /// buffer is not large enough to accommodate the object.  If `TYPE` is
    /// allocator-aware, forward the supplied allocator to the object's
    /// constructor. On exception, this function has no effect.  The
    /// behavior is undefined unless the placeholder is empty.
    template <class TYPE, class... ARGS>
    void createObject(bslma::Allocator* allocator, ARGS&&... args);
#endif

    /// Destroy the object of the specified `TYPE` contained in this
    /// placeholder, leaving the placeholder empty.  The behavior is
    /// undefined unless the placeholder contains an object of the specified
    /// `TYPE`.
    ///
    /// Note that `TYPE` does not necessarily has to be the same as the
    /// original type specified on object creation, as long as `TYPE` is
    /// polymorphic and is the base class of the original type.
    template <class TYPE>
    void deleteObject() BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS
    template <class TYPE>
    TYPE* object() BSLS_KEYWORD_NOEXCEPT;

    /// Return a pointer to the contained object of the specified `TYPE`,
    /// or 0 if the placeholder is empty.  The behavior is undefined unless
    /// the placeholder is empty, or contains an object of the specified
    /// `TYPE`.
    ///
    /// Note that `TYPE` does not necessarily has to be the same as the
    /// original type specified on object creation, as long as `TYPE` is
    /// polymorphic and is the base class of the original type.
    template <class TYPE>
    const TYPE* object() const BSLS_KEYWORD_NOEXCEPT;

    void* objectAddress() BSLS_KEYWORD_NOEXCEPT;

    /// Return the address of the contained object, or 0 if the placeholder
    /// is empty.
    const void* objectAddress() const BSLS_KEYWORD_NOEXCEPT;
};

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// -----------------------------------
// class ObjectPlaceHolder_ObjectGuard
// ------------------------------------

// CREATORS
template <class PLACEHOLDER>
inline ObjectPlaceHolder_ObjectGuard<
    PLACEHOLDER>::ObjectPlaceHolder_ObjectGuard(PLACEHOLDER* placeholder)
    BSLS_KEYWORD_NOEXCEPT : d_placeholder_p(placeholder)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(placeholder);
}

template <class PLACEHOLDER>
inline ObjectPlaceHolder_ObjectGuard<
    PLACEHOLDER>::~ObjectPlaceHolder_ObjectGuard()
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_placeholder_p != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        d_placeholder_p->deallocateObject();
    }
}

// MANIPULATORS
template <class PLACEHOLDER>
inline void
ObjectPlaceHolder_ObjectGuard<PLACEHOLDER>::release() BSLS_KEYWORD_NOEXCEPT
{
    d_placeholder_p = 0;
}

// -----------------------
// class ObjectPlaceHolder
// -----------------------

// PRIVATE MANIPULATORS
template <size_t SIZE>
inline void ObjectPlaceHolder<SIZE>::storeAllocator(
    bslma::Allocator* allocator) BSLS_KEYWORD_NOEXCEPT
{
    // the buffer now contains a pointer to the allocator
    *reinterpret_cast<bslma::Allocator**>(d_buffer.buffer()) = allocator;
}

template <size_t SIZE>
inline void ObjectPlaceHolder<SIZE>::storeObjectAddress(void* address)
    BSLS_KEYWORD_NOEXCEPT
{
    // the buffer now contains a pointer to the object
    *reinterpret_cast<void**>(d_buffer.buffer() + sizeof(void*)) = address;
}

template <size_t SIZE>
template <class TYPE>
inline TYPE*
ObjectPlaceHolder<SIZE>::allocateObject(bslma::Allocator* allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(allocator);
    BSLS_ASSERT_SAFE(d_state == e_EMPTY);

    void* objectAddress;
    if (sizeof(TYPE) <= k_BUFFER_SIZE) {
        // The local buffer is large enough to satisfy the request.  Return a
        // pointer to the local buffer.

        // "allocate" memory
        objectAddress = d_buffer.buffer();

        // the placeholder now contains an internal object
        d_state = e_FULL_INT;
    }
    else {
        // The local buffer is too small to satisfy the request.  Hand over the
        // request to the allocator.

        // allocate memory
        objectAddress = allocator->allocate(sizeof(TYPE));

        // store used allocator and obtained object address
        storeAllocator(allocator);
        storeObjectAddress(objectAddress);

        // the placeholder now contains an external object
        d_state = e_FULL_EXT;
    }

    return reinterpret_cast<TYPE*>(objectAddress);
}

template <size_t SIZE>
inline void ObjectPlaceHolder<SIZE>::deallocateObject() BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state != e_EMPTY);

    if (d_state == e_FULL_INT) {
        // Released memory belongs to the local buffer.  Consider it freed.

        // NOTHING
    }
    else {
        // Released memory does not belong to the local buffer.  Hand over the
        // request to the allocator.
        loadAllocator()->deallocate(objectAddress());
    }

    // the placeholder is now empty
    d_state = e_EMPTY;
}

// PRIVATE ACCESSORS
template <size_t SIZE>
inline bslma::Allocator*
ObjectPlaceHolder<SIZE>::loadAllocator() const BSLS_KEYWORD_NOEXCEPT
{
    char* buffer = const_cast<char*>(d_buffer.buffer());
    return *reinterpret_cast<bslma::Allocator**>(buffer);
}

template <size_t SIZE>
inline void*
ObjectPlaceHolder<SIZE>::loadObjectAddress() const BSLS_KEYWORD_NOEXCEPT
{
    char* buffer = const_cast<char*>(d_buffer.buffer());
    return *reinterpret_cast<void**>(buffer + sizeof(void*));
}

// CREATORS
template <size_t SIZE>
inline ObjectPlaceHolder<SIZE>::ObjectPlaceHolder() BSLS_KEYWORD_NOEXCEPT
: d_state(e_EMPTY)
{
    // NOTHING
}

template <size_t SIZE>
inline ObjectPlaceHolder<SIZE>::~ObjectPlaceHolder()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state == e_EMPTY);
}

// MANIPULATORS
// clang-format off
#if !BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES // $var-args=9
// clang-format on

template <size_t SIZE>
template <class TYPE, class... ARGS>
inline void ObjectPlaceHolder<SIZE>::createObject(bslma::Allocator* allocator,
                                                  ARGS&&... args)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(allocator);
    BSLS_ASSERT_SAFE(d_state == e_EMPTY);

    // allocate object
    TYPE*       object = allocateObject<TYPE>(allocator);
    ObjectGuard objectGuard(this);

    // construct object
    bslma::ConstructionUtil::construct<TYPE>(object,
                                             allocator,
                                             bsl::forward<ARGS>(args)...);

    // success
    objectGuard.release();
}

#endif

template <size_t SIZE>
template <class TYPE>
inline void ObjectPlaceHolder<SIZE>::deleteObject() BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state != e_EMPTY);

    // destroy object
    bslma::DestructionUtil::destroy(object<TYPE>());

    // release memory
    deallocateObject();
}

// ACCESSORS
template <size_t SIZE>
template <class TYPE>
inline TYPE* ObjectPlaceHolder<SIZE>::object() BSLS_KEYWORD_NOEXCEPT
{
    return reinterpret_cast<TYPE*>(objectAddress());
}

template <size_t SIZE>
template <class TYPE>
inline const TYPE*
ObjectPlaceHolder<SIZE>::object() const BSLS_KEYWORD_NOEXCEPT
{
    return reinterpret_cast<const TYPE*>(objectAddress());
}

template <size_t SIZE>
inline void* ObjectPlaceHolder<SIZE>::objectAddress() BSLS_KEYWORD_NOEXCEPT
{
    if (d_state == e_EMPTY) {
        return 0;  // RETURN
    }

    // The object is either located internally or externally.  In the first
    // case, return the address of the local buffer, in the second case,
    // extract the address of the object from the local buffer.

    char* buffer = const_cast<char*>(d_buffer.buffer());

    return d_state == e_FULL_INT ? static_cast<void*>(buffer)
                                 : loadObjectAddress();
}

template <size_t SIZE>
inline const void*
ObjectPlaceHolder<SIZE>::objectAddress() const BSLS_KEYWORD_NOEXCEPT
{
    return const_cast<ObjectPlaceHolder*>(this)->objectAddress();
}

}  // close package namespace
}  // close enterprise namespace

// clang-format off

#endif // End C++11 code

// clang-format on

#endif
