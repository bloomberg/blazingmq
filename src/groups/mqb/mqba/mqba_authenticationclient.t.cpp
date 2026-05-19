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

#include <mqba_authenticationclient.h>

// MQB
#include <mqbplug_authncredential.h>
#include <mqbplug_credentialprovider.h>

// BMQ
#include <bmqio_testchannel.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqtst_scopedlogobserver.h>
#include <bmqu_memoutstream.h>
#include <bmqu_time.h>

// BDE
#include <ball_severity.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlcc_sharedobjectpool.h>
#include <bdlf_bind.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_memory.h>
#include <bsl_optional.h>
#include <bsl_string.h>
#include <bsl_vector.h>
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

typedef bdlcc::SharedObjectPool<
    bdlbb::Blob,
    bdlcc::ObjectPoolFunctors::DefaultCreator,
    bdlcc::ObjectPoolFunctors::RemoveAll<bdlbb::Blob> >
    BlobSpPool;

struct TestClock {
    bdlmt::EventScheduler&              d_scheduler;
    bdlmt::EventSchedulerTestTimeSource d_timeSource;

    TestClock(bdlmt::EventScheduler& scheduler)
    : d_scheduler(scheduler)
    , d_timeSource(&scheduler)
    {
    }

    bsls::TimeInterval realtimeClock() { return d_timeSource.now(); }
    bsls::TimeInterval monotonicClock() { return d_timeSource.now(); }
    bsls::Types::Int64 highResTimer()
    {
        return d_timeSource.now().totalNanoseconds();
    }
};

void createBlob(bdlbb::BlobBufferFactory* bufferFactory,
                void*                     arena,
                bslma::Allocator*         allocator)
{
    new (arena) bdlbb::Blob(bufferFactory, allocator);
}

bsl::optional<mqbplug::AuthnCredential>
successCredentialFunc(bsl::ostream&      error,
                      const bsl::string& mechanism,
                      const bsl::string& payload)
{
    (void)error;
    bsl::vector<char> data(payload.begin(), payload.end());
    return mqbplug::AuthnCredential(mechanism, data);
}

bsl::optional<mqbplug::AuthnCredential>
failingCredentialFunc(bsl::ostream& error)
{
    error << "credential provider is broken";
    return bsl::nullopt;
}

bmqp_ctrlmsg::AuthenticationMessage
makeSuccessResponse(const bdlb::NullableValue<bsls::Types::Int64>& lifetimeMs =
                        bdlb::NullableValue<bsls::Types::Int64>())
{
    bmqp_ctrlmsg::AuthenticationMessage   msg;
    bmqp_ctrlmsg::AuthenticationResponse& resp =
        msg.makeAuthenticationResponse();
    resp.status().category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
    resp.status().code()     = 0;
    resp.status().message()  = "OK";
    resp.lifetimeMs()        = lifetimeMs;
    return msg;
}

bmqp_ctrlmsg::AuthenticationMessage
makeFailureResponse(const bsl::string& message, int code)
{
    bmqp_ctrlmsg::AuthenticationMessage   msg;
    bmqp_ctrlmsg::AuthenticationResponse& resp =
        msg.makeAuthenticationResponse();
    resp.status().category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
    resp.status().code()     = code;
    resp.status().message()  = message;
    return msg;
}

int decodeAuthenticationEvent(bmqp_ctrlmsg::AuthenticationMessage* output,
                              const bdlbb::Blob&                   blob,
                              bslma::Allocator*                    allocator)
{
    bmqp::Event event(&blob, allocator);
    if (!event.isAuthenticationEvent()) {
        return -1;
    }
    return event.loadAuthenticationEvent(output);
}

class TestBench {
  public:
    // DATA
    bdlbb::PooledBlobBufferFactory      d_bufferFactory;
    BlobSpPool                          d_blobSpPool;
    bsl::shared_ptr<bmqio::TestChannel> d_channel;
    bdlmt::EventScheduler               d_scheduler;
    TestClock                           d_testClock;
    bslma::Allocator*                   d_allocator_p;

    // CREATORS
    explicit TestBench(bslma::Allocator* allocator)
    : d_bufferFactory(256, allocator)
    , d_blobSpPool(bdlf::BindUtil::bind(&createBlob,
                                        &d_bufferFactory,
                                        bdlf::PlaceHolders::_1,
                                        bdlf::PlaceHolders::_2),
                   1024,
                   allocator)
    , d_channel(new bmqio::TestChannel(allocator))
    , d_scheduler(bsls::SystemClockType::e_MONOTONIC, allocator)
    , d_testClock(d_scheduler)
    , d_allocator_p(allocator)
    {
        bmqu::Time::shutdown();
        bmqu::Time::initialize(
            bdlf::BindUtil::bind(&TestClock::realtimeClock, &d_testClock),
            bdlf::BindUtil::bind(&TestClock::monotonicClock, &d_testClock),
            bdlf::BindUtil::bind(&TestClock::highResTimer, &d_testClock),
            d_allocator_p);

        int rc = d_scheduler.start();
        BMQTST_ASSERT_EQ(rc, 0);
    }

