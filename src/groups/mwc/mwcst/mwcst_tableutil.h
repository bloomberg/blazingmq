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

// mwcst_tableutil.h -*-C++-*-
#ifndef INCLUDED_MWCST_TABLEUTIL
#define INCLUDED_MWCST_TABLEUTIL

//@PURPOSE: Provide a set of functions for working with tables
//
//@CLASSES:
// mwcu::TableUtil
//
//@SEE_ALSO:
//
//@DESCRIPTION: This component defines a utility, 'mwcu::TableUtil',
//  containing functions for working with tables defined by a
//  'mwcu::TableInfoProvider'.
//
/// printTable
///---------
// This function prints a table of data to a stream by using a provided
// 'mwcu::TableInfoProvider' object which provides information about the table
// to be printed.  It supports printing tables with an arbitrary
// number of header rows, and ensures that all columns are sized correctly so
// all their values fit.
//
/// Usage Example
///-------------
// We will print a 5x5 table where each cell contains its coordinates in the
// form '(row x column)'.  To do this, we must define a 'TableInfoProvider'
// implementation that will handle this for us.
//..
//  class SimpleInfoProvider : public mwcu::TableInfoProvider {
//
//      // ACCESSORS
//      virtual int numRows() const
//      {
//          return 5;
//      }
//
//      virtual int numColumns(int level) const
//      {
//          return 6;
//      }
//
//      virtual int numHeaderLevels() const
//      {
//          return 1;
//      }
//
//      virtual int getValueSize(int row, int column) const
//      {
//          if (column == 0) {
//              return 1;
//          } else {
//              return bsl::strlen("(#x#)");
//          }
//      }
//
//      virtual bsl::ostream& printValue(bsl::ostream& stream,
//                                       int           row,
//                                       int           column,
//                                       int           width) const
//      {
//          if (column == 0) {
//              return stream << bsl::right << row;
//          } else {
//              bsl::ostringstream ss;
//              ss << '(' << row << 'x' << column - 1 << ')';
//              return stream << bsl::right << ss.str();
//          }
//      }
//
//
//      virtual int getHeaderSize(int level, int column) const
//      {
//          return column == 0 ? 0 : 1;
//      }
//
//      virtual int getParentHeader(int level, int column) const
//      {
//          return -1;
//      }
//
//      virtual bsl::ostream& printHeader(bsl::ostream& stream,
//                                        int           level,
//                                        int           column,
//                                        int           width) const
//      {
//          if (column == 0) {
//              return stream << "";
//          } else {
//              return stream << bsl::right << column - 1;
//          }
//      }
//  };
//..
// Now, we can use an object of this class to see what our table looks like.
//..
//  SimpleInfoProvider provider;
//  TableUtil::printTable(bsl::cout, provider);
//..
//  The following table should be printed to cout.
//..
//   |     0|     1|     2|     3|     4
//  -+------+------+------+------+------
//  0| (0x0)| (0x1)| (0x2)| (0x3)| (0x4)
//  1| (1x0)| (1x1)| (1x2)| (1x3)| (1x4)
//  2| (2x0)| (2x1)| (2x2)| (2x3)| (2x4)
//  3| (3x0)| (3x1)| (3x2)| (3x3)| (3x4)
//  4| (4x0)| (4x1)| (4x2)| (4x3)| (4x4)
//..

#ifndef INCLUDED_BSL_OSTREAM
#include <bsl_ostream.h>
#endif

#ifndef INCLUDED_BSL_STRING
#include <bsl_string.h>
#endif

#ifndef INCLUDED_BSL_VECTOR
#include <bsl_vector.h>
#endif

namespace BloombergLP {

namespace mwcst {
class BaseTable;
}

namespace mwcu {

// FORWARD DECLARATIONS
class TableInfoProvider;

// ================
// struct TableUtil
// ================

/// Utility functions for working with tables.
struct TableUtil {
    // CLASS METHODS

    /// Print the table described by the specified `info` to the specified
    /// `stream`.  Return `0` on success or a negative value if the `info`
    /// is inconsistent.
    static int printTable(bsl::ostream& stream, const TableInfoProvider& info);

    /// Output the table described by the specified `info` to the specified
    /// `dest` vector<vector<string>>.  Note that only the first header row
    /// will be output to `dest`, as (*dest)[0].  Return `0` on success or
    /// a negative value if the `info` is inconsistent.
    static int outputToVector(bsl::vector<bsl::vector<bsl::string> >* dest,
                              const TableInfoProvider&                info);

    /// Print the specified `table` to the specified
    /// `stream` as a csv with one line for the header, and one
    /// line per row.
    static void printCsv(bsl::ostream& stream, const mwcst::BaseTable& table);

    /// Print the specified `info` to the specified `stream` in CSV format
    /// with one line for the header, and one line per row.
    static void printCsv(bsl::ostream& stream, const TableInfoProvider& info);
};

}  // close package namespace
}  // close enterprise namespace

#endif
