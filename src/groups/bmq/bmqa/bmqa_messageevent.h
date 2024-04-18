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

// bmqa_messageevent.h                                                -*-C++-*-
#ifndef INCLUDED_BMQA_MESSAGEEVENT
#define INCLUDED_BMQA_MESSAGEEVENT

/// @file bmqa_messageevent.h
///
/// @brief Provide the application with data event notifications.
///
/// This component provides a @bbref{bmqa::MessageEvent} notification object
/// used by the @bbref{bmqa::Session} to provide BlazingMQ applications with
/// data messages received from the broker.  The application can consume the
/// messages by asking this @bbref{bmqa::MessageEvent} for a
/// @bbref{bmqa::MessageIterator}.
///
/// Note that @bbref{bmqa::MessageEvent} is implemented using the pimpl idiom,
/// so copying a @bbref{bmqa::MessageEvent} is very cheap (a pointer copy).
/// All copies of this @bbref{bmqa::MessageEvent} will share the same
/// underlying implementation.
///
/// @see @bbref{bmqa::MessageIterator}:
///      iterator over the messages in this event
/// @see @bbref{bmqa::Message}:
///      type of the Message

// BMQ

#include <bmqa_messageiterator.h>
#include <bmqt_messageeventtype.h>

// BDE
#include <bsl_iosfwd.h>
#include <bsl_memory.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqimp {
class Event;
}

namespace bmqa {

// ==================
// class MessageEvent
// ==================

/// An event containing messages received from a queue.  The application can
/// consume messages using the message iterator.
class MessageEvent {
  private:
    // DATA
    bsl::shared_ptr<bmqimp::Event> d_impl_sp;  // pimpl

  public:
    // CREATORS

    /// Create an unset instance.  Note that `type()` will return
    /// `bmqt::MessageEventType::UNDEFINED`.
    MessageEvent();

    // ACCESSORS

    /// Return a `MessageIterator` object usable for iterating over the
    /// `Message` objects contained in this `MessageEvent`.  Note that
    /// obtaining an iterator invalidates (resets) any previously obtained
    /// iterator.  The behavior is undefined if `type()` returns
    /// `bmqt::MessageEventType::UNDEFINED`.
    MessageIterator messageIterator() const;

    /// Return the type of messages contained in this MessageEvent.
    bmqt::MessageEventType::Enum type() const;

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
bsl::ostream& operator<<(bsl::ostream& stream, const MessageEvent& rhs);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

}  // close package namespace

// ------------------
// class MessageEvent
// ------------------

inline bsl::ostream& bmqa::operator<<(bsl::ostream&             stream,
                                      const bmqa::MessageEvent& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
