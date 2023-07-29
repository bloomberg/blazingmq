// Copyright 2023 Bloomberg Finance L.P.
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

// producer.cpp                                                       -*-C++-*-

// This file is part of the BlazingMQ tutorial "Hello World".
//
// The simplest use case involves a Publisher task, a Consumer task, and a
// Message Broker.  The Producer and/or the Consumer create a named queue in
// the Broker.  The Producer pushes messages into the queue, and the consumer
// pulls them.  Once a message has been consumed it can be removed from the
// queue.
//
// In essence the queue is a mailbox for messages to be sent to a Consumer.  It
// allows decoupling the processing between the Producer and the Consumer: one
// of the tasks may be down without impacting the other, and the Consumer can
// process messages at a different pace than the Producer.

// BMQ
#include <bmqa_event.h>
#include <bmqa_messageproperties.h>
#include <bmqa_queueid.h>
#include <bmqa_session.h>
#include <bmqt_queueflags.h>
#include <bmqt_resultcode.h>

// BDE
#include <ball_fileobserver.h>
#include <ball_log.h>
#include <ball_loggermanager.h>
#include <ball_loggermanagerconfiguration.h>
#include <bdlb_print.h>
#include <bdlb_scopeexit.h>
#include <bdlb_string.h>
#include <bdlf_bind.h>
#include <bdlt_currenttime.h>
#include <bdlt_epochutil.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_spinlock.h>

using namespace BloombergLP;

namespace {

const char k_LOG_CATEGORY[] = "PRODUCER";

void readUserInput(bsl::string* input)
{
    const int k_BUFFER_SIZE = 256;

    char buffer[k_BUFFER_SIZE];
    bsl::cout << "> " << bsl::flush;
    bsl::cin.clear();
    bsl::cin.getline(buffer, k_BUFFER_SIZE);
    *input = buffer;
    bdlb::String::trim(input);
}

}  // close unnamed namespace

// CLASSES
class QueueManager;

// ==================
// class EventHandler
// ==================

/// Concrete implementation of an event handler.  Note that the methods are
/// called on the session's own threads.
class EventHandler : public bmqa::SessionEventHandler {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY(k_LOG_CATEGORY);

    // DATA
    QueueManager* d_queueManager_p;  // Pointer to the queue manager (held,
                                     // not owned)

  public:
    // MANIPULATORS
    void onSessionEvent(const bmqa::SessionEvent& sessionEvent)
        BSLS_KEYWORD_OVERRIDE;
    // Process the specified 'sessionEvent' received from the broker.

    void onMessageEvent(const bmqa::MessageEvent& messageEvent)
        BSLS_KEYWORD_OVERRIDE;
    // Process the specified 'messageEvent' received from the broker.

    /// Set the associated member of this event handler to the specified
    /// `value` and return a reference offering modifiable access to this
    /// object.
    EventHandler& setQueueManager(QueueManager* value);

    // ACCESSORS

    /// Return a pointer to the corresponding member associated with this
    /// event handler.
    QueueManager* queueManager() const;
};

// ==================
// class QueueManager
// ==================

/// Object to keep track of application queues and associated contexts.
class QueueManager {
  private:
    BALL_LOG_SET_CLASS_CATEGORY("PRODUCER.QUEUEMANAGER");

  private:
    // PRIVATE TYPES

    /// `MessagesMap` is an alias for a map of `Message` objects associated
    /// with a QueueContext.
    ///
    /// * messageCorrelationId -> message
    typedef bsl::unordered_map<bmqt::CorrelationId, bmqa::Message> MessagesMap;

    /// Struct holding members associated with the state of a queue.
    struct QueueContext {
        // DATA
        MessagesMap        d_messagesPosted;
        bsls::Types::Int64 d_numMessagesPosted;
        bsls::Types::Int64 d_numMessagesAcked;
        bsls::Types::Int64 d_numMessagesNAcked;

        QueueContext()
        : d_messagesPosted()
        , d_numMessagesPosted(0)
        , d_numMessagesAcked(0)
        , d_numMessagesNAcked(0)
        {
            // NOTHING
        }
    };

