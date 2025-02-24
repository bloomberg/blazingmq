// Copyright 2019-2023 Bloomberg Finance L.P.
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

// bmqc_orderedhashmapwithhistory.h                                   -*-C++-*-
#ifndef INCLUDED_BMQC_ORDEREDHASHMAPWITHHISTORY
#define INCLUDED_BMQC_ORDEREDHASHMAPWITHHISTORY

//@PURPOSE: Provide a hash set with predictive iteration order and a history of
//          erased items.
//
//@CLASSES:
//  bmqc::OrderedHashMapWithHistory : Hash table with predictive iteration
//                                    order and history.
//
//@SEE_ALSO: bmqc::OrderedHashMap
//
//@DESCRIPTION: 'bmqc::OrderedHashMapWithHistory' is a wrapper around
// 'bmqc::OrderedHashMap' which adds insertion time in nanoseconds as part of
// the value.  It keeps history of erased keys until called 'gc' outside of
// specified time window.  For optimization (at expense of extra memory), it
// tracks the history from the moment an item is inserted.  That means, there
// are 3 collections effectively: 1) a hashtable, 2) a list of all items
// including erased ones which get tracked as history, and 3) a list of valid,
// not-erased items.  'OrderedHashMap' provides the 1) and the 2).  This
// component adds 3) and exposes new iterator over valid, not-erased items.
//

#include <bmqc_orderedhashmap.h>

