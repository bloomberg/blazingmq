// Copyright 2022-2023 Bloomberg Finance L.P.
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

// mwcio_ntcchannel.h                                                 -*-C++-*-
#ifndef INCLUDED_MWCIO_NTCCHANNEL
#define INCLUDED_MWCIO_NTCCHANNEL

//@PURPOSE: Provide a a bi-directional async channel implemented using NTC.
//
//@CLASSES:
//  mwcio::Channel: Interface for a NTC-based bi-directional async channel.
//
//@DESCRIPTION: This component provides a mechanism, 'mwcio::NtcChannel',
// implemented by NTC to asynchronously send and receive arbitrary blobs of
// data.

// MWC

#include <mwcio_channel.h>
#include <mwcio_channelfactory.h>
#include <mwcio_connectoptions.h>
#include <mwcio_listenoptions.h>
#include <mwcio_status.h>
#include <mwct_propertybag.h>

// NTC
#include <ntcf_system.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlmt_signaler.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mwcio {

// =============
// class NtcRead
// =============

/// This class describes the state of a single read operation. This class
/// is not thread safe.
class NtcRead {
    // INSTANCE DATA
    mwcio::Channel::ReadCallback d_callback;
    bsl::shared_ptr<ntci::Timer> d_timer_sp;
    int                          d_numNeeded;
    bool                         d_complete;
    bslma::Allocator*            d_allocator_p;

