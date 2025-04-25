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

// mqba_sessionnegotiator.h                                           -*-C++-*-
#ifndef INCLUDED_MQBA_SESSIONNEGOTIATOR
#define INCLUDED_MQBA_SESSIONNEGOTIATOR

/// @file mqba_sessionnegotiator.h
///
/// @brief Provide a negotiator for establishing sessions.
///
/// @bbref{mqba::SessionNegotiator} implements the @bbref{mqbnet::Negotiator}
/// interface to negotiate a connection with a BlazingMQ client or another
/// bmqbrkr.  From a @bbref{bmqio::Channel}, it will exchange negotiation
/// identity message, and create a session associated to the channel on
/// success.
///
/// Thread Safety                              {#mqba_sessionnegotiator_thread}
/// =============
/// TODO

// MQB
#include <mqbconfm_messages.h>
#include <mqbnet_negotiationcontext.h>
#include <mqbnet_negotiator.h>
#include <mqbnet_session.h>

// BMQ
#include <bmqio_channel.h>
#include <bmqio_status.h>
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlcc_objectpool.h>
#include <bdlcc_sharedobjectpool.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class EventScheduler;
}
namespace mqbblp {
class ClusterCatalog;
}
namespace mqbi {
class Dispatcher;
}
namespace mqbi {
class DomainFactory;
}
namespace bmqst {
class StatContext;
}

namespace mqba {

// =======================
// class SessionNegotiator
// =======================

/// Negotiator for a BlazingMQ session with client or broker
class SessionNegotiator : public mqbnet::Negotiator {
  public:
    // TYPES

    /// Type of a pool of shared pointers to blob
    typedef bdlcc::SharedObjectPool<
        bdlbb::Blob,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::RemoveAll<bdlbb::Blob> >
        BlobSpPool;

  private:
    // PRIVATE TYPES
    typedef bsl::shared_ptr<mqbnet::NegotiationContext> NegotiationContextSp;

  private:
    // DATA

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

    /// Buffer factory to use in constructed client sessions
    ///
    /// @todo This field should be removed once we retire the code for message
    ///       properties conversion in @bbref{mqba::ClientSession}.
    bdlbb::BlobBufferFactory* d_bufferFactory_p;

    /// Dispatcher to inject into new client session.s
    mqbi::Dispatcher* d_dispatcher_p;

    /// Domain factory to inject into new client sessions.
    mqbi::DomainFactory* d_domainFactory_p;

    /// Top-level stat context for all clients/queue stats.
    bmqst::StatContext* d_statContext_p;

    /// Shared object pool of blobs to inject into new client sessions.
    BlobSpPool* d_blobSpPool_p;

    /// Cluster catalog to query for cluster information.
    mqbblp::ClusterCatalog* d_clusterCatalog_p;

    /// Pointer to the event scheduler to use (held, not owned).
    bdlmt::EventScheduler* d_scheduler_p;

    /// The callback to invoke on received admin command.
    mqbnet::Session::AdminCommandEnqueueCb d_adminCb;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator not implemented.
    SessionNegotiator(const SessionNegotiator&);             // = delete
    SessionNegotiator& operator=(const SessionNegotiator&);  // = delete

  private:
    // PRIVATE MANIPULATORS

    /// Invoked when received a `ClientIdentity` negotiation message with
    /// the specified `context`.  Creates and return a Session on success,
    /// or return a null pointer and populate the specified
    /// `errorDescription` with a description of the error on failure.
    bsl::shared_ptr<mqbnet::Session>
    onClientIdentityMessage(bsl::ostream&               errorDescription,
                            const NegotiationContextSp& context);

    /// Invoked when received a `BrokerResponse` negotiation message with
    /// the specified `context`.  Creates and return a Session on success,
    /// or return a null pointer and populate the specified
    /// `errorDescription` with a description of the error on failure.
    bsl::shared_ptr<mqbnet::Session>
    onBrokerResponseMessage(bsl::ostream&               errorDescription,
                            const NegotiationContextSp& context);

    /// Send the specified `message` to the peer associated with the
    /// specified `context` and return 0 on success, or return a non-zero
    /// code on error and populate the specified `errorDescription` with a
    /// description of the error.
    int sendNegotiationMessage(bsl::ostream& errorDescription,
                               const bmqp_ctrlmsg::NegotiationMessage& message,
                               const NegotiationContextSp& context);

