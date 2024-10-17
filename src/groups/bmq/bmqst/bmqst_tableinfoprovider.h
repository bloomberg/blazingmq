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

// bmqst_tableinfoprovider.h -*-C++-*-
#ifndef INCLUDED_BMQST_TABLEINFOPROVIDER
#define INCLUDED_BMQST_TABLEINFOPROVIDER

//@PURPOSE: Provide a protocol for an object that describes a printable table
//
//@CLASSES:
// bmqst::TableInfoProvider
//
//@SEE_ALSO: bmqst_tableutil
//
//@DESCRIPTION:
// This component defines a pure protocol, 'bmqst::TableInfoProvider', which
// provides all the information necessary for 'bmqst::TableUtil' to print
// any arbitrary table.
//
// A 'TableInfoProvider' can describe a table with an arbitrary number of
// header rows. A header cell in a header row above the first one simply
// encompasses one or more of the columns below it.
//
/// Usage Example
///-------------
// See the usage example of 'bmqst::TableUtil' for a simple
// implementation.

#ifndef INCLUDED_BSL_OSTREAM
#include <bsl_ostream.h>
#endif

namespace BloombergLP {
namespace bmqst {

// =======================
// class TableInfoProvider
// =======================

/// Protocol for an object that describes a table to be printed by
/// `bmqst::PrintTableUtil`.
class TableInfoProvider {
  public:
    // CREATORS
    virtual ~TableInfoProvider();

    // ACCESSORS

    /// Return the number of data rows in the table
    virtual int numRows() const = 0;

    /// Return the number of columns at the specified header `level`.
    /// `level == 0` is the first (bottom) header row, and is the actual
    /// number of columns in the table.  `level == 1` is the row above it,
    /// and so on.
    virtual int numColumns(int level) const = 0;

    /// Return `true` if the table has a title, and `false` otherwise.
    virtual bool hasTitle() const = 0;

    /// Return the number of header rows in the table.  This must return at
    /// least `1`.
    virtual int numHeaderLevels() const = 0;

    /// Return the printed length of the value in the specified `column` of
    /// the specified `row` in the table.
    virtual int getValueSize(int row, int column) const = 0;

    /// Print the value in the specified `column` of the specified `row` to
    /// the specified `stream` with the specified `width`.  The width of
    /// the `stream` is already set when this is called as a convenience.
    /// Return the `stream`.
    virtual bsl::ostream&
    printValue(bsl::ostream& stream, int row, int column, int width) const = 0;

    /// Return the printed length of the header cell in the specified
    /// `column` at the specified header `level`.
    virtual int getHeaderSize(int level, int column) const = 0;

    /// Return the header cell index in header `level + 1` which is above
    /// the specified `column` in the specified header `level`.
    virtual int getParentHeader(int level, int column) const = 0;

    /// Print the title of this table to the specified `stream`.  Return
    /// the `stream`.
    virtual bsl::ostream& printTitle(bsl::ostream& stream) const = 0;

    /// Print the header of the specified `column` at the specified
    /// header `level` to the specified `stream` with the specified
    /// `width`.  The width of the `stream` is already set when this is
    /// called as a convenience.  Return the `stream`.
    virtual bsl::ostream& printHeader(bsl::ostream& stream,
                                      int           level,
                                      int           column,
                                      int           width) const = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
