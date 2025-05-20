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

// mqbblp_queueconsumptionmonitor.h                                   -*-C++-*-
#ifndef INCLUDED_MQBBLP_QUEUECONSUMPTIONMONITOR
#define INCLUDED_MQBBLP_QUEUECONSUMPTIONMONITOR

/// @file mqbblp_queueconsumptionmonitor.h
///
/// @brief Provide a component that alerts if a queue is not read for a while.
///
/// @brief{mqbblp::QueueConsumptionMonitor} provides a mechanism that
/// monitors a queue and alerts if the queue has not been consumed for a
/// configurable amount of time.  Monitoring does not happen until a "maximum
/// idle time" has been set (using `setMaxIdleTime`).  Monitoring can be
/// disabled by setting the maximum idle time to zero.
///
/// Once in monitoring mode, the component is operated by a series of calls to
/// `onMessagePosted` when message is posted and `onMessageSent` when it is
/// delivered.
/// When `onMessagePosted` is called, it schedules alarm event to be executed
/// in the maximum idle time (if it was not already scheduled).  When
/// alarm event is executed, it calls for each substream `logAlarmCb` callback,
/// which checks if there are un-delivered messages. If the oldest undelivered
/// message alartm time in the past, alarm is logged and monitor puts this
/// substream in 'idle' state. Then monitor calculates the earliest alarm time
/// for all substreams and reschedules the alarm event if alarm time is in the
/// future. When `onMessageSent` is called for corresponding substream, if
/// substream is in 'idle' state, it is put back to 'alive' state and an INFO
/// record is written to the log. When queue becomes empty for corresponding
/// substream (e.g. by queue purging or messages garbage collected due to TTL),
/// it is put back to 'alive' state and an INFO record is written to the log.
///
/// The `maxIdleTime` represents the minimum time before an alarm will be
/// emitted would the queue be stale.
///
/// Thread safety                      {#mqbblp_queueconsumptionmonitor_thread}
/// =============
///
/// This component is *not* thread safe.  Its functions *must* be called from
/// the queue's dispatcher thread.
///
/// Usage Example                       {#mqbblp_queueconsumptionmonitor_usage}
/// =============
///
/// This example shows how to use this component.
///
/// ```
/// mqbblp::QueueConsumptionMonitor monitor;
/// monitor.setMaxIdleTime(20);
/// // put 2 messages in queue
///
/// // notify first message posted
/// monitor.onMessagePosted(id);
///
/// // notify second message posted
/// monitor.onMessagePosted(id);
///
/// bslmt::ThreadUtil::microSleep(21 *
/// bdlt::TimeUnitRatio::k_MICROSECONDS_PER_SECOND); // sleep for 21 seconds
///
/// // log ALARM
///
/// // consume first message
/// monitor.onMessageSent(id);
///
/// // log INFO: back to active
///
/// bslmt::ThreadUtil::microSleep(21 *
/// bdlt::TimeUnitRatio::k_MICROSECONDS_PER_SECOND); // sleep for 21 seconds
///
/// // log ALARM
///
/// // consume second message
/// monitor.onMessageSent(id);
///
/// // log INFO: back to active
/// ```

// MQB
#include <mqbblp_queuestate.h>
#include <mqbi_queue.h>

// BDE
#include <ball_log.h>
#include <bsl_unordered_map.h>
#include <bsl_utility.h>
#include <bslh_hash.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_cpp11.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATIONS
namespace mqbi {
class StorageIterator;
}

namespace mqbblp {

// =============================
// class QueueConsumptionMonitor
// =============================

/// Mechanism to monitor a queue and alert if it is not consumed for a
/// configurable amount of time.
class QueueConsumptionMonitor {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.QUEUECONSUMPTIONMONITOR");

  public:
    // PUBLIC TYPES

    /// Struct-enum representing the state the queue can be in.
    struct State {
        enum Enum { e_ALIVE = 0, e_IDLE = 1 };

        // CLASS METHODS

        /// Write the string representation of the specified enumeration
        /// `value` to the specified output `stream`, and return a reference
        /// to `stream`.  Optionally specify an initial indentation `level`,
        /// whose absolute value is incremented recursively for nested
        /// objects.  If `level` is specified, optionally specify
        /// `spacesPerLevel`, whose absolute value indicates the number of
        /// spaces per indentation level for this and all of its nested
        /// objects.  If `level` is negative, suppress indentation of the
        /// first line.  If `spacesPerLevel` is negative, format the entire
        /// output on one line, suppressing all but the initial indentation
        /// (as governed by `level`).  See `toAscii` for what constitutes
        /// the string representation of a `State::Enum` value.
        static bsl::ostream& print(bsl::ostream& stream,
                                   State::Enum   value,
                                   int           level          = 0,
                                   int           spacesPerLevel = 4);

