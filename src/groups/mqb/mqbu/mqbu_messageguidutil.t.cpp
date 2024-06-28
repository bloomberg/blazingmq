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

// mqbu_messageguidutil.t.cpp                                         -*-C++-*-
#include <mqbu_messageguidutil.h>

// BMQ
#include <bmqt_messageguid.h>
#include <mwcu_memoutstream.h>

// MWC
#include <mwcc_orderedhashmap.h>
#include <mwcu_printutil.h>

// BDE
#include <bdlb_guid.h>
#include <bdlb_guidutil.h>
#include <bdlf_bind.h>
#include <bdlt_datetime.h>
#include <bdlt_epochutil.h>
#include <bdlt_timeunitratio.h>
#include <bsl_list.h>
#include <bsl_set.h>
#include <bsl_sstream.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslmt_barrier.h>
#include <bslmt_threadgroup.h>
#include <bsls_platform.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// BENCHMARKING LIBRARY
#ifdef BSLS_PLATFORM_OS_LINUX
#include <benchmark/benchmark.h>
#endif

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Thread function: wait on the specified `barrier` and then generate the
/// specified `numGUIDs` (in a tight loop) and store them in the specified
/// `out`.
static void threadFunction(bsl::vector<bmqt::MessageGUID>* out,
                           bslmt::Barrier*                 barrier,
                           int                             numGUIDs)
{
    out->reserve(numGUIDs);
    barrier->wait();

    while (--numGUIDs >= 0) {
        bmqt::MessageGUID guid;
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        out->push_back(guid);
    }
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
//   Exercise the basic functionality of the component.
//
// Plan:
//   - Generate a GUID, export it to hex and binary buffer and verify a new
//     GUID build from those buffer is equal.
//   - Generate two GUIDs and make sure they don't compare equal
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    {
        // Create a GUID
        bmqt::MessageGUID guid;
        ASSERT_EQ(guid.isUnset(), true);
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        ASSERT_EQ(guid.isUnset(), false);

        // Export to binary representation
        unsigned char binaryBuffer[bmqt::MessageGUID::e_SIZE_BINARY];
        guid.toBinary(binaryBuffer);

        bmqt::MessageGUID fromBinGUID;
        fromBinGUID.fromBinary(binaryBuffer);
        ASSERT_EQ(fromBinGUID.isUnset(), false);
        ASSERT_EQ(fromBinGUID, guid);

        // Export to hex representation
        char hexBuffer[bmqt::MessageGUID::e_SIZE_HEX];
        guid.toHex(hexBuffer);
        ASSERT_EQ(true,
                  bmqt::MessageGUID::isValidHexRepresentation(hexBuffer));

        bmqt::MessageGUID fromHexGUID;
        fromHexGUID.fromHex(hexBuffer);
        ASSERT_EQ(fromHexGUID.isUnset(), false);
        ASSERT_EQ(fromHexGUID, guid);
    }

    {
        // Create 2 GUIDs, confirm operator!=
        bmqt::MessageGUID guid1;
        mqbu::MessageGUIDUtil::generateGUID(&guid1);

        bmqt::MessageGUID guid2;
        mqbu::MessageGUIDUtil::generateGUID(&guid2);

        ASSERT_NE(guid1, guid2);
    }

    {
        // Ensure that unordered map compiles when custom hash algo is
        // specified.

        bsl::unordered_map<bmqt::MessageGUID,
                           int,
                           bslh::Hash<bmqt::MessageGUIDHashAlgo> >
                          myMap(s_allocator_p);
        bmqt::MessageGUID guid;
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        myMap.insert(bsl::make_pair(guid, 1));

        ASSERT_EQ(1u, myMap.count(guid));
    }
}

static void test2_multithread()
// ------------------------------------------------------------------------
// MULTITHREAD
//
// Concerns:
//   Test mqbu::MessageGUIDUtil::generateGUID() in a multi-threaded
//   environment, making sure that each GUIDs are unique across all
//   threads.
//
// Plan:
//   - Spawn a few threads and have them generate a high number of GUIDs in
//     a tight loop.  Once they are done, make sure each generated GUID is
//     unique.
//
// Testing:
//   Unicity of the generated GUID in a multithreaded environment.
// ------------------------------------------------------------------------
{
    s_ignoreCheckGblAlloc = true;
    // Can't ensure no global memory is allocated because
    // 'bslmt::ThreadUtil::create()' uses the global allocator to allocate
    // memory.

    mwctst::TestHelper::printTestName("MULTITHREAD");

    const int k_NUM_THREADS = 10;

#ifdef BSLS_PLATFORM_OS_AIX
    // This test case times out on AIX if 'k_NUM_GUIDS' is close to 1 million
    // (it's unable to complete in 90 seconds).
    const int k_NUM_GUIDS = 500000;  // 500k
#elif defined(__has_feature)
    // Avoid timeout under MemorySanitizer
    const int k_NUM_GUIDS = __has_feature(memory_sanitizer) ? 500000    // 500k
                                                            : 1000000;  // 1M
#elif defined(__SANITIZE_MEMORY__)
    // GCC-supported macros for checking MSAN
    const int k_NUM_GUIDS = 500000;  // 500k
#else
    const int                k_NUM_GUIDS = 1000000;   // 1M
#endif

    bslmt::ThreadGroup threadGroup(s_allocator_p);

    // Barrier to get each thread to start at the same time; `+1` for this
    // (main) thread.
    bslmt::Barrier barrier(k_NUM_THREADS + 1);

    bsl::vector<bsl::vector<bmqt::MessageGUID> > threadsData(s_allocator_p);
    threadsData.resize(k_NUM_THREADS);

    for (int i = 0; i < k_NUM_THREADS; ++i) {
        int rc = threadGroup.addThread(bdlf::BindUtil::bind(&threadFunction,
                                                            &threadsData[i],
                                                            &barrier,
                                                            k_NUM_GUIDS));
        ASSERT_EQ_D(i, rc, 0);
    }

    barrier.wait();
    threadGroup.joinAll();

    // Check uniqueness across all threads
    bsl::set<bmqt::MessageGUID> allGUIDs(s_allocator_p);
    for (int tIt = 0; tIt < k_NUM_THREADS; ++tIt) {
        const bsl::vector<bmqt::MessageGUID>& guids = threadsData[tIt];
        for (int gIt = 0; gIt < k_NUM_GUIDS; ++gIt) {
            ASSERT_EQ(allGUIDs.insert(guids[gIt]).second, true);
        }
    }
}

