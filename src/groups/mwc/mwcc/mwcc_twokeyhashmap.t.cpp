// Copyright 2023 Bloomberg Finance L.P.
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

// mwcc_twokeyhashmap.t.cpp                                           -*-C++-*-
#include <mwcc_twokeyhashmap.h>

// BDE
#include <bsl_algorithm.h>   // bsl::swap
#include <bsl_functional.h>  // bsl::cref, bsl::hash
#include <bsl_iostream.h>
#include <bsl_sstream.h>
#include <bsl_string.h>
#include <bsl_utility.h>  // bsl::pair
#include <bsl_vector.h>
#include <bslma_testallocator.h>
#include <bslmf_movableref.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;

//=============================================================================
//                             TEST PLAN
//-----------------------------------------------------------------------------
// The component under test: mwcc_twokeyhashmap
//
//-----------------------------------------------------------------------------
// [0]  usage example
// [1]  basic creators
// [2]  copyAndMove
// [3]  insert
// [4]  erase
// [5]  eraseByKey1
// [6]  eraseByKey2
// [7]  clear
// [8]  swap
// [9]  rangeObservers
// [10] findByKey1
// [11] findByKey2
// [12] sizeObservers
//-----------------------------------------------------------------------------

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// A wrapper around `bsl::hash<T>` having an ID, to be able to tell one
/// object from another.
template <class T>
class TestHasher {
  private:
    // PRIVATE DATA
    int d_id;

  public:
    // CREATORS
    explicit TestHasher(int id = 0) BSLS_KEYWORD_NOEXCEPT : d_id(id) {}

  public:
    // MANIPULATORS
    size_t operator()(const T& value) const BSLS_KEYWORD_NOEXCEPT
    {
        return bsl::hash<T>()(value);
    }

    int id() const BSLS_KEYWORD_NOEXCEPT { return d_id; }
};

/// An "Integer" that may throw on copy, or equality comparison.  A
/// specialization of `bsl::hash` is defined for this type.
class ThrowingInteger {
  public:
    // TYPES
    struct CopyException {};

    struct ComparisonException {};

  private:
    // PRIVATE DATA
    int d_value;

    bool d_throwOnCopy;

    bool d_throwOnComparison;

  public:
    // CREATORS
    ThrowingInteger(int  value             = 0,
                    bool throwOnCopy       = false,
                    bool throwOnComparison = false) BSLS_KEYWORD_NOEXCEPT
    // IMPLICIT
    : d_value(value),
      d_throwOnCopy(throwOnCopy),
      d_throwOnComparison(throwOnComparison)
    {
    }

    ThrowingInteger(const ThrowingInteger& original)
    : d_value(original.d_value)
    , d_throwOnCopy(original.d_throwOnCopy)
    , d_throwOnComparison(original.d_throwOnComparison)
    {
        if (original.d_throwOnCopy) {
            throw CopyException();
        }
    }

    ThrowingInteger& operator=(const ThrowingInteger& original)
    {
        if (original.d_throwOnCopy) {
            throw CopyException();
        }

        d_value             = original.d_value;
        d_throwOnCopy       = original.d_throwOnCopy;
        d_throwOnComparison = original.d_throwOnComparison;

        return *this;
    }

  public:
    // MANIPULATORS
    int& value() BSLS_KEYWORD_NOEXCEPT { return d_value; }

  public:
    // ACCESSORS
    int value() const BSLS_KEYWORD_NOEXCEPT { return d_value; }

    friend bool operator==(const ThrowingInteger& lhs,
                           const ThrowingInteger& rhs)
    {
        if (lhs.d_throwOnComparison || rhs.d_throwOnComparison) {
            throw ComparisonException();
        }

        return lhs.d_value == rhs.d_value;
    }
};

}  // close anonymous namespace

namespace bsl {

template <>
struct hash<ThrowingInteger> {
    // MANIPULATORS
    size_t operator()(const ThrowingInteger& value) const BSLS_KEYWORD_NOEXCEPT
    {
        return bsl::hash<int>()(value.value());
    }
};

}  // close bsl namespace

//=============================================================================
//                              TEST CASES
//-----------------------------------------------------------------------------

static void test0_usageExample()
// ------------------------------------------------------------------------
// USAGE EXAMPLE
//
// Concerns:
//   Test that the usage example provided in the documentation of the
//   component is correct.
//
// Plan:
//   Implement the usage example.
//
// Testing:
//   Usage example
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // create a map
    typedef mwcc::TwoKeyHashMap<bsl::string, int, bsl::string> TestMap;
    TestMap                                                    map(&alloc);

    // add elements
    map.insert("first", 1, "Hello");

    // size
    ASSERT_EQ(map.size(), 1u);

    bsl::pair<TestMap::iterator, TestMap::InsertResult> p =
        map.insert("second", 2, "World");
    BSLS_ASSERT(p.second == TestMap::e_INSERTED);
    BSLS_ASSERT(p.first != map.end());

    // size
    ASSERT_EQ(map.size(), 2u);

    // find elements using 'findByKey()' methods
    {
        TestMap::iterator iter = map.findByKey1("first");
        BSLS_ASSERT(iter != map.end());
        BSLS_ASSERT(iter->key1() == "first");
        BSLS_ASSERT(iter->key2() == 1);
        BSLS_ASSERT(iter->value() == "Hello");

        iter = map.findByKey2(2);
        BSLS_ASSERT(iter != map.end());
        BSLS_ASSERT(iter->key1() == "second");
        BSLS_ASSERT(iter->key2() == 2);
        BSLS_ASSERT(iter->value() == "World");

        iter = map.findByKey2(999);
        BSLS_ASSERT(iter == map.end());
    }

    // iterating 1
    for (TestMap::iterator iter = map.begin(); iter != map.end(); ++iter) {
        bsl::cout << iter->key1() << " : " << iter->key2() << " => "
                  << iter->value() << bsl::endl;
    }

    // iterating 2
    for (TestMap::iterator iter = map.begin(TestMap::e_SECOND_KEY);
         iter != map.end();
         ++iter) {
        // ...
    }

    // erasing elements
    {
        BSLS_ASSERT(map.size() == 2);
        TestMap::iterator iter = map.findByKey1("first");
        map.erase(iter);
        BSLS_ASSERT(map.size() == 1);
    }

    // clearing the map
    {
        map.clear();
        ASSERT_EQ(map.size(), 0u);
        ASSERT(map.begin() == map.end());
        ASSERT(map.begin() == map.end());
    }
}

static void test1_basicCreators()
// ------------------------------------------------------------------------
// BASIC CREATORS
//
// Concerns:
//   Ensure proper behavior of basic creator methods.
//
// Plan:
//   1. Create an instance of 'TwoKeyHashMap', destroy it. Check that no
//      memory is leaked.
//
//   2. Default-construct an instance of 'TwoKeyHashMap'. Check
//      postconditions.
//
//   3. Construct an instance of 'TwoKeyHashMap' providing one hasher
//      object. Check postconditions.
//
//   4. Construct an instance of 'TwoKeyHashMap' providing two hasher
//      objects. Check postconditions.
//
// Testing:
//   Destructor
//   Default constructor
//   One-hasher constructor
//   Two-hasher constructor
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. destructor
    {
        mwcc::TwoKeyHashMap<int, int, int> map(&alloc);
    }

    // 2. default constructor
    {
        mwcc::TwoKeyHashMap<int, int, int, TestHasher<int>, TestHasher<int> >
            map(&alloc);

        ASSERT_EQ(map.empty(), true);
        ASSERT_EQ(map.size(), 0u);
        ASSERT_EQ(map.begin() == map.end(), true);
        ASSERT_EQ(map.hash1().id(), 0);
        ASSERT_EQ(map.hash2().id(), 0);
        ASSERT_EQ(map.allocator() == &alloc, true);
    }

    // 3. one-hasher constructor
    {
        mwcc::TwoKeyHashMap<int, int, int, TestHasher<int>, TestHasher<int> >
            map(TestHasher<int>(1), &alloc);

        ASSERT_EQ(map.empty(), true);
        ASSERT_EQ(map.size(), 0u);
        ASSERT_EQ(map.begin() == map.end(), true);
        ASSERT_EQ(map.hash1().id(), 1);
        ASSERT_EQ(map.hash2().id(), 0);
        ASSERT_EQ(map.allocator() == &alloc, true);
    }

    // 4. two-hasher constructor
    {
        mwcc::TwoKeyHashMap<int, int, int, TestHasher<int>, TestHasher<int> >
            map(TestHasher<int>(1), TestHasher<int>(2), &alloc);

        ASSERT_EQ(map.empty(), true);
        ASSERT_EQ(map.size(), 0u);
        ASSERT_EQ(map.begin() == map.end(), true);
        ASSERT_EQ(map.hash1().id(), 1);
        ASSERT_EQ(map.hash2().id(), 2);
        ASSERT_EQ(map.allocator() == &alloc, true);
    }
}

