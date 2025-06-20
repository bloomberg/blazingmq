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

// bmqp_crc32c.t.cpp                                                  -*-C++-*-
#include <bmqp_crc32c.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

#include <bmqu_memoutstream.h>
#include <bmqu_outstreamformatsaver.h>
#include <bmqu_printutil.h>

// BDE
#include <bdlbb_blobutil.h>
#include <bdlde_crc32.h>
#include <bdlf_bind.h>
#include <bdlt_timeunitratio.h>
#include <bsl_algorithm.h>
#include <bsl_cstdlib.h>
#include <bsl_cstring.h>
#include <bsl_ctime.h>
#include <bsl_iomanip.h>
#include <bsl_iostream.h>
#include <bsl_iterator.h>
#include <bsl_numeric.h>
#include <bsl_vector.h>
#include <bslmt_barrier.h>
#include <bslmt_threadgroup.h>
#include <bsls_alignedbuffer.h>
#include <bsls_alignmentfromtype.h>
#include <bsls_alignmentutil.h>
#include <bsls_timeutil.h>

// BENCHMARKING LIBRARY
#ifdef BMQTST_BENCHMARK_ENABLED
#include <benchmark/benchmark.h>
#endif

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Struct representing a record in a table of performance statistics.
struct TableRecord {
    bsls::Types::Int64 d_size;
    bsls::Types::Int64 d_timeOne;
    bsls::Types::Int64 d_timeTwo;
    double             d_ratio;
};

/// Thread function: wait on the specified barrier and then, for each buffer
/// in the specified `in`, calculate its crc32c value and store it in `out`.
static void threadFunction(bsl::vector<unsigned int>* out,
                           const bsl::vector<char*>&  in,
                           bslmt::Barrier*            barrier,
                           bool                       useSoftware)
{
    out->reserve(in.size());
    barrier->wait();

    for (bsl::vector<unsigned int>::size_type i = 0; i < in.size(); ++i) {
        unsigned int crc32c;
        if (useSoftware) {
            crc32c = bmqp::Crc32c_Impl::calculateSoftware(in[i], i + 1);
        }
        else {
            crc32c = bmqp::Crc32c::calculate(in[i], i + 1);
        }
        out->push_back(crc32c);
    }
}

/// Populate the specified `bufferLengths` with various lengths in
/// increasing sorted order.  Return the maximum length populated.  Note
/// that `bufferLengths` will be cleared.
static int populateBufferLengthsSorted(bsl::vector<int>* bufferLengths)
{
    BSLS_ASSERT_SAFE(bufferLengths);

    bufferLengths->clear();

    bufferLengths->push_back(11);
    bufferLengths->push_back(16);  // 2^4
    bufferLengths->push_back(21);
    bufferLengths->push_back(59);
    bufferLengths->push_back(64);  // 2^7
    bufferLengths->push_back(69);
    bufferLengths->push_back(251);
    bufferLengths->push_back(256);  // 2^8
    bufferLengths->push_back(261);
    bufferLengths->push_back(1019);
    bufferLengths->push_back(1024);  // 2^10 = 1 Ki
    bufferLengths->push_back(1029);
    bufferLengths->push_back(4091);
    bufferLengths->push_back(4096);  // 2^12 = 4 Ki
    bufferLengths->push_back(4101);
    bufferLengths->push_back(16379);
    bufferLengths->push_back(16384);  // 16 Ki
    bufferLengths->push_back(16389);
    bufferLengths->push_back(65536);     // 64 Ki
    bufferLengths->push_back(262144);    // 256 Ki
    bufferLengths->push_back(1048576);   // 1 Mi
    bufferLengths->push_back(4194304);   // 4 Mi
    bufferLengths->push_back(16777216);  // 16 Mi
    bufferLengths->push_back(67108864);  // 64 Mi

    bsl::sort(bufferLengths->begin(), bufferLengths->end());

    return bufferLengths->back();
}

#ifdef BMQTST_BENCHMARK_ENABLED
/// Populate the specified `bufferLengths` with various lengths in
/// increasing sorted order. Apply these arguments to Google Benchmark
/// internals Note that upper bound is 64 Ki
static void populateBufferLengthsSorted_GoogleBenchmark_Small(
    benchmark::internal::Benchmark* b)
{
    std::vector<long int> buffLens;
    buffLens.push_back(11);
    buffLens.push_back(16);  // 2^4
    buffLens.push_back(21);
    buffLens.push_back(59);
    buffLens.push_back(64);  // 2^7
    buffLens.push_back(69);
    buffLens.push_back(251);
    buffLens.push_back(256);  // 2^8
    buffLens.push_back(261);
    buffLens.push_back(1019);
    buffLens.push_back(1024);  // 2^10 = 1 Ki
    buffLens.push_back(1029);
    buffLens.push_back(4091);
    buffLens.push_back(4096);  // 2^12 = 4 Ki
    buffLens.push_back(4101);
    buffLens.push_back(16379);
    buffLens.push_back(16384);  // 16 Ki
    buffLens.push_back(16389);
    buffLens.push_back(65536);  // 64 Ki
    for (auto i : buffLens) {
        b->Args({i});
    }
}

/// Populate the specified `bufferLengths` with various lengths in
/// increasing soorted order. Apply these arguments to Google Benchmark
/// internals Note that upper bound is 64Mi
static void populateBufferLengthsSorted_GoogleBenchmark_Large(
    benchmark::internal::Benchmark* b)
{
    std::vector<long int> buffLens;
    buffLens.push_back(11);
    buffLens.push_back(16);  // 2^4
    buffLens.push_back(21);
    buffLens.push_back(59);
    buffLens.push_back(64);  // 2^7
    buffLens.push_back(69);
    buffLens.push_back(251);
    buffLens.push_back(256);  // 2^8
    buffLens.push_back(261);
    buffLens.push_back(1019);
    buffLens.push_back(1024);  // 2^10 = 1 Ki
    buffLens.push_back(1029);
    buffLens.push_back(4091);
    buffLens.push_back(4096);  // 2^12 = 4 Ki
    buffLens.push_back(4101);
    buffLens.push_back(16379);
    buffLens.push_back(16384);  // 16 Ki
    buffLens.push_back(16389);
    buffLens.push_back(65536);     // 64 Ki
    buffLens.push_back(262144);    // 256 Ki
    buffLens.push_back(1048576);   // 1 Mi
    buffLens.push_back(4194304);   // 4 Mi
    buffLens.push_back(16777216);  // 16 Mi
    buffLens.push_back(67108864);  // 64 Mi
    for (auto i : buffLens) {
        b->Args({i});
    }
}
#endif

/// Print the specified `headers` to the specified `out` in the following
/// format:
///
/// ```
/// ===================================================================
/// | <headers[0]> | <headers[1]> | ... | <headers[headers.size() - 1]>
/// ===================================================================
/// ```
static void printTableHeader(bsl::ostream&                   out,
                             const bsl::vector<bsl::string>& headers)
{
    const unsigned int sumSizes = bsl::accumulate(headers.begin(),
                                                  headers.end(),
                                                  bsl::string(""))
                                      .size();

    const unsigned int headerLineLength = sumSizes + (3 * headers.size() - 1);

    bsl::fill_n(bsl::ostream_iterator<char>(bsl::cout), '=', headerLineLength);
    bsl::cout << '\n';
    for (bsl::vector<bsl::string>::size_type col = 0; col < headers.size();
         ++col) {
        out << "| " << headers[col];

        // Last column?
        if (col == (headers.size() - 1)) {
            out << '\n';
        }
        else {
            out << ' ';
        }
    }
    bsl::fill_n(bsl::ostream_iterator<char>(bsl::cout), '=', headerLineLength);
    bsl::cout << '\n';
}

/// Print the specified `tableRecords` to the specified `out`, using the
/// specified `headerCols` to determine the appropriate width for each column.
/// Print in the following format:
///
/// ```
/// | <size> | <timeOne> | <timeTwo> | <ratio>
/// ```
static void printTableRows(bsl::ostream&                   out,
                           const bsl::vector<TableRecord>& tableRecords,
                           const bsl::vector<bsl::string>& headerCols)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(headerCols.size() == 4);

    bmqu::OutStreamFormatSaver fmtSaver(out);

    out << bsl::right << bsl::fixed;

    for (bsl::vector<TableRecord>::size_type i = 0; i < tableRecords.size();
         ++i) {
        const TableRecord& record = tableRecords[i];

        // size
        out << "| " << bsl::setw(headerCols[0].size() + 1) << record.d_size;
        // timeOne
        out << "| " << bsl::setw(headerCols[1].size() + 1) << record.d_timeOne;
        // timeTwo
        out << "| " << bsl::setw(headerCols[2].size() + 1) << record.d_timeTwo;
        // ratio
        out << "| " << bsl::setw(headerCols[3].size()) << bsl::setprecision(3)
            << record.d_ratio;

        out << '\n';
    }
}

