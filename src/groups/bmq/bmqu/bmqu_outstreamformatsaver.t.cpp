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

// bmqu_outstreamformatsaver.t.cpp                                    -*-C++-*-
#include <bmqu_outstreamformatsaver.h>

// BMQU
#include <bmqu_memoutstream.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
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
// Plan:
//  1) Set formatting, restore format state with call to 'restore',  and
//     verify that format state was restored.
//  2) Set formatting, let the formatSaver go out of scope (and thus
//     destroy), and verify that format state was restored.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    const unsigned int k_FIFTY = 50;

    bmqu::MemOutStream obj(s_allocator_p);
    {
        bmqu::OutStreamFormatSaver fmtSaver(obj);

        // Set formatting and print
        obj << bsl::nouppercase << bsl::showbase << bsl::hex;
        obj << k_FIFTY;

        ASSERT_EQ(obj.str(), "0x32");

        // Restore formatting
        fmtSaver.restore();

        obj.reset();
        obj << k_FIFTY;

        ASSERT_EQ(obj.str(), "50");

        // Set formatting again and print
        obj.reset();
        obj << bsl::nouppercase << bsl::showbase << bsl::hex;
        obj << k_FIFTY;

        ASSERT_EQ(obj.str(), "0x32");
    }
    // NOTE: We expect that at the end of the inner scope above, 'fmtSaver' is
    //       is destroyed and the format state of 'obj' is restored.
    obj.reset();
    obj << k_FIFTY;

    ASSERT_EQ(obj.str(), "50");
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
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
