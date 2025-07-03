// Copyright 2021-2023 Bloomberg Finance L.P.
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

// bmqpi_dtspan.cpp                                                   -*-C++-*-
#include <bmqpi_dtspan.h>

#include <bmqscm_version.h>
// BDE
#include <bsl_algorithm.h>
#include <bsl_utility.h>
#include <bslma_default.h>

namespace BloombergLP {
namespace bmqpi {

// ------------
// class DTSpan
// ------------

DTSpan::~DTSpan()
{
    // NOTHING
}

// ---------------------
// class DTSpan::Baggage
// ---------------------

DTSpan::Baggage::Baggage(bslma::Allocator* allocator)
: d_data(bslma::Default::allocator(allocator))
{
    // NOTHING
}

DTSpan::Baggage::const_iterator DTSpan::Baggage::begin() const
{
    return d_data.cbegin();
}

DTSpan::Baggage::const_iterator DTSpan::Baggage::end() const
{
    return d_data.cend();
}

bool DTSpan::Baggage::has(const bsl::string& key) const
{
    return d_data.find(key) != d_data.end();
}

bsl::string_view DTSpan::Baggage::get(const bsl::string&      key,
                                      const bsl::string_view& dflt) const
{
    bsl::string_view result(dflt);

    MapType::const_iterator it = d_data.find(key);
    if (it != d_data.end()) {
        result = it->second;
    }

    return result;
}

void DTSpan::Baggage::put(const bsl::string_view& key,
                          const bsl::string_view& value)
{
    bsl::pair<MapType::iterator, bool> result = d_data.emplace(key, value);
    if (!result.second) {
        result.first->second = value;
    }
}

bool DTSpan::Baggage::erase(const bsl::string& key)
{
    return d_data.erase(key) > 0;
}

}  // close package namespace
}  // close enterprise namespace
