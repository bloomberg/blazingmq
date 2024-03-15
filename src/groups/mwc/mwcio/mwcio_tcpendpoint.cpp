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

// mwcio_tcpendpoint.cpp                                              -*-C++-*-
#include <mwcio_tcpendpoint.h>

#include <mwcscm_version.h>
// MWC
#include <mwcu_memoutstream.h>
#include <mwcu_stringutil.h>

// BDE
#include <bdlb_print.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_cstdlib.h>
#include <bsl_ostream.h>
#include <bslma_allocator.h>
#include <bslma_default.h>

namespace BloombergLP {
namespace mwcio {

namespace {
const char k_SCHEME[]   = "tcp://";
const int  k_SCHEME_LEN = sizeof(k_SCHEME) / sizeof(char) - 1;
}  // close unnamed namespace

// -----------------
// class TCPEndpoint
// -----------------

TCPEndpoint::TCPEndpoint(bslma::Allocator* allocator)
: d_uri(bslma::Default::allocator(allocator))
, d_port(0)
, d_host(bslma::Default::allocator(allocator))
{
    // NOTHING
}

TCPEndpoint::TCPEndpoint(const bsl::string& uri, bslma::Allocator* allocator)
: d_uri(bslma::Default::allocator(allocator))
, d_port(0)
, d_host(bslma::Default::allocator(allocator))
{
    fromUri(uri);
}

TCPEndpoint::TCPEndpoint(const bslstl::StringRef& host,
                         int                      port,
                         bslma::Allocator*        allocator)
: d_uri(bslma::Default::allocator(allocator))
, d_port(0)
, d_host(bslma::Default::allocator(allocator))
{
    assign(host, port);
}

bool TCPEndpoint::fromUri(const bsl::string& uri)
{
    d_uri.clear();

    // Check 'uri' starts by the proper expected scheme
    if (!mwcu::StringUtil::startsWith(uri, k_SCHEME)) {
        return false;  // RETURN
    }

    // Check there is a ':' somewhere after the scheme
    const size_t colon = uri.find_last_of(':');
    if (colon < k_SCHEME_LEN) {
        // Means there was no ':' in the uri beside the ones in the scheme part
        return false;  // RETURN
    }

    // Sanity check, there shall only be one colon after the scheme, which is
    // the one found above (during a reverse find).
    if (uri.find_first_of(':', k_SCHEME_LEN) != colon) {
        return false;  // RETURN
    }

    // Extract the port part: i.e. after the last ':'
    long temp_port = bsl::strtol(uri.c_str() + colon + 1, 0, 10);
    if (temp_port <= 0 || temp_port > INT_MAX) {
        // Invalid value for port number
        return false;  // RETURN
    }
    d_port = static_cast<int>(temp_port);

    // Extract the host part: i.e. between '/' and ':'
    d_host.assign(uri, k_SCHEME_LEN, colon - k_SCHEME_LEN);

    // All good, assign 'd_uri' (which is what's checked for in operator bool)
    d_uri = uri;

    return true;
}

void TCPEndpoint::fromUriRaw(const bsl::string& uri)
{
    d_uri = uri;

    const size_t separator = uri.find_last_of(':');

    long temp_port = bsl::strtol(uri.c_str() + separator + 1, 0, 10);
    if (temp_port > 0 && temp_port <= INT_MAX) {
        // Maintaining strtol long value and casting if port in range
        d_port = static_cast<int>(temp_port);
    }
    d_host.assign(uri, k_SCHEME_LEN, separator - k_SCHEME_LEN);
}

TCPEndpoint& TCPEndpoint::assign(const bslstl::StringRef& host, int port)
{
    bdlma::LocalSequentialAllocator<128> localAllocator;
    mwcu::MemOutStream                   ss(&localAllocator);
    ss << k_SCHEME << host << ":" << port;

    fromUriRaw(ss.str());

    return *this;
}

bsl::ostream&
TCPEndpoint::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bdlb::Print::indent(stream, level, spacesPerLevel);

    if (this->operator bool()) {
        stream << d_uri;
    }
    else {
        stream << "** invalid endpoint **";
    }

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
