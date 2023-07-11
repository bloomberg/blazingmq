// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mqbs_voidstorageiterator.cpp                                       -*-C++-*-
#include <mqbs_voidstorageiterator.h>

// BDE
#include <bslmt_once.h>

namespace BloombergLP {
namespace mqbs {

namespace {
bsls::ObjectBuffer<bsl::shared_ptr<bdlbb::Blob> > s_blob_sp;
mqbi::StorageMessageAttributes                    s_attr;

const bsl::shared_ptr<bdlbb::Blob>& blob()
{
    BSLMT_ONCE_DO
    {
        new (s_blob_sp.buffer()) bsl::shared_ptr<bdlbb::Blob>(new bdlbb::Blob);
    }

    return s_blob_sp.object();
}
}
// -------------------------
// class VoidStorageIterator
// -------------------------

// CREATORS
VoidStorageIterator::VoidStorageIterator()
{
    // NOTHING
}

VoidStorageIterator::~VoidStorageIterator()
{
    // NOTHING
}

// ACCESSORS
const bmqt::MessageGUID& VoidStorageIterator::guid() const
{
    return d_invalidGuid;
}

bmqp::RdaInfo& VoidStorageIterator::rdaInfo() const
{
    static bmqp::RdaInfo dummy;
    return dummy;
}

unsigned int VoidStorageIterator::subscriptionId() const
{
    return bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID;
}

const bsl::shared_ptr<bdlbb::Blob>& VoidStorageIterator::appData() const
{
    return blob();
}

const bsl::shared_ptr<bdlbb::Blob>& VoidStorageIterator::options() const
{
    return blob();
}

const mqbi::StorageMessageAttributes& VoidStorageIterator::attributes() const
{
    return s_attr;
}

bool VoidStorageIterator::atEnd() const
{
    return true;
}

bool VoidStorageIterator::hasReceipt() const
{
    return false;
}

// MANIPULATORS
bool VoidStorageIterator::advance()
{
    return false;
}

void VoidStorageIterator::reset()
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace
