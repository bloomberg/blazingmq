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
#include <mqbblp_queuestate.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbi_queue.h>
#include <mqbmock_cluster.h>
#include <mqbmock_dispatcher.h>
#include <mqbmock_domain.h>
#include <mqbmock_queue.h>
#include <mqbs_inmemorystorage.h>
#include <mqbstat_brokerstats.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_queueutil.h>
#include <bmqt_messageguid.h>
#include <bmqt_queueflags.h>
#include <bmqtsk_alarmlog.h>
#include <bmqtst_scopedlogobserver.h>
#include <bmqtst_testhelper.h>
#include <bmqu_memoutstream.h>

// BDE
#include <ball_loggermanager.h>
#include <ball_record.h>
#include <ball_severity.h>
#include <bdlb_string.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlt_timeunitratio.h>
#include <bsl_memory.h>
#include <bsl_set.h>

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

class QueueConsumptionMonitorTest : public QueueConsumptionMonitor {
    // This class is a test driver for the 'QueueConsumptionMonitor' class.
    // It is used to get access to the protected members of the class for
    // testing purposes.
  public:
    // CREATORS
    QueueConsumptionMonitorTest(QueueState*              queueState,
                                const HaveUndeliveredCb& haveUndeliveredCb,
                                const LoggingCb&         loggingCb,
                                bslma::Allocator*        allocator)
    : QueueConsumptionMonitor(queueState,
                              haveUndeliveredCb,
                              loggingCb,
                              allocator)
    {
        // NOTHING
    }

    ~QueueConsumptionMonitorTest() BSLS_KEYWORD_OVERRIDE { reset(); }

    // MODIFIERS

    void alarmEventDispatched() BSLS_KEYWORD_OVERRIDE
    {
        QueueConsumptionMonitor::alarmEventDispatched();
    }

    void idleEventDispatched(const bsl::string& appId) BSLS_KEYWORD_OVERRIDE
    {
        QueueConsumptionMonitor::idleEventDispatched(appId);
    }
};

struct MockStorageIterator : public mqbi::StorageIterator {
    // PUBLIC DATA
    bmqt::MessageGUID d_guid;

    mqbi::AppMessage d_appMessage;

    bsl::shared_ptr<bdlbb::Blob> d_appData;

    bsl::shared_ptr<bdlbb::Blob> d_options;

    mqbi::StorageMessageAttributes d_messageAttributes;

    // CREATORS
    MockStorageIterator()
    : d_guid()
    , d_appMessage(bmqp::RdaInfo())
    , d_appData()
    , d_options()
    , d_messageAttributes()
    {
    }

    // MANIPULATORS
    void clearCache() BSLS_KEYWORD_OVERRIDE
    {
        d_appData.reset();
        d_options.reset();
        d_messageAttributes.reset();
    }

    bool advance() BSLS_KEYWORD_OVERRIDE { return true; }

    // ACCESSORS
    void
    reset(const BSLA_UNUSED bmqt::MessageGUID& where) BSLS_KEYWORD_OVERRIDE
    {
    }

    const bmqt::MessageGUID& guid() const BSLS_KEYWORD_OVERRIDE
    {
        return d_guid;
    }

    const mqbi::AppMessage& appMessageView(
        BSLA_UNUSED unsigned int appOrdinal) const BSLS_KEYWORD_OVERRIDE
    {
        return d_appMessage;
    }

    mqbi::AppMessage&
    appMessageState(BSLA_UNUSED unsigned int appOrdinal) BSLS_KEYWORD_OVERRIDE
    {
        return d_appMessage;
    }

    const bsl::shared_ptr<bdlbb::Blob>& appData() const BSLS_KEYWORD_OVERRIDE
    {
        return d_appData;
    }

    const bsl::shared_ptr<bdlbb::Blob>& options() const BSLS_KEYWORD_OVERRIDE
    {
        return d_options;
    }

    const mqbi::StorageMessageAttributes&
    attributes() const BSLS_KEYWORD_OVERRIDE
    {
        return d_messageAttributes;
    }

    bool atEnd() const BSLS_KEYWORD_OVERRIDE { return false; }

    bool hasReceipt() const BSLS_KEYWORD_OVERRIDE { return true; }
};

struct Test : bmqtst::Test {
    // PUBLIC DATA
    bsl::string                    d_id;
    mqbmock::Dispatcher            d_dispatcher;
    bdlbb::PooledBlobBufferFactory d_bufferFactory;
    mqbmock::Cluster               d_cluster;
    mqbmock::Domain                d_domain;
    mqbmock::Queue                 d_queue;
    QueueState                     d_queueState;
    QueueConsumptionMonitorTest    d_monitor;
    mqbs::InMemoryStorage          d_storage;
    bsl::set<bsl::string>          d_haveUndelivered;

