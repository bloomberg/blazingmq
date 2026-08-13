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

#ifndef INCLUDED_BMQP_REQUESTMANAGERREQUEST
#define INCLUDED_BMQP_REQUESTMANAGERREQUEST

/// @file bmqp_requestmanagerrequest.h
///
/// @brief Provide a request and its associated context.
///
/// This component provides a mechanism, @bbref{bmqp::RequestManagerRequest},
/// representing a request that was sent and is pending a response for it,
/// holding the request itself along with all the associated context state.
/// This allows for both synchronous and asynchronous response management.
/// This component also provides @bbref{bmqp::RequestManagerComponentId}, the
/// set of identifiers with which a request can be tagged for the purpose of
/// component-scoped cancellation.
///
/// Requests are created, sent and completed by the
/// @bbref{bmqp::RequestManager} mechanism; refer to that component for the
/// request lifecycle and for usage examples.
///
/// Thread Safety                          {#bmqp_requestmanagerrequest_thread}
/// =============
///
/// The @bbref{bmqp::RequestManagerRequest} class is fully thread-safe (see
/// `bsldoc_glossary`), meaning that two threads can safely call any methods on
/// the *same* *instance* without external synchronization, and re-entrant
/// safe, meaning that a method of this object can be called from within a
/// callback emanating from that same object instance.
///
/// Distributed Trace Integration              {#bmqp_requestmanagerrequest_dt}
/// =============================
///
/// An externally created @bbref{bmqpi::DTSpan} may be attached to a request
/// using the `setDTSpan` method.  This ensures that a span representative of a
/// request survives for at least the lifetime of the request object.
///
/// A request which owns a `DTSpan` can additionally be provided with a
/// @bbref{bmqpi::DTContext} via the `setDTContext` method.  This guarantees
/// that the `DTSpan` will be the active span of the `DTContext` whenever a
/// response- or signal- callback of the request is invoked.  The easiest (and
/// recommended) way to attach a `DTContext` is by using the
/// `bmqp::RequestManager::setDTContext` method on the object used to create
/// new request instances; the context will be propagated to any new requests
/// created.  Note that setting a `DTContext` without a `DTSpan` will have no
/// effect.

// BMQ

#include <bmqp_ctrlmsg_messages.h>
#include <bmqpi_dtcontext.h>
#include <bmqpi_dtspan.h>
#include <bmqt_resultcode.h>

// BDE
#include <bdld_manageddatum.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_allocatorargt.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_semaphore.h>
#include <bsls_assert.h>
#include <bsls_cpp11.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace bmqp {

// FORWARD DECLARATION
class RequestManager;

// ================================
// struct RequestManagerComponentId
// ================================

/// @brief Integer constants used to tag requests sent via a `RequestManager`
/// for the purpose of component-scoped cancellation.
struct RequestManagerComponentId {
    // CONSTANTS
    enum {
        /// Default; no component association.
        k_NO_COMPONENT_ID = 0,

        /// Cluster FSM component id.
        k_CLUSTER_FSM = 1,

        /// Base value for Partition FSM component ids.  Each partition
        /// gets a unique componentId computed as
        /// `k_PARTITION_FSM_PREFIX + partitionId`.  Use the
        /// `partitionFSM()` class method to obtain the componentId for a
        /// given partition.
        k_PARTITION_FSM_PREFIX = 900
    };

    // CLASS METHODS

    /// @brief Return the componentId for the partition with the specified
    /// `partitionId`.  The behavior is undefined unless
    /// `0 <= partitionId < 100`.
    ///
    /// @param partitionId The partition to compute a componentId for.
    ///
    /// @return The componentId associated with `partitionId`.
    static int partitionFSM(int partitionId);

    /// @brief Return true if the specified `componentId` is valid, including
    /// `k_NO_COMPONENT_ID`.
    ///
    /// @param componentId The identifier to validate.
    ///
    /// @return true if `componentId` is valid, and false otherwise.
    static bool isValid(int componentId);
};

// ===========================
// class RequestManagerRequest
// ===========================

/// @brief Object representing a request sent, pending response for it; holding
/// the request and all associated context state.  This allows for both
/// synchronous and asynchronous response management.
class RequestManagerRequest {
  public:
    // PUBLIC CLASS DATA

    /// Default group identifier, indicating that the request does not belong
    /// to any group.
    static const int k_NO_GROUP_ID = -1;

