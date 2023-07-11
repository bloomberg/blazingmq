// Copyright 2022-2023 Bloomberg Finance L.P.
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

// bmqt_subscription.cpp                                              -*-C++-*-
#include <bmqt_subscription.h>

#include <bmqscm_version.h>
// BDE
#include <bsl_limits.h>
#include <bslim_printer.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bsls_annotation.h>

namespace BloombergLP {
namespace bmqt {

// ------------------
// class QueueOptions
// ------------------

const int Subscription::k_CONSUMER_PRIORITY_MIN =
    bsl::numeric_limits<int>::min() / 2;
const int Subscription::k_CONSUMER_PRIORITY_MAX =
    bsl::numeric_limits<int>::max() / 2;
const int Subscription::k_DEFAULT_MAX_UNCONFIRMED_MESSAGES = 1000;
const int Subscription::k_DEFAULT_MAX_UNCONFIRMED_BYTES    = 33554432;
const int Subscription::k_DEFAULT_CONSUMER_PRIORITY        = 0;

// PRIVATE CLASS METHODS

unsigned int SubscriptionHandle::nextId()
{
    static bsls::AtomicUint s_id = 0;

    return ++s_id;
}

}  // close package namespace
}  // close enterprise namespace
