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

// bmqpi_credentialprovider.h                                  -*-C++-*-
#ifndef INCLUDED_BMQPI_CREDENTIALPROVIDER
#define INCLUDED_BMQPI_CREDENTIALPROVIDER

/// @file bmqpi_credentialprovider.h
///
/// @brief Provide an interface for providing the credential of the client.

// BMQ
#include <bmqt_authncredential.h>

// BDE
#include <bsl_functional.h>

namespace BloombergLP {
namespace bmqpi {

// ========================
// class CredentialProvider
// ========================

/// A pure interface for providing the credential of the client.
class CredentialProvider {
  public:
    // CREATORS

    /// Destructor
    virtual ~CredentialProvider();

    // MANIPULATORS

    virtual void loadCredential(bmqt::AuthnCredential* credential) const = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
