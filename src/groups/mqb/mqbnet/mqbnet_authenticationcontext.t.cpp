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

#include <mqbnet_authenticationcontext.h>

// BMQ
#include <bmqio_testchannel.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqtst_scopedlogobserver.h>
#include <bmqu_memoutstream.h>
#include <bmqu_time.h>

// BDE
#include <ball_severity.h>
#include <bdlf_bind.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_memory.h>
#include <bsl_optional.h>
#include <bslma_allocator.h>
#include <bsls_systemclocktype.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

struct TestClock {
    bdlmt::EventScheduler*              d_scheduler_p;
    bdlmt::EventSchedulerTestTimeSource d_timeSource;

    explicit TestClock(bdlmt::EventScheduler* scheduler,
                       bslma::Allocator*      allocator)
    : d_scheduler_p(scheduler)
    , d_timeSource(scheduler, allocator)
    {
        BSLS_ASSERT_OPT(scheduler);
    }

    bsls::TimeInterval realtimeClock() { return d_timeSource.now(); }
    bsls::TimeInterval monotonicClock() { return d_timeSource.now(); }
    bsls::Types::Int64 highResTimer()
    {
        return d_timeSource.now().totalNanoseconds();
    }
};

struct TestBench {
    bsl::shared_ptr<bmqio::TestChannel> d_channel;
    bdlmt::EventScheduler               d_scheduler;
    TestClock                           d_testClock;
    bslma::Allocator*                   d_allocator_p;

    TestBench(const TestBench&) BSLS_KEYWORD_DELETED;
    TestBench& operator=(const TestBench&) BSLS_KEYWORD_DELETED;

    explicit TestBench(bslma::Allocator* allocator)
    : d_channel(bsl::allocate_shared<bmqio::TestChannel>(allocator))
    , d_scheduler(bsls::SystemClockType::e_MONOTONIC, allocator)
    , d_testClock(&d_scheduler, allocator)
    , d_allocator_p(allocator)
    {
        bmqu::Time::shutdown();
        bmqu::Time::initialize(
            bdlf::BindUtil::bindS(allocator,
                                  &TestClock::realtimeClock,
                                  &d_testClock),
            bdlf::BindUtil::bindS(allocator,
                                  &TestClock::monotonicClock,
                                  &d_testClock),
            bdlf::BindUtil::bindS(allocator,
                                  &TestClock::highResTimer,
                                  &d_testClock),
            d_allocator_p);

        int rc = d_scheduler.start();
        BMQTST_ASSERT_EQ(rc, 0);
    }

    ~TestBench()
    {
        d_scheduler.cancelAllEventsAndWait();
        d_scheduler.stop();
    }

    bsl::shared_ptr<mqbnet::AuthenticationContext>
    createContext(mqbnet::AuthenticationState::Enum state =
                      mqbnet::AuthenticationState::e_AUTHENTICATING)
    {
        bmqp_ctrlmsg::AuthenticationMessage authnMsg;
        return bsl::allocate_shared<mqbnet::AuthenticationContext>(
            d_allocator_p,
            static_cast<mqbnet::InitialConnectionContext*>(0),
            "testMechanism",
            authnMsg,
            bmqp::EncodingType::e_BER,
            state);
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   - AuthenticationContext can be constructed in the AUTHENTICATING state,
//     transitioned to AUTHENTICATED, and subsequently closed.
//
// Plan:
//   1) Create a context in AUTHENTICATING state.
//   2) Call setAuthenticatedAndScheduleReauthn with no lifetime.
//   3) Verify it returns 0 (success).
//   4) Close the context.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    bsl::shared_ptr<mqbnet::AuthenticationContext> ctx = tb.createContext();

    bmqu::MemOutStream                 errStream(alloc);
    bsl::optional<bsls::Types::Uint64> noLifetime;

    int rc = ctx->setAuthenticatedAndScheduleReauthn(errStream,
                                                     &tb.d_scheduler,
                                                     noLifetime,
                                                     tb.d_channel);
    BMQTST_ASSERT_EQ(rc, 0);

    ctx->onClose(&tb.d_scheduler);
}

static void test2_zeroLifetimeTimeout()
// ------------------------------------------------------------------------
// ZERO LIFETIME TIMEOUT
//
// Concerns:
//   - When lifetime is 0, the timer fires immediately and the channel
//     is closed.
//
// Plan:
//   1) Create a context, authenticate with a lifetime of 0 ms.
//   2) Advance time by a minimal amount to trigger the scheduler.
//   3) Verify the channel was closed.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ZERO LIFETIME TIMEOUT");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    bsl::shared_ptr<mqbnet::AuthenticationContext> ctx = tb.createContext();

