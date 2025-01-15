// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbu_loadbalancer.t.cpp                                            -*-C++-*-
#include <mqbu_loadbalancer.h>

#include <bsl_limits.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// A dummy type to use as the template parameter
struct MyDummyType {};

/// Validates that the specified `obj` is balanced, that is the difference
/// between the maximum and the minimum clients associated to processors is
/// at most 1.
static void ensureIsBalanced(const mqbu::LoadBalancer<MyDummyType>& obj)
{
    int minClients = bsl::numeric_limits<int>::max();
    int maxClients = bsl::numeric_limits<int>::min();

    for (int i = 0; i < obj.processorsCount(); ++i) {
        int count = obj.clientsCountForProcessor(i);
        PVVV("    Processor " << i << " has " << count << " clients");
        minClients = bsl::min(minClients, count);
        maxClients = bsl::max(maxClients, count);
    }
    PVV("    MinClients: " << minClients << ", maxClients: " << maxClients);
    BMQTST_ASSERT_LE(maxClients - minClients, 1);
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    const int                       k_NUM_PROCESSORS = 3;
    const int                       k_NUM_CLIENTS    = 13;
    mqbu::LoadBalancer<MyDummyType> obj(k_NUM_PROCESSORS,
                                        bmqtst::TestHelperUtil::allocator());

    // Ensure that 'numProcessors()' accessor returns the number of processors
    // set at construction; and that there are no clients.
    BMQTST_ASSERT_EQ(obj.processorsCount(), k_NUM_PROCESSORS);
    BMQTST_ASSERT_EQ(obj.clientsCount(), 0);

    // Make sure each processor has no clients.
    PV(":: Verifying " << k_NUM_PROCESSORS << " processors have no clients");
    for (int i = 0; i < k_NUM_PROCESSORS; ++i) {
        PVVV("      Checking processor " << i);
        BMQTST_ASSERT_EQ(obj.clientsCountForProcessor(i), 0);
    }

    // Register some clients, and verify the associated 'processorId' is within
    // the '[0..processors - 1]' range
    PV(":: Registering " << k_NUM_CLIENTS << " clients");
    for (int i = 0; i < k_NUM_CLIENTS; ++i) {
        PVVV("      Registering client " << i);
        int processorId = obj.getProcessorForClient(
            reinterpret_cast<MyDummyType*>(i));
        BMQTST_ASSERT_LT(processorId, k_NUM_PROCESSORS);
    }
    BMQTST_ASSERT_EQ(obj.clientsCount(), k_NUM_CLIENTS);

    // Make sure that each processor has at least one client associated and
    // that all clients have been registered.
    PV(":: Checking clients count for " << k_NUM_PROCESSORS << " processors");
    int sumClients = 0;
    for (int i = 0; i < k_NUM_PROCESSORS; ++i) {
        PVVV("      Checking processor " << i);
        int clientsCount = obj.clientsCountForProcessor(i);
        sumClients += clientsCount;
        BMQTST_ASSERT_LE(1, clientsCount);
    }
    BMQTST_ASSERT_EQ(sumClients, k_NUM_CLIENTS);

    // Remove all clients
    PV(":: Removing " << k_NUM_CLIENTS << " clients");
    for (int i = 0; i < k_NUM_CLIENTS; ++i) {
        obj.removeClient(reinterpret_cast<MyDummyType*>(i));
    }

    PV(":: Verifying " << k_NUM_PROCESSORS << " processors have no clients");
    BMQTST_ASSERT_EQ(obj.clientsCount(), 0);
    for (int i = 0; i < k_NUM_PROCESSORS; ++i) {
        PVVV("      Verifying processor " << i);
        BMQTST_ASSERT_EQ(0, obj.clientsCountForProcessor(i));
    }

    // Ensure removing of non existing client doesn't crash
    obj.removeClient(reinterpret_cast<MyDummyType*>(0));
}

static void test2_singleProcessorLoadBalancer()
{
    bmqtst::TestHelper::printTestName("SINGLE PROCESSOR LOAD BALANCER");

    const int                       k_NUM_PROCESSORS = 1;
    const int                       k_NUM_CLIENTS    = 10;
    mqbu::LoadBalancer<MyDummyType> obj(k_NUM_PROCESSORS,
                                        bmqtst::TestHelperUtil::allocator());

    // Ensure that 'numProcessors()' accessor returns the number of processors
    // set at construction; and that there are no clients.
    BMQTST_ASSERT_EQ(obj.processorsCount(), k_NUM_PROCESSORS);
    BMQTST_ASSERT_EQ(obj.clientsCount(), 0);

    // Verify that registerClient always returns the same value
    int processor = -1;
    PV(":: Registering " << k_NUM_CLIENTS << " clients");
    for (int i = 0; i < k_NUM_CLIENTS; ++i) {
        PVVV("      Registering client " << i);
        int processorId = obj.getProcessorForClient(
            reinterpret_cast<MyDummyType*>(i));
        if (i == 0) {
            processor = processorId;
        }
        BMQTST_ASSERT_EQ(processor, processorId);
    }

    BMQTST_ASSERT_EQ(obj.clientsCount(), k_NUM_CLIENTS);
}