        /// Return the non-modifiable string representation corresponding to
        /// the specified enumeration `value`, if it exists, and a unique
        /// (error) string otherwise.  The string representation of `value`
        /// matches its corresponding enumerator name with the `e_` prefix
        /// elided.  Note that specifying a `value` that does not match any
        /// of the enumerators will result in a string representation that
        /// is distinct from any of those corresponding to the enumerators,
        /// but is otherwise unspecified.
        static const char* toAscii(State::Enum value);
    };

    /// Callback function to log alarm info.
    /// Return `true` if there are un-delivered messages for the specified
    /// `id`, `false` otherwise.  If the specified `enableLog` is true, there
    /// are un-delivered messages for the specified `id` and calculated alarm
    /// time for the specified `now` is in the past, alarm is logged. Set in
    /// the specified `alarmTime_p` calculated alarm time for the oldest
    /// undelivered message.
    /// If `enableLog` is false, `alarmTime_p` is not set and alarm is not
    /// logged.
    typedef bsl::function<bool(bsls::TimeInterval*       alarmTime_p,
                               const bsl::string&        id,
                               const bsls::TimeInterval& now,
                               bool                      enableLog)>
        LoggingCb;

  private:
    // PRIVATE TYPES

    /// Struct representing the context for each sub stream of the queue.
    struct SubStreamInfo {
        // CREATORS

        SubStreamInfo();
        
        ~SubStreamInfo();

        // PUBLIC DATA

        /// EventHandle for the idle event, used by event scheduler.
        bdlmt::EventSchedulerEventHandle d_idleEventHandle;

        /// The current state.
        State::Enum d_state;
    };

    typedef bsl::unordered_map<bsl::string, SubStreamInfo> SubStreamInfoMap;

    typedef SubStreamInfoMap::iterator SubStreamInfoMapIter;

    typedef SubStreamInfoMap::const_iterator SubStreamInfoMapConstIter;

    // DATA

    /// Object representing the state of the queue associated with this object.
    /// Held but not owned.
    QueueState* d_queueState_p;

    /// Event scheduler. Held but not owned.
    bdlmt::EventScheduler* d_scheduler_p;

    /// EventHandle for triggering alarm, used by event scheduler.
    bdlmt::EventSchedulerEventHandle d_alarmEventHandle;

    /// Maximum time, in seconds, before the queue is declared idle.
    bsls::Types::Int64 d_maxIdleTimeSec;

    SubStreamInfoMap d_subStreamInfos;

    /// Callback to log alarm info if there are undelivered messages.  Return
    /// `true` if there are undelivered messages, `false` otherwise.
    LoggingCb d_loggingCb;

    // NOT IMPLEMENTED
    QueueConsumptionMonitor(const QueueConsumptionMonitor&) BSLS_CPP11_DELETED;
    QueueConsumptionMonitor&
    operator=(const QueueConsumptionMonitor&) BSLS_CPP11_DELETED;

    // ACCESSORS

    /// Return the `SubStreamInfo` corresponding to the specified `id`.
    const SubStreamInfo& subStreamInfo(const bsl::string& id) const;

    // MANIPULATORS

    /// Return the `SubStreamInfo` corresponding to the specified `id`.  It
    /// is an error to specify an `id` that has not been previously
    /// registered via `registerSubStream`.
    SubStreamInfo& subStreamInfo(const bsl::string& id);

    /// Update the specified `subStreamInfo`, associated to the specified
    /// `id`, and write log, upon transition to alive state.
    void onTransitionToAlive(SubStreamInfo*     subStreamInfo,
                             const bsl::string& id);

    /// Schedule the alarm event for the specified `alarmTime`.
    void scheduleAlarmEvent(const bsls::TimeInterval& alarmTime);

    /// Schedule the idle event for the specified `subStreamInfo` and `id`.
    void scheduleIdleEvent(SubStreamInfo*     subStreamInfo,
                           const bsl::string& id);

    /// Handler called by EventScheduler in scheduler dispatcher thread
    /// to forward alarm event to the queue dispatcher thread.
    void executeAlarmInQueueDispatcher();

    /// Handler called by EventScheduler in scheduler dispatcher thread
    /// to forward idle event to the queue dispatcher thread.
    void executeIdleInQueueDispatcher(const bsl::string id);

    /// Cancel all idle events (for all substreams) if they were scheduled.
    void cancelIdleEvents();

  protected:
    /// Alarm event dispatcher, executed in queue dispatcher thread.
    /// It checks if there are substreams that meet alarm condition and trigger
    /// the alarm for them.  If there are undelivered messages (among
    /// substreams) it reschedules alarm event.
    void alarmEventDispatched();