/// Print a table having the specified `headerCols` and `tableRecords` to
/// the specified `out`.
static void printTable(bsl::ostream&                   out,
                       const bsl::vector<bsl::string>& headerCols,
                       const bsl::vector<TableRecord>& tableRecords)
{
    printTableHeader(out, headerCols);
    printTableRows(out, tableRecords, headerCols);
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
//   - Calculate CRC32-C on a buffer and compare the result to the known
//     correct CRC32-C.
//   - Calculate CRC32-C on a null buffer using the previous crc as a
//     starting point and verify that the result is the same as the
//     previous result.
//   - Calculate CRC32-C on a buffer with length 0 using the previous crc
//     as a starting point and verify that the result is the same as the
//     previous crc.
//   - Calculate CRC32-C on a prefix of a buffer and then on the rest of
//     the buffer using the result of the first calculation as starting
//     point and compare the final result to the known correct CRC32-C
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    {
        PV("BUFFER");

        const char*        buffer         = "12345678";
        const unsigned int length         = strlen(buffer);
        const unsigned int expectedCrc    = 0x6087809A;
        unsigned int       crc32cDefault  = 0;
        unsigned int       crc32cSoftware = 0;
        unsigned int       crc32cHWSerial = 0;

        // Default
        crc32cDefault = bmqp::Crc32c::calculate(buffer, length);

        // Software
        crc32cSoftware = bmqp::Crc32c_Impl::calculateSoftware(buffer, length);

        // Hardware Serial
        crc32cHWSerial = bmqp::Crc32c_Impl::calculateHardwareSerial(buffer,
                                                                    length);

        BMQTST_ASSERT_EQ(crc32cDefault, expectedCrc);
        BMQTST_ASSERT_EQ(crc32cSoftware, expectedCrc);
        BMQTST_ASSERT_EQ(crc32cHWSerial, expectedCrc);
    }

    {
        PV("NULL BUFFER");

        // Test edge case of null buffer and length = 0
        const unsigned int expectedCrc    = 0;
        unsigned int       crc32cDef      = 0;
        unsigned int       crc32cSW       = 0;
        unsigned int       crc32cHWSerial = 0;
        // Default
        crc32cDef = bmqp::Crc32c::calculate(0, 0);

        // Software
        crc32cSW = bmqp::Crc32c_Impl::calculateSoftware(0, 0);

        /// Hardware Serial
        crc32cHWSerial = bmqp::Crc32c_Impl::calculateHardwareSerial(0, 0);

        BMQTST_ASSERT_EQ(crc32cDef, expectedCrc);
        BMQTST_ASSERT_EQ(crc32cSW, expectedCrc);
        BMQTST_ASSERT_EQ(crc32cHWSerial, expectedCrc);
    }

    {
        PV("LENGTH PARAMETER EQUAL TO 0");

        const char*  buffer         = "12345678";
        unsigned int length         = 0;
        unsigned int expectedCrc    = 0;  // because length = 0
        unsigned int crc32cDefault  = 0;
        unsigned int crc32cSoftware = 0;
        unsigned int crc32cHWSerial = 0;

        // Default
        crc32cDefault = bmqp::Crc32c::calculate(buffer, length);

        // Software
        crc32cSoftware = bmqp::Crc32c_Impl::calculateSoftware(buffer, length);

        // Hardware Serial
        crc32cHWSerial = bmqp::Crc32c_Impl::calculateHardwareSerial(buffer,
                                                                    length);

        BMQTST_ASSERT_EQ(crc32cDefault, expectedCrc);
        BMQTST_ASSERT_EQ(crc32cSoftware, expectedCrc);
        BMQTST_ASSERT_EQ(crc32cHWSerial, expectedCrc);

        // Compute value for whole buffer
        length         = strlen(buffer);
        expectedCrc    = 0x6087809A;
        crc32cDefault  = 0;
        crc32cSoftware = 0;
        crc32cHWSerial = 0;

        // Default
        crc32cDefault = bmqp::Crc32c::calculate(buffer, length);

        // Software
        crc32cSoftware = bmqp::Crc32c_Impl::calculateSoftware(buffer, length);

        // Hardware Serial
        crc32cHWSerial = bmqp::Crc32c_Impl::calculateHardwareSerial(buffer,
                                                                    length);

        BMQTST_ASSERT_EQ(crc32cDefault, expectedCrc);
        BMQTST_ASSERT_EQ(crc32cSoftware, expectedCrc);
        BMQTST_ASSERT_EQ(crc32cHWSerial, expectedCrc);

        // Now specify length 0 and pass the previous crc and expect the
        // previous crc to be the result.
        length = 0;

        unsigned int prevCrc = crc32cDefault;

        // Default
        crc32cDefault = bmqp::Crc32c::calculate(buffer, length, prevCrc);

        // Software
        crc32cSoftware = bmqp::Crc32c_Impl::calculateSoftware(buffer,
                                                              length,
                                                              prevCrc);

        // Hardware Serial
        crc32cHWSerial = bmqp::Crc32c_Impl::calculateHardwareSerial(buffer,
                                                                    length,
                                                                    prevCrc);

        BMQTST_ASSERT_EQ(crc32cDefault, expectedCrc);
        BMQTST_ASSERT_EQ(crc32cSoftware, expectedCrc);
        BMQTST_ASSERT_EQ(crc32cHWSerial, expectedCrc);
    }

    {
        PV("SPLIT BUFFER WITH PREVIOUS CRC")
        // Prefix of a buffer, then the rest using the previous crc as the
        // starting point
        const char*        buffer         = "12345678";
        const unsigned int expectedCrc    = 0x6087809A;
        unsigned int       crc32cDefault  = 0;
        unsigned int       crc32cSoftware = 0;
        unsigned int       crc32cHWSerial = 0;

        const unsigned int prefixLength = 3;

        // Default
        crc32cDefault = bmqp::Crc32c::calculate(buffer, prefixLength);

        // Software
        crc32cSoftware = bmqp::Crc32c_Impl::calculateSoftware(buffer,
                                                              prefixLength);

        // Hardware Serial
        crc32cHWSerial =
            bmqp::Crc32c_Impl::calculateHardwareSerial(buffer, prefixLength);

        const unsigned int restLength = strlen(buffer) - prefixLength;

        // Default
        crc32cDefault = bmqp::Crc32c::calculate(buffer + prefixLength,
                                                restLength,
                                                crc32cDefault);
        // Software
        crc32cSoftware = bmqp::Crc32c_Impl::calculateSoftware(buffer +
                                                                  prefixLength,
                                                              restLength,
                                                              crc32cSoftware);

        // Hardware Serial
        crc32cHWSerial = bmqp::Crc32c_Impl::calculateHardwareSerial(
            buffer + prefixLength,
            restLength,
            crc32cHWSerial);

        BMQTST_ASSERT_EQ(crc32cDefault, expectedCrc);
        BMQTST_ASSERT_EQ(crc32cSoftware, expectedCrc);
        BMQTST_ASSERT_EQ(crc32cHWSerial, expectedCrc);
    }
}

static void test2_calculateOnBuffer()
// ------------------------------------------------------------------------
// CALCULATE CRC32-C ON BUFFER
//
// Concerns:
//   Verify the correctness of computing CRC32-C on a buffer w/o a previous
//   CRC (which would have to be taken into account if it were provided)
//   using both the default and software implementations.
//
// Plan:
//   - Calculate CRC32-C for various buffers and compare the results to the
//     known correct CRC32-C values for these buffers.
//
// Testing:
//   Correctness of CRC32-C calculation on an entire buffer at once using
//   both the default and software flavors for calculating CRC32-C.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CALCULATE CRC32-C ON BUFFER");

    struct Test {
        int          d_line;
        const char*  d_buffer;
        unsigned int d_expectedCrc32c;
    } k_DATA[] = {{L_, "", 0},
                  {L_, "DYB|O", 0},
                  {L_, "0", 0x629E1AE0},
                  {L_, "1", 0x90F599E3},
                  {L_, "2", 0x83A56A17},
                  {L_, "~", 0x8F9DB87B},
                  {L_, "22", 0x47B26CF9},
                  {L_, "fa", 0x8B9F1387},
                  {L_, "t0-", 0x77E2D1A9},
                  {L_, "34v}", 0x031AD8A7},
                  {L_, "shaii", 0xB0638FB5},
                  {L_, "3jf-_3", 0xE186B745},
                  {L_, "bonjour", 0x156088D2},
                  {L_, "vbPHbvtB", 0x12AAFAA6},
                  {L_, "aoDicgezd", 0xBF5E01C8},
                  {L_, "123456789", 0xe3069283},
                  {L_, "gaaXsSP1al", 0xC4E61D23},
                  {L_, "2Wm9bbNDehd", 0x54A11873},
                  {L_, "GamS0NJhAl8y", 0x0044AC66}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": calculating CRC32-C (w/o previous) on '"
                        << test.d_buffer << "'");

        unsigned int length = strlen(test.d_buffer);

        // Default
        unsigned int crc32cDefault = bmqp::Crc32c::calculate(test.d_buffer,
                                                             length);
        // Software
        unsigned int crc32cSoftware =
            bmqp::Crc32c_Impl::calculateSoftware(test.d_buffer, length);
        // HW Serial
        unsigned int crc32cHWSerial =
            bmqp::Crc32c_Impl::calculateHardwareSerial(test.d_buffer, length);

        // Verify correctness
        BMQTST_ASSERT_EQ_D("line " << test.d_line << " (Default)",
                           crc32cDefault,
                           test.d_expectedCrc32c);
        BMQTST_ASSERT_EQ_D("line " << test.d_line << " (Software)",
                           crc32cSoftware,
                           test.d_expectedCrc32c);
        BMQTST_ASSERT_EQ_D("line " << test.d_line << " (HW Serial)",
                           crc32cHWSerial,
                           test.d_expectedCrc32c);

        // Test edge case of non-null buffer and length = 0

        // Default
        crc32cDefault = bmqp::Crc32c::calculate(test.d_buffer, 0);

        // Software
        crc32cSoftware = bmqp::Crc32c_Impl::calculateSoftware(test.d_buffer,
                                                              0);

        // Hardware Serial
        crc32cHWSerial =
            bmqp::Crc32c_Impl::calculateHardwareSerial(test.d_buffer, 0);

        BMQTST_ASSERT_EQ_D("line " << test.d_line << " (Default)",
                           crc32cDefault,
                           0u);
        BMQTST_ASSERT_EQ_D("line " << test.d_line << " (Software)",
                           crc32cSoftware,
                           0u);
        BMQTST_ASSERT_EQ_D("line " << test.d_line << " (HW Serial)",
                           crc32cHWSerial,
                           0u);
    }
}

static void test3_calculateOnMisalignedBuffer()
// ------------------------------------------------------------------------
// CALCULATE CRC32-C ON MISALIGNED BUFFER
//
// Concerns:
//   Ensure that calculating CRC32-C on a buffer that is not at an
//   alignment boundary yields the same result as for one that is.
//
// Plan:
//   - Generate buffers at an alignment boundary consisting of
//     '1 <= i <= 7' characters preceding the test buffers from the
//     previous test cases and calculate CRC32-C for the test buffers at
//     'i' bytes from the beginning of the aligned buffer.
//
// Testing:
//   Calculating CRC32-C on a buffer that is not at an alignment boundary
//   - bmqp::Crc32c::calculate(const void   *data,
//                             unsigned int  length,
//                             unsigned int  crc = k_NULL_CRC32C);
//   - bmqp::Crc32c_Impl::calculateSoftware(
//                                      const void   *data,
//                                      unsigned int  length,
//                                      unsigned int  crc = k_NULL_CRC32C);
//   - bmqp::Crc32c_Impl::calculateHardwareSerial(
//                                      const void   *data,
//                                      unsigned int  length,
//                                      unsigned int  crc = k_NULL_CRC32C);
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "CALCULATE CRC32-C ON MISALIGNED BUFFER");

    const int k_BUFFER_SIZE = 1024;
    const int k_MY_ALIGNMENT =
        bsls::AlignmentFromType<bsls::AlignmentUtil::MaxAlignedType>::VALUE;

    // Aligned buffer sufficiently large to contain every buffer generated
    // for testing below
    bsls::AlignedBuffer<k_BUFFER_SIZE, k_MY_ALIGNMENT> allocBuffer;

    char* allocPtr = allocBuffer.buffer();

    // Sanity check: test.d_buffer at alignment boundary
    BSLS_ASSERT_OPT(
        bsls::AlignmentUtil::calculateAlignmentOffset(allocPtr,
                                                      k_MY_ALIGNMENT) == 0);

    struct Test {
        int          d_line;
        const char*  d_buffer;
        unsigned int d_expectedCrc32c;
    } k_DATA[] = {{L_, "", 0},
                  {L_, "DYB|O", 0},
                  {L_, "0", 0x629E1AE0},
                  {L_, "1", 0x90F599E3},
                  {L_, "2", 0x83A56A17},
                  {L_, "~", 0x8F9DB87B},
                  {L_, "22", 0x47B26CF9},
                  {L_, "fa", 0x8B9F1387},
                  {L_, "t0-", 0x77E2D1A9},
                  {L_, "34v}", 0x031AD8A7},
                  {L_, "shaii", 0xB0638FB5},
                  {L_, "3jf-_3", 0xE186B745},
                  {L_, "bonjour", 0x156088D2},
                  {L_, "vbPHbvtB", 0x12AAFAA6},
                  {L_, "aoDicgezd", 0xBF5E01C8},
                  {L_, "123456789", 0xe3069283},
                  {L_, "gaaXsSP1al", 0xC4E61D23},
                  {L_, "2Wm9bbNDehd", 0x54A11873},
                  {L_, "GamS0NJhAl8y", 0x0044AC66}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test&        test          = k_DATA[idx];
        const unsigned int testBufLength = strlen(test.d_buffer);

        for (unsigned int i = 1; i < k_MY_ALIGNMENT; ++i) {
            const unsigned int newBufLength = testBufLength + i + 1;

            // Sanity check: aligned buffer sufficiently large for some prefix
            //               padding plus the test data, and the test data is
            //               *not* starting at an alignment boundary inside the
            //               aligned buffer.
            BSLS_ASSERT_OPT(k_BUFFER_SIZE >= newBufLength);
            BSLS_ASSERT_OPT(bsls::AlignmentUtil::calculateAlignmentOffset(
                                allocPtr + i,
                                k_MY_ALIGNMENT) != 0);

            bsl::memset(allocPtr, '\0', newBufLength);

            // Create a C-string with 'i' characters preceding the test buffer:
            // 'XX...X<test.d_buffer>'
            bsl::memset(allocPtr, 'X', i);
            bsl::memcpy(allocPtr + i, test.d_buffer, testBufLength);

            PVV(test.d_line
                << ": calculating CRC32-C on '" << test.d_buffer << "' ["
                << bsl::hex << static_cast<void*>(allocPtr + i) << "], " << i
                << " bytes past an alignment boundary: " << allocPtr);

            // Default
            unsigned int crc32cDefault =
                bmqp::Crc32c::calculate(allocPtr + i, testBufLength);
            // Software
            unsigned int crc32cSoftware = bmqp::Crc32c_Impl::calculateSoftware(
                allocPtr + i,
                testBufLength);

            // Hardware Serial
            unsigned int crc32cHWSerial =
                bmqp::Crc32c_Impl::calculateHardwareSerial(allocPtr + i,
                                                           testBufLength);

            // Verify correctness
            BMQTST_ASSERT_EQ_D("line " << test.d_line << " (Default)",
                               crc32cDefault,
                               test.d_expectedCrc32c);
            BMQTST_ASSERT_EQ_D("line " << test.d_line << " (Software)",
                               crc32cSoftware,
                               test.d_expectedCrc32c);
            BMQTST_ASSERT_EQ_D("line " << test.d_line << " (HW Serial)",
                               crc32cHWSerial,
                               test.d_expectedCrc32c);
        }
    }
}