    /// `QueuesMap` is an alias for a map of contexts associated with a
    /// queue.
    ///
    /// * queueCorrelationId -> queueContext
    typedef bsl::unordered_map<bmqt::CorrelationId, QueueContext> QueuesMap;

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0  // Success
        ,
        rc_UNKNOWN_QUEUE = -1  // Unknown queue
        ,
        rc_UNKNOWN_MESSAGE = -2  // Unknown message
    };

  private:
    // DATA
    QueuesMap              d_queues;
    mutable bsls::SpinLock d_queuesLock;

  public:
    // CREATORS

    /// Create a new `QueueManager` object.
    explicit QueueManager();

    // MANIPULATORS

    /// Insert the queue with the specified `queueId` into the queues
    /// being managed by this object.
    bool insertQueue(const bmqa::QueueId& queueId);

    /// Remove the queue with the specified `queueId` from the queues
    /// being managed by this object. Behaviour is undefined if the
    /// specified `queueId` does not exist.
    void removeQueue(const bmqa::QueueId& queueId);

    /// Track the specified `message` with the specified
    /// `messageCorrelationId` with the corresponding queue specified
    /// by `queueId` so that the corresponding Ack message can be later
    /// processed via `onAckMessage(...)`.
    bool trackMessage(const bmqa::Message&       message,
                      const bmqa::QueueId&       queueId,
                      const bmqt::CorrelationId& messageCorrelationId);

    /// Stop tracking the message with the specified `messageCorrelationId`
    /// with the corresponding queue specified by `queueId`. Used for
    /// for cleanup in case postMessage(...) fails.
    bool untrackMessage(const bmqa::QueueId&       queueId,
                        const bmqt::CorrelationId& messageCorrelationId);

    /// Process the specified `message` belonging to an ACK event and update
    /// the appropriate counters of messages associated with this object and
    /// the message's `queueId`. Return 0 on success, or a negative error
    /// code if this ACK does not correspond to a message previously
    /// registered via `trackMessage(...)` for message's `queueId`.
    int onAckMessage(const bmqa::Message& message);

    bsls::Types::Int64 numMessagesPosted(const bmqa::QueueId& queueId) const;
    bsls::Types::Int64 numMessagesAcked(const bmqa::QueueId& queueId) const;

    /// Get the value of corresponding attribute associated with the
    /// specified `queueId`. Return negative error code if the specified
    /// `queueId` is not found.
    bsls::Types::Int64 numMessagesNAcked(const bmqa::QueueId& queueId) const;
};
// ------------------
// class EventHandler
// ------------------
// MANIPULATORS
void EventHandler::onSessionEvent(const bmqa::SessionEvent& sessionEvent)
{
    // This method is executed in one of the *EVENT HANDLER* threads

    BALL_LOG_SET_CATEGORY("EVENTHANDLER");

    BALL_LOG_INFO << "Got session event: " << sessionEvent;

    switch (sessionEvent.type()) {
    case bmqt::SessionEventType::e_CONNECTED: {
        // The connection to the broker is established (as a result of a call
        // to the 'start' method).  Application can now open the queues.
        BALL_LOG_INFO << "We are connected to the broker";
    } break;
    case bmqt::SessionEventType::e_DISCONNECTED: {
        // The connection to the broker is terminated (as a result of a call to
        // the 'stop' method).
        BALL_LOG_INFO << "We are disconnected from the broker";
    } break;
    case bmqt::SessionEventType::e_CONNECTION_LOST: {
        // The connection to the broker dropped.  This usually occurs due to
        // network issues or broker crash.  Producer application may not get
        // ACKs for outstanding PUT messages, and certain operations like
        // posting a message, opening or configuring a queue may return
        // failure.
        BALL_LOG_INFO << "Lost connection with the broker";
    } break;
    case bmqt::SessionEventType::e_STATE_RESTORED: {
        // The state with the broker has been restored (i.e., connection with
        // the broker has been reestablished *and* all queues which were
        // previously opened haven been re-opened again).  Note that in rare
        // cases, one or more queues may fail to re-open (see
        // 'e_QUEUE_REOPEN_RESULT' case).
        BALL_LOG_INFO << "Restored state with the broker";
    } break;
    case bmqt::SessionEventType::e_RECONNECTED: {
        // The connection to the broker has been reestablished.  Queues which
        // were opened before the connection went down have not yet been opened
        // and thus, any action on those queues may fail (producer may fail to
        // post messages), etc.
        BALL_LOG_INFO << "We are reconnected with the broker";
    } break;
    case bmqt::SessionEventType::e_CONNECTION_TIMEOUT: {
        // The connection to the broker has timed out.
        BALL_LOG_ERROR << "Connection to the broker has timed out. "
                       << "Please check if BlazingMQ broker is running on your"
                       << " machine";
    } break;
    case bmqt::SessionEventType::e_TIMEOUT: {
        // The method 'Session::nextEvent' timed out.
        BALL_LOG_INFO << "Timed out.";
    } break;
    case bmqt::SessionEventType::e_ERROR: {
        // An internal error occurred.
        BALL_LOG_ERROR << "Unexpected session error: "
                       << sessionEvent.errorDescription()
                       << ", status code: " << sessionEvent.statusCode();
    } break;
    case bmqt::SessionEventType::e_QUEUE_OPEN_RESULT: {
        // When using one of the flavors of open queue async API.
        if (sessionEvent.statusCode() == 0) {
            BALL_LOG_INFO << "The queue " << sessionEvent.queueId().uri()
                          << " is now open.";
        }
        else {
            // Queue failed to open.  This means that producer will not be able
            // to post messages.  Application should raise an alarm if
            // appropriate.
            BALL_LOG_ERROR << "The queue " << sessionEvent.queueId().uri()
                           << " failed to open with status code "
                           << sessionEvent.statusCode();
        }
    } break;
    case bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT: {
        // The queue-reopen operation is complete (queue-reopen occurs when
        // connection with the broker is reestablished after going down).
        if (sessionEvent.statusCode() == 0) {
            // Queue-reopen succeeded.
            BALL_LOG_INFO << "The queue " << sessionEvent.queueId().uri()
                          << " has been successfully reopened.";
        }
        else {
            // In rare cases, a queue may fail to reopen.  This means that
            // consumer will not get any PUSH messages, and producer will not
            // be able to POST messages.  Application should raise an alarm if
            // appropriate.
            BALL_LOG_ERROR << "The queue " << sessionEvent.queueId().uri()
                           << " failed to reopen with status code "
                           << sessionEvent.statusCode();
        }
    } break;
    case bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT: {
        // When using the close queue async
        if (sessionEvent.statusCode() == 0) {
            BALL_LOG_INFO << "The queue " << sessionEvent.queueId().uri()
                          << " is now close.";
        }
        else {
            BALL_LOG_ERROR << "The queue " << sessionEvent.queueId().uri()
                           << " failed to close with status code "
                           << sessionEvent.statusCode();
        }
    } break;
    case bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT: {
        // Result of one of the async flavors of Session.configureQueue API.
        if (sessionEvent.statusCode() == 0) {
            BALL_LOG_INFO << "The queue " << sessionEvent.queueId().uri()
                          << " has been configured";
        }
        else {
            BALL_LOG_ERROR << "The queue " << sessionEvent.queueId().uri()
                           << " failed to configure with status code "
                           << sessionEvent.statusCode();
        }
    } break;
    case bmqt::SessionEventType::e_SLOWCONSUMER_NORMAL: {
        BALL_LOG_INFO << "The event buffer inside BlazingMQ SDK has drained, "
                      << "and is back to normal state.";
    } break;
    case bmqt::SessionEventType::e_SLOWCONSUMER_HIGHWATERMARK: {
        BALL_LOG_WARN << "The event buffer inside BlazingMQ SDK is filling up "
                      << " and reached high watermark!  This indicates that "
                      << "producer is slow at processing ACKs or needs to "
                      << "process them in a separate thread or is not invoking"
                      << " 'Session::nextEvent' if using session in sync mode";
    } break;
    case bmqt::SessionEventType::e_HOST_UNHEALTHY:
    case bmqt::SessionEventType::e_HOST_HEALTH_RESTORED:
    case bmqt::SessionEventType::e_QUEUE_SUSPENDED:
    case bmqt::SessionEventType::e_QUEUE_RESUMED:
    case bmqt::SessionEventType::e_CANCELED:
    case bmqt::SessionEventType::e_UNDEFINED:
    default: {
        BALL_LOG_ERROR << "Got unexpected event: " << sessionEvent;
    };
    }
}

