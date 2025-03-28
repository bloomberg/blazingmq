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
#include <mqba_authenticator.h>

#include <mqbscm_version.h>

/// Implementation Notes
///====================

// MQB
#include <mqbblp_clustercatalog.h>

// BMQ

// BDE
#include <ball_log.h>

namespace BloombergLP {
namespace mqba {

namespace {
BALL_LOG_SET_NAMESPACE_CATEGORY("MQBA.AUTHENTICATOR");

const int k_AUTHENTICATION_READTIMEOUT = 3 * 60;  // 3 minutes

}

// -----------------------
// class Authenticator
// -----------------------

void Authenticator::readCallback(const bmqio::Status&           status,
                                 int*                           numNeeded,
                                 bdlbb::Blob*                   blob,
                                 const AuthenticationContextSp& context)
{
}

int Authenticator::decodeNegotiationMessage(
    bsl::ostream&                  errorDescription,
    const AuthenticationContextSp& context,
    const bdlbb::Blob&             blob)
{
    return 0;
}

int Authenticator::onAuthenticationRequest(
    bsl::ostream&                  errorDescription,
    const AuthenticationContextSp& context)
{
    return 0;
}

int Authenticator::onAuthenticationResponse(
    bsl::ostream&                  errorDescription,
    const AuthenticationContextSp& context)
{
    return 0;
}

int Authenticator::sendAuthenticationMessage(
    bsl::ostream&                           errorDescription,
    const bmqp_ctrlmsg::NegotiationMessage& message,
    const AuthenticationContextSp&          context)
{
    return 0;
}

void Authenticator::initiateOutboundAuthentication(
    const AuthenticationContextSp& context)
{
}

void Authenticator::scheduleRead(const AuthenticationContextSp& context)
{
}

// CREATORS
Authenticator::Authenticator(bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_clusterCatalog_p(0)
{
}

/// Destructor
Authenticator::~Authenticator()
{
    // NOTHING: (required because of inheritance)
}

void Authenticator::authenticate(
    mqbnet::AuthenticatorContext*                  context,
    const bsl::shared_ptr<bmqio::Channel>&         channel,
    const mqbnet::Authenticator::AuthenticationCb& authenticationCb)
{
    // Create a AuthenticationContextSp for that connection
    AuthenticationContextSp AuthenticationContext;
    AuthenticationContext.createInplace(d_allocator_p);

    AuthenticationContext->d_authenticationCb = authenticationCb;
}

}  // close package namespace
}  // close enterprise namespace
