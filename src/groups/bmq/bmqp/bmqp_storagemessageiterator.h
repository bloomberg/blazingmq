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

// bmqp_storagemessageiterator.h                                      -*-C++-*-
#ifndef INCLUDED_BMQP_STORAGEMESSAGEITERATOR
#define INCLUDED_BMQP_STORAGEMESSAGEITERATOR

//@PURPOSE: Provide a mechanism to iterate over messages of a 'STORAGE' event.
//
//@CLASSES:
//  bmqp::StorageMessageIterator: iterator over 'StorageEvent'.
//
//@DESCRIPTION: 'bmqp::StorageMessageIterator' is an iterator-like mechanism
// providing read-only sequential access to messages contained into a
// StorageEvent.
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
//  while ((rc = storageMessageIterator.next()) == 1) {
//    // Use storageMessageIterator accessors, such as ..
//    int flags = stoageMessageIterator.flags();
//  }
//  if (rc < 0) {
//    // Invalid PushMessage Event
//    BALL_LOG_ERROR_BLOCK {
//        BALL_LOG_OUTPUT_STREAM << "Invalid 'StorageEvent' [rc: " << rc
//                               << "]\n";
//        storageMessageIterator.dumpBlob(BALL_LOG_OUTPUT_STREAM);
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

// ============================
// class StorageMessageIterator
// ============================

/// An iterator providing read-only sequential access to messages contained
/// into a `StorageEvent`.
class StorageMessageIterator {
  private:
    // DATA
    bmqu::BlobIterator d_blobIter;
    // Blob iterator pointing to the
    // current message in the blob.

    bmqu::BlobObjectProxy<StorageHeader> d_header;
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
    void copyFrom(const StorageMessageIterator& src);

  public:
    // CREATORS

    /// Create an invalid instance.  Only valid operations on an invalid
    /// instance are assignment, `reset` and `isValid`.
    StorageMessageIterator();

    /// Initialize a new instance using the specified `blob` and
    /// `eventHeader`.  Behavior is undefined if the `blob` pointer is null,
    /// or the pointed-to blob does not contain enough bytes to fit at least
    /// the `eventHeader`.
    StorageMessageIterator(const bdlbb::Blob* blob,
                           const EventHeader& eventHeader);

    /// Copy constructor from the specified `src`.  Needed because
    /// BlobObjectProxy doesn't authorize copy semantic.
    StorageMessageIterator(const StorageMessageIterator& src);

    // MANIPULATORS

    /// Assignment operator from the specified `rhs`.  Needed because
    /// BlobObjectProxy doesn't authorize copy semantic.
    StorageMessageIterator& operator=(const StorageMessageIterator& rhs);

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
    void reset(const bdlbb::Blob* blob, const StorageMessageIterator& other);

    /// Set the internal state of this instance to be same as default
    /// constructed, i.e., invalid.
    void clear();

    // ACCESSORS

    /// Return true if this iterator is initialized and valid, and `next()`
    /// can be called on this instance, or return false in all other cases.
    bool isValid() const;

    /// Return a const reference to the StorageHeader currently pointed to
    /// by this iterator.  The behavior is undefined unless `isValid`
    /// returns true.
    const StorageHeader& header() const;

    /// Return a const reference to the position of header of the currently
    /// pointed to by this iterator.  The behavior is undefined unless
    /// `isValid` returns true.
    const bmqu::BlobPosition& headerPosition() const;

    /// Load into the specified `position` the starting location of the data
    /// of the storage message currently pointed to by this iterator.  The
    /// behavior is undefined unless `position` is not null.  Return zero on
    /// success, non zero value otherwise.
    int loadDataPosition(bmqu::BlobPosition* position) const;

    /// Dump the beginning of the blob associated to this
    /// StorageMessageIterator to the specified `stream`.
    void dumpBlob(bsl::ostream& stream) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------------------
// class StorageMessageIterator
// ----------------------------

// CREATORS
inline StorageMessageIterator::StorageMessageIterator()
: d_blobIter(0, bmqu::BlobPosition(), 0, true)
, d_advanceLength(-1)
{
    // NOTHING
}

inline StorageMessageIterator::StorageMessageIterator(
    const bdlbb::Blob* blob,
    const EventHeader& eventHeader)
: d_blobIter(0, bmqu::BlobPosition(), 0, true)  // no def ctor - set in reset
{
    reset(blob, eventHeader);
}

inline StorageMessageIterator::StorageMessageIterator(
    const StorageMessageIterator& src)
: d_blobIter(0,
             bmqu::BlobPosition(),
             0,
             true)  // no def ctor - set in copyFrom
{
    copyFrom(src);
}

// MANIPULATORS
inline StorageMessageIterator&
StorageMessageIterator::operator=(const StorageMessageIterator& rhs)
{
    if (this != &rhs) {
        copyFrom(rhs);
    }

    return *this;
}

inline void StorageMessageIterator::clear()
{
    d_blobIter.reset(0, bmqu::BlobPosition(), 0, true);
    d_header.reset();
    d_advanceLength = -1;
}

// ACCESSORS
inline bool StorageMessageIterator::isValid() const
{
    return (d_advanceLength != -1) && !d_blobIter.atEnd();
}

inline const StorageHeader& StorageMessageIterator::header() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return *d_header;
}

inline const bmqu::BlobPosition& StorageMessageIterator::headerPosition() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header.position();
}

inline int
StorageMessageIterator::loadDataPosition(bmqu::BlobPosition* position) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(position);
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(StorageMessageType::e_UNDEFINED !=
                     d_header->messageType());

    return bmqu::BlobUtil::findOffsetSafe(
        position,
        *d_blobIter.blob(),
        d_blobIter.position(),
        (d_header->headerWords() * Protocol::k_WORD_SIZE));
}

}  // close package namespace
}  // close enterprise namespace

#endif
