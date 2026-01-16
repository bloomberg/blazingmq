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

// mqbstat_dispatcherstats.cpp                                        -*-C++-*-
#include <mqbstat_dispatcherstats.h>

// BMQ
#include <bmqst_statcontext.h>

// BDE
#include <bdlma_localsequentialallocator.h>

namespace BloombergLP {
namespace mqbstat {

namespace {

/// Name of the stat context to create (holding all dispatcher's statistics)
static const char k_DISPATCHER_STAT_NAME[] = "dispatcher";

}  // close unnamed namespace

// ---------------------
// class DomainStatsUtil
// ---------------------

bsl::shared_ptr<bmqst::StatContext>
DispatcherStatsUtil::initializeStatContext(int               historySize,
                                           bslma::Allocator* allocator)
{
    bdlma::LocalSequentialAllocator<1024> localAllocator(allocator);

    bmqst::StatContextConfiguration config(k_DISPATCHER_STAT_NAME, &localAllocator);
    config.isTable(true)
        .defaultHistorySize(historySize)
        .statValueAllocator(allocator)
        .storeExpiredSubcontextValues(true)
        .value("queued_count")
        .value("queued_time", bmqst::StatValue::e_DISCRETE);

    return bsl::shared_ptr<bmqst::StatContext>(
        new (*allocator) bmqst::StatContext(config, allocator),
        allocator);
}

bslma::ManagedPtr<bmqst::StatContext>
    initializeDispatcherQueueStatContext(bmqst::StatContext*      parent,
                                const bslstl::StringRef& name,
                                bslma::Allocator*        allocator)
{
    bdlma::LocalSequentialAllocator<512> localAllocator(allocator);

    bmqst::StatContextConfiguration statConfig(name, &localAllocator);
    return parent->addSubcontext(statConfig);
}                                

}  // close package namespace
}  // close enterprise namespace
