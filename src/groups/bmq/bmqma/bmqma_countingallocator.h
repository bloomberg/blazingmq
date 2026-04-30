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

#ifndef INCLUDED_BMQMA_COUNTINGALLOCATOR
#define INCLUDED_BMQMA_COUNTINGALLOCATOR

//@PURPOSE: Provide a 'bslma::Allocator' reporting to a 'bmqst::StatContext'.
//
//@CLASSES:
//  bmqma::CountingAllocator : instrumented allocator adaptor.
//
//@SEE_ALSO: bmqma_countingallocatorstore
//           bmqma_countingallocatorutil
//           bmqst_statcontext
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
// bytes currently allocated across the entire allocator tree.  This allows to
// set an 'allocationLimit', triggering a user-provided callback the first time
// it is breached (useful to dump allocations and eventually abort the process
// in order to protect the host).
//
// All 'CountingAllocator' instances in a tree share a single
// 'AllocationLimitChecker' (owned by the root and referenced by children via
// 'bsl::shared_ptr').  Each allocation or deallocation updates the shared
// checker's atomic counter.
//
// Note that this allocation tracking has a few implications:
//: o this allocation book-keeping is akin to a duplicate of what the
//:   associated StatContext already tracks, but is needed because StatContext
//:   is not thread-safe when it comes to computing the value.
//: o the allocation limit applies to the entire tree; it should be configured
//:   on the root 'CountingAllocator' via 'setAllocationLimit'.

#include <bmqst_statvalue.h>

// BDE
#include <ball_log.h>
#include <bsl_functional.h>
#include <bsl_map.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_istriviallycopyable.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_mutex.h>
#include <bsls_atomic.h>
#include <bsls_deprecatefeature.h>
#include <bsls_keyword.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARE
namespace bmqst {
class StatContext;
}
namespace bmqst {
class StatContextTableInfoProvider;
}
namespace bmqst {
class Table;
}
namespace bmqst {
class BasicTableInfoProvider;
}

namespace bmqma {

// =======================
// class CountingAllocator
// =======================

/// An allocator reporting its allocations and deallocations to a
/// `bmqst::StatContext`.
class CountingAllocator BSLS_KEYWORD_FINAL : public bslma::Allocator {
  public:
    // PUBLIC TYPES

    /// Alias for a signed integral type capable of representing the number
    /// of bytes in this platform's virtual address space.
    typedef bsls::Types::size_type size_type;

    typedef bsl::function<void()> AllocationLimitCallback;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQMA.COUNTINGALLOCATOR");

  private:
    // TYPES

    /// @brief Tracks total bytes allocated and fires a one-shot callback
    /// when a configured limit is breached.
    class AllocationLimitChecker {
      private:
        // DATA

        /// Total number of bytes currently allocated.
        bsls::AtomicUint64 d_allocated;

        /// Maximum allowed allocations, in bytes.  Defaulted to
        /// `Uint64::max` (i.e. no limit).
        bsls::Types::Uint64 d_allocationLimit;

        /// Set to `true` the first time `d_allocationLimit` is breached,
        /// ensuring the callback is invoked at most once.
        bsls::AtomicBool d_alarmTriggered;

        /// User-supplied callback to invoke once the total allocation has
        /// crossed the `d_allocationLimit`.
        AllocationLimitCallback d_allocationLimitCb;

      public:
        // CREATORS

        /// Create an `AllocationLimitChecker` with zero bytes allocated,
        /// no limit, and no callback.
        AllocationLimitChecker()
        : d_allocated(0)
        , d_allocationLimit(bsl::numeric_limits<bsls::Types::Uint64>::max())
        , d_alarmTriggered(false)
        , d_allocationLimitCb()
        {
            // NOTHING
        }

        // MANIPULATORS

        /// Register the specified `callback` to be invoked when the total
        /// bytes allocated is beyond the specified `limit`.  Note that
        /// this method is not thread-safe and should be called prior to
        /// any allocations.
        void setLimit(bsls::Types::Uint64            limit,
                      const AllocationLimitCallback& callback);

        /// Update the tracked allocation total by the specified
        /// `deltaValue` (positive for allocation, negative for
        /// deallocation) and fire the limit callback if the limit is
        /// breached for the first time.
        void update(bsls::Types::Int64 deltaValue);
    };

    // DATA
    bslma::Allocator* d_allocator_p;

    bslma::ManagedPtr<bmqst::StatContext> d_statContext_mp;

