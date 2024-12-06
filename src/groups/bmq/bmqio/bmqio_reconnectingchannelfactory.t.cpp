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

// bmqio_reconnectingchannelfactory.t.cpp                             -*-C++-*-
#include <bmqio_reconnectingchannelfactory.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

#include <bmqio_testchannel.h>
#include <bmqio_testchannelfactory.h>
#include <bmqsys_mocktime.h>
#include <bmqu_printutil.h>

// BDE
#include <bdlf_bind.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_cstdlib.h>
#include <bsl_ctime.h>
#include <bsl_memory.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_annotation.h>
#include <bsls_objectbuffer.h>
#include <bsls_types.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;
using namespace bmqio;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

static const int k_RECONNECT_INTERVAL = 10;
static const int k_MAX_INTERVAL       = 100;

/// constants used for the reconnection logic
static const int k_INTERVAL_RESET_LIMIT = 200;

// ------------
// class Tester
// ------------

class Tester : public bmqtst::Test {
  private:
    // TYPES

    /// Struct holding the information emitted by the
    /// ReconnectingChannelFactory to the user, via the connect callback.
    struct ConnectResultItem {
        ChannelFactoryEvent::Enum d_event;   // The event that was delivered
        Status                    d_status;  // The associated status

        ConnectResultItem(ChannelFactoryEvent::Enum event,
                          const Status&             status)
        : d_event(event)
        , d_status(status)
        {
        }
    };

    // DATA
    bmqsys::MockTime                               d_mockTime;
    bdlmt::EventScheduler                          d_scheduler;
    bdlmt::EventSchedulerTestTimeSource            d_timeSource;
    TestChannelFactory                             d_baseFactory;
    bsls::ObjectBuffer<ReconnectingChannelFactory> d_reconnectingFactory;
    bsl::vector<bsl::string>                       d_resolverResults;
    // The endpoints to result from the resolverFn method.
    bsl::deque<ConnectResultItem> d_connectResultItems;
    // List of calls to the connect callback made by the
    // ReconnectingFactory under test.
    bslma::ManagedPtr<ChannelFactoryOperationHandle> d_connectHandle;
    // The handle supplied to connect.
    TestChannel d_channel;
    // The channel to use when simulating a successfull channel up
    // connection.

  private:
    /// Callback supplied to the ReconnectingChannelFactory for use when it
    /// needs to figure out how long to wait before scheduling the next
    /// connection attempt.
    void connectIntervalFn(bsls::TimeInterval*       interval,
                           const ConnectOptions&     options,
                           const bsls::TimeInterval& timeSinceLastAttempt);

    /// Callback supplied to the ReconnectingChannelFactory for use when it
    /// needs to resolve a connection endpoint.
    void resolverFn(bsl::vector<bsl::string>* out,
                    const ConnectOptions&     options);

    /// Callback supplied to the `connect` call.  This method appends all
    /// parameters to the `d_connectResultItems` queue that can later be
    /// queried to ensure the proper events were emitted by the
    /// ReconnectingChannelFactory.
    void connectResultCb(ChannelFactoryEvent::Enum       event,
                         const Status&                   status,
                         const bsl::shared_ptr<Channel>& channel);

  public:
    Tester();
    ~Tester() BSLS_KEYWORD_OVERRIDE;

    ReconnectingChannelFactory& obj();
    TestChannelFactory&         baseFactory();

    /// Return a reference offering modifiable access to the object
    /// currently under test and its base factory as well as the test
    /// channel.
    TestChannel& testChannel();

    /// Set the list of endpoints the resolverFn should next return to the
    /// `count` number of endpoints starting at `endpoints`.
    void setResolverResults(const char** endpoints, size_t count);

    /// Advance the scheduler by the specified `value`; this scheduler is
    /// the one used for scheduling the reconnection.
    void advanceSchedulerTime(int value);

    // void advanceMockTime(int value);
    // Advance the high res timer by the specified 'value'.  This timer is
    // used to keep track of the last successfull connect.

    /// Initiate a connect from the object under test, using the specified
    /// `options`.
    void connect(const ConnectOptions& options);

