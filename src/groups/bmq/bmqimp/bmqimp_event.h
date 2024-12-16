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

// bmqimp_event.h                                                     -*-C++-*-
#ifndef INCLUDED_BMQIMP_EVENT
#define INCLUDED_BMQIMP_EVENT

//@PURPOSE: Provide a value-semantic type representing an event.
//
//@CLASSES:
//  bmqimp::Event: value-semantic type representing an event
//
//@DESCRIPTION: This component provides a generic 'bmqimp::Event' notification
// object used by the 'bmqimp::Session' to provide BlazingMQ applications with
// information regarding changes in the session status or the result of
// operations with the message queue broker or messages.  There are two types
// of events:
//
//: o SessionEvent: A session event is a notification to indicate the success
//:   or the failure of an operation (openQueue, connection lost with the
//:   broker, ...). This event is composed of 4 attributes: the type of the
//:   sessionEvent, a status code, an optional correlationId (used for async
//:   requests) and an optional errorDescription.
//
//: o MessageEvent: A message event represents messages being received from the
//:   broker. This event is composed of two attributes: a blob (owned by this
//:   object) and a blobPosition pointing to the first bytes of the message in
//:   the blob.
//
/// Done Callback
///-------------
// Events of type 'SessionEvent' can be associated with a 'doneCallback'
// functor, that will be invoked when clear() is called.  The intent is that we
// can leverage this to execute some cleanup operation once the Event has been
// delivered and fully processed by the client (for example, reset the
// correlationId of a Queue once the closeQueue event has been handled).  This
// is leveraging the fact that events delivered to the client ('bmqa::Event')
// are using the pimpl idiom with a shared pointer to the 'bmqimp::Event'; so
// that 'clear()' is only invoked once the client no longer has any reference
// to the event (i.e., the event can be copied and enqueued by the application
// without invoking the done callback too early).
//
/// Event Callback
///-------------
// Events of type 'SessionEvent' can be associated with an 'eventCallback'
// functor that will be invoked at the discretion of the component user
// (usually when an event is popped off of a queue) and provided an event on
// which the callback is meant to operate.  The intent is that we can leverage
// this to execute some custom logic at any point during the lifetime of the
// event.  This contrasts with the 'doneCallback' in that, by intent,
// invocation of the 'eventCallback' can occur at the control of the component
// user, whereas invocation of the 'doneCallback' should typically only occur
// upon cleanup, and only once, via a call to 'clear'.  For example, an
// application user may wait for a synchronous operation (e.g., sync
// 'configureQueue') whose response is then received from the broker and
// meant to be processed only *after* currently pending events.  Hence, the
// 'eventCallback' can be invoked once the event is popped off of the event
// buffer (queue), allowing to synchronize the final notification to the
// application user such that all previously pending events received from the
// broker will have already been processed.

// BMQ

#include <bmqimp_queue.h>
#include <bmqp_ackmessageiterator.h>
#include <bmqp_event.h>
#include <bmqp_eventutil.h>
#include <bmqp_protocol.h>
#include <bmqp_pushmessageiterator.h>
#include <bmqp_puteventbuilder.h>
#include <bmqp_putmessageiterator.h>
#include <bmqp_queueid.h>
#include <bmqt_correlationid.h>
#include <bmqt_messageguid.h>
#include <bmqt_sessioneventtype.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_objectbuffer.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqimp {
class MessageCorrelationIdContainer;
}
namespace bmqimp {
class Queue;
}

namespace bmqimp {

struct SubscriptionId {
    const int          d_queueId;
    const unsigned int d_subscriptionId;

    SubscriptionId(int queueId, unsigned int subscriptionId);
};

// ===========
// class Event
// ===========

/// Value-semantic type representing an event
class Event {
  public:
    // TYPES

    /// Signature of a `void` callback method
    typedef bsl::function<void(void)> VoidFunctor;

