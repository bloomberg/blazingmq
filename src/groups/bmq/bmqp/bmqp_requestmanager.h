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

// bmqp_requestmanager.h                                              -*-C++-*-
#ifndef INCLUDED_BMQP_REQUESTMANAGER
#define INCLUDED_BMQP_REQUESTMANAGER

//@PURPOSE: Provide a mechanism to manipulate requests and their response.
//
//@CLASSES:
//  bmqp::RequestManagerRequest: request and its associated context
//  bmqp::RequestManager:        mechanism to manage requests and responses
//
//@DESCRIPTION: 'bmqp::RequestManager' is a mechanism to manage requests (using
// the 'bmqp::RequestManagerRequest' type) and their associated responses.
// This component takes care of encoding requests (using the
// 'bmqp::SchemaEventBuilder' component) and sending them over a provided
// 'bmqio::Channel' (or using a raw 'sendFn' method, suitable for test drivers
// implementation).  When a response is received, it can be injected to the
// 'RequestManager' (via the 'processResponse()' method), which will invoke the
// appropriate callback and wake up any eventual waiter on that request.  This
// component supports synchronous as well as asynchronous response processing,
// as well as response timeout.  Response processing is done through two steps:
// when the response is received and injected into the 'RequestManager', the
// response callback associated to the corresponding request will be invoked;
// and the application is responsible for calling 'signal()' on the Request in
// order to wake up and/or notify anyone waiting on a response.
//
/// Threading model and executor
///----------------------------
// Requests' response callback and async notifier callback can be invoked from
// three different places:
//: o when the user calls 'RequestManager::processResponse()',
//: o when the user calls 'RequestManager::cancelAllRequests()',
//: o when a request times out
// While the user can control the context surrounding an invocation of the
// first two methods, it has no control over the later one, which originates
// from the scheduler thread.  Therefore, in order to provide better
// application execution control to the user, RequestManager constructor takes
// an optional executor.  If such an executor is provided, it will be used to
// invoke the timeout response of a request; if no executor is provided, the
// 'SystemExector' will be used by default and invoke the response processing
// inline from within the scheduler thread.
//
/// Late response mode
///------------------
// A late response is a response received after the request has been locally
// timed out.  The 'RequestManager' can be configured (per a constructor
// boolean flag) to either ignore or process such responses.  When
// 'lateResponseMode' is true, a response passed through the 'processResponse'
// method will be processed and invoke its associated response callback even it
// the request has already been timed out, and therefore the callback of such a
// request will be invoked twice (once for the local timeout, and once when
// either the response is received, or the request is cancelled).