    /// Simulate a drop of the channel previously returned from a
    /// successfull connect.
    void closeChannel();

    /// Make sure the base factory received a `connect` call from the object
    /// under test, for the specified `endpoint`, and invoke the connect
    /// resultCb with the specified `event`.  If `event` corresponds to
    /// `e_CHANNEL_OP`, a testChannel is created and supplied to the
    /// callback.  The specified `line` is used for friendlier log on error.
    void ensureConnectAndEmitEvent(int                       line,
                                   bslstl::StringRef         endpoint,
                                   ChannelFactoryEvent::Enum event);

    /// Ensure the connect resultCb was invoked from the object under test,
    /// and with the specified `event`.  The specified `line` is used for
    /// friendlier log on error.
    void checkResult(int line, ChannelFactoryEvent::Enum event);

    /// Return the number of not yet processed invocation of the connect
    /// resultCb.
    int resultsCount();
};

void Tester::connectIntervalFn(bsls::TimeInterval*       interval,
                               const ConnectOptions&     options,
                               const bsls::TimeInterval& timeSinceLastAttempt)
{
    // For the test driver purpose, we don't care about exponential reconnect
    // and jitter, so this method is a simplified version of the default one
    // ('ReconnectingChannelFactoryUtil::defaultConnectIntervalFn'), which ends
    // up always returning the attempt interval supplied in the connect
    // options; however it is left in a 'dumb-looking' state so that it looks
    // exactly like the implementation of the real method, with the random and
    // exponential taken out.

    PVV("connectIntervalFn (interval: "
        << *interval << ", timeSinceLastAttempt: " << timeSinceLastAttempt
        << ")");

    if (timeSinceLastAttempt >= k_INTERVAL_RESET_LIMIT) {
        interval->setTotalNanoseconds(0);
        return;  // RETURN
    }

    if (*interval == bsls::TimeInterval()) {
        interval->setTotalNanoseconds(0);
    }

    *interval = options.attemptInterval();

    if (*interval > k_MAX_INTERVAL) {
        *interval = k_MAX_INTERVAL;
    }
}

void Tester::resolverFn(bsl::vector<bsl::string>* out,
                        const ConnectOptions&     options)
{
    PVV("Resolving '" << options.endpoint() << "' to '"
                      << bmqu::PrintUtil::printer(d_resolverResults) << "'");
    *out = d_resolverResults;
}

void Tester::connectResultCb(ChannelFactoryEvent::Enum       event,
                             const Status&                   status,
                             const bsl::shared_ptr<Channel>& channel)
{
    PVV("ConnectResultCb: [event: " << event << ", status: " << status
                                    << ", channel: " << channel.get() << "]");

    d_connectResultItems.emplace_back(event, status);
}

Tester::Tester()
: d_scheduler(bmqtst::TestHelperUtil::allocator())
, d_timeSource(&d_scheduler)
, d_baseFactory(bmqtst::TestHelperUtil::allocator())
, d_resolverResults(bmqtst::TestHelperUtil::allocator())
, d_connectResultItems(bmqtst::TestHelperUtil::allocator())
, d_connectHandle()
, d_channel(bmqtst::TestHelperUtil::allocator())
{
    d_scheduler.start();

    ReconnectingChannelFactoryConfig config(
        &d_baseFactory,
        &d_scheduler,
        bmqtst::TestHelperUtil::allocator());
    config
        .setEndpointResolveFn(bdlf::BindUtil::bind(&Tester::resolverFn,
                                                   this,
                                                   bdlf::PlaceHolders::_1,
                                                   bdlf::PlaceHolders::_2))
        .setReconnectIntervalFn(
            bdlf::BindUtil::bind(&Tester::connectIntervalFn,
                                 this,
                                 bdlf::PlaceHolders::_1,
                                 bdlf::PlaceHolders::_2,
                                 bdlf::PlaceHolders::_3));

    new (d_reconnectingFactory.buffer())
        ReconnectingChannelFactory(config,
                                   bmqtst::TestHelperUtil::allocator());
    obj().start();
}

