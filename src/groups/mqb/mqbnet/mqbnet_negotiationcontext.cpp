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

// mqbnet_negotiationcontext.cpp                           -*-C++-*-
#include <mqbnet_negotiationcontext.h>

#include <mqbscm_version.h>
namespace BloombergLP {
namespace mqbnet {

// ------------------------
// class NegotiationContext
// ------------------------

NegotiationContext::NegotiationContext(
    InitialConnectionContext* initialConnectionContext,
    bslma::Allocator*         allocator)
: d_initialConnectionContext_p(initialConnectionContext)
, d_negotiationMessage(allocator)
, d_clusterName(allocator)
, d_connectionType(mqbnet::ConnectionType::e_UNKNOWN)
, d_maxMissedHeartbeats(0)
, d_eventProcessor_p(0)
, d_cluster_p(0)
{
    // NOTHING
}

void NegotiationContext::setNegotiationMessage(
    const bmqp_ctrlmsg::NegotiationMessage& value)
{
    d_negotiationMessage = value;
}

void NegotiationContext::setClusterName(const bsl::string& value)
{
    d_clusterName = value;
}

void NegotiationContext::setConnectionType(ConnectionType::Enum value)
{
    d_connectionType = value;
}

void NegotiationContext::setMaxMissedHeartbeats(int value)
{
    d_maxMissedHeartbeats = value;
}

void NegotiationContext::setEventProcessor(SessionEventProcessor* value)
{
    d_eventProcessor_p = value;
}

void NegotiationContext::setCluster(Cluster* value)
{
    d_cluster_p = value;
}

// ACCESSORS

InitialConnectionContext* NegotiationContext::initialConnectionContext() const
{
    return d_initialConnectionContext_p;
}

const bmqp_ctrlmsg::NegotiationMessage&
NegotiationContext::negotiationMessage() const
{
    return d_negotiationMessage;
}

const bsl::string& NegotiationContext::clusterName() const
{
    return d_clusterName;
}

ConnectionType::Enum NegotiationContext::connectionType() const
{
    return d_connectionType;
}

int NegotiationContext::maxMissedHeartbeats() const
{
    return d_maxMissedHeartbeats;
}

SessionEventProcessor* NegotiationContext::eventProcessor() const
{
    return d_eventProcessor_p;
}

Cluster* NegotiationContext::cluster() const
{
    return d_cluster_p;
}

}  // close package namespace
}  // close enterprise namespace
