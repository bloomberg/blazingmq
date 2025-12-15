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

// bmqt_authncredential.h                                         -*-C++-*-
#ifndef INCLUDED_BMQT_AUTHNCREDENTIAL
#define INCLUDED_BMQT_AUTHNCREDENTIAL

/// @file bmqt_authncredential.h
///
/// @brief Provide a value-semantic type for credentials used for
/// authentication.

// BDE
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {
namespace bmqt {

// =====================
// class AuthnCredential
// =====================

class AuthnCredential {
  private:
    bsl::string       d_mechanism;  // authentication mechanism
    bsl::vector<char> d_data;       // authentication data

  private:
    // NOT IMPLEMENTED (delete non-allocator creators)
    AuthnCredential(const AuthnCredential&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(AuthnCredential, bslma::UsesBslmaAllocator)

    // CREATORS
    explicit AuthnCredential(bslma::Allocator* allocator = 0);

    explicit AuthnCredential(const AuthnCredential& other,
                             bslma::Allocator*      allocator = 0);

    explicit AuthnCredential(bslmf::MovableRef<AuthnCredential> other,
                             bslma::Allocator*                  allocator = 0);

    /// Create an `AuthnCredential` object with the specified `mechanism`
    /// and `data`, using the specified `allocator` to supply memory.  If
    /// `allocator` is 0, the currently installed default allocator is used.
    explicit AuthnCredential(const bsl::string&       mechanism,
                             const bsl::vector<char>& data,
                             bslma::Allocator*        allocator = 0);

    ~AuthnCredential();

    // ASSIGNMENT (no allocator parameters)
    AuthnCredential& operator=(const AuthnCredential& rhs);
    AuthnCredential& operator=(bslmf::MovableRef<AuthnCredential> rhs);

    // MANIPULATORS
    AuthnCredential& setMechanism(const bsl::string& mechanism);
    AuthnCredential& setData(const bsl::vector<char>& data);

    // ACCESSORS
    const bsl::string&       mechanism() const;
    const bsl::vector<char>& data() const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
