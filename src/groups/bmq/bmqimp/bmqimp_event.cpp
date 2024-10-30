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

// bmqimp_event.cpp                                                   -*-C++-*-
#include <bmqimp_event.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqimp_messagecorrelationidcontainer.h>
#include <bmqimp_queue.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_ostream.h>
#include <bslim_printer.h>
#include <bslma_allocator.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqimp {

// -----------
// class Event
// -----------

// CREATORS
Event::Event(bdlbb::BlobBufferFactory* blobBufferFactory,
             bslma::Allocator*         allocator)
: d_allocator_p(allocator)
, d_messageCorrelationIdContainer_p(0)
, d_type(EventType::e_UNINITIALIZED)
, d_doneCallback(bsl::allocator_arg, allocator)
, d_eventCallback(bsl::allocator_arg, allocator)
, d_queues(allocator)
, d_queuesBySubscriptionId(allocator)
, d_bufferFactory_p(blobBufferFactory)
, d_sessionEventType(bmqt::SessionEventType::e_UNDEFINED)
, d_statusCode(0)
, d_correlationId()
, d_errorDescription(allocator)
, d_msgEventMode(MessageEventMode::e_UNINITIALIZED)
, d_rawEvent(allocator)
, d_pushMsgIter(blobBufferFactory, allocator)
, d_ackMsgIter()
, d_putMsgIter(blobBufferFactory, allocator)
, d_putEventBuilderBuffer()
, d_isPutEventBuilderConstructed(false)
, d_correlationIds(allocator)

{
    // NOTHING
}

Event::Event(const Event& other, bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_messageCorrelationIdContainer_p(other.d_messageCorrelationIdContainer_p)
, d_type(other.d_type)
, d_doneCallback(bsl::allocator_arg, allocator, other.d_doneCallback)
, d_eventCallback(bsl::allocator_arg, allocator, other.d_eventCallback)
, d_queues(other.d_queues, allocator)
, d_queuesBySubscriptionId(other.d_queuesBySubscriptionId, allocator)
, d_bufferFactory_p(other.d_bufferFactory_p)
, d_sessionEventType(other.d_sessionEventType)
, d_statusCode(other.d_statusCode)
, d_correlationId(other.d_correlationId)
, d_errorDescription(other.d_errorDescription, allocator)
, d_msgEventMode(other.d_msgEventMode)
, d_rawEvent(allocator)
, d_pushMsgIter(other.d_bufferFactory_p, allocator)
, d_ackMsgIter()
, d_putMsgIter(other.d_bufferFactory_p, allocator)
, d_putEventBuilderBuffer()
, d_isPutEventBuilderConstructed(false)
, d_correlationIds(other.d_correlationIds, allocator)

{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(EventType::e_MESSAGE != other.d_type ||
                    MessageEventMode::e_WRITE != other.d_msgEventMode);
    // Per contract, it is undefined behavior if 'other' instance is in
    // MessageEventMode::e_WRITE.  This is enforced because logically, it
    // doesn't make sense to have two bmqimp::Events to be writing to
    // underlying blob.  This is also the reason why PutEventBuilder is not
    // constructed in d_putEventBuilderBuffer.

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_type ==
                                              EventType::e_SESSION)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    if (other.d_rawEvent.isCloned()) {
        // Other raw (bmqp) events coming from IO thread are cloned before
        // they are enqueued to event queue.  This case handles that.
        d_rawEvent    = other.d_rawEvent;
        d_pushMsgIter = other.d_pushMsgIter;
        d_ackMsgIter  = other.d_ackMsgIter;
        d_putMsgIter  = other.d_putMsgIter;
    }
    else {
        // A raw (bmqp) event may not be cloned created (indirectly) via
        // bmqa.Session.loadMessageEventBuilder.  In this case, clone it, and
        // preserve state of iterators.
        d_rawEvent = other.d_rawEvent.clone(d_allocator_p);
        d_pushMsgIter.reset(d_rawEvent.blob(), other.d_pushMsgIter);
        d_ackMsgIter.reset(d_rawEvent.blob(), other.d_ackMsgIter);
        d_putMsgIter.reset(d_rawEvent.blob(), other.d_putMsgIter);
    }

    // NOTE that PutEventBuilder will never be constructed (from
    // d_putEventBuilderBuffer).  This is per contract of this copy
    // constructor.
}

