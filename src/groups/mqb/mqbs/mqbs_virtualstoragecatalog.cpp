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
#include <mqbstat_queuestats.h>

#include <bmqtsk_alarmlog.h>

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
, d_dataStream(allocator)
, d_totalBytes(0)
, d_numMessages(0)
, d_defaultAppMessage(bmqp::RdaInfo())
, d_defaultNonApplicableAppMessage(bmqp::RdaInfo())
, d_isProxy(false)
, d_queue_p(0)
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(storage);
    BSLS_ASSERT_SAFE(allocator);

    d_defaultNonApplicableAppMessage.setRemovedState();
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

mqbi::StorageResult::Enum
VirtualStorageCatalog::put(const bmqt::MessageGUID&  msgGUID,
                           int                       msgSize,
                           unsigned int              refCount,
                           mqbi::DataStreamMessage** out)
{
    bsl::pair<VirtualStorage::DataStreamIterator, bool> insertResult =
        d_dataStream.insert(bsl::make_pair(
            msgGUID,
            mqbi::DataStreamMessage(refCount, msgSize, d_allocator_p)));

    mqbi::StorageResult::Enum result = mqbi::StorageResult::e_SUCCESS;

    mqbi::DataStreamMessage& dataStreamMessage = insertResult.first->second;

    if (!insertResult.second) {
        // Duplicate GUID
        if (d_isProxy) {
            // A proxy can receive subsequent PUSH messages for the same GUID
            // but different apps
            BSLS_ASSERT_SAFE(refCount <= d_ordinals.size());

            dataStreamMessage.d_numApps = refCount;
        }
        result = mqbi::StorageResult::e_GUID_NOT_UNIQUE;
    }
    else {
        d_totalBytes += msgSize;
        ++d_numMessages;
    }

    if (out) {
        // The auto-confirm case when we need to update App states.
        *out = &dataStreamMessage;

        setup(*out);
    }

    return result;
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

    const mqbi::StorageResult::Enum rc = it->value()->confirm(&data->second);
    if (queue() && mqbi::StorageResult::e_SUCCESS == rc) {
        queue()->stats()->onEvent(
            mqbstat::QueueStatsDomain::EventType::e_DEL_MESSAGE,
            data->second.d_size,
            it->key1());
    }

    return rc;
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
        const mqbi::DataStreamMessage& dataStreamMessage = data->second;

        const mqbi::AppMessage& appMessage =
            appMessageView(dataStreamMessage, it->value()->ordinal());
        if (!appMessage.isPending()) {
            it->value()->onGC(dataStreamMessage.d_size);
        }
    }

    d_totalBytes -= data->second.d_size;
    --d_numMessages;

    d_dataStream.erase(data);

    return mqbi::StorageResult::e_SUCCESS;
}

void VirtualStorageCatalog::removeAll()
{
    for (VirtualStoragesIter it = d_virtualStorages.begin();
         it != d_virtualStorages.end();
         ++it) {
        it->value()->resetStats();
    }
    d_dataStream.clear();
    d_numMessages = 0;
    d_totalBytes  = 0;
}

void VirtualStorageCatalog::removeAll(const mqbu::StorageKey& appKey)
{
    BSLS_ASSERT_SAFE(!appKey.isNull());

    VirtualStoragesIter itVs = d_virtualStorages.findByKey2(appKey);
    BSLS_ASSERT_SAFE(itVs != d_virtualStorages.end());
    VirtualStorage* vs = itVs->value().get();

    DataStreamIterator itData;
    bsls::Types::Int64 total = d_dataStream.size();
    if (seek(&itData, vs) < total) {
        purgeImpl(vs, itData, numVirtualStorages());
    }
}

bsls::Types::Int64 VirtualStorageCatalog::seek(DataStreamIterator*   it,
                                               const VirtualStorage* vs,
                                               bsls::Types::Int64*   bytes)
{
    BSLS_ASSERT_SAFE(vs);

    DataStreamIterator& itData = *it;  // alias

    itData = d_dataStream.begin();

    if (d_isProxy) {
        return 0;
    }

    bsls::Types::Int64 result = 0;
    for (; itData != d_dataStream.end(); ++itData, ++result) {
        const mqbi::DataStreamMessage& data    = itData->second;
        const unsigned int             numApps = data.d_numApps;

        if (vs->ordinal() < numApps) {
            break;  // BREAK
        }
        // else 'dataStreamMessage' is older than this VirtualStorage

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(bytes)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            *bytes += data.d_size;
        }
    }

    return result;
}

