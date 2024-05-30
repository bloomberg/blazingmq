// Copyright 2017-2023 Bloomberg Finance L.P.
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

// mqbs_virtualstoragecatalog.cpp                                     -*-C++-*-
#include <mqbs_virtualstoragecatalog.h>

#include <mqbscm_version.h>
// MQB
#include <mqbi_queueengine.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_utility.h>
#include <bslma_allocator.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbs {

// ---------------------------
// class VirtualStorageCatalog
// ---------------------------

// CREATORS
VirtualStorageCatalog::VirtualStorageCatalog(mqbi::Storage*    storage,
                                             bslma::Allocator* allocator)
: d_storage_p(storage)
, d_virtualStorages(allocator)
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(storage);
    BSLS_ASSERT_SAFE(allocator);
}

VirtualStorageCatalog::~VirtualStorageCatalog()
{
    // TBD: Should it be asserted here that 'd_virtualStorages' is empty?
    d_virtualStorages.clear();
}

// MANIPULATORS
mqbi::StorageResult::Enum
VirtualStorageCatalog::put(const bmqt::MessageGUID& msgGUID,
                           int                      msgSize,
                           const bmqp::RdaInfo&     rdaInfo,
                           unsigned int             subScriptionId,
                           const mqbu::StorageKey&  appKey)
{
    if (!appKey.isNull()) {
        VirtualStoragesIter it = d_virtualStorages.findByKey2(appKey);
        BSLS_ASSERT_SAFE(it != d_virtualStorages.end());

        return it->value()->put(msgGUID,
                                msgSize,
                                rdaInfo,
                                subScriptionId);  // RETURN
    }

    // Add guid to all virtual storages.

    for (VirtualStoragesIter it = d_virtualStorages.begin();
         it != d_virtualStorages.end();
         ++it) {
        it->value()->put(msgGUID, msgSize, rdaInfo, subScriptionId);
    }

    return mqbi::StorageResult::e_SUCCESS;  // RETURN
}

bslma::ManagedPtr<mqbi::StorageIterator>
VirtualStorageCatalog::getIterator(const mqbu::StorageKey& appKey)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!appKey.isNull());

    VirtualStoragesIter it = d_virtualStorages.findByKey2(appKey);
    BSLS_ASSERT_SAFE(it != d_virtualStorages.end());
    return it->value()->getIterator(appKey);
}

mqbi::StorageResult::Enum VirtualStorageCatalog::getIterator(
    bslma::ManagedPtr<mqbi::StorageIterator>* out,
    const mqbu::StorageKey&                   appKey,
    const bmqt::MessageGUID&                  msgGUID)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!appKey.isNull());

    VirtualStoragesIter it = d_virtualStorages.findByKey2(appKey);
    BSLS_ASSERT_SAFE(it != d_virtualStorages.end());
    return it->value()->getIterator(out, appKey, msgGUID);
}

mqbi::StorageResult::Enum
VirtualStorageCatalog::remove(const bmqt::MessageGUID& msgGUID,
                              const mqbu::StorageKey&  appKey)
{
    if (!appKey.isNull()) {
        VirtualStoragesIter it = d_virtualStorages.findByKey2(appKey);
        BSLS_ASSERT_SAFE(it != d_virtualStorages.end());
        return it->value()->remove(msgGUID);  // RETURN
    }

    // Remove guid from all virtual storages.
    for (VirtualStoragesIter it = d_virtualStorages.begin();
         it != d_virtualStorages.end();
         ++it) {
        it->value()->remove(msgGUID);  // ignore rc
    }

    return mqbi::StorageResult::e_SUCCESS;
}

mqbi::StorageResult::Enum
VirtualStorageCatalog::removeAll(const mqbu::StorageKey& appKey)
{
    if (!appKey.isNull()) {
        VirtualStoragesIter it = d_virtualStorages.findByKey2(appKey);
        BSLS_ASSERT_SAFE(it != d_virtualStorages.end());
        return it->value()->removeAll(appKey);  // RETURN
    }

    // Clear all virtual storages.
    for (VirtualStoragesIter it = d_virtualStorages.begin();
         it != d_virtualStorages.end();
         ++it) {
        it->value()->removeAll(it->key2());  // ignore rc
    }

    return mqbi::StorageResult::e_SUCCESS;
}

