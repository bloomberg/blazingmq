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

// bmqtsk_logcontroller.t.cpp                                         -*-C++-*-
#include <bmqtsk_logcontroller.h>

#include <bmqu_memoutstream.h>

// MQB
#include <mqbcfg_messages.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_logControllerConfigFromObj()
// ------------------------------------------------------------------------
// LOG CONTROLLER CONFIG FROM OBJ
//
// Concerns:
//   - Should be able to initialize LogControllerConfig with bdld::Datum.
//   - Inner map with SyslogConfig should be processed correctly too.
//
// Plan:
//   1. Fill the MockObj structure representing LogControllerConfig.
//   2. Initialize the LogControllerConfig with the given MockObj.
//   3. Verify that fromObj call succeeded.
//   4. Verify that syslog properties were correctly set.
//   5. Verify that logDump properties were correctly set.
//
// Testing:
//   - LogControllerConfig::fromObj
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LogControllerConfig::fromObj Test");
    mqbcfg::LogController lc(bmqtst::TestHelperUtil::allocator());

    lc.syslog().enabled()   = true;
    lc.syslog().appName()   = "testapp";
    lc.syslog().logFormat() = "test %d (%t) %s %F:%l %m\n\n";
    lc.syslog().verbosity() = "INFO";

    lc.categories().push_back("category:info:red");

    lc.fileName()                 = "fileName";
    lc.fileMaxAgeDays()           = 8;
    lc.rotationBytes()            = 2048;
    lc.logfileFormat()            = "%d (%t) %s %F:%l %m\n\n";
    lc.consoleFormat()            = "%d (%t) %s %F:%l %m\n\n";
    lc.loggingVerbosity()         = "debug";
    lc.consoleSeverityThreshold() = "info";

    bmqtsk::LogControllerConfig config(bmqtst::TestHelperUtil::allocator());
    bmqu::MemOutStream          errorDesc(bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(config.fromObj<mqbcfg::LogController>(errorDesc, lc), 0);

    BMQTST_ASSERT_D(errorDesc.str(), errorDesc.str().empty());

    BMQTST_ASSERT_EQ(config.fileName(), "fileName");
    BMQTST_ASSERT_EQ(config.fileMaxAgeDays(), 8);
    BMQTST_ASSERT_EQ(config.rotationBytes(), 2048);
    BMQTST_ASSERT_EQ(config.logfileFormat(), "%d (%t) %s %F:%l %m\n\n");
    BMQTST_ASSERT_EQ(config.consoleFormat(), "%d (%t) %s %F:%l %m\n\n");
    BMQTST_ASSERT_EQ(config.loggingVerbosity(), ball::Severity::DEBUG);
    BMQTST_ASSERT_EQ(config.bslsLogSeverityThreshold(),
                     bsls::LogSeverity::e_ERROR);
    BMQTST_ASSERT_EQ(config.consoleSeverityThreshold(), ball::Severity::INFO);

    BMQTST_ASSERT_EQ(config.syslogEnabled(), true);
    BMQTST_ASSERT_EQ(config.syslogFormat(), "test %d (%t) %s %F:%l %m\n\n");
    BMQTST_ASSERT_EQ(config.syslogAppName(), "testapp");
    BMQTST_ASSERT_EQ(config.syslogVerbosity(), ball::Severity::INFO);

    BMQTST_ASSERT_EQ(config.recordBufferSizeBytes(), 32768);
    BMQTST_ASSERT_EQ(config.recordingVerbosity(), ball::Severity::OFF);
    BMQTST_ASSERT_EQ(config.triggerVerbosity(), ball::Severity::OFF);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_logControllerConfigFromObj(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    // 'e_CHECK_DEF_GBL_ALLOC' check fails because 'fromDatum' function of
    // bmqtsk::LogControllerConfig allocates bsl::string with default
    // allocator.
    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