static void test3_print()
// ------------------------------------------------------------------------
// PRINT
//
// Concerns:
//   Test printing of a MessageGUID generated GUID, with each of its parts.
//
// Plan:
//   - Hand craft a GUID from hex, use MessageGUIDUtil::print to print it
//     and compare result to expected values.
//   - Generate a 'real' GUID and verify its brokerId field matches the
//     return value of the 'brokerIdHex' accessor.
//
// Testing:
//   Printing of the various parts of a GUID.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;

    mwctst::TestHelper::printTestName("PRINT");

    PV("Testing printing an unset GUID");
    {
        bmqt::MessageGUID guid;
        ASSERT_EQ(guid.isUnset(), true);

        mwcu::MemOutStream out(s_allocator_p);
        mqbu::MessageGUIDUtil::print(out, guid);
        ASSERT_EQ(out.str(), "** UNSET **");
    }

    PV("Test printing of a valid handcrafted GUID");
    {
        // Construct a guid from hex representation and print internal details.
        const char k_HEX_BUFFER[] = "40000500010EA8F9515DCACE04742D2E";
        // The above corresponds to a GUID with the following values:
        //   Version....: [1]
        //   Counter....: [5]
        //   Timer tick.: [297593876864458]
        //   BrokerId...: [CE04742D2E]

        bmqt::MessageGUID guid;
        guid.fromHex(k_HEX_BUFFER);

        // Print and compare
        const char k_EXPECTED[] = "[version: 1, counter: 5, timerTick: "
                                  "297593876864458, brokerId: CE04742D2E]";
        mwcu::MemOutStream out(s_allocator_p);
        mqbu::MessageGUIDUtil::print(out, guid);
        ASSERT_EQ(out.str(), k_EXPECTED);
    }

    PV("Verify brokerId");
    {
        bmqt::MessageGUID guid;
        mqbu::MessageGUIDUtil::generateGUID(&guid);

        mwcu::MemOutStream out(s_allocator_p);
        mqbu::MessageGUIDUtil::print(out, guid);

        // Extract the BrokerId from the printed string, that is the 10
        // characters before the last one (an hexadecimal brokerId is 10
        // characters, and we skip the closing ']').
        bslstl::StringRef printedBrokerId =
            bslstl::StringRef(&out.str()[out.length() - 11], 10);
        ASSERT_EQ(printedBrokerId, mqbu::MessageGUIDUtil::brokerIdHex());
    }
}

static void test4_defaultHashUniqueness()
// ------------------------------------------------------------------------
// DEFAULT HASH UNIQUENESS
//
// Concerns:
//   Verify the uniqueness of the hash of a GUID using default hash algo.
//
// Plan:
//   - Generate a lots of GUIDs, compute their hash, and measure some
//     collisions statistics.
//
// Testing:
//   Hash uniqueness of the generated GUIDs.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Because there is no emplace on unordered_map, the temporary list
    // created upon insertion of objects in the map uses the default
    // allocator.

    mwctst::TestHelper::printTestName("DEFAULT HASH UNIQUENESS");

#ifdef BSLS_PLATFORM_OS_AIX
    const bsls::Types::Int64 k_NUM_GUIDS = 1000000;  // 1M
#elif defined(__has_feature)
    // Avoid timeout under MemorySanitizer
    const bsls::Types::Int64 k_NUM_GUIDS = __has_feature(memory_sanitizer)
                                               ? 1000000    // 1M
                                               : 10000000;  // 10M
#elif defined(__SANITIZE_MEMORY__)
    // GCC-supported macros for checking MSAN
    const bsls::Types::Int64 k_NUM_GUIDS = 1000000;  // 1M
#else
    const bsls::Types::Int64 k_NUM_GUIDS = 10000000;  // 10M
