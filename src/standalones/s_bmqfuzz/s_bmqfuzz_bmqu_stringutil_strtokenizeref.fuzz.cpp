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

#include <fuzzer/FuzzedDataProvider.h>
#include <string>

#include <bmqu_stringutil.h>
#include <bsl_string.h>

using namespace BloombergLP;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // define provider
    FuzzedDataProvider provider(data, size);

    // define the parameters of target function
    std::string delims = provider.ConsumeRandomLengthString();
    std::string str    = provider.ConsumeRemainingBytesAsString();

    // call the target function
    bmqu::StringUtil::strTokenizeRef(bsl::string(str.data(), str.size()),
                                     bslstl::StringRef(delims.data(),
                                                       delims.size()));

    return 0;
}