/// Plan:
///   - For various buffers, calculate CRC32-C for a prefix chunk of the
///     buffer, and pass the calculated CRC32-C as a parameter (`crc`) for
///     calculating the final CRC32-C value.  Compare the final CRC32-C
///     values to the known correct crc32c values for these buffers.
///
/// Testing:
///   Correctness of CRC32-C calculation on an entire buffer at once using
///   both the default and software flavors.
/// ------------------------------------------------------------------------
static void test4_calculateOnBufferWithPreviousCrc()
{
    bmqtst::TestHelper::printTestName(
        "CALCULATE CRC32-C ON BUFFER WITH PREVIOUS CRC");

    struct Test {
        int          d_line;
        const char*  d_buffer;
        unsigned int d_prefixLen;
        unsigned int d_expectedCrc32c;
    } k_DATA[] = {
        // Note that for d_buffer[0:d_prefixLen] (the prefix string ) we
        // already asserted that we calculated the correct CRC32-C values in
        // test case 2. So this data is suitable for checking that
        // incorporating another buffer into the CRC32-C of the prefix behaves
        // correctly.
        {L_, "", 0, 0},
        {L_, "DYB|O--", 5, 0xD1436CCE},
        {L_, "0sef", 1, 0x50588062},
        {L_, "13", 1, 0x813E4763},
        {L_, "2s34faw", 1, 0xED5E0C1C},
        {L_, "~ahaer", 1, 0x45F10742},
        {L_, "22aasd", 2, 0x22B28122},
        {L_, "faghar", 2, 0xD9253928},
        {L_, "t0-aavk", 3, 0x8A752D3F},
        {L_, "34v}acv", 4, 0xC36C7D1D},
        {L_, "shaiig5bg", 5, 0x9E26CF81},
        {L_, "123456789", 9, 0xe3069283},
        {L_, "3jf-_3adfg", 6, 0xEDA627B3},
        {L_, "bonjour421h", 7, 0xD23EF1DF},
        {L_, "vbPHbvtB45gga", 8, 0xFCC29260},
        {L_, "aoDicgezd==7h", 9, 0x171D042A},
        {L_, "gaaXsSP1aldsafad", 10, 0xFD5078EF},
        {L_, "2Wm9bbNDehd32qf", 11, 0x9F7277C6},
        {L_, "GamS0NJhAl8yw3th", 12, 0x6033D909}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": calculating CRC32-C (with previous CRC) on '"
                        << test.d_buffer << "'");

        const unsigned int prefixLen = test.d_prefixLen;
        const unsigned int suffixLen = strlen(test.d_buffer) - prefixLen;

        // Default
        unsigned int crc32cDefault = bmqp::Crc32c::calculate(test.d_buffer,
                                                             prefixLen);

        crc32cDefault = bmqp::Crc32c::calculate(test.d_buffer + prefixLen,
                                                suffixLen,
                                                crc32cDefault);

        BMQTST_ASSERT_EQ_D("line " << test.d_line << " (Default)",
                           crc32cDefault,
                           test.d_expectedCrc32c);

        // Software
        unsigned int crc32cSoftware =
            bmqp::Crc32c_Impl::calculateSoftware(test.d_buffer, prefixLen);

        crc32cSoftware = bmqp::Crc32c_Impl::calculateSoftware(test.d_buffer +
                                                                  prefixLen,
                                                              suffixLen,
                                                              crc32cSoftware);

        BMQTST_ASSERT_EQ_D("line " << test.d_line << " (Software)",
                           crc32cSoftware,
                           test.d_expectedCrc32c);

        // Test edge case where non-null buffer and length = 0 with previous
        // CRC returns the previous CRC
        const unsigned int previousCrc = crc32cDefault;

        // Default
        crc32cDefault = bmqp::Crc32c::calculate(test.d_buffer, 0, previousCrc);

        BMQTST_ASSERT_EQ_D("line " << test.d_line << " (Default)",
                           crc32cDefault,
                           previousCrc);

        // Software
        crc32cSoftware = bmqp::Crc32c_Impl::calculateSoftware(test.d_buffer,
                                                              0,
                                                              previousCrc);

        BMQTST_ASSERT_EQ_D("line " << test.d_line << " (Software)",
                           crc32cSoftware,
                           previousCrc);

        // Test edge case where null buffer and length = 0 with previous
        // CRC returns the previous CRC
        // Note: We could have not put this test block inside the loop but
        //       we might as well because it tells us that a null buffer with a
        //       length of 0 will yield the previous CRC for multiple different
        //       previous CRCs (not just one or a couple).

        // Default
        crc32cDefault = bmqp::Crc32c::calculate(0, 0, previousCrc);

        BMQTST_ASSERT_EQ_D("line " << test.d_line << " (Default)",
                           crc32cDefault,
                           previousCrc);

        // Software
        crc32cSoftware = bmqp::Crc32c_Impl::calculateSoftware(0,
                                                              0,
                                                              previousCrc);

        BMQTST_ASSERT_EQ_D("line " << test.d_line << " (Software)",
                           crc32cSoftware,
                           previousCrc);
    }
}

static void test5_multithreadedCrc32cDefault()
// ------------------------------------------------------------------------
// MULTITHREAD DEFAULT CRC32-C
//
// Concerns:
//   Test bmqp::Crc32c::calculate(const void *data, unsigned int length)
//   in a multi-threaded environment, making sure that each CRC32-C is
//   computed correctly across all threads when they perform the
//   calculation concurrently.
//
// Plan:
//   - Generate a large set of random payloads of different sizes.
//   - In serial, calculate the CRC32-C value for each of these payloads.
//   - Spawn a few threads and have them calculate the CRC32-C value for
//     each payload. Once they are done, make sure all calculated CRC32-C
//     values from the threads match the CRC32-C values calculated in
//     serial.
//
// Testing:
//   Correctness of default (possibly hardware-based) CRC32-C calculation
//   in a multithreaded environment.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    bmqtst::TestHelperUtil::ignoreCheckGblAlloc() = true;
    // Can't ensure no global memory is allocated because
    // 'bslmt::ThreadUtil::create()' uses the global allocator to allocate
    // memory.

    bmqtst::TestHelper::printTestName("MULTITHREAD DEFAULT CRC32-C");

    enum { k_NUM_PAYLOADS = 10000, k_NUM_THREADS = 10 };

    // Input buffers
    bsl::vector<char*> payloads(bmqtst::TestHelperUtil::allocator());
    payloads.reserve(k_NUM_PAYLOADS);

    // Populate with random data
    for (unsigned int i = 0; i < k_NUM_PAYLOADS; ++i) {
        char* buffer = static_cast<char*>(
            bmqtst::TestHelperUtil::allocator()->allocate(i + 1));
        bsl::generate_n(buffer, i + 1, bsl::rand);
        payloads.push_back(buffer);
    }

    // [1] Serial calculation
    bsl::vector<unsigned int> serialCrc32cData(
        bmqtst::TestHelperUtil::allocator());
    serialCrc32cData.reserve(k_NUM_PAYLOADS);
    for (unsigned int i = 0; i < k_NUM_PAYLOADS; ++i) {
        const unsigned int crc32c = bmqp::Crc32c::calculate(payloads[i],
                                                            i + 1);
        serialCrc32cData.push_back(crc32c);
    }

    // [2] Multithreaded Calculation
    bslmt::ThreadGroup threadGroup(bmqtst::TestHelperUtil::allocator());

    // Barrier to get each thread to start at the same time; `+1` for this
    // (main) thread.
    bslmt::Barrier barrier(k_NUM_THREADS + 1);

    bsl::vector<bsl::vector<unsigned int> > threadsCrc32cData(
        k_NUM_THREADS,
        bmqtst::TestHelperUtil::allocator());
    for (unsigned int i = 0; i < k_NUM_THREADS; ++i) {
        int rc = threadGroup.addThread(
            bdlf::BindUtil::bind(&threadFunction,
                                 &threadsCrc32cData[i],
                                 payloads,
                                 &barrier,
                                 false));
        BSLS_ASSERT_OPT(rc == 0);
    }

    barrier.wait();
    threadGroup.joinAll();

    // Compare CRC32-C: serial result vs. each thread's result
    for (unsigned int i = 0; i < k_NUM_THREADS; ++i) {
        const bsl::vector<unsigned int>& currThreadCrc32cData =
            threadsCrc32cData[i];
        for (unsigned int j = 0; j < k_NUM_PAYLOADS; ++j) {
            BMQTST_ASSERT_EQ_D("thread " << i << "[" << j << "]",
                               serialCrc32cData[j],
                               currThreadCrc32cData[j]);
        }
    }

    // Delete allocated payloads
    for (unsigned int i = 0; i < k_NUM_PAYLOADS; ++i) {
        bmqtst::TestHelperUtil::allocator()->deallocate(payloads[i]);
    }
}

static void test6_multithreadedCrc32cSoftware()
// ------------------------------------------------------------------------
// MULTITHREAD SOFTWARE CRC32-C
//
// Concerns:
//   Test bmqp::Crc32c_Impl::calculateSoftware(const void  *data,
//                                             unsigned int length)
//   in a multi-threaded environment, making sure that each CRC32-C is
//   computed correctly across all threads when they perform the
//   calculation concurrently.
//
// Plan:
//   - Generate a large set of random payloads of different sizes.
//   - In serial, calculate the CRC32-C value for each of these payloads.
//   - Spawn a few threads and have them calculate the CRC32-C value for
//     each payload using the software-based utility. Once they are done,
//     make sure all calculated CRC32-C values from the threads match the
//     CRC32-C values calculated in serial.
//
// Testing:
//   Correctness of software-based CRC32-C calculation in a multithreaded
//   environment.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    bmqtst::TestHelperUtil::ignoreCheckGblAlloc() = true;
    // Can't ensure no global memory is allocated because
    // 'bslmt::ThreadUtil::create()' uses the global allocator to allocate
    // memory.

    bmqtst::TestHelper::printTestName("MULTITHREAD SOFTWARE CRC32-C");

    enum { k_NUM_PAYLOADS = 10000, k_NUM_THREADS = 10 };

    // Input buffers
    bsl::vector<char*> payloads(bmqtst::TestHelperUtil::allocator());
    payloads.reserve(k_NUM_PAYLOADS);

    // Populate with random data
    for (unsigned int i = 0; i < k_NUM_PAYLOADS; ++i) {
        char* buffer = static_cast<char*>(
            bmqtst::TestHelperUtil::allocator()->allocate(i + 1));
        bsl::generate_n(buffer, i + 1, bsl::rand);
        payloads.push_back(buffer);
    }

    // [1] Serial calculation
    bsl::vector<unsigned int> serialCrc32cData(
        bmqtst::TestHelperUtil::allocator());
    serialCrc32cData.reserve(k_NUM_PAYLOADS);
    for (unsigned int i = 0; i < k_NUM_PAYLOADS; ++i) {
        const unsigned int crc32c =
            bmqp::Crc32c_Impl::calculateSoftware(payloads[i], i + 1);
        serialCrc32cData.push_back(crc32c);
    }

    // [2] Multithreaded Calculation
    bslmt::ThreadGroup threadGroup(bmqtst::TestHelperUtil::allocator());

    // Barrier to get each thread to start at the same time; `+1` for this
    // (main) thread.
    bslmt::Barrier barrier(k_NUM_THREADS + 1);

    bsl::vector<bsl::vector<unsigned int> > threadsCrc32cData(
        bmqtst::TestHelperUtil::allocator());
    threadsCrc32cData.resize(k_NUM_THREADS);
    for (unsigned int i = 0; i < k_NUM_THREADS; ++i) {
        int rc = threadGroup.addThread(
            bdlf::BindUtil::bind(&threadFunction,
                                 &threadsCrc32cData[i],
                                 payloads,
                                 &barrier,
                                 true));
        BSLS_ASSERT_OPT(rc == 0);
    }

    barrier.wait();
    threadGroup.joinAll();

    // Compare crc32c: serial result vs. each thread's result
    for (unsigned int i = 0; i < k_NUM_THREADS; ++i) {
        const bsl::vector<unsigned int>& currThreadCrc32cData =
            threadsCrc32cData[i];
        for (unsigned int j = 0; j < k_NUM_PAYLOADS; ++j) {
            BMQTST_ASSERT_EQ_D("thread " << i << "[" << j << "]",
                               serialCrc32cData[j],
                               currThreadCrc32cData[j]);
        }
    }

    // Delete allocated payloads
    for (unsigned int i = 0; i < k_NUM_PAYLOADS; ++i) {
        bmqtst::TestHelperUtil::allocator()->deallocate(payloads[i]);
    }
}

