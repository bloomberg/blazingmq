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

// mwcc_twokeyhashmap.h                                               -*-C++-*-
#ifndef INCLUDED_MWCC_TWOKEYHASHMAP
#define INCLUDED_MWCC_TWOKEYHASHMAP

//@PURPOSE: Provide a simple hash map container with two keys.
//
//@CLASSES:
// mwcc::TwoKeyHashMap:      hash map container with two keys
// mwcc::TwoKeyHashMapValue: element of the 'TwoKeyHashMap' container
//
//@DESCRIPTION:
// This component provides a mechanism, 'mwcc::TwoKeyHashMap', which is an
// equivalent to a hash map container but using two keys instead of one.  The
// 'TwoKeyHashMap' (which we will refer to as the "map") tries to follow the
// style of STL containers to the extent that is possible.
//
/// Usage
///-----
// Following is a very simple usage example.  First lets create an instance of
// 'TwoKeyHashMap' with 2 different types for keys and one type for values:
//..
//  typedef mwcc::TwoKeyHashMap<bsl::string,
//                              int,
//                              bsl::string> TestMap;
//  TestMap map(&allocator);
//..
// Next we add elements into our map using the 'insert()' method:
//..
//  map.insert("first", 1, "Hello");
//..
// Note that this method returns a pair containing an iterator and a result
// code.  If the code is 'e_INSERTED', an element was successfully created and
// the iterator refers to that element.  The code is 'e_FIRST_KEY_EXIST' if the
// first key already exists, and in this case the iterator refers to the
// element indexed by the first key.  The code is 'e_SECOND_KEY_EXIST' if the
// second key already exists, and in this case the iterator refers to the
// element indexed by the second key.  For example:
//..
//  pair<TestMap::iterator,
//       TestMap::InsertResult> p = map.insert("second", 2, "World");
//  BSLS_ASSERT(p.second == TestMap::e_INSERTED);
//  BSLS_ASSERT(p.first  != map.end());
//..
// You can obtain the number of elements in the map using the 'size()' method:
//..
//  BSLS_ASSERT(map.size() == 2);
//..
// The map provides two "findByKey()" methods that return iterators to the
// contained element if the key is present and a past-the-end iterator
// otherwise:
//..
//  TestMap::iterator iter = map.findByKey1("first");
//  BSLS_ASSERT(iter != map.end());
//  BSLS_ASSERT(iter->key1()  == "first");
//  BSLS_ASSERT(iter->key2()  == 1);
//  BSLS_ASSERT(iter->value() == "Hello");
//
//  iter = map.findByKey2(2);
//  BSLS_ASSERT(iter != map.end());
//  BSLS_ASSERT(iter->key1()  == "second");
//  BSLS_ASSERT(iter->key2()  == 2);
//  BSLS_ASSERT(iter->value() == "World");
//
//  iter = map.findByKey2(999);
//  BSLS_ASSERT(iter == map.end());
//..
// And here is how to iterate on the map:
//..
//  for (TestMap::iterator iter = map.begin(); iter != map.end(); ++iter) {
//      bsl::cout << iter->key1()  << " : "
//                << iter->key2()  << " => "
//                << iter->value() << bsl::endl;
//  }
//..
// By default 'begin()' and 'end()' return iterators for a range defined by the
// first key.  You can explicitly specify which key to use by providing an
// optional parameter:
//..
//  for (TestMap::iterator iter  = map.begin(TestMap::e_SECOND_KEY);
//                         iter != map.end(); ++iter) {
//      // iterate through the range defined by the second key
//  }
//..
// The map provides a method 'erase()' to remove an element referred to by an
// iterator.  Keep in mind that the iterator becomes invalid as a result of
// this operation.
//..
//  BSLS_ASSERT(map.size() == 2);
//  TestMap::iterator iter = map.findByKey1("first");
//  map.erase(iter);
//  BSLS_ASSERT(map.size() == 1);
//..
// You can also remove all elements at once with the 'clear()' method:
//..
//  map.clear();
//  BSLS_ASSERT(map.size() == 0);
//  BSLS_ASSERT(map.begin() == map.end());
//..
//
/// Thread Safety
///--------------
// NOT THREAD SAFE.
//
/// Exception Safety
///----------------
//: o no 'erase()', 'clear()', 'begin()', 'cbegin()', 'end()', 'cend()',
//:   'empty()', 'size()', 'max_size()' function throws an exception.
//:
//: o no constructor, assignment operator or 'keyIndex()' function on a
//:   returned iterator throws an exception.
//:
//: o no 'swap()' function throws an exception.
//:
//: o if an exception is thrown by an 'insert()' function while inserting a
//:   single element, that function has no effects.
//:
//: o if an exception is thrown by an 'eraseByKey1()' or 'eraseByKey2()'
//:   function, that function has no effects.

// MWC

// BDE
#include <bdlb_scopeexit.h>
#include <bdlf_bind.h>
#include <bdlf_memfn.h>
#include <bdlma_pool.h>
#include <bsl_algorithm.h>   // bsl::min, bsl::swap
#include <bsl_functional.h>  // bsl::hash
#include <bsl_iterator.h>
#include <bsl_unordered_map.h>
#include <bsl_utility.h>  // bsl::pair
#include <bslma_allocator.h>
#include <bslma_constructionutil.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_movableref.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmf_util.h>
#include <bsls_assert.h>
#include <bsls_compilerfeatures.h>
#include <bsls_keyword.h>
#include <bsls_objectbuffer.h>
#include <bsls_util.h>

namespace BloombergLP {

namespace mwcc {

template <class, class, class, class, class>
class TwoKeyHashMap;
template <class>
class TwoKeyHashMapValue;

// ==========================
// struct TwoKeyHashMap_Erase
// ==========================

/// Erase the specified `value` from the specified `container` via a call to
/// `container->erase(*value)`.
struct TwoKeyHashMap_Erase {
    // MANIPULATORS
    template <class CONTAINER, class VALUE>
    void operator()(CONTAINER* container, const VALUE* value) const;
};

// =========================
// struct TwoKeyHashMap_Node
// =========================

/// A node storing a mapped value.  The node is stored as the value in both
/// hash maps for each of the 2 keys.  The actual value that the user code
/// stores in the `TwoKeyHashMap` is stored in this struct.
template <class MAP>
struct TwoKeyHashMap_Node {
    // TYPES
    typedef typename bsl::unordered_map<typename MAP::first_key_type,
                                        TwoKeyHashMap_Node*,
                                        typename MAP::first_hasher>::iterator
        K1MapIter;

    typedef typename bsl::unordered_map<typename MAP::second_key_type,
                                        TwoKeyHashMap_Node*,
                                        typename MAP::second_hasher>::iterator
        K2MapIter;

    // PUBLIC DATA
    K1MapIter d_k1MapIter;

    K2MapIter d_k2MapIter;

    bsls::ObjectBuffer<typename MAP::mapped_type> d_value;

