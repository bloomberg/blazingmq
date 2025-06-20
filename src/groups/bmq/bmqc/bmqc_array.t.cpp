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

// bmqc_array.t.cpp                                                   -*-C++-*-
#include <bmqc_array.h>

// BDE
#include <bsl_algorithm.h>
#include <bsl_cstdio.h>
#include <bsl_cstdlib.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_default.h>
#include <bslma_testallocator.h>
#include <bslmf_if.h>
#include <bslmf_isconst.h>

#include <bsls_platform.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

#ifdef BMQTST_BENCHMARK_ENABLED
// BENCHMARK
#include <benchmark/benchmark.h>
#endif  // BMQTST_BENCHMARK_ENABLED

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

struct TestType {
    // CLASS LEVEL DATA
    static int s_numAliveInstances;

    // DATA
    bslma::Allocator* d_allocator_p;
    bsl::string       d_value;

    // CREATORS
    explicit TestType(int value, bslma::Allocator* basicAllocator = 0)
    : d_allocator_p(bslma::Default::allocator(basicAllocator))
    , d_value(basicAllocator)
    {
        d_value.resize(16);
        bsl::sprintf(&d_value[0], "%d", value);
        ++s_numAliveInstances;
    }

    explicit TestType(const char* value, bslma::Allocator* basicAllocator = 0)
    : d_allocator_p(bslma::Default::allocator(basicAllocator))
    , d_value(value, basicAllocator)
    {
        ++s_numAliveInstances;
    }

    TestType(const TestType& other, bslma::Allocator* basicAllocator = 0)
    : d_allocator_p(bslma::Default::allocator(basicAllocator))
    , d_value(other.d_value, basicAllocator)
    {
        ++s_numAliveInstances;
    }

    ~TestType() { --s_numAliveInstances; }

    // MANIPULATORS
    TestType& operator=(const TestType& rhs)
    {
        if (this != &rhs) {
            d_value = rhs.d_value;
        }

        return *this;
    }

    // ACCESSORS
    int valueAsInt() const { return bsl::stoi(d_value); }
};

// FREE OPERATORS
bool operator==(const TestType& lhs, const TestType& rhs)
{
    return lhs.valueAsInt() == rhs.valueAsInt();
}

bool operator<(const TestType& lhs, const TestType& rhs)
{
    return lhs.valueAsInt() < rhs.valueAsInt();
}

int TestType::s_numAliveInstances(0);