    // CREATORS
    Test();
    ~Test() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    void putMessage(const bsl::string& appId = bsl::string());

    // ACCESSORS
    bslma::ManagedPtr<mqbi::StorageIterator>
    haveUndeliveredCb(bsls::TimeInterval*       alarmTime_p,
                      const bsl::string&        appId,
                      const bsls::TimeInterval& now) const;

    void loggingCb(
        const bsl::string&                              appId,
        const bslma::ManagedPtr<mqbi::StorageIterator>& oldestMsgIt) const;
};

Test::Test()
: d_id()
, d_dispatcher(bmqtst::TestHelperUtil::allocator())
, d_bufferFactory(1024, bmqtst::TestHelperUtil::allocator())
, d_cluster(&d_bufferFactory, bmqtst::TestHelperUtil::allocator())
, d_domain(&d_cluster, bmqtst::TestHelperUtil::allocator())
, d_queue(&d_domain, bmqtst::TestHelperUtil::allocator())
, d_queueState(&d_queue,
               bmqt::Uri("bmq://bmq.test.local/test_queue"),
               802701,
               mqbu::StorageKey::k_NULL_KEY,
               1,
               &d_domain,
               d_cluster._resources(),
               bmqtst::TestHelperUtil::allocator())
, d_monitor(&d_queueState,
            bdlf::BindUtil::bind(&Test::haveUndeliveredCb,
                                 this,
                                 bdlf::PlaceHolders::_1,   // alarmTime_p
                                 bdlf::PlaceHolders::_2,   // appId
                                 bdlf::PlaceHolders::_3),  // now
            bdlf::BindUtil::bind(&Test::loggingCb,
                                 this,
                                 bdlf::PlaceHolders::_1,   // appId
                                 bdlf::PlaceHolders::_2),  // oldestMsgIt

            bmqtst::TestHelperUtil::allocator())
, d_storage(d_queue.uri(),
            mqbu::StorageKey::k_NULL_KEY,
            mqbs::DataStore::k_INVALID_PARTITION_ID,
            getDomainConfig(),
            d_domain.capacityMeter(),
            bmqtst::TestHelperUtil::allocator())
, d_haveUndelivered(bmqtst::TestHelperUtil::allocator())
{
    d_dispatcher._setInDispatcherThread(true);
    d_queue._setDispatcher(&d_dispatcher);

    bmqu::MemOutStream errorDescription(bmqtst::TestHelperUtil::allocator());

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
}

Test::~Test()
{
    d_domain.unregisterQueue(&d_queue);
}

void Test::putMessage(const bsl::string& id)
{
    d_monitor.onMessagePosted();
    d_haveUndelivered.insert(id);
}

bslma::ManagedPtr<mqbi::StorageIterator>
Test::haveUndeliveredCb(bsls::TimeInterval*       alarmTime_p,
                        const bsl::string&        appId,
                        const bsls::TimeInterval& now) const
{
    bslma::ManagedPtr<mqbi::StorageIterator> oldestMsgIt;

    const bool haveUndelivered = d_haveUndelivered.contains(appId);
    if (haveUndelivered) {
        bslma::Allocator*    alloc = bslma::Default::defaultAllocator();
        MockStorageIterator* mockStorageIterator_p = new (*alloc)
            MockStorageIterator();
        oldestMsgIt.load(mockStorageIterator_p, alloc);

        if (alarmTime_p) {
            *alarmTime_p = now;
        }
    }

    return oldestMsgIt;
}

void Test::loggingCb(
    BSLA_UNUSED const bsl::string& id,
    BSLA_UNUSED const bslma::ManagedPtr<mqbi::StorageIterator>& oldestMsgIt)
    const
{
    BALL_LOG_SET_CATEGORY("MQBBLP.QUEUECONSUMPTIONMONITORTEST");

    BMQTSK_ALARMLOG_ALARM("QUEUE_STUCK")
        << "Test Alarm" << BMQTSK_ALARMLOG_END;
}

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