    // NOT IMPLEMENTED
    TwoKeyHashMap_Node(const TwoKeyHashMap_Node&) BSLS_KEYWORD_DELETED;
    TwoKeyHashMap_Node&
    operator=(const TwoKeyHashMap_Node&) BSLS_KEYWORD_DELETED;

    // CREATORS
    template <class VALUE_T>
    TwoKeyHashMap_Node(K1MapIter k1MapIter,
                       K2MapIter k2MapIter,
                       BSLS_COMPILERFEATURES_FORWARD_REF(VALUE_T) value,
                       bslma::Allocator* allocator);

    ~TwoKeyHashMap_Node();
};

// ===================
// class TwoKeyHashMap
// ===================

/// Provides a template for an associative container with two keys, K1 and
/// K2, containing objects of type VALUE.  `TwoKeyHashMap` meets the
/// requirements of Container as specified in the C++ standard, except that
/// it is not EqualityComparable.
template <class K1,
          class K2,
          class VALUE,
          class HASH1 = bsl::hash<K1>,
          class HASH2 = bsl::hash<K2> >
class TwoKeyHashMap {
  private:
    // PRIVATE TYPES
    typedef TwoKeyHashMap_Node<TwoKeyHashMap> Node;

  public:
    // TYPES
    enum KeyIndex {
        // Defines a numeric type to be used as the key index.

        e_FIRST_KEY = 1  // ALWAYS 1
        ,
        e_SECOND_KEY = 2  // ALWAYS 2
    };

    enum InsertResult {
        // Defines a numeric type to be used as the result of the 'insert()'
        // member function.

        e_INSERTED = 0  // ALWAYS 0
        ,
        e_FIRST_KEY_EXISTS = 1  // ALWAYS 1
        ,
        e_SECOND_KEY_EXISTS = 2  // ALWAYS 2
    };

    typedef K1                                first_key_type;
    typedef K2                                second_key_type;
    typedef VALUE                             mapped_type;
    typedef HASH1                             first_hasher;
    typedef HASH2                             second_hasher;
    typedef TwoKeyHashMapValue<TwoKeyHashMap> value_type;
    typedef value_type&                       reference;
    typedef const value_type&                 const_reference;
    typedef value_type*                       pointer;
    typedef const value_type*                 const_pointer;
    typedef ptrdiff_t                         difference_type;
    typedef size_t                            size_type;

    /// Provides a mutable iterator over the `TwoKeyHashMap` container.
    /// `iterator` meets the requirements of ForwardIterator and
    /// OutputIterator as specified in the C++ standard, except that, given
    /// `a` and `b`, dereferenceable iterators of type `iterator`, `a == b`
    /// does not imply that `++a == ++b`, unless both iterators are bound to
    /// the same key (i.e. `a.keyIndex() == b.keyIndex()`).
    class iterator {
      public:
        // TYPES
        typedef typename TwoKeyHashMap::value_type      value_type;
        typedef typename TwoKeyHashMap::difference_type difference_type;
        typedef typename TwoKeyHashMap::reference       reference;
        typedef typename TwoKeyHashMap::pointer         pointer;
        typedef bsl::forward_iterator_tag               iterator_category;

      private:
        // PRIVATE DATA
        KeyIndex d_keyIndex;

        TwoKeyHashMap* d_map_p;

        Node* d_node_p;

        // FRIENDS
        friend class TwoKeyHashMap;

      private:
        // PRIVATE CREATORS

        /// Internal constructor.
        iterator(KeyIndex       keyIndex,
                 TwoKeyHashMap* map,
                 Node*          node) BSLS_KEYWORD_NOEXCEPT;

      public:
        // CREATORS

        /// Create a `iterator` object that behaves like the past-the-end
        /// iterator of some unspecified empty container.  Two default-
        /// constructed iterators always compares equal.  The value of
        /// `keyIndex()` is unspecified.
        iterator() BSLS_KEYWORD_NOEXCEPT;

      public:
        // MANIPULATORS

        /// Move the iterator to the next element in the `TwoKeyHashMap`.
        /// Return `*this`.  The behavior is undefined unless the iterator
        /// is dereferenceable.
        iterator& operator++();

        /// Make a copy of `*this`, perform `++(*this)` and return the copy.
        /// The behavior is undefined unless the iterator is
        /// dereferenceable.
        iterator operator++(int);

        /// Swap the contents of `lhs` and `rhs`.
        friend void swap(iterator& lhs, iterator& rhs) BSLS_KEYWORD_NOEXCEPT
        {
            // NOTE: This function had to be defined inline.  For reasons I
            //       don't completely understand the code does not compile
            //       otherwise.

            using bsl::swap;

            swap(lhs.d_keyIndex, rhs.d_keyIndex);
            swap(lhs.d_map_p, rhs.d_map_p);
            swap(lhs.d_node_p, rhs.d_node_p);
        }

      public:
        // ACCESSORS

        /// Return a modifiable reference the `TwoKeyHashMapValue` object
        /// referred to by this iterator.  The behavior is undefined unless
        /// the iterator is dereferenceable.
        reference operator*() const;

        /// Return a pointer the `TwoKeyHashMapValue` object referred to by
        /// this iterator.  The behavior is undefined unless the iterator is
        /// dereferenceable.
        pointer operator->() const;

        /// Return the index of the key this iterator is currently bound to.
        KeyIndex keyIndex() const BSLS_KEYWORD_NOEXCEPT;

        /// Return `true` if the specified iterators `lhs` and `rhs` refer
        /// to the same object, and `false` otherwise.  The behavior is
        /// undefined unless `lhs` and `rhs` refer to the same container, or
        /// are both default-constructed.
        friend bool operator==(const iterator& lhs, const iterator& rhs)
        {
            // NOTE: This function had to be defined inline.  For reasons I
            //       don't completely understand the code does not compile
            //       otherwise.

            // PRECONDITIONS
            BSLS_ASSERT(lhs.d_map_p == rhs.d_map_p);

            return lhs.d_node_p == rhs.d_node_p;
        }

        /// Return `!(lhs == rhs)`.
        friend bool operator!=(const iterator& lhs, const iterator& rhs)
        {
            // NOTE: This function had to be defined inline.  For reasons I
            //       don't completely understand the code does not compile
            //       otherwise.

            return !(lhs == rhs);
        }
    };

    /// Provides a non-mutable iterator over the `TwoKeyHashMap` container.
    /// `const_iterator` meets the requirements of ForwardIterator as
    /// specified in the C++ standard, except that, given `a` and `b`,
    /// dereferenceable iterators of type `const_iterator`, `a == b` does
    /// not imply that `++a == ++b`, unless both iterators are bound to the
    /// same key (i.e. `a.keyIndex() == b.keyIndex()`).
    class const_iterator {
      public:
        // TYPES
        typedef typename TwoKeyHashMap::value_type      value_type;
        typedef typename TwoKeyHashMap::difference_type difference_type;
        typedef typename TwoKeyHashMap::const_reference reference;
        typedef typename TwoKeyHashMap::const_pointer   pointer;
        typedef bsl::forward_iterator_tag               iterator_category;

