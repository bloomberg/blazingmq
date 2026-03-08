// Copyright 2014-2025 Bloomberg Finance L.P.
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

// bmqt_authncredential.t.cpp                                   -*-C++-*-
#include <bmqt_authncredential.h>

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
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    struct Local {
        inline static bsl::vector<char>
        makeAuthnData(bslma::Allocator* alloc = 0)
        {
            bsl::vector<char> res(alloc);
            res.push_back('d');
            res.push_back('a');
            res.push_back('t');
            res.push_back('a');
            return res;
        }
    };

    PV("Test default constructor");
    {
        bmqt::AuthnCredential cred;
        ASSERT(cred.mechanism().empty());
        ASSERT(cred.data().empty());
    }

    PV("Test parameterized constructor");
    {
        bsl::string           mechanism("BMQ");
        bsl::vector<char>     data(Local::makeAuthnData());
        bmqt::AuthnCredential cred(mechanism, data);

        ASSERT(cred.mechanism() == mechanism);
        ASSERT(cred.data() == data);
    }

    PV("Test setMechanism and setData");
    {
        bmqt::AuthnCredential cred;
        bsl::string           mechanism("BMQ");
        bsl::vector<char>     data(Local::makeAuthnData());

        cred.setMechanism(mechanism);
        cred.setData(data);

        ASSERT(cred.mechanism() == mechanism);
        ASSERT(cred.data() == data);
    }

    PV("Test allocator usage");
    {
        bslma::Allocator*     allocator = bmqtst::TestHelperUtil::allocator();
        bmqt::AuthnCredential cred(allocator);
        ASSERT(cred.mechanism().get_allocator() == allocator);
        ASSERT(cred.data().get_allocator() == allocator);

        bsl::string           mechanism("BMQ", allocator);
        bsl::vector<char>     data(Local::makeAuthnData(allocator));
        bmqt::AuthnCredential cred2(mechanism, data, allocator);

        ASSERT(cred2.mechanism() == mechanism);
        ASSERT(cred2.data() == data);
    }

    PV("Test empty credential");
    {
        bmqt::AuthnCredential cred;
        ASSERT(cred.mechanism().empty());
        ASSERT(cred.data().empty());
    }

    PV("Test destructor");
    {
        bmqt::AuthnCredential* credPtr = new bmqt::AuthnCredential();
        delete credPtr;  // Ensure no memory leaks
    }
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

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
