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

// mqbblp_pushstream.cpp                                              -*-C++-*-

#include <mqbblp_pushstream.h>

#include <bsl_memory.h>
#include <mqbscm_version.h>

// BDE
#include <bsla_annotations.h>

namespace BloombergLP {
namespace mqbblp {

// ----------------
// class PushStream
// ----------------

PushStream::PushStream(
    const bsl::shared_ptr<bdlma::ConcurrentPool>& pushElementsPool_sp,
    bslma::Allocator*                             allocator)
: d_stream(allocator)
, d_apps(allocator)
, d_pushElementsPool_sp(
      pushElementsPool_sp
          ? pushElementsPool_sp
          : bsl::allocate_shared<bdlma::ConcurrentPool>(allocator,
                                                        sizeof(Element),
                                                        allocator))
, d_nextSequenceNumber(0)
// ConcurrentPool doesn't have allocator traits, have to pass allocator
// twice
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_pushElementsPool_sp);
    BSLS_ASSERT_SAFE(d_pushElementsPool_sp->blockSize() == sizeof(Element));
}

// ----------------------------
// class VirtualStorageIterator
// ----------------------------

// PRIVATE MANIPULATORS
void PushStreamIterator::clearCache()
{
    d_appData_sp.reset();
    d_options_sp.reset();
    d_attributes.reset();
}

// PRIVATE ACCESSORS
bool PushStreamIterator::loadMessageAndAttributes() const
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

const PushStream::Message& PushStreamIterator::message() const
{
    BSLS_ASSERT_SAFE(d_iterator != d_owner_p->d_stream.end());

    return d_iterator->second;
}

// CREATORS
PushStreamIterator::PushStreamIterator(
    mqbi::Storage*              storage,
    PushStream*                 owner,
    const PushStream::iterator& initialPosition)
: d_storage_p(storage)
, d_attributes()
, d_appData_sp()
, d_options_sp()
, d_owner_p(owner)
, d_currentElement(0)
, d_currentOrdinal(mqbi::Storage::k_INVALID_ORDINAL)
, d_iterator(initialPosition)
{
    BSLS_ASSERT_SAFE(d_storage_p);
    BSLS_ASSERT_SAFE(d_owner_p);
}

PushStreamIterator::~PushStreamIterator()
{
    // NOTHING
}

unsigned int PushStreamIterator::numApps() const
{
    return message().numElements();
}

bsls::Types::Uint64 PushStreamIterator::sequenceNumber() const
{
    return message().sequenceNumber();
}

void PushStreamIterator::removeCurrentElement()
{
    BSLS_ASSERT_SAFE(!atEnd());
    BSLS_ASSERT_SAFE(d_currentElement);

    PushStream::Element* del = d_currentElement;

    // still keep the same ordinal numbering
    d_currentElement = d_currentElement->next();
    ++d_currentOrdinal;

    d_owner_p->remove(del, false);
    // cannot erase the GUID because of the d_iterator

    if (message().numElements() == 0) {
        BSLS_ASSERT_SAFE(d_currentElement == 0);
    }
}

void PushStreamIterator::removeAllElements()
{
    d_currentElement = message().d_appMessages.front();
    d_currentOrdinal = 0;

    while (d_currentElement) {
        removeCurrentElement();
    }
}

// MANIPULATORS
bool PushStreamIterator::advance()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());

    clearCache();

    if (message().numElements() == 0) {
        d_iterator = d_owner_p->d_stream.erase(d_iterator);
    }
    else {
        ++d_iterator;
    }

    d_currentOrdinal = mqbi::Storage::k_INVALID_ORDINAL;

    return !atEnd();
}

void PushStreamIterator::reset(const bmqt::MessageGUID& where)
{
    clearCache();

    if (where.isUnset()) {
        // Reset iterator to beginning
        d_iterator = d_owner_p->d_stream.begin();
    }
    else {
        d_iterator = d_owner_p->d_stream.find(where);
    }

    d_currentOrdinal = mqbi::Storage::k_INVALID_ORDINAL;
}

