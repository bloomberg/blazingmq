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

// mqbblp_localqueue.h                                                -*-C++-*-
#ifndef INCLUDED_MQBBLP_LOCALQUEUE
#define INCLUDED_MQBBLP_LOCALQUEUE

//@PURPOSE: Provide a queue implementation for a queue managed by this broker.
//
//@CLASSES:
//
//
//@DESCRIPTION:

// MQB

#include <mqbblp_queuestate.h>
#include <mqbi_dispatcher.h>
#include <mqbi_queue.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqt_messageguid.h>

#include <bmqu_throttledaction.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlmt_throttle.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_cpp11.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcmd {
class LocalQueue;
}
namespace mqbcmd {
class PurgeQueueResult;
}
namespace mqbi {
class Domain;
}
namespace mqbi {
class QueueEngine;
}

namespace mqbblp {

// ================
// class LocalQueue
// ================

class LocalQueue BSLS_CPP11_FINAL {
    // TBD

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.LOCALQUEUE");

  private:
    // DATA
    bslma::Allocator*                    d_allocator_p;
    QueueState*                          d_state_p;
    bslma::ManagedPtr<mqbi::QueueEngine> d_queueEngine_mp;
    bmqu::ThrottledActionParams          d_throttledFailedPutMessages;
    bdlmt::Throttle                      d_throttledDuplicateMessages;
    // Throttler for duplicates.
    bool d_haveStrongConsistency;

  private:
    // NOT IMPLEMENTED
    LocalQueue(const LocalQueue& other) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented
    LocalQueue& operator=(const LocalQueue& other) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(LocalQueue, bslma::UsesBslmaAllocator)

    // CREATORS
    LocalQueue(QueueState* state, bslma::Allocator* allocator);

    // MANIPULATORS

    /// Configure this queue instance, with `isReconfigure` flag indicating
    /// queue is being reconfigured, and populate `errorDescription` with a
    /// human readable string in case of an error.  Return zero on success
    /// and non-zero value otherwise.  Behavior is undefined unless this
    /// function is invoked from queue-dispatcher thread.
    int configure(bsl::ostream& errorDescription, bool isReconfigure);

    /// Return the queue engine used by this queue.
    mqbi::QueueEngine* queueEngine();

    /// Reset the state of this object.
    void resetState();

    int importState(bsl::ostream& errorDescription);

    /// Close this queue.
    void close();

    /// Obtain a handle to this queue, for the client represented by the
    /// specified `clientContext` and using the specified `handleParameters`
    /// and `upstreamSubQueueId`.  Invoke the specified `callback` with the
    /// result.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void getHandle(const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                                                              clientContext,
                   const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                   unsigned int upstreamSubQueueId,
                   const mqbi::QueueHandle::GetHandleCallback& callback);

    /// Configure the specified `handle` with the specified
    /// `streamParameters`, and invoke the specified `configuredCb` callback
    /// when finished.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void configureHandle(
        mqbi::QueueHandle*                                 handle,
        const bmqp_ctrlmsg::StreamParameters&              streamParameters,
        const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb);

    /// Release the specified `handle`.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void
    releaseHandle(mqbi::QueueHandle*                         handle,
                  const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                  bool                                       isFinal,
                  const mqbi::QueueHandle::HandleReleasedCallback& releasedCb);

    /// Called by the `Dispatcher` when it has the specified `event` to
    /// deliver to the client.
    void onDispatcherEvent(const mqbi::DispatcherEvent& event);

    /// Called by the dispatcher to flush any pending operation; mainly used
    /// to provide batch and nagling mechanism.
    void flush();

    /// Add the message with the specified `msgGUID`, specified `appData` as
    /// application payload and specified `options` to this queue.  The
    /// specified `source` correspond to the producer of the message, and
    /// the specified `correlationId` is used to notify back the `source`
    /// when either the message failed to be posted, or was successfully
    /// posted (the time at which a success confirm is generated depends on
    /// the configuration and the SLA, whether it's when the message is
    /// locally saved, or when one, multiple or all remove member of the
    /// cluster have received it).
    void postMessage(const bmqp::PutHeader&              putHeader,
                     const bsl::shared_ptr<bdlbb::Blob>& appData,
                     const bsl::shared_ptr<bdlbb::Blob>& options,
                     mqbi::QueueHandle*                  source);

    /// Called when a message with the specified `msgGUID` and `blob`
    /// associated payload is pushed to this queue.  Note that depending
    /// upon the location of the queue instance, `blob` may be empty.
    void onPushMessage(const bmqt::MessageGUID&            msgGUID,
                       const bsl::shared_ptr<bdlbb::Blob>& blob);

    /// Invoked by the Data Store when it receives quorum Receipts for the
    /// specified `msgGUID`.  Send ACK to the specified `qH` if it is
    /// present in the queue handle catalog.  Update ACK time stats using
    /// the specified `arrivalTimepoint`.
    ///
    /// THREAD: This method is called from the Storage dispatcher thread.
    void onReceipt(const bmqt::MessageGUID&  msgGUID,
                   mqbi::QueueHandle*        qH,
                   const bsls::Types::Int64& arrivalTimepoint);

    /// Invoked by the Data Store when it removes (times out waiting for
    /// quorum Receipts for) a message with the specified `msgGUID`.  Send
    /// NACK with the specified `result` to the specified `qH` if it is
    /// present in the queue handle catalog.
    ///
    /// THREAD: This method is called from the Storage dispatcher thread.
    void onRemoval(const bmqt::MessageGUID& msgGUID,
                   mqbi::QueueHandle*       qH,
                   bmqt::AckResult::Enum    result);

    /// Invoked by the Data Store when it transmit all cached replication
    /// records.  Switch from PUT processing to PUSH delivery.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void deliverIfNeeded();

    /// Confirm the message with the specified `msgGUID` for the specified
    /// `upstreamSubQueueId` stream of the queue on behalf of the client
    /// identified by the specified `source`.  Also note that since there
    /// are no `ack of confirms`, this method is void, and will eventually
    /// throttle warnings if the `msgGUID` is invalid.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void confirmMessage(const bmqt::MessageGUID& msgGUID,
                        unsigned int             upstreamSubQueueId,
                        mqbi::QueueHandle*       source);

    /// Reject the message with the specified `msgGUID` for the specified
    /// `upstreamSubQueueId` stream of the queue on the specified `source`.
    ///  Return resulting RDA counter.
    ///
    /// THREAD: this method can be called from any thread and is responsible
    ///         for calling the corresponding method on the `Queue`, on the
    ///         Queue's dispatcher thread.
    int rejectMessage(const bmqt::MessageGUID& msgGUID,
                      unsigned int             upstreamSubQueueId,
                      mqbi::QueueHandle*       source);

    // ACCESSORS

    /// Load into the specified `out` object the internal details about this
    /// queue.
    void loadInternals(mqbcmd::LocalQueue* out) const;

    /// Return the domain this queue belongs to.
    mqbi::Domain* domain() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------
// class LocalQueue
// ----------------

inline mqbi::QueueEngine* LocalQueue::queueEngine()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    return d_queueEngine_mp.get();
}

}  // close package namespace
}  // close enterprise namespace

#endif