//
/// Thread Safety
///-------------
// The 'bmqp::RequestManager' and 'bmqp::RequestManagerRequest' classes are
// fully thread-safe (see 'bsldoc_glossary'), meaning that two threads can
// safely call any methods on the *same* *instance* without external
// synchronization, and re-entrant safe, meaning that a method of this object
// can be called from within a callback emanating from that same object
// instance.
//
/// Request Cancellation Order
///--------------------------
// Outstanding requests can be cancelled by invoking 'cancelAllRequests'.  The
// order in which response callbacks of the cancelled requests will be invoked
// is the order in which requests were original sent e.g., if request 'A' was
// sent followed by request 'B', and while both requests are outstanding,
// 'cancelAllRequests' is invoked, response callback of request 'A' will be
// invoked first, followed by response callback of request 'B'.  This guarantee
// is provided because application may rely on the assumption that response
// callbacks are invoked in the order of sending requests.  There are two
// versions of 'cancelAllRequests': the one that does take 'groupId' and the
// one that does not.  Without 'groupId', all requests get cancelled, otherwise
// only those requests which were associated with the same id by 'setGroupId'.
//
/// Distributed Trace Integration
///-----------------------------
// An externally created 'bmqpi::DTSpan' may be attached to a request using the
// 'RequestManagerRequest::setDTSpan' method. This ensures that a span
// representative of a request survives for at least the lifetime of the
// request object.
//
// A 'RequestManagerRequest' which owns a 'DTSpan' can additionally be provided
// with a 'bmqpi::DTContext' via the 'RequestManagerRequest::setDTContext'
// method. This guarantees that the 'DTSpan' will be the active span of the
// 'DTContext' whenever a response- or signal- callback of the request is
// invoked. The easiest (and recommended) way to attach a 'DTContext' is by
// using the 'RequestManager::setDTContext' method on the object used to create
// new 'RequestManagerRequest' instances; the context will be propagated to any
// new requests created. Note that setting a 'DTContext' without a 'DTSpan'
// will have no effect.
//
/// Usage Example (basic)
///---------------------
// This example shows basic usage of the 'RequestManager' object.
//
// First, let's create a 'RequestManager' object:
//..
//  bslma::Allocator              allocator = bslma::Default::allocator();
//  bdlbb::PooledBlobBufferFactory blobBufferFactory(4069, allocator);
//  bdlmt::EventScheduler         scheduler(bsls::SystemClockType::e_MONOTONIC,
//                                          allocator);
//
//  typedef bmqp::RequestManager<bmqp_ctrlmsg::ControlMessage,
//                               bmqp_ctrlmsg::ControlMessage>
//                                                          RequestManagerType;
// RequestManagerType requestManager(bmqp::EventType::e_CONTROL,
//                                   &blobBufferFactory,
//                                   &scheduler,
//                                   false,  // late response mode
//                                   &allocator);
//..
//
// Then we can use it to create and send a request.  (here we assume that we
// have an already established and valid 'bmqio::Channel' object to use)
//..
//  // We first ask a Request object to the RequestManager
//  RequestManagerType::RequestSp request = requestManager.createRequest();
//
//  // Populate the request
//  bmqp_ctrlmsg::OpenQueue& req = request->request().choice().makeOpenQueue();
//  req.uri() = "bmq://bmq.test.mem.priority/myQueue"
//  [...]
//
//  // Now set the response callback to be invoked
//  request->setResponseCb(bdlf::BindUtil::bind(&MyClass::onOpenQueueResponse,
//                                              this,
//                                              bdlf::PlaceHolders::_1));
//
//  // Now set the async notifier callback that will be invoked when signaling
//  // on the request
//  request->setAsyncNotifierCb(bdlf::BindUtil::bind(&MyClass::enqueueEvent,
//                                                   this,
//                                                   bdlf::PlaceHolders::_1));
//
//  // Finally, we can send the request
//  bmqio::StatusCategory::Enum rc = requestManager.sendRequest(
//                                                      request,
//                                                      channel,
//                                                      "bmqMachine123",
//                                                      bsls::TimeInterval(30),
//                                                      64 * 1024 * 1024);
//  if (rc != bmqio::StatusCategory::e_SUCCESS) {
//      // Request failed to encode/be sent; process error handling (note that
//      // neither the 'responseCb' nor the 'asyncNotifierCb' will ever get
//      // invoked in this case).
//  }
//
//  // Request was successfully sent, we can either wait for an answer with
//  // 'request->wait();'
//  // or return.
//..
//
// From the IO channel, where the request comes in, we simply forward it to the
// RequestManager:
//..
//  // controlMessage is a 'bmqp_ctrlmsg::ControlMessage' that was received and
//  // decoded.. out of interest, its should 'id()' field should contain the
//  // same id that was used when sending the request (it is used as the
//  // correlator).
//  requestManager.processResponse(controlMessage);
//      // Note that if a response callback was configured, it will be invoked
//      // from this call to 'processResponse()', so the caller need to be
//      // careful with regards to mutex.
//..
//
// When 'processResponse' is called, the registered response callback will be
// invoked, let's look at a typical implementation of such a method:
//..
//  void
//  MyClass::onOpenQueueResponse(const RequestManagerType::RequestSp& context)
//  {
//    if (context->result() != bmqt::GenericResult::e_SUCCESS) {
//        // Request failed/timedout/got canceled
//        // Do any kind of cleanup, processing, ...
//        context->signal(); // Notify the waiters and invoke the asyncNotifier
//        return;                                                     // RETURN
//    }
//    // Request was success, process the response
//    // Do something with request->response();
//    context->signal(); // Notify waiters
//    // Do some more processing if needed
//  }
//..
//
/// Usage Example (synchronous with response callback)
///--------------------------------------------------
// This example illustrates how to typically use this object to synchronously
// handle a request with a response callback.
//
//..
//  RequestManagerType::RequestSp request = requestManager.createRequest();
//  bmqp_ctrlmsg::OpenQueue& req = request->request().choice().makeOpenQueue();
//  req.uri() = "bmq://bmq.test.mem.priority/myQueue"
//  [...]
//
//  // Now set the response callback to be invoked
//  request->setResponseCb(bdlf::BindUtil::bind(&MyClass::onOpenQueueResponse,
//                                              this,
//                                              bdlf::PlaceHolders::_1));
//
//  // For synchronous request, we don't need to specify an 'asyncNotifierCb'
//
//  // Finally, we can send the request
//  bmqio::StatusCategory::Enum rc = requestManager.sendRequest(
//                                                      request,
//                                                      channel,
//                                                      "bmqMachine123",
//                                                      bsls::TimeInterval(30),
//                                                      64 * 1024 * 1024);
//  if (rc != bmqio::StatusCategory::e_SUCCESS) {
//      // Request failed to encode/be sent; process error handling (note that
//      // neither the 'responseCb' nor the 'asyncNotifierCb' will ever get
//      // invoked in this case).
//  }
//
//  // Request was successfully sent, wait for a response
//  request->wait();
//
//  // Do more work.. from this point, the response callback
//  // (onOpenQueueResponse) has been invoked
//..
//
// In the response callback, 'context->signal()' is what will wake up the
// caller's thread above that is waiting in the 'request->wait()' call.
//
//
/// Usage Example (synchronous without response callback)
///-----------------------------------------------------
// If we don't want to use a separate function for processing the response, we
// can implement both the request and the response processing part in the same
// method, by not providing a response callback, as illustrated by the
// following example:
//
//..
//  RequestManagerType::RequestSp request = requestManager.createRequest();
//  bmqp_ctrlmsg::OpenQueue& req = request->request().choice().makeOpenQueue();
//  req.uri() = "bmq://bmq.test.mem.priority/myQueue"
//  [...]
//
//  // We don't set a response callback, which means that the RequestManager
//  // will itself invoke 'signal' when a response is received.
//  // We also don't need to specify any 'asyncNotifierCb'
//
//  // Send the request
//  bmqio::StatusCategory::Enum rc = requestManager.sendRequest(
//                                                      request,
//                                                      channel,
//                                                      "bmqMachine123",
//                                                      bsls::TimeInterval(30),
//                                                      64 * 1024 * 1024);
//  if (rc != bmqio::StatusCategory::e_SUCCESS) {
//      // Request failed to encode/be sent; process error handling (note that
//      // neither the 'responseCb' nor the 'asyncNotifierCb' will ever get
//      // invoked in this case).
//  }
//
//  // Request was successfully sent, wait for a response
//  request->wait();
//
//  if (request->result() != bmqt::GenericResult::e_SUCCESS) {
//        // Request failed/timedout/got canceled
//        // Do any kind of cleanup, processing, ...
//        return;                                                     // RETURN
//  }
//
//  // Request was success, process the response
//  // Do something with request->response();
//..
//
/// Usage Example (asynchronous)
///----------------------------
// This example illustrates how to use the RequestManager to send a Request,
// asynchronously processing its response:
//
//..
//  RequestManagerType::RequestSp request = requestManager.createRequest();
//  bmqp_ctrlmsg::OpenQueue& req = request->request().choice().makeOpenQueue();
//  req.uri() = "bmq://bmq.test.mem.priority/myQueue"
//  [...]
//
//  // We can set either the response callback or the async notifier callback
//  request->setResponseCb(bdlf::BindUtil::bind(&MyClass::onOpenQueueResponse,
//                                              this,
//                                              bdlf::PlaceHolders::_1));
//
//  // Finally, we can send the request
//  bmqio::StatusCategory::Enum rc = requestManager.sendRequest(
//                                                      request,
//                                                      channel,
//                                                      "bmqMachine123",
//                                                      bsls::TimeInterval(30),
//                                                      64 * 1024 * 1024);
//  if (rc != bmqio::StatusCategory::e_SUCCESS) {
//      // Request failed to encode/be sent; process error handling (note that
//      // neither the 'responseCb' nor the 'asyncNotifierCb' will ever get
//      // invoked in this case).
//  }
//
//  // The request is async, so we do not call 'request->wait()'
//..
//
// Whenever the response comes in, or the request gets canceled or times out,
// the response callback will be invoked; and there is no need to call
// 'signal()' from there.

