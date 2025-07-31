// Copyright 2021-2025 Bloomberg Finance L.P.
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

// bmqpi_basiccredentialprovider.cpp                                 -*-C++-*-
#include <bmqauthn_basiccredentialprovider.h>

#include <bmqscm_version.h>

// BDE
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>

namespace BloombergLP {
namespace bmqauthn {

// -----------------------------
// class BasicCredentialProvider
// -----------------------------

// CREATORS
BasicCredentialProvider::BasicCredentialProvider(
    const bsl::string_view& username,
    const bsl::string_view& password,
    bslma::Allocator*       allocator)
: d_username(username, allocator)
, d_password(password, allocator)
, d_mechanism("Basic")
, d_allocator_p(allocator)
{
    // NOTHING
}

/// Destructor
BasicCredentialProvider::~BasicCredentialProvider()
{
    // NOTHING: (required because of inheritance)
}

// MANIPULATORS
void BasicCredentialProvider::loadCredential(
    bmqt::AuthnCredential* credential) const
{
    bsl::string       data(d_username + ":" + d_password, d_allocator_p);
    bsl::vector<char> vec(data.begin(), data.end(), d_allocator_p);
    credential->setData(vec);
    credential->setMechanism(d_mechanism);
}

}  // close package namespace
}  // close enterprise namespace
