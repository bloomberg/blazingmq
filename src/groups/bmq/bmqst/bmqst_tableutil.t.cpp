// Copyright 2023 Bloomberg Finance L.P.
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

#include <bmqst_tableutil.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

#include <bmqst_tableinfoprovider.h>

// BDE
#include <bdlb_tokenizer.h>
#include <bsl_iostream.h>
#include <bsl_vector.h>
#include <bsls_assert.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

namespace {

/// @brief Append the space-separated tokens of `s` to `out`.
/// @param[out] out  Vector to append tokens to.
/// @param      s    Space-separated input.
void stringVector(bsl::vector<bsl::string>* out, bsl::string_view s)
{
    bdlb::Tokenizer tokenizer(s, " ");
    for (bdlb::TokenizerIterator iter = tokenizer.begin();
         iter != tokenizer.end();
         ++iter) {
        out->emplace_back(*iter);
    }
}

// ===========================
// class TestTableInfoProvider
// ===========================

class TestTableInfoProvider : public bmqst::TableInfoProvider {
  private:
    typedef bsl::vector<bsl::string> Row;
    typedef bsl::vector<Row>         Table;

    Table d_headers;
    Table d_table;

    TestTableInfoProvider(const TestTableInfoProvider&);
    TestTableInfoProvider& operator=(const TestTableInfoProvider&);

  public:
    explicit TestTableInfoProvider(bslma::Allocator* basicAllocator = 0)
    : d_headers(basicAllocator)
    , d_table(basicAllocator)
    {
    }

    ~TestTableInfoProvider() BSLS_KEYWORD_OVERRIDE {}

    void addRow(const Row& row) { d_table.push_back(row); }

    void addHeaderLevel(const Row& row) { d_headers.push_back(row); }

    int numRows() const BSLS_KEYWORD_OVERRIDE
    {
        return static_cast<int>(d_table.size());
    }

    int numColumns(int) const BSLS_KEYWORD_OVERRIDE
    {
        BSLS_ASSERT(d_headers.size() > 0);
        return static_cast<int>(d_headers[0].size());
    }

    bool hasTitle() const BSLS_KEYWORD_OVERRIDE { return false; }

    int numHeaderLevels() const BSLS_KEYWORD_OVERRIDE
    {
        return static_cast<int>(d_headers.size());
    }

    int getValueSize(int row, int column) const BSLS_KEYWORD_OVERRIDE
    {
        return static_cast<int>(d_table[row][column].length());
    }

    bsl::ostream& printValue(bsl::ostream& stream,
                             int           row,
                             int           column,
                             int) const BSLS_KEYWORD_OVERRIDE
    {
        return stream << d_table[row][column];
    }

    int getHeaderSize(int level, int column) const BSLS_KEYWORD_OVERRIDE
    {
        const Row& row = d_headers[level];
        for (size_t i = 0; i < row.size(); ++i) {
            if (row[i] == "-") {
                continue;
            }
            if (column == 0) {
                return static_cast<int>(row[i].length());
            }
            --column;
        }
        BSLS_ASSERT(false);
        return -1;
    }

    int getParentHeader(int level, int column) const BSLS_KEYWORD_OVERRIDE
    {
        const Row& row = d_headers[level + 1];
        while (row[column] != "-") {
            --column;
        }
        BSLS_ASSERT(column >= 0);
        int index = -1;
        for (; column >= 0; --column) {
            if (row[column] != "-") {
                index++;
            }
        }
        return index;
    }

    bsl::ostream& printTitle(bsl::ostream& stream) const BSLS_KEYWORD_OVERRIDE
    {
        return stream;
    }

    bsl::ostream& printHeader(bsl::ostream& stream,
                              int           level,
                              int           column,
                              int) const BSLS_KEYWORD_OVERRIDE
    {
        const Row& row = d_headers[level];
        for (size_t i = 0; i < row.size(); ++i) {
            if (row[i] == "-") {
                continue;
            }
            if (column == 0) {
                return stream << row[i];
            }
            --column;
        }
        BSLS_ASSERT(false);
        return stream;
    }
};

}  // close unnamed namespace

//=============================================================================
//                                    TESTS
//-----------------------------------------------------------------------------

static void test1_usageExample()
{
    bmqtst::TestHelper::printTestName("USAGE EXAMPLE");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

    TestTableInfoProvider    tip(alloc);
    bsl::vector<bsl::string> row(alloc);

    stringVector(&row, "a b c");
    tip.addHeaderLevel(row);

    row.clear();
    stringVector(&row, "1 2 3");
    tip.addRow(row);

    row.clear();
    stringVector(&row, "4 5 6");
    tip.addRow(row);
    bmqst::TableUtil::printTable(bsl::cout, tip);
}

//=============================================================================
//                                MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 1: test1_usageExample(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