static void test7_calculateOnBlob()
// ------------------------------------------------------------------------
// CALCULATE CRC32-C ON BLOB w/o PREVIOUS CRC
//
// Concerns:
//   Verify the correctness of calculating CRC32-C on a blob w/o a previous
//   CRC (which would have to be taken into account if it were provided)
//   using both the default and software flavors.
//
// Plan:
//   - Calculate CRC32-C for a blob having no data buffers and verify that
//     the result corresponds to the 'null' CRC32-C.
//   - Calculate CRC32-C for a blob composed of various buffers and compare
//     the result to the known correct CRC32-C value for the buffer
//     consisting of the individual buffers concatenated.
//   - Calculate CRC32-C for a blob composed of one buffer of a typical
//     size and compare the result to the known correct CRC32-C value for
//     the buffer.
//
// Testing:
//   Correctness of CRC32-C calculation on an entire blob at once using
//   both the default and software flavors.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "CALCULATE CRC32-C ON BLOB w/o PREVIOUS CRC");

    {
        // One empty blob (no data buffers)

        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());

        // Assumption
        BSLS_ASSERT_OPT(blob.numDataBuffers() == 0);

        // Default
        const unsigned int crc32cDefault = bmqp::Crc32c::calculate(blob);
        // Software
        const unsigned int crc32cSoftware =
            bmqp::Crc32c_Impl::calculateSoftware(blob);

        const unsigned int expectedCrc32c = bmqp::Crc32c::k_NULL_CRC32C;
        BMQTST_ASSERT_EQ(crc32cDefault, expectedCrc32c);
        BMQTST_ASSERT_EQ(crc32cSoftware, expectedCrc32c);
    }

    {
        // Multiple blob buffers, each of small length

        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());

        char                  one[] = "one";
        bsl::shared_ptr<char> onePtr;
        onePtr.reset(one,
                     bslstl::SharedPtrNilDeleter(),
                     bmqtst::TestHelperUtil::allocator());
        bdlbb::BlobBuffer buf1(onePtr, bsl::strlen(one));

        blob.appendDataBuffer(buf1);

        char                  two[] = "two";
        bsl::shared_ptr<char> twoPtr;
        twoPtr.reset(two,
                     bslstl::SharedPtrNilDeleter(),
                     bmqtst::TestHelperUtil::allocator());
        bdlbb::BlobBuffer buf2(twoPtr, bsl::strlen(two));

        blob.appendDataBuffer(buf2);

        char                  three[] = "three";
        bsl::shared_ptr<char> threePtr;
        threePtr.reset(three,
                       bslstl::SharedPtrNilDeleter(),
                       bmqtst::TestHelperUtil::allocator());
        bdlbb::BlobBuffer buf3(threePtr, bsl::strlen(three));

        blob.appendDataBuffer(buf3);

        // Default
        const unsigned int crc32cDefault = bmqp::Crc32c::calculate(blob);
        // Software
        const unsigned int crc32cSoftware =
            bmqp::Crc32c_Impl::calculateSoftware(blob);

        const unsigned int expectedCrc32c = 0xA0EA6901;
        BMQTST_ASSERT_EQ(crc32cDefault, expectedCrc32c);
        BMQTST_ASSERT_EQ(crc32cSoftware, expectedCrc32c);
    }

    {
        // One blob buffer of medium length

        bdlbb::Blob blob(bmqtst::TestHelperUtil::allocator());

        char buf[] = "This will be put in a blob buffer of typical"
                     " size, and then we will test the crc32c calculation"
                     " (blob version) with only one blob buffer to ensure"
                     " that the logic of the loop works even for one blob"
                     " buffer. Moreover, append some lines bellow to increase"
                     " the size of this buffer."
                     "#######################################################"
                     "#######################################################"
                     "#######################################################"
                     "#######################################################"
                     "#######################################################"
                     "#######################################################"
                     "#######################################################"
                     "#######################################################"
                     "#######################################################"
                     "#######################################################";
        bsl::shared_ptr<char> bufSP;
        bufSP.reset(buf,
                    bslstl::SharedPtrNilDeleter(),
                    bmqtst::TestHelperUtil::allocator());
        bdlbb::BlobBuffer blobBuf(bufSP, bsl::strlen(buf));

        blob.appendDataBuffer(blobBuf);

        // Default
        const unsigned int crc32cDefault = bmqp::Crc32c::calculate(blob);
        // Software
        const unsigned int crc32cSoftware =
            bmqp::Crc32c_Impl::calculateSoftware(blob);

        const unsigned int expectedCrc32c = 0xD86F726E;
        BMQTST_ASSERT_EQ(crc32cDefault, expectedCrc32c);
        BMQTST_ASSERT_EQ(crc32cSoftware, expectedCrc32c);
    }
}

static void test8_calculateOnBlobWithPreviousCrc()

// ------------------------------------------------------------------------
// CALCULATE CRC32-C ON BLOB WITH PREVIOUS CRC
//
// Concerns:
//   Verify the correctness of calculating CRC32-C on a blob with a previous
//   CRC using both the default and software flavors.
//
// Plan:
//   - Calculate CRC32-C for a blob having no data buffers and pass along a
//     contrived previous CRC. Verify that the result is the previous CRC that
//     was passed.
//     the result corresponds to the 'null' CRC32-C.
//   - Calculate CRC32-C for a blob composed of various buffers and compare
//     the result to the known correct CRC32-C value for the buffer consisting
//     of the individual buffers concatenated.
//   - Calculate CRC32-C for a blob composed of one buffer of a typical size
//     and compare the result to the known correct CRC32-C value for the
//     buffer.
//
// Testing:
//   Correctness of CRC32-C calculation on an entire blob with a previous CRC
//   using both the default and software flavors.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "CALCULATE CRC32-C ON BLOB WITH PREVIOUS CRC");

    {
        // One empty blob (no data buffers) with a contrived previous crc

        bdlbb::Blob        blob(bmqtst::TestHelperUtil::allocator());
        const unsigned int prevCrc = 0xA0EA6901;

        // Assumption
        BSLS_ASSERT_OPT(blob.numDataBuffers() == 0);

        // Default
        const unsigned int crc32cDefault = bmqp::Crc32c::calculate(blob,
                                                                   prevCrc);
        // Software
        const unsigned int crc32cSoftware =
            bmqp::Crc32c_Impl::calculateSoftware(blob, prevCrc);

        BMQTST_ASSERT_EQ(crc32cDefault, prevCrc);
        BMQTST_ASSERT_EQ(crc32cSoftware, prevCrc);
    }

    {
        // Two blobs, each with data buffers of small size

        bdlbb::Blob blob1(bmqtst::TestHelperUtil::allocator());

        char                  one[] = "one";
        bsl::shared_ptr<char> onePtr;
        onePtr.reset(one,
                     bslstl::SharedPtrNilDeleter(),
                     bmqtst::TestHelperUtil::allocator());
        bdlbb::BlobBuffer buf1(onePtr, bsl::strlen(one));

        blob1.appendDataBuffer(buf1);

        bdlbb::Blob blob2(bmqtst::TestHelperUtil::allocator());

        char                  two[] = "two";
        bsl::shared_ptr<char> twoPtr;
        twoPtr.reset(two,
                     bslstl::SharedPtrNilDeleter(),
                     bmqtst::TestHelperUtil::allocator());
        bdlbb::BlobBuffer buf2(twoPtr, bsl::strlen(two));

        blob2.appendDataBuffer(buf2);

        char                  three[] = "three";
        bsl::shared_ptr<char> threePtr;
        threePtr.reset(three,
                       bslstl::SharedPtrNilDeleter(),
                       bmqtst::TestHelperUtil::allocator());
        bdlbb::BlobBuffer buf3(threePtr, bsl::strlen(three));

        blob2.appendDataBuffer(buf3);

        // Default
        unsigned int crc32cDefault = 0;

        crc32cDefault = bmqp::Crc32c::calculate(blob1, crc32cDefault);
        crc32cDefault = bmqp::Crc32c::calculate(blob2, crc32cDefault);

        // Software
        unsigned int crc32cSoftware = 0;

        crc32cSoftware = bmqp::Crc32c_Impl::calculateSoftware(blob1,
                                                              crc32cSoftware);
        crc32cSoftware = bmqp::Crc32c_Impl::calculateSoftware(blob2,
                                                              crc32cSoftware);

        const unsigned int expectedCrc32c = 0xA0EA6901;
        BMQTST_ASSERT_EQ(crc32cDefault, expectedCrc32c);
        BMQTST_ASSERT_EQ(crc32cSoftware, expectedCrc32c);
    }

    {
        // Two blobs, each with data buffers of typical size

        // One
        bdlbb::Blob blob1(bmqtst::TestHelperUtil::allocator());

        char one[] = "This will be put in a blob buffer of typical"
                     " size, and then we will test the crc32c calculation"
                     " (blob version) with only one blob buffer to ensure"
                     " that the logic of the loop works even for one blob"
                     " buffer. Moreover, append some lines bellow to increase"
                     " the size of this buffer.";
        bsl::shared_ptr<char> oneSP;
        oneSP.reset(one,
                    bslstl::SharedPtrNilDeleter(),
                    bmqtst::TestHelperUtil::allocator());
        bdlbb::BlobBuffer blobBuf1(oneSP, bsl::strlen(one));

        blob1.appendDataBuffer(blobBuf1);

        // Two
        bdlbb::Blob blob2(bmqtst::TestHelperUtil::allocator());

        char two[] = "#######################################################"
                     "#######################################################"
                     "#######################################################"
                     "#######################################################"
                     "#######################################################"
                     "#######################################################"
                     "#######################################################"
                     "#######################################################"
                     "#######################################################"
                     "#######################################################";

        bsl::shared_ptr<char> twoSP;
        twoSP.reset(two,
                    bslstl::SharedPtrNilDeleter(),
                    bmqtst::TestHelperUtil::allocator());
        bdlbb::BlobBuffer blobBuf2(twoSP, bsl::strlen(two));

        blob2.appendDataBuffer(blobBuf2);

        // Default
        unsigned int crc32cDefault = 0;

        crc32cDefault = bmqp::Crc32c::calculate(blob1, crc32cDefault);
        crc32cDefault = bmqp::Crc32c::calculate(blob2, crc32cDefault);

        // Software
        unsigned int crc32cSoftware = 0;

        crc32cSoftware = bmqp::Crc32c_Impl::calculateSoftware(blob1,
                                                              crc32cSoftware);
        crc32cSoftware = bmqp::Crc32c_Impl::calculateSoftware(blob2,
                                                              crc32cSoftware);

        const unsigned int expectedCrc32c = 0xD86F726E;
        BMQTST_ASSERT_EQ(crc32cDefault, expectedCrc32c);
        BMQTST_ASSERT_EQ(crc32cSoftware, expectedCrc32c);
    }
}
// ============================================================================
//                              PERFORMANCE TESTS
// ----------------------------------------------------------------------------
BSLA_MAYBE_UNUSED
static void testN1_performanceDefaultUserInput()
// ------------------------------------------------------------------------
// PERFORMANCE: CALCULATE CRC32-C DEFAULT ON USER INPUT
//
// Concerns:
//   Test the performance of
//       bmqp::Crc32c::calculate(const void   *data,
//                               unsigned int  length);
//   on an user input.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "PERFORMANCE: CALCULATE CRC32-C DEFAULT ON USER INPUT");

    cout << "Please enter the string:" << endl << "> ";

    // Read from stdin
    bsl::string input(bmqtst::TestHelperUtil::allocator());
    bsl::getline(bsl::cin, input);

    const size_t       k_NUM_ITERS = 100000;  // 100K
    unsigned int       crc32c      = 0;
    bsls::Types::Int64 startTime   = bsls::TimeUtil::getTimer();
    for (size_t i = 0; i < k_NUM_ITERS; ++i) {
        crc32c = bmqp::Crc32c::calculate(input.c_str(), input.size());
    }
    bsls::Types::Int64 endTime = bsls::TimeUtil::getTimer();

    const unsigned char* sum = reinterpret_cast<const unsigned char*>(&crc32c);

    printf("\nCRC32-C : %02x%02x%02x%02x\n", sum[0], sum[1], sum[2], sum[3]);

    cout << "Average Time (nano sec): " << (endTime - startTime) / k_NUM_ITERS
         << "\n\n";
}

