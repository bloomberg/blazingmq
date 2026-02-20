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
//@DESCRIPTION: This component defines a mechanism, 'bmqtst::LoggingAllocator'
// which implements the 'bslma::Allocator' protocol and which logs stack traces
// on any allocation.

// BDE
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bsls_atomic.h>
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
    /// @brief Base allocator to use.
    bslma::Allocator* d_allocator_p;

    /// @brief The flag indicaing that the allocator must fail instantly if any
    ///        undesired allocation happens.
    bool d_failFast;

    /// @brief The flag indicating that this allocator has failed.
    bsls::AtomicBool d_failed;

  private:
    // NOT IMPLEMENTED
    LoggingAllocator(const LoggingAllocator&) BSLS_KEYWORD_DELETED;
    LoggingAllocator& operator=(const LoggingAllocator&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// @brief Construct a logging allocator.
    /// @param failFast The flag indicating whether this allocator must raise
    ///                 an exception instantly on any undesired allocation.
    /// @param allocator The base allocator to use, or use the default
    ///                  allocator if this arg is null.
    explicit LoggingAllocator(bool              failFast  = false,
                              bslma::Allocator* allocator = 0);

    /// Destroy this object.
    ~LoggingAllocator() BSLS_KEYWORD_OVERRIDE;

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
    void* allocate(size_type size) BSLS_KEYWORD_OVERRIDE;

    /// @brief Return the memory block at the specified `address` back to this
    ///        allocator.   The behavior is undefined unless `address` was
    ///        allocated using this allocator object and has not already been
    ///        deallocated.
    /// @param address The address of the memory block to deallocate.  If this
    ///        argument is a null pointer, the function doesn't have an effect.
    void deallocate(void* address) BSLS_KEYWORD_OVERRIDE;

    /// @brief Check whether the undesired allocations were recorded and raise
    ///        an exception if any.
    void checkNoAllocations();
};

}  // close package namespace
}  // close enterprise namespace

#endif
