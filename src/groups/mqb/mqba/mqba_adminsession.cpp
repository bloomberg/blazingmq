// Copyright 2022-2023 Bloomberg Finance L.P.
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

// mqba_adminsession.cpp                                              -*-C++-*-
#include <mqba_adminsession.h>

#include <mqbscm_version.h>
/// Implementation Notes
///====================
//
/// Threading
///---------
// In order to be lock free, *all* methods are or must be executed on the
// *client dispatcher thread*.  The only exceptions are:
//: o processEvent: main entry of processing of data from the channel, executes
//:   on the IO thread and does basic safe processing before enqueuing for the
//:   client dispatcher
//: o tearDown: executes on the IO thread when the channel went down
//
/// Shutdown
///--------
// Expecting an ungraceful shutdown at any time.  'Disconnect' message and
// graceful disconnect sequence were removed for simplicity.
//

//
// The following variables are used for keeping track of the shutdown state:
//: o d_self:
//:   This is used, as described above, for the situation of remotely activated
//:   asynchronous messages, which can't be synchronized by enqueuing a marker
//:   in the dispatcher's queue.
//: o d_running:
//:   This flag is only set and checked in the client thread.  All new messages
//:   received from the channel will be dropped after this flag is set to
//:   'false'.
//

// BMQ
#include <bmqp_controlmessageutil.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>

#include <bmqio_status.h>
#include <bmqu_blob.h>
#include <bmqu_printutil.h>
#include <bmqu_weakmemfn.h>

// BDE
#include <ball_log.h>
#include <bdlb_scopeexit.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_limits.h>
#include <bsl_memory.h>
#include <bsl_vector.h>
#include <bslmt_semaphore.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqba {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("MQBA.ADMINSESSION");

/// Based on the client type, return the pointer to the correct identity
/// field of the specified `negotiationMesage`.
bmqp_ctrlmsg::ClientIdentity* extractClientIdentity(
    const bmqp_ctrlmsg::NegotiationMessage& negotiationMessage)
{
    switch (negotiationMessage.selectionId()) {
    case bmqp_ctrlmsg::NegotiationMessage::SELECTION_INDEX_CLIENT_IDENTITY: {
        return const_cast<bmqp_ctrlmsg::ClientIdentity*>(
            &negotiationMessage.clientIdentity());  // RETURN
    }
    case bmqp_ctrlmsg::NegotiationMessage::SELECTION_INDEX_BROKER_RESPONSE: {
        return const_cast<bmqp_ctrlmsg::ClientIdentity*>(
            &negotiationMessage.brokerResponse().brokerIdentity());  // RETURN
    }
    default: {
        BSLS_ASSERT_OPT(false && "Invalid negotiation message");
        return 0;  // RETURN
    }
    };
}

}  // close unnamed namespace

// -------------------------
// struct AdminSessionState
// -------------------------

AdminSessionState::AdminSessionState(BlobSpPool*               blobSpPool,
                                     bdlbb::BlobBufferFactory* bufferFactory,
                                     bmqp::EncodingType::Enum  encodingType,
                                     bslma::Allocator*         allocator)
: d_allocator_p(allocator)
, d_dispatcherClientData()
, d_bufferFactory_p(bufferFactory)
, d_blobSpPool_p(blobSpPool)
, d_schemaEventBuilder(bufferFactory, allocator, encodingType)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(encodingType != bmqp::EncodingType::e_UNKNOWN);
}

// -------------------
// class AdminSession
// -------------------

// PRIVATE MANIPULATORS
void AdminSession::sendPacket()
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    bdlb::ScopeExitAny resetBlobScopeGuard(
        bdlf::BindUtil::bind(&bmqp::SchemaEventBuilder::reset,
                             &d_state.d_schemaEventBuilder));
    const bdlbb::Blob& blob = d_state.d_schemaEventBuilder.blob();

    // This method is the centralized *single* place where we should try to
    // send data to the client over the channel.

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_running)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    // Try to send the data, or drop it if we fail due to high watermark limit.
    bmqio::Status status;
    d_channel_sp->write(&status, blob);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            status.category() != bmqio::StatusCategory::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        if (status.category() == bmqio::StatusCategory::e_CONNECTION) {
            // This code relies on the fact that `e_CONNECTION` error cannot be
            // returned without calling `tearDown`.
            return;  // RETURN
        }
        if (status.category() == bmqio::StatusCategory::e_LIMIT) {
            BALL_LOG_ERROR
                << "#ADMCLIENT_SEND_FAILURE " << description()
                << ": Failed to send data [size: "
                << bmqu::PrintUtil::prettyNumber(blob.length())
                << " bytes] to admin client due to channel watermark limit"
                << "; dropping.";
        }
        else {
            BALL_LOG_INFO << "#ADMCLIENT_SEND_FAILURE " << description()
                          << ": Failed to send data [size: "
                          << bmqu::PrintUtil::prettyNumber(blob.length())
                          << " bytes] to admin client with status: " << status;
        }
    }
}

void AdminSession::initiateShutdownDispatched(const ShutdownCb& callback)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(callback);

    d_running = false;
    callback();
}

void AdminSession::finalizeAdminCommand(
    const bmqp_ctrlmsg::ControlMessage& adminCommandCtrlMsg,
    const bsl::string&                  commandExecResults)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(adminCommandCtrlMsg.choice().isAdminCommandValue());
    BSLS_ASSERT_SAFE(d_state.d_schemaEventBuilder.blob().length() == 0);

    // Send success/error response to client
    bdlma::LocalSequentialAllocator<2048> localAllocator(
        d_state.d_allocator_p);

    bmqp_ctrlmsg::ControlMessage response(&localAllocator);
    response.rId() = adminCommandCtrlMsg.rId().value();
    response.choice().makeAdminCommandResponse();

    response.choice().adminCommandResponse().text() = commandExecResults;

    int rc = d_state.d_schemaEventBuilder.setMessage(
        response,
        bmqp::EventType::e_CONTROL);

    if (rc != 0) {
        BALL_LOG_ERROR << "#ADMCLIENT_SEND_FAILURE " << description()
                       << ": Unable to send admin command response [reason: "
                       << "ENCODING_FAILED, rc: " << rc << "]: " << response;
        return;  // RETURN
    }

    BALL_LOG_TRACE << description()
                   << ": Sending adminCommand response: " << response;

    // Send the response.
    sendPacket();
}

void AdminSession::onProcessedAdminCommand(
    const bmqp_ctrlmsg::ControlMessage& adminCommandCtrlMsg,
    int                                 rc,
    const bsl::string&                  commandExecResults)
{
    // executed by the *ANY* thread
    (void)rc;

    dispatcher()->execute(
        bdlf::BindUtil::bind(&AdminSession::finalizeAdminCommand,
                             this,
                             adminCommandCtrlMsg,
                             commandExecResults),
        this,
        mqbi::DispatcherEventType::e_DISPATCHER);
}

void AdminSession::enqueueAdminCommand(
    const bmqp_ctrlmsg::ControlMessage& adminCommandCtrlMsg)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(adminCommandCtrlMsg.choice().isAdminCommandValue());

    bmqp_ctrlmsg::AdminCommand req =
        adminCommandCtrlMsg.choice().adminCommand();

    d_adminCb(
        d_channel_sp->peerUri(),
        req.command(),
        bdlf::BindUtil::bind(bmqu::WeakMemFnUtil::weakMemFn(
                                 &AdminSession::onProcessedAdminCommand,
                                 d_self.acquireWeak()),
                             adminCommandCtrlMsg,
                             bdlf::PlaceHolders::_1,   // rc
                             bdlf::PlaceHolders::_2),  // commandExecResults
        false);                                        // fromReroute
}

// CREATORS
AdminSession::AdminSession(
    const bsl::shared_ptr<bmqio::Channel>&        channel,
    const bmqp_ctrlmsg::NegotiationMessage&       negotiationMessage,
    const bsl::string&                            sessionDescription,
    mqbi::Dispatcher*                             dispatcher,
    AdminSessionState::BlobSpPool*                blobSpPool,
    bdlbb::BlobBufferFactory*                     bufferFactory,
    bdlmt::EventScheduler*                        scheduler,
    const mqbnet::Session::AdminCommandEnqueueCb& adminCb,
    bslma::Allocator*                             allocator)
: d_self(this)  // use default allocator
, d_running(true)
, d_negotiationMessage(negotiationMessage, allocator)
, d_clientIdentity_p(extractClientIdentity(d_negotiationMessage))
, d_description(sessionDescription, allocator)
, d_channel_sp(channel)
, d_state(blobSpPool,
          bufferFactory,
          bmqp::SchemaEventBuilderUtil::bestEncodingSupported(
              d_clientIdentity_p->features()),
          allocator)
, d_scheduler_p(scheduler)
, d_adminCb(adminCb)
{
    // Register this client to the dispatcher
    mqbi::Dispatcher::ProcessorHandle processor = dispatcher->registerClient(
        this,
        mqbi::DispatcherClientType::e_SESSION);

    BALL_LOG_INFO << description() << ": created "
                  << "[dispatcherProcessor: " << processor
                  << ", identity: " << *(d_clientIdentity_p)
                  << ", ptr: " << this;
}

AdminSession::~AdminSession()
{
    // executed by ONE of the *QUEUE* dispatcher threads

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_self.isValid());

    BALL_LOG_INFO << description() << ": destructor";

    // Unregister from the dispatcher
    dispatcher()->unregisterClient(this);
}

// MANIPULATORS
//   (virtual: mqbnet::Session)
void AdminSession::processEvent(
    const bmqp::Event&     event,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    // executed by the *IO* thread

    if (!event.isControlEvent()) {
        BALL_LOG_ERROR << "#ADMCLIENT_UNEXPECTED_EVENT " << description()
                       << ": Unexpected event type: " << event;
        return;  // RETURN
    }

    bdlma::LocalSequentialAllocator<2048> localAllocator(
        d_state.d_allocator_p);
    bmqp_ctrlmsg::ControlMessage controlMessage(&localAllocator);

    int rc = event.loadControlEvent(&controlMessage);
    if (rc != 0) {
        BALL_LOG_ERROR << "#CORRUPTED_EVENT " << description()
                       << ": Received invalid control message from client "
                       << "[reason: 'failed to decode', rc: " << rc << "]:\n"
                       << bmqu::BlobStartHexDumper(event.blob());
        return;  // RETURN
    }

    BALL_LOG_INFO << description()
                  << ": Received control message: " << controlMessage;

    rc = bmqp::ControlMessageUtil::validate(controlMessage);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        // Malformed control message should be rejected
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << "#CORRUPTED_CONTROL_MESSAGE " << description()
                       << ": Received malformed control message from "
                       << "client [reason: 'malformed fields', rc: " << rc
                       << "]";
        return;  // RETURN
    }

    typedef bmqp_ctrlmsg::ControlMessageChoice MsgChoice;  // shortcut

    const MsgChoice& choice = controlMessage.choice();

    // Only 'AdminCommand' messages are expected.  'Disconnect' messages are
    // removed from AdminSession for simplicity.
    if (choice.selectionId() != MsgChoice::SELECTION_ID_ADMIN_COMMAND) {
        BALL_LOG_ERROR << "#ADMCLIENT_IMPROPER_BEHAVIOR " << description()
                       << ": unexpected controlMessage: " << controlMessage;
        return;  // RETURN
    }

    // Dispatch the event to be processed in dispatcher thread.
    dispatcher()->execute(
        bdlf::BindUtil::bind(&AdminSession::enqueueAdminCommand,
                             this,
                             controlMessage),
        this);
}

void AdminSession::tearDownImpl(bslmt::Semaphore* semaphore)
{
    // executed by the *CLIENT* dispatcher thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    d_self.invalidate();

    // We can now wake up the IO thread, and let the channel be destroyed
    semaphore->post();
}

void AdminSession::tearDown(const bsl::shared_ptr<void>& session,
                            bool                         isBrokerShutdown)
{
    // executed by the *IO* thread
    (void)session;
    (void)isBrokerShutdown;

    // Cancel the reads on the channel
    d_channel_sp->cancelRead();

    // Enqueue an event to the client dispatcher thread and wait for it to
    // finish; only after this will we have the guarantee that no method will
    // try to use the channel.
    bslmt::Semaphore semaphore;

    // NOTE: We use the e_DISPATCHER type because this Client is dying, and the
    //       process of this event by the dispatcher should not add it to the
    //       flush list.
    dispatcher()->execute(
        bdlf::BindUtil::bind(&AdminSession::tearDownImpl, this, &semaphore),
        this,
        mqbi::DispatcherEventType::e_DISPATCHER);
    semaphore.wait();

    // At this point, we are sure the client dispatcher thread has no pending
    // processing that could be using the channel, so we can return, which will
    // destroy the channel.  The session will die when all references to the
    // 'session' go out of scope.
}

void AdminSession::initiateShutdown(const ShutdownCb&         callback,
                                    const bsls::TimeInterval& timeout,
                                    bool supportShutdownV2)
{
    // executed by the *ANY* thread
    (void)timeout;
    (void)supportShutdownV2;

    dispatcher()->execute(
        bdlf::BindUtil::bind(&AdminSession::initiateShutdownDispatched,
                             this,
                             callback),
        this);
}

void AdminSession::invalidate()
{
    // NOTHING
}

// MANIPULATORS
//   (virtual: mqbi::DispatcherClient)
void AdminSession::onDispatcherEvent(const mqbi::DispatcherEvent& event)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    // NOTE: We don't perform 'd_operationState' check in this method because
    //       it might be desirable to dispatch certain callbacks to the client
    //       session object while it is being cleaned up.  Performing above
    //       check in each callback/method provides more flexibility.

    BALL_LOG_TRACE << description() << ": processing dispatcher event '"
                   << event << "'";

    switch (event.type()) {
    case mqbi::DispatcherEventType::e_CALLBACK: {
        const mqbi::DispatcherCallbackEvent* realEvent =
            &event.getAs<mqbi::DispatcherCallbackEvent>();

        BSLS_ASSERT_SAFE(realEvent->callback());
        realEvent->callback()(dispatcherClientData().processorHandle());
    } break;

    case mqbi::DispatcherEventType::e_ACK:
    case mqbi::DispatcherEventType::e_CLUSTER_STATE:
    case mqbi::DispatcherEventType::e_CONFIRM:
    case mqbi::DispatcherEventType::e_CONTROL_MSG:
    case mqbi::DispatcherEventType::e_DISPATCHER:
    case mqbi::DispatcherEventType::e_PUSH:
    case mqbi::DispatcherEventType::e_PUT:
    case mqbi::DispatcherEventType::e_RECOVERY:
    case mqbi::DispatcherEventType::e_REJECT:
    case mqbi::DispatcherEventType::e_REPLICATION_RECEIPT:
    case mqbi::DispatcherEventType::e_STORAGE:
    case mqbi::DispatcherEventType::e_UNDEFINED:
    default: {
        BALL_LOG_ERROR
            << "#ADMCLIENT_UNEXPECTED_EVENT " << description()
            << ": Session received unexpected dispatcher event [type: "
            << event.type() << "]";
    }
    };
}

void AdminSession::flush()
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    // No pending events are expected for admin session.
}

}  // close package namespace
}  // close enterprise namespace
