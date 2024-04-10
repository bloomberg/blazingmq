// Copyright 2019-2023 Bloomberg Finance L.P.
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

// mwcc_monitoredqueue.h                                              -*-C++-*-
#ifndef INCLUDED_MWCC_MONITOREDQUEUE
#define INCLUDED_MWCC_MONITOREDQUEUE

//@PURPOSE: Provide a queue that monitors its load.
//
//@CLASSES:
//  mwcc::MonitoredQueueState: Queue's monitor state enum
//  mwcc::MonitoredQueue:      A queue that monitors its load
//  mwcc::MonitoredQueueUtil:  Monitored  queue utilities
//
//@SEE_ALSO: bdlcc_fixedqueue, bdlcc_singleconsumerqueue,
//  bdlcc_singleproducerqueue
//
//@DESCRIPTION: This component defines a mechanism,
// 'mwcc::MonitoredQueue', which is a simple wrapper around a Queue type that
// monitors its size and alarms (once) when its size crosses into low
// watermark, high watermark, and high watermark2.  The state of the mechanism
// is represented with the 'mwcc::MonitoredQueueState' enum.  It additionally
// defines a utility struct, 'mwcc::MonitoredQueueUtil', providing utility
// functions associated with MonitoredQueues.
//
// The following parameters are used to configure this component's watermarks
// and thus set boundaries for the corresponding states ('NORMAL',
// 'HIGH_WATERMARK', 'HIGH_WATERMARK2'):
//: o !lowWatermark!: When the queue's state is in 'HIGH_WATERMARK' and the
//:   popping of an item reduces its size to 'lowWatermark', it emits 'NORMAL'
//:   and its state goes back to 'NORMAL'.
//:
//: o !highWatermark!: When the queue's state is in 'NORMAL' and the pushing of
//:   an item increases its size to 'highWatermark', it emits 'HIGH_WATERMARK'
//:   and enters the 'HIGH_WATERMARK' state.  It will go back to 'NORMAL' once
//:   the queue size decreases to 'lowWatermark'.
//:
//: o !highWatermark2!: When the queue's state is in either 'NORMAL' or
//:   'HIGH_WATERMARK' and the pushing of an item increases its size to
//:   'highWatermark2', it emits 'HIGH_WATERMARK2' and enters the
//:   'HIGH_WATERMARK2' state.  It will go back to 'NORMAL' once the queue size
//:   decreases to 'lowWatermark'.
//
/// Thread Safety
///-------------
// Thread-Safe in general use.  All the push/pop manipulators are thread safe
// since the queue has been completely initialised and set up.  However,
// 'setWatermarks()' and 'setStateCallback()' are not Thread-Safe.  They must
// not be called concurrently.
//
/// Supported Clock-Types
///---------------------
// Wherever this component supports timed operations, specifically in
// 'timedPopFront()', the specified timeout is an absolute offset which matches
// the epoch used in 'mwcsys::Time::nowMonotonicTime()'.

// MWC

// BDE
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_sstream.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_allocatorargt.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_condition.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_keyword.h>
#include <bsls_performancehint.h>
#include <bsls_systemclocktype.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace mwcc {

// ==========================
// struct MonitoredQueueState
// ==========================

