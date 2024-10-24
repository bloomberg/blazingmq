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

// bmqp_pushmessageiterator.h                                         -*-C++-*-
#ifndef INCLUDED_BMQP_PUSHMESSAGEITERATOR
#define INCLUDED_BMQP_PUSHMESSAGEITERATOR

//@PURPOSE: Provide a mechanism to iterate over messages of a 'PUSH' event.
//
//@CLASSES:
//  bmqp::PushMessageIterator: read-only sequential iterator on 'PushEvent'.
//
//@DESCRIPTION: 'bmqp::PushMessageIterator' is an iterator-like mechanism
// providing read-only sequential access to messages contained into a
// PushEvent.
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
//  while ((rc = pushMessageIterator.next()) == 1) {
//    // Use pushMessageIterator accessors, such as ..
//    int payloadSize = pushMessageIterator.messagePayloadSize();
//  }
//  if (rc < 0) {
//    // Invalid PushMessage Event
//    BALL_LOG_ERROR_BLOCK {
//        BALL_LOG_OUTPUT_STREAM << "Invalid 'PushEvent' [rc: " << rc << "]\n";
//        pushMessageIterator.dumpBlob(BALL_LOG_OUTPUT_STREAM);
//    }
//  }
//..

// BMQ

#include <bmqp_optionsview.h>
#include <bmqp_protocol.h>
#include <bmqp_queueid.h>

#include <bmqu_blob.h>
#include <bmqu_blobiterator.h>

// BDE
#include <bdlb_nullablevalue.h>
#include <bdlbb_blob.h>
#include <bsl_ostream.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>

namespace BloombergLP {

namespace bmqp {

// FORWARD DECLARATION
class MessageProperties;

// =========================
// class PushMessageIterator
// =========================

/// An iterator providing read-only sequential access to messages contained
/// into a `PushEvent`.
class PushMessageIterator {
  private:
    // PRIVATE TYPES
    typedef bdlb::NullableValue<OptionsView> NullableOptionsView;

  private:
    // DATA
    bmqu::BlobIterator d_blobIter;
    // Blob iterator pointing to the
    // current message in the blob.

    PushHeader d_header;
    // Current PushHeader

    mutable int d_applicationDataSize;
    // Computed application data real size
    // (without padding). -1 if not
    // initialized. Note that if
    // 'd_decompressFlag' is true, this will
    // store the size of decompressed
    // application data and vice-versa.

    mutable int d_lazyMessagePayloadSize;
    // Lazily computed payload real size
    // (without padding).  -1 if not
    // initialized.

    mutable bmqu::BlobPosition d_lazyMessagePayloadPosition;
    // Lazily computed payload position.
    // Unset if not initialized.

    mutable int d_messagePropertiesSize;
    // Message properties size.  0 if not
    // initialized.  Note that this length
    // includes padding and message
    // properties header.

    bmqu::BlobPosition d_applicationDataPosition;
    // Application Data position. For each
    // blob, initialized in next().

    mutable int d_optionsSize;
    // Message options size.

    mutable bmqu::BlobPosition d_optionsPosition;
    // Message options position.  Unset if
    // not initialized.

    int d_advanceLength;
    // How much should we advance in the
    // blob when calling 'next()'.  The
    // iterator is considered in invalid
    // state if this value is == -1.

    mutable NullableOptionsView d_optionsView;
    // The OptionsView for this iterator.

    bool d_decompressFlag;
    // Flag indicating if message should be
    // decompressed when calling next().
    // false if not initialized.

    bdlbb::Blob d_applicationData;
    // Decompressed application data.
    // Populated only if d_decompressFlag is
    // true (empty otherwise).

    bdlbb::BlobBufferFactory* d_bufferFactory_p;
    // Buffer factory used for decompressed
    // application data.

    bslma::Allocator* d_allocator_p;
    // Allocator to use by this object.

  private:
    // PRIVATE MANIPULATORS

    /// Make this instance a copy of the specified `src`, that is copy and
    /// adjust each of its members to represent the same object as the one
    /// from `src`.
    void copyFrom(const PushMessageIterator& src);

    // PRIVATE ACCESSORS

    /// Load into `d_optionsView` a view over the options associated with
    /// the  message currently pointed to by this iterator.  Behavior is
    /// undefined unless latest call to `next()` returned 1.
    void initCachedOptionsView() const;

    /// Load the `d_lazyMessagePayloadPosition` variable to payload for the
    /// message currently pointed to by this iterator.  Return zero on
    /// success, and a non-zero value otherwise.  Behavior is undefined
    /// unless `d_decompressFlag` is true and the latest call to `next()`
    /// returned 1.
    int loadMessagePayloadPosition() const;

