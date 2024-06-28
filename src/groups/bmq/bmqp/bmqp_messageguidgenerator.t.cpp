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

// bmqp_messageguidgenerator.t.cpp                                    -*-C++-*-
#include <bmqp_messageguidgenerator.h>

// BMQ
#include <bmqt_messageguid.h>

// MWC
#include <mwcc_orderedhashmap.h>
#include <mwcu_memoutstream.h>
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
#include <bsl_unordered_set.h>
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
                           bmqp::MessageGUIDGenerator*     generator,
                           int                             numGUIDs)
{
    out->reserve(numGUIDs);
    barrier->wait();

    while (--numGUIDs >= 0) {
        bmqt::MessageGUID guid;
        generator->generateGUID(&guid);
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
    s_ignoreCheckDefAlloc = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    mwctst::TestHelper::printTestName("BREATHING TEST");

    bmqp::MessageGUIDGenerator generator(0);

    {
        // Create a GUID
        bmqt::MessageGUID guid;
        ASSERT_EQ(guid.isUnset(), true);
        generator.generateGUID(&guid);
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
        generator.generateGUID(&guid1);

        bmqt::MessageGUID guid2;
        generator.generateGUID(&guid2);

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
        generator.generateGUID(&guid);
        myMap.insert(bsl::make_pair(guid, 1));

        ASSERT_EQ(1u, myMap.count(guid));
    }
}

static void test2_extract()
// ------------------------------------------------------------------------
// TBD:
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;  // Implicit string conversion in ASSERT_EQ

    mwctst::TestHelper::printTestName("EXTRACT");

    {
        PVV("All 0s");
        const char k_HEX_BUFFER[] = "40000000000000000000000000000000";
        // The above corresponds to a GUID with the following values:
        //   Version....: [1]
        //   Counter....: [0]
        //   Timer tick.: [0]
        //   ClientID...: [000000000000]

        bmqt::MessageGUID guid;
        guid.fromHex(k_HEX_BUFFER);

        int                version;
        unsigned int       counter;
        bsls::Types::Int64 timerTick;
        bsl::string        clientId(s_allocator_p);

        int rc = bmqp::MessageGUIDGenerator::extractFields(&version,
                                                           &counter,
                                                           &timerTick,
                                                           &clientId,
                                                           guid);
        ASSERT_EQ(rc, 0);
        ASSERT_EQ(version, 1);
        ASSERT_EQ(counter, 0u);
        ASSERT_EQ(timerTick, 0);
        ASSERT_EQ(clientId, "000000000000");
    }

    {
        PVV("All 1s");
        const char k_HEX_BUFFER[] = "7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";
        // The above corresponds to a GUID with the following values:
        //   Version....: [1]
        //   Counter....: [4194304]
        //   Timer tick.: [72057594037927936]
        //   ClientID...: [FFFFFFFFFFFF]

        bmqt::MessageGUID guid;
        guid.fromHex(k_HEX_BUFFER);

        int                version;
        unsigned int       counter;
        bsls::Types::Int64 timerTick;
        bsl::string        clientId(s_allocator_p);

        int rc = bmqp::MessageGUIDGenerator::extractFields(&version,
                                                           &counter,
                                                           &timerTick,
                                                           &clientId,
                                                           guid);
        ASSERT_EQ(rc, 0);
        ASSERT_EQ(version, 1);
        ASSERT_EQ(counter, 4194303u);
        ASSERT_EQ(timerTick, 72057594037927935);
        ASSERT_EQ(clientId, "FFFFFFFFFFFF");
    }

    {
        PVV("Randomly crafted");
        const char k_HEX_BUFFER[] = "6D3AC9010EA8F9515DCA0DCE04742D2E";
        // The above corresponds to a GUID with the following values:
        //   Version....: [1]
        //   Counter....: [2964169]
        //   Timer tick.: [297593876864458]
        //   ClientID...: [0DCE04742D2E]

        bmqt::MessageGUID guid;
        guid.fromHex(k_HEX_BUFFER);

        int                version;
        unsigned int       counter;
        bsls::Types::Int64 timerTick;
        bsl::string        clientId(s_allocator_p);

        int rc = bmqp::MessageGUIDGenerator::extractFields(&version,
                                                           &counter,
                                                           &timerTick,
                                                           &clientId,
                                                           guid);
        ASSERT_EQ(rc, 0);
        ASSERT_EQ(version, 1);
        ASSERT_EQ(counter, 2964169u);
        ASSERT_EQ(timerTick, 297593876864458);
        ASSERT_EQ(clientId, "0DCE04742D2E");
    }
}

static void test3_multithreadUseIP()
// ------------------------------------------------------------------------
// MULTITHREAD
//
// Concerns:
//   Test bmqp::MessageGUIDGenerator::generateGUID() in a multi-threaded
//   environment, making sure that each GUIDs are unique accross all
//   threads.  This test uses default implementation of calculating
//   'ClientId' part of a GUID, which attempts to use IP address of the
//   host.  See test4_multithreadUseHostname() for alternate scenario.
//   Note that it is possible that IP resolution fails because of the way
//   host is configured (eg, inside a dpkg chroot jail, etc).  In such
//   scenario, this test will behave same as test4.
//
// Plan:
//   - Spawn a few threads and have them generate a high number of GUIDs in
//     a tight loop.  Once they are done, make sure each generated GUID is
//     unique.
//
// Testing:
//   Unicity of the generated GUID in a multithreaded environment when
//   using IP to calculate 'ClientId' part of the GUIDs.
// ------------------------------------------------------------------------
{
    s_ignoreCheckGblAlloc = true;
    // Can't ensure no global memory is allocated because
    // 'bslmt::ThreadUtil::create()' uses the global allocator to allocate
    // memory.

    s_ignoreCheckDefAlloc = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

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

    bmqp::MessageGUIDGenerator generator(0);

    for (int i = 0; i < k_NUM_THREADS; ++i) {
        int rc = threadGroup.addThread(bdlf::BindUtil::bind(&threadFunction,
                                                            &threadsData[i],
                                                            &barrier,
                                                            &generator,
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

static void test4_multithreadUseHostname()
// ------------------------------------------------------------------------
// MULTITHREAD
//
// Concerns:
//   Test bmqp::MessageGUIDGenerator::generateGUID() in a multi-threaded
//   environment, making sure that each GUIDs are unique accross all
//   threads.  This test creates the generator which doesn't resolve
//   hostname ip address and uses hostname instead to calculate 'ClientId'
//   part of the GUID.
//
// Plan:
//   - Spawn a few threads and have them generate a high number of GUIDs in
//     a tight loop.  Once they are done, make sure each generated GUID is
//     unique.
//
// Testing:
//   Unicity of the generated GUID in a multithreaded environment when
//   using hostname to calculate 'ClientId' part of the GUIDs.
// ------------------------------------------------------------------------
{
    s_ignoreCheckGblAlloc = true;
    // Can't ensure no global memory is allocated because
    // 'bslmt::ThreadUtil::create()' uses the global allocator to allocate
    // memory.

    s_ignoreCheckDefAlloc = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

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

    // Create generator with 'doIpResolving' flag set to false
    bmqp::MessageGUIDGenerator generator(0, false);

    for (int i = 0; i < k_NUM_THREADS; ++i) {
        int rc = threadGroup.addThread(bdlf::BindUtil::bind(&threadFunction,
                                                            &threadsData[i],
                                                            &barrier,
                                                            &generator,
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

static void test5_print()
// ------------------------------------------------------------------------
// PRINT
//
// Concerns:
//   Test printing of a MessageGUID generated GUID, with each of its parts.
//
// Plan:
//   - Hand craft a GUID from hex, use MessageGUIDGenerator::print to print
//     it and compare result to expected values.
//   - Generate a 'real' GUID and verify its clientId field matches the
//     return value of the 'clientIdHex' accessor.
//
// Testing:
//   Printing of the various parts of a GUID.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    mwctst::TestHelper::printTestName("PRINT");

    bmqp::MessageGUIDGenerator generator(0);

    PV("Testing printing an unset GUID");
    {
        bmqt::MessageGUID guid;
        ASSERT_EQ(guid.isUnset(), true);

        mwcu::MemOutStream out(s_allocator_p);
        bmqp::MessageGUIDGenerator::print(out, guid);
        ASSERT_EQ(out.str(), "** UNSET **");
    }

    PV("Test printing of a valid handcrafted GUID (version 1)");
    {
        // Construct a guid from hex representation and print internal details.
        const char k_HEX_BUFFER[] = "6D3AC9010EA8F9515DCA0DCE04742D2E";
        // The above corresponds to a GUID with the following values:
        //   Version....: [1]
        //   Counter....: [2964169]
        //   Timer tick.: [297593876864458]
        //   ClientID...: [0DCE04742D2E]

        bmqt::MessageGUID guid;
        guid.fromHex(k_HEX_BUFFER);

        // Print and compare
        const char k_EXPECTED[] = "1-2964169-297593876864458-0DCE04742D2E";
        mwcu::MemOutStream out(s_allocator_p);
        bmqp::MessageGUIDGenerator::print(out, guid);
        ASSERT_EQ(out.str(), k_EXPECTED);
    }

    PV("Test printing of an unsupported handcrafted GUID (version 0)");
    {
        // Construct a guid from hex representation and print internal details.
        const char k_HEX_BUFFER[] = "00000500010EA8F9515DCACE04742D2E";
        // The above corresponds to a GUID with the following values:
        //   Version....: [0]
        //   Counter....: [5]
        //   Timer tick.: [297593876864458]
        //   BrokerId...: [CE04742D2E]

        bmqt::MessageGUID guid;
        guid.fromHex(k_HEX_BUFFER);

        // Print and compare
        const char         k_EXPECTED[] = "[Unsupported GUID version 0]";
        mwcu::MemOutStream out(s_allocator_p);
        bmqp::MessageGUIDGenerator::print(out, guid);
        ASSERT_EQ(out.str(), k_EXPECTED);
    }

    PV("Verify clientId");
    {
        bmqt::MessageGUID guid;
        generator.generateGUID(&guid);

        mwcu::MemOutStream out(s_allocator_p);
        bmqp::MessageGUIDGenerator::print(out, guid);

        PVV("Output: '" << out.str() << "',  GUID " << guid);
        // Extract the ClientId from the printed string, that is the last 12
        // characters.
        bslstl::StringRef printedClientId =
            bslstl::StringRef(&out.str()[out.length() - 12], 12);
        ASSERT_EQ(printedClientId, generator.clientIdHex());
    }

    PV("Test printing of a test-only GUID");
    {
        bmqt::MessageGUID guid1 = bmqp::MessageGUIDGenerator::testGUID();
        bmqt::MessageGUID guid2 = bmqp::MessageGUIDGenerator::testGUID();

        ASSERT(!guid1.isUnset());
        ASSERT(!guid2.isUnset());

        // Print and compare
        const char         k_EXPECTED_1[] = "1-0-0-000000000001";
        const char         k_EXPECTED_2[] = "1-0-0-000000000002";
        mwcu::MemOutStream out1(s_allocator_p);
        mwcu::MemOutStream out2(s_allocator_p);
        bmqp::MessageGUIDGenerator::print(out1, guid1);
        bmqp::MessageGUIDGenerator::print(out2, guid2);

        ASSERT_EQ(out1.str(), k_EXPECTED_1);
        ASSERT_EQ(out2.str(), k_EXPECTED_2);
    }
}

static void test6_defaultHashUniqueness()
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

    bmqp::MessageGUIDGenerator generator(0);

    // Generate GUIDs and update the 'hashes' map
    for (bsls::Types::Int64 i = 0; i < k_NUM_GUIDS; ++i) {
        bmqt::MessageGUID guid;
        generator.generateGUID(&guid);

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
            bmqp::MessageGUIDGenerator::print(cout, guids[i]);
            cout << endl;
        }
    }
}

static void test7_customHashUniqueness()
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

    bmqp::MessageGUIDGenerator generator(0);

    // Generate GUIDs and update the 'hashes' map
    for (bsls::Types::Int64 i = 0; i < k_NUM_GUIDS; ++i) {
        bmqt::MessageGUID guid;
        generator.generateGUID(&guid);

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
            bmqp::MessageGUIDGenerator::print(cout, guids[i]);
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
//   (printed from the 'bmqp_messageguidutil::initialize()' method at
//   startup).
//
// Plan:
//   - Build a GUID from reading it's hex representation from stdin, and
//     use the MessageGUIDGenerator::print to decode and print it's various
//     parts.
//
// Testing:
//   -
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;  // istringstream allocates

    mwctst::TestHelper::printTestName("DECODE");

    cout << "Please enter the hex representation of a GUID, followed by\n"
         << "<enter> when done (optionally, specify the nanoSecondsFromEpoch\n"
         << "to resolve the time):\n"
         << "  hexGUID [nanoSecondsFromEpoch]" << endl
         << endl;

    // Read from stdin
    char buffer[256];
    bsl::cin.getline(buffer, 256, '\n');

    bsl::istringstream is(buffer);

    bsl::string        hexGuid(s_allocator_p);
    bsls::Types::Int64 nanoSecondsFromEpoch = 0;

    is >> hexGuid;

    // Ensure valid input
    if (!bmqt::MessageGUID::isValidHexRepresentation(hexGuid.c_str())) {
        cout << "The input '" << buffer << "' is not a valid hex GUID" << endl;
        return;  // RETURN
    }

    // Read optional nanoSecondsFromEpoch
    if (!is.eof()) {
        is >> nanoSecondsFromEpoch;
        if (is.fail()) {
            cout << "The input '" << buffer << "' is not properly formatted "
                 << "[hexGuid nanoSecondsFromEpoch]" << endl;
            return;  // RETURN
        }
    }

    // Make a GUID out of it
    bmqt::MessageGUID guid;
    guid.fromHex(hexGuid.c_str());
    ASSERT_EQ(guid.isUnset(), false);

    // Print it
    cout << "--------------------------------" << endl;
    bmqp::MessageGUIDGenerator::print(cout, guid);
    cout << endl;

    if (nanoSecondsFromEpoch != 0) {
        int                version;
        unsigned int       counter;
        bsls::Types::Int64 timerTick;
        bsl::string        clientId;

        int rc = bmqp::MessageGUIDGenerator::extractFields(&version,
                                                           &counter,
                                                           &timerTick,
                                                           &clientId,
                                                           guid);

        if (rc == 0) {
            bsls::TimeInterval interval;
            interval.setTotalNanoseconds(nanoSecondsFromEpoch + timerTick);

            const bdlt::Datetime timestamp =
                bdlt::EpochUtil::convertFromTimeInterval(interval);

            bsl::cout << "Converted timestamp (UTC): " << timestamp
                      << bsl::endl;
        }
        else {
            bsl::cout << "GUID field extraction failed (rc: " << rc << ")"
                      << bsl::endl;
        }
    }
}

BSLA_MAYBE_UNUSED
static void testN2_bmqtPerformance()
// ------------------------------------------------------------------------
// PERFORMANCE
//
// Concerns:
//   Test the performance of bmqp::MessageGUIDGenerator::generateGUID()
//
// Plan:
//   - Time the generation of a huge number of bmqt::MessageGUIDs in a
//     tight loop, single threaded.
//
// Testing:
//   Performance of the bmqt::MessageGUID generation.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    mwctst::TestHelper::printTestName("PERFORMANCE");

    const bsls::Types::Int64 k_NUM_GUIDS = 1000000;  // 1O million

    // ----------------------------
    // bmqt::MessageGUID generation
    // ----------------------------

    bmqp::MessageGUIDGenerator generator(0);

    // Warm the cache
    for (int i = 0; i < 1000; ++i) {
        bmqt::MessageGUID guid;
        generator.generateGUID(&guid);
        (void)guid;
    }

    bsls::Types::Int64 start = bsls::TimeUtil::getTimer();

    for (bsls::Types::Int64 i = 0; i < k_NUM_GUIDS; ++i) {
        bmqt::MessageGUID guid;
        generator.generateGUID(&guid);
        (void)guid;
    }

    bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

    cout << "Calculated " << k_NUM_GUIDS << " bmqt::MessageGUIDs in "
         << mwcu::PrintUtil::prettyTimeInterval(end - start) << ".\n"
         << "Above implies that 1 bmqt::MessageGUID was calculated in "
         << (end - start) / k_NUM_GUIDS << " nano seconds.\n"
         << "In other words: "
         << mwcu::PrintUtil::prettyNumber((k_NUM_GUIDS * 1000000000) /
                                          (end - start))
         << " bmqt::MessageGUIDs per second.\n\n"
         << endl;
}

BSLA_MAYBE_UNUSED
static void testN2_bdlbPerformance()
// ------------------------------------------------------------------------
// PERFORMANCE
//
// Concerns:
//   Test performance of bdlb::GuidUtil::generate().
//
// Plan:
//   - Time the generation of a huge number of bdlb::GuidUtil::generate() in a
//     tight loop, single threaded.
//
// Testing:
//   Performance of the bdlb::Guid generation.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

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
    s_ignoreCheckDefAlloc = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    mwctst::TestHelper::printTestName("DEFAULT HASH BENCHMARK");

    const size_t                 k_NUM_ITERATIONS = 10000000;  // 10M
    bsl::hash<bmqt::MessageGUID> hasher;  // same as: bslh::Hash<> hasher;
    bmqt::MessageGUID            guid;
    bmqp::MessageGUIDGenerator   generator(0);

    generator.generateGUID(&guid);

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

BSLA_MAYBE_UNUSED static void testN4_customHashBenchmark()
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
    s_ignoreCheckDefAlloc = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    mwctst::TestHelper::printTestName("CUSTOM HASH BENCHMARK");

    const size_t                          k_NUM_ITERATIONS = 10000000;  // 10M
    bslh::Hash<bmqt::MessageGUIDHashAlgo> hasher;
    bmqt::MessageGUID                     guid;
    bmqp::MessageGUIDGenerator            generator(0);

    generator.generateGUID(&guid);

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
    s_ignoreCheckDefAlloc = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    mwctst::TestHelper::printTestName("HASH TABLE w/ DEFAULT HASH BENCHMARK");

    const size_t      k_NUM_ELEMS = 10000000;  // 10M
    bmqt::MessageGUID guid;
    bsl::unordered_map<bmqt::MessageGUID, size_t> ht(s_allocator_p);
    ht.reserve(k_NUM_ELEMS);

    bmqp::MessageGUIDGenerator generator(0);

    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        generator.generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }

    ht.clear();

    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (size_t i = 1; i <= k_NUM_ELEMS; ++i) {
        generator.generateGUID(&guid);
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
    s_ignoreCheckDefAlloc = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    mwctst::TestHelper::printTestName("HASH TABLE w/ CUSTOM HASH BENCHMARK");

    const size_t      k_NUM_ELEMS = 10000000;  // 10M
    bmqt::MessageGUID guid;

    bsl::unordered_map<bmqt::MessageGUID,
                       size_t,
                       bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        ht(s_allocator_p);
    ht.reserve(k_NUM_ELEMS);

    bmqp::MessageGUIDGenerator generator(0);

    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        generator.generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }

    ht.clear();

    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (size_t i = 1; i <= k_NUM_ELEMS; ++i) {
        generator.generateGUID(&guid);
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
    s_ignoreCheckDefAlloc = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    mwctst::TestHelper::printTestName("ORDERED MAP DEFAULT HASH BENCHMARK");

    const size_t      k_NUM_ELEMS = 10000000;  // 10M
    bmqt::MessageGUID guid;

    mwcc::OrderedHashMap<bmqt::MessageGUID, size_t> ht(k_NUM_ELEMS,
                                                       s_allocator_p);

    bmqp::MessageGUIDGenerator generator(0);

    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        generator.generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }

    ht.clear();

    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (size_t i = 1; i <= k_NUM_ELEMS; ++i) {
        generator.generateGUID(&guid);
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
    s_ignoreCheckDefAlloc = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    mwctst::TestHelper::printTestName("ORDERED MAP CUSTOM HASH BENCHMARK");

    const size_t      k_NUM_ELEMS = 10000000;  // 10M
    bmqt::MessageGUID guid;

    mwcc::OrderedHashMap<bmqt::MessageGUID,
                         size_t,
                         bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        ht(k_NUM_ELEMS, s_allocator_p);

    bmqp::MessageGUIDGenerator generator(0);

    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        generator.generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }

    ht.clear();

    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (size_t i = 1; i <= k_NUM_ELEMS; ++i) {
        generator.generateGUID(&guid);
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

// Begin Benchmarking Tests

#ifdef BSLS_PLATFORM_OS_LINUX
static void testN1_decode_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// DECODE
//
// Concerns: Expose a way to decode a GUID from its hex representation for
//   quick troubleshooting.  Optionally, support resolving the timer tick
//   field if the 'currentTimerTick' and 'secondsFromEpoch' are provided
//   (printed from the 'bmqp_messageguidutil::initialize()' method at
//   startup).
//
// Plan:
//   - Build a GUID from reading its hex representation from stdin, and
//     use the MessageGUIDGenerator::print to decode and print it's various
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
             << "<enter> when done (optionally, specify the "
                "nanoSecondsFromEpoch\n"
             << "to resolve the time):\n"
             << "  hexGUID [nanoSecondsFromEpoch]" << endl
             << endl;

        // Read from stdin
        char buffer[256];
        bsl::cin.getline(buffer, 256, '\n');

        bsl::istringstream is(buffer);

        bsl::string        hexGuid(s_allocator_p);
        bsls::Types::Int64 nanoSecondsFromEpoch = 0;

        is >> hexGuid;

        // Ensure valid input
        if (!bmqt::MessageGUID::isValidHexRepresentation(hexGuid.c_str())) {
            cout << "The input '" << buffer << "' is not a valid hex GUID"
                 << endl;
            return;  // RETURN
        }

        // Read optional nanoSecondsFromEpoch
        if (!is.eof()) {
            is >> nanoSecondsFromEpoch;
            if (is.fail()) {
                cout << "The input '" << buffer
                     << "' is not properly formatted "
                     << "[hexGuid nanoSecondsFromEpoch]" << endl;
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
        bmqp::MessageGUIDGenerator::print(cout, guid);
        cout << endl;

        if (nanoSecondsFromEpoch != 0) {
            int                version;
            unsigned int       counter;
            bsls::Types::Int64 timerTick;
            bsl::string        clientId;

            int rc = bmqp::MessageGUIDGenerator::extractFields(&version,
                                                               &counter,
                                                               &timerTick,
                                                               &clientId,
                                                               guid);

            if (rc == 0) {
                bsls::TimeInterval interval;
                interval.setTotalNanoseconds(nanoSecondsFromEpoch + timerTick);

                const bdlt::Datetime timestamp =
                    bdlt::EpochUtil::convertFromTimeInterval(interval);

                bsl::cout << "Converted timestamp (UTC): " << timestamp
                          << bsl::endl;
            }
            else {
                bsl::cout << "GUID field extraction failed (rc: " << rc << ")"
                          << bsl::endl;
            }
        }
    }
}

static void testN2_bmqtPerformance_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// PERFORMANCE
//
// Concerns:
//   Test the performance of bmqp::MessageGUIDGenerator::generateGUID()
//   and compare it with that of bdlb::GuidUtil::generate().
//
// Plan:
//   - Time the generation of a huge number of bmqt::MessageGUIDs in a
//     tight loop, single threaded.
//   - Repeat above for bdlb::GuidUtil::generate().
//
// Testing:
//   Performance of the bmqt::MessageGUID generation and comparison with
//   bdlb::Guid generation.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    mwctst::TestHelper::printTestName("GOOGLE BENCHMARK PERFORMANCE");
    // ----------------------------
    // bmqt::MessageGUID generation
    // ----------------------------

    bmqp::MessageGUIDGenerator generator(0);

    // Warm the cache
    for (int i = 0; i < 1000; ++i) {
        bmqt::MessageGUID guid;
        generator.generateGUID(&guid);
        (void)guid;
    }

    for (auto _ : state) {
        for (bsls::Types::Int64 i = 0; i < state.range(0); ++i) {
            bmqt::MessageGUID guid;
            generator.generateGUID(&guid);
            (void)guid;
        }
    }
}

static void testN2_bdlbPerformance_GoogleBenchmark(benchmark::State& state)
{
    // ---------------------
    // bdlb::GUID generation
    // ---------------------

    // Warm the cache
    s_ignoreCheckDefAlloc = true;

    mwctst::TestHelper::printTestName("GOOGLE BENCHMARK PERFORMANCE");
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
    s_ignoreCheckDefAlloc = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    mwctst::TestHelper::printTestName("GOOGLE BENCHMARK: DEFAULT "
                                      "HASH BENCHMARK");

    bsl::hash<bmqt::MessageGUID> hasher;  // same as: bslh::Hash<> hasher;
    bmqt::MessageGUID            guid;
    bmqp::MessageGUIDGenerator   generator(0);

    generator.generateGUID(&guid);

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
    s_ignoreCheckDefAlloc = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    mwctst::TestHelper::printTestName(
        "GOOGLE BENCHMARK CUSTOM HASH BENCHMARK");

    bslh::Hash<bmqt::MessageGUIDHashAlgo> hasher;
    bmqt::MessageGUID                     guid;
    bmqp::MessageGUIDGenerator            generator(0);

    generator.generateGUID(&guid);

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
    s_ignoreCheckDefAlloc = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    mwctst::TestHelper::printTestName("GOOGLE BENCHMARK HASH TABLE "
                                      "w/ DEFAULT HASH BENCHMARK");

    bmqt::MessageGUID                             guid;
    bsl::unordered_map<bmqt::MessageGUID, size_t> ht(s_allocator_p);
    ht.reserve(state.range(0));

    bmqp::MessageGUIDGenerator generator(0);

    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        generator.generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }

    ht.clear();

    for (auto _ : state) {
        for (int i = 1; i <= state.range(0); ++i) {
            generator.generateGUID(&guid);
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
    s_ignoreCheckDefAlloc = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    mwctst::TestHelper::printTestName("GOOGLE BENCHMARK HASH TABLE "
                                      "w/ CUSTOM HASH BENCHMARK");

    bmqt::MessageGUID guid;

    bsl::unordered_map<bmqt::MessageGUID,
                       size_t,
                       bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        ht(s_allocator_p);
    ht.reserve(state.range(0));

    bmqp::MessageGUIDGenerator generator(0);

    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        generator.generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }

    ht.clear();

    for (auto _ : state) {
        for (int i = 1; i <= state.range(0); ++i) {
            generator.generateGUID(&guid);
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
    s_ignoreCheckDefAlloc = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    mwctst::TestHelper::printTestName("GOOGLE BENCHMARK ORDERED MAP "
                                      "DEFAULT HASH BENCHMARK");

    bmqt::MessageGUID guid;

    mwcc::OrderedHashMap<bmqt::MessageGUID, size_t> ht(state.range(0),
                                                       s_allocator_p);

    bmqp::MessageGUIDGenerator generator(0);

    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        generator.generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }

    ht.clear();
    for (auto _ : state) {
        for (int i = 1; i <= state.range(0); ++i) {
            generator.generateGUID(&guid);
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
    s_ignoreCheckDefAlloc = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    mwctst::TestHelper::printTestName("GOOGLE BENCHMARK ORDERED MAP "
                                      "CUSTOM HASH BENCHMARK");

    bmqt::MessageGUID guid;

    mwcc::OrderedHashMap<bmqt::MessageGUID,
                         size_t,
                         bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        ht(state.range(0), s_allocator_p);

    bmqp::MessageGUIDGenerator generator(0);

    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        generator.generateGUID(&guid);
        ht.insert(bsl::make_pair(guid, i));
    }

    ht.clear();

    for (auto _ : state) {
        for (int i = 1; i <= state.range(0); ++i) {
            generator.generateGUID(&guid);
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
    // To be called only once per process instantiation.
    bsls::TimeUtil::initialize();

    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 7: test7_customHashUniqueness(); break;
    case 6: test6_defaultHashUniqueness(); break;
    case 5: test5_print(); break;
    case 4: test4_multithreadUseHostname(); break;
    case 3: test3_multithreadUseIP(); break;
    case 2: test2_extract(); break;
    case 1: test1_breathingTest(); break;
    case -1: testN1_decode(); break;
    case -2:
        // Todo: split test case
        MWC_BENCHMARK_WITH_ARGS(testN2_bdlbPerformance,
                                RangeMultiplier(10)
                                    ->Range(10, 10000000)
                                    ->Unit(benchmark::kMillisecond));
        MWC_BENCHMARK_WITH_ARGS(testN2_bmqtPerformance,
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