static void test2_copyAndMove()
// ------------------------------------------------------------------------
// COPY AND MOVE
//
// Concerns:
//   Ensure proper behavior of the copy constructor and the copy assignment
//   operator.
//
// Plan:
//   1. Create a map.  Add elements.  Copy the map using the copy
//      constructor.  Check that the copy's contents is identical to that
//      of the original object.
//
//   2. Create a map.  Add elements.  Move the map using the move
//      constructor.  Check that the original object's contents was moved
//      to the moved-to object.
//
//   3. Create a map.  Add elements.  Copy the map using the copy
//      assignament operator.  Check that the copy's contents is identical
//      to that of the original object.
//
//   4. Create a map.  Add elements.  Move the map using the move
//      assignment operator.  Check that the original object's contents was
//      moved to the moved-to object.
//
// Testing:
//   Copy constructor
//   Move constructor
//   Copy assignment operator
//   Move assignment operator
// ------------------------------------------------------------------------
{
    typedef mwcc::TwoKeyHashMap<int, bsl::string, bsl::string> Map;

    bslma::TestAllocator alloc;
    Map                  original(&alloc);

    // insert 5 elements
    original.insert(1, "first", "value1");
    original.insert(2, "second", "value2");
    original.insert(3, "third", "value3");
    original.insert(4, "forth", "value4");
    original.insert(5, "fifth", "value5");

    ASSERT_EQ(original.size(), 5u);
    ASSERT_EQ(original.findByKey1(1) != original.end() &&
                  original.findByKey1(2) != original.end() &&
                  original.findByKey1(3) != original.end() &&
                  original.findByKey1(4) != original.end() &&
                  original.findByKey1(5) != original.end(),
              true);

    // 1. copy construction
    {
        // do copy
        Map copy(original, &alloc);

        // the copy's contents is identical to original's
        ASSERT_EQ(copy.size(), 5u);

        ASSERT_EQ(copy.findByKey1(1) != copy.end() &&
                      *original.findByKey1(1) == *copy.findByKey1(1),
                  true);

        ASSERT_EQ(copy.findByKey1(2) != copy.end() &&
                      *original.findByKey1(2) == *copy.findByKey1(2),
                  true);

        ASSERT_EQ(copy.findByKey1(3) != copy.end() &&
                      *original.findByKey1(3) == *copy.findByKey1(3),
                  true);

        ASSERT_EQ(copy.findByKey1(4) != copy.end() &&
                      *original.findByKey1(4) == *copy.findByKey1(4),
                  true);

        ASSERT_EQ(copy.findByKey1(5) != copy.end() &&
                      *original.findByKey1(5) == *copy.findByKey1(5),
                  true);
    }

    // 2. move construction
    {
        // make a copy of 'original' that we will then move
        Map movedFrom(original, &alloc);

        // do move
        Map movedTo(bslmf::MovableRefUtil::move(movedFrom), &alloc);

        // the moved-to object contents is identical to original's
        ASSERT_EQ(movedTo.size(), 5u);

        ASSERT_EQ(movedTo.findByKey1(1) != movedTo.end() &&
                      *original.findByKey1(1) == *movedTo.findByKey1(1),
                  true);

        ASSERT_EQ(movedTo.findByKey1(2) != movedTo.end() &&
                      *original.findByKey1(2) == *movedTo.findByKey1(2),
                  true);

        ASSERT_EQ(movedTo.findByKey1(3) != movedTo.end() &&
                      *original.findByKey1(3) == *movedTo.findByKey1(3),
                  true);

        ASSERT_EQ(movedTo.findByKey1(4) != movedTo.end() &&
                      *original.findByKey1(4) == *movedTo.findByKey1(4),
                  true);

        ASSERT_EQ(movedTo.findByKey1(5) != movedTo.end() &&
                      *original.findByKey1(5) == *movedTo.findByKey1(5),
                  true);
    }

    // 3. copy assignment
    {
        Map copy(&alloc);

        // insert elements into copy
        copy.insert(1, "first", "value1");
        copy.insert(999, "999-th", "value999");
        ASSERT_EQ(copy.size(), 2u);

        // do copy
        copy = original;

        // the copy's contents is identical to original's
        ASSERT_EQ(copy.size(), 5u);

        ASSERT_EQ(copy.findByKey1(1) != copy.end() &&
                      *original.findByKey1(1) == *copy.findByKey1(1),
                  true);

        ASSERT_EQ(copy.findByKey1(2) != copy.end() &&
                      *original.findByKey1(2) == *copy.findByKey1(2),
                  true);

        ASSERT_EQ(copy.findByKey1(3) != copy.end() &&
                      *original.findByKey1(3) == *copy.findByKey1(3),
                  true);

        ASSERT_EQ(copy.findByKey1(4) != copy.end() &&
                      *original.findByKey1(4) == *copy.findByKey1(4),
                  true);

        ASSERT_EQ(copy.findByKey1(5) != copy.end() &&
                      *original.findByKey1(5) == *copy.findByKey1(5),
                  true);
    }

    // 4. move assignment
    {
        // make a copy of 'original' that we will then move
        Map movedFrom(original, &alloc);

        Map movedTo(&alloc);

        // insert elements into 'movedTo'
        movedTo.insert(1, "first", "value1");
        movedTo.insert(999, "999-th", "value999");
        ASSERT_EQ(movedTo.size(), 2u);

        // do move
        movedTo = bslmf::MovableRefUtil::move(movedFrom);

        // the moved-to object contents is identical to original's
        ASSERT_EQ(movedTo.size(), 5u);

        ASSERT_EQ(movedTo.findByKey1(1) != movedTo.end() &&
                      *original.findByKey1(1) == *movedTo.findByKey1(1),
                  true);

        ASSERT_EQ(movedTo.findByKey1(2) != movedTo.end() &&
                      *original.findByKey1(2) == *movedTo.findByKey1(2),
                  true);

        ASSERT_EQ(movedTo.findByKey1(3) != movedTo.end() &&
                      *original.findByKey1(3) == *movedTo.findByKey1(3),
                  true);

        ASSERT_EQ(movedTo.findByKey1(4) != movedTo.end() &&
                      *original.findByKey1(4) == *movedTo.findByKey1(4),
                  true);

        ASSERT_EQ(movedTo.findByKey1(5) != movedTo.end() &&
                      *original.findByKey1(5) == *movedTo.findByKey1(5),
                  true);
    }
}

