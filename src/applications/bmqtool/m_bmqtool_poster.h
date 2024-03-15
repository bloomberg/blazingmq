//
// Created by syuzvinsky on 3/7/24.
//

#ifndef INCLUDED_M_BMQTOOL_POSTER
#define INCLUDED_M_BMQTOOL_POSTER

#include "m_bmqtool_filelogger.h"
#include <bdlbb_pooledblobbufferfactory.h>
#include <bmqa_messageproperties.h>
#include <bmqa_session.h>
#include <bsl_string.h>
#include <m_bmqtool_parameters.h>

namespace BloombergLP {
namespace m_bmqtool {

class Poster {
  private:
    bslma::Allocator* d_allocator_p;
    // Held, not owned

    Parameters* d_parameters_p;

    bmqa::Session* d_session_p;
    // A session to post messages. Held, not owned

    FileLogger* d_fileLogger;
    // Held, not owned

    int d_remainingEvents;
    int d_numMessagesPosted;
    int d_msgUntilNextTimestamp;

    bdlbb::PooledBlobBufferFactory d_bufferFactory;
    // Buffer factory for the payload of the
    // published message

    bdlbb::PooledBlobBufferFactory d_timeBufferFactory;
    // Small buffer factory for the first
    // blob of the published message, to
    // hold the timestamp information

    bdlbb::Blob d_blob;
    // Blob to post

    bmqa::QueueId d_queueId;

    bmqa::MessageProperties d_properties;

  public:
    Poster(bmqa::Session* session,
           Parameters* parameters,
           FileLogger* fileLogger,
           const bmqa::QueueId& queueId,
           bslma::Allocator* allocator);

    bool pendingPost() const;
    bsl::pair<size_t, size_t> postNext();
};

}  // close package namespace
}  // close enterprise namespace

#endif  // INCLUDED_M_BMQTOOL_POSTER
