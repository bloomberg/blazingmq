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

// mqbs_datastore.t.cpp                                               -*-C++-*-
#include <mqbs_datastore.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// MWC
#include <mwcu_printutil.h>

// BDE
#include <bsl_utility.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

// BENCHMARKING LIBRARY
#ifdef BSLS_PLATFORM_OS_LINUX
#include <benchmark/benchmark.h>
#endif

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

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
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    {
        PV("DataStoreRecordKey");

        const bsls::Types::Uint64 k_SEQUENCE_NUM     = 10482;
        const unsigned int        k_PRIMARY_LEASE_ID = 9999;

        // Default constructor
        mqbs::DataStoreRecordKey keyDefault;
        ASSERT_EQ(keyDefault.d_sequenceNum, 0U);
        ASSERT_EQ(keyDefault.d_primaryLeaseId, 0U);

        // Valued constructor
        mqbs::DataStoreRecordKey keyValued(k_SEQUENCE_NUM, k_PRIMARY_LEASE_ID);
        ASSERT_EQ(keyValued.d_sequenceNum, k_SEQUENCE_NUM);
        ASSERT_EQ(keyValued.d_primaryLeaseId, k_PRIMARY_LEASE_ID);
    }

    {
        PV("DataStoreRecord");

        const mqbs::RecordType::Enum k_RECORD_TYPE =
            mqbs::RecordType::e_CONFIRM;
        const bsls::Types::Uint64 k_RECORD_OFFSET                   = 1024;
        const unsigned int        k_DATA_OR_QLIST_RECORD_PADDED_LEN = 8238;

        // Default constructor
        mqbs::DataStoreRecord recordDefault;
        ASSERT_EQ(recordDefault.d_recordOffset, 0U);
        ASSERT_EQ(recordDefault.d_messageOffset, 0U);
        ASSERT_EQ(recordDefault.d_appDataUnpaddedLen, 0U);
        ASSERT_EQ(recordDefault.d_dataOrQlistRecordPaddedLen, 0U);
        ASSERT_EQ(recordDefault.d_recordType, mqbs::RecordType::e_UNDEFINED);
        ASSERT_EQ(recordDefault.d_messagePropertiesInfo.isPresent(), false);

        // Valued constructor 1
        mqbs::DataStoreRecord recordValued1(k_RECORD_TYPE, k_RECORD_OFFSET);
        ASSERT_EQ(recordValued1.d_recordOffset, k_RECORD_OFFSET);
        ASSERT_EQ(recordValued1.d_messageOffset, 0U);
        ASSERT_EQ(recordValued1.d_appDataUnpaddedLen, 0U);
        ASSERT_EQ(recordValued1.d_dataOrQlistRecordPaddedLen, 0U);
        ASSERT_EQ(recordValued1.d_recordType, k_RECORD_TYPE);
        ASSERT_EQ(recordValued1.d_messagePropertiesInfo.isPresent(), false);

        // Valued constructor 2
        mqbs::DataStoreRecord recordValued2(k_RECORD_TYPE,
                                            k_RECORD_OFFSET,
                                            k_DATA_OR_QLIST_RECORD_PADDED_LEN);
        ASSERT_EQ(recordValued2.d_recordOffset, k_RECORD_OFFSET);
        ASSERT_EQ(recordValued2.d_messageOffset, 0U);
        ASSERT_EQ(recordValued2.d_appDataUnpaddedLen, 0U);
        ASSERT_EQ(recordValued2.d_dataOrQlistRecordPaddedLen,
                  k_DATA_OR_QLIST_RECORD_PADDED_LEN);
        ASSERT_EQ(recordValued2.d_recordType, k_RECORD_TYPE);
        ASSERT_EQ(recordValued2.d_messagePropertiesInfo.isPresent(), false);
    }
}

static void test2_defaultHashUniqueness()
// ------------------------------------------------------------------------
// DEFAULT HASH UNIQUENESS
//
// Concerns:
//   Verify the uniqueness of the hash of a DataStoreRecordKey using
//   default hash algo.
//
// Plan:
//   - Generate lots of DataStoreRecordKeys, compute their hash, and
//     measure some collisions statistics.
//
// Testing:
//   Hash uniqueness of the generated DataStoreRecordKeys using default
//   hash.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Because there is no emplace on unordered_map, the temporary list
    // created upon insertion of objects in the map uses the default
    // allocator.

    mwctst::TestHelper::printTestName("DEFAULT HASH UNIQUENESS");

#ifdef BSLS_PLATFORM_OS_AIX
    const bsls::Types::Int64 k_NUM_KEYS = 1000000;  // 1M
