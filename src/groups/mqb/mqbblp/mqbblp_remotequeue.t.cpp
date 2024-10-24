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

// mqbblp_remotequeue.t.cpp                                           -*-C++-*-
#include <mqbblp_remotequeue.h>

// MQB
#include <mqbcfg_messages.h>
#include <mqbi_queue.h>
#include <mqbmock_cluster.h>
#include <mqbmock_dispatcher.h>
#include <mqbmock_domain.h>
#include <mqbmock_queue.h>
#include <mqbmock_queuehandle.h>
#include <mqbstat_brokerstats.h>
#include <mqbu_messageguidutil.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_routingconfigurationutils.h>
#include <bmqt_queueflags.h>

#include <bmqsys_time.h>
#include <bmqu_atomicstate.h>

// BDE
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_memory.h>
#include <bsl_queue.h>
#include <bslmt_semaphore.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// ================
// struct TestClock
// ================

struct TestClock {
    // DATA
    bdlmt::EventSchedulerTestTimeSource& d_timeSource;

    // CREATORS
    TestClock(bdlmt::EventSchedulerTestTimeSource& timeSource)
    : d_timeSource(timeSource)
    {
        // NOTHING
    }

    // MANIPULATORS
    bsls::TimeInterval realtimeClock() { return d_timeSource.now(); }

    bsls::TimeInterval monotonicClock() { return d_timeSource.now(); }

    bsls::Types::Int64 highResTimer()
    {
        return d_timeSource.now().totalNanoseconds();
    }
};

// =======
// helpers
// =======
void verifyBroadfcastPut(
    BSLS_ANNOTATION_UNUSED const bmqp::PutHeader& putHeader,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<bdlbb::Blob>& appData,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<bdlbb::Blob>& options,
    mqbi::QueueHandle*                                         source,
    mqbi::QueueHandle*                                         origin,
    size_t*                                                    count)
{
    ASSERT_EQ(source, origin);

    ++(*count);
}
}

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

/// mocking the bare minimum to make mqbblp::RemoteQueue postMessage work
class TestBench {
    typedef mqbblp::RemoteQueue::StateSpPool StateSpPool;

  public:
    struct PutEvent {
        const bmqp::PutHeader        d_header;
        mqbi::DispatcherClient*      d_source;
        bsl::shared_ptr<bdlbb::Blob> d_appData;
        bsl::shared_ptr<bdlbb::Blob> d_options;

        PutEvent(const mqbi::DispatcherPutEvent& event,
                 mqbi::DispatcherClient*         source);
    };
    class TestRemoteQueue {
      public:
        bsl::shared_ptr<mqbmock::Queue> d_queue_sp;
        mqbu::StorageKey                d_storageKey;
        mqbblp::QueueState              d_queueState;
        mqbblp::RemoteQueue             d_remoteQueue;
        TestRemoteQueue(TestBench&                          theBench,
                        bmqt::Uri&                          uri,
                        int                                 timeout,
                        int                                 ackWindowSize,
                        bmqp_ctrlmsg::RoutingConfiguration& routingConfig);
    };

  public:
    bdlbb::PooledBlobBufferFactory      d_bufferFactory;
    mqbmock::Dispatcher                 d_dispatcher;
    mqbmock::Cluster                    d_cluster;
    mqbmock::Domain                     d_domain;
    mqbi::DispatcherEvent               d_event;
    mqbmock::Dispatcher::EventGuard     d_event_sp;
    bmqp_ctrlmsg::QueueHandleParameters d_params;
    bmqt::AckResult::Enum               d_status;
    bsl::queue<PutEvent>                d_puts;
    TestClock                           d_testClock;
    StateSpPool                         d_stateSpPool;
    bslma::Allocator*                   d_allocator_p;

    TestBench(bslma::Allocator* allocator_p);
    ~TestBench();

    bsl::shared_ptr<mqbmock::Queue> getQueue();

    void setup(mqbblp::RemoteQueue&                theQueue,
               bmqp_ctrlmsg::RoutingConfiguration& routingConfig);

    /// Install to handle (record) Cluster events
    void eventProcessor(const mqbi::DispatcherEvent& event);

    /// Ack all recorded PUTs
    void ackPuts(bmqt::AckResult::Enum status);

    void dropPuts();

    void advanceTime(const bsls::TimeInterval& step);
};

