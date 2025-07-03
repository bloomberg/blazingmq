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

// mqbblp_messagegroupidmanager.cpp                                   -*-C++-*-
#include <mqbblp_messagegroupidmanager.h>

#include <mqbscm_version.h>
/// Implementation Notes
///====================
//
/// MessageGroupIdManager
///---------------------
// We need this class to support 5 operations very efficiently:
//
// 1.  Addition of new Handle
// 2.  Removal of a Handle
// 3.  Allocation of a Message Group Id to the least loaded Handle
// 4.  Cleanup of Message Group Id that haven't been used for a while
// 5.  When a new Handle arrives, Message Group Ids might get rebalanced
//
// In order to optimize those operations, we use 4 data structures:
//
// a)  A map ('MsgGroupIdInfo') from Message Group Id to the assigned Handle
//     and last used time.  This allows fast access to assigned Handle if
//     already set.  It also allows us to quickly get from a 'MsgGroupId' to an
//     'iterator' to this data structure which is handy, because those
//     'iterator's are used extensively.
// b)  A set ('LeastUsedMsgGroupIdFirst') with 'iterator's to 'MsgGroupIdInfo'
//     in such order that the least recently used one (the one with the oldest
//     timestamp) is on the beginning of this set.  This allows use to quickly
//     retrieve Message Group Ids that are old and can get Garbage Collected.
// c)  A map ('HandleToGroups') from Handles to a set of
//     'MsgGroupIdInfo::iterator's that correspond to the Message Group Ids
//     assigned to this Handle.  This allows use to remove handles efficiently
//     as well as know how many and which Message Group Ids to rebalance if
//     required.
// d)  A set ('LeastLoadedHandleFirst') with 'HandleToGroups::iterator's
//     ordered in such a way that the Handle with the least Message Group Ids
//     is always at the beginning of the set.  This allows use to quickly map
//     the least mapped Handle when a new unmapped Message Group Id arrives.
//
//                                +-----------------------------+
//                                | b) LeastUsedMsgGroupIdFirst |
//                                +---+---------------+---------+
//                                    |               |
// +---------------------------+      | < Timestamp   |
// | a) MsgGroupIdInfo         |      |               |
// |                           | <----+               |
// |         +--------------+  |                      |
// | ID +--> | Handle, Time |  | <--------------------+
// |         +--------------+  |
// |                           | <------+
// +------------------+-----+--+ <+     |
//                    ^     ^     |     |
//  +----------------------------------------------+
//  |                 |     |     |     |          |
//  |              +--+--+--+--+--+--+--+--+       |
//  |  Handle +--> +  *  |  *  |  *  |  *  |   ... |
//  |              +-----------+-----+-----+    ^  |
//  | c) HandleToGroups  ^                      |  |
//  +----------------------------------------------+
//                       |                      |
//                       | < Loaded             |
//              +--------+-------------------+  |
//              | d) LeastLoadedHandleFirst  |--+
//              +----------------------------+
//

// MQB
#include <mqbcmd_messages.h>

#include <bmqu_outstreamformatsaver.h>
#include <bmqu_printutil.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_map.h>
#include <bsl_sstream.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bsla_annotations.h>
#include <bslim_printer.h>