void EventHandler::onMessageEvent(const bmqa::MessageEvent& messageEvent)
{
    switch (messageEvent.type()) {
    case bmqt::MessageEventType::e_ACK: {
        // This event is received because we specified correlation IDs in the
        // messages, which requires the broker to send us acknowledgement
        // messages.  We can query the acknowledgement status (success/failure)
        // as well as the correlation ID of the published message.  See also
        // function 'postEvent'.
        bmqa::MessageIterator msgIter = messageEvent.messageIterator();
        while (msgIter.nextMessage()) {
            const bmqa::Message& msg = msgIter.message();
            queueManager()->onAckMessage(msg);
        }
    } break;
    case bmqt::MessageEventType::e_UNDEFINED:
    case bmqt::MessageEventType::e_PUT:
    case bmqt::MessageEventType::e_PUSH:
    default: {
        // We should not get any other message event than ACK since we are only
        // publishing...
        BALL_LOG_ERROR << "Got unexpected message event: " << messageEvent;
    } break;
    }
}

EventHandler& EventHandler::setQueueManager(QueueManager* value)
{
    d_queueManager_p = value;
    return *this;
}

// ACCESSORS
QueueManager* EventHandler::queueManager() const
{
    return d_queueManager_p;
}

// ------------------
// class QueueManager
// ------------------