TestBench::TestBench(bslma::Allocator* allocator_p)
: d_bufferFactory(256, allocator_p)
, d_dispatcher(allocator_p)
, d_cluster(&d_bufferFactory, allocator_p)
, d_domain(&d_cluster, allocator_p)
, d_event(allocator_p)
, d_event_sp(d_dispatcher._withEvent(&d_cluster, &d_event))
, d_params(allocator_p)
, d_status(bmqt::AckResult::e_UNKNOWN)
, d_puts(allocator_p)
, d_testClock(d_cluster._timeSource())
, d_stateSpPool(8192, allocator_p)
, d_allocator_p(allocator_p)
{
    d_params.flags() |= bmqt::QueueFlags::e_WRITE;

    d_dispatcher._setInDispatcherThread(true);

    d_cluster._setEventProcessor(
        bdlf::BindUtil::bind(&TestBench::eventProcessor,
                             this,
                             bdlf::PlaceHolders::_1));

    bmqsys::Time::initialize(
        bdlf::BindUtil::bind(&TestClock::realtimeClock, &d_testClock),
        bdlf::BindUtil::bind(&TestClock::monotonicClock, &d_testClock),
        bdlf::BindUtil::bind(&TestClock::highResTimer, &d_testClock));

    bmqu::MemOutStream errorDescription(allocator_p);

    d_cluster.start(errorDescription);
}

TestBench::~TestBench()
{
    bmqt::UriParser::shutdown();

    d_cluster.stop();

    d_event.reset();

    bmqsys::Time::shutdown();
}

bsl::shared_ptr<mqbmock::Queue> TestBench::getQueue()
{
    bsl::shared_ptr<mqbmock::Queue> queue_sp(
        new (*d_allocator_p) mqbmock::Queue(&d_domain, d_allocator_p),
        d_allocator_p);

    queue_sp->_setDispatcher(&d_dispatcher);

    return queue_sp;
}

TestBench::PutEvent::PutEvent(const mqbi::DispatcherPutEvent& event,
                              mqbi::DispatcherClient*         source)
: d_header(event.putHeader())
, d_source(source)
, d_appData(event.blob())
, d_options(event.blob())
{
    // NOTHING
}

void TestBench::eventProcessor(const mqbi::DispatcherEvent& event)
{
    if (event.type() == mqbi::DispatcherEventType::e_PUT) {
        const mqbi::DispatcherPutEvent* realEvent =
            &event.getAs<mqbi::DispatcherPutEvent>();

        if (realEvent->isRelay()) {
            d_puts.push(PutEvent(*realEvent, event.source()));
        }
    }
}

void TestBench::ackPuts(bmqt::AckResult::Enum status)
{
    while (!d_puts.empty()) {
        const bmqp::PutHeader& ph = d_puts.front().d_header;
        bmqp::AckMessage       ackMessage(
            bmqp::ProtocolUtil::ackResultToCode(status),
            ph.correlationId(),
            ph.messageGUID(),
            ph.queueId());
        if (bmqp::PutHeaderFlagUtil::isSet(
                ph.flags(),
                bmqp::PutHeaderFlags::e_ACK_REQUESTED) ||
            status == bmqt::AckResult::e_NOT_READY) {
            mqbi::DispatcherEvent ackEvent(d_allocator_p);
            ackEvent.makeAckEvent()
                .setAckMessage(ackMessage)
                .setBlob(d_puts.front().d_appData)
                .setOptions(d_puts.front().d_options);
            d_puts.front().d_source->onDispatcherEvent(ackEvent);
        }
        d_puts.pop();
    }
}

void TestBench::dropPuts()
{
    bsl::queue<PutEvent>().swap(d_puts);
}

void TestBench::advanceTime(const bsls::TimeInterval& step)
{
    d_cluster.advanceTime(step.totalSeconds());
}

TestBench::TestRemoteQueue::TestRemoteQueue(
    TestBench&                          theBench,
    bmqt::Uri&                          uri,
    int                                 timeout,
    int                                 ackWindowSize,
    bmqp_ctrlmsg::RoutingConfiguration& routingConfig)
: d_queue_sp(theBench.getQueue())
, d_storageKey(mqbu::StorageKey::k_NULL_KEY)
, d_queueState(d_queue_sp.get(),
               uri,
               1,  // id
               d_storageKey,
               1,  // partition
               &theBench.d_domain,
               theBench.d_cluster._resources(),
               theBench.d_allocator_p)
, d_remoteQueue(&d_queueState,
                timeout,
                ackWindowSize,
                &theBench.d_stateSpPool,
                theBench.d_allocator_p)
{
    d_queueState.setRoutingConfig(routingConfig);
    d_queue_sp->_setDispatcherEventHandler(
        bdlf::BindUtil::bind(&mqbblp::RemoteQueue::onDispatcherEvent,
                             &d_remoteQueue,
                             bdlf::PlaceHolders::_1));  // event
    d_remoteQueue.onOpenUpstream(1, bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);
}

class TestQueueHandle : public mqbmock::QueueHandle {
  public:
    TestBench&                                               d_bench;
    mqbmock::DispatcherClient                                d_dc;
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext> d_clientContext;

    bsl::queue<bsl::shared_ptr<bdlbb::Blob> > d_data;
    bsl::queue<bsl::shared_ptr<bdlbb::Blob> > d_options;
    bsl::queue<bmqt::MessageGUID>             d_guids;

