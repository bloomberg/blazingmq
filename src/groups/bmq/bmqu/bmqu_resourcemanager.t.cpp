// Copyright 2024 Bloomberg Finance L.P.
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

// bmqu_resourcemanager.t.cpp                                         -*-C++-*-
#include <bmqu_resourcemanager.h>

// BMQ
#include <bmqu_memoutstream.h>

// BDE
#include <bdlf_bind.h>
#include <bsl_stdexcept.h>
#include <bsl_vector.h>
#include <bslmt_barrier.h>
#include <bslmt_threadgroup.h>
#include <bslmt_threadutil.h>
#include <bsls_assert.h>
#include <bsls_platform.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

static bool startsWith(bslstl::StringRef str, bslstl::StringRef substr)
{
    return str.find(substr, 0) == 0;
}

struct Tester {
    static void compare(const bsl::vector<bsl::string>& expected,
                        const bsl::vector<bsl::string>& observed)
    {
        if (expected.size() != observed.size()) {
            bmqu::MemOutStream reason(s_allocator_p);
            reason << "expected and observed event vectors have different"
                   << " sizes: " << expected.size()
                   << " != " << observed.size();
            fail(reason.str(), expected, observed);
        }
        for (size_t i = 0; i < observed.size(); i++) {
            if (!startsWith(observed.at(i), expected.at(i))) {
                bmqu::MemOutStream reason(s_allocator_p);
                reason << "observed event [" << i << ", '" << observed.at(i)
                       << "'] doesn't start "
                       << "with prefix '" << expected.at(i) << "'";
                fail(reason.str(), expected, observed);
            }
        }
    }

    static void fail(const bsl::string&              reason,
                     const bsl::vector<bsl::string>& expected,
                     const bsl::vector<bsl::string>& observed)
    {
        bmqu::MemOutStream errorMessage(s_allocator_p);
        errorMessage << "Failed events check: " << reason << bsl::endl
                     << "\texpected: [";
        for (size_t i = 0; i < expected.size(); i++) {
            if (i > 0) {
                errorMessage << ", ";
            }
            errorMessage << "'" << expected.at(i) << "'";
        }
        errorMessage << "]" << bsl::endl;
        errorMessage << "\tobserved: [";
        for (size_t i = 0; i < observed.size(); i++) {
            if (i > 0) {
                errorMessage << ", ";
            }
            errorMessage << "'" << observed.at(i) << "'";
        }
        errorMessage << "]" << bsl::endl;

        throw bsl::runtime_error(errorMessage.str());
        ASSERT_D(errorMessage.str(), false);
    }

    bslmt::Mutex d_mutex;

    bsl::vector<bsl::string> d_events;

    Tester()
    : d_mutex()
    , d_events(s_allocator_p)
    {
        // NOTHING
    }

    static Tester *inst() {
        static Tester inst;
        return &inst;
    }

    static void logEvent(const bsl::string &event) {
        Tester* tester = inst();

        bslmt::LockGuard<bslmt::Mutex> guard(&tester->d_mutex);
        tester->d_events.push_back(event);
    }

    static bsl::vector<bsl::string> getEvents() {
        Tester* tester = inst();

        bslmt::LockGuard<bslmt::Mutex> guard(&tester->d_mutex);
        return bsl::vector<bsl::string>(tester->d_events, s_allocator_p);
    }

    static void expectEmpty()
    {
        bsl::vector<bsl::string> observedEvents = getEvents();
        bsl::vector<bsl::string> expectedEvents(s_allocator_p);
        compare(expectedEvents, observedEvents);
    }

    static void expect(bslstl::StringRef expected1)
    {
        bsl::vector<bsl::string> observedEvents = getEvents();
        bsl::vector<bsl::string> expectedEvents(s_allocator_p);
        expectedEvents.push_back(expected1);
        compare(expectedEvents, observedEvents);
    }

    static void expect(bslstl::StringRef expected1,
                       bslstl::StringRef expected2)
    {
        bsl::vector<bsl::string> observedEvents = getEvents();
        bsl::vector<bsl::string> expectedEvents(s_allocator_p);
        expectedEvents.push_back(expected1);
        expectedEvents.push_back(expected2);
        compare(expectedEvents, observedEvents);
    }

