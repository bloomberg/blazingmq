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

// mwcma_countingallocatorutil.h                                      -*-C++-*-
#ifndef INCLUDED_MWCMA_COUNTINGALLOCATORUTIL
#define INCLUDED_MWCMA_COUNTINGALLOCATORUTIL

//@PURPOSE: Provide a utility for installing 'mwcma::CountingAllocator'.
//
//@CLASSES:
//  mwcma::CountingAllocatorUtil : utility functions.
//
//@SEE_ALSO: mwcma_countingallocatorstore
//
//@DESCRIPTION: This component defines a utility,
// 'mwcma::CountingAllocatorUtil' which provides useful functions for using
// 'mwcma::CountingAllocator' objects in an application.
//
// The function 'mwcma::CountingAllocatorUtil::initGlobalAllocator' should be
// called in 'main' to install counting allocators.  Refer to the usage example
// in 'mwcma_countingallocatorstore'.

// MWC

// BDE
#include <bsl_iosfwd.h>
#include <bsl_string.h>

namespace BloombergLP {

// FORWARD DECLARE
namespace mwcst {
class StatContext;
}
namespace mwcst {
class StatContextConfiguration;
}

namespace mwcma {

// FORWARD DECLARE
class CountingAllocatorStore;

// ============================
// struct CountingAllocatorUtil
// ============================

/// Container for utility functions for working with counting allocators
/// (`mwcma::CountingAllocator`).
struct CountingAllocatorUtil {
    // CLASS METHODS

    /// Set the global and default allocators to counting allocators with
    /// stats reported to a sub-context having the specified
    /// `topAllocatorName` under a top-level stat context with the specified
    /// `globalStatContextConfiguration` or with the specified
    /// `globalStatContextName` and default configuration.  The default
    /// allocator will have name "Default Allocator", and the global
    /// allocator will have name "Global Allocator".  This function should
    /// be called once in `main`.  The behavior is undefined if this
    /// function is called more than once.
    static void initGlobalAllocators(
        const mwcst::StatContextConfiguration& globalStatContextConfiguration,
        const bslstl::StringRef&               topAllocatorName);
    static void
    initGlobalAllocators(const bslstl::StringRef& globalStatContextName,
                         const bslstl::StringRef& topAllocatorName);

    /// Return the stat context created by `initGlobalAllocators`.  The
    /// behavior is undefined unless `initGlobalAllocators` has been called.
    static mwcst::StatContext* globalStatContext();

    /// Return the top-level allocator store created by
    /// `initGlobalAllocators`.  Any allocator retrieved from this store is
    /// guaranteed to remain valid until the end of the program.  The
    /// behavior is undefined unless `initGlobalAllocators` has been called.
    static mwcma::CountingAllocatorStore& topAllocatorStore();

    /// Print to the specified `stream` the allocator statistics captured by
    /// the specified `context`, which must correspond to a
    /// `CountingAllocator` configured StatContext.
    static void printAllocations(bsl::ostream&             stream,
                                 const mwcst::StatContext& context);
};

}  // close package namespace
}  // close enterprise namespace

#endif
