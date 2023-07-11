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

#include <mqbscm_version.h>
// BDE
#include <bsl_cstring.h>
#include <bsl_utility.h>
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
                               bslma::Allocator*       allocator)
: d_allocator_p(allocator)
, d_storage_p(storage)
, d_appId(appId, allocator)
, d_appKey(appKey)
, d_guids(allocator)
, d_totalBytes(0)
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
VirtualStorage::get(bsl::shared_ptr<bdlbb::Blob>*   appData,
                    bsl::shared_ptr<bdlbb::Blob>*   options,
                    mqbi::StorageMessageAttributes* attributes,
                    const bmqt::MessageGUID&        msgGUID) const
{
    if (!hasMessage(msgGUID)) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    return d_storage_p->get(appData, options, attributes, msgGUID);
}

mqbi::StorageResult::Enum
VirtualStorage::get(mqbi::StorageMessageAttributes* attributes,
                    const bmqt::MessageGUID&        msgGUID) const
{
    if (!hasMessage(msgGUID)) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    return d_storage_p->get(attributes, msgGUID);
}

int VirtualStorage::configure(
    BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription,
    BSLS_ANNOTATION_UNUSED const mqbconfm::Storage& config,
    BSLS_ANNOTATION_UNUSED const mqbconfm::Limits& limits,
    BSLS_ANNOTATION_UNUSED const bsls::Types::Int64 messageTtl,
    BSLS_ANNOTATION_UNUSED const int                maxDeliveryAttempts)
{
    // NOTHING
    return 0;
}

void VirtualStorage::setQueue(BSLS_ANNOTATION_UNUSED mqbi::Queue* queue)
{
    BSLS_ASSERT_OPT(false && "Should not be invoked.");
}

mqbi::Queue* VirtualStorage::queue()
{
    BSLS_ASSERT_OPT(false && "Should not be invoked.");
    return static_cast<mqbi::Queue*>(0);
}

void VirtualStorage::close()
{
    // NOTHING
}

mqbi::StorageResult::Enum VirtualStorage::put(const bmqt::MessageGUID& msgGUID,
                                              int                      msgSize,
                                              const bmqp::RdaInfo&     rdaInfo,
                                              unsigned int subScriptionId)
{
    if (d_guids
            .insert(bsl::make_pair(
                msgGUID,
                MessageContext(msgSize, rdaInfo, subScriptionId)))
            .second == false) {
        // Duplicate GUID
        return mqbi::StorageResult::e_GUID_NOT_UNIQUE;  // RETURN
    }

    // Success: new GUID
    d_totalBytes += msgSize;
    return mqbi::StorageResult::e_SUCCESS;
}

mqbi::StorageResult::Enum VirtualStorage::put(
    BSLS_ANNOTATION_UNUSED mqbi::StorageMessageAttributes* attributes,
    BSLS_ANNOTATION_UNUSED const bmqt::MessageGUID& msgGUID,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<bdlbb::Blob>& appData,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<bdlbb::Blob>& options,
    BSLS_ANNOTATION_UNUSED const mqbi::Storage::StorageKeys& storageKeys)
{
    BSLS_ASSERT_OPT(false && "Should not be invoked.");
    return mqbi::StorageResult::e_INVALID_OPERATION;
}

bslma::ManagedPtr<mqbi::StorageIterator>
VirtualStorage::getIterator(const mqbu::StorageKey& appKey)
{
    BSLS_ASSERT_SAFE(d_appKey == appKey);
    static_cast<void>(appKey);

    bslma::ManagedPtr<mqbi::StorageIterator> mp(
        new (*d_allocator_p) VirtualStorageIterator(this, d_guids.begin()),
        d_allocator_p);

    return mp;
}

mqbi::StorageResult::Enum
VirtualStorage::getIterator(bslma::ManagedPtr<mqbi::StorageIterator>* out,
                            const mqbu::StorageKey&                   appKey,
                            const bmqt::MessageGUID&                  msgGUID)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_appKey == appKey);
    static_cast<void>(appKey);

    GuidListIter it = d_guids.find(msgGUID);
    if (it == d_guids.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    out->load(new (*d_allocator_p) VirtualStorageIterator(this, it),
              d_allocator_p);

    return mqbi::StorageResult::e_SUCCESS;
}

mqbi::StorageResult::Enum VirtualStorage::releaseRef(
    BSLS_ANNOTATION_UNUSED const bmqt::MessageGUID& msgGUID,
    BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& appKey,
    BSLS_ANNOTATION_UNUSED bsls::Types::Int64 timestamp,
    BSLS_ANNOTATION_UNUSED bool               onReject)
{
    BSLS_ASSERT_OPT(false && "Should not be invoked.");
    return mqbi::StorageResult::e_INVALID_OPERATION;
}

mqbi::StorageResult::Enum
VirtualStorage::remove(const bmqt::MessageGUID&    msgGUID,
                       int*                        msgSize,
                       BSLS_ANNOTATION_UNUSED bool clearAll)

