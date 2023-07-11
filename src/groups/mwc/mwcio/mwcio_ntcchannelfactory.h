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

// mwcio_ntcchannelfactory.h                                          -*-C++-*-
#ifndef INCLUDED_MWCIO_NTCCHANNELFACTORY
#define INCLUDED_MWCIO_NTCCHANNELFACTORY

//@PURPOSE: Provide a 'mwcio::NtcChannel' factory.
//
//@CLASSES:
//  mwcio::NtcChannelFactory: Factory for mwcio::NtcChannel' objects
//
//@SEE_ALSO:
//  mwcio_channelfactory
//
//@DESCRIPTION: This component defines a mechanism, 'mwcio::NtcChannelFactory',
// that implements the 'mwcio::ChannelFactory' protocol to produce and manage
// 'mwcio::NtcChannel' objects that implement the 'mwcio::Channel' protocol.

// MWC

#include <mwcio_channelfactory.h>
#include <mwcio_connectoptions.h>
#include <mwcio_listenoptions.h>
#include <mwcio_ntcchannel.h>

// NTF
#include <ntca_interfaceconfig.h>
#include <ntci_interface.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlcc_objectcatalog.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_memory.h>
#include <bslmt_condition.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>

namespace BloombergLP {
namespace mwcio {

// =======================
// class NtcChannelFactory
// =======================

/// This class provides a factory of `mwcio::NtcChannel` objects.
class NtcChannelFactory : public mwcio::ChannelFactory {
  public:
    // TYPES

    /// This typedef defines the signature of a function invoked when a
    /// channel has been created.
    typedef void CreateFnType(
        const bsl::shared_ptr<mwcio::NtcChannel>&                    channel,
        const bsl::shared_ptr<mwcio::ChannelFactoryOperationHandle>& handle);

    /// This typedef defines a function invoked when a channel has been
    /// created.
    typedef bsl::function<CreateFnType> CreateFn;

    /// This typedef defines the signature of a function invoked when a
    /// channel limit has been reached.
    typedef void LimitFnType();

    /// This typedef defines a function invoked when a channel limit has
    /// been reached.
    typedef bsl::function<LimitFnType> LimitFn;

  private:
    // PRIVATE TYPES

    /// This typedef defines an entry in a catalog of listeners.
    typedef bsl::shared_ptr<mwcio::NtcListener> ListenerEntry;

    /// This typedef defines a catalog of listeners.
    typedef bdlcc::ObjectCatalog<ListenerEntry> ListenerCatalog;

    /// This typedef defines an iterator over a catalog of listeners.
    typedef bdlcc::ObjectCatalogIter<ListenerEntry> ListenerIterator;

    /// This typedef defines an entry in a catalog of channels.
    typedef bsl::shared_ptr<mwcio::NtcChannel> ChannelEntry;

    /// This typedef defines a catalog of channels.
    typedef bdlcc::ObjectCatalog<ChannelEntry> ChannelCatalog;

    /// This typedef defines an iterator over a catalog of channels.
    typedef bdlcc::ObjectCatalogIter<ChannelEntry> ChannelIterator;

    enum State {
        e_STATE_DEFAULT,
        e_STATE_STARTED,
        e_STATE_STOPPING,
        e_STATE_STOPPED
    };

    // INSTANCE DATA
    bsl::shared_ptr<ntci::Interface> d_interface_sp;
    ListenerCatalog                  d_listeners;
    ChannelCatalog                   d_channels;
    bdlmt::Signaler<CreateFnType>    d_createSignaler;
    bdlmt::Signaler<LimitFnType>     d_limitSignaler;
    bool                             d_owned;
    bslmt::Mutex                     d_stateMutex;
    bslmt::Condition                 d_stateCondition;
    State                            d_state;
    bslma::Allocator*                d_allocator_p;

  private:
    // NOT IMPLEMENTED
    NtcChannelFactory(const NtcChannelFactory&) BSLS_KEYWORD_DELETED;
    NtcChannelFactory&
    operator=(const NtcChannelFactory&) BSLS_KEYWORD_DELETED;

  private:
    // PRIVATE MANIPULATORS

    /// Process the specified `event` having the specified `status` for
    /// the specified `channel` and invoke the specified `callback`.
    void processListenerResult(
        mwcio::ChannelFactoryEvent::Enum             event,
        const mwcio::Status&                         status,
        const bsl::shared_ptr<mwcio::Channel>&       channel,
        const mwcio::ChannelFactory::ResultCallback& callback);

    /// Process the closure of the channel identified by the specified
    /// `handle`.
    void processListenerClosed(int handle);

    /// Process the specified `event` having the specified `status` for
    /// the specified `channel` and invoke the specified `callback`.
    void processChannelResult(
        mwcio::ChannelFactoryEvent::Enum             event,
        const mwcio::Status&                         status,
        const bsl::shared_ptr<mwcio::Channel>&       channel,
        const mwcio::ChannelFactory::ResultCallback& callback);

    /// Process the closure of the listener identified by the specified
    /// `handle`.
    void processChannelClosed(int handle);

  public:
    // PUBLIC TYPES

