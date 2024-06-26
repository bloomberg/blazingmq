// Copyright 2014-2024 Bloomberg Finance L.P.
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

// m_bmqtool_interactive.cpp                                          -*-C++-*-
#include <m_bmqtool_interactive.h>

// BMQTOOL
#include <m_bmqtool_inpututil.h>
#include <m_bmqtool_parameters.h>
#include <m_bmqtool_poster.h>

// BMQ
#include <bmqa_event.h>
#include <bmqa_messageeventbuilder.h>
#include <bmqa_messageproperties.h>
#include <bmqa_session.h>
#include <bmqt_compressionalgorithmtype.h>
#include <bmqt_queueflags.h>
#include <bmqt_queueoptions.h>
#include <bmqt_resultcode.h>
#include <mwcu_memoutstream.h>

// BDE
#include <baljsn_decoder.h>
#include <baljsn_decoderoptions.h>
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlf_memfn.h>
#include <bdls_processutil.h>
#include <bdlt_currenttime.h>
#include <bsl_iostream.h>
#include <bslmt_lockguard.h>
#include <bslmt_turnstile.h>

namespace BloombergLP {
namespace m_bmqtool {

namespace {

/// Print to stdout the specified `message` prefixed by the specified
/// `prefix`.
void printMessage(bsl::ostream& out, int index, const bmqa::Message& message)
{
    const int k_MAX_DUMPED_BYTES = 32;

    bdlbb::Blob blob;
    message.getData(&blob);
    bdlbb::BlobUtilAsciiDumper dumpy(&blob);
    mwcu::MemOutStream         ss;
    ss << dumpy;

    bslstl::StringRef blobBegin(ss.str().data(),
                                bsl::min(static_cast<int>(ss.str().length()),
                                         k_MAX_DUMPED_BYTES));

    out << bsl::endl
        << "  #" << bsl::setw(3) << index << "  [" << message.messageGUID()
        << "] Queue: '" << message.queueId() << "' = '" << blobBegin << "'";

    if (message.hasProperties()) {
        bmqa::MessageProperties properties;
        message.loadProperties(&properties);
        out << ", with properties: " << properties;
    }

#ifdef BMQ_ENABLE_MSG_GROUPID
    if (message.hasGroupId()) {
        out << ", with Group Id: " << message.groupId();
    }
#endif
}

}  // close unnamed namespace

// -----------------
// class Interactive
// -----------------

// PRIVATE MANIPULATORS
void Interactive::printHelp()
{
    BALL_LOG_INFO
        << bsl::endl
        << "Commands:" << bsl::endl
        << "  start (async=true)" << bsl::endl
        << "  stop (async=true)" << bsl::endl
        << "  open uri=\"\" flags=\"\" (async=true) (maxUnconfirmedMessages=x)"
        << " (maxUnconfirmedBytes=y) (consumerPriority=z)" << bsl::endl
        << "    (subscriptions=[{\"expression\": \"\", (\"correlationId\": "
           "u), "
        << "(\"maxUnconfirmedMessages\": v), (\"maxUnconfirmedBytes\": w), "
        << "(\"consumerPriority\": p)}])" << bsl::endl
        << "  configure uri=\"\" (async=true) (maxUnconfirmedMessages=x)"
        << " (maxUnconfirmedBytes=y) (consumerPriority=z)" << bsl::endl
        << "    (subscriptions=[{\"expression\": \"\", (\"correlationId\": "
           "u), "
        << "(\"maxUnconfirmedMessages\": v), (\"maxUnconfirmedBytes\": w), "
        << "(\"consumerPriority\": p)}])" << bsl::endl
        << "  close uri=\"\" (async=true)" << bsl::endl
        << "  post uri=\"\" payload=[\"\",\"\"] (async=true) "
           "(compressionAlgorithmType=[NONE|ZLIB])"
        << bsl::endl
        << "    (messageProperties=[{\"name\": \"\", \"value\": \"\", "
           "\"type\": \"\"}])"
        << bsl::endl
        << "  batch-post uri=\"\" payload=[\"\",\"\"] (msgSize=u) "
           "(eventSize=v) (eventsCount=w) (postInterval=x) (postRate=y)"
        << bsl::endl
        << "  list (uri=\"\")" << bsl::endl
        << "  confirm uri=\"\" guid=\"\" "
        << "('*' for all, '+/-n' for oldest/latest 'n')" << bsl::endl
        << "  help" << bsl::endl
        << "  bye" << bsl::endl
        << bsl::endl
        << "Examples:" << bsl::endl

        << "  open uri=\"bmq://bmq.test.persistent.priority/qqq\" "
           "flags=\"write\""
        << bsl::endl
        << "  open uri=\"bmq://bmq.test.persistent.priority/qqq\" "
           "flags=\"read\" subscriptions="
        << "[{\"expression\": \"x > 0\", \"correlationId\": 1}, "
        << "{\"expression\": \"x <= 0\", \"correlationId\": 2}]" << bsl::endl
        << "    - 'open' command requires 'uri' and 'flags' arguments, "
        << "parameter 'subscriptions' is optional" << bsl::endl
        << bsl::endl
        << "  post uri=\"bmq://bmq.test.persistent.priority/qqq\" "
           "payload=[\"sample message\"]"
        << bsl::endl
        << "  post uri=\"bmq://bmq.test.persistent.priority/qqq\" "
           "payload=[\"sample message\"] "
        << "messageProperties=[{\"name\": \"x\", \"value\": \"10\", \"type\": "
           "\"E_INT\"}, "
        << "{\"name\": \"sample_str\", \"value\": \"foo\", \"type\": "
           "\"E_STRING\"}]"
        << bsl::endl
        << "    - 'post' command requires 'uri' and 'payload' arguments, "
        << "parameter 'messageProperties' is optional" << bsl::endl
        << bsl::endl
        << "  batch-post uri=\"bmq://bmq.test.persistent.priority/qqq\" "
           "payload=[\"sample message\"] eventsCount=300 postInterval=5000 "
           "postRate=10"
        << bsl::endl
        << "    - 'batch-post' command requires 'uri' argument, "
           "all the rest are optional"
        << bsl::endl
        << bsl::endl
        << "  load-post uri=\"bmq://bmq.test.persistent.priority/qqq\" "
           "file=\"message.dump\""
        << bsl::endl
        << "    - 'load-post' command requires 'uri' and 'file' arguments"
        << bsl::endl
        << bsl::endl;
}

void Interactive::processCommand(const StartCommand& command)
{
    BALL_LOG_INFO << "--> Starting session: " << command;
    int rc;
    if (command.async()) {
        rc = d_session_p->startAsync(bsls::TimeInterval(5.0));
    }
    else {
        rc = d_session_p->start(bsls::TimeInterval(5.0));
    }

    ball::Severity::Level severity = (rc == 0 ? ball::Severity::INFO
                                              : ball::Severity::ERROR);
    BALL_LOG_STREAM(severity)
        << "<-- session.start(5.0) => " << bmqt::GenericResult::Enum(rc)
        << " (" << rc << ")";

    if (d_parameters_p->noSessionEventHandler()) {
        BALL_LOG_INFO << "Creating processing threads";
        for (int i = 0; i < d_parameters_p->numProcessingThreads(); ++i) {
            bslmt::ThreadUtil::Handle threadHandle;
            rc = bslmt::ThreadUtil::create(
                &threadHandle,
                bdlf::MemFnUtil::memFn(&Interactive::eventHandlerThread,
                                       this));
            BSLS_ASSERT_SAFE(rc == 0);
            d_eventHandlerThreads.push_back(threadHandle);
        }
    }
}

void Interactive::processCommand(const StopCommand& command)
{
    BALL_LOG_INFO << "--> Stopping session: " << command;

    if (command.async()) {
        d_session_p->stopAsync();
    }
    else {
        d_session_p->stop();
    }

    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // MUTEX LOCKED

        // Clear pending unconfirmed messages so that we can re-start fresh
        d_uris.clear();
    }

