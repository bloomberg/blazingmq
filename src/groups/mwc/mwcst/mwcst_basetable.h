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

// mwcst_basetable.h -*-C++-*-
#ifndef INCLUDED_MWCST_BASETABLE
#define INCLUDED_MWCST_BASETABLE

//@PURPOSE: Provide a generic BaseTable protocol
//
//@CLASSES:
// mwcst::BaseTable
//
//@DESCRIPTION: This component defines a pure protocol, 'mwcst::BaseTable', for
// a
// generic 2-dimensional table of values.

#ifndef INCLUDED_BSL_STRING
#include <bsl_string.h>
#endif

namespace BloombergLP {

namespace mwcst {

// FORWARD DECLARATIONS
class Value;

// ===============
// class BaseTable
// ===============

/// Protocol for a generic table of `mwcst::Value`s
class BaseTable {
  public:
    // CREATORS
    virtual ~BaseTable();

    // MANIPULATORS

    // ACCESSORS

    /// Return the number of columns in the table.
    virtual int numColumns() const = 0;

    /// Return the number of rows in the table.
    virtual int numRows() const = 0;

    /// Return the name of the specified `column`.
    virtual bslstl::StringRef columnName(int column) const = 0;

    /// Load into the specified `value` the value in the specified `column`
    /// of the specified `row`.
    virtual void value(mwcst::Value* value, int row, int column) const = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