mqbi::StorageResult::Enum
VirtualStorageCatalog::purge(const mqbu::StorageKey& appKey,
                             const PurgeCallback&    cb)
{
    BSLS_ASSERT_SAFE(!appKey.isNull());

    VirtualStoragesConstIter it = d_virtualStorages.findByKey2(appKey);
    if (it == d_virtualStorages.end()) {
        return mqbi::StorageResult::e_APPKEY_NOT_FOUND;
    }

    VirtualStorage*    vs = it->value().get();
    DataStreamIterator itData;
    bsls::Types::Int64 total = d_dataStream.size();

    if (seek(&itData, vs) < total) {
        if (cb) {
            mqbi::StorageResult::Enum rc = cb(appKey, itData);
            if (mqbi::StorageResult::e_SUCCESS != rc) {
                return rc;
            }
        }

        purgeImpl(vs, itData, numVirtualStorages());
    }

    return mqbi::StorageResult::e_SUCCESS;
}

mqbi::StorageResult::Enum
VirtualStorageCatalog::purgeImpl(VirtualStorage*     vs,
                                 DataStreamIterator& itData,
                                 unsigned int        replacingOrdinal)
{
    BSLS_ASSERT_SAFE(!d_ordinals.empty());

    while (itData != d_dataStream.end()) {
        const bmqt::MessageGUID& msgGUID           = itData->first;
        mqbi::DataStreamMessage* dataStreamMessage = &itData->second;

        mqbi::StorageResult::Enum result = mqbi::StorageResult::e_SUCCESS;

        // Must call 'seek' before 'purge'
        setup(dataStreamMessage);

        if (vs->remove(dataStreamMessage, replacingOrdinal)) {
            // The 'data' was not already removed or confirmed.
            result = d_storage_p->releaseRef(msgGUID);
        }

        if (result == mqbi::StorageResult::e_ZERO_REFERENCES) {
            itData = d_dataStream.erase(itData);
        }
        else {
            if (result == mqbi::StorageResult::e_GUID_NOT_FOUND) {
                BALL_LOG_WARN
                    << "#STORAGE_PURGE_ERROR " << "Partition ["
                    << d_storage_p->partitionId() << "]"
                    << ": Attempting to purge GUID '" << msgGUID
                    << "' from virtual storage with appId '" << vs->appId()
                    << "' & appKey '" << vs->appKey() << "' for queue '"
                    << d_storage_p->queueUri() << "' & queueKey '"
                    << d_storage_p->queueKey()
                    << "', but GUID does not exist in the underlying "
                       "storage.";
            }
            else if (result == mqbi::StorageResult::e_NON_ZERO_REFERENCES) {
                // Not logging this case.
            }
            else if (result == mqbi::StorageResult::e_SUCCESS) {
                // Not logging this case.
            }
            else {
                BMQTSK_ALARMLOG_ALARM("STORAGE_PURGE_ERROR")
                    << "Partition [" << d_storage_p->partitionId() << "]"
                    << ": Attempting to purge GUID '" << msgGUID
                    << "' from virtual storage with appId '" << vs->appId()
                    << "' & appKey '" << vs->appKey() << "] for queue '"
                    << d_storage_p->queueUri() << "' & queueKey '"
                    << d_storage_p->queueKey()
                    << "', with invalid context (refCount is already "
                       "zero)."
                    << BMQTSK_ALARMLOG_END;
            }
            ++itData;
        }
    }

    if (queue()) {
        queue()->stats()->onEvent(
            mqbstat::QueueStatsDomain::EventType::e_PURGE,
            0,
            vs->appId());
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

    // Grow ordinals
    appOrdinal = d_ordinals.size();
    d_ordinals.resize(appOrdinal + 1);

    BSLS_ASSERT_SAFE(appOrdinal <= d_virtualStorages.size());

    VirtualStorageSp vsp;
    vsp.createInplace(d_allocator_p,
                      d_storage_p,
                      appId,
                      appKey,
                      appOrdinal,
                      d_numMessages,
                      d_allocator_p);
    d_virtualStorages.insert(appId, appKey, vsp);

    d_ordinals[appOrdinal] = vsp;

    BALL_LOG_INFO << "Adding VirtualStorage for appId '" << vsp->appId()
                  << "' & appKey '" << vsp->appKey() << "' for queue '"
                  << d_storage_p->queueUri() << "' & queueKey '"
                  << d_storage_p->queueKey() << "', with ordinal "
                  << vsp->ordinal();

    if (d_queue_p) {
        BSLS_ASSERT_SAFE(d_queue_p->queueEngine());
        // QueueEngines use the key to look up the id

        d_queue_p->queueEngine()->registerStorage(appId, appKey, appOrdinal);
    }

    return 0;
}

mqbi::StorageResult::Enum
VirtualStorageCatalog::removeVirtualStorage(const mqbu::StorageKey& appKey,
                                            const PurgeCallback&    cb)
{
    BSLS_ASSERT_SAFE(!appKey.isNull());

    VirtualStoragesConstIter it = d_virtualStorages.findByKey2(appKey);
    if (it == d_virtualStorages.end()) {
        return mqbi::StorageResult::e_APPKEY_NOT_FOUND;
    }

    // Make sure there is no AppMessage in the pending states

    BSLS_ASSERT_SAFE(!d_ordinals.empty());

    VirtualStorage*    removing        = it->value().get();
    const unsigned int removingOrdinal = removing->ordinal();

    VirtualStorageSp replacing;
    unsigned int     replacingOrdinal = removingOrdinal;

    replacing        = d_ordinals.back();
    replacingOrdinal = replacing->ordinal();
    BSLS_ASSERT_SAFE(replacingOrdinal + 1 == d_ordinals.size());

    BALL_LOG_INFO << "Removing VirtualStorage with appId '"
                  << removing->appId() << "' & appKey '" << removing->appKey()
                  << "' for queue '" << d_storage_p->queueUri()
                  << "' & queueKey '" << d_storage_p->queueKey()
                  << "'. Replacing " << removingOrdinal << " with "
                  << replacingOrdinal;

    // Replace [vs->ordinal()] with [maxVirtualStorageSpOrdinal]

    DataStreamIterator itData;
    bsls::Types::Int64 total = d_dataStream.size();

    if (seek(&itData, removing) < total) {
        if (cb) {
            mqbi::StorageResult::Enum rc = cb(appKey, itData);
            if (mqbi::StorageResult::e_SUCCESS != rc) {
                return rc;
            }
        }
    }

    purgeImpl(removing, itData, replacingOrdinal);

    if (d_queue_p) {
        BSLS_ASSERT_SAFE(d_queue_p->queueEngine());
        // QueueEngines use the key to look up the id

        d_queue_p->queueEngine()->unregisterStorage(removing->appId(),
                                                    appKey,
                                                    removingOrdinal);
    }

    if (removingOrdinal != replacingOrdinal) {
        // Replacement

        d_ordinals[removingOrdinal] = replacing;
        replacing->replaceOrdinal(removingOrdinal);

        if (d_queue_p) {
            BSLS_ASSERT_SAFE(d_queue_p->queueEngine());
            d_queue_p->queueEngine()->registerStorage(replacing->appId(),
                                                      replacing->appKey(),
                                                      replacing->ordinal());
        }
    }

    d_ordinals.resize(d_ordinals.size() - 1);

    d_virtualStorages.erase(it);

    return mqbi::StorageResult::e_SUCCESS;
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
    mqbi::DataStreamMessage* dataStreamMessage,
    const mqbu::StorageKey&  appKey)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dataStreamMessage);
    BSLS_ASSERT_SAFE(!appKey.isNull());

    VirtualStoragesIter it = d_virtualStorages.findByKey2(appKey);
    BSLS_ASSERT_SAFE(it != d_virtualStorages.end());

    it->value()->confirm(dataStreamMessage);
}