    /// Signature of a per-event callback method invoked with the specified
    /// `eventSp`.
    typedef bsl::function<void(const bsl::shared_ptr<Event>& eventSp)>
        EventCallback;

    /// queueId -> queue
    typedef bsl::unordered_map<bmqp::QueueId, bsl::shared_ptr<Queue> >
        QueuesMap;

    typedef bsl::unordered_map<SubscriptionId, bsl::shared_ptr<Queue> >
        QueuesBySubscriptionId;

    /// Enum identifying the type of the Event
    struct EventType {
        // TYPES
        enum Enum {
            e_UNINITIALIZED  // Uninitialized event
            ,
            e_RAW  // Raw event wrapper
            ,
            e_SESSION  // Session event
            ,
            e_MESSAGE  // Message event
            ,
            e_REQUEST  // User request event
        };
    };

    /// Enum identify mode of MessageEvent
    struct MessageEventMode {
        // TYPES
        enum Enum { e_UNINITIALIZED, e_READ, e_WRITE };
    };

  private:
    // PRIVATE TYPES
    typedef bsls::ObjectBuffer<bmqp::PutEventBuilder> PutEventBuilderBuffer;

  private:
    // DATA

    // - - - - - - - - - - - - - - - -
    // Non event specific data members
    bslma::Allocator* d_allocator_p;

    MessageCorrelationIdContainer* d_messageCorrelationIdContainer_p;
    // CorrelationId container
    // NOTE: This data member may or may
    // not be set depending on whether is
    // was generated from inside the
    // eventQueue or from the
    // brokerSession.

    EventType::Enum d_type;
    // The type of this event

    VoidFunctor d_doneCallback;
    // Callback associated with the
    // lifetime of this event; invoked upon
    // clearing or destruction of the event

    EventCallback d_eventCallback;
    // Callback associated with this event
    // and invoked at the discretion of the
    // user (usually when the event is
    // popped off of a queue)

    QueuesMap d_queues;
    // Map of queues associated with this
    // event.  Note that this map is
    // thread-safe to use because it is
    // populated once.

    QueuesBySubscriptionId d_queuesBySubscriptionId;

    bdlbb::BlobBufferFactory* d_bufferFactory_p;

    // - - - - - - - - - - - - - - - - -
    // SessionEvent related data members
    bmqt::SessionEventType::Enum d_sessionEventType;
    // Type of the session event

    int d_statusCode;
    // Status code of the session event

    bmqt::CorrelationId d_correlationId;
    // CorrelationId (if any) associated to this
    // session event

    bsl::string d_errorDescription;
    // Optional description of an error, if
    // statusCode is != 0

    // - - - - - - - - - - - - - - - - -
    // MessageEvent related data members
    MessageEventMode::Enum d_msgEventMode;
    // Are we iterating or building the msg
    // event

    bmqp::Event d_rawEvent;
    // Raw event that holds the blob.

    bmqp::PushMessageIterator d_pushMsgIter;
    // Iterator over the raw event in case raw
    // event is of type PUSH.

    bmqp::AckMessageIterator d_ackMsgIter;
    // Iterator over the raw event in case raw
    // event is of type ACK.

    bmqp::PutMessageIterator d_putMsgIter;
    // Iterator over the raw event in case raw
    // event is of type PUT.

    PutEventBuilderBuffer d_putEventBuilderBuffer;
    // Object buffer for put event builder.

    bool d_isPutEventBuilderConstructed;
    // Flag to indicate if bmqp::PutEventBuilder
    // has been constructed in its buffer.  We
    // cannot rely on 'd_msgEventMode'

    bsl::vector<bsl::pair<bmqt::CorrelationId, unsigned int> >
        d_correlationIds;
    // Ordered list of correlationIds for all
    // the PUT/ACK/PUSH messages in this event.
    // For PUSH messages optional corresponding
    // subscription Ids may be provided.

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Event, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new uninitialized Event using the specified `allocator`.
    Event(bdlbb::BlobBufferFactory* blobBufferFactory,
          bslma::Allocator*         allocator);