// BMQ

#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqp_schemaeventbuilder.h>
#include <bmqpi_dtcontext.h>
#include <bmqpi_dtspan.h>
#include <bmqt_resultcode.h>

#include <bmqc_orderedhashmap.h>
#include <bmqex_bindutil.h>
#include <bmqex_executionpolicy.h>
#include <bmqex_executor.h>
#include <bmqex_systemexecutor.h>
#include <bmqio_channel.h>
#include <bmqio_status.h>
#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdld_manageddatum.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_utility.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_allocatorargt.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bslmt_semaphore.h>
#include <bsls_assert.h>
#include <bsls_cpp11.h>
#include <bsls_performancehint.h>
#include <bsls_systemclocktype.h>
#include <bsls_timeinterval.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace bmqp {

// FORWARD DECLARATION
template <class REQUEST, class RESPONSE>
class RequestManager;

// ===========================
// class RequestManagerRequest
// ===========================

/// Object representing a request sent, pending response for it; holding the
/// request and all associated context state.  This allows for both
/// synchronous and asynchronous response management.
template <class REQUEST, class RESPONSE>
class RequestManagerRequest {
  public:
    // PUBLIC CLASS DATA
    static const int k_NO_GROUP_ID = -1;

    // TYPES

    /// SelfType is an alias to this `class`.
    typedef RequestManagerRequest<REQUEST, RESPONSE> SelfType;

    typedef bsl::shared_ptr<SelfType> SelfTypeSp;

    /// Shared and weak pointer to self type alias
    typedef bsl::weak_ptr<SelfType> SelfTypeWp;

    /// Signature of a callback for delivering the response in the
    /// specified `context`.
    typedef bsl::function<void(const SelfTypeSp& context)> ResponseCb;

    /// Signature of a callback for signaling a response in the specified
    /// `context`.
    typedef bsl::function<void(const SelfTypeSp& context)> AsyncNotifierCb;

  private:
    // DATA
    SelfTypeWp d_self_wp;
    // Weak pointer to self

    REQUEST d_requestMessage;
    // The request

    RESPONSE d_responseMessage;
    // The response

    bslmt::Semaphore d_semaphore;
    // Semaphore associated to this request,
    // used for synchronous calls wait.

    bdlmt::EventScheduler::EventHandle d_timeoutSchedulerHandle;
    // Scheduler handle for the timeout
    // associated to this request.

    ResponseCb d_responseCb;
    // Response callback, if any, to invoke
    // upon reception of a response for this
    // request.

    AsyncNotifierCb d_asyncNotifierCb;
    // Callback invoked when calling signal,
    // if it exists.

    bsls::Types::Int64 d_sendTime;
    // Time when the request was sent, used
    // for statistics/logging.

    bsl::string d_nodeDescription;
    // Description of the node the request
    // was sent to.

    bool d_haveTimeout;
    // Whether the timeout for the request
    // has been invoked.