#elif defined(__has_feature)
    // Avoid timeout under MemorySanitizer
    const bsls::Types::Int64 k_NUM_KEYS = __has_feature(memory_sanitizer)
                                              ? 1000000    // 1M
                                              : 10000000;  // 10M
#elif defined(__SANITIZE_MEMORY__)
    // GCC-supported macros for checking MSAN
    const bsls::Types::Int64 k_NUM_KEYS = 1000000;  // 1M
#else
    const bsls::Types::Int64 k_NUM_KEYS = 10000000;  // 10M
#endif

    typedef bsl::vector<mqbs::DataStoreRecordKey> Keys;

    // hash -> vector of corresponding DataStoreRecordKeys
    bsl::unordered_map<size_t, Keys> hashes(s_allocator_p);
    hashes.reserve(k_NUM_KEYS);

    bsl::hash<mqbs::DataStoreRecordKey> hasher;
    size_t                              maxCollisionsHash = 0;
    size_t                              maxCollisions     = 0;

    // Generate Keys and update the 'hashes' map
    for (bsls::Types::Int64 i = 0; i < k_NUM_KEYS; ++i) {
        mqbs::DataStoreRecordKey key(i, 7);

        size_t hash = hasher(key);

        Keys& keys = hashes[hash];
        keys.push_back(key);
        if (maxCollisions < keys.size()) {
            maxCollisions     = keys.size();
            maxCollisionsHash = hash;
        }
    }

    // Above value is just chosen after looking at the number of collisions
    // by running this test case manually.  In most runs, number of
    // collisions was in the range of [0, 3].
    const size_t k_MAX_EXPECTED_COLLISIONS = 4;

    ASSERT_LT(maxCollisions, k_MAX_EXPECTED_COLLISIONS);

    if (s_verbosityLevel >= 1 || maxCollisions >= k_MAX_EXPECTED_COLLISIONS) {
        cout << "Hash collision percentage..........: "
             << 100 - 100.0f * hashes.size() / k_NUM_KEYS << "%" << endl
             << "Max collisions.....................: " << maxCollisions
             << endl
             << "Hash...............................: " << maxCollisionsHash
             << endl
             << "Num Keys with that hash...........: "
             << hashes[maxCollisionsHash].size() << endl
             << "Keys with the highest collisions..: " << endl;

        Keys& keys = hashes[maxCollisionsHash];
        for (size_t i = 0; i < keys.size(); ++i) {
            cout << "  " << keys[i] << '\n';
        }
    }
}

static void test3_customHashUniqueness()
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
    const bsls::Types::Int64 k_NUM_KEYS = 5000000;  // 5M
#elif defined(__has_feature)
    // Avoid timeout under MemorySanitizer
    const bsls::Types::Int64 k_NUM_KEYS = __has_feature(memory_sanitizer)
                                              ? 5000000    // 5M
                                              : 10000000;  // 10M
#elif defined(__SANITIZE_MEMORY__)
    // GCC-supported macros for checking MSAN
    const bsls::Types::Int64 k_NUM_KEYS = 5000000;  // 5M
#else
    const bsls::Types::Int64 k_NUM_KEYS = 10000000;  // 10M