static void test3_insert()
// ------------------------------------------------------------------------
// INSERT
//
// Concerns:
//   Ensure proper behavior of the 'insert()' method.
//
// Plan:
//   1. Create a map. Add elements via a call to 'insert()', specifying an
//      unique first key, an unique second key, a mapped value and a key
//      index.  Check that:
//      - The returned result code is 'e_INSERTED';
//      - The returned iterator refers to the inserted element;
//      - The returned iterator is bound the specified key index;
//      - 'insert()' has not affected other elements, already present in
//        the map.
//
//   2. Create a map. Add elements. Call 'insert()', specifying an existing
//      first key, an unique second key, a mapped value and a key index.
//      Check that:
//      - The returned result code is 'e_FIRST_KEY_EXIST';
//      - The returned iterator refers to the element indexed by the first
//        key;
//      - The returned iterator is bound to the first key (regardless of
//        the value of the specified key index);
//      - No element has been added to the map;
//      - 'insert()' has not affected other elements, already present in
//        the map.
//
//   3. Create a map. Add elements. Call 'insert()', specifying an unique
//      first key, an existing second key, a mapped value and a key index.
//      Check that:
//      - The returned result code is 'e_SECOND_KEY_EXIST';
//      - The returned iterator refers to the element indexed by the second
//        key;
//      - The returned iterator is bound to the second key (regardless of
//        the value of the specified key index);
//      - No element has been added to the map;
//      - 'insert()' has not affected other elements, already present in
//        the map.
//
//   4. Create a map. Add elements. Call 'insert()', specifying an unique
//      first key, an unique second key and mapped value, given that one
//      of these three object will throw on copy.  Check that the insert
//      operation had no effect.
//
// Testing:
//   insert()
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. general case
    {
        typedef mwcc::TwoKeyHashMap<int, bsl::string, bsl::string> Map;
        typedef bsl::pair<Map::iterator, Map::InsertResult>        InsertPair;

        Map map(&alloc);

        // insert first element
        InsertPair pair1 = map.insert(1, "first", "value1", Map::e_FIRST_KEY);

        Map::InsertResult result1   = pair1.second;
        Map::iterator     iterator1 = pair1.first;

        // element inserted
        ASSERT_EQ(map.size(), 1u);
        ASSERT_EQ(result1, Map::e_INSERTED);

        // returned iterator refers to the inserted element and is bound to the
        // first key
        ASSERT_EQ(iterator1 == map.end(), false);
        ASSERT_EQ(iterator1.keyIndex(), Map::e_FIRST_KEY);
        ASSERT_EQ(iterator1->key1(), 1);
        ASSERT_EQ(iterator1->key2(), "first");
        ASSERT_EQ(iterator1->value(), "value1");

        // insert second element
        InsertPair pair2 =
            map.insert(2, "second", "value2", Map::e_SECOND_KEY);

        Map::InsertResult result2   = pair2.second;
        Map::iterator     iterator2 = pair2.first;

        // element inserted
        ASSERT_EQ(map.size(), 2u);
        ASSERT_EQ(result2, Map::e_INSERTED);

        // returned iterator refers to the inserted element and is bound to the
        // second key
        ASSERT_EQ(iterator2 == map.end(), false);
        ASSERT_EQ(iterator2.keyIndex(), Map::e_SECOND_KEY);
        ASSERT_EQ(iterator2->key1(), 2);
        ASSERT_EQ(iterator2->key2(), "second");
        ASSERT_EQ(iterator2->value(), "value2");

        // the first element is intact
        ASSERT_EQ(map.size(), 2u);
        ASSERT_EQ(iterator1->key1(), 1);
        ASSERT_EQ(iterator1->key2(), "first");
        ASSERT_EQ(iterator1->value(), "value1");
    }

    // 2. first key already exists
    {
        typedef mwcc::TwoKeyHashMap<int, bsl::string, bsl::string> Map;

        Map map(&alloc);

        // add elements
        map.insert(1, "first", "value1");

        // try insert
        bsl::pair<Map::iterator, Map::InsertResult> pair =
            map.insert(1, "second", "value2", Map::e_SECOND_KEY);

        // nothing inserted
        ASSERT_EQ(map.size(), 1u);

        // check returned values and that the map contents hasn't changed
        Map::InsertResult result   = pair.second;
        Map::iterator     iterator = pair.first;

        ASSERT_EQ(result, Map::e_FIRST_KEY_EXISTS);

        ASSERT_EQ(iterator == map.end(), false);
        ASSERT_EQ(iterator.keyIndex(), Map::e_FIRST_KEY);
        ASSERT_EQ(iterator->key1(), 1);
        ASSERT_EQ(iterator->key2(), "first");
        ASSERT_EQ(iterator->value(), "value1");
    }

    // 3. second key already exists
    {
        typedef mwcc::TwoKeyHashMap<int, bsl::string, bsl::string> Map;

        Map map(&alloc);

        // add elements
        map.insert(1, "first", "value1");

        // try insert
        bsl::pair<Map::iterator, Map::InsertResult> pair =
            map.insert(2, "first", "value2", Map::e_FIRST_KEY);

        // check returned values and that the map contents hasn't changed
        Map::InsertResult result   = pair.second;
        Map::iterator     iterator = pair.first;

        ASSERT_EQ(result, Map::e_SECOND_KEY_EXISTS);

        ASSERT_EQ(iterator == map.end(), false);
        ASSERT_EQ(iterator.keyIndex(), Map::e_SECOND_KEY);
        ASSERT_EQ(iterator->key1(), 1);
        ASSERT_EQ(iterator->key2(), "first");
        ASSERT_EQ(iterator->value(), "value1");
    }

    // 4. check exception safety
    {
        typedef mwcc::
            TwoKeyHashMap<ThrowingInteger, ThrowingInteger, ThrowingInteger>
                Map;

        Map map(&alloc);

        // insert one element to the map
        map.insert(ThrowingInteger(0, false),   // doesn't throw
                   ThrowingInteger(0, false),   // doesn't throw
                   ThrowingInteger(0, false));  // doesn't throw

        // try inserting another element, given that the first key copy
        // constructor throws
        bool exceptionThrown = false;
        try {
            map.insert(ThrowingInteger(1, true),    // throws on copy
                       ThrowingInteger(1, false),   // doesn't throw
                       ThrowingInteger(1, false));  // doesn't throw
        }
        catch (const ThrowingInteger::CopyException&) {
            exceptionThrown = true;
        }

        // an exception was thrown
        ASSERT_EQ(exceptionThrown, true);

        // the map state hasn't changed
        ASSERT_EQ(map.size(), 1u);
        ASSERT_EQ(map.begin()->key1().value(), 0);
        ASSERT_EQ(map.begin()->key2().value(), 0);
        ASSERT_EQ(map.begin()->value().value(), 0);

        // try inserting another element, given that the second key copy
        // constructor throws
        exceptionThrown = false;
        try {
            map.insert(ThrowingInteger(1, false),   // doesn't throw
                       ThrowingInteger(1, true),    // throws on copy
                       ThrowingInteger(1, false));  // doesn't throw
        }
        catch (const ThrowingInteger::CopyException&) {
            exceptionThrown = true;
        }

        // an exception was thrown
        ASSERT_EQ(exceptionThrown, true);

        // the map state hasn't changed
        ASSERT_EQ(map.size(), 1u);
        ASSERT_EQ(map.begin()->key1().value(), 0);
        ASSERT_EQ(map.begin()->key2().value(), 0);
        ASSERT_EQ(map.begin()->value().value(), 0);

        // try inserting another element, given that the value copy
        // constructor throws
        exceptionThrown = false;
        try {
            map.insert(ThrowingInteger(1, false),  // doesn't throw
                       ThrowingInteger(1, false),  // doesn't throw
                       ThrowingInteger(1, true));  // throws on copy
        }
        catch (const ThrowingInteger::CopyException&) {
            exceptionThrown = true;
        }

        // an exception was thrown
        ASSERT_EQ(exceptionThrown, true);

        // the map state hasn't changed
        ASSERT_EQ(map.size(), 1u);
        ASSERT_EQ(map.begin()->key1().value(), 0);
        ASSERT_EQ(map.begin()->key2().value(), 0);
        ASSERT_EQ(map.begin()->value().value(), 0);
    }
}

