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

// bmqa_event.h                                                       -*-C++-*-
#ifndef INCLUDED_BMQA_EVENT
#define INCLUDED_BMQA_EVENT

//@PURPOSE: Provide a generic variant encompassing all types of events.
//
//@CLASSES:
//  bmqa::Event: variant encompassing all types of events.
//
//@SEE_ALSO:
//  bmqa::MessageEvent : Data event notification.
//  bmqa::SessionEvent : Session and queue status event notification.
//
//@DESCRIPTION: This component provides a generic 'bmqa::Event' notification
// object used by the 'bmqa::Session' to provide BlazingMQ applications with
// information regarding the status of the session or data coming from queues.
// This 'bmqa::Event' object is only used when user wants to process messages
// from the EventQueue using its own thread, by calling the
// bmqa::Session::nextEvent method.
//
// A 'bmqa::Event' can either be a 'bmqa::SessionEvent' or a
// 'bmqa::MessageEvent'.  The former describes notifications such as the result
// of a session start or the opening of a queue.  The latter carries
// application messages retrieved from a queue.
//
// Note that event is implemented using the pimpl idiom, so copying an event is
// very cheap (a pointer copy).  All copies of this 'bmqa::Event', as well as
// any 'SessionEvent' or 'MessageEvent' extracted from it will share the same
// underlying implementation.

// BMQ

#include <bmqa_messageevent.h>
#include <bmqa_sessionevent.h>

// BDE
#include <bsl_iosfwd.h>
#include <bsl_memory.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqimp {
class Event;
}

namespace bmqa {

// ===========
// class Event
// ===========

/// A variant type encompassing all types of events
class Event {
  private:
    // DATA
    bsl::shared_ptr<bmqimp::Event> d_impl_sp;  // pimpl

  public:
    // CREATORS

    /// Default constructor
    Event();

    // ACCESSORS

    /// Return the `SessionEvent` variant.  The behavior is undefined unless
    /// `isSessionEvent` returns true.
    SessionEvent sessionEvent() const;

    /// Return the `MessageEvent` variant.  The behavior is undefined unless
    /// `isMessageEvent` returns true.
    MessageEvent messageEvent() const;

    /// Return true if the event is a session event.
    bool isSessionEvent() const;

    /// Return true if the event is a message event.
    bool isMessageEvent() const;

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

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const Event& rhs);

}  // close package namespace

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------
// class Event
// -----------

inline bsl::ostream& bmqa::operator<<(bsl::ostream&      stream,
                                      const bmqa::Event& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
