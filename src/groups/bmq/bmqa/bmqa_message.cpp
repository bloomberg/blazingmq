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

// bmqa_message.cpp                                                   -*-C++-*-
#include <bmqa_message.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqa_messageproperties.h>
#include <bmqimp_event.h>
#include <bmqimp_messagecorrelationidcontainer.h>
#include <bmqimp_queue.h>
#include <bmqp_event.h>
#include <bmqp_eventutil.h>
#include <bmqp_messageproperties.h>
#include <bmqp_protocolutil.h>
#include <bmqp_queueid.h>
#include <bmqt_resultcode.h>

// MWC
#include <mwcu_blob.h>
#include <mwcu_memoutstream.h>

// BDE
#include <bdlma_localsequentialallocator.h>
#include <bsl_iostream.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bslim_printer.h>
#include <bslma_default.h>
#include <bslmf_assert.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqa {

// -------------
// class Message
// -------------

#ifdef BMQ_ENABLE_MSG_GROUPID
namespace {
// Compile-time assert to keep the hardcoded value of max group id size in sync
// with its actual size.  We need to hard code the size in 'bmqa_message.h/cpp'
// because none of the 'bmqp' headers can be included in 'bmqa' headers.
BSLMF_ASSERT(Message::k_GROUP_ID_MAX_LENGTH ==
             bmqp::Protocol::k_MSG_GROUP_ID_MAX_LENGTH);
}  // close unnamed namespace
#endif

// CREATORS
Message::Message()
{
    d_impl.d_event_p = 0;
}

// PRIVATE ACCESSORS
bool Message::isInitialized() const
{
    return d_impl.d_event_p != 0;
}

// MANIPULATORS
Message& Message::setDataRef(const bdlbb::Blob* data)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(data);
    BSLS_ASSERT_SAFE(isInitialized() &&
                     "message is invalid: use "
                     "'MessageEventBuilder::startMessage' to get one");
    BSLS_ASSERT_SAFE(d_impl.d_event_p->putEventBuilder() &&
                     "message not editable");

    d_impl.d_event_p->putEventBuilder()->setMessagePayload(data);
    return *this;
}

Message& Message::setDataRef(const char* data, size_t length)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(data);
    BSLS_ASSERT_SAFE(isInitialized() &&
                     "message is invalid: use "
                     "'MessageEventBuilder::startMessage' to get one");
    BSLS_ASSERT_SAFE(d_impl.d_event_p->putEventBuilder() &&
                     "message not editable");

    d_impl.d_event_p->putEventBuilder()->setMessagePayload(data, length);
    return *this;
}

Message& Message::setPropertiesRef(const MessageProperties* properties)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(properties);
    BSLS_ASSERT_SAFE(isInitialized() &&
                     "message is invalid: use "
                     "'MessageEventBuilder::startMessage' to get one");
    BSLS_ASSERT_SAFE(d_impl.d_event_p->putEventBuilder() &&
                     "message not editable");

    const bmqp::MessageProperties* const* propertiesImpl =
        reinterpret_cast<const bmqp::MessageProperties* const*>(properties);

    d_impl.d_event_p->putEventBuilder()->setMessageProperties(*propertiesImpl);

    return *this;
}

Message& Message::clearPropertiesRef()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isInitialized() &&
                     "message is invalid: use "
                     "'MessageEventBuilder::startMessage' to get one");
    BSLS_ASSERT_SAFE(d_impl.d_event_p->putEventBuilder() &&
                     "message not editable");

    d_impl.d_event_p->putEventBuilder()->clearMessageProperties();

    return *this;
}

Message& Message::setCorrelationId(const bmqt::CorrelationId& correlationId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isInitialized() &&
                     "message is invalid: use "
                     "'MessageEventBuilder::startMessage' to get one");
    BSLS_ASSERT_SAFE(d_impl.d_event_p->putEventBuilder() &&
                     "message not editable");

    d_impl.d_correlationId = correlationId;

    // Because this object can be a copy of what MessageEventBuilder holds (as
    // in the case of legacy BlazingMQ Python bindings), and
    // MessageEventBuilder::packMessage does not take this object as an
    // argument, store this id in the d_event_p so that / MessageEventBuilder
    // can read it.

    d_impl.d_event_p->setCorrelationId(correlationId);

    return *this;
}

