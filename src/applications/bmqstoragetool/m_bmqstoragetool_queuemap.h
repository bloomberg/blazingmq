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

#ifndef INCLUDED_M_BMQSTORAGETOOL_QUEUEMAP
#define INCLUDED_M_BMQSTORAGETOOL_QUEUEMAP

//@PURPOSE: Provide a mapping between queue uri and queue key.
//
//@CLASSES:
//  m_bmqstoragetool::QueueMap: a mapping between queue uri and queue key.
//
//@DESCRIPTION: 'QueueMap' provides a mapping of queue uri to queue key
// and mapping of queue key to queue info.

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
// Map of queue key -> queue info.
typedef bsl::unordered_map<bsl::string, mqbu::StorageKey> QueueUriToKeyMap;
// Map of queue uri -> queue key.

// ==============
// class QueueMap
// ==============
class QueueMap {
  private:
    // PRIVATE DATA

    /// Map of queue key -> queue info.
    QueueKeyToInfoMap d_queueKeyToInfoMap;
    /// Map of queue uri -> queue key.
    QueueUriToKeyMap d_queueUriToKeyMap;
    /// Allocator used inside the class.
    bslma::Allocator* d_allocator_p;

  public:
    // CREATORS

    /// Constructor using the specified `allocator`.
    explicit QueueMap(bslma::Allocator* allocator);

    // MANIPULATORS

    /// Insert queue info into internal maps.
    void insert(const bmqp_ctrlmsg::QueueInfo& queueInfo);

    /// Update queue info in internal maps.
    void update(const bmqp_ctrlmsg::QueueInfoUpdate& queueUpdateInfo);

    // ACCESSORS

    /// Find queue info by queue key. Return `true` if key found and
    /// queueInfo_p contains valid data, `false` otherwise.
    bool findInfoByKey(bmqp_ctrlmsg::QueueInfo* queueInfo_p,
                       const mqbu::StorageKey&  key) const;

    /// Find queue key by queue uri. Return `true` if uri found and
    /// queueKey_p contains valid data, `false` otherwise.
    bool findKeyByUri(mqbu::StorageKey*  queueKey_p,
                      const bsl::string& uri) const;

    /// Return all queues info
    bsl::vector<bmqp_ctrlmsg::QueueInfo> queueInfos() const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