Tester::~Tester()
{
    // Some invariants checking
    BMQTST_ASSERT(d_connectResultItems.empty());
    BMQTST_ASSERT(d_baseFactory.connectCalls().empty());

    obj().stop();
    d_reconnectingFactory.object()
        .bmqio::ReconnectingChannelFactory ::~ReconnectingChannelFactory();

    d_scheduler.stop();
}

ReconnectingChannelFactory& Tester::obj()
{
    return d_reconnectingFactory.object();
}

TestChannelFactory& Tester::baseFactory()
{
    return d_baseFactory;
}

TestChannel& Tester::testChannel()
{
    return d_channel;
}

void Tester::setResolverResults(const char** endpoints, size_t count)
{
    d_resolverResults.clear();

    while (count--) {
        d_resolverResults.emplace_back(
            bsl::string(*endpoints++, bmqtst::TestHelperUtil::allocator()));
    }
}

void Tester::advanceSchedulerTime(int value)
{
    d_timeSource.advanceTime(bsls::TimeInterval(value));
}

// void
// Tester::advanceMockTime(int value)
// {
//     d_mockTime.advanceHighResTimer(value);
// }

void Tester::connect(const ConnectOptions& options)
{
    PVV("Connecting using '" << options << "'");

    // Inform the baseFactory how it should respond to the connect.
    const Status successStatus(StatusCategory::e_SUCCESS,
                               bmqtst::TestHelperUtil::allocator());
    baseFactory().setConnectStatus(successStatus);

    Status status(bmqtst::TestHelperUtil::allocator());
    obj().connect(&status,
                  &d_connectHandle,
                  options,
                  bdlf::BindUtil::bind(&Tester::connectResultCb,
                                       this,
                                       bdlf::PlaceHolders::_1,
                                       bdlf::PlaceHolders::_2,
                                       bdlf::PlaceHolders::_3));
    BMQTST_ASSERT(status);
}

void Tester::closeChannel()
{
    PV("Closing channel");

    // Ensure that the object under test registered itself to the as an
    // observer of the channel down.
    BMQTST_ASSERT_EQ(d_channel.onCloseCalls().size(), 1U);

    TestChannel::OnCloseCall& call = d_channel.onCloseCalls().front();
    call.d_closeFn(Status(StatusCategory::e_CONNECTION,
                          bmqtst::TestHelperUtil::allocator()));
    d_channel.onCloseCalls().pop_front();
}

void Tester::ensureConnectAndEmitEvent(int                       line,
                                       bslstl::StringRef         endpoint,
                                       ChannelFactoryEvent::Enum event)
{
    // The baseFactory must have received a 'connect' call from the object
    // under test.
    BMQTST_ASSERT_EQ_D("Line: " << line,
                       d_baseFactory.connectCalls().size(),
                       1U);

    const TestChannelFactory::ConnectCall& call =
        d_baseFactory.connectCalls().front();
    // Ensure the endpoint matches the expectation
    BMQTST_ASSERT_EQ_D("Line: " << line,
                       bsl::string(endpoint,
                                   bmqtst::TestHelperUtil::allocator()),
                       call.d_options.endpoint());

    bsl::shared_ptr<Channel> channel = bsl::shared_ptr<Channel>();
    if (event == ChannelFactoryEvent::e_CHANNEL_UP) {
        channel.reset(&d_channel, bslstl::SharedPtrNilDeleter());
    }

    PV("Emiting " << event << " from the testFactory for " << endpoint);
    call.d_cb(event, Status(bmqtst::TestHelperUtil::allocator()), channel);
    d_baseFactory.connectCalls().pop_front();
}

void Tester::checkResult(int line, ChannelFactoryEvent::Enum event)
{
    BMQTST_ASSERT_EQ_D("Line: " << line, d_connectResultItems.size(), 1U);

    const ConnectResultItem& item = d_connectResultItems.front();
    BMQTST_ASSERT_EQ_D("Line: " << line, item.d_event, event);
    d_connectResultItems.pop_front();
}

