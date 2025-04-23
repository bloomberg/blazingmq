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

// mqbnet_authenticator.h -*-C++-*-
#ifndef INCLUDED_MQBNET_AUTHENTICATOR
#define INCLUDED_MQBNET_AUTHENTICATOR

//@PURPOSE:
//
//@CLASSES:
//
//@DESCRIPTION:

// MQB
#include <mqbnet_authenticationcontext.h>

// BDE
#include <bsl_memory.h>

namespace BloombergLP {

namespace mqbnet {

// ================
// class Authenticator
// ================

/// Protocol for an Authenticator
class Authenticator {
  public:
    // CREATORS

    /// Destructor
    virtual ~Authenticator();

    // MANIPULATORS

    virtual int handleAuthenticationOnMsgType(
        const bsl::shared_ptr<AuthenticationContext>& context) = 0;

    /// Send out outbound authentication message or reverse connection request
    /// with the specified `context`.
    virtual int authenticationOutboundOrReverse(
        const bsl::shared_ptr<AuthenticationContext>& context) = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
