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

// mwcc_orderedhashmap.t.cpp                                          -*-C++-*-
#include <mwcc_orderedhashmap.h>

// BDE
#include <bsl_algorithm.h>
#include <bsl_cmath.h>  // for 'sqrt'
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>  // for performance comparison test
#include <bsl_utility.h>
#include <bslh_hash.h>
#include <bslma_default.h>
#include <bsls_platform.h>
#include <bsls_timeutil.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// BENCHMARKING LIBRARY
#ifdef BSLS_PLATFORM_OS_LINUX
#include <benchmark/benchmark.h>
#endif

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

namespace {

/// Return true if the specified `n` is prime and false otherwise.
bool isPrime(size_t n)
{
    const size_t squareRootFloored = static_cast<size_t>(
        bsl::sqrt(static_cast<long double>(n)));
    for (size_t i = 2; i <= squareRootFloored; ++i) {
        if (n % i == 0) {
            return false;  // RETURN
        }
    }

    return n >= 2;
}

class IdentityHasher {
  public:
    IdentityHasher() {}

    size_t operator()(size_t x) const { return x; }
};

struct TestKeyType {
    // CLASS LEVEL DATA
    static size_t s_numDeletions;

    // DATA
    size_t d_a;

    // CREATORS
    TestKeyType(size_t a) { d_a = a; }

    ~TestKeyType() { s_numDeletions += 1; }
};

size_t TestKeyType::s_numDeletions(0);

// FREE FUNCTIONS
bool operator==(const TestKeyType& lhs, const TestKeyType& rhs)
{
    return lhs.d_a == rhs.d_a;
}

template <class HASH_ALGORITHM>
void hashAppend(HASH_ALGORITHM& hashAlgo, const TestKeyType& key)
{
    using bslh::hashAppend;  // for ADL
    hashAppend(hashAlgo, key.d_a);
}

struct TestValueType {
    // CLASS LEVEL DATA
    static size_t s_numDeletions;

    // DATA
    size_t d_b;

    // CREATORS
    TestValueType(size_t b) { d_b = b; }

    ~TestValueType() { s_numDeletions += 1; }
};

size_t TestValueType::s_numDeletions(0);

}  // close unnamed namespace

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise basic functionality before beginning testing in earnest.
//   Probe that functionality to discover basic errors.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    // Breathing test
    typedef mwcc::OrderedHashMap<size_t, bsl::string> MyMapType;
    typedef MyMapType::iterator                       IterType;
    typedef MyMapType::const_iterator                 ConstIterType;

    const bsl::string s("foo", s_allocator_p);

    MyMapType        map(s_allocator_p);
    const MyMapType& cmap = map;
    ASSERT_EQ(true, map.begin() == map.end());
    ASSERT_EQ(true, cmap.begin() == cmap.end());

    map.clear();

    ASSERT_EQ(0U, map.count(1));
    ASSERT_EQ(0U, map.erase(1));
    ASSERT_EQ(true, map.end() == map.find(1));
    ASSERT_EQ(true, cmap.empty());
    ASSERT_EQ(true, cmap.end() == cmap.find(1));
    ASSERT_EQ(0U, cmap.count(1));
    ASSERT_EQ(0U, cmap.size());

    bsl::pair<IterType, bool> rc = map.insert(bsl::make_pair(1, s));
    ASSERT_EQ(true, rc.first != map.end());
    ASSERT_EQ(rc.second, true);
    ASSERT_EQ(1U, rc.first->first);
    ASSERT_EQ(s, rc.first->second);
    ASSERT_EQ(1U, cmap.count(1));

    ConstIterType cit = cmap.find(1);
    ASSERT_EQ(true, cmap.end() != cit);
    ASSERT_EQ(1U, cmap.size());
    ASSERT_EQ(false, cmap.empty());
    ASSERT_EQ(1U, map.erase(1));
    ASSERT_EQ(true, map.begin() == map.end());
    ASSERT_EQ(true, cmap.begin() == cmap.end());
    ASSERT_EQ(true, cmap.end() == cmap.find(1));
}

static void test2_impDetails_nextPrime()
// ------------------------------------------------------------------------
// IMP DETAILS - NEXT PRIME
//
// Concerns:
//   1. Able to generate an increasing sequence of primes chosen to
//      disperse hash codes across buckets as uniformly as possible.
//   2. Able to detect failure (i.e. return value of 0) when the last prime
//      number in the sequence has been exhausted.
//
// Testing:
//   OrderedHashMap_ImpDetails::nextPrime
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("IMP DETAILS - NEXT PRIME");

    const size_t k_MAX_ITERATIONS = 1000;

    size_t prevPrime = 1;   // One is not prime, but we gotta start somewhere
    size_t currPrime = 1;   // One is not prime, but we gotta start somewhere
    size_t lastPrime = -1;  // Marker for value returned after exhausting the
                            // sequence
    size_t i = 0;
    while (i++ < k_MAX_ITERATIONS) {
        prevPrime = currPrime;
        currPrime = mwcc::OrderedHashMap_ImpDetails::nextPrime(currPrime + 1);
        if (currPrime == 0) {
            lastPrime = currPrime;
            break;  // RETURN
        }

        ASSERT_EQ(isPrime(currPrime), true);
        ASSERT_GT(currPrime, prevPrime);
    }

    ASSERT_EQ(lastPrime, 0U);
}

