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

// mqbu_storagekey.t.cpp                                              -*-C++-*-
#include <mqbu_storagekey.h>

// MQB
#include <mqbs_filestoreprotocol.h>

// MWC
#include <mwcc_orderedhashmap.h>
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// BDE
#include <bdlb_random.h>
#include <bdlde_md5.h>
#include <bsl_set.h>
#include <bslh_hash.h>
#include <bsls_keyword.h>
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
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Populate the specified `key` buffer with a unique character array of
/// length `mqbu::StorageKey::e_KEY_LENGTH_BINARY` by hashing the specified
/// `value`, and also populate the specified `keys` with the result.
/// Behavior is undefined unless `key` and `keys` are not null and buffer
/// pointed by `key` is at least `mqbu::StorageKey::e_KEY_LENGTH_BINARY`
/// long.
void generateStorageKey(mqbu::StorageKey*                     key,
                        bsl::unordered_set<mqbu::StorageKey>* keys,
                        const bsl::string&                    value)
{
    bdlde::Md5::Md5Digest digest;
    bdlde::Md5            md5(value.data(), value.length());

    bsls::Types::Int64 time = bsls::TimeUtil::getTimer();
    md5.update(&time, sizeof(time));
    // NOTE: We add the time in the initial hash so that an open queue, close
    //       queue, open queue will yield a different hash, even for the same
    //       URI.
    md5.loadDigestAndReset(&digest);
    key->fromBinary(digest.buffer());

    while (keys->find(*key) != keys->end()) {
        // 'hashKey' already exists. Re-hash the hash, and append the current
        // time to the md5 input data (so that collisions won't potentially
        // degenerate to a long 'linkedList' like, since the hash of the hash
        // has a deterministic value).
        md5.update(digest.buffer(), mqbs::FileStoreProtocol::k_HASH_LENGTH);
        time = bsls::TimeUtil::getTimer();
        md5.update(&time, sizeof(time));
        md5.loadDigestAndReset(&digest);
        key->fromBinary(digest.buffer());
    }

    // Found a unique key
    keys->insert(*key);
}

/// Populate the specified `keys` with the specified `numKeys` number of
/// randomly generated `StorageKey`.
void generateStorageKeys(bsl::unordered_set<mqbu::StorageKey>* keys,
                         int                                   numKeys)
{
    mqbu::StorageKey storageKey;
    bsl::string      uri("bmq://domain.subdomain.app/queue");

    for (int i = 0; i < numKeys; ++i) {
        generateStorageKey(&storageKey, keys, uri);
    }
}

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

