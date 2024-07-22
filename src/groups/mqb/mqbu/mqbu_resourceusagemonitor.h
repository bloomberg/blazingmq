// Copyright 2016-2023 Bloomberg Finance L.P.
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

// mqbu_resourceusagemonitor.h                                        -*-C++-*-
#ifndef INCLUDED_MQBU_RESOURCEUSAGEMONITOR
#define INCLUDED_MQBU_RESOURCEUSAGEMONITOR

//@PURPOSE: Provide a simple two-resource usage monitor (low/high watermark).
//
//@CLASSES:
//  mqbu::ResourceUsageMonitorState:           Monitor state enum
//  mqbu::ResourceUsageMonitorStateTransition: State transition enum
//  mqbu::ResourceUsageMonitor:                Two-resource usage monitor
//
//
//@DESCRIPTION:
// This component provides a simple mechanism, 'mqbu::ResourceUsageMonitor' to
// control the usage of two resources (messages and bytes) by emitting low
// watermark, high watermark or full events (with the
// 'mqbu::ResourceUsageMonitorStateTransition') once only when a threshold has
// been crossed; the state of the mechanism is represented with the
// 'mqbu::ResourceUsageMonitorState' enum.
//
// The following parameters are used to configure this component (only bytes
// are mentioned but these apply to bytes and messages independently):
//
//: o !byteLowWatermarkRatio!: when the monitor is in 'FULL' or
//:   'HIGH_WATERMARK' state with respect to bytes, and its bytes ratio becomes
//:   less than or equal to the 'byteLowWatermarkRatio' parameter, it emits
//:   'LOW_WATERMARK' and its state goes back to 'NORMAL'.
//
//: o !byteHighWatermarkRatio!: when the monitor is in 'NORMAL' state with
//:   respect to bytes, and its bytes ratio becomes greater than or equal to
//:   the 'byteHighWatermarkRatio' parameter but is strictly less than 1.0,
//:   the monitor emits 'HIGH_WATERMARK' and enters the 'HIGH_WATERMARK' state.
//:   It will go back to 'NORMAL' once the ratio becomes less than or equal to
//:   the 'byteLowWatermarkRatio' parameter.
//
//: o !byteCapacity!: the maximum value that the resource can represent. When
//:   the monitor's bytes value becomes greater than or equal to the
//:   'byteCapacity' parameter, the monitor emits 'FULL' and enters the 'FULL'
//:   state with respect to bytes.
//
/// Thread Safety
///-------------
// NOT Thread-Safe.
//
/// USAGE
///-----
// This section illustrates intended use of this component.
//
// First, let's initialize the monitor.
//..
//  const double             k_BYTE_LOW_WATERMARK_RATIO     = 0.6;
//  const double             k_BYTE_HIGH_WATERMARK_RATIO    = 0.8;
//  const bsls::Types::Int64 k_BYTE_CAPACITY                = 100;
//  const double             k_MESSAGE_LOW_WATERMARK_RATIO  = 0.5;
//  const double             k_MESSAGE_HIGH_WATERMARK_RATIO = 0.9;
//  const bsls::Types::Int64 k_MESSAGE_CAPACITY             = 10;
//
//  // Create a mqbu::ResourceUsageMonitor object
//  mqbu::ResourceUsageMonitor monitor(k_BYTE_LOW_WATERMARK_RATIO,
//                                     k_BYTE_HIGH_WATERMARK_RATIO,
//                                     k_BYTE_CAPACITY,
//                                     k_MESSAGE_LOW_WATERMARK_RATIO,
//                                     k_MESSAGE_HIGH_WATERMARK_RATIO,
//                                     k_MESSAGE_CAPACITY);
//
//..
// Then, we check to see if we've hit our high watermark or capacity before
// sending messages (i.e. we're no longer in 'NORMAL' state).
//..
//  // TYPE
//  typedef mqbu::ResourceUsageMonitorState RUMState;
//
//  // Check resource usage state
//  bool highWatermarkLimitReached =   monitor.state()
//                                  != RUMState::e_STATE_NORMAL;
//  if (!highWatermarkLimitReached) {
//      // Send message(s)
//
//      // Update resource usage
//      monitor.update(msgSize, 1);
//  }
//  else {
//      // Save it to send later
//  }
//..
// After hitting high watermark or capacity, we decrease our resource usage to
// a point that takes us below our low watermark (and back to the 'NORMAL'
// state), so we check for that and resume sending messages if that's the case.
//..
//  // TYPE
//  typedef mqbu::ResourceUsageMonitorStateTransition RUMStateTransition;
//
//  // Update resource usage state (e.g. confirmed message from client)
//  RUMStateTransition::Enum change = monitor.update(-msgSize, -1);
//
//  // Check if we're back to below our low watermark
//  if (change == RUMStateTransition::e_LOW_WATERMARK) {
//       // Resume sending messages
//  }
//..