    bsl::queue<size_t> d_sequence;

    bmqt::AckResult::Enum d_status;
    size_t                d_postSequence;
    size_t                d_ackSequence;

    TestQueueHandle(const bsl::shared_ptr<mqbi::Queue>& queue_sp,
                    TestBench&                          theBench,
                    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                                 clientContext,
                    unsigned int upstreamSubQueueId = 1);

    ~TestQueueHandle() BSLS_KEYWORD_OVERRIDE;

    void postOneMessage(mqbblp::RemoteQueue* queue_p);

    void
    onAckMessage(const bmqp::AckMessage& ackMessage) BSLS_KEYWORD_OVERRIDE;

    size_t nextPostSequenceNumber();
    size_t nextAckSequenceNumber();

    size_t count();
};

TestQueueHandle::TestQueueHandle(
    const bsl::shared_ptr<mqbi::Queue>&                       queue_sp,
    TestBench&                                                theBench,
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    unsigned int upstreamSubQueueId)
: mqbmock::QueueHandle(queue_sp,
                       clientContext,
                       0,
                       theBench.d_params,
                       theBench.d_allocator_p)
, d_bench(theBench)
, d_dc(theBench.d_allocator_p)
, d_clientContext(clientContext)
, d_data(theBench.d_allocator_p)
, d_options(theBench.d_allocator_p)
, d_guids(theBench.d_allocator_p)
, d_sequence(theBench.d_allocator_p)
, d_status(bmqt::AckResult::e_SUCCESS)
, d_postSequence(0)
, d_ackSequence(0)
{
    d_clientContext->setClient(&d_dc);
    registerSubStream(bmqp_ctrlmsg::SubQueueIdInfo(),
                      upstreamSubQueueId,
                      mqbi::QueueCounts(1, 1));
}

TestQueueHandle::~TestQueueHandle()
{
    unregisterSubStream(bmqp_ctrlmsg::SubQueueIdInfo(),
                        mqbi::QueueCounts(1, 1),
                        true);
}

void TestQueueHandle::postOneMessage(mqbblp::RemoteQueue* queue_p)
{
    bsl::shared_ptr<bdlbb::Blob> data_sp;

    data_sp.createInplace(d_bench.d_allocator_p, &d_bench.d_bufferFactory);
    d_data.push(data_sp);

    // save data and options for future checks
    const char data[] = "data";

    data_sp->setLength(sizeof(data));
    strcpy(data_sp->buffer(0).data(), data);

    const char options[] = "options";

    bsl::shared_ptr<bdlbb::Blob> options_sp;

    options_sp.createInplace(d_bench.d_allocator_p, &d_bench.d_bufferFactory);

    d_options.push(options_sp);

    options_sp->setLength(sizeof(options));
    strcpy(options_sp->buffer(0).data(), options);

    bmqp::PutHeader   putHeader;
    bmqt::MessageGUID randomGUID;
    mqbu::MessageGUIDUtil::generateGUID(&randomGUID);
    putHeader.setMessageGUID(randomGUID);

    if (!queue()->isAtMostOnce()) {
        putHeader.setFlags(bmqp::PutHeaderFlags::e_ACK_REQUESTED);
        d_guids.push(randomGUID);

        d_sequence.push(nextPostSequenceNumber());
    }

    queue_p->postMessage(putHeader, data_sp, options_sp, this);
}

inline size_t TestQueueHandle::count()
{
    return d_sequence.size();
}

// virtual
void TestQueueHandle::onAckMessage(const bmqp::AckMessage& ackMessage)
{
    ASSERT_NE(0U, count());
    ASSERT_EQ(d_sequence.front(), nextAckSequenceNumber());

    ASSERT_EQ(d_guids.front(), ackMessage.messageGUID());
    ASSERT_EQ(d_status,
              bmqp::ProtocolUtil::ackResultFromCode(ackMessage.status()));

    d_sequence.pop();
    d_guids.pop();
    d_data.pop();
    d_options.pop();
}

size_t TestQueueHandle::nextPostSequenceNumber()
{
    return ++d_postSequence;
}

size_t TestQueueHandle::nextAckSequenceNumber()
{
    return ++d_ackSequence;
}

