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

// bmqp_ackmessageiterator.h                                          -*-C++-*-
#ifndef INCLUDED_BMQP_ACKMESSAGEITERATOR
#define INCLUDED_BMQP_ACKMESSAGEITERATOR

//@PURPOSE: Provide a mechanism to iterate over the messages of an AckEvent.
//
//@CLASSES:
//  bmqp::AckMessageIterator: read-only sequential iter on 'AckEvent'.
//
//@DESCRIPTION: 'bmqp::AckMessageIterator' is an iterator-like mechanism
// providing read-only sequential access to messages contained into a AckEvent.
//
/// Error handling: Logging and Assertion
///-------------------------------------
//: o logging: This iterator will not log anything in case of invalid data:
//:   this is the caller's responsibility to check the return value of
//:   'isValid()' and/or 'next()' and take action (the 'dumpBlob()' method can
//:    be used to print some helpful information).
//: o assertion: When built in SAFE mode, the iterator will assert when
//:   inconsistencies between the blob and the headers are detected.
//
/// Usage
///-----
// Typical usage of this iterator should follow the following pattern:
//..
//  int rc = 0;
//  while ((rc = ackMessageIterator.next()) == 1) {
//    // Use ackMessageIterator accessors, such as ..
//    int status = ackMessageIterator.message().status();
//  }
//  if (rc < 0) {
//    // Invalid AckMessage Event
//    BALL_LOG_ERROR_BLOCK {
//        BALL_LOG_OUTPUT_STREAM << "Invalid 'AckMessageEvent' [rc: " << rc
//                               << "]\n";
//        ackMessageIterator.dumpBlob(BALL_LOG_OUTPUT_STREAM);
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

// ========================
// class AckMessageIterator
// ========================

/// An iterator providing read-only sequential access to messages contained
/// into a `AckEvent`.
class AckMessageIterator {
  private:
    // DATA
    bmqu::BlobIterator d_blobIter;
    // Blob iterator pointing to the current
    // message in the blob.

    bmqu::BlobObjectProxy<AckHeader> d_header;
    // Header

    bmqu::BlobObjectProxy<AckMessage> d_message;
    // Current message

    int d_advanceLength;
    // How much should we advance in
    // 'next()'.

  private:
    // PRIVATE MANIPULATORS

    /// Make this instance a copy of the specified `src`, that is copy and
    /// adjust each of its members to represent the same object as the one
    /// from `src`.
    void copyFrom(const AckMessageIterator& src);

  public:
    // CREATORS

    /// Create an invalid instance.  Only valid operations on an invalid
    /// instance are assignment, `reset` and `isValid`.
    AckMessageIterator();

    /// Initialize a new instance using the specified `blob` and
    /// `eventHeader`.  Behavior is undefined if the `blob` pointer is null,
    /// or the pointed-to blob does not contain enough bytes to fit at least
    /// the `eventHeader`.
    AckMessageIterator(const bdlbb::Blob* blob,
                       const EventHeader& eventHeader);

    /// Copy constructor from the specified `src`.  Needed because
    /// BlobObjectProxy doesn't authorize copy semantic.
    AckMessageIterator(const AckMessageIterator& src);

    // MANIPULATORS

    /// Assignment operator from the specified `rhs`.  Needed because
    /// BlobObjectProxy doesn't authorize copy semantic.
    AckMessageIterator& operator=(const AckMessageIterator& rhs);

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

    /// Point this instance to the specified `blob` using the position and
    /// other meta data from the specified `other` instance.  This method is
    /// useful when it is desired to copy `other` instance into this
    /// instance but the blob being held by `other` instance will not
    /// outlive this instance.  Behavior is undefined unless `blob` is
    /// non-null.  Return 0 on success, and non-zero on error.
    int reset(const bdlbb::Blob* blob, const AckMessageIterator& other);

    /// Set the internal state of this instance to be same as default
    /// constructed, i.e., invalid.
    void clear();

    /// Dump the beginning of the blob associated to this AckMessageIterator
    /// to the specified `stream`.
    void dumpBlob(bsl::ostream& stream);

    // ACCESSORS

    /// Return true if this iterator is initialized and valid, and `next()`
    /// can be called on this instance, or return false in all other cases.
    bool isValid() const;

    /// Return a const reference to the ack header of this event.  Behavior
    /// is undefined unless `isValid` returns true.
    const AckHeader& header() const;

    /// Return a const reference to the message currently point to by this
    /// iterator.  Behavior is undefined unless latest call to `next()`
    /// returned 1.
    const AckMessage& message() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------------
// class AckMessageIterator
// ------------------------

// CREATORS
inline AckMessageIterator::AckMessageIterator()
: d_blobIter(0, bmqu::BlobPosition(), 0, true)
, d_advanceLength(0)
{
    // NOTHING
}

inline AckMessageIterator::AckMessageIterator(const bdlbb::Blob* blob,
                                              const EventHeader& eventHeader)
: d_blobIter(0, bmqu::BlobPosition(), 0, true)  // no def ctor - set in reset
{
    reset(blob, eventHeader);
}

inline AckMessageIterator::AckMessageIterator(const AckMessageIterator& src)
: d_blobIter(0,
             bmqu::BlobPosition(),
             0,
             true)  // no def ctor - set in copyFrom
{
    copyFrom(src);
}

// MANIPULATORS
inline AckMessageIterator&
AckMessageIterator::operator=(const AckMessageIterator& rhs)
{
    copyFrom(rhs);
    return *this;
}

inline void AckMessageIterator::clear()
{
    d_blobIter.reset(0, bmqu::BlobPosition(), 0, true);
    d_header.reset();
    d_message.reset();
    d_advanceLength = 0;
}

// ACCESSORS
inline bool AckMessageIterator::isValid() const
{
    return d_header.isSet();
}

inline const AckHeader& AckMessageIterator::header() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return *d_header;
}

inline const AckMessage& AckMessageIterator::message() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return *d_message;
}

}  // close package namespace
}  // close enterprise namespace

#endif
