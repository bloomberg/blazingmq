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

// bmqp_event.h                                                       -*-C++-*-
#ifndef INCLUDED_BMQP_EVENT
#define INCLUDED_BMQP_EVENT

//@PURPOSE: Provide a mechanism to access messages from a raw BlazingMQ event
// packet.
//
//@CLASSES:
//  bmqp::Event: mechanism for accessing messages from a raw BlazingMQ event
//               packet.
//
//@DESCRIPTION: 'bmqp::Event' component provides a mechanism to access messages
// from a BlazingMQ event packet, as specified in the BlazingMQ  wire protocol.
//

// BMQ

#include <bmqp_ackmessageiterator.h>
#include <bmqp_confirmmessageiterator.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqp_pushmessageiterator.h>
#include <bmqp_putmessageiterator.h>
#include <bmqp_recoverymessageiterator.h>
#include <bmqp_rejectmessageiterator.h>
#include <bmqp_storagemessageiterator.h>

#include <bmqu_blob.h>
#include <bmqu_blobobjectproxy.h>
#include <bmqu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bsl_iosfwd.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_movableref.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqp {

// ===========
// class Event
// ===========

/// This class provides a mechanism for accessing messages from a raw BMQ
/// event packet.
class Event {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQP.EVENT");

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use by this object.

    bsl::shared_ptr<const bdlbb::Blob> d_clonedBlob_sp;
    // SharedPtr that might be pointing
    // to a blob (in case of cloned
    // event).

    const bdlbb::Blob* d_blob_p;
    // Raw pointer that always points to
    // the blob represented by this event
    // (as specified upon construction).

    bmqu::BlobObjectProxy<EventHeader> d_header;
    // Event Header from this blob (or
    // unset if the blob/event is
    // invalid).

  private:
    // PRIVATE MANIPULATORS

    /// Initialize this instance with the specified `blob`. If the specified
    /// `clone` is true, the event will copy the blob to its internal shared
    /// pointer and take ownership of that copy; otherwise the event is only
    /// valid from within the scope of the `blob`.
    void initialize(const bdlbb::Blob* blob, bool clone);

    // PRIVATE ACCESSORS

    /// Load into the specified `message`, the decoded message contained in
    /// this event.  The behavior is undefined unless `isControlEvent()`,
    /// `isElectorEvent()`, or `isAuthenticationEvent()` returns true.  Return
    /// 0 on success, and a non-zero return code on error.
    template <class TYPE>
    int loadSchemaEvent(TYPE* message) const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Event, bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Create a `bmqp::Event` instance that does not refer to any blob,
    /// and is not valid (`isValid()` returns false), using the specified
    /// `allocator`.  This instance can be made refer to a blob by calling
    /// `reset()` or by assigning another instance to it.
    explicit Event(bslma::Allocator* allocator);

    /// Create a new `bmqp::Event` instance from the specified `blob` and
    /// using the specified `allocator`.  If the optionally specified
    /// `clone` is true, this object will clone the blob into its internal
    /// shared pointer.
    Event(const bdlbb::Blob* blob,
          bslma::Allocator*  allocator,
          bool               clone = false);

    /// Copy constructor of the specified `src` event, using the optionally
    /// specified `allocator`.
    Event(const Event& src, bslma::Allocator* allocator = 0);

    /// Create a new `bmqp::Event` instance having the same contents as the
    /// specified `original` object, leaving `original` in a valid but
    /// unspecified state.  Optionally specify an `allocator` used to supply
    /// memory.  If `allocator` is 0, the default memory allocator is used.
    Event(bslmf::MovableRef<Event> src, bslma::Allocator* allocator = 0);

    // MANIPULATORS

    /// Assignment operator of the specified `rhs`.
    Event& operator=(const Event& rhs);

    /// Replace the contents of `*this` with those of `rhs`, leaving `rhs` in a
    /// valid but unspecified state.  Return `*this`.
    Event& operator=(bslmf::MovableRef<Event> rhs);

    /// Reset this Event to use the specified `blob`.  If the optionally
    /// specified `clone` is true, this object will clone the blob into its
    /// internal shared pointer.
    void reset(const bdlbb::Blob* blob, bool clone = false);

    /// Set internal state of this instance as if it is default constructed.
    void clear();

    // ACCESSORS

    /// Return true is this Event is initialized with a valid blob.
    bool isValid() const;

