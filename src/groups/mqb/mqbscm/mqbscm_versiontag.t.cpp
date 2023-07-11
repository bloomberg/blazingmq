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

// mqbscm_versiontag.t.cpp                                            -*-C++-*-
#include <mqbscm_versiontag.h>

// BDE
#include <bsl_iostream.h>
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
            cout << "Checking version consistency" << endl;
        BSLIM_TESTUTIL_ASSERT(MQB_VERSION_MAJOR ==
                              (MQB_VERSION / 10000) % 100);
        BSLIM_TESTUTIL_ASSERT(MQB_VERSION_MINOR == (MQB_VERSION / 100) % 100);
        BSLIM_TESTUTIL_ASSERT(0 == MQB_VERSION % 100);

        // Make sure we never commit a specific version in main (999999 is
        // the 'test build version')
        if (verbose)
            cout << "Ensuring version number is 999999" << endl;
        BSLIM_TESTUTIL_ASSERT(MQB_EXT_VERSION == 999999);
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
