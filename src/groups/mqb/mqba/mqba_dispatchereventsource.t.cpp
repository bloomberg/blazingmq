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

// mqba_dispatchereventsource.t.cpp                                   -*-C++-*-
#include <mqba_dispatchereventsource.h>

// BMQ
#include <bmqtst_table.h>

// MQB
#include <mqbevt_ackevent.h>
#include <mqbevt_callbackevent.h>
#include <mqbevt_clusterstateevent.h>
#include <mqbevt_confirmevent.h>
#include <mqbevt_controlmessageevent.h>
#include <mqbevt_dispatcherevent.h>
#include <mqbevt_pushevent.h>
#include <mqbevt_putevent.h>
#include <mqbevt_receiptevent.h>
#include <mqbevt_recoveryevent.h>
#include <mqbevt_rejectevent.h>
#include <mqbevt_storageevent.h>

// BDE
#include <bdlma_localsequentialallocator.h>
#include <bsl_iostream.h>
#include <bsl_vector.h>
#include <bsls_stopwatch.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                 CONSTANTS
// ----------------------------------------------------------------------------

static const int k_BENCHMARK_ITERATIONS = 100000;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

template <typename EVENT_TYPE>
void benchmarkEventType(bmqtst::Table* results, const char* eventName)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(results);

    results->column("Name").insertValue(eventName);
    results->column("sizeof").insertValue(bsl::to_string(sizeof(EVENT_TYPE)));

    // Construction time
    bdlma::LocalSequentialAllocator<1024> lsa(
        bmqtst::TestHelperUtil::allocator());
    bsls::Stopwatch stopwatch;
    stopwatch.reset();
    stopwatch.start();
    for (int i = 0; i < k_BENCHMARK_ITERATIONS; ++i) {
        EVENT_TYPE event(&lsa);
        (void)event;
    }
    stopwatch.stop();
    results->column("constructor, ns/op")
        .insertValue(bsl::to_string(stopwatch.elapsedTime() /
                                    k_BENCHMARK_ITERATIONS * 1e9));

    stopwatch.reset();
    stopwatch.start();
    {
        EVENT_TYPE event(&lsa);
        for (int i = 0; i < k_BENCHMARK_ITERATIONS; ++i) {
            event.reset();
        }
    }
    stopwatch.stop();

    results->column("reset (ns/op)")
        .insertValue(bsl::to_string(stopwatch.elapsedTime() /
                                    k_BENCHMARK_ITERATIONS * 1e9));
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise basic functionality before beginning testing in earnest.
//   Probe that functionality to discover basic errors.
//
// Testing:
//   - Basic construction and destruction
//   - Getting events from the source
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    mqba::DispatcherEventSource obj(bmqtst::TestHelperUtil::allocator());

    // Verify we can get each type of event
    {
        bsl::shared_ptr<mqbevt::AckEvent> event = obj.getAckEvent();
        BMQTST_ASSERT(event);
        BMQTST_ASSERT_EQ(event->type(), mqbi::DispatcherEventType::e_ACK);
    }

    {
        bsl::shared_ptr<mqbevt::CallbackEvent> event = obj.getCallbackEvent();
        BMQTST_ASSERT(event);
        BMQTST_ASSERT_EQ(event->type(), mqbi::DispatcherEventType::e_CALLBACK);
    }

    {
        bsl::shared_ptr<mqbevt::ClusterStateEvent> event =
            obj.getClusterStateEvent();
        BMQTST_ASSERT(event);
        BMQTST_ASSERT_EQ(event->type(),
                         mqbi::DispatcherEventType::e_CLUSTER_STATE);
    }

    {
        bsl::shared_ptr<mqbevt::ConfirmEvent> event = obj.getConfirmEvent();
        BMQTST_ASSERT(event);
        BMQTST_ASSERT_EQ(event->type(), mqbi::DispatcherEventType::e_CONFIRM);
    }

    {
        bsl::shared_ptr<mqbevt::ControlMessageEvent> event =
            obj.getControlMessageEvent();
        BMQTST_ASSERT(event);
        BMQTST_ASSERT_EQ(event->type(),
                         mqbi::DispatcherEventType::e_CONTROL_MSG);
    }

    {
        bsl::shared_ptr<mqbevt::DispatcherEvent> event =
            obj.getDispatcherEvent();
        BMQTST_ASSERT(event);
        BMQTST_ASSERT_EQ(event->type(),
                         mqbi::DispatcherEventType::e_DISPATCHER);
    }

    {
        bsl::shared_ptr<mqbevt::PushEvent> event = obj.getPushEvent();
        BMQTST_ASSERT(event);
        BMQTST_ASSERT_EQ(event->type(), mqbi::DispatcherEventType::e_PUSH);
    }

    {
        bsl::shared_ptr<mqbevt::PutEvent> event = obj.getPutEvent();
        BMQTST_ASSERT(event);
        BMQTST_ASSERT_EQ(event->type(), mqbi::DispatcherEventType::e_PUT);
    }

    {
        bsl::shared_ptr<mqbevt::ReceiptEvent> event = obj.getReceiptEvent();
        BMQTST_ASSERT(event);
        BMQTST_ASSERT_EQ(event->type(),
                         mqbi::DispatcherEventType::e_REPLICATION_RECEIPT);
    }

    {
        bsl::shared_ptr<mqbevt::RecoveryEvent> event = obj.getRecoveryEvent();
        BMQTST_ASSERT(event);
        BMQTST_ASSERT_EQ(event->type(), mqbi::DispatcherEventType::e_RECOVERY);
    }

    {
        bsl::shared_ptr<mqbevt::RejectEvent> event = obj.getRejectEvent();
        BMQTST_ASSERT(event);
        BMQTST_ASSERT_EQ(event->type(), mqbi::DispatcherEventType::e_REJECT);
    }

    {
        bsl::shared_ptr<mqbevt::StorageEvent> event = obj.getStorageEvent();
        BMQTST_ASSERT(event);
        BMQTST_ASSERT_EQ(event->type(), mqbi::DispatcherEventType::e_STORAGE);
    }
}

