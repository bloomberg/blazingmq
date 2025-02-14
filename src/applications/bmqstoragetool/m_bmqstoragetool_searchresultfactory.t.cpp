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

// bmqstoragetool
#include <m_bmqstoragetool_filemanager.h>
#include <m_bmqstoragetool_filemanagermock.h>
#include <m_bmqstoragetool_searchresultfactory.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace m_bmqstoragetool;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the component.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");
    // Empty parameters
    CommandLineArguments arguments(bmqtst::TestHelperUtil::allocator());
    Parameters params(arguments, bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator()) FileManagerMock(),
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());

    // Create printer
    bsl::shared_ptr<Printer> printer = createPrinter(
        params.d_printMode,
        resultStream,
        bmqtst::TestHelperUtil::allocator());

    // Create payload dumper
    bslma::ManagedPtr<PayloadDumper> payloadDumper;
    if (params.d_dumpPayload) {
        payloadDumper.load(
            new (*bmqtst::TestHelperUtil::allocator())
                PayloadDumper(resultStream,
                              fileManager->dataFileIterator(),
                              params.d_dumpLimit,
                              bmqtst::TestHelperUtil::allocator()),
            bmqtst::TestHelperUtil::allocator());
    }

    bsl::shared_ptr<SearchResult> searchResult =
        SearchResultFactory::createSearchResult(
            &params,
            fileManager,
            printer,
            payloadDumper,
            bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT(dynamic_cast<SearchResult*>(searchResult.get()) != 0);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
