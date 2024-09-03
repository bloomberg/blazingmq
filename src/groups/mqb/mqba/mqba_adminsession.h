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

// mqba_adminsession.h                                                -*-C++-*-
#ifndef INCLUDED_MQBA_ADMINSESSION
#define INCLUDED_MQBA_ADMINSESSION

//@PURPOSE: Provide a session for interaction with BlazingMQ broker admin
// clients.
//
//@CLASSES:
//  mqba::AdminSession     : mechanism representing a session with an admin
//  mqba::AdminSessionState: VST representing the state of a session
//
//@DESCRIPTION: This component provides a mechanism, 'mqba::AdminSession', that
// allows BlazingMQ broker to send and receive messages from an admin connected
// to the broker.  'mqba::AdminSessionState' is a value semantic type holding
// the state associated to an 'mqba::Session'.

// MQB

#include <mqbi_dispatcher.h>
#include <mqbi_queue.h>
#include <mqbnet_session.h>

// BMQ
#include <bmqp_schemaeventbuilder.h>

// MWC
#include <mwcio_channel.h>
#include <mwcu_sharedresource.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlcc_objectpool.h>
#include <bdlcc_sharedobjectpool.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_utility.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bslmt {
class Semaphore;
}
namespace bmqp {
class Event;
}

namespace mqba {

// =========================
// struct AdminSessionState
// =========================

/// VST representing the state of a session
struct AdminSessionState {
  public:
    // TYPES

    /// Pool of shared pointers to Blobs
    typedef bdlcc::SharedObjectPool<
        bdlbb::Blob,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::RemoveAll<bdlbb::Blob> >
        BlobSpPool;

  public:
    // PUBLIC DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use.

    mqbi::DispatcherClientData d_dispatcherClientData;
    // Dispatcher client data associated to
    // this session.

    bdlbb::BlobBufferFactory* d_bufferFactory_p;
    // Blob buffer factory to use.

    BlobSpPool* d_blobSpPool_p;
    // Pool of shared pointers to blob to
    // use.

    bmqp::SchemaEventBuilder d_schemaEventBuilder;
    // Builder for schema messages.  To be
    // used only in client dispatcher
    // thread.

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    AdminSessionState(const AdminSessionState&);             // = delete;
    AdminSessionState& operator=(const AdminSessionState&);  // = delete;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(AdminSessionState,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor of a new session state using the specified `dispatcher`,
    /// `blobSpPool` and `bufferFactory`.  The specified `encodingType` is
    /// the encoding which the schema event builder will use.  Memory
    /// allocations are performed using the specified `allocator`.
    AdminSessionState(BlobSpPool*               blobSpPool,
                      bdlbb::BlobBufferFactory* bufferFactory,
                      bmqp::EncodingType::Enum  encodingType,
                      bslma::Allocator*         allocator);
};

// ==================
// class AdminSession
// ==================

/// A session with a BlazingMQ admin application
class AdminSession : public mqbnet::Session, public mqbi::DispatcherClient {
  private:
    // PRIVATE TYPES
    typedef bsl::function<void(void)> VoidFunctor;

  private:
    // DATA
    mwcu::SharedResource<AdminSession> d_self;
    // This object is used to avoid
    // executing a callback if the session
    // has been destroyed: this is *ONLY* to
    // be used with the callbacks that will
    // be called from outside of the
    // dispatcher's thread.

    bool d_running;
    // Show whether the session is running.

    bmqp_ctrlmsg::NegotiationMessage d_negotiationMessage;
    // Negotiation message received from the
    // remote peer.

    bmqp_ctrlmsg::ClientIdentity* d_clientIdentity_p;
    // Raw pointer to the right field in
    // 'd_negotiationMessage' (depending
    // whether it's a 'client' or a
    // 'broker').

    bsl::string d_description;
    // Short identifier for this session.

    bsl::shared_ptr<mwcio::Channel> d_channel_sp;
    // Channel associated to this session.

