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

// mwcst_statcontexttableinfoprovider.h                               -*-C++-*-
#ifndef INCLUDED_MWCST_STATCONTEXTTABLEINFOPROVIDER
#define INCLUDED_MWCST_STATCONTEXTTABLEINFOPROVIDER

//@PURPOSE: Provide a 'mwcst::TableInfoProvider' for a table 'StatContext'.
//
//@CLASSES:
// mwcst::StatContextTableInfoProvider
//
//@SEE_ALSO: mwcst_statcontext
//           mwcst_tableutil
//
//@DESCRIPTION: This component defines a mechanism,
// 'mwcst::StatContextTableInfoProvider' which is an implementation of
// 'mwcst::TableInfoProvider' using a 'mwcst::StatContext'.  It gives an
// application a way of easily printing a table 'mwcst::StatContext' to a
// stream.
//
// Refer to the documentation of 'mwcst::Table' and 'mwcst::TableUtil' for
// usage examples.

#include <mwcst_statcontext.h>

#include <mwcst_tableinfoprovider.h>

#include <bsl_functional.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_allocatorargt.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {
namespace mwcst {

// ==============================================
// class StatContextTableInfoProviderCustomColumn
// ==============================================

/// Protocol for a class that defines a custom column to be printed by a
/// `mwcst::StatContextTableInfoProvider`
class StatContextTableInfoProviderCustomColumn {
  public:
    // CREATORS
    virtual ~StatContextTableInfoProviderCustomColumn();

    // ACCESSORS

    /// Return the size of the value to print for the specified `valueType`
    /// of the specified `context` being printed at the specified
    /// indentation `level`
    virtual int getValueSize(int                       level,
                             const mwcst::StatContext& context,
                             StatContext::ValueType    valueType) const = 0;

    /// Print the value of the specified `valueType` of the specified
    /// `context` being printed at the specified indentation `level` to the
    /// specified `stream` and return the `stream`.
    virtual bsl::ostream&
    printValue(bsl::ostream&             stream,
               int                       level,
               const mwcst::StatContext& context,
               StatContext::ValueType    valueType) const = 0;
};

// ==================================
// class StatContextTableInfoProvider
// ==================================

/// `mwcst::TableInfoProvider` for a table `mwcst::StatContext`
class StatContextTableInfoProvider : public mwcst::TableInfoProvider {
  public:
    // PUBLIC TYPES

    /// Function used to determine if the specified `context`s specified
    /// `valueType line should be included in the output.  Return `true'
    /// for a context to be included, or `false` for a context to be
    /// filtered out
    typedef bsl::function<bool(const StatContext*     context,
                               StatContext::ValueType valueType,
                               int                    level)>
        FilterFn;

    /// Comparator used to sort StatContexts at the same level.
    typedef bsl::function<bool(const StatContext* lhs, const StatContext* rhs)>
        SortFn;

    typedef bsl::function<bsls::Types::Int64(const mwcst::StatValue&)>
                                                           IntValueFunctor;
    typedef bsl::function<double(const mwcst::StatValue&)> DoubleValueFunctor;

    enum PrintType { DMCST_INT_VALUE, DMCST_NS_INTERVAL_VALUE };

  private:
    // PRIVATE TYPES
    typedef StatContextTableInfoProviderCustomColumn CustomColumn;

    typedef bsls::Types::Int64 (*Int0ArgFunc)(const mwcst::StatValue& value);
    typedef bsls::Types::Int64 (*Int1ArgFunc)(
        const mwcst::StatValue&            value,
        const StatValue::SnapshotLocation& arg1);
    typedef bsls::Types::Int64 (*Int2ArgFunc)(
        const mwcst::StatValue&            value,
        const StatValue::SnapshotLocation& arg1,
        const StatValue::SnapshotLocation& arg2);

    typedef double (*Double0ArgFunc)(const mwcst::StatValue& value);
    typedef double (*Double1ArgFunc)(const mwcst::StatValue&            value,
                                     const StatValue::SnapshotLocation& arg1);
    typedef double (*Double2ArgFunc)(const mwcst::StatValue&            value,
                                     const StatValue::SnapshotLocation& arg1,
                                     const StatValue::SnapshotLocation& arg2);

    struct RowInfo {
        const StatContext*     d_context_p;
        StatContext::ValueType d_valueType;
        int                    d_level;
    };

    struct ColumnInfo {
        // DATA
        int                 d_groupIndex;
        bsl::string         d_name;
        int                 d_statValueIndex;
        PrintType           d_printType;
        IntValueFunctor     d_intFunc;
        DoubleValueFunctor  d_doubleFunc;
        const CustomColumn* d_customColumn_p;

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(ColumnInfo, bslma::UsesBslmaAllocator)

        // CREATORS
        ColumnInfo(int                      groupIndex,
                   const bslstl::StringRef& name,
                   int                      statValueIndex,
                   PrintType                printType,
                   const IntValueFunctor&   func,
                   bslma::Allocator*        basicAllocator = 0)
        : d_groupIndex(groupIndex)
        , d_name(name, basicAllocator)
        , d_statValueIndex(statValueIndex)
        , d_printType(printType)
        , d_intFunc(bsl::allocator_arg, basicAllocator, func)
        , d_doubleFunc(bsl::allocator_arg, basicAllocator)
        , d_customColumn_p(0)
        {
        }

