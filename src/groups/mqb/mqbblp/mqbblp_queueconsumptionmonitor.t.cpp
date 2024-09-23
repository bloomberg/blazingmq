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

// mqbblp_queueconsumptionmonitor.t.cpp                               -*-C++-*-
#include <mqbblp_queueconsumptionmonitor.h>

// MBQ
#include <mqbblp_queuehandlecatalog.h>
#include <mqbblp_queuestate.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbconfm_messages.h>
#include <mqbi_queue.h>
#include <mqbi_queueengine.h>
#include <mqbmock_cluster.h>
#include <mqbmock_dispatcher.h>
#include <mqbmock_domain.h>
#include <mqbmock_queue.h>
#include <mqbs_inmemorystorage.h>
#include <mqbstat_brokerstats.h>
#include <mqbu_messageguidutil.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_queueutil.h>
#include <bmqt_messageguid.h>
#include <bmqt_queueflags.h>

// MWC
#include <mwctsk_alarmlog.h>
#include <mwctst_scopedlogobserver.h>
#include <mwctst_testhelper.h>
#include <mwcu_memoutstream.h>

// BDE
#include <ball_loggermanager.h>
#include <ball_record.h>
#include <ball_severity.h>
#include <bdlb_string.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlt_timeunitratio.h>
#include <bsl_memory.h>

using namespace BloombergLP;
using namespace bsl;
using namespace mqbblp;

// ============================================================================
//                                  UTILITIES
// ----------------------------------------------------------------------------

static mqbconfm::Domain getDomainConfig()
{
    mqbconfm::Domain domainCfg;
    domainCfg.deduplicationTimeMs() = 0;  // No history
    domainCfg.messageTtl() = bsl::numeric_limits<bsls::Types::Int64>::max();
    return domainCfg;
}

struct ClientContext {
    // PUBLIC DATA
    mqbmock::DispatcherClient d_dispatcherClient;
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>
        d_requesterContext_sp;

    // CREATORS
    ClientContext();
    virtual ~ClientContext();
};

ClientContext::ClientContext()
: d_dispatcherClient(s_allocator_p)
, d_requesterContext_sp(new (*s_allocator_p)
                            mqbi::QueueHandleRequesterContext(s_allocator_p),
                        s_allocator_p)
{
    d_requesterContext_sp->setClient(&d_dispatcherClient);
}

ClientContext::~ClientContext()
{
    // NOTHING
}

struct Test : mwctst::Test {
    typedef bsl::vector<
        bsl::pair<mqbi::QueueHandle*, bmqp_ctrlmsg::QueueHandleParameters> >
        TestQueueHandleSeq;

    // PUBLIC DATA
    bmqt::Uri                                 d_uri;
    unsigned int                              d_id;
    mqbu::StorageKey                          d_storageKey;
    int                                       d_partitionId;
    mqbmock::Dispatcher                       d_dispatcher;
    bdlbb::PooledBlobBufferFactory            d_bufferFactory;
    mqbmock::Cluster                          d_cluster;
    mqbmock::Domain                           d_domain;
    mqbmock::Queue                            d_queue;
    QueueState                                d_queueState;
    QueueConsumptionMonitor                   d_monitor;
    mqbs::InMemoryStorage                     d_storage;
    bdlbb::Blob                               d_dataBlob, d_optionBlob;
    bsl::unordered_map<mqbu::StorageKey, int> d_advance;
    unsigned int                              d_clientId;
    ClientContext                             d_consumer1;
    ClientContext                             d_consumer2;
    ClientContext                             d_producer;
    TestQueueHandleSeq                        d_testQueueHandles;

    // CREATORS
    Test();
    ~Test() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    void               putMessage();
    mqbi::QueueHandle* createClient(
        const ClientContext&   clientContext,
        bmqt::QueueFlags::Enum role,
        const bsl::string&     appId = bmqp::ProtocolUtil::k_DEFAULT_APP_ID,
        unsigned int subQueueId      = bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);