Message& Message::setCompressionAlgorithmType(
    bmqt::CompressionAlgorithmType::Enum value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isInitialized() &&
                     "message is invalid: use "
                     "'MessageEventBuilder::startMessage' to get one");
    BSLS_ASSERT_SAFE(d_impl.d_event_p->putEventBuilder() &&
                     "message not editable");

    bmqp::PutEventBuilder* builder = d_impl.d_event_p->putEventBuilder();
    builder->setCompressionAlgorithmType(value);

    return *this;
}

#ifdef BMQ_ENABLE_MSG_GROUPID
Message& Message::setGroupId(const bsl::string& groupId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isInitialized() &&
                     "message is invalid: use "
                     "'MessageEventBuilder::startMessage' to get one");
    BSLS_ASSERT_SAFE(d_impl.d_event_p->putEventBuilder() &&
                     "message not editable");

    bmqp::PutEventBuilder* builder = d_impl.d_event_p->putEventBuilder();
    builder->setMsgGroupId(groupId);

    d_impl.d_groupId = groupId;

    return *this;
}

Message& Message::clearGroupId()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isInitialized() &&
                     "message is invalid: use "
                     "'MessageEventBuilder::startMessage' to get one");
    BSLS_ASSERT_SAFE(d_impl.d_event_p->putEventBuilder() &&
                     "message not editable");

    bmqp::PutEventBuilder* builder = d_impl.d_event_p->putEventBuilder();
    builder->clearMsgGroupId();

    d_impl.d_groupId.clear();

    return *this;
}
#endif

// ACCESSORS
Message Message::clone(bslma::Allocator* basicAllocator) const
{
    Message result(*this);

    // Clone of an uninitialized message... nothing more to do
    if (!isInitialized()) {
        return result;  // RETURN
    }

    bslma::Allocator* allocator = bslma::Default::allocator(basicAllocator);

    result.d_impl.d_clonedEvent_sp.createInplace(allocator,
                                                 *(d_impl.d_event_p),
                                                 allocator);
    result.d_impl.d_event_p = result.d_impl.d_clonedEvent_sp.get();

    // Other fields of result.d_impl are already copied courtesy the way
    // 'result' is created.

    return result;
}

bool Message::isValid() const
{
    return isInitialized();
}

const bmqa::QueueId& Message::queueId() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isInitialized());

    // Lazy lookup of the queueId: if it is already initialized, return it
    bsl::shared_ptr<bmqimp::Queue>& queueSpRef =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(d_impl.d_queueId);

    if (queueSpRef->id() != bmqimp::Queue::k_INVALID_QUEUE_ID) {
        return d_impl.d_queueId;  // RETURN
    }

    queueSpRef = d_impl.d_event_p->lookupQueue();

    BSLS_ASSERT_SAFE(queueSpRef);
    // If not in safe mode, we simply return a queueId for which
    // 'isInitialized()' will return false (because
    // 'bmqimp::BrokerSession::lookupQueue()' return an empty shared pointer)

    return d_impl.d_queueId;
}

const bmqt::CorrelationId& Message::correlationId() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isInitialized());

    const bmqp::Event& rawEvent = d_impl.d_event_p->rawEvent();

    BSLS_ASSERT_OPT((rawEvent.isPutEvent() || rawEvent.isAckEvent() ||
                     rawEvent.isPushEvent()) &&
                    "Invalid raw event type");

    // Correlation ID for a ACK, PUT, and PUSH msgs is already set in
    // bmqa::MessageIterator::nextMessage().

    return d_impl.d_correlationId;
}

const bmqt::SubscriptionHandle& Message::subscriptionHandle() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isInitialized());

    const bmqp::Event& rawEvent = d_impl.d_event_p->rawEvent();

    BSLS_ASSERT_OPT(rawEvent.isPushEvent() && "Invalid raw event type");

    // Subscription Handle for a PUSH msg is already set in
    // bmqa::MessageIterator::nextMessage().

    return d_impl.d_subscriptionHandle;
}

bmqt::CompressionAlgorithmType::Enum Message::compressionAlgorithmType() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isInitialized());

    const bmqp::Event& rawEvent = d_impl.d_event_p->rawEvent();

    if (rawEvent.isPushEvent()) {
        return d_impl.d_event_p->pushMessageIterator()
            ->header()
            .compressionAlgorithmType();  // RETURN
    }
    else if (rawEvent.isPutEvent()) {
        return d_impl.d_event_p->putMessageIterator()
            ->header()
            .compressionAlgorithmType();  // RETURN
    }

    BSLS_ASSERT_OPT(false && "Invalid raw event type");
    return bmqt::CompressionAlgorithmType::e_NONE;
}