void test1_breathingTest()
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
    PV("Test some invalid StorageKeys");
    mqbu::StorageKey s1;
    ASSERT_EQ(true, s1.isNull());

    mqbu::StorageKey s2;
    s1 = s2;

    ASSERT_EQ(true, s1.isNull());
    ASSERT_EQ(true, s2.isNull());

    mqbu::StorageKey s3(s1);
    ASSERT_EQ(true, s3.isNull());

    PV("Create StorageKey s4 and s5 from valid hex");
    const char          k_VALID_HEX[] = "ABCDEF1234";
    const unsigned char k_VALID_BIN[] = {0xAB, 0xCD, 0xEF, 0x12, 0x34};
    mqbu::StorageKey    s4;
    s4.fromHex(k_VALID_HEX);
    ASSERT_EQ(false, s4.isNull());

    mqbu::StorageKey s5(mqbu::StorageKey::HexRepresentation(), k_VALID_HEX);
    ASSERT_EQ(false, s5.isNull());
    ASSERT_EQ(0,
              bsl::memcmp(s4.data(),
                          s5.data(),
                          mqbu::StorageKey::e_KEY_LENGTH_BINARY));

    PV("Create StorageKey s6 and s7 from valid binary");
    const char       k_VALID_BINARY[] = "ABCDE";
    mqbu::StorageKey s6;
    s6.fromBinary(k_VALID_BINARY);
    ASSERT_EQ(false, s6.isNull());
    ASSERT_EQ(0,
              bsl::memcmp(k_VALID_BINARY,
                          s6.data(),
                          mqbu::StorageKey::e_KEY_LENGTH_BINARY));
    ASSERT_EQ(s6, reinterpret_cast<const mqbu::StorageKey&>(k_VALID_BINARY));

    mqbu::StorageKey s7(mqbu::StorageKey::BinaryRepresentation(),
                        k_VALID_BINARY);
    ASSERT_EQ(false, s7.isNull());
    ASSERT_EQ(0,
              bsl::memcmp(s6.data(),
                          s7.data(),
                          mqbu::StorageKey::e_KEY_LENGTH_BINARY));

    PV("Create StorageKey from integer");
    mqbu::StorageKey s8(0u);
    ASSERT_EQ(false, s8.isNull());

    mqbu::StorageKey s9(0u);
    ASSERT_EQ(false, s9.isNull());

    ASSERT_EQ(true, s8 == s9);
    ASSERT_EQ(false, s8 != s9);

    mqbu::StorageKey s10(bsl::numeric_limits<unsigned int>::max());
    ASSERT_EQ(false, s10.isNull());

    mqbu::StorageKey s11(bsl::numeric_limits<unsigned int>::max());
    ASSERT_EQ(false, s11.isNull());

    ASSERT_EQ(true, s10 == s11);
    ASSERT_EQ(false, s10 != s11);

    PV("Checking accessors");
    char s[mqbu::StorageKey::e_KEY_LENGTH_HEX];
    s5.loadHex(s);
    ASSERT_EQ(0, memcmp(k_VALID_HEX, s, mqbu::StorageKey::e_KEY_LENGTH_HEX));

    bsl::vector<char> binBuf(s_allocator_p);
    s5.loadBinary(&binBuf);
    ASSERT_EQ(0,
              memcmp(k_VALID_BIN,
                     &binBuf[0],
                     mqbu::StorageKey::e_KEY_LENGTH_BINARY));

    PV("Checking overloaded == and != operators");
    ASSERT_EQ(false, s4 == s6);
    ASSERT_EQ(true, s4 == s5);
    ASSERT_EQ(false, s4 != s5);
    ASSERT_EQ(true, s4 != s6);

    PV("Checking default hashing and less than operator");
    bsl::map<mqbu::StorageKey, int> storageMap(s_allocator_p);
    storageMap.insert(bsl::make_pair(s2, 1));
    storageMap.insert(bsl::make_pair(s4, 2));
    storageMap.insert(bsl::make_pair(s6, 3));
    ASSERT_EQ(false, storageMap.insert(bsl::make_pair(s2, 1)).second);
    bsl::map<mqbu::StorageKey, int>::const_iterator it = storageMap.find(s4);
    ASSERT_EQ(true, storageMap.end() != it);
    ASSERT_EQ(s4, it->first);
    ASSERT_EQ(2, it->second);

    bsl::unordered_map<mqbu::StorageKey, int> unorderedStorageMap(
        s_allocator_p);
    unorderedStorageMap.insert(bsl::make_pair(s2, 1));
    unorderedStorageMap.insert(bsl::make_pair(s4, 2));
    unorderedStorageMap.insert(bsl::make_pair(s6, 3));
    ASSERT_EQ(false, unorderedStorageMap.insert(bsl::make_pair(s2, 1)).second);
    bsl::unordered_map<mqbu::StorageKey, int>::const_iterator uit =
        unorderedStorageMap.find(s4);
    ASSERT_EQ(s4, uit->first);
    ASSERT_EQ(2, uit->second);

    s4.reset();
    ASSERT_EQ(true, s4.isNull());

    PV("Ensure that unordered map compiles when custom hash algo is "
       "specified");
    bsl::unordered_map<mqbu::StorageKey,
                       int,
                       bslh::Hash<mqbu::StorageKeyHashAlgo> >
        myMap(s_allocator_p);

    myMap.insert(bsl::make_pair(s6, 2));

    ASSERT_EQ(1u, myMap.count(s6));
}

