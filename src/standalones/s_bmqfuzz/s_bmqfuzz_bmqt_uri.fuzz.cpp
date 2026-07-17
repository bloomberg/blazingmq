// Copyright 2025 Bloomberg Finance L.P.
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

#include <bmqt_uri.h>

#include <bsl_string.h>

using namespace BloombergLP;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    bsl::string fuzz_input(reinterpret_cast<const char*>(data), size);

    bmqt::Uri   uri;
    bsl::string error;
    const int   rc = bmqt::UriParser::parse(&uri, &error, fuzz_input);

    if (rc != 0) {
        return 0;
    }

    bsl::string components;
    components.append(uri.scheme().data(), uri.scheme().length());
    components.append(uri.authority().data(), uri.authority().length());
    components.append(uri.domain().data(), uri.domain().length());
    components.append(uri.tier().data(), uri.tier().length());
    components.append(uri.queue().data(), uri.queue().length());
    components.append(uri.qualifiedDomain().data(),
                      uri.qualifiedDomain().length());
    components.append(uri.path().data(), uri.path().length());
    components.append(uri.id().data(), uri.id().length());

    const bslstl::StringRef canonical = uri.canonical();
    components.append(canonical.data(), canonical.length());

    (void)uri.isCanonical();
    (void)uri.asString();

    return 0;
}