QueueManager::QueueManager()
: d_queues()
, d_queuesLock(bsls::SpinLock::s_unlocked)
{
    // NOTHING
}

bool QueueManager::insertQueue(const bmqa::QueueId& queueId)
{
    bsls::SpinLockGuard guard(&d_queuesLock);  // LOCKED

    QueuesMap::iterator iter = d_queues.find(queueId.correlationId());
    if (iter == d_queues.end()) {
        d_queues.insert(
            bsl::make_pair(queueId.correlationId(), QueueContext()));
        return true;  // RETURN
    }
    return false;
}

void QueueManager::removeQueue(const bmqa::QueueId& queueId)
{
    bsls::SpinLockGuard guard(&d_queuesLock);  // LOCKED

    QueuesMap::iterator iter = d_queues.find(queueId.correlationId());
    BSLS_ASSERT_SAFE(iter != d_queues.end());
    d_queues.erase(iter);
}

bool QueueManager::trackMessage(
    const bmqa::Message&       message,
    const bmqa::QueueId&       queueId,
    const bmqt::CorrelationId& messageCorrelationId)
{
    const bmqt::CorrelationId& corrId = queueId.correlationId();

    bsls::SpinLockGuard guard(&d_queuesLock);  // LOCKED

    QueuesMap::iterator iter = d_queues.find(corrId);
    if (iter == d_queues.end()) {
        BALL_LOG_ERROR << "Unable to insert a message with correlationId '"
                       << messageCorrelationId << "'. Reason: "
                       << "Unknown queue: [queueId: " << queueId << "]";
        return false;  // RETURN
    }

    // Check if message with the same correlationId exists
    MessagesMap&                messagesPosted = iter->second.d_messagesPosted;
    MessagesMap::const_iterator cmiter         = messagesPosted.find(
        messageCorrelationId);
    if (cmiter != messagesPosted.end()) {
        BALL_LOG_ERROR << "Unable to insert message with correlation ID '"
                       << messageCorrelationId
                       << "' to queue [queueId: " << queueId
                       << "]. Reason: Message with correlation"
                       << " ID already exists.";
        return false;  // RETURN
    }
    messagesPosted.insert(bsl::make_pair(messageCorrelationId, message));
    iter->second.d_numMessagesPosted++;
    return true;
}

