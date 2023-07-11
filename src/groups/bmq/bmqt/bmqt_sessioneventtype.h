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

// bmqt_sessioneventtype.h                                            -*-C++-*-
#ifndef INCLUDED_BMQT_SESSIONEVENTTYPE
#define INCLUDED_BMQT_SESSIONEVENTTYPE

//@PURPOSE: Provide an enumeration for the different types of session events.
//
//@CLASSES:
//  bmqt::SessionEventType: Enumeration for the types of session events.
//
//@DESCRIPTION: 'bmqt::SessionEventType' is an enumeration for the different
// types of session events.
//
//: o !CONNECTED!: The connection with the broker is established, this event is
//:   only fired once (if for any reason the connection gets lost and
//:   automatically reconnected, this will emit a 'CONNECTION_RESTORED' event).
//
//: o !DISCONNECTED!: The connection with the broker is terminated (after the
//:   user called stop on the session). This is the last event that will be
//:   delivered.
//
//: o !CONNECTION_LOST!: The session was connected to the broker, but that
//:   connection dropped.
//
//: o !RECONNECTED!: The connection to the broker has been re-established.
//:   This event is followed by zero or more 'QUEUE_REOPEN_RESULT' events, and
//:   one 'STATE_RESTORED' event.
//
//: o !STATE_RESTORED!: Session's state has been restored after connection
//:   has been re-established with the broker. This event is preceded by one
//:   'RECONNECTED' event, and zero or more 'QUEUE_REOPEN_RESULT' events.
//
//: o !CONNECTION_TIMEOUT!: The connection to the broker has timedout.
//
//: o !QUEUE_OPEN_RESULT!: Indicates the result of an 'openQueue' operation.
//
//: o !QUEUE_REOPEN_RESULT!: Indicates the result of an 'openQueue' operation,
//:   which happens when the connection to the broker has been restored and
//:   queues previously opened have been automatically reopened.  This event is
//:   preceded by a 'RECONNECTED' event and followed by zero or more
//:   'QUEUE_REOPEN_RESULT' events, and one 'STATE_RESTORED' event.
//
//: o !QUEUE_CLOSE_RESULT!: Indicates the result of a 'closeQueue' operation
//
//: o !SLOWCONSUMER_NORMAL!: Notifies that the EventQueue is back to its low
//:   watermark level.
//
//: o !SLOWCONSUMER_HIGHWATERMARK!: Notifies that events are accumulating in
//:   the EventQueue, which has now reached it's high watermark level.
//
//: o !QUEUE_CONFIGURE_RESULT!: Indicates the result of a 'configureQueue'
//:   operation.
//
//: o !HOST_UNHEALTHY!: Indicates the host has become unhealthy. Only issued if
//    a 'bmqpi::HostHealthMonitor' has been installed to the session, via the
//    'bmqt::SessionOptions' object.
//
//: o !HOST_HEALTH_RESTORED!: Indicates the health of the host has restored,
//    and all queues have resumed operation. Following a Host Health incident,
//    this indicates that the application has resumed normal operation.
//
//: o !QUEUE_SUSPENDED!: Indicates that an open queue has suspended operation:
//    * Consumers are configured to receive no more messages from broker
//      (i.e., maxUnconfirmedMessages := 0, maxUnconfirmedBytes := 0,
//      consumerPriority := INT_MIN).
//    * Attempts to pack messages targeting queue will be rejected by
//      'bmqa::MessageEventBuilder::pack()'. Clients may still attempt to POST
//      existing packed events.
//    * Attempts by client to reconfigure queue will not take effect until the
//      queue has resumed.
//
//: o !QUEUE_RESUMED!: Indicates that a suspended queue has resumed normal
//    operation (i.e., effects of QUEUE_SUSPENDED state no longer apply).
//
//: o !ERROR!: Indicates a generic error.
//
//: o !TIMEOUT! Indicates that the specified operation has timed out.
//
//: o !CANCELED!: Indicates that the specified operation was canceled.
//

// BMQ

// BDE
#include <bsl_ostream.h>
#include <bsl_string.h>

namespace BloombergLP {
namespace bmqt {

// =======================
// struct SessionEventType
// =======================

/// Enumeration for the types of session events
struct SessionEventType {
    // TYPES
    enum Enum {
        e_ERROR = -1  // Generic error
        ,
        e_TIMEOUT = -2  // Time out of the operation
        ,
        e_CANCELED = -3  // The operation was canceled
        ,
        e_UNDEFINED = 0,
        e_CONNECTED = 1  // Session started
        ,
        e_DISCONNECTED = 2  // Session terminated
        ,
        e_CONNECTION_LOST = 3  // Lost connection to the broker
        ,
        e_RECONNECTED = 4  // Reconnected with the broker
        ,
        e_STATE_RESTORED = 5  // Client's state has been restored
        ,
        e_CONNECTION_TIMEOUT = 6  // The connection to broker timedOut
        ,
        e_QUEUE_OPEN_RESULT = 7  // Result of openQueue operation
        ,
        e_QUEUE_REOPEN_RESULT = 8  // Result of re-openQueue operation
        ,
        e_QUEUE_CLOSE_RESULT = 9  // Result of closeQueue operation
        ,
        e_SLOWCONSUMER_NORMAL = 10  // EventQueue is at lowWatermark
        ,
        e_SLOWCONSUMER_HIGHWATERMARK = 11  // EventQueue is at highWatermark
        ,
        e_QUEUE_CONFIGURE_RESULT = 12  // Result of configureQueue
        ,
        e_HOST_UNHEALTHY = 13  // Host has become unhealthy
        ,
        e_HOST_HEALTH_RESTORED = 14  // Host's health has been restored
        ,
        e_QUEUE_SUSPENDED = 15  // Queue has suspended operation
        ,
        e_QUEUE_RESUMED = 16  // Queue has resumed operation
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
    /// `SessionEventType::Value` value.
    static bsl::ostream& print(bsl::ostream&          stream,
                               SessionEventType::Enum value,
                               int                    level          = 0,
                               int                    spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `BMQT_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(SessionEventType::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(SessionEventType::Enum*  out,
                          const bslstl::StringRef& str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, SessionEventType::Enum value);

}  // close package namespace

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------
// struct SessionEventType
// -----------------------

// FREE OPERATORS
inline bsl::ostream& bmqt::operator<<(bsl::ostream&                stream,
                                      bmqt::SessionEventType::Enum value)
{
    return SessionEventType::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