namespace BloombergLP {
namespace mqbblp {

namespace {

/// A Handle and the time we last used it (for a given Message Group Id).
typedef bsl::pair<MessageGroupIdManager::Handle, MessageGroupIdManager::Time>
    HandleAndTime;

/// A mapping from a Message Group Id to it's assigned Handle and time last
/// seen.  This can't be an `unordered_set` because if we used that instead,
/// iterators might become invalid during `insert()`.  This is data
/// structure a).
typedef bsl::map<MessageGroupIdManager::MsgGroupId, HandleAndTime>
    MsgGroupIdInfo;

// -------------------------
// struct GroupLruComparator
// -------------------------

/// Returns `true` if the specified `lhs` is considered less than the
/// specified `rhs` or `false` otherwise.
struct GroupLruComparator {
    bool operator()(const MsgGroupIdInfo::iterator& lhs,
                    const MsgGroupIdInfo::iterator& rhs) const;
};

bool GroupLruComparator::operator()(const MsgGroupIdInfo::iterator& lhs,
                                    const MsgGroupIdInfo::iterator& rhs) const
{
    const HandleAndTime& left  = lhs->second;
    const HandleAndTime& right = rhs->second;

    // Compares the time in a way that prioritizes oldest.
    if (left.second < right.second) {
        return true;  // RETURN
    }
    if (left.second > right.second) {
        return false;  // RETURN
    }

    // Compares Message Group Ids in a way that prioritizes the one with the
    // lexicographically smallest id.
    return lhs->first < rhs->first;
}

// -------------------------------
// struct DirectMsgGroupComparator
// -------------------------------

/// Returns `true` if the specified `lhs` is considered less than the
/// specified `rhs` or `false` otherwise.
struct DirectMsgGroupComparator {
    bool operator()(const MsgGroupIdInfo::iterator& lhs,
                    const MsgGroupIdInfo::iterator& rhs) const;
};

bool DirectMsgGroupComparator::operator()(
    const MsgGroupIdInfo::iterator& lhs,
    const MsgGroupIdInfo::iterator& rhs) const
{
    // Compares Message Group Ids in a way that prioritizes the one with the
    // lexicographically smallest id.
    return lhs->first < rhs->first;
}

/// A set of `MsgGroupIdInfo::iterator`s.  Note that the order is not
/// important so reuse the age order.
typedef bsl::set<MsgGroupIdInfo::iterator, DirectMsgGroupComparator>
    MsgGroupIdSet;

/// A mapping from `Handle`s to sets of Message Group Ids.  This can't be an
/// `unordered_set` because if we used that instead, iterators might become
/// invalid during `insert()`.  This is data structure c).
typedef bsl::map<MessageGroupIdManager::Handle, MsgGroupIdSet> HandleToGroups;

// -------------------------
// struct HandlesOrderPolicy
// -------------------------

/// Returns `true` if the specified `lhs` is considered less than the
/// specified `rhs` or `false` otherwise.
struct HandlesOrderPolicy {
    bool operator()(const HandleToGroups::iterator& lhs,
                    const HandleToGroups::iterator& rhs) const;
};

bool HandlesOrderPolicy::operator()(const HandleToGroups::iterator& lhs,
                                    const HandleToGroups::iterator& rhs) const
{
    const int left  = lhs->second.size();
    const int right = rhs->second.size();

    // Compares the count of groups in a way that prioritizes least used.
    if (left < right) {
        return true;  // RETURN
    }
    if (left > right) {
        return false;  // RETURN
    }

    // Sizes the same - give up and compare Handles themselves.
    return lhs->first < rhs->first;
}

/// A set of Message Group Ids ordered in a way that the least used is at
/// the beginning.  This is data structure b).
typedef bsl::set<MsgGroupIdInfo::iterator, GroupLruComparator>
    LeastUsedMsgGroupIdFirst;

/// A set of Handles ordered in a way that the least loaded one is at the
/// beginning.  This is data structure d).
typedef bsl::set<HandleToGroups::iterator, HandlesOrderPolicy>
    LeastLoadedHandleFirst;

/// Returns the last-seen timestamp for the `MsgGroupIdInfo` of the
/// specified `value`.
const MessageGroupIdManager::Time&
lastSeenFor(const MsgGroupIdInfo::value_type& value)
{
    return value.second.second;
}

/// Returns the Handle for the `MsgGroupIdInfo` of the specified `value`.
const MessageGroupIdManager::Handle&
handleFor(const MsgGroupIdInfo::value_type& value)
{
    return value.second.first;
}

/// Returns the Message Group Id for the `MsgGroupIdInfo` of the specified
/// `value`.
const MessageGroupIdManager::MsgGroupId&
msgGroupIdFor(const MsgGroupIdInfo::value_type& value)
{
    return value.first;
}

}  // close unnamed namespace

// ----------------------------------
// class MessageGroupIdManager::Index
// ----------------------------------

class MessageGroupIdManager::Index {
  private:
    // DATA
    bslma::Allocator*        d_allocator_p;
    HandleToGroups           d_handleToGroups;
    LeastLoadedHandleFirst   d_leastLoadedHandleFirst;
    MsgGroupIdInfo           d_msgGroupIdInfo;
    LeastUsedMsgGroupIdFirst d_leastUsedMsgGroupIdFirst;

  private:
    // NOT IMPLEMENTED
    Index(const Index&);             // = delete
    Index& operator=(const Index&);  // = delete

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Index, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Creates a new `Index` using the specified `allocator_p`.
    explicit Index(bslma::Allocator* allocator_p);

    // MANIPULATORS

