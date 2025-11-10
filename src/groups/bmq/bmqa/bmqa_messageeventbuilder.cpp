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

// bmqa_messageeventbuilder.cpp                                       -*-C++-*-
#include <bmqa_messageeventbuilder.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqa_messageproperties.h>
#include <bmqa_queueid.h>
#include <bmqimp_event.h>
#include <bmqimp_queue.h>
#include <bmqp_messageguidgenerator.h>
#include <bmqp_protocol.h>
#include <bmqt_messageguid.h>
#include <bmqt_queueflags.h>

// BDE
#include <ball_log.h>
#include <bsl_memory.h>
#include <bslma_managedptr.h>
#include <bslmf_assert.h>
#include <bsls_assert.h>
#include <bsls_nullptr.h>
#include <bsls_performancehint.h>
#include <bslstl_sharedptr.h>

namespace BloombergLP {
namespace bmqa {

// CLASS-SCOPE CATEGORY
BALL_LOG_SET_CLASS_CATEGORY("BMQA.MESSAGEEVENTBUILDER");

const char k_tracePropertyName[] = "bmq.traceparent";

namespace {
// Compile time sanity checks
BSLMF_ASSERT(sizeof(Message) == sizeof(MessageImpl));
BSLMF_ASSERT(sizeof(MessageEvent) == sizeof(bsl::shared_ptr<bmqimp::Event>));
BSLMF_ASSERT(sizeof(QueueId) == sizeof(bsl::shared_ptr<bmqimp::Queue>));

}  // close unnamed namespace

// -------------------------
// class MessageEventBuilder
// -------------------------

MessageEventBuilder::MessageEventBuilder()
: d_impl()
{
    // NOTHING
}

Message& MessageEventBuilder::startMessage()
{
    // Get bmqimp::Event from bmqa::MessageEvent
    typedef bsl::shared_ptr<bmqimp::Event> EventSP;
    EventSP& eventSpRef = reinterpret_cast<EventSP&>(d_impl.d_msgEvent);

    BSLS_ASSERT_OPT(eventSpRef &&
                    "This builder is invalid, it must be obtained by a call "
                    "to bmqa::Session.loadMessageEventBuilder() !");

    BSLS_ASSERT_OPT((bmqimp::Event::MessageEventMode::e_WRITE ==
                     eventSpRef->messageEventMode()) &&
                    "reset() must be called on this builder.");

    // Get bmqa::MessageImpl from bmqa::Message
    MessageImpl& msgImplRef = reinterpret_cast<MessageImpl&>(d_impl.d_msg);

    msgImplRef.d_event_p = eventSpRef.get();
    msgImplRef.d_event_p->putEventBuilder()->startMessage();

    // Reset CorrelationId for the current message.
    d_impl.d_msg.setCorrelationId(bmqt::CorrelationId());

    return d_impl.d_msg;
}

bmqt::EventBuilderResult::Enum
MessageEventBuilder::packMessage(const bmqa::QueueId& queueId)
{
    // Get bmqa::MessageImpl from bmqa::Message
    MessageImpl& msgImplRef = reinterpret_cast<MessageImpl&>(d_impl.d_msg);
    BSLS_ASSERT_OPT(msgImplRef.d_event_p &&
                    "StartMessage must be called before 'packMessage'.");
    BSLS_ASSERT_SAFE(d_impl.d_guidGenerator_sp);

    bmqp::PutEventBuilder* builder = msgImplRef.d_event_p->putEventBuilder();
    BSLS_ASSERT_SAFE(builder->messageGUID().isUnset());

    // Because setCorrelationId could be called on a copy of what this object
    // holds (as in the case of legacy BlazingMQ Python bindings), read from
    // d_event_p (instead of msgImplRef.d_correlationId).

    const bmqt::CorrelationId& corrId = msgImplRef.d_event_p->correlationId();

    // Extract internal queueId; in order to do that, first get the rep
    typedef bsl::shared_ptr<bmqimp::Queue> QueueSP;
    const QueueSP& queueSpRef = reinterpret_cast<const QueueSP&>(queueId);

    // Check that the queue is valid, which means not only the OPENED state but
    // it also may be in PENDING or REOPENING states and still accept PUT
    // messages.
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!queueSpRef->isValid())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::EventBuilderResult::e_QUEUE_INVALID;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !bmqt::QueueFlagsUtil::isWriter(queueSpRef->flags()))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::EventBuilderResult::e_QUEUE_READONLY;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(queueSpRef->isSuspended())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::EventBuilderResult::e_QUEUE_SUSPENDED;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(builder->unpackedMessageSize() <=
                                              0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::EventBuilderResult::e_PAYLOAD_EMPTY;  // RETURN
    }