// NOTE: Throughout this test driver, we only instantiate the following
//       'bmqc::Array' of templated type 'TestType' and size 'k_STATIC_LEN' to
//       assist in providing meaningful coverage report.
const int                                   k_STATIC_LEN = 10;
typedef bmqc::Array<TestType, k_STATIC_LEN> ObjType;

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the component.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    {
        PV("Basic accessors and manipulators");

        ObjType        obj(bmqtst::TestHelperUtil::allocator());
        const ObjType& constObj = obj;

        BMQTST_ASSERT_EQ(true, obj.empty());
        BMQTST_ASSERT_EQ(0UL, obj.size());
        BMQTST_ASSERT_EQ(bmqtst::TestHelperUtil::allocator(),
                         obj.get_allocator().mechanism());
        BMQTST_ASSERT_EQ(true, obj.begin() == obj.end());
        BMQTST_ASSERT_EQ(true, constObj.begin() == constObj.end());
    }

    {
        PV("'push_back' and iteration via 'begin' & 'end'");

        ObjType        obj(bmqtst::TestHelperUtil::allocator());
        const ObjType& constObj = obj;

        const int k_NB_ITEMS = 50;
        for (int i = 1; i <= k_NB_ITEMS; ++i) {
            obj.push_back(TestType(i, bmqtst::TestHelperUtil::allocator()));
        }

        BMQTST_ASSERT_EQ(false, obj.empty());
        BMQTST_ASSERT_EQ(static_cast<size_t>(k_NB_ITEMS), obj.size());
        BMQTST_ASSERT_EQ(true, obj.begin() != obj.end());

        for (int idx = 0, i = 1; idx < k_NB_ITEMS; ++idx, ++i) {
            BMQTST_ASSERT_EQ_D(idx, i, obj[idx].valueAsInt());
            BMQTST_ASSERT_EQ_D(idx, i, constObj[idx].valueAsInt());
        }

        int i = 1;
        for (ObjType::iterator it = obj.begin(); it != obj.end(); ++it, ++i) {
            BMQTST_ASSERT_EQ_D(i, it->valueAsInt(), i);
            BMQTST_ASSERT_EQ_D(i, (*it).valueAsInt(), i);
        }
        i = 1;
        for (ObjType::const_iterator it = constObj.begin();
             it != constObj.end();
             ++it, ++i) {
            BMQTST_ASSERT_EQ_D(i, it->valueAsInt(), i);
            BMQTST_ASSERT_EQ_D(i, (*it).valueAsInt(), i);
        }
    }

    {
        PV("Random access iterator");

        ObjType obj(bmqtst::TestHelperUtil::allocator());

        const int k_NB_ITEMS = 50;
        for (int i = 1; i <= k_NB_ITEMS; ++i) {
            obj.push_back(TestType(i, bmqtst::TestHelperUtil::allocator()));
        }

        // Iterator operator+=
        int i = 1;
        for (ObjType::iterator it = obj.begin(); it != obj.end();
             it += 1, ++i) {
            BMQTST_ASSERT_EQ_D(i, it->valueAsInt(), i);
            BMQTST_ASSERT_EQ_D(i, (*it).valueAsInt(), i);
        }

        // Iterator operator-=
        i = k_NB_ITEMS;
        for (ObjType::iterator it = obj.end(); i > 0; --i) {
            it -= 1;
            BMQTST_ASSERT_EQ_D(i, it->valueAsInt(), i);
            BMQTST_ASSERT_EQ_D(i, (*it).valueAsInt(), i);
        }

        {
            // Misc. iterator expressions
            ObjType::iterator it = obj.begin();
            BMQTST_ASSERT_EQ(it->valueAsInt(), 1);
            BMQTST_ASSERT_EQ((++it)->valueAsInt(), 2);
            BMQTST_ASSERT_EQ((--it)->valueAsInt(), 1);
            BMQTST_ASSERT_EQ(it[0].valueAsInt(), 1);

            ObjType::iterator it2 = it++;
            BMQTST_ASSERT_EQ(it2->valueAsInt(), 1);
            BMQTST_ASSERT_EQ(it->valueAsInt(), 2);
            BMQTST_ASSERT(it != it2);
            BMQTST_ASSERT(it > it2);
            BMQTST_ASSERT(it >= it2);
            BMQTST_ASSERT(it2 <= it);
            BMQTST_ASSERT(it2 < it);
            BMQTST_ASSERT(it == it2 + 1);
            BMQTST_ASSERT(it == 1 + it2);
            BMQTST_ASSERT(it - 1 == it2);
            BMQTST_ASSERT(it - it2 == 1);
            BMQTST_ASSERT(it[0] == it2[1]);

            ObjType::iterator it3 = --it;
            BMQTST_ASSERT_EQ(it3->valueAsInt(), 1);
            BMQTST_ASSERT_EQ(it2->valueAsInt(), 1);
            BMQTST_ASSERT(it == it3);
            BMQTST_ASSERT(it >= it3);
            BMQTST_ASSERT(it <= it3);
            BMQTST_ASSERT(it + 1 == 1 + it);
            BMQTST_ASSERT(it3 + 1 == 1 + it3);
            BMQTST_ASSERT(it + 1 == it3 + 1);
            BMQTST_ASSERT(it - it3 == 0);
            BMQTST_ASSERT(it3 - it == 0);
            BMQTST_ASSERT(it[2] == it3[2]);
            BMQTST_ASSERT(*it++ == *it3);
        }

        {
            // const-iterator
            ObjType::const_iterator it = obj.begin();
            BMQTST_ASSERT_EQ(it->valueAsInt(), 1);
            BMQTST_ASSERT_EQ((++it)->valueAsInt(), 2);
            BMQTST_ASSERT_EQ((--it)->valueAsInt(), 1);

            ObjType::const_iterator it2 = it++;
            BMQTST_ASSERT_EQ(it2->valueAsInt(), 1);
            BMQTST_ASSERT_EQ(it->valueAsInt(), 2);

            ObjType::const_iterator it3 = --it;
            BMQTST_ASSERT_EQ(it3->valueAsInt(), 1);
            BMQTST_ASSERT_EQ(it2->valueAsInt(), 1);
        }
    }

    {
        PV("Count constructor");

        const int k_NB_ITEMS = 100;

        ObjType obj(k_NB_ITEMS,
                    TestType(1, bmqtst::TestHelperUtil::allocator()),
                    bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(false, obj.empty());
        BMQTST_ASSERT_EQ(static_cast<size_t>(k_NB_ITEMS), obj.size());
        BMQTST_ASSERT_EQ(
            k_NB_ITEMS,
            bsl::count(obj.begin(),
                       obj.end(),
                       TestType(1, bmqtst::TestHelperUtil::allocator())));
    }

    {
        PV("InputIterator constructor");

        const int             k_NB_ITEMS = 100;
        bsl::vector<TestType> v(k_NB_ITEMS,
                                TestType(1,
                                         bmqtst::TestHelperUtil::allocator()),
                                bmqtst::TestHelperUtil::allocator());

        ObjType obj(v.begin(), v.end(), bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(false, obj.empty());
        BMQTST_ASSERT_EQ(static_cast<size_t>(k_NB_ITEMS), obj.size());
        BMQTST_ASSERT_EQ(
            k_NB_ITEMS,
            bsl::count(obj.begin(),
                       obj.end(),
                       TestType(1, bmqtst::TestHelperUtil::allocator())));
    }

    {
        PV("Copy constructor");

        ObjType obj(bmqtst::TestHelperUtil::allocator());

        const int k_NB_ITEMS = 100;

        for (int i = 1; i <= k_NB_ITEMS; ++i) {
            obj.push_back(TestType(i, bmqtst::TestHelperUtil::allocator()));
        }

        ObjType objCopy(obj, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(false, objCopy.empty());
        BMQTST_ASSERT_EQ(static_cast<size_t>(k_NB_ITEMS), objCopy.size());
        BMQTST_ASSERT_EQ(true, objCopy.begin() != objCopy.end());

        int i = 1;
        for (ObjType::iterator it = objCopy.begin(); it != objCopy.end();
             ++it) {
            BMQTST_ASSERT_EQ_D(i, it->valueAsInt(), i);
            ++i;
        }

        objCopy.clear();
        BMQTST_ASSERT_EQ(true, objCopy.empty());
        BMQTST_ASSERT_EQ(0UL, objCopy.size());
        BMQTST_ASSERT_EQ(true, objCopy.begin() == objCopy.end());

        BMQTST_ASSERT_EQ(static_cast<size_t>(k_NB_ITEMS), obj.size());
    }

    {
        PV("Assignment operator");

        ObjType obj1(bmqtst::TestHelperUtil::allocator());
        ObjType obj2(bmqtst::TestHelperUtil::allocator());

        const int k_NB_ITEMS_1 = 100;
        const int k_NB_ITEMS_2 = 500;

        for (int i = 1; i <= k_NB_ITEMS_1; ++i) {
            obj1.push_back(TestType(i, bmqtst::TestHelperUtil::allocator()));
        }

        for (int i = 1; i <= k_NB_ITEMS_2; ++i) {
            obj2.push_back(TestType(i, bmqtst::TestHelperUtil::allocator()));
        }

        BMQTST_ASSERT_EQ(false, obj2.empty());
        BMQTST_ASSERT_EQ(static_cast<size_t>(k_NB_ITEMS_2), obj2.size());
        BMQTST_ASSERT_EQ(true, obj2.begin() != obj2.end());

        // Self assignment
        obj1 = obj1;
        BMQTST_ASSERT_EQ(false, obj1.empty());
        BMQTST_ASSERT_EQ(static_cast<size_t>(k_NB_ITEMS_1), obj1.size());
        BMQTST_ASSERT_EQ(true, obj1.begin() != obj1.end());

        obj2 = obj1;

        BMQTST_ASSERT_EQ(false, obj2.empty());
        BMQTST_ASSERT_EQ(static_cast<size_t>(k_NB_ITEMS_1), obj2.size());
        BMQTST_ASSERT_EQ(true, obj2.begin() != obj2.end());
    }

    {
        PV("front() and back()");

        ObjType        obj(bmqtst::TestHelperUtil::allocator());
        const ObjType& constObj = obj;

        for (int i = 1; i <= 2 * k_STATIC_LEN; ++i) {
            obj.push_back(TestType(i, bmqtst::TestHelperUtil::allocator()));
            BMQTST_ASSERT_EQ(1, obj.front().valueAsInt());
            BMQTST_ASSERT_EQ(i, obj.back().valueAsInt());

            BMQTST_ASSERT_EQ(1, constObj.front().valueAsInt());
            BMQTST_ASSERT_EQ(i, constObj.back().valueAsInt());
        }
    }

    {
        PV("'clear', 'operator=' and 'destructor' invoke ~TYPE");

        {
            ObjType obj(bmqtst::TestHelperUtil::allocator());

            const int k_NB_ITEMS = 50;

            for (int i = 0; i < k_NB_ITEMS; ++i) {
                obj.push_back(
                    TestType("a", bmqtst::TestHelperUtil::allocator()));
            }

            BMQTST_ASSERT_EQ(k_NB_ITEMS, TestType::s_numAliveInstances);

            obj.clear();

            BMQTST_ASSERT_EQ(0, TestType::s_numAliveInstances);
        }

        {
            ObjType obj1(bmqtst::TestHelperUtil::allocator());
            ObjType obj2(bmqtst::TestHelperUtil::allocator());

            for (int i = 0; i < 50; ++i) {
                obj1.push_back(
                    TestType("a", bmqtst::TestHelperUtil::allocator()));
            }

            for (int i = 0; i < 100; ++i) {
                obj2.push_back(
                    TestType("b", bmqtst::TestHelperUtil::allocator()));
            }

            BMQTST_ASSERT_EQ(150, TestType::s_numAliveInstances);

            obj2 = obj1;

            BMQTST_ASSERT_EQ(100, TestType::s_numAliveInstances);
        }

        {
            ObjType obj1(bmqtst::TestHelperUtil::allocator());
            ObjType obj2(bmqtst::TestHelperUtil::allocator());

            for (int i = 0; i < 50; ++i) {
                obj1.push_back(
                    TestType("a", bmqtst::TestHelperUtil::allocator()));
            }

            for (int i = 0; i < 100; ++i) {
                obj2.push_back(
                    TestType("b", bmqtst::TestHelperUtil::allocator()));
            }

            BMQTST_ASSERT_EQ(150, TestType::s_numAliveInstances);

            obj1 = obj2;

            BMQTST_ASSERT_EQ(200, TestType::s_numAliveInstances);
        }

        {
            ObjType* obj = new (*bmqtst::TestHelperUtil::allocator())
                ObjType(bmqtst::TestHelperUtil::allocator());

            for (int i = 0; i < 50; ++i) {
                obj->push_back(
                    TestType("a", bmqtst::TestHelperUtil::allocator()));
            }

            BMQTST_ASSERT_EQ(50, TestType::s_numAliveInstances);

            bmqtst::TestHelperUtil::allocator()->deleteObject(obj);

            BMQTST_ASSERT_EQ(0, TestType::s_numAliveInstances);
        }
    }
}

static void test2_outOfBoundValidation()
// ------------------------------------------------------------------------
// OUT OF BOUND VALIDATION
//
// Concerns:
//   Verifies that out of bound access and iteration are asserting under
//   safe mode.
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("OUT OF BOUND VALIDATION");

    {
        PV("front, back and []");

        ObjType        obj(bmqtst::TestHelperUtil::allocator());
        const ObjType& constObj = obj;

        // non-const
        BMQTST_ASSERT_SAFE_FAIL(obj.front());
        BMQTST_ASSERT_SAFE_FAIL(obj.back());
        BMQTST_ASSERT_SAFE_FAIL(obj[0]);

        // const
        BMQTST_ASSERT_SAFE_FAIL(constObj.front());
        BMQTST_ASSERT_SAFE_FAIL(constObj.back());
        BMQTST_ASSERT_SAFE_FAIL(constObj[0]);

        // insert some items
        const int k_NB_ITEMS = 5;
        for (int i = 1; i <= k_NB_ITEMS; ++i) {
            obj.push_back(TestType(i, bmqtst::TestHelperUtil::allocator()));
        }

        BMQTST_ASSERT_SAFE_FAIL(constObj[k_NB_ITEMS + 1]);

        static_cast<void>(constObj);
    }
}

static void test3_noMemoryAllocation()
// ------------------------------------------------------------------------
// NO MEMORY ALLOCATION
//
// Concerns:
//   Verifies that no memory is allocated while the Array is within it's
//   static length.
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("NO MEMORY ALLOCATION");

    bslma::TestAllocator ta("testAlloc");

    ObjType obj(&ta);

    BMQTST_ASSERT_EQ(ta.numBlocksInUse(), 0);

    // Add up to k_STATIC_LEN, no allocations
    for (int i = 1; i <= k_STATIC_LEN; ++i) {
        obj.push_back(TestType(i, bmqtst::TestHelperUtil::allocator()));
        BMQTST_ASSERT_EQ(ta.numBlocksInUse(), 0);
    }

    // Adding one more, allocation"
    obj.push_back(TestType(0, bmqtst::TestHelperUtil::allocator()));
    BMQTST_ASSERT_NE(ta.numBlocksInUse(), 0);
}