static void test4_erase()
// ------------------------------------------------------------------------
// ERASE
//
// Concerns:
//   Ensure proper behavior of the 'erase()' method.
//
// Plan:
//   1. Create a map, add elements. Remove one of those elements via a call
//      to 'erase()', specifying an iterator bound to the first key.  Check
//      that:
//      - The element referred to by the specified iterator was removed;
//      - The order of remaining elements, as defined by the first and by
//        the second key, hasn't changed.
//
//   2. Create a map, add elements. Remove one of those elements via a call
//      to 'erase()', specifying an iterator bound to the second key.
//      Check that:
//      - The element referred to by the specified iterator was removed;
//      - The order of remaining elements, as defined by the first and by
//        the second key, hasn't changed.
//
//   3. Create a map, add elements. Remove one of those elements via a call
//      to 'erase()', specifying an iterator bound to the first key.  Check
//      that:
//      - The returned iterator refers to the next element after the
//        removed one, as ordered by the first key.  Or is the past-the-end
//        iterator, if there is no next element;
//      - The returned iterator is bound the first key.
//
//   4. Create a map, add elements. Remove one of those elements via a call
//      to 'erase()', specifying an iterator bound to the second key.
//      Check that:
//      - The returned iterator refers to the next element after the
//        removed one, as ordered by the second key.  Or is the past-the-
//        end iterator, if there is no next element;
//      - The returned iterator is bound the first key.
//
// Testing:
//   erase()
// ------------------------------------------------------------------------
{
    typedef mwcc::TwoKeyHashMap<int, bsl::string, bsl::string> Map;

    bslma::TestAllocator alloc;

    // 1. erase by iterator bound to first key
    {
        Map::iterator it;
        Map           map(&alloc);

        // insert 5 elements
        map.insert(1, "first", "value1");
        map.insert(2, "second", "value2");
        map.insert(3, "third", "value3");
        map.insert(4, "forth", "value4");
        map.insert(5, "fifth", "value5");
        ASSERT_EQ(map.size(), 5u);

        // make a list of elements ordered according to the first key
        it = map.begin(Map::e_FIRST_KEY);
        bsl::vector<const Map::value_type*> elementsByKey1(5, &alloc);
        elementsByKey1[0] = &*it++;
        elementsByKey1[1] = &*it++;
        elementsByKey1[2] = &*it++;
        elementsByKey1[3] = &*it++;
        elementsByKey1[4] = &*it++;

        // make a list of elements ordered according to the second key
        it = map.begin(Map::e_SECOND_KEY);
        bsl::vector<const Map::value_type*> elementsByKey2(5, &alloc);
        elementsByKey2[0] = &*it++;
        elementsByKey2[1] = &*it++;
        elementsByKey2[2] = &*it++;
        elementsByKey2[3] = &*it++;
        elementsByKey2[4] = &*it++;

        // find element with first key equal to 3
        Map::iterator erased = map.findByKey1(3);
        ASSERT_EQ(erased == map.end(), false)
        ASSERT_EQ(erased->key1(), 3);
        ASSERT_EQ(erased.keyIndex(), Map::e_FIRST_KEY);

        const Map::value_type* erased_p = &*erased;

        // remove that element by an iterator bound to the first key
        map.erase(erased);

        // element removed
        ASSERT_EQ(map.size(), 4u);
        ASSERT_EQ(map.findByKey1(3) == map.end(), true);

        // order of remaining elements indexed by the first key hasn't changed
        it = map.begin(Map::e_FIRST_KEY);
        ASSERT_EQ(elementsByKey1[0] == erased_p || elementsByKey1[0] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey1[1] == erased_p || elementsByKey1[1] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey1[2] == erased_p || elementsByKey1[2] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey1[3] == erased_p || elementsByKey1[3] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey1[4] == erased_p || elementsByKey1[4] == &*it++,
                  true);

        // order of remaining elements indexed by the second key hasn't changed
        it = map.begin(Map::e_SECOND_KEY);
        ASSERT_EQ(elementsByKey2[0] == erased_p || elementsByKey2[0] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey2[1] == erased_p || elementsByKey2[1] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey2[2] == erased_p || elementsByKey2[2] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey2[3] == erased_p || elementsByKey2[3] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey2[4] == erased_p || elementsByKey2[4] == &*it++,
                  true);
    }

    // 2. erase by iterator bound to second key
    {
        Map::iterator it;
        Map           map(&alloc);

        // insert 5 elements
        map.insert(1, "first", "value1");
        map.insert(2, "second", "value2");
        map.insert(3, "third", "value3");
        map.insert(4, "forth", "value4");
        map.insert(5, "fifth", "value5");
        ASSERT_EQ(map.size(), 5u);

        // make a list of elements ordered according to the first key
        it = map.begin(Map::e_FIRST_KEY);
        bsl::vector<const Map::value_type*> elementsByKey1(5, &alloc);
        elementsByKey1[0] = &*it++;
        elementsByKey1[1] = &*it++;
        elementsByKey1[2] = &*it++;
        elementsByKey1[3] = &*it++;
        elementsByKey1[4] = &*it++;

        // make a list of elements ordered according to the second key
        it = map.begin(Map::e_SECOND_KEY);
        bsl::vector<const Map::value_type*> elementsByKey2(5, &alloc);
        elementsByKey2[0] = &*it++;
        elementsByKey2[1] = &*it++;
        elementsByKey2[2] = &*it++;
        elementsByKey2[3] = &*it++;
        elementsByKey2[4] = &*it++;

        // find element with second key equal to "third"
        Map::iterator erased = map.findByKey2("third");
        ASSERT_EQ(erased == map.end(), false)
        ASSERT_EQ(erased->key2(), "third");
        ASSERT_EQ(erased.keyIndex(), Map::e_SECOND_KEY);

        const Map::value_type* erased_p = &*erased;

        // remove that element by an iterator bound to the first key
        map.erase(erased);

        // element removed
        ASSERT_EQ(map.size(), 4u);
        ASSERT_EQ(map.findByKey2("third") == map.end(), true);

        // order of remaining elements indexed by the first key hasn't changed
        it = map.begin(Map::e_FIRST_KEY);
        ASSERT_EQ(elementsByKey1[0] == erased_p || elementsByKey1[0] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey1[1] == erased_p || elementsByKey1[1] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey1[2] == erased_p || elementsByKey1[2] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey1[3] == erased_p || elementsByKey1[3] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey1[4] == erased_p || elementsByKey1[4] == &*it++,
                  true);

        // order of remaining elements indexed by the second key hasn't changed
        it = map.begin(Map::e_SECOND_KEY);
        ASSERT_EQ(elementsByKey2[0] == erased_p || elementsByKey2[0] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey2[1] == erased_p || elementsByKey2[1] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey2[2] == erased_p || elementsByKey2[2] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey2[3] == erased_p || elementsByKey2[3] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey2[4] == erased_p || elementsByKey2[4] == &*it++,
                  true);
    }

    // 3. check return value when erasing by an iterator bound to the first key
    {
        Map::iterator it;
        Map           map(&alloc);

        // insert 5 elements
        map.insert(1, "first", "value1");
        map.insert(2, "second", "value2");
        map.insert(3, "third", "value3");
        map.insert(4, "forth", "value4");
        map.insert(5, "fifth", "value5");
        ASSERT_EQ(map.size(), 5u);

        while (!map.empty()) {
            // erase first element, as ordered by the first key
            Map::iterator nextAfterErased = map.erase(
                map.begin(Map::e_FIRST_KEY));

            // 'erase()' returned an iterator to the next element
            ASSERT_EQ(nextAfterErased == map.begin(Map::e_FIRST_KEY), true);
            ASSERT_EQ(nextAfterErased.keyIndex(), Map::e_FIRST_KEY);
        }
    }

    // 4. check return value when erasing by an iterator bound to the second
    //    key
    {
        Map::iterator it;
        Map           map(&alloc);

        // insert 5 elements
        map.insert(1, "first", "value1");
        map.insert(2, "second", "value2");
        map.insert(3, "third", "value3");
        map.insert(4, "forth", "value4");
        map.insert(5, "fifth", "value5");
        ASSERT_EQ(map.size(), 5u);

        while (!map.empty()) {
            // erase first element, as ordered by the second key
            Map::iterator nextAfterErased = map.erase(
                map.begin(Map::e_SECOND_KEY));

            // 'erase()' returned an iterator to the next element
            ASSERT_EQ(nextAfterErased == map.begin(Map::e_SECOND_KEY), true);
            ASSERT_EQ(nextAfterErased.keyIndex(), Map::e_SECOND_KEY);
        }
    }
}