// MQB

// BDE
#include <bsl_algorithm.h>
#include <bsl_iosfwd.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbu {

// ================================
// struct ResourceUsageMonitorState
// ================================

/// This struct defines the type of state that a ResourceUsageMonitor can be
/// in with regards to a resource.
struct ResourceUsageMonitorState {
    // TYPES
    enum Enum {
        e_STATE_NORMAL = 0  // Monitor is in normal state
        ,
        e_STATE_HIGH_WATERMARK = 1  // Monitor is in high watermark state
        ,
        e_STATE_FULL = 2  // Monitor is full
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
    /// the initial indentation (as governed by `level`) value.  See
    /// `toAscii` for what constitutes the string representation of a
    /// `ResourceUsageMonitorState::Enum` value.
    static bsl::ostream& print(bsl::ostream&                   stream,
                               ResourceUsageMonitorState::Enum value,
                               int                             level = 0,
                               int spacesPerLevel                    = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(ResourceUsageMonitorState::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                   stream,
                         ResourceUsageMonitorState::Enum value);

// ==========================================
// struct ResourceUsageMonitorStateTransition
// ==========================================

/// This struct defines the type of state transition that a
/// ResourceUsageMonitor may undergo.
struct ResourceUsageMonitorStateTransition {
    // TYPES
    enum Enum {
        e_NO_CHANGE  // The state of the monitor hasn't changed
        ,
        e_LOW_WATERMARK  // The value is back to low watermark
        ,
        e_HIGH_WATERMARK  // The value has reached the high watermark
        ,
        e_FULL  // The monitored resource is full
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
    /// the initial indentation (as governed by `level`) value.  See
    /// `toAscii` for what constitutes the string representation of a
    /// `ResourceUsageMonitorStateTransition::Enum` value.
    static bsl::ostream& print(bsl::ostream& stream,
                               ResourceUsageMonitorStateTransition::Enum value,
                               int level          = 0,
                               int spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char*
    toAscii(ResourceUsageMonitorStateTransition::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                             stream,
                         ResourceUsageMonitorStateTransition::Enum value);

// ==========================
// class ResourceUsageMonitor
// ==========================

/// Mechanism to monitor two resources (bytes and messages).
class ResourceUsageMonitor {
  private:
    // PRIVATE STRUCT

    /// This struct provides a VST which captures various attributes
    /// associated with a resource in the monitor.
    struct ResourceAttributes {
      private:
        // DATA
        bsls::Types::Int64 d_capacity;
        // maximum resource capacity
        double d_lowWatermarkRatio;
        // low watermark threshold ratio
        double d_highWatermarkRatio;
        // high watermark threshold ratio
        bsls::Types::Int64 d_value;
        // current resource usage value
        ResourceUsageMonitorState::Enum d_state;
        // current state of the monitor

      public:
        // CREATORS
        ResourceAttributes(bsls::Types::Int64              capacity,
                           double                          lowWatermarkRatio,
                           double                          highWatermarkRatio,
                           bsls::Types::Int64              value,
                           ResourceUsageMonitorState::Enum state);

        // MANIPULATORS
        ResourceAttributes& setCapacity(bsls::Types::Int64 value);
        ResourceAttributes& setLowWatermarkRatio(double value);
        ResourceAttributes& setHighWatermarkRatio(double value);
        ResourceAttributes& setValue(bsls::Types::Int64 value);

        /// Set the corresponding attribute to the specified `value` and
        /// return a reference offering modifiable access to this object.
        ResourceAttributes& setState(ResourceUsageMonitorState::Enum value);

        /// Set the low watermark ratio and high watermark ratio to the
        /// specified `low` and `high` respectively, and return a reference
        /// offering modifiable access to this object.
        ResourceAttributes& setWatermarkRatios(double low, double high);

        // ACCESSORS
        bsls::Types::Int64 capacity() const;
        double             lowWatermarkRatio() const;
        double             highWatermarkRatio() const;
        bsls::Types::Int64 value() const;

        /// Get the value of the corresponding attribute.
        ResourceUsageMonitorState::Enum state() const;
    };

  private:
    // DATA
    ResourceAttributes d_bytes;     // resource attributes for bytes
    ResourceAttributes d_messages;  // resource attributes for messages

  private:
    // PRIVATE MANIPULATORS

    /// Update the value and state stored in the specified `attributes`, to
    /// account for a change of the specified `delta` in value and any
    /// associated change in state using the capacity and thresholds in
    /// `attributes` as the limit configuration parameters.  Return the
    /// change in state caused by this update.
    ResourceUsageMonitorStateTransition::Enum
    updateValueInternal(ResourceAttributes* attributes,
                        bsls::Types::Int64  delta);

  public:
    // CREATORS

    /// Create a `mqbu::ResourceUsageMonitor` object having the specified
    /// `byteCapacity` and `messageCapacity`.  The low and high watermark
    /// ratios for bytes and messages will be set to the default values.
    /// The values for bytes and messages being monitored are set to 0.
    ResourceUsageMonitor(bsls::Types::Int64 byteCapacity,
                         bsls::Types::Int64 messageCapacity);

    /// Create a `mqbu::ResourceUsageMonitor` object having the specified
    /// `byteCapacity`, `messageCapacity`, `byteLowWatermarkRatio`,
    /// `byteHighWatermarkRatio`, `messageLowWatermarkRatio`, and
    /// `messageHighWatermarkRatio`.  The values for bytes and messages
    /// being monitored are set to 0.  Behavior is undefined unless
    /// `0 <= lowWatermarkRatio <= highWatermarkRatio <= 1.0`.
    ResourceUsageMonitor(bsls::Types::Int64 byteCapacity,
                         bsls::Types::Int64 messageCapacity,
                         double             byteLowWatermarkRatio,
                         double             byteHighWatermarkRatio,
                         double             messageLowWatermarkRatio,
                         double             messageHighWatermarkRatio);

    // MANIPULATORS

    /// Reset this object.  The values for bytes and messages being
    /// monitored are set to 0 while the limits and watermark thresholds set
    /// at construction remain unchanged.
    void reset();

    /// Reset this object using the specified `byteCapacity`,
    /// `messageCapacity`, `byteLowWatermark`, `byteHighWatermark`,
    /// `messageLowWatermark`, and `messageHighWatermark`.  The values for
    /// bytes and messages being monitored are set to 0.  Behavior is
    /// undefined unless `0 <= lowWatermark <= highWatermark <= capacity`.
    void reset(bsls::Types::Int64 byteCapacity,
               bsls::Types::Int64 messageCapacity,
               bsls::Types::Int64 byteLowWatermark,
               bsls::Types::Int64 byteHighWatermark,
               bsls::Types::Int64 messageLowWatermark,
               bsls::Types::Int64 messageHighWatermark);

    /// Reset this object using the specified `byteCapacity`,
    /// `messageCapacity`, `byteLowWatermarkRatio`,
    /// `byteHighWatermarkRatio`, `messageLowWatermarkRatio`, and
    /// `messageHighWatermarkRatio`.  The values for bytes and messages
    /// being monitored are set to 0.  Behavior is undefined unless
    /// `0 <= lowWatermarkRatio <= highWatermarkRatio <= 1.0`.
    void resetByRatio(bsls::Types::Int64 byteCapacity,
                      bsls::Types::Int64 messageCapacity,
                      double             byteLowWatermarkRatio,
                      double             byteHighWatermarkRatio,
                      double             messageLowWatermarkRatio,
                      double             messageHighWatermarkRatio);

    /// Reconfigure this object to have the specified `byteCapacity`,
    /// `messageCapacity`, `byteLowWatermark`, `byteHighWatermark`,
    /// `messageLowWatermark`, and `messageHighWatermark`.  The values for
    /// bytes and messages being monitored remain unchanged.  Behavior is
    /// undefined unless `0 <= lowWatermark <= highWatermark <= capacity`.
    /// Note that the resulting state of this object is the same as that of
    /// creating a new `mqbu::ResourceUsageMonitor` object and calling
    /// `update()` on it with this object's bytes and messages.
    void reconfigure(bsls::Types::Int64 byteCapacity,
                     bsls::Types::Int64 messageCapacity,
                     bsls::Types::Int64 byteLowWatermark,
                     bsls::Types::Int64 byteHighWatermark,
                     bsls::Types::Int64 messageLowWatermark,
                     bsls::Types::Int64 messageHighWatermark);

    /// Reconfigure this object to have the specified `byteCapacity`,
    /// `messageCapacity`, `byteLowWatermarkRatio`,
    /// `byteHighWatermarkRatio`, `messageLowWatermarkRatio`, and
    /// `messageHighWatermarkRatio`.  The values for bytes and messages
    /// being monitored remain unchanged.  Behavior is undefined unless
    /// `0 <= lowWatermarkRatio <= highWatermarkRatio <= 1.0`.  Note that
    /// the resulting state of this object is the same as that of creating
    /// a new `mqbu::ResourceUsageMonitor` object and calling `update()` on
    /// it with this object's bytes and messages.
    void reconfigureByRatio(bsls::Types::Int64 byteCapacity,
                            bsls::Types::Int64 messageCapacity,
                            double             byteLowWatermarkRatio,
                            double             byteHighWatermarkRatio,
                            double             messageLowWatermarkRatio,
                            double             messageHighWatermarkRatio);

    /// Inform the monitor of a change of the specified `bytesDelta` and
    /// `messagesDelta` in the values of bytes and messages being monitored,
    /// respectively.  Return the change in state of the monitor with
    /// respect to the highest limit reached for either bytes or messages.
    ResourceUsageMonitorStateTransition::Enum
    update(bsls::Types::Int64 bytesDelta, bsls::Types::Int64 messagesDelta);

    /// Inform the monitor of a change of the specified `delta` in the value
    /// of bytes being monitored, and return the change in state of the
    /// monitor with respect to bytes, if any.
    ResourceUsageMonitorStateTransition::Enum
    updateBytes(bsls::Types::Int64 delta);

    /// Inform the monitor of a change of the specified `delta` in the value
    /// of messages being monitored, and return the change in state of the
    /// monitor with respect to messages, if any.
    ResourceUsageMonitorStateTransition::Enum
    updateMessages(bsls::Types::Int64 delta);

    // ACCESSORS

    /// Return the current value of bytes being monitored by this instance.
    bsls::Types::Int64 bytes() const;

    /// Return the bytes capacity parameter.
    bsls::Types::Int64 byteCapacity() const;

    /// Return the bytes low watermark threshold ratio.
    double byteLowWatermarkRatio() const;

    /// Return the bytes high watermark threshold ratio.
    double byteHighWatermarkRatio() const;

    /// Return the current state of this monitor with respect to bytes.
    ResourceUsageMonitorState::Enum byteState() const;

    /// Return the current value of messages being monitored by this
    /// instance.
    bsls::Types::Int64 messages() const;

    /// Return the messages capacity parameter.
    bsls::Types::Int64 messageCapacity() const;

    /// Return the messages low watermark threshold ratio.
    double messageLowWatermarkRatio() const;

    /// Return the messages high watermark threshold ratio.
    double messageHighWatermarkRatio() const;

    /// Return the current state of this monitor with respect to messages.
    ResourceUsageMonitorState::Enum messageState() const;

    /// Return the current state of this monitor.  Note that the current
    /// state of this monitor is the highest limit reached for either bytes
    /// or messages.
    ResourceUsageMonitorState::Enum state() const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// ====================
// struct SingleCounter
// ====================

struct SingleCounter {
    /// Thread-safe int value hierarchical tracker
    /// Relies on an owner for creation/destruction

    SingleCounter* d_parent_p;
    /// If not null, gets updated

    bsls::AtomicInt64 d_value;
    /// The value being tracked

    // CREATORS
    explicit SingleCounter(SingleCounter* parent);

    /// Add the specified `delta` to the value.
    void update(int delta);

    /// Return the value being tracked.
    int value() const;
};

// FREE OPERATORS
bsl::ostream& operator<<(bsl::ostream&               stream,
                         const ResourceUsageMonitor& value);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------------
// class ResourceAttributes
// ------------------------

// CREATORS
inline ResourceUsageMonitor::ResourceAttributes::ResourceAttributes(
    bsls::Types::Int64              capacity,
    double                          lowWatermarkRatio,
    double                          highWatermarkRatio,
    bsls::Types::Int64              value,
    ResourceUsageMonitorState::Enum state)
: d_capacity(capacity)
, d_lowWatermarkRatio(lowWatermarkRatio)
, d_highWatermarkRatio(highWatermarkRatio)
, d_value(value)
, d_state(state)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0.0 <= d_lowWatermarkRatio &&
                     d_lowWatermarkRatio <= d_highWatermarkRatio &&
                     d_highWatermarkRatio <= 1.0);
}

// MANIPULATORS
inline ResourceUsageMonitor::ResourceAttributes&
ResourceUsageMonitor::ResourceAttributes::setCapacity(bsls::Types::Int64 value)
{
    d_capacity = value;
    return *this;
}

inline ResourceUsageMonitor::ResourceAttributes&
ResourceUsageMonitor::ResourceAttributes::setLowWatermarkRatio(double value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0.0 <= value && value <= d_highWatermarkRatio);

    d_lowWatermarkRatio = value;
    return *this;
}

inline ResourceUsageMonitor::ResourceAttributes&
ResourceUsageMonitor::ResourceAttributes::setHighWatermarkRatio(double value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_lowWatermarkRatio <= value && value <= 1.0);

    d_highWatermarkRatio = value;
    return *this;
}

inline ResourceUsageMonitor::ResourceAttributes&
ResourceUsageMonitor::ResourceAttributes::setValue(bsls::Types::Int64 value)
{
    d_value = value;
    return *this;
}

inline ResourceUsageMonitor::ResourceAttributes&
ResourceUsageMonitor::ResourceAttributes::setState(
    ResourceUsageMonitorState::Enum value)
{
    d_state = value;
    return *this;
}

inline ResourceUsageMonitor::ResourceAttributes&
ResourceUsageMonitor::ResourceAttributes::setWatermarkRatios(double low,
                                                             double high)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0.0 <= low && low <= high && high <= 1.0);

