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

// bmqst_testtableinfoprovider.cpp -*-C++-*-
#include <bmqst_testtableinfoprovider.h>

#include <bmqscm_version.h>
#include <bsl_ostream.h>

namespace BloombergLP {
namespace bmqst {

// ---------------------------
// class TestTableInfoProvider
// ---------------------------

// CREATORS
TestTableInfoProvider::TestTableInfoProvider(bslma::Allocator* basicAllocator)
: d_headers(basicAllocator)
, d_table(basicAllocator)
{
}

TestTableInfoProvider::~TestTableInfoProvider()
{
}

// MANIPULATORS
void TestTableInfoProvider::reset()
{
    d_headers.clear();
    d_table.clear();
}

void TestTableInfoProvider::addRow(const Row& row)
{
    d_table.push_back(row);
}

void TestTableInfoProvider::addHeaderLevel(const Row& row)
{
    d_headers.push_back(row);
}

// ACCESSORS
int TestTableInfoProvider::numRows() const
{
    return static_cast<int>(d_table.size());
}

int TestTableInfoProvider::numColumns(int /*level*/) const
{
    BSLS_ASSERT(d_headers.size() > 0);
    return static_cast<int>(d_headers[0].size());
}

bool TestTableInfoProvider::hasTitle() const
{
    return false;
}

int TestTableInfoProvider::numHeaderLevels() const
{
    return static_cast<int>(d_headers.size());
}

int TestTableInfoProvider::getValueSize(int row, int column) const
{
    return static_cast<int>(d_table[row][column].length());
}

bsl::ostream& TestTableInfoProvider::printValue(bsl::ostream& stream,
                                                int           row,
                                                int           column,
                                                int /*width*/) const
{
    return stream << d_table[row][column];
}

int TestTableInfoProvider::getHeaderSize(int level, int column) const
{
    const Row& row = d_headers[level];

    for (size_t i = 0; i < row.size(); ++i) {
        if (row[i] == "-") {
            continue;
        }

        if (column == 0) {
            return static_cast<int>(row[i].length());
        }
        else {
            --column;
        }
    }

    BSLS_ASSERT(false);
    return -1;
}

int TestTableInfoProvider::getParentHeader(int level, int column) const
{
    const Row& row = d_headers[level + 1];

    while (row[column] != "-") {
        --column;
    }

    BSLS_ASSERT(column >= 0);

    // Column is now the cell in 'row' that is the parent for (level, column)

    int index = -1;
    for (; column >= 0; --column) {
        if (row[column] == "-") {
            continue;
        }

        if (row[column] != "-") {
            index++;
        }
    }

    return index;
}

bsl::ostream& TestTableInfoProvider::printTitle(bsl::ostream& stream) const
{
    return stream;
}

bsl::ostream& TestTableInfoProvider::printHeader(bsl::ostream& stream,
                                                 int           level,
                                                 int           column,
                                                 int /*width*/) const
{
    const Row& row = d_headers[level];

    for (size_t i = 0; i < row.size(); ++i) {
        if (row[i] == "-") {
            continue;
        }

        if (column == 0) {
            return stream << row[i];
        }
        else {
            --column;
        }
    }

    BSLS_ASSERT(false);
    return stream;
}

}  // close package namespace
}  // close enterprise namespace