    AdminSessionState d_state;
    // The state associated to this session.

    bdlmt::EventScheduler* d_scheduler_p;
    // Pointer to the event scheduler to
    // use (held, not owned)

    mqbnet::Session::AdminCommandEnqueueCb d_adminCb;
    // The callback to invoke on received
    // admin command.

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    AdminSession(const AdminSession&);             // = delete;
    AdminSession& operator=(const AdminSession&);  // = delete;

  private:
    // PRIVATE MANIPULATORS

    /// Initiate the shutdown of the session and invoke the specified
    /// `callback` upon completion of (asynchronous) shutdown sequence.
    void initiateShutdownDispatched(const ShutdownCb& callback);

    /// Extract admin command from the specified `adminCmdCtrlMsg` and
    /// enqueue it for execution.
    void
    enqueueAdminCommand(const bmqp_ctrlmsg::ControlMessage& adminCmdCtrlMsg);

    /// Queue admin command results sending back to the admin client.  Can
    /// be called from any thread.  The specified `adminCommandCtrlMsg` is
    /// the initial control message containing admin command, that has been
    /// executed, `rc` is a return code representing success or error in
    /// admin command execution, `commandExecResults` is the command
    /// execution results that can be structured, non-structured or just
    /// an error message text.
    void onProcessedAdminCommand(
        const bmqp_ctrlmsg::ControlMessage& adminCommandCtrlMsg,
        int                                 rc,
        const bsl::string&                  commandExecResults);

    /// Do form the AdminCommandResponse message and send it to the admin
    /// client in response to the specified `adminCommandCtrlMsg` with the
    /// `commandExecResults` text response.
    void finalizeAdminCommand(
        const bmqp_ctrlmsg::ControlMessage& adminCommandCtrlMsg,
        const bsl::string&                  commandExecResults);

    /// Internal method to send the constructed schema message, stored in
    /// the session state, to the client.  *All* send operations from
    /// session should use this method, so it will reset the schema event
    /// builder after sending.
    void sendPacket();

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(AdminSession, bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Constructor of a new session associated to the specified `channel`
    /// and using the specified `dispatcher`, `blobSpPool`, `bufferFactory`
    /// and `scheduler`.  The specified `negotiationMessage` represents the
    /// identity received from the peer during negotiation, and the
    /// specified `sessionDescription` is the short form description of the
    /// session.  Memory allocations are performed using the specified
    /// `allocator`.  The specified `adminEnqueueCb` callback is used to
    /// enqueue admin commands to entity that is responsible for executing
    /// admin commands.
    AdminSession(const bsl::shared_ptr<mwcio::Channel>&  channel,
                 const bmqp_ctrlmsg::NegotiationMessage& negotiationMessage,
                 const bsl::string&                      sessionDescription,
                 mqbi::Dispatcher*                       dispatcher,
                 AdminSessionState::BlobSpPool*          blobSpPool,
                 bdlbb::BlobBufferFactory*               bufferFactory,
                 bdlmt::EventScheduler*                  scheduler,
                 const mqbnet::Session::AdminCommandEnqueueCb& adminEnqueueCb,
                 bslma::Allocator*                             allocator);

    /// Destructor
    ~AdminSession() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbnet::Session)

    /// Admin session doesn't use flushing mechanism, this method is defined
    /// only to fit the interface.
    void flush() BSLS_KEYWORD_OVERRIDE;

    /// Initiate the shutdown of the session and invoke the specified
    /// `callback` upon completion of (asynchronous) shutdown sequence or
    /// if the specified `timeout` is expired.
    /// The optional (temporary) specified 'supportShutdownV2' indicates
    /// shutdown V2 logic which is not applicable to `AdminSession`
    /// implementation.
    void
    initiateShutdown(const ShutdownCb&         callback,
                     const bsls::TimeInterval& timeout,
                     bool supportShutdownV2 = false) BSLS_KEYWORD_OVERRIDE;

