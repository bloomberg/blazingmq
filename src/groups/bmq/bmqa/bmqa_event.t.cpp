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

// bmqa_event.t.cpp                                                   -*-C++-*-
#include <bmqa_event.h>

// BMQ
#include <bmqimp_event.h>

// BDE
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_memory.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Convert the specified `event` to a `bmqa` event (perform the same
/// `magic` that's being done in `bmqa::Session` when it wants to convert an
/// event received from the `bmqimp::EventQueue` to the user provided
/// eventHandler (which expects a `bmqa::Event`)
static bmqa::Event convertEvent(const bsl::shared_ptr<bmqimp::Event>& event)
{
    bmqa::Event out;

    bsl::shared_ptr<bmqimp::Event>& implRef =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Event>&>(out);

    implRef = event;

    return out;
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    {
        PV("Creating a default uninitialized event");
        bsl::shared_ptr<bmqimp::Event> eventImpl(
            new (*bmqtst::TestHelperUtil::allocator())
                bmqimp::Event(&bufferFactory,
                              bmqtst::TestHelperUtil::allocator()),
            bmqtst::TestHelperUtil::allocator());
        bmqa::Event event = convertEvent(eventImpl);

        ASSERT_EQ(event.isSessionEvent(), false);
        ASSERT_EQ(event.isMessageEvent(), false);
        PV("EmptyEvent: " << event);
    }

    {
        PV("Creating a SessionEvent");
        bsl::shared_ptr<bmqimp::Event> eventImpl(
            new (*bmqtst::TestHelperUtil::allocator())
                bmqimp::Event(&bufferFactory,
                              bmqtst::TestHelperUtil::allocator()),
            bmqtst::TestHelperUtil::allocator());
        eventImpl->configureAsSessionEvent(bmqt::SessionEventType::e_TIMEOUT,
                                           -3,
                                           bmqt::CorrelationId(13),
                                           "test");
        bmqa::Event event = convertEvent(eventImpl);

        // Validate type of the event
        ASSERT_EQ(event.isSessionEvent(), true);
        ASSERT_EQ(event.isMessageEvent(), false);

        // Validate session event values
        bmqa::SessionEvent se = event.sessionEvent();
        ASSERT_EQ(se.type(), bmqt::SessionEventType::e_TIMEOUT);
        ASSERT_EQ(se.statusCode(), -3);
        ASSERT_EQ(se.correlationId(), bmqt::CorrelationId(13));
        ASSERT_EQ(se.errorDescription(), "test");
        PV("Event: " << event);
    }
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
