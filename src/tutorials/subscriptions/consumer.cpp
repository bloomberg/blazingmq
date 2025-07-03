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

// consumer.cpp                                                       -*-C++-*-

// This file is part of the BlazingMQ tutorial "Subscriptions".
//
// The use case involves a Publisher task, a Consumer task, and a Message
// Broker.  The Producer and/or the Consumer create a named queue in the
// Broker.  The Producer pushes messages into the queue, and the consumer
// pulls them using different subscriptions.  Once a message has been consumed
// it can be removed from the queue.
//
// In essence the queue is a mailbox for messages to be sent to a Consumer.  It
// allows decoupling the processing between the Producer and the Consumer: one
// of the tasks may be down without impacting the other, and the Consumer can
// process messages at a different pace than the Producer.

// BMQ
#include <bmqa_closequeuestatus.h>
#include <bmqa_configurequeuestatus.h>
#include <bmqa_event.h>
#include <bmqa_messageproperties.h>
#include <bmqa_openqueuestatus.h>
#include <bmqa_queueid.h>
#include <bmqa_session.h>
#include <bmqt_correlationid.h>
#include <bmqt_queueflags.h>
#include <bmqt_resultcode.h>
#include <bmqt_subscription.h>

// BDE
#include <ball_fileobserver.h>
#include <ball_log.h>
#include <ball_loggermanager.h>
#include <ball_loggermanagerconfiguration.h>
#include <bdlb_string.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bsl_algorithm.h>
#include <bsl_ios.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_sstream.h>
#include <bsl_unordered_map.h>
#include <bsl_utility.h>
#include <bsla_annotations.h>
#include <bslh_hash.h>
#include <bsls_assert.h>
#include <bsls_spinlock.h>
#include <bsls_systemclocktype.h>
#include <bsls_systemtime.h>
#include <bsls_types.h>

using namespace BloombergLP;

namespace {

// FUNCTIONS
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

/// Print the current property value associated with the specified
/// `iterator` to the specify `stream`.
static void printPropertyValue(bsl::ostream&                          stream,
                               const bmqa::MessagePropertiesIterator& iterator)
{
    bmqt::PropertyType::Enum ptype = iterator.type();
    BSLS_ASSERT(bmqt::PropertyType::e_UNDEFINED != ptype);

    switch (ptype) {
    case bmqt::PropertyType::e_BOOL: {
        stream << bsl::boolalpha << iterator.getAsBool();
    } break;  // BREAK

    case bmqt::PropertyType::e_CHAR: {
        stream << iterator.getAsChar();
    } break;  // BREAK

    case bmqt::PropertyType::e_SHORT: {
        stream << iterator.getAsShort();
    } break;  // BREAK

    case bmqt::PropertyType::e_INT32: {
        stream << iterator.getAsInt32();
    } break;  // BREAK

    case bmqt::PropertyType::e_INT64: {
        stream << iterator.getAsInt64();
    } break;  // BREAK

    case bmqt::PropertyType::e_STRING: {
        stream << iterator.getAsString();
    } break;  // BREAK

    case bmqt::PropertyType::e_BINARY: {
        stream << " ** BINARY **";
    } break;  // BREAK

    case bmqt::PropertyType::e_UNDEFINED:
    default: BSLS_ASSERT_OPT(false && "Unreachable by design.");
    }
}

// CLASSES
class QueueManager;

// ==================
// class EventHandler
// ==================

/// Concrete implementation of an event handler.  Note that the methods are
/// called on the session's own threads.
class EventHandler : public bmqa::SessionEventHandler {
  private:
    // DATA
    bmqa::Session* d_session_p;
    // Pointer to session (held, not owned)

    QueueManager* d_queueManager_p;
    // Pointer to the queue manager (held,
    // not owned)

    bmqt::SubscriptionHandle* d_subscriptionHandle_p;
    // Pointer to the subscription handle
    // used to identify messages from the
    // specific subscription (held, not
    // owned)

  public:
    // MANIPULATORS