#endif

    typedef bsl::vector<bmqt::MessageGUID> Guids;

    // hash -> vector of corresponding GUIDs
    bsl::unordered_map<size_t, Guids> hashes(s_allocator_p);
    hashes.reserve(k_NUM_GUIDS);

    bsl::hash<bmqt::MessageGUID> hasher;
    size_t                       maxCollisionsHash = 0;
    size_t                       maxCollisions     = 0;

    // Generate GUIDs and update the 'hashes' map
    for (bsls::Types::Int64 i = 0; i < k_NUM_GUIDS; ++i) {
        bmqt::MessageGUID guid;
        mqbu::MessageGUIDUtil::generateGUID(&guid);

        size_t hash = hasher(guid);

        Guids& guids = hashes[hash];
        guids.push_back(guid);
        if (maxCollisions < guids.size()) {
            maxCollisions     = guids.size();
            maxCollisionsHash = hash;
        }
    }

    // Above value is just chosen after looking at the number of collisions
    // by running this test case manually.  In most runs, number of
    // collisions was in the range of [0, 3].
    const size_t k_MAX_EXPECTED_COLLISIONS = 4;

    ASSERT_LT(maxCollisions, k_MAX_EXPECTED_COLLISIONS);

    if (maxCollisions >= k_MAX_EXPECTED_COLLISIONS) {
        cout << "Hash collision percentage..........: "
             << 100 - 100.0f * hashes.size() / k_NUM_GUIDS << "%" << endl
             << "Max collisions.....................: " << maxCollisions
             << endl
             << "Hash...............................: " << maxCollisionsHash
             << endl
             << "Num GUIDs with that hash...........: "
             << hashes[maxCollisionsHash].size() << endl
             << "GUIDs with the highest collisions..: " << endl;

        Guids& guids = hashes[maxCollisionsHash];
        for (size_t i = 0; i < guids.size(); ++i) {
            cout << "  ";
            mqbu::MessageGUIDUtil::print(cout, guids[i]);
            cout << endl;
        }
    }
}

static void test5_customHashUniqueness()
// ------------------------------------------------------------------------
// CUSTOM HASH UNIQUENESS
//
// Concerns:
//   Verify the uniqueness of the hash of a GUID using custom hash algo.
//
// Plan:
//   - Generate a lots of GUIDs, compute their hash, and measure some
//     collisions statistics.
//
// Testing:
//   Hash uniqueness of the generated GUIDs.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Because there is no emplace on unordered_map, the temporary list
    // created upon insertion of objects in the map uses the default
    // allocator.

    mwctst::TestHelper::printTestName("CUSTOM HASH UNIQUENESS");

#ifdef BSLS_PLATFORM_OS_AIX
    const bsls::Types::Int64 k_NUM_GUIDS = 1000000;  // 1M
#elif defined(__has_feature)
    // Avoid timeout under MemorySanitizer
    const bsls::Types::Int64 k_NUM_GUIDS = __has_feature(memory_sanitizer)
                                               ? 1000000    // 1M
                                               : 10000000;  // 10M
#elif defined(__SANITIZE_MEMORY__)
    // GCC-supported macros for checking MSAN
    const bsls::Types::Int64 k_NUM_GUIDS = 1000000;  // 1M
#else
    const bsls::Types::Int64 k_NUM_GUIDS = 10000000;  // 10M
#endif

    typedef bsl::vector<bmqt::MessageGUID> Guids;

    // hash -> vector of corresponding GUIDs
    bsl::unordered_map<size_t, Guids> hashes(s_allocator_p);

    hashes.reserve(k_NUM_GUIDS);

    bslh::Hash<bmqt::MessageGUIDHashAlgo> hasher;
    size_t                                maxCollisionsHash = 0;
    size_t                                maxCollisions     = 0;

    // Generate GUIDs and update the 'hashes' map
    for (bsls::Types::Int64 i = 0; i < k_NUM_GUIDS; ++i) {
        bmqt::MessageGUID guid;
        mqbu::MessageGUIDUtil::generateGUID(&guid);

        size_t hash = hasher(guid);

        Guids& guids = hashes[hash];
        guids.push_back(guid);
        if (maxCollisions < guids.size()) {
            maxCollisions     = guids.size();
            maxCollisionsHash = hash;
        }
    }

    // Above value is just chosen after looking at the number of collisions
    // by running this test case manually.  In most runs, number of
    // collisions was in the range of [0, 3].
    const size_t k_MAX_EXPECTED_COLLISIONS = 4;

    ASSERT_LT(maxCollisions, k_MAX_EXPECTED_COLLISIONS);

    if (maxCollisions >= k_MAX_EXPECTED_COLLISIONS) {
        cout << "Hash collision percentage..........: "
             << 100 - 100.0f * hashes.size() / k_NUM_GUIDS << "%" << endl
             << "Max collisions.....................: " << maxCollisions
             << endl
             << "Hash...............................: " << maxCollisionsHash
             << endl
             << "Num GUIDs with that hash...........: "
             << hashes[maxCollisionsHash].size() << endl
             << "GUIDs with the highest collisions..: " << endl;

        Guids& guids = hashes[maxCollisionsHash];
        for (size_t i = 0; i < guids.size(); ++i) {
            cout << "  ";
            mqbu::MessageGUIDUtil::print(cout, guids[i]);
            cout << endl;
        }
    }
}