void test2_streamout()
{
    mwctst::TestHelper::printTestName("STREAM OUT");

    // Create StorageKey from valid hex rep
    const char k_HEX[] = "0123456789";

    mqbu::StorageKey s1(mqbu::StorageKey::HexRepresentation(), k_HEX);

    mwcu::MemOutStream osstr(s_allocator_p);
    osstr << s1;

    bsl::string storageKeyStr(k_HEX, s_allocator_p);

    ASSERT_EQ(storageKeyStr, osstr.str());

    PV("StorageKey [" << osstr.str() << "]");
}

void test3_defaultHashUniqueness()
// ------------------------------------------------------------------------
// DEFAULT HASH UNIQUENESS
//
// Concerns:
//   Verify the uniqueness of the hash of a StorageKey using default hash
//   algo.
//
// Plan:
//   - Generate a lots of StorageKeys, compute their hash, and measure some
//     collisions statistics.
//
// Testing:
//   Hash uniqueness of the generated StorageKeys.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("DEFAULT HASH UNIQUENESS");

    s_ignoreCheckDefAlloc = true;
    // Because there is no emplace on unordered_map, the temporary list
    // created upon insertion of objects in the map uses the default
    // allocator.

    enum {
        k_NUM_ELEMS = 10000  // 10K
    };

    typedef bsl::unordered_set<mqbu::StorageKey>::const_iterator CITER;
    typedef bsl::vector<mqbu::StorageKey>                        StorageKeys;

    // hash -> vector of corresponding StorageKeys
    bsl::unordered_map<size_t, StorageKeys> hashes(s_allocator_p);
    bsl::unordered_set<mqbu::StorageKey>    keySet(s_allocator_p);

    hashes.reserve(k_NUM_ELEMS);
    generateStorageKeys(&keySet, k_NUM_ELEMS);  // k_NUM_ELEMS in keySet

    bsl::hash<mqbu::StorageKey> hasher;
    size_t                      maxCollisionsHash = 0;
    size_t                      maxCollisions     = 0;

    for (CITER citer = keySet.cbegin(); citer != keySet.cend(); ++citer) {
        const mqbu::StorageKey& currKey = *citer;

        size_t hash = hasher(currKey);

        StorageKeys& keysByHash = hashes[hash];
        keysByHash.push_back(currKey);
        if (maxCollisions < keysByHash.size()) {
            maxCollisions     = keysByHash.size();
            maxCollisionsHash = hash;
        }
    }

    // Update this comment

    // Above value is just chosen after looking at the number of collisions
    // by running this test case manually.  In most runs, number of
    // collisions was in the range of [1, 1].
    const size_t k_MAX_EXPECTED_COLLISIONS = 2;

    ASSERT_LT(maxCollisions, k_MAX_EXPECTED_COLLISIONS);

    if (true || (maxCollisions >= k_MAX_EXPECTED_COLLISIONS)) {
        cout << "Number of collisions...............: "
             << k_NUM_ELEMS - hashes.size() << endl
             << "Hash collision percentage..........: "
             << 100 - 100.0f * (hashes.size() / k_NUM_ELEMS) << "%" << endl
             << "Max collisions.....................: " << maxCollisions
             << endl
             << "Hash...............................: " << maxCollisionsHash
             << endl
             << "Num StorageKeys with that hash...........: "
             << hashes[maxCollisionsHash].size() << endl
             << "StorageKeys with the highest collisions..: " << endl;

        StorageKeys& keys = hashes[maxCollisionsHash];
        for (size_t i = 0; i < keys.size(); ++i) {
            cout << "  ";
            keys[i].print(cout);
            cout << endl;
        }
    }
}

