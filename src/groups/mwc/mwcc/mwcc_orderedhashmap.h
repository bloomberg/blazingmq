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

// mwcc_orderedhashmap.h                                              -*-C++-*-
#ifndef INCLUDED_MWCC_ORDEREDHASHMAP
#define INCLUDED_MWCC_ORDEREDHASHMAP

//@PURPOSE: Provide a hash table with predictive iteration order.
//
//@CLASSES:
//  mwcc::OrderedHashMap : Hash table with predictive iteration order.
//
//@SEE_ALSO: bsl::unordered_map
//
//@DESCRIPTION: 'mwcc::OrderedHashMap' provides an associative container with
// constant time performance for basic operations (insertion, deletion and
// lookup), along with predictive iteration order.  The iteration order is the
// order in which keys are inserted in the container.  Note that this feature
// comes at the cost of additional memory, which is proportional to the number
// of elements in the container.  Specifically, compared to bsl::unordered_map,
// this container has a memory overhead of one pointer per element.
//
// This container maintains a maximum load factor of 1, and hash table is
// rehashed everytime load factor exceeds 1.
//
// An important factor in the performance this container (and any hash table in
// general) is the choice of hash function.  In general, one wants the hash
// function to return uniformly distributed values that can be assigned to
// buckets with few collisions.
//
/// Exception Safety
///----------------
// At this time, this component provides *no* exception safety guarantee.  In
// other words, this component is *not* exception neutral.  If any exception is
// thrown during the invocation of a method on the object, the object is left
// in an inconsistent state, and using the object from that point forward will
// cause undefined behavior.
//
/// Behavior of insert() routine
///----------------------------
// An important non-standard feature of 'insert()' routine of this container is
// that the newly inserted element is always constructed such that
// 'container.end()' before the 'insert()' operation becomes the iterator of
// the newly inserted element.  Following snippet explains it:
//
//..
// typedef mwcc::OrderedHashMap<int, int> MyMapType;
// typedef MyMapType::iterator            IterType;
//
// IterType                  endIt    = map.end();
// bsl::pair<IterType, bool> insertRc = map.insert(bsl::make_pair(1, 10));
//
// BSLS_ASSERT(endIt == insertRc.first)
// BSLS_ASSERT(1     == endIt->first);
// BSLS_ASSERT(10    == endIt->second);
//..
//
// This feature can be very useful when it is desired to remember one's
// position in the container.  So if container's end is encountered, and
// elements are added after that, one can simply reuse previous end iterator to
// retrieve the first newly added element without maintaining any additional
// state or performing additional lookups.
//
// The 'rinsert()' routine does not provide this feature, as the element is
// always at the beginning of the underlying list, and the end() iterator
// remains unaffected.
//
/// Iterator, pointer and reference invalidation
///--------------------------------------------
// No method of 'OrderedHashMap' invalidates a pointer or reference to an
// element in the set, unless it also erases that element, such as any 'erase'
// overload, 'clear', or the destructor (that erases all elements).  Pointers
// and references are stable through a rehash.
//
// A non-standard feature of this container, however, is that iterators are
// *not* invalidated even if the underlying hash table is rehashed.  This is
// made possible by the fact the iterators refer to the underlying sequential
// list, which remains unchanged during rehashing.
//
/// Thread Safety
///-------------
// Not thread safe.
//
/// Usage
///-----
// TBD
//..
//..

// MWC
#include <mwcu_safecast.h>

// BDE
#include <bdlma_pool.h>
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bsl_stdexcept.h>
#include <bsl_utility.h>
#include <bslalg_scalarprimitives.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmf_removecvq.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {

namespace mwcc {

// FORWARD DECLARATION
template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
class OrderedHashMap;

// ================================
// struct OrderedHashMap_ImpDetails
// ================================

/// PRIVATE CLASS. For use only by `mwcc::OrderedHashMap` implementation.
struct OrderedHashMap_ImpDetails {
    /// Return the next prime number greater-than or equal to the specified
    /// `n` in the increasing sequence of primes chosen to disperse hash
    /// codes across buckets as uniformly as possible.  Return zero if `n`
    /// is greater than the last prime number in the sequence.  Note that,
    /// typically, prime numbers in the sequence have increasing values that
    /// reflect a growth factor (e.g., each value in the sequence may be,
    /// approximately, two times the preceding value).  Also note that all
    /// the returned values are representable within a 32-bit signed integer.
    static size_t nextPrime(size_t n);
};

// =========================
// class OrderedHashMap_Link
// =========================

/// PRIVATE CLASS. For use only by `mwcc::OrderedHashMap` implementation.
/// Each node is part of two lists, the sequential list (which provides
/// insertion order iteration feature), and the bucket list (which provides
/// open hashing feature in case of collision).
class OrderedHashMap_Link {
  private:
    // DATA
    OrderedHashMap_Link* d_nextInList_p;

    OrderedHashMap_Link* d_prevInList_p;

    OrderedHashMap_Link* d_nextInBucket_p;

  public:
    // CREATORS
    ~OrderedHashMap_Link();

    // MANIPULATORS

    /// Set the specified `next` link as the next link in the sequential
    /// list.
    void setNextInList(OrderedHashMap_Link* next);

    /// Set the specified `prev` link as the previous link in the sequential
    /// list.
    void setPrevInList(OrderedHashMap_Link* prev);

    /// Set the specified `next` link as the next link in the bucket list.
    void setNextInBucket(OrderedHashMap_Link* next);

    /// Set all links to zero.
    void reset();

    // ACCESSORS
    OrderedHashMap_Link* nextInList() const;
    OrderedHashMap_Link* prevInList() const;

    /// Return the corresponding link.
    OrderedHashMap_Link* nextInBucket() const;
};

// =========================
// class OrderedHashMap_Node
// =========================

/// PRIVATE CLASS TEMPLATE. For use only by `mwcc::OrderedHashMap`
/// implementation.
template <class VALUE>
class OrderedHashMap_Node : public OrderedHashMap_Link {
  private:
    // DATA
    VALUE d_value;