    /// Copy constructor from the specified `other` using the specified
    /// `allocator`.  The behavior is undefined *if* `messageEventMode()` of
    /// `other` instance is `MessageEventMode::WRITE`.
    Event(const Event& other, bslma::Allocator* allocator);

    /// Destructor
    ~Event();

    // - - - - - - - - - - - - - - -
    // Non event specific operations
    // MANIPULATORS

    /// Assignment operator from the specified `rhs`. Behavior is undefined
    /// *if* `messageEventMode()` of `rhs` is `MessageEventMode::WRITE`.
    Event& operator=(const Event& rhs);

    /// Resets the iterators associated to this event.
    void resetIterators();

    /// Reset this Event to empty state.
    void reset();

    /// Reset this Event to empty state.  If a `doneCallback` was set and
    /// the event is of type `SessionEvent`, this method will first invoke
    /// that functor before resetting the event.  Used by the
    /// `bdlcc::SharedObjectPool`.
    void clear();

    /// Set the correlationId container associated to this `Event` to the
    /// specified `msgCorrIdContainer`.
    Event& setMessageCorrelationIdContainer(
        MessageCorrelationIdContainer* msgCorrIdContainer);

    /// Configure this instance as a wrapper to the specified `rawEvent`
    /// (that owns the blob) and set the type of this event to be
    /// `RAWEVENT`.
    Event& configureAsRawEvent(const bmqp::Event& rawEvent);

    /// Configure this instance as a user request event, set the type of
    /// this event to be `REQUESTEVENT` and set the associated event
    /// callback to the specified `value`.
    Event& configureAsRequestEvent(const EventCallback& value);

    /// Set the attributes of this Event to the corresponding ones specified
    /// in `sessionEventType` and the optionally specified `statusCode`,
    /// `correlationId` and `errorDescription`.  Also change the type of
    /// this Event to be `SESSIONEVENT`.
    Event& configureAsSessionEvent(
        bmqt::SessionEventType::Enum sessionEventType,
        int                          statusCode       = 0,
        const bmqt::CorrelationId&   correlationId    = bmqt::CorrelationId(),
        const bslstl::StringRef&     errorDescription = "");

    /// Configure this instance as a message event in read mode with the
    /// specified `rawEvent` (that owns the blob), and load the appropriate
    /// message iterator (PUSH, ACK, PUT) depending upon the type of
    /// `rawEvent`. Put message iterators loaded using this will always
    /// decompress the application data. Behavior is undefined unless
    /// `rawEvent` is a PUSH, a ACK or a PUT event.  Also change the type of
    /// this Event to be `MESSAGEEVENT`, and type of message event mode to
    /// `READ`.
    Event& configureAsMessageEvent(const bmqp::Event& rawEvent);

    /// Configure this instance as a message event in write mode with the
    /// specified `blobSpPool_p` to allocate shared pointers to blobs.
    /// Behavior is undefined unless `bufferFactory` points to a valid blob
    /// buffer factory.  Also change the type of this Event to be
    /// `MESSAGEEVENT`, and type of message event mode to `WRITE`.
    Event&
    configureAsMessageEvent(bmqp::BlobPoolUtil::BlobSpPool* blobSpPool_p);

    /// Change the message event mode with which this instance is currently
    /// configured to MessageEventMode::READ *without* *any* modification to
    /// the underlying blob.  This method has no effect if the mode is
    /// `READ` already.  Behavior is undefined unless this instance's
    /// `type()` is MESSAGEVENT.  Note that this method should logically be
    /// thought of as just changing the view on the underlying blob from
    /// write (builder) to read (iterator).
    Event& downgradeMessageEventModeToRead();