        ColumnInfo(int                       groupIndex,
                   const bslstl::StringRef&  name,
                   int                       statValueIndex,
                   PrintType                 printType,
                   const DoubleValueFunctor& func,
                   bslma::Allocator*         basicAllocator = 0)
        : d_groupIndex(groupIndex)
        , d_name(name, basicAllocator)
        , d_statValueIndex(statValueIndex)
        , d_printType(printType)
        , d_intFunc(bsl::allocator_arg, basicAllocator)
        , d_doubleFunc(bsl::allocator_arg, basicAllocator, func)
        , d_customColumn_p(0)
        {
        }

        ColumnInfo(int                      groupIndex,
                   const bslstl::StringRef& name,
                   const CustomColumn*      customColumn,
                   bslma::Allocator*        basicAllocator = 0)
        : d_groupIndex(groupIndex)
        , d_name(name, basicAllocator)
        , d_statValueIndex(0)
        , d_printType(DMCST_INT_VALUE)
        , d_intFunc(bsl::allocator_arg, basicAllocator)
        , d_doubleFunc(bsl::allocator_arg, basicAllocator)
        , d_customColumn_p(customColumn)
        {
        }

        ColumnInfo(const ColumnInfo& other,
                   bslma::Allocator* basicAllocator = 0)
        : d_groupIndex(other.d_groupIndex)
        , d_name(other.d_name, basicAllocator)
        , d_statValueIndex(other.d_statValueIndex)
        , d_printType(other.d_printType)
        , d_intFunc(bsl::allocator_arg, basicAllocator, other.d_intFunc)
        , d_doubleFunc(bsl::allocator_arg, basicAllocator, other.d_doubleFunc)
        , d_customColumn_p(other.d_customColumn_p)
        {
        }
    };

    // DATA
    bslma::ManagedPtr<CustomColumn> d_defaultIdColumn_p;
    // Created the first time
    // 'addDefaultIdColumn' is called.

    const StatContext* d_context_p;

    bsl::vector<RowInfo> d_rows;
    // The records of our table.  A '0'
    // element indicates the direct values
    // of the context above it.

    bsl::string d_title;

    bsl::vector<bsl::string> d_columnGroups;

    bsl::vector<ColumnInfo> d_columns;
    // Map from column index to column info

    int d_precision;

    FilterFn d_filter;

    SortFn d_cmp;

    bool d_considerChildrenOfFilteredContexts;

    bool d_printSeparators;

    bslma::Allocator* d_allocator_p;

    // NOT IMPLEMENTED
    StatContextTableInfoProvider(const StatContextTableInfoProvider&);
    StatContextTableInfoProvider&
    operator=(const StatContextTableInfoProvider&);

    // PRIVATE MANIPULATORS
    void addContext(const StatContext* context, int level, bool isFilteredOut);

    // PRIVATE ACCESSORS
    const StatValue& getValue(const RowInfo&    row,
                              const ColumnInfo& column) const;

  public:
    // CREATORS
    StatContextTableInfoProvider(bslma::Allocator* basicAllocator = 0);
    ~StatContextTableInfoProvider() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Iterate over our associated StatContext and figure out which
    /// contexts should be used to define our table.  This should be called
    /// after the context is snapshotted and before this object is used to
    /// print the table.
    void update();

    /// Set the context associated with this `StatContextTableInfoProvider`
    void setContext(const StatContext* context);

    /// Set the precision to use for printing `double` values.  The default
    /// precision is `2`, meaning 2 decimal places are printed.
    void setPrecision(int precision);

    /// Set the function used to filter out contexts from the table
    void setFilter(const FilterFn& filter);

    /// Set the comparator used to sort contexts at the same level
    void setComparator(const SortFn& comparator);

    /// Set whether children of filtered out contexts should still be
    /// considered.  By default if a context is filtered out, its children
    /// won't be displayed.
    void considerChildrenOfFilteredContexts(bool value);

    /// Set whether INT_VALUE columns should be printed with separators.
    /// Default is `true`.
    void printIntsWithSeparator(bool value);

    /// Set the title of this table to the specified `title`.
    void setTitle(const bslstl::StringRef& title);

    /// Set the name of the column group that columns added by future calls
    /// to `addColumn` will belong to.
    void setColumnGroup(const bslstl::StringRef& name);

    /// Add a column printing the context ids at the appropriate
    /// indentation level to the current column group.  Normally, this
    /// should be included as the first column, and should belong to its
    /// own column group.  Use the optionally specified `maxSize` to define
    /// a maximum width of a column.
    void addDefaultIdColumn(const bslstl::StringRef& name, int maxSize = 0);

    /// Add the specified `column` with the specified `name` to the set of
    /// columns belonging to the current column group.
    void addColumn(const bslstl::StringRef& name, const CustomColumn* column);

