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

// mqbstat_dispatcherstats.cpp                                        -*-C++-*-
#include <mqbstat_dispatcherstats.h>

#include <mqbscm_version.h>
// BMQ
#include <bmqt_uri.h>

// MQB
#include <mqbi_cluster.h>
#include <mqbi_domain.h>

// MWC
#include <mwcst_statcontext.h>
#include <mwcst_statutil.h>
#include <mwcst_statvalue.h>

// BDE
#include <bdld_datummapbuilder.h>
#include <bdld_manageddatum.h>
#include <bdlma_localsequentialallocator.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbstat {

// ---------------------
// class DomainStatsUtil
// ---------------------

bsl::shared_ptr<mwcst::StatContext>
DispatcherStatsUtil::initializeStatContext(int               historySize,
                                           bslma::Allocator* allocator)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    mwcst::StatContextConfiguration config("dispatcher", &localAllocator);
    config.defaultHistorySize(historySize)
        .statValueAllocator(allocator)
        .storeExpiredSubcontextValues(true);

    return bsl::shared_ptr<mwcst::StatContext>(
        new (*allocator) mwcst::StatContext(config, allocator),
        allocator);
}

bslma::ManagedPtr<mwcst::StatContext>
DispatcherStatsUtil::initializeClientStatContext(mwcst::StatContext* parent,
                                                 const bslstl::StringRef& name,
                                                 bslma::Allocator* allocator)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    mwcst::StatContextConfiguration statConfig(name, &localAllocator);
    statConfig.isTable(true)
        .value("enq_undefined")
        .value("enq_dispatcher")
        .value("enq_callback")
        .value("enq_control_msg")
        .value("enq_confirm")
        .value("enq_reject")
        .value("enq_push")
        .value("enq_put")
        .value("enq_ack")
        .value("enq_cluster_state")
        .value("enq_storage")
        .value("enq_recovery")
        .value("enq_replication_receipt")
        .value("done_undefined")
        .value("done_dispatcher")
        .value("done_callback")
        .value("done_control_msg")
        .value("done_confirm")
        .value("done_reject")
        .value("done_push")
        .value("done_put")
        .value("done_ack")
        .value("done_cluster_state")
        .value("done_storage")
        .value("done_recovery")
        .value("done_replication_receipt")
        .value("nb_client");
    return parent->addSubcontext(statConfig);
}

}  // close package namespace
}  // close enterprise namespace
