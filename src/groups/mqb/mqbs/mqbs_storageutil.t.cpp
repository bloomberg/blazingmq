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

// mqbs_storageutil.t.cpp                                             -*-C++-*-
#include <mqbs_storageutil.h>

// MQB
#include <mqbi_storage.h>

#include <bmqsys_time.h>

// BDE
#include <bdlt_currenttime.h>
#include <bdlt_datetime.h>
#include <bdlt_datetimeinterval.h>
#include <bdlt_epochutil.h>
#include <bdlt_timeunitratio.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

// =============
// struct Tester
// =============
struct Tester {
  public:
    // DATA
    const bsl::string d_domain1;
    const bsl::string d_domain2;
    const bsl::string d_domain3;

    const bmqt::Uri d_d1uri1;
    const bmqt::Uri d_d1uri2;
    const bmqt::Uri d_d1uri3;
    const bmqt::Uri d_d2uri1;
    const bmqt::Uri d_d2uri2;
    const bmqt::Uri d_d2uri3;
    const bmqt::Uri d_d3uri1;

  public:
    // CREATORS
    Tester()
    : d_domain1("bmq.random.x", bmqtst::TestHelperUtil::allocator())
    , d_domain2("bmq.haha", bmqtst::TestHelperUtil::allocator())
    , d_domain3("bmq.random.y", bmqtst::TestHelperUtil::allocator())
    , d_d1uri1("bmq://bmq.random.x/q1", bmqtst::TestHelperUtil::allocator())
    , d_d1uri2("bmq://bmq.random.x/q2", bmqtst::TestHelperUtil::allocator())
    , d_d1uri3("bmq://bmq.random.x/q3", bmqtst::TestHelperUtil::allocator())
    , d_d2uri1("bmq://bmq.haha/baddie", bmqtst::TestHelperUtil::allocator())
    , d_d2uri2("bmq://bmq.haha/excellent", bmqtst::TestHelperUtil::allocator())
    , d_d2uri3("bmq://bmq.haha/soso", bmqtst::TestHelperUtil::allocator())
    , d_d3uri1("bmq://bmq.random.y/generic",
               bmqtst::TestHelperUtil::allocator())
    {
        // NOTHING
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
static void test1_queueMessagesCountComparator()
// ------------------------------------------------------------------------
// QUEUE MESSAGES COUNT COMPARATOR
//
// Concerns:
//   Ensure proper behavior of 'queueMessagesCountComparator' method.
//
// Testing:
//   queueMessagesCountComparator(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("QUEUE MESSAGES COMPARATOR");

    Tester tester;

    using namespace mqbs;

    StorageUtil::QueueMessagesCount lhs(tester.d_d1uri1, 10);

    StorageUtil::QueueMessagesCount rhs1(tester.d_d1uri2, 5);
    StorageUtil::QueueMessagesCount rhs2(tester.d_d2uri1, 10);
    StorageUtil::QueueMessagesCount rhs3(tester.d_d3uri1, 15);

    BMQTST_ASSERT(StorageUtil::queueMessagesCountComparator(lhs, rhs1));
    BMQTST_ASSERT(!StorageUtil::queueMessagesCountComparator(lhs, rhs2));
    BMQTST_ASSERT(!StorageUtil::queueMessagesCountComparator(lhs, rhs3));
}

static void test2_mergeQueueMessagesCountMap()
// ------------------------------------------------------------------------
// MERGE QUEUE MESSAGES COUNT
//
// Concerns:
//   Ensure proper behavior of 'mergeQueueMessagesCountMap' method.
//
// Testing:
//   mergeQueueMessagesCountMap(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("MERGE QUEUE MESSAGES");

    Tester tester;

    using namespace mqbs;

    StorageUtil::QueueMessagesCountMap qs1(
        bmqtst::TestHelperUtil::allocator());
    qs1.insert(bsl::make_pair(tester.d_d1uri1, 10));
    qs1.insert(bsl::make_pair(tester.d_d1uri2, 20));