    const bsls::Types::Uint64          lifetimeMs = 0;
    bsl::optional<bsls::Types::Uint64> lifetime(lifetimeMs);

    bmqu::MemOutStream errStream(alloc);
    int                rc = ctx->setAuthenticatedAndScheduleReauthn(errStream,
                                                     &tb.d_scheduler,
                                                     lifetime,
                                                     tb.d_channel);
    BMQTST_ASSERT_EQ(rc, 0);

    bmqtst::ScopedLogObserver logObserver(ball::Severity::e_ERROR, alloc);

    tb.d_testClock.d_timeSource.advanceTime(
        bsls::TimeInterval().addMilliseconds(1));

    BMQTST_ASSERT_EQ(tb.d_channel->numCloseCalls(), 1u);

    bmqio::TestChannel::CloseCall closeCall = tb.d_channel->popCloseCall();
    BMQTST_ASSERT_EQ(closeCall.d_status.category(),
                     bmqio::StatusCategory::e_CANCELED);

    BMQTST_ASSERT_EQ(logObserver.records().size(), 1u);
    BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
        logObserver.records()[0],
        ".*Reauthentication timeout.*",
        alloc));

    ctx->onClose(&tb.d_scheduler);
}

static void test3_reauthenticationTimeout()
// ------------------------------------------------------------------------
// REAUTHENTICATION TIMEOUT
//
// Concerns:
//   - When a lifetime is specified during authentication, a timer is
//     scheduled.
//   - The channel is NOT closed before the lifetime expires.
//   - If no reauthentication is received before the timer fires, the
//     channel is closed.
//
// Plan:
//   1) Create a context, authenticate with a lifetime of 5000 ms.
//   2) Advance time in 1000 ms increments, verifying the channel is not
//      closed at each step before the lifetime expires.
//   3) Advance time to hit the lifetime boundary.
//   4) Verify the channel was closed and the error was logged.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("REAUTHENTICATION TIMEOUT");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    bsl::shared_ptr<mqbnet::AuthenticationContext> ctx = tb.createContext();

    const bsls::Types::Uint64          lifetimeMs = 5000;
    const bsls::Types::Uint64          stepMs     = 1000;
    bsl::optional<bsls::Types::Uint64> lifetime(lifetimeMs);

    bmqu::MemOutStream errStream(alloc);
    int                rc = ctx->setAuthenticatedAndScheduleReauthn(errStream,
                                                     &tb.d_scheduler,
                                                     lifetime,
                                                     tb.d_channel);
    BMQTST_ASSERT_EQ(rc, 0);

    BMQTST_ASSERT_EQ(tb.d_channel->numCloseCalls(), 0u);

    bmqtst::ScopedLogObserver logObserver(ball::Severity::e_ERROR, alloc);

    // 2) Advance in steps, channel must stay open before the lifetime
    for (bsls::Types::Uint64 elapsed = stepMs; elapsed < lifetimeMs;
         elapsed += stepMs) {
        PVV("Advancing to " << elapsed << " ms");
        tb.d_testClock.d_timeSource.advanceTime(
            bsls::TimeInterval().addMilliseconds(stepMs));
        BMQTST_ASSERT_EQ(tb.d_channel->numCloseCalls(), 0u);
    }

    // 3) Final step reaches the lifetime
    tb.d_testClock.d_timeSource.advanceTime(
        bsls::TimeInterval().addMilliseconds(stepMs));

    // 4) Channel must now be closed
    BMQTST_ASSERT_EQ(tb.d_channel->numCloseCalls(), 1u);

    bmqio::TestChannel::CloseCall closeCall = tb.d_channel->popCloseCall();
    BMQTST_ASSERT_EQ(closeCall.d_status.category(),
                     bmqio::StatusCategory::e_CANCELED);

    BMQTST_ASSERT_EQ(logObserver.records().size(), 1u);
    BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
        logObserver.records()[0],
        ".*Reauthentication timeout.*",
        alloc));

    ctx->onClose(&tb.d_scheduler);
}

