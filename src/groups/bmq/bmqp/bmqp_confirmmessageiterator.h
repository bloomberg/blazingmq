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

// bmqp_confirmmessageiterator.h                                      -*-C++-*-
#ifndef INCLUDED_BMQP_CONFIRMMESSAGEITERATOR
#define INCLUDED_BMQP_CONFIRMMESSAGEITERATOR

//@PURPOSE: Provide a mechanism to iterate over messages of a 'CONFIRM' event.
//
//@CLASSES:
//  bmqp::ConfirmMessageIterator: 'ConfirmEvent' read-only sequential iterator
//
//@DESCRIPTION: 'bmqp::ConfirmMessageIterator' is an iterator-like mechanism
// providing read-only sequential access to messages contained into an
// ConfirmEvent.
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
//  while ((rc = confirmMessageIterator.next()) == 1) {
//    // Use confirmMessageIterator accessors, such as ..
//    const bmqt::MessageGUID& id =
//                              confirmMessageIterator.message().messageGUID();
//  }
//  if (rc < 0) {
//    // Invalid ConfirmMessage Event
//    BALL_LOG_ERROR_BLOCK {
//        BALL_LOG_OUTPUT_STREAM << "Invalid 'ConfirmMessageEvent' [rc: " << rc
//                               << "]\n";
//        confirmMessageIterator.dumpBlob(BALL_LOG_OUTPUT_STREAM);
//    }
//  }
//..

// BMQ

#include <bmqp_protocol.h>

// MWC
#include <mwcu_blob.h>
#include <mwcu_blobiterator.h>
#include <mwcu_blobobjectproxy.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_ostream.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqp {

// ============================
// class ConfirmMessageIterator
// ============================

/// An iterator providing read-only sequential access to messages contained
/// into a `ConfirmEvent`.
class ConfirmMessageIterator {
  private:
    // DATA
    mwcu::BlobIterator d_blobIter;
    // Blob iterator pointing to the
    // current message in the blob.

    mwcu::BlobObjectProxy<ConfirmHeader> d_header;
    // Header

    mwcu::BlobObjectProxy<ConfirmMessage> d_message;
    // Current message

    int d_advanceLength;
    // How much should we advance in
    // 'next()'.

  private:
    // PRIVATE MANIPULATORS

    /// Make this instance a copy of the specified `src`, that is copy and
    /// adjust each of its members to represent the same object as the one
    /// from `src`.
    void copyFrom(const ConfirmMessageIterator& src);

  public:
    // CREATORS

    /// Create an invalid instance.  Only valid operations on an invalid
    /// instance are assignment, `reset` and `isValid`.
    ConfirmMessageIterator();

    /// Initialize a new instance using the specified `blob` and
    /// `eventHeader`.  Behavior is undefined if the `blob` pointer is null,
    /// or the pointed-to blob does not contain enough bytes to fit at least
    /// the `eventHeader`.
    ConfirmMessageIterator(const bdlbb::Blob* blob,
                           const EventHeader& eventHeader);

    /// Copy constructor from the specified `src`.  Needed because
    /// BlobObjectProxy doesn't authorize copy semantic.
    ConfirmMessageIterator(const ConfirmMessageIterator& src);

    // MANIPULATORS

    /// Assignment operator from the specified `rhs`.  Needed because
    /// BlobObjectProxy doesn't authorize copy semantic.
    ConfirmMessageIterator& operator=(const ConfirmMessageIterator& rhs);

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
    /// ConfirmMessageIterator to the specified `stream`.
    void dumpBlob(bsl::ostream& stream);

    // ACCESSORS

    /// Return true if this iterator is initialized and valid, and `next()`
    /// can be called on this instance, or return false in all other cases.
    bool isValid() const;

    /// Return a const reference to the confirm header of this event.
    /// Behavior is undefined unless `isValid` returns true.
    const ConfirmHeader& header() const;

    /// Return a const reference to the message currently point to by this
    /// iterator.  Behavior is undefined unless latest call to `next()`
    /// returned 1.
    const ConfirmMessage& message() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------------------
// class ConfirmMessageIterator
// ----------------------------

// CREATORS
inline ConfirmMessageIterator::ConfirmMessageIterator()
: d_blobIter(0, mwcu::BlobPosition(), 0, true)
, d_advanceLength(0)
{
    // NOTHING
}

inline ConfirmMessageIterator::ConfirmMessageIterator(
    const bdlbb::Blob* blob,
    const EventHeader& eventHeader)
: d_blobIter(0, mwcu::BlobPosition(), 0, true)  // no def ctor - set in reset
{
    reset(blob, eventHeader);
}

inline ConfirmMessageIterator::ConfirmMessageIterator(
    const ConfirmMessageIterator& src)
: d_blobIter(0,
             mwcu::BlobPosition(),
             0,
             true)  // no def ctor - set in copyFrom
{
    copyFrom(src);
}

// MANIPULATORS
inline ConfirmMessageIterator&
ConfirmMessageIterator::operator=(const ConfirmMessageIterator& rhs)
{
    copyFrom(rhs);
    return *this;
}

inline void ConfirmMessageIterator::clear()
{
    d_blobIter.reset(0, mwcu::BlobPosition(), 0, true);
    d_header.reset();
    d_message.reset();
    d_advanceLength = 0;
}

// ACCESSORS
inline bool ConfirmMessageIterator::isValid() const
{
    return d_header.isSet();
}

inline const ConfirmHeader& ConfirmMessageIterator::header() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return *d_header;
}

inline const ConfirmMessage& ConfirmMessageIterator::message() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return *d_message;
}

}  // close package namespace
}  // close enterprise namespace

#endif
