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

#ifndef INCLUDED_BMQTST_TESTGUARD
#define INCLUDED_BMQTST_TESTGUARD

//@PURPOSE: Provide a guard setting up the environment of a test driver.
//
//@CLASSES:
//   bmqtst::TestGuard: test driver environment guard
//
//@DESCRIPTION: 'bmqtst::TestGuard' sets up the environment of a test driver.
// For its lifetime it:
//: o parses the test case number and the verbosity from the command line
//: o installs an assert handler marking the test as failed
//: o sets up BALL logging to stdout
//: o installs test allocators as the global and default allocators, and
//:   provides the test allocator via 'TestHelperUtil::allocator()'
//
// 'finish' checks that the test allocator has no leaks and, unless disabled
// with 'disableDefaultAllocatorCheck' or 'disableGlobalAllocatorCheck', that
// the default and global allocators were not used.  All
// objects using 'TestHelperUtil::allocator()' must be destroyed before
// 'finish' is called.
//
/// Usage Example
///-------------
//..
//  int main(int argc, char* argv[])
//  {
//      bmqtst::TestGuard guard(argc, argv);
//
//      bmqtst::runTest(guard.testCase());
//
//      return guard.finish();
//  }
//..

// BDE
#include <ball_attributecontext.h>
#include <ball_fileobserver.h>
#include <ball_loggermanager.h>
#include <ball_multiplexobserver.h>
#include <balst_stacktracetestallocator.h>
#include <bslma_defaultallocatorguard.h>
#include <bslma_testallocator.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace bmqtst {

// ===============
// class TestGuard
// ===============

/// @brief Set up the test driver environment for the lifetime of this object.
class TestGuard {
  private:
    // DATA
    bool                            d_checkDefaultAllocator;
    bool                            d_checkGlobalAllocator;
    const int                       d_testCase;
    bool                            d_finished;
    bsls::AssertFailureHandlerGuard d_assertGuard;
    ball::AttributeContextProctor   d_attributeContextProctor;
    ball::MultiplexObserver         d_logMultiplexObserver;
    ball::FileObserver              d_logStdoutObserver;
    ball::LoggerManagerScopedGuard  d_logManagerGuard;
    bslma::TestAllocator&           d_gblAlloc;
    bslma::TestAllocator            d_defAlloc;
    bslma::DefaultAllocatorGuard    d_defAllocGuard;
    bslma::TestAllocator            d_testAlloc;
    balst::StackTraceTestAllocator  d_stTestAlloc;

  private:
    // NOT IMPLEMENTED
    TestGuard(const TestGuard&) BSLS_KEYWORD_DELETED;
    TestGuard& operator=(const TestGuard&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// @brief Set up the test driver environment.
    ///
    /// @param argc                   Number of command-line arguments.
    /// @param argv                   Command-line arguments: program name,
    ///                               test case number, then one argument per
    ///                               verbosity level.
    /// @param useStackTraceAllocator Whether `TestHelperUtil::allocator()`
    ///                               records stack traces of allocations.
    TestGuard(int argc, char* argv[], bool useStackTraceAllocator = false);

    /// @brief Tear down the test driver environment.  The behavior is
    /// undefined unless `finish` was called.
    ~TestGuard();

    // MANIPULATORS

    /// @brief Report that the test case given on the command line does not
    /// exist and mark the test as failed.
    void caseNotFound();

    /// @brief Do not check in `finish` that the default allocator was not
    /// used.
    void disableDefaultAllocatorCheck();

    /// @brief Do not check in `finish` that the global allocator was not
    /// used.
    void disableGlobalAllocatorCheck();

    /// @brief Check the allocators and report the test result.
    ///
    /// @return The test status to return from `main`.
    int finish();

    // ACCESSORS

    /// @brief Return the test case number given on the command line.
    int testCase() const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