      private:
        // PRIVATE DATA
        KeyIndex d_keyIndex;

        const TwoKeyHashMap* d_map_p;

        const Node* d_node_p;

        // FRIENDS
        friend class TwoKeyHashMap;

      private:
        // PRIVATE CREATORS

        /// Internal constructor.
        const_iterator(KeyIndex             keyIndex,
                       const TwoKeyHashMap* map,
                       const Node*          node) BSLS_KEYWORD_NOEXCEPT;

      public:
        // CREATORS

        /// Create a `const_iterator` object that behaves like the past-the-
        /// end iterator of some unspecified empty container.  Two default-
        /// constructed iterators always compares equal.  The value of
        /// `keyIndex()` is unspecified.
        const_iterator() BSLS_KEYWORD_NOEXCEPT;

        /// Create a `const_iterator` object that refers to the same element
        /// as the specified iterator `it`, if any.
        const_iterator(iterator it) BSLS_KEYWORD_NOEXCEPT;  // IMPLICIT

      public:
        // MANIPULATORS

        /// Move the iterator to the next element in the `TwoKeyHashMap`.
        /// Return `*this`.  The behavior is undefined unless the iterator
        /// is dereferenceable.
        const_iterator& operator++();

        /// Make a copy of `*this`, perform `++(*this)` and return the copy.
        /// The behavior is undefined unless the iterator is
        /// dereferenceable.
        const_iterator operator++(int);

        /// Swap the contents of `lhs` and `rhs`.
        friend void swap(const_iterator& lhs,
                         const_iterator& rhs) BSLS_KEYWORD_NOEXCEPT
        {
            // NOTE: This function had to be defined inline.  For reasons I
            //       don't completely understand the code does not compile
            //       otherwise.

            using bsl::swap;

            swap(lhs.d_keyIndex, rhs.d_keyIndex);
            swap(lhs.d_map_p, rhs.d_map_p);
            swap(lhs.d_node_p, rhs.d_node_p);
        }

      public:
        // ACCESSORS

        /// Return a const reference the `TwoKeyHashMapValue` object
        /// referred to by this iterator.  The behavior is undefined unless
        /// the iterator is dereferenceable.
        reference operator*() const;

        /// Return a const pointer the `TwoKeyHashMapValue` object referred
        /// to by this iterator.  The behavior is undefined unless the
        /// iterator is dereferenceable.
        pointer operator->() const;

        /// Return the index of the key this iterator is currently bound to.
        KeyIndex keyIndex() const BSLS_KEYWORD_NOEXCEPT;

        /// Return `true` if the specified iterators `lhs` and `rhs` refer
        /// to the same object, and `false` otherwise.  The behavior is
        /// undefined unless `lhs` and `rhs` refer to the same container, or
        /// are both default-constructed.
        friend bool operator==(const const_iterator& lhs,
                               const const_iterator& rhs)
        {
            // NOTE: This function had to be defined inline.  For reasons I
            //       don't completely understand the code does not compile
            //       otherwise.

            // PRECONDITIONS
            BSLS_ASSERT(lhs.d_map_p == rhs.d_map_p);

            return lhs.d_node_p == rhs.d_node_p;
        }

        /// Return `!(lhs == rhs)`.
        friend bool operator!=(const const_iterator& lhs,
                               const const_iterator& rhs)
        {
            // NOTE: This function had to be defined inline.  For reasons I
            //       don't completely understand the code does not compile
            //       otherwise.

            return !(lhs == rhs);
        }
    };

  private:
    // PRIVATE TYPES
    typedef bsl::unordered_map<K1, Node*, HASH1> K1Map;
    typedef typename K1Map::iterator             K1MapIter;
    typedef typename K1Map::const_iterator       K1MapConstIter;

    typedef bsl::unordered_map<K2, Node*, HASH2> K2Map;
    typedef typename K2Map::iterator             K2MapIter;
    typedef typename K2Map::const_iterator       K2MapConstIter;

  private:
    // PRIVATE DATA
    K1Map d_k1Map;  // Map key1 -> node

    K2Map d_k2Map;  // Map key2 -> node

    bslma::ManagedPtr<bdlma::Pool> d_nodeAllocator;  // Pool of nodes

    // FRIENDS
    friend class iterator;
    friend class const_iterator;

  public:
    // CREATORS

    /// Create a `TwoKeyHashMap` object that is initially empty with the
    /// optionally specified `basicAllocator` used to supply memory.  Since
    /// hash functions for both keys are not provided, a default-constructed
    /// values are used.  If `basicAllocator` is 0, the default memory
    /// allocator is used.
    explicit TwoKeyHashMap(bslma::Allocator* basicAllocator = 0);

    /// Create a `TwoKeyHashMap` object that is initially empty with the
    /// specified `hash1` hash function for the first and the optionally
    /// specified `basicAllocator` used to supply memory.  Since hash
    /// function for the second key is not provided, a default-constructed
    /// values are used.  If `basicAllocator` is 0, the default memory
    /// allocator is used.
    explicit TwoKeyHashMap(const HASH1&      hash1,
                           bslma::Allocator* basicAllocator = 0);

    /// Create a `TwoKeyHashMap` object that is initially empty with the
    /// specified `hash1` hash function for the first key and the specified
    /// `hash2` hash function for the second key and the optionally
    /// specified `basicAllocator` used to supply memory.  If
    /// `basicAllocator` is 0, the default memory allocator is used.
    TwoKeyHashMap(const HASH1&      hash1,
                  const HASH2&      hash2,
                  bslma::Allocator* basicAllocator = 0);

    /// Create a `TwoKeyHashMap` object having the same contents as the
    /// specified `original` object.  Optionally specify a `basicAllocator`
    /// used to supply memory.  If `basicAllocator` is 0, the default memory
    /// allocator is used.  Hash functions are copied from the `original`
    /// object.  Note that the order of elements in `*this` may differ from
    /// that of `original`.
    TwoKeyHashMap(const TwoKeyHashMap& original,
                  bslma::Allocator*    basicAllocator = 0);

    /// Create a `TwoKeyHashMap` object having the same contents as the
    /// specified `original` object, leaving `original` in a valid but
    /// unspecified state.  Optionally specify a `basicAllocator` used to
    /// supply memory.  If `basicAllocator` is 0, the default memory
    /// allocator is used.  Hash functions are copied from the `original`
    /// object.  Note that the order of elements in `*this` may differ from
    /// that of `original`.
    TwoKeyHashMap(bslmf::MovableRef<TwoKeyHashMap> original,
                  bslma::Allocator*                basicAllocator = 0);

    /// Destroy this object.
    ~TwoKeyHashMap();

  public:
    // MANIPULATORS

    /// Replace the contents of `*this` with those of `rhs`.  Return
    /// `*this`.  Hash functions are copied from the `rhs` object.  Note
    /// that the order of elements in `*this` may differ from that of `rhs`.
    TwoKeyHashMap& operator=(const TwoKeyHashMap& rhs);

