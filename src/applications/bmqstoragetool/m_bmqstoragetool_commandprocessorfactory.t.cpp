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
#include <m_bmqstoragetool_commandprocessorfactory.h>
#include <m_bmqstoragetool_cslfileprocessor.h>
#include <m_bmqstoragetool_filemanager.h>
#include <m_bmqstoragetool_filemanagermock.h>
#include <m_bmqstoragetool_journalfileprocessor.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

#include <bsl_iostream.h>
#include <bsl_ostream.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace m_bmqstoragetool;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the component. Check that
//   `JournalFileProcessor` object is created by defauilt.
//
// Testing:
//   createCommandProcessor()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");
    // Empty parameters
    CommandLineArguments arguments(bmqtst::TestHelperUtil::allocator());
    Parameters params(arguments, bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator()) FileManagerMock(),
        bmqtst::TestHelperUtil::allocator());

    bslma::ManagedPtr<CommandProcessor> cmdProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            bsl::cout,
            bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT(dynamic_cast<JournalFileProcessor*>(cmdProcessor.get()) !=
                  0);
}

static void test2_cslProcessorTest()
// ------------------------------------------------------------------------
// CSL FILE PROCESSOR TEST
//
// Concerns:
//   Check that `CslFileProcessor` object is created for CSL mode parameters.
//
// Testing:
//   createCommandProcessor()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CSL FILE PROCESSOR TEST");
    CommandLineArguments arguments(bmqtst::TestHelperUtil::allocator());
    Parameters params(arguments, bmqtst::TestHelperUtil::allocator());
    // CSL mode parameters
    params.d_cslMode = true;
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator()) FileManagerMock(),
        bmqtst::TestHelperUtil::allocator());

    bslma::ManagedPtr<CommandProcessor> cmdProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            bsl::cout,
            bmqtst::TestHelperUtil::allocator());
    ASSERT(dynamic_cast<CslFileProcessor*>(cmdProcessor.get()) != 0);
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
    case 2: test2_cslProcessorTest(); break;
    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
