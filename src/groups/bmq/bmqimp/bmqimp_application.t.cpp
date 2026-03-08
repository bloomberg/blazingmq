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

// bmqimp_application.t.cpp                                           -*-C++-*-
#include <bmqimp_application.h>

// BMQ
#include <bmqimp_event.h>
#include <bmqimp_eventqueue.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_resultcode.h>
#include <bmqt_sessioneventtype.h>
#include <bmqt_sessionoptions.h>

// BDE
#include <bsl_memory.h>
#include <bsls_timeinterval.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    // Create a default application
    bmqt::SessionOptions options(bmqtst::TestHelperUtil::allocator());
    bmqp_ctrlmsg::NegotiationMessage negotiationMessage(
        bmqtst::TestHelperUtil::allocator());
    bmqimp::EventQueue::EventHandlerCallback emptyEventHandler;

    bmqimp::Application obj(options,
                            negotiationMessage,
                            emptyEventHandler,
                            bmqtst::TestHelperUtil::allocator());
}

static void test2_startStopTest()
// ------------------------------------------------------------------------
// START STOP TEST
//
// Concerns:
//   Exercise sync start and stop behaviour without network connection
//
// Plan:
//   1. Create application object with an empty event handler
//   2. Verify stop() can be called before start()
//   3. Verify start() returns e_TIMEOUT, isStarted() returns false
//   4. Verify there are no valid session events in the user event queue
//
// Testing:
//   start()
//   stop()
//   isStarted()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("START STOP TEST");

    // Create a default application, make sure it can start/stop
    bmqt::SessionOptions options(bmqtst::TestHelperUtil::allocator());
    bmqp_ctrlmsg::NegotiationMessage negotiationMessage(
        bmqtst::TestHelperUtil::allocator());
    bmqimp::EventQueue::EventHandlerCallback emptyEventHandler;

    bmqimp::Application obj(options,
                            negotiationMessage,
                            emptyEventHandler,
                            bmqtst::TestHelperUtil::allocator());

    // Stop without previous start
    obj.stop();

    BMQTST_ASSERT(!obj.isStarted());

    int rc = obj.start(bsls::TimeInterval(0.1));

    BMQTST_ASSERT_EQ(rc, bmqt::GenericResult::e_TIMEOUT);

    BMQTST_ASSERT(!obj.isStarted());

    obj.stop();

    bsl::shared_ptr<bmqimp::Event> event = obj.brokerSession().nextEvent(
        bsls::TimeInterval(0.1));

    BMQTST_ASSERT(event);
    BMQTST_ASSERT_EQ(event->sessionEventType(),
                     bmqt::SessionEventType::e_TIMEOUT);
}

static void test3_startStopAsyncTest()
// ------------------------------------------------------------------------
// START STOP TEST
//
// Concerns:
//   Exercise async start and stop behaviour without network connection
//
// Plan:
//   1. Create application object with an empty event handler
//   2. Verify stopAsync() can be called before startAsync()
//   3. Verify startAsync() generates CONNECTION_TIMEOUT event
//   4. Verify startAsync() interrupted by stop() generates no event
//
// Testing:
//   startAsync()
//   stopAsync()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("START STOP TEST");

    // Create a default application, make sure it can start/stop
    bmqt::SessionOptions options(bmqtst::TestHelperUtil::allocator());
    bmqp_ctrlmsg::NegotiationMessage negotiationMessage(
        bmqtst::TestHelperUtil::allocator());
    bmqimp::EventQueue::EventHandlerCallback emptyEventHandler;

    bmqimp::Application obj(options,
                            negotiationMessage,
                            emptyEventHandler,
                            bmqtst::TestHelperUtil::allocator());

    // Stop without previous start
    obj.stopAsync();

    // Should be no DISCONNECTED event
    bsl::shared_ptr<bmqimp::Event> event = obj.brokerSession().nextEvent(
        bsls::TimeInterval(0.1));

    BMQTST_ASSERT(event);
    BMQTST_ASSERT_EQ(event->sessionEventType(),
                     bmqt::SessionEventType::e_TIMEOUT);

    BMQTST_ASSERT(!obj.isStarted());

    int rc = obj.startAsync(bsls::TimeInterval(0.1));
    BMQTST_ASSERT_EQ(rc, bmqt::GenericResult::e_SUCCESS);

    // Expect CONNECTION_TIMEOUT event
    event = obj.brokerSession().nextEvent(bsls::TimeInterval(1));
    BMQTST_ASSERT(event);
    BMQTST_ASSERT_EQ(event->sessionEventType(),
                     bmqt::SessionEventType::e_CONNECTION_TIMEOUT);

    BMQTST_ASSERT(!obj.isStarted());

    // Interrupted start
    rc = obj.startAsync(bsls::TimeInterval(0.5));
    BMQTST_ASSERT_EQ(rc, bmqt::GenericResult::e_SUCCESS);

    obj.stop();

    event = obj.brokerSession().nextEvent(bsls::TimeInterval(1));
    BMQTST_ASSERT(event);
    BMQTST_ASSERT_EQ(event->sessionEventType(),
                     bmqt::SessionEventType::e_TIMEOUT);
    BMQTST_ASSERT(!obj.isStarted());
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_startStopAsyncTest(); break;
    case 2: test2_startStopTest(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_DEFAULT);
    // Default: EventQueue uses bmqc::MonitoredFixedQueue, which uses
    //          'bdlcc::SharedObjectPool' which uses bslmt::Semaphore which
    //          generates a unique name using an ostringstream, hence the
    //          default allocator.
}
