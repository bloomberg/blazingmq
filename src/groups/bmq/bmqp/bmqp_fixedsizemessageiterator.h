// Copyright 2026 Bloomberg Finance L.P.
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

#ifndef INCLUDED_BMQP_FIXEDSIZEMESSAGEITERATOR
#define INCLUDED_BMQP_FIXEDSIZEMESSAGEITERATOR

//@PURPOSE: Provide an iterator over events made of fixed-size messages.
//
//@CLASSES:
//  bmqp::FixedSizeMessageIterator: read-only sequential iterator template
//
//@DESCRIPTION: 'bmqp::FixedSizeMessageIterator<HEADER, MESSAGE>' is an
// iterator-like mechanism providing read-only sequential access to the
// messages of an event laid out as:
//..
//  [EventHeader][HEADER][MESSAGE][MESSAGE]...
//..
// where 'HEADER' declares its own size ('headerWords()') and the size of each
// following 'MESSAGE' ('perMessageWords()').  It holds the logic shared by
// 'bmqp::AckMessageIterator', 'bmqp::ConfirmMessageIterator' and
// 'bmqp::RejectMessageIterator', which derive from it.
//
// The 'HEADER' type must provide:
//: o 'static const int k_MIN_HEADER_SIZE'
//: o 'int headerWords() const'
//: o 'int perMessageWords() const'
//
/// Error handling: Logging and Assertion
///-------------------------------------
//: o logging: This iterator will not log anything in case of invalid data:
//:   this is the caller's responsibility to check the return value of
//:   'isValid()' and/or 'next()' and take action (the 'dumpBlob()' method can
//:    be used to print some helpful information).
//: o assertion: When built in SAFE mode, the iterator will assert when
//:   inconsistencies between the blob and the headers are detected.
//: o malformed header: 'reset()' rejects a 'HEADER' smaller than
//:   'HEADER::k_MIN_HEADER_SIZE' or declaring zero per-message words, and
//:   leaves the iterator invalid, so that 'next()' always advances.

// BMQ

#include <bmqp_protocol.h>

#include <bmqu_blob.h>
#include <bmqu_blobiterator.h>
#include <bmqu_blobobjectproxy.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_ostream.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqp {

// ==============================
// class FixedSizeMessageIterator
// ==============================

/// An iterator providing read-only sequential access to the fixed-size
/// `MESSAGE`s following a `HEADER` in an event.
template <class HEADER, class MESSAGE>
class FixedSizeMessageIterator {
  private:
    // DATA

    /// Blob iterator pointing to the current message in the blob.
    bmqu::BlobIterator d_blobIter;

    /// Header
    bmqu::BlobObjectProxy<HEADER> d_header;

    /// Current message
    bmqu::BlobObjectProxy<MESSAGE> d_message;

    /// How much should we advance in 'next()'.
    int d_advanceLength;

  private:
    // PRIVATE MANIPULATORS

    /// Make this instance a copy of the specified `src`, that is copy and
    /// adjust each of its members to represent the same object as the one
    /// from `src`.
    void copyFrom(const FixedSizeMessageIterator& src);

  public:
    // CREATORS

    /// Create an invalid instance.  Only valid operations on an invalid
    /// instance are assignment, `reset` and `isValid`.
    FixedSizeMessageIterator();

    /// Initialize a new instance using the specified `blob` and
    /// `eventHeader`.  Behavior is undefined if the `blob` pointer is null,
    /// or the pointed-to blob does not contain enough bytes to fit at least
    /// the `eventHeader`.
    FixedSizeMessageIterator(const bdlbb::Blob* blob,
                             const EventHeader& eventHeader);

    /// Copy constructor from the specified `src`.  Needed because
    /// BlobObjectProxy doesn't authorize copy semantic.
    FixedSizeMessageIterator(const FixedSizeMessageIterator& src);

    // MANIPULATORS

    /// Assignment operator from the specified `rhs`.  Needed because
    /// BlobObjectProxy doesn't authorize copy semantic.
    FixedSizeMessageIterator& operator=(const FixedSizeMessageIterator& rhs);

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
    int reset(const bdlbb::Blob* blob, const FixedSizeMessageIterator& other);

    /// Set the internal state of this instance to be same as default
    /// constructed, i.e., invalid.
    void clear();

    /// Dump the beginning of the blob associated to this iterator to the
    /// specified `stream`.
    void dumpBlob(bsl::ostream& stream);

    // ACCESSORS

    /// Return true if this iterator is initialized and valid, and `next()`
    /// can be called on this instance, or return false in all other cases.
    bool isValid() const;

    /// Return a const reference to the header of this event.  Behavior is
    /// undefined unless `isValid` returns true.
    const HEADER& header() const;

    /// Return a const reference to the message currently point to by this
    /// iterator.  Behavior is undefined unless latest call to `next()`
    /// returned 1.
    const MESSAGE& message() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------------------
// class FixedSizeMessageIterator
// ------------------------------

// PRIVATE MANIPULATORS
template <class HEADER, class MESSAGE>
inline void FixedSizeMessageIterator<HEADER, MESSAGE>::copyFrom(
    const FixedSizeMessageIterator& src)
{
    d_blobIter      = src.d_blobIter;
    d_advanceLength = src.d_advanceLength;

    if (!src.d_header.isSet()) {
        d_header.reset();
        d_message.reset();
    }
    else {
        d_header.reset(src.d_header.blob(),
                       src.d_header.position(),
                       src.d_header.length());

        if (src.d_message.isSet()) {
            d_message.reset(src.d_message.blob(),
                            src.d_message.position(),
                            d_header->perMessageWords() *
                                Protocol::k_WORD_SIZE);
        }
    }
}

// CREATORS
template <class HEADER, class MESSAGE>
inline FixedSizeMessageIterator<HEADER, MESSAGE>::FixedSizeMessageIterator()
: d_blobIter(0, bmqu::BlobPosition(), 0, true)
, d_advanceLength(0)
{
    // NOTHING
}

template <class HEADER, class MESSAGE>
inline FixedSizeMessageIterator<HEADER, MESSAGE>::FixedSizeMessageIterator(
    const bdlbb::Blob* blob,
    const EventHeader& eventHeader)
: d_blobIter(0, bmqu::BlobPosition(), 0, true)  // no def ctor - set in reset
, d_advanceLength(0)
{
    reset(blob, eventHeader);
}

template <class HEADER, class MESSAGE>
inline FixedSizeMessageIterator<HEADER, MESSAGE>::FixedSizeMessageIterator(
    const FixedSizeMessageIterator& src)
: d_blobIter(0,
             bmqu::BlobPosition(),
             0,
             true)  // no def ctor - set in copyFrom
, d_advanceLength(0)
{
    copyFrom(src);
}

// MANIPULATORS
template <class HEADER, class MESSAGE>
inline FixedSizeMessageIterator<HEADER, MESSAGE>&
FixedSizeMessageIterator<HEADER, MESSAGE>::operator=(
    const FixedSizeMessageIterator& rhs)
{
    copyFrom(rhs);
    return *this;
}

template <class HEADER, class MESSAGE>
inline int FixedSizeMessageIterator<HEADER, MESSAGE>::next()
{
    enum RcEnum {
        // Value for the various RC error categories
        /// There is another message after this one
        rc_HAS_NEXT = 1,
        /// This is the last message
        rc_AT_END = 0,
        /// The Iterator is an invalid state
        rc_INVALID = -1,
        /// The number of bytes in the blob is less than the payload size of
        /// the message declared in the header
        rc_NOT_ENOUGH_BYTES = -2
    };

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isValid())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_INVALID;  // RETURN
    }

    // 'reset()' rejects zero header and per-message sizes
    BSLS_ASSERT_SAFE(d_advanceLength > 0);

    if (d_blobIter.advance(d_advanceLength) == false) {
        d_header.reset();
        return rc_AT_END;  // RETURN
    }

    // Update 'advanceLength' for next iteration.
    // NOTE: We do it at every 'next' (even if the value is always going to be
    //       the same) because in 'reset()', we first set it to the size of the
    //       'HEADER', since first 'next()' should move beyond the 'HEADER'.
    d_advanceLength = d_header->perMessageWords() * Protocol::k_WORD_SIZE;

    // Update the current message, supporting protocol evolution by reading as
    // many bytes as the header declares (and not as many as the size of the
    // struct)
    d_message.reset(d_blobIter.blob(),
                    d_blobIter.position(),
                    d_header->perMessageWords() * Protocol::k_WORD_SIZE);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_message.isSet())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_NOT_ENOUGH_BYTES;  // RETURN
    }

    return rc_HAS_NEXT;
}

