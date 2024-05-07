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

// mwcst_basictableinfoprovider.h -*-C++-*-
#ifndef INCLUDED_MWCST_BASICTABLEINFOPROVIDER
#define INCLUDED_MWCST_BASICTABLEINFOPROVIDER

//@PURPOSE: Provide an adapter of 'mwcst::BaseTable' to
//'mwcst::TableInfoProvider'
//
//@CLASSES:
// mwcst::BasicTableInfoProvider
//
//@DESCRIPTION: This component defines a mechanism,
// 'mwcst::BasicTableInfoProvider', which adapts the 'mwcst::BaseTable'
// protocol to the 'mwcst::TableInfoProvider' protocol.  It allows the user to
// specifiy exactly how a 'mwcst::BaseTable' should be printed.

#include <mwcst_tableinfoprovider.h>

#include <bdlb_nullablevalue.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmf_nil.h>

namespace BloombergLP {

namespace mwcst {

// FORWARD DECLARATIONS
class BaseTable;
class BasicTableInfoProvider;
class BasicTableInfoProvider_ValueSizeVisitor;
class BasicTableInfoProvider_ValuePrintVisitor;

// =========================================
// class BasicTableInfoProvider_ColumnFormat
// =========================================

/// Format of a a column to be printed by a `BasicTableInfoProvider`
class BasicTableInfoProvider_ColumnFormat {
  private:
    // PRIVATE TYPES
    enum Justification { DMCU_LEFT, DMCU_RIGHT, DMCU_CENTER };

    enum ValueType { DMCU_DEFAULT, DMCU_NS_TIME_INTERVAL, DMCU_MEMORY };

    typedef bdlb::NullableValue<bsl::string> NullableString;

    // DATA
    bsl::string d_tableColumnName;  // Name of column in table to print

    int d_tableColumnIndex;
    // Index of 'd_tableColumnName' in the
    // table

    bsl::string d_printColumnName;  // Name to print for this column

    bool d_printSeparators;  // Print a ',' separator in numbers

    int d_precision;  // How many decimal places to print for
                      // all decimal numbers

    ValueType d_type;  // How to interpret the value

    Justification d_justification;  // How to justify the value

    int d_columnGroupIndex;
    // Index of the column group this column
    // belongs to

    NullableString d_zeroString;  // Value to display instead of '0'

    NullableString d_extremeString;  // Value to display instead of min or
                                     // max value

    // FRIENDS
    friend class BasicTableInfoProvider;
    friend class BasicTableInfoProvider_ValueSizeVisitor;
    friend class BasicTableInfoProvider_ValuePrintVisitor;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(BasicTableInfoProvider_ColumnFormat,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    BasicTableInfoProvider_ColumnFormat(
        const bslstl::StringRef& tableColumnName,
        int                      tableColumnIndex,
        const bslstl::StringRef& printColumnName,
        bslma::Allocator*        basicAllocator = 0);
    BasicTableInfoProvider_ColumnFormat(
        const BasicTableInfoProvider_ColumnFormat& other,
        bslma::Allocator*                          basicAllocator = 0);

    // MANIPULATORS
    BasicTableInfoProvider_ColumnFormat& justifyLeft();
    BasicTableInfoProvider_ColumnFormat& justifyRight();

    /// Set the justification for values.  Default is `right`.
    BasicTableInfoProvider_ColumnFormat& justifyCenter();

    /// Assume numerical values in the column are nanosecond time intervals,
    /// and print them accordingly.
    BasicTableInfoProvider_ColumnFormat& printAsNsTimeInterval();

    /// Assume numerical values in the column are byte amounts, and print
    /// them accordingly
    BasicTableInfoProvider_ColumnFormat& printAsMemory();

    /// Print any decimal values in the column with the specified
    /// `precision`.  Default is `2`.
    BasicTableInfoProvider_ColumnFormat& setPrecision(int precision);

    /// Set whether a `,` should be printed as a separator in large
    /// numbers.  Default is `true`.
    BasicTableInfoProvider_ColumnFormat& printSeparators(bool value);

    /// Set the string to display instead of `0` to the specified `value`.
    BasicTableInfoProvider_ColumnFormat&
    zeroString(const bslstl::StringRef& value);

    /// Set the string to display instead of "N/A" when the value to
    /// display is either `max` or `min` to the specified `value`.
    BasicTableInfoProvider_ColumnFormat&
    extremeValueString(const bslstl::StringRef& value);
};

// =============================================
// class BasicTableInfoProvider_ValueSizeVisitor
// =============================================

/// A `mwcst::Value` visitor that returns the printed size of the value
class BasicTableInfoProvider_ValueSizeVisitor {
  public:
    // PUBLIC TYPES
    typedef int ResultType;