    bool d_haveResponse;
    // Whether a response for teh request has
    // been received.

    int d_groupId;
    // The 'groupId' associated with this
    // request. It is used when canceling
    // requests, allowing to cancel all
    // requests sharing the same group.

    bsl::shared_ptr<bmqpi::DTSpan> d_dtSpan_sp;
    // Distributed Trace span representing
    // this request.

    bmqpi::DTContext* d_dtContext_p;
    // A Distributed Trace context which will
    // take 'd_dtSpan_sp' as the active span
    // for the duration of any callbacks
    // executed on behalf of this request. If
    // either this or 'd_dtSpan_sp' are null,
    // then 'd_dtSpan_sp' will not be the
    // active span for any context.

    bdld::ManagedDatum d_userData;
    // Optional userData associated with this
    // request.

    // FRIENDS
    friend class RequestManager<REQUEST, RESPONSE>;

  private:
    // NOT IMPLEMENTED
    RequestManagerRequest(const RequestManagerRequest&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    RequestManagerRequest&
    operator=(const RequestManagerRequest&) BSLS_CPP11_DELETED;

    /// If a Distributed Trace span and context have been set for this
    /// request, then this sets the span as the active one within the
    /// context, returning a `token` that will revert the context's state
    /// upon its destruction. If either no span has been set for this
    /// request, or no context has been set for the span to be made
    /// active within, then an empty `ManagedPtr` is returned.
    bslma::ManagedPtr<void> activateDTSpan() const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RequestManagerRequest,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object using the specified `allocator`
    explicit RequestManagerRequest(bslma::Allocator* allocator);

    /// Destroy this object
    ~RequestManagerRequest();

    // MANIPULATORS

    /// Clear this object to a default constructed state.
    void clear();

    /// Wakes up any thread that was waiting on this request (from a call to
    /// `wait()`) by positing to the semaphore.  This will also invoke the
    /// `asyncNotifierCb` if one was provided.
    void signal();

    /// Block until a response for this request is received (or until the
    /// request times out or gets canceled).
    void wait();

    REQUEST& request();

    /// Return a reference offering modifiable access to the corresponding
    /// member of this object.
    RESPONSE& response();

    RequestManagerRequest& setResponseCb(const ResponseCb& value);

    /// Set the corresponding member to the specified `value` and return a
    /// reference offering modifiable access to this object.
    RequestManagerRequest& setAsyncNotifierCb(const AsyncNotifierCb& value);

    /// Set group identifier to the specified value to assist canceling
    /// requests belonging to one group only.  By default, the identifier is
    /// `k_NO_GROUP_ID` meaning the request does not belong to any group in
    /// which case it gets cancelled by `cancelAllRequests` without
    /// `groupId`.  Return a reference offering modifiable access to this
    /// object.
    RequestManagerRequest& setGroupId(int value);

    /// Take shared ownership of the specified `span` and ensure that it
    /// lives at least as long as this object. The `span` is intended to
    /// represent this request.
    ///
    /// If there is a Distributed Trace context (set via `setDTContext`),
    /// then `span` will be made its active span for the duration of any
    /// callbacks executed on behalf of this request.
    RequestManagerRequest&
    setDTSpan(const bsl::shared_ptr<bmqpi::DTSpan>& span);

    /// Stores an unowned pointer to `context`: If a span has been set (via
    /// `setDTSpan`), then it will be made the active span of `ctx` for the
    /// duration of any callbacks executed on behalf of this request.
    RequestManagerRequest& setDTContext(bmqpi::DTContext* ctx);

    /// Take ownership of the specified `value` and destroy the user data
    /// previously managed by this object.  The behavior is undefined unless
    /// `value` was allocated using the same allocator used by this object
    /// and is not subsequently destroyed externally using `Datum::destroy`.
    RequestManagerRequest& adoptUserData(const bdld::Datum& value);

    // ACCESSORS
    bool            isLateResponse() const;
    bool            isLocalTimeout() const;
    bool            isError() const;
    const REQUEST&  request() const;
    const RESPONSE& response() const;

    /// Return the value of the corresponding member.
    const ResponseCb& responseCb() const;

    /// Convenient accessor to return the `GenericResult` of the response
    /// associated to this object: return either `success` or the category
    /// code associated to the Failure type of the response.
    bmqt::GenericResult::Enum result() const;

    /// Return the description of the node the request was sent to.
    const bsl::string& nodeDescription() const;

    /// Return the associated group id (`NO_GROUP_ID` by default).
    int groupId() const;

    /// Return the associated user data.
    const bdld::Datum& userData() const;
};

// ====================
// class RequestManager
// ====================

/// Mechanism to manage requests and their response.
template <class REQUEST, class RESPONSE>
class RequestManager {
  public:
    // TYPES
    typedef bmqp::BlobPoolUtil::BlobSpPool BlobSpPool;

    /// Signature of a method to send a request, represented by the
    /// specified `blob`.  Return 0 on success, and a non-zero value
    /// otherwise, populating the optionally specified `status` with
    /// information pertaining to the error.
    typedef bsl::function<bmqt::GenericResult::Enum(
        const bsl::shared_ptr<bdlbb::Blob>& blob)>
        SendFn;

    typedef RequestManagerRequest<REQUEST, RESPONSE> RequestType;