Event::~Event()
{
    // This reset ensures that the PutEventBuilder buffer is destroyed. In the
    // case that we created the bmqimp::Event in place (not from the pool) the
    // object buffer of PutEventBuilder is not cleared and we leak.
    reset();
}

// MANIPULATORS
Event& Event::operator=(const Event& rhs)
{
    if (this == &rhs) {
        return *this;  // RETURN
    }

    BSLS_ASSERT_OPT(EventType::e_MESSAGE != rhs.d_type ||
                    MessageEventMode::e_WRITE != rhs.d_msgEventMode);
    // See note in copy constructor impl explaining above assert.

    reset();

    d_messageCorrelationIdContainer_p = rhs.d_messageCorrelationIdContainer_p;
    d_type                            = rhs.d_type;
    d_doneCallback                    = rhs.d_doneCallback;
    d_eventCallback                   = rhs.d_eventCallback;
    d_queues                          = rhs.d_queues;
    d_queuesBySubscriptionId          = rhs.d_queuesBySubscriptionId;
    d_sessionEventType                = rhs.d_sessionEventType;
    d_statusCode                      = rhs.d_statusCode;
    d_correlationId                   = rhs.d_correlationId;
    d_errorDescription                = rhs.d_errorDescription;
    d_msgEventMode                    = rhs.d_msgEventMode;
    d_correlationIds                  = rhs.d_correlationIds;

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_type ==
                                              EventType::e_SESSION)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return *this;  // RETURN
    }

    if (rhs.d_rawEvent.isCloned()) {
        // Other raw (bmqp) events coming from IO thread are cloned before
        // they are enqueued to event queue.  This case handles that.
        d_rawEvent    = rhs.d_rawEvent;
        d_pushMsgIter = rhs.d_pushMsgIter;
        d_ackMsgIter  = rhs.d_ackMsgIter;
        d_putMsgIter  = rhs.d_putMsgIter;
    }
    else {
        // A raw (bmqp) event may not be cloned created (indirectly) via
        // bmqa.Session.loadMessageEventBuilder.  In this case, clone it, and
        // preserve state of iterators.
        d_rawEvent = rhs.d_rawEvent.clone(d_allocator_p);
        d_pushMsgIter.reset(d_rawEvent.blob(), rhs.d_pushMsgIter);
        d_ackMsgIter.reset(d_rawEvent.blob(), rhs.d_ackMsgIter);
        d_putMsgIter.reset(d_rawEvent.blob(), rhs.d_putMsgIter);
    }

    // d_putEventBuilderBuffer is left in cleared state per contract.

    return *this;
}

void Event::resetIterators()
{
    if (d_rawEvent.isPushEvent()) {
        d_rawEvent.loadPushMessageIterator(&d_pushMsgIter, true);
    }
    else if (d_rawEvent.isAckEvent()) {
        d_rawEvent.loadAckMessageIterator(&d_ackMsgIter);
    }
    else {
        BSLS_ASSERT_SAFE(d_rawEvent.isPutEvent());
        d_rawEvent.loadPutMessageIterator(&d_putMsgIter, true);
    }
}

void Event::reset()
{
    d_messageCorrelationIdContainer_p = 0;
    d_type                            = EventType::e_UNINITIALIZED;
    d_sessionEventType                = bmqt::SessionEventType::e_UNDEFINED;
    d_statusCode                      = 0;
    d_correlationId                   = bmqt::CorrelationId();
    d_queues.clear();
    d_queuesBySubscriptionId.clear();
    d_errorDescription.clear();
    d_doneCallback  = bsl::nullptr_t();
    d_eventCallback = bsl::nullptr_t();

    if (d_isPutEventBuilderConstructed) {
        d_putEventBuilderBuffer.object().~PutEventBuilder();
        d_isPutEventBuilderConstructed = false;
    }

    d_msgEventMode = MessageEventMode::e_UNINITIALIZED;
    d_rawEvent.clear();
    d_pushMsgIter.clear();
    d_ackMsgIter.clear();
    d_putMsgIter.clear();
    d_correlationIds.clear();
}

void Event::clear()
{
    if (type() == EventType::e_SESSION && d_doneCallback) {
        d_doneCallback();
    }

    reset();
}

Event& Event::configureAsRawEvent(const bmqp::Event& rawEvent)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_UNINITIALIZED);
    BSLS_ASSERT_SAFE(rawEvent.isCloned());

    d_type     = EventType::e_RAW;
    d_rawEvent = rawEvent;

    return *this;
}

