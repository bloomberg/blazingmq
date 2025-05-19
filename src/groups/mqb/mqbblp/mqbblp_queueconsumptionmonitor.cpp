// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mqbblp_queueconsumptionmonitor.cpp                                 -*-C++-*-
#include <mqbblp_queueconsumptionmonitor.h>

#include <mqbscm_version.h>
// MBQ
#include <mqbblp_queuehandlecatalog.h>
#include <mqbcmd_messages.h>
#include <mqbi_queueengine.h>
#include <mqbi_storage.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_queueflags.h>
#include <bmqt_uri.h>

#include <bmqtsk_alarmlog.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

// BDE
#include <ball_record.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_iomanip.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_stdallocator.h>
#include <bsls_performancehint.h>
#include <bslstl_stringref.h>

namespace BloombergLP {
namespace mqbblp {

namespace {
/// The period of the idle polling timer, in seconds.
static const bsls::Types::Int64 k_IDLE_TIMER_PERIOD_SEC = 30;
}  // close unnamed namespace

// -------------------------------------
// struct QueueConsumptionMonitor::State
// -------------------------------------

bsl::ostream& QueueConsumptionMonitor::State::print(
    bsl::ostream&                        stream,
    QueueConsumptionMonitor::State::Enum value,
    int                                  level,
    int                                  spacesPerLevel)
{
    stream << bmqu::PrintUtil::indent(level, spacesPerLevel)
           << QueueConsumptionMonitor::State::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* QueueConsumptionMonitor::State::toAscii(
    QueueConsumptionMonitor::State::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(ALIVE)
        CASE(IDLE)
    default: return "(* UNKNOWN *)";
    }

#undef case
}

// ---------------------------------------------
// struct QueueConsumptionMonitor::SubStreamInfo
// ---------------------------------------------

QueueConsumptionMonitor::SubStreamInfo::SubStreamInfo()
: d_idleEventHandle()
, d_state(State::e_ALIVE)
{
    // NOTHING
}

// -----------------------------
// class QueueConsumptionMonitor
// -----------------------------

// CREATORS
QueueConsumptionMonitor::QueueConsumptionMonitor(QueueState*       queueState,
                                                 const LoggingCb&  loggingCb,
                                                 bslma::Allocator* allocator)
: d_queueState_p(queueState)
, d_scheduler_p(queueState->scheduler())
, d_alarmEventHandle()
, d_maxIdleTimeSec(0)
, d_subStreamInfos(allocator)
, d_loggingCb(loggingCb)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p);
    BSLS_ASSERT_SAFE(d_scheduler_p);
    BSLS_ASSERT_SAFE(d_loggingCb);
}

QueueConsumptionMonitor::~QueueConsumptionMonitor()
{
    reset();
}

// MANIPULATORS
QueueConsumptionMonitor&
QueueConsumptionMonitor::setMaxIdleTime(bsls::Types::Int64 value)
{
    // Should always be called from the queue thread, but will be invoked from
    // the cluster thread once upon queue creation.

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(value >= 0);

    if (value == d_maxIdleTimeSec) {
        // No change
        return *this;  // RETURN
    }

    d_maxIdleTimeSec = value;

    cancelIdleEvents();

    for (SubStreamInfoMapIter iter = d_subStreamInfos.begin(),
                              last = d_subStreamInfos.end();
         iter != last;
         ++iter) {
        iter->second = SubStreamInfo();
    }

    // If alarm event was already scheduled
    if (d_alarmEventHandle) {
        // If monitor is disabled (e.g. in case of reconfigure),
        // cancel the event.
        if (value == 0) {
            d_scheduler_p->cancelEventAndWait(&d_alarmEventHandle);
            d_alarmEventHandle.release();
        }
        else {
            // TODO: need to reschedule the event at the new maxIdleTime from
            // the previously scheduled event. But there is no way to get
            // execution time of the previously scheduled event, because
            // d_scheduler_p is also used by throttleEventHandle and
            // EventScheduler::nextPendingEventTime() returns the closest event
            // time for all handles. So, just reschedule the event to be
            // executed in 'now + new maxIdleTime' time.
            d_scheduler_p->rescheduleEvent(
                d_alarmEventHandle,
                calculateAlarmTime(0, d_scheduler_p->now()));
        }
    }

    return *this;
}

void QueueConsumptionMonitor::registerSubStream(const bsl::string& id)
{
    // Should always be called from the queue thread, but will be invoked from
    // the cluster thread once upon queue creation.

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_subStreamInfos.find(id) == d_subStreamInfos.end());

    d_subStreamInfos.insert(bsl::make_pair(id, SubStreamInfo()));
}