int Tester::resultsCount()
{
    return d_connectResultItems.size();
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

BMQTST_TEST_F(Tester, SingleHost)
// ------------------------------------------------------------------------
// In this test:
// 1. connect to a single-resolved host, with a numAttempt of 3, and make
//    it succeed at second attempt
// 2. drop the channel, and ensure a reconnection happens, which will
//    perform 3 failing attempts before giving up for ever
// ------------------------------------------------------------------------
{
    const char* k_ENDPOINT[] = {"singleHost:123"};
    setResolverResults(k_ENDPOINT, 1);

    ConnectOptions options(bmqtst::TestHelperUtil::allocator());
    options.setEndpoint("dummyWillBeResolved:123")
        .setNumAttempts(3)
        .setAttemptInterval(bsls::TimeInterval(k_RECONNECT_INTERVAL))
        .setAutoReconnect(true);

    {
        PV("connect, [Fail - Connect]");
        connect(options);

        // First attempt, should fail and user receive a connect_attempt_failed
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINT[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED);

        // Advance by less than the reconnect interval, and verify no connect
        // were called yet
        advanceSchedulerTime(k_RECONNECT_INTERVAL - 1);
        BMQTST_ASSERT(baseFactory().connectCalls().empty());

        // Advance time to trigger the reconnect
        advanceSchedulerTime(1);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINT[0],
                                  ChannelFactoryEvent::e_CHANNEL_UP);
        checkResult(L_, ChannelFactoryEvent::e_CHANNEL_UP);

        // No more events expected
        advanceSchedulerTime(2 * k_RECONNECT_INTERVAL);
        BMQTST_ASSERT(baseFactory().connectCalls().empty());
    }

    {
        PV("close channel, check reconnect, [Fail - Fail - Fail]");
        closeChannel();

        // No connect should have happened yet
        advanceSchedulerTime(k_RECONNECT_INTERVAL - 1);
        BMQTST_ASSERT(baseFactory().connectCalls().empty());

        // Connect Attempt Failed [1/3]
        advanceSchedulerTime(1);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINT[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED);

        // Connect Attempt Failed [2/3]
        advanceSchedulerTime(k_RECONNECT_INTERVAL);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINT[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED);

        // Connect Failed [3/3]
        advanceSchedulerTime(k_RECONNECT_INTERVAL);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINT[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_FAILED);
    }

    // NumAttempt exhausted, no more connection expected
    advanceSchedulerTime(2 * k_RECONNECT_INTERVAL);
    BMQTST_ASSERT(baseFactory().connectCalls().empty());
}

BMQTST_TEST_F(Tester, MultipleHosts)
// ------------------------------------------------------------------------
// In this test:
// 1. connect to a 3 resolved hosts list, with a numAttempt of 3, and make
//    it succeed at second endpoint of second attempt
// 2. drop the channel, and ensure a reconnection happens, which will
//    perform 3 failing attempts of all 3 hosts before giving up for ever
// ------------------------------------------------------------------------
{
    const char* k_ENDPOINTS[] = {"first:123", "second:456", "third:789"};
    setResolverResults(k_ENDPOINTS, 3);

    ConnectOptions options(bmqtst::TestHelperUtil::allocator());
    options.setEndpoint("dummyWillBeResolved:123")
        .setNumAttempts(3)
        .setAttemptInterval(bsls::TimeInterval(k_RECONNECT_INTERVAL))
        .setAutoReconnect(true);

    {
        PV("connect, [{Fail x 3}, {Fail, Fail, Connect}]");
        connect(options);

        // First attempt, going through each of the 3 hosts with no delay
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[1],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[2],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED);

        // Advance by less than the reconnect interval, and verify no connect
        // were called yet
        advanceSchedulerTime(k_RECONNECT_INTERVAL - 1);
        BMQTST_ASSERT(baseFactory().connectCalls().empty());

        // Advance time to trigger the reconnect
        advanceSchedulerTime(1);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);

        // Make it succeed
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[1],
                                  ChannelFactoryEvent::e_CHANNEL_UP);
        checkResult(L_, ChannelFactoryEvent::e_CHANNEL_UP);

        // No more events expected
        advanceSchedulerTime(2 * k_RECONNECT_INTERVAL);
        BMQTST_ASSERT(baseFactory().connectCalls().empty());
    }

    {
        PV("close channel, check reconnect, [{Fail x 3} x 3]");
        closeChannel();

        // No connect should have happened yet
        advanceSchedulerTime(k_RECONNECT_INTERVAL - 1);
        BMQTST_ASSERT(baseFactory().connectCalls().empty());

        // Connect Attempt Failed [1/3]
        advanceSchedulerTime(1);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[1],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[2],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED);

        // Connect Attempt Failed [2/3]
        advanceSchedulerTime(k_RECONNECT_INTERVAL);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[1],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[2],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED);

        // Connect Failed [3/3]
        advanceSchedulerTime(k_RECONNECT_INTERVAL);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[1],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[2],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_FAILED);
    }

    // NumAttempt exhausted, no more connection expected
    advanceSchedulerTime(2 * k_RECONNECT_INTERVAL);
    BMQTST_ASSERT(baseFactory().connectCalls().empty());
}

