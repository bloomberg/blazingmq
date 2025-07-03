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

// bmqimp_messagecorrelationidcontainer.h                             -*-C++-*-
#ifndef INCLUDED_BMQIMP_MESSAGECORRELATIONIDCONTAINER
#define INCLUDED_BMQIMP_MESSAGECORRELATIONIDCONTAINER

//@PURPOSE: Provide a mechanism to manage message correlationIds.
//
//@CLASSES:
//  bmqimp::MessageCorrelationIdContainer: mechanism to manage message
//          correlationIds
//
//@DESCRIPTION: 'bmqimp::MessageCorrelationIdContainer' is a container-like
// mechanism that allows storing of 'correlationIds', with an assigned
// 'queueId'. Upon addition of a 'correlationId', a unique 'key' (integer type)
// is returned which can be used later on to assign the 'queueId' as well as
// retrieve and remove the 'correlationId'.
//
/// Thread Safety
///-------------
// Thread safe.

// BMQ

#include <bmqp_protocol.h>
#include <bmqp_requestmanager.h>
#include <bmqt_correlationid.h>
#include <bmqt_messageguid.h>

#include <bmqc_orderedhashmap.h>

// BDE
#include <bsl_functional.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_cpp11.h>
#include <bsls_spinlock.h>

namespace BloombergLP {
namespace bmqimp {

// ===================================
// class MessageCorrelationIdContainer
// ===================================

/// Implementation of a container for message correlationIds.
class MessageCorrelationIdContainer {
  public:
    // PUBLIC TYPES

    typedef bmqp::RequestManager<bmqp_ctrlmsg::ControlMessage,
                                 bmqp_ctrlmsg::ControlMessage>
        RequestManagerType;

    /// Struct representing the correlationId and queueId for a given
    /// message.
    struct QueueAndCorrelationId {
        bmqt::CorrelationId d_correlationId;
        // CorrelationId for message.

        bmqp::QueueId d_queueId;
        // QueueId for message.

        bmqp::EventType::Enum d_messageType;
        // Type of the stored message.

        bmqp::PutHeader d_header;
        // PUT message header.

        bdlbb::Blob d_messageData;
        // Message data.

        RequestManagerType::RequestSp d_requestContext;
        // Control request context.

        /// Create a `QueueAndCorrelationId` having an invalid queueId and
        /// empty correlationId using the specified `allocator`.
        QueueAndCorrelationId(bslma::Allocator* allocator);

        /// Create a `QueueAndCorrelationId` having the specified `queueId`
        /// and the specified `correlationId` using the specified
        /// `allocator`.
        QueueAndCorrelationId(const bmqt::CorrelationId& correlationId,
                              const bmqp::QueueId&       queueId,
                              bslma::Allocator*          allocator);

        /// Copy constructor from the specified `other` using the specified
        /// `allocator`.
        QueueAndCorrelationId(const QueueAndCorrelationId& other,
                              bslma::Allocator*            allocator);
    };

    /// Callback signature for iterating over all elements in the container.
    typedef bsl::function<bool(bool*                        removeItem,
                               const bmqt::MessageGUID&     key,
                               const QueueAndCorrelationId& qac)>
        KeyIdsCb;

  private:
    /// Map of key to an object containing correlationId and queueId of a
    /// message.  This should be an ordered container so that any local
    /// NAKs are generated in the order in which PUTs were posted.
    typedef bmqc::OrderedHashMap<bmqt::MessageGUID, QueueAndCorrelationId>
        CorrelationIdsMap;

    typedef bmqc::OrderedHashMap<bmqt::MessageGUID, bsls::TimeInterval>
        HandleAndExpirationTimeMap;

    /// Map of key (queueId) to an ordered map of handle (message GUID and
    /// and the message expiration time
    typedef bsl::unordered_map<bmqp::QueueId, HandleAndExpirationTimeMap>
        QueueItemsMap;

    mutable bsls::SpinLock d_lock;  // Spin lock for manipulating data
                                    // members

    CorrelationIdsMap d_correlationIds;
    // Map of all registered items.

    QueueItemsMap d_queueItems;
    // Per queue message Ids with
    // timestamps.

    size_t d_numPuts;  // Number of pending PUT messages.

    size_t d_numControls;
    // Number of pending control requests.

    bslma::Allocator* d_allocator_p;
    // Allocator to use.

  private:
    /// Remove the item pointed by the specifed `cit` from the
    /// `d_correlationIds`.  If the item is PUT message with `ACK_REQUESTED`
    /// flag also remove related item from the `d_queueItems` container.
    /// Decrement `d_numPuts` or `d_numControls` counter depending on the
    /// item's type.  Return a constant iterator pointing to the next valid
    /// item from the `d_correlationIds` container or its `end` iterator.
    /// The caller must acquire the `d_lock` before calling this method.
    /// The behavior is underfined if the `cit` doesn't point to a valid
    /// item.
    CorrelationIdsMap::const_iterator
    removeLocked(const CorrelationIdsMap::const_iterator& cit);

