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

// bmqpi_hosthealthmonitor.t.cpp                                      -*-C++-*-
#include <bmqpi_hosthealthmonitor.h>

// BMQ
#include <bmqt_hosthealthstate.h>

// BDE
#include <bdlmt_signaler.h>
#include <bsls_protocoltest.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;

/// A test implementation of the 'bmqpi::HostHealthMonitor` protocol.
struct HostHealthMonitorTestImp
: public bsls::ProtocolTestImp<bmqpi::HostHealthMonitor> {
    bdlmt::SignalerConnection
    observeHostHealth(const HostHealthChangeFn&) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    bmqt::HostHealthState::Enum hostState() const BSLS_KEYWORD_OVERRIDE;
};

// Define one of HostHealthMonitorTestImp methods out-of-line, to instruct the
// compiler to bake the class's vtable into *this* translation unit.
bmqt::HostHealthState::Enum HostHealthMonitorTestImp::hostState() const
{
    return markDone();
}

static void dummyHostHealthCallback(bmqt::HostHealthState::Enum)
{
}

// ============================================================================
//                                    TESTS
// ============================================================================

static void test1_breathingTest()
// ------------------------------------------------------------------------
// PROTOCOL TEST:
//   Ensure this class is a properly defined protocol class.
//
// Plan:
//: 1 Define a concrete derived implementation, 'HostHealthMonitorTestImp',
//:   of the protocol.
//:
//: 2 Create an object of the 'bsls::ProtocolTest' class template
//:   parameterized by 'HostHealthMonitorTestImp', and use it to verify
//:   that:
//:
//:   1 The protocol is abstract. (C-1)
//:
//:   2 The protocol has no data members. (C-2)
//:
//:   3 The protocol has a virtual destructor. (C-3)
//:
//: 3 Use the 'BSLS_PROTOCOLTEST_ASSERT' macro to verify that
//:   non-creator methods of the protocol are:
//:
//:   1 virtual, (C-4)
//:
//:   2 publicly accessible. (C-5)
//
// Testing:
//   PROTOCOL TEST
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // The default allocator check fails in this test case because the
    // 'markDone' methods of AbstractSession may sometimes return a
    // memory-aware object without utilizing the parameter allocator.

    bmqtst::TestHelper::printTestName("BREATHING TEST");

    PV("Creating a concrete object");
    bsls::ProtocolTest<HostHealthMonitorTestImp> monitor;

    PV("Verify that the protocol is abstract");
    BMQTST_ASSERT(monitor.testAbstract());

    PV("Verify that there are no data members");
    BMQTST_ASSERT(monitor.testNoDataMembers());

    PV("Verify that the destructor is virtual");
    BMQTST_ASSERT(monitor.testVirtualDestructor());

    PV("Verify that all methods are public and virtual");

    BSLS_PROTOCOLTEST_ASSERT(monitor,
                             observeHostHealth(dummyHostHealthCallback));
    BSLS_PROTOCOLTEST_ASSERT(monitor, hostState());
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
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
