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

// mqbs_voidstorageiterator.h                                         -*-C++-*-
#ifndef INCLUDED_MQBS_VOIDSTORAGEITERATOR
#define INCLUDED_MQBS_VOIDSTORAGEITERATOR

//@PURPOSE: Provide an iterator over an empty storage.
//
//@CLASSES:
//  mqbs::VoidStorageIterator
//
//@DESCRIPTION: 'mqbs::VoidStorageIterator' provides a concrete implementation
// of the 'mqbi::Storage' interface that iterates over an empty range of
// messages.  This is used by substreams created with an unauthorized app id,
// until it is dynamically registered.

// MQB
#include <mqbi_storage.h>

// BMQ
#include <bmqt_messageguid.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_memory.h>
#include <bsls_keyword.h>

namespace BloombergLP {

namespace bmqt {
class MessageGUID;
}
namespace mqbi {
class StorageMessageAttributes;
}

namespace mqbs {

// =========================
// class VoidStorageIterator
// =========================

class VoidStorageIterator : public mqbi::StorageIterator {
  private:
    // DATA
    bmqt::MessageGUID d_invalidGuid;
    // Constant representing a null message GUID.

  private:
    // NOT IMPLEMENTED
    VoidStorageIterator(const VoidStorageIterator&) BSLS_KEYWORD_DELETED;
    VoidStorageIterator&
    operator=(const VoidStorageIterator&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `VoidStorageIterator` object.
    VoidStorageIterator();

    /// Destroy this object.
    ~VoidStorageIterator() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual mqbi::StorageIterator)

    /// Return a reference offering non-modifiable access to the guid
    /// associated to the item currently pointed at by this iterator.  The
    /// behavior is undefined unless `atEnd` returns `false`.
    virtual const bmqt::MessageGUID& guid() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering modifiable access to the RdaInfo
    /// associated to the item currently pointed at by this iterator.  The
    /// behavior is undefined unless `atEnd` returns `false`.
    bmqp::RdaInfo& rdaInfo() const BSLS_KEYWORD_OVERRIDE;

    /// Return subscription id associated to the item currently pointed at
    /// by this iterator.
    /// The behavior is undefined unless `atEnd` returns `false`.
    unsigned int subscriptionId() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering non-modifiable access to the application
    /// data associated with the item currently pointed at by this iterator.
    /// The behavior is undefined unless `atEnd` returns `false`.
    virtual const bsl::shared_ptr<bdlbb::Blob>&
    appData() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering non-modifiable access to the options
    /// associated with the item currently pointed at by this iterator.  The
    /// behavior is undefined unless `atEnd` returns `false`.
    virtual const bsl::shared_ptr<bdlbb::Blob>&
    options() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering non-modifiable access to the attributes
    /// associated with the message currently pointed at by this iterator.
    /// The behavior is undefined unless `atEnd` returns `false`.
    virtual const mqbi::StorageMessageAttributes&
    attributes() const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if this iterator is currently at the end of the items'
    /// collection, and hence doesn't reference a valid item.
    virtual bool atEnd() const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if this iterator is currently not at the end of the
    /// `items` collection and the message currently pointed at by this
    /// iterator has received replication factor Receipts.
    virtual bool hasReceipt() const BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Advance the iterator to the next item.  The behavior is undefined
    /// unless `atEnd` returns `false`.  Return `true` if the iterator then
    /// points to a valid item, or `false` if it now is at the end of the
    /// items' collection.
    virtual bool advance() BSLS_KEYWORD_OVERRIDE;

    /// Reset the iterator to point to first item, if any, in the underlying
    /// storage.
    virtual void reset() BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
