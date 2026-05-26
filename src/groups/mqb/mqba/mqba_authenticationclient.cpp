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

#include <mqba_authenticationclient.h>

#include <mqbscm_version.h>

// MQB
#include <mqbplug_authncredential.h>

// BMQ
#include <bmqio_channel.h>
#include <bmqio_status.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqp_schemaeventbuilder.h>
#include <bmqu_memoutstream.h>
#include <bmqu_time.h>

// BDE
#include <bdlf_bind.h>
#include <bsl_optional.h>
#include <bslmt_lockguard.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqba {

// --------------------------
// class AuthenticationClient
// --------------------------

// PRIVATE MANIPULATORS
void AuthenticationClient::onReauthenticate()
{
    // executed by the *SCHEDULER* thread

    bmqu::MemOutStream errStream(d_allocator_p);
    const int          rc = authenticate(errStream);
    if (rc != 0) {
        BALL_LOG_ERROR << "Reauthentication failed: " << errStream.str();
    }
}

void AuthenticationClient::scheduleReauthentication(
    bsls::Types::Int64 lifetimeMs)
{
    // executed by the *IO* thread

    // Reauthenticate at 80% of the lifetime to give margin
    const bsls::Types::Int64 reauthMs = static_cast<bsls::Types::Int64>(
        static_cast<double>(lifetimeMs) * 0.8);

    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);

        d_scheduler_p->cancelEvent(&d_reauthHandle);
        d_scheduler_p->scheduleEvent(
            &d_reauthHandle,
            bsls::TimeInterval(bmqu::Time::nowMonotonicClock())
                .addMilliseconds(reauthMs),
            bdlf::BindUtil::bind(&AuthenticationClient::onReauthenticate,
                                 this));
    }

    // Guaranteed to exist, the caller has already checked
    bsl::shared_ptr<bmqio::Channel> channel = d_channel_wp.lock();
    BALL_LOG_INFO << "Scheduled reauthentication"
                  << " [peer: " << channel.get() << "]" << " in " << reauthMs
                  << " ms (lifetimeMs: " << lifetimeMs << ")";
}

// CREATORS
AuthenticationClient::AuthenticationClient(
    const mqbplug::CredentialProvider::CredentialCb& credentialCb,
    const bsl::shared_ptr<bmqio::Channel>&           channel,
    BlobSpPool*                                      blobSpPool,
    bdlmt::EventScheduler*                           scheduler,
    bslma::Allocator*                                allocator)
: d_credentialCb(bsl::allocator_arg, allocator, credentialCb)
, d_channel_wp(channel)
, d_blobSpPool_p(blobSpPool)
, d_scheduler_p(scheduler)
, d_mutex()
, d_reauthHandle()
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_credentialCb);
    BSLS_ASSERT_SAFE(channel);
    BSLS_ASSERT_SAFE(d_blobSpPool_p);
    BSLS_ASSERT_SAFE(d_scheduler_p);
    BSLS_ASSERT_SAFE(d_allocator_p);
}

AuthenticationClient::~AuthenticationClient()
{
    stop();
}

// MANIPULATORS
int AuthenticationClient::authenticate(bsl::ostream& errorDescription)
{
    // executed by the *IO* or *SCHEDULER* thread

    enum RcEnum {
        rc_SUCCESS            = 0,
        rc_CHANNEL_GONE       = -1,
        rc_CREDENTIAL_FAILURE = -2,
        rc_BUILD_FAILURE      = -3,
        rc_WRITE_FAILURE      = -4
    };

    bsl::shared_ptr<bmqio::Channel> channel = d_channel_wp.lock();
    if (!channel) {
        errorDescription << "Channel is no longer available";
        return rc_CHANNEL_GONE;  // RETURN
    }

    // Obtain credentials
    bmqu::MemOutStream                      credErrStream(d_allocator_p);
    bsl::optional<mqbplug::AuthnCredential> credential = d_credentialCb(
        credErrStream);

    if (!credential.has_value()) {
        errorDescription << "Failed to obtain credentials: "
                         << credErrStream.str();
        return rc_CREDENTIAL_FAILURE;  // RETURN
    }

    BALL_LOG_INFO << "Sending AuthenticationRequest"
                  << " [peer: " << channel.get() << "] with mechanism '"
                  << credential->mechanism() << "'";

    // Build AuthenticationRequest
    bmqp_ctrlmsg::AuthenticationMessage  message;
    bmqp_ctrlmsg::AuthenticationRequest& request =
        message.makeAuthenticationRequest();

    request.mechanism() = credential->mechanism();
    if (!credential->data().empty()) {
        request.data().makeValue(credential->data());
    }

    // Encode and send
    bmqp::SchemaEventBuilder builder(d_blobSpPool_p,
                                     bmqp::EncodingType::e_BER,
                                     d_allocator_p);

    int rc = builder.setMessage(message, bmqp::EventType::e_AUTHENTICATION);
    if (rc != 0) {
        errorDescription << "Failed building AuthenticationMessage "
                         << "[rc: " << rc << ", message: " << message << "]";
        return rc_BUILD_FAILURE;  // RETURN
    }

    bmqio::Status status;
    channel->write(&status, *builder.blob());
    if (!status) {
        errorDescription << "Failed sending AuthenticationMessage "
                         << "[status: " << status << ", message: " << message
                         << "]";
        return rc_WRITE_FAILURE;  // RETURN
    }

    return rc_SUCCESS;
}

int AuthenticationClient::handleResponse(
    bsl::ostream&                              errorDescription,
    const bmqp_ctrlmsg::AuthenticationMessage& response)
{
    // executed by the *IO* thread

    enum RcEnum {
        rc_SUCCESS                = 0,
        rc_INVALID_AUTHN_RESPONSE = -1,
        rc_AUTHENTICATION_FAILURE = -2
    };

    if (!response.isAuthenticationResponseValue()) {
        errorDescription << "Expected AuthenticationResponse but received: "
                         << response;
        return rc_INVALID_AUTHN_RESPONSE;  // RETURN
    }

    const bmqp_ctrlmsg::AuthenticationResponse& authnResponse =
        response.authenticationResponse();

    if (authnResponse.status().category() !=
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        errorDescription << "Authentication failed: "
                         << authnResponse.status().message()
                         << " [code: " << authnResponse.status().code() << "]";
        return rc_AUTHENTICATION_FAILURE;  // RETURN
    }

    bsl::shared_ptr<bmqio::Channel> channel = d_channel_wp.lock();
    if (!channel) {
        // This should never happen. The owner of this client should keep
        // the channel alive while the response is being processed.
        BALL_LOG_WARN << "Channel is no longer available";
        return rc_SUCCESS;  // RETURN
    }

    BALL_LOG_INFO << "Authentication successful [peer: " << channel.get()
                  << "], [lifetimeMs: "
                  << (authnResponse.lifetimeMs().isNull()
                          ? "N/A"
                          : bsl::to_string(authnResponse.lifetimeMs().value()))
                  << "]";

    if (authnResponse.lifetimeMs().has_value()) {
        scheduleReauthentication(authnResponse.lifetimeMs().value());
    }

    return rc_SUCCESS;
}

void AuthenticationClient::stop()
{
    // executed by *ANY* thread (during channel teardown)

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);

    d_scheduler_p->cancelEvent(&d_reauthHandle);
    d_channel_wp.reset();
}

}  // close package namespace
}  // close enterprise namespace