#endif

    typedef bsl::vector<mqbs::DataStoreRecordKey> Keys;

    // hash -> vector of corresponding Keys
    bsl::unordered_map<size_t, Keys> hashes(s_allocator_p);

    hashes.reserve(k_NUM_KEYS);

    mqbs::DataStoreRecordKeyHashAlgo hasher;
    size_t                           maxCollisionsHash = 0;
    size_t                           maxCollisions     = 0;

    // Generate Keys and update the 'hashes' map
    for (bsls::Types::Int64 i = 0; i < k_NUM_KEYS; ++i) {
        mqbs::DataStoreRecordKey key(i, 7);

        size_t hash = hasher(key);

        Keys& keys = hashes[hash];
        keys.push_back(key);
        if (maxCollisions < keys.size()) {
            maxCollisions     = keys.size();
            maxCollisionsHash = hash;
        }
    }

    // Above value is just chosen after looking at the number of collisions
    // by running this test case manually.  In most runs, number of
    // collisions was in the range of [0, 3].
    const size_t k_MAX_EXPECTED_COLLISIONS = 4;

    ASSERT_LT(maxCollisions, k_MAX_EXPECTED_COLLISIONS);

    if (s_verbosityLevel >= 1 || maxCollisions >= k_MAX_EXPECTED_COLLISIONS) {
        cout << "Hash collision percentage..........: "
             << 100 - 100.0f * hashes.size() / k_NUM_KEYS << "%" << endl
             << "Max collisions.....................: " << maxCollisions
             << endl
             << "Hash...............................: " << maxCollisionsHash
             << endl
             << "Num Keys with that hash...........: "
             << hashes[maxCollisionsHash].size() << endl
             << "Keys with the highest collisions..: " << endl;

        Keys& keys = hashes[maxCollisionsHash];
        for (size_t i = 0; i < keys.size(); ++i) {
            cout << "  " << keys[i] << '\n';
        }
    }
}
BSLA_MAYBE_UNUSED
static void testN1_defaultHashBenchmark()
// ------------------------------------------------------------------------
// DEFAULT HASH BENCHMARK
//
// Concerns:
//   Benchmark hashing function of a DataStoreRecordKey using default
//   hashing algo.
//
// Plan:
//   - Generate hash of a DataStoreRecordKey in a timed loop.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("DEFAULT HASH BENCHMARK");

    const size_t                        k_NUM_ITERATIONS = 10000000;  // 10M
    bsl::hash<mqbs::DataStoreRecordKey> hasher;
    // same as: bslh::Hash<> hasher;

    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
        hasher(mqbs::DataStoreRecordKey(i, 7));
    }
    bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

    cout << "Calculated " << k_NUM_ITERATIONS << " default hashes of the Key"
         << " in " << mwcu::PrintUtil::prettyTimeInterval(end - begin) << ".\n"
         << "Above implies that 1 hash of the Key was calculated in "
         << (end - begin) / k_NUM_ITERATIONS << " nano seconds.\n"
         << "In other words: "
         << mwcu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                (k_NUM_ITERATIONS * 1000000000) / (end - begin)))
         << " hashes per second.\n";
}
BSLA_MAYBE_UNUSED
static void testN2_customHashBenchmark()
// ------------------------------------------------------------------------
// CUSTOM HASH BENCHMARK
//
// Concerns:
//   Benchmark hashing function of a Key using custom hashing algo.
//
// Plan:
//   - Generate hash of a Key in a timed loop.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("CUSTOM HASH BENCHMARK");

    const size_t                     k_NUM_ITERATIONS = 10000000;  // 10M
    mqbs::DataStoreRecordKeyHashAlgo hasher;

    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
        hasher(mqbs::DataStoreRecordKey(i, 7));
    }
    bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

    cout << "Calculated " << k_NUM_ITERATIONS << " custom hashes of the Key"
         << "in " << mwcu::PrintUtil::prettyTimeInterval(end - begin) << ".\n"
         << "Above implies that 1 hash of the Key was calculated in "
         << (end - begin) / k_NUM_ITERATIONS << " nano seconds.\n"
         << "In other words: "
         << mwcu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                (k_NUM_ITERATIONS * 1000000000) / (end - begin)))
         << " hashes per second.\n";
}
BSLA_MAYBE_UNUSED
static void testN3_orderedMapWithDefaultHashBenchmark()
// ------------------------------------------------------------------------
// ORDERED HASH MAP w/ DEFAULT HASH BENCHMARK
//
// Concerns:
//   Benchmark lookup() in an orderedMap(KEY=mqbs::DataStoreRecordKey) with
//   default hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ORDERED MAP DEFAULT HASH BENCHMARK");

    const size_t             k_NUM_ELEMS = 10000000;  // 10M
    mqbs::DataStoreRecordKey key;

    mwcc::OrderedHashMap<mqbs::DataStoreRecordKey, size_t> ht(k_NUM_ELEMS,
                                                              s_allocator_p);
    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        ht.insert(bsl::make_pair(mqbs::DataStoreRecordKey(i, 7), i));
    }

    ht.clear();

    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (size_t i = 1; i <= k_NUM_ELEMS; ++i) {
        ht.insert(bsl::make_pair(mqbs::DataStoreRecordKey(i, 7), i));
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
BSLA_MAYBE_UNUSED
static void testN4_orderedMapWithCustomHashBenchmark()
// ------------------------------------------------------------------------
// ORDERED MAP w/ CUSTOM HASH BENCHMARK
//
// Concerns:
//   Benchmark lookup() in an orderedMap(KEY=mqbs::DataStoreRecordKey)
//   with custom hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ORDERED MAP CUSTOM HASH BENCHMARK");

    const size_t             k_NUM_ELEMS = 10000000;  // 10M
    mqbs::DataStoreRecordKey key;

    mwcc::OrderedHashMap<mqbs::DataStoreRecordKey,
                         size_t,
                         mqbs::DataStoreRecordKeyHashAlgo>
        ht(k_NUM_ELEMS, s_allocator_p);

    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        ht.insert(bsl::make_pair(mqbs::DataStoreRecordKey(i, 7), i));
    }

    ht.clear();

    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (size_t i = 1; i <= k_NUM_ELEMS; ++i) {
        ht.insert(bsl::make_pair(mqbs::DataStoreRecordKey(i, 7), i));
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
         << " insertions per second.\n";
}