    /// This typedef defines an operation handle for an operation on a
    /// channel factory.
    typedef mwcio::ChannelFactoryOperationHandle OpHandle;

    // CREATORS

    /// Create a new NTC channel factory using the specified `interface` to
    /// create asynchronous sockets and timers. Optionally specify
    /// a `basicAllocator` used to supply memory. If `basicAllocator` is 0,
    /// the currently installed default allocator is used. Note that the
    /// caller is responsible for starting and stopping the `interface`.
    explicit NtcChannelFactory(
        const bsl::shared_ptr<ntci::Interface>& interface,
        bslma::Allocator*                       basicAllocator = 0);

    /// Create a new NTC channel factory using the specified
    /// `interfaceConfig` for the mechanism to create asynchronous sockets
    /// and timers.  Allocate blob buffers when receiving data using the
    /// specified `blobBufferFactory`. Optionally specify a `basicAllocator`
    /// used to supply memory. If `basicAllocator` is 0, the currently
    /// installed default allocator is used.
    explicit NtcChannelFactory(const ntca::InterfaceConfig& interfaceConfig,
                               bdlbb::BlobBufferFactory*    blobBufferFactory,
                               bslma::Allocator* basicAllocator = 0);

    /// Destroy this object.
    ~NtcChannelFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Start the channel factory. Return 0 on success and a non-zero value
    /// otherwise.
    int start();

    /// Stop the channel factory. Note that the behavior is undefined unless
    /// the thread calling this function is the same thread that called
    /// `start()` and is not one of the I/O threads used by this object.
    void stop();

    /// Listen for connections according to the specified `options` (whose
    /// meaning is implementation-defined), and invoke the specified `cb`
    /// when they are created or a connection attempt fails.  Load into the
    /// optionally-specified `handle` a handle that can be used to cancel
    /// this operation.  Return `e_SUCCESS` on success or a failure
    /// StatusCategory on error, populating the optionally-specified
    /// `status` with more detailed error information.
    void listen(Status*                      status,
                bslma::ManagedPtr<OpHandle>* handle,
                const ListenOptions&         options,
                const ResultCallback&        cb) BSLS_KEYWORD_OVERRIDE;

    /// Attempt to establish a connection according to the specified
    /// `options` (whose meaning is implementation-defined), and invoke the
    /// specified `cb` when it is created or the connection attempt fails.
    /// Load into the optionally-specified `handle` a handle that can be
    /// used to cancel this operation.  Return `e_SUCCESS` on success or a
    /// failure StatusCategory on error, populating the
    /// optionally-specified `status` with more detailed error information.
    /// Note that if `options.autoReconnect()` is true, then the client
    /// expects for the connection to be automatically recreated whenever
    /// it receives a `ChannelFactoryEvent::e_CONNECT_FAILED` or whenever
    /// the channel produced as a result of this connection is closed.  If
    /// `options.autoReconnect()` is `true` and the implementing
    /// `ChannelFactory` doesn't provide this behavior, it must fail the
    /// connection immediately.
    void connect(Status*                      status,
                 bslma::ManagedPtr<OpHandle>* handle,
                 const ConnectOptions&        options,
                 const ResultCallback&        cb) BSLS_KEYWORD_OVERRIDE;

    /// Register the specified `cb` to be invoked when a channel is
    /// created. Return a `bdlmt::SignalerConnection` object than can be
    /// used to unregister the callback.
    bdlmt::SignalerConnection onCreate(const CreateFn& cb);

    /// Register the specified `cb` to be invoked when a channel limit is
    /// reached. Return a `bdlmt::SignalerConnection` object than can be
    /// used to unregister the callback.
    bdlmt::SignalerConnection onLimit(const LimitFn& cb);

    /// Load into the specified `result` the channel having the specified
    /// `channelId`. Return 0 on success and a non-zero value otherwise.
    int lookupChannel(bsl::shared_ptr<mwcio::NtcChannel>* result,
                      int                                 channelId);

    /// Invoke the specified `visitor` once for every currently active
    /// channel managed by this object.  The `visitor` will be invoked with
    /// a single argument, `const bsl::shared_ptr<mwcio::NtzChannel>&`.
    /// Note that it is unspecified whether channels created or closed
    /// during the execution of this function will be included.
    template <typename VISITOR>
    void visitChannels(VISITOR& visitor);
};

// ============================
// struct NtcChannelFactoryUtil
// ============================

/// Utility struct for holding functions useful when dealing with
/// NtzChannelFactory objects.
struct NtcChannelFactoryUtil {
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
};

// -----------------------
// class NtcChannelFactory
// -----------------------

template <typename VISITOR>
void NtcChannelFactory::visitChannels(VISITOR& visitor)
{
    typedef bsl::vector<bsl::shared_ptr<mwcio::NtcChannel> > ChannelVector;
    ChannelVector                                            channels;

    {
        ChannelIterator iterator(d_channels);
        while (iterator) {
            channels.push_back(iterator.value());
            ++iterator;
        }
    }

    for (ChannelVector::const_iterator it = channels.begin();
         it != channels.end();
         ++it) {
        visitor(*it);
    }
}

}  // close package namespace
}  // close enterprise namespace

#endif