  private:
    // NOT IMPLEMENTED
    OrderedHashMap_Node();
    OrderedHashMap_Node(const OrderedHashMap_Node&);
    OrderedHashMap_Node& operator=(const OrderedHashMap_Node&);
    ~OrderedHashMap_Node();

  public:
    // MANIPULATORS

    /// Return a reference providing modifiable access to the `value` held
    /// by this object.
    VALUE& value();

    // ACCESSORS

    /// Return a reference providing non-modifiable access to the `value`
    /// held by this object.
    const VALUE& value() const;
};

// =============
// struct OrderedHashMap_Bucket
// =============

/// PRIVATE CLASS. For use only by `mwcc::OrderedHashMap` implementation.
struct OrderedHashMap_Bucket {
    typedef OrderedHashMap_Link Link;

  public:
    // PUBLIC DATA
    Link* d_first_p;

    Link* d_last_p;

  public:
    // CREATORS

    /// Create an instance with first and last links set to zero.
    OrderedHashMap_Bucket();

    /// Destroy this instance.
    ~OrderedHashMap_Bucket();
};

// =======================================
// class OrderedHashMap_SequentialIterator
// =======================================

/// PRIVATE CLASS TEMPLATE. For use only by `mwcc::OrderedHashMap`
/// implementation.
template <class VALUE>
class OrderedHashMap_SequentialIterator {
  private:
    // PRIVATE TYPES
    typedef typename bsl::remove_cv<VALUE>::type NcType;

    typedef OrderedHashMap_SequentialIterator<NcType> NcIter;

    typedef OrderedHashMap_Link Link;

    typedef OrderedHashMap_Node<VALUE> Node;

    // FRIENDS
    template <class LHM_KEY,
              class LHM_VALUE,
              class LHM_HASH,
              typename LHM_VALUE_TYPE>
    friend class OrderedHashMap;

    friend class OrderedHashMap_SequentialIterator<const VALUE>;

    template <class VALUE1, class VALUE2>
    friend bool operator==(const OrderedHashMap_SequentialIterator<VALUE1>&,
                           const OrderedHashMap_SequentialIterator<VALUE2>&);

    // DATA
    Link* d_link_p;

  private:
    // PRIVATE CREATORS

    /// Create an iterator instance pointing to the specified `link`.
    explicit OrderedHashMap_SequentialIterator(Link* link);

  public:
    // CREATORS

    /// Create a singular iterator (i.e., one that cannot be incremented,
    /// decremented, or dereferenced.
    OrderedHashMap_SequentialIterator();

    /// Create an iterator to `VALUE` from the corresponding iterator to
    /// non-const `VALUE`.  If `VALUE` is not const-qualified, then this
    /// constructor becomes the copy constructor.  Otherwise, the copy
    /// constructor is implicitly generated.
    OrderedHashMap_SequentialIterator(const NcIter& other);

    // MANIPULATORS

    /// Advance this iterator to the next element in the sequential list and
    /// return its new value.  The behavior is undefined unless this
    /// iterator is in the range `[begin() .. end())` (i.e., the iterator is
    /// not singular, is not `end()`, and has not been invalidated).
    OrderedHashMap_SequentialIterator& operator++();

    /// Move this iterator to the previous element in the sequential list
    /// and return its new value.  The behavior is undefined unless this
    /// iterator is in the range `( begin(), end() ]` (i.e., the iterator is
    /// not singular, is not `begin()`, and has not been invalidated).
    OrderedHashMap_SequentialIterator& operator--();

    /// Advance this iterator to the next element in the sequential list and
    /// return its previous value.  The behavior is undefined unless this
    /// iterator is in the range `[begin() .. end())` (i.e., the iterator is
    /// not singular, is not `end()`, and has not been invalidated).
    OrderedHashMap_SequentialIterator operator++(int);

    /// Move this iterator to the previous element in the sequential list
    /// and return its previous value.  The behavior is undefined unless
    /// this iterator is in the range `( begin(), end() ]` (i.e., the
    /// iterator is not singular, is not `begin()`, and has not been
    /// invalidated).
    OrderedHashMap_SequentialIterator operator--(int);

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
bool operator==(const OrderedHashMap_SequentialIterator<VALUE1>& lhs,
                const OrderedHashMap_SequentialIterator<VALUE2>& rhs);

/// Return `true` if the specified iterators `lhs` and `rhs` do not have the
/// same value and `false` otherwise.  Two iterators have the same value if
/// both refer to the same element of the same list or both are the end()
/// iterator of the same list.  The return value is undefined unless both
/// `lhs` and `rhs` are non-singular.
template <class VALUE1, class VALUE2>
bool operator!=(const OrderedHashMap_SequentialIterator<VALUE1>& lhs,
                const OrderedHashMap_SequentialIterator<VALUE2>& rhs);

// ===================================
// class OrderedHashMap_BucketIterator
// ===================================

/// PRIVATE CLASS TEMPLATE. For use only by `mwcc::OrderedHashMap`
/// implementation.
template <class VALUE>
class OrderedHashMap_BucketIterator {
    // FRIENDS
    template <class LHM_KEY,
              class LHM_VALUE,
              class LHM_HASH,
              typename LHM_VALUE_TYPE>
    friend class OrderedHashMap;

    friend class OrderedHashMap_BucketIterator<const VALUE>;

    template <class VALUE1, class VALUE2>
    friend bool operator==(const OrderedHashMap_BucketIterator<VALUE1>&,
                           const OrderedHashMap_BucketIterator<VALUE2>&);

  private:
    // PRIVATE TYPES
    typedef typename bsl::remove_cv<VALUE>::type NcType;

    typedef OrderedHashMap_BucketIterator<NcType> NcIter;

    typedef OrderedHashMap_Link Link;

    typedef OrderedHashMap_Node<VALUE> Node;

    typedef OrderedHashMap_Bucket Bucket;

    // DATA
    Bucket* d_bucket_p;

    Link* d_link_p;

  private:
    // PRIVATE CREATORS

