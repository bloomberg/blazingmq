// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqt_sessionoptions.t.cpp                                          -*-C++-*-
#include <bmqt_sessionoptions.h>
#include <z_bmqt_sessionoptions.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <bsl_ios.h>
#include <bsl_string_view.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_sessionOptions()
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    // Create default sessionOptions
    z_bmqt_SessionOptions* sessionOptions;
    z_bmqt_SessionOptions__create(&sessionOptions);
    bmqt::SessionOptions sessionOptions_cpp(s_allocator_p);

    // Make sure 'k_BROKER_DEFAULT_PORT' and the default brokerUri are in sync
    {
        PV("CHECKING brokerUri()");

        bsl::string_view result = z_bmqt_SessionOptions__brokerUri(
            sessionOptions);
        ASSERT_EQ(sessionOptions_cpp.brokerUri(), result);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_sessionOptions(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
