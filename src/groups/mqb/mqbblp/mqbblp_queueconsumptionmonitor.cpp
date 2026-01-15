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

QueueConsumptionMonitor::SubStreamInfo::~SubStreamInfo()
{
    BSLS_ASSERT_SAFE(!d_idleEventHandle);
}

// -----------------------------
// class QueueConsumptionMonitor
// -----------------------------

// CREATORS
QueueConsumptionMonitor::QueueConsumptionMonitor(
    QueueState*              queueState,
    const HaveUndeliveredCb& haveUndeliveredCb,
    const LoggingCb&         loggingCb,
    bslma::Allocator*        allocator)
: d_queueState_p(queueState)
, d_alarmEventHandle()
, d_maxIdleTimeSec(0)
, d_subStreamInfos(allocator)
, d_scheduledAlarmTime()
, d_haveUndeliveredCb(haveUndeliveredCb)
, d_loggingCb(loggingCb)
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p);
    BSLS_ASSERT_SAFE(d_haveUndeliveredCb);
    BSLS_ASSERT_SAFE(d_loggingCb);
}

QueueConsumptionMonitor::~QueueConsumptionMonitor()
{
    BSLS_ASSERT_SAFE(!d_alarmEventHandle);
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

    if (d_maxIdleTimeSec == 0) {
        // Monitoring is disabled, cancel the idle events and reset all
        // substreams states.
        cancelIdleEvents(true);
    }

    // No change in app states when new max idle time value is set.

    // If alarm event was already scheduled
    if (d_alarmEventHandle) {
        // Cancel the event and execute alarmEventDispatched() to reschedule
        // alarm event for the new maxIdleTime. If rc != 0, it means that
        // cancellation was not successful because the event was already
        // dispatched. So it's not needed to call alarmEventDispatched() again.
        int rc = d_queueState_p->scheduler()->cancelEventAndWait(
            &d_alarmEventHandle);
        if (rc == 0) {
            d_alarmEventHandle.release();

            if (d_maxIdleTimeSec > 0) {
                alarmEventDispatched();
            }
        }
    }

    return *this;
}

void QueueConsumptionMonitor::registerSubStream(const bsl::string& appId)
{
    // Should always be called from the queue thread, but will be invoked from
    // the cluster thread once upon queue creation.

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_subStreamInfos.find(appId) == d_subStreamInfos.end());

    d_subStreamInfos.insert(bsl::make_pair(appId, SubStreamInfo()));
}

void QueueConsumptionMonitor::unregisterSubStream(const bsl::string& appId)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->inDispatcherThread());

    SubStreamInfoMapConstIter iter = d_subStreamInfos.find(appId);
    BSLS_ASSERT_SAFE(iter != d_subStreamInfos.end());
    d_subStreamInfos.erase(iter);
}

void QueueConsumptionMonitor::reset()
{
    // Should always be called from the queue thread, but will be invoked from
    // the cluster thread once upon queue creation.

    d_maxIdleTimeSec = 0;

    if (d_alarmEventHandle) {
        int rc = d_queueState_p->scheduler()->cancelEventAndWait(
            &d_alarmEventHandle);
        if (rc == 0) {
            d_alarmEventHandle.release();
        }
    }

    cancelIdleEvents(false);

    d_subStreamInfos.clear();
}

void QueueConsumptionMonitor::onMessagePosted()
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->inDispatcherThread());

    if (d_alarmEventHandle || isMonitoringDisabled()) {
        return;  // RETURN
    }

    // Schedule the event to be executed in 'now + maxIdleTime' time.
    scheduleAlarmEvent(
        calculateAlarmTime(0, d_queueState_p->scheduler()->now()));
}

void QueueConsumptionMonitor::onMessageSent(const bsl::string& appId)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->inDispatcherThread());

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(isMonitoringDisabled())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    SubStreamInfo& info = subStreamInfo(appId);
    if (info.d_state == State::e_IDLE) {
        // Call callback to check un-delivered messages with alarm time in the
        // past.
        bsls::TimeInterval now = d_queueState_p->scheduler()->now();
        bsls::TimeInterval alarmTime;
        bslma::ManagedPtr<mqbi::StorageIterator> oldestMsgIt =
            d_haveUndeliveredCb(&alarmTime, appId, now);
        if (oldestMsgIt) {
            if (alarmTime <= now) {
                // There are un-delivered messages with alarm time in the past.
                // Since the idle event was scheduled, it will continue to
                // track state.
                BSLS_ASSERT_SAFE(info.d_idleEventHandle);

                return;  // RETURN
            }

            // Since there is the oldest un-delivered message with alarm time
            // in the future, schedule or reschedule the alarm event if needed.
            scheduleOrRescheduleAlarmEventIfNeeded(alarmTime);
        }

        // There are no un-delivered messages or alarm time of the oldest
        // message is in the future, so we can transition the substream to
        // alive state.
        onTransitionToAlive(&info, appId);
    }
}

