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

#include <bmqc_orderedhashmap.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

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
#include <bmqtst_testhelper.h>

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

/// This class provides a legacy custom implementation of hashing algorithm for
/// `bmqt::MessageGUID`.  The implementation uses the unrolled djb2 hash, that
/// was faster than the default hashing algorithm that comes with `bslh`
/// package, and was used from 2016 to 2024.  There are a few problems that
/// were exposed later, leading to the change of the used hash function:
/// - Strong data dependency across 16 lines of code prevents CPU optimization.
/// - Computing the hash by one byte is slow compared to computing it by blocks
/// of 8 bytes, which is possible since we have the prior information that the
/// `data` array has fixed size 16.
/// - The hash distribution is easy to collide, we have a test which
/// reproducibly shows these collisions.
/// - It's easy to introduce a `data` generator that generates different arrays
/// with the same hash.
class LegacyHash {
  private:
    // DATA
    bsls::Types::Uint64 d_result;

  public:
    // TYPES
    typedef bsls::Types::Uint64 result_type;

    /// Constructor
    LegacyHash()
    : d_result(0)
    {
    }

    // MANIPULATORS
    /// Compute the unrolled djb2 hash on the specified `data`. The specified
    /// `numBytes` is not used.
    void operator()(const void* data, BSLS_ANNOTATION_UNUSED size_t numBytes)
    {
        d_result = 5381ULL;

        const char* start = reinterpret_cast<const char*>(data);
        d_result          = (d_result << 5) + d_result + start[0];
        d_result          = (d_result << 5) + d_result + start[1];
        d_result          = (d_result << 5) + d_result + start[2];
        d_result          = (d_result << 5) + d_result + start[3];
        d_result          = (d_result << 5) + d_result + start[4];
        d_result          = (d_result << 5) + d_result + start[5];
        d_result          = (d_result << 5) + d_result + start[6];
        d_result          = (d_result << 5) + d_result + start[7];
        d_result          = (d_result << 5) + d_result + start[8];
        d_result          = (d_result << 5) + d_result + start[9];
        d_result          = (d_result << 5) + d_result + start[10];
        d_result          = (d_result << 5) + d_result + start[11];
        d_result          = (d_result << 5) + d_result + start[12];
        d_result          = (d_result << 5) + d_result + start[13];
        d_result          = (d_result << 5) + d_result + start[14];
        d_result          = (d_result << 5) + d_result + start[15];
    }

    /// Return the computed hash.
    result_type computeHash() { return d_result; }
};

/// Hash with bad quality but with minimum number of operations.
/// Purely to compare other hashes with ideal performance scenario.
class BaselineHash {
  private:
    // DATA
    bsls::Types::Uint64 d_result;

  public:
    // TYPES
    typedef bsls::Types::Uint64 result_type;

    /// Constructor
    BaselineHash()
    : d_result(0)
    {
    }

    // MANIPULATORS
    /// Compute the trivial hash of the specified `data`. The specified
    /// `numBytes` is not used.
    void operator()(const void* data, BSLS_ANNOTATION_UNUSED size_t numBytes)
    {
        const bsls::Types::Uint64* start =
            reinterpret_cast<const bsls::Types::Uint64*>(data);

        // Touch all the bytes in the data
        d_result = start[0] ^ start[1];
    }

    /// Return the computed hash.
    result_type computeHash() { return d_result; }
};

class Mx3Hash {
  private:
    // DATA
    bsls::Types::Uint64 d_result;

  public:
    // TYPES
    typedef bsls::Types::Uint64 result_type;

    /// Constructor
    Mx3Hash()
    : d_result(0)
    {
    }

    // MANIPULATORS
    /// Compute the hash of the specified `data`. The specified `numBytes` is
    /// not used.
    void operator()(const void* data, BSLS_ANNOTATION_UNUSED size_t numBytes)
    {
        const bsls::Types::Uint64* start =
            reinterpret_cast<const bsls::Types::Uint64*>(data);

        struct LocalFuncs {
            inline static bsls::Types::Uint64 mix(bsls::Types::Uint64 x)
            {
                x ^= x >> 32;
                x *= 0xbea225f9eb34556d;
                x ^= x >> 29;
                x *= 0xbea225f9eb34556d;
                x ^= x >> 32;
                x *= 0xbea225f9eb34556d;
                x ^= x >> 29;
                return x;
            }

            inline static bsls::Types::Uint64 combine(bsls::Types::Uint64 lhs,
                                                      bsls::Types::Uint64 rhs)
            {
                lhs ^= rhs + 0x517cc1b727220a95 + (lhs << 6) + (lhs >> 2);
                return lhs;
            }
        };

        d_result = LocalFuncs::combine(LocalFuncs::mix(start[0]),
                                       LocalFuncs::mix(start[1]));
    }

    /// Return the computed hash.
    result_type computeHash() { return d_result; }
};

/// Data struct holding the results of a benchmark
struct HashBenchmarkStats {
    /// Name of the current case
    bsl::string d_caseName;

    /// The total number of hash evaluations
    size_t d_numIterations;

    /// The time (in nanoseconds) passed for all hash evaluations
    bsls::Types::Int64 d_timeDeltaNs;

    /// The average time (in nanoseconds) to compute one hash
    bsls::Types::Int64 d_timePerHashNs;

    /// The estimated hash computation rate (per second)
    bsls::Types::Int64 d_hashesPerSecond;
};

/// The class for aggregation and pretty printing simple stats.
class Table {
  public:
    // FORWARD DECLARATIONS
    class ColumnView;
    friend class ColumnView;