    if (corrId.isUnset()) {
        if (bmqt::QueueFlagsUtil::isAck(queueSpRef->flags())) {
            // A queue opened with ackFlag requires a CorrelationId for every
            // message posted.
            return bmqt::EventBuilderResult::e_MISSING_CORRELATION_ID;
            // RETURN
        }
    }
    else {
        builder->setFlags(bmqp::PutHeaderFlags::e_ACK_REQUESTED);
    }

    bmqt::EventBuilderResult::Enum rc;
    bmqt::MessageGUID              guid;
    d_impl.d_guidGenerator_sp->generateGUID(&guid);
    builder->setMessageGUID(guid);

    // If distributed tracing is enabled, create a span and inject trace
    // span into message properties
    bsl::shared_ptr<bmqp::MessageProperties> propsWithDT;
    bsl::shared_ptr<bmqpi::DTSpan>           span;
    if (d_impl.d_dtTracer_sp && d_impl.d_dtContext_sp) {
        copyPropertiesAndInjectDT(&propsWithDT, &span, builder, queueId);
    }

    if (queueSpRef->isOldStyle()) {
        // Temporary; shall remove after 2nd roll out of "new style" brokers.
        rc = builder->packMessageInOldStyle(queueSpRef->id());
    }
    else {
        bmqp::MessagePropertiesInfo info =
            queueSpRef->schemaGenerator().getSchemaId(
                builder->messageProperties());
        builder->setMessagePropertiesInfo(info);

        rc = builder->packMessage(queueSpRef->id());
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(
            rc == bmqt::EventBuilderResult::e_SUCCESS)) {
        if (builder->lastPackedMesageCompressionRatio() != 1) {
            BSLS_ASSERT_SAFE(builder->lastPackedMesageCompressionRatio() !=
                             -1);
            queueSpRef->statReportCompressionRatio(
                builder->lastPackedMesageCompressionRatio());
        }

        // Add message related info into the event on success.
        msgImplRef.d_event_p->addMessageInfo(queueSpRef, guid, corrId, span);
    }

    return rc;
}

void MessageEventBuilder::reset()
{
    // Get bmqa::MessageImpl from bmqa::Message
    MessageImpl& msgImplRef = reinterpret_cast<MessageImpl&>(d_impl.d_msg);
    msgImplRef.d_event_p    = 0;
    msgImplRef.d_correlationId.makeUnset();

    // Get bmqimp::Event from bmqa::MessageEvent
    typedef bsl::shared_ptr<bmqimp::Event> EventSP;
    EventSP& eventSpRef = reinterpret_cast<EventSP&>(d_impl.d_msgEvent);

    d_impl.d_messageCountFinal     = 0;
    d_impl.d_messageEventSizeFinal = 0;

    eventSpRef->upgradeMessageEventModeToWrite();
    // underlying PutEventBuilder is reset in
    // 'upgradeMessageEventModeToWrite'
}

const MessageEvent& MessageEventBuilder::messageEvent()
{
    // Get bmqimp::Event from bmqa::MessageEvent
    typedef bsl::shared_ptr<bmqimp::Event> EventSP;
    EventSP& eventSpRef = reinterpret_cast<EventSP&>(d_impl.d_msgEvent);

    if (eventSpRef->messageEventMode() ==
        bmqimp::Event::MessageEventMode::e_WRITE) {
        // On this call of the 'messageEvent()', we switch from WRITE to READ.
        // In READ mode, PutEventBuilder will not be accessible, so we have
        // to cache the final properties of the built MessageEvent to be
        // returned on demand.
        d_impl.d_messageCountFinal =
            eventSpRef->putEventBuilder()->messageCount();
        d_impl.d_messageEventSizeFinal =
            eventSpRef->putEventBuilder()->eventSize();
    }

    eventSpRef->downgradeMessageEventModeToRead();  // builds the blob too

    return d_impl.d_msgEvent;
}

