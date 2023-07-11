// Copyright 2015-2023 Bloomberg Finance L.P.
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

// bmqp_recoverymessageiterator.h                                     -*-C++-*-
#ifndef INCLUDED_BMQP_RECOVERYMESSAGEITERATOR
#define INCLUDED_BMQP_RECOVERYMESSAGEITERATOR

//@PURPOSE: Provide a mechanism to iterate over messages of a 'RECOVERY' event.
//
//@CLASSES:
//  bmqp::RecoveryMessageIterator: iterator over 'RecoveryEvent'.
//
//@DESCRIPTION: 'bmqp::RecoveryMessageIterator' is an iterator-like mechanism
// providing read-only sequential access to messages contained into a
// RecoveryEvent.
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
//  while ((rc = recoveryMessageIterator.next()) == 1) {
//    // Use recoveryMessageIterator accessors, such as ..
//    int flags = recoveryMessageIterator.flags();
//  }
//  if (rc < 0) {
//    // Invalid RecoveryMessage Event
//    BALL_LOG_ERROR_BLOCK {
//        BALL_LOG_OUTPUT_STREAM << "Invalid 'RecoveryEvent' [rc: " << rc
//                               << "]\n";
//        recoveryMessageIterator.dumpBlob(BALL_LOG_OUTPUT_STREAM);
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

// =============================
// class RecoveryMessageIterator
// =============================

/// An iterator providing read-only sequential access to messages contained
/// into a `RecoveryEvent`.
class RecoveryMessageIterator {
  private:
    // DATA
    mwcu::BlobIterator d_blobIter;
    // Blob iterator pointing to the
    // current message in the blob.

    mwcu::BlobObjectProxy<RecoveryHeader> d_header;
    // Current PutHeader

    int d_advanceLength;
    // How much should we advance in the
    // blob when calling 'next()'.  The
    // iterator is considered in invalid
    // state if this value is == -1.

  private:
    // PRIVATE MANIPULATORS

    /// Make this instance a copy of the specified `src`, that is copy and
    /// adjust each of its members to represent the same object as the one
    /// from `src`.
    void copyFrom(const RecoveryMessageIterator& src);

  public:
    // CREATORS

    /// Create an invalid instance.  Only valid operations on an invalid
    /// instance are assignment, `reset`, `isValid` and destruction.
    RecoveryMessageIterator();

    /// Initialize a new instance using the specified `blob` and
    /// `eventHeader`.  Behavior is undefined if the `blob` pointer is null,
    /// or the pointed-to blob does not contain enough bytes to fit at least
    /// the `eventHeader`.
    RecoveryMessageIterator(const bdlbb::Blob* blob,
                            const EventHeader& eventHeader);

    /// Copy constructor from the specified `src`.  Needed because
    /// BlobObjectProxy doesn't authorize copy semantic.
    RecoveryMessageIterator(const RecoveryMessageIterator& src);

    // MANIPULATORS

    /// Assignment operator from the specified `rhs`.  Needed because
    /// BlobObjectProxy doesn't authorize copy semantic.
    RecoveryMessageIterator& operator=(const RecoveryMessageIterator& rhs);

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
    /// `eventHeader`.  Return 0 on success, non-zero value otherwise.
    int reset(const bdlbb::Blob* blob, const EventHeader& eventHeader);

    /// Point this instance to the specified `blob` using the position and
    /// other meta data from the specified `other` instance.  This method is
    /// useful when it is desired to copy `other` instance into this
    /// instance but the blob being held by `other` instance will not
    /// outlive this instance.  The behavior is undefined unless `blob` is
    /// non-null.
    void reset(const bdlbb::Blob* blob, const RecoveryMessageIterator& other);

    /// Set the internal state of this instance to be same as default
    /// constructed, i.e., invalid.
    void clear();

    // ACCESSORS

    /// Return true if this iterator is initialized and valid, and `next()`
    /// can be called on this instance, or return false in all other cases.
    bool isValid() const;

    /// Return a const reference to the RecoveryHeader currently pointed to
    /// by this iterator.  The behavior is undefined unless `isValid`
    /// returns true.
    const RecoveryHeader& header() const;

    /// Load into the specified `position` the starting location of the file
    /// chunk of the recovery message currently pointed to by this iterator.
    /// The behavior is undefined unless `position` is not null.  Return
    /// zero on success, non zero value otherwise.
    int loadChunkPosition(mwcu::BlobPosition* position) const;

    /// Dump the beginning of the blob associated to this
    /// RecoveryMessageIterator to the specified `stream`.
    void dumpBlob(bsl::ostream& stream) const;
};

// ============================================================================
//                            INLINE DEFINITIONS
// ============================================================================

// -----------------------------
// class RecoveryMessageIterator
// -----------------------------

// CREATORS
inline RecoveryMessageIterator::RecoveryMessageIterator()
: d_blobIter(0, mwcu::BlobPosition(), 0, true)
, d_advanceLength(-1)
{
    // NOTHING
}

inline RecoveryMessageIterator::RecoveryMessageIterator(
    const bdlbb::Blob* blob,
    const EventHeader& eventHeader)
: d_blobIter(0, mwcu::BlobPosition(), 0, true)  // no def ctor - set in reset
{
    reset(blob, eventHeader);
}

inline RecoveryMessageIterator::RecoveryMessageIterator(
    const RecoveryMessageIterator& src)
: d_blobIter(0,
             mwcu::BlobPosition(),
             0,
             true)  // no def ctor - set in copyFrom
{
    copyFrom(src);
}

// MANIPULATORS
inline RecoveryMessageIterator&
RecoveryMessageIterator::operator=(const RecoveryMessageIterator& rhs)
{
    if (this != &rhs) {
        copyFrom(rhs);
    }

    return *this;
}

inline void RecoveryMessageIterator::clear()
{
    d_blobIter.reset(0, mwcu::BlobPosition(), 0, true);
    d_header.reset();
    d_advanceLength = -1;
}

// ACCESSORS
inline bool RecoveryMessageIterator::isValid() const
{
    return d_advanceLength != -1 && !d_blobIter.atEnd();
}

inline const RecoveryHeader& RecoveryMessageIterator::header() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return *d_header;
}

inline int
RecoveryMessageIterator::loadChunkPosition(mwcu::BlobPosition* position) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(position);
    BSLS_ASSERT_SAFE(isValid());

    return mwcu::BlobUtil::findOffsetSafe(
        position,
        *d_blobIter.blob(),
        d_blobIter.position(),
        (d_header->headerWords() * Protocol::k_WORD_SIZE));
}

}  // close package namespace
}  // close enterprise namespace

#endif