static void test1_fanoutBasic()
// ------------------------------------------------------------------------
// basic tests using fanout
//
// Concerns:
//   Proper behavior of PUTs (re)transmission.
//
// Plan:
//   1. Post messages. Ack messages with e_SUCCESS.
//      Producers should receive all ACKs.
//   2. Post messages. Ack messages with e_NOT_READY.
//      Producers should receive no ACKs.
//   3. Simulate queue reopening.
//      RemoteQueue retransmits pending messages.
//      Ack messages with e_SUCCESS.
//      Producers should receive all ACKs.
//   4. Post messages. Simulate timeout.
//      Producers should receive e_UNKNOWN ACKs.
//   5. Post messages.  Clear upstream of pending messages.
//      Simulate queue reopen.
//      RemoteQueue retransmits pending messages.
//      Ack messages with e_SUCCESS.
//      Producers should receive all ACKs.
//   6. Run timer which shoudl not rechedule because there are no pending.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("basic tests using fanout");

    bmqt::UriParser::initialize(s_allocator_p);

    bsl::shared_ptr<bmqst::StatContext> statContext =
        mqbstat::BrokerStatsUtil::initializeStatContext(30, s_allocator_p);

    TestBench theBench(s_allocator_p);

    bmqt::Uri uri("bmq://bmq.test.local/test_queue", s_allocator_p);
    bmqp_ctrlmsg::RoutingConfiguration routingConfig;
    size_t                             ackWindowSize = 1000;
    int                                timeout       = 10;
    TestBench::TestRemoteQueue         theQueue(theBench,
                                        uri,
                                        timeout,
                                        ackWindowSize,
                                        routingConfig);

    bsl::shared_ptr<mqbi::QueueHandleRequesterContext> clientContext_sp(
        new (*s_allocator_p) mqbi::QueueHandleRequesterContext(s_allocator_p),
        s_allocator_p);

    TestQueueHandle x(theQueue.d_queue_sp, theBench, clientContext_sp);
    TestQueueHandle y(theQueue.d_queue_sp, theBench, clientContext_sp);

    // 1. --------------------- test successful delivery ----------------------
    x.d_status = bmqt::AckResult::e_SUCCESS;
    y.d_status = bmqt::AckResult::e_SUCCESS;

    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);

    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);

    // everything is ACK'ed with e_SUCCESS
    theBench.ackPuts(bmqt::AckResult::e_SUCCESS);
    ASSERT_EQ(0U, x.count());
    ASSERT_EQ(0U, y.count());

    // 2. --------------------- test e_NOT_READY ------------------------------
    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);
    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);

    // Everything is pending
    theBench.ackPuts(bmqt::AckResult::e_NOT_READY);
    // RemoteQueue should terminate 'e_NOT_READY'
    ASSERT_EQ(2U, x.count());
    ASSERT_EQ(3U, y.count());

    // 3. --------------------- test retransmission ---------------------------
    // 'd_pendingMessages' is still full
    // simulate queue reopening
    theQueue.d_remoteQueue.onLostUpstream();
    theQueue.d_remoteQueue.onOpenUpstream(
        2,
        bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);

    // everything is ACK'ed with e_SUCCESS
    theBench.ackPuts(bmqt::AckResult::e_SUCCESS);
    ASSERT_EQ(0U, x.count());
    ASSERT_EQ(0U, y.count());

    // 4. --------------------- test expiration -------------------------------
    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);
    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);

    // everything is still pending
    ASSERT_EQ(2U, x.count());
    ASSERT_EQ(3U, y.count());

    x.d_status = bmqt::AckResult::e_UNKNOWN;
    y.d_status = bmqt::AckResult::e_UNKNOWN;

    theBench.advanceTime(bsls::TimeInterval(timeout + 1, 0));

    // everything non broadcast is ACK'ed with e_UNKNOWN
    // All broadcast PUTs are still pending.
    ASSERT_EQ(0U, x.count());
    ASSERT_EQ(0U, y.count());

    // 5. --------------------- upstream not responding -----------------------
    // All broadcast PUTs are still pending.
    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);
    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);

    theBench.dropPuts();

    // simulate queue reopening
    theQueue.d_remoteQueue.onLostUpstream();
    theQueue.d_remoteQueue.onOpenUpstream(
        1,
        bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);

    x.d_status = bmqt::AckResult::e_SUCCESS;
    y.d_status = bmqt::AckResult::e_SUCCESS;

    theBench.ackPuts(bmqt::AckResult::e_SUCCESS);

    ASSERT_EQ(0U, x.count());
    ASSERT_EQ(0U, y.count());

    // run timer
    theBench.advanceTime(bsls::TimeInterval(1, 0));
}

