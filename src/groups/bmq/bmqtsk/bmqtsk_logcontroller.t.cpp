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

// BDE
#include <bdld_datum.h>
#include <bdld_datumarraybuilder.h>
#include <bdld_datummapbuilder.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_logControllerConfigFromDatum()
// ------------------------------------------------------------------------
// LOG CONTROLLER CONFIG FROM DATUM
//
// Concerns:
//   - Should be able to initialize LogControllerConfig with bdld::Datum.
//   - Inner map with SyslogConfig should be processed correctly too.
//
// Plan:
//   1. Fill the bdld::Datum structure representing LogControllerConfig.
//   2. Initialize the LogControllerConfig with the given bdld::Datum.
//   3. Verify that fromDatum call succeeded.
//   4. Verify that syslog properties were correctly set.
//
// Testing:
//   - LogControllerConfig::fromDatum
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LogControllerConfig::fromDatum Test");

    bdld::DatumMapBuilder syslogBuilder(s_allocator_p);
    syslogBuilder.pushBack("enabled", bdld::Datum::createBoolean(true));
    syslogBuilder.pushBack("appName",
                           bdld::Datum::createStringRef("testapp",
                                                        s_allocator_p));
    syslogBuilder.pushBack(
        "logFormat",
        bdld::Datum::createStringRef("test %d (%t) %s %F:%l %m\n\n",
                                     s_allocator_p));
    syslogBuilder.pushBack("verbosity",
                           bdld::Datum::createStringRef("info",
                                                        s_allocator_p));

    bdld::DatumArrayBuilder categoriesBuilder(s_allocator_p);
    categoriesBuilder.pushBack(
        bdld::Datum::copyString("category:info:red", s_allocator_p));

    bdld::DatumMapBuilder logControllerBuilder(s_allocator_p);
    logControllerBuilder.pushBack("fileName",
                                  bdld::Datum::copyString("fileName",
                                                          s_allocator_p));
    logControllerBuilder.pushBack("fileMaxAgeDays",
                                  bdld::Datum::createDouble(8.2));
    logControllerBuilder.pushBack("rotationBytes",
                                  bdld::Datum::createDouble(2048));
    logControllerBuilder.pushBack(
        "logfileFormat",
        bdld::Datum::copyString("%d (%t) %s %F:%l %m\n\n", s_allocator_p));
    logControllerBuilder.pushBack(
        "consoleFormat",
        bdld::Datum::copyString("%d (%t) %s %F:%l %m\n\n", s_allocator_p));
    logControllerBuilder.pushBack("loggingVerbosity",
                                  bdld::Datum::copyString("debug",
                                                          s_allocator_p));
    logControllerBuilder.pushBack("bslsLogSeverityThreshold",
                                  bdld::Datum::copyString("info",
                                                          s_allocator_p));
    logControllerBuilder.pushBack("consoleSeverityThreshold",
                                  bdld::Datum::copyString("info",
                                                          s_allocator_p));
    logControllerBuilder.pushBack("categories", categoriesBuilder.commit());
    logControllerBuilder.pushBack("syslog", syslogBuilder.commit());

    bdld::Datum                 datum = logControllerBuilder.commit();
    bmqtsk::LogControllerConfig config(s_allocator_p);
    bmqu::MemOutStream          errorDesc(s_allocator_p);
    config.fromDatum(errorDesc, datum);
    bdld::Datum::destroy(datum, s_allocator_p);

    ASSERT_D(errorDesc.str(), errorDesc.str().empty());

    ASSERT_EQ(config.fileMaxAgeDays(), 8);
    ASSERT_EQ(config.rotationBytes(), 2048);
    ASSERT_EQ(config.loggingVerbosity(), ball::Severity::DEBUG);

    ASSERT_EQ(config.syslogEnabled(), true);
    ASSERT_EQ(config.syslogFormat(), "test %d (%t) %s %F:%l %m\n\n");
    ASSERT_EQ(config.syslogAppName(), "testapp");
    ASSERT_EQ(config.syslogVerbosity(), ball::Severity::INFO);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_logControllerConfigFromDatum(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    // 'e_CHECK_DEF_GBL_ALLOC' check fails because 'fromDatum' function of
    // bmqtsk::LogControllerConfig allocates bsl::string with default
    // allocator.
    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
