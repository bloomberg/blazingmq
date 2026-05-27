// Copyright 2026 Bloomberg Finance L.P.
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

#ifndef INCLUDED_MQBA_AUTHENTICATIONCLIENT
#define INCLUDED_MQBA_AUTHENTICATIONCLIENT

/// @file mqba_authenticationclient.h
///
/// @brief Provide a client-side authenticator for an outbound connection.
///
/// @bbref{mqba::AuthenticationClient} implements the
/// @bbref{mqbnet::AuthenticationClient} interface to authenticate an
/// outbound connection to a broker.  It obtains credentials from a
/// @bbref{mqbplug::CredentialProvider::CredentialFunc} and sends an
/// @bbref{bmqp_ctrlmsg::AuthenticationRequest} to the remote broker.

// MQB
#include <mqbnet_authenticationclient.h>
#include <mqbplug_credentialprovider.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlcc_sharedobjectpool.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_mutex.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqio {
class Channel;
}
namespace mqba {

// ==========================
// class AuthenticationClient
// ==========================

/// Client-side authenticator for an outbound broker-to-broker connection.
class AuthenticationClient : public mqbnet::AuthenticationClient {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBA.AUTHENTICATIONCLIENT");

  public:
    // TYPES

    /// Pool of shared pointers to Blobs
    typedef bdlcc::SharedObjectPool<
        bdlbb::Blob,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::RemoveAll<bdlbb::Blob> >
        BlobSpPool;

  private:
    // DATA

    /// Function to obtain credentials for authentication.
    mqbplug::CredentialProvider::CredentialCb d_credentialCb;

    /// Reference to the channel being authenticated.
    bsl::weak_ptr<bmqio::Channel> d_channel_wp;

    BlobSpPool* d_blobSpPool_p;

    /// Scheduler for reauthentication timers.  Held, not owned.
    bdlmt::EventScheduler* d_scheduler_p;

    /// Mutex to protect access to EventHandle and channel.
    mutable bslmt::Mutex d_mutex;

    /// Handle for the scheduled reauthentication event, if any.
    bdlmt::EventScheduler::EventHandle d_reauthHandle;

    bslma::Allocator* d_allocator_p;

  private:
    // NOT IMPLEMENTED
    AuthenticationClient(const AuthenticationClient&) BSLS_KEYWORD_DELETED;
    AuthenticationClient&
    operator=(const AuthenticationClient&) BSLS_KEYWORD_DELETED;

  private:
    // PRIVATE MANIPULATORS

    /// Callback invoked by the scheduler when the reauthentication timer
    /// fires.  Sends a fresh AuthenticationRequest on the stored channel.
    void onReauthenticate();

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(AuthenticationClient,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `AuthenticationClient` using the specified
    /// `credentialFunc` for obtaining credentials, the specified `channel`
    /// for sending messages, the specified `blobSpPool` for building
    /// messages, and the specified `scheduler` for reauthentication timers.
    /// Use the specified `allocator` for all memory allocations.
    AuthenticationClient(
        const mqbplug::CredentialProvider::CredentialCb& credentialCb,
        const bsl::shared_ptr<bmqio::Channel>&           channel,
        BlobSpPool*                                      blobSpPool,
        bdlmt::EventScheduler*                           scheduler,
        bslma::Allocator*                                allocator);

    /// Destructor
    ~AuthenticationClient() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbnet::AuthenticationClient)

    /// Send an AuthenticationRequest using credentials from the configured
    /// provider.  Return 0 on success, or a non-zero value and populate the
    /// specified `errorDescription` with the description of any failure
    /// encountered.
    int authenticate(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified `response` received from the remote broker.
    /// Return 0 if authentication succeeded, or a non-zero value and
    /// populate the specified `errorDescription` with the description of
    /// any failure encountered.
    int handleResponse(bsl::ostream& errorDescription,
                       const bmqp_ctrlmsg::AuthenticationMessage& response)
        BSLS_KEYWORD_OVERRIDE;

    /// Stop the authentication client, cancelling any pending
    /// reauthentication timers.
    void stop() BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
