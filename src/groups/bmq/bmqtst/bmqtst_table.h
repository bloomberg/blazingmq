// Copyright 2026 Bloomberg Finance L.P.
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

// bmqtst_table.h                                                     -*-C++-*-
#ifndef INCLUDED_BMQTST_TABLE
#define INCLUDED_BMQTST_TABLE

/// @file bmqtst_table.h
/// @brief Provide a utility for pretty-printing tabular data.
///
/// This component provides a mechanism for aggregating and pretty-printing
/// tabular data in test drivers and benchmark code. The table automatically
/// calculates column widths based on content and formats output with aligned
/// columns and separators.
///
/// # Usage Example
///
/// ## Example 1: Creating and printing a simple table
///
/// The following code illustrates how to create a table and populate it with
/// data.
///
/// ```cpp
/// bmqtst::Table table(bmqtst::TestHelperUtil::allocator());
///
/// // Add data to columns
/// table.column("Test").insertValue("foo");
/// table.column("Test").insertValue("bar");
/// table.column("Time, ns").insertValue(static_cast<bsls::Types::Uint64>(30));
/// table.column("Time, ns").insertValue(static_cast<bsls::Types::Uint64>(25));
///
/// // Print the table
/// table.print(bsl::cout);
///
/// // Output:
/// //  Test | Time, ns
/// // ================
/// //   foo |       30
/// //   bar |       25
/// ```

// BDE
#include <bsl_map.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bmqtst {

// ===========
// class Table
// ===========

/// @class Table
/// @brief Class for aggregation and pretty printing simple stats.
///
/// This class provides a mechanism to create and format tabular data with
/// automatic column width calculation and aligned output.
class Table {
  public:
    // FORWARD DECLARATIONS
    class ColumnView;
    friend class ColumnView;

  private:
    // DATA

    /// Allocator used to supply memory
    bslma::Allocator* d_allocator_p;

    /// 2-dimensional table with values presented as `bsl::string`,
    /// the column index is first, the row index is the second.
    /// The 0-th row contains column labels.
    bsl::vector<bsl::vector<bsl::string> > d_columns;

    /// The mapping between column title and ColumnView-s
    bsl::map<bsl::string, ColumnView> d_views;

    // ACCESSORS

    /// @brief Return the specified text padded to the specified width.
    /// @param text The text to pad
    /// @param width The desired width
    /// @return The padded string with spaces prepended
    bsl::string pad(bsl::string_view text, size_t width) const;

  public:
    // PUBLIC TYPES

    /// @class ColumnView
    /// @brief View class for manipulating a specific column in the table.
    ///
    /// This class provides a handle to a specific column within a Table,
    /// allowing values to be inserted into that column.
    class ColumnView {
      private:
        // PRIVATE TYPES

        /// Reference to the parent table
        Table& d_table;

        /// The index of this column in the parent table
        size_t d_columnIndex;

      public:
        // CREATORS

        /// @brief Create a ColumnView for the specified table and column
        /// index.
        /// @param table The parent table
        /// @param columnIndex The index of the column in the parent table
        explicit ColumnView(Table& table, size_t columnIndex);

        // MANIPULATORS

        /// @brief Insert the specified value to the end of the column.
        /// @param value The string value to insert
        void insertValue(const bsl::string& value);

        /// @brief Insert the specified value to the end of the column.
        /// @param value The unsigned 64-bit value to insert
        void insertValue(const bsls::Types::Uint64& value);
    };

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Table, bslma::UsesBslmaAllocator)

    // CREATORS

    /// @brief Create a Table using the optionally specified allocator.
    /// @param allocator The allocator to use for memory allocation
    explicit Table(bslma::Allocator* allocator = 0);

    // MANIPULATORS

    /// @brief Return a ColumnView manipulator for the specified column title.
    /// @param columnTitle The title of the column to access
    /// @return A ColumnView object for manipulating the column
    ///
    /// Note: we return ColumnView by value, not by reference.  If we return
    ///       a reference to an object in the stored map, the reference can
    ///       become invalid if we continue to add new nodes to the map.
    ///
    /// Guarantee: each ColumnView returned before is valid as manipulator to
    ///            its column while the parent Table object is valid.
    ColumnView column(bsl::string_view columnTitle);

    // ACCESSORS

    /// @brief Print the stored data as a pretty table to the specified stream.
    /// @param os The output stream to print to
    void print(bsl::ostream& os) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------
// class Table::ColumnView
// -----------------------

inline Table::ColumnView::ColumnView(Table& table, size_t columnIndex)
: d_table(table)
, d_columnIndex(columnIndex)
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace

#endif