static void test2_broadcastBasic()
// ------------------------------------------------------------------------
// basic tests using broadcast
//
// Concerns:
//   Proper behavior of broadcast PUTs (re)transmission.
//
//   1. Post 'ackWindowSize + 1' messages. e_SUCCESS ack the last one.
//      Queue should be empty of all pending PUTs.
//   2. Post 'ackWindowSize + 1' messages. e_NOT_READY ack the last one.
//      Queue should keep all pending PUTs.
//   3. Simulate queue reopening.
//      RemoteQueue retransmits pending messages.
//      Ack messages with e_SUCCESS.
//      Queue should be empty of all pending PUTs.
//   4. Post 'ackWindowSize + 1' messages. Simulate timeout.
//      Queue should keep all pending PUTs.
//   5. Clear upstream of pending messages.
//      Simulate queue reopen.
//      RemoteQueue retransmits pending messages.
//      Ack messages with e_SUCCESS.
//      Queue should be empty of all pending PUTs.
//   6. Force close the queue.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("basic tests using broadcast");

    bmqt::UriParser::initialize(s_allocator_p);

    bsl::shared_ptr<bmqst::StatContext> statContext =
        mqbstat::BrokerStatsUtil::initializeStatContext(30, s_allocator_p);

    TestBench theBench(s_allocator_p);

    bmqt::Uri uri("bmq://bmq.test.local/test_queue", s_allocator_p);
    bmqp_ctrlmsg::RoutingConfiguration routingConfig;

    bmqp::RoutingConfigurationUtils::setAtMostOnce(&routingConfig);

    size_t                     ackWindowSize = 1000;
    int                        timeout       = 10;
    TestBench::TestRemoteQueue theQueue(theBench,
                                        uri,
                                        timeout,
                                        ackWindowSize,
                                        routingConfig);

    bsl::shared_ptr<mqbi::QueueHandleRequesterContext> clientContext_sp(
        new (*s_allocator_p) mqbi::QueueHandleRequesterContext(s_allocator_p),
        s_allocator_p);

    theQueue.d_queue_sp->_setAtMostOnce(true);

    TestQueueHandle z(theQueue.d_queue_sp, theBench, clientContext_sp);

    size_t pendingBroadcastPuts;
    size_t pendingBroadcastPutsWithData;

    ++ackWindowSize;  // extra PUT will request an ACK

    // 1. --------------------- test successful delivery ----------------------
    z.d_status = bmqt::AckResult::e_SUCCESS;

    for (size_t i = 0; i < ackWindowSize; ++i) {
        z.postOneMessage(&theQueue.d_remoteQueue);
    }

    // everything but broadcast is ACK'ed with e_SUCCESS
    // One (Nth) broadcast is ACK'ed resulting in removal of all previously
    // broadcasted PUTs.
    theBench.ackPuts(bmqt::AckResult::e_SUCCESS);
    ASSERT_EQ(0U, z.count());

    pendingBroadcastPutsWithData = 0;
    pendingBroadcastPuts = theQueue.d_remoteQueue.iteratePendingMessages(
        bdlf::BindUtil::bind(&verifyBroadfcastPut,
                             bdlf::PlaceHolders::_1,  // putHeader
                             bdlf::PlaceHolders::_2,  // appData
                             bdlf::PlaceHolders::_3,  // options
                             bdlf::PlaceHolders::_4,
                             &z,  // source
                             &pendingBroadcastPutsWithData));

    ASSERT_EQ(0U, pendingBroadcastPuts);
    ASSERT_EQ(0U, pendingBroadcastPutsWithData);

    // 2. --------------------- test e_NOT_READY ------------------------------
    for (size_t i = 0; i < ackWindowSize; ++i) {
        z.postOneMessage(&theQueue.d_remoteQueue);
    }

    // Everything is pending including all N broadcast PUTs
    theBench.ackPuts(bmqt::AckResult::e_NOT_READY);
    // RemoteQueue should terminate 'e_NOT_READY'
    ASSERT_EQ(0U, z.count());

    pendingBroadcastPutsWithData = 0;
    pendingBroadcastPuts = theQueue.d_remoteQueue.iteratePendingMessages(
        bdlf::BindUtil::bind(&verifyBroadfcastPut,
                             bdlf::PlaceHolders::_1,  // putHeader
                             bdlf::PlaceHolders::_2,  // appData
                             bdlf::PlaceHolders::_3,  // options
                             bdlf::PlaceHolders::_4,
                             &z,  // source
                             &pendingBroadcastPutsWithData));

    ASSERT_EQ(ackWindowSize, pendingBroadcastPuts);
    ASSERT_EQ(ackWindowSize, pendingBroadcastPutsWithData);

    // 3. --------------------- test retransmission ---------------------------
    // 'd_pendingMessages' is still full
    // simulate queue reopening
    theQueue.d_remoteQueue.onLostUpstream();
    theQueue.d_remoteQueue.onOpenUpstream(
        2,
        bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);
    // All broadcast PUTs are retransmitted and only GUIDs are being kept

    pendingBroadcastPutsWithData = 0;
    pendingBroadcastPuts = theQueue.d_remoteQueue.iteratePendingMessages(
        bdlf::BindUtil::bind(&verifyBroadfcastPut,
                             bdlf::PlaceHolders::_1,  // putHeader
                             bdlf::PlaceHolders::_2,  // appData
                             bdlf::PlaceHolders::_3,  // options
                             bdlf::PlaceHolders::_4,
                             &z,  // source
                             &pendingBroadcastPutsWithData));

    ASSERT_EQ(ackWindowSize, pendingBroadcastPuts);
    ASSERT_EQ(0U, pendingBroadcastPutsWithData);

    // everything but broadcast is ACK'ed with e_SUCCESS
    // One (N + 1) broadcast is ACK'ed resulting in removal of all previously
    // broadcasted PUTs.
    theBench.ackPuts(bmqt::AckResult::e_SUCCESS);

    ASSERT_EQ(0U, z.count());

    pendingBroadcastPutsWithData = 0;
    pendingBroadcastPuts = theQueue.d_remoteQueue.iteratePendingMessages(
        bdlf::BindUtil::bind(&verifyBroadfcastPut,
                             bdlf::PlaceHolders::_1,  // putHeader
                             bdlf::PlaceHolders::_2,  // appData
                             bdlf::PlaceHolders::_3,  // options
                             bdlf::PlaceHolders::_4,
                             &z,  // source
                             &pendingBroadcastPutsWithData));

    ASSERT_EQ(0U, pendingBroadcastPuts);
    ASSERT_EQ(0U, pendingBroadcastPutsWithData);

    // 4. --------------------- test expiration -------------------------------
    for (size_t i = 0; i < ackWindowSize; ++i) {
        z.postOneMessage(&theQueue.d_remoteQueue);
    }

    // everything is still pending
    ASSERT_EQ(0U, z.count());

    pendingBroadcastPutsWithData = 0;
    pendingBroadcastPuts = theQueue.d_remoteQueue.iteratePendingMessages(
        bdlf::BindUtil::bind(&verifyBroadfcastPut,
                             bdlf::PlaceHolders::_1,  // putHeader
                             bdlf::PlaceHolders::_2,  // appData
                             bdlf::PlaceHolders::_3,  // options
                             bdlf::PlaceHolders::_4,
                             &z,  // source
                             &pendingBroadcastPutsWithData));

    ASSERT_EQ(ackWindowSize, pendingBroadcastPuts);
    ASSERT_EQ(0U, pendingBroadcastPutsWithData);

    theBench.advanceTime(bsls::TimeInterval(timeout + 1, 0));

    // All broadcast PUTs are still pending.
    ASSERT_EQ(0U, z.count());

    pendingBroadcastPutsWithData = 0;
    pendingBroadcastPuts = theQueue.d_remoteQueue.iteratePendingMessages(
        bdlf::BindUtil::bind(&verifyBroadfcastPut,
                             bdlf::PlaceHolders::_1,  // putHeader
                             bdlf::PlaceHolders::_2,  // appData
                             bdlf::PlaceHolders::_3,  // options
                             bdlf::PlaceHolders::_4,
                             &z,  // source
                             &pendingBroadcastPutsWithData));

    ASSERT_EQ(ackWindowSize, pendingBroadcastPuts);
    ASSERT_EQ(0U, pendingBroadcastPutsWithData);

    // 5. --------------------- upstream not responding -----------------------
    // All broadcast PUTs are still pending.
    for (size_t i = 0; i < ackWindowSize; ++i) {
        z.postOneMessage(&theQueue.d_remoteQueue);
    }

    theBench.dropPuts();

    // simulate queue reopening
    theQueue.d_remoteQueue.onLostUpstream();
    theQueue.d_remoteQueue.onOpenUpstream(
        3,
        bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);

    theBench.ackPuts(bmqt::AckResult::e_SUCCESS);

    ASSERT_EQ(0U, z.count());

    // broadcast queue should NOT retransmit, so no ACKs for broadcasted PUTs,
    // so all broadcasted PUTs are still pending.
    pendingBroadcastPutsWithData = 0;
    pendingBroadcastPuts = theQueue.d_remoteQueue.iteratePendingMessages(
        bdlf::BindUtil::bind(&verifyBroadfcastPut,
                             bdlf::PlaceHolders::_1,  // putHeader
                             bdlf::PlaceHolders::_2,  // appData
                             bdlf::PlaceHolders::_3,  // options
                             bdlf::PlaceHolders::_4,
                             &z,  // source
                             &pendingBroadcastPutsWithData));

    ASSERT_EQ(0U, pendingBroadcastPuts);
    ASSERT_EQ(0U, pendingBroadcastPutsWithData);

    // 6. ---------------- posting when there is no upstream ------------------
    theQueue.d_remoteQueue.onLostUpstream();

    for (size_t i = 0; i < ackWindowSize; ++i) {
        z.postOneMessage(&theQueue.d_remoteQueue);
    }

    pendingBroadcastPutsWithData = 0;
    pendingBroadcastPuts = theQueue.d_remoteQueue.iteratePendingMessages(
        bdlf::BindUtil::bind(&verifyBroadfcastPut,
                             bdlf::PlaceHolders::_1,  // putHeader
                             bdlf::PlaceHolders::_2,  // appData
                             bdlf::PlaceHolders::_3,  // options
                             bdlf::PlaceHolders::_4,
                             &z,  // source
                             &pendingBroadcastPutsWithData));

    ASSERT_EQ(ackWindowSize, pendingBroadcastPuts);
    ASSERT_EQ(ackWindowSize, pendingBroadcastPutsWithData);

    // simulate queue reopening
    theQueue.d_remoteQueue.onOpenUpstream(
        2,
        bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);

    theBench.ackPuts(bmqt::AckResult::e_SUCCESS);
    ASSERT_EQ(0U, z.count());

    pendingBroadcastPutsWithData = 0;
    pendingBroadcastPuts = theQueue.d_remoteQueue.iteratePendingMessages(
        bdlf::BindUtil::bind(&verifyBroadfcastPut,
                             bdlf::PlaceHolders::_1,  // putHeader
                             bdlf::PlaceHolders::_2,  // appData
                             bdlf::PlaceHolders::_3,  // options
                             bdlf::PlaceHolders::_4,
                             &z,  // source
                             &pendingBroadcastPutsWithData));

    ASSERT_EQ(0U, pendingBroadcastPuts);
    ASSERT_EQ(0U, pendingBroadcastPutsWithData);

    // 7. ---------------- posting when there is no upstream ------------------
    theQueue.d_remoteQueue.onLostUpstream();

    for (size_t i = 0; i < ackWindowSize; ++i) {
        z.postOneMessage(&theQueue.d_remoteQueue);
    }

    // simulate reopen failure
    theQueue.d_remoteQueue.onOpenFailure(bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);

    ASSERT_EQ(0U, z.count());

    pendingBroadcastPutsWithData = 0;
    pendingBroadcastPuts = theQueue.d_remoteQueue.iteratePendingMessages(
        bdlf::BindUtil::bind(&verifyBroadfcastPut,
                             bdlf::PlaceHolders::_1,  // putHeader
                             bdlf::PlaceHolders::_2,  // appData
                             bdlf::PlaceHolders::_3,  // options
                             bdlf::PlaceHolders::_4,
                             &z,  // source
                             &pendingBroadcastPutsWithData));

    ASSERT_EQ(0U, pendingBroadcastPuts);
    ASSERT_EQ(0U, pendingBroadcastPutsWithData);

    theBench.dropPuts();

    for (size_t i = 0; i < ackWindowSize; ++i) {
        z.postOneMessage(&theQueue.d_remoteQueue);
    }

    ASSERT_EQ(0U, theBench.d_puts.size());
    ASSERT_EQ(0U, z.count());

    theQueue.d_remoteQueue.close();
}

