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

// mqbnet_initialconnectionhandlercontext.cpp                       -*-C++-*-
#include <mqbnet_initialconnectionhandlercontext.h>

#include <mqbscm_version.h>

namespace BloombergLP {
namespace mqbnet {

// -------------------------------------
// class InitialConnectionHandlerContext
// -------------------------------------

InitialConnectionHandlerContext::InitialConnectionHandlerContext(
    bool isIncoming)
: d_isIncoming(isIncoming)
, d_maxMissedHeartbeat(0)
, d_eventProcessor_p(0)
, d_resultState_p(0)
, d_userData_p(0)
, d_cluster_p(0)
{
    // NOTHING
}

InitialConnectionHandlerContext&
InitialConnectionHandlerContext::setMaxMissedHeartbeat(int value)
{
    d_maxMissedHeartbeat = value;
    return *this;
}

InitialConnectionHandlerContext&
InitialConnectionHandlerContext::setUserData(void* value)
{
    d_userData_p = value;
    return *this;
}

InitialConnectionHandlerContext&
InitialConnectionHandlerContext::setResultState(void* value)
{
    d_resultState_p = value;
    return *this;
}

InitialConnectionHandlerContext&
InitialConnectionHandlerContext::setEventProcessor(
    SessionEventProcessor* value)
{
    d_eventProcessor_p = value;
    return *this;
}

InitialConnectionHandlerContext&
InitialConnectionHandlerContext::setCluster(Cluster* cluster)
{
    d_cluster_p = cluster;
    return *this;
}

bool InitialConnectionHandlerContext::isIncoming() const
{
    return d_isIncoming;
}

Cluster* InitialConnectionHandlerContext::cluster() const
{
    return d_cluster_p;
}

int InitialConnectionHandlerContext::maxMissedHeartbeat() const
{
    return d_maxMissedHeartbeat;
}

void* InitialConnectionHandlerContext::userData() const
{
    return d_userData_p;
}

void* InitialConnectionHandlerContext::resultState() const
{
    return d_resultState_p;
}

SessionEventProcessor* InitialConnectionHandlerContext::eventProcessor() const
{
    return d_eventProcessor_p;
}

}  // close package namespace
}  // close enterprise namespace