    /// Return the size (in bytes) of compressed application data for the
    /// message currently pointed to by this iterator.  Behavior is
    /// undefined unless latest call to `next()` returned 1.  Note that
    /// compressed application data includes compressed message properties
    /// and message payload excluding message padding, and excludes options.
    int compressedApplicationDataSize() const;

  public:
    // CREATORS

    /// Create an invalid instance using the specified `allocator` and
    /// the specified `blobBufferFactory`. Note that the `d_decompressFlag`
    /// is initialized to false. The only valid operations on an invalid
    /// instance are assignment, `reset` and `isValid`.
    PushMessageIterator(bdlbb::BlobBufferFactory* bufferFactory,
                        bslma::Allocator*         allocator);

    /// Initialize a new instance using the specified `blob`, `eventHeader`,
    /// `decompressFlag`, `blobBufferFactory` and `allocator`.  Behavior is
    /// undefined if the `blob` pointer is null, or the pointed-to blob does
    /// not contain enough bytes to fit at least the `eventHeader`.
    PushMessageIterator(const bdlbb::Blob*        blob,
                        const EventHeader&        eventHeader,
                        bool                      decompressFlag,
                        bdlbb::BlobBufferFactory* blobBufferFactory,
                        bslma::Allocator*         allocator);

    /// Copy constructor, from the specified `src`, using the specified
    /// `allocator`.  Needed because BlobObjectProxy doesn't authorize copy
    /// semantic.
    PushMessageIterator(const PushMessageIterator& src,
                        bslma::Allocator*          allocator);

    // MANIPULATORS

    /// Assignment operator from the specified `rhs`.  Needed because
    /// BlobObjectProxy doesn't authorize copy semantic.
    PushMessageIterator& operator=(const PushMessageIterator& rhs);

    /// Advance to the next message.  Return 1 if the new position is valid
    /// and represent a valid message, 0 if iteration has reached the end of
    /// the event, or < 0 if an error was encountered.  Note that if this
    /// method returns 0, this instance goes in an invalid state, and after
    /// that, only valid operations on this instance are assignment, `reset`
    /// and `isValid`.
    int next();

    /// Reset this instance using the specified `blob` and `eventHeader` and
    /// the specified `decompressFlag`. The behaviour is undefined if the
    /// `blob` pointer is null, or the pointed-to blob does not contain
    /// enough bytes to fit at least the `eventHeader`.  Return 0 on
    /// success, and non-zero on error.
    int reset(const bdlbb::Blob* blob,
              const EventHeader& eventHeader,
              bool               decompressFlag);

    /// Point this instance to the specified `blob` using the position and
    /// other meta data from the specified `other` instance.  This method is
    /// useful when it is desired to copy `other` instance into this
    /// instance but the blob being held by `other` instance will not
    /// outlive this instance.  The behavior is undefined unless `blob` is
    /// non-null.  Return 0 on success, and non-zero on error.
    int reset(const bdlbb::Blob* blob, const PushMessageIterator& other);

    /// Set the internal state of this instance to be same as default
    /// constructed, i.e., invalid.
    void clear();

    /// Dump the beginning of the blob associated to this
    /// PushMessageIterator to the specified `stream`.
    void dumpBlob(bsl::ostream& stream);

    // ACCESSORS

    /// Return true if this iterator is initialized and valid, and `next()`
    /// can be called on this instance, or return false in all other cases.
    bool isValid() const;

    /// Return a const reference to the PushHeader currently pointed to by
    /// this iterator.  Behavior is undefined unless `isValid` returns true.
    const PushHeader& header() const;

    /// Return true if the message currently pointed by this iterator has
    /// properties associated with it, false otherwise.  Behavior is
    /// undefined unless `isValid` returns true.
    bool hasMessageProperties() const;

    /// Return `true` if the message currently pointed by this iterator has
    /// a Group Id associated with it, `false` otherwise.  Behavior is
    /// undefined unless `isValid` returns true.
    bool hasMsgGroupId() const;

    /// Return true if the message currently pointed by this iterator has an
    /// implicit application data, which means that message contains only a
    /// PUSH header and options.
    bool isApplicationDataImplicit() const;

    /// Return true if the message currently pointed by this iterator has
    /// options associated with it, false otherwise.  Behavior is undefined
    /// unless `isValid` returns true.
    bool hasOptions() const;

    /// Load into the specified `blob` the options associated with the
    /// message currently under iteration.  Return zero on success, and a
    /// non-zero value otherwise.  Behavior is undefined unless latest call
    /// to `next()` returned 1.  Note that if no options are associated with
    /// the current message, this method will return success.
    int loadOptions(bdlbb::Blob* blob) const;

