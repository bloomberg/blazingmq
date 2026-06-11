// Copyright 2014-2023 Bloomberg Finance L.P.
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

#ifndef INCLUDED_BMQP_QUEUEINFO
#define INCLUDED_BMQP_QUEUEINFO

//@PURPOSE: Provide a template container for per-queue stream information.
//
//@CLASSES:
//  bmqp::QueueInfo: template container mapping {appId, subId} to a value
//
//@DESCRIPTION: 'bmqp::QueueInfo' provides a template container that maps
// a pair of {appId, subId} to a user-specified value type, with support for
// subscription management.  It wraps 'bmqc::TwoKeyHashMap' and provides
// named iterators with 'appId()', 'subId()', and 'value()' accessors.

// BMQ
#include <bmqc_twokeyhashmap.h>
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bsla_annotations.h>
#include <bslim_printer.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>

namespace BloombergLP {

namespace bmqp {

// ================
// struct QueueInfo
// ================

template <class VALUE>
// NOLINTBEGIN(cppcoreguidelines-special-member-functions)
struct QueueInfo {
    struct StreamInfo {
        VALUE d_value;

        StreamInfo(const VALUE& value);
    };

    typedef bmqc::TwoKeyHashMap<bsl::string, unsigned int, StreamInfo>
        StreamsMap;
    // Map {appId, subId} to StreamInfo

    /// This struct provides named access to `VALUE` - by `appId` and by
    /// `subId`
    // NOLINTBEGIN(cppcoreguidelines-special-member-functions)
    struct iterator {
        typename StreamsMap::iterator d_iterator;

        iterator(const typename StreamsMap::iterator& it);
        iterator(const iterator& other);

        VALUE& value();

        unsigned int subId() const;

        const bsl::string& appId() const;

        iterator& operator++();

        iterator* operator->();

        bool operator!=(const iterator& other) const;
        bool operator==(const iterator& other) const;
    };
    // NOLINTEND(cppcoreguidelines-special-member-functions)
    /// This struct provides named access to `VALUE` - by `appId` and by
    /// `subId`
    struct const_iterator {
        typename StreamsMap::const_iterator d_iterator;

        const_iterator(const typename StreamsMap::iterator& it);
        const_iterator(const typename StreamsMap::const_iterator& cit);

        const_iterator(const iterator& other);

        const VALUE& value() const;

        unsigned int subId() const;

        const bsl::string& appId() const;

        const_iterator& operator++();

        const_iterator* operator->();

        bool operator!=(const const_iterator& other) const;
        bool operator==(const const_iterator& other) const;
    };

    /// Map subscriptionId to StreamInfo
    typedef bsl::unordered_map<unsigned int, iterator> SubscriptionsMap;
    typedef bsl::vector<bmqp_ctrlmsg::Subscription>    Subscriptions;

    StreamsMap d_streams;

    SubscriptionsMap d_subscriptions;

    bslma::Allocator* d_allocator_p;

    QueueInfo(bslma::Allocator* allocator);
    QueueInfo(const QueueInfo& other, bslma::Allocator* allocator = 0);

    iterator
    insert(const bsl::string& appId, unsigned int subId, const VALUE& value);
    iterator insert(const bmqp_ctrlmsg::QueueHandleParameters& parameters,
                    const VALUE&                               value);
    iterator insert(const bmqp_ctrlmsg::QueueStreamParameters& parameters,
                    const VALUE&                               value);

    iterator       findBySubIdSafe(unsigned int subId);
    const_iterator findBySubIdSafe(unsigned int subId) const;

    iterator       findByAppIdSafe(const bsl::string& appId);
    const_iterator findByAppIdSafe(const bsl::string& appId) const;

    iterator       findBySubId(unsigned int subId);
    const_iterator findBySubId(unsigned int subId) const;

    iterator findByHandleParameters(
        const bmqp_ctrlmsg::QueueHandleParameters& parameters);

    iterator       findByAppId(const bsl::string& appId);
    const_iterator findByAppId(const bsl::string& appId) const;

    const_iterator findBySubscriptionIdSafe(unsigned int subscriptionId) const;

    const_iterator findBySubscriptionId(unsigned int subscriptionId) const;

    const_iterator begin() const;
    iterator       begin();

    const_iterator end() const;
    iterator       end();