{
    GuidList::const_iterator it = d_guids.find(msgGUID);
    if (it == d_guids.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    if (msgSize) {
        *msgSize = it->second.d_size;
    }
    d_totalBytes -= it->second.d_size;
    d_guids.erase(it);
    return mqbi::StorageResult::e_SUCCESS;
}

mqbi::StorageResult::Enum VirtualStorage::removeAll(
    BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& appKey)
{
    d_guids.clear();
    d_totalBytes = 0;
    return mqbi::StorageResult::e_SUCCESS;
}

void VirtualStorage::dispatcherFlush(bool, bool)
{
    BSLS_ASSERT_OPT(false && "Should not be invoked.");
}

// ACCESSORS
mqbi::StorageResult::Enum
VirtualStorage::getMessageSize(int*                     msgSize,
                               const bmqt::MessageGUID& msgGUID) const
{
    GuidList::const_iterator cit = d_guids.find(msgGUID);
    if (cit == d_guids.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    *msgSize = cit->second.d_size;
    return mqbi::StorageResult::e_SUCCESS;
}

int VirtualStorage::numVirtualStorages() const
{
    BSLS_ASSERT_OPT(false && "Should not be invoked.");
    return 0;
}

bool VirtualStorage::hasVirtualStorage(
    BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& appKey,
    BSLS_ANNOTATION_UNUSED bsl::string* appId) const
{
    BSLS_ASSERT_OPT(false && "Should not be invoked.");
    return false;
}

bool VirtualStorage::hasVirtualStorage(
    BSLS_ANNOTATION_UNUSED const bsl::string& appId,
    BSLS_ANNOTATION_UNUSED mqbu::StorageKey* appKey) const
{
    BSLS_ASSERT_OPT(false && "Should not be invoked.");
    return false;
}

bool VirtualStorage::hasReceipt(const bmqt::MessageGUID& msgGUID) const
{
    return d_storage_p->hasReceipt(msgGUID);
}

void VirtualStorage::loadVirtualStorageDetails(
    BSLS_ANNOTATION_UNUSED AppIdKeyPairs* buffer) const
{
    BSLS_ASSERT_OPT(false && "Should not be invoked.");
}

int VirtualStorage::gcExpiredMessages(
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64* latestGcMsgTimestampEpoch,
    BSLS_ANNOTATION_UNUSED bsls::Types::Int64* configuredTtlValue,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 secondsFromEpoch)
{
    BSLS_ASSERT_OPT(false && "Should not be invoked.");
    return 0;
}

bool VirtualStorage::gcHistory()
{
    BSLS_ASSERT_OPT(false && "Should not be invoked.");
    return false;
}

int VirtualStorage::addVirtualStorage(
    BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription,
    BSLS_ANNOTATION_UNUSED const bsl::string& appId,
    BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& appKey)
{
    BSLS_ASSERT_OPT(false && "Should not be invoked.");
    return -1;
}

bool VirtualStorage::removeVirtualStorage(
    BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& appKey)
{
    BSLS_ASSERT_OPT(false && "Should not be invoked.");
    return false;
}

// ----------------------------
// class VirtualStorageIterator
// ----------------------------

// PRIVATE MANIPULATORS
void VirtualStorageIterator::clear()
{
    // Clear previous state, if any.  This is required so that new state can be
    // loaded in 'appData', 'options' or 'attributes' routines.
    d_appData_sp.reset();
    d_options_sp.reset();
    d_attributes.reset();
    d_haveReceipt = false;
}

// PRIVATE ACCESSORS
bool VirtualStorageIterator::loadMessageAndAttributes() const
{
    BSLS_ASSERT_SAFE(!atEnd());

    if (!d_appData_sp) {
        mqbi::StorageResult::Enum rc = d_virtualStorage_p->d_storage_p->get(
            &d_appData_sp,
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
VirtualStorageIterator::VirtualStorageIterator(
    VirtualStorage*                                 storage,
    const VirtualStorage::GuidList::const_iterator& initialPosition)
: d_virtualStorage_p(storage)
, d_iterator(initialPosition)
, d_attributes()
, d_appData_sp()
, d_options_sp()
, d_haveReceipt(false)
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
    BSLS_ASSERT_SAFE(!atEnd());

    clear();
    ++d_iterator;
    return !atEnd();
}

void VirtualStorageIterator::reset()
{
    clear();

    // Reset iterator to beginning
    d_iterator = d_virtualStorage_p->d_guids.begin();
}

// ACCESSORS
const bmqt::MessageGUID& VirtualStorageIterator::guid() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());

    return d_iterator->first;
}

bmqp::RdaInfo& VirtualStorageIterator::rdaInfo() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());

    return d_iterator->second.d_rdaInfo;
}

unsigned int VirtualStorageIterator::subscriptionId() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());

    return d_iterator->second.d_subscriptionId;
}

const bsl::shared_ptr<bdlbb::Blob>& VirtualStorageIterator::appData() const
{
    loadMessageAndAttributes();
    return d_appData_sp;
}

const bsl::shared_ptr<bdlbb::Blob>& VirtualStorageIterator::options() const
{
    loadMessageAndAttributes();
    return d_options_sp;
}

const mqbi::StorageMessageAttributes&
VirtualStorageIterator::attributes() const
{
    loadMessageAndAttributes();
    return d_attributes;
}

bool VirtualStorageIterator::atEnd() const
{
    return (d_iterator == d_virtualStorage_p->d_guids.end());
}

bool VirtualStorageIterator::hasReceipt() const
{
    if (atEnd()) {
        return false;  // RETURN
    }
    if (!d_haveReceipt) {
        // 'd_attributes.hasReceipt' can be stale.  Double check by reloading
        if (d_virtualStorage_p->d_storage_p->hasReceipt(d_iterator->first)) {
            d_haveReceipt = true;
        }
    }

    return d_haveReceipt;
}

}  // close package namespace
}  // close enterprise namespace
