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

#ifndef INCLUDED_BMQP_REQUESTMANAGER
#define INCLUDED_BMQP_REQUESTMANAGER

//@PURPOSE: Provide a mechanism to manipulate requests and their response.
//
//@CLASSES:
//  bmqp::RequestManager: mechanism to manage requests and responses
//
//@SEE_ALSO:
//  bmqp_requestmanagerrequest: request and its associated context
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
// The 'bmqp::RequestManager' class is fully thread-safe (see
// 'bsldoc_glossary'), meaning that two threads can safely call any methods on
// the *same* *instance* without external synchronization, and re-entrant safe,
// meaning that a method of this object can be called from within a callback
// emanating from that same object instance.
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
// callbacks are invoked in the order of sending requests.  There are three
// versions of 'cancelAllRequests': the one that takes no filter argument, the
// one that takes 'groupId', and the one that takes 'componentId'.  Without
// any filter all requests get cancelled.  With 'groupId', only those requests
// which were associated with the same id by 'setGroupId' get cancelled.  With
// 'componentId', only those requests tagged with the same component identifier
// via 'setComponentId' get cancelled.  The two filtered versions operate on
// orthogonal axes and can therefore coexist on the same 'RequestManager'
// instance without interference.
//
/// Distributed Trace Integration
///-----------------------------
// A 'bmqpi::DTContext' provided at construction is propagated to every
// 'bmqp::RequestManagerRequest' created by this object, so that a span
// attached to a request is activated within that context whenever a callback
// for the request is invoked.  See the 'bmqp_requestmanagerrequest' component
// for details.
//
/// Usage Example (basic)
///---------------------
// This example shows basic usage of the 'RequestManager' object.
//
// First, let's create a 'RequestManager' object:
//..
//  bslma::Allocator               allocator = bslma::Default::allocator();
//  bdlbb::PooledBlobBufferFactory blobBufferFactory(4096, &allocator);
//  bdlmt::EventScheduler          scheduler(
//                                     bsls::SystemClockType::e_MONOTONIC,
//                                     allocator);
//
//  bmqp::RequestManager requestManager(bmqp::EventType::e_CONTROL,
//                                      &blobBufferFactory,
//                                      &scheduler,
//                                      false,  // late response mode
//                                      &allocator);
//..
//
// Then we can use it to create and send a request.  (here we assume that we
// have an already established and valid 'bmqio::Channel' object to use)
//..
//  // We first ask a Request object to the RequestManager
//  bmqp::RequestManager::RequestSp request = requestManager.createRequest();
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
//  MyClass::onOpenQueueResponse(const bmqp::RequestManager::RequestSp&
//  context)
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
//  bmqp::RequestManager::RequestSp request = requestManager.createRequest();
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
//  bmqp::RequestManager::RequestSp request = requestManager.createRequest();
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
//  bmqp::RequestManager::RequestSp request = requestManager.createRequest();
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

#include <bmqp_blobpoolutil.h>
#include <bmqp_protocol.h>
#include <bmqp_schemaeventbuilder.h>
#include <bmqt_resultcode.h>

#include <bmqc_orderedhashmap.h>
#include <bmqex_executor.h>

// BDE
#include <ball_log.h>
#include <bsl_functional.h>
#include <bsl_limits.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_mutex.h>
#include <bsls_cpp11.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlbb {
class Blob;
}
namespace bdlmt {
class EventScheduler;
}
namespace bmqio {
class Channel;
}
namespace bmqp_ctrlmsg {
class ControlMessage;
}
namespace bmqpi {
class DTContext;
}

namespace bmqp {

// FORWARD DECLARATION
class RequestManagerRequest;

// ====================
// class RequestManager
// ====================

/// Mechanism to manage requests and their response.
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

    typedef RequestManagerRequest RequestType;

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

    typedef RequestMap::iterator RequestMapIter;

    typedef RequestMap::const_iterator RequestMapConstIter;

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
    // Id most recently assigned to a request, or
    // zero if no request has been sent yet (int
    // not atomicInt since this will always be
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

    /// @brief Return the identifier to assign to the next request to send.
    ///
    /// Identifiers are handed out in increasing order and resume from the
    /// lowest valid identifier once the highest one has been used.  An
    /// identifier held by an outstanding request is never handed out
    /// again for as long as that request remains outstanding.
    ///
    /// @return A strictly positive identifier which is not in use by any
    ///         outstanding request.
    ///
    /// Note that this method must be called with `d_mutex` locked.
    int generateRequestId();

    /// Callback invoked by the scheduler when the request identified by the
    /// specified `requestId` has timedout.
    void onRequestTimeout(int requestId);

    /// Apply the specified `response` to the specified `request`.
    void applyResponse(const RequestSp&                    request,
                       const bmqp_ctrlmsg::ControlMessage& response);

    /// Cancel all outstanding requests matching the specified `groupId` and
    /// `componentId` filters, with the specified `reason` response
    /// description.  A value of `NO_GROUP_ID` for `groupId` disables
    /// group filtering; a value of `k_NO_COMPONENT_ID` for `componentId`
    /// disables component filtering.  Both filters are applied with AND
    /// semantics.  The corresponding response callbacks will be invoked in
    /// the order in which requests were sent.
    void cancelAllRequestsImpl(const bmqp_ctrlmsg::ControlMessage& reason,
                               int                                 groupId,
                               int componentId);

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
    int processResponse(const bmqp_ctrlmsg::ControlMessage& response);

    /// Cancel all outstanding requests with the specified `reason` response
    /// description.  The corresponding response callbacks will be invoked
    /// in the order in which requests were sent.
    void cancelAllRequests(const bmqp_ctrlmsg::ControlMessage& reason);

    /// Cancel all outstanding requests belonging to the specified
    /// `groupId`, with the specified `reason` response description.  The
    /// behavior is undefined if `groupId` is `NO_GROUP_ID`.  The
    /// corresponding response callbacks will be invoked in the order in
    /// which requests were sent.
    void cancelGroupRequests(const bmqp_ctrlmsg::ControlMessage& reason,
                             int                                 groupId);

    /// Cancel all outstanding requests tagged with the specified
    /// `componentId`, with the specified `reason` response description.
    /// The behavior is undefined if `componentId` is
    /// `k_NO_COMPONENT_ID`.  The corresponding response callbacks will be
    /// invoked in the order in which requests were sent.
    void cancelComponentRequests(const bmqp_ctrlmsg::ControlMessage& reason,
                                 int componentId);
};

}  // close package namespace
}  // close enterprise namespace

#endif
