// Copyright 2016-2025 Bloomberg Finance L.P.
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

// bmqauthnbasic_basicauthenticator.h                             -*-C++-*-
#ifndef INCLUDED_AUTHNBASIC_BASICAUTHENTICATOR
#define INCLUDED_AUTHNBASIC_BASICAUTHENTICATOR

//@PURPOSE:
//
//@CLASSES:
//
//@DESCRIPTION:
//

// MQB
#include <mqbcfg_messages.h>
#include <mqbplug_authenticator.h>

// BMQ

// BDE
#include <ball_log.h>

namespace BloombergLP {
namespace bmqauthnbasic {

// =============================
// class BasicAuthenticationData
// =============================

class BasicAuthenticationData : public mqbplug::AuthenticationData {
  private:
    // PRIVATE TYPES
    bsl::vector<char> d_authPayload;
    bsl::string       d_clientIpAddress;

  private:
    // PRIVATE ACCESSORS

  public:
    // CREATORS
    BasicAuthenticationData(const bsl::vector<char>& authPayload,
                            const bsl::string&       clientIpAddress);

    ~BasicAuthenticationData() BSLS_KEYWORD_OVERRIDE;

    // ACESSORS

    /// Return the authentication material provided by the client.
    const bsl::vector<char>& authPayload() const BSLS_KEYWORD_OVERRIDE;

    /// Return the IP Address of the client.
    bslstl::StringRef clientIpAddress() const BSLS_KEYWORD_OVERRIDE;
};

// ========================
// class BasicAuthenticator
// ========================

class BasicAuthenticator : public mqbplug::Authenticator {
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBAUTHN.BASICAUTHENTICATOR");

  private:
    // PRIVATE TYPES
    bsl::vector<char> d_authPayload;
    bsl::string       d_clientIpAddress;

  private:
    // DATA
    const mqbcfg::AuthenticatorPluginConfig* d_authenticatorConfig_p;

    bool d_isStarted;

    bslma::Allocator* d_allocator_p;

  private:
    // PRIVATE ACCESSORS

  public:
    // NOT IMPLEMENTED
    BasicAuthenticator(const BasicAuthenticator& other)            = delete;
    BasicAuthenticator& operator=(const BasicAuthenticator& other) = delete;

    // CREATORS

    BasicAuthenticator(bslma::Allocator* allocator);

    /// Destructor.
    ~BasicAuthenticator() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Return the name of the plugin.
    bslstl::StringRef name() const BSLS_KEYWORD_OVERRIDE;

    /// Return the authentication mechanism the Authenticator supports.
    bslstl::StringRef mechanism() const BSLS_KEYWORD_OVERRIDE;

    /// Authenticate using the data provided in the specified `input`.
    /// - Return `0` on success, and populate the specified `result` with
    ///   client identity and its remaining lifetime if it has a fixed
    ///   duration.
    /// - Return a non-zero plugin-specific return code upon failure, and
    ///   populate the specified `errorDescription` with a brief reason for
    ///   logging purposes.
    int authenticate(bsl::ostream&                      errorDescript,
                     mqbplug::AuthenticationResult*     result,
                     const mqbplug::AuthenticationData& input) const
        BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Start the Authenticator and return 0 on success, or return a non-zero
    /// value and populate the specified `errorDescription` with the
    /// description of any failure encountered.
    int start(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Stop the Authenticator.
    void stop() BSLS_KEYWORD_OVERRIDE;
};

// =====================================
// class BasicAuthenticatorPluginFactory
// =====================================

class BasicAuthenticatorPluginFactory
: public mqbplug::AuthenticatorPluginFactory {
  public:
    // CREATORS
    BasicAuthenticatorPluginFactory();
    ~BasicAuthenticatorPluginFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    bslma::ManagedPtr<mqbplug::Authenticator>
    create(bslma::Allocator* allocator) BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------------
// class BasicAuthenticationData
// -----------------------------

inline const bsl::vector<char>& BasicAuthenticationData::authPayload() const
{
    return d_authPayload;
}

inline bslstl::StringRef BasicAuthenticationData::clientIpAddress() const
{
    return d_clientIpAddress;
}

// ------------------------
// class BasicAuthenticator
// ------------------------

inline bslstl::StringRef BasicAuthenticator::name() const
{
    return "BasicAuthenticator";
}

inline bslstl::StringRef BasicAuthenticator::mechanism() const
{
    return "Basic";
}

}  // close package namespace
}  // close enterprise namespace

#endif
