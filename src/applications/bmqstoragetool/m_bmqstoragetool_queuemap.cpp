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

// bmqstoragetool
#include <m_bmqstoragetool_queuemap.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

namespace {

/// This class implements unary predicate for checking if a particular
/// AppIdInfo is presented in a vector of them.
class AppIdMatcher {
    // PRIVATE DATA
    const bsl::vector<bmqp_ctrlmsg::AppIdInfo>* d_appIds_p;
    // Pointer to a vector of AppIdInfo for searching in it.

  public:
    /// Constructor with the specified `appIds`.
    explicit AppIdMatcher(const bsl::vector<bmqp_ctrlmsg::AppIdInfo>& appIds)
    : d_appIds_p(&appIds)
    {
    }

    /// Checks if the cpesified `appIdInfo` is presented in the internal
    /// vector. Return `true` if such instance is found, `false` otherwise.
    bool operator()(const bmqp_ctrlmsg::AppIdInfo& appIdInfo)
    {
        return bsl::find(d_appIds_p->begin(), d_appIds_p->end(), appIdInfo) !=
               d_appIds_p->end();
    }
};

}  // close unnamed namespace

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
    mqbu::StorageKey queueKey(mqbu::StorageKey::BinaryRepresentation(),
                              queueInfo.key().begin());
    d_queueKeyToInfoMap[queueKey]       = queueInfo;
    d_queueUriToKeyMap[queueInfo.uri()] = queueKey;
}

void QueueMap::update(const bmqp_ctrlmsg::QueueInfoUpdate& queueInfoUpdate)
{
    mqbu::StorageKey queueKey(mqbu::StorageKey::BinaryRepresentation(),
                              queueInfoUpdate.key().begin());
    QueueKeyToInfoMap::iterator it = d_queueKeyToInfoMap.find(queueKey);
    if (it != d_queueKeyToInfoMap.end()) {
        bsl::vector<bmqp_ctrlmsg::AppIdInfo>& appIds = it->second.appIds();
        // Remove AppIds
        const bsl::vector<bmqp_ctrlmsg::AppIdInfo>& removedAppIds =
            queueInfoUpdate.removedAppIds();
        bsl::erase_if(appIds, AppIdMatcher(removedAppIds));
        // Add AppIds
        const bsl::vector<bmqp_ctrlmsg::AppIdInfo>& addedAppIds =
            queueInfoUpdate.addedAppIds();
        appIds.insert(appIds.cend(), addedAppIds.begin(), addedAppIds.end());
    }
}

// ACCESSORS

bool QueueMap::findInfoByKey(bmqp_ctrlmsg::QueueInfo* queueInfo_p,
                             const mqbu::StorageKey&  key) const
{
    // PRECONDITIONS
    BSLS_ASSERT(queueInfo_p);

    QueueKeyToInfoMap::const_iterator it = d_queueKeyToInfoMap.find(key);
    if (it != d_queueKeyToInfoMap.end()) {
        *queueInfo_p = it->second;
        return true;  // RETURN
    }
    return false;
}

bool QueueMap::findKeyByUri(mqbu::StorageKey*  queueKey_p,
                            const bsl::string& uri) const
{
    // PRECONDITIONS
    BSLS_ASSERT(queueKey_p);

    QueueUriToKeyMap::const_iterator it = d_queueUriToKeyMap.find(uri);
    if (it != d_queueUriToKeyMap.end()) {
        *queueKey_p = it->second;
        return true;  // RETURN
    }
    return false;
}

}  // close package namespace
}  // close enterprise namespace