    /// Replace the contents of `*this` with those of `rhs`, leaving 'rhs in
    /// a valid but unspecified state'.  Return `*this`.  Hash functions are
    /// copied from the `rhs` object.  Note that the order of elements in
    /// `*this` may differ from that of `rhs`.
    TwoKeyHashMap& operator=(bslmf::MovableRef<TwoKeyHashMap> rhs);

    /// Return 'insert(value.key1(), value.key2(), value.value(),
    /// keyIndex)'.
    bsl::pair<iterator, InsertResult> insert(const value_type& value,
                                             KeyIndex keyIndex = e_FIRST_KEY);

    /// Insert the specified mapped `value` indexed by the specified keys
    /// `k1` and `k2`, leaving `k1`, `k2` and `value` in a valid but
    /// unspecified state.  Return a pair containing an iterator and a
    /// result code.  On success, the code value is `e_INSERTED`, the
    /// iterator refers to the newly created element and is bound to the key
    /// specified by `keyIndex`.  Otherwise, if `k1` already exists, the
    /// code value is `e_FIRST_KEY_EXISTS`, the iterator refers to the
    /// element indexed by `k1` and is bound to the first key.  Otherwise,
    /// if `k2` already exists, the code value is `e_SECOND_KEY_EXISTS`, the
    /// iterator refers to the element indexed by `k2` and is bound to the
    /// second key.  On failure, this function has no effect.
    ///
    /// This function meets the strong exception guarantee. If an exception
    /// is thrown, this function has no effect.
    template <class K1_T, class K2_T, class VALUE_T>
    bsl::pair<iterator, InsertResult>
    insert(BSLS_COMPILERFEATURES_FORWARD_REF(K1_T) k1,
           BSLS_COMPILERFEATURES_FORWARD_REF(K2_T) k2,
           BSLS_COMPILERFEATURES_FORWARD_REF(VALUE_T) value,
           KeyIndex keyIndex = e_FIRST_KEY);

    /// Remove the element referred by the specified iterator `it`.  Return
    /// an iterator to the element following the removed one, as indexed by
    /// the key `it` is currently bound to.  If `it` refers to the last
    /// element, return `end(it.keyIndex())`.  The behavior is undefined
    /// unless `it` refers to an element in this container.  Note that
    /// references, pointers and iterators to the erased element are
    /// invalidated.  Past-the-end iterators are also invalidated.  Other
    /// references, pointers and iterators are not affected.  The order of
    /// the elements that are not erased is preserved.
    iterator erase(const_iterator it);

    int eraseByKey1(const first_key_type& k1);

    /// Remove the element indexed by the specified key `k1` or `k2`
    /// respectively.  Return 0 on success and a non-zero value if no such
    /// element is found.  Note that references, pointers and iterators to
    /// the erased element are invalidated.  Past-the-end iterators are also
    /// invalidated.  Other references, pointers and iterators are not
    /// affected.  The order of the elements that are not erased is
    /// preserved.
    ///
    /// This function meets the strong exception guarantee. If an exception
    /// is thrown, this function has no effect.
    int eraseByKey2(const second_key_type& k2);

    /// Remove all elements from the container.  Invalidate any references,
    /// pointers, or iterators referring to contained elements.  Invalidate
    /// past-the-end iterators.
    void clear() BSLS_KEYWORD_NOEXCEPT;

    /// Swap the contents of `*this` and `other`.  Note that iterators
    /// referring to elements contained in `*this` and `other`, as well as
    /// past-the-end iterators for these two containers, are invalidated.
    /// Pointers and references to contained elements are not affected.
    void swap(TwoKeyHashMap& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS
    iterator begin(KeyIndex keyIndex = e_FIRST_KEY) BSLS_KEYWORD_NOEXCEPT;
    const_iterator
    begin(KeyIndex keyIndex = e_FIRST_KEY) const BSLS_KEYWORD_NOEXCEPT;

    /// Return an iterator to the first element in the container as indexed
    /// by the specified `keyIndex`.  If the container is empty, return
    /// `end(keyIndex)`.
    const_iterator
    cbegin(KeyIndex keyIndex = e_FIRST_KEY) const BSLS_KEYWORD_NOEXCEPT;

    iterator end(KeyIndex keyIndex = e_FIRST_KEY) BSLS_KEYWORD_NOEXCEPT;
    const_iterator
    end(KeyIndex keyIndex = e_FIRST_KEY) const BSLS_KEYWORD_NOEXCEPT;

    /// Returns an iterator to the element following the last element of the
    /// container as indexed by the specified `keyIndex`.
    const_iterator
    cend(KeyIndex keyIndex = e_FIRST_KEY) const BSLS_KEYWORD_NOEXCEPT;

    iterator       findByKey1(const first_key_type& k1);
    const_iterator findByKey1(const first_key_type& k1) const;

    /// Return an iterator to the element indexed by the specified key `k1`
    /// or `k2` respectively.  If such element is not found, return
    /// `end(e_FIRST_KEY)` or `end(e_SECOND_KEY)` respectively.
    iterator       findByKey2(const second_key_type& k2);
    const_iterator findByKey2(const second_key_type& k2) const;

    /// Return `true` if the container contains no elements, and `false`
    /// otherwise.
    bool empty() const BSLS_KEYWORD_NOEXCEPT;

    /// Return the current number of elements in the container.
    size_type size() const BSLS_KEYWORD_NOEXCEPT;

    /// Return the maximum possible number of elements in the container.
    size_type max_size() const BSLS_KEYWORD_NOEXCEPT;

    /// Returns the function used to hash the first key.
    first_hasher hash1() const;

    /// Returns the function used to hash the second key.
    second_hasher hash2() const;

    /// Return the allocator used by this container to supply memory.
    bslma::Allocator* allocator() const;

#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
    bool assertInvariants() const;
#endif

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TwoKeyHashMap, bslma::UsesBslmaAllocator)
};

// ========================
// class TwoKeyHashMapValue
// ========================

/// Defines an element in a `TwoKeyHashMap` container.  Objects of this type
/// are not copyable, movable or swappable, and they can not be created by
/// users directly.  `TwoKeyHashMapValue` provides access to the first and
/// the second key associated with the element, as well as to the mapped
/// value.  Two `TwoKeyHashMapValue` objects can be compared for equality or
/// inequality using corresponding free comparison operators.
template <class MAP>
class TwoKeyHashMapValue : private TwoKeyHashMap_Node<MAP> {
  private:
    // NOT IMPLEMENTED
    TwoKeyHashMapValue() BSLS_KEYWORD_DELETED;
    TwoKeyHashMapValue(const TwoKeyHashMapValue&) BSLS_KEYWORD_DELETED;
    TwoKeyHashMapValue&
    operator=(const TwoKeyHashMapValue&) BSLS_KEYWORD_DELETED;

  public:
    // MANIPULATORS

    /// Return a modifiable reference to the mapped value associated with
    /// this `TwoKeyHashMapValue`.
    typename MAP::mapped_type& value() BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Return a const reference to the first key associated with this
    /// `TwoKeyHashMapValue`.
    const typename MAP::first_key_type& key1() const BSLS_KEYWORD_NOEXCEPT;

