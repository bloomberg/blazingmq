// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mqbblp_queuesessionmanager.cpp                                     -*-C++-*-
#include <mqbblp_queuesessionmanager.h>

#include <mqbscm_version.h>
/// Implementation Notes
///====================
// See notes in mqba_clientsession.cpp with regards to shutdown handling.

// MQB
#include <mqbconfm_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_dispatcher.h>
#include <mqbi_queue.h>

// BMQ
#include <bmqp_protocolutil.h>
#include <bmqp_queueutil.h>
#include <bmqt_uri.h>

// BDE
#include <bdld_datum.h>
#include <bdld_datummapbuilder.h>
#include <bdlf_bind.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_utility.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbblp {

namespace {

/// Create the queue stats datum associated with the specified `statContext`
/// and having the specified `domain`, `cluster`, and `queueFlags`.
void createQueueStatsDatum(bmqst::StatContext* statContext,
                           const bsl::string&  domain,
                           const bsl::string&  cluster,
                           bsls::Types::Uint64 queueFlags)
{
    typedef bslma::ManagedPtr<bdld::ManagedDatum> ManagedDatumMp;
    bslma::Allocator* alloc = statContext->datumAllocator();
    ManagedDatumMp    datum = statContext->datum();

    bdld::DatumMapBuilder builder(alloc);
    builder.pushBack("queueFlags",
                     bdld::Datum::createInteger64(queueFlags, alloc));
    builder.pushBack("domain", bdld::Datum::copyString(domain, alloc));
    builder.pushBack("cluster", bdld::Datum::copyString(cluster, alloc));

    datum->adopt(builder.commit());
}

/// This method does nothing; it is just used so that we can control the
/// closeQueue flow for a client, by binding the specified `handle` (which
/// is a shared_ptr to the queue handle being closed) to events being
/// enqueued to the dispatcher and let it die `naturally` when the
/// associated queue's dispatcher thread is drained up to the event.
void handleHolderDummy(
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<void>& handle)
{
    // NOTHING
}

}  // close unnamed namespace

// -------------------------
// class QueueSessionManager
// -------------------------

void QueueSessionManager::onDomainQualifiedCb(
    const bmqp_ctrlmsg::Status&         status,
    const bsl::string&                  resolvedDomain,
    const GetHandleCallback&            successCallback,
    const ErrorCallback&                errorCallback,
    const bmqt::Uri&                    uri,
    const bmqp_ctrlmsg::ControlMessage& request,
    const bmqu::AtomicValidatorSp&      validator)
{
    // executed by the *CLIENT* dispatcher thread (TBD: for now)

    bmqu::AtomicValidatorGuard guard(validator.get());
    if (!guard.isValid()) {
        // The session was destroyed before we received the response (see
        // implementation notes at top of this file for explanation).
        BALL_LOG_INFO << "The session was destroyed before the response to: "
                      << request;
        return;  // RETURN
    }

    if (d_shutdownInProgress) {
        return;  // RETURN
    }

    if (status.category() != bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        // Failed to qualify domain..
        BALL_LOG_WARN << "#CLIENT_OPENQUEUE_FAILURE "
                      << d_dispatcherClient_p->description()
                      << ": Error while qualifying domain: [reason: " << status
                      << ", request: " << request << "]";

        // Send an error from the client dispatcher thread
        dispatchErrorCallback(errorCallback,
                              status.category(),
                              status.message(),
                              status.code());

        return;  // RETURN
    }

    // Successfully qualified domain, now open the queue on the domain;
    // replacing the queue URI with the resolved domain.
    bmqp_ctrlmsg::ControlMessage ctrlMessage(request);
    bmqt::UriBuilder             uriBuilder(uri);
    bmqt::Uri                    qualifiedUri;
    uriBuilder.setQualifiedDomain(resolvedDomain);
    int rc = uriBuilder.uri(&qualifiedUri);
    BSLS_ASSERT_SAFE(rc == 0);
    if (rc != 0) {
        BALL_LOG_WARN << "#CLIENT_OPENQUEUE_FAILURE "
                      << d_dispatcherClient_p->description()
                      << ": Error while building qualifed URI from request: "
                      << request << ", builder rc: " << rc
                      << ", qualified domain [" << resolvedDomain << "].";

        // Send an error from the client dispatcher thread
        dispatchErrorCallback(errorCallback,
                              bmqp_ctrlmsg::StatusCategory::E_REFUSED,
                              "Error while building qualified URI.",
                              0);
        return;  // RETURN
    }

    ctrlMessage.choice().openQueue().handleParameters().uri() =
        qualifiedUri.asString();

    d_domainFactory_p->createDomain(
        qualifiedUri.qualifiedDomain(),
        bdlf::BindUtil::bind(&QueueSessionManager::onDomainOpenCb,
                             this,
                             bdlf::PlaceHolders::_1,  // status
                             bdlf::PlaceHolders::_2,  // domain
                             successCallback,
                             errorCallback,
                             qualifiedUri,
                             ctrlMessage,
                             validator));
}

