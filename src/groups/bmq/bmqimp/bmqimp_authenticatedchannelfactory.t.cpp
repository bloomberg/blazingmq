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

// bmqimp_authenticatedchannelfactory.t.cpp                          -*-C++-*-
#include <bmqimp_authenticatedchannelfactory.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

#include <bmqio_testchannel.h>
#include <bmqio_testchannelfactory.h>
#include <bmqp_blobpoolutil.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqp_schemaeventbuilder.h>
#include <bmqsys_mocktime.h>
#include <bmqt_authncredential.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_deque.h>
#include <bsl_memory.h>
#include <bsl_optional.h>
#include <bsl_ostream.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_objectbuffer.h>
#include <bsls_timeinterval.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;
using namespace bmqio;
using namespace bmqimp;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

typedef bmqt::SessionOptions::AuthnCredentialCb AuthnCredentialCb;

/// Struct holding the (event, status) pair delivered via the connect
/// callback.
struct ResultItem {
    ChannelFactoryEvent::Enum d_event;
    Status                    d_status;

    ResultItem(ChannelFactoryEvent::Enum event, const Status& status)
    : d_event(event)
    , d_status(status)
    {
    }
};

/// Return a valid credential callback that always returns a credential with
/// mechanism "BASIC" and data "testdata".
AuthnCredentialCb makeDefaultCredentialCb()
{
    return [](bsl::ostream&) -> bsl::optional<bmqt::AuthnCredential> {
        bsl::vector<char> data;
        const char        raw[] = "testdata";
        data.assign(&raw[0], &raw[8]);
        return bsl::optional<bmqt::AuthnCredential>(bsl::in_place,
                                                    bsl::string("BASIC"),
                                                    data);
    };
}

// -----------------
// class TestContext
// -----------------

/// Reusable test infrastructure for AuthenticatedChannelFactory tests.
class TestContext {
  private:
    // DATA
    bdlbb::PooledBlobBufferFactory                  d_bufferFactory;
    bmqp::BlobPoolUtil::BlobSpPoolSp                d_blobSpPool;
    bmqsys::MockTime                                d_mockTime;
    bdlmt::EventScheduler                           d_scheduler;
    bdlmt::EventSchedulerTestTimeSource             d_timeSource;
    TestChannelFactory                              d_baseFactory;
    TestChannel                                     d_channelImpl;
    bsl::shared_ptr<Channel>                        d_channel;
    bsls::ObjectBuffer<AuthenticatedChannelFactory> d_factory;
    bsl::deque<ResultItem>                          d_results;

    bslma::ManagedPtr<ChannelFactory::OpHandle> d_connectHandle;

  private:
    // NOT IMPLEMENTED
    TestContext(const TestContext&);
    TestContext& operator=(const TestContext&);

    /// Callback supplied to the `connect` call.
    void connectResultCb(ChannelFactoryEvent::Enum       event,
                         const Status&                   status,
                         const bsl::shared_ptr<Channel>& channel);

  public:
    // CREATORS
    explicit TestContext(const AuthnCredentialCb& authnCredentialCb =
                             makeDefaultCredentialCb());
    ~TestContext();

    // MANIPULATORS

    /// Call connect on the factory under test.
    void connect();

    /// Retrieve the stored ConnectCall from the base factory and invoke its
    /// callback with e_CHANNEL_UP and the test channel.
    void emitChannelUp();

    /// Build an authentication response blob with the specified
    /// `statusCategory`.  Optionally specify `lifetimeMs` to include a
    /// session lifetime in the response.
    bdlbb::Blob
    buildAuthResponse(bmqp_ctrlmsg::StatusCategory::Value statusCategory,
                      int                                 lifetimeMs = -1);

    /// Pop the ReadCall from the test channel and invoke its read callback
    /// with success status and the specified `blob`.
    void simulateBrokerResponse(const bdlbb::Blob& blob);

    /// Advance the scheduler time by the specified `milliseconds`.
    void advanceTime(int milliseconds);

    /// Assert that the front of d_results has the specified `event`, then
    /// pop it.
    void checkResult(ChannelFactoryEvent::Enum event);

    // ACCESSORS