static void test3_close()
// ------------------------------------------------------------------------
// close queue with pending messages
//
// Concerns:
//   Proper behavior of PUTs (re)transmission.
//
// Plan:
//   1. Post messages. Force close the queue
//      Producers should not receive any NACKs.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("close queue with pending messages");

    bmqt::UriParser::initialize(s_allocator_p);

    bsl::shared_ptr<bmqst::StatContext> statContext =
        mqbstat::BrokerStatsUtil::initializeStatContext(30, s_allocator_p);

    TestBench theBench(s_allocator_p);

    bmqt::Uri uri("bmq://bmq.test.local/test_queue", s_allocator_p);
    bmqp_ctrlmsg::RoutingConfiguration routingConfig;
    size_t                             ackWindowSize = 1000;
    int                                timeout       = 10;
    TestBench::TestRemoteQueue         theQueue(theBench,
                                        uri,
                                        timeout,
                                        ackWindowSize,
                                        routingConfig);

    bsl::shared_ptr<mqbi::QueueHandleRequesterContext> clientContext_sp(
        new (*s_allocator_p) mqbi::QueueHandleRequesterContext(s_allocator_p),
        s_allocator_p);

    TestQueueHandle x(theQueue.d_queue_sp, theBench, clientContext_sp);
    TestQueueHandle y(theQueue.d_queue_sp, theBench, clientContext_sp);

    // 1. --------------------- test successful delivery ----------------------

    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);

    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);

    theQueue.d_remoteQueue.close();
    ASSERT_EQ(2U, x.count());
    ASSERT_EQ(3U, y.count());
}

