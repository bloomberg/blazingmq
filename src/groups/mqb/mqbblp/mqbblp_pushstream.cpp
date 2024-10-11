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

#include <mqbscm_version.h>

namespace BloombergLP {
namespace mqbblp {

namespace {

void noOpDeleter(bdlma::ConcurrentPool*)
{
    // NOTHING
}

}  // close unnamed namespace

// ----------------
// class PushStream
// ----------------

PushStream::PushStream(
    const bsl::optional<bdlma::ConcurrentPool*>& pushElementsPool,
    bslma::Allocator*                            allocator)
: d_stream(allocator)
, d_apps(allocator)
, d_pushElementsPool_sp()
{
    allocator = bslma::Default::allocator(allocator);

    if (pushElementsPool.has_value()) {
        d_pushElementsPool_sp.reset(pushElementsPool.value(),
                                    noOpDeleter,
                                    allocator);
    }

    if (!d_pushElementsPool_sp) {
        d_pushElementsPool_sp.load(
            new (*allocator) bdlma::ConcurrentPool(sizeof(Element), allocator),
            allocator);
    }
    BSLS_ASSERT_SAFE(d_pushElementsPool_sp);
    BSLS_ASSERT_SAFE(d_pushElementsPool_sp->blockSize() == sizeof(Element));
}

// ----------------------------
// class VirtualStorageIterator
// ----------------------------

// PRIVATE MANIPULATORS
void PushStreamIterator::clear()
{
    // Clear previous state, if any.  This is required so that new state can be
    // loaded in 'appData', 'options' or 'attributes' routines.
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

// CREATORS
PushStreamIterator::PushStreamIterator(
    mqbi::Storage*              storage,
    PushStream*                 owner,
    const PushStream::iterator& initialPosition)
: d_storage_p(storage)
, d_iterator(initialPosition)
, d_attributes()
, d_appData_sp()
, d_options_sp()
, d_owner_p(owner)
, d_currentElement(0)
, d_currentOrdinal(mqbi::Storage::k_INVALID_ORDINAL)
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
    BSLS_ASSERT_SAFE(!atEnd());
    return d_iterator->second.numElements();
}

void PushStreamIterator::removeCurrentElement()
{
    BSLS_ASSERT_SAFE(!atEnd());
    BSLS_ASSERT_SAFE(d_currentElement);

    PushStream::Element* del = d_currentElement;

    // still keep the same ordinal numbering
    d_currentElement = d_currentElement->next();
    ++d_currentOrdinal;

    d_owner_p->remove(del);
    d_owner_p->destroy(del, true);
    // doKeepGuid because of the d_iterator

    if (d_iterator->second.numElements() == 0) {
        BSLS_ASSERT_SAFE(d_currentElement == 0);
    }
}

// MANIPULATORS
bool PushStreamIterator::advance()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());

    clear();

    if (d_iterator->second.numElements() == 0) {
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
    clear();

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
        d_currentElement = d_iterator->second.front();
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
    unsigned int                upstreamSubQueueId,
    mqbi::Storage*              storage,
    PushStream*                 owner,
    const PushStream::iterator& initialPosition)
: PushStreamIterator(storage, owner, initialPosition)
{
    d_itApp = owner->d_apps.find(upstreamSubQueueId);

    BSLS_ASSERT_SAFE(d_itApp != owner->d_apps.end());

    d_currentElement = d_itApp->second.d_elements.front();

    BSLS_ASSERT_SAFE(d_currentElement->app().d_app == d_itApp->second.d_app);
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

    clear();

    PushStream::Element* del = d_currentElement;

    d_currentElement = d_currentElement->nextInApp();

    d_owner_p->remove(del);
    d_owner_p->destroy(del, false);
    // do not keep Guid

    if (atEnd()) {
        return false;
    }

    BSLS_ASSERT_SAFE(d_itApp->second.d_elements.numElements());
    BSLS_ASSERT_SAFE(d_currentElement);

    return true;
}

bool VirtualPushStreamIterator::atEnd() const
{
    return (d_currentElement == 0);
}

PushStream::Element*
VirtualPushStreamIterator::element(unsigned int appOrdinal) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());

    // Ignore ordinal  when the app is fixed;
    // 'd_currentElement' does not depend on 'appOrdinal'
    (void)appOrdinal;

    return d_currentElement;
}

}  // close package namespace
}  // close enterprise namespace
