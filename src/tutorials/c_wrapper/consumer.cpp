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

// BDE
#include <bsl_iostream.h>
#include <bslmt_condition.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bsls_annotation.h>
#include <bsls_keyword.h>
#include <bsls_timeinterval.h>

#include <z_bmqa_confirmeventbuilder.h>
#include <z_bmqa_messageevent.h>
#include <z_bmqa_openqueuestatus.h>
#include <z_bmqt_queueflags.h>
#include <z_bmqa_session.h>
#include <z_bmqa_sessionevent.h>
#include <z_bmqt_correlationid.h>

// SYSTEM
#include <signal.h>

using namespace BloombergLP;

// CONSTANTS
const char k_QUEUE_URL[] = "bmq://bmq.test.mem.priority/test-queue";

// FUNCTIONS
bsl::function<void(int)> shutdownHandler;

/// Handle the specified `signal`
void signalHandler(int signal)
{
    shutdownHandler(signal);
}

typedef struct SetSessionArgs {
    z_bmqa_Session* session;
} SetSessionArgs;

void onMessageEvent(const z_bmqa_MessageEvent* messageEvent, void* data)
// Handle the specified 'messageEvent'
{
    // Load a ConfirmEventBuilder from the session
    z_bmqa_Session* session = *(static_cast<z_bmqa_Session**>(data));
    z_bmqa_ConfirmEventBuilder* confirmBuilder;
    z_bmqa_ConfirmEventBuilder__create(&confirmBuilder);

    z_bmqa_Session__loadConfirmEventBuilder(session, confirmBuilder);

    if (z_bmqa_MessageEvent__type(messageEvent) ==
        z_bmqt_MessageEventType::ec_PUSH) {
        z_bmqa_MessageIterator* msgIter;
        z_bmqa_MessageEvent__messageIterator(messageEvent, &msgIter);
        while (z_bmqa_MessageIterator__nextMessage(msgIter)) {
            const z_bmqa_Message* msg;
            z_bmqa_MessageIterator__message(msgIter, &msg);
            char* data;
            int   rc = z_bmqa_Message__getData(msg, &data);

            const z_bmqt_MessageGUID* messageGUID;
            z_bmqa_Message__messageGUID(msg, &messageGUID);
            char* messageGUID_str;
            z_bmqt_MessageGUID__toString(messageGUID, &messageGUID_str);
            if (rc == 0) {
                bsl::cout << "Got message with GUID: "
                          << bsl::string(messageGUID_str) << ", data:\n"
                          << bsl::string(data) << "\n";
            }
            else {
                bsl::cerr << "Failed to decode message with GUID: "
                          << bsl::string(messageGUID_str) << ", rc: " << rc
                          << "\n";
            }

            rc = z_bmqa_ConfirmEventBuilder__addMessageConfirmation(
                confirmBuilder,
                msg);
            if (rc != 0) {
                bsl::cerr << "Failed to add confirm message with GUID: "
                          << bsl::string(messageGUID_str)
                          << " to the builder, rc: " << rc << "\n";
            }

            delete[] messageGUID_str;
        }

        // Confirm reception of the messages so that it can be deleted from the
        // queue.

        // int rc = d_session_p->confirmMessages(&confirmBuilder);
        int rc = z_bmqa_Session__confirmMessages(session, confirmBuilder);
        if (rc != 0) {
            bsl::cerr << "Failed to confirm "
                      << z_bmqa_ConfirmEventBuilder__messageCount(
                             confirmBuilder)
                      << " messages"
                      << ", rc: " << rc;

            // Since 'Session::confirmMessages' failed, it would not have reset
            // the 'builder'.  So we reset it here.

            z_bmqa_ConfirmEventBuilder__reset(confirmBuilder);
        }
    }
    else {
        const char* eventType = z_bmqt_MessageEventType::toAscii(
            z_bmqa_MessageEvent__type(messageEvent));
        bsl::cerr << "Got unexpected event type: " << bsl::string(eventType)
                  << "\n";
    }

    z_bmqa_ConfirmEventBuilder__delete(&confirmBuilder);
}

void onSessionEvent(const z_bmqa_SessionEvent* sessionEvent, void* data)
// Handle the specified 'sessionEvent'.  This method is executed in one of
// the *EVENT HANDLER* threads.
{
    char* out;
    z_bmqa_SessionEvent__toString(sessionEvent, &out);
    bsl::cout << "Got session event: " << bsl::string(out) << "\n";
    delete [] out;
}