  private:
    // PRIVATE TYPES
    typedef BasicTableInfoProvider_ColumnFormat ColumnFormat;

    // DATA
    const ColumnFormat* d_fmt_p;

    // NOT IMPLEMENTED
    BasicTableInfoProvider_ValueSizeVisitor(
        const BasicTableInfoProvider_ValueSizeVisitor&);
    BasicTableInfoProvider_ValueSizeVisitor&
    operator=(const BasicTableInfoProvider_ValueSizeVisitor&);

  public:
    // CREATORS
    BasicTableInfoProvider_ValueSizeVisitor();

    // MANIPULATORS
    void setFormat(const ColumnFormat* fmt);

    // ACCESSORS

    /// Return the printed size of the specified `value` determined using
    /// the `BasicTableInfoProvider_ColumnFormat` set by the last call to
    /// `setFormat`.
    int operator()(bsls::Types::Int64 value) const;
    int operator()(int value) const;
    int operator()(const bslstl::StringRef& value) const;
    int operator()(double value) const;
    int operator()(bslmf::Nil value) const;

    /// Return `1`
    template <typename TYPE>
    int operator()(const TYPE& value) const;
};

// ==============================================
// class BasicTableInfoProvider_ValuePrintVisitor
// ==============================================

/// A `mwcst::Value` visitor that prints the a value
class BasicTableInfoProvider_ValuePrintVisitor {
  public:
    // PUBLIC TYPES
    typedef int ResultType;

  private:
    // PRIVATE TYPES
    typedef BasicTableInfoProvider_ColumnFormat ColumnFormat;

    // DATA
    const ColumnFormat* d_fmt_p;
    bsl::ostream*       d_stream_p;

    // NOT IMPLEMENTED
    BasicTableInfoProvider_ValuePrintVisitor(
        const BasicTableInfoProvider_ValuePrintVisitor&);
    BasicTableInfoProvider_ValuePrintVisitor&
    operator=(const BasicTableInfoProvider_ValuePrintVisitor&);

  public:
    // CREATORS
    BasicTableInfoProvider_ValuePrintVisitor();

    // MANIPULATORS
    void reset(const ColumnFormat* fmt, bsl::ostream* stream);

    // ACCESSORS

    /// Print the specified `value` using the
    /// `BasicTableInfoProvider_ColumnFormat` and `bsl::ostream` set by the
    /// last call to `reset`.
    int operator()(bsls::Types::Int64 value) const;
    int operator()(int value) const;
    int operator()(const bslstl::StringRef& value) const;
    int operator()(double value) const;
    int operator()(bslmf::Nil value) const;

    /// Print the specified `value` directly.
    template <typename TYPE>
    int operator()(const TYPE& value) const;
};

// ============================
// class BasicTableInfoProvider
// ============================

/// Adapts `mwcst::BaseTable` to `mwcst::TableInfoProvider`
class BasicTableInfoProvider : public mwcst::TableInfoProvider {
  public:
    // PUBLIC TYPES
    typedef BasicTableInfoProvider_ColumnFormat ColumnFormat;