    /// Create an iterator referring to the specified `bucket`, initially
    /// pointing to the first node in that `bucket`, or a past-the-end value
    /// if the `bucket` is empty.
    explicit OrderedHashMap_BucketIterator(Bucket* bucket);

    /// Create an iterator referring to the specified `bucket`, initially
    /// pointing to the specified `link` in that bucket.  The behavior is
    /// undefined unless `link` is part of `bucket`, or `link` is 0.
    OrderedHashMap_BucketIterator(Bucket* bucket, Link* link);

  public:
    // CREATORS

    /// Create a default-constructed iterator referring to an empty list of
    /// nodes.  All default-constructed iterators are non-dereferenceable
    /// and refer to the same empty list.
    OrderedHashMap_BucketIterator();

    /// Create an iterator at the same position as the specified `original`
    /// iterator.  Note that this constructor enables converting from
    /// modifiable to `const` iterator types.
    OrderedHashMap_BucketIterator(const NcIter& other);

    // MANIPULATORS

    /// Move this iterator to the next element in the hash table bucket and
    /// return a reference providing modifiable access to this iterator.
    /// The behavior is undefined unless the iterator refers to a valid (not
    /// yet erased) element in a bucket.  Note that this iterator is
    /// invalidated when the underlying hash table is rehashed.
    OrderedHashMap_BucketIterator& operator++();

    // ACCESSORS

    /// Return a reference providing modifiable access to the element (of
    /// the template parameter `VALUE`) at which this iterator is
    /// positioned.  The behavior is undefined unless the iterator refers to
    /// a valid (not yet erased) element in a hash table bucket.  Note that
    /// this iterator is invalidated when the underlying hash table is
    /// rehashed.
    VALUE& operator*() const;

    /// Return the address of the element (of the template parameter
    /// `VALUE`) at which this iterator is positioned.  The behavior is
    /// undefined unless the iterator refers to a valid (not yet erased)
    /// element a hash table bucket.  Note that this iterator is invalidated
    /// when the underlying hash table is rehashed.
    VALUE* operator->() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and the specified `rhs` iterators
/// have the same value and `false` otherwise.  Two iterators have the same
/// value if they refer to the same element in the same hash table, or if
/// both iterators are positioned after the end of a hash table bucket.
template <class VALUE1, class VALUE2>
bool operator==(const OrderedHashMap_BucketIterator<VALUE1>& lhs,
                const OrderedHashMap_BucketIterator<VALUE2>& rhs);

/// Return `true` if the specified `lhs` and the specified `rhs` iterators
/// do not have the same value and `false` otherwise.  Two iterators do not
/// have the same value if they refer to the different elements in the same
/// hash table, or if either (but not both) of the iterators are positioned
/// after the end of a hash table bucket.
template <class VALUE1, class VALUE2>
bool operator!=(const OrderedHashMap_BucketIterator<VALUE1>& lhs,
                const OrderedHashMap_BucketIterator<VALUE2>& rhs);

// ====================
// class OrderedHashMap
// ====================

/// This class provides a hash table with predictive iteration order.
template <class KEY,
          class VALUE,
          class HASH       = bsl::hash<KEY>,
          class VALUE_TYPE = bsl::pair<const KEY, VALUE> >
class OrderedHashMap {
  private:
    // PRIVATE TYPES
    typedef VALUE_TYPE ValueType;

    typedef OrderedHashMap_ImpDetails      ImpDetails;
    typedef OrderedHashMap_Link            Link;
    typedef OrderedHashMap_Node<ValueType> Node;
    typedef OrderedHashMap_Bucket          Bucket;

    enum {
        e_NODE_SIZE = sizeof(Node)  // For NodePool
        ,
        e_INITIAL_NUM_BUCKET = 13  // Must be prime
    };

  public:
    // TYPES
    typedef KEY key_type;

    typedef ValueType value_type;

    typedef bslma::Allocator* allocator_type;

    typedef HASH hasher;

    typedef OrderedHashMap_SequentialIterator<value_type> iterator;

    typedef OrderedHashMap_SequentialIterator<const value_type> const_iterator;

    typedef OrderedHashMap_BucketIterator<value_type> local_iterator;

    typedef OrderedHashMap_BucketIterator<const value_type>
        const_local_iterator;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    bdlma::Pool d_nodePool;  // Owns all nodes

    Bucket* d_bucketArray_p;

    Link* d_sentinel_p;  // end()

    size_t d_bucketArraySize;

    size_t d_numElements;

  private:
    // PRIVATE ACCESSORS

    /// Return the address of the bucket for the specified `key`.
    Bucket* getBucketForKey(const key_type& key) const;

    /// Return the node having the specified `key` in the specified
    /// `bucket`, or zero if no node with `key` exists in the `bucket`.
    Link* findKeyInBucket(const key_type& key, Bucket* bucket) const;

    // PRIVATE MANIPULATORS

    /// Create the hash table with zero elements.
    void initialize();

    /// Create and return a new node which is same as the existing sentinel.
    Link* createNewSentinel();

    /// Construct a node having the specified `value` at the node pointed by
    /// the specified `link`.
    void constructNode(Link* link, const value_type& value);

    /// Create a node having the specified `value`, and return it.
    Link* createNode(const value_type& value);

    /// Append the node pointed by the specified `link` at the specified
    /// `bucket`.
    void appendNodeToBucket(Link* link, Bucket* bucket);

    /// Make the node pointed by the specified `link` as the new sentinel.
    void makeSentinel(Link* link);

    /// Push the node pointed by the specified `link` to the front of the
    /// sequential list.
    void pushFront(Link* link);

    /// Rehash the hash table if its load factor is greater than 1.0, and
    /// return true.  Return false otherwise.  Note that the iterators are
    /// not invalidated by rehashing.
    bool rehashIfNeeded();

    // PRIVATE CLASS METHODS
    static const key_type& get_key(const bsl::pair<const KEY, VALUE>& value)
    {
        return value.first;
    }

