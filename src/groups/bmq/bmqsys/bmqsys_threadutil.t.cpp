// Copyright 2026 Bloomberg Finance L.P.
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

#include <bmqsys_threadutil.h>

// BDE
#include <bdlf_bind.h>
#include <bslmt_threadutil.h>

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
//   Set the thread name "once".
//   Validate the thread name does not update.
//
// Testing:
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    // Skip under MemorySanitizer: bslmt::ThreadUtil::getThreadName uses
    // pthread_getname_np which leaves the buffer uninitialized from MSan's
    // perspective.
#if defined(__has_feature)  // Clang-supported method for checking sanitizers.
    const bool skipTest = __has_feature(memory_sanitizer);
#elif defined(__SANITIZE_MEMORY__)
    // GCC-supported macros for checking MSAN.
    const bool skipTest = true;
#else
    const bool skipTest = false;
#endif

    if (skipTest) {
        bsl::cout << "Test skipped (running under sanitizer)" << bsl::endl;
        return;  // RETURN
    }

    bsl::string threadName;

    bmqsys::ThreadUtil::setCurrentThreadNameOnce("TestThread1");
    bslmt::ThreadUtil::getThreadName(&threadName);
    BMQTST_ASSERT_EQ(threadName, "TestThread1");

    bmqsys::ThreadUtil::setCurrentThreadNameOnce("TestThread2");
    bslmt::ThreadUtil::getThreadName(&threadName);
    BMQTST_ASSERT_EQ(threadName, "TestThread1");
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