BMQTST_TEST_F(Tester, EmptyAndChangingResolvingList)
// ------------------------------------------------------------------------
// In this test:
//    Verify that the Factory properly invokes the 'EndpointResolveFn' at
//    the expected time, and ensures it handles both a changing size of
//    resolved hosts as well as an empty resolved hosts list.
//------------------------------------------------------------------------
{
    // This will be used to temporarily change the resolver list, to ensure
    // the `EndpointResolveFn` was only called at expected time.
    const char* k_GARBAGE_ENDPOINTS[] = {"garbage:123"};

    ConnectOptions options(bmqtst::TestHelperUtil::allocator());
    options.setEndpoint("dummyWillBeResolved:123")
        .setNumAttempts(99)  // 'infinite' retry for this test
        .setAttemptInterval(bsls::TimeInterval(k_RECONNECT_INTERVAL))
        .setAutoReconnect(true);

    {
        PV("Resolving to two hosts, connect [{fail x 2}]");

        const char* k_ENDPOINTS[] = {"first:123", "second:456"};
        setResolverResults(k_ENDPOINTS, 2);

        connect(options);
        setResolverResults(k_GARBAGE_ENDPOINTS, 1);

        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[1],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED);
    }

    {
        PV("Resolving to three hosts, [{fail x 3}]");

        // Advance by less than the reconnect interval, before changing the
        // resolving hosts
        advanceSchedulerTime(k_RECONNECT_INTERVAL - 1);

        const char* k_ENDPOINTS[] = {"third:123", "fourth:456", "fifth:789"};
        setResolverResults(k_ENDPOINTS, 3);

        advanceSchedulerTime(1);
        setResolverResults(k_GARBAGE_ENDPOINTS, 1);

        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[1],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[2],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED);
    }

    {
        PV("Resolving to an empty hosts list");

        // Advance by less than the reconnect interval, before changing the
        // resolving hosts
        advanceSchedulerTime(k_RECONNECT_INTERVAL - 1);

        const char* k_ENDPOINTS[] = {"dummy"};
        setResolverResults(k_ENDPOINTS, 0);

        advanceSchedulerTime(1);

        BMQTST_ASSERT(baseFactory().connectCalls().empty());
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED);

        advanceSchedulerTime(k_RECONNECT_INTERVAL);
        BMQTST_ASSERT(baseFactory().connectCalls().empty());
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED);
    }

    {
        PV("Resolving to two hosts, [{fail x 2}]");

        // Advance by less than the reconnect interval, before changing the
        // resolving hosts
        advanceSchedulerTime(k_RECONNECT_INTERVAL - 1);

        const char* k_ENDPOINTS[] = {"sixth:123", "seventh:456"};
        setResolverResults(k_ENDPOINTS, 2);

        advanceSchedulerTime(1);
        setResolverResults(k_GARBAGE_ENDPOINTS, 1);

        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[1],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED);
    }
}

