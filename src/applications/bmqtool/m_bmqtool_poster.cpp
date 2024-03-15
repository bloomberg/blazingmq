//
// Created by syuzvinsky on 3/7/24.
//

#include <m_bmqtool_inpututil.h>
#include <ball_log.h>
#include <bdlbb_blobutil.h>
#include <bdlt_currenttime.h>
#include <bmqt_queueflags.h>
#include <bsls_timeutil.h>
#include <m_bmqtool_poster.h>
#include <bmqimp_event.h>

namespace BloombergLP {
namespace m_bmqtool {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("BMQTOOL.POSTER");

// How often (in ms) should a message be stamped with the latency: because
// computing the current time is expensive, we will only insert the timestamp
// inside a sample subset of the messages (1 every 'k_LATENCY_INTERVAL_MS'
// time), as computed by the configured frequency of message publishing.
const int k_LATENCY_INTERVAL_MS = 5;

// Return the current time -in nanoseconds- using either the system time or the
// performance timer (depending on the value of the specified 'resolutionTimer'
bsls::Types::Int64 getNowAsNs(ParametersLatency::Value resolutionTimer)
{
    if (resolutionTimer == ParametersLatency::e_EPOCH) {
        bsls::TimeInterval now = bdlt::CurrentTime::now();
        return now.totalNanoseconds();  // RETURN
    }
    else if (resolutionTimer == ParametersLatency::e_HIRES) {
        return bsls::TimeUtil::getTimer();  // RETURN
    }
    else {
        BSLS_ASSERT_OPT(false && "Unsupported latency mode");
    }

    return 0;
}

}

Poster::Poster(
    bmqa::Session* session,
    Parameters* parameters,
    FileLogger* fileLogger,
    const bmqa::QueueId& queueId,
    bslma::Allocator* allocator)
    : d_allocator_p(allocator)
    , d_parameters_p(parameters)
    , d_session_p(session)
    , d_fileLogger(fileLogger)
    , d_remainingEvents(0)
    , d_numMessagesPosted(0)
    , d_msgUntilNextTimestamp(0)
    , d_bufferFactory(4096, d_allocator_p)
    , d_timeBufferFactory(sizeof(bdlb::BigEndianInt64), d_allocator_p)
    , d_blob(&d_bufferFactory, d_allocator_p)
    , d_queueId(queueId, d_allocator_p)
    , d_properties(d_allocator_p)
{
    BSLS_ASSERT_SAFE(session);
    BSLS_ASSERT_SAFE(parameters);

    d_remainingEvents = d_parameters_p->eventsCount();

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
            d_timeBufferFactory.allocate(&latencyBuffer);
            latencyBuffer.setSize(sizeof(bdlb::BigEndianInt64));
            bdlb::BigEndianInt64 zero = bdlb::BigEndianInt64::make(0);
            bsl::memcpy(latencyBuffer.buffer().get(),
                        &zero,
                        sizeof(zero));
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

bool Poster::pendingPost() const
{
    // eventsCount() == 0 means endless posting,
    // otherwise check if remainingEvents is positive
    return d_parameters_p->eventsCount() == 0 || d_remainingEvents > 0;
}

bsl::pair<size_t, size_t> Poster::postNext()
{
    BSLS_ASSERT_SAFE(d_session_mp);
    BSLS_ASSERT_SAFE(pendingPost());
    BSLS_ASSERT_SAFE(d_queueId.isValid());

    bmqa::MessageEventBuilder eventBuilder;
    d_session_p->loadMessageEventBuilder(&eventBuilder);

    int postedEventsSize = 0;
    int postedMessagesSize = 0;

    for (int evtId = 0;
         evtId < d_parameters_p->postRate() && pendingPost();
         ++evtId) {
        if (d_parameters_p->eventSize() == 0) {
            // To get nice stats chart with round numbers in bench mode, we
            // usually start with eventSize == 0; however posting Events
            // with 0 message in them cause an assert or an error to spew,
            // so just avoid it.
            break;  // BREAK
        }

        postedMessagesSize = 0;
        eventBuilder.reset();
        for (int msgId = 0; msgId < d_parameters_p->eventSize();
             ++msgId, ++d_numMessagesPosted) {
            bmqa::Message& msg    = eventBuilder.startMessage();
            int            length = 0;

            // Set a correlationId if queue is open in ACK mode
            if  (bmqt::QueueFlagsUtil::isAck(d_parameters_p->queueFlags())) {
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
                            getNowAsNs(d_parameters_p->latency()));

                        // Update the number of messages until next
                        // timestamp:
                        int nbMsgPerSec = d_parameters_p->eventSize() *
                                          d_parameters_p->postRate() *
                                          1000 /
                                          d_parameters_p->postInterval();
                        d_msgUntilNextTimestamp = nbMsgPerSec *
                                                  k_LATENCY_INTERVAL_MS /
                                                  1000;
                    }

                    bdlbb::BlobBuffer buffer;
                    d_timeBufferFactory.allocate(&buffer);
                    buffer.setSize(sizeof(bdlb::BigEndianInt64));
                    bsl::memcpy(buffer.buffer().get(),
                                &timeNs,
                                sizeof(timeNs));
                    d_blob.swapBufferRaw(0, &buffer);
                }
                msg.setDataRef(&d_blob);

                length = d_blob.length();
            }

            if (d_properties.numProperties()) {
                msg.setPropertiesRef(&d_properties);
            }
            bmqt::EventBuilderResult::Enum rc = eventBuilder.packMessage(
                d_queueId);
            if (rc != 0) {
                BALL_LOG_ERROR << "Failed to pack message [rc: " << rc
                               << "]";
                continue;  // CONTINUE
            }
            postedMessagesSize += length;
        }

        // Now publish the event
        const bmqa::MessageEvent& messageEvent =
            eventBuilder.messageEvent();

        // Write PUTs to log file before posting
        if (d_fileLogger->isOpen()) {
            bmqa::MessageIterator it = messageEvent.messageIterator();
            while (it.nextMessage()) {
                const bmqa::Message& message = it.message();
                d_fileLogger->writePutMessage(message);
            }
        }

        int rc = d_session_p->post(messageEvent);

        if (rc != 0) {
            BALL_LOG_ERROR
                << "Failed to post: " << bmqt::PostResult::Enum(rc) << " ("
                << rc << ")";
            continue;  // CONTINUE
        }

        const bsl::shared_ptr<bmqimp::Event>& eventImpl =
            reinterpret_cast<const bsl::shared_ptr<bmqimp::Event>&>(
                messageEvent);
        postedEventsSize += eventImpl->rawEvent().blob()->length();

        if (d_parameters_p->eventsCount() > 0) {
            --d_remainingEvents;
        }
    }

    return bsl::make_pair(postedEventsSize, postedMessagesSize);
}

}  // close package namespace
}  // close enterprise namespace