    /// Return true if this instance, or the instance from which this
    /// instance was initialized (copy-constructed), was cloned.  Return
    /// false in all other cases.
    bool isCloned() const;

    /// Return a new Event, that is a clone of this event, using the
    /// specified `allocator`.  The behavior is undefined unless `isValid()`
    /// returns true.
    Event clone(bslma::Allocator* allocator) const;

    /// Return the encoding type of this Authentication event.  The
    /// behavior is undefined unless `isAuthenticationEvent()` returns true.
    EncodingType::Enum authenticationEventEncodingType() const;

    /// Return the type of this event.  The behavior is undefined unless
    /// `isValid()` returns true.
    EventType::Enum type() const;

    /// Return true if this event is of the corresponding type.  The
    /// behavior is undefined unless `isValid()` returns true.
    bool isControlEvent() const;
    bool isPutEvent() const;
    bool isConfirmEvent() const;
    bool isPushEvent() const;
    bool isAckEvent() const;
    bool isClusterStateEvent() const;
    bool isElectorEvent() const;
    bool isStorageEvent() const;
    bool isRecoveryEvent() const;
    bool isPartitionSyncEvent() const;
    bool isHeartbeatReqEvent() const;
    bool isHeartbeatRspEvent() const;
    bool isRejectEvent() const;
    bool isReceiptEvent() const;
    bool isAuthenticationEvent() const;

    /// Load into the specified `message`, the decoded message contained in
    /// this event.  The behavior is undefined unless `isControlEvent()`
    /// returns true.  Return 0 on success, and a non-zero return code on
    /// error.
    template <class TYPE>
    int loadControlEvent(TYPE* message) const;

    /// Load into the specified `message`, the decoded message contained in
    /// this event.  The behavior is undefined unless `isElectorEvent()`
    /// returns true.  Return 0 on success, and a non-zero return code on
    /// error.
    template <class TYPE>
    int loadElectorEvent(TYPE* message) const;

    /// Load into the specified `message`, the decoded message contained in
    /// this event.  The behavior is undefined unless `isAuthenticationEvent()`
    /// returns true.  Return 0 on success, and a non-zero return code on
    /// error.
    template <class TYPE>
    int loadAuthenticationEvent(TYPE* message) const;

    void loadAckMessageIterator(AckMessageIterator* iterator) const;
    void loadConfirmMessageIterator(ConfirmMessageIterator* iterator) const;
    void loadPushMessageIterator(PushMessageIterator* iterator,
                                 bool decompressFlag = false) const;
    void loadPutMessageIterator(PutMessageIterator* iterator,
                                bool decompressFlag = false) const;
    void loadStorageMessageIterator(StorageMessageIterator* iterator) const;
    void loadRecoveryMessageIterator(RecoveryMessageIterator* iterator) const;

    /// Load into the specified `iterator` an iterator suitable for the type
    /// represented by this event.  Note that the output `iterator` may or
    /// may not be in valid state, as indicated by a call to `isValid` on
    /// the `iterator`.  The behavior is undefined unless the event is of
    /// the corresponding type.
    void loadRejectMessageIterator(RejectMessageIterator* iterator) const;

    /// Return a pointer to the blob associated to this event.
    const bdlbb::Blob* blob() const;

    /// Write the string representation of the specified `event` to the
    /// specified output `stream`, and return a reference to `stream`.
    /// Optionally specify an initial indentation `level`, whose absolute
    /// value is incremented recursively for nested objects.  If `level` is
    /// specified, optionally specify `spacesPerLevel`, whose absolute value
    /// indicates the number of spaces per indentation level for this and
    /// all of its nested objects.  If `level` is negative, suppress
    /// indentation of the first line.  If `spacesPerLevel` is negative,
    /// format the entire output on one line, suppressing all but the
    /// initial indentation (as governed by `level`).
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Format the specified `event` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const Event& event);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------
// class Event
// -----------

