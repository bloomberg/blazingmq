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

// mqbstat_statcontroller.t.cpp                                       -*-C++-*-
#include <mqbstat_statcontroller.h>

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbplug_pluginmanager.h>

#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_string.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// `fake` command processor to pass to the object under test, print the
/// specified `cmd` from the specified `source` to stdout, and write
/// `SUCCESS` to the specified `os`, return 0.
static int processCommand(const bslstl::StringRef& source,
                          const bsl::string&       cmd,
                          bsl::ostream&            os)
{
    cout << "Processing command '" << cmd << "' from '" << source << "'";
    os << "SUCCESS";

    return 0;
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   - breathing test
//
// Plan:
//   Instantiate the component under test.
//
// Testing:
//   Breathing test of the component
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("breathing test");

    // Create a default StatController, make sure it can start/stop
    mqbcfg::AppConfig cfg(
        bmqtst::TestHelperUtil::allocator());  // empty default config
    mqbcfg::BrokerConfig::set(cfg);

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlmt::EventScheduler  scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    mqbplug::PluginManager pluginManager;

    scheduler.start();
    mqbstat::StatController obj(
        bdlf::BindUtil::bind(&processCommand,
                             bdlf::PlaceHolders::_1,   // source
                             bdlf::PlaceHolders::_2,   // cmd
                             bdlf::PlaceHolders::_3),  // os
        &pluginManager,
        &bufferFactory,
        0,  // no allocatorsStatContext
        &scheduler,
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream errStream(bmqtst::TestHelperUtil::allocator());
    int                rc = obj.start(errStream);
    BMQTST_ASSERT_EQ(rc, 0);
    obj.stop();
    scheduler.stop();
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqsys::Time::initialize();

    switch (_testCase) {
    case 0:
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_DEFAULT);
    // Do not check fro default/global allocator usage.
}