static void testN2_performanceDefault()
// ------------------------------------------------------------------------
// PERFORMANCE: CALCULATE CRC32-C ON BUFFER DEFAULT
//
// Concerns:
//   Test the performance of bmqp::Crc32c::calculate(const void  *data,
//                                                   unsigned int length);
//   in a single thread environment.  On a supported platform (see 'Support
//   for Hardware Acceleration' in the header file), this will use a
//   hardware-accelerated implementation.  Otherwise, it will use a
//   portable software implementation.
//
// Plan:
//   - Time a large number of CRC32-C calculations for buffers of varying
//     sizes in a single thread and take the average.  Compare the average
//     to that of 'bdlde::crc32'.
//
// Testing:
//   Performance of calculating CRC32-C using the default implementation.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "PERFORMANCE: CALCULATE CRC32-C ON BUFFER DEFAULT");

    const int k_NUM_ITERS = 100000;  // 100K

    bsl::vector<int> bufferLengths(bmqtst::TestHelperUtil::allocator());
    const int        k_MAX_SIZE = populateBufferLengthsSorted(&bufferLengths);

    // Read in random input
    char* buffer = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(k_MAX_SIZE));
    bsl::generate_n(buffer, k_MAX_SIZE, bsl::rand);

    // Measure calculation time and report
    bsl::vector<TableRecord> tableRecords(bmqtst::TestHelperUtil::allocator());
    for (unsigned i = 0; i < bufferLengths.size(); ++i) {
        const int length = bufferLengths[i];

        bsl::cout << "---------------------\n"
                  << " SIZE = " << length << '\n'
                  << " ITERATIONS = " << k_NUM_ITERS << '\n'
                  << "---------------------\n";

        //===================================================================//
        //                        [1] Crc32c (default)
        unsigned int crc32c = bmqp::Crc32c::calculate(buffer, length);

        // <time>
        bsls::Types::Int64 startTime = bsls::TimeUtil::getTimer();
        for (unsigned int l = 0; l < k_NUM_ITERS; ++l) {
            crc32c = bmqp::Crc32c::calculate(buffer, length);
        }
        bsls::Types::Int64 t1 = bsls::TimeUtil::getTimer() - startTime;
        // </time>

        //===================================================================//
        //                         [2] BDE bdlde::crc32
        bdlde::Crc32 crcBde(buffer, length);
        unsigned int crc32 = crcBde.checksumAndReset();

        // <time>
        startTime = bsls::TimeUtil::getTimer();
        for (unsigned int l = 0; l < k_NUM_ITERS; ++l) {
            crcBde.update(buffer, length);
            crc32 = crcBde.checksumAndReset();
        }
        bsls::Types::Int64 t2 = bsls::TimeUtil::getTimer() - startTime;
        // </time>

        //===================================================================//
        //                            Report
        TableRecord record;
        record.d_size    = length;
        record.d_timeOne = t1 / k_NUM_ITERS;
        record.d_timeTwo = t2 / k_NUM_ITERS;
        record.d_ratio   = static_cast<double>(t2) / static_cast<double>(t1);

        tableRecords.push_back(record);

        unsigned char* sum;
        bsl::cout << "Checksum\n";
        sum = reinterpret_cast<unsigned char*>(&crc32c);
        printf("  CRC32-C (Default): %02x%02x%02x%02x\n",
               sum[0],
               sum[1],
               sum[2],
               sum[3]);
        sum = reinterpret_cast<unsigned char*>(&crc32);
        printf("  CRC32 (BDE)      : %02x%02x%02x%02x\n",
               sum[0],
               sum[1],
               sum[2],
               sum[3]);

        bsl::cout << "Average Time(ns)\n"
                  << "  Default                       : " << record.d_timeOne
                  << '\n'
                  << "  bdlde::Crc32                  : " << record.d_timeTwo
                  << '\n'
                  << "  Ratio (bdlde::Crc32 / Default): " << record.d_ratio
                  << "\n\n";
    }
    // Print performance comparison table
    bsl::vector<bsl::string> headerCols;
    headerCols.emplace_back("Size(B)");
    headerCols.emplace_back("Def time(ns)");
    headerCols.emplace_back("bdlde::crc32 time(ns)");
    headerCols.emplace_back("Ratio(bdlde::crc32 / Def)");

    printTable(bsl::cout, headerCols, tableRecords);

    bmqtst::TestHelperUtil::allocator()->deallocate(buffer);
}

BSLA_MAYBE_UNUSED
static void testN3_defaultCalculateThroughput()
// ------------------------------------------------------------------------
// BENCHMARK: CALCULATE CRC32-C THROUGHPUT DEFAULT
//
// Concerns:
//   Test the throughput (GB/s) of CRC32-C calculation using the default
//   implementation in a single thread environment.  On a
//   supported platform (see 'Support for Hardware Acceleration' in the
//   header file), the default method will use a hardware-accelerated
//   implementation.
//
// Plan:
//   - Time a large number of CRC32-C calculations using the default
//     implementation for a buffer of fixed size in a single
//     thread and take the average for each implementation.  Calculate the
//     throughput using its average.
//
// Testing:
//   Throughput (GB/s) of CRC32-C calculation using the default
//   implementation in a single thread environment.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "BENCHMARK: CALCULATE CRC32-C THROUGPUT DEFAULT");

    const bsls::Types::Uint64 k_NUM_ITERS = 1000000;  // 1M
    unsigned int              resultDef   = 0;
    const int                 bufLen      = 12345;
    char*                     buf         = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(bufLen));
    bsl::memset(buf, 'x', bufLen);
    //=======================================================================//
    //              [1] Crc32c (default -- hardware on linux)
    resultDef = bmqp::Crc32c::calculate(buf, bufLen);

    // <time>
    bsls::Types::Int64 startDef = bsls::TimeUtil::getTimer();
    for (size_t i = 0; i < k_NUM_ITERS; ++i) {
        resultDef = bmqp::Crc32c::calculate(buf, bufLen);
    }
    bsls::Types::Int64 diffDef = bsls::TimeUtil::getTimer() - startDef;
    // </time>

    //=======================================================================//
    //                        [4] Report

    cout << endl;
    cout << "=========================\n";
#ifdef BSLS_PLATFORM_CPU_64_BIT
    cout << "DEFAULT {SSE 4.2, 64 bit}\n";
#else
    cout << "DEFAULT {SSE 4.2, 32 bit}\n";
#endif
    cout << "=========================\n";
    cout << "For a payload of length " << bufLen << ", completed "
         << k_NUM_ITERS << " HW-version iterations in "
         << bmqu::PrintUtil::prettyTimeInterval(diffDef) << ".\n"
         << "Above implies that 1 HW-version iteration was calculated in "
         << diffDef / k_NUM_ITERS << " nano seconds.\n"
         << "In other words: "
         << bmqu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                (k_NUM_ITERS * bdlt::TimeUnitRatio::k_NS_PER_S) / diffDef))
         << " HW-version iterations per second." << endl
         << "HW-version throughput: "
         << bmqu::PrintUtil::prettyBytes(
                (k_NUM_ITERS * bdlt::TimeUnitRatio::k_NS_PER_S * bufLen) /
                diffDef)
         << " per second.\n\n"
         << endl;

    bmqtst::TestHelperUtil::allocator()->deallocate(buf);
    static_cast<void>(resultDef);
}

BSLA_MAYBE_UNUSED
static void testN3_softwareCalculateThroughput()
// ------------------------------------------------------------------------
// BENCHMARK: CALCULATE CRC32-C THROUGHPUT SOFTWARE
//
// Concerns:
//   Test the throughput (GB/s) of CRC32-C calculation using the default
//   and software implementations in a single thread environment.
//
// Plan:
//   - Time a large number of CRC32-C calculations using the
//     software implementation for a buffer of fixed size in a single
//     thread and take the average.  Calculate the
//     throughput using its average.
//
// Testing:
//   Throughput (GB/s) of CRC32-C calculation using the
//   software implementation in a single thread environment.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "BENCHMARK: CALCULATE CRC32-C THROUGPUT SOFTWARE");

    const bsls::Types::Uint64 k_NUM_ITERS = 1000000;  // 1M
    unsigned int              resultSW    = 0;
    const int                 bufLen      = 12345;
    char*                     buf         = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(bufLen));
    bsl::memset(buf, 'x', bufLen);
    //=======================================================================//
    //                        [2] Crc32c (software)
    resultSW = bmqp::Crc32c_Impl::calculateSoftware(buf, bufLen);

    // <time>
    bsls::Types::Int64 startSW = bsls::TimeUtil::getTimer();
    for (size_t i = 0; i < k_NUM_ITERS; ++i) {
        resultSW = bmqp::Crc32c_Impl::calculateSoftware(buf, bufLen);
    }
    bsls::Types::Int64 diffSW = bsls::TimeUtil::getTimer() - startSW;
    // </time>

    cout << "=========================\n";
    cout << "        SOFTWARE         \n";
    cout << "=========================\n";
    cout << "For a payload of length " << bufLen << ", completed "
         << k_NUM_ITERS << " SW-version iterations in "
         << bmqu::PrintUtil::prettyTimeInterval(diffSW) << ".\n"
         << "Above implies that 1 SW-version iteration was calculated in "
         << diffSW / k_NUM_ITERS << " nano seconds.\n"
         << "In other words: "
         << bmqu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                (k_NUM_ITERS * bdlt::TimeUnitRatio::k_NS_PER_S) / diffSW))
         << " SW-version iterations per second." << endl
         << "SW-version throughput: "
         << bmqu::PrintUtil::prettyBytes(
                (k_NUM_ITERS * bdlt::TimeUnitRatio::k_NS_PER_S * bufLen) /
                diffSW)
         << " per second.\n"
         << endl
         << endl;

    bmqtst::TestHelperUtil::allocator()->deallocate(buf);
    static_cast<void>(resultSW);
}