static void test4_reserve()
// ------------------------------------------------------------------------
// RESERVE
//
// Concerns:
//   Verifies that no memory is allocated after calling reserve with enough
//   capacity for the items.
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("RESERVE");

    bslma::TestAllocator ta("testAlloc");

    ObjType obj(&ta);

    const int k_CAPACITY = k_STATIC_LEN + 500;

    obj.reserve(k_CAPACITY);
    const bsls::Types::Int64 numAllocations = ta.numAllocations();

    // Add up to k_CAPACITY items, no allocations
    for (int i = 1; i <= k_CAPACITY; ++i) {
        obj.push_back(TestType(i, bmqtst::TestHelperUtil::allocator()));
    }
    BMQTST_ASSERT_EQ(ta.numAllocations(), numAllocations);
}

static void test5_resize()
// ------------------------------------------------------------------------
// RESIZE
//
// Concerns:
//   Functionality of the 'resize' method with respect to contractual
//   guarantees as well as memory allocation.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("RESIZE");

    bslma::TestAllocator ta("testAlloc");

    ObjType obj(&ta);

    const bsls::Types::Int64 numAllocations = ta.numAllocations();

    // 1) Resize to static length
    obj.resize(k_STATIC_LEN, TestType(1, bmqtst::TestHelperUtil::allocator()));

    BMQTST_ASSERT_EQ(obj.size(), static_cast<size_t>(k_STATIC_LEN));
    BMQTST_ASSERT_EQ(TestType::s_numAliveInstances, k_STATIC_LEN);
    BMQTST_ASSERT_EQ(ta.numAllocations(), numAllocations);

    // 2) Resize to half the static length
    obj.resize(k_STATIC_LEN / 2,
               TestType(2, bmqtst::TestHelperUtil::allocator()));

    BMQTST_ASSERT_EQ(obj.size(), static_cast<size_t>(k_STATIC_LEN / 2));
    BMQTST_ASSERT_EQ(TestType::s_numAliveInstances, k_STATIC_LEN / 2);
    BMQTST_ASSERT_EQ(ta.numAllocations(), numAllocations);

    for (size_t i = 0; i < obj.size(); ++i) {
        BMQTST_ASSERT_EQ_D(i, obj[i].valueAsInt(), 1);
    }

    // k_FULL_LENGTH exercises both static and dynamic parts of 'bmqc_array'.
    const size_t k_FULL_LEN = 2 * k_STATIC_LEN;

    // 3) Resize to twice the static length
    obj.resize(2 * k_STATIC_LEN,
               TestType(3, bmqtst::TestHelperUtil::allocator()));

    BMQTST_ASSERT_EQ(obj.size(), static_cast<size_t>(k_FULL_LEN));
    BMQTST_ASSERT_EQ(TestType::s_numAliveInstances, 2 * k_STATIC_LEN);
    BMQTST_ASSERT_EQ(ta.numAllocations(), numAllocations + 1);

    for (size_t i = 0; i < k_FULL_LEN; ++i) {
        const int expected = (i < (k_STATIC_LEN / 2)) ? 1 : 3;
        BMQTST_ASSERT_EQ_D(i, obj[i].valueAsInt(), expected);
    }

    // 4) Again resize to twice the static length (i.e. same size)
    obj.resize(k_FULL_LEN, TestType(4, bmqtst::TestHelperUtil::allocator()));

    BMQTST_ASSERT_EQ(obj.size(), static_cast<size_t>(k_FULL_LEN));
    BMQTST_ASSERT_EQ(TestType::s_numAliveInstances, 2 * k_STATIC_LEN);
    BMQTST_ASSERT_EQ(ta.numAllocations(), numAllocations + 1);

    for (size_t i = 0; i < k_FULL_LEN; ++i) {
        const int expected = (i < (k_STATIC_LEN / 2)) ? 1 : 3;
        BMQTST_ASSERT_EQ_D(i, obj[i].valueAsInt(), expected);
    }

    // 5) Resize to zero
    obj.resize(0, TestType(5, bmqtst::TestHelperUtil::allocator()));

    BMQTST_ASSERT(obj.empty());
    BMQTST_ASSERT_EQ(obj.size(), 0UL);
    BMQTST_ASSERT_EQ(TestType::s_numAliveInstances, 0);
    BMQTST_ASSERT_EQ(ta.numAllocations(), numAllocations + 1);
}