Event& Event::configureAsRequestEvent(const EventCallback& value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_UNINITIALIZED);

    d_type = EventType::e_REQUEST;
    setEventCallback(value);

    return *this;
}

Event&
Event::configureAsSessionEvent(bmqt::SessionEventType::Enum sessionEventType,
                               int                          statusCode,
                               const bmqt::CorrelationId&   correlationId,
                               const bslstl::StringRef&     errorDescription)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_UNINITIALIZED);

    d_type             = EventType::e_SESSION;
    d_sessionEventType = sessionEventType;
    d_statusCode       = statusCode;
    d_correlationId    = correlationId;
    d_errorDescription = errorDescription;

    return *this;
}

Event& Event::configureAsMessageEvent(const bmqp::Event& rawEvent)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_UNINITIALIZED);
    BSLS_ASSERT_OPT(rawEvent.isPushEvent() || rawEvent.isAckEvent() ||
                    rawEvent.isPutEvent());

    d_type         = EventType::e_MESSAGE;
    d_msgEventMode = MessageEventMode::e_READ;
    d_rawEvent     = rawEvent;

    resetIterators();

    return *this;
}

Event&
Event::configureAsMessageEvent(bmqp::BlobPoolUtil::BlobSpPool* blobSpPool_p)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_UNINITIALIZED);
    BSLS_ASSERT_SAFE(blobSpPool_p);

    d_type         = EventType::e_MESSAGE;
    d_msgEventMode = MessageEventMode::e_WRITE;
    new (d_putEventBuilderBuffer.buffer())
        bmqp::PutEventBuilder(blobSpPool_p, d_allocator_p);
    d_isPutEventBuilderConstructed = true;

    return *this;
}

Event& Event::downgradeMessageEventModeToRead()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_MESSAGE);

    if (MessageEventMode::e_READ == d_msgEventMode) {
        return *this;  // RETURN
    }

    d_msgEventMode = MessageEventMode::e_READ;

    d_rawEvent.reset(&(d_putEventBuilderBuffer.object().blob()));

    BSLS_ASSERT(d_rawEvent.isPutEvent());
    // Only PUT events can be built via SDK

    if (d_rawEvent.isValid()) {
        d_rawEvent.loadPutMessageIterator(&d_putMsgIter);
    }
    else {
        BSLS_ASSERT_SAFE(false && "Invalid MessageEvent");
        // NOTE: The Event is invalid likely if the MessageEventBuilder didn't
        //       contain any message. Leave the event in this state,
        //       i.e., pointing to the blob from the putEventBuilderBuffer
        //       (that may be the empty blob), and brokerSession::post will
        //       handle this correctly (by returning invalid argument error).
    }

    return *this;
}

Event& Event::upgradeMessageEventModeToWrite()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type() == EventType::e_MESSAGE);

    if (MessageEventMode::e_WRITE == d_msgEventMode) {
        return *this;  // RETURN
    }

    d_msgEventMode = MessageEventMode::e_WRITE;
    BSLS_ASSERT_SAFE(d_isPutEventBuilderConstructed);
    d_putEventBuilderBuffer.object().reset();
    d_putMsgIter.clear();
    d_rawEvent.clear();
    d_queues.clear();
    d_queuesBySubscriptionId.clear();
    d_correlationIds.clear();
    d_correlationId.makeUnset();
    return *this;
}

Event& Event::insertQueue(const bsl::shared_ptr<Queue>& queue)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queue);

    const bmqp::QueueId queueId(queue->id(), queue->subQueueId());

    d_queues.insert(bsl::make_pair(queueId, queue));

    return *this;
}

Event& Event::insertQueue(unsigned int                  subscriptionId,
                          const bsl::shared_ptr<Queue>& queue)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queue);

    d_queuesBySubscriptionId.insert(
        bsl::make_pair(SubscriptionId(queue->id(), subscriptionId), queue));

    return *this;
}

void Event::addMessageInfo(const bsl::shared_ptr<Queue>& queue,
                           const bmqt::MessageGUID&      guid,
                           const bmqt::CorrelationId&    corrId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queue);
    BSLS_ASSERT_SAFE(type() == EventType::e_MESSAGE);

    insertQueue(queue);

    // Add correlationId (even if it's empty) to the event's list.  It is
    // used by the message iterator to access correlationId by the message
    // index.
    addCorrelationId(corrId);

    // Insert correlationId and queueId and into correlationIds maps of
    // correlationId container.
    if (!corrId.isUnset()) {
        bmqp::QueueId qId(queue->id(), queue->subQueueId());
        d_messageCorrelationIdContainer_p->add(guid, corrId, qId);
    }
}