/// This struct defines the type of state that a `mwcc::MonitoredQueue`
/// can be in.
struct MonitoredQueueState {
    // TYPES
    enum Enum {
        e_NORMAL                   = 0,
        e_HIGH_WATERMARK_REACHED   = 1,
        e_HIGH_WATERMARK_2_REACHED = 2,
        e_QUEUE_FILLED             = 3
    };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// `MonitoredQueueState::Enum` value.
    static bsl::ostream& print(bsl::ostream&             stream,
                               MonitoredQueueState::Enum value,
                               int                       level          = 0,
                               int                       spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(MonitoredQueueState::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&             stream,
                         MonitoredQueueState::Enum value);

// ==========================
// class MonitoredQueueTraits
// ==========================

template <class QUEUE>
struct MonitoredQueueTraits;

// ====================
// class MonitoredQueue
// ====================

/// Queue wrapper that monitors the queue's size to report when it becomes
/// full.
template <class QUEUE, class QUEUE_TRAITS = MonitoredQueueTraits<QUEUE> >
class MonitoredQueue {
    /// Called when the state of this `MonitoredQueue` changes.
    typedef bsl::function<void(MonitoredQueueState::Enum state)> StateCallback;

    typedef QUEUE_TRAITS                 Traits;
    typedef typename Traits::ElementType ElementType;

  private:
    // DATA
    bsls::Types::Int64 d_highWatermark2;

    bsls::Types::Int64 d_highWatermark;

    bsls::Types::Int64 d_lowWatermark;

    StateCallback d_stateChangedCb;

    bsls::AtomicInt d_state;
    // How full the queue is.
    //   0: below high watermark
    //   1: reached high watermark but never filled queue
    //   2: reached high watermark 2 but never full
    //   3: filled queue
    // The state doesn't go down until the queue size
    // reaches the low watermark.

    QUEUE d_queue;

    bsls::AtomicInt64 d_queueLength;

    bool d_supportTimedOperations;
    // Whether timed operations are supported
    // (timedPopFront). This has a slight performance
    // impact (conditionVariable.signal()), so it should
    // be enabled only if any timed operations will be
    // used on the queue.

    bslmt::Mutex d_timedOperationsMutex;
    // Mutex to use with the below condition variable for
    // timed operations

    bslmt::Condition d_timedOperationsCondition;
    // Condition variable to notify timedOperation of
    // data added to the queue

    // PRIVATE MANIPULATORS

    /// Atomically set the level of this queue to the specified `newState`,
    /// if it denotes a "more full" state.  Return `true` if this thread was
    /// the one that actually set the level or `false` if another thread set
    /// it or the `newState` denotes a "less full" state.
    bool setState(int newState);

    /// Increment `d_queueLength` and report if necessary
    void incrementLength();

    /// Decrement `d_queueLength` and report if necessary
    void decrementLength();

  private:
    // NOT IMPLEMENTED
    MonitoredQueue(const MonitoredQueue<QUEUE, QUEUE_TRAITS>&)
        BSLS_KEYWORD_DELETED;
    MonitoredQueue<QUEUE, QUEUE_TRAITS>&
    operator=(const MonitoredQueue<QUEUE, QUEUE_TRAITS>&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MonitoredQueue, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `mwcc::MonitoredQueue` object with capacity for the
    /// specified `initialCapacity` number of elements and the optionally
    /// specified `supportTimedOperations` flag indicating if this queue
    /// supports timed operations (false by default).  Use the optionally
    /// specified `basicAllocator` to supply memory.
    explicit MonitoredQueue(
        typename Traits::InitialCapacityType initialCapacity,
        bslma::Allocator*                    basicAllocator = 0);
    MonitoredQueue(typename Traits::InitialCapacityType initialCapacity,
                   bool                                 supportTimedOperations,
                   bslma::Allocator*                    basicAllocator = 0);

    // MANIPULATORS

    /// Set the high and low watermarks and return a reference offering
    /// modifiable access to this object.  The specified watermarks have to
    /// be of the following order:
    /// 0 <= lowWatermark < highWatermark <= highWatermark2 <= `LLONG_MAX`
    /// Note: `highWatermark` and `highWatermark` can only be equal if they
    /// are also equal to the upper limit of the `Int64` type.
    MonitoredQueue<QUEUE, QUEUE_TRAITS>&
    setWatermarks(bsls::Types::Int64 lowWatermark,
                  bsls::Types::Int64 highWatermark =
                      bsl::numeric_limits<bsls::Types::Int64>::max(),
                  bsls::Types::Int64 highWatermark2 =
                      bsl::numeric_limits<bsls::Types::Int64>::max());

    /// Set the state callback to be invoked when the state of the
    /// `MonitoredQueue` changes to the specified `stateChangedCb` and
    /// return a reference offering modifiable access to this object.
    MonitoredQueue<QUEUE, QUEUE_TRAITS>&
    setStateCallback(const StateCallback& stateChangedCb);

    /// Append the specified `value` to the back of this queue, blocking
    /// until either space is available - if necessary - or the queue is
    /// disabled.  Return 0 on success, and a nonzero value if the queue is
    /// disabled.
    int pushBack(const ElementType& value);

    /// Append the specified move-insertable `value` to the back of this
    /// queue, blocking until either space is available - if necessary - or
    /// the queue is disabled.  Return 0 on success, and a nonzero value if
    /// the queue is disabled.
    int pushBack(bslmf::MovableRef<ElementType> value);

    /// Attempt to append the specified `value` to the back of this queue
    /// without blocking.  Return 0 on success, and a non-zero value if the
    /// queue is full or disabled.
    int tryPushBack(const ElementType& value);

    /// Attempt to append the specified move-insertable `value` to the back
    /// of this queue without blocking.  `value` is left in a valid but
    /// unspecified state.  Return 0 on success, and a non-zero value if the
    /// queue is full or disabled.
    int tryPushBack(bslmf::MovableRef<ElementType> value);

    /// Remove the element from the front of this queue and load that
    /// element into the specified `value`.  If the queue is empty, block
    /// until it is not empty.  Return 0 on success, and a non-zero value
    /// otherwise.
    int popFront(ElementType* value);

    /// Attempt to remove the element from the front of this queue without
    /// blocking, and, if successful, load the specified `value` with the
    /// removed element.  Return 0 on success, and a non-zero value if queue
    /// was empty.  On failure, `value` is not changed.
    int tryPopFront(ElementType* value);

    /// Pop an element from the front of the queue into the specified
    /// `buffer`.  Block if there are no elements in the queue, up to the
    /// specified `timeout` *absolute* time.  Return 0 if an item was
    /// successfully retrieved, -1 on timeout, and non-zero value different
    /// from -1 if an error occurs.  The behavior is undefined unless this
    /// queue supports timed operations as determined by the optional
    /// `supportTimedOperations` flag specified at construction.
    int timedPopFront(ElementType* buffer, const bsls::TimeInterval& timeout);

    /// Disable queueing on the `MonitoredQueue`.  All subsequent calls
    /// to `pushBack` and `tryPushBack` will fail immediately.  All blocked
    /// invocations of `pushBack` will fail immediately.  If the queue is
    /// already disabled, this method has no effect.
    void disablePushBack();

    /// Enable queuing.  If the queue is not enqueue disabled, this call has
    /// no effect.
    void enablePushBack();

    /// Remove all items from this queue, and reset its state to an empty
    /// queue.  The behavior is undefined if this `MonitoredQueue` is
    /// concurrently being modified while calling `reset`.
    void reset();

    // ACCESSORS

    /// Returns the number of elements currently in this queue.
    bsls::Types::Int64 numElements() const;

    /// Return the maximum number of elements that may be stored in this
    /// queue.
    bsls::Types::Int64 capacity() const;

    /// Return `true` if this queue is empty and `false` otherwise.
    bool isEmpty() const;

    /// Return the low watermark set by the last call to `setWatermarks`.
    bsls::Types::Int64 lowWatermark() const;

    /// Return the high watermark set by the last call to `setWatermarks`.
    bsls::Types::Int64 highWatermark() const;

    /// Return the second high watermark set by the last call to
    /// `setWatermarks`.
    bsls::Types::Int64 highWatermark2() const;

    /// Return the state of the MonitoredQueue.
    MonitoredQueueState::Enum state() const;
};

// =========================
// struct MonitoredQueueUtil
// =========================

/// Utility functions for MonitoredQueues
struct MonitoredQueueUtil {
    // CLASS METHODS

    /// Print a description of the specified `state` of a queue with the
    /// specified `queueName` with the specified `lowWatermark`,
    /// `highWatermark`, `highWatermark2` and `queueSize` to the logger
    /// system.  The specified `warningString` is printed when 'state ==
    /// e_HIGH_WATERMARK_REACHED', `e_HIGH_WATERMARK_2_REACHED` or
    /// `state == e_QUEUE_FILLED`.
    static void stateLogCallback(const bsl::string& queueName,
                                 const bsl::string& warningString,
                                 bsls::Types::Int64 lowWatermark,
                                 bsls::Types::Int64 highWatermark,
                                 bsls::Types::Int64 highWatermark2,
                                 bsls::Types::Int64 queueSize,
                                 mwcc::MonitoredQueueState::Enum state);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------
// class MonitoredQueue
// --------------------

// PRIVATE MANIPULATORS
template <class QUEUE, class QUEUE_TRAITS>
inline bool MonitoredQueue<QUEUE, QUEUE_TRAITS>::setState(int newState)
{
    int oldState = d_state;
    while (oldState < newState) {
        if (oldState == d_state.testAndSwap(oldState, newState)) {
            return true;  // RETURN
        }

        oldState = d_state;
    }

    return false;
}

template <class QUEUE, class QUEUE_TRAITS>
inline void MonitoredQueue<QUEUE, QUEUE_TRAITS>::incrementLength()
{
    const bsls::Types::Int64 newLength = ++d_queueLength;

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            newLength >= d_highWatermark2 &&
            d_state < MonitoredQueueState::e_HIGH_WATERMARK_2_REACHED)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        if (setState(MonitoredQueueState::e_HIGH_WATERMARK_2_REACHED)) {
            if (d_stateChangedCb) {
                d_stateChangedCb(
                    MonitoredQueueState::e_HIGH_WATERMARK_2_REACHED);
            }
        }
    }
    else if (newLength >= d_highWatermark &&
             d_state < MonitoredQueueState::e_HIGH_WATERMARK_REACHED) {
        if (setState(MonitoredQueueState::e_HIGH_WATERMARK_REACHED)) {
            if (d_stateChangedCb) {
                d_stateChangedCb(
                    MonitoredQueueState::e_HIGH_WATERMARK_REACHED);
            }
        }
    }
}

template <class QUEUE, class QUEUE_TRAITS>
inline void MonitoredQueue<QUEUE, QUEUE_TRAITS>::decrementLength()
{
    const bsls::Types::Int64 newLength = --d_queueLength;

    if (d_state > MonitoredQueueState::e_NORMAL &&
        newLength <= d_lowWatermark) {
        // See if we should be the one to report
        int oldState = d_state;
        while (oldState > MonitoredQueueState::e_NORMAL) {
            if (oldState ==
                d_state.testAndSwap(oldState, MonitoredQueueState::e_NORMAL)) {
                if (d_stateChangedCb) {
                    d_stateChangedCb(MonitoredQueueState::e_NORMAL);
                }

                return;  // RETURN
            }

            oldState = d_state;
        }
    }
}

// CREATORS
template <class QUEUE, class QUEUE_TRAITS>
inline MonitoredQueue<QUEUE, QUEUE_TRAITS>::MonitoredQueue(
    typename Traits::InitialCapacityType initialCapacity,
    bslma::Allocator*                    basicAllocator)
: d_highWatermark2(bsl::numeric_limits<int>::max())
, d_highWatermark(bsl::numeric_limits<int>::max())
, d_lowWatermark(0)
, d_stateChangedCb(bsl::allocator_arg, basicAllocator)
, d_state(0)
, d_queue(initialCapacity, basicAllocator)
, d_queueLength(0)
, d_supportTimedOperations(false)
, d_timedOperationsCondition(bsls::SystemClockType::e_MONOTONIC)
{
    // NOTHING
}

template <class QUEUE, class QUEUE_TRAITS>
inline MonitoredQueue<QUEUE, QUEUE_TRAITS>::MonitoredQueue(
    typename Traits::InitialCapacityType initialCapacity,
    bool                                 supportTimedOperations,
    bslma::Allocator*                    basicAllocator)
: d_highWatermark2(bsl::numeric_limits<int>::max())
, d_highWatermark(bsl::numeric_limits<int>::max())
, d_lowWatermark(0)
, d_stateChangedCb(bsl::allocator_arg, basicAllocator)
, d_state(0)
, d_queue(initialCapacity, basicAllocator)
, d_queueLength(0)
, d_supportTimedOperations(supportTimedOperations)
, d_timedOperationsCondition(bsls::SystemClockType::e_MONOTONIC)
{
    // NOTHING
}

// MANIPULATORS
template <class QUEUE, class QUEUE_TRAITS>
inline MonitoredQueue<QUEUE, QUEUE_TRAITS>&
MonitoredQueue<QUEUE, QUEUE_TRAITS>::setWatermarks(
    bsls::Types::Int64 lowWatermark,
    bsls::Types::Int64 highWatermark,
    bsls::Types::Int64 highWatermark2)
{
    const bsls::Types::Int64 intMax =
        bsl::numeric_limits<bsls::Types::Int64>::max();
    BSLS_ASSERT(lowWatermark >= 0);
    BSLS_ASSERT(lowWatermark < highWatermark);
    BSLS_ASSERT(highWatermark == intMax || highWatermark < highWatermark2);
    BSLS_ASSERT(highWatermark == intMax || highWatermark <= capacity());
    BSLS_ASSERT(highWatermark2 == intMax || highWatermark2 <= capacity());

    d_lowWatermark   = lowWatermark;
    d_highWatermark  = highWatermark;
    d_highWatermark2 = highWatermark2;

    (void)intMax;  // prod-build compiler happiness

    return *this;
}

template <class QUEUE, class QUEUE_TRAITS>
inline MonitoredQueue<QUEUE, QUEUE_TRAITS>&
MonitoredQueue<QUEUE, QUEUE_TRAITS>::setStateCallback(const StateCallback& cb)
{
    d_stateChangedCb = cb;
    return *this;
}

template <class QUEUE, class QUEUE_TRAITS>
inline int
MonitoredQueue<QUEUE, QUEUE_TRAITS>::tryPushBack(const ElementType& object)
{
    if (d_queue.tryPushBack(object)) {
        // We've filled the queue.  Alarm
        if (!Traits::isPushBackDisabled(d_queue) &&
            setState(MonitoredQueueState::e_QUEUE_FILLED) &&
            d_stateChangedCb) {
            d_stateChangedCb(MonitoredQueueState::e_QUEUE_FILLED);
        }

        return -1;  // RETURN
    }

    incrementLength();

    if (d_supportTimedOperations) {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_timedOperationsMutex);
        d_timedOperationsCondition.signal();
    }

    return 0;
}

template <class QUEUE, class QUEUE_TRAITS>
inline int MonitoredQueue<QUEUE, QUEUE_TRAITS>::tryPushBack(
    bslmf::MovableRef<ElementType> object)
{
    if (d_queue.tryPushBack(bslmf::MovableRefUtil::move(object))) {
        // We've filled the queue.  Alarm
        if (!Traits::isPushBackDisabled(d_queue) &&
            setState(MonitoredQueueState::e_QUEUE_FILLED) &&
            d_stateChangedCb) {
            d_stateChangedCb(MonitoredQueueState::e_QUEUE_FILLED);
        }

        return -1;  // RETURN
    }

    incrementLength();

    if (d_supportTimedOperations) {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_timedOperationsMutex);
        d_timedOperationsCondition.signal();
    }

