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

// mwcst_tableutil.t.cpp -*-C++-*-

#include <mwcst_tableutil.h>

#include <mwcst_testutil.h>

#include <bslma_testallocator.h>
#include <bsls_assert.h>
#include <mwcst_tableinfoprovider.h>
#include <mwcst_testtableinfoprovider.h>
#include <mwcu_memoutstream.h>

#include <bsl_iostream.h>
#include <bsl_set.h>
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
static int testStatus = 0;

//=============================================================================
//                       STANDARD BDE TEST DRIVER MACROS
//-----------------------------------------------------------------------------

#define P BSLS_BSLTESTUTIL_P    // Print identifier and value.
#define L_ BSLS_BSLTESTUTIL_L_  // current Line number

//=============================================================================
//                      GLOBAL HELPER FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------

// ========================
// class SimpleInfoProvider
// ========================

class SimpleInfoProvider : public mwcst::TableInfoProvider {
    // ACCESSORS
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

int SimpleInfoProvider::numRows() const
{
    return 5;
}

int SimpleInfoProvider::numColumns(int /*level*/) const
{
    return 6;
}

bool SimpleInfoProvider::hasTitle() const
{
    return false;
}

int SimpleInfoProvider::numHeaderLevels() const
{
    return 1;
}

int SimpleInfoProvider::getValueSize(int /*row*/, int column) const
{
    if (column == 0) {
        return 1;
    }
    else {
        return bsl::strlen("(#x#)");
    }
}

bsl::ostream& SimpleInfoProvider::printValue(bsl::ostream& stream,
                                             int           row,
                                             int           column,
                                             int /*width*/) const
{
    if (column == 0) {
        return stream << bsl::right << row;
    }
    else {
        bsl::ostringstream ss;
        ss << '(' << row << 'x' << column - 1 << ')';
        return stream << bsl::right << ss.str();
    }
}

int SimpleInfoProvider::getHeaderSize(int /*level*/, int column) const
{
    return column == 0 ? 0 : 1;
}

int SimpleInfoProvider::getParentHeader(int /*level*/, int /*column*/) const
{
    return -1;
}

bsl::ostream& SimpleInfoProvider::printTitle(bsl::ostream& stream) const
{
    return stream;
}

bsl::ostream& SimpleInfoProvider::printHeader(bsl::ostream& stream,
                                              int /*level*/,
                                              int column,
                                              int /*width*/) const
{
    if (column == 0) {
        return stream << "";
    }
    else {
        return stream << bsl::right << column - 1;
    }
}

//=============================================================================
//                                MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
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
        const int NUM_DATA = sizeof(DATA) / sizeof(*DATA);

        for (int dataIdx = 0; dataIdx < NUM_DATA; ++dataIdx) {
            const TestData& data = DATA[dataIdx];
            const int       LINE = data.d_line;

            P(LINE);

            mwcst::TestTableInfoProvider tip;

            tip.addHeaderLevel(mwcst::TestUtil::stringVector(data.d_header));

            const char* const* rows = data.d_rows;
            while (*rows) {
                tip.addRow(mwcst::TestUtil::stringVector(*rows));
                ++rows;
            }

            mwcu::MemOutStream stream;
            mwcst::TableUtil::printCsv(stream, tip);
            ASSERT_EQUALS(stream.str(), data.d_expected);
        }

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

        SimpleInfoProvider provider;
        mwcst::TableUtil::printTable(bsl::cout, provider);
    } break;
        /* TODO fix this test once mwcst::TestTable is written
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
      tip.addHeaderLevel(mwcst::TestUtil::stringVector(data.d_header));
      const char * const *rows = data.d_rows;
      while (*rows) {
          tip.addRow(mwcst::TestUtil::stringVector(*rows));
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
        const int NUM_DATA = sizeof(DATA) / sizeof(*DATA);

        for (int dataIdx = 0; dataIdx < NUM_DATA; ++dataIdx) {
            const TestData& data = DATA[dataIdx];
            const int       LINE = data.d_line;

            P(LINE);

            mwcst::TestTableInfoProvider tip;

            bsl::vector<bsl::vector<bsl::string> > expected;
            expected.push_back(mwcst::TestUtil::stringVector(data.d_header));
            tip.addHeaderLevel(expected.back());

            const char* const* rows = data.d_rows;
            while (*rows) {
                expected.push_back(mwcst::TestUtil::stringVector(*rows));
                tip.addRow(expected.back());
                ++rows;
            }

            bsl::vector<bsl::vector<bsl::string> > output;
            int ret = mwcst::TableUtil::outputToVector(&output, tip);
            ASSERT_EQUALS(ret, 0);
            ASSERT_EQUALS(output, expected);
        }

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