  private:
    // PRIVATE TYPES
    typedef BasicTableInfoProvider_ValueSizeVisitor  ValueSizeVisitor;
    typedef BasicTableInfoProvider_ValuePrintVisitor ValuePrintVisitor;
    typedef bsl::vector<ColumnFormat>                ColumnList;

    // DATA
    mwcst::BaseTable*         d_table_p;
    bsl::string               d_title;
    bsl::vector<bsl::string>  d_columnGroups;
    ColumnList                d_columns;
    mutable ValueSizeVisitor  d_valueSizeVisitor;
    mutable ValuePrintVisitor d_valuePrintVisitor;

    // NOT IMPLEMENTED
    BasicTableInfoProvider(const BasicTableInfoProvider&);
    BasicTableInfoProvider& operator=(const BasicTableInfoProvider&);

  public:
    // CREATORS
    BasicTableInfoProvider(bslma::Allocator* basicAllocator = 0);
    BasicTableInfoProvider(mwcst::BaseTable* table,
                           bslma::Allocator* basicAllocator = 0);

    // MANIPULATORS

    /// Clear out all columns and column groups
    void reset();

    /// Set the table to be printed to the specified `table`.  Any added
    /// columns will continue to refer to the same column indices.
    void setTable(mwcst::BaseTable* table);

    /// Find the right column indices in the current table corresponding to
    /// the source column names of the current columns.
    void bind();

    /// Set the title of this table to the specified `title`.
    void setTitle(const bslstl::StringRef& title);

    /// Set the name of the column group that columns added by future calls
    /// to `addColumn` will belong to to the specified `groupName`
    void setColumnGroup(const bslstl::StringRef& groupName);

    ColumnFormat& addColumn(const bslstl::StringRef& tableColumnName,
                            const bslstl::StringRef& printColumnName);
    // Add a column to be printed with the specified 'printColumnName'
    // using the column with the specified 'tableColumnName' as the
    // source column from the associated table.  Return a 'ColumnFormat'
    // that can be used to more precisely format the column.

    // ACCESSORS

    // mwcst::TableInfoProvider
    int  numRows() const BSLS_KEYWORD_OVERRIDE;
    int  numColumns(int level) const BSLS_KEYWORD_OVERRIDE;
    bool hasTitle() const BSLS_KEYWORD_OVERRIDE;
    int  numHeaderLevels() const BSLS_KEYWORD_OVERRIDE;
    int  getValueSize(int row, int column) const BSLS_KEYWORD_OVERRIDE;
    bsl::ostream& printValue(bsl::ostream& stream,
                             int           row,
                             int           column,
                             int           width) const BSLS_KEYWORD_OVERRIDE;
    int getHeaderSize(int level, int column) const BSLS_KEYWORD_OVERRIDE;
    int getParentHeader(int level, int column) const BSLS_KEYWORD_OVERRIDE;
    bsl::ostream& printTitle(bsl::ostream& stream) const BSLS_KEYWORD_OVERRIDE;
    bsl::ostream& printHeader(bsl::ostream& stream,
                              int           level,
                              int           column,
                              int           width) const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------------------------
// class BasicTableInfoProvider_ValueSizeVisitor
// ---------------------------------------------

// ACCESSORS
template <typename TYPE>
int BasicTableInfoProvider_ValueSizeVisitor::operator()(
    const TYPE& value) const
{
    (void)value;  // compiler warning unused parameter 'value'
    return 1;
}

// ----------------------------------------------
// class BasicTableInfoProvider_ValuePrintVisitor
// ----------------------------------------------

// ACCESSORS
template <typename TYPE>
int BasicTableInfoProvider_ValuePrintVisitor::operator()(
    const TYPE& value) const
{
    (*d_stream_p) << value;
    return 0;
}

}  // close package namespace
}  // close enterprise namespace

#endif
