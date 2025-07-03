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

// consumer.cpp                                                       -*-C++-*-

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
#include <bmqa_closequeuestatus.h>
#include <bmqa_configurequeuestatus.h>
#include <bmqa_event.h>
#include <bmqa_messageiterator.h>
#include <bmqa_openqueuestatus.h>
#include <bmqa_queueid.h>
#include <bmqa_session.h>
#include <bmqa_sessionevent.h>
#include <bmqt_correlationid.h>
#include <bmqt_messageeventtype.h>
#include <bmqt_messageguid.h>
#include <bmqt_queueflags.h>
#include <bmqt_queueoptions.h>
#include <bmqt_resultcode.h>
#include <bmqt_uri.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bsl_iostream.h>
#include <bsl_ostream.h>
#include <bsla_annotations.h>
#include <bslma_managedptr.h>
#include <bslmt_condition.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bsls_keyword.h>
#include <bsls_timeinterval.h>

// SYSTEM
#include <bsl_functional.h>
#include <signal.h>

using namespace BloombergLP;

namespace {
// TYPES
using ManagedHandler = bslma::ManagedPtr<bmqa::SessionEventHandler>;

// CONSTANTS
const char k_QUEUE_URL[] = "bmq://bmq.test.mem.priority/test-queue";

// FUNCTIONS
bsl::function<void(int)> shutdownHandler;

/// Handle the specified `signal`
void signalHandler(int signal)
{
    shutdownHandler(signal);
}

// CLASSES

/// Concrete implementation of an event handler.  Note that the methods are
/// called on the session's own threads.
class EventHandler : public bmqa::SessionEventHandler {
  private:
    // DATA
    bmqa::Session* d_session_p;  // Pointer to session (held, not owned)

  public:
    // MANIPULATORS
    void onMessageEvent(const bmqa::MessageEvent& messageEvent)
        BSLS_KEYWORD_OVERRIDE;
    // Process the specified 'messageEvent' received from the broker.

    void onSessionEvent(const bmqa::SessionEvent& sessionEvent)
        BSLS_KEYWORD_OVERRIDE;
    // Process the specified 'sessionEvent' received from the broker.

    void setSession(bmqa::Session* session);
};

void EventHandler::onMessageEvent(const bmqa::MessageEvent& messageEvent)
// Handle the specified 'messageEvent'
{
    // Load a ConfirmEventBuilder from the session
    bmqa::ConfirmEventBuilder confirmBuilder;
    d_session_p->loadConfirmEventBuilder(&confirmBuilder);

    if (messageEvent.type() == bmqt::MessageEventType::e_PUSH) {
        bmqa::MessageIterator msgIter = messageEvent.messageIterator();
        while (msgIter.nextMessage()) {
            const bmqa::Message& msg = msgIter.message();
            bdlbb::Blob          data;
            int                  rc = msg.getData(&data);
            if (rc == 0) {
                bsl::cout << "Got message with GUID: " << msg.messageGUID()
                          << ", data:\n"
                          << bdlbb::BlobUtilHexDumper(&data) << "\n";
            }
            else {
                bsl::cerr << "Failed to decode message with GUID: "
                          << msg.messageGUID() << ", rc: " << rc << "\n";
            }

            rc = confirmBuilder.addMessageConfirmation(msg);
            if (rc != 0) {
                bsl::cerr << "Failed to add confirm message with GUID: "
                          << msg.messageGUID() << " to the builder, rc: " << rc
                          << "\n";
            }
        }

        // Confirm reception of the messages so that it can be deleted from the
        // queue.

        int rc = d_session_p->confirmMessages(&confirmBuilder);
        if (rc != 0) {
            bsl::cerr << "Failed to confirm " << confirmBuilder.messageCount()
                      << " messages"
                      << ", rc: " << rc;

            // Since 'Session::confirmMessages' failed, it would not have reset
            // the 'builder'.  So we reset it here.

            confirmBuilder.reset();
        }
    }
    else {
        bsl::cerr << "Got unexpected event type: " << messageEvent.type()
                  << "\n";
    }
}

void EventHandler::onSessionEvent(const bmqa::SessionEvent& sessionEvent)
// Handle the specified 'sessionEvent'.  This method is executed in one of
// the *EVENT HANDLER* threads.
{
    bsl::cout << "Got session event: " << sessionEvent << "\n";
}

void EventHandler::setSession(bmqa::Session* session)
{
    d_session_p = session;
}

}  // close unnamed namespace

//=============================================================================
//                                 CONSUMER
//-----------------------------------------------------------------------------

/// Open a queue with the specified `session`.  The queue is created if it
/// does not exist already.  Each queue is identified by a short URL
/// containing a namespace and a queue name.
static void consume(bmqa::Session* session)
{
    bmqt::CorrelationId   corrId(bmqt::CorrelationId::autoValue());
    bmqa::QueueId         queueId(corrId);
    bmqa::OpenQueueStatus status = session->openQueueSync(
        &queueId,
        k_QUEUE_URL,
        bmqt::QueueFlags::e_READ);
    if (!status || !queueId.isValid()) {
        bsl::cerr << "Failed to open queue: '" << k_QUEUE_URL
                  << "', status: " << status << "\n";
        return;  // RETURN
    }
    else {
        bsl::cout << "Queue ['" << k_QUEUE_URL << "'] has been opened. "
                  << "Waiting for messages...\nPress Ctrl+C for shutdown\n";
    }

    bslmt::Mutex     d_mx;
    bslmt::Condition d_cv;
    shutdownHandler = [&d_cv](int) {
        d_cv.signal();
    };
    signal(SIGINT, signalHandler);
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mx);
    d_cv.wait(&d_mx);

    // Graceful shutdown by first synchronously shutting the flow of incoming
    // messages and only then closing the queue.
    bmqt::QueueOptions options;
    options.setMaxUnconfirmedMessages(0).setMaxUnconfirmedBytes(0);
    bmqa::ConfigureQueueStatus configureStatus =
        session->configureQueueSync(&queueId, options);
    if (!configureStatus) {
        // Error! Log something
        bsl::cout << "Graceful shutdown of the queue failed: "
                  << bmqt::ConfigureQueueResult::Enum(configureStatus.result())
                  << " (" << configureStatus << ")\n";
        return;  // RETURN
    }

    bmqa::CloseQueueStatus closeStatus = session->closeQueueSync(&queueId);
    if (!closeStatus) {
        bsl::cerr << "Failed to close queue: '" << k_QUEUE_URL
                  << "', status: " << closeStatus << "\n";
        return;  // RETURN
    }
    bsl::cerr << "Queue ['" << k_QUEUE_URL << "'] has been shut down "
              << "gracefully and is now closed.\n";
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(BSLA_UNUSED int argc, BSLA_UNUSED const char* argv[])
{
    // Start a session with the message broker.  This makes the SDK connect to
    // the local broker by default, unless the 'Session' is created with an
    // optional 'SessionOptions' object.

    EventHandler*  eventHandler = new EventHandler();
    ManagedHandler eventHandlerMp(eventHandler);
    bmqa::Session  session(eventHandlerMp);
    eventHandler->setSession(&session);
    int rc = session.start();
    if (rc != 0) {
        bsl::cerr << "Failed to start the session with the BlazingMQ broker"
                  << ", rc: " << bmqt::GenericResult::Enum(rc) << "\n";
        return rc;  // RETURN
    }
    consume(&session);
    session.stop();

    return 0;
}