    /// Add an item into `d_queueItems` map using the specified `queueId` as
    /// a key and the specified `itemGUID` and `expirationTime` as a value
    /// pair.
    void addQueueItem(const bmqp::QueueId&      queueId,
                      const bmqt::MessageGUID&  itemGUID,
                      const bsls::TimeInterval& expirationTime);

    /// Remove an item from the `d_queueItems` container using the specified
    /// `queueId` as a key to find the per queue items map and then remove
    /// an item using the specified `itemGUID` as a key of that second map.
    /// If the removed item is the last one in the items map then also
    /// remove the entry from the first map that has the `queueId` as a key.
    /// The behavior is underfined if there is no item with `queueId` key in
    /// the first map or with `itemGUID` in the second map.
    void removeQueueItem(const bmqp::QueueId&     queueId,
                         const bmqt::MessageGUID& itemGUID);

  private:
    // NOT IMPLEMENTED
    MessageCorrelationIdContainer(const MessageCorrelationIdContainer&)
        BSLS_CPP11_DELETED;
    MessageCorrelationIdContainer&
    operator=(const MessageCorrelationIdContainer&) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MessageCorrelationIdContainer,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object using the specified `allocator`.
    MessageCorrelationIdContainer(bslma::Allocator* allocator);

    // MANIPULATORS

    /// Remove all inserted items and reset the state of this object to a
    /// default constructed one.
    void reset();

    /// Add the specified `correlationId` and `queueId` mapping them to the
    /// specified `key` that can be used to retrieve them later.
    void add(const bmqt::MessageGUID&   key,
             const bmqt::CorrelationId& correlationId,
             const bmqp::QueueId&       queueId);

    /// Add the specified `context` and the `blob` and return a GUID key
    /// that can be used to retrieve it later.
    bmqt::MessageGUID add(const RequestManagerType::RequestSp& context,
                          const bmqp::QueueId&                 queueId,
                          const bdlbb::Blob&                   blob);

    /// Remove the item uniquely identified by the specified `key`,
    /// and populate the optionally specified `correlationId` with the
    /// removed correlationId.  Return zero on success, non-zero value
    /// otherwise.
    int remove(const bmqt::MessageGUID& key,
               bmqt::CorrelationId*     correlationId = 0);

    /// Associate the specified message data to the item having the key
    /// equals to the GUID from the specified PUT `header`.  The behavior is
    /// undefined if the GUID does not correspond to a previously registered
    /// item.
    void associateMessageData(const bmqp::PutHeader&    header,
                              const bdlbb::Blob&        appData,
                              const bsls::TimeInterval& sentTime);

    // ACCESSORS

    /// Iterate and invoke the specified `callback` on every inserted item.
    /// Return true if the iteration was not interrupted, false otherwise.
    bool iterateAndInvoke(const KeyIdsCb& callback);

    /// Iterate and invoke the specified `callback` on every item that has
    /// a key listed in the specified `keys`.  Return true if the iteration
    /// was not interrupted, false otherwise.
    bool iterateAndInvoke(const bsl::vector<bmqt::MessageGUID>& keys,
                          const KeyIdsCb&                       callback);

    /// Fill the specified `keys` with keys of the items with the expiration
    /// time less or equal to the specified `expirationTime`.  The
    /// expiration time is calculated by adding the queue expiration timeout
    /// from the specified `queueExpirationTimeoutMap` and the item's sent
    /// time.  Return a timestamp of the next nearest expired item.
    bsls::TimeInterval getExpiredIds(
        bsl::vector<bmqt::MessageGUID>*     keys,
        const bsl::unordered_map<int, int>& queueExpirationTimeoutMap,
        const bsls::TimeInterval&           expirationTime);

    /// Load into the specified `correlationId` the correlationId of the
    /// item having the specified `key` if found .  Return 0 if `key` was
    /// found, non-zero otherwise.
    int find(bmqt::CorrelationId*     correlationId,
             const bmqt::MessageGUID& key) const;

    /// Return the current number of elements in the container.
    size_t size() const;

    /// Return the current number of PUT messages in the container.
    size_t numberOfPuts() const;

    /// Return the current number of control requests in the container.
    size_t numberOfControls() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------------------
// class MessageCorrelationIdContainer
// -----------------------------------

inline size_t MessageCorrelationIdContainer::size() const
{
    return d_correlationIds.size();
}

inline size_t MessageCorrelationIdContainer::numberOfPuts() const
{
    return d_numPuts;
}

inline size_t MessageCorrelationIdContainer::numberOfControls() const
{
    return d_numControls;
}

}  // close package namespace
}  // close enterprise namespace

#endif
