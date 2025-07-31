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

// bmqpauthn_basiccredentialprovider.h                          -*-C++-*-
#ifndef INCLUDED_BMQAUTHN_BASICCREDENTIALPROVIDER
#define INCLUDED_BMQAUTHN_BASICCREDENTIALPROVIDER

/// @file bmqpauthn_basiccredentialprovider.h
///
/// @brief ddd

// BMQ
#include <bmqpi_credentialprovider.h>
#include <bmqt_authncredential.h>

// BDE
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace bmqauthn {

// =============================
// class BasicCredentialProvider
// =============================

class BasicCredentialProvider : public bmqpi::CredentialProvider {
  private:
    // DATA
    bsl::string       d_username;
    bsl::string       d_password;
    bsl::string       d_mechanism;
    bslma::Allocator* d_allocator_p;

  public:
    // TRAITES
    BSLMF_NESTED_TRAIT_DECLARATION(BasicCredentialProvider,
                                   bslma::UsesBslmaAllocator)
    // CREATORS

    // Create a `BasicCredentialProvider` with the specified `username`
    // and `password`.
    BasicCredentialProvider(const bsl::string_view& username,
                            const bsl::string_view& password,
                            bslma::Allocator*       allocator = 0);

    /// Destructor
    ~BasicCredentialProvider() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    void loadCredential(bmqt::AuthnCredential* credential) const
        BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
