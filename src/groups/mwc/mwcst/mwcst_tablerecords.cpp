// Copyright 2022-2023 Bloomberg Finance L.P.
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

// mwcst_tablerecords.cpp                                             -*-C++-*-
#include <mwcst_tablerecords.h>

#include <bsl_algorithm.h>
#include <bslmf_allocatorargt.h>
#include <mwcscm_version.h>

namespace BloombergLP {
namespace mwcst {

namespace {

// STRUCTS
struct SortBufferComparator {
    TableRecords::SortFn* d_sort_p;

    // ACCESSORS
    bool operator()(const bsl::pair<const StatContext*, bool>& lhs,
                    const bsl::pair<const StatContext*, bool>& rhs)
    {
        return (*d_sort_p)(lhs.first, rhs.first);
    }
};

}  // close anonymous namesapace

// ------------------------
// class TableRecordsRecord
// ------------------------

// PRIVATE CREATORS
TableRecordsRecord::TableRecordsRecord()
: d_context_p(0)
, d_type(StatContext::DMCST_TOTAL_VALUE)
, d_level(0)
{
}

TableRecordsRecord::TableRecordsRecord(const StatContext*     context,
                                       StatContext::ValueType type,
                                       int                    level)
: d_context_p(context)
, d_type(type)
, d_level(level)
{
}

// ACCESSORS
const StatContext& TableRecordsRecord::context() const
{
    return *d_context_p;
}

StatContext::ValueType TableRecordsRecord::type() const
{
    return d_type;
}

int TableRecordsRecord::level() const
{
    return d_level;
}

// ------------------
// class TableRecords
// ------------------

// PRIVATE MANIPULATORS
void TableRecords::addContext(const StatContext* context,
                              int                level,
                              bool               isFilteredOut)
{
    if (!isFilteredOut) {
        d_records.push_back(
            Record(context, StatContext::DMCST_TOTAL_VALUE, level));
    }
    else if (!d_considerChildrenOfFilteredContexts) {
        return;
    }

    if (!context->isTable()) {
        // If we're not printing a table, don't consider any children.
        return;
    }

    // First filter anything we don't need
    bool                haveUnfilteredChildren = false;
    size_t              sortBufferStartSize    = d_sortBuffer.size();
    StatContextIterator iter                   = context->subcontextIterator();
    for (; iter; ++iter) {
        d_sortBuffer.resize(d_sortBuffer.size() + 1);
        d_sortBuffer.back().first = iter;
        if (!d_filter ||
            d_filter(
                Record(iter, StatContext::DMCST_TOTAL_VALUE, level + 1))) {
            d_sortBuffer.back().second = false;
            haveUnfilteredChildren     = true;
        }
        else {
            d_sortBuffer.back().second = true;
        }
    }

    // Add 'direct' row if we have any children left
    if (haveUnfilteredChildren && !isFilteredOut) {
        Record rec(context, StatContext::DMCST_DIRECT_VALUE, level + 1);
        if (!d_filter || d_filter(rec)) {
            d_records.push_back(rec);
        }
    }

    // Add 'expired' row if the StatContext has any expired values
    if (context->hasExpiredValues() && !isFilteredOut) {
        Record rec(context, StatContext::DMCST_EXPIRED_VALUE, level + 1);
        if (!d_filter || d_filter(rec)) {
            d_records.push_back(rec);
        }
    }

    // TODO decide if we should have a line for active children total

    // Sort
    if (d_sort) {
        SortBufferComparator cmp;
        cmp.d_sort_p = &d_sort;
        bsl::sort(d_sortBuffer.begin() + sortBufferStartSize,
                  d_sortBuffer.end(),
                  cmp);
    }

    for (size_t i = sortBufferStartSize; i < d_sortBuffer.size(); ++i) {
        addContext(d_sortBuffer[i].first, level + 1, d_sortBuffer[i].second);
    }

    d_sortBuffer.resize(sortBufferStartSize);
}

// CREATORS
TableRecords::TableRecords(bslma::Allocator* basicAllocator)
: d_context_p(0)
, d_filter(bsl::allocator_arg, basicAllocator)
, d_sort(bsl::allocator_arg, basicAllocator)
, d_records(basicAllocator)
, d_considerChildrenOfFilteredContexts(false)
, d_sortBuffer(basicAllocator)
{
}

// MANIPULATORS
void TableRecords::setFilter(const FilterFn& filter)
{
    d_filter = filter;
}

void TableRecords::setSort(const SortFn& sort)
{
    d_sort = sort;
}

void TableRecords::setContext(const StatContext* context)
{
    d_context_p = context;
}

void TableRecords::considerChildrenOfFilteredContexts(bool value)
{
    d_considerChildrenOfFilteredContexts = value;
}

void TableRecords::update()
{
    d_records.clear();
    addContext(
        d_context_p,
        0,
        d_filter &&
            !d_filter(Record(d_context_p, StatContext::DMCST_TOTAL_VALUE, 0)));
}

// ACCESSORS
const StatContext* TableRecords::context() const
{
    return d_context_p;
}

int TableRecords::numRecords() const
{
    return (int)d_records.size();
}

const TableRecords::Record& TableRecords::record(int index) const
{
    BSLS_ASSERT_SAFE(index < (int)d_records.size());
    return d_records[index];
}

}  // close package namespace
}  // close enterprise namespace