    ~TestBench()
    {
        d_scheduler.cancelAllEventsAndWait();
        d_scheduler.stop();
    }

    // MANIPULATORS
    mqba::AuthenticationClient*
    createClient(const mqbplug::CredentialProvider::CredentialFunc& credFunc)
    {
        return new (*d_allocator_p) mqba::AuthenticationClient(credFunc,
                                                               d_channel,
                                                               &d_blobSpPool,
                                                               &d_scheduler,
                                                               d_allocator_p);
    }

    mqbplug::CredentialProvider::CredentialFunc
    makeSuccessCredentialFunc(const bsl::string& mechanism = "BASIC",
                              const bsl::string& payload   = "user:pass")
    {
        return bdlf::BindUtil::bind(&successCredentialFunc,
                                    bdlf::PlaceHolders::_1,
                                    mechanism,
                                    payload);
    }

    mqbplug::CredentialProvider::CredentialFunc makeFailingCredentialFunc()
    {
        return &failingCredentialFunc;
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_authenticateSuccess()
// ------------------------------------------------------------------------
// AUTHENTICATE SUCCESS
//
// Concerns:
//   - 'authenticate()' returns 0 on success.
//   - An AuthenticationRequest is written to the channel with the correct
//     mechanism and credential data.
//
// Plan:
//   1) Construct a client with a valid credential func and live channel.
//   2) Call 'authenticate()', verify the return code
//   3) Pop the write call, decode the blob, verify the request contents.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("AUTHENTICATE SUCCESS");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    // 1)
    bslma::ManagedPtr<mqba::AuthenticationClient> client(
        tb.createClient(tb.makeSuccessCredentialFunc("BASIC", "user:pass")),
        alloc);

    // 2)
    bmqu::MemOutStream errStream(alloc);
    int                rc = client->authenticate(errStream);
    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(tb.d_channel->numWriteCalls(), 1u);

    // 3)
    bmqio::TestChannel::WriteCall       wc = tb.d_channel->popWriteCall();
    bmqp_ctrlmsg::AuthenticationMessage decoded;
    rc = decodeAuthenticationEvent(&decoded, wc.d_blob, alloc);
    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT(decoded.isAuthenticationRequestValue());

    const bmqp_ctrlmsg::AuthenticationRequest& req =
        decoded.authenticationRequest();
    BMQTST_ASSERT_EQ(req.mechanism(), "BASIC");
    BMQTST_ASSERT(req.data().has_value());

    bsl::string dataStr(req.data().value().begin(),
                        req.data().value().end(),
                        alloc);
    BMQTST_ASSERT_EQ(dataStr, "user:pass");

    client->stop();
}

static void test2_authenticateChannelGone()
// ------------------------------------------------------------------------
// AUTHENTICATE CHANNEL GONE
//
// Concerns:
//   - 'authenticate()' returns rc_CHANNEL_GONE when the channel has been
//     destroyed, effectively handling dead channels gracefully.
//
// Plan:
//   1) Construct a client
//   2) Destroy the external channel shared_ptr
//   3) Call 'authenticate()', and verify the error code and description.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("AUTHENTICATE CHANNEL GONE");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    // 1)
    bslma::ManagedPtr<mqba::AuthenticationClient> client(
        tb.createClient(tb.makeSuccessCredentialFunc()),
        alloc);

    // 2)
    tb.d_channel.reset();

    // 3)
    bmqu::MemOutStream errStream(alloc);
    int                rc = client->authenticate(errStream);
    BMQTST_ASSERT_EQ(rc, -1);
    BMQTST_ASSERT(bsl::string(errStream.str(), alloc)
                      .find("Channel is no longer available") !=
                  bsl::string::npos);

    client->stop();
}

