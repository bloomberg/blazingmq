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

// mqbnet_authenticationcontext.cpp                           -*-C++-*-
#include <mqbnet_authenticationcontext.h>

#include <mqbscm_version.h>
// MQB
#include <mqbnet_initialconnectioncontext.h>

// BDE
#include <bsla_annotations.h>
#include <bslmt_lockguard.h>
#include <bsls_atomic.h>

namespace BloombergLP {
namespace mqbnet {

// ---------------------------
// class AuthenticationContext
// ---------------------------

AuthenticationContext::AuthenticationContext(
    InitialConnectionContext*                  initialConnectionContext,
    const bmqp_ctrlmsg::AuthenticationMessage& authenticationMessage,
    bmqp::EncodingType::Enum                   authenticationEncodingType,
    const ReauthenticateCb&                    reauthenticateCb,
    State                                      state,
    ConnectionType::Enum                       connectionType,
    BSLA_UNUSED bslma::Allocator* allocator)
: d_authenticationResultSp()
, d_initialConnectionContext_p(initialConnectionContext)
, d_authenticationMessage(authenticationMessage)
, d_timeoutHandle()
, d_authenticationEncodingType(authenticationEncodingType)
, d_reauthenticateCb(reauthenticateCb)
, d_state(state)
, d_connectionType(connectionType)
{
    // NOTHING
}

AuthenticationContext& AuthenticationContext::setAuthenticationResult(
    const bsl::shared_ptr<mqbplug::AuthenticationResult>& value)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // MUTEX LOCKED

    d_authenticationResultSp = value;
    return *this;
}

AuthenticationContext& AuthenticationContext::setInitialConnectionContext(
    InitialConnectionContext* value)
{
    d_initialConnectionContext_p = value;
    return *this;
}

AuthenticationContext& AuthenticationContext::setAuthenticationMessage(
    const bmqp_ctrlmsg::AuthenticationMessage& value)
{
    d_authenticationMessage = value;
    return *this;
}

AuthenticationContext& AuthenticationContext::setAuthenticationEncodingType(
    bmqp::EncodingType::Enum value)
{
    d_authenticationEncodingType = value;
    return *this;
}

AuthenticationContext&
AuthenticationContext::setAuthenticateCb(const ReauthenticateCb& value)
{
    d_reauthenticateCb = value;
    return *this;
}

bsls::AtomicInt& AuthenticationContext::state()
{
    return d_state;
}

AuthenticationContext&
AuthenticationContext::setConnectionType(ConnectionType::Enum value)
{
    d_connectionType = value;
    return *this;
}

const bsl::shared_ptr<mqbplug::AuthenticationResult>&
AuthenticationContext::authenticationResult() const
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // MUTEX LOCKED

    return d_authenticationResultSp;
}

InitialConnectionContext*
AuthenticationContext::initialConnectionContext() const
{
    return d_initialConnectionContext_p;
}

const bmqp_ctrlmsg::AuthenticationMessage&
AuthenticationContext::authenticationMessage() const
{
    return d_authenticationMessage;
}

bmqp::EncodingType::Enum
AuthenticationContext::authenticationEncodingType() const
{
    return d_authenticationEncodingType;
}

const AuthenticationContext::ReauthenticateCb&
AuthenticationContext::reauthenticateCb() const
{
    return d_reauthenticateCb;
}

ConnectionType::Enum AuthenticationContext::connectionType() const
{
    return d_connectionType;
}

AuthenticationContext::EventHandle& AuthenticationContext::timeoutHandle()
{
    return d_timeoutHandle;
}

bslmt::Mutex& AuthenticationContext::timeoutHandleMutex()
{
    return d_timeoutHandleMutex;
}

}  // namespace mqbnet
}  // namespace BloombergLP