static void test4_buffering()
// ------------------------------------------------------------------------
// nackPendingPosts
//
// Concerns:
//   Proper behavior of the 'onUnavailableUpstreamDispatched' method.
//
// Plan:
//   Post un-acknowledged message(s) and then call
//   'onUnavailableUpstreamDispatched' method.
//
// Testing:
//   onUnavailableUpstreamDispatched
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("buffering");

    bmqt::UriParser::initialize(s_allocator_p);

    bsl::shared_ptr<bmqst::StatContext> statContext =
        mqbstat::BrokerStatsUtil::initializeStatContext(30, s_allocator_p);

    TestBench theBench(s_allocator_p);

    bmqt::Uri uri("bmq://bmq.test.local/test_queue", s_allocator_p);
    bmqp_ctrlmsg::RoutingConfiguration routingConfig;
    size_t                             ackWindowSize = 1000;
    int                                timeout       = 10;
    TestBench::TestRemoteQueue         theQueue(theBench,
                                        uri,
                                        timeout,
                                        ackWindowSize,
                                        routingConfig);

    bsl::shared_ptr<mqbi::QueueHandleRequesterContext> clientContext_sp(
        new (*s_allocator_p) mqbi::QueueHandleRequesterContext(s_allocator_p),
        s_allocator_p);

    TestQueueHandle x(theQueue.d_queue_sp, theBench, clientContext_sp);
    TestQueueHandle y(theQueue.d_queue_sp, theBench, clientContext_sp);

    // ------------------------ successful delivery ---------------------------
    x.d_status = bmqt::AckResult::e_SUCCESS;
    y.d_status = bmqt::AckResult::e_SUCCESS;

    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);

    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);

    ASSERT_EQ(5U, theBench.d_puts.size());

    // Start buffering.
    theQueue.d_remoteQueue.onLostUpstream();

    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);

    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);

    ASSERT_EQ(5U, theBench.d_puts.size());

    // ACK with e_SUCCESS
    theBench.ackPuts(bmqt::AckResult::e_SUCCESS);
    ASSERT_EQ(2U, x.count());
    ASSERT_EQ(3U, y.count());

    ASSERT_EQ(0U, theBench.d_puts.size());

    // Reopen the queue.
    theQueue.d_remoteQueue.onOpenUpstream(
        2,
        bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);

    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);

    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);

    ASSERT_EQ(10U, theBench.d_puts.size());

    // everything is ACK'ed with e_SUCCESS
    theBench.ackPuts(bmqt::AckResult::e_SUCCESS);
    ASSERT_EQ(0U, x.count());
    ASSERT_EQ(0U, y.count());

    theBench.advanceTime(bsls::TimeInterval(timeout + 1, 0));
    bslmt::Semaphore sem;

    theBench.d_cluster.waitForScheduler();
}

