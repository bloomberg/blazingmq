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

// m_bmqstoragetool_queuemap.h -*-C++-*-
#ifndef INCLUDED_M_BMQSTORAGETOOL_QUEUEMAP
#define INCLUDED_M_BMQSTORAGETOOL_QUEUEMAP

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// MQB
#include <mqbs_filestoreprotocol.h>

// BDE
#include <bsl_unordered_map.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

typedef bsl::unordered_map<mqbu::StorageKey, bmqp_ctrlmsg::QueueInfo>
                                                          QueueKeyToInfoMap;
typedef bsl::unordered_map<bsl::string, mqbu::StorageKey> QueueUriToKeyMap;

// ==============
// class QueueMap
// ==============
class QueueMap {
    QueueKeyToInfoMap d_queueKeyToInfoMap;
    QueueUriToKeyMap  d_queueUriToKeyMap;

  public:
    // CREATORS

    explicit QueueMap(bslma::Allocator* allocator);

    // MANIPULATORS

    void insert(const bmqp_ctrlmsg::QueueInfo& queueInfo);
    // Insert queue info data into internal maps

    void update(const bmqp_ctrlmsg::QueueInfoUpdate& queueUpdateInfo);
    // Update queue info data in internal maps

    // ACCESSORS

    bool findInfoByKey(bmqp_ctrlmsg::QueueInfo* queueInfo_p,
                       const mqbu::StorageKey&  key) const;
    // Find queue info by queue key. Return 'true' if key found and
    // queueInfo_p contains valid data, 'false' otherwise.

    bool findKeyByUri(mqbu::StorageKey*  queueKey_p,
                      const bsl::string& uri) const;
    // Find queue info by queue key. Return 'true' if uri found and
    // queueKey_p contains valid data, 'false' otherwise.
};

}  // close package namespace
}  // close enterprise namespace

#endif