void QueueConsumptionMonitor::unregisterSubStream(const bsl::string& id)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    SubStreamInfoMapConstIter iter = d_subStreamInfos.find(id);
    BSLS_ASSERT_SAFE(iter != d_subStreamInfos.end());
    d_subStreamInfos.erase(iter);
}

void QueueConsumptionMonitor::reset()
{
    // Should always be called from the queue thread, but will be invoked from
    // the cluster thread once upon queue creation.

    d_maxIdleTimeSec = 0;

    if (d_alarmEventHandle) {
        d_scheduler_p->cancelEventAndWait(&d_alarmEventHandle);
        d_alarmEventHandle.release();
    }

    cancelIdleEvents();

    d_subStreamInfos.clear();
}

void QueueConsumptionMonitor::onMessagePosted()
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    if (d_alarmEventHandle || d_maxIdleTimeSec == 0) {
        // Event is already scheduled or monitoring is disabled.
        return;  // RETURN
    }

    // Schedule the event to be executed in 'now + maxIdleTime' time.
    scheduleAlarmEvent(calculateAlarmTime(0, d_scheduler_p->now()));
}

void QueueConsumptionMonitor::onMessageSent(const bsl::string& id)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_maxIdleTimeSec == 0)) {
        // monitoring is disabled
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    SubStreamInfo& info = subStreamInfo(id);
    if (info.d_state == State::e_IDLE) {
        // substream was in idle state
        onTransitionToAlive(&info, id);
        if (!d_alarmEventHandle) {
            // Schedule alarm event to check if there are other un-delivered
            // messages.
            scheduleAlarmEvent(calculateAlarmTime(0, d_scheduler_p->now()));
        }
    }
}

void QueueConsumptionMonitor::onTransitionToAlive(SubStreamInfo* subStreamInfo,
                                                  const bsl::string& id)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    if (subStreamInfo->d_idleEventHandle) {
        // Cancel the idle event if it was scheduled.
        d_scheduler_p->cancelEventAndWait(&subStreamInfo->d_idleEventHandle);
        subStreamInfo->d_idleEventHandle.release();
    }

    subStreamInfo->d_state = State::e_ALIVE;

    bdlma::LocalSequentialAllocator<2048> localAllocator(0);

    bmqt::UriBuilder uriBuilder(d_queueState_p->uri(), &localAllocator);
    uriBuilder.setId(id);

    bmqt::Uri uri(&localAllocator);
    uriBuilder.uri(&uri);

    BALL_LOG_INFO << "Queue '" << uri << "' no longer appears to be stuck.";
}

void QueueConsumptionMonitor::scheduleAlarmEvent(
    const bsls::TimeInterval& alarmTime)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    d_scheduler_p->scheduleEvent(
        &d_alarmEventHandle,
        alarmTime,
        bdlf::BindUtil::bind(
            &QueueConsumptionMonitor::executeAlarmInQueueDispatcher,
            this));
}

void QueueConsumptionMonitor::scheduleIdleEvent(SubStreamInfo* subStreamInfo,
                                                const bsl::string& id)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));
    BSLS_ASSERT_SAFE(!subStreamInfo->d_idleEventHandle);
    BSLS_ASSERT_SAFE(subStreamInfo->d_state == State::e_IDLE);

    bsls::TimeInterval idleime = d_scheduler_p->now();
    idleime.addSeconds(bsl::min(d_maxIdleTimeSec, k_IDLE_TIMER_PERIOD_SEC));
    d_scheduler_p->scheduleEvent(
        &subStreamInfo->d_idleEventHandle,
        idleime,
        bdlf::BindUtil::bind(
            &QueueConsumptionMonitor::executeIdleInQueueDispatcher,
            this,
            id));
}

void QueueConsumptionMonitor::executeAlarmInQueueDispatcher()
{
    // executed by the *SCHEDULER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_scheduler_p->isInDispatcherThread());

    // Forward event to the queue dispatcher thread
    d_queueState_p->queue()->dispatcher()->execute(
        bdlf::BindUtil::bind(&QueueConsumptionMonitor::alarmEventDispatched,
                             this),
        d_queueState_p->queue());
}

void QueueConsumptionMonitor::executeIdleInQueueDispatcher(
    const bsl::string id)
{
    // executed by the *SCHEDULER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_scheduler_p->isInDispatcherThread());

    // Forward event to the queue dispatcher thread
    d_queueState_p->queue()->dispatcher()->execute(
        bdlf::BindUtil::bind(&QueueConsumptionMonitor::idleEventDispatched,
                             this,
                             id),
        d_queueState_p->queue());
}

