// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mwcc_orderedhashmap.cpp                                            -*-C++-*-
#include <mwcc_orderedhashmap.h>

#include <mwcscm_version.h>
// BDE
#include <bsl_algorithm.h>

namespace BloombergLP {
namespace mwcc {

// --------------------------------
// struct OrderedHashMap_ImpDetails
// --------------------------------

size_t OrderedHashMap_ImpDetails::nextPrime(size_t n)
{
    // This routine is copied from bslstl_hashtable.h, which cannot be included
    // in this component.

    static const size_t s_primes[] = {
        2,        5,         13,        29,        61,         127,
        257,      521,       1049,      2099,      4201,       8419,
        16843,    33703,     67409,     134837,    269513,     539039,
        1078081,  2156171,   5312353,   10624709,  21249443,   42498893,
        84997793, 169995589, 339991181, 679982363, 1359964751, 2719929503u};

    static const size_t s_nPrimes = sizeof(s_primes) / sizeof(s_primes[0]);
    static const size_t* const s_beginPrimes = s_primes;
    static const size_t* const s_endPrimes   = s_primes + s_nPrimes;

    const size_t* result = bsl::lower_bound(s_beginPrimes, s_endPrimes, n);

    if (s_endPrimes == result) {
        return 0;  // RETURN
    }

    return *result;
}

}  // close package namespace
}  // close enterprise namespace