    /// Return the number of pending results.
    size_t resultsCount() const;

    /// Return a reference offering modifiable access to the test channel.
    TestChannel& testChannel();

    /// Return a reference offering modifiable access to the factory under
    /// test.
    AuthenticatedChannelFactory& obj();

    /// Return a reference offering modifiable access to the base factory.
    TestChannelFactory& baseFactory();
};

void TestContext::connectResultCb(ChannelFactoryEvent::Enum       event,
                                  const Status&                   status,
                                  const bsl::shared_ptr<Channel>& channel)
{
    PVV("ConnectResultCb: [event: " << event << ", status: " << status
                                    << ", channel: " << channel.get() << "]");
    d_results.emplace_back(event, status);
}

TestContext::TestContext(const AuthnCredentialCb& authnCredentialCb)
: d_bufferFactory(4096, bmqtst::TestHelperUtil::allocator())
, d_blobSpPool(
      bmqp::BlobPoolUtil::createBlobPool(&d_bufferFactory,
                                         bmqtst::TestHelperUtil::allocator()))
, d_scheduler(bsls::SystemClockType::e_MONOTONIC,
              bmqtst::TestHelperUtil::allocator())
, d_timeSource(&d_scheduler)
, d_baseFactory(bmqtst::TestHelperUtil::allocator())
, d_channelImpl(bmqtst::TestHelperUtil::allocator())
, d_channel(&d_channelImpl, bslstl::SharedPtrNilDeleter())
, d_results(bmqtst::TestHelperUtil::allocator())
, d_connectHandle()
{
    d_scheduler.start();

    AuthenticatedChannelFactoryConfig config(
        &d_baseFactory,
        &d_scheduler,
        authnCredentialCb,
        bsls::TimeInterval(30),  // authentication timeout
        d_blobSpPool.get(),
        bmqtst::TestHelperUtil::allocator());

    new (d_factory.buffer())
        AuthenticatedChannelFactory(config,
                                    bmqtst::TestHelperUtil::allocator());
}

TestContext::~TestContext()
{
    d_factory.object()
        .bmqimp::AuthenticatedChannelFactory::~AuthenticatedChannelFactory();
    d_scheduler.stop();
}

void TestContext::connect()
{
    const Status successStatus(StatusCategory::e_SUCCESS,
                               bmqtst::TestHelperUtil::allocator());
    d_baseFactory.setConnectStatus(successStatus);

    ConnectOptions options(bmqtst::TestHelperUtil::allocator());
    options.setEndpoint("tcp://localhost:30114")
        .setNumAttempts(1)
        .setAttemptInterval(bsls::TimeInterval(1));

    Status status(bmqtst::TestHelperUtil::allocator());
    obj().connect(&status,
                  &d_connectHandle,
                  options,
                  bdlf::BindUtil::bind(&TestContext::connectResultCb,
                                       this,
                                       bdlf::PlaceHolders::_1,
                                       bdlf::PlaceHolders::_2,
                                       bdlf::PlaceHolders::_3));
    BMQTST_ASSERT(status);
}

void TestContext::emitChannelUp()
{
    BMQTST_ASSERT_EQ(d_baseFactory.connectCalls().size(), 1U);

    const TestChannelFactory::ConnectCall& call =
        d_baseFactory.connectCalls().front();

    call.d_cb(ChannelFactoryEvent::e_CHANNEL_UP,
              Status(bmqtst::TestHelperUtil::allocator()),
              d_channel);
    d_baseFactory.connectCalls().pop_front();
}

bdlbb::Blob TestContext::buildAuthResponse(
    bmqp_ctrlmsg::StatusCategory::Value statusCategory,
    int                                 lifetimeMs)
{
    bmqp_ctrlmsg::AuthenticationMessage authMessage(
        bmqtst::TestHelperUtil::allocator());
    bmqp_ctrlmsg::AuthenticationResponse& resp =
        authMessage.makeAuthenticationResponse();
    resp.status().category() = statusCategory;

    if (lifetimeMs >= 0) {
        resp.lifetimeMs().makeValue(lifetimeMs);
    }

    bmqp::SchemaEventBuilder builder(d_blobSpPool.get(),
                                     bmqp::EncodingType::e_BER,
                                     bmqtst::TestHelperUtil::allocator());

    int rc = builder.setMessage(authMessage,
                                bmqp::EventType::e_AUTHENTICATION);
    BMQTST_ASSERT_EQ(rc, 0);

    return *builder.blob();
}

void TestContext::simulateBrokerResponse(const bdlbb::Blob& blob)
{
    BMQTST_ASSERT_EQ(d_channelImpl.numReadCalls(), 1U);

    TestChannel::ReadCall readCall = d_channelImpl.popReadCall();

    // Build a mutable blob to pass to the read callback
    bdlbb::Blob mutableBlob(blob, bmqtst::TestHelperUtil::allocator());

    int numNeeded = 1;
    readCall.d_readCallback(Status(bmqtst::TestHelperUtil::allocator()),
                            &numNeeded,
                            &mutableBlob);
}

void TestContext::advanceTime(int milliseconds)
{
    d_timeSource.advanceTime(
        bsls::TimeInterval().addMilliseconds(milliseconds));
}

void TestContext::checkResult(ChannelFactoryEvent::Enum event)
{
    BMQTST_ASSERT_GE(d_results.size(), 1U);

    const ResultItem& item = d_results.front();
    BMQTST_ASSERT_EQ(item.d_event, event);
    d_results.pop_front();
}

size_t TestContext::resultsCount() const
{
    return d_results.size();
}

TestChannel& TestContext::testChannel()
{
    return d_channelImpl;
}

AuthenticatedChannelFactory& TestContext::obj()
{
    return d_factory.object();
}