    /// Change the message event mode with which this instance is currently
    /// configured to MessageEventMode::WRITE, *and* reset the underlying
    /// blob.  This method has no effect if the mode is already `WRITE`.
    /// Behavior is undefined unless this instance's `type()` is
    /// MESSAGEVENT.  Note that this method should logically be thought of
    /// as resetting this instance as a message event in write mode.
    Event& upgradeMessageEventModeToWrite();

    // ACCESSORS

    /// Get a pointer to the correlationId container.
    MessageCorrelationIdContainer* messageCorrelationIdContainer() const;

    /// Get the type of this Event.
    EventType::Enum type() const;

    // MANIPULATORS

    /// Set the type of this Event to the specified `value`.
    Event& setType(EventType::Enum value);

    /// Set the done callback to invoke when this event is returned to the
    /// object pool to the specified `value` (read the `Done Callback`
    /// section in this component-level documentation for more information.
    /// Return a reference offering modifiable access to this object.
    Event& setDoneCallback(const VoidFunctor& value);

    /// Set the event callback associated with this event to the specified
    /// `value` (read the `Event Callback` section in this component-level
    /// documentation for more information).  Return a reference offering
    /// modifiable access to this object.
    Event& setEventCallback(const EventCallback& value);

    /// Insert the specified `queue` to the queues associated with this
    /// event.  Return a reference offering modifiable access to this
    /// object.  The behavior is undefined unless `queue` is a valid object.
    Event& insertQueue(const bsl::shared_ptr<Queue>& queue);
    Event& insertQueue(unsigned int                  subscriptionId,
                       const bsl::shared_ptr<Queue>& queue);

    // ACCESSORS

    /// Return the done callback associated to this event.
    const VoidFunctor& doneCallback() const;

    /// Return the event callback associated to this event.
    const EventCallback& eventCallback() const;

    const bsl::shared_ptr<Queue> lookupQueue() const;

    /// Return a reference not offering modifiable access to the map of
    /// queues associated with this event.  The returned map is thread-safe
    /// to use because it is populated once.
    const QueuesMap& queues() const;

    // - - - - - - - - - - - - - - - -
    // SessionEvent specific operations
    // ACCESSORS

    /// Return the type of the Session Event. The behavior is undefined if
    /// the event is not of type `SessionEvent`.
    bmqt::SessionEventType::Enum sessionEventType() const;

    /// Return the correlationId associated to this event, if any. The
    /// behavior is undefined if the event is not of type `SessionEvent`.
    const bmqt::CorrelationId& correlationId() const;

    /// Return the status code that indicates success or the cause of a
    /// failure.  The behavior is undefined if the event is not of type
    /// `SessionEvent`.
    int statusCode() const;

    /// Return a printable description of the error, if `statusCode` returns
    /// non-zero.  Return an empty string otherwise.  The behavior is
    /// undefined if the event is not of type `SessionEvent`.
    const bsl::string& errorDescription() const;

    // MANIPULATORS

    /// Set the type of the event to the specified `value`.  The behavior is
    /// undefined if the event is not of type `SessionEvent`.
    Event& setSessionEventType(bmqt::SessionEventType::Enum value);

    /// Set the correlationId of the event to the specified `value`.  The
    /// behavior is undefined if the event is not of type `SessionEvent`.
    Event& setCorrelationId(const bmqt::CorrelationId& value);

    /// Set the statusCode of the event to the specified `value`.  The
    /// behavior is undefined if the event is not of type `SessionEvent`.
    Event& setStatusCode(int value);

    /// Set the errorDescription of the event to the specified `value`.  The
    /// behavior is undefined if the event is not of type `SessionEvent`.
    Event& setErrorDescription(const bslstl::StringRef& value);

    // - - - - - - - - - - - - - - - -
    // MessageEvent specific operations
    // ACCESSORS

    /// Return the mode of the Message Event. The behavior is undefined if
    /// the event is not of type `MessageEvent`.
    MessageEventMode::Enum messageEventMode() const;