// ============================================================================
//                              PERFORMANCE TESTS
// ----------------------------------------------------------------------------
BSLA_MAYBE_UNUSED
static void testN1_decode()
// ------------------------------------------------------------------------
// DECODE
//
// Concerns: Expose a way to decode a GUID from its hex representation for
//   quick troubleshooting.  Optionally, support resolving the timer tick
//   field if the 'currentTimerTick' and 'secondsFromEpoch' are provided
//   (printed from the 'mqbu_messageguidutil::initialize()' method at
//   startup).
//
// Plan:
//   - Build a GUID from reading it's hex representation from stdin, and
//     use the MessageGUIDUtil::print to decode and print it's various
//     parts.
//
// Testing:
//   -
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;  // istringstream allocates

    mwctst::TestHelper::printTestName("DECODE");

    cout << "Please enter the hex representation of a GUID, followed by\n"
         << "<enter> when done (optionally, specify the currentTimerTick as\n"
         << "well as secondsFromEpoch to resolve the time):\n"
         << "  hexGUID [currentTimerTick secondsFromEpoch]" << endl
         << endl;

    // Read from stdin
    char buffer[256];
    bsl::cin.getline(buffer, 256, '\n');

    bsl::istringstream is(buffer);

    bsl::string        hexGuid(s_allocator_p);
    bsls::Types::Int64 startTimerTick        = 0;
    bsls::Types::Int64 startSecondsFromEpoch = 0;

    is >> hexGuid;

    // Ensure valid input
    if (!bmqt::MessageGUID::isValidHexRepresentation(hexGuid.c_str())) {
        cout << "The input '" << buffer << "' is not a valid hex GUID" << endl;
        return;  // RETURN
    }

    // Read optional startTimerTick and startSecondsFromEpoch
    if (!is.eof()) {
        // start  timer tick
        is >> startTimerTick;
        if (is.fail()) {
            cout << "The input '" << buffer << "' is not properly formatted "
                 << "[hexGuid currentTimerTick secondsFromEpoch]" << endl;
            return;  // RETURN
        }

        // seconds from epoch
        is >> startSecondsFromEpoch;
        if (is.fail()) {
            cout << "The input '" << buffer << "' is not properly formatted "
                 << "[hexGuid currentTimerTick secondsFromEpoch]" << endl;
            return;  // RETURN
        }
    }

    // Make a GUID out of it
    bmqt::MessageGUID guid;
    guid.fromHex(hexGuid.c_str());
    ASSERT_EQ(guid.isUnset(), false);

    // Print it
    cout << "--------------------------------" << endl;
    mqbu::MessageGUIDUtil::print(cout, guid);
    cout << endl;

    if (startTimerTick != 0) {
        int                version;
        unsigned int       counter;
        bsls::Types::Int64 guidTimerTick;
        bsl::string        brokerId;

        mqbu::MessageGUIDUtil::extractFields(&version,
                                             &counter,
                                             &guidTimerTick,
                                             &brokerId,
                                             guid);

        bdlt::Datetime timestamp = bdlt::EpochUtil::convertFromTimeT(
            startSecondsFromEpoch);

        timestamp.addMicroseconds(
            (guidTimerTick - startTimerTick) /
            bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MICROSECOND);

        bsl::cout << "Converted timestamp (UTC): " << timestamp << bsl::endl;
    }
}

BSLA_MAYBE_UNUSED
static void testN2_bdlbPerformance()
// ------------------------------------------------------------------------
// PERFORMANCE
//
// Concerns:
//   Test the performance of bdlb::GuidUtil::generate().
//
// Plan:
//   - Time the generation of a huge number of bdlb::GuidUtil::generate()
//     in a tight loop, single threaded.
//
// Testing:
//   Performance of bdlb::Guid generation.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("PERFORMANCE");

    const bsls::Types::Int64 k_NUM_GUIDS = 1000000;  // 1 million

    // ---------------------
    // bdlb::GUID generation
    // ---------------------

    // Warm the cache
    for (int i = 0; i < 1000; ++i) {
        bdlb::Guid guid;
        bdlb::GuidUtil::generate(&guid);
        (void)guid;
    }

    bsls::Types::Int64 start = bsls::TimeUtil::getTimer();

    for (bsls::Types::Int64 i = 0; i < k_NUM_GUIDS; ++i) {
        bdlb::Guid guid;
        bdlb::GuidUtil::generate(&guid);
        (void)guid;
    }

    bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

    cout << "Calculated " << k_NUM_GUIDS << " bdlb::Guids in "
         << mwcu::PrintUtil::prettyTimeInterval(end - start) << ".\n"
         << "Above implies that 1 bdlb::Guid was calculated in "
         << (end - start) / k_NUM_GUIDS << " nano seconds.\n"
         << "In other words: "
         << mwcu::PrintUtil::prettyNumber((k_NUM_GUIDS * 1000000000) /
                                          (end - start))
         << " bdlb::Guids per second." << endl;
}

