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

// mwcst_tableschema.h                                                -*-C++-*-
#ifndef INCLUDED_MWCST_TABLESCHEMA
#define INCLUDED_MWCST_TABLESCHEMA

//@PURPOSE: Descriptor for the schema of a 'mwcst::Table' (columns).
//
//@CLASSES:
// mwcst::TableSchemaColumn : a column in a 'mwcst::TableSchema'.
// mwcst::TableSchema       : the columns of a 'mwcst::Table'
//
//@SEE_ALSO: mwcst_table
//
//@DESCRIPTION: This component provides a mechanism, 'mwcst::TableSchema' which
// is used to define the columns of a 'mwct::Table'.
//
// See 'mwcst::Table' for more details.

#include <mwcst_statcontext.h>
#include <mwcst_statvalue.h>
#include <mwcst_value.h>

#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mwcst {

// FORWARD DECLARATIONS
class TableSchema;

// =======================
// class TableSchemaColumn
// =======================

/// A column in a `mwcst::TableSchema`.
class TableSchemaColumn {
  public:
    // PUBLIC TYPES
    typedef bsl::function<void(Value*                 value,
                               const StatContext&     context,
                               int                    level,
                               StatContext::ValueType type)>
        ValueFn;

  private:
    // DATA
    bsl::string d_name;

    ValueFn d_fn;

    // FRIENDS
    friend class TableSchema;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TableSchemaColumn,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    TableSchemaColumn(const bslstl::StringRef& name,
                      const ValueFn&           fn,
                      bslma::Allocator*        basicAllocator = 0);
    TableSchemaColumn(const TableSchemaColumn& other,
                      bslma::Allocator*        basicAllocator = 0);

    // ACCESSORS
    const bsl::string& name() const;

    void evaluate(Value*                 value,
                  const StatContext&     context,
                  int                    level,
                  StatContext::ValueType type) const;
};

// =================
// class TableSchema
// =================

/// The columns of a `mwcst::Table`.
class TableSchema {
  public:
    // PUBLIC TYPES
    typedef TableSchemaColumn Column;

  private:
    // DATA
    bsl::vector<Column> d_columns;

    bslma::Allocator* d_allocator_p;

    // NOT IMPLEMENTED
    TableSchema& operator=(const TableSchema&);

  public:
    // CREATORS
    TableSchema(bslma::Allocator* basicAllocator = 0);
    TableSchema(const TableSchema& other,
                bslma::Allocator*  basicAllocator = 0);

    // MANIPULATORS
    TableSchemaColumn& addColumn(const bslstl::StringRef& name,
                                 const Column::ValueFn&   func);

    void addDefaultIdColumn(const bslstl::StringRef& name);

    TableSchemaColumn& addColumn(const bslstl::StringRef& name,
                                 int                      statIndex,
                                 bsls::Types::Int64 (*func)(const StatValue&));
    TableSchemaColumn&
    addColumn(const bslstl::StringRef& name,
              int                      statIndex,
              bsls::Types::Int64 (*func)(const StatValue&,
                                         const StatValue::SnapshotLocation&),
              const StatValue::SnapshotLocation& snapshot);
    TableSchemaColumn&
    addColumn(const bslstl::StringRef& name,
              int                      statIndex,
              bsls::Types::Int64 (*func)(const StatValue&,
                                         const StatValue::SnapshotLocation&,
                                         const StatValue::SnapshotLocation&),
              const StatValue::SnapshotLocation& snapshot1,
              const StatValue::SnapshotLocation& snapshot2);

    TableSchemaColumn& addColumn(const bslstl::StringRef& name,
                                 int                      statIndex,
                                 double (*func)(const StatValue&));
    TableSchemaColumn& addColumn(
        const bslstl::StringRef& name,
        int                      statIndex,
        double (*func)(const StatValue&, const StatValue::SnapshotLocation&),
        const StatValue::SnapshotLocation& snapshot);
    TableSchemaColumn&
    addColumn(const bslstl::StringRef& name,
              int                      statIndex,
              double (*func)(const StatValue&,
                             const StatValue::SnapshotLocation&,
                             const StatValue::SnapshotLocation&),
              const StatValue::SnapshotLocation& snapshot1,
              const StatValue::SnapshotLocation& snapshot2);

    // ACCESSORS
    int                      numColumns() const;
    const TableSchemaColumn& column(int index) const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
