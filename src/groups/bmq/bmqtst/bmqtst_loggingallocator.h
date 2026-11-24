// Copyright 2025-2026 Bloomberg Finance L.P.
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

#ifndef INCLUDED_BMQTST_LOGGINGALLOCATOR
#define INCLUDED_BMQTST_LOGGINGALLOCATOR

//@PURPOSE: Provide an allocator printing the stack trace of each allocation.
//
//@CLASSES:
//  bmqtst::LoggingAllocator: allocator adaptor printing stack traces
//
//@DESCRIPTION: 'bmqtst::LoggingAllocator' implements the 'bslma::Allocator'
// protocol.  It forwards all calls to an underlying allocator and prints the
// stack trace of each allocation to 'stderr', except for a few known
// allocations made by BDE and BMQ utilities.  It is used to find the origin
// of unexpected default allocator usage in test drivers.

// BDE
#include <bslma_allocator.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace bmqtst {

// ======================
// class LoggingAllocator
// ======================

/// @brief Allocator printing the stack trace of each allocation.
class LoggingAllocator BSLS_KEYWORD_FINAL : public bslma::Allocator {
  private:
    // DATA
    bslma::Allocator* d_allocator_p;

  private:
    // NOT IMPLEMENTED
    LoggingAllocator(const LoggingAllocator&) BSLS_KEYWORD_DELETED;
    LoggingAllocator& operator=(const LoggingAllocator&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// @brief Create an allocator forwarding to the specified `allocator`.
    ///
    /// @param allocator The underlying allocator.
    explicit LoggingAllocator(bslma::Allocator* allocator);

    /// @brief Destroy this object.
    ~LoggingAllocator() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// @brief Print the stack trace of this call, unless it is a known
    /// allocation, and allocate from the underlying allocator.
    ///
    /// @param size The minimum size of the block in bytes.
    ///
    /// @return The address of the allocated block, or null if `size` is 0.
    void* allocate(size_type size) BSLS_KEYWORD_OVERRIDE;

    /// @brief Return the block at the specified `address` to the underlying
    /// allocator.
    ///
    /// @param address The address of the block, or null for no effect.
    void deallocate(void* address) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
