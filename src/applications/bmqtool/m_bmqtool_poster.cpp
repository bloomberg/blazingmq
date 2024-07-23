// Copyright 2024 Bloomberg Finance L.P.
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

// m_bmqtool_poster.cpp                                               -*-C++-*-

// BMQTOOL
#include <m_bmqtool_inpututil.h>
#include <m_bmqtool_poster.h>
#include <m_bmqtool_statutil.h>

// BMQ
#include <bmqimp_event.h>
#include <bmqt_queueflags.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blobutil.h>

namespace BloombergLP {
namespace m_bmqtool {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("BMQTOOL.POSTER");

}

PostingContext::PostingContext(
    bmqa::Session*                  session,
    Parameters*                     parameters,
    const bmqa::QueueId&            queueId,
    FileLogger*                     fileLogger,
    mwcst::StatContext*             statContext,
    bdlbb::PooledBlobBufferFactory* bufferFactory,
    bdlbb::PooledBlobBufferFactory* timeBufferFactory,
    bslma::Allocator*               allocator)
: d_allocator_p(allocator)
, d_timeBufferFactory_p(timeBufferFactory)
, d_parameters_p(parameters)
, d_session_p(session)
, d_fileLogger(fileLogger)
, d_statContext_p(statContext)
, d_remainingEvents(d_parameters_p->eventsCount())
, d_numMessagesPosted(0)
, d_msgUntilNextTimestamp(0)
, d_blob(bufferFactory, d_allocator_p)
, d_queueId(queueId, d_allocator_p)
, d_properties(d_allocator_p)
, d_autoIncrementedValue(0)
{
    BSLS_ASSERT_SAFE(session);
    BSLS_ASSERT_SAFE(parameters);
    BSLS_ASSERT_SAFE(d_queueId.isValid());

    InputUtil::populateProperties(&d_properties,
                                  d_parameters_p->messageProperties());

    // Prepare the blob that we will post over and over again
    if (d_parameters_p->sequentialMessagePattern().empty()) {
        int msgPayloadSize = d_parameters_p->msgSize();

        if (d_parameters_p->latency() != ParametersLatency::e_NONE) {
            // To optimize, if asked to insert latency, we put in a
            // first blob of 8 bytes that will be swapped out at every
            // post with a new timestamp value.
            bdlbb::BlobBuffer latencyBuffer;
            d_timeBufferFactory_p->allocate(&latencyBuffer);
            latencyBuffer.setSize(sizeof(bdlb::BigEndianInt64));
            bdlb::BigEndianInt64 zero = bdlb::BigEndianInt64::make(0);
            bsl::memcpy(latencyBuffer.buffer().get(), &zero, sizeof(zero));
            d_blob.appendDataBuffer(latencyBuffer);
            msgPayloadSize -= sizeof(bdlb::BigEndianInt64);
        }

        // Initialize a buffer of the right published size, with
        // alphabet's letters
        for (int i = 0; i < msgPayloadSize; ++i) {
            char c = static_cast<char>('A' + i % 26);
            bdlbb::BlobUtil::append(&d_blob, &c, 1);
        }
    }
}

bool PostingContext::pendingPost() const
{
    // eventsCount() == 0 means endless posting,
    // otherwise check if remainingEvents is positive
    return d_parameters_p->eventsCount() == 0 || d_remainingEvents > 0;
}

void PostingContext::postNext()
{
    BSLS_ASSERT_SAFE(pendingPost());

    bmqa::MessageEventBuilder eventBuilder;
    d_session_p->loadMessageEventBuilder(&eventBuilder);

    for (int evtId = 0; evtId < d_parameters_p->postRate() && pendingPost();
         ++evtId) {
        if (d_parameters_p->eventSize() == 0) {
            // To get nice stats chart with round numbers in bench mode, we
            // usually start with eventSize == 0; however posting Events
            // with 0 message in them cause an assert or an error to spew,
            // so just avoid it.
            break;  // BREAK
        }

        eventBuilder.reset();
        for (bsl::uint64_t msgId = 0; msgId < d_parameters_p->eventSize();
             ++msgId, ++d_numMessagesPosted) {
            bmqa::Message& msg    = eventBuilder.startMessage();
            int            length = 0;

            // Set a correlationId if queue is open in ACK mode
            if (bmqt::QueueFlagsUtil::isAck(d_parameters_p->queueFlags())) {
                msg.setCorrelationId(bmqt::CorrelationId::autoValue());
            }

            if (!d_parameters_p->sequentialMessagePattern().empty()) {
                char buffer[128];
                length = snprintf(
                    buffer,
                    sizeof(buffer),
                    d_parameters_p->sequentialMessagePattern().c_str(),
                    d_numMessagesPosted);
                msg.setDataRef(buffer, length);
            }
            else {
                // Insert latency if required...
                if (d_parameters_p->latency() != ParametersLatency::e_NONE) {
                    bdlb::BigEndianInt64 timeNs;

                    if (d_msgUntilNextTimestamp != 0) {
                        --d_msgUntilNextTimestamp;
                        timeNs = bdlb::BigEndianInt64::make(0);
                    }
                    else {
                        // Insert the timestamp
                        timeNs = bdlb::BigEndianInt64::make(
                            StatUtil::getNowAsNs(d_parameters_p->latency()));

                        // Update the number of messages until next
                        // timestamp:
                        int nbMsgPerSec = d_parameters_p->eventSize() *
                                          d_parameters_p->postRate() * 1000 /
                                          d_parameters_p->postInterval();
                        d_msgUntilNextTimestamp = nbMsgPerSec *
                                                  k_LATENCY_INTERVAL_MS / 1000;
                    }

                    bdlbb::BlobBuffer buffer;
                    d_timeBufferFactory_p->allocate(&buffer);
                    buffer.setSize(sizeof(bdlb::BigEndianInt64));
                    bsl::memcpy(buffer.buffer().get(),
                                &timeNs,
                                sizeof(timeNs));
                    d_blob.swapBufferRaw(0, &buffer);
                }
                msg.setDataRef(&d_blob);

                length = d_blob.length();
            }

            if (!d_parameters_p->autoIncrementedField().empty()) {
                d_properties.setPropertyAsInt64(
                    d_parameters_p->autoIncrementedField(),
                    d_autoIncrementedValue++);
            }

            if (d_properties.numProperties()) {
                msg.setPropertiesRef(&d_properties);
            }

            bmqt::EventBuilderResult::Enum rc = eventBuilder.packMessage(
                d_queueId);
            if (rc != 0) {
                BALL_LOG_ERROR << "Failed to pack message [rc: " << rc << "]";
                continue;  // CONTINUE
            }
            d_statContext_p->adjustValue(k_STAT_MSG, length);
        }

        // Now publish the event
        const bmqa::MessageEvent& messageEvent = eventBuilder.messageEvent();

        // Write PUTs to log file before posting
        if (d_fileLogger && d_fileLogger->isOpen()) {
            bmqa::MessageIterator it = messageEvent.messageIterator();
            while (it.nextMessage()) {
                const bmqa::Message& message = it.message();
                d_fileLogger->writePutMessage(message);
            }
        }

        int rc = d_session_p->post(messageEvent);

        if (rc != 0) {
            BALL_LOG_ERROR << "Failed to post: " << bmqt::PostResult::Enum(rc)
                           << " (" << rc << ")";
            continue;  // CONTINUE
        }

        const bsl::shared_ptr<bmqimp::Event>& eventImpl =
            reinterpret_cast<const bsl::shared_ptr<bmqimp::Event>&>(
                messageEvent);
        d_statContext_p->adjustValue(k_STAT_EVT,
                                     eventImpl->rawEvent().blob()->length());

        if (d_parameters_p->eventsCount() > 0) {
            --d_remainingEvents;
        }
    }
}

Poster::Poster(FileLogger*         fileLogger,
               mwcst::StatContext* statContext,
               bslma::Allocator*   allocator)
: d_allocator_p(bslma::Default::allocator(allocator))
, d_bufferFactory(4096, d_allocator_p)
, d_timeBufferFactory(sizeof(bdlb::BigEndianInt64), d_allocator_p)
, d_statContext(statContext)
, d_fileLogger(fileLogger)
{
    BSLS_ASSERT_SAFE(fileLogger);
    BSLS_ASSERT_SAFE(statContext);
    BSLS_ASSERT_SAFE(allocator);
}

bsl::shared_ptr<PostingContext>
Poster::createPostingContext(bmqa::Session*       session,
                             Parameters*          parameters,
                             const bmqa::QueueId& queueId)
{
    return bsl::make_shared<PostingContext>(session,
                                            parameters,
                                            queueId,
                                            d_fileLogger,
                                            d_statContext,
                                            &d_bufferFactory,
                                            &d_timeBufferFactory,
                                            d_allocator_p);
}

}  // close package namespace
}  // close enterprise namespace
