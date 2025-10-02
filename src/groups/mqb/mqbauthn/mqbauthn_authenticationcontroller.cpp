// Copyright 2017-2023 Bloomberg Finance L.P.
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

// mqbauthn_authenticationcontroller.cpp                          -*-C++-*-
#include <mqbauthn_authenticationcontroller.h>

#include <mqbscm_version.h>
// BMQ
#include <bmqtsk_alarmlog.h>
#include <bmqu_memoutstream.h>

// MQB
#include <mqbauthn_anonpassauthenticator.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbplug_authenticator.h>
#include <mqbplug_pluginfactory.h>

// BDE
#include <ball_log.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bsl_unordered_set.h>

namespace BloombergLP {
namespace mqbauthn {

namespace {

typedef bsl::unordered_set<mqbplug::PluginFactory*> PluginFactories;

}  // close unnamed namespace

// ------------------------------
// class AuthenticationController
// ------------------------------

AuthenticationController::AuthenticationController(
    mqbplug::PluginManager* pluginManager,
    bslma::Allocator*       allocator)
: d_pluginManager_p(pluginManager)
, d_isStarted(false)
, d_allocator_p(allocator)
{
}

int AuthenticationController::start(bsl::ostream& errorDescription)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "start() can only be called once on this object");

    enum RcEnum {
        // Enum for the various RC error categories
        rc_SUCCESS             = 0,
        rc_DUPLICATE_MECHANISM = -1,
        rc_INVALID_CONFIG      = -2
    };

    bmqu::MemOutStream errorStream(d_allocator_p);

    BALL_LOG_INFO << "Starting AuthenticationController";

    const mqbcfg::AuthenticatorConfig& authenticatorConfig =
        mqbcfg::BrokerConfig::get().authentication();

    // Set the default anonymous credential
    const bdlb::NullableValue<mqbcfg::AnonymousCredential>&
        anonymousCredential = authenticatorConfig.anonymousCredential();

    if (anonymousCredential.isNull()) {
        // If no anonymous credential is set, we use an empty string
        BALL_LOG_INFO << "No anonymous credential configured, "
                         "using Anonymous as default mechanism and empty "
                         "string as default identity.";
        d_anonymousCredential = mqbcfg::Credential();
        d_anonymousCredential->mechanism() =
            mqbauthn::AnonPassAuthenticator::k_MECHANISM;
        d_anonymousCredential->identity() = "";
    }
    else if (anonymousCredential->isCredentialValue()) {
        BALL_LOG_INFO << "Using configured anonymous credential.";
        d_anonymousCredential = anonymousCredential->credential();
    }
    else if (anonymousCredential->isDisallowValue()) {
        BALL_LOG_INFO << "Anonymous credential disallowed.";
        d_anonymousCredential.reset();
    }
    else {
        errorDescription << "Expected credential or disallow but received a "
                            "type that's not defined.";
        return rc_INVALID_CONFIG;  // RETURN
    }