    return 0;
}

template <class QUEUE, class QUEUE_TRAITS>
inline int
MonitoredQueue<QUEUE, QUEUE_TRAITS>::pushBack(const ElementType& object)
{
    if (tryPushBack(object) != 0) {
        if (d_queue.pushBack(object) != 0) {
            return -1;  // RETURN
        }

        incrementLength();

        if (d_supportTimedOperations) {
            bslmt::LockGuard<bslmt::Mutex> guard(&d_timedOperationsMutex);
            d_timedOperationsCondition.signal();
        }
    }

    return 0;
}

template <class QUEUE, class QUEUE_TRAITS>
inline int MonitoredQueue<QUEUE, QUEUE_TRAITS>::pushBack(
    bslmf::MovableRef<ElementType> value)
{
    const int currentQueueLen = static_cast<int>(numElements());
    if (currentQueueLen + 1 >= capacity()) {
        // We've filled the queue.  Alarm
        if (!Traits::isPushBackDisabled(d_queue) &&
            setState(MonitoredQueueState::e_QUEUE_FILLED) &&
            d_stateChangedCb) {
            d_stateChangedCb(MonitoredQueueState::e_QUEUE_FILLED);
        }

        return -1;  // RETURN
    }

    if (d_queue.pushBack(bslmf::MovableRefUtil::move(value)) != 0) {
        return -1;  // RETURN
    }

    incrementLength();

    if (d_supportTimedOperations) {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_timedOperationsMutex);
        d_timedOperationsCondition.signal();
    }

    return 0;
}

