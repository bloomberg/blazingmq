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

// mqbnet_initialconnectionhandler.h                            -*-C++-*-
#ifndef INCLUDED_MQBNET_INITIALCONNECTIONHANDLER
#define INCLUDED_MQBNET_INITIALCONNECTIONHANDLER

//@PURPOSE:
//
//@CLASSES:
//
//@DESCRIPTION: Read from IO and commands authenticator and negotiator.
// A session would be created at the end upon success.

// MQB
#include <mqbnet_initialconnectionhandlercontext.h>

// BMQ

// BDE
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_string.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqio {
class Channel;
}

namespace mqbnet {

// FORWARD DECLARATION
class Session;
class SessionEventProcessor;
class Cluster;

// ==============================
// class InitialConnectionHandler
// ==============================

class InitialConnectionHandler {
  public:
    // TYPES
    typedef bsl::function<void(int                status,
                               const bsl::string& errorDescription,
                               const bsl::shared_ptr<Session>& session)>
        InitialConnectionCb;

  public:
    // CREATORS

    /// Destructor
    virtual ~InitialConnectionHandler();

    // MANIPULATORS

    virtual void
    initialConnect(mqbnet::InitialConnectionHandlerContext* context,
                   const bsl::shared_ptr<bmqio::Channel>&   channel,
                   const InitialConnectionCb& initialConnectionCb) = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
