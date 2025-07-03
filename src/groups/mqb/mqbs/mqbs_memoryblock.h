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

// mqbs_memoryblock.h                                                 -*-C++-*-
#ifndef INCLUDED_MQBS_MEMORYBLOCK
#define INCLUDED_MQBS_MEMORYBLOCK

//@PURPOSE: Provide a VST to represent a contiguous block of memory.
//
//@CLASSES:
//  mqbs::MemoryBlock: VST to represent a contiguous block of memory.
//
//@DESCRIPTION: 'mqbs::MemoryBlock' provides a value-semantic type to represent
// a contiguous block of memory.

// MQB

// BDE
#include <bsl_type_traits.h>
#include <bslmf_isbitwiseequalitycomparable.h>
#include <bslmf_istriviallycopyable.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbs {

// =================
// class MemoryBlock
// =================

/// This component provides a VST to represent a contiguous block of memory.
class MemoryBlock {
  private:
    // DATA
    char* d_base_p;  // Pointer to start of the contiguous
                     // memory

    bsls::Types::Uint64 d_size;  // Size of the block of memory

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MemoryBlock,
                                   bslmf::IsBitwiseEqualityComparable)
    BSLMF_NESTED_TRAIT_DECLARATION(MemoryBlock, bsl::is_trivially_copyable)

    // CREATORS

    /// Create an empty instance.
    MemoryBlock();

    /// Create a memory block of specified `size` beginning at specified
    /// `base`.
    MemoryBlock(char* base, bsls::Types::Uint64 size);

    // MANIPULATORS

    /// Clear this instance.
    void clear();

    /// Reset this instance to pointed to a memory block of specified `size`
    /// beginning at specified `base`.
    void reset(char* base, bsls::Types::Uint64 size);

    MemoryBlock& setBase(char* value);

    /// Set the corresponding field to the specified `value`, and return a
    /// reference offering modifiable access to this instance.
    MemoryBlock& setSize(bsls::Types::Uint64 value);

    // ACCESSORS
    char* base() const;

    /// Return the corresponding field.
    bsls::Types::Uint64 size() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------
// class MemoryBlock
// -----------------

// CREATORS
inline MemoryBlock::MemoryBlock()
{
    clear();
}

inline MemoryBlock::MemoryBlock(char* base, bsls::Types::Uint64 size)
{
    reset(base, size);
}

// MANIPULATORS
inline void MemoryBlock::clear()
{
    d_base_p = 0;
    d_size   = 0;
}

inline void MemoryBlock::reset(char* base, bsls::Types::Uint64 size)
{
    setBase(base);
    setSize(size);
}

inline MemoryBlock& MemoryBlock::setBase(char* value)
{
    d_base_p = value;
    return *this;
}

inline MemoryBlock& MemoryBlock::setSize(bsls::Types::Uint64 value)
{
    d_size = value;
    return *this;
}

// ACCESSORS
inline char* MemoryBlock::base() const
{
    return d_base_p;
}

inline bsls::Types::Uint64 MemoryBlock::size() const
{
    return d_size;
}

}  // close package namespace
}  // close enterprise namespace

#endif