// BDE
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bsl_utility.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmf_removecv.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bmqc {

// ===========================================
// struct OrderedHashMapWithHistory_ImpDetails
// ===========================================

/// PRIVATE CLASS.
// For use only by `bmqc::OrderedHashMapWithHistory` implementation.
struct OrderedHashMapWithHistory_ImpDetails {
    // PRIVATE CLASS DATA
    /// How many messages to GC when GC required in
    /// `bmqc::OrderedHashMapWithHistory::insert`
    static const int k_INSERT_GC_MESSAGES_BATCH_SIZE;
};

// ========================================
// class OrderedHashMapWithHistory_Iterator
// ========================================

/// For use only by `bmqc::OrderedHashMapWithHistory` implementation.
/// This iterator iterates un-expired (not historical) records.
template <class VALUE>
class OrderedHashMapWithHistory_Iterator {
  private:
    // PRIVATE TYPES
    typedef typename bsl::remove_cv<VALUE>::type       NcType;
    typedef OrderedHashMapWithHistory_Iterator<NcType> NcIter;

    typedef OrderedHashMap_SequentialIterator<VALUE> BaseIterator;

    // FRIENDS
    template <class OHM_KEY,
              class OHM_VALUE,
              class OHM_HASH,
              typename OHM_VALUE_TYPE>
    friend class OrderedHashMapWithHistory;
    friend class OrderedHashMapWithHistory_Iterator<const VALUE>;

    template <class VALUE1, class VALUE2>
    friend bool operator==(const OrderedHashMapWithHistory_Iterator<VALUE1>&,
                           const OrderedHashMapWithHistory_Iterator<VALUE2>&);

    // DATA
    BaseIterator d_baseIterator;

  private:
    // PRIVATE CREATORS

    /// Create an iterator instance pointing to the same item as the
    /// specified `baseIterator`.
    explicit OrderedHashMapWithHistory_Iterator(
        const BaseIterator& baseIterator);

  public:
    // CREATORS

    /// Create a singular iterator (i.e., one that cannot be incremented,
    /// decremented, or dereferenced.
    OrderedHashMapWithHistory_Iterator();

    /// Create an iterator to `VALUE` from the corresponding iterator to
    /// non-const `VALUE`.  If `VALUE` is not const-qualified, then this
    /// constructor becomes the copy constructor.  Otherwise, the copy
    /// constructor is implicitly generated.
    OrderedHashMapWithHistory_Iterator(const NcIter& other);

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    OrderedHashMapWithHistory_Iterator& operator=(const NcIter& rhs);

    /// Advance this iterator to the next element in the sequential list and
    /// return its new value.  The behavior is undefined unless this
    /// iterator is in the range `[begin() .. end())` (i.e., the iterator is
    /// not singular, is not `end()`, and has not been invalidated).
    OrderedHashMapWithHistory_Iterator& operator++();

    /// Advance this iterator to the next element in the sequential list and
    /// return its previous value.  The behavior is undefined unless this
    /// iterator is in the range `[begin() .. end())` (i.e., the iterator is
    /// not singular, is not `end()`, and has not been invalidated).
    OrderedHashMapWithHistory_Iterator operator++(int);

    // ACCESSORS

    /// Return a reference to the list object referenced by this iterator.
    /// The behavior is undefined unless this iterator is in the range
    /// `[begin() .. end())` (i.e., the iterator is not singular, is not
    /// `end()`, and has not been invalidated).
    VALUE& operator*() const;

    /// Return a pointer to the list object referenced by this iterator.
    /// The behavior is undefined unless this iterator is in the range
    /// `[begin() .. end())` (i.e., the iterator is not singular, is not
    /// `end()`, and has not been invalidated).
    VALUE* operator->() const;
};

// FREE OPERATORS

/// Return `true` if the specified iterators `lhs` and `rhs` have the same
/// value and `false` otherwise.  Two iterators have the same value if both
/// refer to the same element of the same list or both are the end()
/// iterator of the same list.  The return value is undefined unless both
/// `lhs` and `rhs` are non-singular.
template <class VALUE1, class VALUE2>
bool operator==(const OrderedHashMapWithHistory_Iterator<VALUE1>& lhs,
                const OrderedHashMapWithHistory_Iterator<VALUE2>& rhs);

/// Return `true` if the specified iterators `lhs` and `rhs` do not have the
/// same value and `false` otherwise.  Two iterators have the same value if
/// both refer to the same element of the same list or both are the end()
/// iterator of the same list.  The return value is undefined unless both
/// `lhs` and `rhs` are non-singular.
template <class VALUE1, class VALUE2>
bool operator!=(const OrderedHashMapWithHistory_Iterator<VALUE1>& lhs,
                const OrderedHashMapWithHistory_Iterator<VALUE2>& rhs);

/// Placeholder to clean live item when it becomes history.
template <class VALUE>
void clean(VALUE& value);

// ===============================
// class OrderedHashMapWithHistory
// ===============================

/// This class provides a hash table with predictive iteration order and a
/// history to track deleted items up to specified timeout.
template <typename KEY,
          typename VALUE,
          typename HASH       = bsl::hash<KEY>,
          typename VALUE_TYPE = bsl::pair<const KEY, VALUE> >
class OrderedHashMapWithHistory {
  public:
    // PUBLIC TYPES

    typedef bsls::Types::Int64 TimeType;

  private:
    // PRIVATE TYPES

    struct Value : public VALUE_TYPE {
        TimeType                                 d_time;
        OrderedHashMap_SequentialIterator<Value> d_next;

        /// `d_next` and `d_prev` implement the list of `live`
        /// un-TTL-expired elements.  See 3) in the Component Description.
        OrderedHashMap_SequentialIterator<Value> d_prev;
        bool                                     d_isLive;
        // not confirmed, not TTLed

        // CREATORS
        Value(const VALUE_TYPE& value, TimeType time);
    };

    typedef OrderedHashMap<KEY, VALUE, HASH, Value> ImplType;

  public:
    // PUBLIC TYPES

    typedef typename ImplType::iterator                     gc_iterator;
    typedef typename ImplType::const_iterator               const_gc_iterator;
    typedef OrderedHashMapWithHistory_Iterator<Value>       iterator;
    typedef OrderedHashMapWithHistory_Iterator<const Value> const_iterator;

  private:
    // PRIVATE DATA

    ImplType       d_impl;
    const TimeType d_timeout;

    iterator d_first;
    iterator d_last;
    // 'd_first' and 'd_last' refer to the first and last 'live'
    // un-TTL-expired elements or they both refer to `end()` if there
    // are no 'live' elements.  See 3) in the Component Description.

    size_t d_historySize;  // how many historical (!d_isLive) items

    /// Whether this container has more elements to `gc`.  This flag might be
    /// set or unset during every `gc` call according to this container's
    /// needs.
    bool d_requireGC;

    /// The `now` time of the last GC.  We assume that the current actual time
    /// is no less than this timestamp.
    TimeType d_lastGCTime;

    /// The iterator pointing to the element where garbage collection should
    /// continue once `gc` is called.  According to contract, this iterator
    /// only goes forward.  All the elements passed by this iterator are either
    /// removed or marked for removal, depending on what happened first:
    /// - If the element was not erased by the user before, but its timeout
    /// happened in this container, it is marked for deletion in `gc` and
    /// iterator goes forward.  Next, it is the user's responsibility to call
    /// `erase` on this element to fully remove it.
    /// - If the user removes the element before its timeout happened, the
    /// element becomes `not alive`, but still lives in the history.
    /// Eventually `gc` reaches this element and fully removes it.
    gc_iterator d_gcIt;

    // PRIVATE CLASS METHODS
    static const KEY& get_key(const bsl::pair<const KEY, VALUE>& value)
    {
        return value.first;
    }

    static const KEY& get_key(const KEY& value) { return value; }

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(OrderedHashMapWithHistory,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an empty object which will maintain a history of erase items
    /// for the specified `timeout` interval.  Optionally specify a
    /// `basicAllocator` used to supply memory.  If `timeout` is zero, no
    /// history is maintained.
    explicit OrderedHashMapWithHistory(TimeType          timeout,
                                       bslma::Allocator* basicAllocator = 0);

  private:
    // NOT IMPLEMENTED
    OrderedHashMapWithHistory(const OrderedHashMapWithHistory&)
        BSLS_KEYWORD_DELETED;
    OrderedHashMapWithHistory&
    operator=(const OrderedHashMapWithHistory&) BSLS_KEYWORD_DELETED;

    /// Remove the specified `it` from the list of `live` un-TTL-expired
    /// items accessed by `live` iterators while still keeping the item in
    /// the `d_impl` as a historical record.
    void unlink(iterator it);

  public:
    // PUBLIC MANIPULATORS

    /// Return a mutating iterator referring to the first element in the
    /// container, if any, or one past the end of this container if there
    /// are no elements.
    iterator begin();

    /// Return a mutating iterator referring to one past the end of this
    /// container.
    iterator end();

    /// Return iterator referring to the first element in the entire history
    /// of items, if any, or one past the end of this container if there are
    /// no elements.
    gc_iterator beginGc();

    /// Return iterator referring to one past the end of the entire history
    /// of items, if any, or one past the end of this container if there are
    /// no elements.
    gc_iterator endGc();

    /// Iterate up to the specified `batchSize` of elements (both historical
    /// and live) and erase expired historical records according to the
    /// specified `now` time.  Return `true`, if there are expired items
    /// unprocessed because of the `batchSize` limit.
    bool gc(TimeType now, unsigned batchSize = 0);

    /// Remove all entries from this container.  Clear all history.  Note
    /// that this container will be empty after calling this method, but
    /// allocated memory may be retained for future use.
    void clear();

    /// Remove from this container the `value_type` object at the specified
    /// `position`.  If the time point for the object is known, keep the
    /// item as historical until the time of expiration routine invocation
    /// exceeds its `timePoint` plus `d_timeout`.
    /// The behavior is undefined unless `position` refers to a `value_type`
    /// object in this container.
    void erase(iterator position);
    void erase(iterator position, TimeType now);

    /// Return an iterator providing modifiable access to the `value_type`
    /// object in this container having the specified `key`, if such an
    /// entry exists and it is not historical, and the past-the-end iterator
    /// (`end`) otherwise.
    iterator find(const KEY& key);

    /// Insert the specified `value` along with the specified `timePoint`
    /// into this container if the key (the `first` element) of the
    /// `value_type` object constructed from `value` does not already exist
    /// in this container; otherwise, this method has no effect.  Return a
    /// `pair` whose `first` member is an iterator referring to the
    /// (possibly newly inserted) `value_type` object in this container
    /// whose key is the same as that of `value`, and whose `second` member
    /// is `true` if a new value was inserted, and `false` if the value was
    /// already present.  If the specified `timePoint` is not zero, keep the
    /// history of inserted key until the time of expiration routine
    /// invocation exceeds `timePoint` plus `d_timeout`.  Note that
    /// `timePoint` is ignored if `d_timeout` is zero (i.e., no history will
    /// be maintained).  Also note that this method requires that the types
    /// `KEY` and `VALUE` both be "copy-constructible".
    template <class SOURCE_TYPE>
    bsl::pair<iterator, bool> insert(const SOURCE_TYPE& value,
                                     TimeType           timePoint);

    // ACCESSORS

    /// Return an iterator providing non-modifiable access to the first
    /// `value_type` object in the sequence of `value_type` objects
    /// maintained by this container, or the `end` iterator if this
    /// containeris empty.
    const_iterator begin() const;

    /// Return an iterator providing non-modifiable access to the
    /// past-the-end element in the sequence of `value_type` objects
    /// maintained by this container.
    const_iterator end() const;

    /// Return the number of live (not historical) `value_type` objects
    /// contained within this container having the specified `key`.  Note
    /// that since an ordered hash map maintains unique keys, the returned
    /// value will be either 0 or 1.
    size_t count(const KEY& key) const;

    /// Return `true` if this container contains no live elements , and
    /// `false` otherwise.
    bool empty() const;

    /// Return an iterator providing non-modifiable access to the
    /// `value_type` object in this container having the specified `key`, if
    /// such an entry exists, and the past-the-end iterator (`end`)
    /// otherwise.
    const_iterator find(const KEY& key) const;

    /// Return `true` if an entry for the specified `key` exists or `false`
    /// otherwise.
    bool isInHistory(const KEY& key) const;

    /// Return the number of elements in this container.
    size_t size() const;

    /// Return the number of elements in this container's history.
    size_t historySize() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------------------
// class OrderedHashMapWithHistory_Iterator
// ---------------------------------------

template <class VALUE>
inline OrderedHashMapWithHistory_Iterator<VALUE>::
    OrderedHashMapWithHistory_Iterator(
        const OrderedHashMap_SequentialIterator<VALUE>& baseIterator)
: d_baseIterator(baseIterator)
{
}

template <class VALUE>
inline OrderedHashMapWithHistory_Iterator<
    VALUE>::OrderedHashMapWithHistory_Iterator()
: d_baseIterator()
{
}

template <class VALUE>
inline OrderedHashMapWithHistory_Iterator<
    VALUE>::OrderedHashMapWithHistory_Iterator(const NcIter& other)
: d_baseIterator(other.d_baseIterator)
{
}

// MANIPULATORS

template <class VALUE>
inline OrderedHashMapWithHistory_Iterator<VALUE>&
OrderedHashMapWithHistory_Iterator<VALUE>::operator=(const NcIter& rhs)
{
    d_baseIterator = rhs.d_baseIterator;
    return *this;
}

template <class VALUE>
inline OrderedHashMapWithHistory_Iterator<VALUE>&
OrderedHashMapWithHistory_Iterator<VALUE>::operator++()
{
    d_baseIterator = d_baseIterator->d_next;

    return *this;
}

template <class VALUE>
inline OrderedHashMapWithHistory_Iterator<VALUE>
OrderedHashMapWithHistory_Iterator<VALUE>::operator++(int)
{
    OrderedHashMapWithHistory_Iterator<VALUE> rc(*this);
    d_baseIterator = d_baseIterator->d_next;

    return rc;
}

// ACCESSORS
template <class VALUE>
inline VALUE& OrderedHashMapWithHistory_Iterator<VALUE>::operator*() const
{
    BSLS_ASSERT_SAFE(d_baseIterator->d_isLive);

    return *d_baseIterator;
}

template <class VALUE>
inline VALUE* OrderedHashMapWithHistory_Iterator<VALUE>::operator->() const
{
    BSLS_ASSERT_SAFE(d_baseIterator->d_isLive);

    return d_baseIterator.operator->();
}

// FREE OPERATORS
template <class VALUE1, class VALUE2>
inline bool operator==(const OrderedHashMapWithHistory_Iterator<VALUE1>& lhs,
                       const OrderedHashMapWithHistory_Iterator<VALUE2>& rhs)
{
    return lhs.d_baseIterator == rhs.d_baseIterator;
}

template <class VALUE1, class VALUE2>
inline bool operator!=(const OrderedHashMapWithHistory_Iterator<VALUE1>& lhs,
                       const OrderedHashMapWithHistory_Iterator<VALUE2>& rhs)
{
    return !(lhs == rhs);
}

template <class VALUE>
inline void clean(BSLS_ANNOTATION_UNUSED VALUE& value)
{
    // NOTHING
}

// -----------
// class Value
// -----------

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::Value::Value(
    const VALUE_TYPE& value,
    TimeType          time)
: VALUE_TYPE(value)
, d_time(time)
, d_next()
, d_prev()
, d_isLive(true)
{
    // NOTHING
}

// -------------------------------
// class OrderedHashMapWithHistory
// -------------------------------

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::
    OrderedHashMapWithHistory(TimeType          timeout,
                              bslma::Allocator* basicAllocator)
: d_impl(basicAllocator)
, d_timeout(timeout)
, d_first(d_impl.end())
, d_last(d_impl.end())
, d_historySize(0)
, d_requireGC(false)
, d_lastGCTime(0)
, d_gcIt(endGc())
{
    // NOTHING
}

// PUBLIC MANIPULATORS
template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline
    typename OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::iterator
    OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::begin()
{
    return iterator(d_first);
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline
    typename OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::iterator
    OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::end()
{
    return iterator(d_impl.end());
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline void OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::clear()
{
    d_impl.clear();

    d_first = d_last = end();
    d_gcIt           = endGc();
    d_requireGC      = false;
    d_lastGCTime     = 0;
    d_historySize    = 0;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline void
OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::erase(iterator it)
{
    TimeType   time = it->d_time;
    const KEY& key  = get_key(*it);

    unlink(it);

    if (time) {
        ++d_historySize;
    }
    else {
        if (d_gcIt == it.d_baseIterator) {
            d_gcIt++;
        }
        d_impl.erase(key);
    }
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline void
OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::erase(iterator it,
                                                               TimeType now)
{
    TimeType   time = it->d_time;
    const KEY& key  = get_key(*it);

    unlink(it);

    if (time && now < time) {
        ++d_historySize;
    }
    else {
        if (d_gcIt == it.d_baseIterator) {
            d_gcIt++;
        }
        d_impl.erase(key);
    }
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline void
OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::unlink(iterator it)
{
    BSLS_ASSERT(it->d_isLive);

    if (it == d_first) {
        if (it == d_last) {
            d_last = d_first = end();
        }
        else {
            d_first = iterator(it->d_next);
        }
    }
    else {
        it->d_prev->d_next = it->d_next;

        if (it == d_last) {
            BSLS_ASSERT_SAFE(it->d_next == d_impl.end());

            d_last = iterator(it->d_prev);
        }
        else {
            it->d_next->d_prev = it->d_prev;
        }
    }

    clean(*it);

    it->d_isLive = false;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline
    typename OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::iterator
    OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::find(
        const KEY& key)
{
    gc_iterator it = d_impl.find(key);

    if (it != d_impl.end()) {
        if (!it->d_isLive) {
            it = d_impl.end();
        }
    }

    return iterator(it);
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
template <class SOURCE_TYPE>
inline bsl::pair<
    typename OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::iterator,
    bool>
OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::insert(
    const SOURCE_TYPE& value,
    TimeType           timePoint)
{
    if (d_requireGC) {
        gc(bsl::max(timePoint, d_lastGCTime),
           OrderedHashMapWithHistory_ImpDetails::
               k_INSERT_GC_MESSAGES_BATCH_SIZE);
    }

    bsl::pair<gc_iterator, bool> result = d_impl.insert(
        Value(value, d_timeout ? timePoint + d_timeout : 0));
    // No need to keep track of element's timePoint if the map is not
    // maintaining any history (i.e., if d_timeout == 0).

    gc_iterator& it = result.first;

    if (result.second) {
        if (d_last.d_baseIterator == it) {
            BSLS_ASSERT_SAFE(d_first.d_baseIterator == it);
        }
        else {
            it->d_prev     = d_last.d_baseIterator;
            d_last->d_next = it;
            d_last         = iterator(it);
        }
        it->d_next = d_impl.end();
    }

    return bsl::pair<iterator, bool>(iterator(it), result.second);
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline bool
OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::gc(TimeType now,
                                                            unsigned batchSize)
{
    // Try to advance to either a young item, or to the end of the batch, or to
    // the end of collection.
    // Return 'true' if at the end of the batch to be called as soon as
    // possible.  Otherwise, return 'false' to be called at the next timeout.
    // In either case, resume iteration from where we stopped (or wrap around).
    // 'erase' can set the iterator back to erase item if its expiration time
    // is sooner than the current one.

    d_lastGCTime = now;
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_gcIt == endGc())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        d_gcIt = beginGc();
    }

    while (d_gcIt != endGc() && d_historySize) {
        if (now < d_gcIt->d_time) {
            // next time resume from this item unless some earlier item(s) will
            // get erased in which case 'erase' will move 'd_gcIt' back.
            break;  // BREAK
        }
        gc_iterator it = d_gcIt++;
        if (it->d_isLive) {
            // This item was not erased by the user yet, but its timeout in
            // this container happened.  Mark it for deletion by setting
            // `d_time` to 0, so the next time user calls `erase` on it, it
            // will be fully removed.
            it->d_time = 0;
        }
        else {
            // This item was erased by the user before, and we can fully remove
            // it right here.
            d_impl.erase(it);
            --d_historySize;
        }
        // Meaning, there is no need for 'd_gcIt' to step back.  Only forward.

        if (--batchSize == 0) {
            // remember where we have stopped and resume from there next time
            d_requireGC = true;
            return true;  // RETURN
        }
    }

    d_requireGC = false;
    return false;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline typename OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::
    gc_iterator
    OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::beginGc()
{
    return d_impl.begin();
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline typename OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::
    gc_iterator
    OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::endGc()
{
    return d_impl.end();
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline typename OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::
    const_iterator
    OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::begin() const
{
    return const_iterator(d_first);
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline typename OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::
    const_iterator
    OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::end() const
{
    return const_iterator(d_impl.end());
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline size_t OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::count(
    const KEY& key) const
{
    const_gc_iterator it = d_impl.find(key);

    return it == d_impl.end() ? 0 : it->d_isLive ? 1 : 0;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline bool
OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::empty() const
{
    return size() == 0;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline typename OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::
    const_iterator
    OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::find(
        const KEY& key) const
{
    const_gc_iterator cit = d_impl.find(key);

    if (cit != d_impl.end()) {
        if (!cit->d_isLive) {
            cit = d_impl.end();
        }
    }

    return const_iterator(cit);
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline bool
OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::isInHistory(
    const KEY& key) const
{
    return d_impl.find(key) != d_impl.end();
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline size_t
OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::size() const
{
    return d_impl.size() - d_historySize;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline size_t
OrderedHashMapWithHistory<KEY, VALUE, HASH, VALUE_TYPE>::historySize() const
{
    return d_historySize;
}

}  // close package namespace
}  // close enterprise namespace

#endif