void QueueSessionManager::onDomainOpenCb(
    const bmqp_ctrlmsg::Status&         status,
    mqbi::Domain*                       domain,
    const GetHandleCallback&            successCallback,
    const ErrorCallback&                errorCallback,
    const bmqt::Uri&                    uri,
    const bmqp_ctrlmsg::ControlMessage& request,
    const bmqu::AtomicValidatorSp&      validator)
{
    // executed by the *ANY* thread (TBD: for now)

    // Preconditions
    BSLS_ASSERT_SAFE(request.choice().isOpenQueueValue());

    bmqu::AtomicValidatorGuard guard(validator.get());
    if (!guard.isValid()) {
        // The session was destroyed before we received the response (see
        // implementation notes at top of this file for explanation).
        BALL_LOG_INFO << "The session was destroyed before the response to: "
                      << request;
        return;  // RETURN
    }

    if (d_shutdownInProgress) {
        return;  // RETURN
    }

    if (status.category() != bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        // Failed to open domain
        BALL_LOG_WARN << "#CLIENT_OPENQUEUE_FAILURE "
                      << d_dispatcherClient_p->description()
                      << ": Error while opening domain: [reason: " << status
                      << ", request: " << request << "]";

        // Send an error from the client dispatcher thread.
        dispatchErrorCallback(errorCallback,
                              status.category(),
                              status.message(),
                              status.code());
        return;  // RETURN
    }

    domain->openQueue(
        uri,
        d_requesterContext_sp,
        request.choice().openQueue().handleParameters(),
        bdlf::BindUtil::bind(&QueueSessionManager::onQueueOpenCb,
                             this,
                             bdlf::PlaceHolders::_1,  // status
                             bdlf::PlaceHolders::_2,  // queueHandle
                             bdlf::PlaceHolders::_3,  // openQueueResponse
                             bdlf::PlaceHolders::_4,  // confirmationCookie
                             successCallback,
                             request,
                             validator));
}

void QueueSessionManager::onQueueOpenCb(
    const bmqp_ctrlmsg::Status&              status,
    mqbi::QueueHandle*                       queueHandle,
    const bmqp_ctrlmsg::OpenQueueResponse&   openQueueResponse,
    const mqbi::OpenQueueConfirmationCookie& confirmationCookie,
    const GetHandleCallback&                 responseCallback,
    const bmqp_ctrlmsg::ControlMessage&      request,
    const bmqu::AtomicValidatorSp&           validator)
{
    // executed by *ANY* thread

    bmqu::AtomicValidatorGuard guard(validator.get());
    if (!guard.isValid()) {
        // The session was destroyed before we received the response (see
        // implementation notes at top of this file for explanation).
        BALL_LOG_INFO << "The session was destroyed before the response: "
                      << openQueueResponse;
        return;  // RETURN
    }

    if (d_shutdownInProgress) {
        return;  // RETURN
    }

    d_dispatcherClient_p->dispatcher()->execute(
        bdlf::BindUtil::bind(&QueueSessionManager::onQueueOpenCbDispatched,
                             this,
                             status,
                             queueHandle,
                             openQueueResponse,
                             confirmationCookie,
                             responseCallback,
                             request),
        d_dispatcherClient_p);
}

