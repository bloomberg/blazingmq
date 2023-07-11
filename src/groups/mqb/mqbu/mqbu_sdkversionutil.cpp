// Copyright 2019-2023 Bloomberg Finance L.P.
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

// mqbu_sdkversionutil.cpp                                            -*-C++-*-
#include <mqbu_sdkversionutil.h>

#include <mqbscm_version.h>
// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>

// BMQ
#include <bmqscm_version.h>

// BDE
#include <bslmf_assert.h>

namespace BloombergLP {
namespace mqbu {

namespace {

// C++
const int k_MINIMUM_SDK_VERSION_SUPPORTED_CPP = BMQ_MAKE_EXT_VERSION(1, 5, 0);
const int k_MINIMUM_SDK_VERSION_RECOMMENDED_CPP = BMQ_MAKE_EXT_VERSION(1,
                                                                       9,
                                                                       0);

BSLMF_ASSERT(k_MINIMUM_SDK_VERSION_SUPPORTED_CPP <=
             k_MINIMUM_SDK_VERSION_RECOMMENDED_CPP);

// Java
const int k_MINIMUM_SDK_VERSION_SUPPORTED_JAVA = BMQ_MAKE_EXT_VERSION(0, 0, 2);
const int k_MINIMUM_SDK_VERSION_RECOMMENDED_JAVA = BMQ_MAKE_EXT_VERSION(0,
                                                                        0,
                                                                        2);
BSLMF_ASSERT(k_MINIMUM_SDK_VERSION_SUPPORTED_JAVA <=
             k_MINIMUM_SDK_VERSION_RECOMMENDED_JAVA);

}  // close unnamed namespace

// ---------------------
// struct SDKVersionUtil
// ---------------------

int SDKVersionUtil::minSdkVersionSupported(
    bmqp_ctrlmsg::ClientLanguage::Value language)
{
    switch (language) {
    case bmqp_ctrlmsg::ClientLanguage::E_CPP: {
        return k_MINIMUM_SDK_VERSION_SUPPORTED_CPP;  // RETURN
    }
    case bmqp_ctrlmsg::ClientLanguage::E_JAVA: {
        return k_MINIMUM_SDK_VERSION_SUPPORTED_JAVA;  // RETURN
    }
    case bmqp_ctrlmsg::ClientLanguage::E_UNKNOWN:
    default: {
        // While introducing new languages, some unknown languages may be
        // received.
        return -1;  // RETURN
    }
    }
}

int SDKVersionUtil::minSdkVersionRecommended(
    bmqp_ctrlmsg::ClientLanguage::Value language)
{
    switch (language) {
    case bmqp_ctrlmsg::ClientLanguage::E_CPP: {
        return k_MINIMUM_SDK_VERSION_RECOMMENDED_CPP;  // RETURN
    }
    case bmqp_ctrlmsg::ClientLanguage::E_JAVA: {
        return k_MINIMUM_SDK_VERSION_RECOMMENDED_JAVA;  // RETURN
    }
    case bmqp_ctrlmsg::ClientLanguage::E_UNKNOWN:
    default: {
        // While introducing new languages, some unknown languages may be
        // received.
        return -1;  // RETURN
    }
    }
}

bool SDKVersionUtil::isDeprecatedSdkVersion(
    bmqp_ctrlmsg::ClientLanguage::Value language,
    int                                 version)
{
    const int minVersion = mqbu::SDKVersionUtil::minSdkVersionRecommended(
        language);
    return version < minVersion;
}

bool SDKVersionUtil::isSupportedSdkVersion(
    bmqp_ctrlmsg::ClientLanguage::Value language,
    int                                 version)
{
    const int minVersion = mqbu::SDKVersionUtil::minSdkVersionSupported(
        language);
    return version >= minVersion;
}

bool SDKVersionUtil::isMinExtendedMessagePropertiesVersion(
    bmqp_ctrlmsg::ClientLanguage::Value language,
    int                                 version)
{
    const mqbcfg::AppConfig& brkrCfg = mqbcfg::BrokerConfig::get();

    switch (language) {
    case bmqp_ctrlmsg::ClientLanguage::E_CPP: {
        return version >= brkrCfg.messagePropertiesV2().minCppSdkVersion();
    }
    case bmqp_ctrlmsg::ClientLanguage::E_JAVA: {
        return version >= brkrCfg.messagePropertiesV2().minJavaSdkVersion();
    }
    case bmqp_ctrlmsg::ClientLanguage::E_UNKNOWN:
    default: {
        // While introducing new languages, some unknown languages may be
        // received.
        return true;  // RETURN
    }
    }
}

}  // close package namespace
}  // close enterprise namespace