inline void Event::initialize(const bdlbb::Blob* blob, bool clone)
{
    d_blob_p = blob;

    if (clone) {
        d_clonedBlob_sp.createInplace(d_allocator_p, *d_blob_p, d_allocator_p);
        d_blob_p = d_clonedBlob_sp.get();
    }

    // Read EventHeader, supporting protocol evolution by reading up to size of
    // the struct bytes (-1 parameter), and then resizing the proxy to match
    // the size declared in the header
    d_header.reset(d_blob_p,
                   bmqu::BlobPosition(),
                   -EventHeader::k_MIN_HEADER_SIZE,
                   true,
                   false);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_header.isSet())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Failed to read required minimum worth of EventHeader
        return;  // RETURN
    }

    const int headerSize = d_header->headerWords() * Protocol::k_WORD_SIZE;
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            headerSize < EventHeader::k_MIN_HEADER_SIZE)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Header is declaring less bytes than expected, probably this is
        // because header is malformed.
        // Explicitly reset the proxy so that isValid() returns false.
        d_header.reset();
        return;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(headerSize >
                                              d_blob_p->length())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Header is declaring more bytes than the blob actually has.
        // Explicitly reset the proxy so that isValid() returns false.
        d_header.reset();
        return;  // RETURN
    }

    d_header.resize(headerSize);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_header.isSet())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BSLS_ASSERT_SAFE(false && "Invalid event");
        // This should never happen
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(d_header->length() == blob->length());

    // Perform some validation:
    //:  o the fragment bit must not be set on a valid EventHeader
    //:  o the type must not be undefined
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_header->fragmentBit() != 0 ||
                                              d_header->type() ==
                                                  EventType::e_UNDEFINED)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Invalidate the header
        d_header.reset();
        return;  // RETURN
    }
}

template <class TYPE>
int Event::loadSchemaEvent(TYPE* message) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isControlEvent() || isElectorEvent() ||
                     isAuthenticationEvent());

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isValid())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // No need to assert 'isValid', this is already captured by the above
        // preconditions check.
        return -1;  // RETURN
    }

    EncodingType::Enum encodingType = EncodingType::e_BER;
    if (d_header->type() == EventType::e_CONTROL ||
        d_header->type() == EventType::e_AUTHENTICATION) {
        encodingType = EventHeaderUtil::encodingType(*d_header);
    }

    bmqu::MemOutStream os;
    int                rc = ProtocolUtil::decodeMessage(os,
                                         message,
                                         *d_blob_p,
                                         d_header->headerWords() *
                                             Protocol::k_WORD_SIZE,
                                         // Skip the EventHeader
                                         encodingType,
                                         d_allocator_p);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!os.isEmpty())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_DEBUG << os.str();
    }

    return rc;
}

inline Event::Event(bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_clonedBlob_sp(0, allocator)
, d_blob_p(0)
{
    // NOTHING
}

inline Event::Event(const bdlbb::Blob* blob,
                    bslma::Allocator*  allocator,
                    bool               clone)
: d_allocator_p(allocator)
, d_clonedBlob_sp(0, allocator)
, d_blob_p(0)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_allocator_p);
    BSLS_ASSERT_SAFE(blob);

    initialize(blob, clone);
}

inline Event::Event(const Event& src, bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_clonedBlob_sp(0, allocator)
, d_blob_p(0)
{
    d_clonedBlob_sp = src.d_clonedBlob_sp;  // src could be a clone
    initialize(src.d_blob_p, false);
}

inline Event::Event(bslmf::MovableRef<Event> src, bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_clonedBlob_sp(0, allocator)
, d_blob_p(0)
{
    d_clonedBlob_sp = bslmf::MovableRefUtil::access(src).d_clonedBlob_sp;
    initialize(bslmf::MovableRefUtil::access(src).d_blob_p, false);
}

inline Event& Event::operator=(const Event& rhs)
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(this == &rhs)) {
        // Return early on self-assignment.
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return *this;
    }

    d_clonedBlob_sp = rhs.d_clonedBlob_sp;  // src could be a clone
    initialize(rhs.d_blob_p, false);
    return *this;
}

inline Event& Event::operator=(bslmf::MovableRef<Event> rhs)
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            this == &bslmf::MovableRefUtil::access(rhs))) {
        // Return early on self-assignment.
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return *this;
    }

    d_clonedBlob_sp = bslmf::MovableRefUtil::access(rhs).d_clonedBlob_sp;
    initialize(bslmf::MovableRefUtil::access(rhs).d_blob_p, false);
    return *this;
}

inline void Event::reset(const bdlbb::Blob* blob, bool clone)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(blob);

    d_clonedBlob_sp.reset();
    initialize(blob, clone);
}

inline void Event::clear()
{
    d_clonedBlob_sp.reset();
    d_blob_p = 0;
    d_header.reset();
}