static void test3_insert()
// ------------------------------------------------------------------------
// INSERT
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("INSERT");

    typedef mwcc::OrderedHashMap<size_t, size_t> MyMapType;
    typedef MyMapType::iterator                  IterType;
    typedef MyMapType::const_iterator            ConstIterType;
    typedef bsl::pair<IterType, bool>            RcType;

    MyMapType map(s_allocator_p);

#if defined(BSLS_PLATFORM_OS_AIX) || defined(BSLS_PLATFORM_OS_SOLARIS)
    // Avoid timeout on AIX and Solaris
    const int k_NUM_ELEMENTS = 100 * 1000;  // 100K
#elif defined(__has_feature)
    // Avoid timeout under MemorySanitizer
    const int k_NUM_ELEMENTS = __has_feature(memory_sanitizer)
                                   ? 100 * 1000    // 100K
                                   : 1000 * 1000;  // 1M
#elif defined(__SANITIZE_MEMORY__)
    // GCC-supported macros for checking MSAN
    const int k_NUM_ELEMENTS = 100 * 1000;  // 100K
#else
    const int k_NUM_ELEMENTS = 1000 * 1000;  // 1M
#endif

    // Insert 1M elements
    for (size_t i = 0; i < k_NUM_ELEMENTS; ++i) {
        RcType rc = map.insert(bsl::make_pair(i, i + 1));
        ASSERT_EQ_D(i, true, rc.second);
        ASSERT_EQ_D(i, true, rc.first != map.end());
        ASSERT_EQ_D(i, i, rc.first->first);
        ASSERT_EQ_D(i, (i + 1), rc.first->second);
        ASSERT_EQ_D(i, true, 1.5 >= map.load_factor());
    }

    ASSERT_EQ(map.size(), k_NUM_ELEMENTS);

    // Iterate and confirm
    {
        const MyMapType& cmap = map;
        size_t           i    = 0;
        for (ConstIterType cit = cmap.begin(); cit != cmap.end(); ++cit) {
            ASSERT_EQ_D(i, true, i < k_NUM_ELEMENTS);
            ASSERT_EQ_D(i, i, cit->first);
            ASSERT_EQ_D(i, (i + 1), cit->second);
            ++i;
        }
    }

    // Reverse iterate using --(end()) and confirm
    {
        const MyMapType& cmap = map;
        size_t           i    = k_NUM_ELEMENTS - 1;
        ConstIterType    cit  = --(cmap.end());  // last element
        for (; cit != cmap.begin(); --cit) {
            ASSERT_EQ_D(i, true, i > 0);
            ASSERT_EQ_D(i, i, cit->first);
            ASSERT_EQ_D(i, (i + 1), cit->second);
            --i;
        }
        ASSERT_EQ(true, cit == cmap.begin());
        ASSERT_EQ(cit->first, i);
        ASSERT_EQ(cit->second, (i + 1));
    }
}

static void test4_rinsert()
// ------------------------------------------------------------------------
// RINSERT
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("RINSERT");

    // rinsert() test
    typedef mwcc::OrderedHashMap<int, int> MyMapType;
    typedef MyMapType::iterator            IterType;
    typedef MyMapType::const_iterator      ConstIterType;
    typedef bsl::pair<IterType, bool>      RcType;

    MyMapType map(s_allocator_p);

#if defined(BSLS_PLATFORM_OS_AIX) || defined(BSLS_PLATFORM_OS_SOLARIS)
    // Avoid timeout on AIX and Solaris
    const int k_NUM_ELEMENTS = 100 * 1000;  // 100K
#elif defined(__has_feature)
    // Avoid timeout under MemorySanitizer
    const int k_NUM_ELEMENTS = __has_feature(memory_sanitizer)
                                   ? 100 * 1000    // 100K
                                   : 1000 * 1000;  // 1M
#elif defined(__SANITIZE_MEMORY__)
    // GCC-supported macros for checking MSAN
    const int k_NUM_ELEMENTS = 100 * 1000;  // 100K
#else
    const int k_NUM_ELEMENTS = 1000 * 1000;  // 1M
#endif

    // Insert 1M elements
    for (int i = 0; i < k_NUM_ELEMENTS; ++i) {
        RcType rc = map.rinsert(bsl::make_pair(i, i + 1));
        ASSERT_EQ_D(i, true, rc.second);
        ASSERT_EQ_D(i, true, rc.first != map.end());
        ASSERT_EQ_D(i, i, rc.first->first);
        ASSERT_EQ_D(i, (i + 1), rc.first->second);
        ASSERT_EQ_D(i, true, 1.5 >= map.load_factor());
    }

    ASSERT_EQ(map.size(), static_cast<size_t>(k_NUM_ELEMENTS));

    // Iterate and confirm
    {
        const MyMapType& cmap = map;
        int              i    = k_NUM_ELEMENTS - 1;
        for (ConstIterType cit = cmap.begin(); cit != cmap.end(); ++cit) {
            ASSERT_EQ_D(i, i, cit->first);
            ASSERT_EQ_D(i, (i + 1), cit->second);
            --i;
        }
    }

    // Reverse iterate using --(end()) and confirm
    {
        const MyMapType& cmap = map;
        int              i    = 0;
        ConstIterType    cit  = --(cmap.end());  // last element
        for (; cit != cmap.begin(); --cit) {
            ASSERT_EQ_D(i, true, i < k_NUM_ELEMENTS);
            ASSERT_EQ_D(i, i, cit->first);
            ASSERT_EQ_D(i, (i + 1), cit->second);
            ++i;
        }
        ASSERT_EQ(true, cit == cmap.begin());
        ASSERT_EQ(i, cit->first);
        ASSERT_EQ(cit->second, (i + 1));
    }
}