    static void clear()
    {
        Tester* tester = inst();

        bslmt::LockGuard<bslmt::Mutex> guard(&tester->d_mutex);
        tester->d_events.clear();
        // allocator check happiness:
        tester->d_events.shrink_to_fit();
    }
};

struct ResourceA {
    bsls::Types::Uint64 d_index;

    ResourceA(bsls::Types::Uint64 index):
    d_index(index) {
        Tester::logEvent("ResourceA: " + bsl::to_string(d_index));
    }

    ~ResourceA() {
        Tester::logEvent("~ResourceA: " + bsl::to_string(d_index));
    }
};

static bsl::shared_ptr<ResourceA> createResourceA(bslma::Allocator *allocator) {
    bsls::Types::Uint64 index = bslmt::ThreadUtil::selfIdAsUint64();

    bsl::shared_ptr<ResourceA> res;
    res.createInplace(allocator, index);
    return res;
}

struct ResourceB {
    ResourceA& d_dependency;

    ResourceB(ResourceA& dependency)
    : d_dependency(dependency)
    {
        Tester::logEvent("ResourceB: " + bsl::to_string(d_dependency.d_index));
    }

    ~ResourceB()
    {
        Tester::logEvent("~ResourceB: " +
                         bsl::to_string(d_dependency.d_index));
    }
};

static bsl::shared_ptr<ResourceB> createResourceB(bslma::Allocator* allocator)
{
    bsl::shared_ptr<ResourceA> dependency =
        bmqu::ResourceManager::getResource<ResourceA>();
    ASSERT(dependency);

    bsl::shared_ptr<ResourceB> res;
    res.createInplace(allocator, *dependency);
    return res;
}

