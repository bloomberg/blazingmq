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

// mqbblp_queuesessionmanager.h                                       -*-C++-*-
#ifndef INCLUDED_MQBBLP_QUEUESESSIONMANAGER
#define INCLUDED_MQBBLP_QUEUESESSIONMANAGER

//@PURPOSE: Provide a mechanism for opening and closing queues.
//
//@CLASSES:
//  mqbblp::QueueSessionManager: mechanism for opening and closing a queue
//

//@DESCRIPTION: This component provides a mechanism,
// 'mqbblp::QueueSessionManager', that immplements opening and closing a queue.

// MQB

#include <mqbi_domain.h>
#include <mqbi_queue.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_queueid.h>

#include <bmqu_atomicvalidator.h>

// BDE
#include <ball_log.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_atomic.h>
#include <bsls_cpp11.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqt {
class Uri;
}
namespace mqbconfm {
class DomainQualification;
}
namespace mqbi {
class DispatcherClient;
}
namespace mqbstat {
class QueueStatsClient;
}
namespace bmqst {
class StatContext;
}

namespace mqbblp {

// =========================
// class QueueSessionManager
// =========================

class QueueSessionManager {
  public:
    // TYPES
    typedef bsl::function<void(
        const bmqp_ctrlmsg::Status&            status,
        mqbi::QueueHandle*                     handle,
        const bmqp_ctrlmsg::OpenQueueResponse& response)>
        GetHandleCallback;

    typedef bsl::function<void(
        const bsl::shared_ptr<mqbi::QueueHandle>& handle)>
        CloseHandleCallback;

    typedef bsl::function<void(
        bmqp_ctrlmsg::StatusCategory::Value failureCategory,
        const bslstl::StringRef&            errorDescription,
        const int                           code)>
        ErrorCallback;

    /// Struct holding information associated to a substream of a queue
    /// opened in the session
    struct SubQueueInfo {
        bsl::shared_ptr<mqbstat::QueueStatsClient> d_stats;
        // Stats of this SubQueue, with regards
        // to the client.

        bmqp::QueueId d_queueId;
        // queueId (id, subId) of the SubQueue

        // CREATORS

        /// Constructor of a new object, initializes all data members to
        /// default values.
        SubQueueInfo(const bmqp::QueueId& queueId);
    };

    /// Struct holding the state associated to a queue opened in the session
    struct QueueState {
        // TYPES

        /// Map of {appId, subQueueId} -> SubQueueInfo
        typedef bmqp::ProtocolUtil::QueueInfo<SubQueueInfo> StreamsMap;

        // PUBLIC DATA
        mqbi::QueueHandle* d_handle_p;
        // QueueHandle of the queue

        bool d_hasReceivedFinalCloseQueue;
        // Flag to indicate if the 'final' closeQueue
        // request for this handle has been received.
        // This flag can be used to reject PUT & CONFIRM
        // messages which some clients try to post after
        // closing a queue (under certain conditions, such
        // incorrect usage cannot be caught in the SDK eg,
        // if messages are being posted from one app
        // thread, while closeQueue request is being sent
        // from another app thread).  This flag can also
        // be used by the queue or queue engine for sanity
        // checking.

        StreamsMap d_subQueueInfosMap;
        // Map of subQueueId to information associated to
        // a substream of a queue opened in this session
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(QueueState, bslma::UsesBslmaAllocator)

        // CREATORS

        /// Constructor of a new object, initializes all data members to
        /// default values and uses the optionally specified `allocator` for
        /// any memory allocation.
        explicit QueueState(bslma::Allocator* allocator = 0);

        /// Constructor of a new object from the specified `original` and
        /// uses the optionally specified `allocator` for any memory
        /// allocation.
        QueueState(const QueueState& original,
                   bslma::Allocator* allocator = 0);
    };

    /// Map of queueId -> QueueState
    typedef bsl::unordered_map<int, QueueState> QueueStateMap;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.QUEUESESSIONMANAGER");

    // DATA
    mqbi::DispatcherClient* d_dispatcherClient_p;
    // Dispatcher client to use, held not
    // owned

    bmqst::StatContext* d_statContext_p;
    // StatContext to use, held not owned

    mqbi::DomainFactory* d_domainFactory_p;
    // DomainFactory to use, held not owned

    bslma::Allocator* d_allocator_p;
    // Allocator to use.