// ACCESSORS
const bmqt::MessageGUID& PushStreamIterator::guid() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());

    return d_iterator->first;
}

PushStream::Element* PushStreamIterator::element(unsigned int appOrdinal) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());
    BSLS_ASSERT_SAFE(appOrdinal < mqbi::Storage::k_INVALID_ORDINAL);

    if (d_currentOrdinal > appOrdinal) {
        d_currentOrdinal = 0;
        d_currentElement = message().d_appMessages.front();
    }

    BSLS_ASSERT_SAFE(d_currentElement);

    while (appOrdinal > d_currentOrdinal) {
        ++d_currentOrdinal;
        d_currentElement = d_currentElement->next();

        BSLS_ASSERT_SAFE(d_currentElement);
    }

    return d_currentElement;
}

const mqbi::AppMessage&
PushStreamIterator::appMessageView(unsigned int appOrdinal) const
{
    return *element(appOrdinal)->appView();
}

mqbi::AppMessage& PushStreamIterator::appMessageState(unsigned int appOrdinal)
{
    return *element(appOrdinal)->appState();
}

const bsl::shared_ptr<bdlbb::Blob>& PushStreamIterator::appData() const
{
    loadMessageAndAttributes();
    return d_appData_sp;
}

const bsl::shared_ptr<bdlbb::Blob>& PushStreamIterator::options() const
{
    loadMessageAndAttributes();
    return d_options_sp;
}

const mqbi::StorageMessageAttributes& PushStreamIterator::attributes() const
{
    loadMessageAndAttributes();
    return d_attributes;
}

bool PushStreamIterator::atEnd() const
{
    return (d_iterator == d_owner_p->d_stream.end());
}

bool PushStreamIterator::hasReceipt() const
{
    return !atEnd();
}

// CREATORS
VirtualPushStreamIterator::VirtualPushStreamIterator(
    unsigned int   upstreamSubQueueId,
    mqbi::Storage* storage,
    PushStream*    owner)
: PushStreamIterator(storage, owner, owner->d_stream.end())
{
    /// An iterator to the App being iterated
    PushStream::Apps::iterator itApp = owner->d_apps.find(upstreamSubQueueId);

    if (itApp != owner->d_apps.end()) {
        const PushStream::App& app = itApp->second;

        BSLS_ASSERT_SAFE(app.d_elements.numElements());

        // Always start with the first element in the App

        d_currentElement = app.d_elements.front();

        BSLS_ASSERT_SAFE(d_currentElement);

        BSLS_ASSERT_SAFE(d_currentElement->app().d_app == app.d_app);

        d_iterator = d_currentElement->iteratorGuid();
    }
}

VirtualPushStreamIterator::~VirtualPushStreamIterator()
{
    // NOTHING
}

unsigned int VirtualPushStreamIterator::numApps() const
{
    BSLS_ASSERT_SAFE(!atEnd());

    return 1;
}

void VirtualPushStreamIterator::removeCurrentElement()
{
    advance();
}

// MANIPULATORS
bool VirtualPushStreamIterator::advance()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());

    clearCache();

    PushStream::Element* del = d_currentElement;

    d_currentElement = d_currentElement->nextInApp();

    d_owner_p->remove(del, true);
    // can erase GUID

    if (atEnd()) {
        return false;
    }

    BSLS_ASSERT_SAFE(d_currentElement);

    d_iterator = d_currentElement->iteratorGuid();

    return true;
}

bool VirtualPushStreamIterator::atEnd() const
{
    return (d_currentElement == 0);
}

PushStream::Element*
VirtualPushStreamIterator::element(BSLA_UNUSED unsigned int appOrdinal) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());

    // Ignore ordinal when the app is fixed;
    // 'd_currentElement' does not depend on 'appOrdinal'

    return d_currentElement;
}

}  // close package namespace
}  // close enterprise namespace