static void test6_assign()
// ------------------------------------------------------------------------
// ASSIGN
//
// Concerns:
//   Basic functionality of the 'assign' method.
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ASSIGN");

    bslma::TestAllocator ta("testAlloc");

    ObjType obj(&ta);
    for (int i = 0; i < k_STATIC_LEN + 5; ++i) {
        obj.push_back(TestType(i, bmqtst::TestHelperUtil::allocator()));
    }

    bsl::vector<TestType> srcVec(bmqtst::TestHelperUtil::allocator());
    for (int i = 0; i < 2 * k_STATIC_LEN; ++i) {
        srcVec.push_back(
            TestType(10 * i, bmqtst::TestHelperUtil::allocator()));
    }

    // Assign to 'obj' from 'srcVec'
    obj.assign(srcVec.begin(), srcVec.end());

    // Verify
    BMQTST_ASSERT_EQ(obj.size(), srcVec.size());
    for (size_t i = 0; i < obj.size(); ++i) {
        BMQTST_ASSERT_EQ_D(i, obj[i].valueAsInt(), srcVec[i].valueAsInt());
    }
}

static void test7_algorithms()
// ------------------------------------------------------------------------
// ALGORITHMS
//
// Concerns:
//   Make sure that the array and elements in it are in sane condition
//   after invoking std algos on a populated array.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ALGORITHMS");

    {
        PV("SORT");

        ObjType obj(bmqtst::TestHelperUtil::allocator());

        // Populate array: {30, 29, ..., 1}
        for (int i = 30; i >= 0; --i) {
            obj.push_back(TestType(i, bmqtst::TestHelperUtil::allocator()));
        }

        // Apply 'bsl::sort'
        bsl::sort(obj.begin(), obj.end());

        // Verify: {1, 2, ..., 30}
        for (int i = 0; i <= 30; ++i) {
            BMQTST_ASSERT_EQ_D(i, obj[i].valueAsInt(), i);
        }
    }
}