    // bslma::ManagedPtr<mqbi::StorageIterator> head(const mqbu::StorageKey& key);
    void advance(const mqbu::StorageKey& key);
    bool loggingCb(const mqbu::StorageKey& appKey, const bool isAlarm);

};

Test::Test()
: d_uri("bmq://bmq.test.local/test_queue")
, d_id(802701)
, d_storageKey(mqbu::StorageKey::k_NULL_KEY)
, d_partitionId(1)
, d_dispatcher(s_allocator_p)
, d_bufferFactory(1024, s_allocator_p)
, d_cluster(&d_bufferFactory, s_allocator_p)
, d_domain(&d_cluster, s_allocator_p)
, d_queue(&d_domain, s_allocator_p)
, d_queueState(&d_queue,
               d_uri,
               d_id,
               d_storageKey,
               d_partitionId,
               &d_domain,
               s_allocator_p)
, d_monitor(&d_queueState,
                       bdlf::BindUtil::bind(&Test::loggingCb,
                                            this,
                                            bdlf::PlaceHolders::_1,   // appKey
                                            bdlf::PlaceHolders::_2),  // isAlarm

  s_allocator_p)
, d_storage(d_queue.uri(),
            mqbu::StorageKey::k_NULL_KEY,
            mqbs::DataStore::k_INVALID_PARTITION_ID,
            getDomainConfig(),
            d_domain.capacityMeter(),
            bmqp::RdaInfo(),
            s_allocator_p)
, d_advance(s_allocator_p)
, d_clientId(0)
, d_consumer1()
, d_consumer2()
, d_producer()
, d_testQueueHandles(s_allocator_p)
{
    d_dispatcher._setInDispatcherThread(true);
    d_queue._setDispatcher(&d_dispatcher);

    mwcu::MemOutStream errorDescription(s_allocator_p);

    bslma::ManagedPtr<mqbi::Queue> queueMp(&d_queue,
                                           0,
                                           bslma::ManagedPtrUtil::noOpDeleter);
    d_domain.registerQueue(errorDescription, queueMp);

    mqbconfm::Storage config;
    mqbconfm::Limits  limits;

    config.makeInMemory();

    limits.messages() = bsl::numeric_limits<bsls::Types::Int64>::max();
    limits.bytes()    = bsl::numeric_limits<bsls::Types::Int64>::max();

    d_storage.configure(errorDescription,
                        config,
                        limits,
                        bsl::numeric_limits<bsls::Types::Int64>::max(),
                        0);

    bslma::ManagedPtr<mqbi::Storage> storageMp(
        &d_storage,
        0,
        bslma::ManagedPtrUtil::noOpDeleter);
    d_queueState.setStorage(storageMp);

    d_domain.queueStatContext()->snapshot();

    d_consumer1.d_dispatcherClient._setDescription("test consumer 1");
    d_consumer2.d_dispatcherClient._setDescription("test consumer 2");
    d_producer.d_dispatcherClient._setDescription("test producer");
}

Test::~Test()
{
    for (TestQueueHandleSeq::reverse_iterator
             iter = d_testQueueHandles.rbegin(),
             last = d_testQueueHandles.rend();
         iter != last;
         ++iter) {
        bsl::shared_ptr<mqbi::QueueHandle> handleSp;
        bsls::Types::Uint64                lostFlags;
        d_queueState.handleCatalog().releaseHandleHelper(&handleSp,
                                                         &lostFlags,
                                                         iter->first,
                                                         iter->second,
                                                         true);
    }

    d_domain.unregisterQueue(&d_queue);
}

void Test::putMessage()
{
    bmqt::MessageGUID messageGUID;
    mqbu::MessageGUIDUtil::generateGUID(&messageGUID);

    mqbi::StorageMessageAttributes messageAttributes;
    bslma::ManagedPtr<bdlbb::Blob> appData(&d_dataBlob,
                                           0,
                                           bslma::ManagedPtrUtil::noOpDeleter);
    bslma::ManagedPtr<bdlbb::Blob> options(&d_dataBlob,
                                           0,
                                           bslma::ManagedPtrUtil::noOpDeleter);

    ASSERT_EQ(d_storage.put(&messageAttributes,
                            messageGUID,
                            appData,
                            options,
                            mqbi::Storage::StorageKeys()),
              mqbi::StorageResult::e_SUCCESS);
}