bsl::ostream&
Event::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (type()) {
    case EventType::e_UNINITIALIZED: {
        printer.printAttribute("type", "UNINITIALIZED");
    } break;
    case EventType::e_SESSION: {
        printer.printAttribute("type", "SESSION");
        printer.printAttribute("sessionEventType", d_sessionEventType);
        printer.printAttribute("statusCode", d_statusCode);
        printer.printAttribute("correlationId", d_correlationId);
        if (!d_errorDescription.empty()) {
            printer.printAttribute("errorDescription", d_errorDescription);
        }
        if (!d_queues.empty()) {
            for (QueuesMap::const_iterator citer = d_queues.begin();
                 citer != d_queues.end();
                 ++citer) {
                BSLS_ASSERT_SAFE(citer->second);
                printer.printAttribute("queue", citer->second->uri());
            }
        }
    } break;
    case EventType::e_MESSAGE: {
        printer.printAttribute("type", "MESSAGE");
        if (MessageEventMode::e_UNINITIALIZED == messageEventMode()) {
            printer.printAttribute("msgEventMode", "UNINITIALIZED");
        }
        else if (MessageEventMode::e_READ == messageEventMode()) {
            printer.printAttribute("rawEventType", d_rawEvent.type());
        }
        if (!d_queues.empty()) {
            for (QueuesMap::const_iterator citer = d_queues.begin();
                 citer != d_queues.end();
                 ++citer) {
                BSLS_ASSERT_SAFE(citer->second);
                printer.printAttribute("queue", citer->second->uri());
            }
        }
        else {
            // TBD
        }
    } break;
    case EventType::e_RAW: {
        printer.printAttribute("type", "RAW");
        printer.printAttribute("rawEventType", d_rawEvent.type());
    } break;
    case EventType::e_REQUEST: {
        printer.printAttribute("type", "REQUEST");
    } break;
    default: {
        BSLS_ASSERT_OPT(false && "Unknown Event type");
    }
    }
    printer.end();

    return stream;
}

// ACCESSORS
const bsl::shared_ptr<Queue> Event::lookupQueue() const
{
    if (rawEvent().type() == bmqp::EventType::e_PUSH) {
        bmqp::RdaInfo rdaInfo;
        int           qId = Queue::k_INVALID_QUEUE_ID;
        unsigned int  sId = bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID;
        d_pushMsgIter.extractQueueInfo(&qId, &sId, &rdaInfo);

        return lookupQueue(qId, sId);  // RETURN
    }
    else {
        bmqp::QueueId queueId(Queue::k_INVALID_QUEUE_ID);
        if (rawEvent().type() == bmqp::EventType::e_PUT) {
            queueId.setId(d_putMsgIter.header().queueId());
        }
        else if (rawEvent().type() == bmqp::EventType::e_ACK) {
            queueId.setId(d_ackMsgIter.message().queueId());
        }
        else {
            BSLS_ASSERT_OPT(false && "Invalid raw event type");
        }

        return lookupQueue(queueId);  // RETURN
    }
}

const bsl::shared_ptr<Queue>
Event::lookupQueue(int queueId, unsigned int subscriptionId) const
{
    bsl::shared_ptr<Queue> result;

    // lookup by 'subscriptionId'
    QueuesBySubscriptionId::const_iterator cit = d_queuesBySubscriptionId.find(
        SubscriptionId(queueId, subscriptionId));

    if (cit == d_queuesBySubscriptionId.end()) {
        return bsl::shared_ptr<Queue>();  // RETURN
    }
    BSLS_ASSERT_SAFE(cit->second);

    return cit->second;
}

const bsl::shared_ptr<Queue>
Event::lookupQueue(const bmqp::QueueId& queueId) const
{
    bsl::shared_ptr<Queue> result;

    // lookup by 'subscriptionId'
    QueuesMap::const_iterator cit = d_queues.find(queueId);

    if (cit == d_queues.end()) {
        return bsl::shared_ptr<Queue>();  // RETURN
    }
    BSLS_ASSERT_SAFE(cit->second);

    return cit->second;
}

}  // close package namespace
}  // close enterprise namespace
