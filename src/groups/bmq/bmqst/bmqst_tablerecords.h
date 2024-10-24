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

// bmqst_tablerecords.h                                               -*-C++-*-
#ifndef INCLUDED_BMQST_TABLERECORDS
#define INCLUDED_BMQST_TABLERECORDS

//@PURPOSE: Provide a container for records in a Stat Context.
//
//@CLASSES:
// bmqst::TableRecordsRecord : An individual record.
// bmqst::TableRecord        : Records of a 'bmqst::StatContext' table.
//
//@SEE_ALSO: bmqst_statcontext
//           bmqst_table
//
//@DESCRIPTION: This component provides a mechanism for accessing and filtering
// or sorting records in a 'bmqst::Table' associated with a
// 'bmqst::StatContext'.
//
// See 'bmqst::Table' for more details.

#include <bmqst_statcontext.h>

#include <bsl_functional.h>
#include <bsl_utility.h>
#include <bsl_vector.h>

namespace BloombergLP {
namespace bmqst {

// FORWARD DECLARATIONS
class TableRecords;

// ========================
// class TableRecordsRecord
// ========================

/// A record in a `bmqst::TableRecords`.
class TableRecordsRecord {
  private:
    // DATA
    const StatContext* d_context_p;

    StatContext::ValueType d_type;

    int d_level;

    // FRIENDS
    friend class TableRecords;

    // PRIVATE CREATORS
    TableRecordsRecord();
    TableRecordsRecord(const StatContext*     context,
                       StatContext::ValueType type,
                       int                    level);

  public:
    // ACCESSORS

    /// Return the associated stat context.
    const StatContext& context() const;

    /// Return the value type.
    StatContext::ValueType type() const;

    /// Return the level of this record in a hierarchy.
    int level() const;
};

// ==================
// class TableRecords
// ==================

/// Records of a `bmqst::StatContext` table.
class TableRecords {
  public:
    // PUBLIC TYPES
    typedef TableRecordsRecord Record;

    /// Function used to determine if the specified `record` should be
    /// included in the output.  Return `true` for a record to be included,
    /// or `false` for the record to be filtered out
    typedef bsl::function<bool(const Record& record)> FilterFn;

    /// Comparator used to sort StatContexts at the same level.
    typedef bsl::function<bool(const StatContext* lhs, const StatContext* rhs)>
        SortFn;

  private:
    // TYPES
    typedef bsl::vector<bsl::pair<const StatContext*, bool> > SortBuffer;

    // DATA
    const StatContext* d_context_p;

    FilterFn d_filter;

    SortFn d_sort;

    bsl::vector<Record> d_records;

    bool d_considerChildrenOfFilteredContexts;

    SortBuffer d_sortBuffer;

    // PRIVATE MANIPULATORS
    void addContext(const StatContext* context, int level, bool isFilteredOut);

    // NOT IMPLEMENTED
    TableRecords(const TableRecords&);
    TableRecords& operator=(const TableRecords&);

  public:
    // CREATORS
    TableRecords(bslma::Allocator* basicAllocator = 0);

    // MANIPULATORS

    /// Set the specified `filter` function.
    void setFilter(const FilterFn& filter);

    /// Set the specified `sort` sorting function.
    void setSort(const SortFn& sort);

    /// Associate the `TableRecords` with the specified stat `context`.
    void setContext(const StatContext* context);

    /// Set whether children of filtered out contexts should still be
    /// considered.  By default if a context is filtered out, its children
    /// won't be displayed.
    void considerChildrenOfFilteredContexts(bool value);

    /// Update the records in this object from the associated stat context.
    /// The behavior is undefined unless `setContext` was previously called
    /// once with a valid context.
    void update();

    // ACCESSORS

    /// Return the associated stat context.
    const StatContext* context() const;

    /// Return the number of records in the table.
    int numRecords() const;

    /// Return the record at the specified `index`.
    const Record& record(int index) const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
