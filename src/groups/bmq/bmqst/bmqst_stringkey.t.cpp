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

// bmqst_stringkey.t.cpp -*-C++-*-

#include <bmqst_stringkey.h>

#include <bmqst_testutil.h>

#include <bmqu_memoutstream.h>

#include <bslma_testallocator.h>

#include <bsl_iostream.h>
#include <bsl_set.h>

using namespace BloombergLP;
using namespace bmqst;
using namespace bsl;

//=============================================================================
//                                  TEST PLAN
//-----------------------------------------------------------------------------
//                              *** Overview ***
//
// The component under test is an efficient string key for associative
// containers that attempts to minimize the copying of its string.  Because
// this component is relatively simple, it will only require a Breathing test,
// and we will mostly concern ourselves with making sure that memory is
// allocated only when it is supposed to be allocated.
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// [ 1] Breathing Test
//=============================================================================
//                      STANDARD BDE ASSERT TEST MACROS
//-----------------------------------------------------------------------------
static int testStatus = 0;

static void aSsErT(int c, const char* s, int i)
{
    if (c) {
        cout << "Error " << __FILE__ << "(" << i << "): " << s
             << "     (failed)" << endl;
        if (0 <= testStatus && testStatus <= 100)
            ++testStatus;
    }
}

//=============================================================================
//                       STANDARD BDE TEST DRIVER MACROS
//-----------------------------------------------------------------------------

#define ASSERT BSLS_BSLTESTUTIL_ASSERT

//=============================================================================
//                      GLOBAL HELPER FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------

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
    case 1: {
        // --------------------------------------------------------------------
        // BREATHING TEST
        //
        // Concerns:
        //   Exercise the basic functionality of the component.
        //
        // Plan:
        //   Create a number of StringKeys, verifying that allocations only
        //   happen when expected.
        //
        // Testing:
        //   Basic functionality
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl
                 << "BREATHING TEST" << endl
                 << "==============" << endl;

        const bslstl::StringRef STRING("String");

        ASSERT(testAllocator.numAllocations() == 0);

        {
            StringKey key(STRING, &testAllocator);
            ASSERT(testAllocator.numAllocations() == 0);
            ASSERT(testAllocator.numDeallocations() == 0);
            ASSERT(key.string() == STRING.data());

            key.makeCopy();
            ASSERT(testAllocator.numAllocations() == 1);
            ASSERT(testAllocator.numDeallocations() == 0);
            ASSERT(key.string() != STRING.data());
            ASSERT(bslstl::StringRef(key.string(), key.length()) == STRING);

            key.makeCopy();
            ASSERT(testAllocator.numAllocations() == 1);
            ASSERT(testAllocator.numDeallocations() == 0);

            {
                StringKey key2(key, &testAllocator);
                ASSERT(testAllocator.numAllocations() == 2);
                ASSERT(testAllocator.numDeallocations() == 0);
                ASSERT(key2.string() != key.string());
                ASSERT(bslstl::StringRef(key2.string(), key2.length()) ==
                       STRING);
            }
            ASSERT(testAllocator.numAllocations() == 2);
            ASSERT(testAllocator.numDeallocations() == 1);
        }
        ASSERT(testAllocator.numAllocations() == 2);
        ASSERT(testAllocator.numDeallocations() == 2);

        {
            StringKey key(STRING, &testAllocator);
        }
        ASSERT(testAllocator.numAllocations() == 2);
        ASSERT(testAllocator.numDeallocations() == 2);

        {
            StringKey          key(STRING, &testAllocator);
            bmqu::MemOutStream stream(&testAllocator);
            stream << key;
            ASSERT(key == stream.str());
        }
        {
            StringKey          key(STRING, &testAllocator);
            bmqu::MemOutStream stream(&testAllocator);
            key.print(stream, 1, 4);
            ASSERT("    String\n" == stream.str());
        }
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