    bsl::shared_ptr<AllocationLimitChecker> d_limitChecker_sp;

  private:
    // NOT IMPLEMENTED
    CountingAllocator(const CountingAllocator&) BSLS_KEYWORD_DELETED;
    CountingAllocator&
    operator=(const CountingAllocator&) BSLS_KEYWORD_DELETED;

  public:
    // CLASS METHODS

    /// Configure the specified `tableInfoProvider` with a filter, a
    /// comparator, and columns to print the statistics generated by this
    /// `bmqma::CountingAllocator`.
    BSLS_DEPRECATE_FEATURE("bmq",
                           "configureStatContextTableInfoProvider",
                           "Use the alternate overload instead.")
    static void configureStatContextTableInfoProvider(
        bmqst::StatContextTableInfoProvider* tableInfoProvider);

    /// Configure the specified `table` with a default set of columns for a
    /// stat table for a stat context created by a `CountingAllocator` using
    /// the specified `startSnapshot` as the most recent snapshot and
    /// `endSnapshot` as the end snapshot for all interval stats, and if the
    /// optionally specified `basicTableInfoProvider` is provided, bind it
    /// to the `table` and configure it to display this table in a default
    /// format.
    static void configureStatContextTableInfoProvider(
        bmqst::Table*                             table,
        bmqst::BasicTableInfoProvider*            basicTableInfoProvider,
        const bmqst::StatValue::SnapshotLocation& startSnapshot,
        const bmqst::StatValue::SnapshotLocation& endSnapshot);

  public:
    // CREATORS

    /// Create a counting allocator with the specified `name` having an
    /// initial byte count of 0.  Optionally specify an `allocator` used to
    /// supply memory.  If `allocator` is 0, the currently installed default
    /// allocator is used.  If `allocator` is itself a counting allocator,
    /// the stat context is created as a child of the stat context of
    /// `allocator`; otherwise, no stat context is created.
    CountingAllocator(const bslstl::StringRef& name,
                      bslma::Allocator*        allocator = 0);

    /// Create a counting allocator with the specified `name` and
    /// `parentStatContext` having an initial byte count of 0.  Optionally
    /// specify an `allocator` used to supply memory.  If `allocator` is 0,
    /// the currently installed default allocator is used.  If `context` is
    /// not a null pointer, the stat context is created as a child of
    /// `context`; otherwise no stat context is created.
    CountingAllocator(const bslstl::StringRef& name,
                      bmqst::StatContext*      parentStatContext,
                      bslma::Allocator*        allocator = 0);

    /// Destroy this object.
    virtual ~CountingAllocator() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Register the specified `callback` to be invoked when the total bytes
    /// allocated across the allocator hierarchy is beyond the specified
    /// `limit`.  Note that this method is not thread-safe and should be
    /// called on the root allocator prior to any allocations.
    void setAllocationLimit(bsls::Types::Uint64            limit,
                            const AllocationLimitCallback& callback);

    //  (virtual bslma::Allocator)

    /// Return a newly allocated block of memory of (at least) the specified
    /// positive `size` (in bytes) and report the allocation size to the
    /// parent StatContext, if it was provided at construction.  If `size`
    /// is 0, a null pointer is returned with no other effect.  If this
    /// allocator cannot return the requested number of bytes, then it will
    /// throw a `bsl::bad_alloc` exception in an exception-enabled build, or
    /// else will abort the program in a non-exception build.  The behavior
    /// is undefined unless `0 <= size`.  Note that the alignment of the
    /// address returned conforms to the platform requirement for any object
    /// of the specified `size`.
    virtual void* allocate(size_type size) BSLS_KEYWORD_OVERRIDE;

    /// Return the memory block at the specified `address` back to this
    /// allocator and report the deallocation size to the parent
    /// StatContext, if it was provided at construction.  If `address` is 0,
    /// this function has no effect.  The behavior is undefined unless
    /// `address` was allocated using this allocator object and has not
    /// already been deallocated.
    virtual void deallocate(void* address) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (specific to bmqma::CountingAllocator)

    /// Return the stat context associated with this allocator, if any.
    const bmqst::StatContext* context() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------
// class CountingAllocator
// -----------------------

// ACCESSORS
//   (specific to bmqma::CountingAllocator)
inline const bmqst::StatContext* CountingAllocator::context() const
{
    return d_statContext_mp.ptr();
}

}  // close package namespace
}  // close enterprise namespace

#endif
