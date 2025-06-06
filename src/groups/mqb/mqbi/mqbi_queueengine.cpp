// Copyright 2014-2023 Bloomberg Finance L.P.
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

// mqbi_queueengine.cpp                                               -*-C++-*-
#include <mqbi_queueengine.h>

#include <mqbscm_version.h>

// BMQ
#include <bmqp_queueid.h>

// BDE
#include <bsla_annotations.h>

namespace BloombergLP {
namespace mqbi {

// -----------------
// class QueueEngine
// -----------------

const mqbu::StorageKey
    QueueEngine::k_DEFAULT_APP_KEY(bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);

QueueEngine::~QueueEngine()
{
    // NOTHING
}

void QueueEngine::afterAppIdRegistered(
    BSLA_UNUSED const mqbi::Storage::AppInfos& addedAppIds)
{
    // NOTHING
}

void QueueEngine::afterAppIdUnregistered(
    BSLA_UNUSED const mqbi::Storage::AppInfos& removedAppIds)
{
    // NOTHING
}

void QueueEngine::registerStorage(BSLA_UNUSED const bsl::string& appId,
                                  BSLA_UNUSED const mqbu::StorageKey& appKey,
                                  BSLA_UNUSED unsigned int appOrdinal)
{
    // NOTHING
}

void QueueEngine::unregisterStorage(BSLA_UNUSED const bsl::string& appId,
                                    BSLA_UNUSED const mqbu::StorageKey& appKey,
                                    BSLA_UNUSED unsigned int appOrdinal)
{
    // NOTHING
}

bsl::ostream& QueueEngine::logAppSubscriptionInfo(
    bsl::ostream&           stream,
    BSLA_MAYBE_UNUSED const bsl::string& appId) const
{
    return stream;
};

}  // close package namespace
}  // close enterprise namespace