    /// Return the size (in bytes) of application data for the message
    /// currently pointed to by this iterator.  Behavior is undefined unless
    /// latest call to `next()` returned 1.  Note that when
    /// `d_decompressFlag` is true, application data includes message
    /// properties and message payload without message padding, but excludes
    /// options. Also, when `d_decompressFlag` is false, this function will
    /// return size of compressed application data without padding.
    int applicationDataSize() const;

    /// Load into the specified `position` the position of the application
    /// data for the message currently pointed to by this iterator.  Return
    /// zero on success, non-zero value in case of error or if application
    /// data is implicit.  Behavior is undefined unless latest call to
    /// `next()` returned 1.  Note that application data includes message
    /// properties and message payload, but excludes the options.
    int loadApplicationDataPosition(bmqu::BlobPosition* position) const;

    /// Load into the specified `blob` the application data for the message
    /// currently pointed to by this iterator.  Return zero on success, and
    /// a non-zero value in case of error or if application data is
    /// implicit.  Behavior is undefined unless latest call to `next()`
    /// returned 1.  Note that application data includes message properties
    /// and message payload, but excludes options.
    int loadApplicationData(bdlbb::Blob* blob) const;

    /// Return the size (in bytes) of properties for the message currently
    /// pointed to by this iterator.  Behavior is undefined unless latest
    /// call to `next()` returned 1 and `d_decompress` is true.  Note that
    /// this length includes padding.  Also note that this method returns
    /// zero if no properties are associated with the current message or
    /// application data is implicit.
    int messagePropertiesSize() const;

    /// Load into the specified `blob` the properties for the message
    /// currently pointed to by this iterator.  Return zero on success, and
    /// a non-zero value otherwise (application data is implicit, other
    /// error).  Behavior is undefined unless latest call to `next()`
    /// returned 1.  Note that if current message does not contain any
    /// properties, this method clears out the `blob` and return success
    /// (see implementation note in the method for reasoning behind that).
    /// Also note that word-aligned padding bytes will also be included.
    int loadMessageProperties(bdlbb::Blob* blob) const;

    /// Load into the specified `properties` the properties associated with
    /// the message currently pointed to by this iterator.  Return zero on
    /// success, and a non-zero value otherwise (application data is
    /// implicit, other error).  Behavior is undefined unless latest call to
    /// `next()` returned 1.  Note that this method returns success if no
    /// properties are associated with the current messsage, and
    /// `properties` will be cleared out in that case (see implementation
    /// note in the method for reasoning behind this).
    int loadMessageProperties(MessageProperties* properties) const;

    /// Return the size (in bytes) of payload for the message currently
    /// pointed to by this iterator.  Behavior is undefined unless latest
    /// call to `next()` returned 1 and `d_decompressFlag` is true.  Note
    /// that this method returns zero if application data is implicit.
    int messagePayloadSize() const;

    /// Load into the specified `blob` the payload for the message currently
    /// pointed to by this iterator.  Return zero on success, and a non-zero
    /// value in case of failure or if application data is implicit.
    /// Behavior is undefined unless latest call to `next()` returned 1 and
    /// `d_decompressFlag` is true.
    int loadMessagePayload(bdlbb::Blob* blob) const;

    /// Return the size (in bytes) of options for the message currently
    /// pointed to by this iterator.  Behavior is undefined unless latest
    /// call to `next()` returned 1.  Note that this length includes
    /// padding.  Also note that this method returns zero if no options are
    /// associated with the current message.
    int optionsSize() const;

    /// Load into the specified `view` a view over the options associated
    /// with the message currently pointed to by this iterator.  Return zero
    /// on success, and a non-zero value otherwise.  Behavior is undefined
    /// unless latest call to `next()` returned 1.  Note that this method
    /// returns success and resets the `iter` if no options are present in
    /// the current message.
    int loadOptionsView(OptionsView* view) const;

    /// Load into the specified `queueId` and `rdaInfo` the queueId pair
    /// (id, subId) and RDA counter associated with the message currently
    /// pointed to by this iterator.  Behavior is undefined unless latest
    /// call to `next()` returned 1, and the currently pointed to message
    /// has at most one SubQueueId.
    void extractQueueInfo(int*          queueId,
                          unsigned int* subscriptionId,
                          RdaInfo*      rdaInfo) const;