static void test5_reopen_failure()
// ------------------------------------------------------------------------
// onReopenFailure
//
// Concerns:
//   Proper behavior of the 'onReopenFailure' method.
//
// Plan:
//   Post un-acknowledged message(s) and then call 'onReopenFailure'.
//
// Testing:
//   onReopenFailure
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("buffering");

    bmqt::UriParser::initialize(s_allocator_p);

    bsl::shared_ptr<bmqst::StatContext> statContext =
        mqbstat::BrokerStatsUtil::initializeStatContext(30, s_allocator_p);

    TestBench theBench(s_allocator_p);

    bmqt::Uri uri("bmq://bmq.test.local/test_queue", s_allocator_p);
    bmqp_ctrlmsg::RoutingConfiguration routingConfig;
    size_t                             ackWindowSize = 1000;
    int                                timeout       = 10;
    TestBench::TestRemoteQueue         theQueue(theBench,
                                        uri,
                                        timeout,
                                        ackWindowSize,
                                        routingConfig);

    bsl::shared_ptr<mqbi::QueueHandleRequesterContext> clientContext_sp(
        new (*s_allocator_p) mqbi::QueueHandleRequesterContext(s_allocator_p),
        s_allocator_p);

    TestQueueHandle x(theQueue.d_queue_sp, theBench, clientContext_sp);
    TestQueueHandle y(theQueue.d_queue_sp, theBench, clientContext_sp);

    // ------------------------ successful delivery ---------------------------
    x.d_status = bmqt::AckResult::e_SUCCESS;
    y.d_status = bmqt::AckResult::e_SUCCESS;

    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);

    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);

    ASSERT_EQ(5U, theBench.d_puts.size());

    // simulate upstream drop
    theQueue.d_remoteQueue.onLostUpstream();

    ASSERT_EQ(2U, x.count());
    ASSERT_EQ(3U, y.count());

    // expecting NACKs
    x.d_status = bmqt::AckResult::e_UNKNOWN;
    y.d_status = bmqt::AckResult::e_UNKNOWN;

    // simulate reopen failure
    theQueue.d_remoteQueue.onOpenFailure(bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);

    ASSERT_EQ(0U, x.count());
    ASSERT_EQ(0U, y.count());

    theBench.dropPuts();

    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);

    x.postOneMessage(&theQueue.d_remoteQueue);
    y.postOneMessage(&theQueue.d_remoteQueue);

    ASSERT_EQ(0U, theBench.d_puts.size());
    ASSERT_EQ(0U, x.count());
    ASSERT_EQ(0U, y.count());

    theQueue.d_remoteQueue.close();
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_fanoutBasic(); break;
    case 2: test2_broadcastBasic(); break;
    case 3: test3_close(); break;
    case 4: test4_buffering(); break;
    case 5: test5_reopen_failure(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