    void erase(const_iterator it);

    void removeSubscriptions(const bsl::string& appId);

    void addSubscriptions(const bmqp_ctrlmsg::StreamParameters& parameters);

    void clear();

    typename StreamsMap::size_type size() const;
};
// NOLINTEND(cppcoreguidelines-special-member-functions)

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------------------
// struct QueueInfo::StreamInfo
// ----------------------------

template <class VALUE>
inline QueueInfo<VALUE>::StreamInfo::StreamInfo(const VALUE& value)
: d_value(value)
{
    // NOTHING
}

// --------------------------
// struct QueueInfo::iterator
// --------------------------

template <class VALUE>
inline QueueInfo<VALUE>::iterator::iterator(
    const typename StreamsMap::iterator& it)
: d_iterator(it)
{
    // NOTHING
}

template <class VALUE>
inline QueueInfo<VALUE>::iterator::iterator(const iterator& other)
: d_iterator(other.d_iterator)
{
    // NOTHING
}

template <class VALUE>
inline VALUE& QueueInfo<VALUE>::iterator::value()
{
    return d_iterator->value().d_value;
}

template <class VALUE>
inline unsigned int QueueInfo<VALUE>::iterator::subId() const
{
    return d_iterator->key2();
}

template <class VALUE>
inline const bsl::string& QueueInfo<VALUE>::iterator::appId() const
{
    return d_iterator->key1();
}

template <class VALUE>
inline typename QueueInfo<VALUE>::iterator&
QueueInfo<VALUE>::iterator::operator++()
{
    ++d_iterator;
    return *this;
}

template <class VALUE>
inline typename QueueInfo<VALUE>::iterator*
QueueInfo<VALUE>::iterator::operator->()
{
    return this;
}

template <class VALUE>
inline bool QueueInfo<VALUE>::iterator::operator!=(const iterator& other) const
{
    return d_iterator != other.d_iterator;
}

template <class VALUE>
inline bool QueueInfo<VALUE>::iterator::operator==(const iterator& other) const
{
    return d_iterator == other.d_iterator;
}

// --------------------------------
// struct QueueInfo::const_iterator
// --------------------------------

template <class VALUE>
inline QueueInfo<VALUE>::const_iterator::const_iterator(
    const typename StreamsMap::iterator& it)
: d_iterator(it)
{
    // NOTHING
}

template <class VALUE>
inline QueueInfo<VALUE>::const_iterator::const_iterator(
    const typename StreamsMap::const_iterator& cit)
: d_iterator(cit)
{
    // NOTHING
}

template <class VALUE>
inline QueueInfo<VALUE>::const_iterator::const_iterator(const iterator& other)
: d_iterator(other.d_iterator)
{
    // NOTHING
}

template <class VALUE>
inline const VALUE& QueueInfo<VALUE>::const_iterator::value() const
{
    return d_iterator->value().d_value;
}

template <class VALUE>
inline unsigned int QueueInfo<VALUE>::const_iterator::subId() const
{
    return d_iterator->key2();
}

template <class VALUE>
inline const bsl::string& QueueInfo<VALUE>::const_iterator::appId() const
{
    return d_iterator->key1();
}

template <class VALUE>
inline typename QueueInfo<VALUE>::const_iterator&
QueueInfo<VALUE>::const_iterator::operator++()
{
    ++d_iterator;
    return *this;
}

template <class VALUE>
inline typename QueueInfo<VALUE>::const_iterator*
QueueInfo<VALUE>::const_iterator::operator->()
{
    return this;
}

template <class VALUE>
inline bool
QueueInfo<VALUE>::const_iterator::operator!=(const const_iterator& other) const
{
    return d_iterator != other.d_iterator;
}

template <class VALUE>
inline bool
QueueInfo<VALUE>::const_iterator::operator==(const const_iterator& other) const
{
    return d_iterator == other.d_iterator;
}

template <class VALUE>
inline QueueInfo<VALUE>::QueueInfo(bslma::Allocator* allocator)
: d_streams(allocator)
, d_subscriptions(allocator)
, d_allocator_p(allocator)
{
}

template <class VALUE>
inline QueueInfo<VALUE>::QueueInfo(const QueueInfo&  other,
                                   bslma::Allocator* allocator)
: d_streams(other.d_streams)
, d_subscriptions(other.d_subscriptions)
, d_allocator_p(allocator)
{
    // NOTHING
}

template <class VALUE>
inline typename QueueInfo<VALUE>::iterator
QueueInfo<VALUE>::insert(const bsl::string& appId,
                         unsigned int       subId,
                         const VALUE&       value)
{
    // Extract previous Subscriptions pointing to the 'appId'
    bsl::vector<bsls::Types::Uint64> subscriptions(d_allocator_p);

    typename SubscriptionsMap::iterator it = d_subscriptions.begin();
    while (it != d_subscriptions.end()) {
        iterator subQueue(it->second);
        if (subQueue->appId() == appId) {
            subscriptions.push_back(it->first);
            it = d_subscriptions.erase(it);
        }
        else {
            ++it;
        }
    }

    d_streams.eraseByKey1(appId);

    bsl::pair<typename StreamsMap::iterator, typename StreamsMap::InsertResult>
        result = d_streams.insert(appId, subId, StreamInfo(value));

    BSLS_ASSERT_SAFE(result.second == StreamsMap::e_INSERTED);

    // Point extracted Subscriptions to the new entry
    iterator itStream(result.first);

    for (size_t i = 0; i < subscriptions.size(); ++i) {
        BSLA_MAYBE_UNUSED bsl::pair<typename SubscriptionsMap::iterator, bool>
                          insertRC = d_subscriptions.insert(
                bsl::make_pair(subscriptions[i], itStream));
        BSLS_ASSERT_SAFE(insertRC.second);
    }
    return itStream;
}

template <class VALUE>
inline typename QueueInfo<VALUE>::iterator
QueueInfo<VALUE>::insert(const bmqp_ctrlmsg::QueueHandleParameters& parameters,
                         const VALUE&                               value)
{
    bmqp_ctrlmsg::SubQueueIdInfo id(d_allocator_p);

    if (!parameters.subIdInfo().isNull()) {
        id = parameters.subIdInfo().value();
    }

    return insert(id.appId(), id.subId(), value);
}

template <class VALUE>
inline typename QueueInfo<VALUE>::iterator
QueueInfo<VALUE>::insert(const bmqp_ctrlmsg::QueueStreamParameters& parameters,
                         const VALUE&                               value)
{
    bmqp_ctrlmsg::SubQueueIdInfo id(d_allocator_p);

    if (!parameters.subIdInfo().isNull()) {
        id = parameters.subIdInfo().value();
    }

    return insert(id.appId(), id.subId(), value);
}

template <class VALUE>
inline typename QueueInfo<VALUE>::iterator
QueueInfo<VALUE>::findBySubIdSafe(unsigned int subId)
{
    return iterator(d_streams.findByKey2(subId));
}

template <class VALUE>
inline typename QueueInfo<VALUE>::const_iterator
QueueInfo<VALUE>::findBySubIdSafe(unsigned int subId) const
{
    return const_iterator(d_streams.findByKey2(subId));
}

template <class VALUE>
inline typename QueueInfo<VALUE>::iterator
QueueInfo<VALUE>::findByAppIdSafe(const bsl::string& appId)
{
    return iterator(d_streams.findByKey1(appId));
}

template <class VALUE>
inline typename QueueInfo<VALUE>::const_iterator
QueueInfo<VALUE>::findByAppIdSafe(const bsl::string& appId) const
{
    return const_iterator(d_streams.findByKey1(appId));
}

template <class VALUE>
inline typename QueueInfo<VALUE>::iterator
QueueInfo<VALUE>::findBySubId(unsigned int subId)
{
    iterator result = findBySubIdSafe(subId);
    BSLS_ASSERT_SAFE(result != d_streams.end());

    return result;
}

template <class VALUE>
inline typename QueueInfo<VALUE>::const_iterator
QueueInfo<VALUE>::findBySubId(unsigned int subId) const
{
    const_iterator result = findBySubIdSafe(subId);
    BSLS_ASSERT_SAFE(result != d_streams.end());

    return result;
}

template <class VALUE>
inline typename QueueInfo<VALUE>::iterator
QueueInfo<VALUE>::findByHandleParameters(
    const bmqp_ctrlmsg::QueueHandleParameters& parameters)
{
    bmqp_ctrlmsg::SubQueueIdInfo id(d_allocator_p);

    if (!parameters.subIdInfo().isNull()) {
        id = parameters.subIdInfo().value();
    }
    return iterator(d_streams.findByKey2(id.subId()));
}

template <class VALUE>
inline typename QueueInfo<VALUE>::iterator
QueueInfo<VALUE>::findByAppId(const bsl::string& appId)
{
    iterator result = findByAppIdSafe(appId);
    BSLS_ASSERT_SAFE(result != d_streams.end());

    return result;
}

template <class VALUE>
inline typename QueueInfo<VALUE>::const_iterator
QueueInfo<VALUE>::findByAppId(const bsl::string& appId) const
{
    const_iterator result = findByAppIdSafe(appId);
    BSLS_ASSERT_SAFE(result != d_streams.end());

    return result;
}

template <class VALUE>
inline typename QueueInfo<VALUE>::const_iterator
QueueInfo<VALUE>::findBySubscriptionIdSafe(unsigned int subscriptionId) const
{
    typename SubscriptionsMap::const_iterator cit = d_subscriptions.find(
        subscriptionId);
    return cit == d_subscriptions.end() ? end() : const_iterator(cit->second);
}

template <class VALUE>
inline typename QueueInfo<VALUE>::const_iterator
QueueInfo<VALUE>::findBySubscriptionId(unsigned int subscriptionId) const
{
    typename SubscriptionsMap::const_iterator cit = d_subscriptions.find(
        subscriptionId);

    BSLS_ASSERT_SAFE(cit != d_subscriptions.end());

    return const_iterator(cit->second);
}
template <class VALUE>
inline typename QueueInfo<VALUE>::const_iterator
QueueInfo<VALUE>::begin() const
{
    return const_iterator(d_streams.begin());
}

template <class VALUE>
inline typename QueueInfo<VALUE>::iterator QueueInfo<VALUE>::begin()
{
    return iterator(d_streams.begin());
}

template <class VALUE>
inline typename QueueInfo<VALUE>::const_iterator QueueInfo<VALUE>::end() const
{
    return const_iterator(d_streams.end());
}

template <class VALUE>
inline typename QueueInfo<VALUE>::iterator QueueInfo<VALUE>::end()
{
    return iterator(d_streams.end());
}

template <class VALUE>
inline void QueueInfo<VALUE>::erase(const_iterator it)
{
    removeSubscriptions(it->appId());

    d_streams.erase(it.d_iterator);
}

template <class VALUE>
inline void QueueInfo<VALUE>::removeSubscriptions(const bsl::string& appId)
{
    typename SubscriptionsMap::iterator it = d_subscriptions.begin();
    while (it != d_subscriptions.end()) {
        iterator subQueue(it->second);
        if (subQueue->appId() == appId) {
            it = d_subscriptions.erase(it);
        }
        else {
            ++it;
        }
    }
}

template <class VALUE>
inline void QueueInfo<VALUE>::addSubscriptions(
    const bmqp_ctrlmsg::StreamParameters& parameters)
{
    removeSubscriptions(parameters.appId());

    iterator itStream = findByAppId(parameters.appId());

    const Subscriptions& subscriptions = parameters.subscriptions();
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (Subscriptions::const_iterator it = subscriptions.begin();
         it != subscriptions.end();
         ++it) {
        BSLA_MAYBE_UNUSED bsl::pair<typename SubscriptionsMap::iterator, bool>
                          insertRC = d_subscriptions.insert(
                bsl::make_pair(it->sId(), itStream));
        BSLS_ASSERT_SAFE(insertRC.second);
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

template <class VALUE>
inline void QueueInfo<VALUE>::clear()
{
    d_streams.clear();
    d_subscriptions.clear();
}

template <class VALUE>
inline typename QueueInfo<VALUE>::StreamsMap::size_type
QueueInfo<VALUE>::size() const
{
    return d_streams.size();
}

template <class T>
inline bsl::ostream& operator<<(bsl::ostream& os, const QueueInfo<T>& rhs)
{
    bslim::Printer printer(&os, 0, -1);  // one line

    printer.printValue(rhs.begin(), rhs.end());

    return os;
}

}  // close package namespace
}  // close enterprise namespace

#endif