BMQTST_TEST_F(Tester, NonReconnecting)
// ------------------------------------------------------------------------
// In this test:
//    Verify the proper handling of non-reconnecting connect calls.
//
// NOTE: in this test we dont verify the 'timing' of operations, we mostly
//       focus on the right calls to the base factory and events back to
//       the user.
//
// We can't 'simulate' a channel close, because for non-reconnecting
// connections, the ReconnectingChannelFactory does NOT register itself as
// an observer of the channel down event.  Therefore instead of closing the
// channel, we just verify that there indeed was no call to 'onClose',
// which is equivalent to closing the channel because the Reconnecting is
// not going to be made aware of the close, therefore we know it won't try
// to reconnect.
// ------------------------------------------------------------------------
{
    ConnectOptions options(bmqtst::TestHelperUtil::allocator());
    options.setEndpoint("dummyWillBeResolved:123")
        .setNumAttempts(3)
        .setAttemptInterval(bsls::TimeInterval(k_RECONNECT_INTERVAL))
        .setAutoReconnect(false);

    {
        PV("Non-reconnecting connect, single host: [Fail x 3]");

        const char* k_ENDPOINT[] = {"singleHost:123"};
        setResolverResults(k_ENDPOINT, 1);

        connect(options);

        // 1st attempt fail
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINT[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED);

        // 2nd attempt fail
        advanceSchedulerTime(k_RECONNECT_INTERVAL);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINT[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED);

        // 3rd attempt fail
        advanceSchedulerTime(k_RECONNECT_INTERVAL);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINT[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_FAILED);

        // No more events expected
        advanceSchedulerTime(2 * k_RECONNECT_INTERVAL);
        BMQTST_ASSERT(baseFactory().connectCalls().empty());
    }

    {
        PV("Non-reconnecting connect, single host: [Fail, Connect, Close");

        const char* k_ENDPOINT[] = {"singleHost:123"};
        setResolverResults(k_ENDPOINT, 1);

        connect(options);

        // 1st attempt fail
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINT[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED);

        // 2nd attempt success
        advanceSchedulerTime(k_RECONNECT_INTERVAL);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINT[0],
                                  ChannelFactoryEvent::e_CHANNEL_UP);
        checkResult(L_, ChannelFactoryEvent::e_CHANNEL_UP);

        // Ensure the factory did not register a channel down observer
        BMQTST_ASSERT(testChannel().onCloseCalls().empty());

        // No more events expected
        advanceSchedulerTime(2 * k_RECONNECT_INTERVAL);
        BMQTST_ASSERT(baseFactory().connectCalls().empty());
    }

    {
        PV("Non-reconnecting connect, multiple hosts: [{Fail * 3} * 3]");

        const char* k_ENDPOINTS[] = {"first:123", "second:456", "third:789"};
        setResolverResults(k_ENDPOINTS, 3);

        connect(options);

        // 1st attempt fail
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[1],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[2],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED);

        // 2nd attempt fail
        advanceSchedulerTime(k_RECONNECT_INTERVAL);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[1],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[2],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED);

        // 3rd attempt fail
        advanceSchedulerTime(k_RECONNECT_INTERVAL);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[1],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[2],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_FAILED);

        // No more events expected
        advanceSchedulerTime(2 * k_RECONNECT_INTERVAL);
        BMQTST_ASSERT(baseFactory().connectCalls().empty());
    }

    {
        PV("Non-reconnecting connect, multiple hosts: "
           "[{Fail * 3} {Fail, Success} Close");

        const char* k_ENDPOINTS[] = {"first:123", "second:456", "third:789"};
        setResolverResults(k_ENDPOINTS, 3);

        connect(options);

        // 1st attempt fail all 3 endpoints
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[1],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[2],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        checkResult(L_, ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED);

        // 2nd attempt fails first endpoint, succeeds on the second
        advanceSchedulerTime(k_RECONNECT_INTERVAL);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[0],
                                  ChannelFactoryEvent::e_CONNECT_FAILED);
        BMQTST_ASSERT_EQ(resultsCount(), 0);
        ensureConnectAndEmitEvent(L_,
                                  k_ENDPOINTS[1],
                                  ChannelFactoryEvent::e_CHANNEL_UP);
        checkResult(L_, ChannelFactoryEvent::e_CHANNEL_UP);

        // Ensure the factory did not register a channel down observer
        BMQTST_ASSERT(testChannel().onCloseCalls().empty());

        // No more events expected
        advanceSchedulerTime(2 * k_RECONNECT_INTERVAL);
        BMQTST_ASSERT(baseFactory().connectCalls().empty());
    }
}