bool QueueManager::untrackMessage(
    const bmqa::QueueId&       queueId,
    const bmqt::CorrelationId& messageCorrelationId)
{
    const bmqt::CorrelationId& corrId = queueId.correlationId();

    bsls::SpinLockGuard guard(&d_queuesLock);  // LOCKED

    QueuesMap::iterator iter = d_queues.find(corrId);
    if (iter == d_queues.end()) {
        BALL_LOG_ERROR << "Unable to remove a message with correlationId '"
                       << messageCorrelationId << "'. Reason: "
                       << "Unknown queue: [queueId: " << queueId << "]";
        return false;  // RETURN
    }

    // find the message with the same correlationId
    MessagesMap&                messagesPosted = iter->second.d_messagesPosted;
    MessagesMap::const_iterator cmiter         = messagesPosted.find(
        messageCorrelationId);
    if (cmiter == messagesPosted.end()) {
        BALL_LOG_ERROR << "Unable to remove message with correlation ID '"
                       << messageCorrelationId
                       << "' to queue [queueId: " << queueId
                       << "]. Reason: Message with correlation"
                       << " ID doesn't exists.";
        return false;  // RETURN
    }
    messagesPosted.erase(messageCorrelationId);
    iter->second.d_numMessagesPosted--;
    return true;
}

int QueueManager::onAckMessage(const bmqa::Message& message)
{
    const bmqt::CorrelationId& corrId = message.queueId().correlationId();

    bsls::SpinLockGuard guard(&d_queuesLock);  // LOCKED

    QueuesMap::iterator iter = d_queues.find(corrId);
    if (iter == d_queues.end()) {
        BALL_LOG_ERROR << "Got Ack/NAck message from the broker with "
                       << "correlation ID '" << message.correlationId()
                       << "' and GUID '" << message.messageGUID()
                       << "' for an unknown queue: [queueId: "
                       << message.queueId() << "]";
        return rc_UNKNOWN_QUEUE;  // RETURN
    }

    // Check if message with the given correlationId exists
    const bmqt::CorrelationId& messageCorrelationId = message.correlationId();
    MessagesMap&               messagesPosted = iter->second.d_messagesPosted;
    MessagesMap::iterator miter = messagesPosted.find(messageCorrelationId);
    if (miter == messagesPosted.end()) {
        // Unknown correlation Id is an error condition in two cases:
        // o If this is successful ACK message (i.e., ack status is zero).
        // o This is a failed ACK message ("NAK") with non empty correlationId.
        //
        // Note that receiving a failed ACK message ("NAK") with empty
        // correlationId is a valid case.  This can occur when a PUT message is
        // sent without correlationId (i.e., application is not interested in
        // receiving an ACK), but BlazingMQ fails to accept the PUT message for
        // some reason (queue quota exceeded, etc.).  In such cases, BlazingMQ
        // sends a NAK message to the application.  For such NAK messages,
        // correlationId will be unset, since no correlationId was specified
        // when PUT message was posted.

        if (message.ackStatus() == 0) {
            BALL_LOG_ERROR << "Got ACK message from the broker with unknown "
                           << "message correlation ID '"
                           << message.correlationId() << "' and GUID '"
                           << message.messageGUID() << "' for queue: [queueId:"
                           << " " << message.queueId() << "]";

            return rc_UNKNOWN_MESSAGE;  // RETURN
        }
        else if (message.correlationId().isUnset()) {
            BALL_LOG_ERROR << "Got NAK message from the broker with unset "
                           << "message correlation ID and GUID '"
                           << message.messageGUID()
                           << "' for queue: [queueId: " << message.queueId()
                           << "].  Ack status: "
                           << bmqt::AckResult::toAscii(
                                  static_cast<bmqt::AckResult::Enum>(
                                      message.ackStatus()));

            return rc_SUCCESS;  // RETURN
        }
        else {
            BALL_LOG_ERROR << "Got NAK message from the broker with unknown "
                           << "message correlation ID '"
                           << message.correlationId() << "' and GUID '"
                           << message.messageGUID()
                           << "' for queue: [queueId: " << message.queueId()
                           << "].  Ack status: "
                           << bmqt::AckResult::toAscii(
                                  static_cast<bmqt::AckResult::Enum>(
                                      message.ackStatus()));

            return rc_UNKNOWN_MESSAGE;  // RETURN
        }
    }

    if (message.ackStatus() == 0) {
        BALL_LOG_INFO << "Got Ack message from the broker with message "
                      << "correlation ID '" << message.correlationId()
                      << "' and GUID '" << message.messageGUID()
                      << "' for queue: [queueId: " << message.queueId() << "]";
        iter->second.d_numMessagesAcked++;
    }
    else {
        BALL_LOG_INFO << "Got NAck message from the broker with message "
                      << "correlation ID '" << message.correlationId()
                      << "' and GUID '" << message.messageGUID()
                      << "' for queue: [queueUri: " << message.queueId().uri()
                      << ", queueId: " << message.queueId()
                      << "].  Ack status: "
                      << bmqt::AckResult::toAscii(
                             static_cast<bmqt::AckResult::Enum>(
                                 message.ackStatus()));
        iter->second.d_numMessagesNAcked++;
    }
    messagesPosted.erase(miter);
    return rc_SUCCESS;
}