    /// Load into the specified `msgGroupId` the Group Id associated with
    /// the message currently pointed to by this iterator.  Return `true` if
    /// the load was successful or `false` otherwise.  Behavior is undefined
    /// unless latest call to `next()` returned 1.
    void extractMsgGroupId(bmqp::Protocol::MsgGroupId* msgGroupId) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------------
// class PushMessageIterator
// -------------------------

// CREATORS
inline PushMessageIterator::PushMessageIterator(
    bdlbb::BlobBufferFactory* bufferFactory,
    bslma::Allocator*         allocator)
: d_blobIter(0, bmqu::BlobPosition(), 0, true)
, d_applicationDataSize(-1)
, d_lazyMessagePayloadSize(-1)
, d_lazyMessagePayloadPosition()
, d_messagePropertiesSize(0)
, d_applicationDataPosition()
, d_optionsSize(0)
, d_optionsPosition()
, d_advanceLength(-1)
, d_optionsView(allocator)
, d_decompressFlag(false)
, d_applicationData(bufferFactory, allocator)
, d_bufferFactory_p(bufferFactory)
, d_allocator_p(allocator)
{
    // NOTHING
}

inline PushMessageIterator::PushMessageIterator(
    const bdlbb::Blob*        blob,
    const EventHeader&        eventHeader,
    bool                      decompressFlag,
    bdlbb::BlobBufferFactory* bufferFactory,
    bslma::Allocator*         allocator)
: d_blobIter(0, bmqu::BlobPosition(), 0, true)  // no def ctor - set in reset
, d_optionsView(allocator)
, d_decompressFlag(decompressFlag)
, d_applicationData(bufferFactory, allocator)
, d_bufferFactory_p(bufferFactory)
, d_allocator_p(allocator)
{
    reset(blob, eventHeader, decompressFlag);
}

inline PushMessageIterator::PushMessageIterator(const PushMessageIterator& src,
                                                bslma::Allocator* allocator)
: d_blobIter(0,
             bmqu::BlobPosition(),
             0,
             true)  // no def ctor - set in copyFrom
, d_applicationData(src.d_bufferFactory_p, allocator)
, d_bufferFactory_p(src.d_bufferFactory_p)
, d_allocator_p(allocator)
{
    copyFrom(src);
}

// MANIPULATORS
inline PushMessageIterator&
PushMessageIterator::operator=(const PushMessageIterator& rhs)
{
    if (this != &rhs) {
        copyFrom(rhs);
    }

    return *this;
}

inline void PushMessageIterator::clear()
{
    d_blobIter.reset(0, bmqu::BlobPosition(), 0, true);
    d_header                     = PushHeader();
    d_applicationDataSize        = -1;
    d_lazyMessagePayloadSize     = -1;
    d_lazyMessagePayloadPosition = bmqu::BlobPosition();
    d_messagePropertiesSize      = 0;
    d_applicationDataPosition    = bmqu::BlobPosition();
    d_optionsSize                = 0;
    d_optionsPosition            = bmqu::BlobPosition();
    d_advanceLength              = -1;
    d_applicationData.removeAll();
    d_optionsView.reset();
}

// ACCESSORS
inline bool PushMessageIterator::isValid() const
{
    return (d_advanceLength != -1) && !d_blobIter.atEnd();
}

inline const PushHeader& PushMessageIterator::header() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header;
}

inline bool PushMessageIterator::hasMessageProperties() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return PushHeaderFlagUtil::isSet(header().flags(),
                                     PushHeaderFlags::e_MESSAGE_PROPERTIES);
}

inline bool PushMessageIterator::hasMsgGroupId() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    if (!hasOptions()) {
        return false;  // RETURN
    }

    initCachedOptionsView();

    BSLS_ASSERT_SAFE(!d_optionsView.isNull());

    OptionsView& optionsView = d_optionsView.value();
    BSLS_ASSERT_SAFE(optionsView.isValid());

    return optionsView.find(OptionType::e_MSG_GROUP_ID) != optionsView.end();
}

inline bool PushMessageIterator::isApplicationDataImplicit() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return PushHeaderFlagUtil::isSet(header().flags(),
                                     PushHeaderFlags::e_IMPLICIT_PAYLOAD);
}

inline bool PushMessageIterator::hasOptions() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(
        (d_optionsSize == 0 && d_optionsPosition == bmqu::BlobPosition()) ||
        (d_optionsSize != 0 && d_optionsPosition != bmqu::BlobPosition()));

    return d_optionsSize > 0;
}

inline int PushMessageIterator::messagePropertiesSize() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(d_decompressFlag);

    return d_messagePropertiesSize;
}

inline int PushMessageIterator::optionsSize() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_optionsSize;
}

inline int PushMessageIterator::loadOptionsView(OptionsView* view) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return view->reset(d_blobIter.blob(), d_optionsPosition, d_optionsSize);
}

}  // close package namespace
}  // close enterprise namespace

#endif
