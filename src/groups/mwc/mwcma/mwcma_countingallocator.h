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

// mwcma_countingallocator.h                                          -*-C++-*-
#ifndef INCLUDED_MWCMA_COUNTINGALLOCATOR
#define INCLUDED_MWCMA_COUNTINGALLOCATOR

//@PURPOSE: Provide a 'bslma::Allocator' reporting to a 'mwcst::StatContext'.
//
//@CLASSES:
//  mwcma::CountingAllocator : instrumented allocator adaptor.
//
//@SEE_ALSO: mwcma_countingallocatorstore
//           mwcma_countingallocatorutil
//           mwcst_statcontext
//
//@DESCRIPTION: This component defines a mechanism, 'mwcma::CountingAllocator'
// which implements the 'bslma::Allocator' protocol and which reports its usage
// to a 'mwcst::StatContext'.  The 'mwcma::CountingAllocator' keeps track of
// how many bytes were allocated and how many allocations were made.
//
// The 'mwcma::CountingAllocator' can report memory usage in an hierarchical
// manner if the 'allocator' provided in the constructor is actually a
// 'mwcma::CountingAllocator' (as determined by 'dynamic_cast').  If so, the
// new allocator adds itself as a sub-table of the provided allocator's
// context.  If the allocator provided at construction is not a
// 'mwcma::CountingAllocator' or doesn't hold a 'mwcst::StatContext' the calls
// to 'allocate 'and 'deallocate' are simply passed through with minimal
// overhead.
//
// See 'mwcma_countingallocatorstore' for how to collect allocation statistics
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

// MWC

#include <mwcst_statvalue.h>

// BDE
#include <ball_log.h>
#include <bsl_functional.h>
#include <bsl_map.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_istriviallycopyable.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_mutex.h>
#include <bsls_atomic.h>
#include <bsls_keyword.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARE
namespace mwcst {
class StatContext;
}
namespace mwcst {
class StatContextTableInfoProvider;
}
namespace mwcst {
class Table;
}
namespace mwcu {
class BasicTableInfoProvider;
}

namespace mwcma {

// =======================
// class CountingAllocator
// =======================

/// An allocator reporting its allocations and deallocations to a
/// `mwcst::StatContext`.
class CountingAllocator BSLS_KEYWORD_FINAL : public bslma::Allocator {
  public:
    // PUBLIC TYPES

    /// Alias for a signed integral type capable of representing the number
    /// of bytes in this platform's virtual address space.
    typedef bsls::Types::size_type size_type;

    typedef bsl::function<void()> AllocationLimitCallback;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MWCMA.COUNTINGALLOCATOR");

  private:
    // DATA
    bslma::ManagedPtr<mwcst::StatContext> d_statContext_mp;

    bslma::Allocator* d_allocator_p;

    CountingAllocator* d_parentCounting_p;
    // The parent allocator this object
    // is a child of, if it is of type
    // 'CountingAllocator', or null
    // otherwise.

    bsls::AtomicUint64 d_allocated;
    // Total number of bytes currently
    // allocated via this allocator and
    // any of its sub-child (only valid
    // if this allocator has an
    // associated 'statContext' since
    // otherwise, deallocation won't
    // be able to keep track of size).

    bsls::AtomicUint64 d_allocationLimit;
    // Maximum allowed allocations, in
    // bytes.  Defaulted to uintMax
    // (i.e. no limit).  Refer to
    // 'allocate' implementation for
    // why it's an atomic.

    AllocationLimitCallback d_allocationLimitCb;
    // The user-supplied callback to
    // invoke once the total allocation
    // has crossed the
    // 'allocationLimit'.  Note that
    // this callback will only ever be
    // invoked at most once, the first
    // time only the limit is breached.

  private:
    // NOT IMPLEMENTED
    CountingAllocator(const CountingAllocator&) BSLS_KEYWORD_DELETED;
    CountingAllocator&
    operator=(const CountingAllocator&) BSLS_KEYWORD_DELETED;

  public:
    // CLASS METHODS

    /// Configure the specified `tableInfoProvider` with a filter, a
    /// comparator, and columns to print the statistics generated by this
    /// `mwcma::CountingAllocator`.  @DEPRECATED: use the version taking a
    /// `mwcst::Table` and `mwcu::BasicTableInfoProvider` instead.
    static void configureStatContextTableInfoProvider(
        mwcst::StatContextTableInfoProvider* tableInfoProvider);

    /// Configure the specified `table` with a default set of columns for a
    /// stat table for a stat context created by a `CountingAllocator` using
    /// the specified `startSnapshot` as the most recent snapshot and
    /// `endSnapshot` as the end snapshot for all interval stats, and if the
    /// optionally specified `basicTableInfoProvider` is provided, bind it
    /// to the `table` and configure it to display this table in a default
    /// format.
    static void configureStatContextTableInfoProvider(
        mwcst::Table*                             table,
        mwcu::BasicTableInfoProvider*             basicTableInfoProvider,
        const mwcst::StatValue::SnapshotLocation& startSnapshot,
        const mwcst::StatValue::SnapshotLocation& endSnapshot);

  private:
    // PRIVATE MANIPULATORS

    /// Method invoked by any child of this object notifying it of a change
    /// in bytes allocated, in the specified `deltaValue` (positive value
    /// implying an allocation, while negative value represent a
    /// deallocation).
    void onAllocationChange(bsls::Types::Int64 deltaValue);

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
                      mwcst::StatContext*      parentStatContext,
                      bslma::Allocator*        allocator = 0);

    /// Destroy this object.
    virtual ~CountingAllocator() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Register the specified `callback` to be invoked when the total bytes
    /// allocated by this object and all of it's children combined is beyond
    /// the specified `limit`.  Note that this method is not thread-safe and
    /// should be called on the allocator prior to its usage.
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
    //   (specific to mwcma::CountingAllocator)

    /// Return the stat context associated with this allocator, if any.
    const mwcst::StatContext* context() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------
// class CountingAllocator
// -----------------------

// ACCESSORS
//   (specific to mwcma::CountingAllocator)
inline const mwcst::StatContext* CountingAllocator::context() const
{
    return d_statContext_mp.ptr();
}

}  // close package namespace
}  // close enterprise namespace

#endif