bsls::Types::Int64
QueueManager::numMessagesPosted(const bmqa::QueueId& queueId) const
{
    bsls::SpinLockGuard guard(&d_queuesLock);  // LOCKED

    QueuesMap::const_iterator citer = d_queues.find(queueId.correlationId());
    if (citer == d_queues.end()) {
        return rc_UNKNOWN_QUEUE;  // RETURN
    }
    return citer->second.d_numMessagesPosted;
}

bsls::Types::Int64
QueueManager::numMessagesAcked(const bmqa::QueueId& queueId) const
{
    bsls::SpinLockGuard guard(&d_queuesLock);  // LOCKED

    QueuesMap::const_iterator citer = d_queues.find(queueId.correlationId());
    if (citer == d_queues.end()) {
        return rc_UNKNOWN_QUEUE;  // RETURN
    }
    return citer->second.d_numMessagesAcked;
}

bsls::Types::Int64
QueueManager::numMessagesNAcked(const bmqa::QueueId& queueId) const
{
    bsls::SpinLockGuard guard(&d_queuesLock);  // LOCKED

    QueuesMap::const_iterator citer = d_queues.find(queueId.correlationId());
    if (citer == d_queues.end()) {
        return rc_UNKNOWN_QUEUE;  // RETURN
    }
    return citer->second.d_numMessagesNAcked;
}

//=============================================================================
//                                PRODUCER
//-----------------------------------------------------------------------------