BSLA_MAYBE_UNUSED
static void testN2_mqbuPerformance()
// ------------------------------------------------------------------------
// PERFORMANCE
//
// Concerns:
//   Test the performance of mqbu::MessageGUIDUtil::generateGUID()
//
// Plan:
//   - Time the generation of a huge number of bmqt::MessageGUIDs in a
//     tight loop, single threaded.
//
// Testing:
//   Performance of the bmqt::MessageGUID generation
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("PERFORMANCE");

    const bsls::Types::Int64 k_NUM_GUIDS = 1000000;  // 1 million

    // ---------------------
    // bmqt::GUID generation
    // ---------------------

    for (int i = 0; i < 1000; ++i) {
        bmqt::MessageGUID guid;
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        (void)guid;
    }

    bsls::Types::Int64 start = bsls::TimeUtil::getTimer();

    for (bsls::Types::Int64 i = 0; i < k_NUM_GUIDS; ++i) {
        bmqt::MessageGUID guid;
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        (void)guid;
    }

    bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

    cout << "Calculated " << k_NUM_GUIDS << " bdlb::Guids in "
         << mwcu::PrintUtil::prettyTimeInterval(end - start) << ".\n"
         << "Above implies that 1 bdlb::Guid was calculated in "
         << (end - start) / k_NUM_GUIDS << " nano seconds.\n"
         << "In other words: "
         << mwcu::PrintUtil::prettyNumber((k_NUM_GUIDS * 1000000000) /
                                          (end - start))
         << " bdlb::Guids per second." << endl;
}

BSLA_MAYBE_UNUSED static void testN3_defaultHashBenchmark()
// ------------------------------------------------------------------------
// DEFAULT HASH BENCHMARK
//
// Concerns:
//   Benchmark hashing function of a GUID using default hashing algo.
//
// Plan:
//   - Generate hash of a GUID in a timed loop.
//
// Testing:
//   NA
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("DEFAULT HASH BENCHMARK");

    const size_t                 k_NUM_ITERATIONS = 10000000;  // 10M
    bsl::hash<bmqt::MessageGUID> hasher;  // same as: bslh::Hash<> hasher;
    bmqt::MessageGUID            guid;
    mqbu::MessageGUIDUtil::generateGUID(&guid);

    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
        hasher(guid);
    }
    bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

    cout << "Calculated " << k_NUM_ITERATIONS << " default hashes of the GUID"
         << " in " << mwcu::PrintUtil::prettyTimeInterval(end - begin) << ".\n"
         << "Above implies that 1 hash of the GUID was calculated in "
         << (end - begin) / k_NUM_ITERATIONS << " nano seconds.\n"
         << "In other words: "
         << mwcu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                (k_NUM_ITERATIONS * 1000000000) / (end - begin)))
         << " hashes per second." << endl;
}

BSLA_MAYBE_UNUSED
static void testN4_customHashBenchmark()
// ------------------------------------------------------------------------
// CUSTOM HASH BENCHMARK
//
// Concerns:
//   Benchmark hashing function of a GUID using custom hashing algo.
//
// Plan:
//   - Generate hash of a GUID in a timed loop.
//
// Testing:
//   NA
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("CUSTOM HASH BENCHMARK");

    const size_t                          k_NUM_ITERATIONS = 10000000;  // 10M
    bslh::Hash<bmqt::MessageGUIDHashAlgo> hasher;
    bmqt::MessageGUID                     guid;
    mqbu::MessageGUIDUtil::generateGUID(&guid);

    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
        hasher(guid);
    }
    bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

    cout << "Calculated " << k_NUM_ITERATIONS << " custom hashes of the GUID"
         << "in " << mwcu::PrintUtil::prettyTimeInterval(end - begin) << ".\n"
         << "Above implies that 1 hash of the GUID was calculated in "
         << (end - begin) / k_NUM_ITERATIONS << " nano seconds.\n"
         << "In other words: "
         << mwcu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                (k_NUM_ITERATIONS * 1000000000) / (end - begin)))
         << " hashes per second." << endl;
}

BSLA_MAYBE_UNUSED static void testN5_hashTableWithDefaultHashBenchmark()
// ------------------------------------------------------------------------
// HASH TABLE w/ DEFAULT HASH BENCHMARK
//
// Concerns:
//   Benchmark lookup() in a hashtable(KEY=bmqt::MessageGUID) with default
//   hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("HASH TABLE w/ DEFAULT HASH BENCHMARK");

    const size_t      k_NUM_ELEMS = 10000000;  // 10M
    bmqt::MessageGUID guid;
    bsl::unordered_map<bmqt::MessageGUID, size_t> ht(s_allocator_p);
    ht.reserve(k_NUM_ELEMS);

    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }

    ht.clear();

    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (size_t i = 1; i <= k_NUM_ELEMS; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }
    bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

    cout << "Inserted " << k_NUM_ELEMS << " elements in hashtable using "
         << "default hash algorithm in "
         << mwcu::PrintUtil::prettyTimeInterval(end - begin) << ".\n"
         << "Above implies that 1 element was inserted in "
         << (end - begin) / k_NUM_ELEMS << " nano seconds.\n"
         << "In other words: "
         << mwcu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                (k_NUM_ELEMS * 1000000000) / (end - begin)))
         << " insertions per second." << endl;
}

BSLA_MAYBE_UNUSED static void testN6_hashTableWithCustomHashBenchmark()
// ------------------------------------------------------------------------
// HASH TABLE w/ CUSTOM HASH BENCHMARK
//
// Concerns:
//   Benchmark lookup() in a hashtable(KEY=bmqt::MessageGUID) with custom
//   hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("HASH TABLE w/ CUSTOM HASH BENCHMARK");

    const size_t      k_NUM_ELEMS = 10000000;  // 10M
    bmqt::MessageGUID guid;

    bsl::unordered_map<bmqt::MessageGUID,
                       size_t,
                       bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        ht(s_allocator_p);
    ht.reserve(k_NUM_ELEMS);

    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }

    ht.clear();

    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (size_t i = 1; i <= k_NUM_ELEMS; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }
    bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

    cout << "Inserted " << k_NUM_ELEMS << " elements in hashtable using "
         << "custom hash algorithm in "
         << mwcu::PrintUtil::prettyTimeInterval(end - begin) << ".\n"
         << "Above implies that 1 element was inserted in "
         << (end - begin) / k_NUM_ELEMS << " nano seconds.\n"
         << "In other words: "
         << mwcu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                (k_NUM_ELEMS * 1000000000) / (end - begin)))
         << " insertions per second." << endl;
}