    // Initialize Authenticators from plugins
    {
        PluginFactories pluginFactories(d_allocator_p);
        d_pluginManager_p->get(mqbplug::PluginType::e_AUTHENTICATOR,
                               &pluginFactories);

        for (PluginFactories::const_iterator factoryIt =
                 pluginFactories.cbegin();
             factoryIt != pluginFactories.cend();
             ++factoryIt) {
            mqbplug::AuthenticatorPluginFactory* factory =
                dynamic_cast<mqbplug::AuthenticatorPluginFactory*>(*factoryIt);
            AuthenticatorMp authenticator = factory->create(d_allocator_p);

            // Check if there's an authenticator with duplicate mechanism
            AuthenticatorMap::const_iterator cit = d_authenticators.find(
                authenticator->mechanism());
            if (cit != d_authenticators.cend()) {
                errorDescription << "Attempting to create duplicate "
                                    "authenticator with mechanism '"
                                 << authenticator->mechanism();
                return rc_DUPLICATE_MECHANISM;
            }

            // Start the authenticator
            if (int status = authenticator->start(errorStream)) {
                BMQTSK_ALARMLOG_ALARM("#AUTHENTICATION")
                    << "Failed to start Authenticator '"
                    << authenticator->name() << "' [rc: " << status
                    << ", error: '" << errorStream.str() << "']"
                    << BMQTSK_ALARMLOG_END;
                errorStream.reset();
                continue;  // CONTINUE
            }

            // Add the authenticator into the collection
            d_authenticators.emplace(
                authenticator->mechanism(),
                bslmf::MovableRefUtil::move(authenticator));
        }

        // Add the anonymous authenticator if no other authenticators
        // are configured.
        if (d_authenticators.empty()) {
            BALL_LOG_INFO << "No authenticators configured, using "
                             "AnonPassAuthenticator as default.";

            // Create an anonymous pass authenticator
            AnonPassAuthenticatorPluginFactory anonFactory;
            AuthenticatorMp authenticator = anonFactory.create(d_allocator_p);

            // Start the anonymous pass authenticator
            if (int status = authenticator->start(errorStream)) {
                BMQTSK_ALARMLOG_ALARM("#AUTHENTICATION")
                    << "Failed to start Authenticator '"
                    << authenticator->name() << "' [rc: " << status
                    << ", error: '" << errorStream.str() << "']"
                    << BMQTSK_ALARMLOG_END;
                errorStream.reset();
                return status;  // RETURN
            }

            // Add the authenticator into the collection
            d_authenticators.emplace(
                authenticator->mechanism(),
                bslmf::MovableRefUtil::move(authenticator));
        }
    }

    d_isStarted = true;

    return rc_SUCCESS;
}

void AuthenticationController::stop()
{
    if (!d_isStarted) {
        return;  // RETURN
    }

    d_isStarted = false;

    // Stop all authenticators
    for (AuthenticatorMap::iterator it = d_authenticators.begin();
         it != d_authenticators.end();
         ++it) {
        it->second->stop();
    }

    d_authenticators.clear();
}

int AuthenticationController::authenticate(
    bsl::ostream&                                   errorDescription,
    bsl::shared_ptr<mqbplug::AuthenticationResult>* result,
    bsl::string_view                                mechanism,
    const mqbplug::AuthenticationData&              input)
{
    // executed by an *AUTHENTICATION* thread

    enum RcEnum {
        // Enum for the various RC error categories
        rc_SUCCESS                 = 0,
        rc_AUTHENTICATION_FAILED   = -1,
        rc_MECHANISM_NOT_SUPPORTED = -2
    };

    int                rc = rc_SUCCESS;
    bmqu::MemOutStream errorStream(d_allocator_p);

    AuthenticatorMap::const_iterator cit = d_authenticators.find(mechanism);
    if (cit != d_authenticators.cend()) {
        const AuthenticatorMp& authenticator = cit->second;
        BALL_LOG_DEBUG << "AuthenticationController: "
                       << "authenticating with mechanism '" << mechanism << "'"
                       << " (authenticator: '" << authenticator->name()
                       << "')";
        rc = authenticator->authenticate(errorStream, result, input);
        if (rc != rc_SUCCESS) {
            errorDescription << "AuthenticationController: failed to "
                                "authenticate with mechanism '"
                             << mechanism << "'. (rc = " << rc
                             << "). Detailed error: " << errorStream.str();
            return (rc * 10 + rc_AUTHENTICATION_FAILED);
        }
    }
    else {
        errorDescription
            << "AuthenticationController: authentication mechanism '"
            << mechanism << "' not supported.";
        return (rc * 10 + rc_MECHANISM_NOT_SUPPORTED);
    }

    return rc_SUCCESS;
}

const bsl::optional<mqbcfg::Credential>&
AuthenticationController::anonymousCredential()
{
    return d_anonymousCredential;
}

}  // close package namespace
}  // close enterprise namespace
