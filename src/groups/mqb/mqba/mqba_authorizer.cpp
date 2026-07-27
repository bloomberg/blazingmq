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

#include <mqba_authorizer.h>

#include <mqbscm_version.h>

// MQB
#include <mqbauthz_authorizationcontroller.h>
#include <mqbplug_authorizer.h>

// BDE
#include <ball_log.h>
#include <ball_scopedattribute.h>
#include <bsl_memory.h>

namespace BloombergLP {
namespace mqba {

// -------------------
// class Authenticator
// -------------------

// CREATORS
Authorizer::Authorizer(mqbauthz::AuthorizationController* authzController)
: d_authzController_p(authzController)
{
    // PRECONDITIONS
    BSLS_ASSERT(d_authzController_p);
}

/// Destructor
Authorizer::~Authorizer()
{
}

bool Authorizer::authorize(const mqbact::Action&                action,
                           const mqbplug::AuthenticationResult& authnResult)
{
    ball::ScopedAttribute principalAttr("principal", authnResult.principal());
    return d_authzController_p->authorizer().authorize(action, authnResult);
}

}  // close package namespace
}  // close enterprise namespace