template <class HEADER, class MESSAGE>
inline int FixedSizeMessageIterator<HEADER, MESSAGE>::reset(
    const bdlbb::Blob* blob,
    const EventHeader& eventHeader)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(blob);

    enum RcEnum {
        // Value for the various RC error categories
        /// Success
        rc_SUCCESS = 0,
        /// The blob doesn't contain a complete EventHeader, or is not followed
        /// by a HEADER
        rc_INVALID_EVENTHEADER = -1,
        /// The blob doesn't contain a complete HEADER, or the HEADER declares
        /// zero header or per-message words
        rc_INVALID_HEADER = -2,
        /// The number of bytes in the blob is less than the header size
        /// declared in the header
        rc_NOT_ENOUGH_BYTES = -3
    };

    d_blobIter.reset(blob, bmqu::BlobPosition(), blob->length(), true);

    // Skip the EventHeader to point to the HEADER
    const bool rc = d_blobIter.advance(eventHeader.headerWords() *
                                       Protocol::k_WORD_SIZE);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!rc)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Set the iterator to invalid state
        d_header.reset();
        return rc_INVALID_EVENTHEADER;  // RETURN
    }

    // Read HEADER, supporting protocol evolution by reading up to size of the
    // struct bytes (-1 parameter), and then resizing the proxy to match the
    // size declared in the header
    d_header.reset(blob, d_blobIter.position(), -HEADER::k_MIN_HEADER_SIZE);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_header.isSet())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_INVALID_HEADER;  // RETURN
    }

    const int headerSize = d_header->headerWords() * Protocol::k_WORD_SIZE;
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            headerSize < HEADER::k_MIN_HEADER_SIZE ||
            d_header->perMessageWords() == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Header is malformed: explicitly reset the proxy so that isValid()
        // returns false.
        d_header.reset();
        return rc_INVALID_HEADER;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(headerSize >
                                              d_blobIter.remaining())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // The header is declaring more bytes than are left in the blob.
        // Explicitly reset the proxy so that isValid() returns false.
        d_header.reset();
        return rc_NOT_ENOUGH_BYTES;  // RETURN
    }

    d_header.resize(headerSize);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_header.isSet())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_INVALID_HEADER;  // RETURN
    }

    // Reset the current message
    d_message.reset();

    // Below code snippet is needed so that 'next()' works seamlessly during
    // first invocation too, by skipping over the 'HEADER'.
    d_advanceLength = headerSize;

    return rc_SUCCESS;
}

template <class HEADER, class MESSAGE>
inline int FixedSizeMessageIterator<HEADER, MESSAGE>::reset(
    const bdlbb::Blob*              blob,
    const FixedSizeMessageIterator& other)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(blob);

    // Copy members from 'other'
    copyFrom(other);
    d_blobIter.reset(blob,
                     other.d_blobIter.position(),
                     other.d_blobIter.remaining(),
                     true);

    return 0;
}

template <class HEADER, class MESSAGE>
inline void FixedSizeMessageIterator<HEADER, MESSAGE>::clear()
{
    d_blobIter.reset(0, bmqu::BlobPosition(), 0, true);
    d_header.reset();
    d_message.reset();
    d_advanceLength = 0;
}

template <class HEADER, class MESSAGE>
inline void
FixedSizeMessageIterator<HEADER, MESSAGE>::dumpBlob(bsl::ostream& stream)
{
    static const int k_MAX_BYTES_DUMP = 128;

    // For now, print only the beginning of the blob.. we may later on print
    // also the bytes around the current position
    if (d_blobIter.blob()) {
        stream << bmqu::BlobStartHexDumper(d_blobIter.blob(),
                                           k_MAX_BYTES_DUMP);
    }
    else {
        stream << "/no blob/";
    }
}

// ACCESSORS
template <class HEADER, class MESSAGE>
inline bool FixedSizeMessageIterator<HEADER, MESSAGE>::isValid() const
{
    return d_header.isSet();
}

template <class HEADER, class MESSAGE>
inline const HEADER& FixedSizeMessageIterator<HEADER, MESSAGE>::header() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return *d_header;
}

template <class HEADER, class MESSAGE>
inline const MESSAGE&
FixedSizeMessageIterator<HEADER, MESSAGE>::message() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return *d_message;
}

}  // close package namespace
}  // close enterprise namespace

#endif
