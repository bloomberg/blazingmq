// Copyright 2016-2023 Bloomberg Finance L.P.
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

// mqbblp_queuehandlecatalog.h                                        -*-C++-*-
#ifndef INCLUDED_MQBBLP_QUEUEHANDLECATALOG
#define INCLUDED_MQBBLP_QUEUEHANDLECATALOG

/// @file mqbblp_queuehandlecatalog.h
///
/// @brief Provide a mechanism to hold and manipulate `QueueHandle` objects.
///
/// Thread Safety                           {#mqbblp_queuehandlecatalog_thread}
/// =============
///
/// Unless specified otherwise, all methods of the
/// @bbref{mqbblp::QueueHandleCatalog} must be executed by the dispatcher
/// thread of the associated queue.

// MQB
#include <mqbi_queue.h>
#include <mqbi_storage.h>
#include <mqbu_resourceusagemonitor.h>

// BMQ
#include <bmqc_twokeyhashmap.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_queueid.h>

// BDE
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcmd {
class QueueHandle;
}

namespace mqbblp {

// FORWARD DECLARATION
class QueueState;

struct QueueHandleCatalog_SubStreamContext {
    bsl::string  d_appId;
    unsigned int d_subQueueId;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(QueueHandleCatalog_SubStreamContext,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    explicit QueueHandleCatalog_SubStreamContext(
        bslma::Allocator* basicAllocator = 0)
    : d_appId(basicAllocator)
    , d_subQueueId(bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID)
    {
    }

    QueueHandleCatalog_SubStreamContext(
        const QueueHandleCatalog_SubStreamContext& src,
        bslma::Allocator*                          basicAllocator = 0)
    : d_appId(src.d_appId, basicAllocator)
    , d_subQueueId(src.d_subQueueId)
    {
    }
};

// ========================
// class QueueHandleCatalog
// ========================

/// Mechanism to hold and manipulate QueueHandle objects associated to a
/// Queue.
class QueueHandleCatalog {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.QUEUEHANDLECATALOG");

  public:
    // PUBLIC TYPES
    typedef QueueHandleCatalog_SubStreamContext SubStreamContext;

    /// `DownstreamKey` is an alias for a key value, representing a
    /// (queueHandlePtr, downstreamSubQueueId) and intended to be used as
    /// the downstream identifier for a subStream.
    ///
    /// * `queueHandlePtr`:       Pointer to queue handle
    /// * `downstreamSubQueueId`: the downstream subQueueId
    typedef bsl::pair<mqbi::QueueHandle*, unsigned int> DownstreamKey;

    /// `iterateConsumers` calls visitor for each handle for each registered
    /// subId and streamParameters.
    typedef bsl::function<void(mqbi::QueueHandle*,
                               const mqbi::QueueHandle::StreamInfo&)>
        Visitor;

  private:
    // PRIVATE TYPES

    /// Type used to uniquely identify a queue handle from a client:
    /// * `RequesterId`: the unique identifier of the requester
    /// * `int`        : the downstream queueId
    typedef bsl::pair<mqbi::QueueHandleRequesterContext::RequesterId, int>
        RequesterKey;

    /// (queueHandlePtr, requester) -> queueHandleSp
    typedef bmqc::TwoKeyHashMap<mqbi::QueueHandle*,
                                RequesterKey,
                                bsl::shared_ptr<mqbi::QueueHandle> >
        HandleMap;

  private:
    // DATA

    /// The associated queue.
    mqbi::Queue* d_queue_p;

    /// Factory to use for creating `QueueHandle` objects.
    bslma::ManagedPtr<mqbi::QueueHandleFactory> d_handleFactory_mp;

    /// Map of all created handles.  `mutable` because `TwoKeyHashMap` doesn't
    /// expose `const` operators.
    mutable HandleMap d_handles;

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    QueueHandleCatalog(const QueueHandleCatalog&);             // = delete;
    QueueHandleCatalog& operator=(const QueueHandleCatalog&);  // = delete;

  private:
    // PRIVATE MANIPULATOR

    /// Shared pointer custom deleter for the specified `handle`.
    void queueHandleDeleter(mqbi::QueueHandle* handle);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(QueueHandleCatalog,
                                   bslma::UsesBslmaAllocator)

    // CREATOR

    /// Create a new object associated to the specified `queue`.  Use the
    /// specified `allocator` for any memory allocations.  Use the specified
    /// 'counter' to aggregate the counting of  unconfirmed by each handle.
    QueueHandleCatalog(mqbi::Queue* queue, bslma::Allocator* allocator);

    /// Destructor.
    ~QueueHandleCatalog();

    // MANIPULATOR

    /// Override the handleFactory used to create QueueHandle objects with
    /// the specified `factory` and return a reference offering modifiable
    /// access to this object.  This is mostly needed only for test driver,
    /// so that a `MockQueueHandle` factory can be injected.
    QueueHandleCatalog&
    setHandleFactory(bslma::ManagedPtr<mqbi::QueueHandleFactory>& factory);

    /// Create and return a new handle for the specified `clientContext`,
    /// with the specified `parameters`, and update the specified `stats`.
    mqbi::QueueHandle*
    createHandle(const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                                                            clientContext,
                 const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                 mqbstat::QueueStatsDomain*                 stats);

    /// Utility method to release the specified `parameters` from the
    /// specified `handle`.  If after this operation `handle` ends up no
    /// longer representing any resource, it is removed from the internal
    /// map, the specified `deleted` is set to true and the specified
    /// `handleSp` is populated with it allowing the caller to control at
    /// which time destruction will occur.  The specified `lostFlags` is set
    /// to the flags that were set on the queue prior to the release, but
    /// are no longer after.  Return 0 on success, or a non-zero value on
    /// error.
    int releaseHandleHelper(
        bsl::shared_ptr<mqbi::QueueHandle>*        handleSp,
        bsls::Types::Uint64*                       lostFlags,
        mqbi::QueueHandle*                         handle,
        const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
        bool                                       isFinal);

    // ACCESSORS

    /// Return the `Queue` object associated to this object.
    mqbi::Queue* queue() const;

    /// Return the number of handles currently owned by this object.
    int handlesCount() const;

    /// Return true if this catalog object has information about the
    /// specified `handle`.
    bool hasHandle(const mqbi::QueueHandle* handle) const;

    /// Return the handle associated with the specified `queueId` of the
    /// requester having the specified `context`, or a null pointer if no
    /// such handle exist.
    mqbi::QueueHandle*
    getHandleByRequester(const mqbi::QueueHandleRequesterContext& context,
                         int queueId) const;

    /// Load into the specified `out` the list of all handles managed by
    /// this catalog object.
    void loadHandles(bsl::vector<mqbi::QueueHandle*>* out) const;

    /// Iterate all consumer handles managed by this catalog object and
    /// invoke the specified `visitor` for each
    /// {handle, StreamParameters} combination.
    void iterateConsumers(const Visitor& visitor) const;

    /// Load into the specified `out` list the internal details about the
    /// handles managed by this catalog.
    void loadInternals(bsl::vector<mqbcmd::QueueHandle>* out) const;

    bsls::Types::Int64 countUnconfirmed() const;
};

// ============================================================================
//                            INLINE DEFINITIONS
// ============================================================================

// ------------------------
// class QueueHandleCatalog
// ------------------------

inline mqbi::Queue* QueueHandleCatalog::queue() const
{
    return d_queue_p;
}

}  // close package namespace
}  // close enterprise namespace

#endif