static void test3_authenticateCredentialFailure()
// ------------------------------------------------------------------------
// AUTHENTICATE CREDENTIAL FAILURE
//
// Concerns:
//   - 'authenticate()' returns rc_CREDENTIAL_FAILURE when the credential
//     func returns nullopt.
//   - No write is attempted on the channel.
//
// Plan:
//   1) Construct a client with a failing credential func
//   2) Call 'authenticate()' and verify the error code, description,
//      and that no data was written to the channel.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("AUTHENTICATE CREDENTIAL FAILURE");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    // 1)
    bslma::ManagedPtr<mqba::AuthenticationClient> client(
        tb.createClient(tb.makeFailingCredentialFunc()),
        alloc);

    // 2)
    bmqu::MemOutStream errStream(alloc);
    int                rc = client->authenticate(errStream);
    BMQTST_ASSERT_EQ(rc, -2);
    BMQTST_ASSERT(bsl::string(errStream.str(), alloc)
                      .find("Failed to obtain credentials") !=
                  bsl::string::npos);
    BMQTST_ASSERT_EQ(tb.d_channel->numWriteCalls(), 0u);

    client->stop();
}

static void test4_authenticateWriteFails()
// ------------------------------------------------------------------------
// AUTHENTICATE WRITE FAILS
//
// Concerns:
//   - 'authenticate()' returns rc_WRITE_FAILURE when the channel write
//     fails.
//
// Plan:
//   1) Construct a client
//   2) Set the test channel's write status to an error
//   3) Call 'authenticate()', and verify the error code and description.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("AUTHENTICATE WRITE FAILS");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    // 1)
    bslma::ManagedPtr<mqba::AuthenticationClient> client(
        tb.createClient(tb.makeSuccessCredentialFunc()),
        alloc);

    // 2)
    tb.d_channel->setWriteStatus(
        bmqio::Status(bmqio::StatusCategory::e_GENERIC_ERROR));

    // 3)
    bmqu::MemOutStream errStream(alloc);
    int                rc = client->authenticate(errStream);
    BMQTST_ASSERT_EQ(rc, -4);
    BMQTST_ASSERT(bsl::string(errStream.str(), alloc).find("Failed sending") !=
                  bsl::string::npos);

    client->stop();
}

static void test5_handleResponseSuccessNoLifetime()
// ------------------------------------------------------------------------
// HANDLE RESPONSE SUCCESS NO LIFETIME
//
// Concerns:
//   - 'handleResponse()' returns 0 for a successful response.
//   - No reauthentication timer is scheduled when lifetimeMs is absent.
//
// Plan:
//   1) Authenticate to establish the connection.
//   2) Call 'handleResponse()' with a success response (no lifetimeMs).
//   3) Advance time significantly and verify no additional writes occur.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HANDLE RESPONSE SUCCESS NO LIFETIME");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    bslma::ManagedPtr<mqba::AuthenticationClient> client(
        tb.createClient(tb.makeSuccessCredentialFunc()),
        alloc);

    // 1)
    bmqu::MemOutStream errStream(alloc);
    int                rc = client->authenticate(errStream);
    BMQTST_ASSERT_EQ(rc, 0);

    size_t writeCountAfterAuth = tb.d_channel->numWriteCalls();

    // 2)
    bmqp_ctrlmsg::AuthenticationMessage response = makeSuccessResponse();

    errStream.reset();
    rc = client->handleResponse(errStream, response);
    BMQTST_ASSERT_EQ(rc, 0);

    // 3)
    tb.d_testClock.d_timeSource.advanceTime(bsls::TimeInterval(60));
    BMQTST_ASSERT_EQ(tb.d_channel->numWriteCalls(), writeCountAfterAuth);

    client->stop();
}

static void test6_handleResponseSuccessWithLifetime()
// ------------------------------------------------------------------------
// HANDLE RESPONSE SUCCESS WITH LIFETIME
//
// Concerns:
//   - 'handleResponse()' returns 0 for a successful response with a
//     lifetimeMs value.
//   - A reauthentication timer fires at exactly 80% of the lifetime.
//   - The reauthentication sends a valid AuthenticationRequest.
//
// Plan:
//   1) Authenticate, then call 'handleResponse()' with lifetimeMs=10000.
//   2) Advance time to 7999ms, verify no reauth fires.
//   3) Advance 1ms more (to 8000ms = 80%), verify one
//      AuthenticationRequest is sent.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HANDLE RESPONSE SUCCESS WITH LIFETIME");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    bslma::ManagedPtr<mqba::AuthenticationClient> client(
        tb.createClient(tb.makeSuccessCredentialFunc()),
        alloc);

    // 1)
    bmqu::MemOutStream errStream(alloc);
    int                rc = client->authenticate(errStream);
    BMQTST_ASSERT_EQ(rc, 0);

    size_t writeCountAfterAuth = tb.d_channel->numWriteCalls();

    const bsls::Types::Int64            lifetimeMs = 10000;  // 10 seconds
    bmqp_ctrlmsg::AuthenticationMessage response   = makeSuccessResponse(
        bdlb::NullableValue<bsls::Types::Int64>(lifetimeMs));

    errStream.reset();
    rc = client->handleResponse(errStream, response);
    BMQTST_ASSERT_EQ(rc, 0);

    // 2)
    tb.d_testClock.d_timeSource.advanceTime(
        bsls::TimeInterval().addMilliseconds(7999));
    BMQTST_ASSERT_EQ(tb.d_channel->numWriteCalls(), writeCountAfterAuth);

    // 3)
    tb.d_testClock.d_timeSource.advanceTime(
        bsls::TimeInterval().addMilliseconds(1));
    BMQTST_ASSERT_EQ(tb.d_channel->numWriteCalls(), writeCountAfterAuth + 1);

    bmqio::TestChannel::WriteCall       wc = tb.d_channel->popWriteCall();
    bmqp_ctrlmsg::AuthenticationMessage decoded;
    rc = decodeAuthenticationEvent(&decoded, wc.d_blob, alloc);
    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT(decoded.isAuthenticationRequestValue());

    client->stop();
}

