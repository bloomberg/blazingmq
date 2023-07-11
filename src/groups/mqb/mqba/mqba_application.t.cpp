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

// mqba_application.t.cpp                                             -*-C++-*-
#include <mqba_application.h>

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>

// BMQ
#include <bmqp_protocolutil.h>

// BDE
#include <bdlmt_eventscheduler.h>
#include <bsls_systemclocktype.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

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
    mwctst::TestHelper::printTestName("breathing test");

    // Create a default application, make sure it can start/stop
    mqbcfg::AppConfig cfg(s_allocator_p);
    cfg.networkInterfaces().tcpInterface().makeValue();

    mqbcfg::BrokerConfig::set(cfg);
    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    s_allocator_p);
    scheduler.start();
    mqba::Application obj(&scheduler,
                          0,  // no allocatorsStatContext
                          s_allocator_p);

    // mwcs::MemOutStream error(s_allocator_p);
    // int rc = obj.start(error);
    // ASSERT_EQ(rc, 0);
    // obj.stop();
    scheduler.stop();
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    bmqp::ProtocolUtil::initialize(s_allocator_p);
    // Force initialize protocolUtil before any 'mqba::Application' object
    // gets created, so that we can pass in the test allocator, instead of
    // having 'mqba::Application' initializing it with the global
    // allocator.

    switch (_testCase) {
    case 0:
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();

    // Disable default/global allocator check:
    //  - Logger uses the default allocator
    TEST_EPILOG(mwctst::TestHelper::e_DEFAULT);
}