void test4_customHashUniqueness()
// ------------------------------------------------------------------------
// DEFAULT HASH UNIQUENESS
//
// Concerns:
//   Verify the uniqueness of the hash of a StorageKey using custom hash
//   algo.
//
// Plan:
//   - Generate a lots of StorageKeys, compute their hash, and measure some
//     collisions statistics.
//
// Testing:
//   Hash uniqueness of the generated StorageKeys.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("CUSTOM HASH UNIQUENESS");

    s_ignoreCheckDefAlloc = true;
    // Because there is no emplace on unordered_map, the temporary list
    // created upon insertion of objects in the map uses the default
    // allocator.

    BSLS_KEYWORD_CONSTEXPR bsl::size_t k_NUM_ELEMS = 10000;  // 10K

    typedef bsl::unordered_set<mqbu::StorageKey>::const_iterator CITER;
    typedef bsl::vector<mqbu::StorageKey>                        StorageKeys;

    // hash -> vector of corresponding StorageKeys
    bsl::unordered_map<size_t, StorageKeys> hashes(s_allocator_p);
    bsl::unordered_set<mqbu::StorageKey>    keySet(s_allocator_p);

    hashes.reserve(k_NUM_ELEMS);
    generateStorageKeys(&keySet, k_NUM_ELEMS);  // k_NUM_ELEMS in keySet

    bslh::Hash<mqbu::StorageKeyHashAlgo> hasher;
    size_t                               maxCollisionsHash = 0;
    size_t                               maxCollisions     = 0;

    // Generate hashes and keep track of recurring hashes
    for (CITER citer = keySet.cbegin(); citer != keySet.cend(); ++citer) {
        const mqbu::StorageKey& currKey = *citer;

        size_t hash = hasher(currKey);

        StorageKeys& keysByHash = hashes[hash];
        keysByHash.push_back(currKey);
        if (maxCollisions < keysByHash.size()) {
            maxCollisions     = keysByHash.size();
            maxCollisionsHash = hash;
        }
    }

    // Update this comment

    // Above value is just chosen after looking at the number of collisions
    // by running this test case manually.  In most runs, number of
    // collisions was in the range of [1, 2].
    const size_t k_MAX_EXPECTED_COLLISIONS = 3;

    ASSERT_LT(maxCollisions, k_MAX_EXPECTED_COLLISIONS);

    if (true || (maxCollisions >= k_MAX_EXPECTED_COLLISIONS)) {
        cout << "Number of collisions...............: "
             << k_NUM_ELEMS - hashes.size() << endl
             << "Hash collision percentage..........: "
             << 100 - 100.0f * hashes.size() / k_NUM_ELEMS << "%" << endl
             << "Max collisions.....................: " << maxCollisions
             << endl
             << "Hash...............................: " << maxCollisionsHash
             << endl
             << "Num StorageKeys with that hash...........: "
             << hashes[maxCollisionsHash].size() << endl
             << "StorageKeys with the highest collisions..: " << endl;

        StorageKeys& keys = hashes[maxCollisionsHash];
        for (size_t i = 0; i < keys.size(); ++i) {
            cout << "  ";
            keys[i].print(cout);
            cout << endl;
        }
    }
}

// ============================================================================
//                              PERFORMANCE TESTS
// ----------------------------------------------------------------------------