static void testN1_dispatcherEventBenchmark()
// ------------------------------------------------------------------------
// DISPATCHER EVENT BENCHMARK
//
// Concerns:
//   Measure the performance characteristics of different event types:
//   - Memory footprint
//   - Construction time
//
// Testing:
//   - sizeof different mqbevt event types
//   - Construction performance
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("DISPATCHER EVENT BENCHMARK");

    bmqtst::Table results(bmqtst::TestHelperUtil::allocator());

    benchmarkEventType<mqbevt::AckEvent>(&results, "mqbevt::AckEvent");
    benchmarkEventType<mqbevt::CallbackEvent>(&results,
                                              "mqbevt::CallbackEvent");
    benchmarkEventType<mqbevt::ClusterStateEvent>(&results,
                                                  "mqbevt::ClusterStateEvent");
    benchmarkEventType<mqbevt::ConfirmEvent>(&results, "mqbevt::ConfirmEvent");
    benchmarkEventType<mqbevt::ControlMessageEvent>(
        &results,
        "mqbevt::ControlMessageEvent");
    benchmarkEventType<mqbevt::DispatcherEvent>(&results,
                                                "mqbevt::DispatcherEvent");
    benchmarkEventType<mqbevt::PushEvent>(&results, "mqbevt::PushEvent");
    benchmarkEventType<mqbevt::PutEvent>(&results, "mqbevt::PutEvent");
    benchmarkEventType<mqbevt::ReceiptEvent>(&results, "mqbevt::ReceiptEvent");
    benchmarkEventType<mqbevt::RecoveryEvent>(&results,
                                              "mqbevt::RecoveryEvent");
    benchmarkEventType<mqbevt::RejectEvent>(&results, "mqbevt::RejectEvent");
    benchmarkEventType<mqbevt::StorageEvent>(&results, "mqbevt::StorageEvent");

    results.print(bsl::cout);
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
    case -1: testN1_dispatcherEventBenchmark(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