Message& MessageEventBuilder::currentMessage()
{
    // PRECONDITIONS
    BSLS_ASSERT(reinterpret_cast<MessageImpl&>(d_impl.d_msg).d_event_p &&
                "StartMessage must be called before asking currentMessage");

    return d_impl.d_msg;
}

int MessageEventBuilder::messageCount() const
{
    // Get bmqimp::Event from bmqa::MessageEvent
    typedef bsl::shared_ptr<bmqimp::Event> EventSP;
    const EventSP& eventSpRef = reinterpret_cast<const EventSP&>(
        d_impl.d_msgEvent);

    if (eventSpRef->messageEventMode() ==
        bmqimp::Event::MessageEventMode::e_READ) {
        // In READ mode, we are not able to access PutEventBuilder anymore
        // to get the message count.  In this case, we rely on the value
        // cached on switching from WRITE to READ.
        return d_impl.d_messageCountFinal;  // RETURN
    }

    return eventSpRef->putEventBuilder()->messageCount();
}

int MessageEventBuilder::messageEventSize() const
{
    // Get bmqimp::Event from bmqa::MessageEvent
    typedef bsl::shared_ptr<bmqimp::Event> EventSP;
    const EventSP& eventSpRef = reinterpret_cast<const EventSP&>(
        d_impl.d_msgEvent);

    if (eventSpRef->messageEventMode() ==
        bmqimp::Event::MessageEventMode::e_READ) {
        // In READ mode, we are not able to access PutEventBuilder anymore
        // to get the event size.  In this case, we rely on the value
        // cached on switching from WRITE to READ.
        return d_impl.d_messageEventSizeFinal;  // RETURN
    }

    return eventSpRef->putEventBuilder()->eventSize();
}

void MessageEventBuilder::copyPropertiesAndInjectDT(
    bsl::shared_ptr<bmqp::MessageProperties>* properties,
    bsl::shared_ptr<bmqpi::DTSpan>*           span,
    bmqp::PutEventBuilder*                    builder,
    const bmqa::QueueId&                      queueId) const
{
    // Get message properties
    const bmqp::MessageProperties* props = builder->messageProperties();
    bsl::shared_ptr<bmqp::MessageProperties> propsCopy =
        bsl::make_shared<bmqp::MessageProperties>(
            props ? *props : bmqp::MessageProperties());

    // Create a child span of the current span for the PUT operation
    bmqpi::DTSpan::Baggage baggage;
    bsl::stringstream      ss;
    bmqt::QueueFlagsUtil::prettyPrint(ss, queueId.flags());
    baggage.put("bmq.queue.flags", ss.str());
    baggage.put("bmq.queue.uri", queueId.uri().asString());
    bsl::shared_ptr<bmqpi::DTSpan> childSpan =
        d_impl.d_dtTracer_sp->createChildSpan(d_impl.d_dtContext_sp->span(),
                                              "bmq.message.put",
                                              baggage);
    if (!childSpan) {
        return;
    }

    // Serialize the span to inject into message properties
    bsl::vector<unsigned char> serializedSpan;
    int rc = d_impl.d_dtTracer_sp->serializeSpan(&serializedSpan, childSpan);
    if (rc != 0) {
        BALL_LOG_WARN << "Failed to serialize span for trace context "
                      << "injection, rc: " << rc;
        childSpan->finish();
        return;  // RETURN
    }

    // Inject the serialized trace span as a reserved binary property
    bsl::vector<char> traceSpanData(serializedSpan.begin(),
                                    serializedSpan.end());
    rc = propsCopy->setPropertyAsBinary(k_tracePropertyName, traceSpanData);
    if (rc != 0) {
        BALL_LOG_WARN << "Failed to inject trace span into "
                      << "message properties, rc: " << rc;
        childSpan->finish();
        return;  // RETURN
    }

    // Set the modified properties back to the builder
    builder->setMessageProperties(propsCopy.get());

    // Return the created span and the properties with injected trace span for
    // later use.  Span will be stored in correlationId.  Properties need to
    // be kept alive until it's compressed in PutEventBuilder::packMessage()
    *span       = childSpan;
    *properties = propsCopy;
}

}  // close package namespace
}  // close enterprise namespace