    bsls::AtomicBool d_shutdownInProgress;
    // Once this flag is set, either the
    // channel has been destroyed and is no
    // longer valid or we sent the
    // 'DisconnectResponse' to the client;
    // in either way, *NO* messages of any
    // sort should be delivered to the
    // client.

    bmqu::AtomicValidatorSp d_validator_sp;
    // Object validator used in callbacks,
    // to avoid executing a callback if the
    // session has been destroyed: this is
    // *ONLY* to be used with the callbacks
    // that will be called from outside of
    // the dispatcher's thread (such as a
    // bmqconf IO thread); because we can't
    // guarantee this queue is drained
    // before destroying the session.

    QueueStateMap d_queues;
    // Map containing the state of all
    // queues opened by this session
    // manager.  Note that value stored in
    // the map is a raw pointer to the
    // QueueHandle, so the handle *must* be
    // manually released during shutdown of
    // the session.  This structure is
    // manipulated only from the client
    // dispatcher thread.

    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>
        d_requesterContext_sp;
    // Context used to uniquely identify
    // this client when requesting a queue
    // handle.  A shared pointer is needed
    // because when the source client has
    // disconnected, this object will be
    // destroyed but the context may still
    // be needed for processing the open
    // queue response or by the associated
    // queue handle.

    // NOT IMPLEMENTED
    QueueSessionManager(const QueueSessionManager&) BSLS_CPP11_DELETED;
    QueueSessionManager&
    operator=(const QueueSessionManager&) BSLS_CPP11_DELETED;

  private:
    // PRIVATE MANIPULATORS

    /// Callback invoked in response to a domain qualification for the
    /// specified `uri` (and from the specified `controlMessage`) made to
    /// the domain manager.  If the specified `status` is SUCCESS, the
    /// request was success and the specified `qualification` contains
    /// the result; otherwise `status` contains the category, error code and
    /// description of the failure.  The specified `validator` must be
    /// checked to ensure the session is still valid at the time the
    /// callback is invoked.
    void onDomainQualifiedCb(const bmqp_ctrlmsg::Status& status,
                             const bsl::string&          resolvedDomain,
                             const GetHandleCallback&    successCallback,
                             const ErrorCallback&        errorCallback,
                             const bmqt::Uri&            uri,
                             const bmqp_ctrlmsg::ControlMessage& request,
                             const bmqu::AtomicValidatorSp&      validator);

    /// Callback invoked in response to an open domain request (in the
    /// specified `controlMessage`) made to the domain manager, for the
    /// specified queue `uri`.  If the specified `status` is SUCCESS, the
    /// request was success and the specified `domain` contains a pointer to
    /// the result; otherwise `status` contains the category, error code and
    /// description of the failure.  The specified `validator` must be
    /// checked to ensure the session is still valid at the time the
    /// callback is invoked.
    void onDomainOpenCb(const bmqp_ctrlmsg::Status&         status,
                        mqbi::Domain*                       domain,
                        const GetHandleCallback&            successCallback,
                        const ErrorCallback&                errorCallback,
                        const bmqt::Uri&                    uri,
                        const bmqp_ctrlmsg::ControlMessage& request,
                        const bmqu::AtomicValidatorSp&      validator);

    void onQueueOpenCb(
        const bmqp_ctrlmsg::Status&                      status,
        mqbi::QueueHandle*                               queueHandle,
        const bmqp_ctrlmsg::OpenQueueResponse&           openQueueResponse,
        const mqbi::Domain::OpenQueueConfirmationCookie& confirmationCookie,
        const GetHandleCallback&                         responseCallback,
        const bmqp_ctrlmsg::ControlMessage&              request,
        const bmqu::AtomicValidatorSp&                   validator);

    /// Callback invoked in response to an open queue request (in the
    /// specified `controlMessage`).  If the specified `status` is SUCCESS,
    /// the request was success and the specified `queueHandle` contains the
    /// handle representing the queue that was allocated for this session
    /// and the specified `confirmationCookie` a cookie that must be set to
    /// `true` to indicate receiving and processing of that response;
    /// otherwise `status` contains the category, error code and description
    /// of the failure.  The `queueHandle` must be released once no longer
    /// needed.
    void onQueueOpenCbDispatched(
        const bmqp_ctrlmsg::Status&                      status,
        mqbi::QueueHandle*                               queueHandle,
        const bmqp_ctrlmsg::OpenQueueResponse&           openQueueResponse,
        const mqbi::Domain::OpenQueueConfirmationCookie& confirmationCookie,
        const GetHandleCallback&                         responseCallback,
        const bmqp_ctrlmsg::ControlMessage&              request);