static void test5_insertEraseInsert()
// ------------------------------------------------------------------------
// INSERT ERASE INSERT
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("INSERT ERASE INSERT");

    // insert/erase/insert test
    typedef mwcc::OrderedHashMap<size_t, size_t> MyMapType;
    typedef MyMapType::iterator                  IterType;
    typedef MyMapType::const_iterator            ConstIterType;
    typedef bsl::pair<IterType, bool>            RcType;

    MyMapType map(s_allocator_p);

    const size_t k_NUM_ELEMENTS = 100 * 1000;  // 100K
    const size_t k_STEP         = 10;

    // Insert elements
    for (size_t i = 0; i < k_NUM_ELEMENTS; ++i) {
        RcType rc = map.insert(bsl::make_pair(i, i + 1));
        ASSERT_EQ_D(i, rc.second, true);
        ASSERT_EQ_D(i, true, rc.first != map.end());
        ASSERT_EQ_D(i, i, rc.first->first);
        ASSERT_EQ_D(i, (i + 1), rc.first->second);
    }

    // Iterate and confirm
    {
        const MyMapType& cmap = map;
        size_t           i    = 0;
        for (ConstIterType cit = cmap.begin(); cit != cmap.end(); ++cit) {
            ASSERT_EQ_D(i, true, i < k_NUM_ELEMENTS);
            ASSERT_EQ_D(i, i, cit->first);
            ASSERT_EQ_D(i, (i + 1), cit->second);
            ++i;
        }
    }

    // Erase few elements
    for (size_t i = 0; i < k_NUM_ELEMENTS; i += k_STEP) {
        ASSERT_EQ_D(i, 1U, map.erase(i));
    }

    // Find erased elements
    for (size_t i = 0; i < k_NUM_ELEMENTS; i += k_STEP) {
        ASSERT_EQ_D(i, true, map.end() == map.find(i));
    }

    // Iterate and confirm
    {
        const MyMapType& cmap = map;
        size_t           i    = 1;
        for (ConstIterType cit = cmap.begin(); cit != cmap.end(); ++cit) {
            ASSERT_EQ_D(i, true, i < k_NUM_ELEMENTS);
            ASSERT_EQ_D(i, i, cit->first);
            ASSERT_EQ_D(i, (i + 1), cit->second);
            ++i;
            if (i % k_STEP == 0) {
                ++i;
            }
        }
    }

    // Insert elements which were erased earlier
    for (size_t i = 0; i < k_NUM_ELEMENTS; i += k_STEP) {
        RcType rc = map.insert(bsl::make_pair(i, i + 1));
        ASSERT_EQ_D(i, true, rc.second);
        ASSERT_EQ_D(i, true, rc.first != map.end());
        ASSERT_EQ_D(i, i, rc.first->first);
        ASSERT_EQ_D(i, (i + 1), rc.first->second);
    }

    // Iterate and confirm
    {
        IterType it = map.begin();
        size_t   i  = 1;

        // Iterate over original elements
        for (; it != map.end(); ++it) {
            ASSERT_EQ_D(i, true, it != map.end());
            ASSERT_EQ_D(i, i, it->first);
            ASSERT_EQ_D(i, (i + 1), it->second);
            if (it->first == (k_NUM_ELEMENTS - 1)) {
                ++it;
                break;
            }
            ++i;
            if (i % k_STEP == 0) {
                ++i;
            }
        }

        // Iterate ove elements inserted after erase operation
        for (i = 0; i < k_NUM_ELEMENTS; i += k_STEP) {
            ASSERT_EQ_D(i, true, it != map.end());
            ASSERT_EQ_D(i, i, it->first);
            ASSERT_EQ_D(i, (i + 1), it->second);
            ++it;
        }

        ASSERT_EQ(true, it == map.end());
    }
}

static void test6_clear()
// ------------------------------------------------------------------------
// CLEAR
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("CLEAR");

    // clear
    typedef mwcc::OrderedHashMap<int, int> MyMapType;
    typedef MyMapType::iterator            IterType;
    typedef bsl::pair<IterType, bool>      RcType;

    MyMapType map(s_allocator_p);
    ASSERT_EQ(true, map.empty());
    ASSERT_EQ(true, map.begin() == map.end());
    ASSERT_EQ(0U, map.size());
    ASSERT_EQ(true, map.load_factor() == 0.0);

    map.clear();
    ASSERT_EQ(true, map.empty());
    ASSERT_EQ(true, map.begin() == map.end());
    ASSERT_EQ(0U, map.size());
    ASSERT_EQ(true, map.load_factor() == 0.0);

    const int k_NUM_ELEMENTS = 100;

    // Insert elements
    for (int i = 0; i < k_NUM_ELEMENTS; ++i) {
        RcType rc = map.insert(bsl::make_pair(i, i + 1));
        ASSERT_EQ_D(i, rc.second, true);
        ASSERT_EQ_D(i, true, rc.first != map.end());
        ASSERT_EQ_D(i, i, rc.first->first);
        ASSERT_EQ_D(i, (i + 1), rc.first->second);
    }

    ASSERT_EQ(false, map.empty());
    ASSERT_EQ(true, map.begin() != map.end());
    ASSERT_EQ(static_cast<int>(k_NUM_ELEMENTS), map.size());

    map.clear();
    ASSERT_EQ(true, map.empty());
    ASSERT_EQ(true, map.begin() == map.end());
    ASSERT_EQ(0U, map.size());
    ASSERT_EQ(true, map.load_factor() == 0.0);
}