    StorageUtil::QueueMessagesCountMap qs2(
        bmqtst::TestHelperUtil::allocator());
    qs2.insert(bsl::make_pair(tester.d_d1uri1, 100));
    qs2.insert(bsl::make_pair(tester.d_d1uri3, 333));

    // qs1 = {URI1 -> 10,  URI2 -> 20}
    // qs2 = {URI1 -> 100, URI3 -> 333}
    StorageUtil::mergeQueueMessagesCountMap(&qs1, qs2);
    BMQTST_ASSERT_EQ(qs1.size(), 3U);  // one entry per URI
    BMQTST_ASSERT_EQ(qs1[tester.d_d1uri1], 10 + 100);
    BMQTST_ASSERT_EQ(qs1[tester.d_d1uri2], 20);
    BMQTST_ASSERT_EQ(qs1[tester.d_d1uri3], 333);
}

static void test3_mergeDomainQueueMessagesCountMap()
// ------------------------------------------------------------------------
// MERGE DOMAIN QUEUE MESSAGES COUNT MAP
//
// Concerns:
//   Ensure proper behavior of 'mergeDomainQueueMessagesCountMap' method.
//
// Testing:
//   mergeDomainQueueMessagesCountMap(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("MERGE DOMAIN QUEUE MESSAGES MAP");

    Tester tester;

    using namespace mqbs;

    StorageUtil::QueueMessagesCountMap qs1(
        bmqtst::TestHelperUtil::allocator());
    qs1.insert(bsl::make_pair(tester.d_d1uri1, 10));
    qs1.insert(bsl::make_pair(tester.d_d1uri2, 20));

    StorageUtil::QueueMessagesCountMap qs2(
        bmqtst::TestHelperUtil::allocator());
    qs2.insert(bsl::make_pair(tester.d_d2uri1, 7777));

    StorageUtil::DomainQueueMessagesCountMap map1(
        bmqtst::TestHelperUtil::allocator());
    map1.insert(bsl::make_pair(tester.d_domain1, qs1));
    map1.insert(bsl::make_pair(tester.d_domain2, qs2));

    StorageUtil::QueueMessagesCountMap qs3(
        bmqtst::TestHelperUtil::allocator());
    qs3.insert(bsl::make_pair(tester.d_d1uri1, 100));
    qs3.insert(bsl::make_pair(tester.d_d1uri3, 333));

    StorageUtil::QueueMessagesCountMap qs4(
        bmqtst::TestHelperUtil::allocator());
    qs4.insert(bsl::make_pair(tester.d_d2uri2, 8888));
    qs4.insert(bsl::make_pair(tester.d_d2uri3, 9999));

    StorageUtil::QueueMessagesCountMap qs5(
        bmqtst::TestHelperUtil::allocator());
    qs5.insert(bsl::make_pair(tester.d_d3uri1, 1234567));

    StorageUtil::DomainQueueMessagesCountMap map2(
        bmqtst::TestHelperUtil::allocator());
    map2.insert(bsl::make_pair(tester.d_domain1, qs3));
    map2.insert(bsl::make_pair(tester.d_domain2, qs4));
    map2.insert(bsl::make_pair(tester.d_domain3, qs5));

    // qs1  = {D1Q1 -> 10, D1Q2 -> 20}
    // qs2  = {D2Q1 -> 7777}
    // qs3  = {D1Q1 -> 100, D1Q3 -> 333}
    // qs4  = {D2Q2 -> 8888, D2Q3 -> 9999}
    // qs5  = {D3Q1 -> 1234567}
    // map1 = {D1Q1 -> 10, D1Q2 -> 20, D2Q1 -> 7777}
    // map2 = {D1Q1 -> 110, D1Q3 -> 333, D2Q2 -> 8888, D2Q3 -> 9999,
    //         D3Q1 -> 1234567}

    StorageUtil::mergeDomainQueueMessagesCountMap(&map1, map2);

    BMQTST_ASSERT_EQ(map1.size(), 3U);
    BMQTST_ASSERT_EQ(map1[tester.d_domain1].size(), 3U);
    BMQTST_ASSERT_EQ(map1[tester.d_domain1][tester.d_d1uri1], 10 + 100);
    BMQTST_ASSERT_EQ(map1[tester.d_domain1][tester.d_d1uri2], 20);
    BMQTST_ASSERT_EQ(map1[tester.d_domain1][tester.d_d1uri3], 333);

    BMQTST_ASSERT_EQ(map1[tester.d_domain2].size(), 3U);
    BMQTST_ASSERT_EQ(map1[tester.d_domain2][tester.d_d2uri1], 7777);
    BMQTST_ASSERT_EQ(map1[tester.d_domain2][tester.d_d2uri2], 8888);
    BMQTST_ASSERT_EQ(map1[tester.d_domain2][tester.d_d2uri3], 9999);
    BMQTST_ASSERT_EQ(map1[tester.d_domain3].size(), 1U);
    BMQTST_ASSERT_EQ(map1[tester.d_domain3][tester.d_d3uri1], 1234567);
}