static void test5_eraseByKey1()
// ------------------------------------------------------------------------
// ERASE BY KEY 1
//
// Concerns:
//   Ensure proper behavior of the 'eraseByKey1()' method.
//
// Plan:
//   1. Create a map, add elements. Remove one of those elements via a call
//      to 'eraseByKey1()', specifying an existing key.  Check that:
//      - The returned value is 0;
//      - The element indexed by the specified key was removed;
//      - The order of remaining elements, as defined by the first and by
//        the second key, hasn't changed.
//
//   2. Create a map, add elements.  Call 'eraseByKey1()', specifying a
//      non-existing key.  Check that:
//      - The returned value is not 0;
//      - No element was removed from the map;
//
//   3. Create a map, add elements.  Call 'eraseByKey1()', specifying an
//      existing key, given that the key object will throw on comparison.
//      Check that the erase operation had no effect.
//
// Testing:
//   eraseByKey1()
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. erase by first key
    {
        typedef mwcc::TwoKeyHashMap<int, bsl::string, bsl::string> Map;

        Map::iterator it;
        Map           map(&alloc);

        // insert 5 elements
        map.insert(1, "first", "value1");
        map.insert(2, "second", "value2");
        map.insert(3, "third", "value3");
        map.insert(4, "forth", "value4");
        map.insert(5, "fifth", "value5");
        ASSERT_EQ(map.size(), 5u);

        // make a list of elements ordered according to the first key
        it = map.begin(Map::e_FIRST_KEY);
        bsl::vector<const Map::value_type*> elementsByKey1(5, &alloc);
        elementsByKey1[0] = &*it++;
        elementsByKey1[1] = &*it++;
        elementsByKey1[2] = &*it++;
        elementsByKey1[3] = &*it++;
        elementsByKey1[4] = &*it++;

        // make a list of elements ordered according to the second key
        it = map.begin(Map::e_SECOND_KEY);
        bsl::vector<const Map::value_type*> elementsByKey2(5, &alloc);
        elementsByKey2[0] = &*it++;
        elementsByKey2[1] = &*it++;
        elementsByKey2[2] = &*it++;
        elementsByKey2[3] = &*it++;
        elementsByKey2[4] = &*it++;

        // find element with first key equal to 3
        Map::iterator erased = map.findByKey1(3);
        ASSERT_EQ(erased == map.end(), false)
        ASSERT_EQ(erased->key1(), 3);

        const Map::value_type* erased_p = &*erased;

        // remove element with first key equal to 3
        int eraseRc = map.eraseByKey1(3);

        // return value is 0
        ASSERT_EQ(eraseRc, 0);

        // element removed
        ASSERT_EQ(map.size(), 4u);
        ASSERT_EQ(map.findByKey1(3) == map.end(), true);

        // order of remaining elements indexed by the first key hasn't changed
        it = map.begin(Map::e_FIRST_KEY);
        ASSERT_EQ(elementsByKey1[0] == erased_p || elementsByKey1[0] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey1[1] == erased_p || elementsByKey1[1] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey1[2] == erased_p || elementsByKey1[2] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey1[3] == erased_p || elementsByKey1[3] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey1[4] == erased_p || elementsByKey1[4] == &*it++,
                  true);

        // order of remaining elements indexed by the second key hasn't changed
        it = map.begin(Map::e_SECOND_KEY);
        ASSERT_EQ(elementsByKey2[0] == erased_p || elementsByKey2[0] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey2[1] == erased_p || elementsByKey2[1] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey2[2] == erased_p || elementsByKey2[2] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey2[3] == erased_p || elementsByKey2[3] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey2[4] == erased_p || elementsByKey2[4] == &*it++,
                  true);
    }

    // 2. erase by first key, which doesn't exist
    {
        typedef mwcc::TwoKeyHashMap<int, bsl::string, bsl::string> Map;

        Map map(&alloc);

        // insert elements
        map.insert(1, "first", "value1");
        ASSERT_EQ(map.size(), 1u);

        // try erasing by key that doesn't exist
        int eraseRc = map.eraseByKey1(2);

        // returned value is non-zero
        ASSERT_NE(eraseRc, 0);

        // the map is intact
        ASSERT_EQ(map.size(), 1u);
        ASSERT_EQ(map.begin()->key1(), 1);
        ASSERT_EQ(map.begin()->key2(), "first");
        ASSERT_EQ(map.begin()->value(), "value1");
    }

    // 3. erase by first key, which exists, but throws on comparison
    {
        typedef mwcc::TwoKeyHashMap<ThrowingInteger, bsl::string, bsl::string>
            Map;

        Map map(&alloc);

        // insert elements
        map.insert(1, "first", "value1");
        ASSERT_EQ(map.size(), 1u);

        bool exceptionThrown = false;

        // try erasing by key that doesn't exist
        try {
            map.eraseByKey1(ThrowingInteger(1, false, true));
        }
        catch (const ThrowingInteger::ComparisonException&) {
            exceptionThrown = true;
        }

        // exception was thrown
        ASSERT_EQ(exceptionThrown, true);

        // the map is intact
        ASSERT_EQ(map.size(), 1u);
        ASSERT_EQ(map.begin()->key1().value(), 1);
        ASSERT_EQ(map.begin()->key2(), "first");
        ASSERT_EQ(map.begin()->value(), "value1");
    }
}

