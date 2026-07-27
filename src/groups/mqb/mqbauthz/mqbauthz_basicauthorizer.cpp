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

#include <mqbauthz_basicauthorizer.h>

#include <mqbscm_version.h>

// MQB
#include <mqbact_actions.h>
#include <mqbplug_authorizer.h>

// BDE
#include <ball_log.h>
#include <bsla_maybeunused.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslmf_movableref.h>

namespace BloombergLP {
namespace mqbauthz {

bsl::string_view BasicAuthorizer::k_NAME = "BasicAuthorizer";

// -------------------------------------
// class BasicAuthorizerPluginFactory
// -------------------------------------

BasicAuthorizer::~BasicAuthorizer()
{
    // NOTHING
}

bsl::string_view BasicAuthorizer::name() const
{
    return k_NAME;
}

bool BasicAuthorizer::authorize(
    const mqbact::Action&   action,
    BSLA_MAYBE_UNUSED const mqbplug::AuthenticationResult& authnResult)

{
    BALL_LOG_INFO << "Authorize allow on " << action;
    return true;
}

// -------------------------------------
// class BasicAuthorizerPluginFactory
// -------------------------------------

BasicAuthorizerPluginFactory::~BasicAuthorizerPluginFactory()
{
    // NOTHING
}

bslma::ManagedPtr<mqbplug::Authorizer>
BasicAuthorizerPluginFactory::create(bslma::Allocator* allocator)
{
    bslma::ManagedPtr<BasicAuthorizer> basicAuthorizer =
        bslma::ManagedPtrUtil::allocateManaged<BasicAuthorizer>(allocator);
    bslma::ManagedPtr<mqbplug::Authorizer> authorizer(
        bslmf::MovableRefUtil::move(basicAuthorizer));
    return authorizer;
}

}  // close package namespace
}  // close enterprise namespace
