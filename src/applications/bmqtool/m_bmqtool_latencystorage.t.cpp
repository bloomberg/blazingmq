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

// bmqtool
#include <m_bmqtool_latencystorage.h>

// BDE
#include <bmqtst_tempfile.h>
#include <bmqtst_testhelper.h>
#include <bsl_cstdlib.h>
#include <bsl_fstream.h>
#include <bsl_ios.h>
#include <bsl_iostream.h>
#include <bsls_types.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace m_bmqtool;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
// ------------------------------------------------------------------------
// NOLINTBEGIN(*-magic-numbers)
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    LatencyStorage storage("test", 2, bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(storage.totalCount(), 0);

    storage.insert(100);
    BMQTST_ASSERT_EQ(storage.totalCount(), 1);

    storage.insert(200);
    BMQTST_ASSERT_EQ(storage.totalCount(), 2);
}
// NOLINTEND(*-magic-numbers)

static void test2_roundingPrecisionTest()
// ------------------------------------------------------------------------
// ROUNDING PRECISION TEST
//
// Verify that latencies are grouped into correct buckets with different
// 'latencyDigits' values.
// ------------------------------------------------------------------------
// NOLINTBEGIN(*-magic-numbers)
{
    bmqtst::TestHelper::printTestName("ROUNDING PRECISION TEST");

    // Test with latencyDigits = 1
    {
        LatencyStorage storage("test", 1);

        storage.insert(100);  // 100 rounds to 100
        storage.insert(105);  // 105 rounds to 100
        storage.insert(199);  // 199 rounds to 100

        storage.insert(200);  // 200 rounds to 200
        storage.insert(205);  // 205 rounds to 200

        BMQTST_ASSERT_EQ(storage.totalCount(), 5);

        // Buckets: 100 (3 items), 200 (2 items)
        // NOLINTNEXTLINE(*-magic-numbers)
        bsls::Types::Int64 p50 = storage.computePercentile(50);
        BMQTST_ASSERT_EQ(p50, 100);

        // NOLINTNEXTLINE(*-magic-numbers)
        bsls::Types::Int64 p100 = storage.computePercentile(100);
        BMQTST_ASSERT_EQ(p100, 200);
    }

    // Test with latencyDigits = 2
    {
        LatencyStorage storage("test", 2, bmqtst::TestHelperUtil::allocator());

        storage.insert(500);   // 500 rounds to 500
        storage.insert(550);   // 550 rounds to 550
        storage.insert(1000);  // 1000 rounds to 1000
        storage.insert(1050);  // 1050 rounds to 1000

        BMQTST_ASSERT_EQ(storage.totalCount(), 4);
        BMQTST_ASSERT_EQ(storage.minLatency(), 500);
        BMQTST_ASSERT_EQ(storage.maxLatency(), 1000);

        // Buckets: 500 (1 item), 550 (1 item), 1000 (2 items)
        // NOLINTNEXTLINE(*-magic-numbers)
        bsls::Types::Int64 p50 = storage.computePercentile(50);
        BMQTST_ASSERT(p50 == 550 || p50 == 1000);

        // NOLINTNEXTLINE(*-magic-numbers)
        bsls::Types::Int64 p100 = storage.computePercentile(100);
        BMQTST_ASSERT_EQ(p100, 1000);
    }
}
// NOLINTEND(*-magic-numbers)