    /// Make the session abandon any work it has.
    void invalidate() BSLS_KEYWORD_OVERRIDE;

    /// Process the specified `event` received from the optionally specified
    /// `source` node.  Note that this method is the entry point for all
    /// incoming events coming from the remote peer.
    void processEvent(const bmqp::Event&   event,
                      mqbnet::ClusterNode* source = 0) BSLS_KEYWORD_OVERRIDE;

    /// Method invoked when the channel associated with the session is going
    /// down.  The session object will be destroyed once the specified
    /// `session` goes out of scope.  The specified `isBrokerShutdown`
    /// indicates if the channel is going down from a shutdown.  This method
    /// is executed on the IO thread, so if the session object's destruction
    /// must execute some long synchronous or heavy operation, it could
    /// offload it to a separate thread, passing in the `session` to prevent
    /// destruction of the session object until the shutdown sequence
    /// completes.
    void tearDown(const bsl::shared_ptr<void>& session,
                  bool isBrokerShutdown) BSLS_KEYWORD_OVERRIDE;

    /// Implementation of the teardown process, posting on the specified
    /// `semaphore` once processing is done.
    void tearDownImpl(bslmt::Semaphore* semaphore);

    // MANIPULATORS
    //   (virtual: mqbi::DispatcherClient)

    /// Process the specified `event` routed by the dispatcher to this
    /// instance.  Dispatcher guarantees that all events to this instance
    /// are dispatched in a single thread.
    void onDispatcherEvent(const mqbi::DispatcherEvent& event)
        BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //  (virtual: mqbnet::Session)

    /// Return the channel associated to this session.
    bsl::shared_ptr<mwcio::Channel> channel() const BSLS_KEYWORD_OVERRIDE;

    /// Return the clusterNode associated to this session, or 0 if there are
    /// none.
    mqbnet::ClusterNode* clusterNode() const BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual: mqbnet::Session)

    /// Return a printable description of the client (e.g. for logging).
    const bsl::string& description() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference not offering modifiable access to the negotiation
    /// message received from the remote peer of this session during the
    /// negotiation phase.
    const bmqp_ctrlmsg::NegotiationMessage&
    negotiationMessage() const BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbi::DispatcherClient)

    /// Return a pointer to the dispatcher this client is associated with.
    mqbi::Dispatcher* dispatcher() BSLS_KEYWORD_OVERRIDE;

    /// Return a reference to the dispatcherClientData.
    mqbi::DispatcherClientData& dispatcherClientData() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual: mqbi::DispatcherClient)

    /// Return a pointer to the dispatcher this client is associated with.
    const mqbi::Dispatcher* dispatcher() const BSLS_KEYWORD_OVERRIDE;

    const mqbi::DispatcherClientData&
    dispatcherClientData() const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------
// class AdminSession
// -------------------

// MANIPULATORS
inline bsl::shared_ptr<mwcio::Channel> AdminSession::channel() const
{
    return d_channel_sp;
}

inline mqbnet::ClusterNode* AdminSession::clusterNode() const
{
    // A AdminSession has no cluster node associated
    return 0;
}

inline const bsl::string& AdminSession::description() const
{
    return d_description;
}

inline mqbi::Dispatcher* AdminSession::dispatcher()
{
    return d_state.d_dispatcherClientData.dispatcher();
}

inline const bmqp_ctrlmsg::NegotiationMessage&
AdminSession::negotiationMessage() const
{
    return d_negotiationMessage;
}

inline const mqbi::Dispatcher* AdminSession::dispatcher() const
{
    return d_state.d_dispatcherClientData.dispatcher();
}

inline const mqbi::DispatcherClientData&
AdminSession::dispatcherClientData() const
{
    return d_state.d_dispatcherClientData;
}

inline mqbi::DispatcherClientData& AdminSession::dispatcherClientData()
{
    return d_state.d_dispatcherClientData;
}

}  // close package namespace
}  // close enterprise namespace

#endif
