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

// bmqma_countingallocator.cpp                                        -*-C++-*-
#include <bmqma_countingallocator.h>

#include <bmqscm_version.h>

#include <bmqst_basictableinfoprovider.h>
#include <bmqst_statcontext.h>
#include <bmqst_statcontexttableinfoprovider.h>
#include <bmqst_statutil.h>
#include <bmqst_table.h>
#include <bmqst_tablerecords.h>

// BDE
#include <balst_stacktraceprintutil.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsla_annotations.h>
#include <bsls_alignmentutil.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqma {

namespace {

// CONSTANTS

/// Constant leveraged to determine if an attempt has been made to
/// deallocate unallocated or previously freed memory.
const unsigned int k_MAGIC = 0xabcdabcd;

// FUNCTIONS
bool statFilter(const bmqst::StatContext*     context,
                bmqst::StatContext::ValueType valueType,
                BSLA_UNUSED int               level)
{
    return (context->isDeleted() &&
            valueType == bmqst::StatContext::e_TOTAL_VALUE) ||
           (bmqst::StatUtil::value(context->value(valueType, 0), 0) > 0);
}

bool statFilter2(const bmqst::TableRecords::Record& rec)
{
    const bool isTotal = (rec.type() == bmqst::StatContext::e_TOTAL_VALUE);
    return (rec.context().isDeleted() && isTotal) ||
           (bmqst::StatUtil::increments(rec.context().value(rec.type(), 0),
                                        0) > 0);
}

bool statSort(const bmqst::StatContext* lhs, const bmqst::StatContext* rhs)
{
    const bmqst::StatValue& lhsTotalValue =
        lhs->value(bmqst::StatContext::e_TOTAL_VALUE, 0);
    const bmqst::StatValue& rhsTotalValue =
        rhs->value(bmqst::StatContext::e_TOTAL_VALUE, 0);

    return bmqst::StatUtil::value(lhsTotalValue, 0) >
           bmqst::StatUtil::value(rhsTotalValue, 0);
}

// STRUCTS

/// Header for an allocated block if allocations are being tracked
union Header {
    struct {
        CountingAllocator::size_type d_numAllocatedBytes;
        // Number of bytes allocated in this
        // allocation.

        unsigned int d_magic;
        // This is here to add some extra
        // checks.  Since we're union'ed with
        // 'MaxAlignedType' this doesn't add
        // anything to the header size.
    } d_data;

    bsls::AlignmentUtil::MaxAlignedType d_dummy;
    // For memory alignment,
};

}  // close unnamed namespace

// -----------------------
// class CountingAllocator
// -----------------------

// CLASS METHODS
void CountingAllocator::configureStatContextTableInfoProvider(
    bmqst::StatContextTableInfoProvider* tableInfoProvider)
{
    typedef bmqst::StatUtil SU;

    tableInfoProvider->setFilter(&statFilter);
    tableInfoProvider->setComparator(&statSort);

    tableInfoProvider->setColumnGroup("");
    tableInfoProvider->addDefaultIdColumn("");

    tableInfoProvider->setColumnGroup("");
    tableInfoProvider->addColumn("Bytes Allocated", 0, SU::value, 0);
    tableInfoProvider->addColumn("-delta-", 0, SU::valueDifference, 0, 1);
    tableInfoProvider->addColumn("Max Bytes Allocated", 0, SU::absoluteMax);
    tableInfoProvider->addColumn("Allocations", 0, SU::increments, 0);
    tableInfoProvider->addColumn("-delta-", 0, SU::incrementsDifference, 0, 1);
    tableInfoProvider->addColumn("Deallocations", 0, SU::decrements, 0);
    tableInfoProvider->addColumn("-delta-", 0, SU::decrementsDifference, 0, 1);
}