static void test6_eraseByKey2()
// ------------------------------------------------------------------------
// ERASE BY KEY 2
//
// Concerns:
//   Ensure proper behavior of the 'eraseByKey2()' method.
//
// Plan:
//   1. Create a map, add elements. Remove one of those elements via a call
//      to 'eraseByKey2()', specifying an existing key.  Check that:
//      - The returned value is 0;
//      - The element indexed by the specified key was removed;
//      - The order of remaining elements, as defined by the first and by
//        the second key, hasn't changed.
//
//   2. Create a map, add elements.  Call 'eraseByKey2()', specifying a
//      non-existing key.  Check that:
//      - The returned value is not 0;
//      - No element was removed from the map;
//
//   3. Create a map, add elements.  Call 'eraseByKey2()', specifying an
//      existing key, given that the key object will throw on comparison.
//      Check that the erase operation had no effect.
//
// Testing:
//   eraseByKey2()
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. erase by second key
    {
        typedef mwcc::TwoKeyHashMap<int, bsl::string, bsl::string> Map;

        Map::iterator it;
        Map           map(&alloc);

        // insert 5 elements
        map.insert(1, "first", "value1");
        map.insert(2, "second", "value2");
        map.insert(3, "third", "value3");
        map.insert(4, "forth", "value4");
        map.insert(5, "fifth", "value5");
        ASSERT_EQ(map.size(), 5u);

        // make a list of elements ordered according to the first key
        it = map.begin(Map::e_FIRST_KEY);
        bsl::vector<const Map::value_type*> elementsByKey1(5, &alloc);
        elementsByKey1[0] = &*it++;
        elementsByKey1[1] = &*it++;
        elementsByKey1[2] = &*it++;
        elementsByKey1[3] = &*it++;
        elementsByKey1[4] = &*it++;

        // make a list of elements ordered according to the second key
        it = map.begin(Map::e_SECOND_KEY);
        bsl::vector<const Map::value_type*> elementsByKey2(5, &alloc);
        elementsByKey2[0] = &*it++;
        elementsByKey2[1] = &*it++;
        elementsByKey2[2] = &*it++;
        elementsByKey2[3] = &*it++;
        elementsByKey2[4] = &*it++;

        // find element with second key equal to "third"
        Map::iterator erased = map.findByKey2("third");
        ASSERT_EQ(erased == map.end(), false)
        ASSERT_EQ(erased->key2(), "third");

        const Map::value_type* erased_p = &*erased;

        // remove element with second key equal to "third"
        int eraseRc = map.eraseByKey2("third");

        // return value is 0
        ASSERT_EQ(eraseRc, 0);

        // element removed
        ASSERT_EQ(map.size(), 4u);
        ASSERT_EQ(map.findByKey2("third") == map.end(), true);

        // order of remaining elements indexed by the first key hasn't changed
        it = map.begin(Map::e_FIRST_KEY);
        ASSERT_EQ(elementsByKey1[0] == erased_p || elementsByKey1[0] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey1[1] == erased_p || elementsByKey1[1] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey1[2] == erased_p || elementsByKey1[2] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey1[3] == erased_p || elementsByKey1[3] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey1[4] == erased_p || elementsByKey1[4] == &*it++,
                  true);

        // order of remaining elements indexed by the second key hasn't changed
        it = map.begin(Map::e_SECOND_KEY);
        ASSERT_EQ(elementsByKey2[0] == erased_p || elementsByKey2[0] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey2[1] == erased_p || elementsByKey2[1] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey2[2] == erased_p || elementsByKey2[2] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey2[3] == erased_p || elementsByKey2[3] == &*it++,
                  true);

        ASSERT_EQ(elementsByKey2[4] == erased_p || elementsByKey2[4] == &*it++,
                  true);
    }

    // 2. erase by second key, which doesn't exist
    {
        typedef mwcc::TwoKeyHashMap<int, bsl::string, bsl::string> Map;

        Map map(&alloc);

        // insert elements
        map.insert(1, "first", "value1");
        ASSERT_EQ(map.size(), 1u);

        // try erasing by key that doesn't exist
        int eraseRc = map.eraseByKey2("second");

        // returned value is non-zero
        ASSERT_NE(eraseRc, 0);

        // the map is intact
        ASSERT_EQ(map.size(), 1u);
        ASSERT_EQ(map.begin()->key1(), 1);
        ASSERT_EQ(map.begin()->key2(), "first");
        ASSERT_EQ(map.begin()->value(), "value1");
    }

    // 3. erase by second key, which exists, but throws on comparison
    {
        typedef mwcc::TwoKeyHashMap<bsl::string, ThrowingInteger, bsl::string>
            Map;

        Map map(&alloc);

        // insert elements
        map.insert("first", 1, "value1");
        ASSERT_EQ(map.size(), 1u);

        bool exceptionThrown = false;

        // try erasing by key that doesn't exist
        try {
            map.eraseByKey2(ThrowingInteger(1, false, true));
        }
        catch (const ThrowingInteger::ComparisonException&) {
            exceptionThrown = true;
        }

        // exception was thrown
        ASSERT_EQ(exceptionThrown, true);

        // the map is intact
        ASSERT_EQ(map.size(), 1u);
        ASSERT_EQ(map.begin()->key1(), "first");
        ASSERT_EQ(map.begin()->key2().value(), 1);
        ASSERT_EQ(map.begin()->value(), "value1");
    }
}

static void test7_clear()
// ------------------------------------------------------------------------
// CLEAR
//
// Concerns:
//   Ensure proper behavior of the 'clear()' method.
//
// Plan:
//   Create a map, add elements. Call 'clear()' and check that the map is
//   now empty. Then, call 'clear()' again and check that the map is still
//   empty.
//
// Testing:
//   clear()
// ------------------------------------------------------------------------
{
    typedef mwcc::TwoKeyHashMap<int, bsl::string, bsl::string> Map;

    bslma::TestAllocator alloc;
    Map                  map(&alloc);

    // insert elements
    map.insert(1, "first", "value1");
    map.insert(2, "second", "value2");
    map.insert(3, "third", "value3");
    ASSERT_EQ(map.size(), 3u);

    // do clear
    map.clear();

    // the map is now empty
    ASSERT_EQ(map.empty(), true);

    // clear again
    map.clear();

    // the map is still empty
    ASSERT_EQ(map.empty(), true);
}

static void test8_swap()
// ------------------------------------------------------------------------
// SWAP
//
// Concerns:
//   Ensure proper behavior of the 'swap()' method.
//
// Plan:
//   Create two maps, fill them with data and swap them. Check that maps
//   contents were swapped.
//
// Testing:
//   swap()
// ------------------------------------------------------------------------
{
    typedef mwcc::TwoKeyHashMap<int, bsl::string, bsl::string> Map;

    bslma::TestAllocator alloc;
    Map                  map1(&alloc);
    Map                  map2(&alloc);
    Map::iterator        it;

    // insert 1 element to map1
    map1.insert(1, "first", "value1");
    ASSERT_EQ(map1.size(), 1u);

    // insert 2 elements to map2
    map2.insert(2, "second", "value2");
    map2.insert(3, "third", "value3");
    ASSERT_EQ(map2.size(), 2u);

    // swap maps
    swap(map1, map2);

    // map1 now have the contents of map2
    ASSERT_EQ(map1.size(), 2u);

    ASSERT_EQ((it = map1.findByKey1(2)) == map1.end(), false);
    ASSERT_EQ(it->key1(), 2);
    ASSERT_EQ(it->key2(), "second");
    ASSERT_EQ(it->value(), "value2");

    ASSERT_EQ((it = map1.findByKey1(3)) == map1.end(), false);
    ASSERT_EQ(it->key1(), 3);
    ASSERT_EQ(it->key2(), "third");
    ASSERT_EQ(it->value(), "value3");

    // map2 now have the contents of map1
    ASSERT_EQ(map2.size(), 1u);

    ASSERT_EQ((it = map2.findByKey1(1)) == map2.end(), false);
    ASSERT_EQ(it->key1(), 1);
    ASSERT_EQ(it->key2(), "first");
    ASSERT_EQ(it->value(), "value1");
}