    /// Shortcut to a Request object
    typedef bsl::shared_ptr<RequestType> RequestSp;

    /// Shortcut to shared_ptr<DTContext> for Distributed Trace.
    typedef bsl::shared_ptr<bmqpi::DTContext> DTContextSp;

    // PUBLIC CLASS DATA

    /// Constant representing the code for a LocalTimeout in a Status
    /// response having a timeout StatusCategory.  LocalTimeout are timeout
    /// responses which are generated internally by the request manager when
    /// the request's associated event scheduler fires.
    static const int k_CODE_TIMEOUT_LOCAL = -1;

    /// Constant representing the code for a RemoteTimeout in a Status
    /// response having a timeout StatusCategory.  RemoteTimeout are timeout
    /// responses which are received as regular response from the
    /// remote-peer.
    static const int k_CODE_TIMEOUT_REMOTE = -2;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQP.REQUESTMANAGER");

  private:
    // PRIVATE TYPES

    /// Map of request id to Request object.  In order to provide `in-order`
    /// guarantee in `cancelAllRequests` (see `Request Cancellation Order`
    /// section), it is important to use a container with deterministic
    /// ordering.  We use a map with an incrementing counter as the key, so
    /// the order of insertion of requests is simply the order of traversal
    /// in the map.  Alternatively, we could also use an ordered hash map,
    /// but using an un-ordered map is not an option.
    typedef bmqc::OrderedHashMap<int, RequestSp> RequestMap;

    typedef typename RequestMap::iterator RequestMapIter;

    typedef typename RequestMap::const_iterator RequestMapConstIter;

    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use.

    bmqp::EventType::Enum d_eventType;
    // Type of events to build when sending
    // requests.

    bslmt::Mutex d_mutex;
    // Mutex for synchronization and thread safety
    // of this object

    bdlmt::EventScheduler* d_scheduler_p;
    // Pointer, held not owned, to the scheduler to
    // use for the requests timeout (must be using
    // the 'MONOTONIC' clock type)

    int d_nextRequestId;
    // Id of the next request to use (int not
    // atomicInt since this will always be
    // manipulated under the 'd_mutex' lock)

    RequestMap d_requests;
    // Map of all outstanding requests

    bmqp::SchemaEventBuilder d_schemaEventBuilder;
    // Builder objects for preparing the requests
    // blobs

    const bool d_lateResponseMode;
    // Whether a late response (i.e. a response
    // received after the request has been locally
    // timed out) should still be processed or not.

    bmqex::Executor d_executor;
    // The executor supplying the threading context
    // to use for processing the timeout of a
    // request.

    DTContextSp d_dtContext_sp;
    // A 'bmqpi::DTContext' propagated to any
    // requests created by this object. If those
    // requests have a 'bmqpi::DTSpan' attached, it
    // will be activated within this context
    // whenever a callback for the request is
    // invoked.

  private:
    // PRIVATE MANIPULATORS

    /// Send the specified `blob_sp` over the specified `channel` using the
    /// specified `watermark`.  Return a Generic Result code representing
    /// the status of delivery of this request.
    static bmqt::GenericResult::Enum
    sendHelper(bmqio::Channel*                     channel,
               const bsl::shared_ptr<bdlbb::Blob>& blob_sp,
               bsls::Types::Int64                  watermark);

    /// Callback invoked by the scheduler when the request identified by the
    /// specified `requestId` has timedout.
    void onRequestTimeout(int requestId);

    /// Apply the specified `response` to the specified `request`.
    void applyResponse(const RequestSp& request, const RESPONSE& response);

    /// Cancel all outstanding requests belonging to the specified
    /// `groupId`, with the specified `reason` response description.  If the
    /// `groupId` is `NO_GROUP_ID`, cancel all requests.  The corresponding
    /// response callbacks will be invoked in the order in which requests
    /// were sent.
    void cancelAllRequestsImpl(const RESPONSE& reason, int groupId);