    /// Idle event dispatcher, executed in queue dispatcher thread.
    /// If there are no un-delivered messages the specified `id`, it calls
    /// onTransitionToAlive(). Otherwise, it reschedules idle event.
    void idleEventDispatched(const bsl::string id);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(QueueConsumptionMonitor,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `QueueConsumptionMonitor` object that monitors the queue
    /// specified by `queueState`. Use the specified `scheduler_p` for events
    //  scheduling and `loggingCb` callback for
    /// logging alarm data. Use the optionally specified `allocator` to supply
    /// memory.  If `allocator` is 0, the currently installed default allocator
    /// is used.
    QueueConsumptionMonitor(QueueState*       queueState,
                            const LoggingCb&  loggingCb,
                            bslma::Allocator* allocator);

    ~QueueConsumptionMonitor();

    // MANIPULATORS

    /// Configure this object to trigger an alarm if the monitored queue is
    /// not empty and has not been consumed for the specified (positive)
    /// `value`, and resets it (see `reset`).  Setting `maxIdleTime` to zero
    /// is permitted, in which case the monitoring is disabled.  This
    /// function may be called more than once.  Each time it is called, the
    /// component behaves as if `onMessagePosted` and `onMessageSent` had never
    /// been called.  If this causes substreams to return to `alive` state,
    /// nothing is logged.  Return a reference offering modifiable access to
    /// this object.
    QueueConsumptionMonitor& setMaxIdleTime(bsls::Types::Int64 value);

    /// Register the substream identified by the specified `id`.
    void registerSubStream(const bsl::string& id);

    /// Stop monitoring the substream identified by the specified `id`.
    /// `id` must have been previously registered via `registerSubStream`.
    void unregisterSubStream(const bsl::string& id);

    /// Put the object back in construction state.
    void reset();

    /// Notify the monitor that message were posted
    /// (for any substream).  It is used to schedule the event
    /// (if it was not scheduled yet) to monitor the delivery.
    void onMessagePosted();

    /// Notify the monitor that one or more messages were sent during the
    /// current time period for the substream specified by `id`.  It is an
    /// error to specify an `id` that has not been previously registered via
    /// `registerSubStream`.
    void onMessageSent(const bsl::string& id);

    // ACCESSORS

    /// Return the current activity status for the monitored queue for the
    /// substream specified by `id`.  It is an error to specify a `id`
    /// that has not been previously registered via `registerSubStream`.
    State::Enum state(const bsl::string& id) const;

    /// Calculate the time interval for the alarm event to be scheduled for the
    /// specified 'arrivalTimeDeltaNs' (in nanoseconds) and `now` as follows:
    /// alarmTime = now - arrivalTimeDeltaNs + maxIdleTime.
    bsls::TimeInterval
    calculateAlarmTime(bsls::Types::Int64        arrivalTimeDeltaNs,
                       const bsls::TimeInterval& now) const;

    /// Return `true` if the alarm event is scheduled, and `false` otherwise.
    bool isAlarmScheduled() const;
};

// FREE OPERATORS

/// Write the string representation of the specified enumeration `value` to
/// the specified output `stream` in a single-line format, and return a
/// reference to `stream`.  See `toAscii` for what constitutes the string
/// representation of a `QueueConsumptionMonitor::State::Enum` value.  Note
/// that this method has the same behavior as
/// ```
/// mqbblp::QueueConsumptionMonitor::State::print(stream, value, 0, -1);
/// ```
bsl::ostream& operator<<(bsl::ostream&                        stream,
                         QueueConsumptionMonitor::State::Enum value);

// ============================================================================
//                            INLINE DEFINITIONS
// ============================================================================

// -----------------------------
// class QueueConsumptionMonitor
// -----------------------------

inline QueueConsumptionMonitor::SubStreamInfo&
QueueConsumptionMonitor::subStreamInfo(const bsl::string& id)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    SubStreamInfoMapIter iter = d_subStreamInfos.find(id);
    BSLS_ASSERT_SAFE(iter != d_subStreamInfos.end());
    return iter->second;
}

inline const QueueConsumptionMonitor::SubStreamInfo&
QueueConsumptionMonitor::subStreamInfo(const bsl::string& id) const
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    SubStreamInfoMapConstIter iter = d_subStreamInfos.find(id);
    BSLS_ASSERT_SAFE(iter != d_subStreamInfos.end());
    return iter->second;
}

inline QueueConsumptionMonitor::State::Enum
QueueConsumptionMonitor::state(const bsl::string& id) const
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    return subStreamInfo(id).d_state;
}

}  // close package namespace

// -------------------------------------
// struct QueueConsumptionMonitor::State
// -------------------------------------

inline bsl::ostream&
mqbblp::operator<<(bsl::ostream&                        stream,
                   QueueConsumptionMonitor::State::Enum value)
{
    return QueueConsumptionMonitor::State::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