    /// Process the specified 'sessionEvent' received from the broker.
    void onSessionEvent(const bmqa::SessionEvent& sessionEvent)
        BSLS_KEYWORD_OVERRIDE;

    /// Process the specified 'messageEvent' received from the broker.
    void onMessageEvent(const bmqa::MessageEvent& messageEvent)
        BSLS_KEYWORD_OVERRIDE;

    /// Set the associated member of this event handler to the specified
    /// `value` and return a reference offering modifiable access to this
    /// object.
    EventHandler& setSession(bmqa::Session* value);
    EventHandler& setQueueManager(QueueManager* value);
    EventHandler& setSubscriptionHandle(bmqt::SubscriptionHandle* value);

    // ACCESSORS

    /// Return a pointer to the corresponding member associated with this
    /// event handler.
    bmqa::Session*            session() const;
    QueueManager*             queueManager() const;
    bmqt::SubscriptionHandle* subscriptionHandle() const;
};

// ==================
// class QueueManager
// ==================

/// Object to keep track of application queues and associated contexts.
class QueueManager {
  public:
    // TYPES
    enum QueueState { e_OPENING, e_OPENED, e_CLOSING, e_CLOSED };

    /// Struct holding members associated with the state of a queue.
    struct QueueContext {
        // DATA
        bmqa::QueueId d_queueId;

        bsls::Types::Int64 d_numMessagesReceived;

        QueueState d_state;
    };

  private:
    // PRIVATE TYPES

    /// `QueuesMap` is an alias for a map of contexts associated with a
    /// queue.
    ///
    /// * queueCorrelationId -> queueContext
    typedef bsl::unordered_map<bmqt::CorrelationId, QueueContext> QueuesMap;

  private:
    // DATA
    QueuesMap d_queues;

    mutable bsls::SpinLock d_queuesLock;

  public:
    // CREATORS

    /// Create a new `QueueManager` object.
    explicit QueueManager();

    // MANIPULATORS

    /// Insert the specified `context` into the queues being managed by this
    /// object.
    void insertQueueContext(const QueueContext& context);

    /// Increment the total count of messages processed for the queue
    /// identified by the specified `corrId` by the specified `delta`.
    /// Return the new total count if the queue is found or -1 otherwise.
    /// The behavior is undefined unless `delta >= 0`.
    bsls::Types::Int64 incrementMessageCount(bmqt::CorrelationId corrId,
                                             bsls::Types::Int64  delta);

    // ACCESSORS

    /// Return the total count of messages processed for the queue
    /// identified by the specified `corrId`.
    bsls::Types::Int64
    getMessageCount(const bmqt::CorrelationId& corrId) const;
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
        // network issues or broker crash.  Consumer application will not get
        // new PUSH messages from now on, and certain operations like opening
        // or configuring a queue, or confirming a message may return failure.
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
        // post messages, consumer will not get new PUSH messages), etc.
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
            // Queue failed to open.  This means that consumer will not get any
            // PUSH messages, and producer will not be able to post messages.
            // Application should raise an alarm if appropriate.
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
                      << "consumer is slow at processing PUSH messages or "
                      << "needs to process them in a separate thread or is not"
                      << "invoking 'Session::nextEvent' if using session in "
                      << "sync mode.";
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
    // This method is executed in one of the *EVENT HANDLER* threads

    BALL_LOG_SET_CATEGORY("EVENTHANDLER");