BSLA_MAYBE_UNUSED
void testN1_defaultHashBenchmark()
// ------------------------------------------------------------------------
// DEFAULT HASH BENCHMARK
//
// Concerns:
//   Benchmark hashing function of a StorageKey using default hashing algo.
//
// Plan:
//   - Generate hash of a StorageKey in a timed loop.
//
// Testing:
//   NA
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("DEFAULT HASH BENCHMARK");

    const size_t                k_NUM_ITERATIONS = 10000000;  // 10M
    bsl::hash<mqbu::StorageKey> hasher;  // same as: bslh::Hash<> hasher;
    mqbu::StorageKey            key;

    // Initialize a valid storage key
    const char k_VALID_HEX[] = "ABCDEF1234";
    key.fromHex(k_VALID_HEX);

    // <time>
    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
        hasher(key);
    }
    bsls::Types::Int64 end = bsls::TimeUtil::getTimer();
    // </time>

    cout << "Calculated " << k_NUM_ITERATIONS << " default hashes of the"
         << " StorageKey in "
         << mwcu::PrintUtil::prettyTimeInterval(end - begin) << ".\n"
         << "Above implies that 1 hash of the StorageKey was calculated in "
         << (end - begin) / k_NUM_ITERATIONS << " nano seconds.\n"
         << "In other words: "
         << mwcu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                (k_NUM_ITERATIONS * 1000000000) / (end - begin)))
         << " hashes per second." << endl;
}

BSLA_MAYBE_UNUSED
void testN2_customHashBenchmark()
// ------------------------------------------------------------------------
// CUSTOM HASH BENCHMARK
//
// Concerns:
//   Benchmark hashing function of a StorageKey using custom hashing algo.
//
// Plan:
//   - Generate hash of a StorageKey in a timed loop.
//
// Testing:
//   NA
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("CUSTOM HASH BENCHMARK");

    const size_t                         k_NUM_ITERATIONS = 10000000;  // 10M
    bslh::Hash<mqbu::StorageKeyHashAlgo> hasher;
    mqbu::StorageKey                     key;

    // Initialize a valid storage key
    const char k_VALID_HEX[] = "ABCDEF1234";
    key.fromHex(k_VALID_HEX);

    // <time>
    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
        hasher(key);
    }
    bsls::Types::Int64 end = bsls::TimeUtil::getTimer();
    // </time>

    cout << "Calculated " << k_NUM_ITERATIONS << " custom hashes of the"
         << " StorageKey in "
         << mwcu::PrintUtil::prettyTimeInterval(end - begin) << ".\n"
         << "Above implies that 1 hash of the StorageKey was calculated in "
         << (end - begin) / k_NUM_ITERATIONS << " nano seconds.\n"
         << "In other words: "
         << mwcu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                (k_NUM_ITERATIONS * 1000000000) / (end - begin)))
         << " hashes per second." << endl;
}

BSLA_MAYBE_UNUSED
void testN3_hashTableWithDefaultHashBenchmark()
// ------------------------------------------------------------------------
// HASH TABLE w/ DEFAULT HASH BENCHMARK
//
// Concerns:
//   Benchmark insert() in a hashtable(KEY=mqbu::StorageKey) with default
//   hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("HASH TABLE w/ DEFAULT HASH BENCHMARK");

    typedef bsl::unordered_set<mqbu::StorageKey>::const_iterator CITER;

    const size_t                                 k_NUM_ELEMS = 10000;  // 10K
    bsl::unordered_set<mqbu::StorageKey>         keySet(s_allocator_p);
    bsl::unordered_map<mqbu::StorageKey, size_t> ht(16843, s_allocator_p);
    ht.reserve(k_NUM_ELEMS);

    generateStorageKeys(&keySet, k_NUM_ELEMS);  // k_NUM_ELEMS in keySet

    int i = 1;

    // <time>
    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (CITER citer = keySet.cbegin(); citer != keySet.cend(); ++citer) {
        ht.insert(bsl::make_pair(*citer, i++));
    }
    bsls::Types::Int64 end = bsls::TimeUtil::getTimer();
    // </time>

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

