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

// mwcst_table.cpp                                                    -*-C++-*-
#include <mwcst_table.h>

#include <mwcscm_version.h>

namespace BloombergLP {
namespace mwcst {

// -----------
// class Table
// -----------

// CREATORS
Table::Table(bslma::Allocator* allocator)
: d_schema(allocator)
, d_records(allocator)
{
}

// MANIPULATORS
TableSchema& Table::schema()
{
    return d_schema;
}

TableRecords& Table::records()
{
    return d_records;
}

// ACCESSORS
int Table::numColumns() const
{
    return d_schema.numColumns();
}

int Table::numRows() const
{
    return d_records.numRecords();
}

bslstl::StringRef Table::columnName(int column) const
{
    return d_schema.column(column).name();
}

void Table::value(Value* value, int row, int column) const
{
    const TableRecords::Record& rec = d_records.record(row);
    d_schema.column(column).evaluate(value,
                                     rec.context(),
                                     rec.level(),
                                     rec.type());
}

}  // close package namespace
}  // close enterprise namespace