    /// Return the raw protocol event this instance is configured with.
    /// Behavior is undefined unless event's `type()` is MESSAGEVENT or
    /// RAWEVENT.
    const bmqp::Event& rawEvent() const;

    /// Return the number of correlationIds maintained by this instance.
    /// Behavior is undefined unless 0 <= `position` < numCorrrelationIds(),
    /// and event's type() is MESSAGEEVENT, `messageEventMode()` is READ and
    /// the underlying raw event is of type ACK.
    int numCorrrelationIds() const;

    /// Return the correlationId at the specified `position`. Behavior is
    /// undefined unless 0 <= `position` < numCorrrelationIds(), and event's
    /// type() is MESSAGEEVENT, `messageEventMode()` is READ and the
    /// underlying raw event is of type ACK, PUT or PUSH.
    const bmqt::CorrelationId& correlationId(int position) const;

    /// Return the subscriptionId at the specified 'position'. Behavior is
    /// undefined unless 0 <= 'position' < numCorrrelationIds(), and event's
    /// type() is MESSAGEEVENT, 'messageEventMode()' is READ and the
    /// underlying raw event is of type PUSH.
    unsigned int subscriptionId(int position) const;

    // MANIPULATORS

    /// Behavior is undefined unless event's `type()` is MESSAGEVENT,
    /// `messageEventMode()` is READ, and the underlying raw event is of
    /// type PUSH.
    bmqp::PushMessageIterator* pushMessageIterator();

    /// Behavior is undefined unless event's `type()` is MESSAGEVENT,
    /// `messageEventMode()` is READ, and the underlying raw event is of
    /// type ACK.
    bmqp::AckMessageIterator* ackMessageIterator();

    /// Behavior is undefined unless event's `type()` is MESSAGEVENT,
    /// `messageEventMode()` is READ, and the underlying raw event is of
    /// type PUT.
    bmqp::PutMessageIterator* putMessageIterator();

    /// Behavior is undefined unless event's `type()` is MESSAGEVENT, and
    /// `messageEventType()` is WRITE.
    bmqp::PutEventBuilder* putEventBuilder();

    /// Add the specified 'correlationId' and the optionally specified
    /// 'subscriptionId' to the list of correlationId-subscriptionId pairs
    /// maintained by this instance.  The behavior is undefined unless
    /// event's type() is MESSAGEEVENT, 'messageEventMode()' is READ and the
    /// underlying raw event is of type ACK, PUT or PUSH.
    void addCorrelationId(const bmqt::CorrelationId& correlationId,
                          unsigned int               subscriptionHandleId =
                              bmqt::SubscriptionHandle::k_INVALID_HANDLE_ID);

    /// Insert the specified `queue` to the queues and the specified
    /// `corrId` to the list of correlationIds associated with this event.
    /// If the `corrId` is not empty associate it with the specified `guid`
    /// in the correlationId container.
    void addMessageInfo(const bsl::shared_ptr<bmqimp::Queue>& queue,
                        const bmqt::MessageGUID&              guid,
                        const bmqt::CorrelationId&            corrId);

    // - - - - - - - - -
    // Utility functions

    // ACCESSORS

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

  private:
    /// Lookup the queue with the specified `queueId` and return a shared
    /// pointer to the Queue objects (if found), or an empty shared pointer
    /// (if not found).
    const bsl::shared_ptr<Queue>
    lookupQueue(const bmqp::QueueId& queueId) const;
    const bsl::shared_ptr<Queue>
    lookupQueue(int queueId, unsigned int subscriptionId) const;
};

// FREE OPERATORS

/// Return `true` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return false otherwise.  Return
/// `false` if one of objects has `d_type` equal `e_MESSAGEEVENT`.
bool operator==(const Event& lhs, const Event& rhs);

/// Return `false` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return `true` otherwise.
bool operator!=(const Event& lhs, const Event& rhs);

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const Event& rhs);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// struct SubscriptionId
// ---------------------