    /// Return a const reference to the second key associated with this
    /// `TwoKeyHashMapValue`.
    const typename MAP::second_key_type& key2() const BSLS_KEYWORD_NOEXCEPT;

    /// Return a const reference to the mapped value associated with this
    /// `TwoKeyHashMapValue`.
    const typename MAP::mapped_type& value() const BSLS_KEYWORD_NOEXCEPT;
};

// FREE OPERATORS

/// Swap the contents of `lhs` and `rhs` via a call to `lhs.swap(rhs)`.
template <class K1, class K2, class VALUE, class H1, class H2>
void swap(TwoKeyHashMap<K1, K2, VALUE, H1, H2>& lhs,
          TwoKeyHashMap<K1, K2, VALUE, H1, H2>& rhs) BSLS_KEYWORD_NOEXCEPT;

/// Compare the contents of the specified `lhs` with those of the specified
/// `rhs`.  Return `true` if they are equal and `false` otherwise.
template <class K1, class K2, class VALUE, class H1, class H2>
bool operator==(const TwoKeyHashMap<K1, K2, VALUE, H1, H2>& lhs,
                const TwoKeyHashMap<K1, K2, VALUE, H1, H2>& rhs);

/// Compare the contents of the specified `lhs` with those of the specified
/// `rhs`.  Return `false` if they are equal and `true` otherwise.
template <class K1, class K2, class VALUE, class H1, class H2>
bool operator!=(const TwoKeyHashMap<K1, K2, VALUE, H1, H2>& lhs,
                const TwoKeyHashMap<K1, K2, VALUE, H1, H2>& rhs);

/// Return 'lhs.key1() == rhs.key1() && lhs.key2() == rhs.key2() &&
/// lhs.value() == rhs.value()'.
template <class MAP>
bool operator==(const TwoKeyHashMapValue<MAP>& lhs,
                const TwoKeyHashMapValue<MAP>& rhs);

/// Return `!(lhs == rhs)`.
template <class MAP>
bool operator!=(const TwoKeyHashMapValue<MAP>& lhs,
                const TwoKeyHashMapValue<MAP>& rhs);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------
// struct TwoKeyHashMap_Erase
// --------------------------

// MANIPULATORS
template <class CONTAINER, class VALUE>
inline void TwoKeyHashMap_Erase::operator()(CONTAINER*   container,
                                            const VALUE* value) const
{
    container->erase(*value);
}

// -------------------------
// struct TwoKeyHashMap_Node
// -------------------------

// CREATORS
template <class MAP>
template <class VALUE_T>
inline TwoKeyHashMap_Node<MAP>::TwoKeyHashMap_Node(
    K1MapIter k1MapIter,
    K2MapIter k2MapIter,
    BSLS_COMPILERFEATURES_FORWARD_REF(VALUE_T) value,
    bslma::Allocator* allocator)
: d_k1MapIter(k1MapIter)
, d_k2MapIter(k2MapIter)
, d_value()
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);

    bslma::ConstructionUtil::construct(d_value.address(),
                                       allocator,
                                       BSLS_COMPILERFEATURES_FORWARD(VALUE_T,
                                                                     value));
}

template <class MAP>
inline TwoKeyHashMap_Node<MAP>::~TwoKeyHashMap_Node()
{
    typedef typename MAP::mapped_type value_type;
    d_value.object().~value_type();
}

// -----------------------------
// class TwoKeyHashMap::iterator
// -----------------------------

// PRIVATE CREATORS
template <class K1, class K2, class VALUE, class H1, class H2>
inline TwoKeyHashMap<K1, K2, VALUE, H1, H2>::iterator::iterator(
    KeyIndex       keyIndex,
    TwoKeyHashMap* map,
    Node*          node) BSLS_KEYWORD_NOEXCEPT : d_keyIndex(keyIndex),
                                        d_map_p(map),
                                        d_node_p(node)
{
    // PRECONDITIONS
    BSLS_ASSERT(keyIndex == e_FIRST_KEY || keyIndex == e_SECOND_KEY);
    BSLS_ASSERT(map);
}

// CREATORS
template <class K1, class K2, class VALUE, class H1, class H2>
inline TwoKeyHashMap<K1, K2, VALUE, H1, H2>::iterator::iterator()
    BSLS_KEYWORD_NOEXCEPT : d_keyIndex(e_FIRST_KEY),
                            d_map_p(0),
                            d_node_p(0)
{
    // NOTHING
}

// MANIPULATORS
template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::iterator&
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::iterator::operator++()
{
    // PRECONDITIONS
    BSLS_ASSERT(d_node_p);

    if (d_keyIndex == e_FIRST_KEY) {
        K1MapIter it = d_node_p->d_k1MapIter;
        d_node_p     = (++it != d_map_p->d_k1Map.end()) ? it->second
                                                        : static_cast<Node*>(0);
    }
    else {
        K2MapIter it = d_node_p->d_k2MapIter;
        d_node_p     = (++it != d_map_p->d_k2Map.end()) ? it->second
                                                        : static_cast<Node*>(0);
    }

    return *this;
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::iterator
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::iterator::operator++(int)
{
    // PRECONDITIONS
    BSLS_ASSERT(d_node_p);

    iterator copy = *this;
    ++(*this);
    return copy;
}

// ACCESSORS
template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::iterator::reference
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::iterator::operator*() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_node_p);

    return reinterpret_cast<reference>(*d_node_p);
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::iterator::pointer
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::iterator::operator->() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_node_p);

    return &reinterpret_cast<reference>(*d_node_p);
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::KeyIndex
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::iterator::keyIndex() const
    BSLS_KEYWORD_NOEXCEPT
{
    return d_keyIndex;
}

// -----------------------------------
// class TwoKeyHashMap::const_iterator
// -----------------------------------

// PRIVATE CREATORS
template <class K1, class K2, class VALUE, class H1, class H2>
inline TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator::const_iterator(
    KeyIndex             keyIndex,
    const TwoKeyHashMap* map,
    const Node*          node) BSLS_KEYWORD_NOEXCEPT : d_keyIndex(keyIndex),
                                              d_map_p(map),
                                              d_node_p(node)
{
    // PRECONDITIONS
    BSLS_ASSERT(keyIndex == e_FIRST_KEY || keyIndex == e_SECOND_KEY);
    BSLS_ASSERT(map);
}

