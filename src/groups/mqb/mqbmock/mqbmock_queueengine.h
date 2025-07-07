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

// mqbmock_queueengine.h                                              -*-C++-*-
#ifndef INCLUDED_MQBMOCK_QUEUEENGINE
#define INCLUDED_MQBMOCK_QUEUEENGINE

//@PURPOSE: Provide mock implementation for 'mqbi::QueueEngine' interface.
//
//@CLASSES:
//  mqbmock::QueueEngine: mock queue engine implementation
//
//@DESCRIPTION: This component provides a mock implementation, of the
// 'mqbi::QueueEngine' protocol that is used to emulate a real queue engine for
// testing purposes.
//
/// Notes
///------
// At the time of this writing, this component implements minimal functionality
// to satisfy compilation.  Further functionality will be developed as
// necessary.

// MQB

#include <mqbi_queueengine.h>
#include <mqbu_storagekey.h>

// BDE
#include <bsl_iosfwd.h>
#include <bsl_memory.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class EventScheduler;
}
namespace mqbcmd {
class QueueEngine;
}

namespace mqbmock {

// =================
// class QueueEngine
// =================

/// Mock implementation of the `mqbi::QueueEngine` interface.
class QueueEngine : public mqbi::QueueEngine {
  private:
    // NOT IMPLEMENTED
    QueueEngine(const QueueEngine&) BSLS_KEYWORD_DELETED;
    QueueEngine& operator=(const QueueEngine&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(QueueEngine, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `mqbmock::QueueEngine` object.  Use the specified
    /// `allocator` for any memory allocation.
    explicit QueueEngine(bslma::Allocator* allocator);

    /// Destructor
    ~QueueEngine() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual mqbi::QueueEngine)

    /// Configure this instance.  The specified `isReconfigure` flag indicate
    /// if queue is being reconfigured. Return zero on success, non-zero value
    /// otherwise and populate the specified `errorDescription`.
    int configure(bsl::ostream& errorDescription,
                  bool          isReconfigure) BSLS_KEYWORD_OVERRIDE;

    /// Reset the internal state of this engine.  If the optionally specified
    /// 'keepConfirming' is 'true', keep the data structures for CONFIRMs
    /// processing.
    void resetState(bool keepConfirming = false) BSLS_KEYWORD_OVERRIDE;

    /// Rebuild the internal state of this engine.  This method is invoked
    /// when the queue this engine is associated with is created from an
    /// existing one, and takes ownership of the already created handles
    /// (typically happens when the queue gets converted between local and
    /// remote).  Return zero on success, non-zero value otherwise and
    /// populate the specified `errorDescription`.  Note that
    /// `rebuildInternalState` must be called on an empty-state object
    /// (i.e., which has just been constructed, or following a call to
    /// `resetState`) after it has been configured.
    int
    rebuildInternalState(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Obtain and return a handle to this queue for the client identified
    /// with the specified `clientContext`, using the specified
    /// `handleParameters`, and invoke the specified `callback` when
    /// finished.
    mqbi::QueueHandle*
    getHandle(const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                                                          clientContext,
              const bmqp_ctrlmsg::QueueHandleParameters&  handleParameters,
              unsigned int                                upstreamSubQueueId,
              const mqbi::QueueHandle::GetHandleCallback& callback)
        BSLS_KEYWORD_OVERRIDE;

    /// Configure the specified `handle` with the specified
    /// `streamParameters` and invoke the specified `configuredCb` when
    /// finished.
    void configureHandle(
        mqbi::QueueHandle*                                 handle,
        const bmqp_ctrlmsg::StreamParameters&              streamParameters,
        const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb)
        BSLS_KEYWORD_OVERRIDE;

    /// Reconfigure the specified `handle` by releasing the specified
    /// `parameters` from its current settings and invoke the specified
    /// `releasedCb` once done.
    void
    releaseHandle(mqbi::QueueHandle*                         handle,
                  const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                  bool                                       isFinal,
                  const mqbi::QueueHandle::HandleReleasedCallback& releasedCb)
        BSLS_KEYWORD_OVERRIDE;

    /// Called when the specified `handle` is usable and ready to receive
    /// messages (usually meaning its client has become available) for the
    /// specified `upstreamSubQueueId` stream of the queue.  When this
    /// method is called, the queue engine should deliver outstanding
    /// messages to the `handle`.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void onHandleUsable(mqbi::QueueHandle* handle,
                        unsigned int upstreamSubQueueId) BSLS_KEYWORD_OVERRIDE;

    /// Called by the mqbi::Queue when a new message with the specified
    /// `msgGUID` is available on the queue and ready to be sent to eventual
    /// interested clients.
    void afterNewMessage(const bmqt::MessageGUID& msgGUID,
                         mqbi::QueueHandle* source) BSLS_KEYWORD_OVERRIDE;

    /// Called by the `mqbi::Queue` when the message identified by the
    /// specified `msgGUID` is confirmed for the specified `subQueueId`
    /// stream of the queue on behalf of the client identified by the
    /// specified `handle`.  Return a negative value on error (GUID was not
    /// found, etc.), 0 if this confirm was for the last reference to that
    /// message and it can be deleted from the queue's associated storage,
    /// or 1 if there are still references.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    int onConfirmMessage(mqbi::QueueHandle*       handle,
                         const bmqt::MessageGUID& msgGUID,
                         unsigned int subQueueId) BSLS_KEYWORD_OVERRIDE;