mqbi::QueueHandle* Test::createClient(const ClientContext&   clientContext,
                                      bmqt::QueueFlags::Enum role,
                                      const bsl::string&     appId,
                                      unsigned int           subQueueId)
{
    bmqp_ctrlmsg::QueueHandleParameters handleParams(s_allocator_p);
    handleParams.uri()        = d_uri.asString();
    handleParams.qId()        = ++d_clientId;
    handleParams.readCount()  = role == bmqt::QueueFlags::e_READ ? 1 : 0;
    handleParams.writeCount() = role == bmqt::QueueFlags::e_WRITE ? 1 : 0;
    handleParams.adminCount() = 0;
    handleParams.flags()      = role;

    mqbi::QueueHandle* queueHandle = d_queueState.handleCatalog().createHandle(
        clientContext.d_requesterContext_sp,
        handleParams,
        &d_queueState.stats());
    d_testQueueHandles.push_back(bsl::make_pair(queueHandle, handleParams));

    // Update the current handle parameters.
    d_queueState.add(handleParams);

    bmqp_ctrlmsg::SubQueueIdInfo subStreamInfo;
    subStreamInfo.appId() = appId;
    queueHandle->registerSubStream(subStreamInfo,
                                   subQueueId,
                                   mqbi::QueueCounts(1, 0));

    return queueHandle;
}

void Test::advance(const mqbu::StorageKey& key)
{
    ++d_advance[key];
}

bool Test::loggingCb(BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& appKey, BSLS_ANNOTATION_UNUSED const bool isAlarm)
{
    BALL_LOG_SET_CATEGORY("MQBBLP.QUEUECONSUMPTIONMONITORTEST");

    bslma::ManagedPtr<mqbi::StorageIterator> out;
    out = d_storage.getIterator(appKey);

    bool queueAtHead = out->atEnd();

    if (isAlarm && !queueAtHead) {
        mwcu::MemOutStream out(s_allocator_p);
        out << "Test Alarm";
        MWCTSK_ALARMLOG_ALARM("QUEUE_STUCK") << out.str() << MWCTSK_ALARMLOG_END;
    }

    return queueAtHead;
}


// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

TEST_F(Test, doNotMonitor)
// ------------------------------------------------------------------------
// Concerns:
//   No change is reported if maxIdleTime is not set to non-zero value
//
// Plan: Instantiate component, put message in queue, and simulate a
//   long inactivity period.
// ------------------------------------------------------------------------
{
    putMessage();

    mwctst::ScopedLogObserver observer(ball::Severity::INFO, s_allocator_p);

    d_monitor.registerSubStream(
        mqbu::StorageKey::k_NULL_KEY);

    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_ALIVE);

    d_monitor.onTimer(0);

    d_monitor.onTimer(1000000);

    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_ALIVE);

    ASSERT_EQ(observer.records().size(), 0U);
}

TEST_F(Test, emptyQueue)
// ------------------------------------------------------------------------
// Concerns:
//   An empty queue is considered active
//
// Plan: Start monitoring, make time pass, state should remain ALIVE.
// ------------------------------------------------------------------------
{
    mwctst::ScopedLogObserver logObserver(ball::Severity::INFO, s_allocator_p);
    size_t                    expectedLogRecords = 0U;

    const bsls::Types::Int64 k_MAX_IDLE_TIME = 10;

    d_monitor.setMaxIdleTime(k_MAX_IDLE_TIME);

    d_monitor.registerSubStream(
        mqbu::StorageKey::k_NULL_KEY);

    d_monitor.onTimer(k_MAX_IDLE_TIME);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords);

    d_monitor.onTimer(k_MAX_IDLE_TIME + 1);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords);
}

