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

// mwcc_multiqueuethreadpool.cpp                                      -*-C++-*-
#include <mwcc_multiqueuethreadpool.h>

#include <mwcscm_version.h>
// BDE
#include <bslma_managedptr.h>

namespace BloombergLP {
namespace mwcc {

// ------------------------------------------
// class MultiQueueThreadPool_QueueCreatorRet
// ------------------------------------------

// CREATORS
MultiQueueThreadPool_QueueCreatorRet::MultiQueueThreadPool_QueueCreatorRet(
    bslma::Allocator* basicAllocator)
: d_context_mp()
, d_name(basicAllocator)
{
    // NOTHING
}

// MANIPULATORS
bslma::ManagedPtr<void>& MultiQueueThreadPool_QueueCreatorRet::context()
{
    return d_context_mp;
}

// ACCESSORS
const bsl::string& MultiQueueThreadPool_QueueCreatorRet::name()
{
    return d_name;
}

}  // close package namespace
}  // close enterprise namespace
