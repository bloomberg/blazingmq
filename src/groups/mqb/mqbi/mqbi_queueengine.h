// Copyright 2014-2023 Bloomberg Finance L.P.
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

// mqbi_queueengine.h                                                 -*-C++-*-
#ifndef INCLUDED_MQBI_QUEUEENGINE
#define INCLUDED_MQBI_QUEUEENGINE

//@PURPOSE: Provide an interface for a QueueEngine.
//
//@CLASSES:
//  mqbi::QueueEngine: Interface for a QueueEngine
//
//@DESCRIPTION: 'mqbi::QueueEngine' provide an interface for a QueueEngine.

// MQB

#include <mqbi_queue.h>
#include <mqbi_storage.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_messageguid.h>

// BDE
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_vector.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcmd {
class QueueEngine;
}

namespace mqbi {

// FORWARD DECLARATION
class QueueHandleRequesterContext;

// =================
// class QueueEngine
// =================

/// Interface for a QueueEngine.
class QueueEngine {
  public:
    // CONSTANTS

    /// This public constant represents the default appKey.
    static const mqbu::StorageKey k_DEFAULT_APP_KEY;

    // CREATORS

    /// Destructor
    virtual ~QueueEngine();

    // MANIPULATORS

    /// Configure this instance.  Return zero on success, non-zero value
    /// otherwise and populate the specified `errorDescription`.
    virtual int configure(bsl::ostream& errorDescription) = 0;

    /// Reset the internal state of this engine.  If the optionally specified
    /// 'keepConfirming' is 'true', keep the data structures for CONFIRMs
    /// processing.
    virtual void resetState(bool keepConfirming = false) = 0;

    /// Rebuild the internal state of this engine.  This method is invoked
    /// when the queue this engine is associated with is created from an
    /// existing one, and takes ownership of the already created handles
    /// (typically happens when the queue gets converted between local and
    /// remote).  Return zero on success, non-zero value otherwise and
    /// populate the specified `errorDescription`.  Note that
    /// `rebuildInternalState` must be called on an empty-state object
    /// (i.e., which has just been constructed, or following a call to
    /// `resetState`) after it has been configured.
    virtual int rebuildInternalState(bsl::ostream& errorDescription) = 0;

    /// Obtain and return a handle to this queue for the client identified
    /// with the specified `clientContext`, using the specified
    /// `handleParameters` and `upstreamSubQueueId`, and invoke the
    /// specified `callback` when finished. In case of error, return a null
    /// pointer.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual QueueHandle* getHandle(
        const bsl::shared_ptr<QueueHandleRequesterContext>& clientContext,
        const bmqp_ctrlmsg::QueueHandleParameters&          handleParameters,
        unsigned int                                        upstreamSubQueueId,
        const QueueHandle::GetHandleCallback&               callback) = 0;

