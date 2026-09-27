// Copyright 2024 Bloomberg Finance L.P.
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

#ifndef INCLUDED_M_BMQSTORAGEWRITER_CSLWRITER
#define INCLUDED_M_BMQSTORAGEWRITER_CSLWRITER

//@PURPOSE: Provide writing of CSL records from JSON input.
//
//@CLASSES:
//  AppIdInfo:       appId and appKey for one application of a queue.
//  QueueCacheEntry: uri and appIds of a queue, keyed by queue key.
//
//@DESCRIPTION: Read CSL records in JSON form and write them to a CSL file,
// building a 'QueueCache' of queue uri and appId information for the journal
// writer to use.

#include <bdljsn_json.h>
#include <bdls_filesystemutil.h>
#include <bsl_map.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <mqbu_storagekey.h>

namespace BloombergLP {
namespace m_bmqstoragewriter {

struct AppIdInfo {
    bsl::string      d_appId;
    mqbu::StorageKey d_appKey;

    BSLMF_NESTED_TRAIT_DECLARATION(AppIdInfo, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an `AppIdInfo` using the specified `allocator` for memory
    /// allocation.
    explicit AppIdInfo(bslma::Allocator* allocator)
    : d_appId(allocator)
    , d_appKey()
    {
    }

    /// Create an `AppIdInfo` having the value of the specified `other`, using
    /// the specified `allocator` for memory allocation.
    AppIdInfo(const AppIdInfo& other, bslma::Allocator* allocator)
    : d_appId(other.d_appId, allocator)
    , d_appKey(other.d_appKey)
    {
    }
};

struct QueueCacheEntry {
    bsl::string            d_uri;
    bsl::vector<AppIdInfo> d_appIds;

    BSLMF_NESTED_TRAIT_DECLARATION(QueueCacheEntry, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `QueueCacheEntry` using the specified `allocator` for memory
    /// allocation.
    explicit QueueCacheEntry(bslma::Allocator* allocator)
    : d_uri(allocator)
    , d_appIds(allocator)
    {
    }

    /// Create a `QueueCacheEntry` having the value of the specified `other`,
    /// using the specified `allocator` for memory allocation.
    QueueCacheEntry(const QueueCacheEntry& other, bslma::Allocator* allocator)
    : d_uri(other.d_uri, allocator)
    , d_appIds(other.d_appIds, allocator)
    {
    }
};

typedef bsl::map<mqbu::StorageKey, QueueCacheEntry> QueueCache;

int processCslInput(QueueCache*                          cache,
                    bdls::FilesystemUtil::FileDescriptor cslFd,
                    bool                                 newFile,
                    bsl::istream&                        input,
                    bslma::Allocator*                    allocator);

}  // close package namespace
}  // close enterprise namespace

#endif