static void test4_loadArrivalTime()
// ------------------------------------------------------------------------
// LOAD ARRIVAL TIME
//
// Concerns:
//   Ensure proper behavior of 'loadArrivalTime' method.
//
// Testing:
//   loadArrivalTime(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LOAD ARRIVAL TIME");

    // Use timepoint if set
    {
        const bsls::Types::Int64 arrivalTimepointNs =
            bmqsys::Time::highResolutionTimer();

        mqbi::StorageMessageAttributes attributes;
        attributes
            .setArrivalTimestamp(12345678)  // Ignored
            .setArrivalTimepoint(arrivalTimepointNs);

        // 1. bsls::Types::Int64 variant
        bsls::Types::Int64 arrivalTimeNs;
        mqbs::StorageUtil::loadArrivalTime(&arrivalTimeNs, attributes);

        // Expect less than 1 millisecond of time elapsed between the
        // calculation of 'arrivalTimeNs' and 'expectedArrivalTimeNs'.
        const bsls::Types::Int64 expectedArrivalTimeNs =
            bdlt::EpochUtil::convertToTimeInterval(bdlt::CurrentTime::utc())
                .totalNanoseconds() -
            (bmqsys::Time::highResolutionTimer() - arrivalTimepointNs);
        BMQTST_ASSERT_LE(
            expectedArrivalTimeNs - arrivalTimeNs,
            1 * bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);

        // 2. bdlt::Datetime variant
        bdlt::Datetime arrivalDatetime;
        mqbs::StorageUtil::loadArrivalTime(&arrivalDatetime, attributes);

        bdlt::Datetime     expectedArrivalDatetime;
        bsls::TimeInterval expectedArrivalTimeInterval;
        expectedArrivalTimeInterval.addNanoseconds(expectedArrivalTimeNs);
        bdlt::EpochUtil::convertFromTimeInterval(&expectedArrivalDatetime,
                                                 expectedArrivalTimeInterval);

        // Expect less than 1 millisecond of time elapsed between the
        // calculation of 'arrivalDatetime' and 'expectedArrivalDatetime'.
        BMQTST_ASSERT_LE(expectedArrivalDatetime - arrivalDatetime,
                         bdlt::DatetimeInterval(0, 0, 0, 0, 1));  // 1 ms
    }

    // If timepoint is unset, use timestamp instead
    {
        const bsls::Types::Int64 arrivalTimeSec =
            bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc());

        mqbi::StorageMessageAttributes attributes;
        attributes.setArrivalTimestamp(arrivalTimeSec);

        // 1. bsls::Types::Int64 variant
        bsls::Types::Int64 arrivalTimeNs;
        mqbs::StorageUtil::loadArrivalTime(&arrivalTimeNs, attributes);

        const bsls::Types::Int64 expectedArrivalTimeNs =
            arrivalTimeSec * bdlt::TimeUnitRatio::k_NANOSECONDS_PER_SECOND;
        BMQTST_ASSERT_EQ(arrivalTimeNs, expectedArrivalTimeNs);

        // 2. bdlt::Datetime variant
        bdlt::Datetime arrivalDatetime;
        mqbs::StorageUtil::loadArrivalTime(&arrivalDatetime, attributes);

        bdlt::Datetime     expectedArrivalDatetime;
        bsls::TimeInterval expectedArrivalTimeInterval;
        expectedArrivalTimeInterval.addNanoseconds(expectedArrivalTimeNs);
        bdlt::EpochUtil::convertFromTimeInterval(&expectedArrivalDatetime,
                                                 expectedArrivalTimeInterval);
        BMQTST_ASSERT_EQ(arrivalDatetime, expectedArrivalDatetime);
    }
}

