// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mqbcfg_brokerconfig.h                                              -*-C++-*-
#ifndef INCLUDED_MQBCFG_BROKERCONFIG
#define INCLUDED_MQBCFG_BROKERCONFIG

//@PURPOSE: Provide global access to broker configuration.
//
//@CLASSES:
//  mqbcfg::BrokerConfig: mechanism to set and get the broker configuration.
//
//@DESCRIPTION: This component defines a mechanism, 'mqbcfg::BrokerConfig',
// that provides a safe way of setting and getting a pointer to the global
// broker configuration.

// BDE
#include <bsls_cpp11.h>

namespace BloombergLP {
namespace mqbcfg {

// FORWARD DECLARATION
class AppConfig;

// ===================
// struct BrokerConfig
// ===================

struct BrokerConfig {
    // NOT IMPLEMENTED
    BrokerConfig() BSLS_CPP11_DELETED;
    BrokerConfig(const BrokerConfig&) BSLS_CPP11_DELETED;
    BrokerConfig& operator=(const BrokerConfig&) BSLS_CPP11_DELETED;

    // MANIPULATORS

    /// Set the configuration to the specified `config`.  This function
    /// shall be called only once, prior any calls to `get`.
    static void set(const AppConfig& config);

    // ACCESSORS

    /// Return the configuration.  This function can be called only after
    /// `set` has been called.
    static const AppConfig& get();
};

}  // close package namespace
}  // close enterprise namespace

#endif