void QueueConsumptionMonitor::onTransitionToAlive(SubStreamInfo* subStreamInfo,
                                                  const bsl::string& appId)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->inDispatcherThread());
    BSLS_ASSERT_SAFE(subStreamInfo->d_state == State::e_IDLE);

    if (subStreamInfo->d_idleEventHandle) {
        // Cancel the idle event if it was scheduled.
        int rc = d_queueState_p->scheduler()->cancelEventAndWait(
            &subStreamInfo->d_idleEventHandle);
        if (rc == 0) {
            subStreamInfo->d_idleEventHandle.release();
        }
    }

    subStreamInfo->d_state = State::e_ALIVE;

    bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);

    BALL_LOG_INFO_BLOCK
    {
        bmqt::UriBuilder uriBuilder(d_queueState_p->uri(), &localAllocator);
        uriBuilder.setId(appId);

        bmqt::Uri uri(&localAllocator);
        uriBuilder.uri(&uri);

        BALL_LOG_OUTPUT_STREAM << "Queue '" << uri
                               << "' no longer appears to be stuck.";
    }
}

void QueueConsumptionMonitor::scheduleAlarmEvent(
    const bsls::TimeInterval& alarmTime)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->inDispatcherThread());
    BSLS_ASSERT_SAFE(!d_alarmEventHandle);

    d_scheduledAlarmTime = alarmTime;

    d_queueState_p->scheduler()->scheduleEvent(
        &d_alarmEventHandle,
        d_scheduledAlarmTime,
        bdlf::BindUtil::bind(
            &QueueConsumptionMonitor::executeAlarmInQueueDispatcher,
            this));
}

void QueueConsumptionMonitor::scheduleOrRescheduleAlarmEventIfNeeded(
    const bsls::TimeInterval& alarmTime)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->inDispatcherThread());

    if (d_alarmEventHandle) {
        // If the event is already scheduled, reschedule it only if the new
        // alarm time is earlier than the currently scheduled one.
        if (alarmTime < d_scheduledAlarmTime) {
            d_scheduledAlarmTime = alarmTime;
            d_queueState_p->scheduler()->rescheduleEvent(d_alarmEventHandle,
                                                         d_scheduledAlarmTime);
        }
    }
    else {
        // If the event is not scheduled, schedule it.
        scheduleAlarmEvent(alarmTime);
    }
}

void QueueConsumptionMonitor::scheduleIdleEvent(SubStreamInfo* subStreamInfo,
                                                const bsl::string& appId)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->inDispatcherThread());
    BSLS_ASSERT_SAFE(!subStreamInfo->d_idleEventHandle);
    BSLS_ASSERT_SAFE(subStreamInfo->d_state == State::e_IDLE);

    bsls::TimeInterval idleTime = d_queueState_p->scheduler()->now();
    idleTime.addSeconds(bsl::min(d_maxIdleTimeSec, k_IDLE_TIMER_PERIOD_SEC));
    d_queueState_p->scheduler()->scheduleEvent(
        &subStreamInfo->d_idleEventHandle,
        idleTime,
        bdlf::BindUtil::bind(
            &QueueConsumptionMonitor::executeIdleInQueueDispatcher,
            this,
            appId));
}

void QueueConsumptionMonitor::executeAlarmInQueueDispatcher()
{
    // executed by the *SCHEDULER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->scheduler()->isInDispatcherThread());

    // Forward event to the queue dispatcher thread
    d_queueState_p->queue()->dispatcher()->execute(
        bdlf::BindUtil::bind(&QueueConsumptionMonitor::alarmEventDispatched,
                             this),
        d_queueState_p->queue());
}