    void onHandleReleased(const bsl::shared_ptr<mqbi::QueueHandle>& handle,
                          const mqbi::QueueHandleReleaseResult&     result,
                          const CloseHandleCallback&          successCallback,
                          const ErrorCallback&                errorCallback,
                          const bmqp_ctrlmsg::ControlMessage& request,
                          const bmqu::AtomicValidatorSp&      validator);

    /// Callback invoked by the queue engine in response to a
    /// `QueueHandle::release` call for the specified `handle`.  If the
    /// specified `isDeleted` is true, this indicates that this release was
    /// the last one of this `handle` and references to it can be removed
    /// from the internal handle map of this component.  If `isDeleted` is
    /// true, `handle` remains valid for as long there is a reference to
    /// this shared pointer.  Note that it is undefined to call any method
    /// such as `postMessage`, `confirmMessage`, etc., on this `handle` in
    /// this case as it has been released from the queue, and per contract,
    /// no more downstream actions are expected.  The `dispatched` flavor of
    /// this method must be executed on the client dispatcher thread.  The
    /// specified `validator` must be checked to ensure the session is still
    /// valid at the time the method is invoked.
    void onHandleReleasedDispatched(
        const bsl::shared_ptr<mqbi::QueueHandle>& handle,
        const mqbi::QueueHandleReleaseResult&     result,
        const CloseHandleCallback&                successCallback,
        const ErrorCallback&                      errorCallback,
        const bmqp_ctrlmsg::ControlMessage&       request);

    void
    dispatchErrorCallback(const ErrorCallback&                errorCallback,
                          bmqp_ctrlmsg::StatusCategory::Value failureCategory,
                          const bsl::string&                  errorDescription,
                          const int                           code);

  public:
    // CREATORS

    /// Create a `QueueSessionManager` object.
    QueueSessionManager(mqbi::DispatcherClient*             dispatcherClient,
                        const bmqp_ctrlmsg::ClientIdentity& clientIdentity,
                        bmqst::StatContext*                 statContext,
                        mqbi::DomainFactory*                domainFactory,
                        bslma::Allocator*                   allocator);

    ~QueueSessionManager();

    // MANIPULATORS
    void processOpenQueue(const bmqp_ctrlmsg::ControlMessage& request,
                          const GetHandleCallback&            successCallback,
                          const ErrorCallback&                errorCallback);

    void processCloseQueue(const bmqp_ctrlmsg::ControlMessage& request,
                           const CloseHandleCallback&          successCallback,
                           const ErrorCallback&                errorCallback);

    /// Signal this component that its associated client is shutting down.
    void shutDown();

    void tearDown();

    // ACCESSORS

    /// Map containing the state of all queues opened by this queue session
    /// manager.
    QueueStateMap& queues();

    /// Return a const reference to the QueueHandleRequesterContext.
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
    requesterContext() const;
};

// ============================================================================
//                            INLINE DEFINITIONS
// ============================================================================

// ----------------------------------------
// struct QueueSessionManager::SubQueueInfo
// ----------------------------------------

inline QueueSessionManager::SubQueueInfo::SubQueueInfo(
    const bmqp::QueueId& queueId)
: d_stats()
, d_queueId(queueId)
{
    d_stats.createInplace();
}

// --------------------------------------
// struct QueueSessionManager::QueueState
// --------------------------------------

inline QueueSessionManager::QueueState::QueueState(bslma::Allocator* allocator)
: d_handle_p(0)
, d_hasReceivedFinalCloseQueue(false)
, d_subQueueInfosMap(allocator)
{
    // NOTHING
}

inline QueueSessionManager::QueueState::QueueState(const QueueState& original,
                                                   bslma::Allocator* allocator)
: d_handle_p(original.d_handle_p)
, d_hasReceivedFinalCloseQueue(original.d_hasReceivedFinalCloseQueue)
, d_subQueueInfosMap(original.d_subQueueInfosMap, allocator)
{
    // NOTHING
}

// -------------------------
// class QueueSessionManager
// -------------------------

inline QueueSessionManager::QueueStateMap& QueueSessionManager::queues()
{
    return d_queues;
}

inline const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
QueueSessionManager::requesterContext() const
{
    return d_requesterContext_sp;
}

inline void QueueSessionManager::shutDown()
{
    d_shutdownInProgress = true;
}

}  // close package namespace
}  // close enterprise namespace

#endif
