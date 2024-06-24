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

#include <mwctsk_alarmlog.h>

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

// When the 'd_dataStream.erase(msgGUID)' is called
//
//  Queues    | QueueEngines      | Storage           | VSC
// ———————————+———————————————————+———————————————————+———————————————————
//
//    confirmMessage
//    |
//    |—————> onConfirmMessage
//    |       | onRejectMessage
//    |       |           |
//    |       +-----------+—————> confirm ——————————> confirm
//    |                   |
//    |                   |
//    |                   |       processDeletionRecord (1)
//    |                   |       |
//    |                   |       V
//    +———————————————————+—————> remove ———————————> remove
//                                                    |
//                                                    +—————————————> erase
//
// Proxy
//    onHandleReleased
//    |
//    +—————————————————————————> removeVirtualStorage
//                                |
//    BroadcastMode               +—————————————————> removeVirtualStorage
//    |                                               |
//    |       Primary                                 |
//    |       afterAppIdUnregistered                  |
//    |       |                                       |
//    |       |                                       |
//    +———————+—————————————> removeAll               |
//                                |   purge           |
//                                |   |               |
//                                V   V               |
//                                purgeCommon         |
//                                |                   V
//                                +—————————————————> removeAll
//                                                    |
//                                releaseRef (1) <————+—————————————> erase
//
//
//
//                                gcExpiredMessages (1)
//                                |
//                                |
//                                +—————————————————> gc
//                                                    |
//                                                    +—————————————> erase
//
// (1) QE::beforeMessageRemoved
//

// CREATORS
VirtualStorageCatalog::VirtualStorageCatalog(mqbi::Storage*    storage,
                                             bslma::Allocator* allocator)
: d_storage_p(storage)
, d_virtualStorages(allocator)
, d_avaialbleOrdinals(allocator)
, d_nextOrdinal(0)
, d_dataStream(allocator)
, d_totalBytes(0)
, d_numMessages(0)
, d_defaultAppMessage(defaultAppMessage().d_rdaInfo)
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

VirtualStorageCatalog::DataStreamIterator
VirtualStorageCatalog::begin(const bmqt::MessageGUID& where)
{
    if (where.isUnset()) {
        return d_dataStream.begin();
    }
    else {
        return d_dataStream.find(where);
    }
}

VirtualStorageCatalog::DataStreamIterator VirtualStorageCatalog::end()
{
    return d_dataStream.end();
}

VirtualStorageCatalog::DataStreamIterator
VirtualStorageCatalog::get(const bmqt::MessageGUID& msgGUID)
{
    DataStreamIterator it = d_dataStream.find(msgGUID);

    if (it != d_dataStream.end()) {
        setup(&it->second);
    }

    return it;
}

void VirtualStorageCatalog::setup(VirtualStorage::DataStreamMessage* data)
{
    // The only case for subsequent resize is proxy receiving subsequent PUSH
    // messages for the same GUID and different apps
    if (data->d_apps.size() < d_nextOrdinal) {
        data->d_apps.resize(d_nextOrdinal, defaultAppMessage());
    }
}

mqbi::StorageResult::Enum
VirtualStorageCatalog::put(const bmqt::MessageGUID&            msgGUID,
                           int                                 msgSize,
                           VirtualStorage::DataStreamMessage** out)
{
    bsl::pair<VirtualStorage::DataStreamIterator, bool> insertResult =
        d_dataStream.insert(bsl::make_pair(
            msgGUID,
            VirtualStorage::DataStreamMessage(msgSize, d_allocator_p)));

    if (!insertResult.second) {
        // Duplicate GUID
        return mqbi::StorageResult::e_GUID_NOT_UNIQUE;  // RETURN
    }

    d_totalBytes += msgSize;
    ++d_numMessages;

    if (out) {
        // The auto-confirm case when we need to update App states.
        *out = &insertResult.first->second;

        setup(*out);
    }

    return mqbi::StorageResult::e_SUCCESS;  // RETURN
}

bslma::ManagedPtr<mqbi::StorageIterator>
VirtualStorageCatalog::getIterator(const mqbu::StorageKey& appKey)
{
    bslma::ManagedPtr<mqbi::StorageIterator> mp;

    if (!appKey.isNull()) {
        VirtualStoragesIter it = d_virtualStorages.findByKey2(appKey);
        BSLS_ASSERT_SAFE(it != d_virtualStorages.end());

        VirtualStorage* vs = it->value().get();

        mp.load(new (*d_allocator_p)
                    VirtualStorageIterator(it->value().get(),
                                           d_storage_p,
                                           this,
                                           d_dataStream.begin()),
                d_allocator_p);

        if (!mp->atEnd()) {
            if (!mp->appMessageView(vs->ordinal()).isPending()) {
                // By contract, the iterator iterates only pending states.
                mp->advance();
            }
        }
    }
    else {
        mp.load(new (*d_allocator_p)
                    StorageIterator(d_storage_p, this, d_dataStream.begin()),
                d_allocator_p);
    }

    return mp;
}