void QueueSessionManager::onQueueOpenCbDispatched(
    const bmqp_ctrlmsg::Status&              status,
    mqbi::QueueHandle*                       queueHandle,
    const bmqp_ctrlmsg::OpenQueueResponse&   openQueueResponse,
    const mqbi::OpenQueueConfirmationCookie& confirmationCookie,
    const GetHandleCallback&                 responseCallback,
    const bmqp_ctrlmsg::ControlMessage&      request)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcherClient_p->dispatcher()->inDispatcherThread(
        d_dispatcherClient_p));
    BSLS_ASSERT_SAFE(request.choice().isOpenQueueValue());

    if (d_shutdownInProgress) {
        return;  // RETURN
    }

    const bmqp_ctrlmsg::QueueHandleParameters& handleParams =
        request.choice().openQueue().handleParameters();
    const bmqp::QueueId queueId =
        bmqp::QueueUtil::createQueueIdFromHandleParameters(handleParams);
    const char* apppId = bmqp::QueueUtil::extractAppId(handleParams);

    if (status.category() == bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        BSLS_ASSERT_SAFE(queueHandle);
        BSLS_ASSERT_SAFE(confirmationCookie->d_handle);
        // in case of success, the cookie must be a valid shared_ptr

        // Update the cookie to point to a null queue handle, which indicates
        // that requester (this client session) has successfully received and
        // processed the open-queue response.
        confirmationCookie->d_handle = 0;

        // Success, configure the handle and the session
        bsl::pair<QueueStateMap::iterator, bool> ins = d_queues.emplace(
            bsl::make_pair(queueId.id(), QueueState()));
        QueueState& qs = ins.first->second;

        if (ins.second) {
            // First time we use this queue
            qs.d_handle_p = queueHandle;
        }
        else {
            // We've used this queue before.  Search for the subId in the
            // associated map of substream information.  If it is found, do
            // nothing; otherwise, insert it into the map of substream
            // information.
            BSLS_ASSERT_SAFE(queueHandle == ins.first->second.d_handle_p);
        }

        QueueState::StreamsMap::iterator subQueueInfo =
            qs.d_subQueueInfosMap.insert(apppId, queueId.subId(), queueId);

        if (!subQueueInfo->value().d_stats && confirmationCookie->d_stats) {
            subQueueInfo->value().d_stats = confirmationCookie->d_stats;

            const bsl::string& domain = queueHandle->queue()->domain()->name();
            const bsl::string& cluster =
                queueHandle->queue()->domain()->cluster()->name();
            // TBD: There is a race in below snippet.  A queue handle's
            //      parameters must be read/written only from queue-dispatcher
            //      thread, but here they are being read from client-dispatcher
            //      thread.  Since this is used only for stats, it is left
            //      'broken' for now.
            const bsls::Types::Int64 queueFlags =
                queueHandle->handleParameters().flags();

            createQueueStatsDatum(subQueueInfo->value().d_stats->statContext(),
                                  domain,
                                  cluster,
                                  queueFlags);
        }
    }

    responseCallback(status, queueHandle, openQueueResponse);
}

void QueueSessionManager::onHandleReleased(
    const bsl::shared_ptr<mqbi::QueueHandle>& handle,
    const mqbi::QueueHandleReleaseResult&     result,
    const CloseHandleCallback&                successCallback,
    const ErrorCallback&                      errorCallback,
    const bmqp_ctrlmsg::ControlMessage&       request,
    const bmqu::AtomicValidatorSp&            validator)
{
    // executed by the *QUEUE* dispatcher thread

    bmqu::AtomicValidatorGuard guard(validator.get());
    if (!guard.isValid()) {
        // The session was destroyed before we received the response (see
        // implementation notes at top of this file for explanation).

        if (handle) {
            // Make sure 'clearClientDispatched' has finished _before_ 'handle'
            // destruction
            BSLS_ASSERT_SAFE(handle->queue());

            handle->queue()->dispatcher()->execute(
                bdlf::BindUtil::bind(&handleHolderDummy, handle),
                handle->queue());
        }

        BALL_LOG_INFO << "The session was destroyed before the response to: "
                      << request;
        return;  // RETURN
    }

    // If 'closeQueue' request failed to be processed for any reason (at this
    // node or at upstream node), this callback will be invoked with a null
    // 'handle'.  We explicitly check for that before an assertion which uses
    // 'handle'.
    if (handle) {
        BSLS_ASSERT_SAFE(
            d_dispatcherClient_p->dispatcher()->inDispatcherThread(
                handle->queue()));
    }

    // NOTE: We don't check 'd_shutdownInProgress' and always enqueue execution
    //       of 'onHandleReleasedDispatched to the dispatcher.  This is valid,
    //       because we successfully acquired the 'validator', so we have
    //       guarantees that we are *before* the 'teardown' method (since the
    //       very first thing it does is invalidate the guard), so it is always
    //       fine to enqueue that method (it will itself check
    //       'd_shutdownInProgress').
    //
    //       Refer to the comments inside 'onHandleReleasedDispatched' for the
    //       reason why this is needed.

    d_dispatcherClient_p->dispatcher()->execute(
        bdlf::BindUtil::bind(&QueueSessionManager::onHandleReleasedDispatched,
                             this,
                             handle,
                             result,
                             successCallback,
                             errorCallback,
                             request),
        d_dispatcherClient_p);
}