const bmqt::MessageGUID& Message::messageGUID() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isInitialized());

    const bmqp::Event& rawEvent = d_impl.d_event_p->rawEvent();

    if (rawEvent.isPushEvent()) {
        return d_impl.d_event_p->pushMessageIterator()
            ->header()
            .messageGUID();  // RETURN
    }
    else if (rawEvent.isAckEvent()) {
        return d_impl.d_event_p->ackMessageIterator()
            ->message()
            .messageGUID();  // RETURN
    }
    else if (rawEvent.isPutEvent()) {
        return d_impl.d_event_p->putMessageIterator()
            ->header()
            .messageGUID();  // RETURN
    }
    else {
        BSLS_ASSERT_OPT(false && "Invalid raw event type");
        // For compiler happiness ...
        return reinterpret_cast<const bmqt::MessageGUID&>(*this);  // RETURN
    }
}

#ifdef BMQ_ENABLE_MSG_GROUPID
const bsl::string& Message::groupId() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isInitialized());

    if (!d_impl.d_groupId.empty()) {
        return d_impl.d_groupId;  // RETURN
    }

    const bmqp::Event& rawEvent = d_impl.d_event_p->rawEvent();

    if (rawEvent.isPushEvent()) {
        const bmqp::PushMessageIterator* msgIterator =
            d_impl.d_event_p->pushMessageIterator();
        if (msgIterator->hasMsgGroupId()) {
            msgIterator->extractMsgGroupId(&d_impl.d_groupId);
        }
        return d_impl.d_groupId;  // RETURN
    }

    if (rawEvent.isPutEvent()) {
        const bmqp::PutMessageIterator* msgIterator =
            d_impl.d_event_p->putMessageIterator();
        if (msgIterator->hasMsgGroupId()) {
            msgIterator->extractMsgGroupId(&d_impl.d_groupId);
        }
        return d_impl.d_groupId;  // RETURN
    }

    BSLS_ASSERT_OPT(false && "Invalid raw event type");
    return d_impl.d_groupId;  // Compiler Happiness
}
#endif

MessageConfirmationCookie bmqa::Message::confirmationCookie() const
{
    // PRECONDITIONS
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isInitialized() &&
                     "message is invalid: use "
                     "'MessageEventBuilder::startMessage' to get one");
    BSLS_ASSERT_SAFE(d_impl.d_event_p->rawEvent().isPushEvent() &&
                     "Event is not a Push event");

    return MessageConfirmationCookie(queueId(), messageGUID());
}

int Message::ackStatus() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isInitialized());

    const bmqp::Event& rawEvent = d_impl.d_event_p->rawEvent();

    BSLS_ASSERT_SAFE(rawEvent.isAckEvent() &&
                     "Event is not an AckMessage event");
    (void)rawEvent;  // Compiler happiness

    return bmqp::ProtocolUtil::ackResultFromCode(
        d_impl.d_event_p->ackMessageIterator()->message().status());
}

int Message::getData(bdlbb::Blob* blob) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isInitialized());

    const bmqp::Event& rawEvent = d_impl.d_event_p->rawEvent();

    if (rawEvent.isPushEvent()) {
        return d_impl.d_event_p->pushMessageIterator()->loadMessagePayload(
            blob);  // RETURN
    }
    else if (rawEvent.isPutEvent()) {
        return d_impl.d_event_p->putMessageIterator()->loadMessagePayload(
            blob);  // RETURN
    }
    else {
        BSLS_ASSERT_OPT(false && "Invalid raw event type");
        return -1;  // Compiler Happiness                              //
                    // RETURN
    }
}

int Message::dataSize() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isInitialized());

    const bmqp::Event& rawEvent = d_impl.d_event_p->rawEvent();

    if (rawEvent.isPushEvent()) {
        return d_impl.d_event_p->pushMessageIterator()
            ->messagePayloadSize();  // RETURN
    }
    else if (rawEvent.isPutEvent()) {
        return d_impl.d_event_p->putMessageIterator()
            ->messagePayloadSize();  // RETURN
    }
    else {
        BSLS_ASSERT_OPT(false && "Invalid raw event type");
        return -1;  // Compiler Happiness                              //
                    // RETURN
    }
}

