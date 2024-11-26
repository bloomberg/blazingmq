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

// bmqu_sharedresource.t.cpp                                          -*-C++-*-
#include <bmqu_sharedresource.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

#include <bmqex_sequentialcontext.h>
#include <bmqu_weakmemfn.h>

// BDE
#include <bdlf_bind.h>
#include <bdlf_memfn.h>
#include <bdlmt_threadpool.h>
#include <bslma_testallocator.h>
#include <bslmt_threadutil.h>
#include <bsls_assert.h>
#include <bsls_timeinterval.h>

// CONVENIENCE
using namespace BloombergLP;

// ============================================================================
//                            USAGE EXAMPLE
// ----------------------------------------------------------------------------

// ===============
// class MyService
// ===============

/// Provides a usage example.
class MyService {
  private:
    // PRIVATE DATA

    // Used to spawn async operations.
    bdlmt::ThreadPool* d_threadPool_p;

    // Used to synchronize with async operations completion on
    // destruction.
    bmqu::SharedResource<MyService> d_self;

  private:
    // PRIVATE MANIPULATORS
    void doStuff();

  public:
    // CREATORS

    /// Create a `MyService` object. Specify a `threadPool` used to
    /// spawn async operations.
    explicit MyService(bdlmt::ThreadPool* threadPool);

    /// Destroy this object. Block the calling thread until all async
    /// operations are completed.
    ~MyService();

  public:
    // MANIPULATORS

    /// Initiate an async operation that does something.
    void asyncDoStuff();
};

// ---------------
// class MyService
// ---------------

// CREATORS
MyService::MyService(bdlmt::ThreadPool* threadPool)
: d_threadPool_p(threadPool)
, d_self(this)
{
    // NOTHING
}

MyService::~MyService()
{
    d_self.invalidate();
}

// MANIPULATORS
void MyService::doStuff()
{
    // NOTHING
}

void MyService::asyncDoStuff()
{
    // acquire the shared resource and bind to the async operation
    d_threadPool_p->enqueueJob(
        bmqu::WeakMemFnUtil::weakMemFn(&MyService::doStuff,
                                       d_self.acquireWeak()));
}

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

struct Sleep {
    void sleep(const bsls::TimeInterval& duration) const
    {
        bslmt::ThreadUtil::sleep(duration);
    }
};

}  // close anonymous namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_resource_creators()
// ------------------------------------------------------------------------
// RESOURCE CREATORS
//
// Concerns:
//   Ensure proper behavior of the creator methods.
//
// Plan:
//   1. Create a 'bmqu::SharedResource' managing no resource by default-
//      constructing a 'bmqu::SharedResource'. Check that no memory was
//      allocated and that the resource is invalid.
//
//   2. Create a 'bmqu::SharedResource' managing no resource by nullptr-
//      constructing a 'bmqu::SharedResource'. Check that no memory was
//      allocated and that the resource is invalid.
//
//   3. Create a 'bmqu::SharedResource' managing a resource. Check that
//      memory was allocated and that the resource is valid and can be
//      acquired. Then destroy the object and check that all allocated
//      memory was released.
//
//   4. Try create a 'bmqu::SharedResource' object specifying a resource, a
//      deleter and an allocator that will throw on use. Check that the
//      deleter is used to free the resource.
//
// Testing:
//   bmqu::SharedResource's default constructor
//   bmqu::SharedResource's nullptr constructor
//   bmqu::SharedResource's resource constructor
//   bmqu::SharedResource's destructor
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. empty resource (default-constructed)
    {
        // no memory allocated yet
        ASSERT(alloc.numBytesInUse() == 0);

        {
            // create shared resource
            bmqu::SharedResource<int> sharedResource(&alloc);

            // no memory allocated still
            ASSERT(alloc.numBytesInUse() == 0);

            // the resource is invalid
            ASSERT(!sharedResource.isValid());
        }
    }

    // 2. empty resource (nullptr-constructed)
    {
        // no memory allocated yet
        ASSERT(alloc.numBytesInUse() == 0);

        {
            // create shared resource
            bmqu::SharedResource<int> sharedResource(bsl::nullptr_t(), &alloc);

            // no memory allocated still
            ASSERT(alloc.numBytesInUse() == 0);

            // the resource is invalid
            ASSERT(!sharedResource.isValid());
        }
    }

    // 3. non-empty resource
    {
        // no memory allocated yet
        ASSERT(alloc.numBytesInUse() == 0);

        {
            int resource = 42;

            // create shared resource
            bmqu::SharedResource<int> sharedResource(&resource, &alloc);

            // memory allocated
            ASSERT(alloc.numBytesInUse() != 0);

            // the resource is valid
            ASSERT(sharedResource.isValid() &&
                   *sharedResource.acquire() == 42);

            // destroy the shared resource
        }

        // memory freed
        ASSERT(alloc.numBytesInUse() == 0);
    }

    // 4. exception safety
    // Current BDE version 4.8 has a memory leak which is already fixed
    // (https://github.com/bloomberg/bde/commit/42950dfbcaf2c76cdabc266afacecee67da21a59)
    // but not released yet. Skip address sanitizer check for BDE version less
    // than 4.9.