void QueueConsumptionMonitor::executeIdleInQueueDispatcher(
    const bsl::string& appId)
{
    // executed by the *SCHEDULER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->scheduler()->isInDispatcherThread());

    // Forward event to the queue dispatcher thread
    d_queueState_p->queue()->dispatcher()->execute(
        bdlf::BindUtil::bind(&QueueConsumptionMonitor::idleEventDispatched,
                             this,
                             appId),
        d_queueState_p->queue());
}

void QueueConsumptionMonitor::cancelIdleEvents(bool resetStates)
{
    // Should always be called from the queue thread, but will be invoked from
    // the cluster thread once upon queue creation.

    for (SubStreamInfoMapIter iter = d_subStreamInfos.begin(),
                              last = d_subStreamInfos.end();
         iter != last;
         ++iter) {
        SubStreamInfo& info = iter->second;
        if (info.d_idleEventHandle) {
            // Cancel the event if it was scheduled.
            int rc = d_queueState_p->scheduler()->cancelEventAndWait(
                &info.d_idleEventHandle);
            if (rc == 0) {
                info.d_idleEventHandle.release();
            }
        }

        if (resetStates) {
            // Reset the substream state to default.
            info.d_state = State::e_ALIVE;
        }
    }
}

void QueueConsumptionMonitor::alarmEventDispatched()
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->inDispatcherThread());

    if (d_alarmEventHandle) {
        d_alarmEventHandle.release();
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(isMonitoringDisabled())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    // Iterate all substreams and check alarms.
    bsls::TimeInterval now = d_queueState_p->scheduler()->now();
    bsls::TimeInterval minAlarmTime;
    minAlarmTime.addMicroseconds(
        bsl::numeric_limits<bsls::Types::Int64>::max());
    for (SubStreamInfoMapIter iter = d_subStreamInfos.begin(),
                              last = d_subStreamInfos.end();
         iter != last;
         ++iter) {
        SubStreamInfo&     info  = iter->second;
        const bsl::string& appId = iter->first;

        if (info.d_state == State::e_IDLE) {
            // skip substream in idle state,
            // idle event is scheduled for tracking it
            BSLS_ASSERT_SAFE(info.d_idleEventHandle);
            continue;  // CONTINUE
        }

        BSLS_ASSERT_SAFE(info.d_state == State::e_ALIVE);

        // Check un-delivered messages.
        bsls::TimeInterval                       alarmTime;
        bslma::ManagedPtr<mqbi::StorageIterator> oldestMsgIt =
            d_haveUndeliveredCb(&alarmTime, appId, now);

        if (!oldestMsgIt) {
            // No un-delivered messages
            continue;  // CONTINUE
        }
        else if (alarmTime <= now) {
            // Alarm time is in the past, log the alarm and mark substream as
            // idle.
            d_loggingCb(appId, oldestMsgIt);
            info.d_state = State::e_IDLE;
            // schedule polling event to check if queue becomes empty due
            // to GC or purge events.
            scheduleIdleEvent(&info, appId);
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

void QueueConsumptionMonitor::idleEventDispatched(const bsl::string& appId)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->inDispatcherThread());

    SubStreamInfo& info = subStreamInfo(appId);

    if (info.d_idleEventHandle) {
        info.d_idleEventHandle.release();
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(isMonitoringDisabled() ||
                                              info.d_state != State::e_IDLE)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(info.d_state == State::e_IDLE);

    // Call callback to check un-delivered messages with alarm time in the
    // past.
    bsls::TimeInterval now = d_queueState_p->scheduler()->now();
    bsls::TimeInterval alarmTime;
    bslma::ManagedPtr<mqbi::StorageIterator> oldestMsgIt =
        d_haveUndeliveredCb(&alarmTime, appId, now);

    if (oldestMsgIt) {
        // There are un-delivered messages with alarm time in the past,
        // schedule the idle event.
        if (alarmTime <= now) {
            scheduleIdleEvent(&info, appId);

            return;  // RETURN
        }

        // Since there is the oldest un-delivered message with alarm time in
        // the future, schedule or reschedule the alarm event if needed.
        scheduleOrRescheduleAlarmEventIfNeeded(alarmTime);
    }

    // There are no un-delivered messages or alarm time of the oldest message
    // is in the future, so we can transition the substream to alive state.
    onTransitionToAlive(&info, appId);
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

}  // close package namespace
}  // close enterprise namespace