int VirtualStorageCatalog::addVirtualStorage(bsl::ostream& errorDescription,
                                             const bsl::string&      appId,
                                             const mqbu::StorageKey& appKey)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!appId.empty());
    BSLS_ASSERT_SAFE(!appKey.isNull());

    VirtualStoragesConstIter cit = d_virtualStorages.findByKey2(appKey);
    if (cit != d_virtualStorages.end()) {
        const VirtualStorage* vs = cit->value().get();
        BSLS_ASSERT_SAFE(!vs->appKey().isNull());

        errorDescription << "Virtual storage exists with same appKey. "
                         << "Specified appId & appKey: [" << appId << "] & ["
                         << appKey << "]. Existing appId & appKey: ["
                         << vs->appId() << "] & [" << vs->appKey() << "].";
        return -1;  // RETURN
    }

    VirtualStorageSp vsp;
    vsp.createInplace(d_allocator_p,
                      d_storage_p,
                      appId,
                      appKey,
                      d_allocator_p);
    d_virtualStorages.insert(appId, appKey, vsp);

    return 0;
}

bool VirtualStorageCatalog::removeVirtualStorage(
    const mqbu::StorageKey& appKey)
{
    if (appKey.isNull()) {
        // Remove all virtual storages
        d_virtualStorages.clear();
        return true;  // RETURN
    }

    VirtualStoragesConstIter it = d_virtualStorages.findByKey2(appKey);
    if (it != d_virtualStorages.end()) {
        d_virtualStorages.erase(it);
        return true;  // RETURN
    }

    return false;
}

mqbi::Storage*
VirtualStorageCatalog::virtualStorage(const mqbu::StorageKey& appKey)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!appKey.isNull());

    VirtualStoragesIter it = d_virtualStorages.findByKey2(appKey);
    BSLS_ASSERT_SAFE(it != d_virtualStorages.end());
    return it->value().get();
}

void VirtualStorageCatalog::autoConfirm(const bmqt::MessageGUID& msgGUID,
                                        const mqbu::StorageKey&  appKey)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!appKey.isNull());

    VirtualStoragesIter it = d_virtualStorages.findByKey2(appKey);
    BSLS_ASSERT_SAFE(it != d_virtualStorages.end());

    it->value()->autoConfirm(msgGUID);
}

// ACCESSORS
bool VirtualStorageCatalog::hasVirtualStorage(const mqbu::StorageKey& appKey,
                                              bsl::string* appId) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!appKey.isNull());

    VirtualStoragesConstIter cit   = d_virtualStorages.findByKey2(appKey);
    const bool               hasVs = (cit != d_virtualStorages.end());

    if (appId) {
        if (hasVs) {
            *appId = cit->value()->appId();
            return true;  // RETURN
        }

        *appId = "";
    }

    return hasVs;
}

bool VirtualStorageCatalog::hasVirtualStorage(const bsl::string& appId,
                                              mqbu::StorageKey*  appKey) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!appId.empty());

    VirtualStoragesConstIter cit   = d_virtualStorages.findByKey1(appId);
    const bool               hasVs = (cit != d_virtualStorages.end());

    if (appKey) {
        if (hasVs) {
            *appKey = cit->key2();
            return true;  // RETURN
        }

        *appKey = mqbu::StorageKey::k_NULL_KEY;
    }

    return hasVs;
}

bool VirtualStorageCatalog::hasMessage(const bmqt::MessageGUID& msgGUID) const
{
    for (VirtualStoragesConstIter it = d_virtualStorages.begin();
         it != d_virtualStorages.end();
         ++it) {
        if (it->value()->hasMessage(msgGUID)) {
            return true;  // RETURN
        }
    }

    return false;
}

void VirtualStorageCatalog::loadVirtualStorageDetails(
    AppIdKeyPairs* buffer) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(buffer);

    for (VirtualStoragesConstIter cit = d_virtualStorages.begin();
         cit != d_virtualStorages.end();
         ++cit) {
        BSLS_ASSERT_SAFE(cit->key2() == cit->value()->appKey());
        buffer->push_back(bsl::make_pair(cit->key1(), cit->key2()));
    }
}

}  // close package namespace
}  // close enterprise namespace