template <class QUEUE, class QUEUE_TRAITS>
inline int
MonitoredQueue<QUEUE, QUEUE_TRAITS>::tryPopFront(ElementType* buffer)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(buffer);

    if (d_queue.tryPopFront(buffer) != 0) {
        // Failure to remove an element from front of queue.
        return -1;  // RETURN
    }

    // Successfully removed an element from front of queue.
    decrementLength();

    return 0;
}

template <class QUEUE, class QUEUE_TRAITS>
inline int MonitoredQueue<QUEUE, QUEUE_TRAITS>::popFront(ElementType* value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(value);

    if (Traits::popFront(&d_queue, value) != 0) {
        // Failure to remove an element from front of queue.
        return -1;  // RETURN
    }

    decrementLength();

    return 0;
}

template <class QUEUE, class QUEUE_TRAITS>
inline int MonitoredQueue<QUEUE, QUEUE_TRAITS>::timedPopFront(
    ElementType*              buffer,
    const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(buffer);
    BSLS_ASSERT(d_supportTimedOperations && "Timed operations not supported");

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS       = 0,
        rc_TIMEOUT       = -1,
        rc_UNKNOWN_ERROR = -2
    };

    if (d_queue.tryPopFront(buffer) == 0) {
        // Successfully removed an element from front of queue.
        decrementLength();
        return rc_SUCCESS;  // RETURN
    }

    // Attempt to remove an element from front of queue was unsuccessful.
    bslmt::LockGuard<bslmt::Mutex> guard(&d_timedOperationsMutex);
    int                            rc = 0;
    while ((rc = d_queue.tryPopFront(buffer)) != 0) {
        const int status = d_timedOperationsCondition.timedWait(
            &d_timedOperationsMutex,
            timeout);
        if (status == -1) {  // timed out
            break;           // BREAK
        }

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(status != 0)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            // Error returned from 'timedWait'.  Likely corruption either in
            // the application or glibc.
            return rc_UNKNOWN_ERROR;  // RETURN
        }
    }
    if (rc != 0) {
        // The wait timed out and 'predicate' returned 'false'.
        return rc_TIMEOUT;  // RETURN
    }

    // The condition variable was either signaled or timed out and 'predicate'
    // returned 'true'.
    decrementLength();

    return rc_SUCCESS;
}