    BALL_LOG_INFO << "<-- session.stop()";

    if (d_parameters_p->noSessionEventHandler()) {
        // Join on all threads
        BALL_LOG_INFO << "Joining event handler threads";
        for (size_t i = 0; i < d_eventHandlerThreads.size(); ++i) {
            bslmt::ThreadUtil::join(d_eventHandlerThreads[i]);
        }
        d_eventHandlerThreads.clear();
    }
}

void Interactive::processCommand(const OpenQueueCommand& command)
{
    // Validate command parameters ...
    if (command.uri().empty()) {
        BALL_LOG_ERROR << "'uri' is mandatory";
        return;  // RETURN
    }

    // Parse and validate URI
    bmqt::Uri   uri;
    bsl::string uriParseError;
    int rc = bmqt::UriParser::parse(&uri, &uriParseError, command.uri());
    if (rc != 0) {
        BALL_LOG_ERROR << "The uri ('" << command.uri()
                       << "') is not valid: " << uriParseError;
        return;  // RETURN
    }

    // Parse and validate FLAGS
    mwcu::MemOutStream  error;
    bsls::Types::Uint64 flags = 0;
    rc = bmqt::QueueFlagsUtil::fromString(error, &flags, command.flags());
    if (rc != 0) {
        BALL_LOG_ERROR << error.str();
        return;  // RETURN
    }

    // Open the queue ...
    BALL_LOG_INFO << "--> Opening queue: " << command;
    bmqa::QueueId queueId(bmqt::CorrelationId::autoValue());

    bmqt::QueueOptions queueOptions;
    queueOptions.setMaxUnconfirmedMessages(command.maxUnconfirmedMessages())
        .setMaxUnconfirmedBytes(command.maxUnconfirmedBytes())
        .setConsumerPriority(command.consumerPriority());

    if (!InputUtil::populateSubscriptions(&queueOptions,
                                          command.subscriptions())) {
        BALL_LOG_ERROR << "Invalid subscriptions";
        return;  // RETURN
    }

    if (command.async()) {
        d_session_p->openQueueAsync(
            &queueId,
            uri,
            flags,
            bdlf::MemFnUtil::memFn(&Interactive::onOpenQueueStatus, this),
            queueOptions);
        return;  // RETURN
    }

    // SYNC
    // Synchronously open the queue.
    // PUSH message processing is concurrent in another thread.  Synchronize it
    // by the means of atomic int in URIs map entry.  'e_IN_PROGRESS' value
    // blocks PUSH message processing.
    // Pre-create the entry before calling 'openQueueSync'.
    // See comments in 'Interactive::onMessage'

    UriEntryPtr           entry = createUriEntry(command.uri(), e_IN_PROGRESS);
    bmqa::OpenQueueStatus status =
        d_session_p->openQueueSync(&queueId, uri, flags, queueOptions);

    // While 'entry->first' is 'e_IN_PROGRESS', PUSH message processing is
    // blocked.  Log open queue response before updating the entry.

    const ball::Severity::Level severity = ((status.result() ==
                                             bmqt::OpenQueueResult::e_SUCCESS)
                                                ? ball::Severity::INFO
                                                : ball::Severity::ERROR);
    BALL_LOG_STREAM(severity) << "<-- session.openQueueSync() => "
                              << status.result() << " (" << status << ")";

    // On success, update the entry
    if (status) {
        // update the atomic status without locking
        entry->d_status = e_SUCCESS;
    }
    else {
        if (status.result() != bmqt::OpenQueueResult::e_ALREADY_IN_PROGRESS &&
            status.result() != bmqt::OpenQueueResult::e_ALREADY_OPENED) {
            // update the atomic status without locking
            entry->d_status = e_FAILURE;

            // remove the entry under lock
            removeUriEntry(command.uri());
        }
    }

    BSLS_ASSERT_SAFE(entry->d_status != e_IN_PROGRESS);
}

void Interactive::processCommand(const ConfigureQueueCommand& command)
{
    // Validate command parameters ...
    if (command.uri().empty()) {
        BALL_LOG_ERROR << "'uri' is mandatory";
        return;  // RETURN
    }

    // Parse and validate URI
    bmqt::Uri   uri;
    bsl::string uriParseError;
    int rc = bmqt::UriParser::parse(&uri, &uriParseError, command.uri());
    if (rc != 0) {
        BALL_LOG_ERROR << "The uri ('" << command.uri()
                       << "') is not valid: " << uriParseError;
        return;  // RETURN
    }

    // Lookup the Queue by URI
    bmqa::QueueId queueId;
    if (d_session_p->getQueueId(&queueId, command.uri()) != 0) {
        BALL_LOG_ERROR << "unknown queue '" << command.uri() << "'";
        return;  // RETURN
    }

    // Construct the queue options
    bmqt::QueueOptions queueOptions;
    queueOptions.setMaxUnconfirmedMessages(command.maxUnconfirmedMessages())
        .setMaxUnconfirmedBytes(command.maxUnconfirmedBytes())
        .setConsumerPriority(command.consumerPriority());

    if (!InputUtil::populateSubscriptions(&queueOptions,
                                          command.subscriptions())) {
        BALL_LOG_ERROR << "Invalid subscriptions";
        return;  // RETURN
    }

    // Configure the queue
    BALL_LOG_INFO << "--> Configuring queue: " << command;

    // ASYNC
    if (command.async()) {
        d_session_p->configureQueueAsync(
            &queueId,
            queueOptions,
            bdlf::MemFnUtil::memFn(&Interactive::onConfigureQueueStatus,
                                   this));

        return;  // RETURN
    }

    // SYNC
    bmqa::ConfigureQueueStatus status =
        d_session_p->configureQueueSync(&queueId, queueOptions);

    const ball::Severity::Level severity =
        (status.result() == bmqt::ConfigureQueueResult::e_SUCCESS
             ? ball::Severity::INFO
             : ball::Severity::ERROR);
    BALL_LOG_STREAM(severity) << "<-- session.configureQueueSync() => "
                              << status.result() << " (" << status << ")";
}

void Interactive::processCommand(const CloseQueueCommand& command)
{
    // Validate command parameters
    if (command.uri().empty()) {
        BALL_LOG_ERROR << "'uri' is mandatory";
        return;  // RETURN
    }

    // Lookup the Queue by URI
    bmqa::QueueId queueId;
    if (d_session_p->getQueueId(&queueId, command.uri()) != 0) {
        BALL_LOG_ERROR << "unknown queue '" << command.uri() << "'";
        return;  // RETURN
    }

    // Close the queue
    BALL_LOG_INFO << "--> Closing queue: " << command;

    if (command.async()) {
        // Logging and response handling will be done in the callback
        d_session_p->closeQueueAsync(
            &queueId,
            bdlf::MemFnUtil::memFn(&Interactive::onCloseQueueStatus, this));
        return;  // RETURN
    }

    // SYNC
    const bmqa::CloseQueueStatus status = d_session_p->closeQueueSync(
        &queueId);

    const ball::Severity::Level severity =
        (status.result() == bmqt::CloseQueueResult::e_SUCCESS
             ? ball::Severity::INFO
             : ball::Severity::ERROR);
    BALL_LOG_STREAM(severity) << "<-- session.closeQueueSync() => "
                              << status.result() << " (" << status << ")";

    // On success, erase messages associated with the queue - those will be
    // resent if the queue reopens
    if (status) {
        removeUriEntry(command.uri());
    }
}

void Interactive::processCommand(const PostCommand& command, bool hasMPs)
{
    // Validate command parameters
    if (command.uri().empty()) {
        BALL_LOG_ERROR << "'uri' is mandatory";
        return;  // RETURN
    }

    // Lookup the Queue by URI
    bmqa::QueueId queueId;
    if (d_session_p->getQueueId(&queueId, command.uri()) != 0) {
        BALL_LOG_ERROR << "unknown queue '" << command.uri() << "'";
        return;  // RETURN
    }

    BALL_LOG_INFO << "--> Posting message: " << command;

    int rc;
    // Build the messageEvent
    bmqa::MessageEventBuilder eventBuilder;
    d_session_p->loadMessageEventBuilder(&eventBuilder);
    for (size_t i = 0; i < command.payload().size(); ++i) {
        bmqa::Message& msg = eventBuilder.startMessage();

        // Put a correlationId if the queue was opened with ACK
        if (bmqt::QueueFlagsUtil::isAck(queueId.flags())) {
            bmqt::CorrelationId cid(bmqt::CorrelationId::autoValue());
            msg.setCorrelationId(cid);
            BALL_LOG_DEBUG << "PUT'ing msg with corrId: " << cid;
        }
        else {
            BALL_LOG_DEBUG << "PUT'ing msg with no corrId";
        }

        bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType;
        const bsl::string& inputCAT = command.compressionAlgorithmType();

        if (bmqt::CompressionAlgorithmType::fromAscii(
                &compressionAlgorithmType,
                inputCAT)) {
            msg.setCompressionAlgorithmType(compressionAlgorithmType);
        }
        else {
            BALL_LOG_WARN << "'" << inputCAT << "' is not a valid compression"
                          << " type, message will be sent uncompressed.";
        }

        bmqa::MessageProperties properties(d_allocator_p);

        if (hasMPs) {
            d_session_p->loadMessageProperties(&properties);

            InputUtil::populateProperties(&properties,
                                          command.messageProperties());
            msg.setPropertiesRef(&properties);
        }

        // Set data
        msg.setDataRef(command.payload()[i].c_str(),
                       command.payload()[i].size());

        if (!command.groupid().empty()) {
#ifdef BMQ_ENABLE_MSG_GROUPID
            msg.setGroupId(command.groupid().c_str());
#else
            BALL_LOG_WARN << "'MsgGroupId' field is not supported. Ignoring "
                          << "it.";
#endif
        }

        bmqt::EventBuilderResult::Enum ebr = eventBuilder.packMessage(queueId);
        if (bmqt::EventBuilderResult::e_SUCCESS != ebr) {
            BALL_LOG_ERROR << "Failed to pack message. rc: " << ebr;
            return;  // RETURN
        }
    }

    // Post
    rc = d_session_p->post(eventBuilder.messageEvent());

    ball::Severity::Level severity = (rc == 0 ? ball::Severity::INFO
                                              : ball::Severity::ERROR);
    BALL_LOG_STREAM(severity)
        << "<-- session.post() => " << bmqt::GenericResult::Enum(rc) << " ("
        << rc << ")";
}

void Interactive::processCommand(const ConfirmCommand& command)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // MUTEX LOCKED