    switch (messageEvent.type()) {
    case bmqt::MessageEventType::e_PUSH: {
        // Load a ConfirmEventBuilder from the session
        bmqa::ConfirmEventBuilder confirmBuilder;
        session()->loadConfirmEventBuilder(&confirmBuilder);

        // Received a 'PUSH' event from the broker.
        bmqa::MessageIterator msgIter = messageEvent.messageIterator();
        while (msgIter.nextMessage()) {
            const bmqa::Message& msg = msgIter.message();

            // Note that 'message' is only valid within the scope of the
            // iterator and the containing message event.  In other words
            // 'message' is merely a pointer to the first byte of the message
            // inside the message event.  If the application wanted to extract
            // the whole message in order to pass it to another thread for
            // example, it can do so by cloning the message: the clone is
            // "detached" from the message event (this involves copying and
            // therefore is not cheap).

            // Get the data in the message.
            bdlbb::Blob data;
            int         rc = msg.getData(&data);
            if (rc == 0) {
                BALL_LOG_INFO << "Got message with GUID: " << msg.messageGUID()
                              << ", correlationId: " << msg.correlationId()
                              << ", data:\n"
                              << bdlbb::BlobUtilHexDumper(&data);
            }
            else {
                // This should never happen (unless memory corruption perhaps)
                BALL_LOG_ERROR << "Failed to decode message with GUID: "
                               << msg.messageGUID() << ", rc: " << rc;
            }

            // Check if the message belongs to the 'foo' subscription.
            if (*subscriptionHandle() == msg.subscriptionHandle()) {
                BALL_LOG_INFO << "The message belongs to the specific "
                              << "subscription";

                // Apply custom logic here.
            }

            // Check if any properties are associated with the message.  Note
            // that BlazingMQ framework does not enforce any format for the
            // message properties, and it is upto the producers and consumers
            // to negotiate.  Also note that in this example, we will simply
            // iterate over the properties and print them.  Typically,
            // applications would use properties for internal routing,
            // filtering, pipelining, etc.
            if (msg.hasProperties()) {
                bmqa::MessageProperties properties;
                rc = msg.loadProperties(&properties);
                if (rc != 0) {
                    BALL_LOG_ERROR << "Failed to load properties from msg "
                                   << "with GUID: " << msg.messageGUID()
                                   << ", rc: " << rc;
                }
                else {
                    BALL_LOG_INFO << "Number of properties in msg with GUID: "
                                  << msg.messageGUID() << ": "
                                  << properties.numProperties();

                    // Iterate over 'properties' and print each one.
                    bmqa::MessagePropertiesIterator propIter(&properties);
                    while (propIter.hasNext()) {
                        bsl::ostringstream propertyValueStream;
                        printPropertyValue(propertyValueStream, propIter);
                        BALL_LOG_INFO << "PropertyName [" << propIter.name()
                                      << "], PropertyType [" << propIter.type()
                                      << "], PropertyValue ["
                                      << propertyValueStream.str() << "].";
                    }
                }
            }

            rc = confirmBuilder.addMessageConfirmation(msg);
            if (rc != 0) {
                BALL_LOG_ERROR << "Failed to add confirm message with GUID: "
                               << msg.messageGUID()
                               << " to the builder, rc: " << rc;
            }

            rc = queueManager()->incrementMessageCount(
                msg.queueId().correlationId(),
                1);
            if (rc == -1) {
                BALL_LOG_ERROR << "Queue associated with this message was not "
                               << "found! [queueId: " << msg.queueId() << "]";
            }
        }

        // Confirm reception of the messages so that it can be deleted from the
        // queue.  Confirming means that the broker is free to purge the
        // message according to the retention policy defined for this queue
        // domain.  Note that we are confirming in batch.  Also note that
        // 'Session::confirmMessages' method resets the 'builder' in case of
        // success.

        int rc = session()->confirmMessages(&confirmBuilder);
        if (rc != 0) {
            BALL_LOG_ERROR << "Failed to confirm "
                           << confirmBuilder.messageCount() << " messages"
                           << ", rc: " << rc;

            // Since 'Session::confirmMessages' failed, it would not have reset
            // the 'builder'.  So we reset it here.  Note that this, to an
            // extent, depends on the reason for failure.  If its a transient
            // error, the right way would be *not* to reset the builder so that
            // we can try again.  If its a fatal error (eg,
            // bmqt::GenericResult::e_INVALID_ARGUMENT), there is no option but
            // to reset the builder.

            confirmBuilder.reset();
        }

    } break;
    case bmqt::MessageEventType::e_UNDEFINED:
    case bmqt::MessageEventType::e_PUT:
    case bmqt::MessageEventType::e_ACK:
    default: {
        // We should not get any other message event than PUSH since we are
        // only consuming...
        BALL_LOG_ERROR << "Got unexpected event type: " << messageEvent.type();
    } break;
    }
}

EventHandler& EventHandler::setSession(bmqa::Session* value)
{
    d_session_p = value;
    return *this;
}

EventHandler& EventHandler::setQueueManager(QueueManager* value)
{
    d_queueManager_p = value;
    return *this;
}

EventHandler&
EventHandler::setSubscriptionHandle(bmqt::SubscriptionHandle* value)
{
    d_subscriptionHandle_p = value;
    return *this;
}

// ACCESSORS
bmqa::Session* EventHandler::session() const
{
    return d_session_p;
}

QueueManager* EventHandler::queueManager() const
{
    return d_queueManager_p;
}

bmqt::SubscriptionHandle* EventHandler::subscriptionHandle() const
{
    return d_subscriptionHandle_p;
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

void QueueManager::insertQueueContext(const QueueContext& context)
{
    bsls::SpinLockGuard guard(&d_queuesLock);  // LOCKED
    d_queues.insert(
        bsl::make_pair(context.d_queueId.correlationId(), context));
}

bsls::Types::Int64
QueueManager::incrementMessageCount(bmqt::CorrelationId corrId,
                                    bsls::Types::Int64  delta)
{
    // PRECONDITIONS
    BSLS_ASSERT(delta >= 0);

    bsls::SpinLockGuard guard(&d_queuesLock);  // LOCKED

    QueuesMap::const_iterator iter = d_queues.find(corrId);
    if (iter == d_queues.end()) {
        return -1;  // RETURN
    }

    // Protect against overflow
    const bsls::Types::Int64 remaining =
        bsl::numeric_limits<bsls::Types::Int64>::max() -
        d_queues[corrId].d_numMessagesReceived;
    d_queues[corrId].d_numMessagesReceived += bsl::min(remaining, delta);

    return d_queues[corrId].d_numMessagesReceived;
}

// ACCESSORS
bsls::Types::Int64
QueueManager::getMessageCount(const bmqt::CorrelationId& corrId) const
{
    bsls::SpinLockGuard       guard(&d_queuesLock);  // LOCKED
    QueuesMap::const_iterator citer = d_queues.find(corrId);
    if (citer == d_queues.end()) {
        return -1;  // RETURN
    }

    return citer->second.d_numMessagesReceived;
}

}  // close unnamed namespace

//=============================================================================
//                                 CONSUMER
//-----------------------------------------------------------------------------

static void consume(bmqa::Session*            session,
                    QueueManager*             queueManager,
                    bmqt::SubscriptionHandle* sub1Handle)
{
    BALL_LOG_SET_CATEGORY("CONSUMER.CONSUME");

    // Open a queue. The queue is created if it does not exist already.  Each
    // queue is identified by a short URL containing a namespace and a queue
    // name.
    //
    // The application also specifies a mode (read, write or both).  If the
    // application opens a queue in read and write mode, it is able to read its
    // own messages.
    //
    // Finally the application specifies a queue id, which can be an integer, a
    // pointer or a smart pointer; it is used by the application to correlate a
    // received event back to a queue.  In this example we only use one queue
    // so it doesn't really matter.  We believe it is far better for the
    // application to provide its own ids than for BlazingMQ to supply a queue
    // ids.
    const char k_QUEUE_URL[] = "bmq://bmq.test.mem.priority/test-queue";

    bsl::string        error;
    bmqt::QueueOptions queueOptions;
    queueOptions.setConsumerPriority(5)
        .setMaxUnconfirmedMessages(64)
        .setMaxUnconfirmedBytes(4 * 1024 * 1024);

    // Populate 'queueOptions' with several subscriptions.
    //
    // Notes:
    // 1. Received messages contain 'correlationId' field that represents the
    // value from the corresponding subscription.
    // 2. Received messages contain 'subscriptionHandle' field that represents
    // the value from the corresponding subscription.
    // 3. Property names, values and expressions are case-sensitive.
    // 4. It is possible to set consumer priority, max unconfirmed messages,
    // max unconfirmed bytes for any specific subscription.  If these
    // parameters were not set explicitly, the bmqt::QueueOptions' parameters
    // for the entire open/configure request will be used.
    {
        bmqt::Subscription           subscription;
        bmqt::SubscriptionExpression expression(
            "firmId == \"FOO\"",
            bmqt::SubscriptionExpression::e_VERSION_1);

        // Specific priority and max unconfirmed messages/bytes only for this
        // subscription.  Other subscriptions will use parameters from the
        // 'queueOptions'.
        subscription.setExpression(expression)
            .setConsumerPriority(10)
            .setMaxUnconfirmedMessages(128)
            .setMaxUnconfirmedBytes(1024 * 1024);

        if (!queueOptions.addOrUpdateSubscription(&error,
                                                  *sub1Handle,
                                                  subscription)) {
            // Error! Log something
            BALL_LOG_ERROR << "<-- queueOptions.addOrUpdateSubscription() => "
                           << error;
            return;  // RETURN
        }
    }
    {
        bmqt::CorrelationId          corrId(bmqt::CorrelationId::autoValue());
        bmqt::SubscriptionHandle     handle(corrId);
        bmqt::Subscription           subscription;
        bmqt::SubscriptionExpression expression(
            "firmId == \"BAR\" && price < 25",
            bmqt::SubscriptionExpression::e_VERSION_1);

        subscription.setExpression(expression);

        if (!queueOptions.addOrUpdateSubscription(&error,
                                                  handle,
                                                  subscription)) {
            // Error! Log something
            BALL_LOG_ERROR << "<-- queueOptions.addOrUpdateSubscription() => "
                           << error;
            return;  // RETURN
        }
    }
    {
        bmqt::CorrelationId          corrId(bmqt::CorrelationId::autoValue());
        bmqt::SubscriptionHandle     handle(corrId);
        bmqt::Subscription           subscription;
        bmqt::SubscriptionExpression expression(
            "firmId == \"BAR\" && price >= 25",
            bmqt::SubscriptionExpression::e_VERSION_1);

        subscription.setExpression(expression);

        if (!queueOptions.addOrUpdateSubscription(&error,
                                                  handle,
                                                  subscription)) {
            // Error! Log something
            BALL_LOG_ERROR << "<-- queueOptions.addOrUpdateSubscription() => "
                           << error;
            return;  // RETURN
        }
    }

    bmqt::CorrelationId corrId(bmqt::CorrelationId::autoValue());
    bmqa::QueueId       queueId(corrId);

    bmqa::OpenQueueStatus status = session->openQueueSync(
        &queueId,
        k_QUEUE_URL,
        bmqt::QueueFlags::e_READ,
        queueOptions);

    if (!status || !queueId.isValid()) {
        // Error! Log something
        BALL_LOG_ERROR << "<-- session->openQueueSync() => "
                       << bmqt::OpenQueueResult::Enum(status.result()) << " ("
                       << status << ")";
        return;  // RETURN
    }

    // Insert queue context
    QueueManager::QueueContext context;
    context.d_queueId             = status.queueId();
    context.d_numMessagesReceived = 0;
    context.d_state               = QueueManager::e_OPENED;
    queueManager->insertQueueContext(context);

    // Queue has been opened. Wait for events and process them.
    bool more = true;
    while (more) {
        BALL_LOG_INFO << "Stop? [y/n]";
        bsl::string input;
        readUserInput(&input);
        more = !(input == "y");
    }

    // Graceful shutdown by first synchronously shutting the flow of incoming
    // messages and only then closing the queue.
    bmqt::QueueOptions options;
    options.setMaxUnconfirmedMessages(0).setMaxUnconfirmedBytes(0);

    bmqa::ConfigureQueueStatus configureStatus =
        session->configureQueueSync(&queueId, options);

    if (!configureStatus) {
        // Error! Log something
        BALL_LOG_ERROR << "<-- session->configureQueueSync() => "
                       << bmqt::ConfigureQueueResult::Enum(
                              configureStatus.result())
                       << " (" << configureStatus << ")";
        return;  // RETURN
    }

    BALL_LOG_INFO << "Queue ['" << queueId.uri() << "'] will no longer "
                  << "receive new messages. It received a total of "
                  << queueManager->getMessageCount(queueId.correlationId())
                  << " messages.";

    // Close the queue (not really needed in this example but provided for
    // completeness).
    bmqa::CloseQueueStatus closeStatus = session->closeQueueSync(&queueId);
    if (!closeStatus) {
        // Error! Log something
        BALL_LOG_ERROR << "<-- session->closeQueueSync() => "
                       << bmqt::CloseQueueResult::Enum(closeStatus.result())
                       << " (" << closeStatus << ")";
        return;  // RETURN
    }

    BALL_LOG_INFO << "Queue ['" << queueId.uri() << "'] has been shut down "
                  << "gracefully and is now closed.";
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(BSLA_UNUSED int argc, BSLA_UNUSED const char* argv[])
{
    // Set up logging with output to the console and verbosity set to
    // INFO-level.  This way we will get logs from the BlazingMQ SDK.
    const bsl::string           logFormat = "%d (%t) %c %s [%F:%l] %m\n";
    const ball::Severity::Level level     = ball::Severity::e_INFO;

    ball::FileObserver observer;
    observer.disableFileLogging();
    observer.setLogFormat(logFormat.c_str(), logFormat.c_str());
    observer.setStdoutThreshold(level);

    ball::LoggerManagerConfiguration config;
    config.setDefaultThresholdLevelsIfValid(ball::Severity::e_OFF,
                                            level,
                                            ball::Severity::e_OFF,
                                            ball::Severity::e_OFF);

    ball::LoggerManagerScopedGuard manager(&observer, config);
    BALL_LOG_SET_CATEGORY("CONSUMER.MAIN");

    // Start a session with the message broker.  This makes the SDK connect to
    // the local broker by default, unless the 'Session' is created with an
    // optional 'SessionOptions' object.  Note that no synchronous operation
    // may be called on the 'Session' before it is started.
    BALL_LOG_INFO << "Starting the session with the BlazingMQ broker";

    EventHandler* eventHandler = new EventHandler();

    QueueManager queueManager;
    eventHandler->setQueueManager(&queueManager);

    bslma::ManagedPtr<bmqa::SessionEventHandler> eventHandlerMp(eventHandler);
    bmqa::Session                                session(eventHandlerMp);
    eventHandler->setSession(&session);

    int rc = session.start();
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to start the session with the BlazingMQ "
                       << "broker, rc: " << bmqt::GenericResult::Enum(rc);
        return rc;  // RETURN
    }

    // We are now connected to the broker. The next step is to open the queue
    // and read messages.
    BALL_LOG_INFO << "Starting the consumer";

    // Specify subscription handle to identify messages from the first
    // subscription.  Although corresponding correlation Id could be used
    // for that, it may be not unique and belong to multiple subscriptions.
    bmqt::CorrelationId      sub1CorrId(bmqt::CorrelationId::autoValue());
    bmqt::SubscriptionHandle sub1Handle(sub1CorrId);
    eventHandler->setSubscriptionHandle(&sub1Handle);

    consume(&session, &queueManager, &sub1Handle);

    // Stop the session with the BlazingMQ broker.  Note that a session which
    // is started without an event handler can only be stopped by calling the
    // async flavor 'stopAsync'.  This will likely be changed in a future
    // release, where it should be ok to invoke 'stop' on the session as well.

    BALL_LOG_INFO << "Stopping the session with the BlazingMQ broker";
    session.stop();

    return 0;
}