#ifdef BSLS_PLATFORM_OS_LINUX
static void
testN1_defaultHashBenchmark_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// DEFAULT HASH BENCHMARK
//
// Concerns:
//   Benchmark hashing function of a DataStoreRecordKey using default
//   hashing algo.
//
// Plan:
//   - Generate hash of a DataStoreRecordKey in a timed loop.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName(
        "GOOGLE BENCHMARK DEFAULT HASH BENCHMARK");

    bsl::hash<mqbs::DataStoreRecordKey> hasher;
    // same as: bslh::Hash<> hasher;
    for (auto _ : state) {
        for (int i = 0; i < state.range(); ++i) {
            hasher(mqbs::DataStoreRecordKey(i, 7));
        }
    }
}

static void testN2_customHashBenchmark_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// CUSTOM HASH BENCHMARK
//
// Concerns:
//   Benchmark hashing function of a Key using custom hashing algo.
//
// Plan:
//   - Generate hash of a Key in a timed loop.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName(
        "GOOGLE BENCHMARK CUSTOM HASH BENCHMARK");

    mqbs::DataStoreRecordKeyHashAlgo hasher;

    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            hasher(mqbs::DataStoreRecordKey(i, 7));
        }
    }
}

static void testN3_orderedMapWithDefaultHashBenchmark_GoogleBenchmark(
    benchmark::State& state)
// ------------------------------------------------------------------------
// ORDERED HASH MAP w/ DEFAULT HASH BENCHMARK
//
// Concerns:
//   Benchmark lookup() in an orderedMap(KEY=mqbs::DataStoreRecordKey) with
//   default hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("GOOGLE BENCHMARK ORDERED"
                                      "MAP DEFAULT HASH BENCHMARK");

    mqbs::DataStoreRecordKey key;

    mwcc::OrderedHashMap<mqbs::DataStoreRecordKey, size_t> ht(state.range(0),
                                                              s_allocator_p);
    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        ht.insert(bsl::make_pair(mqbs::DataStoreRecordKey(i, 7), i));
    }

    ht.clear();

    for (auto _ : state) {
        for (int i = 1; i <= state.range(0); ++i) {
            ht.insert(bsl::make_pair(mqbs::DataStoreRecordKey(i, 7), i));
        }
    }
}

static void testN4_orderedMapWithCustomHashBenchmark_GoogleBenchmark(
    benchmark::State& state)
// ------------------------------------------------------------------------
// ORDERED MAP w/ CUSTOM HASH BENCHMARK
//
// Concerns:
//   Benchmark lookup() in an orderedMap(KEY=mqbs::DataStoreRecordKey)
//   with custom hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("GOOGLE BENCHMARK ORDERED MAP"
                                      "CUSTOM HASH BENCHMARK");

    mqbs::DataStoreRecordKey key;

    mwcc::OrderedHashMap<mqbs::DataStoreRecordKey,
                         size_t,
                         mqbs::DataStoreRecordKeyHashAlgo>
        ht(state.range(0), s_allocator_p);

    // Warmup
    for (size_t i = 1; i <= 1000; ++i) {
        ht.insert(bsl::make_pair(mqbs::DataStoreRecordKey(i, 7), i));
    }

    ht.clear();
    for (auto _ : state) {
        for (int i = 1; i <= state.range(0); ++i) {
            ht.insert(bsl::make_pair(mqbs::DataStoreRecordKey(i, 7), i));
        }
    }
}
#endif
// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_customHashUniqueness(); break;
    case 2: test2_defaultHashUniqueness(); break;
    case 1: test1_breathingTest(); break;
    case -1:
        MWC_BENCHMARK_WITH_ARGS(testN1_defaultHashBenchmark,
                                RangeMultiplier(10)
                                    ->Range(10, 10000000)
                                    ->Unit(benchmark::kMillisecond));
        break;
    case -2:
        MWC_BENCHMARK_WITH_ARGS(testN2_customHashBenchmark,
                                RangeMultiplier(10)
                                    ->Range(10, 10000000)
                                    ->Unit(benchmark::kMillisecond));
        break;
    case -3:
        MWC_BENCHMARK_WITH_ARGS(testN3_orderedMapWithDefaultHashBenchmark,
                                RangeMultiplier(10)
                                    ->Range(10, 10000000)
                                    ->Unit(benchmark::kMillisecond));
        break;
    case -4:
        MWC_BENCHMARK_WITH_ARGS(testN4_orderedMapWithCustomHashBenchmark,
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