    // Validate command parameters
    if (command.guid().empty()) {
        BALL_LOG_ERROR << "'guid' is mandatory";
        return;  // RETURN
    }

    // Parse and validate URI
    bmqt::Uri   uri;
    bsl::string uriParseError;
    int rc = bmqt::UriParser::parse(&uri, &uriParseError, command.uri());
    if (rc != 0) {
        BALL_LOG_ERROR << "The uri ('" << command.uri()
                       << "') is not valid: " << uriParseError;
        return;  // RETURN
    }

    // Lookup the Queue by URI
    bmqa::QueueId queueId;
    if (d_session_p->getQueueId(&queueId, command.uri()) != 0) {
        BALL_LOG_ERROR << "unknown queue '" << command.uri() << "'";
        return;  // RETURN
    }

    UrisMaps::iterator mapIter = d_uris.find(command.uri());
    BSLS_ASSERT_SAFE(mapIter != d_uris.end());
    const UriEntryPtr& entry = mapIter->second;
    // The status must indicate that the queue is opened.
    BSLS_ASSERT_SAFE(entry->d_status != e_IN_PROGRESS);

    // Process guids
    MessagesMap&                   messages = entry->d_messages;
    bsl::vector<bmqt::MessageGUID> guids;

    if (command.guid() == "*") {
        guids.reserve(messages.size());
        for (MessagesMap::const_iterator it = messages.begin();
             it != messages.end();
             ++it) {
            guids.push_back(it->first);
        }
    }
    else if (command.guid()[0] == '+' || command.guid()[0] == '-') {
        bsl::istringstream is(command.guid());
        int                value;
        is >> value;
        if (is.fail()) {
            BALL_LOG_ERROR << "'" << command.guid() << "' is not a valid GUID";
            return;  // RETURN
        }

        if (value > 0) {
            // Oldest guids
            for (MessagesMap::const_iterator it = messages.begin();
                 it != messages.end();
                 ++it) {
                guids.push_back(it->first);
                if (--value == 0) {
                    break;  // BREAK
                }
            }
        }
        else {
            // Newest guids
            value *= -1;
            int toSkip                     = messages.size() - value;
            toSkip                         = (toSkip < 0 ? 0 : toSkip);
            MessagesMap::const_iterator it = messages.begin();
            while (toSkip-- != 0) {
                ++it;
            }
            while (it != messages.end()) {
                guids.push_back(it->first);
                ++it;
            }
        }
    }
    else {
        if (!bmqt::MessageGUID::isValidHexRepresentation(
                command.guid().c_str())) {
            BALL_LOG_ERROR << "'" << command.guid() << "' is not a valid GUID";
            return;  // RETURN
        }

        bmqt::MessageGUID guid;
        guid.fromHex(command.guid().c_str());

        MessagesMap::const_iterator it = messages.find(guid);
        if (it == messages.end()) {
            BALL_LOG_ERROR << "'" << guid << "' NOT found";
            return;  // RETURN
        }

        guids.push_back(guid);
    }

    if (guids.empty()) {
        BALL_LOG_INFO << "No messages to confirm";
        return;  // RETURN
    }

    // Retrieve the message from the map
    for (bsl::vector<bmqt::MessageGUID>::const_iterator itGUID = guids.begin();
         itGUID != guids.end();
         ++itGUID) {
        MessagesMap::const_iterator itMsg = messages.find(*itGUID);
        BSLS_ASSERT_SAFE(itMsg != messages.end());

        BALL_LOG_INFO << "--> Confirming message with GUID " << *itGUID;

        // Send the confirm on the session (sadly under the mutex lock, but
        // this is interactive mode ...)
        rc = d_session_p->confirmMessage(itMsg->second);

        ball::Severity::Level severity = (rc == 0 ? ball::Severity::INFO
                                                  : ball::Severity::ERROR);
        BALL_LOG_STREAM(severity)
            << "<-- session.confirmMessage() => "
            << bmqt::GenericResult::Enum(rc) << " (" << rc << ")";

        // Remove from the map
        messages.erase(*itGUID);
    }
}

void Interactive::processCommand(const ListCommand& command)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // MUTEX LOCKED

    int index = 0;

    if (command.uri().isNull()) {
        // Iterate over all open queues' messages

        size_t numUnconfirmed = 0;
        for (UrisMaps::const_iterator mapCiter = d_uris.begin();
             mapCiter != d_uris.end();
             ++mapCiter) {
            const UriEntryPtr& entry = mapCiter->second;
            numUnconfirmed += entry->d_messages.size();
        }

        BALL_LOG_INFO_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM
                << "Unconfirmed message listing: " << numUnconfirmed
                << " messages";

            for (UrisMaps::const_iterator mapCiter = d_uris.begin();
                 mapCiter != d_uris.end();
                 ++mapCiter) {
                const UriEntryPtr& entry    = mapCiter->second;
                const MessagesMap& messages = entry->d_messages;
                for (MessagesMap::const_iterator msgCiter = messages.begin();
                     msgCiter != messages.end();
                     ++msgCiter) {
                    printMessage(BALL_LOG_OUTPUT_STREAM,
                                 ++index,
                                 msgCiter->second);
                }
            }
        }
    }
    else {
        // Iterate over messages from the user-provided queue uri
        UrisMaps::const_iterator mapCiter = d_uris.find(command.uri().value());
        if (mapCiter == d_uris.end()) {
            BALL_LOG_ERROR << "unknown queue '" << command.uri() << "'";
            return;  // RETURN
        }

        const UriEntryPtr& entry    = mapCiter->second;
        const MessagesMap& messages = entry->d_messages;

        BALL_LOG_INFO_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM
                << "Unconfirmed message listing: " << messages.size()
                << " messages";

            for (MessagesMap::const_iterator msgCiter = messages.begin();
                 msgCiter != messages.end();
                 ++msgCiter) {
                printMessage(BALL_LOG_OUTPUT_STREAM,
                             ++index,
                             msgCiter->second);
            }
        }
    }
}

void Interactive::processCommand(const BatchPostCommand& command)
{
    Parameters parameters(d_allocator_p);
    parameters.setEventsCount(command.eventsCount());
    parameters.setPostRate(command.postRate());
    parameters.setEventSize(command.eventSize());
    parameters.setPostInterval(command.postInterval());
    parameters.setMsgSize(command.msgSize());

    // Lookup the Queue by URI
    bmqa::QueueId queueId;
    if (d_session_p->getQueueId(&queueId, command.uri()) != 0) {
        BALL_LOG_ERROR << "unknown queue '" << command.uri() << "'";
        return;  // RETURN
    }
    parameters.setQueueFlags(queueId.flags());

    bslmt::Turnstile turnstile(1000.0);
    if (parameters.postInterval() != 0) {
        turnstile.reset(1000.0 / parameters.postInterval());
    }

    bsl::shared_ptr<PostingContext> postingContext =
        d_poster_p->createPostingContext(d_session_p, &parameters, queueId);

    while (postingContext->pendingPost()) {
        postingContext->postNext();
        if (parameters.postInterval() != 0 && postingContext->pendingPost()) {
            turnstile.waitTurn();
        }
    }

    BALL_LOG_INFO << "All messages have been posted";
}

void Interactive::processCommand(const LoadPostCommand& command)
{
    // Validate command parameters
    if (command.uri().empty()) {
        BALL_LOG_ERROR << "'uri' is mandatory";
        return;  // RETURN
    }
    if (command.file().empty()) {
        BALL_LOG_ERROR << "'file' is mandatory";
        return;  // RETURN
    }

    // Lookup the Queue by URI
    bmqa::QueueId queueId;
    if (d_session_p->getQueueId(&queueId, command.uri()) != 0) {
        BALL_LOG_ERROR << "unknown queue '" << command.uri() << "'";
        return;  // RETURN
    }

    BALL_LOG_INFO << "--> Loading and posting message: " << command;

    // Build the messageEvent
    bmqa::MessageEventBuilder eventBuilder;
    d_session_p->loadMessageEventBuilder(&eventBuilder);
    bmqa::Message& msg = eventBuilder.startMessage();

    // Put a correlationId if the queue was opened with ACK
    if (bmqt::QueueFlagsUtil::isAck(queueId.flags())) {
        bmqt::CorrelationId cid(bmqt::CorrelationId::autoValue());
        msg.setCorrelationId(cid);
        BALL_LOG_DEBUG << "PUT'ing msg with corrId: " << cid;
    }
    else {
        BALL_LOG_DEBUG << "PUT'ing msg with no corrId";
    }

    // Load message content from the file
    mwcu::MemOutStream payloadStream;
    mwcu::MemOutStream propertiesStream;
    mwcu::MemOutStream errorDescription;
    if (!InputUtil::loadMessageFromFile(&payloadStream,
                                        &propertiesStream,
                                        &errorDescription,
                                        command.file(),
                                        d_allocator_p)) {
        BALL_LOG_ERROR << "Failed to load message from file: "
                       << errorDescription.str();
        return;  // RETURN
    }

    // Set message properties
    const bsl::string              properties = propertiesStream.str();
    bmqa::MessageProperties        messageProperties(d_allocator_p);
    bdlbb::PooledBlobBufferFactory bufferFactory(1024, d_allocator_p);
    if (!properties.empty()) {
        // Convert string to blob
        bdlbb::Blob blob(&bufferFactory, d_allocator_p);
        bdlbb::BlobUtil::append(&blob,
                                properties.c_str(),
                                static_cast<int>(properties.size()));

        // Deserialize message properties from blob
        const int rc = messageProperties.streamIn(blob);
        if (rc != 0) {
            BALL_LOG_ERROR << "Failed to streamIn message properties, rc = "
                           << rc;
            return;  // RETURN
        }
        msg.setPropertiesRef(&messageProperties);
    }

    // Set payload
    bsl::string payload = payloadStream.str();
    msg.setDataRef(payload.c_str(), payload.size());

    bmqt::EventBuilderResult::Enum ebr = eventBuilder.packMessage(queueId);
    if (bmqt::EventBuilderResult::e_SUCCESS != ebr) {
        BALL_LOG_ERROR << "Failed to pack message. rc: " << ebr;
        return;  // RETURN
    }

    // Post
    const int rc = d_session_p->post(eventBuilder.messageEvent());

    ball::Severity::Level severity = (rc == 0 ? ball::Severity::INFO
                                              : ball::Severity::ERROR);
    BALL_LOG_STREAM(severity)
        << "<-- session.post() => " << bmqt::GenericResult::Enum(rc) << " ("
        << rc << ")";
}

Interactive::UriEntryPtr
Interactive::createUriEntry(const bsl::string&          uri,
                            Interactive::UriEntryStatus status)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // MUTEX LOCKED

    UrisMaps::const_iterator it = d_uris.find(uri);

    if (it == d_uris.end()) {
        UriEntryPtr ptr(new (*d_allocator_p) UriEntry, d_allocator_p);

        ptr->d_status = status;
        it            = d_uris.insert(bsl::make_pair(uri, ptr)).first;
    }

    return it->second;
}

void Interactive::removeUriEntry(const bsl::string& uri)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // MUTEX LOCKED

    UrisMaps::iterator mapIter = d_uris.find(uri);
    if (mapIter == d_uris.end()) {
        BALL_LOG_ERROR << "'uri' " << uri << " was not found.";
        return;  // RETURN
    }

    d_uris.erase(mapIter);
}

void Interactive::eventHandlerThread()
{
    while (true) {
        bmqa::Event event = d_session_p->nextEvent();

        if (event.isSessionEvent()) {
            d_sessionEventHandler_p->onSessionEvent(event.sessionEvent());
            if (event.sessionEvent().type() ==
                bmqt::SessionEventType::e_DISCONNECTED) {
                break;  // BREAK
            }
        }
        else if (event.isMessageEvent()) {
            d_sessionEventHandler_p->onMessageEvent(event.messageEvent());
        }
    }

    BALL_LOG_INFO << "EventHandlerThread terminated";
}

Interactive::Interactive(Parameters*       parameters,
                         Poster*           poster,
                         bslma::Allocator* allocator)
: d_session_p(0)
, d_sessionEventHandler_p(0)
, d_parameters_p(parameters)
, d_uris(allocator)
, d_eventHandlerThreads(allocator)
, d_producerIdProperty("** NONE **", allocator)
, d_poster_p(poster)
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(parameters);
    BSLS_ASSERT_SAFE(poster);

    bdls::ProcessUtil::getProcessName(&d_producerIdProperty);  // ignore rc

    bsl::ostringstream pidStr;
    pidStr << ":" << bdls::ProcessUtil::getProcessId();

    d_producerIdProperty.append(pidStr.str());
}

int Interactive::initialize(bmqa::Session*             session,
                            bmqa::SessionEventHandler* sessionEventHandler)
{
    // Print the welcome banner
    BALL_LOG_INFO << "Welcome to the BlazingMQ tool.\n"
                  << "Type 'help' to see the list of commands. Crl-D to quit.";

    d_session_p             = session;
    d_sessionEventHandler_p = sessionEventHandler;

    return 0;
}

int Interactive::mainLoop()
{
    bool started = false;

    while (true) {
        bsl::string input;

        if (!InputUtil::getLine(&input)) {
            break;  // BREAK
        }

        if (input.empty()) {
            continue;  // CONTINUE
        }
        else if ((input == "bye") || (input == "quit") || (input == "q")) {
            break;  // BREAK
        }
        else if (input == "help") {
            printHelp();
        }
        else {
            bsl::string                     verb, jsonInput, error;
            bsl::unordered_set<bsl::string> keys(d_allocator_p);

            // How to tell "messageProperties=[]" from the absence of the key?
            // baljsn::Decoder does not seem to help so we track all keys.

            InputUtil::preprocessInput(&verb, &jsonInput, input, &keys);
            if (verb == "start") {
                StartCommand command;
                if (InputUtil::parseCommand(&command, &error, jsonInput)) {
                    processCommand(command);
                    started = true;
                }
            }
            else if (verb == "stop") {
                StopCommand command;
                if (InputUtil::parseCommand(&command, &error, jsonInput)) {
                    processCommand(command);
                    started = false;
                }
            }
            else if (verb == "open") {
                OpenQueueCommand command;
                if (InputUtil::parseCommand(&command, &error, jsonInput)) {
                    processCommand(command);
                }
            }
            else if (verb == "configure") {
                ConfigureQueueCommand command;
                if (InputUtil::parseCommand(&command, &error, jsonInput)) {
                    processCommand(command);
                }
            }
            else if (verb == "close") {
                CloseQueueCommand command;
                if (InputUtil::parseCommand(&command, &error, jsonInput)) {
                    processCommand(command);
                }
            }
            else if (verb == "post") {
                PostCommand command;
                if (InputUtil::parseCommand(&command, &error, jsonInput)) {
                    const bdlat_AttributeInfo* mps =
                        PostCommand::lookupAttributeInfo(
                            PostCommand::ATTRIBUTE_ID_MESSAGE_PROPERTIES);
                    bool hasMPs = false;
                    if (mps) {
                        hasMPs = keys.find(mps->name()) != keys.cend();
                    }
                    processCommand(command, hasMPs);
                }
            }
            else if (verb == "list") {
                ListCommand command;
                if (InputUtil::parseCommand(&command, &error, jsonInput)) {
                    processCommand(command);
                }
            }
            else if (verb == "confirm") {
                ConfirmCommand command;
                if (InputUtil::parseCommand(&command, &error, jsonInput)) {
                    processCommand(command);
                }
            }
            else if (verb == "batch-post") {
                BatchPostCommand command;
                if (InputUtil::parseCommand(&command, &error, jsonInput)) {
                    processCommand(command);
                }
            }
            else if (verb == "load-post") {
                LoadPostCommand command;
                if (InputUtil::parseCommand(&command, &error, jsonInput)) {
                    processCommand(command);
                }
            }
            else {
                BALL_LOG_ERROR << "Unknown command: " << verb;
            }

            if (!error.empty()) {
                BALL_LOG_ERROR << "Unable to decode: " << jsonInput
                               << bsl::endl
                               << error;
            }
        }
    }

    if (started) {
        d_session_p->stop();
    }

    // Clear the unconfirmed message map: the blobs are coming from the IO, the
    // blob buffer factory is owned by the bmqa::Session which is destroyed
    // when clearing the session ManagedPtr.
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // MUTEX LOCKED
        d_uris.clear();
    }

    return 0;
}

void Interactive::onMessage(const bmqa::Message& message)
{
    // Look up URI entry.  In the case of asynchronous open queue command, the
    // entry is created upon open queue response which is processed before any
    // PUSH message (in the same thread).  In the case of synchronous open
    // queue command (in another thread), the entry is pre-created.
    // The entry has atomic int status.  When open queue response and PUSH
    // message processing happen on different threads, the status serves as a
    // synchronization primitive.  Spin until the value is e_IN_PROGRESS.
    // See comment in 'Interactive::processCommand'.
    UriEntryPtr entry;
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // MUTEX LOCKED

        UrisMaps::iterator mapIter = d_uris.find(
            message.queueId().uri().asString());
        BSLS_ASSERT_SAFE(mapIter != d_uris.end());

        entry = mapIter->second;  // increment reference counter
    }
    BSLS_ASSERT_SAFE(entry);

    // Spin until e_IN_PROGRESS.
    while (e_IN_PROGRESS == entry->d_status) {
        bslmt::ThreadUtil::yield();
    }

    BSLS_ASSERT_SAFE(entry->d_status == e_SUCCESS);

    // Add the message to the internal map
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // MUTEX LOCKED

        entry->d_messages.insert(
            bsl::make_pair(message.messageGUID(), message.clone()));
    }
}

void Interactive::onOpenQueueStatus(const bmqa::OpenQueueStatus& status)
{
    if (d_parameters_p->verbosity() != ParametersVerbosity::e_SILENT) {
        BALL_LOG_INFO << "==> OPEN_QUEUE_RESULT received: " << status;
    }

    const ball::Severity::Level severity = ((status.result() ==
                                             bmqt::OpenQueueResult::e_SUCCESS)
                                                ? ball::Severity::INFO
                                                : ball::Severity::ERROR);
    BALL_LOG_STREAM(severity) << "<-- session.openQueueAsync() => "
                              << status.result() << " (" << status << ")";

    if (status.result() == bmqt::OpenQueueResult::e_SUCCESS) {
        BSLS_ASSERT_OPT(status.queueId().isValid() &&
                        status.queueId().uri().isValid());

        // On success, create an entry for the full uri in our maps of
        // unconfirmed messages
        createUriEntry(status.queueId().uri().asString(), e_SUCCESS);
    }
}

void Interactive::onConfigureQueueStatus(
    const bmqa::ConfigureQueueStatus& status)
{
    if (d_parameters_p->verbosity() != ParametersVerbosity::e_SILENT) {
        BALL_LOG_INFO << "==> CONFIGURE_QUEUE_RESULT received: " << status;
    }

    const ball::Severity::Level severity =
        ((status.result() == bmqt::ConfigureQueueResult::e_SUCCESS)
             ? ball::Severity::INFO
             : ball::Severity::ERROR);
    BALL_LOG_STREAM(severity) << "<-- session.configureQueueAsync() => "
                              << status.result() << " (" << status << ")";
}

void Interactive::onCloseQueueStatus(const bmqa::CloseQueueStatus& status)
{
    if (d_parameters_p->verbosity() != ParametersVerbosity::e_SILENT) {
        BALL_LOG_INFO << "==> CLOSE_QUEUE_RESULT received: " << status;
    }

    const ball::Severity::Level severity = ((status.result() ==
                                             bmqt::CloseQueueResult::e_SUCCESS)
                                                ? ball::Severity::INFO
                                                : ball::Severity::ERROR);
    BALL_LOG_STREAM(severity) << "<-- session.closeQueueAsync() => "
                              << status.result() << " (" << status << ")";

    if (status.result() == bmqt::CloseQueueResult::e_SUCCESS) {
        BSLS_ASSERT_OPT(status.queueId().uri().isValid());

        // On success, remove context associated with the queue.
        removeUriEntry(status.queueId().uri().asString());
    }
}

}  // close package namespace
}  // close enterprise namespace
