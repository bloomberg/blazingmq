// Copyright 2015-2023 Bloomberg Finance L.P.
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

#include <bmqp_requestmanager.h>

#include <bmqscm_version.h>

// BMQ
#include <bmqex_systemexecutor.h>
#include <bmqio_channel.h>
#include <bmqio_status.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_requestmanagerrequest.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>
#include <bmqu_time.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_utility.h>
#include <bsla_annotations.h>
#include <bslma_managedptr.h>
#include <bslmt_lockguard.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>
#include <bsls_systemclocktype.h>

namespace BloombergLP {
namespace bmqp {

namespace {

/// Lowest identifier which can be assigned to a request; zero is reserved
/// to indicate that no request has been sent yet.
const int k_FIRST_REQUEST_ID = 1;

/// Highest identifier which can be assigned to a request, as dictated by
/// the type of the identifier field on the wire.
const int k_LAST_REQUEST_ID = bsl::numeric_limits<int>::max();

}  // close unnamed namespace

// --------------------
// class RequestManager
// --------------------

bmqt::GenericResult::Enum
RequestManager::sendHelper(bmqio::Channel*                     channel,
                           const bsl::shared_ptr<bdlbb::Blob>& blob_sp,
                           bsls::Types::Int64                  watermark)
{
    bmqio::Status status;
    channel->write(&status, *blob_sp, watermark);

    switch (status.category()) {
    case bmqio::StatusCategory::e_SUCCESS:
        return bmqt::GenericResult::e_SUCCESS;
    case bmqio::StatusCategory::e_CONNECTION:
        return bmqt::GenericResult::e_NOT_CONNECTED;
    case bmqio::StatusCategory::e_LIMIT:
        return bmqt::GenericResult::e_NOT_READY;
    case bmqio::StatusCategory::e_GENERIC_ERROR:
    case bmqio::StatusCategory::e_TIMEOUT:
    case bmqio::StatusCategory::e_CANCELED:
    default: return bmqt::GenericResult::e_UNKNOWN;
    }
}

int RequestManager::generateRequestId()
{
    // executed with 'd_mutex' locked

    // Each iteration considers a distinct identifier, and there are more
    // identifiers than there can be entries in 'd_requests', hence this loop
    // terminates.
    do {
        d_nextRequestId = (d_nextRequestId == k_LAST_REQUEST_ID)
                              ? k_FIRST_REQUEST_ID
                              : d_nextRequestId + 1;
    } while (d_requests.find(d_nextRequestId) != d_requests.end());

    return d_nextRequestId;
}

void RequestManager::onRequestTimeout(int requestId)
{
    // executed by the thread selected by 'd_executor'

    RequestSp request;

    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // MUTEX LOCKED

        RequestMapIter it = d_requests.find(requestId);

        if (it == d_requests.end()) {
            // The request must have completed at the same time from a
            // different thread while we were waiting on the 'd_mutex'.
            return;  // RETURN
        }

        request = it->second;

        request->d_haveTimeout = true;
        // Do not remove the request from the map yet (a response will
        // eventually be received, or the request be canceled).

        // Explicitly invalidate the timeout since we processed it
        request->d_timeoutSchedulerHandle.release();

        if (!d_lateResponseMode) {
            d_requests.erase(it);
        }
    }  // close guard scope

    BALL_LOG_ERROR << "Request with '" << request->nodeDescription()
                   << "' has timed out: " << request->request();

    // Now prepare a response and invoke the callback/signal outside the mutex.
    bmqp_ctrlmsg::ControlMessage& response = request->response();

    // 1. 'fake' a response, with a Timeout status type
    response.rId().makeValue(requestId);

    bmqu::MemOutStream os;
    os << "The request timedout after "
       << bmqu::PrintUtil::prettyTimeInterval(
              bmqu::Time::highResolutionTimer() - request->d_sendTime);

    response.choice().makeStatus();
    response.choice().status().code() = k_CODE_TIMEOUT_LOCAL;
    response.choice().status().message().assign(os.str().data(),
                                                os.str().length());
    reinterpret_cast<int&>(response.choice().status().category()) =
        static_cast<int>(bmqt::GenericResult::e_TIMEOUT);
    // Note that above reinterpret & static casts are needed to suppress
    // compiler diagnostics, because this component does not include
    // bmqp_ctrlmsg_messages.h on purpose.

    // The lateResponseMode assumes that 'onRequestTimeout' is serialized with
    // 'processResponse' and 'cancelAllRequests' (by using 'd_executor').
    // Meaning, the executor is responsible for ignoring timeouts after
    // responses.

