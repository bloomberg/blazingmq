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

// mwcst_testtableinfoprovider.h -*-C++-*-
#ifndef INCLUDED_MWCST_TESTTABLEINFOPROVIDER
#define INCLUDED_MWCST_TESTTABLEINFOPROVIDER

//@PURPOSE: Provide a test implementation of 'mwcst::TableInfoProvider'
//
//@CLASSES:
// mwcst::TestTableInfoProvider
//
//@SEE_ALSO:
//
//@DESCRIPTION: This component defines a mechanism,
// 'mwcst::TestTableInfoProvider', which is a test implementation of the
// 'mwcst::TableInfoProvider' protocol for use in test drivers.  It allows the
// user to specify the values returned by the TIP.

#include <bsl_string.h>
#include <bsl_vector.h>
#include <mwcst_tableinfoprovider.h>

namespace BloombergLP {
namespace mwcst {

// ===========================
// class TestTableInfoProvider
// ===========================

/// Test implementation of `mwcst::TableInfoProvider`
class TestTableInfoProvider : public mwcst::TableInfoProvider {
  private:
    // PRIVATE TYPES
    typedef bsl::vector<bsl::string> Row;
    typedef bsl::vector<Row>         Table;

    // DATA
    Table d_headers;
    Table d_table;

    // NOT IMPLEMENTED
    TestTableInfoProvider(const TestTableInfoProvider&);
    TestTableInfoProvider& operator=(const TestTableInfoProvider&);

  public:
    // CREATORS
    TestTableInfoProvider(bslma::Allocator* basicAllocator = 0);
    ~TestTableInfoProvider() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Clear rows and headers;
    void reset();

    /// Append a row of data to the table described by this TIP.
    void addRow(const Row& row);

    /// Add a new header level to the table described by this TIP.  The
    /// parent of a cell `i` in the previous level will be the first cell in
    /// the new level with index `<= i` not equal to `-`.
    void addHeaderLevel(const Row& row);

    // ACCESSORS

    // TableInfoProvider
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

}  // close package namespace
}  // close enterprise namespace

#endif
