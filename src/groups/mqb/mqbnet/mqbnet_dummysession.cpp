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

// mqbnet_dummysession.cpp                                            -*-C++-*-
#include <mqbnet_dummysession.h>

#include <mqbscm_version.h>
// BDE
#include <bsl_iostream.h>
#include <bsla_annotations.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbnet {

// ------------------
// class DummySession
// ------------------

DummySession::DummySession(
    const bsl::shared_ptr<bmqio::Channel>&  channel,
    const bmqp_ctrlmsg::NegotiationMessage& negotiationMessage,
    ClusterNode*                            clusterNode,
    const bsl::string&                      description,
    bslma::Allocator*                       allocator)
: d_channel_sp(channel)
, d_negotiationMessage(negotiationMessage, allocator)
, d_clusterNode_p(clusterNode)
, d_description(description, allocator)
{
    BALL_LOG_INFO << d_description << ": created "
                  << "[" << negotiationMessage << "]";
}

DummySession::~DummySession()
{
    BALL_LOG_INFO << d_description << ": destroyed";
}

void DummySession::tearDown(BSLA_UNUSED const bsl::shared_ptr<void>& handle,
                            BSLA_UNUSED bool isBrokerShutdown)
{
    // NOTHING
}

void DummySession::initiateShutdown(const ShutdownCb& callback)
{
    // NOTHING
}

void DummySession::invalidate()
{
    // NOTHING
}

void DummySession::processEvent(BSLA_UNUSED const bmqp::Event& event,
                                BSLA_UNUSED mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_OPT(false &&
                    "'processEvent' should never be called on a DummySession");

    // Processing of packet received should never be invoked on a
    // 'DummySession'; a 'sessionEventProcessor' should have been registered to
    // handle all read from the associated channel.
}

}  // close package namespace
}  // close enterprise namespace