TEST_F(Test, logFormat)
// ------------------------------------------------------------------------
// Concerns: State becomes IDLE after set period then returns to normal
//   when message is processed - this is a typical, full scenario.
//
// Plan: Instantiate component, put message in queue, make time pass and
// check that state flips to IDLE according to specs, check logs, make more
// time pass and check that state remains 'idle', signal component that a
// message was consumed, check that state flips to 'alive', make more time
// pass, check that state remains 'alive'.
// ------------------------------------------------------------------------
{
    mwctst::ScopedLogObserver logObserver(ball::Severity::INFO, s_allocator_p);

    const bsls::Types::Int64 k_MAX_IDLE_TIME = 10;

    d_monitor.setMaxIdleTime(k_MAX_IDLE_TIME);

    d_monitor.registerSubStream(
        mqbu::StorageKey::k_NULL_KEY);

    putMessage();

    d_monitor.onTimer(k_MAX_IDLE_TIME);
    d_monitor.onTimer(2 * k_MAX_IDLE_TIME + 1);
    ASSERT_EQ(logObserver.records().size(), 1u);
    ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
        logObserver.records().back(),
        "ALARM \\[QUEUE_STUCK\\]",
        s_allocator_p));
}

TEST_F(Test, putAliveIdleSendAlive)
// ------------------------------------------------------------------------
// Concerns: State becomes IDLE after set period then returns to normal
//   when message is processed - this is a typical, full scenario.
//
// Plan: Instantiate component, put message in queue, make time pass and
// check that state flips to IDLE according to specs, check logs, make more
// time pass and check that state remains 'idle', signal component that a
// message was consumed, check that state flips to 'alive', make more time
// pass, check that state remains 'alive'.
// ------------------------------------------------------------------------
{
    mwctst::ScopedLogObserver logObserver(ball::Severity::INFO, s_allocator_p);
    size_t                    expectedLogRecords = 0U;

    const bsls::Types::Int64 k_MAX_IDLE_TIME = 10;

    d_monitor.setMaxIdleTime(k_MAX_IDLE_TIME);

    d_monitor.registerSubStream(
        mqbu::StorageKey::k_NULL_KEY);

    putMessage();

    d_monitor.onTimer(k_MAX_IDLE_TIME);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords);

    d_monitor.onTimer(2 * k_MAX_IDLE_TIME - 1);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords);

    d_monitor.onTimer(2 * k_MAX_IDLE_TIME);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords);

    d_monitor.onTimer(2 * k_MAX_IDLE_TIME + 1);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_IDLE);
    ASSERT_EQ(logObserver.records().size(), ++expectedLogRecords);
    ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
        logObserver.records().back(),
        "ALARM \\[QUEUE_STUCK\\]",
        s_allocator_p));

    d_monitor.onTimer(2 * k_MAX_IDLE_TIME + 2);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_IDLE);

    d_monitor.onMessageSent(mqbu::StorageKey::k_NULL_KEY);
    advance(mqbu::StorageKey::k_NULL_KEY);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_IDLE);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords);

    d_monitor.onTimer(3 * k_MAX_IDLE_TIME + 2);
    ASSERT_EQ(logObserver.records().size(), ++expectedLogRecords);
    ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
        logObserver.records().back(),
        "no longer appears to be stuck",
        s_allocator_p));
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_ALIVE);
}