void threadFunction_test3(bslmt::Barrier*             barrier,
                          bsl::shared_ptr<ResourceA>* res)
{
    barrier->wait();

    static const size_t k_ITERS = 1000;

    for (size_t i = 0; i < k_ITERS; i++) {
        bsl::shared_ptr<ResourceA> sp =
            bmqu::ResourceManager::getResource<ResourceA>();
        if (i > 0) {
            // Getting the same resource every time
            ASSERT_EQ(sp.get(), res->get());
        }
        *res = sp;
    }
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_oneResourceOneThread()
{
    bmqtst::TestHelper::printTestName("ONE RESOURCE ONE THREAD");

    bmqu::ResourceManager::init(s_allocator_p);
    // expect no resource creation events yet
    Tester::expectEmpty();

    bmqu::ResourceManager::registerResourceFactory<ResourceA>(createResourceA);
    // expect no resource creation events yet
    Tester::expectEmpty();

    bsl::shared_ptr<ResourceA> res1 =
        bmqu::ResourceManager::getResource<ResourceA>();
    // expect only one resource creation event
    Tester::expect("ResourceA");

    bsl::shared_ptr<ResourceA> res2 =
        bmqu::ResourceManager::getResource<ResourceA>();
    // expect the same resource
    ASSERT_EQ(res1.get(), res2.get());
    // expect no events other than the initial resource creation
    Tester::expect("ResourceA");

    res1.reset();
    res2.reset();
    // expect no events other than the initial resource creation, and
    // no destructor event yet despite all pointers gathered before reset
    Tester::expect("ResourceA");

    bsl::shared_ptr<ResourceA> res3 =
        bmqu::ResourceManager::getResource<ResourceA>();
    // expect no events other than the initial resource creation, and
    // no extra creation when we re-acquired the resource
    Tester::expect("ResourceA");

    res3.reset();
    bmqu::ResourceManager::deinit();
    // expect a new destructor event after we released all references to
    // the resource and deinitialized the resource manager
    Tester::expect("ResourceA", "~ResourceA");

    // clean the events in Tester for simplicity
    Tester::clear();
    // should be able to reinitialize the resource manager
    bmqu::ResourceManager::init(s_allocator_p);
    // expect no resource creation events yet
    Tester::expectEmpty();

    bmqu::ResourceManager::registerResourceFactory<ResourceA>(createResourceA);
    // expect no resource creation events yet
    Tester::expectEmpty();

    bsl::shared_ptr<ResourceA> res4 =
        bmqu::ResourceManager::getResource<ResourceA>();
    // expect only one resource creation event
    Tester::expect("ResourceA");

    res4.clear();
    bmqu::ResourceManager::deinit();
    // expect a new destructor event after we released all references to
    // the resource and deinitialized the resource manager
    Tester::expect("ResourceA", "~ResourceA");

    Tester::clear();
}

static void test2_manyResourcesOneThread()
{
    bmqtst::TestHelper::printTestName("MANY RESOURCES ONE THREAD");

    bmqu::ResourceManager::init(s_allocator_p);

    bmqu::ResourceManager::registerResourceFactory<ResourceA>(createResourceA);
    bmqu::ResourceManager::registerResourceFactory<ResourceB>(createResourceB);
    // expect no resource creation events yet
    Tester::expectEmpty();

    bsl::shared_ptr<ResourceB> res1 =
        bmqu::ResourceManager::getResource<ResourceB>();
    // expect ResourceA being built before ResourceB, since we called ResourceA
    // from ResourceB factory just before building ResourceB
    Tester::expect("ResourceA", "ResourceB");

    // clean the events in Tester for simplicity
    Tester::clear();

    res1.clear();
    bmqu::ResourceManager::deinit();
    // expect destructors being called in the opposite order of how we register
    // resource factories, so ResourceB will be released before ResourceA
    Tester::expect("~ResourceB", "~ResourceA");

    Tester::clear();
}

static void test3_oneResourceManyThreads()
{
    bmqtst::TestHelper::printTestName("ONE RESOURCE MANY THREADS");

    static const size_t k_NUM_THREADS = 16;

    bmqu::ResourceManager::init(s_allocator_p);

    bmqu::ResourceManager::registerResourceFactory<ResourceA>(createResourceA);
    // expect no resource creation events yet
    Tester::expectEmpty();

    // Barrier to get each thread to start at the same time; `+1` for this
    // (main) thread.
    bslmt::Barrier barrier(k_NUM_THREADS + 1);

    bsl::vector<bsl::shared_ptr<ResourceA> > resources(k_NUM_THREADS,
                                                       s_allocator_p);

    {
        bslmt::ThreadGroup threadGroup(s_allocator_p);
        for (size_t i = 0; i < k_NUM_THREADS; ++i) {
            int rc = threadGroup.addThread(
                bdlf::BindUtil::bind(&threadFunction_test3,
                                     &barrier,
                                     &resources.at(i)));
            ASSERT_EQ_D(i, rc, 0);
        }
        barrier.wait();
        threadGroup.joinAll();
    }
    // threads are joined and threadGroup is gone

    for (size_t i = 0; i < resources.size(); i++) {
        for (size_t j = i + 1; j < resources.size(); j++) {
            // Each thread acquired resource exclusively built for it
            ASSERT_NE(resources.at(i).get(), resources.at(j).get());
        }
    }

    {
        bsl::vector<bsl::string> events = Tester::getEvents();
        ASSERT_EQ(events.size(), k_NUM_THREADS);
        for (size_t i = 0; i < events.size(); i++) {
            startsWith(events.at(i), "ResourceA");
        }
    }

    // clean the events in Tester for simplicity
    Tester::clear();

    resources.clear();
    // expect no more events
    Tester::expectEmpty();

    bmqu::ResourceManager::deinit();

    // expect destructors for all resources allocated per each thread
    {
        bsl::vector<bsl::string> events = Tester::getEvents();
        ASSERT_EQ(events.size(), k_NUM_THREADS);
        for (size_t i = 0; i < events.size(); i++) {
            startsWith(events.at(i), "~ResourceA");
        }
    }

    Tester::clear();
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_oneResourceManyThreads(); break;
    case 2: test2_manyResourcesOneThread(); break;
    case 1: test1_oneResourceOneThread(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
