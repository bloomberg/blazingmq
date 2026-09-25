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

#ifndef INCLUDED_MQBU_DOMAINUTIL
#define INCLUDED_MQBU_DOMAINUTIL

///@PURPOSE: Provide validation for broker domain names.
///
///@CLASSES:
///  mqbu::DomainUtil: Utility methods for broker domain names
///
///@DESCRIPTION: 'mqbu::DomainUtil' validates domain names used in broker
/// configuration lookups, redirects, and admin commands.

// BDE
#include <bsl_string_view.h>

namespace BloombergLP {
namespace mqbu {

// =================
// struct DomainUtil
// =================

/// Provide validation for broker domain names.
struct DomainUtil {
    /// Return `true` if the specified `domain` is non-empty, contains only
    /// ASCII letters, digits, dashes, underscores, and dots, and has no
    /// consecutive dots.
    static bool isValidDomain(bsl::string_view domain);
};

}  // close package namespace
}  // close enterprise namespace

#endif