BSLA_MAYBE_UNUSED
void testN4_hashTableWithCustomHashBenchmark()
// ------------------------------------------------------------------------
// HASH TABLE w/ CUSTOM HASH BENCHMARK
//
// Concerns:
//   Benchmark insert() in a hashtable(KEY=mqbu::StorageKey) with custom
//   hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("HASH TABLE w/ CUSTOM HASH BENCHMARK");

    typedef bsl::unordered_set<mqbu::StorageKey>::const_iterator CITER;

    const size_t                         k_NUM_ELEMS = 10000;  // 10K
    bsl::unordered_set<mqbu::StorageKey> keySet(s_allocator_p);
    bsl::unordered_map<mqbu::StorageKey,
                       size_t,
                       bslh::Hash<mqbu::StorageKeyHashAlgo> >
        ht(16843, s_allocator_p);
    ht.reserve(k_NUM_ELEMS);

    generateStorageKeys(&keySet, k_NUM_ELEMS);

    int i = 1;

    // <time>
    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (CITER citer = keySet.cbegin(); citer != keySet.cend(); ++citer) {
        ht.insert(bsl::make_pair(*citer, i++));
    }
    bsls::Types::Int64 end = bsls::TimeUtil::getTimer();
    // </time>

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

BSLA_MAYBE_UNUSED
void testN5_orderedMapWithDefaultHashBenchmark()
// ------------------------------------------------------------------------
// ORDERED HASH MAP w/ DEFAULT HASH BENCHMARK
//
// Concerns:
//   Benchmark insert() in an orderedMap(KEY=mqbu::StorageKey) with
//   default hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ORDERED MAP DEFAULT HASH BENCHMARK");

    typedef bsl::unordered_set<mqbu::StorageKey>::const_iterator CITER;

    const size_t                         k_NUM_ELEMS = 10000;  // 10K
    bsl::unordered_set<mqbu::StorageKey> keySet(s_allocator_p);

    generateStorageKeys(&keySet, k_NUM_ELEMS);

    mwcc::OrderedHashMap<mqbu::StorageKey, size_t> ht(16843, s_allocator_p);

    int i = 1;
    // Warmup
    for (CITER citer = keySet.cbegin(); i <= 1000 && citer != keySet.cend();
         ++citer) {
        ht.insert(bsl::make_pair(*citer, i++));
    }

    ht.clear();
    i = 1;

    // <time>
    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (CITER citer = keySet.cbegin(); citer != keySet.cend(); ++citer) {
        ht.insert(bsl::make_pair(*citer, i++));
    }
    bsls::Types::Int64 end = bsls::TimeUtil::getTimer();
    // </time>

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
void testN6_orderedMapWithCustomHashBenchmark()
// ------------------------------------------------------------------------
// ORDERED HASH MAP w/ DEFAULT HASH BENCHMARK
//
// Concerns:
//   Benchmark insert() in an orderedMap(KEY=mqbu::StorageKey) using a
//   custom hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ORDERED MAP Custom HASH BENCHMARK");

    typedef bsl::unordered_set<mqbu::StorageKey>::const_iterator CITER;

    const size_t                         k_NUM_ELEMS = 10000;  // 10K
    bsl::unordered_set<mqbu::StorageKey> keySet(s_allocator_p);

    generateStorageKeys(&keySet, k_NUM_ELEMS);

    mwcc::OrderedHashMap<mqbu::StorageKey,
                         size_t,
                         bslh::Hash<mqbu::StorageKeyHashAlgo> >
        ht(16843, s_allocator_p);

    int i = 1;
    // Warmup
    for (CITER citer = keySet.cbegin(); i <= 1000 && citer != keySet.cend();
         ++citer) {
        ht.insert(bsl::make_pair(*citer, i++));
    }

    ht.clear();
    i = 1;

    // <time>
    bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (CITER citer = keySet.cbegin(); citer != keySet.cend(); ++citer) {
        ht.insert(bsl::make_pair(*citer, i++));
    }
    bsls::Types::Int64 end = bsls::TimeUtil::getTimer();
    // </time>

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