mqbi::StorageResult::Enum VirtualStorageCatalog::getIterator(
    bslma::ManagedPtr<mqbi::StorageIterator>* out,
    const mqbu::StorageKey&                   appKey,
    const bmqt::MessageGUID&                  msgGUID)
{
    DataStreamIterator data = d_dataStream.find(msgGUID);

    if (data == d_dataStream.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    if (!appKey.isNull()) {
        VirtualStoragesIter it = d_virtualStorages.findByKey2(appKey);
        BSLS_ASSERT_SAFE(it != d_virtualStorages.end());

        VirtualStorage* vs = it->value().get();

        out->load(new (*d_allocator_p)
                      VirtualStorageIterator(vs, d_storage_p, this, data),
                  d_allocator_p);

        const mqbi::AppMessage& appView = (*out)->appMessageView(
            vs->ordinal());

        if (!appView.isPending()) {
            // By contract, the iterator iterates only pending states.

            return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
        }
    }
    else {
        out->load(new (*d_allocator_p)
                      StorageIterator(d_storage_p, this, data),
                  d_allocator_p);
    }

    return mqbi::StorageResult::e_SUCCESS;
}

mqbi::StorageResult::Enum
VirtualStorageCatalog::confirm(const bmqt::MessageGUID& msgGUID,
                               const mqbu::StorageKey&  appKey)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!appKey.isNull());

    VirtualStorage::DataStreamIterator data = get(msgGUID);
    if (data == d_dataStream.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    VirtualStoragesIter it = d_virtualStorages.findByKey2(appKey);
    BSLS_ASSERT_SAFE(it != d_virtualStorages.end());

    setup(&data->second);

    return it->value()->confirm(&data->second);
}

mqbi::StorageResult::Enum
VirtualStorageCatalog::remove(const bmqt::MessageGUID& msgGUID)
{
    // Remove all Apps states at once.
    if (0 == d_dataStream.erase(msgGUID)) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    return mqbi::StorageResult::e_SUCCESS;
}