void VirtualStorageCatalog::calibrate()
{
    for (VirtualStoragesIter it = d_virtualStorages.begin();
         it != d_virtualStorages.end();
         ++it) {
        DataStreamIterator itData;
        bsls::Types::Int64 bytes       = 0;
        bsls::Types::Int64 numMessages = seek(&itData,
                                              it->value().get(),
                                              &bytes);
        it->value()->setNumRemoved(numMessages, bytes);
    }
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

void VirtualStorageCatalog::loadVirtualStorageDetails(AppInfos* buffer) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(buffer);

    for (VirtualStoragesConstIter cit = d_virtualStorages.begin();
         cit != d_virtualStorages.end();
         ++cit) {
        BSLS_ASSERT_SAFE(cit->key2() == cit->value()->appKey());
        buffer->insert(bsl::make_pair(cit->key1(), cit->key2()));
    }
}

void VirtualStorageCatalog::setup(mqbi::DataStreamMessage* data) const
{
    // The only case for subsequent resize is proxy receiving subsequent PUSH
    // messages for the same GUID and different apps
    const unsigned int numApps = data->d_numApps;
    if (data->d_apps.size() < numApps) {
        data->d_apps.resize(numApps, defaultAppMessage());
    }
}

const mqbi::AppMessage& VirtualStorageCatalog::appMessageView(
    const mqbi::DataStreamMessage& dataStreamMessage,
    unsigned int                   ordinal) const
{
    const unsigned int numApps = dataStreamMessage.d_numApps;

    if (ordinal < numApps) {
        if (dataStreamMessage.d_apps.size() > ordinal) {
            return dataStreamMessage.app(ordinal);
        }
        return d_defaultAppMessage;
    }
    else {
        // The message is older than the App associated with the 'appOrdinal'
        return d_defaultNonApplicableAppMessage;
    }
}

}  // close package namespace
}  // close enterprise namespace