static void test3_percentileComputationTest()
// ------------------------------------------------------------------------
// PERCENTILE COMPUTATION TEST
//
// Test 0th, 50th, 95th, and 100th percentile computation.
// ------------------------------------------------------------------------
// NOLINTBEGIN(*-magic-numbers)
{
    bmqtst::TestHelper::printTestName("PERCENTILE COMPUTATION TEST");

    LatencyStorage storage("test", 1);

    // Insert 10 latencies: 10, 10, 20, 20, 20, 30, 30, 40, 50, 60
    // NOLINTBEGIN(*-magic-numbers)
    for (int i = 0; i < 2; ++i) {
        storage.insert(10);
    }
    // NOLINTEND(*-magic-numbers)
    // NOLINTBEGIN(*-magic-numbers)
    for (int i = 0; i < 3; ++i) {
        storage.insert(20);
    }
    // NOLINTEND(*-magic-numbers)
    // NOLINTBEGIN(*-magic-numbers)
    for (int i = 0; i < 2; ++i) {
        storage.insert(30);
    }
    // NOLINTEND(*-magic-numbers)
    storage.insert(40);
    storage.insert(50);
    storage.insert(60);

    BMQTST_ASSERT_EQ(storage.totalCount(), 10);

    // 0th percentile should return first value
    bsls::Types::Int64 p0 = storage.computePercentile(0);
    BMQTST_ASSERT_EQ(p0, 10);

    // 50th percentile (median)
    // NOLINTNEXTLINE(*-magic-numbers)
    bsls::Types::Int64 p50 = storage.computePercentile(50);
    BMQTST_ASSERT(p50 >= 20 && p50 <= 30);

    // 95th percentile should be high
    // NOLINTNEXTLINE(*-magic-numbers)
    bsls::Types::Int64 p95 = storage.computePercentile(95);
    BMQTST_ASSERT(p95 >= 50);

    // 100th percentile should be max
    // NOLINTNEXTLINE(*-magic-numbers)
    bsls::Types::Int64 p100 = storage.computePercentile(100);
    BMQTST_ASSERT_EQ(p100, 60);
}
// NOLINTEND(*-magic-numbers)

static void test4_emptyStorageTest()
// ------------------------------------------------------------------------
// EMPTY STORAGE TEST
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("EMPTY STORAGE TEST");

    LatencyStorage storage("test", 2, bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(storage.totalCount(), 0);
    BMQTST_ASSERT_EQ(storage.minLatency(), 0);
    BMQTST_ASSERT_EQ(storage.maxLatency(), 0);
    BMQTST_ASSERT_EQ(storage.avgLatency(), 0);
    BMQTST_ASSERT_EQ(storage.computePercentile(50), 0);
    BMQTST_ASSERT_EQ(storage.computePercentile(100), 0);
}

static void test5_statisticsTest()
// ------------------------------------------------------------------------
// STATISTICS TEST
// ------------------------------------------------------------------------
// NOLINTBEGIN(*-magic-numbers)
{
    bmqtst::TestHelper::printTestName("STATISTICS TEST");

    LatencyStorage storage("test", 1);

    // Insert: 10, 10, 20, 30, 30, 30
    // Min: 10, Max: 30, Avg: (10+10+20+30+30+30)/6 = 130/6 ≈ 21.67
    storage.insert(10);
    storage.insert(10);
    storage.insert(20);
    storage.insert(30);
    storage.insert(30);
    storage.insert(30);

    BMQTST_ASSERT_EQ(storage.minLatency(), 10);
    BMQTST_ASSERT_EQ(storage.maxLatency(), 30);

    bsls::Types::Int64 avg = storage.avgLatency();
    BMQTST_ASSERT_EQ(avg, 21);  // 130 / 6 = 21 (integer division)
}
// NOLINTEND(*-magic-numbers)

static void test6_saveAndLoadTest()
// ------------------------------------------------------------------------
// SAVE AND LOAD TEST
//
// Save to file, read back, and verify JSON structure and field values.
// ------------------------------------------------------------------------
// NOLINTBEGIN(*-magic-numbers)
{
    bmqtst::TestHelper::printTestName("SAVE AND LOAD TEST");

    bmqtst::TempFile tempFile;
    {
        LatencyStorage storage("test", 2, bmqtst::TestHelperUtil::allocator());

        storage.insert(100);
        storage.insert(100);
        storage.insert(200);
        storage.insert(300);
        storage.insert(300);
        storage.insert(300);

        BMQTST_ASSERT_EQ(storage.minLatency(), 100);
        BMQTST_ASSERT_EQ(storage.maxLatency(), 300);

        int rc = storage.save(tempFile.path());
        BMQTST_ASSERT_EQ(rc, 0);
    }

    // Verify file exists and contains JSON
    bsl::ifstream file(tempFile.path().c_str());
    BMQTST_ASSERT(file.good());

    bsl::string line;
    bsl::string content;
    while (bsl::getline(file, line)) {
        content += line + "\n";
    }
    file.close();

    // Check that content contains expected JSON fields
    BMQTST_ASSERT(content.find("\"min\": 100") != bsl::string::npos);
    BMQTST_ASSERT(content.find("\"max\": 300") != bsl::string::npos);
    BMQTST_ASSERT(content.find("\"dataPoints\": {") != bsl::string::npos);
    BMQTST_ASSERT(content.find("\"100\": 2") !=
                  bsl::string::npos);  // 100 appears 2 times
    BMQTST_ASSERT(content.find("\"300\": 3") !=
                  bsl::string::npos);  // 300 appears 3 times
    BMQTST_ASSERT(content.find("\"median\":") != bsl::string::npos);
    BMQTST_ASSERT(content.find("\"99percentile\":") != bsl::string::npos);
}
// NOLINTEND(*-magic-numbers)

static void test_N1_manualSaveInspection()
// ------------------------------------------------------------------------
// MANUAL SAVE INSPECTION TEST
//
// Insert a large number of random latencies, save to file, and print
// stats and sample output for manual inspection.
// ------------------------------------------------------------------------
// NOLINTBEGIN(performance-avoid-endl)
{
    bmqtst::TestHelper::printTestName("MANUAL SAVE INSPECTION TEST");

    bsl::string filename = "/tmp/latency_storage_dump.json";

    LatencyStorage storage("test", 3, bmqtst::TestHelperUtil::allocator());

    cout << "\nGenerating random latencies..." << endl;

    const bsls::Types::Int64 k_ONE_MILLION = 1000000;
    const bsls::Types::Int64 k_ONE_BILLION = 1000000000;

    // NOLINTBEGIN(*-magic-numbers,performance-avoid-endl)
    for (bsls::Types::Int64 i = 0; i < k_ONE_MILLION; ++i) {
        // NOLINTBEGIN(cert-msc30-c,cert-msc50-cpp)
        bsls::Types::Int64 randomBase = static_cast<bsls::Types::Int64>(
                                            bsl::rand()) %
                                        k_ONE_BILLION;
        // NOLINTEND(cert-msc30-c,cert-msc50-cpp)
        // NOLINTNEXTLINE(*-magic-numbers,cert-msc30-c,cert-msc50-cpp)
        int                divisorExp = static_cast<int>(bsl::rand()) % 9;
        bsls::Types::Int64 divisor    = 1;
        // NOLINTBEGIN(*-magic-numbers)
        for (int j = 0; j < divisorExp; ++j) {
            divisor *= 10;
        }
        // NOLINTEND(*-magic-numbers)

        bsls::Types::Int64 latency = (randomBase / divisor) * divisor;
        storage.insert(latency);

        // Print progress every 100k
        if ((i + 1) % 100000 == 0) {
            cout << "  Inserted " << (i + 1) << " latencies" << endl;
        }
    }
    // NOLINTEND(*-magic-numbers,performance-avoid-endl)

    cout << "Saving to file..." << endl;
    int rc = storage.save(filename);
    BMQTST_ASSERT_EQ(rc, 0);

    cout << filename << endl;

    cout << "\nStatistics:" << endl;
    storage.printSummary(cout);

    // Get file size
    bsl::ifstream file(filename.c_str(), bsl::ios_base::ate);
    if (file) {
        bsl::streampos size = file.tellg();
        cout << "\nFile size: " << size << " bytes" << endl;
        file.close();
    }

    cout << "\nFirst 50 lines of JSON:" << endl;
    cout << "-----" << endl;
    bsl::ifstream infile(filename.c_str());
    bsl::string   line;
    int           lineCount = 0;
    // NOLINTBEGIN(*-magic-numbers,performance-avoid-endl)
    while (bsl::getline(infile, line) && lineCount < 50) {
        cout << line << endl;
        ++lineCount;
    }
    // NOLINTEND(*-magic-numbers,performance-avoid-endl)
    infile.close();
    cout << "-----" << endl;
}
// NOLINTEND(performance-avoid-endl)

// ============================================================================
//                                 MAIN PROGRAM
// ============================================================================

int main(int argc, char* argv[])
// NOLINTBEGIN(*-magic-numbers,cert-err34-c,cppcoreguidelines-pro-bounds-pointer-arithmetic,performance-avoid-endl)
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_breathingTest(); break;
    case 2: test2_roundingPrecisionTest(); break;
    case 3: test3_percentileComputationTest(); break;
    case 4: test4_emptyStorageTest(); break;
    case 5: test5_statisticsTest(); break;
    case 6: test6_saveAndLoadTest(); break;
    case -1: test_N1_manualSaveInspection(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_DEFAULT);
}
// NOLINTEND(*-magic-numbers,cert-err34-c,cppcoreguidelines-pro-bounds-pointer-arithmetic,performance-avoid-endl)