static void test7_erase()
// ------------------------------------------------------------------------
// ERASE
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ERASE");

    // erase
    typedef mwcc::OrderedHashMap<int, int> MyMapType;
    typedef MyMapType::iterator            IterType;
    typedef MyMapType::const_iterator      ConstIterType;
    typedef bsl::pair<IterType, bool>      RcType;

    const int k_NUM_ELEMENTS = 100;
    MyMapType map(s_allocator_p);

    // Insert elements
    for (int i = 0; i < k_NUM_ELEMENTS; ++i) {
        RcType rc = map.insert(bsl::make_pair(i, i));
        ASSERT_EQ_D(i, rc.second, true);
        ASSERT_EQ_D(i, true, rc.first != map.end());
        ASSERT_EQ_D(i, i, rc.first->first);
        ASSERT_EQ_D(i, i, rc.first->second);
    }

    const MyMapType& cmap = map;

    for (ConstIterType cit = cmap.begin(); cit != cmap.end();) {
        map.erase(cit++);
    }

    ASSERT_EQ(0U, map.size());
    ASSERT_EQ(true, map.empty());
}

static void test8_eraseClear()
// ------------------------------------------------------------------------
// ERASE CLEAR
//
// Concerns:
//   Erase/clear invoke destructors of keys and values.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ERASE CLEAR");

    // Erase/clear invoke destructors of keys and values
    typedef mwcc::OrderedHashMap<TestKeyType, TestValueType> MyMapType;
    typedef MyMapType::iterator                              IterType;
    typedef bsl::pair<IterType, bool>                        RcType;

    const size_t k_NUM_ELEMENTS = 100;
    MyMapType    map(s_allocator_p);

    // Insert elements
    for (size_t i = 0; i < k_NUM_ELEMENTS; ++i) {
        RcType rc = map.insert(
            bsl::make_pair(TestKeyType(i), TestValueType(i)));
        ASSERT_EQ_D(i, rc.second, true);
        ASSERT_EQ_D(i, true, rc.first != map.end());
    }

    ASSERT_EQ(k_NUM_ELEMENTS, map.size());

    // Reset static counters
    TestKeyType::s_numDeletions   = 0;
    TestValueType::s_numDeletions = 0;

    // Erase every element from that map
    for (IterType it = map.begin(); it != map.end();) {
        map.erase(it++);
    }

    ASSERT_EQ(TestKeyType::s_numDeletions, k_NUM_ELEMENTS);
    ASSERT_EQ(TestValueType::s_numDeletions, k_NUM_ELEMENTS);
}

static void test9_insertFailure()
// ------------------------------------------------------------------------
// INSERT FAILURE
//
// Concerns:
//   Inserting (key, value) pair corresponding to already present elements
//   fails.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("INSERT FAILURE");

    // insert (key, value) already present in the container
    typedef mwcc::OrderedHashMap<size_t, size_t> MyMapType;
    typedef MyMapType::iterator                  IterType;
    typedef bsl::pair<IterType, bool>            RcType;

    const size_t k_NUM_ELEMENTS = 100000;
    MyMapType    map(s_allocator_p);

    // Insert elements
    for (size_t i = 0; i < k_NUM_ELEMENTS; ++i) {
        RcType rc = map.insert(bsl::make_pair(i, i));
        ASSERT_EQ_D(i, true, rc.second);
        ASSERT_EQ_D(i, true, rc.first != map.end());
    }

    // insert same keys again
    for (size_t i = 0; i < k_NUM_ELEMENTS; ++i) {
        RcType rc = map.insert(bsl::make_pair(i, i));
        ASSERT_EQ_D(i, rc.second, false);
        ASSERT_EQ_D(i, true, rc.first == map.find(i));
    }
}

static void test10_erasureIterator()
// ------------------------------------------------------------------------
// ERASURE ITERATOR
//
// Concerns:
//   Use iterator returned by erase(const_iterator)
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ERASURE ITERATOR");
    // Use iterator returned by erase(const_iterator)

    typedef mwcc::OrderedHashMap<size_t, size_t> MyMapType;
    typedef MyMapType::iterator                  IterType;
    typedef bsl::pair<IterType, bool>            RcType;

    const size_t k_NUM_ELEMENTS = 10000;
    MyMapType    map(s_allocator_p);

    // Insert elements
    for (size_t i = 0; i < k_NUM_ELEMENTS; ++i) {
        RcType rc = map.insert(bsl::make_pair(i, i));
        ASSERT_EQ_D(i, rc.second, true);
        ASSERT_EQ_D(i, true, rc.first != map.end());
    }

    // Find
    IterType iter = map.find(9000);
    ASSERT_EQ(true, iter != map.end());
    ASSERT_EQ(iter->first, 9000U);

    // erase
    IterType it = map.erase(iter);
    ASSERT_EQ(true, it != map.end());
    ASSERT_EQ(true, it == map.find(9001));

    size_t i = 9001;
    for (; it != map.end(); ++it) {
        ASSERT_EQ_D(i, i, it->first);
        ++i;
    }
}