static void test5_loadArrivalTimeDelta()
// ------------------------------------------------------------------------
// LOAD ARRIVAL TIME DELTA
//
// Concerns:
//   Ensure proper behavior of 'loadArrivalTimeDelta' method.
//
// Testing:
//   loadArrivalTimeDelta(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LOAD ARRIVAL TIME DELTA");

    // Use timepoint if set
    {
        const bsls::Types::Int64 arrivalTimepointNs =
            bmqsys::Time::highResolutionTimer();

        mqbi::StorageMessageAttributes attributes;
        attributes
            .setArrivalTimestamp(12345678)  // Ignored
            .setArrivalTimepoint(arrivalTimepointNs);

        bsls::Types::Int64 arrivalTimeDeltaNs;
        mqbs::StorageUtil::loadArrivalTimeDelta(&arrivalTimeDeltaNs,
                                                attributes);

        // Expect less than 1 millisecond of time elapsed between the
        // calculation of 'arrivalTimeDeltaNs' and
        // 'expectedArrivalTimeDeltaNs'.
        const bsls::Types::Int64 expectedArrivalTimeDeltaNs =
            bmqsys::Time::highResolutionTimer() - arrivalTimepointNs;
        BMQTST_ASSERT_LE(
            expectedArrivalTimeDeltaNs - arrivalTimeDeltaNs,
            1 * bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
    }

    // If timepoint is unset, use timestamp instead
    {
        const bsls::Types::Int64 arrivalTimeSec =
            bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc());

        mqbi::StorageMessageAttributes attributes;
        attributes.setArrivalTimestamp(arrivalTimeSec);

        bsls::Types::Int64 arrivalTimeDeltaNs;
        mqbs::StorageUtil::loadArrivalTimeDelta(&arrivalTimeDeltaNs,
                                                attributes);

        // Expect less than 1 millisecond of time elapsed between the
        // calculation of 'arrivalTimeDeltaNs' and
        // 'expectedArrivalTimeDeltaNs'.
        const bsls::Types::Int64 expectedArrivalTimeDeltaNs =
            (bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()) -
             arrivalTimeSec) *
            bdlt::TimeUnitRatio::k_NANOSECONDS_PER_SECOND;
        BMQTST_ASSERT_LE(
            expectedArrivalTimeDeltaNs - arrivalTimeDeltaNs,
            1 * bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqsys::Time::initialize();

    switch (_testCase) {
    case 0:
    case 5: test5_loadArrivalTimeDelta(); break;
    case 4: test4_loadArrivalTime(); break;
    case 3: test3_mergeDomainQueueMessagesCountMap(); break;
    case 2: test2_mergeQueueMessagesCountMap(); break;
    case 1: test1_queueMessagesCountComparator(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqsys::Time::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