  private:
    // NOT IMPLEMENTED
    RequestManager(const RequestManager&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    RequestManager& operator=(const RequestManager&) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RequestManager, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object using the specified `blobSpPool_p`, `scheduler`
    /// and `executor` and the provided `allocator` for memory allocation.
    /// Events sent will be of the specified `eventType`.  If `executor` is
    /// specified, it will be used around the invocation of the callback
    /// when a request times out.  Note that `scheduler` must be
    /// configured to use the `bsls::SystemClockType::e_MONOTONIC` clock
    /// type.
    RequestManager(bmqp::EventType::Enum  eventType,
                   BlobSpPool*            blobSpPool_p,
                   bdlmt::EventScheduler* scheduler,
                   bool                   lateResponseMode,
                   bslma::Allocator*      allocator = 0);
    RequestManager(bmqp::EventType::Enum  eventType,
                   BlobSpPool*            blobSpPool_p,
                   bdlmt::EventScheduler* scheduler,
                   bool                   lateResponseMode,
                   const bmqex::Executor& executor,
                   bslma::Allocator*      allocator = 0);
    RequestManager(bmqp::EventType::Enum  eventType,
                   BlobSpPool*            blobSpPool_p,
                   bdlmt::EventScheduler* scheduler,
                   bool                   lateResponseMode,
                   const bmqex::Executor& executor,
                   const DTContextSp&     dtContextSp,
                   bslma::Allocator*      allocator = 0);

    /// Destroy this object.
    ~RequestManager();

    // MANIPULATORS

    /// Set this object executor to the specified `executor` (may not be
    /// available at construction time).
    RequestManager& setExecutor(const bmqex::Executor& executor);

    /// Get a new Request object.
    RequestSp createRequest();

    /// Send the specified `request` over the specified `channel` using the
    /// specified `description` and the optionally specified write
    /// `watermark` and schedule a time out of the request after the
    /// specified relative `timeout` time interval.  In case of error,
    /// populate the optionally specified `errorDescription` if not null, or
    /// log the error.  Return a Generic Result code representing the status
    /// of delivery of this request.
    bmqt::GenericResult::Enum
    sendRequest(const RequestSp&          request,
                bmqio::Channel*           channel,
                const bsl::string&        description,
                const bsls::TimeInterval& timeout,
                bsls::Types::Int64        watermark =
                    bsl::numeric_limits<bsls::Types::Int64>::max(),
                bsl::string* errorDescription = 0);

    /// Send the specified `request` by invoking the specified `sendFn`
    /// method with the built blob corresponding to the request.  Use the
    /// specified `description`.  Schedule a time out of the request after
    /// the specified relative `timeout` time interval.  In case of error,
    /// populate the optionally specified `errorDescription` if not null, or
    /// log the error.  Return a Generic Result code representing the status
    /// of delivery of this request.
    bmqt::GenericResult::Enum sendRequest(const RequestSp&   request,
                                          const SendFn&      sendFn,
                                          const bsl::string& description,
                                          const bsls::TimeInterval& timeout,
                                          bsl::string* errorDescription = 0);

    /// Process the specified `response` and return 0 if the response is for
    /// a valid request, or non-zero otherwise (for example if the request
    /// has been removed due to timeout).
    int processResponse(const RESPONSE& response);

    /// Cancel all outstanding requests with the specified `reason` response
    /// description.  The corresponding response callbacks will be invoked
    /// in the order in which requests were sent.
    void cancelAllRequests(const RESPONSE& reason);

    /// Cancel all outstanding requests belonging to the specified
    /// `groupId`, with the specified `reason` response description.  The
    /// behavior is undefined if `groupId` is `NO_GROUP_ID`.  The
    /// corresponding response callbacks will be invoked in the order in
    /// which requests were sent.
    void cancelAllRequests(const RESPONSE& reason, int groupId);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------
// class RequestManagerRequest
// ---------------------------

template <class REQUEST, class RESPONSE>
RequestManagerRequest<REQUEST, RESPONSE>::RequestManagerRequest(
    bslma::Allocator* allocator)
: d_requestMessage(allocator)
, d_responseMessage(allocator)
, d_semaphore()
, d_timeoutSchedulerHandle()
, d_responseCb(bsl::allocator_arg, allocator)
, d_asyncNotifierCb(bsl::allocator_arg, allocator)
, d_sendTime(0)
, d_nodeDescription(allocator)
, d_haveTimeout(false)
, d_haveResponse(false)
, d_groupId(k_NO_GROUP_ID)
, d_dtSpan_sp(NULL)
, d_dtContext_p(NULL)
, d_userData(allocator)
{
    // NOTHING
}

template <class REQUEST, class RESPONSE>
RequestManagerRequest<REQUEST, RESPONSE>::~RequestManagerRequest()
{
    clear();
}

template <class REQUEST, class RESPONSE>
void RequestManagerRequest<REQUEST, RESPONSE>::clear()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_timeoutSchedulerHandle);

    // Reset the semaphore
    while (d_semaphore.tryWait() == 0) {
        // nothing
    }