static void test4_reauthenticationBeforeTimeout()
// ------------------------------------------------------------------------
// REAUTHENTICATION BEFORE TIMEOUT
//
// Concerns:
//   - When a successful reauthentication occurs before the timer fires,
//     the old timer is cancelled and a new one is scheduled.
//   - The channel is not closed.
//
// Plan:
//   1) Create a context, authenticate with a lifetime of 5000 ms.
//   2) Advance time to 4000 ms (before timeout).
//   3) Verify channel was NOT closed.
//   4) Transition back to AUTHENTICATING via tryStartReauthentication().
//   5) Call setAuthenticatedAndScheduleReauthn again with a new lifetime.
//   6) Advance time another 5000 ms past the new schedule point.
//   7) Verify the channel was closed by the new timer.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("REAUTHENTICATION BEFORE TIMEOUT");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    bsl::shared_ptr<mqbnet::AuthenticationContext> ctx = tb.createContext();

    const bsls::Types::Uint64          lifetimeMs = 5000;
    bsl::optional<bsls::Types::Uint64> lifetime(lifetimeMs);

    bmqu::MemOutStream errStream(alloc);

    // 1) Initial authentication with lifetime
    int rc = ctx->setAuthenticatedAndScheduleReauthn(errStream,
                                                     &tb.d_scheduler,
                                                     lifetime,
                                                     tb.d_channel);
    BMQTST_ASSERT_EQ(rc, 0);

    // 2) Advance time to before the timeout
    tb.d_testClock.d_timeSource.advanceTime(
        bsls::TimeInterval().addMilliseconds(4000));

    // 3) Channel should not have been closed
    BMQTST_ASSERT_EQ(tb.d_channel->numCloseCalls(), 0u);

    // 4) Simulate reauthentication starting
    const bool started = ctx->tryStartReauthentication();
    BMQTST_ASSERT(started);

    // 5) Reauthenticate with a new lifetime
    errStream.reset();
    const bsls::Types::Uint64          newLifetimeMs = 5000;
    bsl::optional<bsls::Types::Uint64> newLifetime(newLifetimeMs);

    rc = ctx->setAuthenticatedAndScheduleReauthn(errStream,
                                                 &tb.d_scheduler,
                                                 newLifetime,
                                                 tb.d_channel);
    BMQTST_ASSERT_EQ(rc, 0);

    // Old timer should have been cancelled, no close yet
    BMQTST_ASSERT_EQ(tb.d_channel->numCloseCalls(), 0u);

    // 6) Advance time past the new timer
    tb.d_testClock.d_timeSource.advanceTime(
        bsls::TimeInterval().addMilliseconds(newLifetimeMs));

    // 7) Now the new timer fires and closes the channel
    BMQTST_ASSERT_EQ(tb.d_channel->numCloseCalls(), 1u);

    ctx->onClose(&tb.d_scheduler);
}

static void test5_noLifetimeNoTimer()
// ------------------------------------------------------------------------
// NO LIFETIME NO TIMER
//
// Concerns:
//   - When no lifetime is specified, no reauthentication timer is
//     scheduled and the channel is never closed by the context.
//
// Plan:
//   1) Create a context, authenticate without a lifetime.
//   2) Advance time significantly.
//   3) Verify the channel was NOT closed.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("NO LIFETIME NO TIMER");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    bsl::shared_ptr<mqbnet::AuthenticationContext> ctx = tb.createContext();

    bmqu::MemOutStream                 errStream(alloc);
    bsl::optional<bsls::Types::Uint64> noLifetime;

    int rc = ctx->setAuthenticatedAndScheduleReauthn(errStream,
                                                     &tb.d_scheduler,
                                                     noLifetime,
                                                     tb.d_channel);
    BMQTST_ASSERT_EQ(rc, 0);

    tb.d_testClock.d_timeSource.advanceTime(
        bsls::TimeInterval().addMilliseconds(100000));

    BMQTST_ASSERT_EQ(tb.d_channel->numCloseCalls(), 0u);

    ctx->onClose(&tb.d_scheduler);
}