static void test8_allocatorProp()
// ------------------------------------------------------------------------
// ALLOCATOR PROPAGATION
//
// Concerns:
//   Make sure that the array copies elements while propagating the correct
//   allocator.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ALLOCPROP");

    {
        PV("ALLOC.UNAWARE");

        bslma::TestAllocator ta;

        ObjType obj(&ta);

        obj.push_back(TestType(0, &ta));
        BMQTST_ASSERT_EQ(true, obj.back().d_allocator_p != &ta);
    }

    {
        PV("ALLOC.AWARE");

        bslma::TestAllocator ta;

        bmqc::Array<bsl::string, 10> obj(&ta);

        obj.push_back("foo");
        BMQTST_ASSERT_EQ(true, obj.back().get_allocator().mechanism() == &ta);
    }
}

static void test9_copyAssignDifferentStaticLength()
// ------------------------------------------------------------------------
// COPY ASSIGN DIFFERENT STATIC LENGTH
//
// Concerns:
//   Make sure that the array copy constructor and copy assignment works
//   for arrays of different static lengths.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("COPYASSIGNDIFFLEN");

    {
        bslma::TestAllocator ta;

        bmqc::Array<TestType, 2> rhs(&ta);

        rhs.resize(2, TestType(42));

        {
            ObjType obj(rhs, &ta);
            BMQTST_ASSERT_EQ(obj.size(), 2u);
            BMQTST_ASSERT_EQ(obj.capacity(), 10u);
            BMQTST_ASSERT_EQ(true,
                             bsl::equal(obj.begin(), obj.end(), rhs.begin()));
        }

        rhs.resize(42, TestType(2));

        {
            ObjType obj(rhs, &ta);
            BMQTST_ASSERT_EQ(obj.size(), 42u);
            BMQTST_ASSERT_EQ(obj.capacity(), 42u);
            BMQTST_ASSERT_EQ(true,
                             bsl::equal(obj.begin(), obj.end(), rhs.begin()));
        }
    }

    {
        bslma::TestAllocator ta;

        bmqc::Array<TestType, 2> rhs(&ta);

        rhs.resize(2, TestType(42));

        {
            ObjType obj(&ta);
            obj = rhs;
            BMQTST_ASSERT_EQ(obj.size(), 2u);
            BMQTST_ASSERT_EQ(obj.capacity(), 10u);
            BMQTST_ASSERT_EQ(true,
                             bsl::equal(obj.begin(), obj.end(), rhs.begin()));
        }

        rhs.resize(42, TestType(2));

        {
            ObjType obj(&ta);
            obj = rhs;
            BMQTST_ASSERT_EQ(obj.size(), 42u);
            BMQTST_ASSERT_EQ(obj.capacity(), 42u);
            BMQTST_ASSERT_EQ(true,
                             bsl::equal(obj.begin(), obj.end(), rhs.begin()));
        }
    }

    {
        bslma::TestAllocator ta;

        bmqc::Array<TestType, 24> rhs(&ta);

        rhs.resize(2, TestType(42));

        {
            ObjType obj(rhs, &ta);
            BMQTST_ASSERT_EQ(obj.size(), 2u);
            BMQTST_ASSERT_EQ(obj.capacity(), 10u);
            BMQTST_ASSERT_EQ(true,
                             bsl::equal(obj.begin(), obj.end(), rhs.begin()));
        }

        rhs.resize(42, TestType(2));

        {
            ObjType obj(rhs, &ta);
            BMQTST_ASSERT_EQ(obj.size(), 42u);
            BMQTST_ASSERT_EQ(obj.capacity(), 42u);
            BMQTST_ASSERT_EQ(true,
                             bsl::equal(obj.begin(), obj.end(), rhs.begin()));
        }
    }

    {
        bslma::TestAllocator ta;

        bmqc::Array<TestType, 24> rhs(&ta);

        rhs.resize(2, TestType(42));

        {
            ObjType obj(&ta);
            obj = rhs;
            BMQTST_ASSERT_EQ(obj.size(), 2u);
            BMQTST_ASSERT_EQ(obj.capacity(), 10u);
            BMQTST_ASSERT_EQ(true,
                             bsl::equal(obj.begin(), obj.end(), rhs.begin()));
        }

        rhs.resize(42, TestType(2));

        {
            ObjType obj(&ta);
            obj = rhs;
            BMQTST_ASSERT_EQ(obj.size(), 42u);
            BMQTST_ASSERT_EQ(obj.capacity(), 42u);
            BMQTST_ASSERT_EQ(true,
                             bsl::equal(obj.begin(), obj.end(), rhs.begin()));
        }
    }
}

static void test10_pushBackSelfRef()
// ------------------------------------------------------------------------
// PUSH BACK AN ELEMENT OF THE ARRAY BY REFERENCE
//
// Concerns:
//   Make sure that the array pushBack() method works well if an element of
//   the array is passed by reference as an argument.  Check that in case
//   of exceeded capacity it won't lead to dangling reference and UB due to
//   memory deallocation before copying the passed argument.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PUSHBACKSELFREF");

    {
        const int k_SIZE = k_STATIC_LEN * 2;
        ObjType   obj(bmqtst::TestHelperUtil::allocator());
        obj.reserve(k_SIZE);
        for (int i = 0; i < k_SIZE; ++i) {
            obj.push_back(TestType(i, bmqtst::TestHelperUtil::allocator()));
        }
        BMQTST_ASSERT_PASS(obj.push_back(obj[k_STATIC_LEN]));
        BMQTST_ASSERT_EQ(k_STATIC_LEN, obj[k_STATIC_LEN].valueAsInt());
        BMQTST_ASSERT_EQ(k_STATIC_LEN, obj[k_SIZE].valueAsInt());
    }
}

#ifdef BMQTST_BENCHMARK_ENABLED

using namespace BloombergLP;

static void VectorIteration_GoogleBenchmark(benchmark::State& state)
{
    bslma::TestAllocator ta;
    bmqc::Array<int, 16> vec(&ta);

    vec.resize(state.range(0), 42);
    for (auto _ : state) {
        for (auto it = vec.begin(); it != vec.end(); ++it) {
        }
    }
}

static void VectorFindLarge_GoogleBenchmark(benchmark::State& state)
{
    bslma::TestAllocator ta;
    bmqc::Array<int, 16> vec(&ta);

    vec.resize(state.range(0), 42);

    for (auto _ : state) {
        bsl::find(vec.begin(), vec.end(), 22);
    }
}

static void VectorPushBack_GoogleBenchmark(benchmark::State& state)
{
    bslma::TestAllocator ta;
    for (auto _ : state) {
        bmqc::Array<int, 16> vec(&ta);
        for (int i = 0; i < state.range(0); ++i) {
            vec.push_back(42);
        }
    };
}

static void VectorAssign_GoogleBenchmark(benchmark::State& state)
{
    bslma::TestAllocator ta;
    bsl::vector<int>     data(&ta);

    data.resize(state.range(0), 42);

    bmqc::Array<int, 16> vec(&ta);
    for (auto _ : state) {
        vec.assign(data.begin(), data.end());
    }
}

#endif  // BMQTST_BENCHMARK_ENABLED

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);
    switch (_testCase) {
    case 0:
    case 10: test10_pushBackSelfRef(); break;
    case 9: test9_copyAssignDifferentStaticLength(); break;
    case 8: test8_allocatorProp(); break;
    case 7: test7_algorithms(); break;
    case 6: test6_assign(); break;
    case 5: test5_resize(); break;
    case 4: test4_reserve(); break;
    case 3: test3_noMemoryAllocation(); break;
    case 2: test2_outOfBoundValidation(); break;
    case 1: test1_breathingTest(); break;
#ifdef BMQTST_BENCHMARK_ENABLED
    case -1:
        benchmark::Initialize(&argc, argv);
        BENCHMARK(VectorAssign_GoogleBenchmark)->Range(8, 4096);
        BENCHMARK(VectorPushBack_GoogleBenchmark)->Range(8, 4096);
        // Vector Iteration Small
        BENCHMARK(VectorIteration_GoogleBenchmark)->Range(16, 512);
        // Vector Iteration Large
        BENCHMARK(VectorIteration_GoogleBenchmark)->Range(1024, 32768);
        BENCHMARK(VectorFindLarge_GoogleBenchmark)->Range(256, 8192);
        benchmark::RunSpecifiedBenchmarks();
        break;
#endif  // BMQTST_BENCHMARK_ENABLED
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }
    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