    // d_self_wp: Purposely not reset-ed :)
    d_requestMessage.reset();
    d_responseMessage.reset();
    d_responseCb      = bsl::nullptr_t();
    d_asyncNotifierCb = bsl::nullptr_t();
    d_sendTime        = 0;
    d_nodeDescription.clear();
    d_dtSpan_sp.reset();
    d_dtContext_p = NULL;
}

template <class REQUEST, class RESPONSE>
void RequestManagerRequest<REQUEST, RESPONSE>::signal()
{
    if (d_asyncNotifierCb) {
        SelfTypeSp context = d_self_wp.lock();
        BSLS_ASSERT_SAFE(context);

        bslma::ManagedPtr<void> spanToken(context->activateDTSpan());
        d_asyncNotifierCb(context);
    }

    d_semaphore.post();
}

template <class REQUEST, class RESPONSE>
void RequestManagerRequest<REQUEST, RESPONSE>::wait()
{
    // No need to timedWait on a timedSemaphore, 'sendRequest()' schedules an
    // event for timeout that will post on the semaphore.
    d_semaphore.wait();
}

template <class REQUEST, class RESPONSE>
REQUEST& RequestManagerRequest<REQUEST, RESPONSE>::request()
{
    return d_requestMessage;
}

template <class REQUEST, class RESPONSE>
RESPONSE& RequestManagerRequest<REQUEST, RESPONSE>::response()
{
    return d_responseMessage;
}

template <class REQUEST, class RESPONSE>
RequestManagerRequest<REQUEST, RESPONSE>&
RequestManagerRequest<REQUEST, RESPONSE>::setResponseCb(
    const ResponseCb& value)
{
    d_responseCb = value;
    return *this;
}

template <class REQUEST, class RESPONSE>
RequestManagerRequest<REQUEST, RESPONSE>&
RequestManagerRequest<REQUEST, RESPONSE>::setAsyncNotifierCb(
    const AsyncNotifierCb& value)
{
    d_asyncNotifierCb = value;
    return *this;
}

template <class REQUEST, class RESPONSE>
RequestManagerRequest<REQUEST, RESPONSE>&
RequestManagerRequest<REQUEST, RESPONSE>::setGroupId(int value)
{
    d_groupId = value;
    return *this;
}

template <class REQUEST, class RESPONSE>
RequestManagerRequest<REQUEST, RESPONSE>&
RequestManagerRequest<REQUEST, RESPONSE>::setDTSpan(
    const bsl::shared_ptr<bmqpi::DTSpan>& span)
{
    d_dtSpan_sp = span;
    return *this;
}

template <class REQUEST, class RESPONSE>
RequestManagerRequest<REQUEST, RESPONSE>&
RequestManagerRequest<REQUEST, RESPONSE>::setDTContext(bmqpi::DTContext* ctx)
{
    d_dtContext_p = ctx;
    return *this;
}

template <class REQUEST, class RESPONSE>
RequestManagerRequest<REQUEST, RESPONSE>&
RequestManagerRequest<REQUEST, RESPONSE>::adoptUserData(
    const bdld::Datum& value)
{
    d_userData.adopt(value);
    return *this;
}

template <class REQUEST, class RESPONSE>
bslma::ManagedPtr<void>
RequestManagerRequest<REQUEST, RESPONSE>::activateDTSpan() const
{
    bslma::ManagedPtr<void> result;
    if (d_dtSpan_sp && d_dtContext_p) {
        result = d_dtContext_p->scope(d_dtSpan_sp);
    }
    return result;
}

template <class REQUEST, class RESPONSE>
bool RequestManagerRequest<REQUEST, RESPONSE>::isLateResponse() const
{
    return d_haveTimeout && d_haveResponse;
}

template <class REQUEST, class RESPONSE>
bool RequestManagerRequest<REQUEST, RESPONSE>::isLocalTimeout() const
{
    return d_haveTimeout && !d_haveResponse;
}

template <class REQUEST, class RESPONSE>
bool RequestManagerRequest<REQUEST, RESPONSE>::isError() const
{
    return isLocalTimeout() ? true : response().choice().isStatusValue();
}

template <class REQUEST, class RESPONSE>
const REQUEST& RequestManagerRequest<REQUEST, RESPONSE>::request() const
{
    return d_requestMessage;
}

template <class REQUEST, class RESPONSE>
const RESPONSE& RequestManagerRequest<REQUEST, RESPONSE>::response() const
{
    return d_responseMessage;
}

template <class REQUEST, class RESPONSE>
const typename RequestManagerRequest<REQUEST, RESPONSE>::ResponseCb&
RequestManagerRequest<REQUEST, RESPONSE>::responseCb() const
{
    return d_responseCb;
}

template <class REQUEST, class RESPONSE>
bmqt::GenericResult::Enum
RequestManagerRequest<REQUEST, RESPONSE>::result() const
{
    if (d_responseMessage.choice().isStatusValue()) {
        return static_cast<bmqt::GenericResult::Enum>(
            d_responseMessage.choice().status().category());  // RETURN
    }

    return bmqt::GenericResult::e_SUCCESS;
}

template <class REQUEST, class RESPONSE>
const bsl::string&
RequestManagerRequest<REQUEST, RESPONSE>::nodeDescription() const
{
    return d_nodeDescription;
}

template <class REQUEST, class RESPONSE>
int RequestManagerRequest<REQUEST, RESPONSE>::groupId() const
{
    return d_groupId;
}

template <class REQUEST, class RESPONSE>
const bdld::Datum& RequestManagerRequest<REQUEST, RESPONSE>::userData() const
{
    return d_userData.datum();
}

// --------------------
// class RequestManager
// --------------------

template <class REQUEST, class RESPONSE>
inline bmqt::GenericResult::Enum RequestManager<REQUEST, RESPONSE>::sendHelper(
    bmqio::Channel*                     channel,
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

template <class REQUEST, class RESPONSE>
void RequestManager<REQUEST, RESPONSE>::onRequestTimeout(int requestId)
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
    RESPONSE& response = request->response();

    // 1. 'fake' a response, with a Timeout status type
    response.rId().makeValue(requestId);

    bmqu::MemOutStream os;
    os << "The request timedout after "
       << bmqu::PrintUtil::prettyTimeInterval(
              bmqsys::Time::highResolutionTimer() - request->d_sendTime);

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

template <class REQUEST, class RESPONSE>
void RequestManager<REQUEST, RESPONSE>::applyResponse(const RequestSp& request,
                                                      const RESPONSE& response)
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

template <class REQUEST, class RESPONSE>
RequestManager<REQUEST, RESPONSE>::RequestManager(
    bmqp::EventType::Enum  eventType,
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

template <class REQUEST, class RESPONSE>
RequestManager<REQUEST, RESPONSE>::RequestManager(
    bmqp::EventType::Enum  eventType,
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

template <class REQUEST, class RESPONSE>
RequestManager<REQUEST, RESPONSE>::RequestManager(
    bmqp::EventType::Enum  eventType,
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

template <class REQUEST, class RESPONSE>
RequestManager<REQUEST, RESPONSE>::~RequestManager()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_requests.empty() &&
                     "There are still outstanding requests, "
                     "'cancelAllRequests()' must be called before destroying "
                     "this object");
}

template <class REQUEST, class RESPONSE>
inline RequestManager<REQUEST, RESPONSE>&
RequestManager<REQUEST, RESPONSE>::setExecutor(const bmqex::Executor& executor)
{
    d_executor = executor;
    return *this;
}

template <class REQUEST, class RESPONSE>
typename RequestManager<REQUEST, RESPONSE>::RequestSp
RequestManager<REQUEST, RESPONSE>::createRequest()
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

template <class REQUEST, class RESPONSE>
bmqt::GenericResult::Enum RequestManager<REQUEST, RESPONSE>::sendRequest(
    const typename RequestManager::RequestSp& request,
    bmqio::Channel*                           channel,
    const bsl::string&                        nodeDescription,
    const bsls::TimeInterval&                 timeout,
    bsls::Types::Int64                        watermark,
    bsl::string*                              errorDescription)
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

template <class REQUEST, class RESPONSE>
bmqt::GenericResult::Enum RequestManager<REQUEST, RESPONSE>::sendRequest(
    const typename RequestManager::RequestSp& request,
    const SendFn&                             sendFn,
    const bsl::string&                        nodeDescription,
    const bsls::TimeInterval&                 timeout,
    bsl::string*                              errorDescription)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // MUTEX LOCKED

    // Inject the requestId in the request
    int requestId = ++d_nextRequestId;
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
    request->d_sendTime              = bmqsys::Time::highResolutionTimer();
    bmqt::GenericResult::Enum sendRc = sendFn(d_schemaEventBuilder.blob_sp());
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
    d_scheduler_p->scheduleEvent(
        &(request->d_timeoutSchedulerHandle),
        bmqsys::Time::nowMonotonicClock() + timeout,
        bmqex::BindUtil::bindExecute(
            bmqex::ExecutionPolicyUtil::oneWay()
                .possiblyBlocking()
                .useExecutor(d_executor)
                .useAllocator(d_allocator_p),
            bdlf::BindUtil::bind(&RequestManager::onRequestTimeout,
                                 this,
                                 requestId)));

    // Insert the request in the map
    bsl::pair<RequestMapIter, bool> insertRC = d_requests.insert(
        bsl::make_pair(requestId, request));
    BSLS_ASSERT_SAFE(insertRC.second);
    (void)insertRC;  // Compiler happiness

    return bmqt::GenericResult::e_SUCCESS;
}

template <class REQUEST, class RESPONSE>
int RequestManager<REQUEST, RESPONSE>::processResponse(
    const RESPONSE& response)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS   = 0,
        rc_NOT_FOUND = -1
    };

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

template <class REQUEST, class RESPONSE>
void RequestManager<REQUEST, RESPONSE>::cancelAllRequests(
    const RESPONSE& reason)
{
    cancelAllRequestsImpl(reason, RequestType::k_NO_GROUP_ID);
}

template <class REQUEST, class RESPONSE>
void RequestManager<REQUEST, RESPONSE>::cancelAllRequests(
    const RESPONSE& reason,
    int             groupId)
{
    BSLS_ASSERT_SAFE(groupId != RequestType::k_NO_GROUP_ID);

    cancelAllRequestsImpl(reason, groupId);
}

template <class REQUEST, class RESPONSE>
void RequestManager<REQUEST, RESPONSE>::cancelAllRequestsImpl(
    const RESPONSE& reason,
    int             groupId)
{
    // Note that requests must be cancelled in the same order in which they
    // were sent.

    // Create a new map so we can work on it outside the mutex
    RequestMap requestsCopy(d_requests.bucket_count(),
                            d_requests.get_allocator());

    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // MUTEX LOCKED

        // Iterate over each requests, moving the ones matching
        // 'nodeDescription' from the 'd_requests' map to 'requests'.
        RequestMapIter it = d_requests.begin();
        while (it != d_requests.end()) {
            if (groupId == RequestType::k_NO_GROUP_ID  // i.e., cancel all
                || groupId == it->second->groupId()) {
                // Do not notify about timed out requests.
                if (!it->second->d_haveTimeout) {
                    bsl::pair<RequestMapIter, bool> insertRC =
                        requestsCopy.insert(
                            bsl::make_pair(it->first, it->second));
                    BSLS_ASSERT_SAFE(insertRC.second);
                    (void)insertRC;  // Compiler happiness
                }
                d_requests.erase(it++);
            }
            else {
                ++it;
            }
        }
    }

    if (groupId == RequestType::k_NO_GROUP_ID) {
        BALL_LOG_INFO << "Canceling all requests (" << requestsCopy.size()
                      << " items) with " << reason << ".";
    }
    else {
        BALL_LOG_INFO << "Canceling requests belonging to '" << groupId
                      << "' group (" << requestsCopy.size() << " items) with "
                      << reason << ".";
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

#endif
