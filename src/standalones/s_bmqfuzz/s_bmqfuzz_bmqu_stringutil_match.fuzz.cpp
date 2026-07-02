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

#include <fuzzer/FuzzedDataProvider.h>
#include <string>

#include <bmqu_stringutil.h>
#include <bsl_string.h>

using namespace BloombergLP;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size)
{
    FuzzedDataProvider provider(Data, Size);
    std::string        pattern = provider.ConsumeRandomLengthString();
    std::string        text    = provider.ConsumeRemainingBytesAsString();
    bmqu::StringUtil::match(bsl::string(text.data(), text.size()),
                            bsl::string(pattern.data(), pattern.size()));
    return 0;
}