static void test6_onCloseBeforeTimeout()
// ------------------------------------------------------------------------
// ON CLOSE BEFORE TIMEOUT
//
// Concerns:
//   - When onClose() is called before the reauthentication timer fires,
//     the timer is cancelled and the channel is not closed by the timeout
//     callback.
//   - Closing the channel is not the responsibility of the authentication
//     context when reauthentication is cancelled; the caller handles it.
//
// Plan:
//   1) Create a context, authenticate with a lifetime.
//   2) Call onClose() before the timer fires.
//   3) Advance time past the original lifetime.
//   4) Verify the channel was NOT closed.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ON CLOSE BEFORE TIMEOUT");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    bsl::shared_ptr<mqbnet::AuthenticationContext> ctx = tb.createContext();

    const bsls::Types::Uint64          lifetimeMs = 5000;
    bsl::optional<bsls::Types::Uint64> lifetime(lifetimeMs);

    bmqu::MemOutStream errStream(alloc);
    int                rc = ctx->setAuthenticatedAndScheduleReauthn(errStream,
                                                     &tb.d_scheduler,
                                                     lifetime,
                                                     tb.d_channel);
    BMQTST_ASSERT_EQ(rc, 0);

    // 2) Close before timeout
    ctx->onClose(&tb.d_scheduler);

    // 3) Advance time past the lifetime
    tb.d_testClock.d_timeSource.advanceTime(
        bsls::TimeInterval().addMilliseconds(lifetimeMs));

    // 4) Channel should not have been closed
    BMQTST_ASSERT_EQ(tb.d_channel->numCloseCalls(), 0u);
}

static void test7_contextDestroyedBeforeTimeout()
// ------------------------------------------------------------------------
// CONTEXT DESTROYED BEFORE TIMEOUT
//
// Concerns:
//   - When the AuthenticationContext is destroyed before the
//     reauthentication timer fires, the WeakMemFn guard prevents the
//     callback from executing on the destroyed object.
//   - No crash or channel close occurs.
//
// Plan:
//   1) Create a context, authenticate with a lifetime.
//   2) Destroy the context.
//   3) Advance time past the lifetime.
//   4) Verify the channel was NOT closed.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CONTEXT DESTROYED BEFORE TIMEOUT");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    {
        bsl::shared_ptr<mqbnet::AuthenticationContext> ctx =
            tb.createContext();

        const bsls::Types::Uint64          lifetimeMs = 5000;
        bsl::optional<bsls::Types::Uint64> lifetime(lifetimeMs);

        bmqu::MemOutStream errStream(alloc);
        int rc = ctx->setAuthenticatedAndScheduleReauthn(errStream,
                                                         &tb.d_scheduler,
                                                         lifetime,
                                                         tb.d_channel);
        BMQTST_ASSERT_EQ(rc, 0);

        // 2) Context destroyed here
    }

    // 3) Advance time past the lifetime
    tb.d_testClock.d_timeSource.advanceTime(
        bsls::TimeInterval().addMilliseconds(10000));

    // 4) Channel should not have been closed
    BMQTST_ASSERT_EQ(tb.d_channel->numCloseCalls(), 0u);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    {
        bmqu::Time::initialize(bmqtst::TestHelperUtil::allocator());

        switch (_testCase) {
        case 0:
        case 7: test7_contextDestroyedBeforeTimeout(); break;
        case 6: test6_onCloseBeforeTimeout(); break;
        case 5: test5_noLifetimeNoTimer(); break;
        case 4: test4_reauthenticationBeforeTimeout(); break;
        case 3: test3_reauthenticationTimeout(); break;
        case 2: test2_zeroLifetimeTimeout(); break;
        case 1: test1_breathingTest(); break;
        default: {
            cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
            bmqtst::TestHelperUtil::testStatus() = -1;
        } break;
        }

        bmqu::Time::shutdown();
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