BMQTST_TEST(DefaultConnectIntervalFn)
// ------------------------------------------------------------------------
// In this test:
//     Verify the well behaving of the default 'connectIntervalFn' method.
// ------------------------------------------------------------------------
{
    static const bsls::Types::Int64 k_RESET    = 100;  // minUpTimeBeforeReset
    static const bsls::Types::Int64 k_MAX      = 50;   // maxInterval
    static const bsls::Types::Int64 k_INTERVAL = 10;   // attemptInterval

    ConnectOptions options(bmqtst::TestHelperUtil::allocator());
    options.setAttemptInterval(bsls::TimeInterval(k_INTERVAL));

    // Initialize the random number generator
    bsl::srand(unsigned(bsl::time(0)));

    // Number of iterations, since the response is `fuzzy` due to random.
    static const int k_ITERATIONS = 1000;

#define FN_CHECK(EXPECTED_MIN, EXPECTED_MAX, INPUT, LAST_ATTEMPT_TIME)        \
    for (int iter = 0; iter < k_ITERATIONS; ++iter) {                         \
        bsls::TimeInterval input(INPUT);                                      \
        ReconnectingChannelFactoryUtil::defaultConnectIntervalFn(             \
            &input,                                                           \
            options,                                                          \
            bsls::TimeInterval(LAST_ATTEMPT_TIME),                            \
            bsls::TimeInterval(k_RESET),                                      \
            bsls::TimeInterval(k_MAX));                                       \
                                                                              \
        BMQTST_ASSERT_GE(input, bsls::TimeInterval(EXPECTED_MIN));            \
        BMQTST_ASSERT_LE(input, bsls::TimeInterval(EXPECTED_MAX));            \
    }

    // Convenience
    static const bsls::Types::Int64 k_INTERVAL_LOW  = k_INTERVAL / 2;
    static const bsls::Types::Int64 k_INTERVAL_HIGH = k_INTERVAL +
                                                      k_INTERVAL / 2;

    // If 'timeSinceLastAttempt' is beyond the reset time, it should always
    // return 0, regardless of the 'input' value.
    FN_CHECK(0, 0, 0, k_RESET);
    FN_CHECK(0, 0, 0, k_RESET + 1);
    FN_CHECK(0, 0, 1, k_RESET);
    FN_CHECK(0, 0, k_INTERVAL - 1, k_RESET);
    FN_CHECK(0, 0, k_INTERVAL, k_RESET);
    FN_CHECK(0, 0, k_INTERVAL + 1, k_RESET);

    // Initial call (will be supplied an interval value of '0'); should always
    // return the 'attemptInterval' with jitter for 'timeSinceLastAttempt <
    // resetTime'.
    FN_CHECK(k_INTERVAL_LOW, k_INTERVAL_HIGH, 0, 0);
    FN_CHECK(k_INTERVAL_LOW, k_INTERVAL_HIGH, 0, k_RESET - 1);

    // 'Regular' call should double with jitter
    FN_CHECK(k_INTERVAL, 2 * k_INTERVAL, k_INTERVAL, 0);
    FN_CHECK(k_INTERVAL + 5, 2 * (k_INTERVAL + 5), k_INTERVAL + 5, 0);

    // Call when 'interval' is beyond 'maxInterval', should return the
    // 'maxInterval'.
    FN_CHECK(k_MAX, k_MAX, k_MAX, 0);
    FN_CHECK(k_MAX, k_MAX, k_MAX + 1, 0);
    FN_CHECK(k_MAX, k_MAX, k_MAX + 1, k_RESET - 1);

#undef FN_CHECK
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqtst::runTest(_testCase);

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