BSLA_MAYBE_UNUSED static void testN7_orderedMapWithDefaultHashBenchmark()
// ------------------------------------------------------------------------
// ORDERED HASH MAP w/ DEFAULT HASH BENCHMARK
//
// Concerns:
//   Benchmark lookup() in an orderedMap(KEY=bmqt::MessageGUID) with
//   default hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ORDERED MAP DEFAULT HASH BENCHMARK");

    const size_t      k_NUM_ELEMS = 10000000;  // 10M
    bmqt::MessageGUID guid;

    mwcc::OrderedHashMap<bmqt::MessageGUID, size_t> ht(k_NUM_ELEMS,
                                                       s_allocator_p);
    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }

    ht.clear();

    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (size_t i = 1; i <= k_NUM_ELEMS; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }
    bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

    cout << "Inserted " << k_NUM_ELEMS << " elements in ordered map using "
         << "default hash algorithm in "
         << mwcu::PrintUtil::prettyTimeInterval(end - begin) << ".\n"
         << "Above implies that 1 element was inserted in "
         << (end - begin) / k_NUM_ELEMS << " nano seconds.\n"
         << "In other words: "
         << mwcu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                (k_NUM_ELEMS * 1000000000) / (end - begin)))
         << " insertions per second." << endl;
}

BSLA_MAYBE_UNUSED static void testN8_orderedMapWithCustomHashBenchmark()
// ------------------------------------------------------------------------
// ORDERED MAP w/ CUSTOM HASH BENCHMARK
//
// Concerns:
//   Benchmark lookup() in an orderedMap(KEY=bmqt::MessageGUID) with custom
//   hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ORDERED MAP CUSTOM HASH BENCHMARK");

    const size_t      k_NUM_ELEMS = 10000000;  // 10M
    bmqt::MessageGUID guid;

    mwcc::OrderedHashMap<bmqt::MessageGUID,
                         size_t,
                         bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        ht(k_NUM_ELEMS, s_allocator_p);

    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }

    ht.clear();

    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (size_t i = 1; i <= k_NUM_ELEMS; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }
    bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

    cout << "Inserted " << k_NUM_ELEMS << " elements in ordered map using "
         << "custom hash algorithm in "
         << mwcu::PrintUtil::prettyTimeInterval(end - begin) << ".\n"
         << "Above implies that 1 element was inserted in "
         << (end - begin) / k_NUM_ELEMS << " nano seconds.\n"
         << "In other words: "
         << mwcu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                (k_NUM_ELEMS * 1000000000) / (end - begin)))
         << " insertions per second." << endl;
}

// Begin Benchmarking Library
#ifdef BSLS_PLATFORM_OS_LINUX
static void testN1_decode_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// DECODE
//
// Concerns: Expose a way to decode a GUID from its hex representation for
//   quick troubleshooting.  Optionally, support resolving the timer tick
//   field if the 'currentTimerTick' and 'secondsFromEpoch' are provided
//   (printed from the 'mqbu_messageguidutil::initialize()' method at
//   startup).
//
// Plan:
//   - Build a GUID from reading it's hex representation from stdin, and
//     use the MessageGUIDUtil::print to decode and print it's various
//     parts.
//
// Testing:
//   -
// ------------------------------------------------------------------------
{
    for (auto _ : state) {
        state.PauseTiming();
        s_ignoreCheckDefAlloc = true;  // istringstream allocates

        mwctst::TestHelper::printTestName("GOOGLE BENCHMARK DECODE");

        cout << "Please enter the hex representation of a GUID, followed by\n"
             << "<enter> when done (optionally, specify the currentTimerTick "
                "as\n"
             << "well as secondsFromEpoch to resolve the time):\n"
             << "  hexGUID [currentTimerTick secondsFromEpoch]" << endl
             << endl;

        // Read from stdin
        char buffer[256];
        bsl::cin.getline(buffer, 256, '\n');

        bsl::istringstream is(buffer);

        bsl::string        hexGuid(s_allocator_p);
        bsls::Types::Int64 startTimerTick        = 0;
        bsls::Types::Int64 startSecondsFromEpoch = 0;

        is >> hexGuid;

        // Ensure valid input
        if (!bmqt::MessageGUID::isValidHexRepresentation(hexGuid.c_str())) {
            cout << "The input '" << buffer << "' is not a valid hex GUID"
                 << endl;
            return;  // RETURN
        }

        // Read optional startTimerTick and startSecondsFromEpoch
        if (!is.eof()) {
            // start  timer tick
            is >> startTimerTick;
            if (is.fail()) {
                cout << "The input '" << buffer
                     << "' is not properly formatted "
                     << "[hexGuid currentTimerTick secondsFromEpoch]" << endl;
                return;  // RETURN
            }

            // seconds from epoch
            is >> startSecondsFromEpoch;
            if (is.fail()) {
                cout << "The input '" << buffer
                     << "' is not properly formatted "
                     << "[hexGuid currentTimerTick secondsFromEpoch]" << endl;
                return;  // RETURN
            }
        }
        state.ResumeTiming();
        // Make a GUID out of it
        bmqt::MessageGUID guid;
        guid.fromHex(hexGuid.c_str());
        ASSERT_EQ(guid.isUnset(), false);

        // Print it
        cout << "--------------------------------" << endl;
        mqbu::MessageGUIDUtil::print(cout, guid);
        cout << endl;

        if (startTimerTick != 0) {
            int                version;
            unsigned int       counter;
            bsls::Types::Int64 guidTimerTick;
            bsl::string        brokerId;

            mqbu::MessageGUIDUtil::extractFields(&version,
                                                 &counter,
                                                 &guidTimerTick,
                                                 &brokerId,
                                                 guid);

            bdlt::Datetime timestamp = bdlt::EpochUtil::convertFromTimeT(
                startSecondsFromEpoch);

            timestamp.addMicroseconds(
                (guidTimerTick - startTimerTick) /
                bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MICROSECOND);

            bsl::cout << "Converted timestamp (UTC): " << timestamp
                      << bsl::endl;
        }
    }
}

static void testN2_mqbuPerformance_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// PERFORMANCE
//
// Concerns:
//   Test the performance of mqbu::MessageGUIDUtil::generateGUID()
//
// Plan:
//   - Time the generation of a huge number of bmqt::MessageGUIDs in a
//     tight loop, single threaded.
//
// Testing:
//   Performance of the bmqt::MessageGUID generation
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("GOOGLE BENCHMARK PERFORMANCE");

    // ----------------------------
    // bmqt::MessageGUID generation
    // ----------------------------

    // Warm the cache
    for (int i = 0; i < 1000; ++i) {
        bmqt::MessageGUID guid;
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        (void)guid;
    }

    for (auto _ : state) {
        for (bsls::Types::Int64 i = 0; i < state.range(0); ++i) {
            bmqt::MessageGUID guid;
            mqbu::MessageGUIDUtil::generateGUID(&guid);
            (void)guid;
        }
    }
}

static void testN2_bdlbPerformance_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// PERFORMANCE
//
// Concerns:
//   Test the performance of bdlb::GUIDUtil::generate()
//
// Plan:
//   - Time the generation of a huge number of bdlb::Guid::guids in a
//     tight loop, single threaded.
//
// Testing:
//   Performance of the bdlb::GUID generation
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("GOOGLE BENCHMARK PERFORMANCE");
    // Warm the cache
    for (int i = 0; i < 1000; ++i) {
        bdlb::Guid guid;
        bdlb::GuidUtil::generate(&guid);
        (void)guid;
    }

    for (auto _ : state) {
        for (bsls::Types::Int64 i = 0; i < state.range(0); ++i) {
            bdlb::Guid guid;
            bdlb::GuidUtil::generate(&guid);
            (void)guid;
        }
    }
}

static void
testN3_defaultHashBenchmark_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// DEFAULT HASH BENCHMARK
//
// Concerns:
//   Benchmark hashing function of a GUID using default hashing algo.
//
// Plan:
//   - Generate hash of a GUID in a timed loop.
//
// Testing:
//   NA
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName(
        "GOOGLE BENCHMARK DEFAULT HASH BENCHMARK");

    bsl::hash<bmqt::MessageGUID> hasher;  // same as: bslh::Hash<> hasher;
    bmqt::MessageGUID            guid;
    mqbu::MessageGUIDUtil::generateGUID(&guid);
    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            hasher(guid);
        }
    }
}

static void testN4_customHashBenchmark_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// CUSTOM HASH BENCHMARK
//
// Concerns:
//   Benchmark hashing function of a GUID using custom hashing algo.
//
// Plan:
//   - Generate hash of a GUID in a timed loop.
//
// Testing:
//   NA
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName(
        "GOOGLE BENCHMARK CUSTOM HASH BENCHMARK");

    bslh::Hash<bmqt::MessageGUIDHashAlgo> hasher;
    bmqt::MessageGUID                     guid;
    mqbu::MessageGUIDUtil::generateGUID(&guid);
    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            hasher(guid);
        }
    }
}

static void testN5_hashTableWithDefaultHashBenchmark_GoogleBenchmark(
    benchmark::State& state)
// ------------------------------------------------------------------------
// HASH TABLE w/ DEFAULT HASH BENCHMARK
//
// Concerns:
//   Benchmark lookup() in a hashtable(KEY=bmqt::MessageGUID) with default
//   hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("GOOGLE BENCHMARK HASH TABLE "
                                      "w/ DEFAULT HASH BENCHMARK");

    bmqt::MessageGUID                             guid;
    bsl::unordered_map<bmqt::MessageGUID, size_t> ht(s_allocator_p);
    ht.reserve(state.range(0));

    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }

    ht.clear();
    for (auto _ : state) {
        for (int i = 1; i <= state.range(0); ++i) {
            mqbu::MessageGUIDUtil::generateGUID(&guid);
            ht.insert(bsl::make_pair(guid, i));
        }
    }
}

static void testN6_hashTableWithCustomHashBenchmark_GoogleBenchmark(
    benchmark::State& state)