void QueueSessionManager::onHandleReleasedDispatched(
    const bsl::shared_ptr<mqbi::QueueHandle>& handle,
    const mqbi::QueueHandleReleaseResult&     result,
    const CloseHandleCallback&                successCallback,
    const ErrorCallback&                      errorCallback,
    const bmqp_ctrlmsg::ControlMessage&       request)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcherClient_p->dispatcher()->inDispatcherThread(
        d_dispatcherClient_p));

    // If 'closeQueue' request failed to be processed for any reason (at this
    // node or at upstream node), this callback will be invoked with a null
    // 'handle'.  We explicitly check for that.
    if (!handle) {
        BALL_LOG_ERROR << "#CLIENT_CLOSEQUEUE_FAILURE "
                       << d_dispatcherClient_p->description()
                       << ": Failed to release handle for request: "
                       << request;

        errorCallback(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                      "Failed to release handle.",
                      0);
        return;  // RETURN
    }

    const bmqp::QueueId queueId =
        bmqp::QueueUtil::createQueueIdFromHandleParameters(
            request.choice().closeQueue().handleParameters());

    QueueStateMap::iterator it = d_queues.find(queueId.id());
    if (it == d_queues.end()) {
        BALL_LOG_ERROR << "#CLIENT_UNKNOWN_QUEUE "
                       << d_dispatcherClient_p->description()
                       << ": Failed to release handle with '"
                       << handle->queue()->uri().asString() << "' [reason: id "
                       << queueId << " not found]";

        errorCallback(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                      "No queue with specified Id",
                      0);
        return;  // RETURN
    }

    // Delete the handle if fully released
    if (result.hasNoHandleClients()) {
        d_queues.erase(it);
        // That deletes all subQueues as well.
        BALL_LOG_INFO << d_dispatcherClient_p->description()
                      << ": Deleted handle "
                      << "[queue: '" << handle->queue()->uri().asString()
                      << "', id: " << queueId << "]";

        // Since 'isDeleted' flag is true, it means that the specified 'handle'
        // is the last reference to the queue handle, which will go out of
        // scope at the end of this function.  However, there could still be
        // some PUT events in the queue-dispatcher thread if the client
        // violated the contract by posting messages after closing the queue.
        // So that those PUT events are processed correctly (they will be
        // rejected by the queue), we need to keep the handle alive, and the
        // way to do that is to simply enqueue an event to the queue's
        // dispatcher thread, with the 'handle' as a bound argument.

        d_dispatcherClient_p->dispatcher()->execute(
            bdlf::BindUtil::bind(&handleHolderDummy, handle),
            handle->queue(),
            mqbi::DispatcherEventType::e_DISPATCHER);
    }
    else if (result.hasNoQueueStreamConsumers() &&
             result.hasNoQueueStreamProducers()) {
        QueueState::StreamsMap::const_iterator sqiIter =
            it->second.d_subQueueInfosMap.findBySubId(queueId.subId());
        BSLS_ASSERT_SAFE(sqiIter != it->second.d_subQueueInfosMap.end());
        BSLS_ASSERT_SAFE(sqiIter->value().d_queueId == queueId);
        it->second.d_subQueueInfosMap.erase(sqiIter);

        BALL_LOG_INFO << d_dispatcherClient_p->description()
                      << ": Deleted subStream [queue: '"
                      << handle->queue()->uri().asString()
                      << "', queueId: " << queueId << "]";
    }

    successCallback(handle);
}

void QueueSessionManager::dispatchErrorCallback(
    const ErrorCallback&                errorCallback,
    bmqp_ctrlmsg::StatusCategory::Value failureCategory,
    const bsl::string&                  errorDescription,
    const int                           code)
{
    // Send an error,
    // we need to enqueue to send from the client dispatcher thread
    d_dispatcherClient_p->dispatcher()->execute(
        bdlf::BindUtil::bind(errorCallback,
                             failureCategory,
                             errorDescription,
                             code),
        d_dispatcherClient_p);
}