bool Message::hasProperties() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isInitialized());

    const bmqp::Event& rawEvent = d_impl.d_event_p->rawEvent();

    if (rawEvent.isPushEvent()) {
        return d_impl.d_event_p->pushMessageIterator()
            ->hasMessageProperties();  // RETURN
    }

    if (rawEvent.isPutEvent()) {
        return d_impl.d_event_p->putMessageIterator()
            ->hasMessageProperties();  // RETURN
    }

    BSLS_ASSERT_OPT(false && "Invalid raw event type");
    return false;  // Compiler Happiness
}

#ifdef BMQ_ENABLE_MSG_GROUPID
bool Message::hasGroupId() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isInitialized());

    const bmqp::Event& rawEvent = d_impl.d_event_p->rawEvent();

    if (rawEvent.isPushEvent()) {
        return d_impl.d_event_p->pushMessageIterator()
            ->hasMsgGroupId();  // RETURN
    }

    if (rawEvent.isPutEvent()) {
        return d_impl.d_event_p->putMessageIterator()
            ->hasMsgGroupId();  // RETURN
    }

    BSLS_ASSERT_OPT(false && "Invalid raw event type");
    return false;  // Compiler Happiness
}
#endif

int Message::loadProperties(MessageProperties* buffer) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isInitialized());
    BSLS_ASSERT_SAFE(buffer);

    bmqp::MessageProperties** propertiesImpl =
        reinterpret_cast<bmqp::MessageProperties**>(buffer);

    const bmqp::Event& rawEvent = d_impl.d_event_p->rawEvent();
    int                rc       = -1;

    if (rawEvent.isPushEvent()) {
        bsl::shared_ptr<bmqimp::Queue>& queue =
            reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(
                d_impl.d_queueId);

        if (queue->id() == bmqimp::Queue::k_INVALID_QUEUE_ID) {
            queue = d_impl.d_event_p->lookupQueue();
        }
        BSLS_ASSERT_SAFE(queue);

        bdlbb::Blob                      propertiesBlob;
        const bmqp::PushMessageIterator& it =
            *d_impl.d_event_p->pushMessageIterator();
        it.loadMessageProperties(&propertiesBlob);

        bmqp::MessagePropertiesInfo input(it.header());

        rc = queue->schemaLearner().read(queue->schemaLearnerContext(),
                                         *propertiesImpl,
                                         input,
                                         propertiesBlob);

        // Forcibly load all properties.
        // REVISIT; this can be removed once MPs support dynamic modification
        if (rc == 0) {
            rc = 1000 *
                 (*propertiesImpl)->loadProperties(false, input.isExtended());
        }
    }
    else if (rawEvent.isPutEvent()) {
        rc = d_impl.d_event_p->putMessageIterator()->loadMessageProperties(
            *propertiesImpl);
        // Schema learning for PUSHs only, not for PUTs assuming 'Message'
        // builds PUTs and receives PUSHs.
    }
    else {
        BSLS_ASSERT_OPT(false && "Invalid raw event type");
    }

    return rc;
}

bsl::ostream&
Message::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    static const int k_MAX_BYTES_DUMP = 256;

    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    if (!isInitialized()) {
        printer.printValue("* invalid message *");
    }
    else {
        const bmqp::Event& rawEvent = d_impl.d_event_p->rawEvent();

        printer.printAttribute("type", rawEvent.type());

        if (rawEvent.isPushEvent() || rawEvent.isPutEvent()) {
            printer.printAttribute("queue", queueId());

            if (rawEvent.isPushEvent()) {
                printer.printAttribute("correlationId", correlationId());
                printer.printAttribute("subscriptionHandle",
                                       subscriptionHandle());
            }

            printer.printAttribute("GUID", messageGUID());
            printer.printAttribute("compressionAlgorithmType",
                                   compressionAlgorithmType());
            bdlbb::Blob blob;
            getData(&blob);

            bdlma::LocalSequentialAllocator<k_MAX_BYTES_DUMP> lsa(0);

            // NOTE: since we know the size, the `defaultAllocator` will
            //       never be used here.
            mwcu::MemOutStream os(k_MAX_BYTES_DUMP, &lsa);

            os << mwcu::BlobStartHexDumper(&blob, k_MAX_BYTES_DUMP);
            stream << "\n" << os.str();
        }
        else if (rawEvent.isAckEvent()) {
            printer.printAttribute(
                "status",
                static_cast<bmqt::AckResult::Enum>(ackStatus()));
            printer.printAttribute("correlationId", correlationId());
            printer.printAttribute("GUID", messageGUID());
            printer.printAttribute("queue", queueId());
        }
    }
    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
