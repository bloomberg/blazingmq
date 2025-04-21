// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbs_offsetptr.h                                                   -*-C++-*-
#ifndef INCLUDED_MQBS_OFFSETPTR
#define INCLUDED_MQBS_OFFSETPTR

//@PURPOSE: Provide pointer-like semantics at an offset in a memory block.
//
//@CLASSES:
//  mqbs::OffsetPtr: Pointer-like semantics at an offset in a memory block.
//
//@SEE ALSO: mqbs::MemoryBlock
//
//@DESCRIPTION: 'mqbs::OffsetPtr' provides pointer-like semantics at an offset
// in a block of memory.

// MQB
#include <mqbs_memoryblock.h>

// BDE
#include <bsl_cstddef.h>  // for bsl::size_t
#include <bsls_alignmentfromtype.h>
#include <bsls_alignmentutil.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbs {

// ===============
// class OffsetPtr
// ===============

/// This component provides pointer-like semantics at an offset in a memory
/// block.
template <class TYPE>
class OffsetPtr {
  private:
    // DATA
    TYPE* d_ptr_p;

  public:
    // CREATORS

    /// Create an offset pointer at the specified `offset` in the specified
    /// `block`.  The behavior is undefined unless `offset` is smaller than
    /// block's size.
    OffsetPtr(const MemoryBlock& block, bsl::size_t offset);

    // MANIPULATORS

    /// Reset this offset pointer to point to specified `offset` in the
    /// specified `block`.  The behavior is undefined unless `offset` is
    /// smaller than block's size.
    void reset(const MemoryBlock& block, bsl::size_t offset);

    // ACCESSORS

    /// Return the address providing modifiable access to the object
    /// referred to by this offset pointer.
    TYPE* operator->() const;

    /// Return a reference providing modifiable access to the object
    /// referred to by this offset pointer.
    TYPE& operator*() const;

    /// Return the address providing modifiable access to the object
    /// referred to by this offset pointer.
    TYPE* get() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------
// class OffsetPtr
// ---------------

// CREATORS
template <class TYPE>
inline OffsetPtr<TYPE>::OffsetPtr(const MemoryBlock& block, bsl::size_t offset)
{
    reset(block, offset);
}

// MANIPULATORS
template <class TYPE>
inline void OffsetPtr<TYPE>::reset(const MemoryBlock& block,
                                   bsl::size_t        offset)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(offset < block.size());
    BSLS_ASSERT_SAFE(bsls::AlignmentUtil::calculateAlignmentOffset(
                         block.base() + offset,
                         bsls::AlignmentFromType<TYPE>::VALUE) == 0);

    d_ptr_p = reinterpret_cast<TYPE*>(block.base() + offset);
}

// ACCESSORS
template <class TYPE>
inline TYPE* OffsetPtr<TYPE>::operator->() const
{
    return d_ptr_p;
}

template <class TYPE>
inline TYPE& OffsetPtr<TYPE>::operator*() const
{
    return *d_ptr_p;
}

template <class TYPE>
inline TYPE* OffsetPtr<TYPE>::get() const
{
    return d_ptr_p;
}

}  // close package namespace
}  // close enterprise namespace

#endif
