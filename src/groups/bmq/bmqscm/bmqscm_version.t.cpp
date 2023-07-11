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

// bmqscm_version.t.cpp                                               -*-C++-*-
#include <bmqscm_version.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bslim_testutil.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------
static int s_testStatus = 0;

static void aSsErT(bool b, const char* s, int i)
{
    if (b) {
        printf("Error " __FILE__ "(%d): %s    (failed)\n", i, s);
        if (s_testStatus >= 0 && s_testStatus <= 100)
            ++s_testStatus;
    }
}

int main(int argc, char** argv)
{
    int  test                = argc > 1 ? atoi(argv[1]) : 0;
    bool verbose             = argc > 2;
    bool veryVerbose         = argc > 3;
    bool veryVeryVerbose     = argc > 4;
    bool veryVeryVeryVerbose = argc > 5;
    // Prevent potential compiler unused warning
    (void)verbose;
    (void)veryVerbose;
    (void)veryVeryVerbose;
    (void)veryVeryVeryVerbose;

    cout << "TEST " << __FILE__ << " CASE " << test << endl;
    switch (test) {
    case 0:
    case 1: {
        if (verbose)
            cout << "Checking version string" << endl;
        bsl::string version(bmqscm::Version::version());
        BSLIM_TESTUTIL_ASSERT(version == "BLP_LIB_BMQ_99.99.99");

        if (verbose)
            cout << "Checking version int" << endl;
        BSLIM_TESTUTIL_ASSERT(bmqscm::Version::versionAsInt() == 999999);
    } break;

    default: {
        cerr << "WARNING: CASE '" << test << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    if (s_testStatus > 0) {
        cerr << "Error, non-zero test status = " << s_testStatus << "."
             << endl;
    }
    return s_testStatus;
}