    // 2. Invoke the response callback/signal: If a response callback was
    //    provided invoke it now, otherwise signal: normally 'signal()' is
    //    called by the caller from its response callback; however, it is
    //    convenient to be able to write a synchronous call in a single method,
    //    with no response callback provided, so we invoke 'signal()' ourself
    //    now for that matter.

    if (request->d_responseCb) {
        bslma::ManagedPtr<void> spanToken(request->activateDTSpan());
        request->d_responseCb(request);
    }
    else {
        request->signal();
    }
}

void RequestManager::applyResponse(
    const RequestSp&                    request,
    const bmqp_ctrlmsg::ControlMessage& response)
{
    // mutex *NOT* locked

    // Cancel the timeout event
    d_scheduler_p->cancelEvent(&(request->d_timeoutSchedulerHandle));

    // Populate response field.

    // The lateResponseMode assumes that 'onRequestTimeout' is serialized with
    // 'processResponse' and 'cancelAllRequests' (by using 'd_executor').
    // Meaning, the response field is (re)set and accessed in the same thread.

    request->response() = response;

    if (request->response().rId().isNull()) {
        // The 'id' field of 'response' is not populated (can happen when we
        // cancelAllRequests by injecting one response object) so we populate
        // it using the 'id' of the original request.
        BSLS_ASSERT_OPT(!request->request().rId().isNull());
        request->response().rId().makeValue(request->request().rId().value());
    }

    // Convert timeout code: If an incoming Status response has a timeout
    // category, then set the code to the remote timeout code
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            response.choice().isStatusValue() &&
            (response.choice().status().category() ==
             bmqp_ctrlmsg::StatusCategory::E_TIMEOUT))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        request->response().choice().status().code() = k_CODE_TIMEOUT_REMOTE;
    }

    // Invoke the response callback/signal: If a response callback was provided
    // invoke it now, otherwise signal: normally 'signal()' is called by the
    // caller from its response callback; however, it is convenient to be able
    // to write a synchronous call in a single method, with no response
    // callback provided, so we invoke 'signal()' ourself now for that matter.
    if (request->d_responseCb) {
        bslma::ManagedPtr<void> spanToken(request->activateDTSpan());
        request->d_responseCb(request);
    }
    else {
        request->signal();
    }
}

RequestManager::RequestManager(bmqp::EventType::Enum  eventType,
                               BlobSpPool*            blobSpPool_p,
                               bdlmt::EventScheduler* scheduler,
                               bool                   lateResponseMode,
                               bslma::Allocator*      allocator)
: d_allocator_p(allocator)
, d_eventType(eventType)
, d_scheduler_p(scheduler)
, d_nextRequestId(0)
, d_requests(allocator)
, d_schemaEventBuilder(blobSpPool_p, bmqp::EncodingType::e_BER, allocator)
, d_lateResponseMode(lateResponseMode)
, d_executor(bmqex::SystemExecutor())  // Use SystemExecutor so that when using
                                       // 'possiblyBlocking' it will inline
                                       // invoke the function in the callers
                                       // thread instead of spawning a new
                                       // thread every time.
, d_dtContext_sp(NULL)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_scheduler_p->clockType() ==
                     bsls::SystemClockType::e_MONOTONIC);
}

RequestManager::RequestManager(bmqp::EventType::Enum  eventType,
                               BlobSpPool*            blobSpPool_p,
                               bdlmt::EventScheduler* scheduler,
                               bool                   lateResponseMode,
                               const bmqex::Executor& executor,
                               bslma::Allocator*      allocator)
: d_allocator_p(allocator)
, d_eventType(eventType)
, d_scheduler_p(scheduler)
, d_nextRequestId(0)
, d_requests(allocator)
, d_schemaEventBuilder(blobSpPool_p, bmqp::EncodingType::e_BER, allocator)
, d_lateResponseMode(lateResponseMode)
, d_executor(executor)
, d_dtContext_sp(NULL)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_scheduler_p->clockType() ==
                     bsls::SystemClockType::e_MONOTONIC);
    BSLS_ASSERT_SAFE(executor);
}

RequestManager::RequestManager(bmqp::EventType::Enum  eventType,
                               BlobSpPool*            blobSpPool_p,
                               bdlmt::EventScheduler* scheduler,
                               bool                   lateResponseMode,
                               const bmqex::Executor& executor,
                               const DTContextSp&     dtContext,
                               bslma::Allocator*      allocator)
: d_allocator_p(allocator)
, d_eventType(eventType)
, d_scheduler_p(scheduler)
, d_nextRequestId(0)
, d_requests(allocator)
, d_schemaEventBuilder(blobSpPool_p, bmqp::EncodingType::e_BER, allocator)
, d_lateResponseMode(lateResponseMode)
, d_executor(executor)
, d_dtContext_sp(dtContext)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_scheduler_p->clockType() ==
                     bsls::SystemClockType::e_MONOTONIC);
    BSLS_ASSERT_SAFE(executor);
}

