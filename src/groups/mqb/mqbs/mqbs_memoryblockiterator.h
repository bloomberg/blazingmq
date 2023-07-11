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

// mqbs_memoryblockiterator.h                                         -*-C++-*-
#ifndef INCLUDED_MQBS_MEMORYBLOCKITERATOR
#define INCLUDED_MQBS_MEMORYBLOCKITERATOR

//@PURPOSE: Provide a mechanism to iterate over a contiguous block of memory.
//
//@CLASSES:
//  mqbs::MemoryBlockIterator: Mechanism to iterate over a memory block.
//
//@SEE ALSO: mqbs::MemoryBlock
//
//@DESCRIPTION: 'mqbs::MemoryBlockIterator' provides a mechanism to iterate
// over a contiguous block of memory.  The direction of iteration
// (forward/backward) is specified at construction and cannot be changed until
// the instance is reset.

// MQB

#include <mqbs_memoryblock.h>

// BDE
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbs {

// =========================
// class MemoryBlockIterator
// =========================

/// This component provides a mechanism to iterate bidirectionally over a
/// contiguous block of memory.
class MemoryBlockIterator {
  private:
    // DATA
    const MemoryBlock* d_block_p;

    bool d_forward;

    bsls::Types::Uint64 d_startingPosition;

    bsls::Types::Uint64 d_currentPosition;

    bsls::Types::Uint64 d_remaining;

  public:
    // CREATORS

    /// Create an empty instance. `atEnd()` will return true.
    MemoryBlockIterator();

    /// Create an instance to iterate over the specified memory `block` at
    /// the specified `position` with `remaining` number of bytes left in
    /// iteration with the specified `forwardIter` flag indicating the
    /// direction of iteration.
    MemoryBlockIterator(const MemoryBlock*  block,
                        bsls::Types::Uint64 position,
                        bsls::Types::Uint64 remaining,
                        bool                forwardIter);

    // MANIPULATORS

    /// Clear this instance.
    void clear();

    /// Reset this instance to iterate over the specified memory `block` at
    /// the specified `position` with `remaining` number of bytes left in
    /// iteration with the specified `forwardIter` flag indicating the
    /// direction of iteration.
    void reset(const MemoryBlock*  block,
               bsls::Types::Uint64 position,
               bsls::Types::Uint64 remaining,
               bool                forwardIter);

    /// Advance by the specified `length` bytes further into this block.
    /// Return the value of `!atEnd` after advancing.  The behavior is
    /// undefined unless `atEnd` returns false before this method is
    /// invoked.
    bool advance(bsls::Types::Uint64 length);

    /// Flips the direction of the iterator.  If the iterator was a
    /// ForwardIterator, it becomes a ReverseIterator and if the iterator
    /// was a ReverseIterator, it becomes a ForwardIterator.
    void flipDirection();

    // ACCESSORS

    /// Return true if this instance is configured to iteration in forward
    /// direction, false otherwise.
    bool isForwardIterator() const;

    /// Return true if this instance is at the end of memory block.
    bool atEnd() const;

    /// Return the memory block over which this instance is iterating.
    const MemoryBlock* block() const;

    /// Return the current position of the iterator.
    bsls::Types::Uint64 position() const;

    /// Return the remaining number of bytes of the iterator.
    bsls::Types::Uint64 remaining() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------------
// class MemoryBlockIterator
// -------------------------

// CREATORS
inline MemoryBlockIterator::MemoryBlockIterator()
{
    clear();
}

inline MemoryBlockIterator::MemoryBlockIterator(const MemoryBlock*  block,
                                                bsls::Types::Uint64 position,
                                                bsls::Types::Uint64 remaining,
                                                bool forwardIter)
{
    reset(block, position, remaining, forwardIter);
}

// MANIPULATORS
inline void MemoryBlockIterator::clear()
{
    d_block_p          = 0;
    d_forward          = true;
    d_currentPosition  = 0;
    d_startingPosition = 0;
    d_remaining        = 0;
}

inline void MemoryBlockIterator::reset(const MemoryBlock*  block,
                                       bsls::Types::Uint64 position,
                                       bsls::Types::Uint64 remaining,
                                       bool                forwardIter)
{
    d_block_p          = block;
    d_forward          = forwardIter;
    d_currentPosition  = position;
    d_startingPosition = position;
    d_remaining        = remaining;
    BSLS_ASSERT_SAFE(d_block_p);
    BSLS_ASSERT_SAFE(d_remaining <= d_block_p->size());
    BSLS_ASSERT_SAFE(d_startingPosition <= d_block_p->size());
}

inline bool MemoryBlockIterator::advance(bsls::Types::Uint64 length)
{
    if (length > d_remaining) {
        d_remaining = 0;
        return false;  // RETURN
    }

    d_remaining -= length;
    if (d_forward) {
        d_currentPosition += length;
    }
    else {
        d_currentPosition -= length;
    }

    return true;
}

inline void MemoryBlockIterator::flipDirection()
{
    bsls::Types::Uint64 length  = 0;
    const bool          reverse = isForwardIterator();

    if (reverse) {
        // If it was a forward iterator, we're flipping to reverse, so we're
        // heading towards the beginning of the memory block.
        length             = d_currentPosition - d_startingPosition;
        d_startingPosition = d_currentPosition + d_remaining;
    }
    else {
        // Otherwise, we're flipping to forward, so we're heading towards the
        // end of the memory block.
        length             = d_startingPosition - d_currentPosition;
        d_startingPosition = d_currentPosition - d_remaining;
    }

    d_remaining = length;
    d_forward   = !reverse;
}

// ACCESSORS
inline bool MemoryBlockIterator::isForwardIterator() const
{
    return d_forward;
}

inline bool MemoryBlockIterator::atEnd() const
{
    return 0 == d_remaining;
}

inline const MemoryBlock* MemoryBlockIterator::block() const
{
    return d_block_p;
}

inline bsls::Types::Uint64 MemoryBlockIterator::position() const
{
    return d_currentPosition;
}

inline bsls::Types::Uint64 MemoryBlockIterator::remaining() const
{
    return d_remaining;
}

}  // close package namespace
}  // close enterprise namespace

#endif
