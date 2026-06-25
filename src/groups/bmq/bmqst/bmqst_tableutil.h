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

#ifndef INCLUDED_BMQST_TABLEUTIL
#define INCLUDED_BMQST_TABLEUTIL

//@PURPOSE: Provide a utility for printing tables.
//
//@CLASSES:
// bmqst::TableUtil
//
//@DESCRIPTION: This component defines a utility, 'bmqst::TableUtil',
// for printing tables described by a 'bmqst::TableInfoProvider'.
//
/// Usage Example
///-------------
// Given a 'bmqst::TableInfoProvider' implementation, we can print a table:
//..
//  TestTableInfoProvider provider;
//  provider.addHeaderLevel({"a", "b", "c"});
//  provider.addRow({"1", "2", "3"});
//  provider.addRow({"4", "5", "6"});
//  bmqst::TableUtil::printTable(bsl::cout, provider);
//..
// The above produces:
//..
//  a| b| c
//  -+--+--
//  1| 2| 3
//  4| 5| 6
//..

#include <bsl_iosfwd.h>

namespace BloombergLP {

namespace bmqst {

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
};

}  // close package namespace
}  // close enterprise namespace

#endif