  private:
    // DATA
    /// 2-dimensional table with values presented as `bsl::string`,
    /// the column index is first, the row index is the second.
    /// The 0-th row contains column labels.
    bsl::vector<bsl::vector<bsl::string> > d_columns;

    /// The mapping between column title and ColumnView-s
    bsl::map<bsl::string, ColumnView> d_views;

    // CLASS METHODS
    static bsl::string pad(const bsl::string& text, size_t width)
    {
        BSLS_ASSERT(text.length() <= width);
        return bsl::string(width - text.length(), ' ') + text;
    }

  public:
    // PUBLIC TYPES
    class ColumnView {
      private:
        // PRIVATE TYPES

        /// Reference to the parent table
        Table& d_table;

        /// The index of this column in the parent table
        size_t d_columnIndex;

      public:
        // CREATORS
        explicit ColumnView(Table& table, size_t columnIndex)
        : d_table(table)
        , d_columnIndex(columnIndex)
        {
            // NOTHING
        }

        // MANIPULATORS

        /// Insert the specified 'value' to the end of the column.
        void insertValue(const bsl::string& value)
        {
            d_table.d_columns.at(d_columnIndex).push_back(value);
        }

        /// Insert the specified 'value' to the end of the column.
        void insertValue(const bsls::Types::Uint64& value)
        {
            d_table.d_columns.at(d_columnIndex)
                .push_back(bsl::to_string(value));
        }
    };

    /// Return a `ColumnView` manipulator to the table column data for the
    /// specified `columnTitle`.
    ///
    /// Note: we return ColumnView by value, not by reference.  If we return
    ///       a reference to an object in the stored map, the reference can
    ///       become invalid if we continue to add new nodes to the map.
    ///
    /// Guarantee: each ColumnView returned before is valid as manipulator to
    ///            its column until the parent Table object is valid.
    ColumnView column(const bsl::string& columnTitle)
    {
        if (d_views.find(columnTitle) == d_views.end()) {
            d_columns.resize(d_columns.size() + 1);
            d_columns.back().push_back(columnTitle);
            d_views.insert(
                bsl::make_pair(columnTitle,
                               ColumnView(*this, d_columns.size() - 1)));
        }
        return d_views.find(columnTitle)->second;
    }

    ///  Print the stored data as a pretty table to the specified `os`.
    void print(bsl::ostream& os) const
    {
        // PRECONDITIONS
        if (d_columns.empty()) {
            return;  // RETURN
        }
        const size_t      rows      = d_columns.front().size();
        const bsl::string separator = " | ";
        for (size_t columnId = 0; columnId < d_columns.size(); columnId++) {
            // Expect all columns to have the same number of rows
            BSLS_ASSERT(rows == d_columns.at(columnId).size());
        }

        // For each column, calculate the longest stored value and remember it
        // as this column's width
        bsl::vector<size_t> paddings;
        paddings.resize(d_columns.size(), 0);
        for (size_t columnId = 0; columnId < d_columns.size(); columnId++) {
            const bsl::vector<bsl::string>& column = d_columns.at(columnId);

            size_t& maxLen = paddings.at(columnId);
            for (size_t rowId = 0; rowId < rows; rowId++) {
                maxLen = bsl::max(maxLen, column.at(rowId).length());
            }
        }

        // Print the table using precalculated column widths
        for (size_t rowId = 0; rowId < rows; rowId++) {
            for (size_t columnId = 0; columnId < d_columns.size();
                 columnId++) {
                if (columnId > 0) {
                    os << separator;
                }
                os << pad(d_columns.at(columnId).at(rowId),
                          paddings.at(columnId));
            }
            os << bsl::endl;

            // Print horizontal line after the initial row
            if (rowId == 0) {
                size_t lineWidth = 0;
                for (bsl::vector<size_t>::const_iterator it =
                         paddings.cbegin();
                     it != paddings.cend();
                     ++it) {
                    lineWidth += *it;
                }
                lineWidth += separator.size() * (paddings.size() - 1);
                os << bsl::string(lineWidth, '=') << bsl::endl;
            }
        }
    }
};

template <class HashType>
HashBenchmarkStats benchmarkHash(const bsl::string& name)
{
    const size_t               k_NUM_ITERATIONS = 100000000;  // 100M
    HashType                   hasher;
    bmqt::MessageGUID          guid;
    bmqp::MessageGUIDGenerator generator(0);

    generator.generateGUID(&guid);

    const bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
    for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(hasher(guid) == i)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            // To prevent this loop from being optimized out in Release
            bsl::cout << bsl::endl;
        }
    }
    const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

    HashBenchmarkStats stats;
    stats.d_caseName        = name;
    stats.d_numIterations   = k_NUM_ITERATIONS;
    stats.d_timeDeltaNs     = end - begin;
    stats.d_timePerHashNs   = (end - begin) / k_NUM_ITERATIONS;
    stats.d_hashesPerSecond = static_cast<bsls::Types::Int64>(
                                  k_NUM_ITERATIONS) *
                              bdlt::TimeUnitRatio::k_NS_PER_S / (end - begin);

    cout << "Calculated " << stats.d_numIterations << " <" << stats.d_caseName
         << "> hashes of the GUID in "
         << bmqu::PrintUtil::prettyTimeInterval(stats.d_timeDeltaNs) << ".\n"
         << "Above implies that 1 hash of the GUID was calculated in "
         << stats.d_timePerHashNs << " nano seconds.\n"
         << "In other words: "
         << bmqu::PrintUtil::prettyNumber(stats.d_hashesPerSecond)
         << " hashes per second." << endl;

    return stats;
}

