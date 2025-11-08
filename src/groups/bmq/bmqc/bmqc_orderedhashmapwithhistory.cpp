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

// bmqc_orderedhashmapwithhistory.cpp                                 -*-C++-*-
#include <bmqc_orderedhashmapwithhistory.h>

#include <bmqscm_version.h>
namespace BloombergLP {
namespace bmqc {

// -------------------------------------------
// struct OrderedHashMapWithHistory_ImpDetails
// -------------------------------------------

const int
    OrderedHashMapWithHistory_ImpDetails::k_INSERT_GC_MESSAGES_BATCH_SIZE =
        1000;

}  // close package namespace
}  // close enterprise namespace