void CountingAllocator::configureStatContextTableInfoProvider(
    bmqst::Table*                             table,
    bmqst::BasicTableInfoProvider*            basicTableInfoProvider,
    const bmqst::StatValue::SnapshotLocation& startSnapshot,
    const bmqst::StatValue::SnapshotLocation& endSnapshot)
{
    typedef bmqst::StatUtil SU;

    if (!table) {
        return;  // RETURN
    }

    const bmqst::StatValue::SnapshotLocation& cur = startSnapshot;
    const bmqst::StatValue::SnapshotLocation& end = endSnapshot;

    // Configure schema
    bmqst::TableSchema* schema = &table->schema();
    schema->addDefaultIdColumn("id");

    schema->addColumn("numAllocated", 0, SU::value, cur);
    schema->addColumn("numAllocatedDelta", 0, SU::valueDifference, cur, end);
    schema->addColumn("maxAllocated", 0, SU::absoluteMax);
    schema->addColumn("numAllocations", 0, SU::increments, cur);
    schema->addColumn("numAllocationsDelta",
                      0,
                      SU::incrementsDifference,
                      cur,
                      end);
    schema->addColumn("numDeallocations", 0, SU::decrements, cur);
    schema->addColumn("numDeallocationsDelta",
                      0,
                      SU::decrementsDifference,
                      cur,
                      end);

    // Configure records
    bmqst::TableRecords* records = &table->records();
    records->setFilter(&statFilter2);
    records->setSort(&statSort);

    if (!basicTableInfoProvider) {
        return;  // RETURN
    }

    // Configure basicTableInfoProvider
    basicTableInfoProvider->setTable(table);

    basicTableInfoProvider->addColumn("id", "").justifyLeft();
    basicTableInfoProvider->addColumn("numAllocated", "Bytes Allocated");
    basicTableInfoProvider->addColumn("numAllocatedDelta", "-delta-")
        .zeroString("");
    basicTableInfoProvider->addColumn("maxAllocated", "Max Bytes Allocated");
    basicTableInfoProvider->addColumn("numAllocations", "Allocations");
    basicTableInfoProvider->addColumn("numAllocationsDelta", "-delta-")
        .zeroString("");
    basicTableInfoProvider->addColumn("numDeallocations", "Deallocations");
    basicTableInfoProvider->addColumn("numDeallocationsDelta", "-delta-")
        .zeroString("");
}

// CREATORS

CountingAllocator::CountingAllocator(const bslstl::StringRef& name,
                                     bslma::Allocator*        allocator)
: d_statContext_mp()
, d_allocator_p(bslma::Default::allocator(allocator))
, d_parentCounting_p(0)
, d_allocated(0)
, d_allocationLimit(bsl::numeric_limits<bsls::Types::Uint64>::max())
// Disable allocation limit by default
{
    CountingAllocator* ca = dynamic_cast<CountingAllocator*>(d_allocator_p);
    if (ca) {
        d_allocator_p = ca->d_allocator_p;
        if (ca->d_statContext_mp) {
            d_statContext_mp = ca->d_statContext_mp->addSubcontext(
                bmqst::StatContextConfiguration(name, allocator));
            d_parentCounting_p = ca;
        }
    }
}

CountingAllocator::CountingAllocator(const bslstl::StringRef& name,
                                     bmqst::StatContext* parentStatContext,
                                     bslma::Allocator*   allocator)
: d_statContext_mp()
, d_allocator_p(bslma::Default::allocator(allocator))
, d_parentCounting_p(0)
, d_allocated(0)
, d_allocationLimit(bsl::numeric_limits<bsls::Types::Uint64>::max())
// Disable allocation limit by default
{
    CountingAllocator* ca = dynamic_cast<CountingAllocator*>(d_allocator_p);
    if (ca) {
        // The 'allocator' is a 'CountingAllocator'
        d_allocator_p      = ca->d_allocator_p;
        d_parentCounting_p = ca;
    }

    if (parentStatContext) {
        if (parentStatContext->hasDefaultHistorySize()) {
            d_statContext_mp = parentStatContext->addSubcontext(
                bmqst::StatContextConfiguration(name, allocator)
                    .isTable(true)
                    .value("Memory"));
        }
        else {
            d_statContext_mp = parentStatContext->addSubcontext(
                bmqst::StatContextConfiguration(name, allocator)
                    .isTable(true)
                    .value("Memory", 2));
        }
    }
}