inline bool Event::isValid() const
{
    return d_header.isSet();
}

inline bool Event::isCloned() const
{
    return d_clonedBlob_sp.get() != 0;
}

inline Event Event::clone(bslma::Allocator* allocator) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(allocator);

    return Event(d_blob_p, allocator, true /* clone == true */);
}

inline EncodingType::Enum Event::authenticationEventEncodingType() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isAuthenticationEvent());

    return EventHeaderUtil::encodingType(*d_header);
}

inline EventType::Enum Event::type() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header->type();
}

inline bool Event::isControlEvent() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header->type() == EventType::e_CONTROL;
}

inline bool Event::isPutEvent() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header->type() == EventType::e_PUT;
}

inline bool Event::isConfirmEvent() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header->type() == EventType::e_CONFIRM;
}

inline bool Event::isPushEvent() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header->type() == EventType::e_PUSH;
}

inline bool Event::isAckEvent() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header->type() == EventType::e_ACK;
}

inline bool Event::isClusterStateEvent() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header->type() == EventType::e_CLUSTER_STATE;
}

inline bool Event::isElectorEvent() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header->type() == EventType::e_ELECTOR;
}

inline bool Event::isStorageEvent() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header->type() == EventType::e_STORAGE;
}

inline bool Event::isRecoveryEvent() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header->type() == EventType::e_RECOVERY;
}

inline bool Event::isPartitionSyncEvent() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header->type() == EventType::e_PARTITION_SYNC;
}

inline bool Event::isHeartbeatReqEvent() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header->type() == EventType::e_HEARTBEAT_REQ;
}

inline bool Event::isHeartbeatRspEvent() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header->type() == EventType::e_HEARTBEAT_RSP;
}

inline bool Event::isRejectEvent() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header->type() == EventType::e_REJECT;
}

inline bool Event::isReceiptEvent() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header->type() == EventType::e_REPLICATION_RECEIPT;
}

inline bool Event::isAuthenticationEvent() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header->type() == EventType::e_AUTHENTICATION;
}

template <class TYPE>
int Event::loadControlEvent(TYPE* message) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isControlEvent());

    return loadSchemaEvent(message);
}

template <class TYPE>
int Event::loadElectorEvent(TYPE* message) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isElectorEvent());

    return loadSchemaEvent(message);
}

template <class TYPE>
int Event::loadAuthenticationEvent(TYPE* message) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isAuthenticationEvent());

    return loadSchemaEvent(message);
}

inline void Event::loadAckMessageIterator(AckMessageIterator* iterator) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isAckEvent());
    BSLS_ASSERT_SAFE(isValid());

    iterator->reset(d_blob_p, *d_header);
}

inline void
Event::loadConfirmMessageIterator(ConfirmMessageIterator* iterator) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isConfirmEvent());
    BSLS_ASSERT_SAFE(isValid());

    iterator->reset(d_blob_p, *d_header);
}

inline void Event::loadPushMessageIterator(PushMessageIterator* iterator,
                                           bool decompressFlag) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isPushEvent());
    BSLS_ASSERT_SAFE(isValid());

    iterator->reset(d_blob_p, *d_header, decompressFlag);
}

inline void Event::loadPutMessageIterator(PutMessageIterator* iterator,
                                          bool decompressFlag) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isPutEvent());
    BSLS_ASSERT_SAFE(isValid());

    iterator->reset(d_blob_p, *d_header, decompressFlag);
}

inline void
Event::loadStorageMessageIterator(StorageMessageIterator* iterator) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isStorageEvent() || isPartitionSyncEvent());
    BSLS_ASSERT_SAFE(isValid());

    iterator->reset(d_blob_p, *d_header);
}

inline void
Event::loadRecoveryMessageIterator(RecoveryMessageIterator* iterator) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isRecoveryEvent());
    BSLS_ASSERT_SAFE(isValid());

    iterator->reset(d_blob_p, *d_header);
}

inline void
Event::loadRejectMessageIterator(RejectMessageIterator* iterator) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isRejectEvent());
    BSLS_ASSERT_SAFE(isValid());

    iterator->reset(d_blob_p, *d_header);
}

inline const bdlbb::Blob* Event::blob() const
{
    return d_blob_p;
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& bmqp::operator<<(bsl::ostream&      stream,
                                      const bmqp::Event& event)
{
    return event.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