TestChannelFactory& TestContext::baseFactory()
{
    return d_baseFactory;
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

BMQTST_TEST(NoCredentialCallback)
// ------------------------------------------------------------------------
// NO CREDENTIAL CALLBACK
//
// Concerns:
//   1. When no authentication credential callback is provided, the
//      factory passes through CHANNEL_UP without attempting
//      authentication.
//
// Plan:
//   1. Create factory with empty authnCredentialCb.
//   2. Connect and emit CHANNEL_UP from base factory.
//   3. Verify user receives CHANNEL_UP immediately.
//   4. Verify no writes to channel (no authentication attempt).
// ------------------------------------------------------------------------
{
    AuthnCredentialCb emptyCb;
    TestContext       ctx(emptyCb);

    ctx.connect();
    ctx.emitChannelUp();

    // User should receive CHANNEL_UP immediately - no auth attempt
    ctx.checkResult(ChannelFactoryEvent::e_CHANNEL_UP);
    BMQTST_ASSERT_EQ(ctx.resultsCount(), 0U);

    // No writes to channel (no authentication attempt)
    BMQTST_ASSERT_EQ(ctx.testChannel().numWriteCalls(), 0U);
}

BMQTST_TEST(BaseFactoryFailure)
// ------------------------------------------------------------------------
// BASE FACTORY FAILURE
//
// Concerns:
//   1. When the base factory emits CONNECT_FAILED, the authenticated
//      factory propagates the failure without attempting authentication.
//
// Plan:
//   1. Connect and emit CONNECT_FAILED from base factory.
//   2. Verify user receives CONNECT_FAILED.
//   3. Verify no writes to channel (no authentication attempt).
// ------------------------------------------------------------------------
{
    TestContext ctx;

    ctx.connect();

    // Emit CONNECT_FAILED from the base factory
    BMQTST_ASSERT_EQ(ctx.baseFactory().connectCalls().size(), 1U);
    const TestChannelFactory::ConnectCall& call =
        ctx.baseFactory().connectCalls().front();
    call.d_cb(ChannelFactoryEvent::e_CONNECT_FAILED,
              Status(bmqtst::TestHelperUtil::allocator()),
              bsl::shared_ptr<Channel>());
    ctx.baseFactory().connectCalls().pop_front();

    // User should receive CONNECT_FAILED
    ctx.checkResult(ChannelFactoryEvent::e_CONNECT_FAILED);
    BMQTST_ASSERT_EQ(ctx.resultsCount(), 0U);

    // No authentication attempt
    BMQTST_ASSERT_EQ(ctx.testChannel().numWriteCalls(), 0U);
}

BMQTST_TEST(SuccessfulAuthentication)
// ------------------------------------------------------------------------
// SUCCESSFUL AUTHENTICATION
//
// Concerns:
//   1. A successful connection with authentication results in
//      CHANNEL_UP being delivered to the user.
//
// Plan:
//   1. Connect and emit CHANNEL_UP from base factory.
//   2. Verify authentication request written to channel.
//   3. Verify read registered on channel for broker response.
//   4. Simulate broker E_SUCCESS response.
//   5. Verify user receives CHANNEL_UP.
// ------------------------------------------------------------------------
{
    TestContext ctx;

    ctx.connect();
    ctx.emitChannelUp();

    // 1 write to channel (authentication request)
    BMQTST_ASSERT_EQ(ctx.testChannel().numWriteCalls(), 1U);
    ctx.testChannel().popWriteCall();

    // 1 read registered on channel
    BMQTST_ASSERT_EQ(ctx.testChannel().numReadCalls(), 1U);

    // No result to the user yet
    BMQTST_ASSERT_EQ(ctx.resultsCount(), 0U);

    // Simulate broker success response
    bdlbb::Blob response = ctx.buildAuthResponse(
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS);
    ctx.simulateBrokerResponse(response);

    // User should receive CHANNEL_UP
    ctx.checkResult(ChannelFactoryEvent::e_CHANNEL_UP);
    BMQTST_ASSERT_EQ(ctx.resultsCount(), 0U);
}

BMQTST_TEST(AuthenticationWithLifetime)
// ------------------------------------------------------------------------
// AUTHENTICATION WITH LIFETIME
//
// Concerns:
//   1. When the broker response includes a lifetimeMs value, a
//      reauthentication timer is scheduled and fires at the expected
//      interval.
//
// Plan:
//   1. Authenticate successfully with lifetimeMs = 10000.
//   2. Verify user receives CHANNEL_UP.
//   3. Advance scheduler past the computed timeout interval (5000ms).
//   4. Verify a reauthentication request is written to the channel.
// ------------------------------------------------------------------------
{
    TestContext ctx;

    ctx.connect();
    ctx.emitChannelUp();

    // Consume the initial write (authentication request)
    BMQTST_ASSERT_EQ(ctx.testChannel().numWriteCalls(), 1U);
    ctx.testChannel().popWriteCall();

    // Simulate broker success response with lifetimeMs = 10000
    bdlbb::Blob response =
        ctx.buildAuthResponse(bmqp_ctrlmsg::StatusCategory::E_SUCCESS, 10000);
    ctx.simulateBrokerResponse(response);

    // User should receive CHANNEL_UP
    ctx.checkResult(ChannelFactoryEvent::e_CHANNEL_UP);
    BMQTST_ASSERT_EQ(ctx.resultsCount(), 0U);

    // No additional writes yet
    BMQTST_ASSERT_EQ(ctx.testChannel().numWriteCalls(), 0U);

    // timeoutInterval(10000):
    //   intervalMsWithRatio  = 10000 * 0.9 = 9000
    //   intervalMsWithBuffer = max(0, 10000 - 5000) = 5000
    //   result = min(9000, 5000) = 5000
    // Advance past the reauthentication timeout
    ctx.advanceTime(5001);

    // Should have triggered a reauthentication write
    BMQTST_ASSERT_EQ(ctx.testChannel().numWriteCalls(), 1U);
    ctx.testChannel().popWriteCall();
}

BMQTST_TEST(CredentialCallbackFailure)
// ------------------------------------------------------------------------
// CREDENTIAL CALLBACK FAILURE
//
// Concerns:
//   1. When the credential callback returns nullopt, the factory delivers
//      exactly one CONNECT_FAILED and does not attempt to read a
//      response.
//
// Plan:
//   1. Create factory with a credential callback that returns nullopt.
//   2. Connect and emit CHANNEL_UP from base factory.
//   3. Verify user receives exactly one CONNECT_FAILED.
//   4. Verify no read registered on channel.
// ------------------------------------------------------------------------
{
    AuthnCredentialCb nulloptCb =
        [](bsl::ostream& error) -> bsl::optional<bmqt::AuthnCredential> {
        error << "credential unavailable";
        return bsl::nullopt;
    };
    TestContext ctx(nulloptCb);

    ctx.connect();
    ctx.emitChannelUp();

    // User should receive CONNECT_FAILED (exactly once)
    ctx.checkResult(ChannelFactoryEvent::e_CONNECT_FAILED);
    BMQTST_ASSERT_EQ(ctx.resultsCount(), 0U);

    // No read registered on channel (Bug A fix validation)
    BMQTST_ASSERT_EQ(ctx.testChannel().numReadCalls(), 0U);
}

BMQTST_TEST(WriteFailure)
// ------------------------------------------------------------------------
// WRITE FAILURE
//
// Concerns:
//   1. When the channel write fails, the factory delivers exactly one
//      CONNECT_FAILED and does not attempt to read a response.
//
// Plan:
//   1. Set channel write status to error.
//   2. Connect and emit CHANNEL_UP from base factory.
//   3. Verify user receives exactly one CONNECT_FAILED.
//   4. Verify no read registered on channel.
// ------------------------------------------------------------------------
{
    TestContext ctx;

    // Set the channel to return error on write
    Status errorStatus(StatusCategory::e_GENERIC_ERROR,
                       "writeError",
                       -1,
                       bmqtst::TestHelperUtil::allocator());
    ctx.testChannel().setWriteStatus(errorStatus);

    ctx.connect();
    ctx.emitChannelUp();

    // User should receive CONNECT_FAILED (exactly once)
    ctx.checkResult(ChannelFactoryEvent::e_CONNECT_FAILED);
    BMQTST_ASSERT_EQ(ctx.resultsCount(), 0U);

    // No read registered on channel (Bug A fix validation)
    BMQTST_ASSERT_EQ(ctx.testChannel().numReadCalls(), 0U);
}

BMQTST_TEST(ReadSetupFailure)
// ------------------------------------------------------------------------
// READ SETUP FAILURE
//
// Concerns:
//   1. When the channel read() call returns an error status, the factory
//      delivers CONNECT_FAILED after a successful write.
//
// Plan:
//   1. Set channel read status to error.
//   2. Connect and emit CHANNEL_UP from base factory.
//   3. Verify authentication request was written successfully.
//   4. Verify user receives CONNECT_FAILED.
// ------------------------------------------------------------------------
{
    TestContext ctx;

    // Set the channel to return error on read
    Status errorStatus(StatusCategory::e_GENERIC_ERROR,
                       "readError",
                       -1,
                       bmqtst::TestHelperUtil::allocator());
    ctx.testChannel().setReadStatus(errorStatus);

    ctx.connect();
    ctx.emitChannelUp();

    // Write to channel succeeds (sendRequest OK)
    BMQTST_ASSERT_EQ(ctx.testChannel().numWriteCalls(), 1U);
    ctx.testChannel().popWriteCall();

    // User should receive CONNECT_FAILED
    ctx.checkResult(ChannelFactoryEvent::e_CONNECT_FAILED);
    BMQTST_ASSERT_EQ(ctx.resultsCount(), 0U);
}

BMQTST_TEST(ReadCallbackFailure)
// ------------------------------------------------------------------------
// READ CALLBACK FAILURE
//
// Concerns:
//   1. When the read callback fires with an error status, the factory
//      delivers CONNECT_FAILED.
//
// Plan:
//   1. Connect, emit CHANNEL_UP, and consume the auth request write.
//   2. Pop the read callback and invoke it with error status.
//   3. Verify user receives CONNECT_FAILED.
// ------------------------------------------------------------------------
{
    TestContext ctx;

    ctx.connect();
    ctx.emitChannelUp();

    // Consume the write
    BMQTST_ASSERT_EQ(ctx.testChannel().numWriteCalls(), 1U);
    ctx.testChannel().popWriteCall();

    BMQTST_ASSERT_EQ(ctx.testChannel().numReadCalls(), 1U);

    // Invoke read callback with error status
    TestChannel::ReadCall readCall = ctx.testChannel().popReadCall();
    Status                errorStatus(StatusCategory::e_GENERIC_ERROR,
                       "readFailed",
                       -1,
                       bmqtst::TestHelperUtil::allocator());
    int                   numNeeded = 1;
    readCall.d_readCallback(errorStatus, &numNeeded, 0);

    // User should receive CONNECT_FAILED
    ctx.checkResult(ChannelFactoryEvent::e_CONNECT_FAILED);
    BMQTST_ASSERT_EQ(ctx.resultsCount(), 0U);
}

BMQTST_TEST(InvalidBrokerResponse)
// ------------------------------------------------------------------------
// INVALID BROKER RESPONSE
//
// Concerns:
//   1. When the broker sends an invalid authentication event in response
//      to an AuthenticationRequest (i.e. an AuthenticationRequest), the
//      factory delivers CONNECT_FAILED.
//
// Plan:
//   1. Connect, emit CHANNEL_UP, and consume the auth request write.
//   2. Build an authentication event containing a request (not response).
//   3. Deliver it via the read callback.
//   4. Verify user receives CONNECT_FAILED.
// ------------------------------------------------------------------------
{
    TestContext ctx;

    ctx.connect();
    ctx.emitChannelUp();

    // Consume the write
    BMQTST_ASSERT_EQ(ctx.testChannel().numWriteCalls(), 1U);
    ctx.testChannel().popWriteCall();

    BMQTST_ASSERT_EQ(ctx.testChannel().numReadCalls(), 1U);

    // Build an event that the client does not expect (authentication request)
    bmqp_ctrlmsg::AuthenticationMessage authMessage(
        bmqtst::TestHelperUtil::allocator());
    bmqp_ctrlmsg::AuthenticationRequest& req =
        authMessage.makeAuthenticationRequest();
    req.mechanism() = "BASIC";

    bdlbb::PooledBlobBufferFactory bufferFactory(
        4096,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool =
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator());

    bmqp::SchemaEventBuilder builder(blobSpPool.get(),
                                     bmqp::EncodingType::e_BER,
                                     bmqtst::TestHelperUtil::allocator());
    int                      rc = builder.setMessage(authMessage,
                                bmqp::EventType::e_AUTHENTICATION);
    BMQTST_ASSERT_EQ(rc, 0);

    bdlbb::Blob requestBlob(*builder.blob(),
                            bmqtst::TestHelperUtil::allocator());

    TestChannel::ReadCall readCall  = ctx.testChannel().popReadCall();
    int                   numNeeded = 1;
    readCall.d_readCallback(Status(bmqtst::TestHelperUtil::allocator()),
                            &numNeeded,
                            &requestBlob);

    // User should receive CONNECT_FAILED
    ctx.checkResult(ChannelFactoryEvent::e_CONNECT_FAILED);
    BMQTST_ASSERT_EQ(ctx.resultsCount(), 0U);
}

BMQTST_TEST(BrokerResponseNotAuthenticationEvent)
// ------------------------------------------------------------------------
// BROKER RESPONSE NOT AUTHENTICATION EVENT
//
// Concerns:
//   1. When the broker sends a valid BlazingMQ event that is not an
//      authentication event (e.g., a control event), the factory
//      delivers CONNECT_FAILED.
//
// Plan:
//   1. Connect, emit CHANNEL_UP, and consume the auth request write.
//   2. Build a control event and deliver it via the read callback.
//   3. Verify user receives CONNECT_FAILED.
// ------------------------------------------------------------------------
{
    TestContext ctx;

    ctx.connect();
    ctx.emitChannelUp();

    // Consume the write
    BMQTST_ASSERT_EQ(ctx.testChannel().numWriteCalls(), 1U);
    ctx.testChannel().popWriteCall();

    BMQTST_ASSERT_EQ(ctx.testChannel().numReadCalls(), 1U);

    // Build a control event instead of authentication event
    bmqp_ctrlmsg::ControlMessage controlMessage(
        bmqtst::TestHelperUtil::allocator());
    controlMessage.choice().makeStatus();
    controlMessage.choice().status().category() =
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS;

    bdlbb::PooledBlobBufferFactory bufferFactory(
        4096,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool =
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator());

    bmqp::SchemaEventBuilder builder(blobSpPool.get(),
                                     bmqp::EncodingType::e_BER,
                                     bmqtst::TestHelperUtil::allocator());
    int rc = builder.setMessage(controlMessage, bmqp::EventType::e_CONTROL);
    BMQTST_ASSERT_EQ(rc, 0);

    bdlbb::Blob controlBlob(*builder.blob(),
                            bmqtst::TestHelperUtil::allocator());

    TestChannel::ReadCall readCall  = ctx.testChannel().popReadCall();
    int                   numNeeded = 1;
    readCall.d_readCallback(Status(bmqtst::TestHelperUtil::allocator()),
                            &numNeeded,
                            &controlBlob);

    // User should receive CONNECT_FAILED
    ctx.checkResult(ChannelFactoryEvent::e_CONNECT_FAILED);
    BMQTST_ASSERT_EQ(ctx.resultsCount(), 0U);
}

BMQTST_TEST(AuthenticationRefused)
// ------------------------------------------------------------------------
// AUTHENTICATION REFUSED
//
// Concerns:
//   1. When the broker responds with an unsuccessful status, the factory
//      delivers CONNECT_FAILED.
//
// Plan:
//   1. Connect, emit CHANNEL_UP, and consume the auth request write.
//   2. Simulate broker response with E_REFUSED.
//   3. Verify user receives CONNECT_FAILED.
// ------------------------------------------------------------------------
{
    TestContext ctx;

    ctx.connect();
    ctx.emitChannelUp();

    // Consume the write
    BMQTST_ASSERT_EQ(ctx.testChannel().numWriteCalls(), 1U);
    ctx.testChannel().popWriteCall();

    // Simulate broker refused response
    bdlbb::Blob response = ctx.buildAuthResponse(
        bmqp_ctrlmsg::StatusCategory::E_REFUSED);
    ctx.simulateBrokerResponse(response);

    // User should receive CONNECT_FAILED
    ctx.checkResult(ChannelFactoryEvent::e_CONNECT_FAILED);
    BMQTST_ASSERT_EQ(ctx.resultsCount(), 0U);
}

BMQTST_TEST(ChannelDownCancelsReauthentication)
// ------------------------------------------------------------------------
// CHANNEL DOWN CANCELS REAUTHENTICATION
//
// Concerns:
//   1. When the channel goes down after a successful authentication with
//      a lifetime, the reauthentication timer is cancelled.
//
// Plan:
//   1. Authenticate successfully with lifetimeMs = 10000.
//   2. Simulate channel down via the onClose callback.
//   3. Advance scheduler past the reauthentication time.
//   4. Verify no additional writes to channel.
// ------------------------------------------------------------------------
{
    TestContext ctx;

    ctx.connect();
    ctx.emitChannelUp();

    // Consume the initial write (authentication request)
    BMQTST_ASSERT_EQ(ctx.testChannel().numWriteCalls(), 1U);
    ctx.testChannel().popWriteCall();

    // Simulate broker success response with lifetimeMs = 10000
    bdlbb::Blob response =
        ctx.buildAuthResponse(bmqp_ctrlmsg::StatusCategory::E_SUCCESS, 10000);
    ctx.simulateBrokerResponse(response);

    // User should receive CHANNEL_UP
    ctx.checkResult(ChannelFactoryEvent::e_CHANNEL_UP);

    // Simulate channel down by invoking the onClose callback
    BMQTST_ASSERT_GE(ctx.testChannel().numOnCloseCalls(), 1U);
    TestChannel::OnCloseCall onCloseCall = ctx.testChannel().popOnCloseCall();
    onCloseCall.d_closeFn(Status(StatusCategory::e_CONNECTION,
                                 bmqtst::TestHelperUtil::allocator()));

    // Advance scheduler past the reauthentication time
    ctx.advanceTime(11000);

    // No additional writes to channel (timer was cancelled)
    BMQTST_ASSERT_EQ(ctx.testChannel().numWriteCalls(), 0U);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    bmqtst::runTest(_testCase);

    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