    /// Add the specified `handle` to the Manager.
    void addHandle(const Handle& handle);

    /// The specified `handle` will be completely removed.  Any Message
    /// Group Ids allocated to it will be removed and the specified
    /// `onDelete` method will be called just before removing, for each one
    /// of them to enable follow-up actions.
    void removeHandle(const Handle& handle);

    /// Inserts a new mapping for the specified `msgGroupId` and `lastSeen`
    /// to the appropriate (least loaded) Handle.
    MsgGroupIdInfo::iterator insert(const MsgGroupId& msgGroupId,
                                    const Time&       lastSeen);

    /// Updates the last seen timestamp for the record corresponding to the
    /// specified `target` iterator to the specified `lastSeen` value.  This
    /// is more efficient than removing and re-adding the mapping.
    void updateTime(const MsgGroupIdInfo::iterator& target,
                    const Time&                     lastSeen);

    /// Limit the number of Message Group Ids per Handle to the specified
    /// `size` limit.  Any excessive Message Group Ids will be removed, and
    /// get added to the specified `tax` vector, to enable follow-up
    /// actions.
    void fixSize(bsl::vector<MsgGroupIdInfo::value_type>* tax, const int size);

    /// Erases the specified `iterator` from all the data structures.  It
    /// takes `iterator` by value (fast anyway) to prevent accidental pass
    /// of a reference that might become invalid during `erase()`.
    void erase(const MsgGroupIdInfo::iterator iterator);

    /// Returns a valid iterator if the specified `msgGroupId` is found in
    /// this manager or `end()` otherwise.
    MsgGroupIdInfo::iterator find(const MsgGroupId& msgGroupId);

    /// Returns an invalid iterator.
    MsgGroupIdInfo::iterator end();

    /// Returns the least recently used mapping or `end()` if there are no
    /// Message Group Ids tracked by this manager.
    MsgGroupIdInfo::iterator lru();

    // ACCESSORS

    /// Returns the number of Handles on this Manager.
    int handlesCount() const;

    /// Returns the number of Message Group Ids tracked by this manager.
    int msgGroupIdsCount() const;

    /// Load into the specified `out` object internal details about this
    /// queue state.
    void loadInternals(mqbcmd::MessageGroupIdManagerIndex* out,
                       const Time&                         now) const;

