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

// bmqa_messageiterator.h                                             -*-C++-*-
#ifndef INCLUDED_BMQA_MESSAGEITERATOR
#define INCLUDED_BMQA_MESSAGEITERATOR

/// @file bmqa_messageiterator.h
///
/// @brief Provide a mechanism to iterate over the messages of a
///        @bbref{bmqa::MessageEvent}.
///
///
/// @bbref{bmqa::MessageIterator} is an iterator-like mechanism providing
/// read-only sequential access to messages contained into a
/// @bbref{bmqa::MessageEvent}.
///
/// Usage                                         {#bmqa_messageiterator_usage}
/// =====
///
/// Typical usage of this iterator should follow the following pattern:
/// ```
/// while (messageIterator.nextMessage()) {
///   const Message& message = messageIterator.message();
///   // Do something with message
/// }
/// ```

// BMQ

#include <bmqa_message.h>

// BDE
#include <ball_log.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqimp {
class Event;
}

namespace bmqa {

// ==========================
// struct MessageIteratorImpl
// ==========================

/// @private
/// Struct to hold the impl of the `MessageIterator`; that is so that we can
/// keep the real impl private and use some special cast to manipulate it,
/// without publicly exposing private members.
struct MessageIteratorImpl {
    // PUBLIC DATA

    /// Raw pointer to the event
    bmqimp::Event* d_event_p;

    /// A `Message`, representing a view to the current message pointing at by
    /// this iterator.  This is so that `message` can return a 'const Message&'
    /// to clearly indicate the lifetime of the Message, and so that we only
    /// create one such object per MessageIterator.
    bmqa::Message d_message;

    /// Position of `d_message` in the underlying message event.
    int d_messageIndex;
};

// =====================
// class MessageIterator
// =====================

/// An iterator providing read-only sequential access to messages contained
/// into a `MesssageEvent`.
class MessageIterator {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQA.MESSAGEITERATOR");

  private:
    // DATA

    /// Implementation. Abstracted in its own struct and private so that we can
    /// do some magic to manipulate it without exposing any
    /// accessors/manipulators (this is wanted since this class is a public
    /// class).
    MessageIteratorImpl d_impl;

  public:
    // CREATORS

    /// Default constructor
    MessageIterator();

    // MANIPULATORS

    /// Advance the position of the iterator to the next message in the
    /// event.  Return true if there is a next message and false otherwise.
    /// Note that advancing to the next message will invalidate any
    /// previously returned bmqa::Message retrieved from the `message()`
    /// call.
    bool nextMessage();

    // ACCESSORS

    /// Return the message to which the iterator is pointing, if any;
    /// otherwise return an invalid message.  Note that `nextMessage` must
    /// be called before `message` in order to advance the iterators
    /// position to the first message in the event.
    const Message& message() const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
