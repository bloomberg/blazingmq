// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbs_virtualstorage.cpp                                            -*-C++-*-
#include <mqbs_virtualstorage.h>
#include <mqbs_virtualstoragecatalog.h>

#include <mqbscm_version.h>
// BDE
#include <bsl_cstring.h>
#include <bsl_utility.h>
#include <bsla_annotations.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbs {

// --------------------
// class VirtualStorage
// --------------------

// CREATORS
VirtualStorage::VirtualStorage(mqbi::Storage*          storage,
                               const bsl::string&      appId,
                               const mqbu::StorageKey& appKey,
                               unsigned int            ordinal,
                               bsls::Types::Int64      numMessagesSoFar,
                               bslma::Allocator*       allocator)
: d_allocator_p(allocator)
, d_storage_p(storage)
, d_appId(appId, allocator)
, d_appKey(appKey)
, d_removedBytes(0)
, d_numRemoved(numMessagesSoFar)
, d_ordinal(ordinal)
{
    BSLS_ASSERT_SAFE(d_storage_p);
    BSLS_ASSERT_SAFE(allocator);
    BSLS_ASSERT_SAFE(!appId.empty());
    BSLS_ASSERT_SAFE(!appKey.isNull());
}

VirtualStorage::~VirtualStorage()
{
    // NOTHING
}

// MANIPULATORS
mqbi::StorageResult::Enum
VirtualStorage::confirm(mqbi::DataStreamMessage* dataStreamMessage)
{
    mqbi::AppMessage& appMessage = dataStreamMessage->app(ordinal());

    if (appMessage.isPending()) {
        appMessage.setConfirmState();

        d_removedBytes += dataStreamMessage->d_size;
        ++d_numRemoved;

        return mqbi::StorageResult::e_SUCCESS;
    }
    else {
        // already deleted
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }
}

mqbi::StorageResult::Enum
VirtualStorage::remove(mqbi::DataStreamMessage* dataStreamMessage)
{
    if (ordinal() < dataStreamMessage->d_numApps) {
        mqbi::AppMessage& appMessage = dataStreamMessage->app(ordinal());

        if (appMessage.isPending()) {
            appMessage.setRemovedState();

            d_removedBytes += dataStreamMessage->d_size;
            ++d_numRemoved;

            return mqbi::StorageResult::e_SUCCESS;
        }
        else {
            // already deleted
            return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
        }
    }
    else {
        // 'dataStreamMessage' is older than this VirtualStorage
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }
}

bool VirtualStorage::remove(mqbi::DataStreamMessage* dataStreamMessage,
                            unsigned int             replacingOrdinal)
{
    const unsigned int thisOrdinal = ordinal();

    if (thisOrdinal >= dataStreamMessage->d_numApps) {
        // This App is newer than the message

        return false;
    }

    // This App is either older than the message, or the result of
    // previous replacement.  In either case, it's state is accurate.

    mqbi::AppMessage& appMessage = dataStreamMessage->app(thisOrdinal);

    const bool wasPending = appMessage.isPending();

    if (wasPending) {
        appMessage.setRemovedState();

        d_removedBytes += dataStreamMessage->d_size;
        ++d_numRemoved;
    }

    if (replacingOrdinal < dataStreamMessage->d_numApps) {
        if (thisOrdinal < replacingOrdinal) {
            BSLS_ASSERT_SAFE(replacingOrdinal + 1 ==
                             dataStreamMessage->d_numApps);

            // replace 'thisOrdinal' with 'maxOrdinal'
            appMessage = dataStreamMessage->d_apps[replacingOrdinal];
        }
        // shrink the set of ordinals
        --dataStreamMessage->d_numApps;
    }

    return wasPending;
}

void VirtualStorage::onGC(int size)
{
    d_removedBytes -= size;
    --d_numRemoved;
}

void VirtualStorage::resetStats()
{
    d_removedBytes = 0;
    d_numRemoved   = 0;
}

void VirtualStorage::replaceOrdinal(unsigned int replacingOrdinal)
{
    d_ordinal = replacingOrdinal;
}

void VirtualStorage::setNumRemoved(bsls::Types::Int64 numMessages,
                                   bsls::Types::Int64 bytes)
{
    d_numRemoved   = numMessages;
    d_removedBytes = bytes;
}

bool VirtualStorage::hasReceipt(const bmqt::MessageGUID& msgGUID) const
{
    return d_storage_p->hasReceipt(msgGUID);
}

unsigned int VirtualStorage::ordinal() const
{
    return d_ordinal;
}

// ----------------------------
// class VirtualStorageIterator
// ----------------------------

// PRIVATE MANIPULATORS
void StorageIterator::clearCache()
{
    d_appData_sp.reset();
    d_options_sp.reset();
    d_attributes.reset();
    d_haveReceipt = false;
}