    /// Called by the `mqbi::Queue` when the message identified by the
    /// specified `msgGUID` is rejected for the specified
    /// `downstreamSubQueueId` stream of the queue on behalf of the client
    /// identified by the specified `handle`.  Return resulting RDA counter.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    int
    onRejectMessage(mqbi::QueueHandle*       handle,
                    const bmqt::MessageGUID& msgGUID,
                    unsigned int downstreamSubQueueId) BSLS_KEYWORD_OVERRIDE;

    /// Called by the mqbi::Queue before a message with the specified
    /// `msgGUID` is removed from the queue.
    void beforeMessageRemoved(const bmqt::MessageGUID& msgGUID)
        BSLS_KEYWORD_OVERRIDE;

    /// Called by the mqbi::Queue *after* *all* messages are removed from
    /// the storage for the client identified by the specified `appId` and
    /// `appKey` (queue has been deleted or purged by admin task, etc).
    /// QueueEngine may use this to update the positions of the QueueHandles
    /// it manages.  Note that `appKey` may be null, in which case the
    /// `purge` action is applicable to the entire queue.  Also note that
    /// `appId` must be empty if and only if `appKey` is null.
    void
    afterQueuePurged(const bsl::string&      appId,
                     const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    /// Notify this queue engine that a message has been posted and
    /// saved in the storage.
    /// See also: `mqbblp::LocalQueue::postMessage`,
    ///           `mqbblp::RemoteQueue::postMessage`
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void afterPostMessage() BSLS_KEYWORD_OVERRIDE;

    /// Given the specified 'putHeader', 'appData', 'mpi', and 'timestamp',
    /// evaluate all application subscriptions and exclude applications with
    /// negative results from message delivery.  Return 0 on success or an
    /// non-zero error code on failure.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    mqbi::StorageResult::Enum evaluateAppSubscriptions(
        const bmqp::PutHeader&              putHeader,
        const bsl::shared_ptr<bdlbb::Blob>& appData,
        const bmqp::MessagePropertiesInfo&  mpi,
        bsls::Types::Uint64                 timestamp) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual mqbi::QueueEngine)

    /// Return the reference count that should be applied to a message
    /// posted to the queue managed by this engine.
    unsigned int messageReferenceCount() const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `out` object the internal information about
    /// this queue engine and associated queue handles.
    void loadInternals(mqbcmd::QueueEngine* out) const BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
