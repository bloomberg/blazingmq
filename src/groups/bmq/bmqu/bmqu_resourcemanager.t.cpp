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

// BDE
#include <bdlf_bind.h>
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

struct Tester {
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
};

struct ResourceA {
    bsls::Types::Uint64 d_index;

    ResourceA(bsls::Types::Uint64 index):
    d_index(index) {
        Tester::logEvent("ResourceA" + bsl::to_string(d_index));
    }

    ~ResourceA() {
        Tester::logEvent("~ResourceA" + bsl::to_string(d_index));
    }
};

static bsl::shared_ptr<ResourceA> createResourceA(bslma::Allocator *allocator) {
    bsls::Types::Uint64 index = bslmt::ThreadUtil::selfIdAsUint64();

    bsl::shared_ptr<ResourceA> res;
    res.createInplace(allocator, index);
    return res;
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bmqu::ResourceManager::init(s_allocator_p);

    bmqu::ResourceManager::registerResourceFactory<ResourceA>(createResourceA);

    {
        bsl::shared_ptr<ResourceA> res = bmqu::ResourceManager::getResource<ResourceA>();
    }

    bmqu::ResourceManager::deinit();

    bsl::vector<bsl::string> events = Tester::getEvents();
    for (const bsl::string &ev : events) {
        bsl::cout << ev << bsl::endl;
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