BSLA_MAYBE_UNUSED
static void testN3_bdldCalculateThroughput()
// ------------------------------------------------------------------------
// BENCHMARK: CALCULATE CRC32-C THROUGHPUT BDE
//
// Concerns:
//   Test the throughput (GB/s) of CRC32-C calculation using the
//   BDE implementation in a single thread environment.
//
// Plan:
//   - Time a large number of CRC32-C calculations using the default and
//     software implementations for a buffer of fixed size in a single
//     thread and take the average for each implementation.  Calculate the
//     throughput of each implementation using its average.
//
// Testing:
//   Throughput (GB/s) of CRC32-C calculation using the
//   BDE implementation in a single thread environment.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "BENCHMARK: CALCULATE CRC32-C THROUGPUT BDE");

    const bsls::Types::Uint64 k_NUM_ITERS = 1000000;  // 1M
    unsigned int              resultBde   = 0;
    const int                 bufLen      = 12345;
    char*                     buf         = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(bufLen));
    bsl::memset(buf, 'x', bufLen);
    //=======================================================================//
    //                           [3] BDE bdlde::crc32
    bdlde::Crc32 crcBde(buf, bufLen);
    resultBde = crcBde.checksumAndReset();

    // <time>
    bsls::Types::Int64 startBde = bsls::TimeUtil::getTimer();
    for (size_t i = 0; i < k_NUM_ITERS; ++i) {
        crcBde.update(buf, bufLen);
        resultBde = crcBde.checksumAndReset();
    }
    bsls::Types::Int64 diffBde = bsls::TimeUtil::getTimer() - startBde;
    // </time>

    //=======================================================================//
    //                        [4] Report

    cout << "==========================\n";
    cout << "     BDE bdlde::crc32     \n";
    cout << "==========================\n";
    cout << "For a payload of length " << bufLen << ", completed "
         << k_NUM_ITERS << " BDE-version iterations in "
         << bmqu::PrintUtil::prettyTimeInterval(diffBde) << ".\n"
         << "Above implies that 1 BDE-version iteration was calculated in "
         << diffBde / k_NUM_ITERS << " nano seconds.\n"
         << "In other words: "
         << bmqu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                (k_NUM_ITERS * bdlt::TimeUnitRatio::k_NS_PER_S) / diffBde))
         << " BDE-version iterations per second." << endl
         << "BDE-version throughput: "
         << bmqu::PrintUtil::prettyBytes(
                (k_NUM_ITERS * bdlt::TimeUnitRatio::k_NS_PER_S * bufLen) /
                diffBde)
         << " per second.\n"
         << endl
         << endl;

    bmqtst::TestHelperUtil::allocator()->deallocate(buf);
    static_cast<void>(resultBde);
}

BSLA_MAYBE_UNUSED
static void testN4_calculateSerialDefault()
// ------------------------------------------------------------------------
// PERFORMANCE: CALCULATE CRC32-C DEFAULT SERIAL
//
// Concerns:
//   Test the performance of
//       bmqp::Crc32c::calculate(const void   *data,
//                               unsigned int  length);
//
// Plan:
//   - Time a large number of crc32c calculations for buffers of varying
//     sizes, single threaded.
//
// Testing:
//   Performance of crc32c calculation using the default implementation
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "PERFORMANCE: CALCULATE CRC32-C DEFAULT");

    const int k_NUM_ITERS = 100000;  // 100K

    bsl::vector<int> bufferLengths(bmqtst::TestHelperUtil::allocator());
    const int        k_MAX_SIZE = populateBufferLengthsSorted(&bufferLengths);

    // Read in random input
    char* buffer = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(k_MAX_SIZE));
    bsl::generate_n(buffer, k_MAX_SIZE, bsl::rand);

    // Measure calculation time and report
    bsl::vector<TableRecord> tableRecords(bmqtst::TestHelperUtil::allocator());
    for (unsigned i = 0; i < bufferLengths.size(); ++i) {
        const int length = bufferLengths[i];

        bsl::cout << "---------------------\n"
                  << " SIZE = " << length << '\n'
                  << " ITERATIONS = " << k_NUM_ITERS << '\n'
                  << "---------------------\n";

        //===============================================================//
        //                     [1] Crc32c (default)
        unsigned int crc32c;
        // <time>
        bsls::Types::Int64 startDef = bsls::TimeUtil::getTimer();
        for (size_t k = 0; k < k_NUM_ITERS; ++k) {
            crc32c = bmqp::Crc32c::calculate(buffer, length);
        }
        bsls::Types::Int64 endDef = bsls::TimeUtil::getTimer();
        // </time>
        TableRecord record;
        record.d_size    = length;
        record.d_timeOne = (endDef - startDef) / k_NUM_ITERS;
        record.d_timeTwo = 0;
        record.d_ratio   = 0;
        tableRecords.push_back(record);

        const unsigned char* sum;
        bsl::cout << "Checksum\n";
        sum = reinterpret_cast<const unsigned char*>(&crc32c);
        printf("  CRC32-C (Default)  : %02x%02x%02x%02x\n",
               sum[0],
               sum[1],
               sum[2],
               sum[3]);

        bsl::cout << "Average Times(ns)\n"
                  << "  CRC32-C (Default)          : " << record.d_timeOne
                  << '\n';
    }
    // Print performance comparison table
    bsl::vector<bsl::string> headerCols;
    headerCols.emplace_back("Size(B)");
    headerCols.emplace_back("Default time(ns)");
    headerCols.emplace_back("Hardware Serial time(ns)");
    headerCols.emplace_back("Ratio(HardwareSerial / Default)");

    printTable(bsl::cout, headerCols, tableRecords);

    bmqtst::TestHelperUtil::allocator()->deallocate(buffer);
}

BSLA_MAYBE_UNUSED
static void testN4_calculateHardwareSerial()
// ------------------------------------------------------------------------
// PERFORMANCE: CALCULATE CRC32-C HARDWARE SERIAL
//
// Concerns:
//   Test the performance of
//       bmqp::Crc32c_Impl::calculateHardwareSerial(const void   *data,
//                                                  unsigned int  length);
//
// Plan:
//   - Time a large number of crc32c calculations for buffers of varying
//     sizes, single threaded.
//
// Testing:
//   Performance of crc32c calculation using the 'hardware serial' approach
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "PERFORMANCE: CALCULATE CRC32-C HARDWARE SERIAL");

    const int k_NUM_ITERS = 100000;  // 100K

    bsl::vector<int> bufferLengths(bmqtst::TestHelperUtil::allocator());
    const int        k_MAX_SIZE = populateBufferLengthsSorted(&bufferLengths);

    // Read in random input
    char* buffer = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(k_MAX_SIZE));
    bsl::generate_n(buffer, k_MAX_SIZE, bsl::rand);

    // Measure calculation time and report
    bsl::vector<TableRecord> tableRecords(bmqtst::TestHelperUtil::allocator());
    for (unsigned i = 0; i < bufferLengths.size(); ++i) {
        const int length = bufferLengths[i];

        bsl::cout << "---------------------\n"
                  << " SIZE = " << length << '\n'
                  << " ITERATIONS = " << k_NUM_ITERS << '\n'
                  << "---------------------\n";

        //===============================================================//
        //                [2] Crc32c (hardware serial)
        unsigned int crcHS;
        // <time>
        bsls::Types::Int64 startHS = bsls::TimeUtil::getTimer();
        for (size_t k = 0; k < k_NUM_ITERS; ++k) {
            crcHS = bmqp::Crc32c_Impl::calculateHardwareSerial(buffer, length);
        }
        bsls::Types::Int64 endHS = bsls::TimeUtil::getTimer();
        // </time>
        //===============================================================//
        //                            Report

        TableRecord record;
        record.d_size    = length;
        record.d_timeOne = 0;
        record.d_timeTwo = (endHS - startHS) / k_NUM_ITERS;
        record.d_ratio   = 0;
        tableRecords.push_back(record);

        const unsigned char* sum;
        bsl::cout << "Checksum\n";
        sum = reinterpret_cast<const unsigned char*>(&crcHS);
        printf("  CRC32-C (HW Serial): %02x%02x%02x%02x\n",
               sum[0],
               sum[1],
               sum[2],
               sum[3]);

        bsl::cout << "Average Times(ns)\n"
                  << "  CRC32-C (HW Serial)        : " << record.d_timeTwo
                  << '\n';
    }
    // Print performance comparison table
    bsl::vector<bsl::string> headerCols;
    headerCols.emplace_back("Size(B)");
    headerCols.emplace_back("Default time(ns)");
    headerCols.emplace_back("Hardware Serial time(ns)");
    headerCols.emplace_back("Ratio(HardwareSerial / Default)");

    printTable(bsl::cout, headerCols, tableRecords);

    bmqtst::TestHelperUtil::allocator()->deallocate(buffer);
}

BSLA_MAYBE_UNUSED
static void testN5_bmqpPerformanceSoftware()
// ------------------------------------------------------------------------
// PERFORMANCE: CALCULATE CRC32-C ON BUFFER SOFTWARE
//
// Concerns:
//   Test the performance of bmqp::Crc32c_Impl::calculateSoftware(
//                                                    const void  *data,
//                                                    unsigned int length);
//   in a single thread environment. This will explicitly use the software
//   implementation.
//
// Plan:
//   - Time a large number of CRC32-C calculations for buffers of varying
//     sizes in a single thread and take the average.
//
// Testing:
//   Performance of calculating CRC32-C using the software implementation.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "PERFORMANCE: CALCULATE CRC32-C ON BUFFER SOFTWARE");

    const int k_NUM_ITERS = 100000;  // 100K

    bsl::vector<int> bufferLengths(bmqtst::TestHelperUtil::allocator());
    const int        k_MAX_SIZE = populateBufferLengthsSorted(&bufferLengths);

    // Read in random input
    char* buffer = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(k_MAX_SIZE));
    bsl::generate_n(buffer, k_MAX_SIZE, bsl::rand);

    // Measure calculation time and report
    bsl::vector<TableRecord> tableRecords(bmqtst::TestHelperUtil::allocator());
    for (unsigned i = 0; i < bufferLengths.size(); ++i) {
        const int length = bufferLengths[i];

        bsl::cout << "---------------------\n"
                  << " SIZE = " << length << '\n'
                  << " ITERATIONS = " << k_NUM_ITERS << '\n'
                  << "---------------------\n";

        //===================================================================//
        //                        [1] Crc32c (software)
        unsigned int crc32c = bmqp::Crc32c_Impl::calculateSoftware(buffer,
                                                                   length);
        // <time>
        bsls::Types::Int64 startTime = bsls::TimeUtil::getTimer();
        for (unsigned int l = 0; l < k_NUM_ITERS; ++l) {
            crc32c = bmqp::Crc32c_Impl::calculateSoftware(buffer, length);
        }
        bsls::Types::Int64 t1 = bsls::TimeUtil::getTimer() - startTime;
        // </time>

        //===================================================================//
        //                            Report
        TableRecord record;
        record.d_size    = length;
        record.d_timeOne = t1 / k_NUM_ITERS;
        record.d_timeTwo = 0;
        record.d_ratio   = 0;
        tableRecords.push_back(record);

        unsigned char* sum;
        bsl::cout << "Checksum\n";
        sum = reinterpret_cast<unsigned char*>(&crc32c);
        printf("  CRC32-C (Software): %02x%02x%02x%02x\n",
               sum[0],
               sum[1],
               sum[2],
               sum[3]);

        bsl::cout << "Average Time(ns)\n"
                  << "  Software                       : " << record.d_timeOne
                  << '\n';
    }
    // Print performance comparison table
    bsl::vector<bsl::string> headerCols;
    headerCols.emplace_back("Size(B)");
    headerCols.emplace_back("SW time(ns)");
    headerCols.emplace_back("bdlde::crc32 time(ns)");
    headerCols.emplace_back("Ratio(bdlde::crc32 / SW)");
    printTable(bsl::cout, headerCols, tableRecords);

    bmqtst::TestHelperUtil::allocator()->deallocate(buffer);
}