#if BSL_VERSION < BSL_MAKE_VERSION(4, 9)
#if defined(__has_feature)  // Clang-supported method for checking sanitizers.
    static const bool skipTest = __has_feature(address_sanitizer);
#elif defined(__SANITIZE_ADDRESS__)
    // GCC-supported macros for checking ASAN.
    static const bool skipTest = true;
#else
    static const bool skipTest = false;  // Default to running the test.
#endif
#else
    static const bool skipTest = false;  // Default to running the test.
#endif

    if (!skipTest) {
        typedef bmqu::SharedResourceFactoryDeleter<int, bslma::Allocator>
            Deleter;

        // this allocator is used to allocate the shared state and will throw
        bslma::TestAllocator badAlloc;
        badAlloc.setAllocationLimit(0);

        // this allocator represents a resource factory
        bslma::TestAllocator resourceFactory;

        // allocate a resource
        int* resource = new (resourceFactory) int(42);
        ASSERT(resourceFactory.numBytesInUse() != 0);

        // try to create a 'bmqu::SharedResource', which should fail
        bool exceptionThrown = false;
        try {
            bmqu::SharedResource<int, Deleter> sharedResource(
                resource,
                Deleter(&resourceFactory),
                &badAlloc);
        }
        catch (...) {
            exceptionThrown = true;
        }

        // creation failed
        ASSERT(exceptionThrown);

        // resource freed using its factory
        ASSERT(resourceFactory.numBytesInUse() == 0);
    }
}

static void test2_resource_acquire()
// ------------------------------------------------------------------------
// RESOURCE ACQUIRE
//
// Concerns:
//   Ensure proper behavior of the 'acquire' and 'acquireWeak' methods.
//
// Plan:
//   1. Acquire the resource before invalidation. Check that 'acquire' and
//      'acquireWeak' return valid pointers.
//
//   2. Acquire the resource during invalidation. Check that 'acquire' and
//      'acquireWeak' return valid pointers.
//
//   3. Acquire the resource after invalidation. Check that 'acquire' and
//      'acquireWeak' return invalid pointers.
//
// Testing:
//   bmqu::SharedResource::acquire
//   bmqu::SharedResource::acquireWeak
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;
    int                  resource = 42;

    // create shared resource
    bmqu::SharedResource<int> sharedResource(&resource, &alloc);

    // 1. acquire before invalidating
    {
        // the resource can be acquired
        ASSERT(sharedResource.acquire() && *sharedResource.acquire() == 42);

        ASSERT(sharedResource.acquireWeak().lock() &&
               *sharedResource.acquireWeak().lock() == 42);
    }

    // 2. acquire while invalidating
    {
        bmqex::SequentialContext context(&alloc);
        int                      rc = context.start();
        BSLS_ASSERT_OPT(rc == 0);

        // hold a shared pointer while we invalidate the resource
        bsl::shared_ptr<int> resourceSP = sharedResource.acquire();

        // invalidate the resource from another thread
        context.executor().post(
            bdlf::MemFnUtil::memFn(&bmqu::SharedResource<int>::invalidate,
                                   &sharedResource));

        // wait till 'invalidate' starts executing
        bslmt::ThreadUtil::sleep(bsls::TimeInterval(0.2));

        // the resource can still be acquired
        ASSERT(sharedResource.acquire() && *sharedResource.acquire() == 42);

        ASSERT(sharedResource.acquireWeak().lock() &&
               *sharedResource.acquireWeak().lock() == 42);

        // release the resource
        resourceSP.reset();

        // join the thread
        context.join();
    }

    // 3. acquire after invalidating
    {
        // NOTE: The resource has been invalidated at this point.

        // the resource can not be acquired
        ASSERT(!sharedResource.acquire());
        ASSERT(!sharedResource.acquireWeak().lock());
    }
}