static void test3_loadBalancing()
{
    bmqtst::TestHelper::printTestName("LOAD BALANCING");

    const int                       k_INITIAL_CLIENTS_COUNT = 137;
    mqbu::LoadBalancer<MyDummyType> obj(5,
                                        bmqtst::TestHelperUtil::allocator());

    PV(":: Registering " << k_INITIAL_CLIENTS_COUNT << " clients");
    for (int i = 0; i < k_INITIAL_CLIENTS_COUNT; ++i) {
        obj.getProcessorForClient(reinterpret_cast<MyDummyType*>(i));
    }
    BMQTST_ASSERT_EQ(obj.clientsCount(), k_INITIAL_CLIENTS_COUNT);

    PV(":: Verifying proper balancing between processors");
    ensureIsBalanced(obj);

    // Inserting the same clients, there should be no change in the clients
    // count
    PV(":: Insert the same " << k_INITIAL_CLIENTS_COUNT << " clients again");
    for (int i = 0; i < k_INITIAL_CLIENTS_COUNT; ++i) {
        obj.getProcessorForClient(reinterpret_cast<MyDummyType*>(i));
    }
    BMQTST_ASSERT_EQ(obj.clientsCount(), k_INITIAL_CLIENTS_COUNT);

    PV(":: Removing a few clients to create imbalanced load");
    const int toRemove[] = {0,
                            5,
                            10,
                            15,  // from processor 0
                            1,
                            21};  // from processor 1
    for (size_t i = 0; i < sizeof(toRemove) / sizeof(int); ++i) {
        obj.removeClient(reinterpret_cast<MyDummyType*>(i));
    }

    PV(":: Register new clients (more than were deleted)");
    for (size_t i = 0; i < 2 * sizeof(toRemove) / sizeof(int); ++i) {
        obj.getProcessorForClient(reinterpret_cast<MyDummyType*>(1000 + i));
    }

    PV(":: Verify processors are balanced again");
    ensureIsBalanced(obj);
}

static void test4_forceAssociate()
{
    bmqtst::TestHelper::printTestName("FORCE_ASSOCIATE");

    const int                       k_NUM_PROCESSORS = 3;
    mqbu::LoadBalancer<MyDummyType> obj(k_NUM_PROCESSORS,
                                        bmqtst::TestHelperUtil::allocator());

    PV(":: Assign client '0' to processor '0'");
    obj.setProcessorForClient(reinterpret_cast<MyDummyType*>(0), 0);
    BMQTST_ASSERT_EQ(obj.clientsCountForProcessor(0), 1);
    BMQTST_ASSERT_EQ(obj.clientsCountForProcessor(1), 0);
    BMQTST_ASSERT_EQ(obj.clientsCountForProcessor(2), 0);
    BMQTST_ASSERT_EQ(
        obj.getProcessorForClient(reinterpret_cast<MyDummyType*>(0)),
        0);

    PV(":: Assign client '1' to processor '0'");
    // Assign client '1' to same processor 0, to make sure load balancing
    // behavior is not overriding the request
    obj.setProcessorForClient(reinterpret_cast<MyDummyType*>(1), 0);
    BMQTST_ASSERT_EQ(obj.clientsCountForProcessor(0), 2);
    BMQTST_ASSERT_EQ(obj.clientsCountForProcessor(1), 0);
    BMQTST_ASSERT_EQ(obj.clientsCountForProcessor(2), 0);
    BMQTST_ASSERT_EQ(
        obj.getProcessorForClient(reinterpret_cast<MyDummyType*>(1)),
        0);

    PV(":: Get processor for client '2'");
    // Ask obj to assign a processor to client '2', it should not be 0 (to
    // honor load balancing).
    BMQTST_ASSERT_NE(
        obj.getProcessorForClient(reinterpret_cast<MyDummyType*>(2)),
        0);

    PV(":: Testing 'setProcessorForClient' with invalid processor");
    // Negative testing, ensure an assertion is thrown if trying to assign to
    // an invalid processor
    BMQTST_ASSERT_OPT_FAIL(
        obj.setProcessorForClient(reinterpret_cast<MyDummyType*>(4),
                                  k_NUM_PROCESSORS));
    BMQTST_ASSERT_OPT_FAIL(
        obj.setProcessorForClient(reinterpret_cast<MyDummyType*>(4), -1));
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 4: test4_forceAssociate(); break;
    case 3: test3_loadBalancing(); break;
    case 2: test2_singleProcessorLoadBalancer(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
