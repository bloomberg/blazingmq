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

// s_bmqfuzz_parseutil.fuzz.cpp                                       -*-C++-*-
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <mqbcmd_messages.h>
#include <mqbcmd_parseutil.h>

using namespace BloombergLP;
using namespace bsl;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    std::string fuzz_input(reinterpret_cast<const char*>(data), size);
    bsl::string sample_p2 = fuzz_input;

    bsl::string     error;
    mqbcmd::Command actual;
    mqbcmd::ParseUtil::parse(&actual, &error, sample_p2);

    return 0;
}
