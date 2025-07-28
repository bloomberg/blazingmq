// Copyright 2021-2023 Bloomberg Finance L.P.
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

// bmqt_authncredential.cpp                                         -*-C++-*-
#include <bmqt_authncredential.h>

#include <bmqscm_version.h>
// BDE
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>

namespace BloombergLP {
namespace bmqt {

// ---------------------
// class AuthnCredential
// ---------------------

// CREATORS

AuthnCredential::AuthnCredential(bslma::Allocator* allocator)
: d_mechanism("", allocator)
, d_data(bsl::vector<char>(), allocator)
{
    // NOTHING
}

AuthnCredential::AuthnCredential(const bsl::string&       mechanism,
                                 const bsl::vector<char>& data,
                                 bslma::Allocator*        allocator)
: d_mechanism(mechanism, allocator)
, d_data(data, allocator)
{
    // NOTHING
}

AuthnCredential::AuthnCredential(const AuthnCredential& other,
                                 bslma::Allocator*      allocator)
: d_mechanism(other.d_mechanism, allocator)
, d_data(other.d_data, allocator)
{
    // NOTHING
}

AuthnCredential::AuthnCredential(bslmf::MovableRef<AuthnCredential> otherRef,
                                 bslma::Allocator*                  allocator)
: d_mechanism(bslmf::MovableRefUtil::access(otherRef).d_mechanism, allocator)
, d_data(bslmf::MovableRefUtil::access(otherRef).d_data, allocator)
{
    // NOTHING
}

AuthnCredential::~AuthnCredential()
{
    // NOTHING
}

// MANIPULATORS

AuthnCredential& AuthnCredential::setMechanism(const bsl::string& mechanism)
{
    d_mechanism = mechanism;
    return *this;
}

AuthnCredential& AuthnCredential::setData(const bsl::vector<char>& data)
{
    d_data = data;
    return *this;
}

// ACCESSORS

const bsl::string& AuthnCredential::mechanism() const
{
    return d_mechanism;
}

const bsl::vector<char>& AuthnCredential::data() const
{
    return d_data;
}

}  // close package namespace
}  // close enterprise namespace
