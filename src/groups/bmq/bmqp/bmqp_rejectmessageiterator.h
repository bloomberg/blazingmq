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

// bmqp_rejectmessageiterator.h                                       -*-C++-*-
#ifndef INCLUDED_BMQP_REJECTMESSAGEITERATOR
#define INCLUDED_BMQP_REJECTMESSAGEITERATOR

//@PURPOSE: Provide a mechanism to iterate over messages of a 'REJECT' event.
//
//@CLASSES:
//  bmqp::RejectMessageIterator: 'RejectEvent' read-only sequential iterator
//
//@DESCRIPTION: 'bmqp::RejectMessageIterator' is an iterator-like mechanism
// providing read-only sequential access to messages contained in a
// RejectEvent.
//
/// Error handling: Logging and Assertion
///-------------------------------------
//: o logging: This iterator will not log anything in case of invalid data:
//:   this is the caller's responsibility to check the return value of
//:   'isValid()' and/or 'next()' and take action (the 'dumpBlob()' method can
//:   be used to print some helpful information).
//: o assertion: When built in SAFE mode, the iterator will assert when
//:   inconsistencies between the blob and the headers are detected.
//
/// Usage
///-----
// Typical usage of this iterator should follow the following pattern:
//..
//  int rc = 0;
//  while ((rc = rejectMessageIterator.next()) == 1) {
//    // Use rejectMessageIterator accessors, such as ..
//    const bmqt::MessageGUID& id =
//                               rejectMessageIterator.message().messageGUID();
//  }
//  if (rc < 0) {
//    // Invalid RejectMessage Event
//    BALL_LOG_ERROR_BLOCK {
//        BALL_LOG_OUTPUT_STREAM << "Invalid 'RejectMessageEvent' [rc: " << rc
//                               << "]\n";
//        rejectMessageIterator.dumpBlob(BALL_LOG_OUTPUT_STREAM);
//    }
//  }
//..

// BMQ

#include <bmqp_protocol.h>

#include <bmqu_blob.h>
#include <bmqu_blobiterator.h>
#include <bmqu_blobobjectproxy.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_ostream.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqp {

// ===========================
// class RejectMessageIterator
// ===========================

/// An iterator providing read-only sequential access to messages contained
/// in a `RejectEvent`.
class RejectMessageIterator {
  private:
    // DATA
    bmqu::BlobIterator d_blobIter;
    // Blob iterator pointing to the
    // current message in the blob.

    bmqu::BlobObjectProxy<RejectHeader> d_header;
    // Header

    bmqu::BlobObjectProxy<RejectMessage> d_message;
    // Current message

    int d_advanceLength;
    // How much should we advance in
    // 'next()'.

  private:
    // PRIVATE MANIPULATORS

    /// Make this instance a copy of the specified `src`, that is copy and
    /// adjust each of its members to represent the same object as the one
    /// from `src`.
    void copyFrom(const RejectMessageIterator& src);

  public:
    // CREATORS

    /// Create an invalid instance.  Only valid operations on an invalid
    /// instance are assignment, `reset` and `isValid`.
    RejectMessageIterator();

    /// Initialize a new instance using the specified `blob` and
    /// `eventHeader`.  Behavior is undefined if the `blob` pointer is null,
    /// or the pointed-to blob does not contain enough bytes to fit at least
    /// the `eventHeader`.
    RejectMessageIterator(const bdlbb::Blob* blob,
                          const EventHeader& eventHeader);

    /// Copy constructor from the specified `src`.  Needed because
    /// BlobObjectProxy doesn't authorize copy semantic.
    RejectMessageIterator(const RejectMessageIterator& src);

    // MANIPULATORS

    /// Assignment operator from the specified `rhs`.  Needed because
    /// BlobObjectProxy doesn't authorize copy semantic.
    RejectMessageIterator& operator=(const RejectMessageIterator& rhs);

    /// Advance to the next message.  Return 1 if the new position is valid
    /// and represent a valid message, 0 if iteration has reached the end of
    /// the event, or < 0 if an error was encountered.  Note that if this
    /// method returns 0, this instance goes in an invalid state, and after
    /// that, only valid operations on this instance are assignment, `reset`
    /// and `isValid`.
    int next();

    /// Reset this instance using the specified `blob` and `eventHeader`.
    /// Behavior is undefined if the `blob` pointer is null, or the
    /// pointed-to blob does not contain enough bytes to fit at least the
    /// `eventHeader`.  Return 0 on success, and non-zero on error.
    int reset(const bdlbb::Blob* blob, const EventHeader& eventHeader);

    /// Set the internal state of this instance to be same as default
    /// constructed, i.e., invalid.
    void clear();

    /// Dump the beginning of the blob associated to this
    /// RejectMessageIterator to the specified `stream`.
    void dumpBlob(bsl::ostream& stream);

    // ACCESSORS

    /// Return true if this iterator is initialized and valid, and `next()`
    /// can be called on this instance, or return false in all other cases.
    bool isValid() const;

    /// Return a const reference to the reject header of this event.
    /// Behavior is undefined unless `isValid` returns true.
    const RejectHeader& header() const;

    /// Return a const reference to the message currently point to by this
    /// iterator.  Behavior is undefined unless latest call to `next()`
    /// returned 1.
    const RejectMessage& message() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------
// class RejectMessageIterator
// ---------------------------

// CREATORS
inline RejectMessageIterator::RejectMessageIterator()
: d_blobIter(0, bmqu::BlobPosition(), 0, true)
, d_advanceLength(0)
{
    // NOTHING
}

inline RejectMessageIterator::RejectMessageIterator(
    const bdlbb::Blob* blob,
    const EventHeader& eventHeader)
: d_blobIter(0, bmqu::BlobPosition(), 0, true)  // no def ctor - set in reset
{
    reset(blob, eventHeader);
}

inline RejectMessageIterator::RejectMessageIterator(
    const RejectMessageIterator& src)
: d_blobIter(0,
             bmqu::BlobPosition(),
             0,
             true)  // no def ctor - set in copyFrom
{
    copyFrom(src);
}

// MANIPULATORS
inline RejectMessageIterator&
RejectMessageIterator::operator=(const RejectMessageIterator& rhs)
{
    copyFrom(rhs);
    return *this;
}

inline void RejectMessageIterator::clear()
{
    d_blobIter.reset(0, bmqu::BlobPosition(), 0, true);
    d_header.reset();
    d_message.reset();
    d_advanceLength = 0;
}

// ACCESSORS
inline bool RejectMessageIterator::isValid() const
{
    return d_header.isSet();
}

inline const RejectHeader& RejectMessageIterator::header() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return *d_header;
}

inline const RejectMessage& RejectMessageIterator::message() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return *d_message;
}

}  // close package namespace
}  // close enterprise namespace

#endif