static void test3_resource_invalidate()
// ------------------------------------------------------------------------
// RESOURCE INVALIDATE
//
// Concerns:
//   Ensure proper behavior of the 'invalidate' method.
//
// Plan:
//   1. Invalidate a resource that is not in use.
//
//   2. Invalidate a resource that is in use. Check that 'invalidate'
//      blocks the calling thread until the resource is out of use, and
//      that the resource is freed using the supplied deleter.
//
// Testing:
//   bmqu::SharedResource::invalidate
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. invalidate unused resource
    {
        int resource = 42;

        // create shared resource
        bmqu::SharedResource<int> sharedResource(&resource, &alloc);

        // invalidate it
        sharedResource.invalidate();

        // the resource was invalidated
        ASSERT(!sharedResource.isValid());
    }

    // 2. invalidate used resource
    {
        typedef bmqu::SharedResourceFactoryDeleter<Sleep, bslma::Allocator>
            Deleter;

        bmqex::SequentialContext context(&alloc);
        int                      rc = context.start();
        BSLS_ASSERT_OPT(rc == 0);

        // allocate the resource
        bslma::TestAllocator resourceFactory;
        Sleep*               sleep = new (resourceFactory) Sleep();
        ASSERT(resourceFactory.numBytesInUse() != 0);

        // create shared resource
        bmqu::SharedResource<Sleep, Deleter> sharedResource(
            sleep,
            Deleter(&resourceFactory),
            &alloc);

        // acquire a weak pointer
        bsl::weak_ptr<Sleep> resourceWP = sharedResource.acquireWeak();

        // acquire a shared pointer and use it in another thread
        context.executor().post(
            bdlf::BindUtil::bindR<void>(&Sleep::sleep,
                                        sharedResource.acquire(),
                                        bsls::TimeInterval(0.2)));

        // invalidate the resource, shall block until the shared pointer is
        // alive
        sharedResource.invalidate();

        // the shared pointer is dead, therefore 'invalidate' did block
        BSLS_ASSERT(resourceWP.expired());

        // the resource was invalidated
        ASSERT(!sharedResource.isValid());

        // the resource was freed using the supplied deleter
        ASSERT(resourceFactory.numBytesInUse() == 0);
    }
}

static void test4_resource_reset()
// ------------------------------------------------------------------------
// RESOURCE RESET
//
// Concerns:
//   Ensure proper behavior of the 'reset' method.
//
// Plan:
//   1. Reset an empty resource to an empty state. Check that this
//      operation has no effect.
//
//   2. Reset a non-empty resource to an empty state, Check that the
//      resource becomes invalid and all the memory is freed.
//
//   3. Reset an empty resource to a non-empty state. Check that the
//      resource now manages the new target.
//
//   4. Reset a non-empty resource to a non-empty state. Check that the
//      resource now manages the new target.
//
//   5. Create a 'bmqu::SharedResource' object specifying a resource, a
//      deleter and an allocator that will throw on second use. Then try
//      reset the resource also supplying a second deleter, which should
//      fail. Check that the first resource was freed using the first
//      deleter, the second resource was feed using the second deleter, and
//      the 'bmqu::SharedResource' object is left in managing no resource.
//
// Testing:
//   bmqu::SharedResource::reset
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. reset empty to empty
    {
        // create shared resource
        bmqu::SharedResource<int> sharedResource(&alloc);

        // reset it
        sharedResource.reset();

        // resource is empty
        BSLS_ASSERT(!sharedResource.isValid());
    }

    // 2. reset non-empty to empty
    {
        int resource1 = 100;

        // create shared resource
        bmqu::SharedResource<int> sharedResource(&resource1, &alloc);

        // reset it
        sharedResource.reset();

        // memory freed
        ASSERT(alloc.numBytesInUse() == 0);

        // resource is empty
        BSLS_ASSERT(!sharedResource.isValid());
    }

    // 3. reset empty to non-empty
    {
        int resource1 = 100;

        // create shared resource
        bmqu::SharedResource<int> sharedResource(&alloc);

        // reset it
        sharedResource.reset(&resource1);

        // resource is valid
        BSLS_ASSERT(sharedResource.isValid() &&
                    *sharedResource.acquire() == 100);
    }

    // 4. reset non-empty to non-empty
    {
        int resource1 = 100;
        int resource2 = 200;

        // create shared resource
        bmqu::SharedResource<int> sharedResource(&resource1, &alloc);

        // reset it
        sharedResource.reset(&resource2);

        // resource is valid
        BSLS_ASSERT(sharedResource.isValid() &&
                    *sharedResource.acquire() == 200);
    }

    // 5. exception safety
    // Current BDE version 4.8 has a memory leak which is already fixed
    // (https://github.com/bloomberg/bde/commit/42950dfbcaf2c76cdabc266afacecee67da21a59)
    // but not released yet. Skip address sanitizer check for BDE version less
    // than 4.9.
#if BSL_VERSION < BSL_MAKE_VERSION(4, 9)
#if defined(__has_feature)  // Clang-supported method for checking sanitizers.
    static const bool skipTest = __has_feature(address_sanitizer);
#elif defined(__SANITIZE_ADDRESS__)
    // GCC-supported macros for checking ASAN.
    static const bool skipTest = true;
#else
    static const bool skipTest = false;  // Default to running the test.
#endif
#else
    static const bool skipTest = false;  // Default to running the test.
#endif

    if (!skipTest) {
        typedef bmqu::SharedResourceFactoryDeleter<int, bslma::Allocator>
            Deleter;

        // this allocator is used to allocate the shared state and will
        // throw the second time it is used
        bslma::TestAllocator onceAlloc;
        onceAlloc.setAllocationLimit(1);

        // this allocators represents resource factories
        bslma::TestAllocator resourceFactory1;
        bslma::TestAllocator resourceFactory2;

        // allocate resources
        int* resource1 = new (resourceFactory1) int(42);
        int* resource2 = new (resourceFactory2) int(42);
        ASSERT(resourceFactory1.numBytesInUse() != 0);
        ASSERT(resourceFactory2.numBytesInUse() != 0);

        // create a 'bmqu::SharedResource'
        bmqu::SharedResource<int, Deleter> sharedResource(
            resource1,
            Deleter(&resourceFactory1),
            &onceAlloc);

        // try to reset the resource, which should fail
        bool exceptionThrown = false;
        try {
            sharedResource.reset(resource2, Deleter(&resourceFactory2));
        }
        catch (...) {
            exceptionThrown = true;
        }

        // creation failed
        ASSERT(exceptionThrown);

        // both resources freed using their respective factories
        ASSERT(resourceFactory1.numBytesInUse() == 0);
        ASSERT(resourceFactory2.numBytesInUse() == 0);

        // the 'bmqu::SharedResource' is left invalid
        ASSERT(!sharedResource.isValid());
    }
}

static void test5_factoryDeleter()
// ------------------------------------------------------------------------
// FACTORY DELETER
//
// Concerns:
//   Ensure proper behavior of the 'bmqu::SharedResourceFactoryDeleter'
//   class.
//
// Plan:
//   Create a factory wrapping an 'bslma::Allocator'. Use it to free a
//   resource previously created using that allocator.
//
// Testing:
//   bmqu::SharedResourceFactoryDeleter
// ------------------------------------------------------------------------
{
    // create factory and allocate resource
    bslma::TestAllocator factory;
    int*                 resource = new (factory) int(42);

    // memory allocated
    ASSERT(factory.numBytesInUse() != 0);

    // create deleter and free resource
    bmqu::SharedResourceFactoryDeleter<int, bslma::Allocator> deleter(
        &factory);
    deleter(resource);

    // memory freed
    ASSERT(factory.numBytesInUse() == 0);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    // bmqu::SharedResource
    case 1: test1_resource_creators(); break;
    case 2: test2_resource_acquire(); break;
    case 3: test3_resource_invalidate(); break;
    case 4: test4_resource_reset(); break;

    // bmqu::SharedResourceFactoryDeleter
    case 5: test5_factoryDeleter(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
