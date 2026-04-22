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

#include <bmqio_resolveutil.h>

#include <bmqscm_version.h>
// BDE
#include <bdlma_localsequentialallocator.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bsls_assert.h>

// NTC
#include <bsl_memory.h>
#include <bsls_keyword.h>
#include <ntsa_error.h>
#include <ntsa_ipaddress.h>
#include <ntsa_ipaddressoptions.h>
#include <ntsf_system.h>
#include <ntsi_resolver.h>

namespace BloombergLP {
namespace bmqio {

// ------------------
// struct ResolveUtil
// ------------------

// CLASS LEVEL METHODS
ntsa::Error ResolveUtil::getDomainName(bsl::string*           result,
                                       const ntsa::IpAddress& ipAddress)
{
    // PRECONDITIONS
    BSLS_ASSERT(result);

    bsl::shared_ptr<ntsi::Resolver> resolver = ntsf::System::createResolver();
    return resolver->getDomainName(result, ipAddress);
}

ntsa::Error ResolveUtil::getHostname(bsl::string* result)
{
    // PRECONDITIONS
    BSLS_ASSERT(result);

    const BSLS_KEYWORD_CONSTEXPR int                  k_ALLOC_ESTIMATE = 1024;
    bdlma::LocalSequentialAllocator<k_ALLOC_ESTIMATE> alloc;

    bsl::shared_ptr<ntsi::Resolver> resolver = ntsf::System::createResolver(
        &alloc);
    return resolver->getHostname(result);
}

ntsa::Error ResolveUtil::getIpAddress(ntsa::Ipv4Address*      result,
                                      const bsl::string_view& domainName)
{
    // PRECONDITIONS
    BSLS_ASSERT(result);

    // Avoid dynamic memory allocation
    const BSLS_KEYWORD_CONSTEXPR int k_ALLOC_ESTIMATE =
        sizeof(ntsa::IpAddressOptions) + sizeof(ntsa::IpAddress) + 1024;
    bdlma::LocalSequentialAllocator<k_ALLOC_ESTIMATE> alloc;

    ntsa::IpAddressOptions options(&alloc);

    // Limit addresses to V4 only
    options.setIpAddressType(ntsa::IpAddressType::e_V4);
    // Return first address
    options.setIpAddressSelector(0);

    bsl::vector<ntsa::IpAddress> ipAddresses(&alloc);

    bsl::shared_ptr<ntsi::Resolver> resolver = ntsf::System::createResolver(
        &alloc);
    ntsa::Error error = resolver->getIpAddress(&ipAddresses,
                                               domainName,
                                               options);
    if (error.code() == ntsa::Error::e_OK) {
        // There should be at least one ip address
        *result = ipAddresses.front().v4();
    }

    return error;
}

ntsa::Error ResolveUtil::getIpAddress(bsl::vector<ntsa::IpAddress>* result,
                                      const bsl::string_view&       domainName)
{
    // PRECONDITIONS
    BSLS_ASSERT(result);

    const BSLS_KEYWORD_CONSTEXPR int k_ALLOC_ESTIMATE =
        1024 + sizeof(ntsa::IpAddressOptions);
    bdlma::LocalSequentialAllocator<k_ALLOC_ESTIMATE> alloc;
    bsl::shared_ptr<ntsi::Resolver> resolver = ntsf::System::createResolver(
        &alloc);
    ntsa::Error error = resolver->getIpAddress(result,
                                               domainName,
                                               ntsa::IpAddressOptions(&alloc));

    return error;
}

ntsa::Error
ResolveUtil::getLocalIpAddress(bsl::vector<ntsa::IpAddress>* result)
{
    // PRECONDITIONS
    BSLS_ASSERT(result);

    bsl::shared_ptr<ntsi::Resolver> resolver = ntsf::System::createResolver();
    return resolver->getLocalIpAddress(result, ntsa::IpAddressOptions());
}

}  // close package namespace
}  // close enterprise namespace