mqbi::StorageResult::Enum
VirtualStorageCatalog::gc(const bmqt::MessageGUID& msgGUID)
{
    VirtualStorage::DataStreamIterator data = d_dataStream.find(msgGUID);
    if (data == d_dataStream.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    // Update each App so the math of numMessages/numBytes stays correct.
    // REVISIT.
    for (VirtualStoragesIter it = d_virtualStorages.begin();
         it != d_virtualStorages.end();
         ++it) {
        it->value()->onGC(data->second);
    }

    d_totalBytes -= data->second.d_size;
    --d_numMessages;

    d_dataStream.erase(data);

    return mqbi::StorageResult::e_SUCCESS;
}

mqbi::StorageResult::Enum
VirtualStorageCatalog::removeAll(const mqbu::StorageKey& appKey)
{
    if (!appKey.isNull()) {
        VirtualStoragesIter itVs = d_virtualStorages.findByKey2(appKey);
        BSLS_ASSERT_SAFE(itVs != d_virtualStorages.end());

        for (DataStreamIterator itData = d_dataStream.begin();
             itData != d_dataStream.end();) {
            mqbi::StorageResult::Enum result = mqbi::StorageResult::e_SUCCESS;

            VirtualStorage::DataStreamMessage* data = &itData->second;
            setup(data);

            if (itVs->value()->remove(data) ==
                mqbi::StorageResult::e_SUCCESS) {
                // The 'data' was not already removed or confirmed.
                result = d_storage_p->releaseRef(itData->first);
            }

            if (result == mqbi::StorageResult::e_ZERO_REFERENCES) {
                itData = d_dataStream.erase(itData);
            }
            else {
                if (result == mqbi::StorageResult::e_GUID_NOT_FOUND) {
                    BALL_LOG_WARN
                        << "#STORAGE_PURGE_ERROR "
                        << "PartitionId [" << d_storage_p->partitionId() << "]"
                        << ": Attempting to purge GUID '" << itData->first
                        << "' from virtual storage with appId '"
                        << itVs->value()->appId() << "' & appKey '" << appKey
                        << "' for queue '" << d_storage_p->queueUri()
                        << "' & queueKey '" << d_storage_p->queueKey()
                        << "', but GUID does not exist in the underlying "
                           "storage.";
                }
                else if (result ==
                         mqbi::StorageResult::e_NON_ZERO_REFERENCES) {
                }
                else if (result == mqbi::StorageResult::e_SUCCESS) {
                }
                else {
                    MWCTSK_ALARMLOG_ALARM("STORAGE_PURGE_ERROR")
                        << "PartitionId [" << d_storage_p->partitionId() << "]"
                        << ": Attempting to purge GUID '" << itData->first
                        << "' from virtual storage with appId '"
                        << itVs->value()->appId() << "' & appKey '" << appKey
                        << "] for queue '" << d_storage_p->queueUri()
                        << "' & queueKey '" << d_storage_p->queueKey()
                        << "', with invalid context (refCount is already "
                           "zero)."
                        << MWCTSK_ALARMLOG_END;
                }
                ++itData;
            }
        }
    }
    else {
        for (VirtualStoragesIter it = d_virtualStorages.begin();
             it != d_virtualStorages.end();
             ++it) {
            it->value()->resetStats();
        }
        d_dataStream.clear();
        d_numMessages = 0;
        d_totalBytes  = 0;
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

    Ordinal appOrdinal;

    // Ordinals ever grow.
    if (d_avaialbleOrdinals.empty()) {
        appOrdinal = d_nextOrdinal++;
    }
    else {
        appOrdinal = d_avaialbleOrdinals.front();
        // There is no conflict because everything 'appOrdinal' was removed.
        d_avaialbleOrdinals.pop_front();
    }

    BSLS_ASSERT_SAFE(appOrdinal <= d_virtualStorages.size());

    VirtualStorageSp vsp;
    vsp.createInplace(d_allocator_p,
                      d_storage_p,
                      appId,
                      appKey,
                      appOrdinal,
                      d_allocator_p);
    d_virtualStorages.insert(appId, appKey, vsp);

    return 0;
}

bool VirtualStorageCatalog::removeVirtualStorage(
    const mqbu::StorageKey& appKey)
{
    if (appKey.isNull()) {
        // Make sure there is no AppMessage in the pending states
        removeAll(appKey);

        // Remove all virtual storages
        d_virtualStorages.clear();
        d_avaialbleOrdinals.clear();
        d_nextOrdinal = 0;
        return true;  // RETURN
    }

    VirtualStoragesConstIter it = d_virtualStorages.findByKey2(appKey);
    if (it != d_virtualStorages.end()) {
        // Make sure there is no AppMessage in the pending states
        removeAll(appKey);

        const VirtualStorage& vs = *it->value();
        d_avaialbleOrdinals.push_back(vs.ordinal());
        d_virtualStorages.erase(it);
        return true;  // RETURN
    }

    return false;
}

VirtualStorage*
VirtualStorageCatalog::virtualStorage(const mqbu::StorageKey& appKey)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!appKey.isNull());

    VirtualStoragesIter it = d_virtualStorages.findByKey2(appKey);
    if (it == d_virtualStorages.end()) {
        return 0;
    }
    else {
        return it->value().get();
    }
}

void VirtualStorageCatalog::autoConfirm(
    VirtualStorage::DataStreamMessage* dataStreamMessage,
    const mqbu::StorageKey&            appKey)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dataStreamMessage);
    BSLS_ASSERT_SAFE(!appKey.isNull());

    VirtualStoragesIter it = d_virtualStorages.findByKey2(appKey);
    BSLS_ASSERT_SAFE(it != d_virtualStorages.end());

    it->value()->confirm(dataStreamMessage);
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
        }
        else {
            *appId = "";
        }
    }

    return hasVs;
}

bool VirtualStorageCatalog::hasVirtualStorage(const bsl::string& appId,
                                              mqbu::StorageKey*  appKey,
                                              unsigned int*      ordinal) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!appId.empty());

    VirtualStoragesConstIter cit   = d_virtualStorages.findByKey1(appId);
    const bool               hasVs = (cit != d_virtualStorages.end());

    if (appKey) {
        if (hasVs) {
            *appKey = cit->key2();
        }
        else {
            *appKey = mqbu::StorageKey::k_NULL_KEY;
        }
    }

    if (ordinal) {
        if (hasVs) {
            *ordinal = cit->value()->ordinal();
        }
    }

    return hasVs;
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