    static const key_type& get_key(const KEY& value) { return value; }

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(OrderedHashMap, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an empty `OrderedHashMap` object with a maximum load factor
    /// of 1.0.  Optionally specify a `basicAllocator` used to supply
    /// memory.  Use a default constructed object of the (template
    /// parameter) type `HASHER` to organize elements in the table.
    explicit OrderedHashMap(bslma::Allocator* basicAllocator = 0);

    /// Create an empty `OrderedHashMap` which will initially have at least
    /// the specified `initialNumBuckets` and a maximum load factor of 1.0.
    /// Optionally specify a `basicAllocator` used to supply memory.  The
    /// behavior is undefined unless `0 < initialNumBuckets`.  Note that
    /// more than `initialNumBuckets` buckets may be created in order to
    /// preserve the bucket allocation strategy of the hash-table (but never
    /// fewer).
    explicit OrderedHashMap(int               initialNumBuckets,
                            bslma::Allocator* basicAllocator = 0);

    /// Create an `OrderedHashMap` having the same value and maximum load
    /// factor as the specified `other`, that will use the optionally
    /// specified `basicAllocator` to supply memory.
    OrderedHashMap(const OrderedHashMap& other,
                   bslma::Allocator*     basicAllocator = 0);

    /// Destroy this object and each of its elements.
    ~OrderedHashMap();

    // MANIPULATORS

    /// Assign to this object the value of the specified `other` object.
    OrderedHashMap& operator=(const OrderedHashMap& other);

    /// Return a mutating iterator referring to the first element in the
    /// container, if any, or one past the end of this container if there
    /// are no elements.
    iterator begin();

    /// Return a mutating iterator referring to one past the end of this
    /// container.
    iterator end();

    /// Return a local iterator providing modifiable access to the first
    /// `value_type` object in the sequence of `value_type` objects of the
    /// bucket having the specified `index` in the array of buckets
    /// maintained by this container, or the `end(index)` iterator if the
    /// bucket is empty.  The behavior is undefined unless 'index <
    /// bucket_count()'.
    local_iterator begin(size_t index);

    /// Return a local iterator providing modifiable access to the
    /// past-the-end element in the sequence of `value_type` objects of the
    /// bucket having the specified `index` in the array of buckets
    /// maintained by this container.  The behavior is undefined unless
    /// `index < bucket_count()`.
    local_iterator end(size_t index);

    /// Remove all entries from this container.  Note that this container
    /// will be empty after calling this method, but allocated memory may be
    /// retained for future use.
    void clear();

    /// Remove from this container the `value_type` object at the specified
    /// `position`, and return an iterator referring to the element
    /// immediately following the removed element, or to the past-the-end
    /// position if the removed element was the last element in the sequence
    /// of elements maintained by this container.  The behavior is
    /// undefined unless `position` refers to a `value_type` object in this
    /// container.
    iterator erase(const_iterator position);

    /// Remove from this container the `value_type` object having the
    /// specified `key`, if it exists, and return 1; otherwise (there is no
    /// `value_type` object having `key` in this container) return 0 with no
    /// other effect.
    size_t erase(const key_type& key);

    /// Remove from this container the sequence of elements starting at the
    /// specified `first` position and ending before the specified `last`
    /// position, and return an iterator providing modifiable access to the
    /// element immediately following the last removed element, or the
    /// position returned by the method `end` if the removed elements were
    /// last in the sequence.  The behavior is undefined unless `first` is
    /// an iterator in the range `[begin() .. end()]` (both endpoints
    /// included) and `last` is an iterator in the range
    /// `[first .. end()]` (both endpoints included).
    const_iterator erase(const_iterator first, const_iterator last);

    /// Return an iterator providing modifiable access to the `value_type`
    /// object in this container having the specified `key`, if such an
    /// entry exists, and the past-the-end iterator (`end`) otherwise.
    iterator find(const key_type& key);

    /// Insert the specified `value` into this container if the key (the
    /// `first` element) of a `value_type` object constructed from `value`
    /// does not already exist in this container; otherwise, this method
    /// has no effect (a `value_type` object having the same key as the
    /// converted `value` already exists in this container) .  Return a
    /// `pair` whose `first` member is an iterator referring to the
    /// (possibly newly inserted) `value_type` object in this container
    /// whose key is the same as that of `value`, and whose `second` member
    /// is `true` if a new value was inserted, and `false` if the value was
    /// already present.  Note that this method requires that the (template
    /// parameter) types `KEY` and `VALUE` both be "copy-constructible".
    template <class SOURCE_TYPE>
    bsl::pair<iterator, bool> insert(const SOURCE_TYPE& value);

    /// Insert the specified `value` into this container at the beginning of
    /// the underlying sequential list if the key (the `first` element) of a
    /// `value_type` object constructed from `value` does not already exist
    /// in this container; otherwise, this method has no effect (a
    /// `value_type` object having the same key as the converted `value`
    /// already exists in this container) .  Return a `pair` whose `first`
    /// member is an iterator referring to the (possibly newly inserted)
    /// `value_type` object in this container whose key is the same as that
    /// of `value`, and whose `second` member is `true` if a new value was
    /// inserted, and `false` if the value was already present.  Note that
    /// this method requires that the (template parameter) types `KEY` and
    /// `VALUE` both be "copy-constructible".
    template <class SOURCE_TYPE>
    bsl::pair<iterator, bool> rinsert(const SOURCE_TYPE& value);

    // void reserve(int numElements);
    // Increase the number of buckets of this set to a quantity such that
    // the ratio between the specified 'numElements' and this quantity does
    // not exceed 'max_load_factor'.  Note that this guarantees that, after
    // the reserve, elements can be inserted to grow the container to
    // 'size() == numElements' without rehashing.  Also note that memory
    // allocations may still occur when growing the container to 'size() ==
    // numElements'.  Also note that this operation has no effect if
    // 'numElements <= size()'.

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

    /// Return a local iterator providing non-modifiable access to the first
    /// `value_type` object in the sequence of `value_type` objects of the
    /// bucket having the specified `index` in the array of buckets
    /// maintained by this container, or the `end(index)` iterator if the
    /// bucket is empty.  The behavior is undefined unless 'index <
    /// bucket_count()'.
    const_local_iterator begin(size_t index) const;

    /// Return a local iterator providing non-modifiable access to the
    /// past-the-end element in the sequence of `value_type` objects of the
    /// bucket having the specified `index` in the array of buckets
    /// maintained by this container.  The behavior is undefined unless
    /// `index < bucket_count()`.
    const_local_iterator end(size_t index) const;

    /// Return the index of the bucket, in the array of buckets maintained
    /// by this container, where values having the specified `key` would be
    /// inserted.
    size_t bucket(const key_type& key) const;

    /// Return the number of buckets in the array of buckets maintained by
    /// this container.
    size_t bucket_count() const;

    /// Return the number of `value_type` objects contained within this
    /// container having the specified `key`.  Note that since an ordered
    /// hash map maintains unique keys, the returned value will be either 0
    /// or 1.
    size_t count(const key_type& key) const;

    /// Return `true` if this container contains no elements, and `false`
    /// otherwise.
    bool empty() const;

    /// Return an iterator providing non-modifiable access to the
    /// `value_type` object in this container having the specified `key`, if
    /// such an entry exists, and the past-the-end iterator (`end`)
    /// otherwise.
    const_iterator find(const key_type& key) const;

    /// Return the number of elements in this container.
    size_t size() const;

    /// Return the current ratio between the `size` of this container and
    /// the number of buckets.  The load factor is a measure of how full the
    /// container is, and a higher load factor typically leads to an
    /// increased number of collisions, thus resulting in a loss of
    /// performance.
    double load_factor() const;

    /// Return the allocator associated with this object.
    allocator_type get_allocator() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------------
// class OrderedHashMap_Link
// -------------------------

// CREATORS
inline OrderedHashMap_Link::~OrderedHashMap_Link()
{
    d_nextInList_p = d_prevInList_p = d_nextInBucket_p = 0;
}

// MANIPULATORS
inline void OrderedHashMap_Link::setNextInList(OrderedHashMap_Link* next)
{
    d_nextInList_p = next;
}

inline void OrderedHashMap_Link::setPrevInList(OrderedHashMap_Link* prev)
{
    d_prevInList_p = prev;
}

inline void OrderedHashMap_Link::setNextInBucket(OrderedHashMap_Link* next)
{
    d_nextInBucket_p = next;
}

inline void OrderedHashMap_Link::reset()
{
    d_nextInList_p   = 0;
    d_prevInList_p   = 0;
    d_nextInBucket_p = 0;
}

// ACCESSORS
inline OrderedHashMap_Link* OrderedHashMap_Link::nextInList() const
{
    return d_nextInList_p;
}

inline OrderedHashMap_Link* OrderedHashMap_Link::prevInList() const
{
    return d_prevInList_p;
}

inline OrderedHashMap_Link* OrderedHashMap_Link::nextInBucket() const
{
    return d_nextInBucket_p;
}

// -------------------------
// class OrderedHashMap_Node
// -------------------------

template <class VALUE>
inline VALUE& OrderedHashMap_Node<VALUE>::value()
{
    return d_value;
}

template <class VALUE>
inline const VALUE& OrderedHashMap_Node<VALUE>::value() const
{
    return d_value;
}

// ----------------------------
// struct OrderedHashMap_Bucket
// ----------------------------

inline OrderedHashMap_Bucket::OrderedHashMap_Bucket()
: d_first_p(0)
, d_last_p(0)
{
}

inline OrderedHashMap_Bucket::~OrderedHashMap_Bucket()
{
    d_first_p = d_last_p = 0;
}

// ---------------------------------------
// class OrderedHashMap_SequentialIterator
// ---------------------------------------

template <class VALUE>
inline OrderedHashMap_SequentialIterator<
    VALUE>::OrderedHashMap_SequentialIterator(Link* link)
: d_link_p(link)
{
}

template <class VALUE>
inline OrderedHashMap_SequentialIterator<
    VALUE>::OrderedHashMap_SequentialIterator()
: d_link_p(0)
{
}

template <class VALUE>
inline OrderedHashMap_SequentialIterator<
    VALUE>::OrderedHashMap_SequentialIterator(const NcIter& other)
: d_link_p(other.d_link_p)
{
}

// MANIPULATORS
template <class VALUE>
inline OrderedHashMap_SequentialIterator<VALUE>&
OrderedHashMap_SequentialIterator<VALUE>::operator++()
{
    BSLS_ASSERT_SAFE(d_link_p);
    d_link_p = d_link_p->nextInList();
    return *this;
}

template <class VALUE>
inline OrderedHashMap_SequentialIterator<VALUE>&
OrderedHashMap_SequentialIterator<VALUE>::operator--()
{
    BSLS_ASSERT_SAFE(d_link_p);
    d_link_p = d_link_p->prevInList();
    return *this;
}

template <class VALUE>
inline OrderedHashMap_SequentialIterator<VALUE>
OrderedHashMap_SequentialIterator<VALUE>::operator++(int)
{
    BSLS_ASSERT_SAFE(d_link_p);
    OrderedHashMap_SequentialIterator<VALUE> rc(d_link_p);
    d_link_p = d_link_p->nextInList();
    return rc;
}

template <class VALUE>
inline OrderedHashMap_SequentialIterator<VALUE>
OrderedHashMap_SequentialIterator<VALUE>::operator--(int)
{
    BSLS_ASSERT_SAFE(d_link_p);
    OrderedHashMap_SequentialIterator<VALUE> rc(d_link_p);
    d_link_p = d_link_p->prevInList();
    return rc;
}

// ACCESSORS
template <class VALUE>
inline VALUE& OrderedHashMap_SequentialIterator<VALUE>::operator*() const
{
    BSLS_ASSERT_SAFE(d_link_p);
    Node* node = static_cast<Node*>(d_link_p);
    return node->value();
}

template <class VALUE>
inline VALUE* OrderedHashMap_SequentialIterator<VALUE>::operator->() const
{
    BSLS_ASSERT_SAFE(d_link_p);
    Node* node = static_cast<Node*>(d_link_p);
    return &(node->value());
}

// FREE OPERATORS
template <class VALUE1, class VALUE2>
inline bool operator==(const OrderedHashMap_SequentialIterator<VALUE1>& lhs,
                       const OrderedHashMap_SequentialIterator<VALUE2>& rhs)
{
    return lhs.d_link_p == rhs.d_link_p;
}

template <class VALUE1, class VALUE2>
inline bool operator!=(const OrderedHashMap_SequentialIterator<VALUE1>& lhs,
                       const OrderedHashMap_SequentialIterator<VALUE2>& rhs)
{
    return !(lhs == rhs);
}

// -----------------------------------
// class OrderedHashMap_BucketIterator
// -----------------------------------

template <class VALUE>
inline OrderedHashMap_BucketIterator<VALUE>::OrderedHashMap_BucketIterator(
    Bucket* bucket)
: d_bucket_p(bucket)
, d_link_p(bucket ? bucket->d_first_p : 0)
{
}

template <class VALUE>
inline OrderedHashMap_BucketIterator<VALUE>::OrderedHashMap_BucketIterator(
    Bucket* bucket,
    Link*   link)
: d_bucket_p(bucket)
, d_link_p(link)
{
}

template <class VALUE>
inline OrderedHashMap_BucketIterator<VALUE>::OrderedHashMap_BucketIterator()
: d_bucket_p(0)
, d_link_p(0)
{
}

template <class VALUE>
inline OrderedHashMap_BucketIterator<VALUE>::OrderedHashMap_BucketIterator(
    const NcIter& other)
: d_bucket_p(other.d_bucket_p)
, d_link_p(other.d_link_p)
{
}

// MANIPULATORS
template <class VALUE>
inline OrderedHashMap_BucketIterator<VALUE>&
OrderedHashMap_BucketIterator<VALUE>::operator++()
{
    BSLS_ASSERT_SAFE(d_link_p);

    if (d_bucket_p->d_last_p == d_link_p) {
        d_link_p = 0;
    }
    else {
        d_link_p = d_link_p->nextInBucket();
    }

    return *this;
}

// ACCESSORS
template <class VALUE>
inline VALUE& OrderedHashMap_BucketIterator<VALUE>::operator*() const
{
    BSLS_ASSERT_SAFE(d_link_p);
    Node* node = static_cast<Node*>(d_link_p);
    return node->value();
}

template <class VALUE>
inline VALUE* OrderedHashMap_BucketIterator<VALUE>::operator->() const
{
    BSLS_ASSERT_SAFE(d_link_p);
    Node* node = static_cast<Node*>(d_link_p);
    return &(node->value());
}

// FREE OPERATORS
template <class VALUE1, class VALUE2>
inline bool operator==(const OrderedHashMap_BucketIterator<VALUE1>& lhs,
                       const OrderedHashMap_BucketIterator<VALUE2>& rhs)
{
    return lhs.d_bucket_p == rhs.d_bucket_p && lhs.d_link_p == rhs.d_link_p;
}

template <class VALUE1, class VALUE2>
inline bool operator!=(const OrderedHashMap_BucketIterator<VALUE1>& lhs,
                       const OrderedHashMap_BucketIterator<VALUE2>& rhs)
{
    return !(lhs == rhs);
}

// --------------------
// class OrderedHashMap
// --------------------

// PRIVATE ACCESSORS
template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline OrderedHashMap_Bucket*
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::getBucketForKey(
    const key_type& key) const
{
    return d_bucketArray_p + bucket(key);
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline OrderedHashMap_Link*
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::findKeyInBucket(
    const key_type& key,
    Bucket*         bucket) const
{
    BSLS_ASSERT_SAFE(bucket);

    for (Link* link = bucket->d_first_p; link != 0;
         link       = link->nextInBucket()) {
        Node* cursor = static_cast<Node*>(link);

        if (get_key(cursor->value()) == key) {
            return link;  // RETURN
        }
    }

    return 0;
}

// PRIVATE MANIPULATORS
template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline void OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::initialize()
{
    d_bucketArray_p = static_cast<Bucket*>(
        d_allocator_p->allocate(sizeof(Bucket) * d_bucketArraySize));

    bsl::fill_n(d_bucketArray_p, d_bucketArraySize, Bucket());
    d_nodePool.reserveCapacity(static_cast<int>(d_bucketArraySize));
    d_sentinel_p = static_cast<Link*>(d_nodePool.allocate());
    new (d_sentinel_p) Link();
    d_sentinel_p->reset();

    // Loop the sentinel.

    d_sentinel_p->setPrevInList(d_sentinel_p);
    d_sentinel_p->setNextInList(d_sentinel_p);
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline OrderedHashMap_Link*
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::createNewSentinel()
{
    Link* link = static_cast<Link*>(d_nodePool.allocate());
    *link      = *d_sentinel_p;
    return link;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline void OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::constructNode(
    Link*             link,
    const value_type& value)
{
    typedef typename bsl::remove_cv<value_type>::type NcValueType;

    OrderedHashMap_Node<NcValueType>* node =
        static_cast<OrderedHashMap_Node<NcValueType>*>(link);

    bslalg::ScalarPrimitives::copyConstruct(&(node->value()),
                                            value,
                                            d_allocator_p);
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline OrderedHashMap_Link*
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::createNode(
    const value_type& value)
{
    Link* link = static_cast<Link*>(d_nodePool.allocate());
    constructNode(link, value);
    return link;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline void OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::appendNodeToBucket(
    Link*   link,
    Bucket* bucket)
{
    link->setNextInBucket(0);

    // Add to end of bucket.

    if (0 == bucket->d_last_p) {
        BSLS_ASSERT_SAFE(0 == bucket->d_first_p);
        bucket->d_first_p = bucket->d_last_p = link;
    }
    else {
        BSLS_ASSERT_SAFE(bucket->d_first_p);
        bucket->d_last_p->setNextInBucket(link);
        bucket->d_last_p = link;
    }
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline void
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::makeSentinel(Link* link)
{
    link->setPrevInList(d_sentinel_p);
    link->setNextInList(d_sentinel_p->nextInList());
    d_sentinel_p->nextInList()->setPrevInList(link);
    d_sentinel_p->setNextInList(link);
    d_sentinel_p = link;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline void OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::pushFront(Link* link)
{
    link->setPrevInList(d_sentinel_p);
    link->setNextInList(d_sentinel_p->nextInList());

    if (d_sentinel_p->nextInList() != d_sentinel_p) {
        // head != 0. Make 'link' existing head's prev.
        d_sentinel_p->nextInList()->setPrevInList(link);
    }
    d_sentinel_p->setNextInList(link);  // set new head

    // Make 'link' tail if existing tail == 0
    if (d_sentinel_p->prevInList() == d_sentinel_p) {
        // tail == 0
        d_sentinel_p->setPrevInList(link);
    }
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
bool OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::rehashIfNeeded()
{
    if (1.0 < (static_cast<double>(d_numElements) /
               static_cast<double>(d_bucketArraySize))) {
        // Calculate next size for d_bucketArraySize.

        size_t oldBucketArraySize = d_bucketArraySize;
        d_bucketArraySize         = ImpDetails::nextPrime(d_numElements);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(0 == d_bucketArraySize)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            throw bsl::runtime_error("HashTable ran out of prime numbers");
        }

        // Reserve in nodepool.

        if (d_bucketArraySize > d_numElements) {
            d_nodePool.reserveCapacity(
                static_cast<int>(d_bucketArraySize - d_numElements));
        }

        // Destroy & deallocate old bucket array.

        for (size_t i = 0; i < oldBucketArraySize; ++i) {
            Bucket* bucket = static_cast<Bucket*>(d_bucketArray_p + i);
            bucket->~Bucket();
        }
        d_allocator_p->deallocate(d_bucketArray_p);

        // Allocate new bucket array of new size.

        d_bucketArray_p = static_cast<Bucket*>(
            d_allocator_p->allocate(sizeof(Bucket) * d_bucketArraySize));
        bsl::fill_n(d_bucketArray_p, d_bucketArraySize, Bucket());

        // For each key in list, rehash & insert in new bucketArray.

        size_t numRehashed = 0;
        for (Link* link = d_sentinel_p->nextInList(); link != d_sentinel_p;
             link       = link->nextInList()) {
            Node*   node   = static_cast<Node*>(link);
            Bucket* bucket = getBucketForKey(get_key(node->value()));
            appendNodeToBucket(link, bucket);
            ++numRehashed;
        }

        BSLS_ASSERT_SAFE(numRehashed == d_numElements);
        static_cast<void>(numRehashed);  // suppress compiler warning

        return true;  // RETURN
    }

    return false;
}

// CREATORS
template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::OrderedHashMap(
    bslma::Allocator* basicAllocator)
: d_allocator_p(bslma::Default::allocator(basicAllocator))
, d_nodePool(e_NODE_SIZE, basicAllocator)
, d_bucketArray_p(0)
, d_sentinel_p(0)
, d_bucketArraySize(e_INITIAL_NUM_BUCKET)
, d_numElements(0)
{
    initialize();
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::OrderedHashMap(
    int               initialNumBuckets,
    bslma::Allocator* basicAllocator)
: d_allocator_p(bslma::Default::allocator(basicAllocator))
, d_nodePool(e_NODE_SIZE, basicAllocator)
, d_bucketArray_p(0)
, d_sentinel_p(0)
, d_bucketArraySize(0)
, d_numElements(0)
{
    d_bucketArraySize = ImpDetails::nextPrime(initialNumBuckets);

    if (0 == d_bucketArraySize) {
        throw bsl::runtime_error("HashTable ran out of prime numbers");
    }

    initialize();
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::OrderedHashMap(
    const OrderedHashMap& other,
    bslma::Allocator*     basicAllocator)
: d_allocator_p(bslma::Default::allocator(basicAllocator))
, d_nodePool(e_NODE_SIZE, basicAllocator)
, d_bucketArray_p(0)
, d_sentinel_p(0)
, d_bucketArraySize(e_INITIAL_NUM_BUCKET)
, d_numElements(0)
{
    initialize();

    // Iterate over 'other' and insert elements in 'this'.

    const_iterator cit = other.begin();
    for (; cit != other.end(); ++cit) {
        insert(bsl::make_pair(cit->first, cit->second));
    }
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::~OrderedHashMap()
{
    clear();
    d_nodePool.release();
    d_allocator_p->deallocate(d_bucketArray_p);
}

// MANIPULATORS
template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>&
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::operator=(
    const OrderedHashMap& other)
{
    if (this != &other) {
        clear();

        // Iterate over 'other' and insert elements in 'this'.

        const_iterator cit = other.begin();
        for (; cit != other.end(); ++cit) {
            insert(bsl::make_pair(cit->first, cit->second));
        }
    }

    return *this;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline typename OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::iterator
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::begin()
{
    return iterator(d_sentinel_p->nextInList());
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline typename OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::iterator
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::end()
{
    return iterator(d_sentinel_p);
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline typename OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::local_iterator
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::begin(size_t index)
{
    BSLS_ASSERT_SAFE(index < bucket_count());
    return local_iterator(d_bucketArray_p + index);
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline typename OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::local_iterator
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::end(size_t index)
{
    BSLS_ASSERT_SAFE(index < bucket_count());
    return local_iterator(d_bucketArray_p + index, 0);
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline void OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::clear()
{
    // Bucket array and nodes obtained from the node pool are *not*
    // deallocated, just reset.

    // Reset bucket array.

    bsl::fill_n(d_bucketArray_p, d_bucketArraySize, Bucket());

    // Destroy each node in the list.

    size_t numDeleted = 0;
    Link*  cursor     = d_sentinel_p->nextInList();
    while (cursor != d_sentinel_p) {
        Link* thisCursor = cursor;
        Node* node       = static_cast<Node*>(cursor);
        node->value().~value_type();
        cursor = cursor->nextInList();
        thisCursor->~Link();
        d_nodePool.deallocate(node);  // node goes back to node pool
        ++numDeleted;
    }

    BSLS_ASSERT_SAFE(numDeleted == d_numElements);
    static_cast<void>(numDeleted);

    // Loop the sentinel.

    d_sentinel_p->setPrevInList(d_sentinel_p);
    d_sentinel_p->setNextInList(d_sentinel_p);
    d_numElements = 0;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline typename OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::iterator
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::erase(const_iterator position)
{
    BSLS_ASSERT_SAFE(end() != position);
    iterator nextPosition(position.d_link_p->nextInList());
    Node*    nodeToErase = static_cast<Node*>(position.d_link_p);

    size_t count = erase(get_key(nodeToErase->value()));
    BSLS_ASSERT(1 == count && "Invalid iterator provided");
    static_cast<void>(count);  // suppress compiler warning
    return nextPosition;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
size_t OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::erase(const key_type& key)
{
    Bucket* bucket       = getBucketForKey(key);
    Link*   prevInBucket = 0;
    for (Link* link = bucket->d_first_p; link != 0;
         link       = link->nextInBucket()) {
        Node* node = static_cast<Node*>(link);
        if (get_key(node->value()) == key) {
            // Invoke destructor.

            node->value().~value_type();

            // Unlink node.
            link->prevInList()->setNextInList(link->nextInList());
            link->nextInList()->setPrevInList(link->prevInList());

            if (prevInBucket) {
                prevInBucket->setNextInBucket(link->nextInBucket());
            }

            // Update this bucket's first and last cursors if needed.
            if (link == bucket->d_first_p) {
                bucket->d_first_p = link->nextInBucket();
            }
            if (link == bucket->d_last_p) {
                bucket->d_last_p = prevInBucket;
            }

            // Delete node.
            link->~Link();
            d_nodePool.deallocate(node);
            if (0 == --d_numElements) {
                // Loop the sentinel.
                d_sentinel_p->setPrevInList(d_sentinel_p);
                d_sentinel_p->setNextInList(d_sentinel_p);
            }
            return 1;  // RETURN
        }

        prevInBucket = link;
    }

    return 0;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
typename OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::const_iterator
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::erase(const_iterator first,
                                                    const_iterator last)
{
    while (first != last) {
        first = erase(first);
    }

    return first;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline typename OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::iterator
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::find(const key_type& key)
{
    Link* link = findKeyInBucket(key, getBucketForKey(key));
    if (link) {
        return iterator(link);  // RETURN
    }

    return iterator(d_sentinel_p);
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
template <class SOURCE_TYPE>
inline bsl::pair<
    typename OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::iterator,
    bool>
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::insert(const SOURCE_TYPE& value)
{
    Bucket* bucket    = getBucketForKey(get_key(value));
    Link*   foundLink = 0;
    if (0 != (foundLink = findKeyInBucket(get_key(value), bucket))) {
        return bsl::make_pair(iterator(foundLink), false);  // RETURN
    }
    // Element does not exist in the container

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rehashIfNeeded())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Find bucket for key again in case table was rehashed
        bucket = getBucketForKey(get_key(value));
    }

    Link* newSentinel = createNewSentinel();
    Link* oldSentinel = d_sentinel_p;  // == new node

    makeSentinel(newSentinel);
    BSLS_ASSERT_SAFE(oldSentinel == d_sentinel_p->prevInList());
    constructNode(oldSentinel, value);
    appendNodeToBucket(oldSentinel, bucket);

    ++d_numElements;
    return bsl::make_pair(iterator(d_sentinel_p->prevInList()), true);
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
template <class SOURCE_TYPE>
inline bsl::pair<
    typename OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::iterator,
    bool>
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::rinsert(const SOURCE_TYPE& value)
{
    Bucket* bucket    = getBucketForKey(get_key(value));
    Link*   foundLink = 0;
    if (0 != (foundLink = findKeyInBucket(get_key(value), bucket))) {
        return bsl::make_pair(iterator(foundLink), false);  // RETURN
    }
    // Element does not exist in the container

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rehashIfNeeded())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Find bucket for key again in case table was rehashed
        bucket = getBucketForKey(get_key(value));
    }

    Link* link = createNode(value);
    appendNodeToBucket(link, bucket);
    pushFront(link);

    ++d_numElements;
    return bsl::make_pair(iterator(d_sentinel_p->nextInList()), true);
}

// ACCESSORS
template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline typename OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::const_iterator
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::begin() const
{
    return const_iterator(d_sentinel_p->nextInList());
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline typename OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::const_iterator
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::end() const
{
    return const_iterator(d_sentinel_p);
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline
    typename OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::const_local_iterator
    OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::begin(size_t index) const
{
    BSLS_ASSERT_SAFE(index < d_bucketArraySize);
    return const_local_iterator(d_bucketArray_p + index);
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline
    typename OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::const_local_iterator
    OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::end(size_t index) const
{
    BSLS_ASSERT_SAFE(index < d_bucketArraySize);
    return const_local_iterator(d_bucketArray_p + index, 0);
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline size_t
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::bucket(const key_type& key) const
{
    hasher hash;
    return hash(key) % d_bucketArraySize;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline size_t
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::bucket_count() const
{
    return d_bucketArraySize;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline size_t
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::count(const key_type& key) const
{
    Link* link = findKeyInBucket(key, getBucketForKey(key));
    return link ? 1 : 0;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline bool OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::empty() const
{
    return 0 == d_numElements;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline typename OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::const_iterator
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::find(const key_type& key) const
{
    Link* link = findKeyInBucket(key, getBucketForKey(key));
    if (link) {
        return const_iterator(link);  // RETURN
    }

    return const_iterator(d_sentinel_p);
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline size_t OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::size() const
{
    return d_numElements;
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline double OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::load_factor() const
{
    return static_cast<double>(d_numElements) /
           static_cast<double>(d_bucketArraySize);
}

template <class KEY, class VALUE, class HASH, class VALUE_TYPE>
inline typename OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::allocator_type
OrderedHashMap<KEY, VALUE, HASH, VALUE_TYPE>::get_allocator() const
{
    return d_allocator_p;
}

}  // close package namespace
}  // close enterprise namespace

#endif