template <class QUEUE, class QUEUE_TRAITS>
inline void MonitoredQueue<QUEUE, QUEUE_TRAITS>::disablePushBack()
{
    Traits::disablePushBack(&d_queue);
}

template <class QUEUE, class QUEUE_TRAITS>
inline void MonitoredQueue<QUEUE, QUEUE_TRAITS>::enablePushBack()
{
    Traits::enablePushBack(&d_queue);
}

template <class QUEUE, class QUEUE_TRAITS>
inline void MonitoredQueue<QUEUE, QUEUE_TRAITS>::reset()
{
    d_queue.removeAll();
    d_queueLength = 0;
    d_state       = 0;
}

// ACCESSORS
template <class QUEUE, class QUEUE_TRAITS>
inline bsls::Types::Int64
MonitoredQueue<QUEUE, QUEUE_TRAITS>::numElements() const
{
    return d_queueLength;
}

template <class QUEUE, class QUEUE_TRAITS>
inline bsls::Types::Int64 MonitoredQueue<QUEUE, QUEUE_TRAITS>::capacity() const
{
    return Traits::capacity(d_queue);
}

template <class QUEUE, class QUEUE_TRAITS>
inline bool MonitoredQueue<QUEUE, QUEUE_TRAITS>::isEmpty() const
{
    return d_queue.isEmpty();
}

