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

#ifndef INCLUDED_MQBPLUG_AUTHNCREDENTIAL
#define INCLUDED_MQBPLUG_AUTHNCREDENTIAL

//@PURPOSE: Provide a value-semantic type for authentication credentials.
//
//@CLASSES:
//  mqbplug::AuthnCredential: authentication mechanism and credential data.
//
//@DESCRIPTION: This component provides a value-semantic type,
// 'mqbplug::AuthnCredential', that holds an authentication mechanism name and
// binary credential data.  It is used by 'CredentialProvider' plugins to
// supply credentials for outbound broker-to-broker authentication.

// BDE
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {
namespace mqbplug {

// ====================
// class AuthnCredential
// ====================

/// A value-semantic type holding an authentication mechanism and binary
/// credential data for outbound authentication.
class AuthnCredential {
  private:
    // DATA
    bsl::string       d_mechanism;
    bsl::vector<char> d_data;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(AuthnCredential, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an `AuthnCredential` object with empty mechanism and data,
    /// using the optionally specified `allocator` to supply memory.
    explicit AuthnCredential(bslma::Allocator* allocator = 0);

    /// Create an `AuthnCredential` object with the specified `mechanism`
    /// and `data`, using the optionally specified `allocator` to supply
    /// memory.
    explicit AuthnCredential(const bsl::string&       mechanism,
                             const bsl::vector<char>& data,
                             bslma::Allocator*        allocator = 0);

    /// Create an `AuthnCredential` object having the same value as the
    /// specified `original`, using the optionally specified `allocator` to
    /// supply memory.
    AuthnCredential(const AuthnCredential& original,
                    bslma::Allocator*      allocator = 0);

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` and return a
    /// reference providing modifiable access to this object.
    AuthnCredential& operator=(const AuthnCredential& rhs);

    // ACCESSORS

    /// Return a reference providing non-modifiable access to the
    /// authentication mechanism.
    const bsl::string& mechanism() const;

    /// Return a reference providing non-modifiable access to the credential
    /// data.
    const bsl::vector<char>& data() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------
// class AuthnCredential
// --------------------

// CREATORS
inline AuthnCredential::AuthnCredential(bslma::Allocator* allocator)
: d_mechanism(allocator)
, d_data(allocator)
{
}

inline AuthnCredential::AuthnCredential(const bsl::string&       mechanism,
                                        const bsl::vector<char>& data,
                                        bslma::Allocator*        allocator)
: d_mechanism(mechanism, allocator)
, d_data(data, allocator)
{
}

inline AuthnCredential::AuthnCredential(const AuthnCredential& original,
                                        bslma::Allocator*      allocator)
: d_mechanism(original.d_mechanism, allocator)
, d_data(original.d_data, allocator)
{
}

// MANIPULATORS
inline AuthnCredential& AuthnCredential::operator=(const AuthnCredential& rhs)
{
    if (this != &rhs) {
        d_mechanism = rhs.d_mechanism;
        d_data      = rhs.d_data;
    }
    return *this;
}

// ACCESSORS
inline const bsl::string& AuthnCredential::mechanism() const
{
    return d_mechanism;
}

inline const bsl::vector<char>& AuthnCredential::data() const
{
    return d_data;
}

}  // close package namespace
}  // close enterprise namespace

#endif
