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

#include <mqba_application.h>

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>

// BMQ
#include <bmqu_memoutstream.h>

// BDE
#include <bdlmt_eventscheduler.h>
#include <bsls_systemclocktype.h>

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

    // Create a default application, make sure it can start/stop
    mqbcfg::AppConfig cfg(bmqtst::TestHelperUtil::allocator());
    cfg.networkInterfaces().tcpInterface().makeValue();

    mqbcfg::BrokerConfig::set(cfg);
    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    scheduler.start();
    mqba::Application obj(&scheduler,
                          0,  // no allocatorsStatContext
                          bmqtst::TestHelperUtil::allocator());

    // bmqs::MemOutStream error(bmqtst::TestHelperUtil::allocator());
    // int rc = obj.start(error);
    // BMQTST_ASSERT_EQ(rc, 0);
    // obj.stop();
    scheduler.stop();
}

static void test2_stopAfterStartFailure()
// ------------------------------------------------------------------------
// STOP AFTER START FAILURE
//
// Concerns:
//   If 'start()' fails before the transport manager is constructed (e.g.
//   a misconfigured authenticator plugin name), 'stop()' must not crash.
//
// Plan:
//   Configure a nonexistent authenticator plugin name so 'start()' fails
//   at the 'AuthenticationController' stage, before the transport manager
//   is ever created.  Verify 'start()' fails and 'stop()' succeeds.
// Testing:
//   Application::start()/stop() on early startup failure
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("stop after start failure");

    mqbcfg::AppConfig cfg(bmqtst::TestHelperUtil::allocator());
    cfg.networkInterfaces().tcpInterface().makeValue();
    cfg.stats().snapshotInterval() = 0;

    mqbcfg::AuthenticatorPluginConfig authenticatorCfg(
        bmqtst::TestHelperUtil::allocator());
    authenticatorCfg.name() = "NonexistentAuthenticator";
    cfg.authentication().authenticators().push_back(authenticatorCfg);

    mqbcfg::BrokerConfig::set(cfg);

    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    scheduler.start();

    {
        mqba::Application obj(&scheduler,
                              0,
                              bmqtst::TestHelperUtil::allocator());

        bmqu::MemOutStream error(bmqtst::TestHelperUtil::allocator());
        int                rc = obj.start(error);
        BMQTST_ASSERT_NE(rc, 0);

        obj.stop();  // must not crash
    }

    scheduler.stop();
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
    case 2: test2_stopAfterStartFailure(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    // Disable default/global allocator check:
    //  - Logger uses the default allocator
    TEST_EPILOG(bmqtst::TestHelper::e_DEFAULT);
}
