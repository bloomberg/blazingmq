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

#include <bmqtst_testguard.h>

#include <bmqscm_version.h>
#include <bmqtst_testhelper.h>

// BDE
#include <ball_loggermanagerconfiguration.h>
#include <ball_severity.h>
#include <bdlm_metricsregistry.h>
#include <bsl_cstdlib.h>
#include <bsl_iostream.h>
#include <bslma_default.h>
#include <bsls_platform.h>

namespace BloombergLP {
namespace bmqtst {

namespace {

/// @brief Parse the command line and set the verbosity level.
///
/// @return The test case number.
int parseArgs(int argc, char* argv[])
{
    BSLS_ASSERT_OPT(argv);

    TestHelperUtil::verbosityLevel() = argc - 2;
    return argc > 1 ? bsl::atoi(argv[1]) : 0;
}

/// @brief Return the BALL severity matching the verbosity level.
ball::Severity::Level logSeverity()
{
    switch (TestHelperUtil::verbosityLevel()) {
    case 0: return ball::Severity::e_WARN;    // RETURN
    case 1: return ball::Severity::e_INFO;    // RETURN
    case 2: return ball::Severity::e_DEBUG;   // RETURN
    default: return ball::Severity::e_TRACE;  // RETURN
    }
}

/// @brief Return the BALL logger manager configuration.
ball::LoggerManagerConfiguration logConfig()
{
    ball::LoggerManagerConfiguration config;
    config.setDefaultThresholdLevelsIfValid(ball::Severity::e_OFF,
                                            logSeverity(),
                                            ball::Severity::e_OFF,
                                            ball::Severity::e_OFF);
    return config;
}

/// @brief Install a test allocator as the global allocator.
///
/// @return The global allocator.
bslma::TestAllocator& installGlobalAllocator()
{
    // The global allocator must outlive all static objects using it.
#ifdef BSLS_PLATFORM_CMP_CLANG
    [[clang::no_destroy]]
#endif
    static bslma::TestAllocator s_gblAlloc("global",
                                           TestHelperUtil::verbosityLevel() >=
                                               4);

    // Create the default metrics registry before the global allocator is set.
    bdlm::MetricsRegistry::defaultInstance();
    bslma::Default::setGlobalAllocator(&s_gblAlloc);
    return s_gblAlloc;
}

}  // close unnamed namespace

// ---------------
// class TestGuard
// ---------------

// CREATORS
TestGuard::TestGuard(int argc, char* argv[], bool useStackTraceAllocator)
: d_checkDefaultAllocator(true)
, d_checkGlobalAllocator(true)
, d_testCase(parseArgs(argc, argv))
, d_finished(false)
, d_assertGuard(BloombergLP::_assertViolationHandler)
, d_attributeContextProctor()
, d_logMultiplexObserver()
, d_logStdoutObserver(logSeverity())
// BALL uses the default allocator, so it is set up before the allocators.
, d_logManagerGuard(&d_logMultiplexObserver, logConfig())
, d_gblAlloc(installGlobalAllocator())
, d_defAlloc("default", TestHelperUtil::verbosityLevel() >= 4)
, d_defLogAlloc(&d_defAlloc)
, d_defAllocGuard(&d_defLogAlloc)
, d_testAlloc("test", TestHelperUtil::verbosityLevel() >= 4)
, d_stTestAlloc()
{
    d_logMultiplexObserver.registerObserver(&d_logStdoutObserver);

    d_testAlloc.setNoAbort(true);
    d_stTestAlloc.setName("test");

    TestHelperUtil::setDefaultAllocator(d_defAlloc);
    if (useStackTraceAllocator) {
        TestHelperUtil::allocator() = &d_stTestAlloc;
    }
    else {
        TestHelperUtil::allocator() = &d_testAlloc;
    }

    bsl::cout << "TEST " << argv[0] << " CASE " << d_testCase << bsl::endl;
}

TestGuard::~TestGuard()
{
    BSLS_ASSERT_OPT(d_finished);

    d_logMultiplexObserver.deregisterObserver(&d_logStdoutObserver);
    TestHelperUtil::allocator() = 0;
}

// MANIPULATORS
void TestGuard::caseNotFound()
{
    bsl::cerr << "WARNING: CASE '" << d_testCase << "' NOT FOUND."
              << bsl::endl;
    TestHelperUtil::testStatus() = -1;
}

void TestGuard::disableDefaultAllocatorCheck()
{
    d_checkDefaultAllocator = false;
}

void TestGuard::disableGlobalAllocatorCheck()
{
    d_checkGlobalAllocator = false;
}

int TestGuard::finish()
{
    BSLS_ASSERT_OPT(!d_finished);
    d_finished = true;

    BMQTST_ASSERT_EQ(d_testAlloc.numBlocksInUse(), 0);

    if (d_checkDefaultAllocator && !TestHelperUtil::ignoreCheckDefAlloc()) {
        BMQTST_ASSERT_EQ(d_defAlloc.numBlocksTotal(), 0);
    }

    if (d_checkGlobalAllocator && !TestHelperUtil::ignoreCheckGblAlloc()) {
        BMQTST_ASSERT_EQ(d_gblAlloc.numBlocksTotal(), 0);
    }

    if (TestHelperUtil::testStatus() > 0) {
        bsl::cerr << "Error, non-zero test status: "
                  << TestHelperUtil::testStatus() << "." << bsl::endl;
    }

    return TestHelperUtil::testStatus();
}

// ACCESSORS
int TestGuard::testCase() const
{
    return d_testCase;
}

}  // close package namespace
}  // close enterprise namespace
