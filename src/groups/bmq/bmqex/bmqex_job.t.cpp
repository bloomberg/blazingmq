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

#include <bmqex_job.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_ostream.h>
#include <bslma_testallocator.h>
#include <bsls_assert.h>
#include <bsls_compilerfeatures.h>

// CONVENIENCE
using namespace BloombergLP;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// ======================
// class NullaryFunctor
// ======================

/// A nullary function for test purposes.
class NullaryFunctor {
  private:
    // PRIVATE DATA
    bool* d_invoked;

  public:
    // CREATORS
    explicit NullaryFunctor(bool* invoked)
    : d_invoked(invoked)
    {
        // PRECONDITIONS
        BSLS_ASSERT(invoked);
    }

  public:
    // MANIPULATORS
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES)
    void operator()() &&
#else
    void operator()()
#endif
    {
        *d_invoked = true;
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathing()
// ------------------------------------------------------------------------
// BREATHING
//
// Concerns:
//   Ensure proper behavior of 'bmqex::Job'.
//
// Plan:
//   Create a job, invoke it, check that the target was invoked.
//
// Testing:
//   basic functionality
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // create target
    bool           invoked = false;
    NullaryFunctor target(&invoked);

    // create job
    bmqex::Job job(target, &alloc);

    // memory allocated
    BMQTST_ASSERT_NE(alloc.numBytesInUse(), 0);

    // invoke job
    job();

    // target invoked
    BMQTST_ASSERT_EQ(invoked, true);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 1: test1_breathing(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