template <class HashType>
static int calcCollisions(const bsl::vector<bmqt::MessageGUID>& guids)
{
    HashType hasher;

    bsl::vector<bsls::Types::Uint64> hashes;
    hashes.resize(guids.size());

    for (size_t i = 0; i < guids.size(); i++) {
        hashes[i] = hasher(guids[i]);
        bsl::string hex;
        hex.resize(32);
        guids[i].toHex(&hex[0]);
    }

    bsl::sort(hashes.begin(), hashes.end());
    int res = 0;
    for (size_t i = 0; i + 1 < hashes.size(); i++) {
        if (hashes[i] == hashes[i + 1]) {
            res++;
        }
    }
    return res;
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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bmqp::MessageGUIDGenerator generator(0);

    {
        // Create a GUID
        bmqt::MessageGUID guid;
        BMQTST_ASSERT_EQ(guid.isUnset(), true);
        generator.generateGUID(&guid);
        BMQTST_ASSERT_EQ(guid.isUnset(), false);

        // Export to binary representation
        unsigned char binaryBuffer[bmqt::MessageGUID::e_SIZE_BINARY];
        guid.toBinary(binaryBuffer);

        bmqt::MessageGUID fromBinGUID;
        fromBinGUID.fromBinary(binaryBuffer);
        BMQTST_ASSERT_EQ(fromBinGUID.isUnset(), false);
        BMQTST_ASSERT_EQ(fromBinGUID, guid);

        // Export to hex representation
        char hexBuffer[bmqt::MessageGUID::e_SIZE_HEX];
        guid.toHex(hexBuffer);
        BMQTST_ASSERT_EQ(
            true,
            bmqt::MessageGUID::isValidHexRepresentation(hexBuffer));

        bmqt::MessageGUID fromHexGUID;
        fromHexGUID.fromHex(hexBuffer);
        BMQTST_ASSERT_EQ(fromHexGUID.isUnset(), false);
        BMQTST_ASSERT_EQ(fromHexGUID, guid);
    }

    {
        // Create 2 GUIDs, confirm operator!=
        bmqt::MessageGUID guid1;
        generator.generateGUID(&guid1);

        bmqt::MessageGUID guid2;
        generator.generateGUID(&guid2);

        BMQTST_ASSERT_NE(guid1, guid2);
    }

    {
        // Ensure that unordered map compiles when custom hash algo is
        // specified.

        bsl::unordered_map<bmqt::MessageGUID,
                           int,
                           bslh::Hash<bmqt::MessageGUIDHashAlgo> >
                          myMap(bmqtst::TestHelperUtil::allocator());
        bmqt::MessageGUID guid;
        generator.generateGUID(&guid);
        myMap.insert(bsl::make_pair(guid, 1));

        BMQTST_ASSERT_EQ(1u, myMap.count(guid));
    }
}

static void test2_extract()
// ------------------------------------------------------------------------
// TBD:
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() =
        true;  // Implicit string conversion in BMQTST_ASSERT_EQ

    bmqtst::TestHelper::printTestName("EXTRACT");

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
        bsl::string        clientId(bmqtst::TestHelperUtil::allocator());

        int rc = bmqp::MessageGUIDGenerator::extractFields(&version,
                                                           &counter,
                                                           &timerTick,
                                                           &clientId,
                                                           guid);
        BMQTST_ASSERT_EQ(rc, 0);
        BMQTST_ASSERT_EQ(version, 1);
        BMQTST_ASSERT_EQ(counter, 0u);
        BMQTST_ASSERT_EQ(timerTick, 0);
        BMQTST_ASSERT_EQ(clientId, "000000000000");
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
        bsl::string        clientId(bmqtst::TestHelperUtil::allocator());

        int rc = bmqp::MessageGUIDGenerator::extractFields(&version,
                                                           &counter,
                                                           &timerTick,
                                                           &clientId,
                                                           guid);
        BMQTST_ASSERT_EQ(rc, 0);
        BMQTST_ASSERT_EQ(version, 1);
        BMQTST_ASSERT_EQ(counter, 4194303u);
        BMQTST_ASSERT_EQ(timerTick, 72057594037927935);
        BMQTST_ASSERT_EQ(clientId, "FFFFFFFFFFFF");
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
        bsl::string        clientId(bmqtst::TestHelperUtil::allocator());

        int rc = bmqp::MessageGUIDGenerator::extractFields(&version,
                                                           &counter,
                                                           &timerTick,
                                                           &clientId,
                                                           guid);
        BMQTST_ASSERT_EQ(rc, 0);
        BMQTST_ASSERT_EQ(version, 1);
        BMQTST_ASSERT_EQ(counter, 2964169u);
        BMQTST_ASSERT_EQ(timerTick, 297593876864458);
        BMQTST_ASSERT_EQ(clientId, "0DCE04742D2E");
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
    bmqtst::TestHelperUtil::ignoreCheckGblAlloc() = true;
    // Can't ensure no global memory is allocated because
    // 'bslmt::ThreadUtil::create()' uses the global allocator to allocate
    // memory.

    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName("MULTITHREAD");

    const int k_NUM_THREADS = 10;

#ifdef BSLS_PLATFORM_OS_SOLARIS
    // This test case times out if 'k_NUM_GUIDS' is close to 1 million
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
    const int k_NUM_GUIDS = 1000000;  // 1M
#endif

    bslmt::ThreadGroup threadGroup(bmqtst::TestHelperUtil::allocator());

    // Barrier to get each thread to start at the same time; `+1` for this
    // (main) thread.
    bslmt::Barrier barrier(k_NUM_THREADS + 1);

    bsl::vector<bsl::vector<bmqt::MessageGUID> > threadsData(
        bmqtst::TestHelperUtil::allocator());
    threadsData.resize(k_NUM_THREADS);

    bmqp::MessageGUIDGenerator generator(0);

    for (int i = 0; i < k_NUM_THREADS; ++i) {
        int rc = threadGroup.addThread(bdlf::BindUtil::bind(&threadFunction,
                                                            &threadsData[i],
                                                            &barrier,
                                                            &generator,
                                                            k_NUM_GUIDS));
        BMQTST_ASSERT_EQ_D(i, rc, 0);
    }

    barrier.wait();
    threadGroup.joinAll();

    // Check uniqueness across all threads
    bsl::set<bmqt::MessageGUID> allGUIDs(bmqtst::TestHelperUtil::allocator());
    for (int tIt = 0; tIt < k_NUM_THREADS; ++tIt) {
        const bsl::vector<bmqt::MessageGUID>& guids = threadsData[tIt];
        for (int gIt = 0; gIt < k_NUM_GUIDS; ++gIt) {
            BMQTST_ASSERT_EQ(allGUIDs.insert(guids[gIt]).second, true);
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
    bmqtst::TestHelperUtil::ignoreCheckGblAlloc() = true;
    // Can't ensure no global memory is allocated because
    // 'bslmt::ThreadUtil::create()' uses the global allocator to allocate
    // memory.

    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName("MULTITHREAD");

    const int k_NUM_THREADS = 10;

#if defined(__has_feature)
    // Avoid timeout under MemorySanitizer
    const int k_NUM_GUIDS = __has_feature(memory_sanitizer) ? 500000    // 500k
                                                            : 1000000;  // 1M
#elif defined(__SANITIZE_MEMORY__)
    // GCC-supported macros for checking MSAN
    const int k_NUM_GUIDS = 500000;  // 500k
#else
    const int k_NUM_GUIDS = 1000000;  // 1M
#endif

    bslmt::ThreadGroup threadGroup(bmqtst::TestHelperUtil::allocator());

    // Barrier to get each thread to start at the same time; `+1` for this
    // (main) thread.
    bslmt::Barrier barrier(k_NUM_THREADS + 1);

    bsl::vector<bsl::vector<bmqt::MessageGUID> > threadsData(
        bmqtst::TestHelperUtil::allocator());
    threadsData.resize(k_NUM_THREADS);

    // Create generator with 'doIpResolving' flag set to false
    bmqp::MessageGUIDGenerator generator(0, false);

    for (int i = 0; i < k_NUM_THREADS; ++i) {
        int rc = threadGroup.addThread(bdlf::BindUtil::bind(&threadFunction,
                                                            &threadsData[i],
                                                            &barrier,
                                                            &generator,
                                                            k_NUM_GUIDS));
        BMQTST_ASSERT_EQ_D(i, rc, 0);
    }

    barrier.wait();
    threadGroup.joinAll();

    // Check uniqueness across all threads
    bsl::set<bmqt::MessageGUID> allGUIDs(bmqtst::TestHelperUtil::allocator());
    for (int tIt = 0; tIt < k_NUM_THREADS; ++tIt) {
        const bsl::vector<bmqt::MessageGUID>& guids = threadsData[tIt];
        for (int gIt = 0; gIt < k_NUM_GUIDS; ++gIt) {
            BMQTST_ASSERT_EQ(allGUIDs.insert(guids[gIt]).second, true);
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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName("PRINT");

    bmqp::MessageGUIDGenerator generator(0);

    PV("Testing printing an unset GUID");
    {
        bmqt::MessageGUID guid;
        BMQTST_ASSERT_EQ(guid.isUnset(), true);

        bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
        bmqp::MessageGUIDGenerator::print(out, guid);
        BMQTST_ASSERT_EQ(out.str(), "** UNSET **");
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
        bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
        bmqp::MessageGUIDGenerator::print(out, guid);
        BMQTST_ASSERT_EQ(out.str(), k_EXPECTED);
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
        bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
        bmqp::MessageGUIDGenerator::print(out, guid);
        BMQTST_ASSERT_EQ(out.str(), k_EXPECTED);
    }

    PV("Verify clientId");
    {
        bmqt::MessageGUID guid;
        generator.generateGUID(&guid);

        bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
        bmqp::MessageGUIDGenerator::print(out, guid);

        PVV("Output: '" << out.str() << "',  GUID " << guid);
        // Extract the ClientId from the printed string, that is the last 12
        // characters.
        bslstl::StringRef printedClientId =
            bslstl::StringRef(&out.str()[out.length() - 12], 12);
        BMQTST_ASSERT_EQ(printedClientId, generator.clientIdHex());
    }

    PV("Test printing of a test-only GUID");
    {
        bmqt::MessageGUID guid1 = bmqp::MessageGUIDGenerator::testGUID();
        bmqt::MessageGUID guid2 = bmqp::MessageGUIDGenerator::testGUID();

        BMQTST_ASSERT(!guid1.isUnset());
        BMQTST_ASSERT(!guid2.isUnset());

        // Print and compare
        const char         k_EXPECTED_1[] = "1-0-0-000000000001";
        const char         k_EXPECTED_2[] = "1-0-0-000000000002";
        bmqu::MemOutStream out1(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream out2(bmqtst::TestHelperUtil::allocator());
        bmqp::MessageGUIDGenerator::print(out1, guid1);
        bmqp::MessageGUIDGenerator::print(out2, guid2);

        BMQTST_ASSERT_EQ(out1.str(), k_EXPECTED_1);
        BMQTST_ASSERT_EQ(out2.str(), k_EXPECTED_2);
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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Because there is no emplace on unordered_map, the temporary list
    // created upon insertion of objects in the map uses the default
    // allocator.

    bmqtst::TestHelper::printTestName("DEFAULT HASH UNIQUENESS");

#if defined(__has_feature)
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
    bsl::unordered_map<size_t, Guids> hashes(
        bmqtst::TestHelperUtil::allocator());
    hashes.reserve(k_NUM_GUIDS);

    bsl::hash<bmqt::MessageGUID> hasher;
    size_t                       maxDuplicatesHash = 0;
    size_t                       maxDuplicates     = 0;

    bmqp::MessageGUIDGenerator generator(0);

    // Generate GUIDs and update the 'hashes' map
    for (bsls::Types::Int64 i = 0; i < k_NUM_GUIDS; ++i) {
        bmqt::MessageGUID guid;
        generator.generateGUID(&guid);

        size_t hash = hasher(guid);

        Guids& guids = hashes[hash];
        guids.push_back(guid);
        if (maxDuplicates < guids.size()) {
            maxDuplicates     = guids.size();
            maxDuplicatesHash = hash;
        }
    }

    // With the custom bit mixer hash function no collisions were observed on
    // different randomized runs, so we expect it to be a very rare event.
    // In the unlikely event of this rare hash collision, we don't want to fail
    // this test, so we allow 1+1=2 max expected duplicates per a hash.
    const size_t k_MAX_EXPECTED_DUPLICATES = 2;

    BMQTST_ASSERT_LT(maxDuplicates, k_MAX_EXPECTED_DUPLICATES);

    if (maxDuplicates >= k_MAX_EXPECTED_DUPLICATES) {
        cout << "Hash duplicates percentage..........: "
             << 100 - 100.0f * hashes.size() / k_NUM_GUIDS << "%" << endl
             << "Max duplicates per hash............: " << maxDuplicates
             << endl
             << "Hash...............................: " << maxDuplicatesHash
             << endl
             << "Num GUIDs with that hash...........: "
             << hashes[maxDuplicatesHash].size() << endl
             << "GUIDs with the highest collisions..: " << endl;

        Guids& guids = hashes[maxDuplicatesHash];
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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Because there is no emplace on unordered_map, the temporary list
    // created upon insertion of objects in the map uses the default
    // allocator.

    bmqtst::TestHelper::printTestName("CUSTOM HASH UNIQUENESS");

#if defined(__has_feature)
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
    bsl::unordered_map<size_t, Guids> hashes(
        bmqtst::TestHelperUtil::allocator());

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

    BMQTST_ASSERT_LT(maxCollisions, k_MAX_EXPECTED_COLLISIONS);

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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() =
        true;  // istringstream allocates

    bmqtst::TestHelper::printTestName("DECODE");

    cout << "Please enter the hex representation of a GUID, followed by\n"
         << "<enter> when done (optionally, specify the nanoSecondsFromEpoch\n"
         << "to resolve the time):\n"
         << "  hexGUID [nanoSecondsFromEpoch]" << endl
         << endl;

    // Read from stdin
    char buffer[256];
    bsl::cin.getline(buffer, 256, '\n');

    bsl::istringstream is(buffer);

    bsl::string        hexGuid(bmqtst::TestHelperUtil::allocator());
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
    BMQTST_ASSERT_EQ(guid.isUnset(), false);

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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName("PERFORMANCE");

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
         << bmqu::PrintUtil::prettyTimeInterval(end - start) << ".\n"
         << "Above implies that 1 bmqt::MessageGUID was calculated in "
         << (end - start) / k_NUM_GUIDS << " nano seconds.\n"
         << "In other words: "
         << bmqu::PrintUtil::prettyNumber((k_NUM_GUIDS * 1000000000) /
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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName("PERFORMANCE");

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
         << bmqu::PrintUtil::prettyTimeInterval(end - start) << ".\n"
         << "Above implies that 1 bdlb::Guid was calculated in "
         << (end - start) / k_NUM_GUIDS << " nano seconds.\n"
         << "In other words: "
         << bmqu::PrintUtil::prettyNumber((k_NUM_GUIDS * 1000000000) /
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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName("DEFAULT HASH BENCHMARK");

    benchmarkHash<bsl::hash<bmqt::MessageGUID> >("default");
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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName("CUSTOM HASH BENCHMARK");
    benchmarkHash<bslh::Hash<bmqt::MessageGUIDHashAlgo> >("custom");
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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName("HASH TABLE w/ DEFAULT HASH BENCHMARK");

    const size_t      k_NUM_ELEMS = 10000000;  // 10M
    bmqt::MessageGUID guid;
    bsl::unordered_map<bmqt::MessageGUID, size_t> ht(
        bmqtst::TestHelperUtil::allocator());
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
         << bmqu::PrintUtil::prettyTimeInterval(end - begin) << ".\n"
         << "Above implies that 1 element was inserted in "
         << (end - begin) / k_NUM_ELEMS << " nano seconds.\n"
         << "In other words: "
         << bmqu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName("HASH TABLE w/ CUSTOM HASH BENCHMARK");

    const size_t      k_NUM_ELEMS = 10000000;  // 10M
    bmqt::MessageGUID guid;

    bsl::unordered_map<bmqt::MessageGUID,
                       size_t,
                       bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        ht(bmqtst::TestHelperUtil::allocator());
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
         << bmqu::PrintUtil::prettyTimeInterval(end - begin) << ".\n"
         << "Above implies that 1 element was inserted in "
         << (end - begin) / k_NUM_ELEMS << " nano seconds.\n"
         << "In other words: "
         << bmqu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName("ORDERED MAP DEFAULT HASH BENCHMARK");

    const size_t      k_NUM_ELEMS = 10000000;  // 10M
    bmqt::MessageGUID guid;

    bmqc::OrderedHashMap<bmqt::MessageGUID, size_t> ht(
        k_NUM_ELEMS,
        bmqtst::TestHelperUtil::allocator());

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
         << bmqu::PrintUtil::prettyTimeInterval(end - begin) << ".\n"
         << "Above implies that 1 element was inserted in "
         << (end - begin) / k_NUM_ELEMS << " nano seconds.\n"
         << "In other words: "
         << bmqu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName("ORDERED MAP CUSTOM HASH BENCHMARK");

    const size_t      k_NUM_ELEMS = 10000000;  // 10M
    bmqt::MessageGUID guid;

    bmqc::OrderedHashMap<bmqt::MessageGUID,
                         size_t,
                         bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        ht(k_NUM_ELEMS, bmqtst::TestHelperUtil::allocator());

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
         << bmqu::PrintUtil::prettyTimeInterval(end - begin) << ".\n"
         << "Above implies that 1 element was inserted in "
         << (end - begin) / k_NUM_ELEMS << " nano seconds.\n"
         << "In other words: "
         << bmqu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                (k_NUM_ELEMS * 1000000000) / (end - begin)))
         << " insertions per second." << endl;
}

BSLA_MAYBE_UNUSED
static void testN9_hashBenchmarkComparison()
// ------------------------------------------------------------------------
// HASH BENCHMARK COMPARISON
//
// Concerns:
//   Benchmark MessageGUID hashing functions and print the results table
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName("HASH BENCHMARK COMPARISON");

    bsl::vector<HashBenchmarkStats> stats;
    stats.push_back(benchmarkHash<bslh::Hash<BaselineHash> >("baseline"));
    stats.push_back(benchmarkHash<bsl::hash<bmqt::MessageGUID> >("default"));
    stats.push_back(benchmarkHash<bslh::Hash<LegacyHash> >("legacy(djb2)"));
    stats.push_back(benchmarkHash<bslh::Hash<Mx3Hash> >("mx3"));
    stats.push_back(
        benchmarkHash<bslh::Hash<bmqt::MessageGUIDHashAlgo> >("mxm"));

    Table table;
    for (size_t i = 0; i < stats.size(); i++) {
        const HashBenchmarkStats& st = stats[i];
        table.column("Name").insertValue(st.d_caseName);
        table.column("Iters").insertValue(st.d_numIterations);
        table.column("Total time (ns)").insertValue(st.d_timeDeltaNs);
        table.column("Per hash (ns)").insertValue(st.d_timePerHashNs);
        table.column("Hash rate (1/sec)").insertValue(st.d_hashesPerSecond);
    }
    table.print(bsl::cout);
}

BSLA_MAYBE_UNUSED
static void testN10_hashCollisionsComparison()
// ------------------------------------------------------------------------
// HASH COLLISIONS COMPARISON
//
// Concerns:
//   Compare hash collisions between different hash algos.
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Because there is no emplace on unordered_map, the temporary list
    // created upon insertion of objects in the map uses the default
    // allocator.

    bmqtst::TestHelper::printTestName("HASH COLLISIONS COMPARISON");

#ifdef BSLS_PLATFORM_OS_SOLARIS
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

    /// This structure holds local static MessageGUID generators
    /// with different distributions
    struct LocalFuncs {
        static void generateGUIDs_bmqpMessageGUIDGenerator1(
            bsl::vector<bmqt::MessageGUID>* guids,
            size_t                          num)
        {
            // PRECONDITIONS
            BSLS_ASSERT(guids);
            guids->resize(num);

            bmqp::MessageGUIDGenerator generator(0);
            for (size_t i = 0; i < num; i++) {
                generator.generateGUID(&guids->at(i));
            }
        }

        static void generateGUIDs_bmqpMessageGUIDGeneratorN(
            bsl::vector<bmqt::MessageGUID>* guids,
            size_t                          num)
        {
            // PRECONDITIONS
            BSLS_ASSERT(guids);
            guids->resize(num);

            static const int k_NUM_GENERATORS = 10;
            bsl::vector<bslma::ManagedPtr<bmqp::MessageGUIDGenerator> >
                generators;
            generators.resize(10);
            for (int i = 0; i < k_NUM_GENERATORS; i++) {
                generators[i].load(new (*bmqtst::TestHelperUtil::allocator())
                                       bmqp::MessageGUIDGenerator(i),
                                   bmqtst::TestHelperUtil::allocator());
            }

            bmqp::MessageGUIDGenerator generator(0);
            for (size_t i = 0; i < num; i++) {
                generators.at(i % k_NUM_GENERATORS)
                    ->generateGUID(&guids->at(i));
            }
        }

        static void generateGUIDs_rand(bsl::vector<bmqt::MessageGUID>* guids,
                                       size_t                          num)
        {
            // PRECONDITIONS
            BSLS_ASSERT(guids);
            guids->resize(num);

            unsigned char buff[bmqt::MessageGUID::e_SIZE_BINARY];

            for (size_t i = 0; i < num; i++) {
                for (size_t j = 0; j < bmqt::MessageGUID::e_SIZE_BINARY; j++) {
                    buff[j] = rand() % 256;
                }
                guids->at(i).fromBinary(buff);
            }
        }

        static void
        generateGUIDs_symmetry_4counters(bsl::vector<bmqt::MessageGUID>* guids,
                                         size_t                          num)
        {
            // PRECONDITIONS
            BSLS_ASSERT(guids);
            guids->resize(num);

            unsigned char  buff[bmqt::MessageGUID::e_SIZE_BINARY];
            bsl::uint32_t* ptr = reinterpret_cast<bsl::uint32_t*>(buff);
            BSLS_ASSERT(0 == bmqt::MessageGUID::e_SIZE_BINARY % 4);

            for (size_t i = 0; i < num; i++) {
                for (size_t j = 0; j * 4 < bmqt::MessageGUID::e_SIZE_BINARY;
                     j++) {
                    ptr[j] = i;
                }
                guids->at(i).fromBinary(buff);
            }
        }

        static void
        generateGUIDs_symmetry_4quarters(bsl::vector<bmqt::MessageGUID>* guids,
                                         size_t                          num)
        {
            // PRECONDITIONS
            BSLS_ASSERT(guids);
            guids->resize(num);

            unsigned char buff[bmqt::MessageGUID::e_SIZE_BINARY];
            int*          ptr = reinterpret_cast<int*>(buff);
            BSLS_ASSERT(0 == bmqt::MessageGUID::e_SIZE_BINARY % 4);

            for (size_t i = 0; i < num; i++) {
                int val = rand();
                for (size_t j = 0; j * 4 < bmqt::MessageGUID::e_SIZE_BINARY;
                     j++) {
                    ptr[j] = val;
                }
                guids->at(i).fromBinary(buff);
            }
        }

        static void
        generateGUIDs_symmetry_2halves(bsl::vector<bmqt::MessageGUID>* guids,
                                       size_t                          num)
        {
            // PRECONDITIONS
            BSLS_ASSERT(guids);
            guids->resize(num);

            unsigned char buff[bmqt::MessageGUID::e_SIZE_BINARY];
            BSLS_ASSERT(0 == bmqt::MessageGUID::e_SIZE_BINARY % 2);

            for (size_t i = 0; i < num; i++) {
                for (size_t j = 0; j * 2 < bmqt::MessageGUID::e_SIZE_BINARY;
                     j++) {
                    buff[j] = rand() % 256;
                }
                bsl::memcpy(&buff[bmqt::MessageGUID::e_SIZE_BINARY / 2],
                            buff,
                            bmqt::MessageGUID::e_SIZE_BINARY / 2);
                guids->at(i).fromBinary(buff);
            }
        }

        static void
        generateGUIDs_counter(bsl::vector<bmqt::MessageGUID>* guids,
                              size_t                          num)
        {
            // PRECONDITIONS
            BSLS_ASSERT(guids);
            guids->resize(num);

            unsigned char  buff[bmqt::MessageGUID::e_SIZE_BINARY];
            bsl::uint32_t* ptr = reinterpret_cast<bsl::uint32_t*>(buff);
            BSLS_ASSERT(0 == bmqt::MessageGUID::e_SIZE_BINARY % 4);

            for (size_t i = 0; i < num; i++) {
                for (size_t j = 1; j < bmqt::MessageGUID::e_SIZE_BINARY / 4;
                     j++) {
                    ptr[j] = 0;
                }
                ptr[0] = i;
                guids->at(i).fromBinary(buff);
            }
        }
    };

    typedef bsl::vector<bmqt::MessageGUID>      GUIDs;
    typedef bsl::function<void(GUIDs*, size_t)> GUIDsGeneratorFunc;
    typedef bsl::function<int(const GUIDs&)>    HashCheckerFunc;

    struct GeneratorContext {
        // PUBLIC DATA
        GUIDsGeneratorFunc d_func;

        bsl::string d_name;

        bsl::string d_description;
    } k_GENERATORS[] = {
        {LocalFuncs::generateGUIDs_bmqpMessageGUIDGenerator1,
         "bmqp_1",
         "One bmqp::MessageGUIDGenerator to generate all GUIDs"},
        {LocalFuncs::generateGUIDs_bmqpMessageGUIDGeneratorN,
         "bmqp_N",
         "Multiple different bmqp::MessageGUIDGenerator-s to generate all "
         "GUIDs"},
        {LocalFuncs::generateGUIDs_rand,
         "rand",
         "Init every uint8_t of GUID as 'rand() % 256': "
         "uint8_t[0 .. 15] <- rand() % 256"},
        {LocalFuncs::generateGUIDs_symmetry_4counters,
         "4counters",
         "Init every uint32_t block of GUID as 'counter': "
         "uint32_t[0..3] <- counter, after: counter++"},
        {LocalFuncs::generateGUIDs_symmetry_4quarters,
         "4quarters",
         "Init every int32_t block of GUID as the same 'rand()' value: "
         "val <- rand(), int32_t[0..3] <- val"},
        {LocalFuncs::generateGUIDs_symmetry_2halves,
         "2halves",
         "Init the first half of GUID as 'rand() % 256' for every uint8_t, "
         "then "
         "copy this memory chunk to the second half"},
        {LocalFuncs::generateGUIDs_counter,
         "counter",
         "Init the uint32_t block of GUID as 'counter', set all other to 0: "
         "uint32_t[0] <- counter++, uint32_t[1..3] <- 0"},
    };
    const size_t k_NUM_GENERATORS = sizeof(k_GENERATORS) /
                                    sizeof(*k_GENERATORS);

    struct HashCheckerContext {
        // PUBLIC DATA
        HashCheckerFunc d_func;

        bsl::string d_name;
    } k_HASH_CHECKERS[] = {
        {calcCollisions<bslh::Hash<BaselineHash> >, "baseline"},
        {calcCollisions<bsl::hash<bmqt::MessageGUID> >, "default"},
        {calcCollisions<bslh::Hash<LegacyHash> >, "legacy(djb2)"},
        {calcCollisions<bslh::Hash<Mx3Hash> >, "mx3"},
        {calcCollisions<bslh::Hash<bmqt::MessageGUIDHashAlgo> >, "mxm"}};
    const size_t k_NUM_HASH_CHECKERS = sizeof(k_HASH_CHECKERS) /
                                       sizeof(*k_HASH_CHECKERS);

    Table table;
    for (size_t checkerId = 0; checkerId < k_NUM_HASH_CHECKERS; checkerId++) {
        const HashCheckerContext& checker = k_HASH_CHECKERS[checkerId];
        table.column("Name").insertValue(checker.d_name);
    }

    bsl::vector<bmqt::MessageGUID> guids(bmqtst::TestHelperUtil::allocator());
    guids.reserve(k_NUM_GUIDS);
    for (size_t genId = 0; genId < k_NUM_GENERATORS; genId++) {
        const GeneratorContext& gen = k_GENERATORS[genId];
        gen.d_func(&guids, k_NUM_GUIDS);

        bsl::string sample(bmqtst::TestHelperUtil::allocator());
        sample.resize(bmqt::MessageGUID::e_SIZE_HEX);
        guids.at(k_NUM_GUIDS / 2).toHex(sample.data());

        bsl::cout << "Distribution <" << gen.d_name << ">:" << bsl::endl;
        bsl::cout << gen.d_description << bsl::endl;
        bsl::cout << "Sample: " << sample << bsl::endl << bsl::endl;

        for (size_t checkerId = 0; checkerId < k_NUM_HASH_CHECKERS;
             checkerId++) {
            const HashCheckerContext& checker    = k_HASH_CHECKERS[checkerId];
            const int                 collisions = checker.d_func(guids);
            table.column(gen.d_name).insertValue(collisions);
        }
    }

    table.print(bsl::cout);
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
        bmqtst::TestHelperUtil::ignoreCheckDefAlloc() =
            true;  // istringstream allocates

        bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK DECODE");

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

        bsl::string        hexGuid(bmqtst::TestHelperUtil::allocator());
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
        BMQTST_ASSERT_EQ(guid.isUnset(), false);

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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK PERFORMANCE");
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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK PERFORMANCE");
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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK: DEFAULT "
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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName(
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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK HASH TABLE "
                                      "w/ DEFAULT HASH BENCHMARK");

    bmqt::MessageGUID                             guid;
    bsl::unordered_map<bmqt::MessageGUID, size_t> ht(
        bmqtst::TestHelperUtil::allocator());
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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK HASH TABLE "
                                      "w/ CUSTOM HASH BENCHMARK");

    bmqt::MessageGUID guid;

    bsl::unordered_map<bmqt::MessageGUID,
                       size_t,
                       bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        ht(bmqtst::TestHelperUtil::allocator());
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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK ORDERED MAP "
                                      "DEFAULT HASH BENCHMARK");

    bmqt::MessageGUID guid;

    bmqc::OrderedHashMap<bmqt::MessageGUID, size_t> ht(
        state.range(0),
        bmqtst::TestHelperUtil::allocator());

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
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'bmqp::MessageGUIDGenerator::ctor' prints a BALL_LOG_INFO which
    // allocates using the default allocator.

    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK ORDERED MAP "
                                      "CUSTOM HASH BENCHMARK");

    bmqt::MessageGUID guid;

    bmqc::OrderedHashMap<bmqt::MessageGUID,
                         size_t,
                         bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        ht(state.range(0), bmqtst::TestHelperUtil::allocator());

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

    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

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
        BMQTST_BENCHMARK_WITH_ARGS(testN2_bdlbPerformance,
                                   RangeMultiplier(10)
                                       ->Range(10, 10000000)
                                       ->Unit(benchmark::kMillisecond));
        BMQTST_BENCHMARK_WITH_ARGS(testN2_bmqtPerformance,
                                   RangeMultiplier(10)
                                       ->Range(10, 10000000)
                                       ->Unit(benchmark::kMillisecond));
        break;
    case -3:
        BMQTST_BENCHMARK_WITH_ARGS(testN3_defaultHashBenchmark,
                                   RangeMultiplier(10)
                                       ->Range(10, 10000000)
                                       ->Unit(benchmark::kMillisecond));
        break;
    case -4:
        BMQTST_BENCHMARK_WITH_ARGS(testN4_customHashBenchmark,
                                   RangeMultiplier(10)
                                       ->Range(10, 10000000)
                                       ->Unit(benchmark::kMillisecond));
        break;
    case -5:
        BMQTST_BENCHMARK_WITH_ARGS(testN5_hashTableWithDefaultHashBenchmark,
                                   RangeMultiplier(10)
                                       ->Range(10, 10000000)
                                       ->Unit(benchmark::kMillisecond));
        break;
    case -6:
        BMQTST_BENCHMARK_WITH_ARGS(testN6_hashTableWithCustomHashBenchmark,
                                   RangeMultiplier(10)
                                       ->Range(10, 10000000)
                                       ->Unit(benchmark::kMillisecond));
        break;
    case -7:
        BMQTST_BENCHMARK_WITH_ARGS(testN7_orderedMapWithDefaultHashBenchmark,
                                   RangeMultiplier(10)
                                       ->Range(10, 10000000)
                                       ->Unit(benchmark::kMillisecond));
        break;
    case -8:
        BMQTST_BENCHMARK_WITH_ARGS(testN8_orderedMapWithCustomHashBenchmark,
                                   RangeMultiplier(10)
                                       ->Range(10, 10000000)
                                       ->Unit(benchmark::kMillisecond));
        break;
    case -9: testN9_hashBenchmarkComparison(); break;
    case -10: testN10_hashCollisionsComparison(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }
#ifdef BSLS_PLATFORM_OS_LINUX
    if (_testCase < 0) {
        benchmark::Initialize(&argc, argv);
        benchmark::RunSpecifiedBenchmarks();
    }
#endif

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}

// ----------------------------------------------------------------------------
// NOTICE:
