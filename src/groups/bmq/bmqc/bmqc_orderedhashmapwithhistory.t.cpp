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

// bmqc_orderedhashmapwithhistory.t.cpp                               -*-C++-*-
#include <bmqc_orderedhashmapwithhistory.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

typedef bmqc::OrderedHashMapWithHistory<size_t, size_t> ObjectUnderTest;
typedef ObjectUnderTest::iterator                       Iterator;

static void setup(ObjectUnderTest& obj, size_t total, int timeout)
{
    bsls::Types::Int64 now = 1;

    for (size_t key = 0; key < total; ++key) {
        bsl::pair<Iterator, bool> rc = obj.insert(bsl::make_pair(key, key + 1),
                                                  now);
        BMQTST_ASSERT_EQ(true, rc.second);
        BMQTST_ASSERT_EQ(true, rc.first != obj.end());
        BMQTST_ASSERT_EQ(true, rc.first == obj.begin() || key);
        BMQTST_ASSERT_EQ(key, rc.first->first);
        BMQTST_ASSERT_EQ(key + 1, rc.first->second);

        BMQTST_ASSERT_EQ(key + 1, obj.size());

        for (size_t i = 0; i <= key; ++i) {
            BMQTST_ASSERT_EQ(true, obj.isInHistory(i));
            BMQTST_ASSERT_EQ(true, obj.find(i) != obj.end());
        }
        now += timeout;
    }
}

static void test1_insert()
{
    // ------------------------------------------------------------------------
    // HISTORY
    //
    // Concerns:
    //   ObjectUnderTest should support inserting elements
    //
    // Plan:
    //   Insert N elements;
    //
    // Testing:
    //   insert, find, isInHistory, size, isInHistory
    // -
    bmqtst::TestHelper::printTestName("HISTORY");

    int             timeout = 1;
    ObjectUnderTest obj(timeout, bmqtst::TestHelperUtil::allocator());

    setup(obj, 10, timeout);
}

static void test2_eraseMiddleToEnd()
{
    // ------------------------------------------------------------------------
    // HISTORY
    //
    // Concerns:
    //   ObjectUnderTest should support erasing from the middle to the end
    //
    // Plan:
    //   Insert N elements;
    //   erase from middle to end;
    //   gc;
    //
    // Testing:
    //   insert, find, isInHistory, size, isInHistory, erase, gc
    // ------------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("ERASE_MIDDLE_TO_END");

    int             timeout = 1;
    ObjectUnderTest obj(timeout, bmqtst::TestHelperUtil::allocator());
    size_t          TOTAL = 10;
    size_t          HALF  = TOTAL / 2;

    setup(obj, TOTAL, timeout);

    // erase middle to end
    for (size_t key = HALF; key < TOTAL; ++key) {
        BMQTST_ASSERT_EQ(true, obj.isInHistory(key));

        Iterator it = obj.find(key);

        BMQTST_ASSERT_EQ(true, it != obj.end());

        obj.erase(it);

        // should be down by 1
        BMQTST_ASSERT_EQ(TOTAL + HALF - key - 1, obj.size());

        for (size_t i = HALF; i <= key; ++i) {
            BMQTST_ASSERT_EQ(true, obj.isInHistory(i));
            BMQTST_ASSERT_EQ(false, obj.find(i) != obj.end());
        }
    }

    // gc the half
    bsls::Types::Int64 now = (HALF + 2) * timeout;
    for (size_t key = HALF; key < TOTAL; ++key, now += timeout) {
        obj.gc(now);
        BMQTST_ASSERT_EQ(false, obj.isInHistory(key));
        BMQTST_ASSERT_EQ(false, obj.find(key) != obj.end());
    }
}

static void test3_eraseMiddleToBegin()
{
    // ------------------------------------------------------------------------
    // HISTORY
    //
    // Concerns:
    //   ObjectUnderTest should support erasing in backward manner
    //
    // Plan:
    //   Insert N elements;
    //   erase from middle to begin;
    //   gc
    //
    // Testing:
    //   insert, find, isInHistory, size, isInHistory, erase, gc
    // ------------------------------------------------------------------------

    bmqtst::TestHelper::printTestName("ERASE_MIDDLE_TO_BEGIN");

    int             timeout = 1;
    ObjectUnderTest obj(timeout, bmqtst::TestHelperUtil::allocator());
    size_t          TOTAL = 10;
    size_t          HALF  = TOTAL / 2;

    setup(obj, TOTAL, timeout);

    // erase end to middle
    for (size_t key = HALF; key-- > 0;) {
        BMQTST_ASSERT_EQ(true, obj.isInHistory(key));
        BMQTST_ASSERT_EQ(true, obj.find(key) != obj.end());

        Iterator it = obj.find(key);

        BMQTST_ASSERT_EQ(true, it != obj.end());

        obj.erase(it);

        // should be down by 1
        BMQTST_ASSERT_EQ(key + HALF, obj.size());

        for (size_t i = HALF; i-- > key;) {
            BMQTST_ASSERT_EQ(true, obj.isInHistory(i));
            BMQTST_ASSERT_EQ(false, obj.find(i) != obj.end());
        }
    }

    // gc the half
    bsls::Types::Int64 now = 2 * timeout;
    for (size_t key = 0; key < HALF; ++key, now += timeout) {
        obj.gc(now);
        BMQTST_ASSERT_EQ(false, obj.isInHistory(key));
        BMQTST_ASSERT_EQ(false, obj.find(key) != obj.end());
    }
}

static void test4_gc()
{
    // ------------------------------------------------------------------------
    // HISTORY
    //
    // Concerns:
    //   ObjectUnderTest should support GC'ing individual items
    //
    // Plan:
    //   Insert N elements;
    //   erase from end to begin;
    //   gc each item
    //
    // Testing:
    //   insert, find, isInHistory, size, isInHistory, erase, gc
    // ------------------------------------------------------------------------

    bmqtst::TestHelper::printTestName("ERASE_AND_GC");

    int             timeout = 1;
    ObjectUnderTest obj(timeout, bmqtst::TestHelperUtil::allocator());
    size_t          TOTAL = 10;
    size_t          HALF  = TOTAL / 2;

    setup(obj, TOTAL, timeout);

    // erase end to middle
    for (size_t key = TOTAL; key-- > HALF;) {
        BMQTST_ASSERT_EQ(true, obj.isInHistory(key));
        BMQTST_ASSERT_EQ(true, obj.find(key) != obj.end());

        Iterator it = obj.find(key);

        BMQTST_ASSERT_EQ(true, it != obj.end());

        obj.erase(it, 1 * timeout);  // 'it' is not expired yet

        // should be down by 1
        BMQTST_ASSERT_EQ(key, obj.size());

        for (size_t i = TOTAL; i-- > key;) {
            BMQTST_ASSERT_EQ(true, obj.isInHistory(i));
            BMQTST_ASSERT_EQ(false, obj.find(i) != obj.end());
        }
    }

    // erase middle to begin
    for (size_t key = HALF; key-- > 0;) {
        BMQTST_ASSERT_EQ(true, obj.isInHistory(key));
        BMQTST_ASSERT_EQ(true, obj.find(key) != obj.end());

        Iterator it = obj.find(key);

        BMQTST_ASSERT_EQ(true, it != obj.end());

        obj.erase(it, (TOTAL + 1) * timeout);  // 'it' is expired

        // should be down by 1
        BMQTST_ASSERT_EQ(key, obj.size());

        for (size_t i = HALF; i-- > key;) {
            BMQTST_ASSERT_EQ(false, obj.isInHistory(i));
            BMQTST_ASSERT_EQ(false, obj.find(i) != obj.end());
        }
    }

    BMQTST_ASSERT_EQ(0U, obj.size());
}

static void test5_insertAfterEnd()
{
    // ------------------------------------------------------------------------
    // HISTORY
    //
    // Concerns:
    //   The 'end()' iterator should point to new inserted item
    //
    // Plan:
    //   Save 'end()'.
    //   Insert.
    //   Make sure the saved iterator is not end anymore and points to the new
    //   one.
    //
    // Testing:
    //   end, insert
    // ------------------------------------------------------------------------

    bmqtst::TestHelper::printTestName("INSERT_AFTER_END");

    int             timeout = 1;
    ObjectUnderTest obj(timeout, bmqtst::TestHelperUtil::allocator());
    Iterator        begin = obj.begin();
    Iterator        end   = obj.end();

    BMQTST_ASSERT(begin == end);

    setup(obj, 1, timeout);

    BMQTST_ASSERT(begin == end);
    BMQTST_ASSERT(end != obj.end());
    BMQTST_ASSERT_EQ(0U, end->first);   // key
    BMQTST_ASSERT_EQ(1U, end->second);  // value
}

static void test6_eraseThenGc()
{
    // ------------------------------------------------------------------------
    // HISTORY
    //
    // Concerns:
    //   Make sure, 'erase' advances 'd_gcIt'. Internal-ticket D166995910
    //
    // Plan:
    //   Insert '1100' items (exceed the batch size of '1000').
    //   'erase' ('confirm') one item at the end to engage 'gc'.
    //   Advance time past the timeout and call 'gc' with the batch size of
    //   '1000'.  'd_gcIt' points at '1001'.
    //   Erase 1001st for real and make the rest history.
    //   Call 'gc' with the batch size of '1000' once.  'gc' should resume from
    //   1002nd.
    //
    // Testing:
    //   insert, gc, erase
    // ------------------------------------------------------------------------

    bmqtst::TestHelper::printTestName("INSERT_AFTER_END");

    int             timeout = 1;
    ObjectUnderTest obj(timeout, bmqtst::TestHelperUtil::allocator());
    const int       BATCH_SIZE = 1000;
    const int       ADDITIONS  = 100;

    // insert 1100 items
    setup(obj, BATCH_SIZE + ADDITIONS, timeout);

    // expire 1001
    bsls::Types::Int64 now = (BATCH_SIZE + 2) * timeout;

    // 'erase' ('confirm') 1100th item to make 'd_historySize > 0'
    obj.erase(obj.find(BATCH_SIZE + ADDITIONS - 1));

    obj.gc(now, BATCH_SIZE);  // 'gc' points at 1001st item

    // erase 1001st for real and make the rest history
    for (int key = BATCH_SIZE; key < (BATCH_SIZE + ADDITIONS - 1); ++key) {
        obj.erase(obj.find(key), now);
    }

    // resume 'gc' from 1002nd
    now += ADDITIONS * timeout;
    obj.gc(now, BATCH_SIZE);
}

static void test7_gcThenInsert()
{
    // ------------------------------------------------------------------------
    // HISTORY
    //
    // Concerns:
    //   The same key gets inserted after 'erase'.
    //
    // Plan:
    //   Insert an element.
    //   Erase the element.
    //   Insert the same key again.
    //
    // Testing:
    //   insert, erase, insert
    // ------------------------------------------------------------------------

    bmqtst::TestHelper::printTestName("INSERT_AFTER_GC");

    int             timeout = 1;
    ObjectUnderTest obj(timeout, bmqtst::TestHelperUtil::allocator());

    // insert 1 item
    setup(obj, 1, timeout);

    // 'erase' ('confirm') the item
    obj.erase(obj.find(0));

    setup(obj, 1, timeout);
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_insert(); break;
    case 2: test2_eraseMiddleToEnd(); break;
    case 3: test3_eraseMiddleToBegin(); break;
    case 4: test4_gc(); break;
    case 5: test5_insertAfterEnd(); break;
    case 6: test6_eraseThenGc(); break;
    case 7: test7_gcThenInsert(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
