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
#include <bmqa_closequeuestatus.h>
#include <bmqa_event.h>
#include <bmqa_messageeventbuilder.h>
#include <bmqa_messageproperties.h>
#include <bmqa_openqueuestatus.h>
#include <bmqa_queueid.h>
#include <bmqa_session.h>
#include <bmqt_queueflags.h>
#include <bmqt_resultcode.h>
#include <bmqt_uri.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_ostream.h>
#include <bsls_annotation.h>

using namespace BloombergLP;

namespace {
typedef bsl::vector<bsl::string> TestMessages;
const char k_QUEUE_URL[] = "bmq://bmq.test.mem.priority/test-queue";
const int  k_QUEUE_ID    = 1;
}  // close unnamed namespace

//=============================================================================
//                                PRODUCER
//-----------------------------------------------------------------------------

/// Send event containing message with the specified `text` to the queue
/// with the specified `queueId`, using the specified `session`.
static bool postEvent(const bsl::string&   text,
                      const bmqa::QueueId& queueId,
                      bmqa::Session*       session)
{
    // Build a 'MessageEvent' containing a single message and add it to the
    // queue.
    bmqa::MessageEventBuilder builder;
    session->loadMessageEventBuilder(&builder);

    // Create a new message in the builder.
    bmqa::Message& message = builder.startMessage();
    message.setDataRef(text.c_str(), text.length());

    int rc = builder.packMessage(queueId);
    if (rc != 0) {
        bsl::cerr << "Failed to pack message: rc: "
                  << bmqt::EventBuilderResult::Enum(rc) << "\n";
        return false;  // RETURN
    }

    const bmqa::MessageEvent& messageEvent = builder.messageEvent();
    rc                                     = session->post(messageEvent);
    if (rc != 0) {
        bsl::cerr << "Failed to post message: rc: " << rc << "\n";
        return false;  // RETURN
    }

    return true;
}

/// Open a queue with the specified `session`. The queue is created if it
/// does not exist already.  Each queue is identified by a short URL
/// containing a namespace and a queue name.
static void produce(bmqa::Session* session)
{
    bmqa::QueueId         queueId(k_QUEUE_ID);
    bmqa::OpenQueueStatus openStatus = session->openQueueSync(
        &queueId,
        k_QUEUE_URL,
        bmqt::QueueFlags::e_WRITE);
    const TestMessages testMessages = {"Hello world!",
                                       "message 1",
                                       "message 2",
                                       "message 3",
                                       "Good Bye!"};
    if (!openStatus) {
        bsl::cerr << "Failed to open queue: '" << k_QUEUE_URL
                  << "', status: " << openStatus << "\n";
        return;  // RETURN
    }
    else {
        bsl::cout << "Queue ['" << k_QUEUE_URL << "'] has been opened. "
                  << "Sending messages.\n";
    }
    for (const auto& input : testMessages) {
        if (!postEvent(input, queueId, session)) {
            break;
        }
    }
    bmqa::CloseQueueStatus closeStatus = session->closeQueueSync(&queueId);
    if (!closeStatus) {
        bsl::cerr << "Failed to close queue: '" << k_QUEUE_URL
                  << "', status: " << closeStatus << "\n";
        return;  // RETURN
    }
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(BSLS_ANNOTATION_UNUSED int         argc,
         BSLS_ANNOTATION_UNUSED const char* argv[])
{
    // Start the session with the BlazingMQ broker
    bmqa::Session session;
    int           rc = session.start();
    if (rc != 0) {
        bsl::cerr << "Failed to start the session with the BlazingMQ broker"
                  << ", rc: " << bmqt::GenericResult::Enum(rc) << "\n";
        return rc;  // RETURN
    }
    produce(&session);
    session.stop();
    return 0;
}