inline SubscriptionId::SubscriptionId(int queueId, unsigned int subscriptionId)
: d_queueId(queueId)
, d_subscriptionId(subscriptionId)
{
    // NOTHING
}

// FREE FUNCTIONS

/// Apply the specified `hashAlgo` to the specified `queueId`.
template <class HASH_ALGORITHM>
void hashAppend(HASH_ALGORITHM& hashAlgo, const SubscriptionId& id)
{
    using bslh::hashAppend;  // for ADL
    hashAppend(hashAlgo, id.d_queueId);
    hashAppend(hashAlgo, id.d_subscriptionId);
}

// FREE OPERATORS
inline bool operator==(const SubscriptionId& lhs, const SubscriptionId& rhs)
{
    return lhs.d_queueId == rhs.d_queueId &&
           lhs.d_subscriptionId == rhs.d_subscriptionId;
}

// -----------
// class Event
// -----------

inline Event& Event::setMessageCorrelationIdContainer(
    MessageCorrelationIdContainer* msgCorrIdCont)
{
    d_messageCorrelationIdContainer_p = msgCorrIdCont;
    return *this;
}

inline MessageCorrelationIdContainer*
Event::messageCorrelationIdContainer() const
{
    return d_messageCorrelationIdContainer_p;
}

inline Event::EventType::Enum Event::type() const
{
    return d_type;
}

inline Event& Event::setType(Event::EventType::Enum value)
{
    d_type = value;
    return *this;
}

inline Event& Event::setDoneCallback(const VoidFunctor& value)
{
    d_doneCallback = value;
    return *this;
}

inline const Event::VoidFunctor& Event::doneCallback() const
{
    return d_doneCallback;
}

inline Event& Event::setEventCallback(const EventCallback& value)
{
    d_eventCallback = value;
    return *this;
}

inline const Event::EventCallback& Event::eventCallback() const
{
    return d_eventCallback;
}

inline const Event::QueuesMap& Event::queues() const
{
    return d_queues;
}

inline bmqt::SessionEventType::Enum Event::sessionEventType() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_SESSION);

    return d_sessionEventType;
}

inline const bmqt::CorrelationId& Event::correlationId() const
{
    return d_correlationId;
}

inline int Event::statusCode() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_SESSION);

    return d_statusCode;
}

inline const bsl::string& Event::errorDescription() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_SESSION);

    return d_errorDescription;
}

inline Event& Event::setSessionEventType(bmqt::SessionEventType::Enum value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_SESSION);

    d_sessionEventType = value;
    return *this;
}

inline Event& Event::setCorrelationId(const bmqt::CorrelationId& value)
{
    d_correlationId = value;
    return *this;
}

inline Event& Event::setStatusCode(int value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_SESSION);

    d_statusCode = value;
    return *this;
}

inline Event& Event::setErrorDescription(const bslstl::StringRef& value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_SESSION);

    d_errorDescription = value;
    return *this;
}

inline Event::MessageEventMode::Enum Event::messageEventMode() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_MESSAGE);

    return d_msgEventMode;
}

inline const bmqp::Event& Event::rawEvent() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_MESSAGE ||
                     type() == EventType::e_RAW);

    return d_rawEvent;
}

inline int Event::numCorrrelationIds() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_MESSAGE);
    BSLS_ASSERT_SAFE(messageEventMode() == MessageEventMode::e_READ);
    BSLS_ASSERT_SAFE(d_rawEvent.isAckEvent() || d_rawEvent.isPutEvent() ||
                     d_rawEvent.isPushEvent());

    return static_cast<int>(d_correlationIds.size());
}