TEST_F(Test, putAliveIdleWithConsumer)
{
    // ------------------------------------------------------------------------
    // Concerns: Same as above, but with two read and one write clients.
    //
    // Plan: Start monitoring, create three clients (2 read and 1 write), put
    // message in queue, create an 'idle' condition, check that the two read
    // clients (but not the write client) are reported in the log.
    // ------------------------------------------------------------------------
    mwctst::ScopedLogObserver logObserver(ball::Severity::INFO, s_allocator_p);
    size_t                    expectedLogRecords = 3U;

    const bsls::Types::Int64 k_MAX_IDLE_TIME = 10;
    d_monitor.setMaxIdleTime(k_MAX_IDLE_TIME);

    d_monitor.registerSubStream(
        mqbu::StorageKey::k_NULL_KEY);

    createClient(d_consumer1, bmqt::QueueFlags::e_READ);
    createClient(d_consumer2, bmqt::QueueFlags::e_READ);
    createClient(d_producer, bmqt::QueueFlags::e_WRITE);

    putMessage();

    d_monitor.onTimer(k_MAX_IDLE_TIME);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords);

    d_monitor.onTimer(2 * k_MAX_IDLE_TIME + 1);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_IDLE);
    ASSERT_EQ(logObserver.records().size(), ++expectedLogRecords);
    ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
        logObserver.records().back(),
        "ALARM \\[QUEUE_STUCK\\]",
        s_allocator_p));
}

TEST_F(Test, putAliveIdleEmptyAlive)
// ------------------------------------------------------------------------
// Concerns: Emptying the queue flips state back to 'alive'.
//
// Plan: Start monitoring, put message in queue, make time pass until state
// flips to 'idle', empty the queue, make time "pass" by zero ticks, state
// must return to 'alive'.
// ------------------------------------------------------------------------
{
    const bsls::Types::Int64 k_MAX_IDLE_TIME = 10;

    d_monitor.setMaxIdleTime(k_MAX_IDLE_TIME);

    d_monitor.registerSubStream(
        mqbu::StorageKey::k_NULL_KEY);
    putMessage();

    d_monitor.onTimer(k_MAX_IDLE_TIME);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_ALIVE);

    d_monitor.onTimer(2 * k_MAX_IDLE_TIME + 1);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_IDLE);

    d_storage.removeAll(d_storageKey);

    d_monitor.onTimer(2 * k_MAX_IDLE_TIME + 1);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_ALIVE);
}

TEST_F(Test, changeMaxIdleTime)
// ------------------------------------------------------------------------
// Concerns: setting max idle time to new value also resets monitoring.
//
// Plan: Instantiate component, put message in queue, make time pass and
// check that state flips to IDLE according to specs, change max idle time,
// check that state is back to 'alive' and things behave as if 'onTimer'
// had never been called.
// ------------------------------------------------------------------------
{
    const bsls::Types::Int64 k_MAX_IDLE_TIME = 10;

    d_monitor.setMaxIdleTime(k_MAX_IDLE_TIME);

    d_monitor.registerSubStream(
        mqbu::StorageKey::k_NULL_KEY);

    putMessage();

    d_monitor.onTimer(0);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_ALIVE);

    d_monitor.onTimer(k_MAX_IDLE_TIME + 1);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_IDLE);

    mwctst::ScopedLogObserver logObserver(ball::Severity::INFO, s_allocator_p);

    d_monitor.setMaxIdleTime(k_MAX_IDLE_TIME * 2);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(logObserver.records().size(), 0u);

    d_monitor.onTimer(k_MAX_IDLE_TIME * 2);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_ALIVE);

    d_monitor.onTimer(k_MAX_IDLE_TIME * 2 + k_MAX_IDLE_TIME * 2);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_ALIVE);

    d_monitor.onTimer(k_MAX_IDLE_TIME * 2 + k_MAX_IDLE_TIME * 2 + 1);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_IDLE);
}

TEST_F(Test, reset)
// ------------------------------------------------------------------------
// Concerns: 'reset' puts component in same state as just after
// construction.
//
// Plan: Instantiate component, put message in queue, call 'reset', make
// time pass beyond idle, check that nothing was logged.
// ------------------------------------------------------------------------
{
    const bsls::Types::Int64 k_MAX_IDLE_TIME = 10;

    d_monitor.setMaxIdleTime(k_MAX_IDLE_TIME);

    d_monitor.registerSubStream(
        mqbu::StorageKey::k_NULL_KEY);

    putMessage();

    d_monitor.onTimer(0);
    ASSERT_EQ(d_monitor.state(mqbu::StorageKey::k_NULL_KEY),
              QueueConsumptionMonitor::State::e_ALIVE);

    mwctst::ScopedLogObserver logObserver(ball::Severity::INFO, s_allocator_p);

    d_monitor.reset();
    d_monitor.onTimer(k_MAX_IDLE_TIME + 1);
    ASSERT_EQ(logObserver.records().size(), 0u);
}