    /// Configure the specified `handle` with the specified
    /// `streamParameters` and invoke the specified `configuredCb` when
    /// finished.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void configureHandle(
        QueueHandle*                                 handle,
        const bmqp_ctrlmsg::StreamParameters&        streamParameters,
        const QueueHandle::HandleConfiguredCallback& configuredCb) = 0;

    /// Reconfigure the specified `handle` by releasing the specified
    /// `parameters` from its current settings and invoke the specified
    /// `releasedCb` once done.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void
    releaseHandle(QueueHandle*                               handle,
                  const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                  bool                                       isFinal,
                  const QueueHandle::HandleReleasedCallback& releasedCb) = 0;

    /// Called when the specified `handle` is usable and ready to receive
    /// messages (usually meaning its client has become available) for the
    /// specified `upstreamSubscriptionId` subscription of the queue.  When
    /// this method is called, the queue engine should deliver outstanding
    /// messages to the `handle`.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void onHandleUsable(QueueHandle* handle,
                                unsigned int upstreamSubscriptionId) = 0;

    /// Called by the mqbi::Queue when a new message with the specified
    /// `msgGUID` is available on the queue and ready to be sent to eventual
    /// interested clients.  If available, the specified `source` points to
    /// the originator of the message.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void afterNewMessage(const bmqt::MessageGUID& msgGUID,
                                 QueueHandle*             source) = 0;

    /// Called by the `mqbi::Queue` when the message identified by the
    /// specified `msgGUID` is confirmed for the specified
    /// `upstreamSubQueueId` stream of the queue on behalf of the client
    /// identified by the specified `handle`.  Return a negative value on
    /// error (GUID was not found, etc.), 0 if this confirm was for the last
    /// reference to that message and it can be deleted from the queue's
    /// associated storage, or 1 if there are still references.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual int onConfirmMessage(mqbi::QueueHandle*       handle,
                                 const bmqt::MessageGUID& msgGUID,
                                 unsigned int upstreamSubQueueId) = 0;

    /// Called by the `mqbi::Queue` when the message identified by the
    /// specified `msgGUID` is rejected for the specified
    /// `upstreamSubQueueId` stream of the queue on behalf of the client
    /// identified by the specified `handle`.  Return resulting RDA counter.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual int onRejectMessage(mqbi::QueueHandle*       handle,
                                const bmqt::MessageGUID& msgGUID,
                                unsigned int upstreamSubQueueId) = 0;

    // TODO: This method should take an 'AppKey' because, in the case of
    //       fanout, a message could be removed from a single subStream without
    //       affecting the others.

    /// Called by the mqbi::Queue before a message with the specified
    /// `msgGUID` is removed from the queue (either it's TTL expired, it was
    /// deleted by admin task, it was confirmed by all recipients, etc).
    /// The QueueEngine may use this to update the positions of the
    /// QueueHandles it manages.
    virtual void beforeMessageRemoved(const bmqt::MessageGUID& msgGUID) = 0;

    /// Called by the mqbi::Queue *after* *all* messages are removed from
    /// the storage for the client identified by the specified `appId` and
    /// `appKey` (queue has been deleted or purged by admin task, etc).
    /// QueueEngine may use this to update the positions of the QueueHandles
    /// it manages.  Note that `appKey` may be null, in which case the
    /// `purge` action is applicable to the entire queue.  Also note that
    /// `appId` must be empty if and only if `appKey` is null.
    virtual void afterQueuePurged(const bsl::string&      appId,
                                  const mqbu::StorageKey& appKey) = 0;

    /// Periodically invoked with the current time provided in the specified
    /// `currentTimer`; can be used for regular status check, such as for
    /// ensuring messages on the queue are flowing and not accumulating.
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void onTimer(bsls::Types::Int64 currentTimer) = 0;

    /// Called after the specified `appIdKeyPair` has been dynamically
    /// registered.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void
    afterAppIdRegistered(const mqbi::Storage::AppIdKeyPair& appIdKeyPair);

    /// Called after the specified `appIdKeyPair` has been dynamically
    /// unregistered.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void
    afterAppIdUnregistered(const mqbi::Storage::AppIdKeyPair& appIdKeyPair);

    /// Given the specified 'putHeader', 'appData', 'mpi', and 'timestamp',
    /// evaluate all Auto (Application) subscriptions and exclude applications
    /// with negative results from message delivery.
    /// Return 0 on success or an non-zero error code on failure.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual StorageResult::Enum
    evaluateAutoSubscriptions(const bmqp::PutHeader&              putHeader,
                              const bsl::shared_ptr<bdlbb::Blob>& appData,
                              const bmqp::MessagePropertiesInfo&  mpi,
                              bsls::Types::Uint64 timestamp) = 0;

    // ACCESSORS

    /// Return the reference count that should be applied to a message
    /// posted to the queue managed by this engine.  Note that returned
    /// value may or may not be equal to `numOpenReaderHandles()` depending
    /// upon the specific type of this engine.
    virtual unsigned int messageReferenceCount() const = 0;

    /// Load into the specified `out` object the internal information about
    /// this queue engine and associated queue handles.
    virtual void loadInternals(mqbcmd::QueueEngine* out) const = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