static void test11_copyConstructor()
// ------------------------------------------------------------------------
// COPY CONSTRUCTOR
//
// Concerns:
//   Copy constructor.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("COPY CONSTRUCTOR");
    // Copy constructor

    // Create object 1. Insert elements.
    // Copy construct object 2 from object 1.
    // Assert that object 2 has same elements etc.

    typedef mwcc::OrderedHashMap<size_t, size_t> MyMapType;
    typedef MyMapType::iterator                  IterType;
    typedef MyMapType::const_iterator            ConstIterType;
    typedef bsl::pair<IterType, bool>            RcType;

    const size_t k_NUM_ELEMENTS = 10000;

    MyMapType* m1p = new (*s_allocator_p) MyMapType(s_allocator_p);

    // Insert elements
    for (size_t i = 0; i < k_NUM_ELEMENTS; ++i) {
        RcType rc = m1p->insert(bsl::make_pair(i, i));
        ASSERT_EQ_D(i, rc.second, true);
        ASSERT_EQ_D(i, true, rc.first != m1p->end());
    }

    MyMapType m2(*m1p, s_allocator_p);

    // Iterate and confirm
    size_t i = 0;
    for (ConstIterType cit = m2.begin(); cit != m2.end(); ++cit) {
        ASSERT_EQ_D(i, cit->first, i);
        ASSERT_EQ_D(i, cit->second, i);
        ++i;
    }

    // Delete object 1 and check object 2 again
    s_allocator_p->deleteObject(m1p);

    i = 0;
    for (ConstIterType cit = m2.begin(); cit != m2.end(); ++cit) {
        ASSERT_EQ_D(i, cit->first, i);
        ASSERT_EQ_D(i, cit->second, i);
        ++i;
    }
}

static void test12_assignmentOperator()
// ------------------------------------------------------------------------
// ASSIGNMENT OPERATOR
//
// Concerns:
//   Assignment operator.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ASSIGNMENT OPERATOR");
    // Assignment operator

    // Create object 1. Insert elements.
    // Create object 2. Insert different elements.
    // object 2 = object 1
    // Assert that object 2 has same elements as object 1 etc.

    typedef mwcc::OrderedHashMap<size_t, size_t> MyMapType;
    typedef MyMapType::iterator                  IterType;
    typedef MyMapType::const_iterator            ConstIterType;
    typedef bsl::pair<IterType, bool>            RcType;

    const size_t k_NUM_ELEMENTS = 10000;

    MyMapType* m1p = new (*s_allocator_p) MyMapType(s_allocator_p);

    // Insert elements
    for (size_t i = 0; i < k_NUM_ELEMENTS; ++i) {
        RcType rc = m1p->insert(bsl::make_pair(i, i));
        ASSERT_EQ_D(i, rc.second, true);
        ASSERT_EQ_D(i, true, rc.first != m1p->end());
    }

    MyMapType m2(s_allocator_p);

    // Insert elements
    for (size_t i = k_NUM_ELEMENTS; i > 0; --i) {
        RcType rc = m2.insert(bsl::make_pair(i, i));
        ASSERT_EQ_D(i, rc.second, true);
        ASSERT_EQ_D(i, true, rc.first != m2.end());
    }

    m2 = *m1p;

    // Iterate and confirm
    size_t i = 0;
    for (ConstIterType cit = m2.begin(); cit != m2.end(); ++cit) {
        ASSERT_EQ_D(i, cit->first, i);
        ASSERT_EQ_D(i, cit->second, i);
        ++i;
    }

    // Delete object 1 and check object 2 again
    s_allocator_p->deleteObject(m1p);
    i = 0;
    for (ConstIterType cit = m2.begin(); cit != m2.end(); ++cit) {
        ASSERT_EQ_D(i, cit->first, i);
        ASSERT_EQ_D(i, cit->second, i);
        ++i;
    }
}

static void test13_previousEndIterator()
// ------------------------------------------------------------------------
// PREVIOUS END ITERATOR
//
// Concerns:
//   Ensure that upon insert()'ing a new element, previous end iterator is
//   pointing to the newly inserted element.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("PREVIOUS END ITERATOR");
    // is pointing to the newly inserted element.

    typedef mwcc::OrderedHashMap<int, int> MyMapType;
    typedef MyMapType::iterator            IterType;
    typedef MyMapType::const_iterator      ConstIterType;

    MyMapType        map(s_allocator_p);
    const MyMapType& cmap = map;
    ASSERT_EQ(true, map.begin() == map.end());
    ASSERT_EQ(true, cmap.begin() == cmap.end());
    ASSERT_EQ(true, cmap.empty());

    IterType      endIt  = map.end();
    ConstIterType endCit = cmap.end();

    int                       i  = 0;
    bsl::pair<IterType, bool> rc = map.insert(bsl::make_pair(i, i * i));

    ASSERT_EQ(true, rc.first == endIt);
    ASSERT_EQ(true, rc.first == endCit);

    ASSERT_EQ(i, endIt->first);
    ASSERT_EQ((i * i), endIt->second);

    ++i;
    for (; i < 10000; ++i) {
        endIt = map.end();
        rc    = map.insert(bsl::make_pair(i, i * i));
        ASSERT_EQ_D(i, true, rc.first == endIt);
        ASSERT_EQ_D(i, i, endIt->first);
        ASSERT_EQ_D(i, (i * i), endIt->second);
    }

    // Erase last element
    map.erase(i - 1);
    ASSERT_EQ((i - 2), (--map.end())->first);
    endIt = map.end();
    ++i;
    rc = map.insert(bsl::make_pair(i, i * i));
    ASSERT_EQ(true, rc.first == endIt);
    ASSERT_EQ(i, endIt->first);
    ASSERT_EQ((i * i), endIt->second);

    // rinsert an element, which doesn't affect end().
    ++i;
    endIt = map.end();
    rc    = map.rinsert(bsl::make_pair(i, i * i));
    ASSERT_EQ(true, endIt == map.end());
    ++i;
    rc = map.insert(bsl::make_pair(i, i * i));
    ASSERT_EQ(true, endIt == rc.first);
    ASSERT_EQ(i, endIt->first);
    ASSERT_EQ((i * i), endIt->second);
}