BSLA_MAYBE_UNUSED
static void testN5_bdldPerformanceSoftware()
// ------------------------------------------------------------------------
// PERFORMANCE: CALCULATE CRC32-C ON BUFFER SOFTWARE
//
// Concerns:
//   Test the performance of bdlde::crc32(
//                                        const void  *data,
//                                        unsigned int length);
//   in a single thread environment. This will explicitly use the software
//   implementation.
//
// Plan:
//   - Time a large number of CRC32-C calculations for buffers of varying
//     sizes in a single thread and take the average.
//
// Testing:
//   Performance of calculating CRC32-C using the bdlde::crc32 implementation.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "PERFORMANCE: CALCULATE CRC32-C ON BUFFER SOFTWARE");

    const int k_NUM_ITERS = 100000;  // 100K

    bsl::vector<int> bufferLengths(bmqtst::TestHelperUtil::allocator());
    const int        k_MAX_SIZE = populateBufferLengthsSorted(&bufferLengths);

    // Read in random input
    char* buffer = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(k_MAX_SIZE));
    bsl::generate_n(buffer, k_MAX_SIZE, bsl::rand);

    // Measure calculation time and report
    bsl::vector<TableRecord> tableRecords(bmqtst::TestHelperUtil::allocator());
    for (unsigned i = 0; i < bufferLengths.size(); ++i) {
        const int length = bufferLengths[i];

        bsl::cout << "---------------------\n"
                  << " SIZE = " << length << '\n'
                  << " ITERATIONS = " << k_NUM_ITERS << '\n'
                  << "---------------------\n";

        //===================================================================//
        //                         [2] BDE bdlde::crc32
        bdlde::Crc32 crcBde(buffer, length);
        unsigned int crc32 = crcBde.checksumAndReset();

        // <time>
        bsls::Types::Int64 startTime = bsls::TimeUtil::getTimer();
        for (unsigned int l = 0; l < k_NUM_ITERS; ++l) {
            crcBde.update(buffer, length);
            crc32 = crcBde.checksumAndReset();
        }
        bsls::Types::Int64 t2 = bsls::TimeUtil::getTimer() - startTime;
        // </time>

        //===================================================================//
        //                            Report
        TableRecord record;
        record.d_size    = length;
        record.d_timeTwo = t2 / k_NUM_ITERS;

        tableRecords.push_back(record);

        unsigned char* sum;
        bsl::cout << "Checksum\n";
        sum = reinterpret_cast<unsigned char*>(&crc32);
        printf("  CRC32 (BDE)       : %02x%02x%02x%02x\n",
               sum[0],
               sum[1],
               sum[2],
               sum[3]);

        bsl::cout << "Average Time(ns)\n"
                  << "  bdlde::Crc32                   : " << record.d_timeTwo
                  << '\n';
    }
    // Print performance comparison table
    bsl::vector<bsl::string> headerCols;
    headerCols.emplace_back("Size(B)");
    headerCols.emplace_back("SW time(ns)");
    headerCols.emplace_back("bdlde::crc32 time(ns)");
    headerCols.emplace_back("Ratio(bdlde::crc32 / SW)");

    printTable(bsl::cout, headerCols, tableRecords);

    bmqtst::TestHelperUtil::allocator()->deallocate(buffer);
}

BSLA_MAYBE_UNUSED
static void testN6_bmqpPerformanceDefault()
// ------------------------------------------------------------------------
// PERFORMANCE: CALCULATE CRC32-C ON BUFFER DEFAULT
//
// Concerns:
//   Test the performance of bmqp::Crc32c::calculate(const void  *data,
//                                                   unsigned int length);
//   in a single thread environment.  On a supported platform (see 'Support
//   for Hardware Acceleration' in the header file), this will use a
//   hardware-accelerated implementation.  Otherwise, it will use a
//   portable software implementation.
//
// Plan:
//   - Time a large number of CRC32-C calculations for buffers of varying
//     sizes in a single thread and take the average.
//
// Testing:
//   Performance of calculating CRC32-C using the default implementation.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    bmqtst::TestHelperUtil::ignoreCheckGblAlloc() = true;
    bmqtst::TestHelper::printTestName(
        "PERFORMANCE: CALCULATE CRC32-C ON BUFFER DEFAULT");

    const int k_NUM_ITERS = 100000;  // 100K

    bsl::vector<int> bufferLengths(bmqtst::TestHelperUtil::allocator());
    const int        k_MAX_SIZE = populateBufferLengthsSorted(&bufferLengths);

    // Read in random input
    char* buffer = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(k_MAX_SIZE));
    bsl::generate_n(buffer, k_MAX_SIZE, bsl::rand);

    // Measure calculation time and report
    bsl::vector<TableRecord> tableRecords(bmqtst::TestHelperUtil::allocator());
    for (unsigned i = 0; i < bufferLengths.size(); ++i) {
        const int length = bufferLengths[i];

        bsl::cout << "---------------------\n"
                  << " SIZE = " << length << '\n'
                  << " ITERATIONS = " << k_NUM_ITERS << '\n'
                  << "---------------------\n";

        //===================================================================//
        //                        [1] Crc32c (default)
        unsigned int crc32c = bmqp::Crc32c::calculate(buffer, length);

        // <time>
        bsls::Types::Int64 startTime = bsls::TimeUtil::getTimer();
        for (unsigned int l = 0; l < k_NUM_ITERS; ++l) {
            crc32c = bmqp::Crc32c::calculate(buffer, length);
        }
        bsls::Types::Int64 t1 = bsls::TimeUtil::getTimer() - startTime;
        // </time>

        //===================================================================//
        //                            Report
        TableRecord record;
        record.d_size    = length;
        record.d_timeOne = t1 / k_NUM_ITERS;
        record.d_timeTwo = 0;
        record.d_ratio   = 0;
        tableRecords.push_back(record);

        unsigned char* sum;
        bsl::cout << "Checksum\n";
        sum = reinterpret_cast<unsigned char*>(&crc32c);
        printf("  CRC32-C (Default): %02x%02x%02x%02x\n",
               sum[0],
               sum[1],
               sum[2],
               sum[3]);

        bsl::cout << "Average Time(ns)\n"
                  << "  Default                       : " << record.d_timeOne
                  << '\n';
    }
    // Print performance comparison table
    bsl::vector<bsl::string> headerCols;
    headerCols.emplace_back("Size(B)");
    headerCols.emplace_back("Def time(ns)");
    headerCols.emplace_back("bdlde::crc32 time(ns)");
    headerCols.emplace_back("Ratio(bdlde::crc32 / Def)");
    printTable(bsl::cout, headerCols, tableRecords);

    bmqtst::TestHelperUtil::allocator()->deallocate(buffer);
}

BSLA_MAYBE_UNUSED
static void testN6_bdldPerformanceDefault()
// ------------------------------------------------------------------------
// PERFORMANCE: CALCULATE CRC32-C ON BUFFER DEFAULT
//
// Concerns:
//   Test the performance of bdlde::Crc32c::(const void  *data,
//                                          unsigned int length);
//   in a single thread environment.  On a supported platform (see 'Support
//   for Hardware Acceleration' in the header file), this will use a
//   hardware-accelerated implementation.  Otherwise, it will use a
//   portable software implementation.
//
// Plan:
//   - Time a large number of CRC32-C calculations for buffers of varying
//     sizes in a single thread and take the average.
//
// Testing:
//   Performance of calculating CRC32-C using the BDE implementation.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    bmqtst::TestHelperUtil::ignoreCheckGblAlloc() = true;
    bmqtst::TestHelper::printTestName(
        "PERFORMANCE: CALCULATE CRC32-C ON BUFFER BDE");

    const int k_NUM_ITERS = 100000;  // 100K

    bsl::vector<int> bufferLengths(bmqtst::TestHelperUtil::allocator());
    const int        k_MAX_SIZE = populateBufferLengthsSorted(&bufferLengths);

    // Read in random input
    char* buffer = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(k_MAX_SIZE));
    bsl::generate_n(buffer, k_MAX_SIZE, bsl::rand);

    // Measure calculation time and report
    bsl::vector<TableRecord> tableRecords(bmqtst::TestHelperUtil::allocator());
    for (unsigned i = 0; i < bufferLengths.size(); ++i) {
        const int length = bufferLengths[i];

        bsl::cout << "---------------------\n"
                  << " SIZE = " << length << '\n'
                  << " ITERATIONS = " << k_NUM_ITERS << '\n'
                  << "---------------------\n";

        //===================================================================//
        //                         [2] BDE bdlde::crc32
        bdlde::Crc32 crcBde(buffer, length);
        unsigned int crc32 = crcBde.checksumAndReset();

        // <time>
        bsls::Types::Int64 startTime = bsls::TimeUtil::getTimer();
        for (unsigned int l = 0; l < k_NUM_ITERS; ++l) {
            crcBde.update(buffer, length);
            crc32 = crcBde.checksumAndReset();
        }
        bsls::Types::Int64 t2 = bsls::TimeUtil::getTimer() - startTime;
        // </time>

        //===================================================================//
        //                            Report
        TableRecord record;
        record.d_size    = length;
        record.d_timeOne = 0;
        record.d_timeTwo = t2 / k_NUM_ITERS;
        record.d_ratio   = 0;
        tableRecords.push_back(record);

        unsigned char* sum;
        bsl::cout << "Checksum\n";
        sum = reinterpret_cast<unsigned char*>(&crc32);
        printf("  CRC32 (BDE)      : %02x%02x%02x%02x\n",
               sum[0],
               sum[1],
               sum[2],
               sum[3]);

        bsl::cout << "Average Time(ns)\n"
                  << "  bdlde::Crc32                  : " << record.d_timeTwo
                  << '\n';
    }
    // Print performance comparison table
    bsl::vector<bsl::string> headerCols;
    headerCols.emplace_back("Size(B)");
    headerCols.emplace_back("Def time(ns)");
    headerCols.emplace_back("bdlde::crc32 time(ns)");
    headerCols.emplace_back("Ratio(bdlde::crc32 / Def)");

    printTable(bsl::cout, headerCols, tableRecords);

    bmqtst::TestHelperUtil::allocator()->deallocate(buffer);
}

#ifdef BMQTST_BENCHMARK_ENABLED

static void
testN1_performanceDefaultUserInput_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// PERFORMANCE: CALCULATE CRC32-C DEFAULT ON USER INPUT
//
// Concerns:
//   Test the performance of
//       bmqp::Crc32c::calculate()
//   on an user input.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "GOOGLE BENCHMARK PERFORMANCE: "
        "CALCULATE CRC32-C DEFAULT ON USER INPUT");

    cout << "Please enter the string:" << endl << "> ";

    // Read from stdin
    bsl::string input(bmqtst::TestHelperUtil::allocator());
    bsl::getline(bsl::cin, input);

    for (auto _ : state) {
        const size_t k_NUM_ITERS = 100000;  // 100K
        for (size_t i = 0; i < k_NUM_ITERS; ++i) {
            bmqp::Crc32c::calculate(input.c_str(), input.size());
        }
    }
}

static void
testN3_defaultCalculateThroughput_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// BENCHMARK: CALCULATE CRC32-C THROUGHPUT DEFAULT
//
// Concerns:
//   Test the throughput (GB/s) of CRC32-C calculation using the default
//   implementation in a single thread environment.  On a
//   supported platform (see 'Support for Hardware Acceleration' in the
//   header file), the default method will use a hardware-accelerated
//   implementation.  Otherwise, the default method will use a portable
//   software implementation.
//
// Plan:
//   - Time a large number of CRC32-C calculations using the default and
//     Calculate the throughput using its average.
//
// Testing:
//   Throughput (GB/s) of CRC32-C calculation using the default
//   implementation in a single thread environment.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK: "
                                      "CALCULATE CRC32-C THROUGHPUT DEFAULT");
    for (auto _ : state) {
        const int bufLen = 12345;
        char*     buf    = static_cast<char*>(
            bmqtst::TestHelperUtil::allocator()->allocate(bufLen));
        bsl::memset(buf, 'x', bufLen);
        bmqp::Crc32c::calculate(buf, bufLen);

        // <time>
        for (int i = 0; i < state.range(0); ++i) {
            bmqp::Crc32c::calculate(buf, bufLen);
        }
        bmqtst::TestHelperUtil::allocator()->deallocate(buf);
    }
}