void QueueConsumptionMonitor::cancelIdleEvents()
{
    // Should always be called from the queue thread, but will be invoked from
    // the cluster thread once upon queue creation.

    for (SubStreamInfoMapIter iter = d_subStreamInfos.begin(),
                              last = d_subStreamInfos.end();
         iter != last;
         ++iter) {
        SubStreamInfo info = iter->second;
        if (info.d_idleEventHandle) {
            // Cancel the event if it was scheduled.
            d_scheduler_p->cancelEventAndWait(&info.d_idleEventHandle);
            info.d_idleEventHandle.release();
        }
    }
}

void QueueConsumptionMonitor::alarmEventDispatched()
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_maxIdleTimeSec == 0)) {
        // monitoring is disabled
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    if (d_alarmEventHandle) {
        d_alarmEventHandle.release();
    }

    // Iterate all substreams and check alarms.
    bsls::TimeInterval now = d_scheduler_p->now();
    bsls::TimeInterval minAlarmTime;
    minAlarmTime.addMicroseconds(
        bsl::numeric_limits<bsls::Types::Int64>::max());
    for (SubStreamInfoMapIter iter = d_subStreamInfos.begin(),
                              last = d_subStreamInfos.end();
         iter != last;
         ++iter) {
        SubStreamInfo&     info = iter->second;
        const bsl::string& id   = iter->first;

        // Call alarm callback to log alarm if condition is met.
        const bsls::TimeInterval alarmTime =
            d_loggingCb(id, now, info.d_state == State::e_ALIVE);

        if (alarmTime == bsls::TimeInterval()) {
            // No un-delivered messages.
            if (info.d_state == State::e_IDLE) {
                onTransitionToAlive(&info, id);
            }
            continue;  // CONTINUE
        }
        else if (alarmTime <= now) {
            // Alarm time is in the past, alarm is logged, mark substream as
            // idle.
            if (info.d_state == State::e_ALIVE) {
                info.d_state = State::e_IDLE;
                // schedule polling event to check if queue becomes empty due
                // to GC or purge events.
                scheduleIdleEvent(&info, id);
            }
            continue;  // CONTINUE
        }

        // Remember the earliest alarm time.
        minAlarmTime = bsl::min(minAlarmTime, alarmTime);
    }

    // If minAlarmTime is set, reschedule the event for the earliest alarm
    // time.
    if (minAlarmTime.totalMicroseconds() !=
        bsl::numeric_limits<bsls::Types::Int64>::max()) {
        scheduleAlarmEvent(minAlarmTime);
    }
}

void QueueConsumptionMonitor::idleEventDispatched(const bsl::string id)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_maxIdleTimeSec == 0)) {
        // monitoring is disabled
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    SubStreamInfo& info = subStreamInfo(id);

    if (info.d_idleEventHandle) {
        info.d_idleEventHandle.release();
    }

    if (info.d_state == State::e_ALIVE) {
        // substream already in alive state
        return;  // RETURN
    }

    // Call alarm logging callback (with disabled log) to check if there are
    // still un-delivered messages.
    const bsls::TimeInterval alarmTime = d_loggingCb(id,
                                                     d_scheduler_p->now(),
                                                     false);

    if (alarmTime == bsls::TimeInterval()) {
        // No un-delivered messages, e.g. queue was purged or messages are
        // garbage collected by TTL.
        onTransitionToAlive(&info, id);
    }
    else {
        // There are still un-delivered messages, reschedule the event.
        scheduleIdleEvent(&info, id);
    }
}

// ACCESSORS

bsls::TimeInterval QueueConsumptionMonitor::calculateAlarmTime(
    bsls::Types::Int64        arrivalTimeDeltaNs,
    const bsls::TimeInterval& now) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(arrivalTimeDeltaNs >= 0);
    BSLS_ASSERT_SAFE(now != bsls::TimeInterval());

    // Calculate the time to schedule the event as:
    // executionTime = now - arrivalTimeDelta + maxIdleTime
    bsls::TimeInterval alarmTime = now;
    alarmTime.addNanoseconds(-arrivalTimeDeltaNs);
    alarmTime.addSeconds(d_maxIdleTimeSec);

    return alarmTime;
}

bool QueueConsumptionMonitor::isAlarmScheduled() const
{
    return d_alarmEventHandle ? true : false;
}

}  // close package namespace
}  // close enterprise namespace
