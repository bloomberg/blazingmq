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

// m_bmqtool_poster.h                                                 -*-C++-*-
#ifndef INCLUDED_M_BMQTOOL_POSTER
#define INCLUDED_M_BMQTOOL_POSTER

//@PURPOSE: Helper classes for posting series of messages.
//
//@CLASSES:
//
//  m_bmqtool::PostingContext: A class to hold a context of a single series
//  of messages to be posted.
//  m_bmqtool::Poster: A factory-semantic class to hold everything needed for
//  posting and ease creating posting contexts: buffers,
//  a logger, an allocator etc.
//
//@DESCRIPTION: 'm_bmqtool_poster' contains classes that abstract the mechanism
// of posting series of messages.

// BMQTOOL
#include <m_bmqtool_filelogger.h>
#include <m_bmqtool_parameters.h>

// BMQ
#include <bmqa_messageproperties.h>
#include <bmqa_session.h>
#include <mwcst_statcontext.h>

// BDE
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_string.h>

namespace BloombergLP {
namespace m_bmqtool {

// ====================
// class PostingContext
// ====================

/// A class to hold a context of a single series of messages to be posted.
class PostingContext {
  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Held, not owned.

    bdlbb::PooledBlobBufferFactory* d_timeBufferFactory_p;
    // Small buffer factory for the first
    // blob of the published message, used to
    // hold the timestamp information.

    Parameters* d_parameters_p;
    // Parameters of posting.

    bmqa::Session* d_session_p;
    // A session to post messages.
    // Held, not owned.

    FileLogger* d_fileLogger;
    // Where to log posted messages.
    // Held, not owned.

    mwcst::StatContext* d_statContext_p;
    // StatContext for msg/event stats.
    // Held, not owned

    int d_remainingEvents;
    // How many events are still left for posting.

    int d_numMessagesPosted;
    // How many events have already been posted.

    int d_msgUntilNextTimestamp;
    // In how many events the next timestamp will be posted.

    bdlbb::Blob d_blob;
    // Blob to post

    bmqa::QueueId d_queueId;
    // Queue ID for posting

    bmqa::MessageProperties d_properties;
    // Properties that will be added to a posted message

  public:
    // CREATORS

    PostingContext(bmqa::Session*                  session,
                   Parameters*                     parameters,
                   const bmqa::QueueId&            queueId,
                   FileLogger*                     fileLogger,
                   mwcst::StatContext*             statContext,
                   bdlbb::PooledBlobBufferFactory* bufferFactory,
                   bdlbb::PooledBlobBufferFactory* timeBufferFactory,
                   bslma::Allocator*               allocator);

    // MANIPULATORS

    /// Post next message. The behavior is undefined unless pendingPost()
    /// is true.
    void postNext();

    // ACCESSORS

    /// Return true is there is at least one message which should be posted.
    bool pendingPost() const;

  private:
    PostingContext(const PostingContext&);
    PostingContext& operator=(const PostingContext&);
};

// ============
// class Poster
// ============

// A factory-semantic class to hold everything needed for posting
// and ease creating posting contexts: buffers, a logger, an allocator etc.
class Poster {
    // DATA
    bslma::Allocator* d_allocator_p;
    // Held, not owned

    bdlbb::PooledBlobBufferFactory d_bufferFactory;
    // Buffer factory for the payload of the
    // published message

    bdlbb::PooledBlobBufferFactory d_timeBufferFactory;
    // Small buffer factory for the first
    // blob of the published message, to
    // hold the timestamp information

    mwcst::StatContext* d_statContext;
    // StatContext for msg/event stats.
    // Held, not owned

    FileLogger* d_fileLogger;
    // Logger to use in case events logging
    // to file has been enabled.
    // Held, not owned

  public:
    // CREATORS
    Poster(FileLogger*         fileLogger,
           mwcst::StatContext* statContext,
           bslma::Allocator*   allocator);

    // MANIPULATORS
    PostingContext createPostingContext(bmqa::Session*       session,
                                        Parameters*          parameters,
                                        const bmqa::QueueId& queueId);

  private:
    Poster(const Poster&);
    Poster& operator=(const Poster&);
};

}  // close package namespace
}  // close enterprise namespace

#endif  // INCLUDED_M_BMQTOOL_POSTER