BMQTST_TEST_F(Test, calculateAlarmTime)
// ------------------------------------------------------------------------
// Concerns:
//   Check that the alarm time is calculated correctly.
//
// Plan: Instantiate component, calculate alarm time and check the result.
// ------------------------------------------------------------------------
{
    d_monitor.setMaxIdleTime(0);
    BMQTST_ASSERT_EQ(d_monitor.calculateAlarmTime(0, bsls::TimeInterval(1, 2)),
                     bsls::TimeInterval(1, 2));

    d_monitor.setMaxIdleTime(10);
    BMQTST_ASSERT_EQ(d_monitor.calculateAlarmTime(0, bsls::TimeInterval(1, 2)),
                     bsls::TimeInterval(11, 2));

    BMQTST_ASSERT_EQ(d_monitor.calculateAlarmTime(2000000000,
                                                  bsls::TimeInterval(1, 2)),
                     bsls::TimeInterval(9, 2));
}

BMQTST_TEST_F(Test, doNotMonitor)
// ------------------------------------------------------------------------
// Concerns:
//   No change is reported if maxIdleTime is not set to non-zero value
//
// Plan: Instantiate component, put message in queue, and check no alarm.
// ------------------------------------------------------------------------
{
    bmqtst::ScopedLogObserver observer(ball::Severity::e_INFO,
                                       bmqtst::TestHelperUtil::allocator());

    d_monitor.registerSubStream(d_id);

    putMessage(d_id);

    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_ALIVE);
    BMQTST_ASSERT(!d_monitor.isAlarmScheduled());

    BMQTST_ASSERT_EQ(observer.records().size(), 0U);
}

BMQTST_TEST_F(Test, emptyQueue)
// ------------------------------------------------------------------------
// Concerns:
//   An empty queue is considered active
//
// Plan: Start monitoring, make time pass, state should remain ALIVE.
// ------------------------------------------------------------------------
{
    bmqtst::ScopedLogObserver logObserver(ball::Severity::e_INFO,
                                          bmqtst::TestHelperUtil::allocator());
    size_t                    expectedLogRecords = 0U;

    d_monitor.setMaxIdleTime(1);

    d_monitor.registerSubStream(d_id);

    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_ALIVE);

    // Simulate alarm event dispatching
    d_monitor.alarmEventDispatched();

    BMQTST_ASSERT(!d_monitor.isAlarmScheduled());

    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_ALIVE);
    BMQTST_ASSERT_EQ(logObserver.records().size(), expectedLogRecords);
}

BMQTST_TEST_F(Test, putAliveIdleSendAlive)
// ------------------------------------------------------------------------
// Concerns: State becomes IDLE after set period then returns to normal
//   when message is processed - this is a typical, full scenario.
//
// Plan: Instantiate component, put message in queue, simulate time pass and
// check that state flips to IDLE according to specs, check logs,
// signal component that a message was consumed, check that state flips to
// 'alive'.
// ------------------------------------------------------------------------
{
    bmqtst::ScopedLogObserver logObserver(ball::Severity::e_INFO,
                                          bmqtst::TestHelperUtil::allocator());
    size_t                    expectedLogRecords = 0U;

    d_monitor.setMaxIdleTime(1);

    d_monitor.registerSubStream(d_id);

    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_ALIVE);

    putMessage(d_id);

    BMQTST_ASSERT(d_monitor.isAlarmScheduled());

    // Simulate alarm event dispatching after max idle time
    d_monitor.alarmEventDispatched();

    BMQTST_ASSERT(!d_monitor.isAlarmScheduled());

    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_IDLE);
    BMQTST_ASSERT_EQ(logObserver.records().size(), ++expectedLogRecords);
    BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
        logObserver.records().back(),
        "ALARM \\[QUEUE_STUCK\\]",
        bmqtst::TestHelperUtil::allocator()));

    // Consume message
    d_haveUndelivered.erase(d_id);
    d_monitor.onMessageSent(d_id);

    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_ALIVE);
    BMQTST_ASSERT_EQ(logObserver.records().size(), ++expectedLogRecords);
    BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
        logObserver.records().back(),
        "no longer appears to be stuck",
        bmqtst::TestHelperUtil::allocator()));
}

BMQTST_TEST_F(Test, putAliveIdleEmptyAlive)
// ------------------------------------------------------------------------
// Concerns: Emptying the queue flips state back to 'alive'.
//
// Plan: Start monitoring, put message in queue, make time pass until state
// flips to 'idle', empty the queue, make time "pass" by zero ticks, state
// must return to 'alive'.
// ------------------------------------------------------------------------
{
    d_monitor.setMaxIdleTime(1);

    d_monitor.registerSubStream(d_id);

    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_ALIVE);

    putMessage(d_id);

    BMQTST_ASSERT(d_monitor.isAlarmScheduled());

    // Simulate alarm event dispatching after max idle time
    d_monitor.alarmEventDispatched();

    BMQTST_ASSERT(!d_monitor.isAlarmScheduled());

    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_IDLE);

    bmqtst::ScopedLogObserver logObserver(ball::Severity::e_INFO,
                                          bmqtst::TestHelperUtil::allocator());

    // Simulate message removing due to TTL or queue purge
    d_haveUndelivered.erase(d_id);

    d_monitor.idleEventDispatched(d_id);

    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_ALIVE);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 1u);
    BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
        logObserver.records().back(),
        "no longer appears to be stuck",
        bmqtst::TestHelperUtil::allocator()));
}