void setSession(void* args, void* eventHandlerData)
{
    SetSessionArgs* args_p = static_cast<SetSessionArgs*>(args);
    z_bmqa_Session** session_p = static_cast<z_bmqa_Session**>(eventHandlerData);
    *session_p = args_p->session;
}

//=============================================================================
//                                 CONSUMER
//-----------------------------------------------------------------------------

/// Open a queue with the specified `session`.  The queue is created if it
/// does not exist already.  Each queue is identified by a short URL
/// containing a namespace and a queue name.
static void consume(z_bmqa_Session* session)
{
    // bmqt::CorrelationId   corrId(bmqt::CorrelationId::autoValue());
    z_bmqt_CorrelationId* corrId;
    z_bmqt_CorrelationId__autoValue(&corrId);
    z_bmqa_QueueId*         queueId;
    z_bmqa_OpenQueueStatus* openStatus;
    z_bmqa_QueueId__createFromCorrelationId(&queueId, corrId);
    z_bmqa_Session__openQueueSync(
        session,
        queueId,
        k_QUEUE_URL,
        static_cast<uint64_t>(z_bmqt_QueueFlags::ec_READ),
        &openStatus);

    if (!z_bmqa_OpenQueueStatus__toBool(openStatus) ||
        !z_bmqa_QueueId__isValid(queueId)) {
        bsl::cerr << "Failed to open queue: '" << k_QUEUE_URL
                  << "', status: " << openStatus << "\n";
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
    // bmqt::QueueOptions options;
    z_bmqt_QueueOptions* options;
    z_bmqt_QueueOptions__create(&options);
    z_bmqt_QueueOptions__setMaxUnconfirmedBytes(options, 0);
    z_bmqt_QueueOptions__setMaxUnconfirmedMessages(options, 0);
    z_bmqa_ConfigureQueueStatus* configureStatus;
    z_bmqa_Session__configureQueueSync(session,
                                       queueId,
                                       options,
                                       0,
                                       &configureStatus);
    if (!z_bmqa_ConfigureQueueStatus__toBool(configureStatus)) {
        // Error! Log something
        bsl::cout << "Graceful shutdown of the queue failed: "
                  << bmqt::ConfigureQueueResult::Enum(
                         z_bmqa_ConfigureQueueStatus__result(configureStatus))
                  << " (" << configureStatus << ")\n";
        return;  // RETURN
    }

    z_bmqa_CloseQueueStatus* closeStatus;
    z_bmqa_Session__closeQueueSync(session, queueId, 0, &closeStatus);
    if (!z_bmqa_CloseQueueStatus__toBool(closeStatus)) {
        bsl::cerr << "Failed to close queue: '" << k_QUEUE_URL
                  << "', status: " << closeStatus << "\n";
        return;  // RETURN
    }
    bsl::cerr << "Queue ['" << k_QUEUE_URL << "'] has been shut down "
              << "gracefully and is now closed.\n";


    // Must delete all objects
    z_bmqa_QueueId__delete(&queueId);
    z_bmqt_QueueOptions__delete(&options);
    z_bmqa_OpenQueueStatus__delete(&openStatus);
    z_bmqa_ConfigureQueueStatus__delete(&configureStatus);
    z_bmqa_CloseQueueStatus__delete(&closeStatus);
    z_bmqt_CorrelationId__delete(&corrId);
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(BSLS_ANNOTATION_UNUSED int         argc,
         BSLS_ANNOTATION_UNUSED const char* argv[])
{
    // Start a session with the message broker.  This makes the SDK connect to
    // the local broker by default, unless the 'Session' is created with an
    // optional 'SessionOptions' object.

    z_bmqa_Session*             session;
    z_bmqa_SessionEventHandler* eventHandler;
    z_bmqa_SessionEventHandler__create(&eventHandler,
                                       onSessionEvent,
                                       onMessageEvent,
                                       sizeof(z_bmqa_Session*));
    z_bmqa_Session__createAsync(&session, eventHandler, NULL);

    SetSessionArgs setSessionArgs;
    setSessionArgs.session = session;
    z_bmqa_SessionEventHandler__callCustomFunction(eventHandler, setSession, &setSessionArgs);

    int rc = z_bmqa_Session__start((z_bmqa_Session*)(session), 0);
    if (rc != 0) {
        bsl::cerr << "Failed to start the session with the BlazingMQ broker"
                  << ", rc: " << bmqt::GenericResult::Enum(rc) << "\n";
        return rc;  // RETURN
    }

    consume(session);
    z_bmqa_Session__stop(session);

    // Calling z_bmqa_Session__delete also deletes the event handler
    z_bmqa_Session__delete(&session);

    return 0;
}