TEST_F(Test, putAliveIdleSendAliveTwoSubstreams)
// ------------------------------------------------------------------------
// Concerns: State becomes IDLE after set period then returns to normal
//   when message is processed - this is a typical, full scenario.
//
// Plan: Instantiate component, put message in queue, make time pass and
// check that state flips to IDLE according to specs, check logs, make more
// time pass and check that state remains 'idle', signal component that a
// message was consumed, check that state flips to 'alive', make more time
// pass, check that state remains 'alive'.
// ------------------------------------------------------------------------
{
    mwctst::ScopedLogObserver logObserver(ball::Severity::INFO, s_allocator_p);
    size_t                    expectedLogRecords = 0U;

    const bsls::Types::Int64 k_MAX_IDLE_TIME = 10;

    mqbu::StorageKey key1, key2;
    key1.fromHex("ABCDEF1234");
    key2.fromHex("1234ABCDEF");

    d_monitor.setMaxIdleTime(k_MAX_IDLE_TIME);

    mwcu::MemOutStream errorDescription(s_allocator_p);
    d_storage.addVirtualStorage(errorDescription, "app1", key1);
    d_storage.addVirtualStorage(errorDescription, "app2", key2);

    d_monitor.registerSubStream(key1);

    d_monitor.registerSubStream(key2);

    putMessage();

    d_monitor.onTimer(k_MAX_IDLE_TIME);
    ASSERT_EQ(d_monitor.state(key1), QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(d_monitor.state(key2), QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords);

    d_monitor.onTimer(2 * k_MAX_IDLE_TIME - 1);
    ASSERT_EQ(d_monitor.state(key1), QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(d_monitor.state(key2), QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords);

    d_monitor.onTimer(2 * k_MAX_IDLE_TIME);
    ASSERT_EQ(d_monitor.state(key1), QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(d_monitor.state(key2), QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords);

    d_monitor.onTimer(2 * k_MAX_IDLE_TIME + 1);
    ASSERT_EQ(d_monitor.state(key1), QueueConsumptionMonitor::State::e_IDLE);
    ASSERT_EQ(d_monitor.state(key2), QueueConsumptionMonitor::State::e_IDLE);

    ASSERT_EQ(logObserver.records().size(), expectedLogRecords += 2);

    for (int i = 0; i < 2; ++i) {
        ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
            logObserver.records().rbegin()[i],
            "ALARM \\[QUEUE_STUCK\\]",
            s_allocator_p));
    }

    d_monitor.onTimer(2 * k_MAX_IDLE_TIME + 2);
    ASSERT_EQ(d_monitor.state(key1), QueueConsumptionMonitor::State::e_IDLE);
    ASSERT_EQ(d_monitor.state(key2), QueueConsumptionMonitor::State::e_IDLE);

    d_monitor.onMessageSent(key1);
    advance(key1);
    ASSERT_EQ(d_monitor.state(key1), QueueConsumptionMonitor::State::e_IDLE);
    ASSERT_EQ(d_monitor.state(key2), QueueConsumptionMonitor::State::e_IDLE);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords);

    d_monitor.onTimer(3 * k_MAX_IDLE_TIME + 2);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords += 1);
    ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
        logObserver.records().back(),
        "Queue 'bmq://bmq.test.local/test_queue\\?id=app1' no longer appears "
        "to be stuck.",
        s_allocator_p));

    ASSERT_EQ(d_monitor.state(key1), QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(d_monitor.state(key2), QueueConsumptionMonitor::State::e_IDLE);

    d_monitor.onMessageSent(key2);
    advance(key2);
    d_monitor.onTimer(3 * k_MAX_IDLE_TIME + 3);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords += 1);
    ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
        logObserver.records().back(),
        "Queue 'bmq://bmq.test.local/test_queue\\?id=app2' no longer appears "
        "to be stuck.",
        s_allocator_p));
    ASSERT_EQ(d_monitor.state(key1), QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(d_monitor.state(key2), QueueConsumptionMonitor::State::e_ALIVE);
}

