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

namespace BloombergLP {
namespace mqbnet {

// ---------------------------
// class AuthenticationContext
// ---------------------------

AuthenticationContext::AuthenticationContext(
    InitialConnectionContext* initialConnectionContext,
    bool                      isReversed,
    ConnectionType::Enum      connectionType,
    bslma::Allocator*         basicAllocator)
: d_initialConnectionContext_p(initialConnectionContext)
, d_authenticationMessage(basicAllocator)
, d_isReversed(isReversed)
, d_connectionType(connectionType)
{
    // NOTHING
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

AuthenticationContext& AuthenticationContext::setIsReversed(bool value)
{
    d_isReversed = value;
    return *this;
}

AuthenticationContext&
AuthenticationContext::setConnectionType(ConnectionType::Enum value)
{
    d_connectionType = value;
    return *this;
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

bool AuthenticationContext::isReversed() const
{
    return d_isReversed;
}

ConnectionType::Enum AuthenticationContext::connectionType() const
{
    return d_connectionType;
}

}  // namespace mqbnet
}  // namespace BloombergLP
