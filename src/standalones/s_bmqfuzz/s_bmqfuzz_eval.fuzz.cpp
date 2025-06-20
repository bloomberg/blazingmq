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

// eval_fuzzer.cpp                                                    -*-C++-*-
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <bmqeval_simpleevaluator.h>
#include <bmqeval_simpleevaluatorparser.hpp>
#include <bmqeval_simpleevaluatorscanner.h>

#include <bdlma_localsequentialallocator.h>

using namespace BloombergLP;
using namespace bsl;
using namespace BloombergLP::bmqeval;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    std::string fuzz_input(reinterpret_cast<const char*>(data), size);

    bdlma::LocalSequentialAllocator<2048> localAllocator;
    CompilationContext                    compilationContext(&localAllocator);
    SimpleEvaluator                       evaluator;
    try {
        evaluator.compile(fuzz_input, compilationContext);
    }
    catch (...) {
    }

    return 0;
}
