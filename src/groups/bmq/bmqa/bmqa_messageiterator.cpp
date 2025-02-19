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

// bmqa_messageiterator.cpp                                           -*-C++-*-
#include <bmqa_messageiterator.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqa_queueid.h>
#include <bmqimp_event.h>
#include <bmqt_correlationid.h>
#include <bmqt_subscription.h>

// BDE
#include <bsl_iostream.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqa {

// ---------------------
// class MessageIterator
// ---------------------

MessageIterator::MessageIterator()
{
    d_impl.d_messageIndex = -1;
}

bool MessageIterator::nextMessage()
{
    // We need to reset the 'queueId' and 'correlationId' of message, because
    // those are lazily fetched, and must be invalidated when moving to the
    // next message.
    MessageImpl& msgImpl = reinterpret_cast<MessageImpl&>(d_impl.d_message);
    msgImpl.d_queueId    = bmqa::QueueId();
    msgImpl.d_correlationId.makeUnset();
    msgImpl.d_subscriptionHandle = bmqt::SubscriptionHandle();
    msgImpl.d_schema_sp.reset();

    int rc = -1;

    if (d_impl.d_event_p->rawEvent().isPushEvent()) {
        rc = d_impl.d_event_p->pushMessageIterator()->next();
        if (rc < 0) {
            BALL_LOG_ERROR_BLOCK
            {
                BALL_LOG_OUTPUT_STREAM << "Invalid 'PushEvent' [rc: " << rc
                                       << "]\n";
                d_impl.d_event_p->pushMessageIterator()->dumpBlob(
                    BALL_LOG_OUTPUT_STREAM);
                BSLS_ASSERT_SAFE(false && "Invalid 'PushEvent'");
            }
        }
    }
    else if (d_impl.d_event_p->rawEvent().isAckEvent()) {
        // For an ACK msg, retrieve correlationId from the underlying raw
        // event.
        rc = d_impl.d_event_p->ackMessageIterator()->next();
        if (rc < 0) {
            BALL_LOG_ERROR_BLOCK
            {
                BALL_LOG_OUTPUT_STREAM << "Invalid 'AckEvent' [rc: " << rc
                                       << "]\n";
                d_impl.d_event_p->ackMessageIterator()->dumpBlob(
                    BALL_LOG_OUTPUT_STREAM);
                BSLS_ASSERT_SAFE(false && "Invalid 'AckEvent'");
            }
        }
    }
    else if (d_impl.d_event_p->rawEvent().isPutEvent()) {
        rc = d_impl.d_event_p->putMessageIterator()->next();
        if (rc < 0) {
            BALL_LOG_ERROR_BLOCK
            {
                BALL_LOG_OUTPUT_STREAM << "Invalid 'PutEvent' [rc: " << rc
                                       << "]\n";
                d_impl.d_event_p->putMessageIterator()->dumpBlob(
                    BALL_LOG_OUTPUT_STREAM);
                BSLS_ASSERT_SAFE(false && "Invalid 'PutEvent'");
            }
        }
    }
    else {
        BSLS_ASSERT_OPT(false && "Unknown message iterator type");
    }

    if (rc == 1) {
        ++d_impl.d_messageIndex;
        BSLS_ASSERT_SAFE(d_impl.d_messageIndex <
                         d_impl.d_event_p->numCorrrelationIds());

        const bmqimp::Event::MessageContext& context =
            d_impl.d_event_p->context(d_impl.d_messageIndex);

        msgImpl.d_correlationId = context.d_correlationId;

        if (d_impl.d_event_p->rawEvent().isPushEvent()) {
            const unsigned int subscriptionId = context.d_subscriptionHandleId;

            msgImpl.d_subscriptionHandle = bmqt::SubscriptionHandle(
                subscriptionId,
                msgImpl.d_correlationId);

            msgImpl.d_schema_sp = context.d_schema_sp;
        }

        return true;  // RETURN
    }

    return false;
}

const Message& MessageIterator::message() const
{
    return d_impl.d_message;
}

}  // close package namespace
}  // close enterprise namespace