    // TYPES

    /// Signature of a callback for delivering the response in the
    /// specified `context`.
    typedef bsl::function<void(
        const bsl::shared_ptr<RequestManagerRequest>& context)>
        ResponseCb;

    /// Signature of a callback for signaling a response in the specified
    /// `context`.
    typedef bsl::function<void(
        const bsl::shared_ptr<RequestManagerRequest>& context)>
        AsyncNotifierCb;

  private:
    // DATA
    bsl::weak_ptr<RequestManagerRequest> d_self_wp;
    // Weak pointer to self

    bmqp_ctrlmsg::ControlMessage d_requestMessage;
    // The request

    bmqp_ctrlmsg::ControlMessage d_responseMessage;
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

    int d_componentId;
    // The 'componentId' associated with this
    // request.  Used to cancel all requests
    // belonging to a specific component.

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
    friend class RequestManager;

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

    bmqp_ctrlmsg::ControlMessage& request();

    /// Return a reference offering modifiable access to the corresponding
    /// member of this object.
    bmqp_ctrlmsg::ControlMessage& response();

    RequestManagerRequest& setResponseCb(const ResponseCb& value);

    /// Set the corresponding member to the specified `value` and return a
    /// reference offering modifiable access to this object.
    RequestManagerRequest& setAsyncNotifierCb(const AsyncNotifierCb& value);

    /// Set group identifier to the specified `value` to assist canceling
    /// requests belonging to one group only.  By default, the identifier is
    /// `k_NO_GROUP_ID` meaning the request does not belong to any group in
    /// which case it gets cancelled by `cancelAllRequests` without
    /// `groupId`.  Return a reference offering modifiable access to this
    /// object.
    RequestManagerRequest& setGroupId(int value);

    /// Set component identifier to the specified `value` to assist canceling
    /// requests belonging to one component only.  By default, the identifier
    /// is `RequestManagerComponentId::k_NO_COMPONENT_ID`.  Return a
    /// reference offering modifiable access to this object.  The behavior is
    /// undefined unless `value` is valid and is not `k_NO_COMPONENT_ID`.
    RequestManagerRequest& setComponentId(int value);

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
    bool                                isLateResponse() const;
    bool                                isLocalTimeout() const;
    bool                                isError() const;
    const bmqp_ctrlmsg::ControlMessage& request() const;
    const bmqp_ctrlmsg::ControlMessage& response() const;

    /// Return the value of the corresponding member.
    const ResponseCb& responseCb() const;

    /// Convenient accessor to return the `GenericResult` of the response
    /// associated to this object: return either `success` or the category
    /// code associated to the Failure type of the response.
    bmqt::GenericResult::Enum result() const;

    /// Convenient accessor to return the domain specific error code of the
    /// response associated to this object: return either `success` or the
    /// error code associated to the Failure type of the response.
    int statusCode() const;

    /// Return the description of the node the request was sent to.
    const bsl::string& nodeDescription() const;

    /// Return the associated group id (`NO_GROUP_ID` by default).
    int groupId() const;

    /// Return the associated component id (`k_NO_COMPONENT_ID` by default).
    int componentId() const;