inline const bmqt::CorrelationId& Event::correlationId(int position) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_MESSAGE);
    BSLS_ASSERT_SAFE(messageEventMode() == MessageEventMode::e_READ);
    BSLS_ASSERT_SAFE(d_rawEvent.isAckEvent() || d_rawEvent.isPutEvent() ||
                     d_rawEvent.isPushEvent());
    BSLS_ASSERT_SAFE(0 <= position);
    BSLS_ASSERT_SAFE(static_cast<int>(d_correlationIds.size()) > position);

    return d_correlationIds[position].first;
}

inline unsigned int Event::subscriptionId(int position) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_MESSAGE);
    BSLS_ASSERT_SAFE(messageEventMode() == MessageEventMode::e_READ);
    BSLS_ASSERT_SAFE(d_rawEvent.isPushEvent());
    BSLS_ASSERT_SAFE(0 <= position);
    BSLS_ASSERT_SAFE(static_cast<int>(d_correlationIds.size()) > position);

    return d_correlationIds[position].second;
}

inline bmqp::PushMessageIterator* Event::pushMessageIterator()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_MESSAGE);
    BSLS_ASSERT_SAFE(messageEventMode() == MessageEventMode::e_READ);
    BSLS_ASSERT_SAFE(d_rawEvent.isPushEvent());

    return &d_pushMsgIter;
}

inline bmqp::AckMessageIterator* Event::ackMessageIterator()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_MESSAGE);
    BSLS_ASSERT_SAFE(messageEventMode() == MessageEventMode::e_READ);
    BSLS_ASSERT_SAFE(d_rawEvent.isAckEvent());

    return &d_ackMsgIter;
}

inline bmqp::PutMessageIterator* Event::putMessageIterator()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_MESSAGE);
    BSLS_ASSERT_SAFE(messageEventMode() == MessageEventMode::e_READ);
    BSLS_ASSERT_SAFE(d_rawEvent.isPutEvent());

    return &d_putMsgIter;
}

inline bmqp::PutEventBuilder* Event::putEventBuilder()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_MESSAGE);
    BSLS_ASSERT_SAFE(messageEventMode() == MessageEventMode::e_WRITE);

    return &(d_putEventBuilderBuffer.object());
}

inline void Event::addCorrelationId(const bmqt::CorrelationId& correlationId,
                                    unsigned int subscriptionHandleId)
{
    // TODO: when ACK event is created locally we have to fill d_correlationIds
    //       before the raw ACK 'bmqp::Event' is created and may be used to
    //       call 'configureAsMessageEvent'.  Deactivate below asserts to allow
    //       adding the correlationIds to the uninitialized event.
    //
    // PRECONDITIONS
    // BSLS_ASSERT_SAFE(type()             == EventType::e_MESSAGE);
    // BSLS_ASSERT_SAFE(messageEventMode() == MessageEventMode::e_READ);
    // BSLS_ASSERT_SAFE(d_rawEvent.isAckEvent());

    d_correlationIds.push_back(
        bsl::make_pair(correlationId, subscriptionHandleId));
}

}  // close package namespace

// ------------------
// class SessionEvent
// ------------------

inline bool bmqimp::operator==(const bmqimp::Event& lhs,
                               const bmqimp::Event& rhs)
{
    if (lhs.type() != rhs.type()) {
        return false;  // RETURN
    }

    if (lhs.type() == Event::EventType::e_UNINITIALIZED) {
        return true;  // RETURN
    }

    if (lhs.type() == Event::EventType::e_SESSION) {
        return lhs.sessionEventType() == rhs.sessionEventType() &&
               lhs.statusCode() == rhs.statusCode() &&
               lhs.correlationId() == rhs.correlationId() &&
               lhs.queues() == rhs.queues() &&
               lhs.errorDescription() == rhs.errorDescription();  // RETURN
    }

    return false;
}

inline bool bmqimp::operator!=(const bmqimp::Event& lhs,
                               const bmqimp::Event& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream& bmqimp::operator<<(bsl::ostream&        stream,
                                        const bmqimp::Event& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