RequestManager::~RequestManager()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_requests.empty() &&
                     "There are still outstanding requests, "
                     "'cancelAllRequests()' must be called before destroying "
                     "this object");
}

RequestManager& RequestManager::setExecutor(const bmqex::Executor& executor)
{
    d_executor = executor;
    return *this;
}

RequestManager::RequestSp RequestManager::createRequest()
{
    RequestSp request;
    request.createInplace(d_allocator_p, d_allocator_p);
    request->d_self_wp =
        request;  // Give request a 'weak_ptr' to itself so it
                  // can get back a 'shared_ptr' in 'signal()'.
    if (d_dtContext_sp) {
        request->setDTContext(d_dtContext_sp.get());
    }

    return request;
}

bmqt::GenericResult::Enum
RequestManager::sendRequest(const RequestSp&          request,
                            bmqio::Channel*           channel,
                            const bsl::string&        nodeDescription,
                            const bsls::TimeInterval& timeout,
                            bsls::Types::Int64        watermark,
                            bsl::string*              errorDescription)
{
    return sendRequest(request,
                       bdlf::BindUtil::bind(&sendHelper,
                                            channel,
                                            bdlf::PlaceHolders::_1,  // blob
                                            watermark),  // watermark
                       nodeDescription,
                       timeout,
                       errorDescription);
}

bmqt::GenericResult::Enum
RequestManager::sendRequest(const RequestSp&          request,
                            const SendFn&             sendFn,
                            const bsl::string&        nodeDescription,
                            const bsls::TimeInterval& timeout,
                            bsl::string*              errorDescription)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // MUTEX LOCKED

    // Inject the requestId in the request
    const int requestId = generateRequestId();
    request->request().rId().makeValue(requestId);

    request->d_nodeDescription = nodeDescription;

    // Prepare the message to send
    d_schemaEventBuilder.reset();
    int rc = d_schemaEventBuilder.setMessage(request->request(), d_eventType);
    if (rc != 0) {
        bmqu::MemOutStream errorDesc;
        errorDesc << "ENCODING_FAILED, rc: " << rc;
        if (errorDescription) {
            *errorDescription = errorDesc.str();
        }
        else {
            BALL_LOG_ERROR << "Unable to send request to '" << nodeDescription
                           << "' [reason: " << errorDesc.str()
                           << "]: " << request->request();
        }
        return bmqt::GenericResult::e_INVALID_ARGUMENT;  // RETURN
    }

    BALL_LOG_INFO << "Sending request to '" << nodeDescription << "' "
                  << "[request: " << request->request()
                  << ", timeout: " << timeout << "]";

    // We are under the 'd_mutex' lock, so we have guarantee that even if the
    // response comes back before we added the request to the map, it won't be
    // processed until we return from this method, meaning we registered it to
    // the map.

    // Send the request
    request->d_sendTime              = bmqu::Time::highResolutionTimer();
    bmqt::GenericResult::Enum sendRc = sendFn(d_schemaEventBuilder.blob());
    if (sendRc != bmqt::GenericResult::e_SUCCESS) {
        bmqu::MemOutStream errorDesc;
        errorDesc << "WRITE_FAILED, status: " << sendRc;
        if (errorDescription) {
            *errorDescription = errorDesc.str();
        }
        else {
            BALL_LOG_ERROR << "Unable to send request to '" << nodeDescription
                           << "' [reason: " << errorDesc.str()
                           << "]: " << request->request();
        }
        return sendRc;  // RETURN
    }

    // Schedule a timeout
    struct Local {
        static void dispatch(bmqex::Executor*             executor,
                             const bsl::function<void()>& callback)
        {
            executor->dispatch(callback);
        }
    };

    d_scheduler_p->scheduleEvent(
        &(request->d_timeoutSchedulerHandle),
        bmqu::Time::nowMonotonicClock() + timeout,
        bdlf::BindUtil::bind(&Local::dispatch,
                             &d_executor,
                             bsl::function<void()>(bdlf::BindUtil::bind(
                                 &RequestManager::onRequestTimeout,
                                 this,
                                 requestId))));

    // Insert the request in the map
    BSLA_MAYBE_UNUSED bsl::pair<RequestMapIter, bool> insertRC =
        d_requests.insert(bsl::make_pair(requestId, request));
    BSLS_ASSERT_OPT(insertRC.second);

    return bmqt::GenericResult::e_SUCCESS;
}

