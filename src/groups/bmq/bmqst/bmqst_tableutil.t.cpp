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

#include <bmqst_testutil.h>

#include <bmqst_tableinfoprovider.h>
#include <bmqu_memoutstream.h>
#include <bslma_testallocator.h>
#include <bsls_assert.h>

#include <bsl_iostream.h>
#include <bsl_sstream.h>

using namespace BloombergLP;
using namespace bsl;

//=============================================================================
//                                  TEST PLAN
//-----------------------------------------------------------------------------
//                              *** Overview ***
//
// TODO
//
// ----------------------------------------------------------------------------
// [  ] TODO
// ----------------------------------------------------------------------------
// [ 1] Breathing Test
//=============================================================================
//                      STANDARD BDE ASSERT TEST MACROS
//-----------------------------------------------------------------------------
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static int testStatus = 0;

//=============================================================================
//                       STANDARD BDE TEST DRIVER MACROS
//-----------------------------------------------------------------------------

#define P BSLS_BSLTESTUTIL_P    // Print identifier and value.
#define L_ BSLS_BSLTESTUTIL_L_  // current Line number

//=============================================================================
//                      GLOBAL HELPER FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------

namespace {

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
//                                MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char* argv[])
// NOLINTBEGIN(*-avoid-c-arrays,*-magic-numbers,performance-avoid-endl)
{
    // NOLINTNEXTLINE(cert-err34-c,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    int test    = argc > 1 ? atoi(argv[1]) : 0;
    int verbose = argc > 2;

    cout << "TEST " << __FILE__ << " CASE " << test << endl;

    // Use test allocator
    bslma::TestAllocator testAllocator;
    testAllocator.setNoAbort(true);
    bslma::Default::setDefaultAllocatorRaw(&testAllocator);

    switch (test) {
    case 0:
    case 4: {
        // --------------------------------------------------------------------
        // PRINT CSV TEST
        //
        // Concerns:
        //  That 'outputToVector' correctly dumps the specified table
        //
        // Plan:
        //  Use a table of tables to be described by a TableInfoProvider, and
        //  make sure resulting output vector is correct.
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl
                 << "PRINT CSV TEST" << endl
                 << "==============" << endl;

        const struct TestData {
            int         d_line;
            const char* d_header;
            const char* d_rows[100];
            const char* d_expected;
        } DATA[] = {
            {L_, "a b c", {"1 2 3", "4 5 6"}, "a,b,c\n1,2,3\n4,5,6\n"},
        };
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        const int NUM_DATA = sizeof(DATA) / sizeof(*DATA);

        // NOLINTBEGIN(performance-avoid-endl)
        for (int dataIdx = 0; dataIdx < NUM_DATA; ++dataIdx) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            const TestData& data = DATA[dataIdx];
            const int       LINE = data.d_line;

            P(LINE);

            TestTableInfoProvider tip;

            tip.addHeaderLevel(bmqst::TestUtil::stringVector(data.d_header));

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            const char* const* rows = data.d_rows;
            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            while (*rows) {
                tip.addRow(bmqst::TestUtil::stringVector(*rows));
                ++rows;
            }
            // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

            bmqu::MemOutStream stream;
            bmqst::TableUtil::printCsv(stream, tip);
            ASSERT_EQUALS(stream.str(), data.d_expected);
        }
        // NOLINTEND(performance-avoid-endl)

    } break;
    case 3: {
        // --------------------------------------------------------------------
        // USAGE EXAMPLE
        //
        // Concerns:
        //   That the usage example builds and works
        //
        // Plan:
        //   Copy usage example
        //
        // Testing:
        //   Usage Example
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl << "USAGE EXAMPLE" << endl << "=============" << endl;

        TestTableInfoProvider tip;
        tip.addHeaderLevel(bmqst::TestUtil::stringVector("a b c"));
        tip.addRow(bmqst::TestUtil::stringVector("1 2 3"));
        tip.addRow(bmqst::TestUtil::stringVector("4 5 6"));
        bmqst::TableUtil::printTable(bsl::cout, tip);
    } break;
        /* TODO fix this test once bmqst::TestTable is written
case 3: {
  // --------------------------------------------------------------------
  // PRINT CSV TEST
  //
  // Concerns:
  //  That 'printCsv' correctly prints the table as csv.
  //
  // Plan:
  //  Use a table of tables to be described by a TableInfoProvider, and
  //  the expected csv output for that table.
  // --------------------------------------------------------------------

  if (verbose) cout << endl
                    << "PRINT CSV TEST" << endl
                    << "==============" << endl;

  const struct TestData {
      int d_line;
      const char *d_header;
      const char *d_rows[100];
      const char *d_expected;
  } DATA[] = {
      { L_, "a b c", { "1 2 3", "4 5 6" },
          "a,b,c\n"
          "1,2,3\n"
          "4,5,6"
      },
  };
  const int NUM_DATA = sizeof(DATA)/sizeof(*DATA);

  for (int dataIdx = 0; dataIdx < NUM_DATA; ++dataIdx) {
      const TestData& data = DATA[dataIdx];
      const int LINE = data.d_line;

      P(LINE);

      TestTableInfoProvider tip;
      tip.addHeaderLevel(bmqst::TestUtil::stringVector(data.d_header));
      const char * const *rows = data.d_rows;
      while (*rows) {
          tip.addRow(bmqst::TestUtil::stringVector(*rows));
          ++rows;
      }

      const char *expected = data.d_expected;

      bsl::ostringstream output;
      int ret = TableUtil::printCsv(output, tip);
      ASSERT_EQUALS(ret, 0);
      ASSERT_EQUALS(output.str(), expected);
  }
} break;
*/
    case 2: {
        // --------------------------------------------------------------------
        // OUTPUT TO VECTOR TEST
        //
        // Concerns:
        //  That 'outputToVector' correctly dumps the specified table
        //
        // Plan:
        //  Use a table of tables to be described by a TableInfoProvider, and
        //  make sure resulting output vector is correct.
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl
                 << "OUTPUT TO VECTOR TEST" << endl
                 << "=====================" << endl;

        const struct TestData {
            int         d_line;
            const char* d_header;
            const char* d_rows[100];
        } DATA[] = {
            {L_, "a b c", {"1 2 3", "4 5 6"}},
        };
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        const int NUM_DATA = sizeof(DATA) / sizeof(*DATA);

        // NOLINTBEGIN(performance-avoid-endl)
        for (int dataIdx = 0; dataIdx < NUM_DATA; ++dataIdx) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            const TestData& data = DATA[dataIdx];
            const int       LINE = data.d_line;

            P(LINE);

            TestTableInfoProvider tip;

            bsl::vector<bsl::vector<bsl::string> > expected;
            expected.push_back(bmqst::TestUtil::stringVector(data.d_header));
            tip.addHeaderLevel(expected.back());

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            const char* const* rows = data.d_rows;
            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            while (*rows) {
                expected.push_back(bmqst::TestUtil::stringVector(*rows));
                tip.addRow(expected.back());
                ++rows;
            }
            // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

            bsl::vector<bsl::vector<bsl::string> > output;
            int ret = bmqst::TableUtil::outputToVector(&output, tip);
            ASSERT_EQUALS(ret, 0);
            ASSERT_EQUALS(output, expected);
        }
        // NOLINTEND(performance-avoid-endl)

    } break;
    case 1: {
        // --------------------------------------------------------------------
        // BREATHING TEST
        //
        // Concerns:
        //   Exercise the basic functionality of the component.
        //
        // Plan:
        //   TODO
        //
        // Testing:
        //   Basic functionality
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl
                 << "BREATHING TEST" << endl
                 << "==============" << endl;

    } break;
    default: {
        cerr << "WARNING: CASE '" << test << "' NOT FOUND." << endl;
        testStatus = -1;
    }
    }

    if (testStatus != 255) {
        if ((testAllocator.numMismatches() != 0) ||
            (testAllocator.numBytesInUse() != 0)) {
            bsl::cout << "*** Error " << __FILE__ << "(" << __LINE__
                      << "): test allocator: " << '\n';
            testAllocator.print();
            testStatus++;
        }
    }

    if (testStatus > 0) {
        cerr << "Error, non-zero test status = " << testStatus << "." << endl;
    }
    return testStatus;
}
// NOLINTEND(*-avoid-c-arrays,*-magic-numbers,performance-avoid-endl)