BMQTST_TEST_F(Test, changeMaxIdleTime)
// ------------------------------------------------------------------------
// Concerns: setting max idle time to new value. Setting zero value resets
// monitoring.
//
// Plan: Instantiate component, put message in queue, simulate time pass and
// check that state flips to IDLE according to specs, change max idle time,
// check that state remains in idle. Then disable monitor and check that
// state is changed to alive. Then enable monitor and check that after max
// idle time state is back to 'idle'.
// ------------------------------------------------------------------------
{
    d_monitor.setMaxIdleTime(1);

    d_monitor.registerSubStream(d_id);

    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_ALIVE);

    putMessage(d_id);

    BMQTST_ASSERT(d_monitor.isAlarmScheduled());

    d_monitor.alarmEventDispatched();

    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_IDLE);

    d_monitor.setMaxIdleTime(2);

    // No change in state if new value is set
    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_IDLE);

    bmqtst::ScopedLogObserver logObserver(ball::Severity::e_INFO,
                                          bmqtst::TestHelperUtil::allocator());

    // Disable monitoring
    d_monitor.setMaxIdleTime(0);

    BMQTST_ASSERT(!d_monitor.isAlarmScheduled());

    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_ALIVE);
    BMQTST_ASSERT_EQ(logObserver.records().size(), 0u);

    // Enable monitoring again
    d_monitor.setMaxIdleTime(2);

    // Simulate message posting to schedule alarm event
    d_monitor.onMessagePosted();

    BMQTST_ASSERT(d_monitor.isAlarmScheduled());

    d_monitor.alarmEventDispatched();

    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_IDLE)

    BMQTST_ASSERT_EQ(logObserver.records().size(), 1u);
}

BMQTST_TEST_F(Test, reset)
// ------------------------------------------------------------------------
// Concerns: 'reset' puts component in same state as just after
// construction.
//
// Plan: Instantiate component, put message in queue, call 'reset', make
// time pass beyond idle, check that nothing was logged.
// ------------------------------------------------------------------------
{
    d_monitor.setMaxIdleTime(1);

    d_monitor.registerSubStream(d_id);

    putMessage(d_id);

    BMQTST_ASSERT(d_monitor.isAlarmScheduled());

    d_monitor.alarmEventDispatched();

    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_IDLE);

    bmqtst::ScopedLogObserver logObserver(ball::Severity::e_INFO,
                                          bmqtst::TestHelperUtil::allocator());
    d_monitor.reset();

    BMQTST_ASSERT(!d_monitor.isAlarmScheduled());

    BMQTST_ASSERT_EQ(logObserver.records().size(), 0u);
}