int RequestManager::processResponse(
    const bmqp_ctrlmsg::ControlMessage& response)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS   = 0,
        rc_NOT_FOUND = -1
    };

    if (response.rId().isNull()) {
        // The response carries no correlator, so it cannot be matched to an
        // outstanding request.
        BALL_LOG_DEBUG << "Received a response without a request id"
                       << ", dropping it";
        return rc_NOT_FOUND;  // RETURN
    }

    RequestSp request;

    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // MUTEX LOCKED

        RequestMapIter it = d_requests.find(response.rId().value());

        if (it == d_requests.end()) {
            // The request must have completed at the same time from a
            // different thread while we were waiting on the 'd_mutex'.
            BALL_LOG_DEBUG << "Received a response for a non existent request"
                           << ", dropping it";
            return rc_NOT_FOUND;  // RETURN
        }

        request = it->second;
        d_requests.erase(it);

        if (request->d_haveTimeout && !d_lateResponseMode) {
            // Ignore late response
            BALL_LOG_DEBUG << "Ignoring late response: " << response;
            return rc_NOT_FOUND;  // RETURN
        }

        request->d_haveResponse = true;
    }

    applyResponse(request, response);

    return rc_SUCCESS;
}

void RequestManager::cancelAllRequests(
    const bmqp_ctrlmsg::ControlMessage& reason)
{
    cancelAllRequestsImpl(reason,
                          RequestType::k_NO_GROUP_ID,
                          RequestManagerComponentId::k_NO_COMPONENT_ID);
}

void RequestManager::cancelGroupRequests(
    const bmqp_ctrlmsg::ControlMessage& reason,
    int                                 groupId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(groupId != RequestType::k_NO_GROUP_ID);

    cancelAllRequestsImpl(reason,
                          groupId,
                          RequestManagerComponentId::k_NO_COMPONENT_ID);
}

void RequestManager::cancelComponentRequests(
    const bmqp_ctrlmsg::ControlMessage& reason,
    int                                 componentId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(componentId !=
                     RequestManagerComponentId::k_NO_COMPONENT_ID);

    cancelAllRequestsImpl(reason, RequestType::k_NO_GROUP_ID, componentId);
}

void RequestManager::cancelAllRequestsImpl(
    const bmqp_ctrlmsg::ControlMessage& reason,
    int                                 groupId,
    int                                 componentId)
{
    // Note that requests must be cancelled in the same order in which they
    // were sent.

    // Create a new map so we can work on it outside the mutex
    RequestMap requestsCopy(d_requests.bucket_count(),
                            d_requests.get_allocator());

    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // MUTEX LOCKED

        RequestMapIter it = d_requests.begin();
        while (it != d_requests.end()) {
            const bool matchGroup = (groupId == RequestType::k_NO_GROUP_ID) ||
                                    (groupId == it->second->groupId());
            const bool matchComponent =
                (componentId ==
                 RequestManagerComponentId::k_NO_COMPONENT_ID) ||
                (componentId == it->second->componentId());

            if (matchGroup && matchComponent) {
                // Do not notify about timed out requests.
                if (!it->second->d_haveTimeout) {
                    BSLA_MAYBE_UNUSED bsl::pair<RequestMapIter, bool>
                                      insertRC = requestsCopy.insert(
                            bsl::make_pair(it->first, it->second));
                    BSLS_ASSERT_SAFE(insertRC.second);
                }
                d_requests.erase(it++);
            }
            else {
                ++it;
            }
        }
    }

    const bool hasGroup     = (groupId != RequestType::k_NO_GROUP_ID);
    const bool hasComponent = (componentId !=
                               RequestManagerComponentId::k_NO_COMPONENT_ID);
    if (!hasGroup && !hasComponent) {
        BALL_LOG_INFO << "Canceling all requests (" << requestsCopy.size()
                      << " items) with " << reason << ".";
    }
    else if (hasGroup && !hasComponent) {
        BALL_LOG_INFO << "Canceling requests belonging to group '" << groupId
                      << "' (" << requestsCopy.size() << " items) with "
                      << reason << ".";
    }
    else if (!hasGroup && hasComponent) {
        BALL_LOG_INFO << "Canceling requests belonging to component '"
                      << componentId << "' (" << requestsCopy.size()
                      << " items) with " << reason << ".";
    }
    else {
        BALL_LOG_INFO
            << "Canceling requests simultaneously belonging to group '"
            << groupId << "' and component '" << componentId << "' ("
            << requestsCopy.size() << " items) with " << reason << ".";
    }

    if (requestsCopy.empty()) {
        return;  // RETURN
    }

    for (RequestMapConstIter it = requestsCopy.begin();
         it != requestsCopy.end();
         ++it) {
        applyResponse(it->second, reason);
    }
}

}  // close package namespace
}  // close enterprise namespace
