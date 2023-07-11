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

// mqbu_sdkversionutil.h                                              -*-C++-*-
#ifndef INCLUDED_MQBU_SDKVERSIONUTIL
#define INCLUDED_MQBU_SDKVERSIONUTIL

//@PURPOSE: Provide utilities for BlazingMQ SDK source control management.
//
//@CLASSES:
//  mqbu::SDKVersionUtil: Utilities for BlazingMQ SDK source control management
//
//@DESCRIPTION: 'mqbu::SDKVersionUtil' provides a set of utility methods for
// SDK source control management to be used throughout BlazingMQ.
//

// MQB

// BMQ
#include <bmqp_ctrlmsg_messages.h>

namespace BloombergLP {
namespace mqbu {

// =====================
// struct SDKVersionUtil
// =====================

/// Provide utilties for BlazingMQ SDK source control management to be used
/// throughout BMQ.
struct SDKVersionUtil {
    /// Return the minimum sdk version supported for the specified
    /// `language`.  If `language` is not recognized, return -1.
    static int
    minSdkVersionSupported(bmqp_ctrlmsg::ClientLanguage::Value language);

    /// Return the minimum sdk version supported for the specified
    /// `language`.  If `language` is not recognized, return -1.
    static int
    minSdkVersionRecommended(bmqp_ctrlmsg::ClientLanguage::Value language);

    /// Return true if the specified `version` of the SDK in the specified
    /// `language` is deprecated and false otherwise.
    static bool
    isDeprecatedSdkVersion(bmqp_ctrlmsg::ClientLanguage::Value language,
                           int                                 version);

    /// Return true if the specified `version` of the SDK in the specified
    /// `language` is supported and false otherwise.
    static bool
    isSupportedSdkVersion(bmqp_ctrlmsg::ClientLanguage::Value language,
                          int                                 version);

    /// Return true if the specified `version` of the SDK should negotiate
    /// `k_MESSAGE_PROPERTIES_EX`.
    static bool isMinExtendedMessagePropertiesVersion(
        bmqp_ctrlmsg::ClientLanguage::Value language,
        int                                 version);
};

}  // close package namespace
}  // close enterprise namespace

#endif