    /// Load into the specified `ids` the Message Group Ids for the
    /// specified `handle`.
    void idsForHandle(IdsForHandle* ids, const Handle& handle) const;
};

// CREATORS
MessageGroupIdManager::Index::Index(bslma::Allocator* allocator_p)
: d_allocator_p(allocator_p)
, d_handleToGroups(allocator_p)
, d_leastLoadedHandleFirst(allocator_p)
, d_msgGroupIdInfo(allocator_p)
, d_leastUsedMsgGroupIdFirst(allocator_p)
{
}

// MANIPULATORS
void MessageGroupIdManager::Index::addHandle(const Handle& handle)
{
    // We don't need to add to data structures a) and b) because there's no
    // Message Group Id for this Handle yet.

    // Add to data structure c).
    bsl::pair<Handle, MsgGroupIdSet> empty(d_allocator_p);
    empty.first = handle;
    const bsl::pair<HandleToGroups::iterator, bool> result =
        d_handleToGroups.insert(empty);
    BSLS_ASSERT_SAFE(result.second);  // The Handle should not already exist.

    // Add to data structure d).
    d_leastLoadedHandleFirst.insert(result.first);
}

void MessageGroupIdManager::Index::removeHandle(const Handle& handle)
{
    // This operation is optimized because in terms of removing from data
    // structures c) and d), since we know we will remove everything for the
    // specified 'handle'.

    // We find the iterator for data structure c).
    const HandleToGroups::iterator it = d_handleToGroups.find(handle);
    BSLS_ASSERT_SAFE(it != d_handleToGroups.end());
    if (it == d_handleToGroups.end()) {
        return;  // RETURN
    }

    // For every Message Group Id for this handle...
    const MsgGroupIdSet& msgGroupIdSet = it->second;
    for (MsgGroupIdSet::iterator mit = msgGroupIdSet.begin();
         mit != msgGroupIdSet.end();
         ++mit) {
        const MsgGroupIdInfo::iterator& iterator = *mit;

        BSLS_ASSERT_SAFE(iterator != d_msgGroupIdInfo.end());

        // Remove from data structure b).
        BSLA_MAYBE_UNUSED const int erased = d_leastUsedMsgGroupIdFirst.erase(
            iterator);
        BSLS_ASSERT_SAFE(erased == 1);

        // Remove from data structure a).
        d_msgGroupIdInfo.erase(iterator);
    }

    // Bulk erase from data structures c) and d).
    d_leastLoadedHandleFirst.erase(it);
    d_handleToGroups.erase(it);
}

MsgGroupIdInfo::iterator
MessageGroupIdManager::Index::insert(const MsgGroupId& msgGroupId,
                                     const Time&       lastSeen)
{
    BSLS_ASSERT_SAFE(!d_leastLoadedHandleFirst.empty());

    // Get a handle from data structure d).  We can directly update c) by using
    // it.
    HandleToGroups::iterator it = *d_leastLoadedHandleFirst.begin();

    const Handle& handle = it->first;

    // Insert to data structure a).
    bsl::pair<Handle, Time>    info(handle, lastSeen);
    MsgGroupIdInfo::value_type value(msgGroupId, info, d_allocator_p);

    const bsl::pair<MsgGroupIdInfo::iterator, bool> result =
        d_msgGroupIdInfo.insert(value);
    BSLS_ASSERT_SAFE(result.second);
    MsgGroupIdInfo::iterator mit = result.first;
    BSLS_ASSERT_SAFE(mit != d_msgGroupIdInfo.end());

    // Insert to data structure b).
    d_leastUsedMsgGroupIdFirst.insert(mit);

    // Update data structure d) (1/2).  This is a 2-step process because 'it'
    // won't be found if we try to 'erase()' after modifying the number of
    // Message Group Ids, since that number is incorporated in the comparison
    // operation used during 'erase()'.
    d_leastLoadedHandleFirst.erase(it);

    // Insert to data structure c).
    BSLA_MAYBE_UNUSED const bsl::pair<MsgGroupIdSet::iterator, bool> rs =
        it->second.insert(mit);
    BSLS_ASSERT_SAFE(rs.second);

    // Update data structure d) (2/2).
    d_leastLoadedHandleFirst.insert(it);

    return mit;
}

void MessageGroupIdManager::Index::updateTime(
    const MsgGroupIdInfo::iterator& target,
    const Time&                     lastSeen)
{
    // This does a remove and re-add to the data structure b) and a
    // modification in data structure a).  All the others should be unaffected.
    BSLA_MAYBE_UNUSED const int erased = d_leastUsedMsgGroupIdFirst.erase(
        target);
    BSLS_ASSERT_SAFE(erased == 1);

    target->second.second = lastSeen;

    (void)d_leastUsedMsgGroupIdFirst.insert(target);
}

void MessageGroupIdManager::Index::fixSize(
    bsl::vector<MsgGroupIdInfo::value_type>* tax,
    const int                                size)
{
    BSLS_ASSERT_SAFE(tax);

    // The most efficient way to iterate-by-handle is to use data structure c).
    // We iterate and remove redundant items from everything, while archiving
    // values in the specified 'tax' vector.
    for (HandleToGroups::iterator it = d_handleToGroups.begin();
         it != d_handleToGroups.end();
         ++it) {
        const MsgGroupIdSet&    msgGroupIdSet = it->second;
        int                     toTax         = msgGroupIdSet.size() - size;
        MsgGroupIdSet::iterator mit           = msgGroupIdSet.begin();
        for (int i = 0; i < toTax; ++i) {
            tax->push_back(**mit);
            // 'erase()' modifies all four data structures.
            erase(*mit++);
        }
    }
}

void MessageGroupIdManager::Index::erase(
    const MsgGroupIdInfo::iterator iterator)
{
    // We find the iterator for data structure c).
    const Handle&                  handle = handleFor(*iterator);
    const HandleToGroups::iterator it     = d_handleToGroups.find(handle);
    BSLS_ASSERT_SAFE(it != d_handleToGroups.end());
    if (it == d_handleToGroups.end()) {
        return;  // RETURN
    }

    // Update data structure d) (1/2).  This is a 2-step process because 'it'
    // won't be found if we try to 'erase()' after modifying the number of
    // Message Group Ids, since that number is incorporated in the comparison
    // operation used during 'erase()'.
    d_leastLoadedHandleFirst.erase(it);

    // Modify the appropriate 'MsgGroupIdSet' from the data structure c).  Note
    // that this doesn't remove any handle from 'd_handleToGroups' thus all
    // 'HandleToGroups::iterator's (including 'it') remain unaffected/valid.
    MsgGroupIdSet&              msgGroupIdSet = it->second;
    BSLA_MAYBE_UNUSED const int count         = msgGroupIdSet.erase(iterator);
    BSLS_ASSERT_SAFE(count == 1);

    // Update data structure d) (2/2).
    d_leastLoadedHandleFirst.insert(it);

    // Remove from data structure b).
    BSLA_MAYBE_UNUSED const int erased = d_leastUsedMsgGroupIdFirst.erase(
        iterator);
    BSLS_ASSERT_SAFE(erased == 1);

    // Remove from data structure a).
    d_msgGroupIdInfo.erase(iterator);
}

MsgGroupIdInfo::iterator
MessageGroupIdManager::Index::find(const MsgGroupId& msgGroupId)
{
    return d_msgGroupIdInfo.find(msgGroupId);
}

MsgGroupIdInfo::iterator MessageGroupIdManager::Index::end()
{
    return d_msgGroupIdInfo.end();
}

MsgGroupIdInfo::iterator MessageGroupIdManager::Index::lru()
{
    const LeastUsedMsgGroupIdFirst::const_iterator target =
        d_leastUsedMsgGroupIdFirst.begin();
    return (target == d_leastUsedMsgGroupIdFirst.end()) ? end() : *target;
}

// ACCESSORS

int MessageGroupIdManager::Index::handlesCount() const
{
    return d_handleToGroups.size();
}

int MessageGroupIdManager::Index::msgGroupIdsCount() const
{
    return d_msgGroupIdInfo.size();
}

void MessageGroupIdManager::Index::loadInternals(
    mqbcmd::MessageGroupIdManagerIndex* out,
    const Time&                         now) const
{
    // Reminder: LeastUsedMsgGroupIdFirst is a set of iterator into a map
    // from string to a pair of QueueHandle* and Int64.
    // The string is a message group ID, and the Int64 is a time in
    // nanoseconds.
    bsl::vector<mqbcmd::LeastRecentlyUsedGroupId>& lruGroupIds =
        out->leastRecentlyUsedGroupIds();
    lruGroupIds.reserve(d_leastUsedMsgGroupIdFirst.size());
    for (LeastUsedMsgGroupIdFirst::const_iterator it =
             d_leastUsedMsgGroupIdFirst.begin();
         it != d_leastUsedMsgGroupIdFirst.end();
         ++it) {
        lruGroupIds.resize(out->leastRecentlyUsedGroupIds().size() + 1);
        mqbcmd::LeastRecentlyUsedGroupId& lruGroupId = lruGroupIds.back();
        lruGroupId.clientDescription() =
            handleFor(**it)->client()->description();
        lruGroupId.msgGroupId()               = (*it)->first;
        lruGroupId.lastSeenDeltaNanoseconds() = now - lastSeenFor(**it);
    }

    out->numMsgGroupsPerClient().reserve(d_leastLoadedHandleFirst.size());
    for (LeastLoadedHandleFirst::const_iterator it =
             d_leastLoadedHandleFirst.begin();
         it != d_leastLoadedHandleFirst.end();
         ++it) {
        out->numMsgGroupsPerClient().resize(
            out->numMsgGroupsPerClient().size() + 1);
        mqbcmd::ClientMsgGroupsCount& clientMsgGroupsCount =
            out->numMsgGroupsPerClient().back();

        clientMsgGroupsCount.clientDescription() =
            (*it)->first->client()->description();
        clientMsgGroupsCount.numMsgGroupIds() = (*it)->second.size();
    }
}

void MessageGroupIdManager::Index::idsForHandle(IdsForHandle* ids,
                                                const Handle& handle) const
{
    BSLS_ASSERT_SAFE(ids);

    // Using data structure c) to get those directly.
    HandleToGroups::const_iterator handleSet = d_handleToGroups.find(handle);
    if (handleSet != d_handleToGroups.end()) {
        for (MsgGroupIdSet::const_iterator it = handleSet->second.begin();
             it != handleSet->second.end();
             ++it) {
            ids->insert((*it)->first);
        }
    }
}

// ---------------------------
// class MessageGroupIdManager
// ---------------------------

void MessageGroupIdManager::clearExpired(const Time& now)
{
    if (d_timeout <= 0) {
        return;  // RETURN
    }
    const Time expirationTime = now - d_timeout;
    for (MsgGroupIdInfo::iterator current = d_index->lru();
         current != d_index->end();
         current = d_index->lru()) {
        if (lastSeenFor(*current) > expirationTime) {
            break;  // BREAK
        }
        d_index->erase(current);
    }
}

// PRIVATE ACCESSORS
bool MessageGroupIdManager::exceedsMappingsLimit(const int extraSpace) const
{
    const bool isMaxGroupLimitActive = (d_maxMsgGroupIds > 0);
    if (!isMaxGroupLimitActive) {
        return false;  // RETURN
    }
    return ((d_index->msgGroupIdsCount() + extraSpace) > d_maxMsgGroupIds);
}

// CREATORS
MessageGroupIdManager::MessageGroupIdManager(const Time&       timeout,
                                             const int         maxMsgGroupIds,
                                             const Rebalance   rebalance,
                                             bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_timeout(timeout)
, d_maxMsgGroupIds(maxMsgGroupIds)
, d_rebalance(rebalance)
, d_index(new(*allocator) Index(allocator), allocator)
{
}

// MANIPULATORS
MessageGroupIdManager::Handle
MessageGroupIdManager::getHandle(const MsgGroupId& msgGroupId, const Time& now)
{
    clearExpired(now);

    MsgGroupIdInfo::iterator current = d_index->find(msgGroupId);
    if (current == d_index->end()) {
        const bool cantFitOneMore = exceedsMappingsLimit(1);
        if (cantFitOneMore && isRebalance()) {
            // With rebalanced queues, re-allocate the LRU if we run-out of
            // Message Group Ids.
            MsgGroupIdInfo::iterator target = d_index->lru();
            BSLS_ASSERT_SAFE(target != d_index->end());
            d_index->erase(target);
        }

        current = d_index->insert(msgGroupId, now);
    }
    else {
        d_index->updateTime(current, now);
    }

    return handleFor(*current);
}

void MessageGroupIdManager::addHandle(const Handle& handle, const Time& now)
{
    clearExpired(now);

    d_index->addHandle(handle);

    if (!isRebalance()) {
        return;  // RETURN
    }

    // Calculate target per-handle limit (round-up)
    const int handlesCount     = d_index->handlesCount();
    const int msgGroupIdsCount = d_index->msgGroupIdsCount();
    const int targetSize       = (msgGroupIdsCount + handlesCount - 1) /
                           handlesCount;

    typedef bsl::vector<MsgGroupIdInfo::value_type> TaxType;
    TaxType                                         tax(d_allocator_p);
    d_index->fixSize(&tax, targetSize);

    // Reinsert them to the least loaded 'handle'(s) - not necessarily the new
    // one...  In case of expiry for example, we might have other equally empty
    // older handles.  Taxed groups will be distributed equally.
    for (TaxType::const_iterator it = tax.begin(); it != tax.end(); ++it) {
        d_index->insert(msgGroupIdFor(*it), lastSeenFor(*it));
    }
}

void MessageGroupIdManager::removeHandle(const Handle& handle)
{
    d_index->removeHandle(handle);
}

// ACCESSORS

void MessageGroupIdManager::idsForHandle(IdsForHandle* ids,
                                         const Handle& handle) const
{
    d_index->idsForHandle(ids, handle);
}

MessageGroupIdManager::Time MessageGroupIdManager::timeout() const
{
    return d_timeout;
}

int MessageGroupIdManager::msgGroupIdsCount() const
{
    return d_index->msgGroupIdsCount();
}

int MessageGroupIdManager::handlesCount() const
{
    return d_index->handlesCount();
}

int MessageGroupIdManager::maxMsgGroupIds() const
{
    return d_maxMsgGroupIds;
}

bool MessageGroupIdManager::isRebalance() const
{
    return (d_rebalance == k_REBALANCE_ON);
}

bool MessageGroupIdManager::exceedsMaxMsgGroupIdsLimit() const
{
    return exceedsMappingsLimit(0);
}

void MessageGroupIdManager::loadInternals(mqbcmd::MessageGroupIdHelper* out,
                                          const Time& now) const
{
    out->timeoutNanoseconds() = timeout();
    out->maxMsgGroupIds()     = maxMsgGroupIds();
    out->isRebalanceOn()      = isRebalance();

    d_index->loadInternals(&out->status(), now);
}

}  // close package namespace
}  // close enterprise namespace