// Begin Google Benchmark Tests
#ifdef BSLS_PLATFORM_OS_LINUX
static void
testN1_defaultHashBenchmark_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// DEFAULT HASH BENCHMARK
//
// Concerns:
//   Benchmark hashing function of a StorageKey using default hashing algo.
//
// Plan:
//   - Generate hash of a StorageKey in a timed loop.
//
// Testing:
//   NA
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("DEFAULT HASH BENCHMARK");

    const size_t                k_NUM_ITERATIONS = state.range(0);  // 10M
    bsl::hash<mqbu::StorageKey> hasher;  // same as: bslh::Hash<> hasher;
    mqbu::StorageKey            key;

    // Initialize a valid storage key
    const char k_VALID_HEX[] = "ABCDEF1234";
    key.fromHex(k_VALID_HEX);

    for (auto _ : state) {
        for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
            hasher(key);
        }
    }
}

static void testN2_customHashBenchmark_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// CUSTOM HASH BENCHMARK
//
// Concerns:
//   Benchmark hashing function of a StorageKey using custom hashing algo.
//
// Plan:
//   - Generate hash of a StorageKey in a timed loop.
//
// Testing:
//   NA
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("CUSTOM HASH BENCHMARK");

    const size_t                         k_NUM_ITERATIONS = state.range(0);
    bslh::Hash<mqbu::StorageKeyHashAlgo> hasher;
    mqbu::StorageKey                     key;

    // Initialize a valid storage key
    const char k_VALID_HEX[] = "ABCDEF1234";
    key.fromHex(k_VALID_HEX);

    // <time>
    for (auto _ : state) {
        for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
            hasher(key);
        }
    }
    // </time>
}

static void testN3_hashTableWithDefaultHashBenchmark_GoogleBenchmark(
    benchmark::State& state)
// ------------------------------------------------------------------------
// HASH TABLE w/ DEFAULT HASH BENCHMARK
//
// Concerns:
//   Benchmark insert() in a hashtable(KEY=mqbu::StorageKey) with default
//   hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("HASH TABLE w/ DEFAULT HASH BENCHMARK");

    typedef bsl::unordered_set<mqbu::StorageKey>::const_iterator CITER;

    const size_t                                 k_NUM_ELEMS = state.range(0);
    bsl::unordered_set<mqbu::StorageKey>         keySet(s_allocator_p);
    bsl::unordered_map<mqbu::StorageKey, size_t> ht(16843, s_allocator_p);
    ht.reserve(k_NUM_ELEMS);

    generateStorageKeys(&keySet, k_NUM_ELEMS);  // k_NUM_ELEMS in keySet

    int i = 1;

    // <time>
    for (auto _ : state) {
        for (CITER citer = keySet.cbegin(); citer != keySet.cend(); ++citer) {
            ht.insert(bsl::make_pair(*citer, i++));
        }
    }
    // </time>
}

static void testN4_hashTableWithCustomHashBenchmark_GoogleBenchmark(
    benchmark::State& state)
// ------------------------------------------------------------------------
// HASH TABLE w/ CUSTOM HASH BENCHMARK
//
// Concerns:
//   Benchmark insert() in a hashtable(KEY=mqbu::StorageKey) with custom
//   hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("HASH TABLE w/ CUSTOM HASH BENCHMARK");

    typedef bsl::unordered_set<mqbu::StorageKey>::const_iterator CITER;

    const size_t                         k_NUM_ELEMS = state.range(0);
    bsl::unordered_set<mqbu::StorageKey> keySet(s_allocator_p);
    bsl::unordered_map<mqbu::StorageKey,
                       size_t,
                       bslh::Hash<mqbu::StorageKeyHashAlgo> >
        ht(16843, s_allocator_p);
    ht.reserve(k_NUM_ELEMS);

    generateStorageKeys(&keySet, k_NUM_ELEMS);

    int i = 1;

    // <time>
    for (auto _ : state) {
        for (CITER citer = keySet.cbegin(); citer != keySet.cend(); ++citer) {
            ht.insert(bsl::make_pair(*citer, i++));
        }
    }
}

static void testN5_orderedMapWithDefaultHashBenchmark_GoogleBenchmark(
    benchmark::State& state)