static void test14_localIterator()
// ------------------------------------------------------------------------
// LOCAL ITERATOR
//
// Concerns:
//   Testing {const_}local_iterator.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("LOCAL ITERATOR");

    // Note that we choose key's type to be an int, and we specify an
    // identify hash function.  With this knowledge and the bucket count
    // of the hash table, we specify such keys such that they will all map
    // to the same bucket in the table.

    typedef mwcc::OrderedHashMap<size_t, size_t, IdentityHasher> MyMapType;
    typedef MyMapType::local_iterator                            LocalIterType;
    typedef MyMapType::const_local_iterator ConstLocalIterType;

    MyMapType        map(s_allocator_p);
    const MyMapType& cmap = map;

    const size_t bucketCount = map.bucket_count();

    size_t key         = bucketCount / 2;
    size_t originalKey = key;

    map.insert(bsl::make_pair(key, key * key));

    size_t        bucket     = map.bucket(key);
    LocalIterType localIt    = map.begin(bucket);
    LocalIterType localEndIt = map.end(bucket);

    ASSERT_EQ(false, localIt == localEndIt);
    ASSERT_EQ(localIt->first, key);
    ASSERT_EQ(localIt->second, key * key);
    ++localIt;
    ASSERT_EQ(true, localIt == localEndIt);

    ConstLocalIterType cLocalIt    = cmap.begin(bucket);
    ConstLocalIterType cLocalEndIt = cmap.end(bucket);

    ASSERT_EQ(false, cLocalIt == cLocalEndIt);
    ASSERT_EQ(cLocalIt->first, key);
    ASSERT_EQ(cLocalIt->second, key * key);
    ++cLocalIt;
    ASSERT_EQ(true, cLocalIt == cLocalEndIt);

    // Add keys such that they all map to same bucket in the table, while
    // ensuring that table is not rehashed.
    for (size_t i = 0; i < (bucketCount - 2); ++i) {
        key += bucketCount;
        map.insert(bsl::make_pair(key, key * key));
    }

    localIt    = map.begin(bucket);
    localEndIt = map.end(bucket);

    key = originalKey;
    for (; localIt != localEndIt; ++localIt, key += bucketCount) {
        ASSERT_EQ(localIt->first, key);
        ASSERT_EQ(localIt->second, key * key);
    }
}

static void test15_eraseRange()
// ------------------------------------------------------------------------
// ERASE
//
// Concerns:
//   Check erasing range
//
// Plan:
//   Insert elements
//   Attempt to erase (begin, begin)
//   Attempt to erase (end, end)
//   Attempt to erase (begin, ++begin)
//   Attempt to erase (begin, end)
//
// Testing:
//   erase(const_iterator first, const_iterator last)
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ERASE RANGE");

    // erase
    typedef mwcc::OrderedHashMap<size_t, size_t> MyMapType;
    typedef MyMapType::iterator                  IterType;
    typedef MyMapType::const_iterator            ConstIterType;
    typedef bsl::pair<IterType, bool>            RcType;

    const size_t k_NUM_ELEMENTS = 100;
    MyMapType    map(s_allocator_p);

    // Insert elements
    for (size_t i = 0; i < k_NUM_ELEMENTS; ++i) {
        RcType rc = map.insert(bsl::make_pair(i, i));
        ASSERT_EQ_D(i, rc.second, true);
        ASSERT_EQ_D(i, true, rc.first != map.end());
        ASSERT_EQ_D(i, i, rc.first->first);
        ASSERT_EQ_D(i, i, rc.first->second);
    }

    ASSERT_EQ(k_NUM_ELEMENTS, map.size());

    ASSERT(map.erase(map.begin(), map.begin()) == map.begin());
    ASSERT_EQ(k_NUM_ELEMENTS, map.size());

    ASSERT(map.erase(map.end(), map.end()) == map.end());
    ASSERT_EQ(k_NUM_ELEMENTS, map.size());

    ConstIterType second = ++map.begin();
    ASSERT(map.erase(map.begin(), second) == second);
    ASSERT(map.begin() == second);
    ASSERT_EQ(k_NUM_ELEMENTS - 1, map.size());
    ASSERT_EQ_D(1, 1U, map.begin()->first);
    ASSERT_EQ_D(1, 1U, map.begin()->second);

    ASSERT(map.erase(--map.end(), map.end()) == map.end());
    ASSERT_EQ(k_NUM_ELEMENTS - 2, map.size());
    ASSERT_EQ_D(k_NUM_ELEMENTS - 2, k_NUM_ELEMENTS - 2, (--map.end())->first);
    ASSERT_EQ_D(k_NUM_ELEMENTS - 2, k_NUM_ELEMENTS - 2, (--map.end())->second);

    ASSERT(map.erase(map.begin(), map.end()) == map.end());

    ASSERT_EQ(0U, map.size());

    ASSERT_EQ(true, map.empty());
}

