// Copyright 2025 Bloomberg Finance L.P.
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

// mqba_authenticator.h                                           -*-C++-*-
#ifndef INCLUDED_MQBA_AUTHENTICATOR
#define INCLUDED_MQBA_AUTHENTICATOR

/// @file mqba_authenticator.h
///
/// @brief
///
///
/// Thread Safety                              {#mqba_authenticator_thread}
/// =============
///
/// The implementation must be thread safe as 'authenticate()' may be called
/// concurrently from many IO threads.

// MQB
#include <mqbconfm_messages.h>
#include <mqbnet_authenticator.h>

// BMQ
#include <bmqio_channel.h>
#include <bmqio_status.h>
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlcc_sharedobjectpool.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbblp {
class ClusterCatalog;
}
namespace mqbi {
class Dispatcher;
}

namespace mqba {

// ===================
// class Authenticator
// ===================

/// Authenticator for a BlazingMQ session with client or broker
class Authenticator : public mqbnet::Authenticator {
  public:
    // TYPES

    /// Type of a pool of shared pointers to blob
    typedef bdlcc::SharedObjectPool<
        bdlbb::Blob,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::RemoveAll<bdlbb::Blob> >
        BlobSpPool;

  private:
    typedef bsl::shared_ptr<mqbnet::AuthenticationContext>
        AuthenticationContextSp;

  private:
    // DATA

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

    BlobSpPool* d_blobSpPool_p;

    /// Cluster catalog to query for cluster information.
    mqbblp::ClusterCatalog* d_clusterCatalog_p;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator not implemented.
    Authenticator(const Authenticator&);             // = delete
    Authenticator& operator=(const Authenticator&);  // = delete

  private:
    // PRIVATE MANIPULATORS

    /// Invoked when received a `ClientIdentity` authentication message with
    /// the specified `context`.  Creates and return a Session on success,
    /// or return a null pointer and populate the specified
    /// `errorDescription` with a description of the error on failure.
    int onAuthenticationRequest(bsl::ostream& errorDescription,
                                const AuthenticationContextSp& context);

    /// Invoked when received a `BrokerResponse` authentication message with
    /// the specified `context`.  Creates and return a Session on success,
    /// or return a null pointer and populate the specified
    /// `errorDescription` with a description of the error on failure.
    int onAuthenticationResponse(bsl::ostream& errorDescription,
                                 const AuthenticationContextSp& context);

    /// Send the specified `message` to the peer associated with the
    /// specified `context` and return 0 on success, or return a non-zero
    /// code on error and populate the specified `errorDescription` with a
    /// description of the error.
    int sendAuthenticationMessage(
        bsl::ostream&                              errorDescription,
        const bmqp_ctrlmsg::AuthenticationMessage& message,
        const AuthenticationContextSp&             context);

    /// Initiate an outbound authentication (i.e., send out some authentication
    /// message and schedule a read of the response) using the specified
    /// `context`.
    /// Senario: reverse connection
    void
    initiateOutboundAuthentication(const AuthenticationContextSp& context);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Authenticator, bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Create a new `Authenticator` using the specified
    /// `bufferFactory`, `dispatcher`, `statContext`, `scheduler` and
    /// `blobSpPool` to inject in the negotiated sessions.  Use the
    /// specified `allocator` for all memory allocations.
    Authenticator(BlobSpPool* blobSpPool, bslma::Allocator* allocator);

    /// Destructor
    ~Authenticator() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Set the cluster catalog to use to the specified `value` and return a
    /// reference offering modifiable access to this object.
    Authenticator& setClusterCatalog(mqbblp::ClusterCatalog* value);

    // MANIPULATORS
    //   (virtual: mqbnet::Authenticator)

    int handleAuthenticationOnMsgType(const AuthenticationContextSp& context)
        BSLS_KEYWORD_OVERRIDE;

    /// Send out outbound authentication message or reverse connection request
    /// with the specified `context`.
    int authenticationOutboundOrReverse(const AuthenticationContextSp& context)
        BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------
// class Authenticator
// -------------------

inline Authenticator&
Authenticator::setClusterCatalog(mqbblp::ClusterCatalog* value)
{
    d_clusterCatalog_p = value;
    return *this;
}

}  // close package namespace
}  // close enterprise namespace

#endif