    /// Add a column to the current column group with the specified `name`
    /// using the specified `func` to calculate the values to display from
    /// the specified `statValueIndex`th value of the context being
    /// printed.  The displayed value will be printed as defined by the
    /// specified `printType`.
    void addColumn(const bslstl::StringRef& name,
                   int                      statValueIndex,
                   PrintType                printType,
                   const IntValueFunctor&   func);
    void addColumn(const bslstl::StringRef&  name,
                   int                       statValueIndex,
                   PrintType                 printType,
                   const DoubleValueFunctor& func);

    void addColumn(const bslstl::StringRef& name,
                   int                      statValueIndex,
                   Int0ArgFunc              func);
    void addColumn(const bslstl::StringRef&           name,
                   int                                statValueIndex,
                   Int1ArgFunc                        func,
                   const StatValue::SnapshotLocation& arg1);
    void addColumn(const bslstl::StringRef&           name,
                   int                                statValueIndex,
                   Int2ArgFunc                        func,
                   const StatValue::SnapshotLocation& arg1,
                   const StatValue::SnapshotLocation& arg2);

    /// Add a column to the current column group with the specified `name`
    /// using the specified `func` to calculate the values to display from
    /// the specified `statValueIndex`th value of the context being
    /// printed, passing the specified `arg1` and `arg2` to the `func`,
    /// assing the value to be printed is a simple integer.
    void addColumn(const bslstl::StringRef& name,
                   int                      statValueIndex,
                   Double0ArgFunc           func);
    void addColumn(const bslstl::StringRef&           name,
                   int                                statValueIndex,
                   Double1ArgFunc                     func,
                   const StatValue::SnapshotLocation& arg1);
    void addColumn(const bslstl::StringRef&           name,
                   int                                statValueIndex,
                   Double2ArgFunc                     func,
                   const StatValue::SnapshotLocation& arg1,
                   const StatValue::SnapshotLocation& arg2);

    // ACCESSORS

    // mwcst::TableInfoProvider

    /// Return the number of data rows in the table
    int numRows() const BSLS_KEYWORD_OVERRIDE;

    /// Return the number of columns at the specified header `level`.
    /// `level == 0` is the first (bottom) header row, and is the actual
    /// number of columns in the table.  `level == 1` is the row above it,
    /// and so on.
    int numColumns(int level) const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if this table has a title, and `false` otherwise.
    bool hasTitle() const BSLS_KEYWORD_OVERRIDE;

    /// Return the number of header rows in the table.  This must return at
    /// least `1`.
    int numHeaderLevels() const BSLS_KEYWORD_OVERRIDE;

    /// Return the printed length of the value in the specified `column` of
    /// the specified `row` in the table.
    int getValueSize(int row, int column) const BSLS_KEYWORD_OVERRIDE;

    /// Print the value in the specified `column` of the specified `row` to
    /// the specified `stream` with the specified `width`.  The width of
    /// the `stream` is already set when this is called as a convenience.
    /// Return the `stream`.
    bsl::ostream& printValue(bsl::ostream& stream,
                             int           row,
                             int           column,
                             int           width) const BSLS_KEYWORD_OVERRIDE;

    /// Return the printed length of the header cell in the specified
    /// `column` at the specified header `level`.
    int getHeaderSize(int level, int column) const BSLS_KEYWORD_OVERRIDE;

    /// Return the header cell index in header `level + 1` which is above
    /// the specified `column` in the specified header `level`.
    int getParentHeader(int level, int column) const BSLS_KEYWORD_OVERRIDE;

    /// Print the title of this table to the specified `stream`.  Return the
    /// `stream`.
    bsl::ostream& printTitle(bsl::ostream& stream) const BSLS_KEYWORD_OVERRIDE;

    /// Print the header of the specified `column` at the specified
    /// header `level` to the specified `stream` with the specified
    /// `width`.  The width of the `stream`is already set when this is
    /// called as a convenience.  Return the `stream`.
    bsl::ostream& printHeader(bsl::ostream& stream,
                              int           level,
                              int           column,
                              int           width) const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------------------------
// class StatContextTableInfoProvider
// ----------------------------------

// MANIPULATORS
inline void
StatContextTableInfoProvider::setContext(const StatContext* context)
{
    d_context_p = context;
}

inline void StatContextTableInfoProvider::setPrecision(int precision)
{
    d_precision = precision;
}

inline void StatContextTableInfoProvider::setFilter(const FilterFn& filter)
{
    d_filter = filter;
}

inline void
StatContextTableInfoProvider::setComparator(const SortFn& comparator)
{
    d_cmp = comparator;
}

inline void
StatContextTableInfoProvider::considerChildrenOfFilteredContexts(bool value)
{
    d_considerChildrenOfFilteredContexts = value;
}

inline void StatContextTableInfoProvider::printIntsWithSeparator(bool value)
{
    d_printSeparators = value;
}

}  // close package namespace

// FREE OPERATORS

}  // close enterprise namespace

#endif