static void test9_rangeObservers()
// ------------------------------------------------------------------------
// RANGE OBSERVERS
//
// Concerns:
//   Ensure proper behavior of range observer methods.
//
// Plan:
//   Test 'begin()', 'cbegin()', 'end()' and 'cend()' member functions with
//   different combination of input parameters, with empty, non-empty,
//   const and non-const containers.
//
// Testing:
//   begin()
//   cbegin()
//   end()
//   cend()
// ------------------------------------------------------------------------
{
    typedef mwcc::TwoKeyHashMap<int, bsl::string, bsl::string> Map;

    bslma::TestAllocator alloc;
    Map                  emptyMap(&alloc);
    Map                  nonEmptyMap(&alloc);
    Map::iterator        iterator;
    Map::const_iterator  constIterator;

    // insert 1 element to nonEmptyMap
    nonEmptyMap.insert(1, "first", "value1");
    ASSERT_EQ(nonEmptyMap.size(), 1u);

    const Map::value_type* element_p = &*nonEmptyMap.begin();

    // non-const empty map
    {
        // begin(), first key
        iterator = emptyMap.begin(Map::e_FIRST_KEY);
        ASSERT_EQ(iterator == emptyMap.end(), true);
        ASSERT_EQ(iterator.keyIndex(), Map::e_FIRST_KEY);

        // begin(), second key
        iterator = emptyMap.begin(Map::e_SECOND_KEY);
        ASSERT_EQ(iterator == emptyMap.end(), true);
        ASSERT_EQ(iterator.keyIndex(), Map::e_SECOND_KEY);

        // cbegin(), first key
        constIterator = emptyMap.cbegin(Map::e_FIRST_KEY);
        ASSERT_EQ(constIterator == emptyMap.cend(), true);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_FIRST_KEY);

        // cbegin(), second key
        constIterator = emptyMap.cbegin(Map::e_SECOND_KEY);
        ASSERT_EQ(constIterator == emptyMap.cend(), true);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_SECOND_KEY);

        // end(), first key
        iterator = emptyMap.end(Map::e_FIRST_KEY);
        ASSERT_EQ(iterator.keyIndex(), Map::e_FIRST_KEY);

        // end(), second key
        iterator = emptyMap.end(Map::e_SECOND_KEY);
        ASSERT_EQ(iterator.keyIndex(), Map::e_SECOND_KEY);

        // cend(), first key
        constIterator = emptyMap.cend(Map::e_FIRST_KEY);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_FIRST_KEY);

        // cend(), second key
        constIterator = emptyMap.cend(Map::e_SECOND_KEY);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_SECOND_KEY);
    }

    // non-const non-empty map
    {
        // begin(), first key
        iterator = nonEmptyMap.begin(Map::e_FIRST_KEY);
        ASSERT_EQ(iterator == nonEmptyMap.end(), false);
        ASSERT_EQ(iterator.keyIndex(), Map::e_FIRST_KEY);
        ASSERT_EQ(*iterator == *element_p, true);

        // begin(), second key
        iterator = nonEmptyMap.begin(Map::e_SECOND_KEY);
        ASSERT_EQ(iterator == nonEmptyMap.end(), false);
        ASSERT_EQ(iterator.keyIndex(), Map::e_SECOND_KEY);
        ASSERT_EQ(*iterator == *element_p, true);

        // cbegin(), first key
        constIterator = nonEmptyMap.cbegin(Map::e_FIRST_KEY);
        ASSERT_EQ(constIterator == nonEmptyMap.cend(), false);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_FIRST_KEY);
        ASSERT_EQ(*constIterator == *element_p, true);

        // cbegin(), second key
        constIterator = nonEmptyMap.cbegin(Map::e_SECOND_KEY);
        ASSERT_EQ(constIterator == nonEmptyMap.cend(), false);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_SECOND_KEY);
        ASSERT_EQ(*constIterator == *element_p, true);

        // end(), first key
        iterator = nonEmptyMap.end(Map::e_FIRST_KEY);
        ASSERT_EQ(iterator.keyIndex(), Map::e_FIRST_KEY);

        // end(), second key
        iterator = nonEmptyMap.end(Map::e_SECOND_KEY);
        ASSERT_EQ(iterator.keyIndex(), Map::e_SECOND_KEY);

        // cend(), first key
        constIterator = nonEmptyMap.cend(Map::e_FIRST_KEY);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_FIRST_KEY);

        // cend(), second key
        constIterator = nonEmptyMap.cend(Map::e_SECOND_KEY);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_SECOND_KEY);
    }

    // const empty map
    {
        // begin(), first key
        constIterator = bsl::cref(emptyMap).get().begin(Map::e_FIRST_KEY);
        ASSERT_EQ(constIterator == emptyMap.end(), true);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_FIRST_KEY);

        // begin(), second key
        constIterator = bsl::cref(emptyMap).get().begin(Map::e_SECOND_KEY);
        ASSERT_EQ(constIterator == emptyMap.end(), true);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_SECOND_KEY);

        // cbegin(), first key
        constIterator = bsl::cref(emptyMap).get().cbegin(Map::e_FIRST_KEY);
        ASSERT_EQ(constIterator == emptyMap.cend(), true);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_FIRST_KEY);

        // cbegin(), second key
        constIterator = bsl::cref(emptyMap).get().cbegin(Map::e_SECOND_KEY);
        ASSERT_EQ(constIterator == emptyMap.cend(), true);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_SECOND_KEY);

        // end(), first key
        constIterator = bsl::cref(emptyMap).get().end(Map::e_FIRST_KEY);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_FIRST_KEY);

        // end(), second key
        constIterator = bsl::cref(emptyMap).get().end(Map::e_SECOND_KEY);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_SECOND_KEY);

        // cend(), first key
        constIterator = bsl::cref(emptyMap).get().cend(Map::e_FIRST_KEY);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_FIRST_KEY);

        // cend(), second key
        constIterator = bsl::cref(emptyMap).get().cend(Map::e_SECOND_KEY);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_SECOND_KEY);
    }

    // const non-empty map
    {
        // begin(), first key
        constIterator = bsl::cref(nonEmptyMap).get().begin(Map::e_FIRST_KEY);
        ASSERT_EQ(constIterator == nonEmptyMap.end(), false);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_FIRST_KEY);
        ASSERT_EQ(*constIterator == *element_p, true);

        // begin(), second key
        constIterator = bsl::cref(nonEmptyMap).get().begin(Map::e_SECOND_KEY);
        ASSERT_EQ(constIterator == nonEmptyMap.end(), false);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_SECOND_KEY);
        ASSERT_EQ(*constIterator == *element_p, true);

        // cbegin(), first key
        constIterator = bsl::cref(nonEmptyMap).get().cbegin(Map::e_FIRST_KEY);
        ASSERT_EQ(constIterator == nonEmptyMap.cend(), false);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_FIRST_KEY);
        ASSERT_EQ(*constIterator == *element_p, true);

        // cbegin(), second key
        constIterator = bsl::cref(nonEmptyMap).get().cbegin(Map::e_SECOND_KEY);
        ASSERT_EQ(constIterator == nonEmptyMap.cend(), false);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_SECOND_KEY);
        ASSERT_EQ(*constIterator == *element_p, true);

        // end(), first key
        constIterator = bsl::cref(nonEmptyMap).get().end(Map::e_FIRST_KEY);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_FIRST_KEY);

        // end(), second key
        constIterator = bsl::cref(nonEmptyMap).get().end(Map::e_SECOND_KEY);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_SECOND_KEY);

        // cend(), first key
        constIterator = bsl::cref(nonEmptyMap).get().cend(Map::e_FIRST_KEY);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_FIRST_KEY);

        // cend(), second key
        constIterator = bsl::cref(nonEmptyMap).get().cend(Map::e_SECOND_KEY);
        ASSERT_EQ(constIterator.keyIndex(), Map::e_SECOND_KEY);
    }
}

