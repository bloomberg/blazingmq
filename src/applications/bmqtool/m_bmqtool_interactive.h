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

// m_bmqtool_interactive.h                                            -*-C++-*-
#ifndef INCLUDED_M_BMQTOOL_INTERACTIVE
#define INCLUDED_M_BMQTOOL_INTERACTIVE

//@PURPOSE: JSON parser and bmqa::Session command processing.
//
//@CLASSES:
//  m_bmqtool::Interactive: JSON parser and bmqa::Session command processing.
//
//@DESCRIPTION: 'm_bmqtool_interactive' provides a loop to read commands (in
// JSON format) and interpret those commands.

// BMQTOOL
#include <m_bmqtool_messages.h>
#include <m_bmqtool_poster.h>

// BMQ
#include <bmqa_closequeuestatus.h>
#include <bmqa_configurequeuestatus.h>
#include <bmqa_message.h>
#include <bmqa_openqueuestatus.h>
#include <bmqt_messageguid.h>

// MWC
#include <mwcc_orderedhashmap.h>

// BDE
#include <ball_log.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslmt_mutex.h>
#include <bslmt_threadutil.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqa {
class Session;
}
namespace bmqa {
class SessionEventHandler;
}
namespace m_bmqtool {
class Parameters;
}

namespace m_bmqtool {

// =================
// class Interactive
// =================

/// JSON parser and bmqa::Session command processing.
class Interactive {
  private:
    // PRIVATE TYPES

    /// Must be ordered by msgGUID to have them oldest first when iterating
    typedef mwcc::OrderedHashMap<bmqt::MessageGUID,
                                 bmqa::Message,
                                 bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        MessagesMap;

    struct UriEntry {
        bsls::AtomicInt d_status;
        MessagesMap     d_messages;
    };
    // URI map entry consists of open queue command status and message map
    // The status '0' is command in-progress, '1' is success, '-1' is
    // failure.

    // TYPES
    enum UriEntryStatus { e_FAILURE = -1, e_IN_PROGRESS = 0, e_SUCCESS = 1 };

    /// URI map entry is reference counted.
    typedef bsl::shared_ptr<UriEntry> UriEntryPtr;

    /// Map of full uri to UriEntryPtr
    typedef bsl::unordered_map<bsl::string, UriEntryPtr> UrisMaps;

    typedef bsl::vector<bslmt::ThreadUtil::Handle> ThreadHandleVector;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQTOOL.INTERACTIVE");

  private:
    bmqa::Session* d_session_p;
    // bmqa::Session to work with

    bmqa::SessionEventHandler* d_sessionEventHandler_p;
    // Session handler to use for events
    // processing, if using custom event handler

    Parameters* d_parameters_p;
    // Parameters to use

    bslmt::Mutex d_mutex;
    // Mutex to protect below map

    UrisMaps d_uris;
    // Map of full URI to UriEntryPtr

    ThreadHandleVector d_eventHandlerThreads;
    // Vector of event handler thread handles, if
    // using custom event handler

    bsl::string d_producerIdProperty;
    // A message property named 'producerId' to be
    // used if 'properties=true' flag has been
    // specified in the 'post' command in
    // interactive mode.  Note that this string is
    // of the format '/path/to/this/bmqtool:<PID>'.

    Poster* d_poster_p;
    // A factory for posting series of messages.
    // Held, not owned

    bslma::Allocator* d_allocator_p;
    // Held, not owned

  private:
    // PRIVATE MANIPULATORS
    void printHelp();

    /// Process the specified `command`.
    void processCommand(const StartCommand& command);
    void processCommand(const StopCommand& command);
    void processCommand(const OpenQueueCommand& command);
    void processCommand(const ConfigureQueueCommand& command);
    void processCommand(const CloseQueueCommand& command);
    void processCommand(const PostCommand& command, bool hasMPs);
    void processCommand(const ConfirmCommand& command);
    void processCommand(const ListCommand& command);
    void processCommand(const BatchPostCommand& command);

    /// Create and insert an entry keyed on the specified `uri` into the map
    /// of full uri to unconfirmed messages contained in this object.  The
    /// specified `status` indicates whether corresponding open queue
    /// command is in progress or has completed.  Return smart pointer to
    /// the newly created entry.
    UriEntryPtr createUriEntry(const bsl::string& uri, UriEntryStatus status);

    /// Remove context for the queue identified by the specified `uri`
    /// from the URIs map.
    void removeUriEntry(const bsl::string& uri);

    /// Load message content from file for the specified `command`.
    /// Return `true` on success or `false` otherwise.
    bool loadMessageFromFile(PostCommand& command);

    /// `main` of the tread used for custom event handler
    void eventHandlerThread();

  public:
    // CREATORS

    /// Constructor using the specified `parameters`, 'poster',
    /// and `allocator`.
    Interactive(Parameters*       parameters,
                Poster*           poster,
                bslma::Allocator* allocator);

    // MANIPULATORS

    /// Initialize this `Interactive` instance, by setting the specified
    /// `session` to use.
    int initialize(bmqa::Session*             session,
                   bmqa::SessionEventHandler* sessionEventHandler);

    /// main loop; read from stdin, parse JSON commands and executes them.
    int mainLoop();

    /// Called by the session's message event handler when a new message has
    /// been received.
    void onMessage(const bmqa::Message& message);

    /// Called by the event processing thread(s) in response to an
    /// asynchronous openQueue.
    void onOpenQueueStatus(const bmqa::OpenQueueStatus& status);

    /// Called by the event processing thread(s) in response to an
    /// asynchronous configureQueue.
    void onConfigureQueueStatus(const bmqa::ConfigureQueueStatus& status);

    /// Called by the event processing thread(s) in response to an
    /// asynchronous closeQueue.
    void onCloseQueueStatus(const bmqa::CloseQueueStatus& status);
};

}  // close package namespace
}  // close enterprise namespace

#endif