BSLA_MAYBE_UNUSED
static void testN1_insertPerformanceOrdered()
// ------------------------------------------------------------------------
// INSERT PERFORMANCE
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("INSERT PERFORMANCE");

    // Performance comparison of insert() with bsl::unordered_map
    const size_t       k_NUM_ELEMENTS = 5000000;
    bsls::Types::Int64 ohmTime;

    {
        // OrderedHashMap
        typedef mwcc::OrderedHashMap<size_t, size_t> MyMapType;

        MyMapType map(k_NUM_ELEMENTS, s_allocator_p);

        bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_NUM_ELEMENTS; ++i) {
            map.insert(bsl::make_pair(i, i));
        }
        bsls::Types::Int64 end = bsls::TimeUtil::getTimer();
        ohmTime                = end - begin;
        cout << "Time diff (OrderedHashMap): " << ohmTime << endl;
    }
}

BSLA_MAYBE_UNUSED
static void testN1_insertPerformanceUnordered()
// ------------------------------------------------------------------------
// INSERT PERFORMANCE
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("INSERT PERFORMANCE");

    // Performance comparison of insert() with bsl::unordered_map
    const size_t       k_NUM_ELEMENTS = 5000000;
    bsls::Types::Int64 umTime;
    {
        // bsl::unordered_map
        typedef bsl::unordered_map<size_t, size_t> MyMapType;

        MyMapType map(s_allocator_p);
        map.reserve(k_NUM_ELEMENTS);

        bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_NUM_ELEMENTS; ++i) {
            map.insert(bsl::make_pair(i, i));
        }
        bsls::Types::Int64 end = bsls::TimeUtil::getTimer();
        umTime                 = end - begin;
        cout << "Time diff (unordered_map) : " << umTime << endl;
    }
}

BSLA_MAYBE_UNUSED static void testN2_erasePerformanceOrdered()
// ------------------------------------------------------------------------
// ERASE PERFORMANCE
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ERASE");

    // Performance comparison of erase() with bsl::unordered_map

    // Insert elements, iterate and erase while iterating

    const size_t       k_NUM_ELEMENTS = 5000000;
    bsls::Types::Int64 ohmTime;
    {
        typedef mwcc::OrderedHashMap<size_t, size_t> MyMapType;
        typedef MyMapType::iterator                  IterType;
        typedef bsl::pair<IterType, bool>            RcType;

        MyMapType map(s_allocator_p);
        // Insert 1M elements
        for (size_t i = 0; i < k_NUM_ELEMENTS; ++i) {
            RcType rc = map.insert(bsl::make_pair(i, i));
            ASSERT_EQ_D(i, i, rc.first->first);
            ASSERT_EQ_D(i, i, rc.first->second);
        }

        // Iterate and erase
        IterType           it    = map.begin();
        bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        while (it != map.end()) {
            map.erase(it++);
        }
        bsls::Types::Int64 end = bsls::TimeUtil::getTimer();
        ohmTime                = end - begin;
        cout << "Time diff (OrderedHashMap): " << ohmTime << endl;
    }
}

BSLA_MAYBE_UNUSED static void testN2_erasePerformanceUnordered()
// ------------------------------------------------------------------------
// ERASE PERFORMANCE
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ERASE");

    // Performance comparison of erase() with bsl::unordered_map

    // Insert elements, iterate and erase while iterating

    const size_t       k_NUM_ELEMENTS = 5000000;
    bsls::Types::Int64 umTime;
    {
        typedef bsl::unordered_map<size_t, size_t> MyMapType;
        typedef MyMapType::iterator                IterType;
        typedef bsl::pair<IterType, bool>          RcType;

        MyMapType map(s_allocator_p);
        // Insert 1M elements
        for (size_t i = 0; i < k_NUM_ELEMENTS; ++i) {
            RcType rc = map.insert(bsl::make_pair(i, i));
            ASSERT_EQ_D(i, i, rc.first->first);
            ASSERT_EQ_D(i, i, rc.first->second);
        }

        // Iterate and erase
        IterType           it    = map.begin();
        bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        while (it != map.end()) {
            map.erase(it++);
        }
        bsls::Types::Int64 end = bsls::TimeUtil::getTimer();
        umTime                 = end - begin;
        cout << "Time diff (unordered_map) : " << umTime << endl;
    }
}

BSLA_MAYBE_UNUSED static void testN3_profile()
// ------------------------------------------------------------------------
// PROFILE
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("PROFILE");

    // A simple snippet which inserts elements in the ordered hash map.
    // This case can be used to profile the component.
    const size_t k_NUM_ELEMENTS = 5000000;

    typedef mwcc::OrderedHashMap<size_t, size_t> MyMapType;
    MyMapType map(k_NUM_ELEMENTS, s_allocator_p);

    // Insert elements.
    for (size_t i = 0; i < k_NUM_ELEMENTS; ++i) {
        map.insert(bsl::make_pair(i, i));
    }
}

// Begin benchmarking library tests (Linux only)
#ifdef BSLS_PLATFORM_OS_LINUX

static void
testN1_insertPerformanceUnordered_GoogleBenchmark(benchmark::State& state)
{
    mwctst::TestHelper::printTestName("INSERT PERFORMANCE");

    // Performance comparison of insert() with bsl::unordered_map
    {
        // UnorderedMap
        typedef bsl::unordered_map<size_t, size_t> MyMapType;
        MyMapType map(state.range(0), s_allocator_p);
        for (auto _ : state) {
            for (size_t i = 0; i < static_cast<size_t>(state.range(0)); ++i) {
                map.insert(bsl::make_pair(i, i));
            }
        }
    }
}