static void test10_findByKey1()
// ------------------------------------------------------------------------
// FIND BY KEY1
//
// Concerns:
//   Ensure proper behavior of the 'findByKey1()' method.
//
// Plan:
//   1. Call 'findByKey1()' on a non-constant map, specifying an existing
//      key.  Check that:
//      - The returned iterator refers to the element indexed by the
//        specified key;
//      - The returned iterator is bound to the first key.
//
//   2. Call 'findByKey1()' on a constant map, specifying an existing key.
//      Check that:
//      - The returned iterator refers to the element indexed by the
//        specified key;
//      - The returned iterator is bound to the first key.
//
//   3. Call 'findByKey1()' on a non-constant map, specifying a non-
//      existing key.  Check that:
//      - The returned iterator is a past-the-end iterator;
//      - The returned iterator is bound to the first key.
//
//   4. Call 'findByKey1()' on a constant map, specifying a non-existing
//      key.  Check that:
//      - The returned iterator is a past-the-end iterator;
//      - The returned iterator is bound to the first key.
//
// Testing:
//   findByKey1()
// ------------------------------------------------------------------------
{
    typedef mwcc::TwoKeyHashMap<int, bsl::string, bsl::string> Map;

    bslma::TestAllocator alloc;
    Map                  map(&alloc);

    // insert elements
    map.insert(1, "first", "value1");
    map.insert(2, "second", "value2");
    ASSERT_EQ(map.size(), 2u);

    // 1. non-const contained, key exists
    {
        Map::iterator it = map.findByKey1(1);
        ASSERT_EQ(it == map.end(), false);
        ASSERT_EQ(it.keyIndex(), Map::e_FIRST_KEY);
        ASSERT_EQ(it->key1(), 1);
        ASSERT_EQ(it->key2(), "first");
        ASSERT_EQ(it->value(), "value1");
    }

    // 2. const contained, key exists
    {
        Map::const_iterator it = bsl::cref(map).get().findByKey1(2);
        ASSERT_EQ(it == map.end(), false);
        ASSERT_EQ(it.keyIndex(), Map::e_FIRST_KEY);
        ASSERT_EQ(it->key1(), 2);
        ASSERT_EQ(it->key2(), "second");
        ASSERT_EQ(it->value(), "value2");
    }

    // 3. non-const contained, key doesn't exist
    {
        Map::iterator it = map.findByKey1(3);
        ASSERT_EQ(it == map.end(), true);
        ASSERT_EQ(it.keyIndex(), Map::e_FIRST_KEY);
    }

    // 4. const contained, key doesn't exist
    {
        Map::const_iterator it = bsl::cref(map).get().findByKey1(3);
        ASSERT_EQ(it == map.end(), true);
        ASSERT_EQ(it.keyIndex(), Map::e_FIRST_KEY);
    }
}

static void test11_findByKey2()
// ------------------------------------------------------------------------
// FIND BY KEY2
//
// Concerns:
//   Ensure proper behavior of the 'findByKey2()' method.
//
// Plan:
//   1. Call 'findByKey2()' on a non-constant map, specifying an existing
//      key.  Check that:
//      - The returned iterator refers to the element indexed by the
//        specified key;
//      - The returned iterator is bound to the first key.
//
//   2. Call 'findByKey2()' on a constant map, specifying an existing key.
//      Check that:
//      - The returned iterator refers to the element indexed by the
//        specified key;
//      - The returned iterator is bound to the first key.
//
//   3. Call 'findByKey2()' on a non-constant map, specifying a non-
//      existing key.  Check that:
//      - The returned iterator is a past-the-end iterator;
//      - The returned iterator is bound to the first key.
//
//   4. Call 'findByKey2()' on a constant map, specifying a non-existing
//      key.  Check that:
//      - The returned iterator is a past-the-end iterator;
//      - The returned iterator is bound to the first key.
//
// Testing:
//   findByKey2()
// ------------------------------------------------------------------------
{
    typedef mwcc::TwoKeyHashMap<int, bsl::string, bsl::string> Map;

    bslma::TestAllocator alloc;
    Map                  map(&alloc);

    // insert elements
    map.insert(1, "first", "value1");
    map.insert(2, "second", "value2");
    ASSERT_EQ(map.size(), 2u);

    // 1. non-const contained, key exists
    {
        Map::iterator it = map.findByKey2("first");
        ASSERT_EQ(it == map.end(), false);
        ASSERT_EQ(it.keyIndex(), Map::e_SECOND_KEY);
        ASSERT_EQ(it->key1(), 1);
        ASSERT_EQ(it->key2(), "first");
        ASSERT_EQ(it->value(), "value1");
    }

    // 2. const contained, key exists
    {
        Map::const_iterator it = bsl::cref(map).get().findByKey2("second");
        ASSERT_EQ(it == map.end(), false);
        ASSERT_EQ(it.keyIndex(), Map::e_SECOND_KEY);
        ASSERT_EQ(it->key1(), 2);
        ASSERT_EQ(it->key2(), "second");
        ASSERT_EQ(it->value(), "value2");
    }

    // 3. non-const contained, key doesn't exist
    {
        Map::iterator it = map.findByKey2("third");
        ASSERT_EQ(it == map.end(), true);
        ASSERT_EQ(it.keyIndex(), Map::e_SECOND_KEY);
    }

    // 4. const contained, key doesn't exist
    {
        Map::const_iterator it = bsl::cref(map).get().findByKey2("third");
        ASSERT_EQ(it == map.end(), true);
        ASSERT_EQ(it.keyIndex(), Map::e_SECOND_KEY);
    }
}

static void test12_sizeObservers()
// ------------------------------------------------------------------------
// SIZE OBSERVERS
//
// Concerns:
//   Ensure proper behavior of size observers methods.
//
// Plan:
//   Create a map, add and remove elements, checking the correct behavior
//   of 'empty()' and 'size()' methods.
//
// Testing:
//   empty()
//   size()
// ------------------------------------------------------------------------
{
    typedef mwcc::TwoKeyHashMap<int, bsl::string, bsl::string> Map;

    bslma::TestAllocator alloc;
    Map                  map(&alloc);

    ASSERT_EQ(map.empty(), true);
    ASSERT_EQ(map.size(), 0u);

    map.insert(1, "first", "value1");

    ASSERT_EQ(map.empty(), false);
    ASSERT_EQ(map.size(), 1u);

    map.insert(2, "second", "value2");

    ASSERT_EQ(map.empty(), false);
    ASSERT_EQ(map.size(), 2u);

    map.insert(3, "third", "value2");

    ASSERT_EQ(map.empty(), false);
    ASSERT_EQ(map.size(), 3u);

    map.erase(map.begin());

    ASSERT_EQ(map.empty(), false);
    ASSERT_EQ(map.size(), 2u);

    map.erase(map.begin());

    ASSERT_EQ(map.empty(), false);
    ASSERT_EQ(map.size(), 1u);

    map.erase(map.begin());

    ASSERT_EQ(map.empty(), true);
    ASSERT_EQ(map.size(), 0u);
}

static void test13_equality()
// ------------------------------------------------------------------------
// EQUALITY
//
// Concerns:
//   Ensure proper behavior of equality check operator.
//
// Plan:
//   Create a map, add and remove elements, checking the correct behavior
//   of '=='operator.
//
// Testing:
//   ==
// ------------------------------------------------------------------------
{
    typedef mwcc::TwoKeyHashMap<int, bsl::string, bsl::string> Map;

    bslma::TestAllocator alloc;
    Map                  map1(&alloc);
    Map                  map2(&alloc);

    map1.insert(1, "first", "value1");
    map1.insert(2, "second", "value2");
    map1.insert(3, "third", "value3");

    for (Map::const_iterator it = map1.cbegin(); it != map1.cend(); ++it) {
        ASSERT(map1 != map2);
        map2.insert(it->key1(), it->key2(), it->value());
    }
    ASSERT(map1 == map2);
    while (!map1.empty()) {
        map1.erase(map1.begin());
        ASSERT(map1 != map2);
    }
    while (!map2.empty()) {
        ASSERT(map1 != map2);
        map2.erase(map2.begin());
    }
    ASSERT(map1 == map2);
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char** argv)
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 13: test13_equality(); break;
    case 12: test12_sizeObservers(); break;
    case 11: test11_findByKey2(); break;
    case 10: test10_findByKey1(); break;
    case 9: test9_rangeObservers(); break;
    case 8: test8_swap(); break;
    case 7: test7_clear(); break;
    case 6: test6_eraseByKey2(); break;
    case 5: test5_eraseByKey1(); break;
    case 4: test4_erase(); break;
    case 3: test3_insert(); break;
    case 2: test2_copyAndMove(); break;
    case 1: test1_basicCreators(); break;
    case 0: test0_usageExample(); break;
    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
