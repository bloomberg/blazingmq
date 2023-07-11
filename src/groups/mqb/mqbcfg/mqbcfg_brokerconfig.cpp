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

// mqbcfg_brokerconfig.cpp                                            -*-C++-*-
#include <mqbcfg_brokerconfig.h>

// BDE
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbcfg {

namespace {

const AppConfig* s_config_p;

}  // close unnamed namespace

// -------------------
// struct BrokerConfig
// -------------------

// MANIPULATORS
void BrokerConfig::set(const AppConfig& config)
{
    BSLS_ASSERT_SAFE(!s_config_p && "config already set");
    s_config_p = &config;
}

// ACCESSORS
const AppConfig& BrokerConfig::get()
{
    BSLS_ASSERT_SAFE(s_config_p && "config not set");
    return *s_config_p;
}

}  // close package namespace
}  // close enterprise namespace