// PRIVATE ACCESSORS
bool StorageIterator::loadMessageAndAttributes() const
{
    BSLS_ASSERT_SAFE(!atEnd());

    if (!d_appData_sp) {
        mqbi::StorageResult::Enum rc = d_storage_p->get(&d_appData_sp,
                                                        &d_options_sp,
                                                        &d_attributes,
                                                        d_iterator->first);
        BSLS_ASSERT_SAFE(mqbi::StorageResult::e_SUCCESS == rc);
        static_cast<void>(rc);  // suppress compiler warning
        return true;            // RETURN
    }
    return false;
}

// CREATORS
StorageIterator::StorageIterator(
    mqbi::Storage*                              storage,
    VirtualStorageCatalog*                      owner,
    const VirtualStorage::DataStream::iterator& initialPosition)
: d_storage_p(storage)
, d_owner_p(owner)
, d_iterator(initialPosition)
, d_attributes()
, d_appData_sp()
, d_options_sp()
, d_haveReceipt(false)
{
    BSLS_ASSERT_SAFE(d_owner_p);
}

StorageIterator::~StorageIterator()
{
    // NOTHING
}

// MANIPULATORS
bool StorageIterator::advance()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());

    clearCache();
    ++d_iterator;
    return !atEnd();
}

void StorageIterator::reset(const bmqt::MessageGUID& where)
{
    clearCache();

    d_iterator = d_owner_p->begin(where);
}

// ACCESSORS
const bmqt::MessageGUID& StorageIterator::guid() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());

    return d_iterator->first;
}

const mqbi::AppMessage&
StorageIterator::appMessageView(unsigned int appOrdinal) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());

    const mqbi::DataStreamMessage& dataStreamMessage = d_iterator->second;

    return d_owner_p->appMessageView(dataStreamMessage, appOrdinal);
}

mqbi::AppMessage& StorageIterator::appMessageState(unsigned int appOrdinal)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());

    mqbi::DataStreamMessage* dataStreamMessage = &d_iterator->second;

    d_owner_p->setup(dataStreamMessage);

    return dataStreamMessage->app(appOrdinal);
}

const bsl::shared_ptr<bdlbb::Blob>& StorageIterator::appData() const
{
    loadMessageAndAttributes();
    return d_appData_sp;
}

const bsl::shared_ptr<bdlbb::Blob>& StorageIterator::options() const
{
    loadMessageAndAttributes();
    return d_options_sp;
}

const mqbi::StorageMessageAttributes& StorageIterator::attributes() const
{
    // Do not load memory-mapped file message (expensive).

    if (d_attributes.refCount() == 0) {
        // No loaded Attributes for the current message yet.

        BSLA_MAYBE_UNUSED mqbi::StorageResult::Enum rc =
            d_storage_p->get(&d_attributes, d_iterator->first);
        BSLS_ASSERT_SAFE(mqbi::StorageResult::e_SUCCESS == rc);
    }
    // else return reference to the previously loaded attributes.

    return d_attributes;
}

bool StorageIterator::atEnd() const
{
    return (d_iterator == d_owner_p->end());
}

bool StorageIterator::hasReceipt() const
{
    if (atEnd()) {
        return false;  // RETURN
    }
    if (!d_haveReceipt) {
        // 'd_attributes.hasReceipt' can be stale.  Double check by reloading
        if (d_storage_p->hasReceipt(d_iterator->first)) {
            d_haveReceipt = true;
        }
    }

    return d_haveReceipt;
}

bool StorageIterator::asFarAs(
    const bdlb::Variant<bsls::Types::Uint64, bmqt::MessageGUID>& stop) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(stop.is<bmqt::MessageGUID>());

    return guid() == stop.the<bmqt::MessageGUID>();
}

// CREATORS
VirtualStorageIterator::VirtualStorageIterator(
    VirtualStorage*                             virtualStorage,
    mqbi::Storage*                              storage,
    VirtualStorageCatalog*                      owner,
    const VirtualStorage::DataStream::iterator& initialPosition)
: StorageIterator(storage, owner, initialPosition)
, d_virtualStorage_p(virtualStorage)
{
    BSLS_ASSERT_SAFE(d_virtualStorage_p);
}

VirtualStorageIterator::~VirtualStorageIterator()
{
    // NOTHING
}

// MANIPULATORS
bool VirtualStorageIterator::advance()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_virtualStorage_p);

    while (StorageIterator::advance()) {
        if (StorageIterator::appMessageView(d_virtualStorage_p->ordinal())
                .isPending()) {
            return true;  // RETURN
        }
    }

    return false;
}

}  // close package namespace
}  // close enterprise namespace
