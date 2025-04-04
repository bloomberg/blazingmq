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

// mqbnet_negotiatorcontext.cpp                                 -*-C++-*-
#include <mqbnet_negotiatorcontext.h>

#include <mqbscm_version.h>

namespace BloombergLP {
namespace mqbnet {

// -----------------------
// class NegotiatorContext
// -----------------------

NegotiatorContext::NegotiatorContext(bool isIncoming)
: d_isIncoming(isIncoming)
, d_maxMissedHeartbeat(0)
, d_eventProcessor_p(0)
, d_resultState_p(0)
, d_userData_p(0)
, d_cluster_p(0)
{
    // NOTHING
}

NegotiatorContext& NegotiatorContext::setMaxMissedHeartbeat(int value)
{
    d_maxMissedHeartbeat = value;
    return *this;
}

NegotiatorContext& NegotiatorContext::setUserData(void* value)
{
    d_userData_p = value;
    return *this;
}

NegotiatorContext& NegotiatorContext::setResultState(void* value)
{
    d_resultState_p = value;
    return *this;
}

NegotiatorContext&
NegotiatorContext::setEventProcessor(SessionEventProcessor* value)
{
    d_eventProcessor_p = value;
    return *this;
}

NegotiatorContext& NegotiatorContext::setCluster(Cluster* cluster)
{
    d_cluster_p = cluster;
    return *this;
}

bool NegotiatorContext::isIncoming() const
{
    return d_isIncoming;
}

Cluster* NegotiatorContext::cluster() const
{
    return d_cluster_p;
}

int NegotiatorContext::maxMissedHeartbeat() const
{
    return d_maxMissedHeartbeat;
}

void* NegotiatorContext::userData() const
{
    return d_userData_p;
}

void* NegotiatorContext::resultState() const
{
    return d_resultState_p;
}

SessionEventProcessor* NegotiatorContext::eventProcessor() const
{
    return d_eventProcessor_p;
}

}  // close package namespace
}  // close enterprise namespace