static void test7_handleResponseAuthenticationFailure()
// ------------------------------------------------------------------------
// HANDLE RESPONSE AUTHENTICATION FAILURE
//
// Concerns:
//   - 'handleResponse()' returns rc_AUTHENTICATION_FAILURE when the
//     response status is not E_SUCCESS.
//   - The error description contains the failure reason.
//
// Plan:
//   1) Construct a client.
//   2) Call 'handleResponse()' with a response whose status category is
//      E_REFUSED. Verify the error code and description.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "HANDLE RESPONSE AUTHENTICATION FAILURE");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    // 1)
    bslma::ManagedPtr<mqba::AuthenticationClient> client(
        tb.createClient(tb.makeSuccessCredentialFunc()),
        alloc);

    // 2)
    bmqp_ctrlmsg::AuthenticationMessage response =
        makeFailureResponse("access denied", 403);

    bmqu::MemOutStream errStream(alloc);
    int                rc = client->handleResponse(errStream, response);
    BMQTST_ASSERT_NE(rc, 0);
    BMQTST_ASSERT(
        bsl::string(errStream.str(), alloc).find("Authentication failed") !=
        bsl::string::npos);

    client->stop();
}

static void test8_handleResponseInvalidMessage()
// ------------------------------------------------------------------------
// HANDLE RESPONSE INVALID MESSAGE
//
// Concerns:
//   - 'handleResponse()' returns rc_INVALID_AUTHN_RESPONSE when the
//     message is not an AuthenticationResponse.
//
// Plan:
//   1) Construct a client.
//   2) Call 'handleResponse()' with an AuthenticationRequest (not a
//      response). Verify the error code and description.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HANDLE RESPONSE INVALID MESSAGE");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    // 1)
    bslma::ManagedPtr<mqba::AuthenticationClient> client(
        tb.createClient(tb.makeSuccessCredentialFunc()),
        alloc);

    // 2)
    bmqp_ctrlmsg::AuthenticationMessage badMsg;
    badMsg.makeAuthenticationRequest();
    badMsg.authenticationRequest().mechanism() = "BASIC";

    bmqu::MemOutStream errStream(alloc);
    int                rc = client->handleResponse(errStream, badMsg);
    BMQTST_ASSERT_NE(rc, 0);
    BMQTST_ASSERT(bsl::string(errStream.str(), alloc)
                      .find("Expected AuthenticationResponse") !=
                  bsl::string::npos);

    client->stop();
}

static void test9_stopCancelsReauthTimer()
// ------------------------------------------------------------------------
// STOP CANCELS REAUTH TIMER
//
// Concerns:
//   - Calling 'stop()' cancels a pending reauthentication timer.
//   - No reauthentication write occurs after stop, even when time
//     advances past the scheduled point.
//
// Plan:
//   1) Authenticate and handle a response with a lifetime to schedule
//      reauthentication.
//   2) Call 'stop()'.
//   3) Advance time past the reauthn lifetime, verify no additional writes.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("STOP CANCELS REAUTH TIMER");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    bslma::ManagedPtr<mqba::AuthenticationClient> client(
        tb.createClient(tb.makeSuccessCredentialFunc()),
        alloc);

    // 1)
    bmqu::MemOutStream errStream(alloc);
    int                rc = client->authenticate(errStream);
    BMQTST_ASSERT_EQ(rc, 0);

    size_t writeCountAfterAuth = tb.d_channel->numWriteCalls();

    const bsls::Types::Int64            lifetimeMs = 10000;
    bmqp_ctrlmsg::AuthenticationMessage response   = makeSuccessResponse(
        bdlb::NullableValue<bsls::Types::Int64>(lifetimeMs));

    errStream.reset();
    rc = client->handleResponse(errStream, response);
    BMQTST_ASSERT_EQ(rc, 0);

    // 2)
    client->stop();

    // 3)
    tb.d_testClock.d_timeSource.advanceTime(
        bsls::TimeInterval().addMilliseconds(10 * lifetimeMs));
    BMQTST_ASSERT_EQ(tb.d_channel->numWriteCalls(), writeCountAfterAuth);
}

