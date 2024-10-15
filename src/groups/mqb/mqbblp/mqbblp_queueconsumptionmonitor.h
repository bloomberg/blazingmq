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

//@PURPOSE: Provide a component that alerts if a queue is not read for a while.
//
//@CLASSES:
//  mqbblp::QueueConsumptionMonitor: mechanism monitoring queue consumption
//
//@DESCRIPTION: 'mqbblp::QueueConsumptionMonitor' provides a mechanism that
// monitors a queue and alerts if the queue has not been consumed for a
// configurable amount of time.  Monitoring does not happen until a "maximum
// idle time" has been set (using 'setMaxIdleTime').  Monitoring can be
// disabled by setting the maximum idle time to zero.
//
// Once in monitoring mode, the component is operated by a series of calls to
// 'onMessageSent' and 'onTimer(currentTime)', in arbitrary order.  Each time
// that 'onTimer(currentTime)' is called, the component first checks whether
// the queue is in 'alive' state.  It is the case if:
//: o this the first call to 'onTimer' since monitoring was switched on
//: o 'onMessageSent' was called since 'onTimer' was last called
//: o the queue is empty
//
// Then the component checks how much "time" has elapsed since the queue was
// last seen in 'active' state (this may be zero, if one of the aforementioned
// condition was met).  If that period exceeds the value specified via
// 'setMaxIdleTime', then the queue is in 'idle' state.  Otherwise it is in
// 'active' state.  If the state changes to 'idle', an alarm is written to the
// log.  If the state changes to 'active', an INFO record is written to the
// log.
//
// The 'maxIdleTime' represents the minimum time before an alarm will be
// emitted would the queue be stale, but the alarm may be emitted anytime
// between '[maxIdleTime, maxIdleTime + frequency[', where 'frequency'
// represents the time period used between two consecutive calls to 'onTimer'.
//
// NOTE: the component does not assume any specific units for "time" - the sole
// constraint is that the values passed to 'onTimer' monotonically (but not
// strictly) increase.  Typically, these values are obtained from
// 'bsls::TimeUtil::getTimer()', which returns a number of nanoseconds elapsed
// from an arbitrary but fixed point in time.
//
/// Thread safety
///-------------
// This component is *not* thread safe.  Its functions *must* be called from
// the queue's dispatcher thread.
//
/// Usage Example
///-------------
// This example shows how to use this component.
//..
//  mqbblp::QueueConsumptionMonitor monitor;
//  monitor.setMaxIdleTime(20 * bdlt::TimeUnitRatio::k_NS_PER_S);
//  put 2 messages in queue
//
//  some time later, at time T:
//  monitor.onTimer(bsls::TimeUtil::getTimer()); // nothing is logged
//
//  // 15 seconds later - T + 15s
//  monitor.onTimer(bsls::TimeUtil::getTimer()); // nothing is logged
//
//  // 15 seconds later - T + 30s
//  monitor.onTimer(bsls::TimeUtil::getTimer()); // log ALARM
//
//  // 15 seconds later - T + 45s
//  monitor.onTimer(bsls::TimeUtil::getTimer()); // nothing is logged
//
//  // 15 seconds later - T + 60s
//  // consume first message
//  monitor.onMessageSent(mqbu::StorageKey::k_NULL_KEY);
//
//  // 15 seconds later - T + 75s
//  monitor.onTimer(bsls::TimeUtil::getTimer()); // log INFO: back to active
//
//  // 15 seconds later - T + 90s
//  monitor.onTimer(bsls::TimeUtil::getTimer()); // nothing is logged
//
//  // 15 seconds later - T + 105s
//  monitor.onTimer(bsls::TimeUtil::getTimer()); // log ALARM
//
//  // 15 seconds later - T + 120s
//  // consume second message
//  monitor.onMessageSent(mqbu::StorageKey::k_NULL_KEY);
//
//  // 15 seconds later - T + 135s
//  monitor.onTimer(bsls::TimeUtil::getTimer()); // log INFO: back to active
//..

// MQB