    d_lowWatermarkRatio  = low;
    d_highWatermarkRatio = high;
    return *this;
}

// ACCESSORS
inline bsls::Types::Int64
ResourceUsageMonitor::ResourceAttributes::capacity() const
{
    return d_capacity;
}

inline double
ResourceUsageMonitor::ResourceAttributes::lowWatermarkRatio() const
{
    return d_lowWatermarkRatio;
}

inline double
ResourceUsageMonitor::ResourceAttributes::highWatermarkRatio() const
{
    return d_highWatermarkRatio;
}

inline bsls::Types::Int64
ResourceUsageMonitor::ResourceAttributes::value() const
{
    return d_value;
}

inline ResourceUsageMonitorState::Enum
ResourceUsageMonitor::ResourceAttributes::state() const
{
    return d_state;
}

// --------------------------
// class ResourceUsageMonitor
// --------------------------

// MANIPULATORS
inline ResourceUsageMonitorStateTransition::Enum
ResourceUsageMonitor::updateBytes(bsls::Types::Int64 delta)
{
    return updateValueInternal(&d_bytes, delta);
}

inline ResourceUsageMonitorStateTransition::Enum
ResourceUsageMonitor::updateMessages(bsls::Types::Int64 delta)
{
    return updateValueInternal(&d_messages, delta);
}

// ACCESSORS
inline bsls::Types::Int64 ResourceUsageMonitor::bytes() const
{
    return d_bytes.value();
}

inline bsls::Types::Int64 ResourceUsageMonitor::byteCapacity() const
{
    return d_bytes.capacity();
}

inline double ResourceUsageMonitor::byteLowWatermarkRatio() const
{
    return d_bytes.lowWatermarkRatio();
}

inline double ResourceUsageMonitor::byteHighWatermarkRatio() const
{
    return d_bytes.highWatermarkRatio();
}

inline ResourceUsageMonitorState::Enum ResourceUsageMonitor::byteState() const
{
    return d_bytes.state();
}

inline bsls::Types::Int64 ResourceUsageMonitor::messages() const
{
    return d_messages.value();
}

inline bsls::Types::Int64 ResourceUsageMonitor::messageCapacity() const
{
    return d_messages.capacity();
}

inline double ResourceUsageMonitor::messageLowWatermarkRatio() const
{
    return d_messages.lowWatermarkRatio();
}

inline double ResourceUsageMonitor::messageHighWatermarkRatio() const
{
    return d_messages.highWatermarkRatio();
}

inline ResourceUsageMonitorState::Enum
ResourceUsageMonitor::messageState() const
{
    return d_messages.state();
}

inline ResourceUsageMonitorState::Enum ResourceUsageMonitor::state() const
{
    // NOTE: Below assumes that the ResourceUsageMonitorState Enum is in
    //       increasing order of limit reached
    return bsl::max(d_bytes.state(), d_messages.state());
}

// --------------------
// struct SingleCounter
// --------------------

inline SingleCounter::SingleCounter(SingleCounter* parent)
: d_parent_p(parent)
, d_value(0)
{
    // NOTHING
}

inline void SingleCounter::update(int delta)
{
    d_value += delta;

    BSLS_ASSERT_SAFE(d_value >= 0);

    if (d_parent_p) {
        d_parent_p->update(delta);
    }
}

inline int SingleCounter::value() const
{
    return d_value;
}

}  // close package namespace

// --------------------------
// class ResourceUsageMonitor
// --------------------------

// FREE OPERATORS
inline bsl::ostream& mqbu::operator<<(bsl::ostream&                     stream,
                                      const mqbu::ResourceUsageMonitor& value)
{
    return value.print(stream, 0, -1);
}

// --------------------------------
// struct ResourceUsageMonitorState
// --------------------------------

// FREE OPERATORS
inline bsl::ostream&
mqbu::operator<<(bsl::ostream&                         stream,
                 mqbu::ResourceUsageMonitorState::Enum value)
{
    return mqbu::ResourceUsageMonitorState::print(stream, value, 0, -1);
}

// ------------------------------------------
// struct ResourceUsageMonitorStateTransition
// ------------------------------------------

// FREE OPERATORS
inline bsl::ostream&
mqbu::operator<<(bsl::ostream&                                   stream,
                 mqbu::ResourceUsageMonitorStateTransition::Enum value)
{
    return mqbu::ResourceUsageMonitorStateTransition::print(stream,
                                                            value,
                                                            0,
                                                            -1);
}

}  // close enterprise namespace

#endif
