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
#include <mqbauthn_anonauthenticator.h>
#include <mqbauthn_pluginlibrary.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbplug_authenticator.h>
#include <mqbplug_pluginfactory.h>

// BDE
#include <ball_log.h>
#include <bdlb_string.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>

namespace BloombergLP {
namespace mqbauthn {

namespace {

typedef bsl::unordered_set<mqbplug::PluginFactory*> PluginFactories;

inline bsl::string normalizeMechanism(bsl::string_view  mech,
                                      bslma::Allocator* allocator = 0)
{
    bsl::string out(mech.begin(), mech.end(), allocator);
    bdlb::String::toUpper(&out);
    return out;
}

}  // close unnamed namespace

// ------------------------------
// class AuthenticationController
// ------------------------------

int AuthenticationController::setAnonymousCredential(
    bsl::ostream& errorDescription)
{
    const mqbcfg::AuthenticatorConfig& authenticatorConfig =
        mqbcfg::BrokerConfig::get().authentication();

    const bdlb::NullableValue<mqbcfg::AnonymousCredential>&
        anonymousCredential = authenticatorConfig.anonymousCredential();

    if (anonymousCredential.isNull()) {
        // If no anonymous credential is set, we use an empty string
        BALL_LOG_INFO << "No anonymous credential configured, "
                         "using ANONYMOUS as mechanism with empty "
                         "string as the default identity.";
        d_anonymousCredential = mqbcfg::Credential();
        d_anonymousCredential->mechanism() =
            mqbauthn::AnonAuthenticator::k_MECHANISM;
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
        return -1;  // RETURN
    }

    return 0;
}

int AuthenticationController::initializeAuthenticators(
    bsl::ostream& errorDescription)
{
    enum RcEnum {
        rc_SUCCESS                      = 0,
        rc_COLLECT_FACTORIES_FAILED     = -1,
        rc_CREATE_AUTHENTICATORS_FAILED = -2,
        rc_VALIDATE_ANONYMOUS_FAILED    = -3,
        rc_DEFAULT_AUTHENTICATOR_FAILED = -4
    };

    const mqbcfg::AuthenticatorConfig& authenticatorConfig =
        mqbcfg::BrokerConfig::get().authentication();

    // Step 1: Collect all available plugin factories (built-in + external)
    PluginFactories pluginFactories(d_allocator_p);
    if (int rc = collectAvailablePluginFactories(&pluginFactories)) {
        return rc * 10 + rc_COLLECT_FACTORIES_FAILED;  // RETURN
    }

    // Step 2: Create authenticators based on configuration
    if (int rc = createConfiguredAuthenticators(errorDescription,
                                                pluginFactories,
                                                authenticatorConfig)) {
        return rc * 10 + rc_CREATE_AUTHENTICATORS_FAILED;  // RETURN
    }

    // Step 3: Validate anonymous credential matches a configured authenticator
    if (int rc = validateAnonymousCredential(errorDescription,
                                             authenticatorConfig)) {
        return rc * 10 + rc_VALIDATE_ANONYMOUS_FAILED;  // RETURN
    }

    // Step 4: Ensure at least one authenticator is available
    if (int rc = ensureDefaultAuthenticator(errorDescription)) {
        return rc * 10 + rc_DEFAULT_AUTHENTICATOR_FAILED;  // RETURN
    }

    BALL_LOG_INFO << "Successfully initialized " << d_authenticators.size()
                  << " authenticator(s)";
    return rc_SUCCESS;
}

int AuthenticationController::collectAvailablePluginFactories(
    PluginFactories* pluginFactories)
{
    // Collect external plugin factories loaded by PluginManager
    d_pluginManager_p->get(mqbplug::PluginType::e_AUTHENTICATOR,
                           pluginFactories);

    // Add built-in plugin factories
    bsl::vector<mqbplug::PluginInfo>::const_iterator it =
        d_builtInPluginLibrary->plugins().cbegin();
    for (; it != d_builtInPluginLibrary->plugins().cend(); ++it) {
        if (it->type() == mqbplug::PluginType::e_AUTHENTICATOR) {
            pluginFactories->insert(it->factory().get());
        }
    }

    return 0;
}

int AuthenticationController::createConfiguredAuthenticators(
    bsl::ostream&                      errorDescription,
    const PluginFactories&             pluginFactories,
    const mqbcfg::AuthenticatorConfig& authenticatorConfig)
{
    enum RcEnum {
        rc_SUCCESS             = 0,
        rc_DUPLICATE_MECHANISM = -1,
        rc_START_AUTHENTICATOR = -2,
        rc_PLUGIN_NOT_FOUND    = -3
    };

    bmqu::MemOutStream errorStream(d_allocator_p);

    // If authenticators are explicitly configured, only create those
    const bsl::vector<mqbcfg::AuthenticatorPluginConfig>& authenticators =
        authenticatorConfig.authenticators();

    if (authenticators.empty()) {
        BALL_LOG_INFO << "No configured authenticator(s)";
        return rc_SUCCESS;
    }

    BALL_LOG_INFO << "Creating " << authenticators.size()
                  << " explicitly configured authenticator(s)";

    bsl::vector<mqbcfg::AuthenticatorPluginConfig>::const_iterator configIt =
        authenticators.cbegin();
    for (; configIt != authenticators.cend(); ++configIt) {
        const bsl::string& pluginName = configIt->name();

        // Find the factory for this named plugin among available factories
        mqbplug::AuthenticatorPluginFactory* factory = 0;
        AuthenticatorMp                      authenticator;

        for (PluginFactories::const_iterator factoryIt =
                 pluginFactories.cbegin();
             factoryIt != pluginFactories.cend();
             ++factoryIt) {
            mqbplug::AuthenticatorPluginFactory* candidateFactory =
                dynamic_cast<mqbplug::AuthenticatorPluginFactory*>(*factoryIt);
            if (candidateFactory) {
                // Try to create authenticator - this will fail if no
                // matching config
                AuthenticatorMp testAuth = candidateFactory->create(
                    d_allocator_p);
                if (testAuth && testAuth->name() == pluginName) {
                    // Found the right factory and already have the
                    // authenticator!
                    factory       = candidateFactory;
                    authenticator = bslmf::MovableRefUtil::move(testAuth);
                    break;
                }
            }
        }

        if (!factory) {
            errorDescription
                << "Authenticator plugin '" << pluginName
                << "' not found in available factories. "
                << "Ensure the plugin is either built-in or listed "
                << "in plugins.enabled[] if it's external";
            return rc_PLUGIN_NOT_FOUND;  // RETURN
        }

        // We already have the authenticator instance from the search above
        if (!authenticator) {
            errorDescription << "Failed to create authenticator plugin '"
                             << pluginName << "'";
            continue;  // Skip if creation failed
        }

        const bsl::string normMech =
            normalizeMechanism(authenticator->mechanism(), d_allocator_p);

        // Check for duplicate mechanisms
        AuthenticatorMap::const_iterator cit = d_authenticators.find(normMech);
        if (cit != d_authenticators.cend()) {
            errorDescription << "Duplicate authenticator mechanism '"
                             << normMech << "' for plugin '" << pluginName
                             << "'. Each mechanism can only "
                             << "have one active authenticator";
            return rc_DUPLICATE_MECHANISM;  // RETURN
        }

        // Start the authenticator
        if (int status = authenticator->start(errorStream)) {
            BMQTSK_ALARMLOG_ALARM("#AUTHENTICATION")
                << "Failed to start Authenticator '" << authenticator->name()
                << "' [rc: " << status << ", error: '" << errorStream.str()
                << "']" << BMQTSK_ALARMLOG_END;
            errorStream.reset();
            return status * 10 + rc_START_AUTHENTICATOR;  // RETURN
        }

        BALL_LOG_INFO << "Started authenticator '" << authenticator->name()
                      << "' with mechanism '" << normMech << "'";

        // Add to collection
        d_authenticators.emplace(normMech,
                                 bslmf::MovableRefUtil::move(authenticator));
    }

    return rc_SUCCESS;
}

int AuthenticationController::createDefaultAnonAuthenticator()
{
    // Create an anonymous pass authenticator
    AnonAuthenticatorPluginFactory anonFactory;
    AuthenticatorMp authenticator = anonFactory.create(d_allocator_p);

    bmqu::MemOutStream errorStream(d_allocator_p);

    // Start the anonymous pass authenticator
    if (int status = authenticator->start(errorStream)) {
        BMQTSK_ALARMLOG_ALARM("#AUTHENTICATION")
            << "Failed to start Authenticator '" << authenticator->name()
            << "' [rc: " << status << ", error: '" << errorStream.str() << "']"
            << BMQTSK_ALARMLOG_END;
        errorStream.reset();
        return status;  // RETURN
    }

    // Add the authenticator into the collection
    const bsl::string normMech = normalizeMechanism(authenticator->mechanism(),
                                                    d_allocator_p);
    d_authenticators.emplace(normMech,
                             bslmf::MovableRefUtil::move(authenticator));

    return 0;
}

int AuthenticationController::validateAnonymousCredential(
    bsl::ostream&                      errorDescription,
    const mqbcfg::AuthenticatorConfig& authenticatorConfig)
{
    enum RcEnum {
        rc_SUCCESS                        = 0,
        rc_NO_MATCHING_AUTHENTICATOR      = -1,
        rc_FAIL_CREATE_ANON_AUTHENTICATOR = -2
    };

    const bdlb::NullableValue<mqbcfg::AnonymousCredential>& anonCredential =
        authenticatorConfig.anonymousCredential();

    if (anonCredential.isNull()) {
        bsl::string anonMechanism = bsl::string(AnonAuthenticator::k_MECHANISM,
                                                d_allocator_p);

        if (d_authenticators.find(anonMechanism) != d_authenticators.cend()) {
            errorDescription << "Provided anonymous authenticator but "
                                "anonymous credential is not provided";
            return rc_NO_MATCHING_AUTHENTICATOR;  // RETURN
        }

        int rc = createDefaultAnonAuthenticator();
        if (rc != 0) {
            return rc_FAIL_CREATE_ANON_AUTHENTICATOR;  // RETURN
        }
    }
    else if (anonCredential->isCredentialValue()) {
        const bsl::string normAnonMech = normalizeMechanism(
            anonCredential->credential().mechanism(),
            d_allocator_p);

        if (d_authenticators.find(normAnonMech) == d_authenticators.cend()) {
            errorDescription
                << "Anonymous credential mechanism '" << normAnonMech
                << "' does not match any configured authenticator";
            return rc_NO_MATCHING_AUTHENTICATOR;  // RETURN
        }
    }

    return rc_SUCCESS;
}

int AuthenticationController::ensureDefaultAuthenticator(
    bsl::ostream& errorDescription)
{
    // If no authenticators are configured, use AnonAuthenticator for
    // anonymous pass authentication
    if (d_authenticators.empty()) {
        BALL_LOG_INFO << "No authenticators configured, using "
                         "AnonAuthenticator as default";

        AnonAuthenticatorPluginFactory anonFactory;
        AuthenticatorMp authenticator = anonFactory.create(d_allocator_p);

        bmqu::MemOutStream errorStream(d_allocator_p);
        if (int status = authenticator->start(errorStream)) {
            errorDescription << "Failed to start AnonAuthenticator: "
                             << errorStream.str();
            BMQTSK_ALARMLOG_ALARM("#AUTHENTICATION")
                << "Failed to start default anonymous authenticator '"
                << authenticator->name() << "' [rc: " << status << ", error: '"
                << errorStream.str() << "']" << BMQTSK_ALARMLOG_END;
            return status;  // RETURN
        }

        const bsl::string normMech =
            normalizeMechanism(authenticator->mechanism(), d_allocator_p);
        d_authenticators.emplace(normMech,
                                 bslmf::MovableRefUtil::move(authenticator));
    }

    return 0;
}

AuthenticationController::AuthenticationController(
    mqbplug::PluginManager* pluginManager,
    bslma::Allocator*       allocator)
: d_builtInPluginLibrary(
      bslma::ManagedPtrUtil::allocateManaged<mqbauthn::PluginLibrary>(
          allocator))
, d_pluginManager_p(pluginManager)
, d_isStarted(false)
, d_allocator_p(allocator)
{
}

int AuthenticationController::start(bsl::ostream& errorDescription)
{
    enum RcEnum {
        // Enum for the various RC error categories
        rc_SUCCESS             = 0,
        rc_ALREADY_STARTED     = -1,
        rc_INVALID_CONFIG      = -2,
        rc_INIT_AUTHENTICATORS = -3
    };

    if (d_isStarted) {
        errorDescription << "start() can only be called once on this object";
        return rc_ALREADY_STARTED;
    }

    BALL_LOG_INFO << "Starting AuthenticationController";

    int rc = setAnonymousCredential(errorDescription);
    if (rc != 0) {
        return rc_INVALID_CONFIG;  // RETURN
    }

    rc = initializeAuthenticators(errorDescription);
    if (rc != 0) {
        return rc * 10 + rc_INIT_AUTHENTICATORS;
    }

    // Initialize Authenticators from plugins

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

    const bsl::string normMech = normalizeMechanism(mechanism, d_allocator_p);
    AuthenticatorMap::const_iterator cit = d_authenticators.find(normMech);
    if (cit != d_authenticators.cend()) {
        const AuthenticatorMp& authenticator = cit->second;
        BALL_LOG_DEBUG << "AuthenticationController: "
                       << "authenticating with mechanism '" << normMech << "'"
                       << " (authenticator: '" << authenticator->name()
                       << "')";
        rc = authenticator->authenticate(errorStream, result, input);
        if (rc != rc_SUCCESS) {
            errorDescription << "AuthenticationController: failed to "
                                "authenticate with mechanism '"
                             << normMech << "'. (rc = " << rc
                             << "). Detailed error: " << errorStream.str();
            return (rc * 10 + rc_AUTHENTICATION_FAILED);
        }
    }
    else {
        errorDescription
            << "AuthenticationController: authentication mechanism '"
            << normMech << "' not supported.";
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