    /// Load into the specified `out` a new session created using the
    /// specified `context` and `description`; or leave `out` untouched and
    /// populate the specified `errorDescription` with a description of the
    /// error in case of failure.
    void createSession(bsl::ostream&                     errorDescription,
                       bsl::shared_ptr<mqbnet::Session>* out,
                       const NegotiationContextSp&       context,
                       const bsl::string&                description);

    /// Return true if the negotiation message in the specified `context` is
    /// for a client using a deprecated version of the libbmq SDK.
    bool
    checkIsDeprecatedSdkVersion(const mqbnet::NegotiationContext& context);

    /// Return true if the negotiation message in the specified `context` is
    /// for a client using an unsupported version of the libbmq SDK.
    bool
    checkIsUnsupportedSdkVersion(const mqbnet::NegotiationContext& context);

    /// Initiate an outbound negotiation (i.e., send out some negotiation
    /// message and schedule a read of the response) using the specified
    /// `context`.
    int initiateOutboundNegotiation(bsl::ostream& errorDescription,
                                    const NegotiationContextSp& context);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(SessionNegotiator,
                                   bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Create a new `SessionNegotiator` using the specified
    /// `bufferFactory`, `dispatcher`, `statContext`, `scheduler` and
    /// `blobSpPool` to inject in the negotiated sessions.  Use the
    /// specified `allocator` for all memory allocations.
    SessionNegotiator(bdlbb::BlobBufferFactory* bufferFactory,
                      mqbi::Dispatcher*         dispatcher,
                      bmqst::StatContext*       statContext,
                      BlobSpPool*               blobSpPool,
                      bdlmt::EventScheduler*    scheduler,
                      bslma::Allocator*         allocator);

    /// Destructor
    ~SessionNegotiator() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Set the admin command callback to use to the specified `value` and
    /// return a reference offering modifiable access to this object.
    SessionNegotiator& setAdminCommandEnqueueCallback(
        const mqbnet::Session::AdminCommandEnqueueCb& value);

    /// Set the cluster catalog to use to the specified `value` and return a
    /// reference offering modifiable access to this object.
    SessionNegotiator& setClusterCatalog(mqbblp::ClusterCatalog* value);

    /// Set the domain factory to the specified `value` and return a
    /// reference offering modifiable access to this object.
    SessionNegotiator& setDomainFactory(mqbi::DomainFactory* value);

    // MANIPULATORS
    //   (virtual: mqbnet::Negotiator)

    /// Create a `session` based on the type of initial connection message in
    /// the specified `context`.  Set `isContinueRead` to true if we want to
    /// continue reading instead of creating a session just yet.
    /// Return 0 on success, or a non-zero error code and populate the
    /// specified `errorDescription` with a description of the error otherwise.
    int createSessionOnMsgType(bsl::ostream& errorDescription,
                               bsl::shared_ptr<mqbnet::Session>* session,
                               bool*                       isContinueRead,
                               const NegotiationContextSp& context)
        BSLS_KEYWORD_OVERRIDE;

    /// Send out outbound negotiation message or reverse connection request
    /// with the specified `context`.
    /// Return 0 on success, or a non-zero error code and populate the
    /// specified `errorDescription` with a description of the error otherwise.
    int negotiateOutboundOrReverse(bsl::ostream& errorDescription,
                                   const NegotiationContextSp& context)
        BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------
// class SessionNegotiator
// -----------------------

inline SessionNegotiator& SessionNegotiator::setAdminCommandEnqueueCallback(
    const mqbnet::Session::AdminCommandEnqueueCb& value)
{
    d_adminCb = value;
    return *this;
}

inline SessionNegotiator&
SessionNegotiator::setClusterCatalog(mqbblp::ClusterCatalog* value)
{
    d_clusterCatalog_p = value;
    return *this;
}

inline SessionNegotiator&
SessionNegotiator::setDomainFactory(mqbi::DomainFactory* value)
{
    d_domainFactory_p = value;
    return *this;
}

}  // close package namespace
}  // close enterprise namespace

#endif