static void test10_stopThenAuthenticate()
// ------------------------------------------------------------------------
// STOP THEN AUTHENTICATE
//
// Concerns:
//   - 'authenticate()' returns rc_CHANNEL_GONE after 'stop()' has been
//     called, because 'stop()' resets the weak_ptr to the channel.
//
// Plan:
//   1) Construct a client.
//   2) Call 'stop()' on the client.
//   3) Call 'authenticate()' and verify it fails with the channel-gone
//      error.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("STOP THEN AUTHENTICATE");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    // 1)
    bslma::ManagedPtr<mqba::AuthenticationClient> client(
        tb.createClient(tb.makeSuccessCredentialFunc()),
        alloc);

    // 2)
    client->stop();

    // 3)
    bmqu::MemOutStream errStream(alloc);
    int                rc = client->authenticate(errStream);
    BMQTST_ASSERT_NE(rc, 0);
    BMQTST_ASSERT(bsl::string(errStream.str(), alloc)
                      .find("Channel is no longer available") !=
                  bsl::string::npos);
}

static void test11_reauthChannelGoneAtTimerFire()
// ------------------------------------------------------------------------
// REAUTH CHANNEL GONE AT TIMER FIRE
//
// Concerns:
//   - When the channel is destroyed before the reauthentication timer
//     fires, the callback handles the expired weak_ptr gracefully.
//   - No crash or undefined behavior occurs.
//   - The failure is logged.
//
// Plan:
//   1) Authenticate and handle a response with a lifetime to schedule
//      reauthentication.
//   2) Destroy all shared_ptrs to the channel.
//   3) Advance time past the reauth point and verify the expected error
//      is logged via a ScopedLogObserver.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("REAUTH CHANNEL GONE AT TIMER FIRE");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    bslma::ManagedPtr<mqba::AuthenticationClient> client(
        tb.createClient(tb.makeSuccessCredentialFunc()),
        alloc);

    // 1)
    bmqu::MemOutStream errStream(alloc);
    int                rc = client->authenticate(errStream);
    BMQTST_ASSERT_EQ(rc, 0);

    const bsls::Types::Int64            lifetimeMs = 10000;
    bmqp_ctrlmsg::AuthenticationMessage response   = makeSuccessResponse(
        bdlb::NullableValue<bsls::Types::Int64>(lifetimeMs));

    errStream.reset();
    rc = client->handleResponse(errStream, response);
    BMQTST_ASSERT_EQ(rc, 0);

    // 2)
    tb.d_channel.reset();

    // 3)
    bmqtst::ScopedLogObserver logObserver(ball::Severity::e_ERROR, alloc);

    tb.d_testClock.d_timeSource.advanceTime(
        bsls::TimeInterval().addMilliseconds(lifetimeMs));

    BMQTST_ASSERT_EQ(logObserver.records().size(), 1u);
    BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
        logObserver.records()[0],
        ".*Reauthentication failed.*",
        alloc));

    client->stop();
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
        case 11: test11_reauthChannelGoneAtTimerFire(); break;
        case 10: test10_stopThenAuthenticate(); break;
        case 9: test9_stopCancelsReauthTimer(); break;
        case 8: test8_handleResponseInvalidMessage(); break;
        case 7: test7_handleResponseAuthenticationFailure(); break;
        case 6: test6_handleResponseSuccessWithLifetime(); break;
        case 5: test5_handleResponseSuccessNoLifetime(); break;
        case 4: test4_authenticateWriteFails(); break;
        case 3: test3_authenticateCredentialFailure(); break;
        case 2: test2_authenticateChannelGone(); break;
        case 1: test1_authenticateSuccess(); break;
        default: {
            cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
            bmqtst::TestHelperUtil::testStatus() = -1;
        } break;
        }

        bmqu::Time::shutdown();
    }

    TEST_EPILOG(bmqtst::TestHelper::e_DEFAULT);
}
