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

// bmqtst_table.cpp                                                   -*-C++-*-
#include <bmqtst_table.h>

// BDE
#include <bsl_algorithm.h>
#include <bsl_iostream.h>
#include <bsl_utility.h>
#include <bslma_default.h>

namespace BloombergLP {
namespace bmqtst {

// -----------
// class Table
// -----------

// CREATORS
Table::Table(bslma::Allocator* allocator)
: d_allocator_p(bslma::Default::allocator(allocator))
, d_columns(d_allocator_p)
, d_views(d_allocator_p)
{
    // NOTHING
}

// CLASS METHODS
bsl::string Table::pad(bsl::string_view text, size_t width) const
{
    BSLS_ASSERT(text.length() <= width);
    return bsl::string(width - text.length(), ' ', d_allocator_p) + text;
}

// MANIPULATORS
Table::ColumnView Table::column(bsl::string_view columnTitle)
{
    bsl::string str(columnTitle, d_allocator_p);
    if (d_views.find(str) == d_views.end()) {
        d_columns.resize(d_columns.size() + 1);
        d_columns.back().push_back(str);
        d_views.insert(
            bsl::make_pair(str, ColumnView(*this, d_columns.size() - 1)));
    }
    return d_views.find(str)->second;
}

// ACCESSORS
void Table::print(bsl::ostream& os) const
{
    // PRECONDITIONS
    if (d_columns.empty()) {
        return;  // RETURN
    }
    const size_t      rows = d_columns.front().size();
    const bsl::string separator(" | ", d_allocator_p);
    for (size_t columnId = 0; columnId < d_columns.size(); columnId++) {
        // Expect all columns to have the same number of rows
        BSLS_ASSERT(rows == d_columns.at(columnId).size());
    }

    // For each column, calculate the longest stored value and remember it
    // as this column's width
    bsl::vector<size_t> paddings(d_allocator_p);
    paddings.resize(d_columns.size(), 0);
    for (size_t columnId = 0; columnId < d_columns.size(); columnId++) {
        const bsl::vector<bsl::string>& column = d_columns.at(columnId);

        size_t& maxLen = paddings.at(columnId);
        for (size_t rowId = 0; rowId < rows; rowId++) {
            maxLen = bsl::max(maxLen, column.at(rowId).length());
        }
    }

    // Print the table using precalculated column widths
    for (size_t rowId = 0; rowId < rows; rowId++) {
        for (size_t columnId = 0; columnId < d_columns.size(); columnId++) {
            if (columnId > 0) {
                os << separator;
            }
            os << pad(d_columns.at(columnId).at(rowId), paddings.at(columnId));
        }
        os << bsl::endl;

        // Print horizontal line after the initial row
        if (rowId == 0) {
            size_t lineWidth = 0;
            for (bsl::vector<size_t>::const_iterator it = paddings.cbegin();
                 it != paddings.cend();
                 ++it) {
                lineWidth += *it;
            }
            lineWidth += separator.size() * (paddings.size() - 1);
            os << bsl::string(lineWidth, '=', d_allocator_p) << bsl::endl;
        }
    }
}

// -----------------------
// class Table::ColumnView
// -----------------------

// MANIPULATORS
void Table::ColumnView::insertValue(const bsl::string& value)
{
    d_table.d_columns.at(d_columnIndex).push_back(value);
}

void Table::ColumnView::insertValue(const bsls::Types::Uint64& value)
{
    d_table.d_columns.at(d_columnIndex).push_back(bsl::to_string(value));
}

}  // close package namespace
}  // close enterprise namespace