static bool postEvent(const bsl::string&   text,
                      const bmqa::QueueId& queueId,
                      bmqa::Session*       session,
                      QueueManager*        queueManager)
{
    BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);

    // Build a 'MessageEvent' containing a single message and add it to the
    // queue.

    // We first create a message event builder on the stack.  We create it with
    // the help of the session in order to reuse the session's blob buffer
    // factory.  The builder holds the message event under construction.  Note
    // that the builder is NOT thread safe.
    bmqa::MessageEventBuilder builder;
    session->loadMessageEventBuilder(&builder);

    // We will associate some properties with the published message.  In order
    // to do so, we first create a valid instance of message properties.
    bmqa::MessageProperties properties;
    session->loadMessageProperties(&properties);

    int rc;

    // We now associate two properties with this instance.
    rc = properties.setPropertyAsInt64(
        "timestamp",
        bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()));
    if (rc != 0) {
        // Failed to set a property.  This may or may not be a fatal error for
        // an application.  In this case, we treat it as non-fatal.

        BALL_LOG_ERROR << "Failed to set 'timestamp' property on the message, "
                       << "rc: " << rc;
    }

    rc = properties.setPropertyAsInt32("routingId", 42);
    if (rc != 0) {
        // Failed to set a property.  This may or may not be a fatal error for
        // an application.  In this case, we treat it as non-fatal.

        BALL_LOG_ERROR << "Failed to set 'routingId' property on the message, "
                       << "rc: " << rc;
    }

    // Create a new message in the builder.
    //
    // The builder has the notion of the current message, i.e. the message that
    // is to be appended into the message event.  In that sense the builder
    // acts like an iterator.  The 'startMessage' method zeros out the current
    // message and returns a reference to the current message.
    bmqa::Message& message = builder.startMessage();

    // At this point, 'message' points to the current message in the builder.
    // Be aware that 'message' is only valid as long as the builder itself is
    // valid.

    // Associated properties with the message.  Note that 'properties' object
    // must be valid until the call to 'packMessage'.
    message.setPropertiesRef(&properties);

    // Set the payload into the message.  Note that you can also set the
    // content as a blob; also note that the payload must be valid until the
    // call to 'packMessage'.
    message.setDataRef(text.c_str(), text.length());

    // Set the correlation ID into the message.  Since we specified ACK flag
    // while opening the queue, we must specify correlation ID for the message.
    // We can use this correlation ID to identify the message when broker sends
    // us an ACK message.  Usually, producer would specify a more meaningful
    // value for correlation ID, but for simplicity, we use the 'autoValue()'
    // routine which automatically generates an ID for us.
    const bmqt::CorrelationId msgCorrelationId =
        bmqt::CorrelationId::autoValue();
    message.setCorrelationId(msgCorrelationId);

    // Once our message is ready, we add it into the builder and associate it
    // with a queue using the queue id.
    rc = builder.packMessage(queueId);
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to pack message: rc: "
                       << bmqt::EventBuilderResult::Enum(rc);
        return false;  // RETURN
    }

    // If we wanted to add a new message into the event, we would call
    // 'startMessage' again, then set all the data and finally call
    // 'packMessage'.  Note that 'startMessage' resets all fields of the new
    // message; you don't need to call it if you want to add a new message that
    // is similar to the previous one but with a few fields changed.  For
    // example, a common use case is to send the same message to a different
    // queue; to do this we would only call 'setCorrelationId' and
    // 'packMessage' with a different correlation ID and a different queue ID.

    // Extract the 'MessageEvent' from the builder, and send it to the broker.
    // The broker will distribute the message to the corresponding queues (in
    // our case, just one queue).  Note that after extracting the built event,
    // the builder must be reset before it can be reused.
    const bmqa::MessageEvent& messageEvent = builder.messageEvent();
    queueManager->trackMessage(message, queueId, msgCorrelationId);
    bdlb::ScopeExitAny untrackGuard(
        bdlf::BindUtil::bind(&QueueManager::untrackMessage,
                             queueManager,
                             queueId,
                             msgCorrelationId));
    rc = session->post(messageEvent);
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to post message: rc: " << rc;
        return false;  // RETURN
    }

    untrackGuard.release();
    return true;
}

