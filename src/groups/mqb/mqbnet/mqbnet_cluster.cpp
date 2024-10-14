// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbnet_cluster.cpp                                                 -*-C++-*-
#include <mqbnet_cluster.h>

#include <mqbscm_version.h>
// BDE
#include <bsls_annotation.h>

namespace BloombergLP {
namespace mqbnet {

// ---------------------
// class ClusterObserver
// ---------------------

ClusterObserver::~ClusterObserver()
{
    // NOTHING
}

void ClusterObserver::onNodeStateChange(
    BSLS_ANNOTATION_UNUSED ClusterNode* node,
    BSLS_ANNOTATION_UNUSED bool         isAvailable)
{
    // NOTHING: void impl to not require interface clients to define it if they
    //          don't care about this notification
}

void ClusterObserver::onProxyConnectionUp(
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<bmqio::Channel>& channel,
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ClientIdentity& identity,
    BSLS_ANNOTATION_UNUSED const bsl::string& description)
{
    // NOTHING: void impl to not require interface clients to define it if they
    //          don't care about this notification
}

// -----------------
// class ClusterNode
// -----------------

ClusterNode::~ClusterNode()
{
    // NOTHING: Protocol
}

// -------------
// class Cluster
// -------------

const int Cluster::k_INVALID_NODE_ID;
const int Cluster::k_ALL_NODES_ID = bsl::numeric_limits<int>::min();

Cluster::~Cluster()
{
    // NOTHING: Protocol
}

}  // close package namespace
}  // close enterprise namespace