// ------------------------------------------------------------------------
// ORDERED HASH MAP w/ DEFAULT HASH BENCHMARK
//
// Concerns:
//   Benchmark insert() in an orderedMap(KEY=mqbu::StorageKey) with
//   default hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ORDERED MAP DEFAULT HASH BENCHMARK");

    typedef bsl::unordered_set<mqbu::StorageKey>::const_iterator CITER;

    const size_t                         k_NUM_ELEMS = state.range(0);
    bsl::unordered_set<mqbu::StorageKey> keySet(s_allocator_p);

    generateStorageKeys(&keySet, k_NUM_ELEMS);

    mwcc::OrderedHashMap<mqbu::StorageKey, size_t> ht(16843, s_allocator_p);

    int i = 1;
    // Warmup
    for (CITER citer = keySet.cbegin(); i <= 1000 && citer != keySet.cend();
         ++citer) {
        ht.insert(bsl::make_pair(*citer, i++));
    }

    ht.clear();
    i = 1;

    // <time>
    for (auto _ : state) {
        for (CITER citer = keySet.cbegin(); citer != keySet.cend(); ++citer) {
            ht.insert(bsl::make_pair(*citer, i++));
        }
    }
    // </time>
}
static void testN6_orderedMapWithCustomHashBenchmark_GoogleBenchmark(
    benchmark::State& state)
// ------------------------------------------------------------------------
// ORDERED HASH MAP w/ DEFAULT HASH BENCHMARK
//
// Concerns:
//   Benchmark insert() in an orderedMap(KEY=mqbu::StorageKey) using a
//   custom hash function.
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ORDERED MAP Custom HASH BENCHMARK");

    typedef bsl::unordered_set<mqbu::StorageKey>::const_iterator CITER;

    const size_t                         k_NUM_ELEMS = state.range(0);
    bsl::unordered_set<mqbu::StorageKey> keySet(s_allocator_p);

    generateStorageKeys(&keySet, k_NUM_ELEMS);

    mwcc::OrderedHashMap<mqbu::StorageKey,
                         size_t,
                         bslh::Hash<mqbu::StorageKeyHashAlgo> >
        ht(16843, s_allocator_p);

    int i = 1;
    // Warmup
    for (CITER citer = keySet.cbegin(); i <= 1000 && citer != keySet.cend();
         ++citer) {
        ht.insert(bsl::make_pair(*citer, i++));
    }

    ht.clear();
    i = 1;

    // <time>
    for (auto _ : state) {
        for (CITER citer = keySet.cbegin(); citer != keySet.cend(); ++citer) {
            ht.insert(bsl::make_pair(*citer, i++));
        }
    }
    // </time>
}

#endif  // BSLS_PLATFORM_OS_LINUX

}  // close unnamed namespace

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    bsls::TimeUtil::initialize();

    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 4: test4_customHashUniqueness(); break;
    case 3: test3_defaultHashUniqueness(); break;
    case 2: test2_streamout(); break;
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
        MWC_BENCHMARK_WITH_ARGS(testN3_hashTableWithDefaultHashBenchmark,
                                RangeMultiplier(10)->Range(10, 10000)->Unit(
                                    benchmark::kMillisecond));
        break;
    case -4:
        MWC_BENCHMARK_WITH_ARGS(testN4_hashTableWithCustomHashBenchmark,
                                RangeMultiplier(10)->Range(10, 10000)->Unit(
                                    benchmark::kMillisecond));
        break;
    case -5:
        MWC_BENCHMARK_WITH_ARGS(testN5_orderedMapWithDefaultHashBenchmark,
                                RangeMultiplier(10)->Range(10, 10000)->Unit(
                                    benchmark::kMillisecond));
        break;
    case -6:
        MWC_BENCHMARK_WITH_ARGS(testN6_orderedMapWithCustomHashBenchmark,
                                RangeMultiplier(10)->Range(10, 10000)->Unit(
                                    benchmark::kMillisecond));
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

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_GBL_ALLOC);
}

// ----------------------------------------------------------------------------
