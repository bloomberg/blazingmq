// Copyright 2026 Bloomberg Finance L.P.
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

#include <bmqu_synchronizedvalue.h>

// BMQ
#include <bmqtst_testhelper.h>

// BDE
#include <bsl_string.h>
#include <bsl_type_traits.h>
#include <bslma_allocator.h>
#include <bslmf_assert.h>
#include <bslmf_isconst.h>
#include <bslmt_barrier.h>
#include <bslmt_mutex.h>
#include <bsls_libraryfeatures.h>

#include <bslmt_readerwritermutex.h>
#include <bslmt_threadutil.h>
#include <bsls_systemtime.h>
#include <bsls_timeinterval.h>
#include <bslstl_sharedptr.h>
#include <gtest/gtest.h>

using namespace BloombergLP;

namespace {

class SharedValueWorker {
    bsl::shared_ptr<bmqu::SynchronizedValue<int> > d_value;
    bslmt::Barrier*                                d_barrier;

  public:
    SharedValueWorker(
        const bsl::shared_ptr<bmqu::SynchronizedValue<int> >& value,
        bslmt::Barrier*                                       barrier)
    : d_value(value)
    , d_barrier(barrier)
    {
    }

    void operator()() { acquireAndWait(); }

    void acquireAndWait()
    {
        typedef bmqu::SynchronizedValue<int>::ReadLockedPtr ReadLockedPtr;
        ReadLockedPtr      lockedPtr(d_value.get());
        bsls::TimeInterval deadline = bsls::SystemTime::nowRealtimeClock();
        deadline.addMilliseconds(500);
        int rc = d_barrier->timedWait(deadline);
        EXPECT_EQ(0, rc);
    }
};

struct SynchronizedWritesWorker {
    bsl::shared_ptr<bmqu::SynchronizedValue<int> > d_value;

  public:
    SynchronizedWritesWorker(
        const bsl::shared_ptr<bmqu::SynchronizedValue<int> >& value)
    : d_value(value)
    {
    }

    void operator()() { increment(); }

    void increment()
    {
        typedef bmqu::SynchronizedValue<int>::WriteLockedPtr WriteLockedPtr;
        WriteLockedPtr lockedPtr(d_value.get());
        int            prevValue = *lockedPtr;
        ++(*lockedPtr);
        EXPECT_EQ(prevValue + 1, *lockedPtr);
    }
};

}  // namespace

// Check that read locks have const access
BSLMF_ASSERT(
    bsl::is_const<
        bmqu::SynchronizedValue<int>::ReadLockedPtr::AccessValueType>::value);

// Check that const SynchronizedValues have const access
BSLMF_ASSERT(bsl::is_const<const bmqu::SynchronizedValue<
                 int>::WriteLockedPtr::AccessValueType>::value);

// Check that write locks have non-const access
BSLMF_ASSERT(
    !bsl::is_const<
        bmqu::SynchronizedValue<int>::WriteLockedPtr::AccessValueType>::value);

BSLMF_ASSERT(
    !bsl::is_copy_constructible<bmqu::SynchronizedValue<int> >::value);

#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_BASELINE_LIBRARY

BSLMF_ASSERT(
    !bsl::is_move_constructible<bmqu::SynchronizedValue<int> >::value);

#endif

TEST(UniqueLock, breathingTest)
{
    bmqu::SynchronizedValue_UniqueLock<bslmt::Mutex>             lock;
    bmqu::SynchronizedValue_UniqueLock<bslmt::ReaderWriterMutex> lock2;
}

TEST(UniqueLock, ownsMutexOnConstruction)
{
    bslmt::ReaderWriterMutex mutex;
    EXPECT_FALSE(mutex.isLocked());
    {
        bmqu::SynchronizedValue_UniqueLock<bslmt::ReaderWriterMutex> lock(
            &mutex);
        EXPECT_TRUE(mutex.isLocked());
        EXPECT_TRUE(mutex.isLockedWrite());
        EXPECT_TRUE(lock.ownsLock());
    }
    EXPECT_FALSE(mutex.isLocked());
}

TEST(SharedLock, breathingTest)
{
    bmqu::SynchronizedValue_SharedLock<bslmt::ReaderWriterMutex> lock;
}

TEST(SharedLock, ownsMutexOnConstruction)
{
    bslmt::ReaderWriterMutex mutex;
    EXPECT_FALSE(mutex.isLocked());
    {
        bmqu::SynchronizedValue_SharedLock<bslmt::ReaderWriterMutex> lock(
            &mutex);
        EXPECT_TRUE(mutex.isLocked());
        EXPECT_TRUE(mutex.isLockedRead());
        EXPECT_TRUE(lock.ownsLock());
    }
    EXPECT_FALSE(mutex.isLocked());
}

TEST(SynchronizedValue, breathingTest)
{
    bmqu::SynchronizedValue<int> value(0);
}

TEST(SynchronizedValue, getsReadLock)
{
    bmqu::SynchronizedValue<bsl::string> value("test");
    {
        bmqu::SynchronizedValue<bsl::string>::ReadLockedPtr lockPtr(&value);
        EXPECT_TRUE(lockPtr.ownsLock());
        EXPECT_EQ(bsl::string("test"), *lockPtr);
    }
    {
        bmqu::SynchronizedValue<bsl::string>::ConstReadLockedPtr lockPtr(
            &value);
        EXPECT_TRUE(lockPtr.ownsLock());
        EXPECT_EQ(bsl::string("test"), *lockPtr);
    }
}

TEST(SynchronizedValue, getsWriteLock)
{
    bmqu::SynchronizedValue<bsl::string> value("test");
    {
        bmqu::SynchronizedValue<bsl::string>::WriteLockedPtr lockPtr(&value);
        EXPECT_TRUE(lockPtr.ownsLock());
        EXPECT_EQ(bsl::string("test"), *lockPtr);
    }
    {
        bmqu::SynchronizedValue<bsl::string>::ConstWriteLockedPtr lockPtr(
            &value);
        EXPECT_TRUE(lockPtr.ownsLock());
        EXPECT_EQ(bsl::string("test"), *lockPtr);
    }
}

TEST(SynchronizedValue, getsLockWithMutex)
{
    typedef bmqu::SynchronizedValue<bsl::string, bslmt::Mutex>
                       SynchronizedString;
    SynchronizedString value("test");
    {
        SynchronizedString::LockedPtr lockPtr(&value);
        EXPECT_TRUE(lockPtr.ownsLock());
        EXPECT_EQ(bsl::string("test"), *lockPtr);
    }
    {
        SynchronizedString::ConstLockedPtr lockPtr(&value);
        EXPECT_TRUE(lockPtr.ownsLock());
        EXPECT_EQ(bsl::string("test"), *lockPtr);
    }
}

class SynchronizedValueSwap : public ::testing::Test {
  protected:
    bmqu::SynchronizedValue<bsl::string> d_obj1;
    bmqu::SynchronizedValue<bsl::string> d_obj2;

    SynchronizedValueSwap()
    : d_obj1("test1")
    , d_obj2("test2")
    {
    }

    ~SynchronizedValueSwap() BSLS_KEYWORD_OVERRIDE;

    template <typename t_LOCKEDPTR>
    static void testSwap(t_LOCKEDPTR& lockPtr1, t_LOCKEDPTR& lockPtr2)
    {
        bsl::string expected1 = *lockPtr1;
        bsl::string expected2 = *lockPtr2;

        bsl::swap(lockPtr1, lockPtr2);
        EXPECT_EQ(expected1, *lockPtr2);
        EXPECT_EQ(expected2, *lockPtr1);
    }
};

SynchronizedValueSwap::~SynchronizedValueSwap()
{
}

TEST_F(SynchronizedValueSwap, constReadLockedPtr)
{
    bmqu::SynchronizedValue<bsl::string>::ConstReadLockedPtr lockPtr1(&d_obj1);
    bmqu::SynchronizedValue<bsl::string>::ConstReadLockedPtr lockPtr2(&d_obj2);
    testSwap(lockPtr1, lockPtr2);
}

TEST_F(SynchronizedValueSwap, readLockedPtr)
{
    bmqu::SynchronizedValue<bsl::string>::ReadLockedPtr lockPtr1(&d_obj1);
    bmqu::SynchronizedValue<bsl::string>::ReadLockedPtr lockPtr2(&d_obj2);
    testSwap(lockPtr1, lockPtr2);
}

TEST_F(SynchronizedValueSwap, constWriteLockedPtr)
{
    bmqu::SynchronizedValue<bsl::string>::ConstWriteLockedPtr lockPtr1(
        &d_obj1);
    bmqu::SynchronizedValue<bsl::string>::ConstWriteLockedPtr lockPtr2(
        &d_obj2);
    testSwap(lockPtr1, lockPtr2);
}

TEST_F(SynchronizedValueSwap, writeLockedPtr)
{
    bmqu::SynchronizedValue<bsl::string>::WriteLockedPtr lockPtr1(&d_obj1);
    bmqu::SynchronizedValue<bsl::string>::WriteLockedPtr lockPtr2(&d_obj2);
    testSwap(lockPtr1, lockPtr2);
}

class SynchronizedValueUniqueSwap : public ::testing::Test {
  protected:
    bmqu::SynchronizedValue<bsl::string, bslmt::Mutex> d_obj1;
    bmqu::SynchronizedValue<bsl::string, bslmt::Mutex> d_obj2;

    SynchronizedValueUniqueSwap()
    : d_obj1("test1")
    , d_obj2("test2")
    {
    }

    ~SynchronizedValueUniqueSwap() BSLS_KEYWORD_OVERRIDE;

    template <typename t_LOCKEDPTR>
    static void testSwap(t_LOCKEDPTR& lockPtr1, t_LOCKEDPTR& lockPtr2)
    {
        bsl::string expected1 = *lockPtr1;
        bsl::string expected2 = *lockPtr2;

        bsl::swap(lockPtr1, lockPtr2);
        EXPECT_EQ(expected1, *lockPtr2);
        EXPECT_EQ(expected2, *lockPtr1);
    }
};

SynchronizedValueUniqueSwap::~SynchronizedValueUniqueSwap()
{
}

TEST_F(SynchronizedValueUniqueSwap, constLockedPtr)
{
    bmqu::SynchronizedValue<bsl::string, bslmt::Mutex>::ConstLockedPtr
        lockPtr1(&d_obj1);
    bmqu::SynchronizedValue<bsl::string, bslmt::Mutex>::ConstLockedPtr
        lockPtr2(&d_obj2);
    testSwap(lockPtr1, lockPtr2);
}

TEST_F(SynchronizedValueUniqueSwap, lockedPtr)
{
    bmqu::SynchronizedValue<bsl::string, bslmt::Mutex>::LockedPtr lockPtr1(
        &d_obj1);
    bmqu::SynchronizedValue<bsl::string, bslmt::Mutex>::LockedPtr lockPtr2(
        &d_obj2);
    testSwap(lockPtr1, lockPtr2);
}

TEST(SynchronizedValue, readerWriterMutexCanShareLocks)
{
    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

    enum { k_NUM_WORKERS = 10 };
    bslmt::Barrier                                 barrier(k_NUM_WORKERS);
    bsl::shared_ptr<bmqu::SynchronizedValue<int> > value =
        bsl::allocate_shared<bmqu::SynchronizedValue<int> >(alloc, 0);
    bsl::vector<bslmt::ThreadUtil::Handle> threads;
    bsl::vector<SharedValueWorker>         workers;

    for (size_t i = 0; i < k_NUM_WORKERS; ++i) {
        bslmt::ThreadUtil::Handle& handle = threads.emplace_back();
        SharedValueWorker& worker = workers.emplace_back(value, &barrier);
        bslmt::ThreadUtil::createWithAllocator(&handle, worker, alloc);
    }

    for (bsl::vector<bslmt::ThreadUtil::Handle>::iterator it = threads.begin(),
                                                          end = threads.end();
         it != end;
         ++it) {
        int rc = bslmt::ThreadUtil::join(*it);
        EXPECT_EQ(0, rc);
    }
}

TEST(SynchronizedValue, writerLocksAreSynchronized)
{
    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

    enum { k_NUM_WORKERS = 10 };
    bsl::shared_ptr<bmqu::SynchronizedValue<int> > value =
        bsl::allocate_shared<bmqu::SynchronizedValue<int> >(alloc, 0);
    bsl::vector<bslmt::ThreadUtil::Handle> threads(alloc);
    bsl::vector<SynchronizedWritesWorker>  workers(alloc);

    for (size_t i = 0; i < k_NUM_WORKERS; ++i) {
        bslmt::ThreadUtil::Handle& handle = threads.emplace_back();
        SynchronizedWritesWorker&  worker = workers.emplace_back(value);
        bslmt::ThreadUtil::createWithAllocator(&handle, worker, alloc);
    }

    for (bsl::vector<bslmt::ThreadUtil::Handle>::iterator it = threads.begin(),
                                                          end = threads.end();
         it != end;
         ++it) {
        int rc = bslmt::ThreadUtil::join(*it);
        EXPECT_EQ(0, rc);
    }

    bmqu::SynchronizedValue<int>::ConstReadLockedPtr lockedPtr(value.get());
    EXPECT_EQ(k_NUM_WORKERS, *lockedPtr);
}

// ========================================================================
//                                  MAIN
// ------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    ::testing::InitGoogleTest(&argc, argv);

    bmqtst::TestHelperUtil::testStatus() = RUN_ALL_TESTS();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
