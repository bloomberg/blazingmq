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

// m_bmqstoragetool_queuemap.cpp -*-C++-*-

// bmqstoragetool
#include <m_bmqstoragetool_queuemap.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// ==============
// class QueueMap
// ==============

// CREATORS

QueueMap::QueueMap(bslma::Allocator* allocator)
: d_queueKeyToInfoMap(allocator)
, d_queueUriToKeyMap(allocator)
{
    // NOTHING
}

// MANIPULATORS

void QueueMap::insert(const bmqp_ctrlmsg::QueueInfo& queueInfo)
{
    auto queueKey = mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     queueInfo.key().begin());
    d_queueKeyToInfoMap[queueKey]       = queueInfo;
    d_queueUriToKeyMap[queueInfo.uri()] = queueKey;
}

void QueueMap::update(const bmqp_ctrlmsg::QueueInfoUpdate& queueInfoUpdate)
{
    auto queueKey = mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     queueInfoUpdate.key().begin());
    if (auto it = d_queueKeyToInfoMap.find(queueKey);
        it != d_queueKeyToInfoMap.end()) {
        bsl::vector<bmqp_ctrlmsg::AppIdInfo>& appIds = it->second.appIds();
        // Remove AppIds
        const bsl::vector<bmqp_ctrlmsg::AppIdInfo>& removedAppIds =
            queueInfoUpdate.removedAppIds();
        bsl::erase_if(
            appIds,
            [&removedAppIds](const bmqp_ctrlmsg::AppIdInfo& appIdInfo) {
                return bsl::find(removedAppIds.begin(),
                                 removedAppIds.end(),
                                 appIdInfo) != removedAppIds.end();
            });
        // Add AppIds
        const bsl::vector<bmqp_ctrlmsg::AppIdInfo>& addedAppIds =
            queueInfoUpdate.addedAppIds();
        appIds.insert(appIds.end(), addedAppIds.begin(), addedAppIds.end());
    }
}

// ACCESSORS

bool QueueMap::findInfoByKey(bmqp_ctrlmsg::QueueInfo* queueInfo_p,
                             const mqbu::StorageKey&  key) const
{
    if (auto it = d_queueKeyToInfoMap.find(key);
        it != d_queueKeyToInfoMap.end()) {
        *queueInfo_p = it->second;
        return true;
    }
    return false;
}

bool QueueMap::findKeyByUri(mqbu::StorageKey*  queueKey_p,
                            const bsl::string& uri) const
{
    if (auto it = d_queueUriToKeyMap.find(uri);
        it != d_queueUriToKeyMap.end()) {
        *queueKey_p = it->second;
        return true;
    }
    return false;
}

}  // close package namespace
}  // close enterprise namespace