TEST_F(Test, putAliveIdleSendAliveTwoSubstreamsTwoConsumers)
// ------------------------------------------------------------------------
// Concerns: State becomes IDLE after set period then returns to normal
//   when message is processed - this is a typical, full scenario.
//
// Plan: Instantiate component, put message in queue, make time pass and
// check that state flips to IDLE according to specs, check logs, make more
// time pass and check that state remains 'idle', signal component that a
// message was consumed, check that state flips to 'alive', make more time
// pass, check that state remains 'alive'.
// ------------------------------------------------------------------------
{
    mwctst::ScopedLogObserver logObserver(ball::Severity::INFO, s_allocator_p);
    size_t                    expectedLogRecords = 3U;

    const bsls::Types::Int64 k_MAX_IDLE_TIME = 10;

    mqbu::StorageKey key1, key2;
    key1.fromHex("ABCDEF1234");
    key2.fromHex("1234ABCDEF");

    d_monitor.setMaxIdleTime(k_MAX_IDLE_TIME);

    mwcu::MemOutStream errorDescription(s_allocator_p);
    d_storage.addVirtualStorage(errorDescription, "app1", key1);
    d_storage.addVirtualStorage(errorDescription, "app2", key2);

    d_monitor.registerSubStream(key1);

    d_monitor.registerSubStream(key2);

    createClient(d_consumer1, bmqt::QueueFlags::e_READ, "app1");
    createClient(d_consumer2, bmqt::QueueFlags::e_READ, "app2");
    createClient(d_producer, bmqt::QueueFlags::e_WRITE);

    putMessage();

    d_monitor.onTimer(k_MAX_IDLE_TIME);
    ASSERT_EQ(d_monitor.state(key1), QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(d_monitor.state(key2), QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords);

    d_monitor.onTimer(2 * k_MAX_IDLE_TIME - 1);
    ASSERT_EQ(d_monitor.state(key1), QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(d_monitor.state(key2), QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords);

    d_monitor.onTimer(2 * k_MAX_IDLE_TIME);
    ASSERT_EQ(d_monitor.state(key1), QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(d_monitor.state(key2), QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords);

    d_monitor.onTimer(2 * k_MAX_IDLE_TIME + 1);
    ASSERT_EQ(d_monitor.state(key1), QueueConsumptionMonitor::State::e_IDLE);
    ASSERT_EQ(d_monitor.state(key2), QueueConsumptionMonitor::State::e_IDLE);

    ASSERT_EQ(logObserver.records().size(), expectedLogRecords += 2);

    for (bsl::vector<ball::Record>::const_iterator iter =
             logObserver.records().end() - 2;
         iter != logObserver.records().end();
         ++iter) {
        ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
            *iter,
            "ALARM \\[QUEUE_STUCK\\]",
            s_allocator_p));
    }

    d_monitor.onTimer(2 * k_MAX_IDLE_TIME + 2);
    ASSERT_EQ(d_monitor.state(key1), QueueConsumptionMonitor::State::e_IDLE);
    ASSERT_EQ(d_monitor.state(key2), QueueConsumptionMonitor::State::e_IDLE);

    d_monitor.onMessageSent(key1);
    advance(key1);
    ASSERT_EQ(d_monitor.state(key1), QueueConsumptionMonitor::State::e_IDLE);
    ASSERT_EQ(d_monitor.state(key2), QueueConsumptionMonitor::State::e_IDLE);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords);

    d_monitor.onTimer(3 * k_MAX_IDLE_TIME + 2);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords += 1);
    ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
        logObserver.records().back(),
        "Queue 'bmq://bmq.test.local/test_queue\\?id=app1' no longer appears "
        "to be stuck",
        s_allocator_p));
    ASSERT_EQ(d_monitor.state(key1), QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(d_monitor.state(key2), QueueConsumptionMonitor::State::e_IDLE);

    d_monitor.onMessageSent(key2);
    advance(key2);
    d_monitor.onTimer(3 * k_MAX_IDLE_TIME + 3);
    ASSERT_EQ(logObserver.records().size(), expectedLogRecords += 1);
    ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
        logObserver.records().back(),
        "Queue 'bmq://bmq.test.local/test_queue\\?id=app2' no longer appears "
        "to be stuck",
        s_allocator_p));
    ASSERT_EQ(d_monitor.state(key1), QueueConsumptionMonitor::State::e_ALIVE);
    ASSERT_EQ(d_monitor.state(key2), QueueConsumptionMonitor::State::e_ALIVE);
}

