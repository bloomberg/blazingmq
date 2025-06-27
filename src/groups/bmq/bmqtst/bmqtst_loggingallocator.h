// Copyright 2014-2025 Bloomberg Finance L.P.
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

// bmqtst_loggingallocator.h                                          -*-C++-*-
#ifndef INCLUDED_BMQTST_LOGGINGALLOCATOR
#define INCLUDED_BMQTST_LOGGINGALLOCATOR

//@PURPOSE: Provide a 'bslma::Allocator' caching allocation stack traces.
//
//@CLASSES:
//  bmqtst::LoggingAllocator : allocator adaptor.
//
//@DESCRIPTION: This component defines a mechanism, 'bmqma::CountingAllocator'
// which implements the 'bslma::Allocator' protocol and which reports its usage
// to a 'bmqst::StatContext'.  The 'bmqma::CountingAllocator' keeps track of
// how many bytes were allocated and how many allocations were made.
//
// The 'bmqma::CountingAllocator' can report memory usage in an hierarchical
// manner if the 'allocator' provided in the constructor is actually a
// 'bmqma::CountingAllocator' (as determined by 'dynamic_cast').  If so, the
// new allocator adds itself as a sub-table of the provided allocator's
// context.  If the allocator provided at construction is not a
// 'bmqma::CountingAllocator' or doesn't hold a 'bmqst::StatContext' the calls
// to 'allocate 'and 'deallocate' are simply passed through with minimal
// overhead.
//
// See 'bmqma_countingallocatorstore' for how to collect allocation statistics
// when necessary while incurring no runtime overhead otherwise.
//
/// Allocation Limit
///----------------
// The 'CountingAllocator', when created with a StatContext, or as a child of a
// 'CountingAllocator' initialized with a StatContext keeps track of the total
// bytes currently allocated by it, and any of its derived allocators (aka
// children).  This allows to set an 'allocationLimit', triggering a
// user-provided callback the first time it is breached (useful to dump
// allocations and eventually abort the process in order to protect the host).
// Note that this allocation tracking has a few implications:
//: o since now every allocation and deallocation has to notify its parenting
//:   hierarchy, for proper allocation accounting, this will have a tiny
//:   performance impact; which should be minimal as the depth of nested
//:   CountingAllocator is usually expected to be very small (in the order of
//:   just a couple levels); and also most memory usually is pooled.
//: o this allocation book-keeping is akin to a duplicate of what the
//:   associated StatContext already tracks, but is needed because StatContext
//:   is not thread-safe when it comes to computing the value.
//: o each CountingAllocator now has a slightly bigger memory footprint, which
//:   should be fine as we usually only instantiate a small handful of such
//:   objects

// BDE
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_istriviallycopyable.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_deprecatefeature.h>
#include <bsls_keyword.h>

namespace BloombergLP {

namespace bmqtst {

// ======================
// class LoggingAllocator
// ======================

/// An allocator caching allocation stack traces.
class LoggingAllocator BSLS_KEYWORD_FINAL : public bslma::Allocator {
  public:
    // PUBLIC TYPES

    /// Alias for a signed integral type capable of representing the number
    /// of bytes in this platform's virtual address space.
    typedef bsls::Types::size_type size_type;

  private:
    // DATA
    /// Base allocator to use.
    bslma::Allocator* d_allocator_p;

  private:
    // NOT IMPLEMENTED
    LoggingAllocator(const LoggingAllocator&) BSLS_KEYWORD_DELETED;
    LoggingAllocator& operator=(const LoggingAllocator&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// @brief Construct a logging allocator.
    /// @param allocator The base allocator to use.  Use the default allocator
    ///                  if arg is NULL.
    explicit LoggingAllocator(bslma::Allocator* allocator = 0);

    /// Destroy this object.
    virtual ~LoggingAllocator() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    //  (virtual bslma::Allocator)

    /// @brief Return a newly allocated block of memory.  If this allocator
    ///        cannot return the requested number of bytes, then it will
    ///        throw a `bsl::bad_alloc` exception in an exception-enabled
    ///        build, or else will abort the program in a non-exception build.
    ///        The behavior is undefined unless `0 <= size`.  Note that the
    ///        alignment of the address returned conforms to the platform
    ///        requirement for any object of the specified `size`.
    /// @param size The minimum size of the newly allocated block (in bytes).
    ///             If `size` is 0, a null pointer is returned with no other
    ///             effect.
    /// @return The memory address of the allocated block or a null pointer.
    virtual void* allocate(size_type size) BSLS_KEYWORD_OVERRIDE;

    /// @brief Return the memory block at the specified `address` back to this
    ///        allocator.   The behavior is undefined unless `address` was
    ///        allocated using this allocator object and has not already been
    ///        deallocated.
    /// @param address The address of the memory block to deallocate.  If this
    ///        argument is a null pointer, the function doesn't have an effect.
    virtual void deallocate(void* address) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