// CREATORS
template <class K1, class K2, class VALUE, class H1, class H2>
inline TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator::const_iterator()
    BSLS_KEYWORD_NOEXCEPT : d_keyIndex(e_FIRST_KEY),
                            d_map_p(0),
                            d_node_p(0)
{
    // NOTHING
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator::const_iterator(
    iterator it) BSLS_KEYWORD_NOEXCEPT : d_keyIndex(it.d_keyIndex),
                                         d_map_p(it.d_map_p),
                                         d_node_p(it.d_node_p)
{
    // NOTHING
}

// MANIPULATORS
template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator&
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator::operator++()
{
    // PRECONDITIONS
    BSLS_ASSERT(d_node_p);

    if (d_keyIndex == e_FIRST_KEY) {
        K1MapIter it = d_node_p->d_k1MapIter;
        d_node_p     = (++it != d_map_p->d_k1Map.end()) ? it->second
                                                        : static_cast<Node*>(0);
    }
    else {
        K2MapIter it = d_node_p->d_k2MapIter;
        d_node_p     = (++it != d_map_p->d_k2Map.end()) ? it->second
                                                        : static_cast<Node*>(0);
    }

    return *this;
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator::operator++(int)
{
    // PRECONDITIONS
    BSLS_ASSERT(d_node_p);

    const_iterator copy = *this;
    ++(*this);
    return copy;
}

// ACCESSORS
template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator::reference
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator::operator*() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_node_p);

    return reinterpret_cast<reference>(*d_node_p);
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator::pointer
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator::operator->() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_node_p);

    return &reinterpret_cast<reference>(*d_node_p);
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::KeyIndex
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator::keyIndex() const
    BSLS_KEYWORD_NOEXCEPT
{
    return d_keyIndex;
}

// -------------------
// class TwoKeyHashMap
// -------------------

// CREATORS
template <class K1, class K2, class VALUE, class H1, class H2>
inline TwoKeyHashMap<K1, K2, VALUE, H1, H2>::TwoKeyHashMap(
    bslma::Allocator* basicAllocator)
: d_k1Map(1,  // bucket count
          H1(),
          typename K1Map::key_equal(),
          basicAllocator)
, d_k2Map(1,  // bucket count
          H2(),
          typename K2Map::key_equal(),
          basicAllocator)