#include <mqbblp_queuestate.h>
#include <mqbi_queue.h>
#include <mqbu_storagekey.h>

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

    /// Struct-enum representing the possible state-transition, as a result
    /// of calls to the `onTimer` method.
    struct Transition {
        enum Enum { e_UNCHANGED = 0, e_ALIVE = 1, e_IDLE = 2 };

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
        /// the string representation of a `Transition::Enum` value.
        static bsl::ostream& print(bsl::ostream&    stream,
                                   Transition::Enum value,
                                   int              level          = 0,
                                   int              spacesPerLevel = 4);

        /// Return the non-modifiable string representation corresponding to
        /// the specified enumeration `value`, if it exists, and a unique
        /// (error) string otherwise.  The string representation of `value`
        /// matches its corresponding enumerator name with the `e_` prefix
        /// elided.  Note that specifying a `value` that does not match any
        /// of the enumerators will result in a string representation that
        /// is distinct from any of those corresponding to the enumerators,
        /// but is otherwise unspecified.
        static const char* toAscii(Transition::Enum value);
    };

    /// Callback function to log alarm info when queue state transitions to
    /// idle. First argument is the app key, second argument is a boolean flag
    /// to enable logging. If `enableLog` is `false`, logging is skipped.
    /// Return `true` if there are un-delivered messages and `false` otherwise.
    typedef bsl::function<bool(const mqbu::StorageKey& appKey, bool enableLog)>
        LoggingCb;

  private:
    // PRIVATE TYPES

    /// Struct representing the context for each sub stream of the queue.
    struct SubStreamInfo {
        // CREATORS

        SubStreamInfo();

        // PUBLIC DATA
        bsls::Types::Int64 d_lastKnownGoodTimer;
        // Timer value, in arbitrary unit, of
        // the last time the substream was in
        // good state.

        bool d_messageSent;
        // Whether a message was sent during
        // the last time slice

        State::Enum d_state;  // The current state.
    };

    typedef bsl::unordered_map<mqbu::StorageKey,
                               SubStreamInfo,
                               bslh::Hash<mqbu::StorageKeyHashAlgo> >
        SubStreamInfoMap;

    typedef SubStreamInfoMap::iterator SubStreamInfoMapIter;

    typedef SubStreamInfoMap::const_iterator SubStreamInfoMapConstIter;

    // DATA
    QueueState* d_queueState_p;
    // Object representing the state of the queue
    // associated with this object, held but not owned.

    bsls::Types::Int64 d_maxIdleTime;
    // Maximum time, in arbitrary units, before the queue
    // is declared idle.

    bsls::Types::Int64 d_currentTimer;
    // Timer, in arbitrary unit, of the current time.

    SubStreamInfoMap d_subStreamInfos;

    /// Callback to log alarm info if there are undelivered messages.
    /// Return `true` if there are undelivered messages, `false` otherwise.
    LoggingCb d_loggingCb;

    // NOT IMPLEMENTED
    QueueConsumptionMonitor(const QueueConsumptionMonitor&) BSLS_CPP11_DELETED;
    QueueConsumptionMonitor&
    operator=(const QueueConsumptionMonitor&) BSLS_CPP11_DELETED;

    // ACCESSORS

    /// Return the `SubStreamInfo` corresponding to the specified `key`.
    const SubStreamInfo& subStreamInfo(const mqbu::StorageKey& key) const;

    /// Return the `SubStreamInfo` corresponding to the specified `key`.  It
    /// is an error to specify a `key` that has not been previously
    /// registered via `registerSubStream`.
    SubStreamInfo& subStreamInfo(const mqbu::StorageKey& key);

    // MANIPULATORS

    /// Update the specified `subStreamInfo`, associated to the specified
    /// `appKey`, and write log, upon transition to alive state.
    void onTransitionToAlive(SubStreamInfo*          subStreamInfo,
                             const mqbu::StorageKey& appKey);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(QueueConsumptionMonitor,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `QueueConsumptionMonitor` object that monitors the queue
    /// specified by `queueState`. Use the specified `loggingCb` callback for
    /// logging alarm data. Use the optionally specified `allocator` to supply
    /// memory.  If `allocator` is 0, the currently installed default allocator
    /// is used.
    QueueConsumptionMonitor(QueueState*       queueState,
                            const LoggingCb&  loggingCb,
                            bslma::Allocator* allocator);

    // MANIPULATORS

    /// Configure this object to trigger an alarm if the monitored queue is
    /// not empty and has not been consumed for the specified (positive)
    /// `value`, and resets it (see `reset`).  Setting `maxIdleTime` to zero
    /// is permitted, in which case the monitoring is disabled.  This
    /// function may be called more than once.  Each time it is called, the
    /// component behaves as if `onTimer` and `onMessageSent` had never been
    /// called.  If this causes substreams to return to `alive` state,
    /// nothing is logged.  Return a reference offering modifiable access to
    /// this object.
    QueueConsumptionMonitor& setMaxIdleTime(bsls::Types::Int64 value);

    /// Register the substream identified by the specified `key`.
    /// `key` may be `StorageKey::k_NULL_KEY`, in which case no other key may
    /// be registered via this function. It is illegal to register the same
    /// substream more than once.
    void registerSubStream(const mqbu::StorageKey& key);

    /// Stop monitoring the substream identified by the specified `key`.
    /// `key` must have been previously registered via `registerSubStream`.
    void unregisterSubStream(const mqbu::StorageKey& key);

    /// Put the object back in construction state.
    void reset();

    /// Close the current time period at the specified `currentTimer` and
    /// check the queue's activity status during that period. Generate an
    /// alarm if the queue becomes idle; or write an info message to the log
    /// if the queue transitions back to active.
    void onTimer(bsls::Types::Int64 currentTimer);

    /// Notify the monitor that one or more messages were sent during the
    /// current time period for the substream specified by `key`.  It is an
    /// error to specify a `key` that has not been previously registered via
    /// `registerSubStream`.
    void onMessageSent(const mqbu::StorageKey& key);

    // ACCESSORS

    /// Return the current activity status for the monitored queue for the
    /// substream specified by `key`.  It is an error to specify a `key`
    /// that has not been previously registered via `registerSubStream`.
    State::Enum state(const mqbu::StorageKey& key) const;
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

/// Write the string representation of the specified enumeration `value` to
/// the specified output `stream` in a single-line format, and return a
/// reference to `stream`.  See `toAscii` for what constitutes the string
/// representation of a `QueueConsumptionMonitor::Transition::Enum` value.
/// Note that this method has the same behavior as
/// ```
/// mqbblp::QueueConsumptionMonitor::Transition::print(stream, value, 0, -1);
/// ```
bsl::ostream& operator<<(bsl::ostream&                             stream,
                         QueueConsumptionMonitor::Transition::Enum value);

// ============================================================================
//                            INLINE DEFINITIONS
// ============================================================================

// -----------------------------
// class QueueConsumptionMonitor
// -----------------------------

inline QueueConsumptionMonitor::SubStreamInfo&
QueueConsumptionMonitor::subStreamInfo(const mqbu::StorageKey& key)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    SubStreamInfoMapIter iter = d_subStreamInfos.find(key);
    BSLS_ASSERT_SAFE(iter != d_subStreamInfos.end());
    return iter->second;
}

inline const QueueConsumptionMonitor::SubStreamInfo&
QueueConsumptionMonitor::subStreamInfo(const mqbu::StorageKey& key) const
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    SubStreamInfoMapConstIter iter = d_subStreamInfos.find(key);
    BSLS_ASSERT_SAFE(iter != d_subStreamInfos.end());
    return iter->second;
}

inline void QueueConsumptionMonitor::onMessageSent(const mqbu::StorageKey& key)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_maxIdleTime == 0)) {
        // monitoring is disabled
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    subStreamInfo(key).d_messageSent = true;
}

inline QueueConsumptionMonitor::State::Enum
QueueConsumptionMonitor::state(const mqbu::StorageKey& key) const
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    return subStreamInfo(key).d_state;
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

// ------------------------------------------
// struct QueueConsumptionMonitor::Transition
// ------------------------------------------

inline bsl::ostream&
mqbblp::operator<<(bsl::ostream&                             stream,
                   QueueConsumptionMonitor::Transition::Enum value)
{
    return QueueConsumptionMonitor::Transition::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