QueueSessionManager::QueueSessionManager(
    mqbi::DispatcherClient*                    dispatcherClient,
    const bmqp_ctrlmsg::ClientIdentity&        clientIdentity,
    const bsl::shared_ptr<bmqst::StatContext>& statContext,
    mqbi::DomainFactory*                       domainFactory,
    bslma::Allocator*                          allocator)
: d_dispatcherClient_p(dispatcherClient)
, d_domainFactory_p(domainFactory)
, d_shutdownInProgress(false)
, d_validator_sp(new(*allocator) bmqu::AtomicValidator(), allocator)
, d_requesterContext_sp(new(*allocator)
                            mqbi::QueueHandleRequesterContext(allocator),
                        allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcherClient_p != 0);
    BSLS_ASSERT_SAFE(statContext);
    BSLS_ASSERT_SAFE(d_domainFactory_p != 0);

    d_requesterContext_sp->setClient(dispatcherClient)
        .setIdentity(clientIdentity)
        .setDescription(dispatcherClient->description())
        .setIsClusterMember(false)
        .setRequesterId(
            mqbi::QueueHandleRequesterContext ::generateUniqueRequesterId())
        .setStatContext(statContext);
}

QueueSessionManager::~QueueSessionManager()
{
    // executed by ONE of the *QUEUE* dispatcher threads

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_shutdownInProgress);
}

void QueueSessionManager::processOpenQueue(
    const bmqp_ctrlmsg::ControlMessage& request,
    const GetHandleCallback&            successCallback,
    const ErrorCallback&                errorCallback)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcherClient_p->dispatcher()->inDispatcherThread(
        d_dispatcherClient_p));
    BSLS_ASSERT_SAFE(request.choice().isOpenQueueValue());

    if (d_shutdownInProgress) {
        return;  // RETURN
    }

    bmqt::Uri                                  uri;
    bsl::string                                error;
    const bmqp_ctrlmsg::QueueHandleParameters& handleParams =
        request.choice().openQueue().handleParameters();

    int rc = bmqt::UriParser::parse(&uri, &error, handleParams.uri());
    if (rc != 0) {
        // Invalid URI (should not happen since the SDK validates the URI
        // before sending the request..)
        BALL_LOG_ERROR << "#CLIENT_OPENQUEUE_FAILURE "
                       << d_dispatcherClient_p->description()
                       << ": Error while opening queue: invalid URI [uri: '"
                       << handleParams.uri() << "' , error: '" << error
                       << "']";

        errorCallback(bmqp_ctrlmsg::StatusCategory::E_REFUSED, error, 0);
        return;  // RETURN
    }

    if (bmqp::QueueUtil::isEmpty(handleParams)) {
        errorCallback(bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT,
                      "At least one of ([read|write]) Count must be >0",
                      0);
        return;  // RETURN
    }

    const bool isExpectingReader = handleParams.readCount() > 0;
    if (bmqt::QueueFlagsUtil::isReader(handleParams.flags()) !=
        isExpectingReader) {
        errorCallback(bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT,
                      "Read flag and readCount must both be set accordingly",
                      0);
        return;  // RETURN
    }

    const bool isExpectingWriter = handleParams.writeCount() > 0;
    if (bmqt::QueueFlagsUtil::isWriter(handleParams.flags()) !=
        isExpectingWriter) {
        errorCallback(bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT,
                      "Write flag and writeCount must both be set accordingly",
                      0);
        return;  // RETURN
    }

    if (handleParams.qId() >= bmqp::QueueId::k_RESERVED_QUEUE_ID) {
        errorCallback(bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT,
                      "qId must not be set from the reserved queue ids range",
                      0);
        return;  // RETURN
    }

    const bool isWriter = bmqt::QueueFlagsUtil::isWriter(handleParams.flags());
    if (isWriter && !bmqp::QueueUtil::isDefaultSubstream(handleParams)) {
        errorCallback(bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT,
                      "AppId should not be specified in the URI if queue is "
                      "opened in write mode",
                      0);
        return;  // RETURN
    }

    // On the first hop, "resolve" the domain, i.e., check if the domain is
    // redirected, and, if yes, use the new name; otherwise go straight to
    // opening the domain.
    if (d_requesterContext_sp->isFirstHop()) {
        d_domainFactory_p->qualifyDomain(
            uri.qualifiedDomain(),
            bdlf::BindUtil::bind(&QueueSessionManager::onDomainQualifiedCb,
                                 this,
                                 bdlf::PlaceHolders::_1,  // status
                                 bdlf::PlaceHolders::_2,  // domain
                                 successCallback,
                                 errorCallback,
                                 uri,
                                 request,
                                 d_validator_sp));
    }
    else {
        // Query the domain manager
        d_domainFactory_p->createDomain(
            uri.qualifiedDomain(),
            bdlf::BindUtil::bind(&QueueSessionManager::onDomainOpenCb,
                                 this,
                                 bdlf::PlaceHolders::_1,  // status
                                 bdlf::PlaceHolders::_2,  // domain
                                 successCallback,
                                 errorCallback,
                                 uri,
                                 request,
                                 d_validator_sp));
    }
}