    /// Return the associated user data.
    const bdld::Datum& userData() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------------
// struct RequestManagerComponentId
// --------------------------------

inline int RequestManagerComponentId::partitionFSM(int partitionId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(partitionId >= 0);
    BSLS_ASSERT_SAFE(partitionId < 100);

    return k_PARTITION_FSM_PREFIX + partitionId;
}

inline bool RequestManagerComponentId::isValid(int componentId)
{
    return componentId == k_NO_COMPONENT_ID || componentId == k_CLUSTER_FSM ||
           (componentId >= k_PARTITION_FSM_PREFIX &&
            componentId < k_PARTITION_FSM_PREFIX + 100);
}

// ---------------------------
// class RequestManagerRequest
// ---------------------------

inline RequestManagerRequest::RequestManagerRequest(
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
, d_componentId(RequestManagerComponentId::k_NO_COMPONENT_ID)
, d_dtSpan_sp(NULL)
, d_dtContext_p(NULL)
, d_userData(allocator)
{
    // NOTHING
}

inline RequestManagerRequest::~RequestManagerRequest()
{
    clear();
}

inline void RequestManagerRequest::clear()
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
    d_groupId     = k_NO_GROUP_ID;
    d_componentId = RequestManagerComponentId::k_NO_COMPONENT_ID;
    d_dtSpan_sp.reset();
    d_dtContext_p = NULL;
}

inline void RequestManagerRequest::signal()
{
    if (d_asyncNotifierCb) {
        bsl::shared_ptr<RequestManagerRequest> context = d_self_wp.lock();
        BSLS_ASSERT_SAFE(context);

        bslma::ManagedPtr<void> spanToken(context->activateDTSpan());
        d_asyncNotifierCb(context);
    }

    d_semaphore.post();
}

inline void RequestManagerRequest::wait()
{
    // No need to timedWait on a timedSemaphore, 'sendRequest()' schedules an
    // event for timeout that will post on the semaphore.
    d_semaphore.wait();
}

inline bmqp_ctrlmsg::ControlMessage& RequestManagerRequest::request()
{
    return d_requestMessage;
}

inline bmqp_ctrlmsg::ControlMessage& RequestManagerRequest::response()
{
    return d_responseMessage;
}

inline RequestManagerRequest&
RequestManagerRequest::setResponseCb(const ResponseCb& value)
{
    d_responseCb = value;
    return *this;
}

inline RequestManagerRequest&
RequestManagerRequest::setAsyncNotifierCb(const AsyncNotifierCb& value)
{
    d_asyncNotifierCb = value;
    return *this;
}

inline RequestManagerRequest& RequestManagerRequest::setGroupId(int value)
{
    d_groupId = value;
    return *this;
}

inline RequestManagerRequest& RequestManagerRequest::setComponentId(int value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(RequestManagerComponentId::isValid(value));
    BSLS_ASSERT_SAFE(value != RequestManagerComponentId::k_NO_COMPONENT_ID);

    d_componentId = value;
    return *this;
}

inline RequestManagerRequest&
RequestManagerRequest::setDTSpan(const bsl::shared_ptr<bmqpi::DTSpan>& span)
{
    d_dtSpan_sp = span;
    return *this;
}

inline RequestManagerRequest&
RequestManagerRequest::setDTContext(bmqpi::DTContext* ctx)
{
    d_dtContext_p = ctx;
    return *this;
}

inline RequestManagerRequest&
RequestManagerRequest::adoptUserData(const bdld::Datum& value)
{
    d_userData.adopt(value);
    return *this;
}

inline bslma::ManagedPtr<void> RequestManagerRequest::activateDTSpan() const
{
    bslma::ManagedPtr<void> result;
    if (d_dtSpan_sp && d_dtContext_p) {
        result = d_dtContext_p->scope(d_dtSpan_sp);
    }
    return result;
}

inline bool RequestManagerRequest::isLateResponse() const
{
    return d_haveTimeout && d_haveResponse;
}

inline bool RequestManagerRequest::isLocalTimeout() const
{
    return d_haveTimeout && !d_haveResponse;
}

inline bool RequestManagerRequest::isError() const
{
    return isLocalTimeout() ? true : response().choice().isStatusValue();
}

inline const bmqp_ctrlmsg::ControlMessage&
RequestManagerRequest::request() const
{
    return d_requestMessage;
}

inline const bmqp_ctrlmsg::ControlMessage&
RequestManagerRequest::response() const
{
    return d_responseMessage;
}

inline const RequestManagerRequest::ResponseCb&
RequestManagerRequest::responseCb() const
{
    return d_responseCb;
}

inline bmqt::GenericResult::Enum RequestManagerRequest::result() const
{
    if (d_responseMessage.choice().isStatusValue()) {
        return static_cast<bmqt::GenericResult::Enum>(
            d_responseMessage.choice().status().category());  // RETURN
    }

    return bmqt::GenericResult::e_SUCCESS;
}

inline int RequestManagerRequest::statusCode() const
{
    if (d_responseMessage.choice().isStatusValue()) {
        return d_responseMessage.choice().status().code();  // RETURN
    }

    return bmqt::GenericResult::e_SUCCESS;
}

inline const bsl::string& RequestManagerRequest::nodeDescription() const
{
    return d_nodeDescription;
}

inline int RequestManagerRequest::groupId() const
{
    return d_groupId;
}

inline int RequestManagerRequest::componentId() const
{
    return d_componentId;
}

inline const bdld::Datum& RequestManagerRequest::userData() const
{
    return d_userData.datum();
}

}  // close package namespace
}  // close enterprise namespace

#endif