static void produce(bmqa::Session* session, QueueManager* queueManager)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(session);
    BSLS_ASSERT_SAFE(queueManager);

    BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);

    // Open a queue. The queue is created if it does not exist already.  Each
    // queue is identified by a short URL containing a namespace and a queue
    // name.
    //
    // The application also specifies a mode (read, write or both).  If the
    // application opens a queue in read and write mode, it is able to read its
    // own messages.
    //
    // In this example we also ask the queue to be opened in ACK mode.  When
    // this flag is specified, SDK enforces producer code to specify a
    // correlation ID for each message that it posts on the queue.  If producer
    // fails to specify correlation ID while building a message,
    // bmqa::MessageEventBuilder::packMessage() will return an appropriate
    // error.
    //
    // Finally the application specifies a queue ID, which can be an integer, a
    // pointer or a smart pointer; it is used by the application to correlate a
    // received event back to a queue.  In this example we only use one queue
    // so it doesn't really matter.
    //
    // NOTE: it is designed this way because we believe it is better for the
    //       application to provide its own IDs than for BlazingMQ to supply a
    //       queue IDs.
    const char k_QUEUE_URL[] = "bmq://bmq.tutorial.hello/test-queue";
    const int  k_QUEUE_ID    = 1;

    bmqa::QueueId         queueId(k_QUEUE_ID);
    bmqa::OpenQueueStatus openStatus = session->openQueueSync(
        &queueId,
        k_QUEUE_URL,
        bmqt::QueueFlags::e_WRITE | bmqt::QueueFlags::e_ACK);

    // Note that 'openQueue()' takes 2 more optional parameters - queue options
    // and timeout.  See 'openQueue()' documentation for more details.

    if (!openStatus) {
        BALL_LOG_ERROR << "Failed to open queue: '" << k_QUEUE_URL
                       << "', status: " << openStatus;
        return;  // RETURN
    }

    // Keep track of 'queueId'
    queueManager->insertQueue(queueId);

    bool more = true;
    while (more) {
        BALL_LOG_INFO << "Enter a message to publish, or an empty message to "
                      << "exit (e.g. just press return)";

        bsl::string input;
        readUserInput(&input);
        if (!input.empty()) {
            more = postEvent(input, queueId, session, queueManager);
        }
        else {
            more = false;
        }
    }
    bmqa::CloseQueueStatus closeStatus = session->closeQueueSync(&queueId);

    BALL_LOG_INFO_BLOCK
    {
        bdlb::Print::newlineAndIndent(BALL_LOG_OUTPUT_STREAM, 1, 4);
        BALL_LOG_OUTPUT_STREAM << "Num messages posted: "
                               << queueManager->numMessagesPosted(queueId);
        bdlb::Print::newlineAndIndent(BALL_LOG_OUTPUT_STREAM, 1, 4);
        BALL_LOG_OUTPUT_STREAM << "Num messages Acked.: "
                               << queueManager->numMessagesAcked(queueId);
        bdlb::Print::newlineAndIndent(BALL_LOG_OUTPUT_STREAM, 1, 4);
        BALL_LOG_OUTPUT_STREAM << "Num messages NAcked: "
                               << queueManager->numMessagesNAcked(queueId);
    }

    queueManager->removeQueue(queueId);
    if (!closeStatus) {
        BALL_LOG_ERROR << "Failed to close queue: '" << k_QUEUE_URL
                       << "', status: " << closeStatus;
        return;  // RETURN
    }
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(BSLS_ANNOTATION_UNUSED int         argc,
         BSLS_ANNOTATION_UNUSED const char* argv[])
{
    // Set up logging with output to the console and verbosity set to
    // INFO-level.  This way we will get logs from the BlazingMQ SDK.
    const bsl::string           logFormat = "%d (%t) %c %s [%F:%l] %m\n";
    const ball::Severity::Level level     = ball::Severity::INFO;

    ball::FileObserver observer;
    observer.disableFileLogging();
    observer.setLogFormat(logFormat.c_str(), logFormat.c_str());
    observer.setStdoutThreshold(level);

    ball::LoggerManagerConfiguration config;
    config.setDefaultThresholdLevelsIfValid(ball::Severity::OFF,
                                            level,
                                            ball::Severity::OFF,
                                            ball::Severity::OFF);

    ball::LoggerManagerScopedGuard manager(&observer, config);

    BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);

    // Start a session with the message broker.  This makes the SDK connect to
    // the local broker by default, unless the 'Session' is created with an
    // optional 'SessionOptions' object.  Note that no synchronous operation
    // may be called on the 'Session' before it is started.
    BALL_LOG_INFO << "Starting the session with the BlazingMQ broker";

    QueueManager  queueManager;
    EventHandler* eventHandler = new EventHandler();
    eventHandler->setQueueManager(&queueManager);

    bslma::ManagedPtr<bmqa::SessionEventHandler> eventHandlerMP(eventHandler);
    bmqa::Session                                session(eventHandlerMP);

    int rc = session.start();
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to start the session with the BlazingMQ "
                       << "broker, rc: " << bmqt::GenericResult::Enum(rc);
        return rc;  // RETURN
    }

    // We are connected to the broker. The next step is to open the queue and
    // publish messages.
    BALL_LOG_INFO << "Starting the producer";
    produce(&session, &queueManager);

    BALL_LOG_INFO << "Stopping the session with the BlazingMQ broker";
    session.stop();

    return 0;
}