void QueueSessionManager::processCloseQueue(
    const bmqp_ctrlmsg::ControlMessage& request,
    const CloseHandleCallback&          successCallback,
    const ErrorCallback&                errorCallback)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcherClient_p->dispatcher()->inDispatcherThread(
        d_dispatcherClient_p));
    BSLS_ASSERT_SAFE(request.choice().isCloseQueueValue());

    if (d_shutdownInProgress) {
        return;  // RETURN
    }

    // Copy because we need to replace the URI by the `resolved` URI.
    bmqp_ctrlmsg::CloseQueue req = request.choice().closeQueue();

    if (bmqp::QueueUtil::isEmpty(req.handleParameters())) {
        errorCallback(bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT,
                      "At least one of [read|write|admin]Count must be > 0",
                      0);
        return;  // RETURN
    }

    const bmqp::QueueId queueId =
        bmqp::QueueUtil::createQueueIdFromHandleParameters(
            req.handleParameters());

    QueueStateMap::iterator it = d_queues.find(queueId.id());
    if (it == d_queues.end()) {
        // Failure to find queue.
        BALL_LOG_WARN << "#CLIENT_UNKNOWN_QUEUE "
                      << d_dispatcherClient_p->description()
                      << ": Requested to close a queue with unknown Id ("
                      << queueId << ").";
        errorCallback(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                      "No queue with specified Id",
                      0);
        return;  // RETURN
    }

    QueueState& queueState = it->second;
    if (queueState.d_hasReceivedFinalCloseQueue) {
        // Duplicate closeQueue request?  A closeQueue request with 'isFinal'
        // flag set to true was already received previously for this queue from
        // this client.
        BALL_LOG_WARN << "#CLIENT_IMPROPER_BEHAVIOR "
                      << d_dispatcherClient_p->description()
                      << ": Requested to close a queue with Id (" << queueId
                      << "), for which a final 'closeQueue' request was "
                      << "already received";
        errorCallback(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                      "Duplicate closeQueue request",
                      0);
        return;  // RETURN
    }

    QueueState::StreamsMap::iterator subQInfoIter =
        queueState.d_subQueueInfosMap.findBySubIdSafe(queueId.subId());
    if (subQInfoIter == queueState.d_subQueueInfosMap.end()) {
        // Failure to find subStream of the queue.
        BALL_LOG_WARN << "#CLIENT_UNKNOWN_QUEUE "
                      << d_dispatcherClient_p->description()
                      << ": Requested to close a queue with unknown subId"
                      << " (" << queueId << ").";
        errorCallback(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                      "No subStream of the queue with specified subId",
                      0);
        return;  // RETURN
    }

    mqbi::QueueHandle* handle = queueState.d_handle_p;

    queueState.d_hasReceivedFinalCloseQueue = req.isFinal();

    // Replace specified URI with the resolved URI, and then release the
    // handle.
    req.handleParameters().uri() = handle->queue()->uri().asString();
    handle->release(
        req.handleParameters(),
        req.isFinal(),
        bdlf::BindUtil::bind(&QueueSessionManager::onHandleReleased,
                             this,
                             bdlf::PlaceHolders::_1,  // handle
                             bdlf::PlaceHolders::_2,  // isDeleted
                             successCallback,
                             errorCallback,
                             request,
                             d_validator_sp));
}

void QueueSessionManager::tearDown()
{
    d_validator_sp->invalidate();
    d_shutdownInProgress = true;
}

}  // close package namespace
}  // close enterprise namespace