static void
testN3_softwareCalculateThroughput_GoogleBenchmark(benchmark::State& state)
{
    //=======================================================================//
    //                        [2] Crc32c (software)
    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK: "
                                      "CALCULATE CRC32-C THROUGHPUT SOFTWARE");
    for (auto _ : state) {
        const int bufLen = 12345;
        char*     buf    = static_cast<char*>(
            bmqtst::TestHelperUtil::allocator()->allocate(bufLen));
        bsl::memset(buf, 'x', bufLen);
        bmqp::Crc32c_Impl::calculateSoftware(buf, bufLen);

        // <time>
        for (int i = 0; i < state.range(0); ++i) {
            bmqp::Crc32c_Impl::calculateSoftware(buf, bufLen);
        }
        bmqtst::TestHelperUtil::allocator()->deallocate(buf);
    }
    // </time>
}

static void
testN3_bdldCalculateThroughput_GoogleBenchmark(benchmark::State& state)
{
    //=======================================================================//
    //                           [3] BDE bdlde::crc32
    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK: "
                                      "CALCULATE CRC32-C THROUGPUT BDE");

    const int bufLen = 12345;
    char*     buf    = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(bufLen));
    bsl::memset(buf, 'x', bufLen);
    bdlde::Crc32 crcBde(buf, bufLen);

    // <time>
    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            crcBde.update(buf, bufLen);
            crcBde.checksumAndReset();
        }
    }
    // </time>
    bmqtst::TestHelperUtil::allocator()->deallocate(buf);
}

static void
testN4_calculateSerialDefault_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// PERFORMANCE: CALCULATE CRC32-C DEFAULT
//
// Concerns:
//   Test the performance of
//       bmqp::Crc32c::calculate()
//   and compare its performance to 'hardware serial' based approach in the
//   below function
//
// Plan:
//   - Time a large number of crc32c calculations for buffers of varying
//     sizes, single threaded.
//
// Testing:
//   Performance of crc32c calculation using the default implementation
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK PERFORMANCE: "
                                      "CALCULATE CRC32-C DEFAULT");

    const int k_MAX_SIZE = 67108864;  // 64 Mi

    // Read in random input
    char* buffer = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(k_MAX_SIZE));
    bsl::generate_n(buffer, k_MAX_SIZE, bsl::rand);

    // Measure calculation time and report

    //===============================================================//
    //                     [1] Crc32c (default)
    // <time>
    for (auto _ : state) {
        const int length = state.range(0);
        for (int k = 0; k < length; ++k) {
            bmqp::Crc32c::calculate(buffer, length);
        }
    }
    // </time>
    bmqtst::TestHelperUtil::allocator()->deallocate(buffer);
}

static void
testN4_calculateHardwareSerial_GoogleBenchmark(benchmark::State& state)
{
    //===============================================================//
    // ------------------------------------------------------------------------
    // PERFORMANCE: CALCULATE CRC32-C HARDWARE SERIAL
    //
    // Concerns:
    //   Test the performance of bmqp::Crc32c_Impl::calculateHardwareSerial()
    //
    // Plan:
    //   - Time a large number of crc32c calculations for buffers of varying
    //     sizes, single threaded.
    //
    // Testing:
    //   Performance of crc32c calculation using the 'hardware serial'
    //   approach.
    // ------------------------------------------------------------------------
    //                [2] Crc32c (hardware serial)
    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK PERFORMANCE: "
                                      "CALCULATE CRC32-C HARDWARE SERIAL");

    const int k_MAX_SIZE = 67108864;  // 64 Mi

    // Read in random input
    char* buffer = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(k_MAX_SIZE));
    bsl::generate_n(buffer, k_MAX_SIZE, bsl::rand);

    // Measure calculation time and report

    // <time>
    for (auto _ : state) {
        const int length = state.range(0);
        for (int k = 0; k < length; ++k) {
            bmqp::Crc32c_Impl::calculateHardwareSerial(buffer, length);
        }
    }
    // </time>
    bmqtst::TestHelperUtil::allocator()->deallocate(buffer);
}

static void
testN5_bmqpPerformanceSoftware_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// PERFORMANCE: CALCULATE CRC32-C ON BUFFER SOFTWARE
//
// Concerns:
//   Test the performance of bmqp::Crc32c_Impl::calculateSoftware()
//   in a single thread environment. This will explicitly use the software
//   implementation.
//
// Plan:
//   - Time a large number of CRC32-C calculations for buffers of varying
//     sizes in a single thread and take the average.
//
// Testing:
//   Performance of calculating CRC32-C using the software implementation.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK PERFORMANCE: "
                                      "CALCULATE CRC32-C ON BUFFER SOFTWARE");

    const int k_MAX_SIZE = 67108864;  // 64 Mi

    // Read in random input
    char* buffer = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(k_MAX_SIZE));
    bsl::generate_n(buffer, k_MAX_SIZE, bsl::rand);

    // Measure calculation time and report
    bsl::vector<TableRecord> tableRecords(bmqtst::TestHelperUtil::allocator());

    //===================================================================//
    //                        [1] Crc32c (software)
    // <time>
    for (auto _ : state) {
        const int length = state.range(0);
        bmqp::Crc32c_Impl::calculateSoftware(buffer, length);
    }
    // </time>
    bmqtst::TestHelperUtil::allocator()->deallocate(buffer);
}

static void
testN5_bdldPerformanceSoftware_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// PERFORMANCE: CALCULATE CRC32-C ON BUFFER SOFTWARE BDE
//
// Concerns:
//   Test the performance of bdlde::Crc32()
//   in a single thread environment. This will explicitly use the software
//   implementation.
//
// Plan:
//   - Time a large number of CRC32-C calculations for buffers of varying
//     sizes in a single thread and take the average.
//
// Testing:
//   Performance of calculating CRC32-C using the BDE software implementation.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "GOOGLE BENCHMARK PERFORMANCE: "
        "CALCULATE CRC32-C ON BUFFER SOFTWARE BDE");

    const int k_MAX_SIZE = 67108864;  // 64 Mi

    // Read in random input
    char* buffer = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(k_MAX_SIZE));
    bsl::generate_n(buffer, k_MAX_SIZE, bsl::rand);

    // Measure calculation time and report
    //===================================================================//
    //                         [2] BDE bdlde::crc32

    const int    length = state.range(0);
    bdlde::Crc32 crcBde(buffer, length);
    // <time>
    for (auto _ : state) {
        for (int l = 0; l < length; ++l) {
            crcBde.update(buffer, length);
            crcBde.checksumAndReset();
        }
    }
    // </time>

    bmqtst::TestHelperUtil::allocator()->deallocate(buffer);
}

BSLA_MAYBE_UNUSED
static void
testN6_bmqpPerformanceDefault_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// PERFORMANCE: CALCULATE CRC32-C ON BUFFER DEFAULT
//
// Concerns:
//   Test the performance of bmqp::Crc32c::calculate(const void  *data,
//                                                   unsigned int length);
//   in a single thread environment.  On a supported platform (see 'Support
//   for Hardware Acceleration' in the header file), this will use a
//   hardware-accelerated implementation.  Otherwise, it will use a
//   portable software implementation.
//
// Plan:
//   - Time CRC32-C calculations for buffers of varying
//     sizes in a single thread and take the average.
//
// Testing:
//   Performance of calculating CRC32-C using the default implementation.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK PERFORMANCE: "
                                      "CALCULATE CRC32-C ON BUFFER DEFAULT");

    const int k_MAX_SIZE = 67108864;  // 64 Mi

    // Read in random input
    char* buffer = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(k_MAX_SIZE));
    bsl::generate_n(buffer, k_MAX_SIZE, bsl::rand);

    // Measure calculation time and report

    //===================================================================//
    //                        [1] Crc32c (default)

    // <time>
    for (auto _ : state) {
        const int length = state.range(0);
        bmqp::Crc32c::calculate(buffer, length);
    }
    // </time>
    bmqtst::TestHelperUtil::allocator()->deallocate(buffer);
}

BSLA_MAYBE_UNUSED
static void
testN6_bdldPerformanceDefault_GoogleBenchmark(benchmark::State& state)
{
    //===================================================================//
    //                         [2] BDE bdlde::crc32
    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK PERFORMANCE: "
                                      "CALCULATE CRC32-C ON BUFFER BDE");

    const int k_MAX_SIZE = 67108864;  // 64 Mi

    // Read in random input
    char* buffer = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(k_MAX_SIZE));
    bsl::generate_n(buffer, k_MAX_SIZE, bsl::rand);

    // Measure calculation time and report

    // <time>
    for (auto _ : state) {
        const int    length = state.range(0);
        bdlde::Crc32 crcBde(buffer, length);
        crcBde.checksumAndReset();
        crcBde.update(buffer, length);
        crcBde.checksumAndReset();
    }
    bmqtst::TestHelperUtil::allocator()->deallocate(buffer);
}

#endif  // BMQTST_BENCHMARK_ENABLED

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    bmqp::Crc32c::initialize();
    // We explicitly initialize before the 'TEST_PROLOG' to circumvent a
    // case where the associated logging infrastructure triggers a default
    // allocation violation for no apparent reason.

    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    size_t seed = bsl::time(0);
    PV("Seed: " << seed);
    bsl::srand(seed);

    switch (_testCase) {
    case 0:
    case 8: test8_calculateOnBlobWithPreviousCrc(); break;
    case 7: test7_calculateOnBlob(); break;
    case 6: test6_multithreadedCrc32cSoftware(); break;
    case 5: test5_multithreadedCrc32cDefault(); break;
    case 4: test4_calculateOnBufferWithPreviousCrc(); break;
    case 3: test3_calculateOnMisalignedBuffer(); break;
    case 2: test2_calculateOnBuffer(); break;
    case 1: test1_breathingTest(); break;
    case -1: BMQTST_BENCHMARK(testN1_performanceDefaultUserInput); break;
    case -2: testN2_performanceDefault(); break;
    case -3:
        BMQTST_BENCHMARK_WITH_ARGS(testN3_defaultCalculateThroughput,
                                   RangeMultiplier(10)
                                       ->Range(10, 1000000)
                                       ->Unit(benchmark::kMillisecond));
        BMQTST_BENCHMARK_WITH_ARGS(testN3_softwareCalculateThroughput,
                                   RangeMultiplier(10)
                                       ->Range(10, 1000000)
                                       ->Unit(benchmark::kMillisecond));
        BMQTST_BENCHMARK_WITH_ARGS(testN3_bdldCalculateThroughput,
                                   RangeMultiplier(10)
                                       ->Range(10, 1000000)
                                       ->Unit(benchmark::kMillisecond));
        break;
    case -4:
        BMQTST_BENCHMARK_WITH_ARGS(
            testN4_calculateSerialDefault,
            Apply(populateBufferLengthsSorted_GoogleBenchmark_Small));
        BMQTST_BENCHMARK_WITH_ARGS(
            testN4_calculateHardwareSerial,
            Apply(populateBufferLengthsSorted_GoogleBenchmark_Small));
        break;
    case -5:
        BMQTST_BENCHMARK_WITH_ARGS(
            testN5_bmqpPerformanceSoftware,
            Apply(populateBufferLengthsSorted_GoogleBenchmark_Small));
        BMQTST_BENCHMARK_WITH_ARGS(
            testN5_bdldPerformanceSoftware,
            Apply(populateBufferLengthsSorted_GoogleBenchmark_Small));
        break;
    case -6:
        BMQTST_BENCHMARK_WITH_ARGS(
            testN6_bmqpPerformanceDefault,
            Apply(populateBufferLengthsSorted_GoogleBenchmark_Large));
        BMQTST_BENCHMARK_WITH_ARGS(
            testN6_bdldPerformanceDefault,
            Apply(populateBufferLengthsSorted_GoogleBenchmark_Large));
        break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }
#ifdef BMQTST_BENCHMARK_ENABLED
    if (_testCase < 0) {
        benchmark::Initialize(&argc, argv);
        benchmark::RunSpecifiedBenchmarks();
    }
#endif
    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
