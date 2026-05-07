// Copyright 2017-2023 Bloomberg Finance L.P.
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

#ifndef INCLUDED_BMQMA_COUNTINGALLOCATORUTIL
#define INCLUDED_BMQMA_COUNTINGALLOCATORUTIL

//@PURPOSE: Provide a utility for installing 'bmqma::CountingAllocator'.
//
//@CLASSES:
//  bmqma::CountingAllocatorUtil : utility functions.
//
//@SEE_ALSO: bmqma_countingallocatorstore
//
//@DESCRIPTION: This component defines a utility,
// 'bmqma::CountingAllocatorUtil' which provides useful functions for using
// 'bmqma::CountingAllocator' objects in an application.
//
// The function 'bmqma::CountingAllocatorUtil::initGlobalAllocator' should be
// called in 'main' to install counting allocators.  Refer to the usage example
// in 'bmqma_countingallocatorstore'.

// BDE
#include <bsl_functional.h>
#include <bsl_iosfwd.h>
#include <bsl_string_view.h>

namespace BloombergLP {

// FORWARD DECLARE
namespace bmqst {
class StatContext;
}
namespace bmqst {
class StatContextConfiguration;
}

namespace bmqma {

// FORWARD DECLARE
class CountingAllocatorStore;

// ============================
// struct CountingAllocatorUtil
// ============================

/// Container for utility functions for working with counting allocators
/// (`bmqma::CountingAllocator`).
struct CountingAllocatorUtil {
    // CLASS METHODS

    /// @brief Set the global and default allocators to counting allocators.
    ///
    /// @param globalStatContextName  Name for the top-level stat context.
    /// @param topAllocatorName       Name for the top-level allocator
    ///                               sub-context.
    /// @param allocationLimit        Maximum bytes before the limit callback
    ///                               fires (0 means no limit).
    /// @param allocationLimitCb      Callback invoked when the allocation
    ///                               limit is exceeded.
    ///
    /// The default allocator will have name "Default Allocator", and the
    /// global allocator will have name "Global Allocator".  This function
    /// should be called once in `main`.  The behavior is undefined if this
    /// function is called more than once.
    static void
    initGlobalAllocators(bsl::string_view             globalStatContextName,
                         bsl::string_view             topAllocatorName,
                         bsls::Types::Uint64          allocationLimit,
                         const bsl::function<void()>& allocationLimitCb);

    /// Return the stat context created by `initGlobalAllocators`.  The
    /// behavior is undefined unless `initGlobalAllocators` has been called.
    static bmqst::StatContext* globalStatContext();

    /// Return the top-level allocator store created by
    /// `initGlobalAllocators`.  Any allocator retrieved from this store is
    /// guaranteed to remain valid until the end of the program.  The
    /// behavior is undefined unless `initGlobalAllocators` has been called.
    static bmqma::CountingAllocatorStore& topAllocatorStore();

    /// Print to the specified `stream` the allocator statistics captured by
    /// the specified `context`, which must correspond to a
    /// `CountingAllocator` configured StatContext.
    static void printAllocations(bsl::ostream&             stream,
                                 const bmqst::StatContext& context);
};

}  // close package namespace
}  // close enterprise namespace

#endif
