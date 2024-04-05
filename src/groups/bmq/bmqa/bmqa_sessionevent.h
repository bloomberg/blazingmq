// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqa_sessionevent.h                                                -*-C++-*-
#ifndef INCLUDED_BMQA_SESSIONEVENT
#define INCLUDED_BMQA_SESSIONEVENT

/// @file bmqa_sessionevent.h
///
/// @brief Provide value-semantic type for system event session notifications.
///
/// @see @bbref{bmqt::SessionEventType}: Enum of the various type of
/// notifications
///
/// This component provides a generic @bbref{bmqa::SessionEvent} notification
/// object used by the @bbref{bmqa::Session} to provide BlazingMQ applications
/// with information regarding changes in the session status or the result of
/// operations with the message queue broker.
///
/// A @bbref{bmqa::SessionEvent} is composed of 4 attributes:
///
///   - **type**: indicate the type of the notification
///
///   - **statusCode**: indicate the status of the operation (success, failure)
///
///   - **correlationId**: optional correlationId used during async response
///
///   - **queueId**: optional queueId associated with the event, of type
///     `OPEN`, `CONFIGURE`, `CLOSE`, `REOPEN`
///
///   - **errorDescription**: optional string with a human readable description
///     of the error, if any
///
/// Note that @bbref{bmqa::SessionEvent} is implemented using the pimpl idiom,
/// so copying a @bbref{bmqa::SessionEvent} is very cheap (a pointer copy).
/// All copies of this @bbref{bmqa::SessionEvent} will share the same
/// underlying implementation.

// BMQ

#include <bmqa_queueid.h>
#include <bmqt_correlationid.h>
#include <bmqt_sessioneventtype.h>

// BDE
#include <bsl_memory.h>
#include <bsl_string.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqimp {
class Event;
}

namespace bmqa {

// ==================
// class SessionEvent
// ==================

/// An event related to the operation of a `Session`.
class SessionEvent {
  private:
    // FRIENDS
    friend bool operator==(const SessionEvent& lhs, const SessionEvent& rhs);
    friend bool operator!=(const SessionEvent& lhs, const SessionEvent& rhs);

  private:
    // DATA
    bsl::shared_ptr<bmqimp::Event> d_impl_sp;  // pimpl

  public:
    // CREATORS

    /// Default constructor
    SessionEvent();

    /// Create a new `SessionEvent` having the same values (pointing to the
    /// same pimpl) as the specified `other`.
    SessionEvent(const SessionEvent& other);

    // MANIPULATORS

    /// Assign to this `SessionEvent` the same values as the one from the
    /// specified `rhs` (i.e., reference the same pimpl).
    SessionEvent& operator=(const SessionEvent& rhs);

    // ACCESSORS

    /// Return the session event type.
    bmqt::SessionEventType::Enum type() const;

    /// Return the correlationId associated to this event, if any.
    const bmqt::CorrelationId& correlationId() const;

    /// Return the queueId associated to this event, if any.  The behavior
    /// is undefined unless this event corresponds to a queue related event
    /// (i.e. `OPEN`, `CONFIGURE`, `CLOSE`, `REOPEN`).
    QueueId queueId() const;

    /// Return the status code that indicates success or the cause of a
    /// failure.
    int statusCode() const;

    /// Return a printable description of the error, if `statusCode` returns
    /// non-zero.  Return an empty string otherwise.
    const bsl::string& errorDescription() const;

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

// FREE OPERATORS

/// Return `true` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return false otherwise.
bool operator==(const SessionEvent& lhs, const SessionEvent& rhs);

/// Return `false` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return `true` otherwise.
bool operator!=(const SessionEvent& lhs, const SessionEvent& rhs);

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const SessionEvent& rhs);

}  // close package namespace

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------
// class SessionEvent
// ------------------

inline bsl::ostream& bmqa::operator<<(bsl::ostream&             stream,
                                      const bmqa::SessionEvent& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