  private:
    // NOT IMPLEMENTED
    NtcRead(const NtcRead&);
    NtcRead& operator=(const NtcRead&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(NtcRead, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new reader that invokes the specified `callback` when
    /// the specified `numNeeded` bytes are available. Optionally specify
    /// a basicAllocator used to supply memory. If `basicAllocator` is 0,
    /// the currently installed default allocator is used.
    NtcRead(const mwcio::Channel::ReadCallback& callback,
            int                                 numNeeded,
            bslma::Allocator*                   basicAllocator = 0);

    /// Destroy this object.
    ~NtcRead();

    // MANIPULATORS

    /// Set the number of bytes needed to the specified `numNeeded`.
    void setNumNeeded(int numNeeded);

    /// Set the timer to the specified `timer`.
    void setTimer(const bsl::shared_ptr<ntci::Timer>& timer);

    /// Set the operation as completed.
    void setComplete();

    /// Set the operation as completed and clear all resources.
    void clear();

    // ACCESSORS

    /// Return the number of bytes needed.
    int numNeeded() const;

    /// Return true if the operation is complete.
    bool isComplete() const;

    /// Return the read callback.
    const mwcio::Channel::ReadCallback& callback() const;

    /// Return the allocator this object was created with.
    bslma::Allocator* allocator() const;
};

// ==================
// class NtcReadQueue
// ==================

/// This class provides a queue of read operations. This class is not
/// thread safe.
class NtcReadQueue {
    // PRIVATE TYPES

    /// This typedef defines a linked list of read operations.
    typedef bsl::list<bsl::shared_ptr<mwcio::NtcRead> > List;

    // INSTANCE DATA
    List              d_list;
    bslma::Allocator* d_allocator_p;

  private:
    // NOT IMPLEMENTED
    NtcReadQueue(const NtcReadQueue&);
    NtcReadQueue& operator=(const NtcReadQueue&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(NtcReadQueue, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new read queue. Optionally specify a `basicAllocator` used
    /// to supply memory. If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    explicit NtcReadQueue(bslma::Allocator* basicAllocator = 0);

    /// Destroy this object.
    ~NtcReadQueue();

    // MANIPULATORS

    /// Append the specified `operation` to the back of the queue.
    void append(const bsl::shared_ptr<mwcio::NtcRead>& operation);

    /// Remove the specified `operation` from the queue.
    void remove(const bsl::shared_ptr<mwcio::NtcRead>& operation);

    /// Pop the front of the queue.
    void pop();

    /// Pop the front of the queue and load the result into the specified
    /// `operation`.
    void pop(bsl::shared_ptr<mwcio::NtcRead>* operation);

    /// Return the front of the queue.
    bsl::shared_ptr<mwcio::NtcRead> front();

    // ACCESSORS

    /// Return the number of operations on the queue.
    bsl::size_t size() const;

    /// Return true if the queue is empty, otherwise return false.
    bool empty() const;
};

// ================
// class NtcChannel
// ================

/// Mechanism for a bi-directional async channel implemented using NTC.
class NtcChannel : public mwcio::Channel,
                   public mwcio::ChannelFactoryOperationHandle,
                   public ntci::StreamSocketSession,
                   public bsl::enable_shared_from_this<NtcChannel> {
    // PRIVATE TYPES
    enum State {
        e_STATE_DEFAULT,
        e_STATE_OPEN,
        e_STATE_CLOSING,
        e_STATE_CLOSED
    };

    // INSTANCE DATA
    bslmt::Mutex                          d_mutex;
    bsl::shared_ptr<ntci::Interface>      d_interface_sp;
    bsl::shared_ptr<ntci::StreamSocket>   d_streamSocket_sp;
    mwcio::NtcReadQueue                   d_readQueue;
    bdlbb::Blob                           d_readCache;
    int                                   d_channelId;
    bsl::string                           d_peerUri;
    State                                 d_state;
    mwcio::ConnectOptions                 d_options;
    mwct::PropertyBag                     d_properties;
    bdlmt::Signaler<WatermarkFnType>      d_watermarkSignaler;
    bdlmt::Signaler<CloseFnType>          d_closeSignaler;
    mwcio::ChannelFactory::ResultCallback d_resultCallback;
    bslma::Allocator*                     d_allocator_p;

  private:
    // NOT IMPLEMENTED
    NtcChannel(const NtcChannel&) BSLS_KEYWORD_DELETED;
    NtcChannel& operator=(const NtcChannel&) BSLS_KEYWORD_DELETED;

  private:
    // PRIVATE MANIPULATORS

    /// Process the connection by the specified `connector` according to
    /// the specified `event`.
    void processConnect(const bsl::shared_ptr<ntci::Connector>& connector,
                        const ntca::ConnectEvent&               event);

    /// Process the timeout of the specified `read` operation by the
    /// specified `timer` according to the specified `event`.
    void processReadTimeout(const bsl::shared_ptr<mwcio::NtcRead>& read,
                            const bsl::shared_ptr<ntci::Timer>&    timer,
                            const ntca::TimerEvent&                event);

    /// Process the cancellation of the specified `read`.
    void processReadCancelled(const bsl::shared_ptr<mwcio::NtcRead>& read);

    /// Process the condition that the size of the read queue is greater
    /// than or equal to the read queue low watermark.
    void processReadQueueLowWatermark(
        const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
        const ntca::ReadQueueEvent& event) BSLS_KEYWORD_OVERRIDE;

    /// Process the condition that the size of the write queue has been
    /// drained down to less than or equal to the write queue low watermark.
    /// This condition will not occur until the write queue high watermark
    /// condition occurs. The write queue low watermark conditions and the
    /// high watermark conditions are guaranteed to occur serially.
    void processWriteQueueLowWatermark(
        const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
        const ntca::WriteQueueEvent& event) BSLS_KEYWORD_OVERRIDE;

    /// Process the condition that the size of the write queue is greater
    /// than the write queue high watermark. This condition will occur the
    /// first time the write queue high watermark has been reached but
    /// then will not subsequently ooccur until the write queue low
    /// watermark. The write queue low watermark conditions and the
    /// high watermark conditions are guaranteed to occur serially.
    void processWriteQueueHighWatermark(
        const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
        const ntca::WriteQueueEvent& event) BSLS_KEYWORD_OVERRIDE;

    /// Process the initiation of the shutdown sequence from the specified
    /// `origin`.
    void processShutdownInitiated(
        const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
        const ntca::ShutdownEvent& event) BSLS_KEYWORD_OVERRIDE;

    /// Process the socket being shut down for reading.
    void processShutdownReceive(
        const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
        const ntca::ShutdownEvent& event) BSLS_KEYWORD_OVERRIDE;

    /// Process the socket being shut down for writing.
    void processShutdownSend(
        const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
        const ntca::ShutdownEvent& event) BSLS_KEYWORD_OVERRIDE;

    /// Process the completion of the shutdown sequence.
    void processShutdownComplete(
        const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
        const ntca::ShutdownEvent& event) BSLS_KEYWORD_OVERRIDE;

    /// Process the closure of the socket.
    void processClose(const mwcio::Status& status);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(NtcChannel, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new channel implemented by the specified `interface`.
    /// Optionally specify a `basicAllocator` used to supply memory. If
    /// 'basicAllocator is 0, the currently installed default allocator
    /// is used.
    explicit NtcChannel(
        const bsl::shared_ptr<ntci::Interface>&      interface,
        const mwcio::ChannelFactory::ResultCallback& resultCallback,
        bslma::Allocator*                            basicAllocator = 0);

    /// Destroy this object.
    ~NtcChannel() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Attempt to establish a connection according to the specified
    /// `options` (whose meaning is implementation-defined), and invoke the
    /// underlying result callback when it is created or the connection
    /// attempt fails. Return `e_SUCCESS` on success or a failure
    /// StatusCategory on error, populating the optionally-specified
    /// `status` with more detailed error information.  Note that if
    /// `options.autoReconnect()` is true, then the client expects for the
    /// connection to be automatically recreated whenever it receives
    /// a `ChannelFactoryEvent::e_CONNECT_FAILED` or whenever the channel
    /// produced as a result of this connection is closed.  If
    /// `options.autoReconnect()` is `true` and the implementing
    /// `ChannelFactory` doesn't provide this behavior, it must fail the
    /// connection immediately.
    int connect(mwcio::Status* status, const mwcio::ConnectOptions& options);

    /// Import the specified `streamSocket` and invoke the underlying result
    /// callback.
    void import(const bsl::shared_ptr<ntci::StreamSocket>& streamSocket);

    /// Initiate an asynchronous (timed) read operation on this channel, or
    /// append this request to the currently pending requests if an
    /// asynchronous read operation was already initiated, with an
    /// associated specified relative `timeout`.  When at least the
    /// specified `numBytes` of data are available after all previous
    /// requests have been processed, if any, or when the timeout is
    /// reached, the specified `readCallback` will be invoked (with
    /// `category() == e_SUCCESS` or `category() == e_TIMEOUT`,
    /// respectively).  Return `e_SUCCESS` on success, or a different value
    /// on failure, populating the optionally specified `status` with
    /// additional information about the failure.
    void read(Status*                   status,
              int                       numBytes,
              const ReadCallback&       readCallback,
              const bsls::TimeInterval& timeout) BSLS_KEYWORD_OVERRIDE;

    /// Enqueue the specified message `blob` to be written to this channel.
    /// Optionally provide `highWaterMark` to specify the maximum data size
    /// that can be enqueued.  If `highWaterMark` is not specified then
    /// INT_MAX is used.  Return 0 on success, and a non-zero value
    /// otherwise.  On error, the return value may equal to one of the
    /// enumerators in `mwcio_ChannelStatus::Type`.  Note that success does
    /// not imply that the data has been written or will be successfully
    /// written to the underlying stream used by this channel.  Also note
    /// that in addition to `highWatermark` the enqueued portion must also
    /// be less than a high watermark value supplied at the construction of
    /// this channel for the write to succeed.
    void write(Status*            status,
               const bdlbb::Blob& blob,
               bsls::Types::Int64 watermark) BSLS_KEYWORD_OVERRIDE;

    /// Cancel the operation.
    void cancel() BSLS_KEYWORD_OVERRIDE;

    /// Cancel all pending read requests, and invoke their read callbacks
    /// with a `mwcio::ChannelStatus::e_CANCELED` status.  Note that if the
    /// channel is active, the read callbacks are invoked in the thread in
    /// which the channel's data callbacks are invoked, else they are
    /// invoked in the thread calling `cancelRead`.
    void cancelRead() BSLS_KEYWORD_OVERRIDE;

    /// Shutdown this channel, and cancel all pending read requests (but do
    /// not invoke them).  Pass the specified `status` to any registered
    /// `CloseFn`s.
    void close(const Status& status) BSLS_KEYWORD_OVERRIDE;

    /// Execute the specified `cb` serialized with calls to any registered
    /// read callbacks, or any `close` or `watermark` event handlers for
    /// this channel.  Return `0` on success or a negative value if the `cb`
    /// could not be enqueued for execution.
    int execute(const ExecuteCb& cb) BSLS_KEYWORD_OVERRIDE;

    /// Register the specified `cb` to be invoked when a `close` event
    /// occurs for this channel.  Return a `bdlmt::SignalerConnection`
    /// object than can be used to unregister the callback.
    bdlmt::SignalerConnection onClose(const CloseFn& cb) BSLS_KEYWORD_OVERRIDE;

    /// Register the specified `cb` to be invoked when a `close` event
    /// occurs for this channel.  Invoke the `cb` as part of the specified
    /// `group`. Return a `bdlmt::SignalerConnection` object than can be
    /// used to unregister the callback.
    bdlmt::SignalerConnection onClose(const CloseFn& cb, int group);

    /// Register the specified `cb` to be invoked when a `watermark` event
    /// occurs for this channel.  Return a `bdlmt::SignalerConnection`
    /// object than can be used to unregister the callback.
    bdlmt::SignalerConnection
    onWatermark(const WatermarkFn& cb) BSLS_KEYWORD_OVERRIDE;

    /// Return a reference providing modifiable access to the properties of
    /// this Channel.
    mwct::PropertyBag& properties() BSLS_KEYWORD_OVERRIDE;

    /// Set the channel ID to the specified `channelId`.
    void setChannelId(int channelId);

    /// Set the write queue low watermark to the specified `lowWatermark`.
    void setWriteQueueLowWatermark(int lowWatermark);

    /// Set the write queue high watermark to the specified `highWatermark`.
    void setWriteQueueHighWatermark(int highWatermark);

    // ACCESSORS

    /// Return the channel ID.
    int channelId() const;

    /// Load into the specified `result` the endpoint of the peer.
    ntsa::Endpoint peerEndpoint() const;

    /// Return the URI of the "remote" end of this channel.  It is up to the
    /// underlying implementation to define the format of the returned URI.
    const bsl::string& peerUri() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference providing modifiable access to the properties of
    /// this Channel.
    const mwct::PropertyBag& properties() const BSLS_KEYWORD_OVERRIDE;

    /// Return the allocator this object was created with.
    bslma::Allocator* allocator() const;
};

// =====================
// struct NtcChannelUtil
// =====================

/// This struct provides utilities for channels.
struct NtcChannelUtil {
    // CLASS METHODS

    /// Load into the specified `status`, if defined, the description of
    /// the specified `error` assigned to the specified `category` that
    /// was detected when performing the specified `operation`.
    static void fail(Status*                     status,
                     mwcio::StatusCategory::Enum category,
                     const bslstl::StringRef&    operation,
                     const ntsa::Error&          error);
};

// =================
// class NtcListener
// =================

/// This class provides a handle to an ongoing `listen` operation of
/// a channel factory.
class NtcListener : public mwcio::ChannelFactoryOperationHandle,
                    public bsl::enable_shared_from_this<NtcListener> {
  public:
    // TYPES

    /// This typedef defines the signature of a function invoked when the
    /// listener is closed.
    typedef void CloseFnType(const mwcio::Status& status);

    /// This typedef defins a function invoked when the listener is closed.
    typedef bsl::function<CloseFnType> CloseFn;

  private:
    // PRIVATE TYPES
    enum State {
        e_STATE_DEFAULT,
        e_STATE_OPEN,
        e_STATE_CLOSING,
        e_STATE_CLOSED
    };

    // INSTANCE DATA
    bslmt::Mutex                          d_mutex;
    bsl::shared_ptr<ntci::Interface>      d_interface_sp;
    bsl::shared_ptr<ntci::ListenerSocket> d_listenerSocket_sp;
    bsl::string                           d_localUri;
    State                                 d_state;
    mwcio::ListenOptions                  d_options;
    mwct::PropertyBag                     d_properties;
    bdlmt::Signaler<CloseFnType>          d_closeSignaler;
    mwcio::ChannelFactory::ResultCallback d_resultCallback;
    bslma::Allocator*                     d_allocator_p;

  private:
    // NOT IMPLEMENTED
    NtcListener(const NtcListener&) BSLS_KEYWORD_DELETED;
    NtcListener& operator=(const NtcListener&) BSLS_KEYWORD_DELETED;

  private:
    // PRIVATE MANIPULATORS

    /// Process the acceptance of the specified `streamSocket` by the
    /// specified `acceptor` according to the specified `event`.
    void processAccept(const bsl::shared_ptr<ntci::Acceptor>&     acceptor,
                       const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
                       const ntca::AcceptEvent&                   event);

    /// Process the closure of the socket.
    void processClose(const mwcio::Status& status);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(NtcListener, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new listener implemented using the specified `interface`.
    /// Invoke the specified `resultCallback` when the channel is accepted.
    /// Optionally specified a `basicAllocator` used to supply memory. If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit NtcListener(
        const bsl::shared_ptr<ntci::Interface>&      interface,
        const mwcio::ChannelFactory::ResultCallback& resultCallback,
        bslma::Allocator*                            basicAllocator = 0);

    /// Destroy this object.
    ~NtcListener() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Listen for connections according to the specified `options` (whose
    /// meaning is implementation-defined), and invoke the underlying result
    /// callback when they are accepted. Return `e_SUCCESS` on success or
    /// a failure StatusCategory on error, populating the
    /// optionally-specified `status` with more detailed error information.
    int listen(mwcio::Status* status, const mwcio::ListenOptions& options);

    /// Cancel the operation.
    void cancel() BSLS_KEYWORD_OVERRIDE;

    /// Register the specified `cb` to be invoked when a `close` event
    /// occurs for this channel.  Return a `bdlmt::SignalerConnection`
    /// object than can be used to unregister the callback.
    bdlmt::SignalerConnection onClose(const CloseFn& cb);

    /// Register the specified `cb` to be invoked when a `close` event
    /// occurs for this channel.  Invoke the `cb` as part of the specified
    /// `group`. Return a `bdlmt::SignalerConnection` object than can be
    /// used to unregister the callback.
    bdlmt::SignalerConnection onClose(const CloseFn& cb, int group);

    /// Return a reference providing modifiable access to the properties of
    /// this object.
    mwct::PropertyBag& properties() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the URI of the "local" end of this listener.  It is up to the
    /// underlying implementation to define the format of the returned URI.
    const bsl::string& localUri() const;

    /// Return a reference providing const access to the properties of this
    /// object.
    const mwct::PropertyBag& properties() const BSLS_KEYWORD_OVERRIDE;

    /// Return the allocator this object was created with.
    bslma::Allocator* allocator() const;
};

// ======================
// struct NtcListenerUtil
// ======================

/// This struct provides utilities for listeners.
struct NtcListenerUtil {
    // CLASS METHODS

    /// Return a reference providing const access to the name of the
    /// property used to define the listen backlog of a ListenOptions.
    /// This property must contain an `Integer`.
    static bslstl::StringRef listenBacklogProperty();

    /// Return a reference providing const access to the name of the
    /// property that will be returned on the handle returned from
    /// `NtcChannelFactory::listen` with an integer property containing the
    /// port to which the listening socket is bound.
    static bslstl::StringRef listenPortProperty();

    /// Load into the specified `status`, if defined, the description of
    /// the specified `error` assigned to the specified `category` that
    /// was detected when performing the specified `operation`.
    static void fail(Status*                     status,
                     mwcio::StatusCategory::Enum category,
                     const bslstl::StringRef&    operation,
                     const ntsa::Error&          error);
};

}  // close package namespace
}  // close enterprise namespace

#endif