static void
testN1_insertPerformanceOrdered_GoogleBenchmark(benchmark::State& state)
{
    mwctst::TestHelper::printTestName("INSERT PERFORMANCE");

    // Performance comparison of insert() with bsl::unordered_map
    {
        // OrderedHashMap
        typedef mwcc::OrderedHashMap<size_t, size_t> MyMapType;

        MyMapType map(static_cast<int>(state.range(0)), s_allocator_p);
        for (auto _ : state) {
            for (size_t i = 0; i < static_cast<size_t>(state.range(0)); ++i) {
                map.insert(bsl::make_pair(i, i));
            }
        }
    }
}

static void
testN2_erasePerformanceUnordered_GoogleBenchmark(benchmark::State& state)
{
    // Unordered Map Erase Performance Test
    typedef bsl::unordered_map<size_t, size_t> MyMapType;
    typedef MyMapType::iterator                IterType;
    typedef bsl::pair<IterType, bool>          RcType;

    MyMapType map(s_allocator_p);
    for (auto _ : state) {
        state.PauseTiming();
        for (size_t i = 0; i < static_cast<size_t>(state.range(0)); ++i) {
            RcType rc = map.insert(bsl::make_pair(i, i));
            ASSERT_EQ_D(i, i, rc.first->first);
            ASSERT_EQ_D(i, i, rc.first->second);
        }
        // Iterate and erase
        IterType it = map.begin();
        state.ResumeTiming();
        while (it != map.end()) {
            map.erase(it++);
        }
    }
}

static void
testN2_erasePerformanceOrdered_GoogleBenchmark(benchmark::State& state)
{
    mwctst::TestHelper::printTestName("ERASE");

    // Performance comparison of erase() with bsl::unordered_map

    // Insert elements, iterate and erase while iterating
    typedef mwcc::OrderedHashMap<size_t, size_t> MyMapType;
    typedef MyMapType::iterator                  IterType;
    typedef bsl::pair<IterType, bool>            RcType;

    MyMapType map(s_allocator_p);
    // Insert 1M elements
    for (auto _ : state) {
        state.PauseTiming();
        for (size_t i = 0; i < static_cast<size_t>(state.range(0)); ++i) {
            RcType rc = map.insert(bsl::make_pair(i, i));
            ASSERT_EQ_D(i, i, rc.first->first);
            ASSERT_EQ_D(i, i, rc.first->second);
        }
        // Iterate and erase
        IterType it = map.begin();
        state.ResumeTiming();
        while (it != map.end()) {
            map.erase(it++);
        }
    }
}

static void testN3_profile_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// PROFILE
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("PROFILE");

    // A simple snippet which inserts elements in the ordered hash map.
    // This case can be used to profile the component.

    typedef mwcc::OrderedHashMap<size_t, size_t> MyMapType;
    MyMapType map(static_cast<int>(state.range(0)), s_allocator_p);

    // Insert elements.
    for (auto _ : state) {
        for (size_t i = 0; i < static_cast<size_t>(state.range(0)); ++i) {
            map.insert(bsl::make_pair(i, i));
        }
    }
}
#endif  // BSLS_PLATFORM_OS_LINUX
//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // One time initialization
    bsls::TimeUtil::initialize();

    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 15: test15_eraseRange(); break;
    case 14: test14_localIterator(); break;
    case 13: test13_previousEndIterator(); break;
    case 12: test12_assignmentOperator(); break;
    case 11: test11_copyConstructor(); break;
    case 10: test10_erasureIterator(); break;
    case 9: test9_insertFailure(); break;
    case 8: test8_eraseClear(); break;
    case 7: test7_erase(); break;
    case 6: test6_clear(); break;
    case 5: test5_insertEraseInsert(); break;
    case 4: test4_rinsert(); break;
    case 3: test3_insert(); break;
    case 2: test2_impDetails_nextPrime(); break;
    case 1: test1_breathingTest(); break;
    case -1:
        MWC_BENCHMARK_WITH_ARGS(testN1_insertPerformanceOrdered,
                                RangeMultiplier(10)
                                    ->Range(10, 5000000)
                                    ->Unit(benchmark::kMillisecond));
        MWC_BENCHMARK_WITH_ARGS(testN1_insertPerformanceUnordered,
                                RangeMultiplier(10)
                                    ->Range(10, 5000000)
                                    ->Unit(benchmark::kMillisecond));
        break;
    case -2:
        MWC_BENCHMARK_WITH_ARGS(testN2_erasePerformanceUnordered,
                                RangeMultiplier(10)
                                    ->Range(100, 50000000)
                                    ->Unit(benchmark::kMillisecond));
        MWC_BENCHMARK_WITH_ARGS(testN2_erasePerformanceOrdered,
                                RangeMultiplier(10)
                                    ->Range(100, 50000000)
                                    ->Unit(benchmark::kMillisecond));
        break;
    case -3:
        MWC_BENCHMARK_WITH_ARGS(testN3_profile,
                                RangeMultiplier(10)
                                    ->Range(10, 5000000)
                                    ->Unit(benchmark::kMillisecond));
        break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }
#ifdef BSLS_PLATFORM_OS_LINUX
    if (_testCase < 0) {
        benchmark::Initialize(&argc, argv);
        benchmark::RunSpecifiedBenchmarks();
    }
#endif

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
