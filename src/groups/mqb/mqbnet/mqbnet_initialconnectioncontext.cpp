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

// mqbnet_initialconnectioncontext.cpp                       -*-C++-*-
#include <mqbnet_initialconnectioncontext.h>

#include <mqbscm_version.h>

namespace BloombergLP {
namespace mqbnet {

// -------------------------------------
// class InitialConnectionContext
// -------------------------------------

InitialConnectionContext::InitialConnectionContext(bool isIncoming)
: d_isIncoming(isIncoming)
, d_resultState_p(0)
, d_userData_p(0)
{
    // NOTHING
}

InitialConnectionContext& InitialConnectionContext::setUserData(void* value)
{
    d_userData_p = value;
    return *this;
}

InitialConnectionContext& InitialConnectionContext::setResultState(void* value)
{
    d_resultState_p = value;
    return *this;
}

InitialConnectionContext& InitialConnectionContext::setChannel(
    const bsl::shared_ptr<bmqio::Channel>& value)
{
    d_channelSp = value;
    return *this;
}

InitialConnectionContext&
InitialConnectionContext::setInitialConnectionCompleteCb(
    const InitialConnectionCompleteCb& value)
{
    d_initialConnectionCompleteCb = value;
    return *this;
}

InitialConnectionContext& InitialConnectionContext::setNegotiationContext(
    const bsl::shared_ptr<NegotiationContext>& value)
{
    d_negotiationCtxSp = value;
    return *this;
}

bool InitialConnectionContext::isIncoming() const
{
    return d_isIncoming;
}

void* InitialConnectionContext::userData() const
{
    return d_userData_p;
}

void* InitialConnectionContext::resultState() const
{
    return d_resultState_p;
}

const bsl::shared_ptr<bmqio::Channel>&
InitialConnectionContext::channel() const
{
    return d_channelSp;
}

const InitialConnectionContext::InitialConnectionCompleteCb&
InitialConnectionContext::initialConnectionCompleteCb() const
{
    return d_initialConnectionCompleteCb;
}

const bsl::shared_ptr<NegotiationContext>&
InitialConnectionContext::negotiationContext() const
{
    return d_negotiationCtxSp;
}

}  // close package namespace
}  // close enterprise namespace