BMQTST_TEST_F(Test, putAliveIdleSendAliveTwoSubstreams)
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
    bmqtst::ScopedLogObserver logObserver(ball::Severity::e_INFO,
                                          bmqtst::TestHelperUtil::allocator());
    mqbu::StorageKey          key1, key2;
    key1.fromHex("ABCDEF1234");
    key2.fromHex("1234ABCDEF");
    bsl::string id1("app1");
    bsl::string id2("app2");

    d_monitor.setMaxIdleTime(1);

    bmqu::MemOutStream errorDescription(bmqtst::TestHelperUtil::allocator());
    d_storage.addVirtualStorage(errorDescription, id1, key1);
    d_storage.addVirtualStorage(errorDescription, id2, key2);

    size_t expectedLogRecords = logObserver.records().size();

    d_monitor.registerSubStream(id1);
    d_monitor.registerSubStream(id2);

    BMQTST_ASSERT_EQ(d_monitor.state(id1),
                     QueueConsumptionMonitor::State::e_ALIVE);
    BMQTST_ASSERT_EQ(d_monitor.state(id2),
                     QueueConsumptionMonitor::State::e_ALIVE);

    putMessage(id1);
    putMessage(id2);

    BMQTST_ASSERT(d_monitor.isAlarmScheduled());

    d_monitor.alarmEventDispatched();

    BMQTST_ASSERT(!d_monitor.isAlarmScheduled());

    BMQTST_ASSERT_EQ(d_monitor.state(id1),
                     QueueConsumptionMonitor::State::e_IDLE);
    BMQTST_ASSERT_EQ(d_monitor.state(id2),
                     QueueConsumptionMonitor::State::e_IDLE);

    BMQTST_ASSERT_EQ(logObserver.records().size(), expectedLogRecords += 2);

    // Check that ALARM was logged for both substreams
    for (int i = 0; i < 2; ++i) {
        BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
            logObserver.records().rbegin()[i],
            "ALARM \\[QUEUE_STUCK\\]",
            bmqtst::TestHelperUtil::allocator()));
    }

    // Consume message from first substream
    d_haveUndelivered.erase(id1);
    d_monitor.onMessageSent(id1);

    BMQTST_ASSERT_EQ(logObserver.records().size(), expectedLogRecords += 1);
    BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
        logObserver.records().back(),
        "Queue 'bmq://bmq.test.local/test_queue\\?id=app1' no longer appears "
        "to be stuck.",
        bmqtst::TestHelperUtil::allocator()));

    BMQTST_ASSERT_EQ(d_monitor.state(id1),
                     QueueConsumptionMonitor::State::e_ALIVE);
    BMQTST_ASSERT_EQ(d_monitor.state(id2),
                     QueueConsumptionMonitor::State::e_IDLE);

    // Consume message from second substream
    d_haveUndelivered.erase(id2);
    d_monitor.onMessageSent(id2);

    BMQTST_ASSERT_EQ(logObserver.records().size(), expectedLogRecords += 1);
    BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
        logObserver.records().back(),
        "Queue 'bmq://bmq.test.local/test_queue\\?id=app2' no longer appears "
        "to be stuck.",
        bmqtst::TestHelperUtil::allocator()));
    BMQTST_ASSERT_EQ(d_monitor.state(id1),
                     QueueConsumptionMonitor::State::e_ALIVE);
    BMQTST_ASSERT_EQ(d_monitor.state(id2),
                     QueueConsumptionMonitor::State::e_ALIVE);
}

BMQTST_TEST_F(Test, usage)
// -------------------------------------------------------------------------
// Concerns: Make sure the usage example is correct.
//
// Plan: Following code can be edited into the usage example via a few
// simple replacements and deletions.
// -------------------------------------------------------------------------
{
#define monitor d_monitor

    bmqtst::ScopedLogObserver logObserver(ball::Severity::e_INFO,
                                          bmqtst::TestHelperUtil::allocator());
    size_t                    expectedLogRecords = 0U;

    monitor.setMaxIdleTime(20);

    monitor.registerSubStream(d_id);

    putMessage(d_id);
    putMessage(d_id);

    BMQTST_ASSERT(d_monitor.isAlarmScheduled());

    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_ALIVE);

    // Simulate alarm event dispatching after max idle time
    monitor.alarmEventDispatched();

    // log ALARM
    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_IDLE);
    BMQTST_ASSERT_EQ(logObserver.records().size(), ++expectedLogRecords);
    BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
        logObserver.records().back(),
        "ALARM \\[QUEUE_STUCK\\]",
        bmqtst::TestHelperUtil::allocator()));

    // consume first message
    monitor.onMessageSent(d_id);

    // Simulate idle event dispatching
    monitor.idleEventDispatched(d_id);

    // remain ALARM
    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_IDLE);

    // consume second message
    d_haveUndelivered.erase(d_id);
    monitor.onMessageSent(d_id);

    BMQTST_ASSERT(!d_monitor.isAlarmScheduled());

    // log INFO: back to active
    BMQTST_ASSERT_EQ(d_monitor.state(d_id),
                     QueueConsumptionMonitor::State::e_ALIVE);
    BMQTST_ASSERT_EQ(logObserver.records().size(), ++expectedLogRecords);

#undef monitor
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    BALL_LOG_SET_CATEGORY("MAIN");

    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    ball::LoggerManager::singleton().setDefaultThresholdLevels(
        ball::Severity::e_OFF,
        ball::Severity::e_INFO,
        ball::Severity::e_OFF,
        ball::Severity::e_OFF);
    {
        mqbcfg::AppConfig brokerConfig(bmqtst::TestHelperUtil::allocator());
        mqbcfg::BrokerConfig::set(brokerConfig);

        bsl::shared_ptr<bmqst::StatContext> statContext =
            mqbstat::BrokerStatsUtil::initializeStatContext(
                30,
                bmqtst::TestHelperUtil::allocator());

        bmqtst::runTest(_testCase);
    }

    bmqt::UriParser::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
