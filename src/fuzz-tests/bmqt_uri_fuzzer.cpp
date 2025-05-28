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

// bmqt_uri_fuzzer.cpp                                                -*-C++-*-
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <bmqt_uri.h>
#include <bsl_string.h>

using namespace BloombergLP;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    bsl::string fuzz_input(reinterpret_cast<const char*>(data), size);

    bmqt::UriParser::initialize();
    bmqt::Uri   uri;
    bsl::string error;
    int         rc = bmqt::UriParser::parse(&uri, &error, fuzz_input);

    // Check it
    bool valid = uri.isValid();
    if (valid) {
        // read out some values
    }

    return 0;
}