TEST_F(Test, usage)
// -------------------------------------------------------------------------
// Concerns: Make sure the usage example is correct.
//
// Plan: Following code can be edited into the usage example via a few
// simple replacements and deletions.
// -------------------------------------------------------------------------
{
#define monitor d_monitor

    mwctst::ScopedLogObserver logObserver(ball::Severity::INFO, s_allocator_p);

    monitor.setMaxIdleTime(20);

    d_monitor.registerSubStream(
        mqbu::StorageKey::k_NULL_KEY);

    putMessage();
    putMessage();

    bsls::Types::Int64 T = 0;
    // at time T
    monitor.onTimer(T);  // nothing is logged
    ASSERT_EQ(logObserver.records().size(), 0u);
    // 15 seconds later - T + 15s
    monitor.onTimer(T += 15);  // nothing is logged
    ASSERT_EQ(logObserver.records().size(), 0u);
    // 15 seconds later - T + 30s
    monitor.onTimer(T += 15);  // log ALARM
    ASSERT_EQ(logObserver.records().size(), 1u);
    // 15 seconds later - T + 45s
    monitor.onTimer(T += 15);  // nothing is logged
    ASSERT_EQ(logObserver.records().size(), 1u);
    // 15 seconds later - T + 60s - consume first message, inform monitor:
    monitor.onMessageSent(mqbu::StorageKey::k_NULL_KEY);
    advance(mqbu::StorageKey::k_NULL_KEY);

    // 15 seconds later - T + 75s
    monitor.onTimer(T += 15);  // log INFO: back to active
    ASSERT_EQ(logObserver.records().size(), 2u);
    // 15 seconds later - T + 90s
    monitor.onTimer(T += 15);  // nothing is logged
    ASSERT_EQ(logObserver.records().size(), 2u);
    // 15 seconds later - T + 105s
    monitor.onTimer(T += 15);  // log ALARM
    ASSERT_EQ(logObserver.records().size(), 3u);
    // 15 seconds later - T + 120s
    d_storage.removeAll(d_storageKey);

    monitor.onTimer(T += 15);  // log INFO: back to active
    ASSERT_EQ(logObserver.records().size(), 4u);

#undef monitor
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    BALL_LOG_SET_CATEGORY("MAIN");

    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    bmqt::UriParser::initialize(s_allocator_p);

    ball::LoggerManager::singleton().setDefaultThresholdLevels(
        ball::Severity::OFF,
        ball::Severity::INFO,
        ball::Severity::OFF,
        ball::Severity::OFF);
    {
        mqbcfg::AppConfig brokerConfig(s_allocator_p);
        mqbcfg::BrokerConfig::set(brokerConfig);

        bsl::shared_ptr<mwcst::StatContext> statContext =
            mqbstat::BrokerStatsUtil::initializeStatContext(30, s_allocator_p);

        mwctst::runTest(_testCase);
    }

    bmqt::UriParser::shutdown();

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_GBL_ALLOC);
}