template <class QUEUE, class QUEUE_TRAITS>
inline bsls::Types::Int64
MonitoredQueue<QUEUE, QUEUE_TRAITS>::lowWatermark() const
{
    return d_lowWatermark;
}

template <class QUEUE, class QUEUE_TRAITS>
inline bsls::Types::Int64
MonitoredQueue<QUEUE, QUEUE_TRAITS>::highWatermark() const
{
    return d_highWatermark;
}

template <class QUEUE, class QUEUE_TRAITS>
inline bsls::Types::Int64
MonitoredQueue<QUEUE, QUEUE_TRAITS>::highWatermark2() const
{
    return d_highWatermark2;
}

template <class QUEUE, class QUEUE_TRAITS>
inline MonitoredQueueState::Enum
MonitoredQueue<QUEUE, QUEUE_TRAITS>::state() const
{
    return static_cast<MonitoredQueueState::Enum>(d_state.load());
}

}  // Close package namespace

// -------------------------------
// struct MonitoredQueueState
// -------------------------------

// FREE OPERATORS
inline bsl::ostream& mwcc::operator<<(bsl::ostream&                   stream,
                                      mwcc::MonitoredQueueState::Enum value)
{
    return mwcc::MonitoredQueueState::print(stream, value, 0, -1);
}

}  // Close enterprise namespace

#endif
