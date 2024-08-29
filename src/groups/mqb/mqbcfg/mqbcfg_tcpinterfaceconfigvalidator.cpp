// Copyright 2024 Bloomberg Finance L.P.
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
// mqbcfg_tcpinterfaceconfigvalidator.cpp

#include <bsls_ident.h>
BSLS_IDENT_RCSID(mqbcfg_tcpinterfaceconfigvalidator_h, "$Id$ $CSID$")

#include <mqbcfg_tcpinterfaceconfigvalidator.h>

#include <ball_log.h>
#include <bsl_algorithm.h>
#include <bsl_functional.h>
#include <bsl_type_traits.h>
#include <bsl_vector.h>

namespace BloombergLP {
namespace mqbcfg {

namespace {

const char LOG_CATEGORY[] = "MQBCFG.TCPINTERFACECONFIGVALIDATOR";
BALL_LOG_SET_NAMESPACE_CATEGORY(LOG_CATEGORY)

/// @brief Transform the range that elements in the range [`begin`, `end`) and
/// then check if the result contains unique items.
template <typename It, typename F>
bool uniqueWith(It begin, It end, F f)
{
    typedef typename bsl::iterator_traits<It>::reference    Reference;
    typedef typename bsl::invoke_result<F, Reference>::type Result;
    bsl::vector<Result>                                     sorted;
    sorted.reserve(bsl::distance(begin, end));

    bsl::transform(begin, end, bsl::back_inserter(sorted), f);
    bsl::sort(sorted.begin(), sorted.end());
    typename bsl::vector<Result>::iterator newEnd = bsl::unique(sorted.begin(),
                                                                sorted.end());
    return sorted.end() == newEnd;
}

}  // close unnamed namespace

// PRIVATE STATIC FUNCTIONS
int TcpInterfaceConfigValidator::port(
    const mqbcfg::TcpInterfaceListener& listener)
{
    return listener.port();
}

bsl::string_view
TcpInterfaceConfigValidator::name(const mqbcfg::TcpInterfaceListener& listener)
{
    return listener.name();
}

bool TcpInterfaceConfigValidator::isValidPort(
    const mqbcfg::TcpInterfaceListener& listener)
{
    return 0 <= listener.port() && listener.port() <= 65535;
}

TcpInterfaceConfigValidator::ErrorCode TcpInterfaceConfigValidator::operator()(
    const mqbcfg::TcpInterfaceConfig& config) const
{
    if (config.listeners().empty()) {
        return k_OK;
    }

    bsl::vector<mqbcfg::TcpInterfaceListener>::const_iterator
        first = config.listeners().cbegin(),
        last  = config.listeners().cend();
    // The names of each network interface is unique
    if (!uniqueWith(first, last, TcpInterfaceConfigValidator::name)) {
        BALL_LOG_ERROR << "TCP interface validation failed: Multiple "
                          "interfaces with the same name";
        return k_DUPLICATE_NAME;
    }

    // The ports of each network interface is unqiue
    if (!uniqueWith(first, last, TcpInterfaceConfigValidator::port)) {
        BALL_LOG_ERROR << "TCP interface validation failed: Multiple "
                          "interfaces using the same port";
        return k_DUPLICATE_PORT;
    }

    // Ports passed are possible port values
    if (!bsl::all_of(first, last, TcpInterfaceConfigValidator::isValidPort)) {
        BALL_LOG_ERROR << "TCP interface validation failed: Invalid port "
                          "specified";
        return k_PORT_RANGE;
    }

    return k_OK;
}

}  // close namespace mqbcfg
}  // close namespace BloombergLP