CountingAllocator::~CountingAllocator()
{
    // NOTHING
}

// MANIPULATORS

void CountingAllocator::onAllocationChange(bsls::Types::Int64 deltaValue)
{
    const bsls::Types::Uint64 totalAllocated = d_allocated.addRelaxed(
        deltaValue);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(totalAllocated >
                                              d_allocationLimit)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        const bsls::Types::Uint64 uint64Max =
            bsl::numeric_limits<bsls::Types::Uint64>::max();
        if (d_allocationLimit.swap(uint64Max) != uint64Max) {
            // Use an atomic swap to only invoke the callback the first time
            // limit is crossed.  Swap 'allocationLimit' with 'Uint64::max',
            // which will disable all further maximum allocation checks.

            BSLS_ASSERT_SAFE(d_allocationLimitCb);
            // If d_allocationLimit was set, 'd_allocationLimitCb' must be
            // a valid callback
            d_allocationLimitCb();
        }
    }

    // Propagate allocation change to parent, if any
    if (d_parentCounting_p) {
        d_parentCounting_p->onAllocationChange(deltaValue);
    }
}

void CountingAllocator::setAllocationLimit(
    bsls::Types::Uint64            limit,
    const AllocationLimitCallback& callback)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(callback);

    d_allocationLimitCb = callback;
    d_allocationLimit   = limit;
}

void* CountingAllocator::allocate(size_type size)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(static_cast<bsls::Types::Uint64>(size) <=
                     static_cast<bsls::Types::Uint64>(
                         bsl::numeric_limits<bsls::Types::Int64>::max()));
    // The 'd_statContext_mp' can adjust value working with values up to
    // and including the max of 'bsls::Types::Int64'.

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(size == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return 0;  // RETURN
    }

    if (!d_statContext_mp) {
        // We're not recording stats; simply allocate a chunk of 'size' bytes.
        return d_allocator_p->allocate(size);  // RETURN
    }

    // We're recording stats; allocate 'sizeof(header) + size' bytes.
    const bsls::Types::Int64 totalSize =
        bsls::AlignmentUtil::roundUpToMaximalAlignment(size) + sizeof(Header);
    BSLS_ASSERT_SAFE(totalSize >= 0);
    d_statContext_mp->adjustValue(0, totalSize);

    Header* header = static_cast<Header*>(d_allocator_p->allocate(totalSize));
    header->d_data.d_numAllocatedBytes = totalSize;
    header->d_data.d_magic             = k_MAGIC;

    onAllocationChange(totalSize);

    return header + 1;
}

void CountingAllocator::deallocate(void* address)
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(address == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    if (!d_statContext_mp) {
        // We're not recording stats; simply deallocate.
        d_allocator_p->deallocate(address);
        return;  // RETURN
    }

    // We're recording stats
    Header* header = static_cast<Header*>(address) - 1;

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(header->d_data.d_magic !=
                                              k_MAGIC)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // Attempting to free either:
        //  1. Previously freed memory; or
        //  2. Unallocated memory
        BALL_LOG_ERROR_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM
                << ((header->d_data.d_magic == ~k_MAGIC)
                        ? "freeing previously freed memory"
                        : "freeing unallocated memory")
                << ".  Allocator: " << d_statContext_mp->name()
                << ".  Stack:\n";
            balst::StackTracePrintUtil::printStackTrace(
                BALL_LOG_OUTPUT_STREAM);
        }

        BSLS_ASSERT_OPT(header->d_data.d_magic == k_MAGIC);
        // Memory 'corruption', force a core dump and abort.
    }

    const CountingAllocator::size_type totalSize =
        header->d_data.d_numAllocatedBytes;
    header->d_data.d_magic = ~k_MAGIC;
    d_allocator_p->deallocate(header);

    d_statContext_mp->adjustValue(0, -totalSize);
    onAllocationChange(-totalSize);
}

}  // close package namespace
}  // close enterprise namespace
