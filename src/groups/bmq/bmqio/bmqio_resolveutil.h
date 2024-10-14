// Copyright 2022-2023 Bloomberg Finance L.P.
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

// bmqio_resolveutil.h                                                -*-C++-*-
#ifndef INCLUDED_BMQIO_RESOLVEUTIL
#define INCLUDED_BMQIO_RESOLVEUTIL

//@PURPOSE: Provide utilities to resolve names to IP addresses.
//
//@CLASSES:
//  bmqio::ResolveUtil: Utility to resolve names to IP addresses.
//
//@DESCRIPTION: This component provides an utility, 'bmqio::ResolveUtil', to
// resolve names to IP addresses.
//
/// Thread Safety
///-------------
// This component is thread safe.

// BDE
#include <bsl_string.h>
#include <bsl_vector.h>

// NTC
#include <ntsa_error.h>
#include <ntsa_ipaddress.h>

namespace BloombergLP {
namespace bmqio {

// ==================
// struct ResolveUtil
// ==================

/// Utility to resolve names to IP addresses.
struct ResolveUtil {
  public:
    // CLASS METHODS

    /// Load into the specified `result` the domain name to which the
    /// specified `ipAddress` is assigned.  Return the error.
    static ntsa::Error getDomainName(bsl::string*           result,
                                     const ntsa::IpAddress& ipAddress);

    /// Load into the specified `result` the hostname of the local machine.
    /// Return the error.
    static ntsa::Error getHostname(bsl::string* result);

    /// Load into the specified `result` the primary IP v4 address assigned
    /// to the specified `domainName`.  Return the error.
    static ntsa::Error getIpAddress(ntsa::Ipv4Address*      result,
                                    const bsl::string_view& domainName);

    /// Load into the specified `result` the IP addresses assigned to the
    /// specified `domainName`.  Return the error.
    static ntsa::Error getIpAddress(bsl::vector<ntsa::IpAddress>* result,
                                    const bsl::string_view&       domainName);

    /// Load into the specified `result` the IP addresses assigned to the
    /// local machine.  Return the error.
    static ntsa::Error getLocalIpAddress(bsl::vector<ntsa::IpAddress>* result);
};

}  // close package namespace
}  // close enterprise namespace

#endif