, d_nodeAllocator(new (*allocator()) bdlma::Pool(sizeof(Node), allocator()),
                  allocator())
{
    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(assertInvariants());
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline TwoKeyHashMap<K1, K2, VALUE, H1, H2>::TwoKeyHashMap(
    const H1&         hash1,
    bslma::Allocator* basicAllocator)
: d_k1Map(1,  // bucket count
          hash1,
          typename K1Map::key_equal(),
          basicAllocator)
, d_k2Map(1,  // bucket count
          H2(),
          typename K2Map::key_equal(),
          basicAllocator)
, d_nodeAllocator(new (*allocator()) bdlma::Pool(sizeof(Node), allocator()),
                  allocator())
{
    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(assertInvariants());
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline TwoKeyHashMap<K1, K2, VALUE, H1, H2>::TwoKeyHashMap(
    const H1&         hash1,
    const H2&         hash2,
    bslma::Allocator* basicAllocator)
: d_k1Map(1,  // bucket count
          hash1,
          typename K1Map::key_equal(),
          basicAllocator)
, d_k2Map(1,  // bucket count
          hash2,
          typename K2Map::key_equal(),
          basicAllocator)
, d_nodeAllocator(new (*allocator()) bdlma::Pool(sizeof(Node), allocator()),
                  allocator())
{
    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(assertInvariants());
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline TwoKeyHashMap<K1, K2, VALUE, H1, H2>::TwoKeyHashMap(
    const TwoKeyHashMap& original,
    bslma::Allocator*    basicAllocator)
: d_k1Map(original.d_k1Map.bucket_count(),
          original.d_k1Map.hash_function(),
          original.d_k1Map.key_eq(),
          basicAllocator)
, d_k2Map(original.d_k2Map.bucket_count(),
          original.d_k2Map.hash_function(),
          original.d_k2Map.key_eq(),
          basicAllocator)
, d_nodeAllocator(new (*allocator()) bdlma::Pool(sizeof(Node), allocator()),
                  allocator())
{
    // free memory on failure
    BDLB_SCOPEEXIT_PROCTOR(guard,
                           bdlf::MemFnUtil::memFn(&TwoKeyHashMap::clear,
                                                  this));

    // copy elements from 'original' to '*this'
    d_nodeAllocator->reserveCapacity(original.size());
    for (const_iterator it = original.cbegin(); it != original.cend(); ++it) {
        insert(*it);
    }

    // success
    guard.release();

    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(assertInvariants());
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline TwoKeyHashMap<K1, K2, VALUE, H1, H2>::TwoKeyHashMap(
    bslmf::MovableRef<TwoKeyHashMap> original,
    bslma::Allocator*                basicAllocator)
: d_k1Map(1,  // bucket count
          H1(),
          typename K1Map::key_equal(),
          basicAllocator)
, d_k2Map(1,  // bucket count
          H2(),
          typename K2Map::key_equal(),
          basicAllocator)
, d_nodeAllocator(new (*allocator()) bdlma::Pool(sizeof(Node), allocator()),
                  allocator())
{
    if (allocator() == bslmf::MovableRefUtil::access(original).allocator()) {
        // '*this' and 'original' use the same allocator. Move the contents.
        bslmf::MovableRefUtil::access(original).swap(*this);
    }
    else {
        // '*this' and 'original' use different allocators. Fallback to copy.
        TwoKeyHashMap(bslmf::MovableRefUtil::access(original), basicAllocator)
            .swap(*this);
    }

    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(assertInvariants());
    BSLS_ASSERT_SAFE(
        bslmf::MovableRefUtil::access(original).assertInvariants());
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline TwoKeyHashMap<K1, K2, VALUE, H1, H2>::~TwoKeyHashMap()
{
    clear();
}

// MANIPULATORS
template <class K1, class K2, class VALUE, class H1, class H2>
inline TwoKeyHashMap<K1, K2, VALUE, H1, H2>&
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::operator=(const TwoKeyHashMap& rhs)
{
#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
    // POSTCONDITIONS
    BDLB_SCOPEEXIT_PROCTOR(
        invariantsGuard,
        bdlf::MemFnUtil::memFn(&TwoKeyHashMap::assertInvariants, this));
#endif

    if (&rhs != this) {
        clear();                                      // release resources
        TwoKeyHashMap(rhs, allocator()).swap(*this);  // copy-and-swap
    }

    return *this;
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline TwoKeyHashMap<K1, K2, VALUE, H1, H2>&
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::operator=(
    bslmf::MovableRef<TwoKeyHashMap> rhs)
{
#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
    // POSTCONDITIONS
    BDLB_SCOPEEXIT_PROCTOR(
        invariantsGuard1,
        bdlf::MemFnUtil::memFn(&TwoKeyHashMap::assertInvariants, this));

    BDLB_SCOPEEXIT_PROCTOR(
        invariantsGuard2,
        bdlf::MemFnUtil::memFn(&TwoKeyHashMap::assertInvariants,
                               &bslmf::MovableRefUtil::access(rhs)));
#endif

    if (&bslmf::MovableRefUtil::access(rhs) != this) {
        if (allocator() == bslmf::MovableRefUtil::access(rhs).allocator()) {
            // '*this' and 'rhs' use the same allocator. Move the contents.
            clear();  // release resources
            TwoKeyHashMap(bslmf::MovableRefUtil::move(rhs),
                          allocator())
                .swap(*this);  // move-and-swap
        }
        else {
            // '*this' and 'rhs' use different allocators. Fallback to copy.
            *this = bslmf::MovableRefUtil::access(rhs);
        }
    }

    return *this;
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline bsl::pair<typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::iterator,
                 typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::InsertResult>
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::insert(const value_type& value,
                                             KeyIndex          keyIndex)
{
    // PRECONDITIONS
    BSLS_ASSERT(keyIndex == e_FIRST_KEY || keyIndex == e_SECOND_KEY);

#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
    // POSTCONDITIONS
    BDLB_SCOPEEXIT_PROCTOR(
        invariantsGuard,
        bdlf::MemFnUtil::memFn(&TwoKeyHashMap::assertInvariants, this));
#endif

    // do insert
    return insert(value.key1(), value.key2(), value.value(), keyIndex);
}

template <class K1, class K2, class VALUE, class H1, class H2>
template <class K1_T, class K2_T, class VALUE_T>
inline bsl::pair<typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::iterator,
                 typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::InsertResult>
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::insert(
    BSLS_COMPILERFEATURES_FORWARD_REF(K1_T) k1,
    BSLS_COMPILERFEATURES_FORWARD_REF(K2_T) k2,
    BSLS_COMPILERFEATURES_FORWARD_REF(VALUE_T) value,
    KeyIndex keyIndex)
{
    // PRECONDITIONS
    BSLS_ASSERT(keyIndex == e_FIRST_KEY || keyIndex == e_SECOND_KEY);

#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
    // POSTCONDITIONS
    BDLB_SCOPEEXIT_PROCTOR(
        invariantsGuard,
        bdlf::MemFnUtil::memFn(&TwoKeyHashMap::assertInvariants, this));
#endif

    bool hasInserted = false;

    // try inserting in k1Map
    bsl::pair<K1MapIter, bool> pair1 = d_k1Map.emplace(
        BSLS_COMPILERFEATURES_FORWARD(K1_T, k1),
        static_cast<Node*>(0));
    K1MapIter k1MapIter = pair1.first;
    hasInserted         = pair1.second;

    if (!hasInserted) {
        return bsl::make_pair(iterator(e_FIRST_KEY, this, k1MapIter->second),
                              e_FIRST_KEY_EXISTS);  // RETURN
    }

    // rollback k1Map insertion on failure
    BDLB_SCOPEEXIT_PROCTOR(k1MapGuard,
                           bdlf::BindUtil::bindR<void>(TwoKeyHashMap_Erase(),
                                                       &d_k1Map,
                                                       &k1MapIter));

    // try inserting in k2Map
    bsl::pair<K2MapIter, bool> pair2 = d_k2Map.emplace(
        BSLS_COMPILERFEATURES_FORWARD(K2_T, k2),
        static_cast<Node*>(0));
    K2MapIter k2MapIter = pair2.first;
    hasInserted         = pair2.second;

    if (!hasInserted) {
        return bsl::make_pair(iterator(e_SECOND_KEY, this, k2MapIter->second),
                              e_SECOND_KEY_EXISTS);  // RETURN
    }

    // rollback k2Map insertion on failure
    BDLB_SCOPEEXIT_PROCTOR(k2MapGuard,
                           bdlf::BindUtil::bindR<void>(TwoKeyHashMap_Erase(),
                                                       &d_k2Map,
                                                       &k2MapIter));

    // store the value
    Node* p = new (*d_nodeAllocator)
        Node(k1MapIter,
             k2MapIter,
             BSLS_COMPILERFEATURES_FORWARD(VALUE_T, value),
             allocator());

    k1MapIter->second = k2MapIter->second = p;

    // success
    k1MapGuard.release();
    k2MapGuard.release();

    return bsl::make_pair(iterator(keyIndex, this, p), e_INSERTED);
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::iterator
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::erase(const_iterator it)
{
    // PRECONDITIONS
    BSLS_ASSERT(it.d_map_p == this && it.d_node_p != 0);

#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
    // POSTCONDITIONS
    BDLB_SCOPEEXIT_PROCTOR(
        invariantsGuard,
        bdlf::MemFnUtil::memFn(&TwoKeyHashMap::assertInvariants, this));
#endif

    // obtain a iterator to the next element
    iterator next = ++iterator(it.d_keyIndex,
                               const_cast<TwoKeyHashMap*>(it.d_map_p),
                               const_cast<Node*>(it.d_node_p));

    // remove key1-to-node link
    d_k1Map.erase(it.d_node_p->d_k1MapIter);

    // remove key2-to-node link
    d_k2Map.erase(it.d_node_p->d_k2MapIter);

    // erase node
    d_nodeAllocator->deleteObject(it.d_node_p);

    // return iterator to the next element
    return next;
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline int
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::eraseByKey1(const first_key_type& k1)
{
#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
    // POSTCONDITIONS
    BDLB_SCOPEEXIT_PROCTOR(
        invariantsGuard,
        bdlf::MemFnUtil::memFn(&TwoKeyHashMap::assertInvariants, this));
#endif

    iterator it = findByKey1(k1);
    return (it == end()) ? -1 : (erase(it), 0);
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline int
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::eraseByKey2(const second_key_type& k2)
{
#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
    // POSTCONDITIONS
    BDLB_SCOPEEXIT_PROCTOR(
        invariantsGuard,
        bdlf::MemFnUtil::memFn(&TwoKeyHashMap::assertInvariants, this));
#endif

    iterator it = findByKey2(k2);
    return (it == end()) ? -1 : (erase(it), 0);
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline void TwoKeyHashMap<K1, K2, VALUE, H1, H2>::clear() BSLS_KEYWORD_NOEXCEPT
{
#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
    // POSTCONDITIONS
    BDLB_SCOPEEXIT_PROCTOR(
        invariantsGuard,
        bdlf::MemFnUtil::memFn(&TwoKeyHashMap::assertInvariants, this));
#endif

    // erase all nodes
    for (K1MapIter it = d_k1Map.begin(); it != d_k1Map.end(); ++it) {
        d_nodeAllocator->deleteObject(it->second);
    }

    // clear key-to-node maps
    d_k1Map.clear();
    d_k2Map.clear();
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline void TwoKeyHashMap<K1, K2, VALUE, H1, H2>::swap(TwoKeyHashMap& other)
    BSLS_KEYWORD_NOEXCEPT
{
    using bsl::swap;

    swap(d_k1Map, other.d_k1Map);
    swap(d_k2Map, other.d_k2Map);
    swap(d_nodeAllocator, other.d_nodeAllocator);
}

// ACCESSORS
template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::iterator
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::begin(KeyIndex keyIndex)
    BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(keyIndex == e_FIRST_KEY || keyIndex == e_SECOND_KEY);

    if (empty()) {
        return end(keyIndex);  // RETURN
    }

    return iterator(keyIndex,
                    this,
                    keyIndex == e_FIRST_KEY ? d_k1Map.begin()->second
                                            : d_k2Map.begin()->second);
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::begin(KeyIndex keyIndex) const
    BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(keyIndex == e_FIRST_KEY || keyIndex == e_SECOND_KEY);

    if (empty()) {
        return end(keyIndex);  // RETURN
    }

    return const_iterator(keyIndex,
                          this,
                          keyIndex == e_FIRST_KEY ? d_k1Map.begin()->second
                                                  : d_k2Map.begin()->second);
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::cbegin(KeyIndex keyIndex) const
    BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(keyIndex == e_FIRST_KEY || keyIndex == e_SECOND_KEY);

    return begin(keyIndex);
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::iterator
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::end(KeyIndex keyIndex)
    BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(keyIndex == e_FIRST_KEY || keyIndex == e_SECOND_KEY);

    return iterator(keyIndex, this, static_cast<Node*>(0));
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::end(KeyIndex keyIndex) const
    BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(keyIndex == e_FIRST_KEY || keyIndex == e_SECOND_KEY);

    return const_iterator(keyIndex, this, static_cast<Node*>(0));
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::cend(KeyIndex keyIndex) const
    BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(keyIndex == e_FIRST_KEY || keyIndex == e_SECOND_KEY);

    return end(keyIndex);
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::iterator
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::findByKey1(const first_key_type& k1)
{
    K1MapIter it = d_k1Map.find(k1);
    return (it == d_k1Map.end()) ? end(e_FIRST_KEY)
                                 : iterator(e_FIRST_KEY, this, it->second);
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::findByKey1(
    const first_key_type& k1) const
{
    K1MapConstIter it = d_k1Map.find(k1);
    return (it == d_k1Map.end())
               ? end(e_FIRST_KEY)
               : const_iterator(e_FIRST_KEY, this, it->second);
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::iterator
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::findByKey2(const second_key_type& k2)
{
    K2MapIter it = d_k2Map.find(k2);
    return (it == d_k2Map.end()) ? end(e_SECOND_KEY)
                                 : iterator(e_SECOND_KEY, this, it->second);
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::findByKey2(
    const second_key_type& k2) const
{
    K2MapConstIter it = d_k2Map.find(k2);
    return (it == d_k2Map.end())
               ? end(e_SECOND_KEY)
               : const_iterator(e_SECOND_KEY, this, it->second);
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline bool
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::empty() const BSLS_KEYWORD_NOEXCEPT
{
    return d_k1Map.empty();
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::size_type
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::size() const BSLS_KEYWORD_NOEXCEPT
{
    return d_k1Map.size();
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::size_type
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::max_size() const BSLS_KEYWORD_NOEXCEPT
{
    return bsl::min(d_k1Map.max_size(), d_k2Map.max_size());
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::first_hasher
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::hash1() const
{
    return d_k1Map.hash_function();
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::second_hasher
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::hash2() const
{
    return d_k2Map.hash_function();
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline bslma::Allocator*
TwoKeyHashMap<K1, K2, VALUE, H1, H2>::allocator() const
{
    return d_k1Map.get_allocator().mechanism();
}

#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
template <class K1, class K2, class VALUE, class H1, class H2>
inline bool TwoKeyHashMap<K1, K2, VALUE, H1, H2>::assertInvariants() const
{
    BSLS_ASSERT_SAFE(d_k1Map.size() == d_k2Map.size());

    for (K1MapConstIter it = d_k1Map.begin(); it != d_k1Map.end(); ++it) {
        const Node* p = it->second;
        BSLS_ASSERT_SAFE(p);
        BSLS_ASSERT_SAFE(p->d_k1MapIter->second == p);
        BSLS_ASSERT_SAFE(p->d_k2MapIter->second == p);
    }

    for (K2MapConstIter it = d_k2Map.begin(); it != d_k2Map.end(); ++it) {
        const Node* p = it->second;
        BSLS_ASSERT_SAFE(p);
        BSLS_ASSERT_SAFE(p->d_k1MapIter->second == p);
        BSLS_ASSERT_SAFE(p->d_k2MapIter->second == p);
    }

    return true;
}
#endif  // BSLS_ASSERT_SAFE_IS_ACTIVE

// ------------------------
// class TwoKeyHashMapValue
// ------------------------

// MANIPULATORS
template <class MAP>
inline typename MAP::mapped_type&
TwoKeyHashMapValue<MAP>::value() BSLS_KEYWORD_NOEXCEPT
{
    return this->d_value.object();
}

// ACCESSORS
template <class MAP>
inline const typename MAP::first_key_type&
TwoKeyHashMapValue<MAP>::key1() const BSLS_KEYWORD_NOEXCEPT
{
    return this->d_k1MapIter->first;
}

template <class MAP>
inline const typename MAP::second_key_type&
TwoKeyHashMapValue<MAP>::key2() const BSLS_KEYWORD_NOEXCEPT
{
    return this->d_k2MapIter->first;
}

template <class MAP>
inline const typename MAP::mapped_type&
TwoKeyHashMapValue<MAP>::value() const BSLS_KEYWORD_NOEXCEPT
{
    return this->d_value.object();
}

}  // close package namespace

// FREE OPERATORS
template <class K1, class K2, class VALUE, class H1, class H2>
inline void
mwcc::swap(TwoKeyHashMap<K1, K2, VALUE, H1, H2>& lhs,
           TwoKeyHashMap<K1, K2, VALUE, H1, H2>& rhs) BSLS_KEYWORD_NOEXCEPT
{
    lhs.swap(rhs);
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline bool mwcc::operator==(const TwoKeyHashMap<K1, K2, VALUE, H1, H2>& lhs,
                             const TwoKeyHashMap<K1, K2, VALUE, H1, H2>& rhs)
{
    if (lhs.size() != rhs.size()) {
        return false;  // RETURN
    }
    typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator lIt =
        lhs.cbegin();
    for (; lIt != lhs.cend(); ++lIt) {
        typename TwoKeyHashMap<K1, K2, VALUE, H1, H2>::const_iterator rIt =
            rhs.findByKey1(lIt->key1());
        if (rIt == rhs.cend() || lIt->key2() != rIt->key2() ||
            lIt->value() != rIt->value()) {
            return false;  // RETURN
        }
    }
    return true;
}

template <class K1, class K2, class VALUE, class H1, class H2>
inline bool mwcc::operator!=(const TwoKeyHashMap<K1, K2, VALUE, H1, H2>& lhs,
                             const TwoKeyHashMap<K1, K2, VALUE, H1, H2>& rhs)
{
    return !(lhs == rhs);
}

template <class MAP>
inline bool mwcc::operator==(const TwoKeyHashMapValue<MAP>& lhs,
                             const TwoKeyHashMapValue<MAP>& rhs)
{
    return lhs.key1() == rhs.key1() && lhs.key2() == rhs.key2() &&
           lhs.value() == rhs.value();
}

template <class MAP>
inline bool mwcc::operator!=(const TwoKeyHashMapValue<MAP>& lhs,
                             const TwoKeyHashMapValue<MAP>& rhs)
{
    return !(lhs == rhs);
}

}  // close enterprise namespace

#endif