// ------------------------------------------------------------------------
// HASH TABLE w/ CUSTOM HASH BENCHMARK
//
// Concerns:
//   Benchmark lookup() in a hashtable(KEY=bmqt::MessageGUID) with custom
//   hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("GOOGLE BENCHMARK HASH TABLE "
                                      "w/ CUSTOM HASH BENCHMARK");

    bmqt::MessageGUID guid;

    bsl::unordered_map<bmqt::MessageGUID,
                       size_t,
                       bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        ht(s_allocator_p);
    ht.reserve(state.range(0));

    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }

    ht.clear();

    for (auto _ : state) {
        for (int i = 1; i <= state.range(0); ++i) {
            mqbu::MessageGUIDUtil::generateGUID(&guid);
            ht.insert(bsl::make_pair(guid, i));
        }
    }
}

static void testN7_orderedMapWithDefaultHashBenchmark_GoogleBenchmark(
    benchmark::State& state)
// ------------------------------------------------------------------------
// ORDERED HASH MAP w/ DEFAULT HASH BENCHMARK
//
// Concerns:
//   Benchmark lookup() in an orderedMap(KEY=bmqt::MessageGUID) with
//   default hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("GOOGLE BENCHMARK ORDERED MAP "
                                      "DEFAULT HASH BENCHMARK");

    bmqt::MessageGUID guid;

    mwcc::OrderedHashMap<bmqt::MessageGUID, size_t> ht(state.range(0),
                                                       s_allocator_p);
    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }

    ht.clear();

    for (auto _ : state) {
        for (int i = 1; i <= state.range(0); ++i) {
            mqbu::MessageGUIDUtil::generateGUID(&guid);
            ht.insert(bsl::make_pair(guid, i));
        }
    }
}

static void testN8_orderedMapWithCustomHashBenchmark_GoogleBenchmark(
    benchmark::State& state)
// ------------------------------------------------------------------------
// ORDERED MAP w/ CUSTOM HASH BENCHMARK
//
// Concerns:
//   Benchmark lookup() in an orderedMap(KEY=bmqt::MessageGUID) with custom
//   hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("GOOGLE BENCHMARK ORDERED MAP "
                                      "CUSTOM HASH BENCHMARK");

    bmqt::MessageGUID guid;

    mwcc::OrderedHashMap<bmqt::MessageGUID,
                         size_t,
                         bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        ht(state.range(0), s_allocator_p);

    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }

    ht.clear();

    for (auto _ : state) {
        for (int i = 1; i <= state.range(0); ++i) {
            mqbu::MessageGUIDUtil::generateGUID(&guid);
            ht.insert(bsl::make_pair(guid, i));
        }
    }
}
#endif

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // To be called only once per process instantiation.  Calling those two
    // here before the TEST_PROLOG because 'MessageGUIDUtil::initialize' prints
    // a BALL_LOG_INFO which allocates using the default allocator.
    bsls::TimeUtil::initialize();
    mqbu::MessageGUIDUtil::initialize();

    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 5: test5_customHashUniqueness(); break;
    case 4: test4_defaultHashUniqueness(); break;
    case 3: test3_print(); break;
    case 2: test2_multithread(); break;
    case 1: test1_breathingTest(); break;
    case -1: testN1_decode(); break;
    case -2:
        MWC_BENCHMARK_WITH_ARGS(testN2_bdlbPerformance,
                                RangeMultiplier(10)
                                    ->Range(10, 10000000)
                                    ->Unit(benchmark::kMillisecond));
        MWC_BENCHMARK_WITH_ARGS(testN2_mqbuPerformance,
                                RangeMultiplier(10)
                                    ->Range(10, 10000000)
                                    ->Unit(benchmark::kMillisecond));
        break;
    case -3:
        MWC_BENCHMARK_WITH_ARGS(testN3_defaultHashBenchmark,
                                RangeMultiplier(10)
                                    ->Range(10, 10000000)
                                    ->Unit(benchmark::kMillisecond));
        break;
    case -4:
        MWC_BENCHMARK_WITH_ARGS(testN4_customHashBenchmark,
                                RangeMultiplier(10)
                                    ->Range(10, 10000000)
                                    ->Unit(benchmark::kMillisecond));
        break;
    case -5:
        MWC_BENCHMARK_WITH_ARGS(testN5_hashTableWithDefaultHashBenchmark,
                                RangeMultiplier(10)
                                    ->Range(10, 10000000)
                                    ->Unit(benchmark::kMillisecond));
        break;
    case -6:
        MWC_BENCHMARK_WITH_ARGS(testN6_hashTableWithCustomHashBenchmark,
                                RangeMultiplier(10)
                                    ->Range(10, 10000000)
                                    ->Unit(benchmark::kMillisecond));
        break;
    case -7:
        MWC_BENCHMARK_WITH_ARGS(testN7_orderedMapWithDefaultHashBenchmark,
                                RangeMultiplier(10)
                                    ->Range(10, 10000000)
                                    ->Unit(benchmark::kMillisecond));
        break;
    case -8:
        MWC_BENCHMARK_WITH_ARGS(testN8_orderedMapWithCustomHashBenchmark,
                                RangeMultiplier(10)
                                    ->Range(10, 10000000)
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

// ----------------------------------------------------------------------------
// NOTICE: