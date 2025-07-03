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

// bmqp_compression.t.cpp                                             -*-C++-*-
#include <bmqp_compression.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocolutil.h>

#include <bmqu_blob.h>
#include <bmqu_memoutstream.h>
#include <bmqu_outstreamformatsaver.h>
#include <bmqu_printutil.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_numeric.h>
#include <bsls_timeutil.h>

// BENCHMARKING LIBRARY
#ifdef BMQTST_BENCHMARK_ENABLED
#include <benchmark/benchmark.h>
#endif
#include <bsl_algorithm.h>
#include <bsl_cstring.h>
#include <bsl_iomanip.h>
#include <bsl_ios.h>
#include <bsl_iostream.h>
#include <bsl_iterator.h>
#include <bsl_memory.h>
#include <bsl_vector.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Struct representing a record in a table of performance statistics.
struct TableRecord {
    bsls::Types::Int64 d_size;               // bytes
    bsls::Types::Int64 d_compressionTime;    // nano-seconds
    bsls::Types::Int64 d_decompressionTime;  // nano-seconds
};

/// Struct representing a record in a table of performance statistics.
struct CompressionRatioRecord {
    bsls::Types::Int64 d_inputSize;       // bytes
    bsls::Types::Int64 d_compressedSize;  // bytes
    double             d_compressionRatio;
};

/// Load into the specified `str` a string of random alphanumeric characters
/// of the specified `len` size.
static void generateRandomString(bsl::string* str, size_t len)
{
    static const char k_ALPHANUM[] = "0123456789"
                                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     "abcdefghijklmnopqrstuvwxyz";

    for (size_t i = 0; i < len; ++i) {
        str->push_back(k_ALPHANUM[rand() % (sizeof(k_ALPHANUM) - 1)]);
    }
}

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

    bsl::fill_n(bsl::ostream_iterator<char>(bsl::cout), headerLineLength, '=');
    bsl::cout << '\n';

    for (bsl::vector<bsl::string>::size_type col = 0; col < headers.size();
         ++col) {
        out << "| " << headers[col]
            << ((col == (headers.size() - 1)) ? '\n' : ' ');
    }

    bsl::fill_n(bsl::ostream_iterator<char>(bsl::cout), headerLineLength, '=');
    bsl::cout << '\n';
}

static void printRecord(bsl::ostream&                   out,
                        const TableRecord&              record,
                        const bsl::vector<bsl::string>& headerCols)
{
    // NOTE: we can't just bsl::setw(colWidth) << PrintUtil::pretty... because
    //       internal PrintUtil uses setw, so instead print in a temp buffer

    bmqu::MemOutStream bytesStream;
    bmqu::MemOutStream compressStream;
    bmqu::MemOutStream decompressStream;

    bytesStream << bmqu::PrintUtil::prettyBytes(record.d_size);
    compressStream << bmqu::PrintUtil::prettyTimeInterval(
        record.d_compressionTime);
    decompressStream << bmqu::PrintUtil::prettyTimeInterval(
        record.d_decompressionTime);

    // size
    out << "| " << bsl::setw(headerCols[0].size() + 1) << bytesStream.str();
    // compressionTime
    out << "| " << bsl::setw(headerCols[1].size() + 1) << compressStream.str();
    // decompressionTime
    out << "| " << bsl::setw(headerCols[2].size()) << decompressStream.str();
    out << '\n';
}

static void printRecord(bsl::ostream&                   out,
                        const CompressionRatioRecord&   record,
                        const bsl::vector<bsl::string>& headerCols)
{
    // NOTE: we can't just bsl::setw(colWidth) << PrintUtil::pretty... because
    //       internal PrintUtil uses setw, so instead print in a temp buffer

    bmqu::MemOutStream inputSizeStream;
    bmqu::MemOutStream compressedSizeStream;

    inputSizeStream << bmqu::PrintUtil::prettyBytes(record.d_inputSize);
    compressedSizeStream << bmqu::PrintUtil::prettyBytes(
        record.d_compressedSize);

    // input size
    out << "| " << bsl::setw(headerCols[0].size() + 1)
        << inputSizeStream.str();
    // compressed size
    out << "| " << bsl::setw(headerCols[1].size() + 1)
        << compressedSizeStream.str();
    // compressionRatio
    out << "| " << bsl::setw(headerCols[2].size())
        << record.d_compressionRatio;
    out << '\n';
}

/// Print the specified `tableRecords` to the specified `out`, using the
/// specified `headerCols` to determine the appropriate width for each
/// column. Print in the following format:
///
/// ```
/// | <size> | <compressionTime> | <decompressionTime>
/// | [<inputSize>] | [<compressedSize>] | [<compressionRatio>]
/// ```
static void printTableRows(bsl::ostream&                   out,
                           const bsl::vector<TableRecord>& tableRecords,
                           const bsl::vector<bsl::string>& headerCols)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(headerCols.size() == 3);

    bmqu::OutStreamFormatSaver fmtSaver(out);

    out << bsl::right << bsl::fixed;

    for (size_t i = 0; i < tableRecords.size(); ++i) {
        const TableRecord& record = tableRecords[i];
        printRecord(out, record, headerCols);
    }
}

/// Print the specified `tableRecords` to the specified `out`, using the
/// specified `headerCols` to determine the appropriate width for each
/// column. Print in the following format:
///
/// ```
/// | <size> | <compressionTime> | <decompressionTime>
/// | [<inputSize>] | [<compressedSize>] | [<compressionRatio>]
/// ```
static void
printTableRows(bsl::ostream&                              out,
               const bsl::vector<CompressionRatioRecord>& tableRecords,
               const bsl::vector<bsl::string>&            headerCols)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(headerCols.size() == 3);

    bmqu::OutStreamFormatSaver fmtSaver(out);

    out << bsl::right << bsl::fixed;

    for (size_t i = 0; i < tableRecords.size(); ++i) {
        const CompressionRatioRecord& record = tableRecords[i];
        printRecord(out, record, headerCols);
    }
}

/// Print a table having the specified `headerCols` and `tableRecords` to
/// the specified `out`.
template <typename T>
static void printTable(bsl::ostream&                   out,
                       const bsl::vector<bsl::string>& headerCols,
                       const bsl::vector<T>&           tableRecords)
{
    printTableHeader(out, headerCols);
    printTableRows(out, tableRecords, headerCols);
}

/// Populate the specified `data` with various strings with size in
/// increasing order. Note that `data` will be cleared.
static void populateData(bsl::vector<bsl::string>* data)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(data);

    data->clear();

    // start with string of length 2 and go till length 2^30
    size_t length = 2;
    for (size_t i = 0; i < 30; ++i) {
        bsl::string str("", bmqtst::TestHelperUtil::allocator());
        generateRandomString(&str, length);
        data->push_back(str);
        length *= 2;
    }
}

template <typename D>
static void eZlibCompressDecompressHelper(
    bsls::Types::Int64*                         compressionTime,
    bsls::Types::Int64*                         decompressionTime,
    const D&                                    data,
    const char*                                 expectedCompressed,
    const bmqt::CompressionAlgorithmType::Enum& algorithm)
{
    bmqu::MemOutStream             error(bmqtst::TestHelperUtil::allocator());
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob input(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob compressed(&bufferFactory,
                           bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob decompressed(&bufferFactory,
                             bmqtst::TestHelperUtil::allocator());

    bdlbb::BlobUtil::append(&input, data.data(), data.length());

    bsls::Types::Int64 startTime = bsls::TimeUtil::getTimer();
    int                rc        = bmqp::Compression::compress(&compressed,
                                         &bufferFactory,
                                         algorithm,
                                         data.data(),
                                         data.length(),
                                         &error,
                                         bmqtst::TestHelperUtil::allocator());
    *compressionTime             = bsls::TimeUtil::getTimer() - startTime;

    BMQTST_ASSERT_EQ(rc, 0);

    // get compressed data and compare with expected_compressed
    BSLS_ASSERT_SAFE(compressed.numDataBuffers() == 1);
    int   bufferSize     = bmqu::BlobUtil::bufferSize(compressed, 0);
    char* receivedBuffer = compressed.buffer(0).data();

    BMQTST_ASSERT_EQ(
        bsl::memcmp(expectedCompressed, receivedBuffer, bufferSize),
        0);

    startTime          = bsls::TimeUtil::getTimer();
    rc                 = bmqp::Compression::decompress(&decompressed,
                                       &bufferFactory,
                                       algorithm,
                                       compressed,
                                       &error,
                                       bmqtst::TestHelperUtil::allocator());
    *decompressionTime = bsls::TimeUtil::getTimer() - startTime;

    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(bdlbb::BlobUtil::compare(decompressed, input), 0);
}

template <typename D>
static void
eZlibCompressDecompressHelper(bsls::Types::Int64* compressionTime,
                              bsls::Types::Int64* decompressionTime,
                              const D&            data)
{
    bmqu::MemOutStream             error(bmqtst::TestHelperUtil::allocator());
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob input(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob compressed(&bufferFactory,
                           bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob decompressed(&bufferFactory,
                             bmqtst::TestHelperUtil::allocator());

    bdlbb::BlobUtil::append(&input, data.data(), data.length());

    bsls::Types::Int64 startTime = bsls::TimeUtil::getTimer();
    int                rc        = bmqp::Compression_Impl::compressZlib(
        &compressed,
        &bufferFactory,
        input,
        -1,
        &error,
        bmqtst::TestHelperUtil::allocator());
    *compressionTime = bsls::TimeUtil::getTimer() - startTime;

    BMQTST_ASSERT_EQ(rc, 0);

    startTime = bsls::TimeUtil::getTimer();
    rc        = bmqp::Compression_Impl::decompressZlib(
        &decompressed,
        &bufferFactory,
        compressed,
        &error,
        bmqtst::TestHelperUtil::allocator());
    *decompressionTime = bsls::TimeUtil::getTimer() - startTime;

    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(bdlbb::BlobUtil::compare(decompressed, input), 0);
}

template <typename D>
static void eZlibCompressionRatioHelper(bsls::Types::Int64* inputSize,
                                        bsls::Types::Int64* compressedSize,
                                        const D&            data,
                                        int                 level)
{
    bmqu::MemOutStream             error(bmqtst::TestHelperUtil::allocator());
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob input(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob compressed(&bufferFactory,
                           bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob decompressed(&bufferFactory,
                             bmqtst::TestHelperUtil::allocator());

    bdlbb::BlobUtil::append(&input, data.data(), data.length());
    *inputSize = input.length();

    int rc = bmqp::Compression_Impl::compressZlib(
        &compressed,
        &bufferFactory,
        input,
        level,
        &error,
        bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(rc, 0);
    *compressedSize = compressed.length();

    rc = bmqp::Compression_Impl::decompressZlib(
        &decompressed,
        &bufferFactory,
        compressed,
        &error,
        bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(bdlbb::BlobUtil::compare(decompressed, input), 0);
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
//   - Create various strings, compress them, decompress and compare the
//     decompressed strings with the original strings.
//   - Create an empty string, compress it, decompress and compare it with
//     the original string.
//   - Create a null buffer i.e. one with no string, compress it,
//     decompress and compare with the original buffer.
//   - Create a null buffer i.e. one with no string, compress it,
//     decompress and compare with the original buffer.
//   - Create an input blob which is created by appending multiple buffers
//     i.e. one after another, compress it, decompress and compare with the
//     original blob.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    {
        PV("NONEMPTY BUFFER");

        struct Test {
            int         d_line;
            const char* d_data;
            const char* d_expected;
        } k_DATA[] = {{L_,
                       "Hello World",
                       "\x78\x9c\xf3\x48"
                       "\xcd\xc9\xc9\x57\x8"
                       "\xcf\x2f\xca\x49\x1"
                       "\x0\x18\xb\x4\x1d"},
                      {L_,
                       "HelloHello",
                       "\x78\x9c\xf3\x48"
                       "\xcd\xc9\xc9\xf7"
                       "\x48\xcd\xc9\xc9"
                       "\x7\x0\x14\xdc\x3"
                       "\xe9"},
                      {L_,
                       "abcdefghij",
                       "\x78\x9c\x4b\x4c"
                       "\x4a\x4e\x49\x4d"
                       "\x4b\xcf\xc8\xcc"
                       "\x2\x0\x15\x86\x3"
                       "\xf8"},
                      {L_,
                       "Hello Hello Hello Hello Hello",
                       "\x78\x9c\xf3\x48"
                       "\xcd\xc9\xc9\x57"
                       "\xf0\xc0\x4e\x2\x0"
                       "\x98\x70\xa\x45"},
                      {L_,
                       "abcdefghijklmnopqrstuvwxyz",
                       "\x78\x9c\x4b\x4c"
                       "\x4a\x4e\x49\x4d"
                       "\x4b\xcf\xc8\xcc"
                       "\xca\xce\xc9\xcd"
                       "\xcb\x2f\x28\x2c"
                       "\x2a\x2e\x29\x2d"
                       "\x2b\xaf\xa8\xac"
                       "\x2\x0\x90\x86\xb"
                       "\x20"},
                      {L_,
                       "abcdefghijklmnopqrstuvwxyz1234567890",
                       "\x78\x9c\x4b\x4c"
                       "\x4a\x4e\x49\x4d"
                       "\x4b\xcf\xc8\xcc"
                       "\xca\xce\xc9\xcd"
                       "\xcb\x2f\x28\x2c"
                       "\x2a\x2e\x29\x2d"
                       "\x2b\xaf\xa8\xac"
                       "\x32\x34\x32\x36"
                       "\x31\x35\x33\xb7"
                       "\xb0\x34\x0\x0\xa"
                       "\xf7\xd\x2d"}};

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test& test = k_DATA[idx];

            bsl::cout << test.d_line << "'" << test.d_data << "'" << bsl::endl;
            bsls::Types::Int64 compressionTime   = 0;
            bsls::Types::Int64 decompressionTime = 0;
            bsl::string        inputString(test.d_data,
                                    bmqtst::TestHelperUtil::allocator());
            eZlibCompressDecompressHelper(
                &compressionTime,
                &decompressionTime,
                inputString,
                test.d_expected,
                bmqt::CompressionAlgorithmType::e_ZLIB);
        }
    }

    {
        PV("BUFFER WITH EMPTY STRING");

        // Test edge case of empty string in buffer
        const bsl::string  data("", bmqtst::TestHelperUtil::allocator());
        bsls::Types::Int64 compressionTime   = 0;
        bsls::Types::Int64 decompressionTime = 0;
        eZlibCompressDecompressHelper(&compressionTime,
                                      &decompressionTime,
                                      data,
                                      "\x78\x9c\x3\x0\x0\x0\x0\x1",
                                      bmqt::CompressionAlgorithmType::e_ZLIB);
    }

    {
        PV("NULL BUFFER");

        // Test edge case of null buffer
        bmqu::MemOutStream error(bmqtst::TestHelperUtil::allocator());
        bdlbb::PooledBlobBufferFactory bufferFactory(
            1024,
            bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob input(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob compressed(&bufferFactory,
                               bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob decompressed(&bufferFactory,
                                 bmqtst::TestHelperUtil::allocator());
        const char* expectedCompressed = "\x78\x9c\x3\x0\x0"
                                         "\x0\x0\x1";

        int rc = bmqp::Compression_Impl::compressZlib(
            &compressed,
            &bufferFactory,
            input,
            -1,
            &error,
            bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(rc, 0);

        // get compressed data and compare with expected_compressed
        BSLS_ASSERT_SAFE(compressed.numDataBuffers() == 1);
        int   bufferSize     = bmqu::BlobUtil::bufferSize(compressed, 0);
        char* receivedBuffer = compressed.buffer(0).data();

        BMQTST_ASSERT_EQ(
            bsl::memcmp(expectedCompressed, receivedBuffer, bufferSize),
            0);

        rc = bmqp::Compression_Impl::decompressZlib(
            &decompressed,
            &bufferFactory,
            compressed,
            &error,
            bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(rc, 0);
        BMQTST_ASSERT_EQ(bdlbb::BlobUtil::compare(decompressed, input), 0);
    }

    {
        PV("MULTIPLE BUFFERS");

        // Test edge case of multiple buffers appended to a blob
        bmqu::MemOutStream error(bmqtst::TestHelperUtil::allocator());
        bdlbb::PooledBlobBufferFactory bufferFactory(
            1024,
            bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob input(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob compressed(&bufferFactory,
                               bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob decompressed(&bufferFactory,
                                 bmqtst::TestHelperUtil::allocator());
        char        one[] = "one";
        bsl::shared_ptr<char> onePtr;
        onePtr.reset(one,
                     bslstl::SharedPtrNilDeleter(),
                     bmqtst::TestHelperUtil::allocator());
        bdlbb::BlobBuffer buf1(onePtr, bsl::strlen(one));
        input.appendDataBuffer(buf1);

        char                  two[] = "two";
        bsl::shared_ptr<char> twoPtr;
        twoPtr.reset(two,
                     bslstl::SharedPtrNilDeleter(),
                     bmqtst::TestHelperUtil::allocator());
        bdlbb::BlobBuffer buf2(twoPtr, bsl::strlen(two));
        input.appendDataBuffer(buf2);

        char                  three[] = "three";
        bsl::shared_ptr<char> threePtr;
        threePtr.reset(three,
                       bslstl::SharedPtrNilDeleter(),
                       bmqtst::TestHelperUtil::allocator());
        bdlbb::BlobBuffer buf3(threePtr, bsl::strlen(three));
        input.appendDataBuffer(buf3);

        const char* expectedCompressed = "\x78\x9c\xcb\xcf\x4b\x2d\x29\xcf\x2f"
                                         "\xc9\x28\x4a\x4d\x5\x0\x1c\x8d\x4"
                                         "\xb5";

        int rc = bmqp::Compression_Impl::compressZlib(
            &compressed,
            &bufferFactory,
            input,
            -1,
            &error,
            bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(rc, 0);

        // get compressed data and compare with expected_compressed
        BSLS_ASSERT_SAFE(compressed.numDataBuffers() == 1);
        int   bufferSize     = bmqu::BlobUtil::bufferSize(compressed, 0);
        char* receivedBuffer = compressed.buffer(0).data();

        BMQTST_ASSERT_EQ(
            bsl::memcmp(expectedCompressed, receivedBuffer, bufferSize),
            0);

        rc = bmqp::Compression_Impl::decompressZlib(
            &decompressed,
            &bufferFactory,
            compressed,
            &error,
            bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(rc, 0);
        BMQTST_ASSERT_EQ(bdlbb::BlobUtil::compare(decompressed, input), 0);
    }
}

static void test2_compression_cluster_message()
// ------------------------------------------------------------------------
// TEST USING CLUSTER MESSAGE
//
// Concerns:
//   Check proper compression and decompression for cluster message.
//
// Plan:
//   - Create a cluster message, compress it, decompress it and compare the
//     decompressed message with the original cluster message.
//
// Testing:
//   Compression/Decompression functionality for non-string input
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // The default allocator check fails in this test case because the
    // 'loggedMessages' methods of Encoder returns a memory-aware
    // object without utilizing the parameter allocator.
    {
        PV("BUFFER WITH CLUSTER MESSAGE");

        // Test case with cluster message as input data
        bmqu::MemOutStream error(bmqtst::TestHelperUtil::allocator());
        bdlbb::PooledBlobBufferFactory bufferFactory(
            1024,
            bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob input(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob compressed(&bufferFactory,
                               bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob decompressed(&bufferFactory,
                                 bmqtst::TestHelperUtil::allocator());

        bmqp_ctrlmsg::ClusterMessage clusterMessage(
            bmqtst::TestHelperUtil::allocator());
        bmqp_ctrlmsg::LeaderAdvisoryCommit commit;

        // Create an update message
        bmqp_ctrlmsg::LeaderMessageSequence lms;
        lms.electorTerm()    = 3U;
        lms.sequenceNumber() = 8U;

        commit.sequenceNumber()                           = lms;
        commit.sequenceNumberCommitted().electorTerm()    = 2U;
        commit.sequenceNumberCommitted().sequenceNumber() = 99U;

        clusterMessage.choice().makeLeaderAdvisoryCommit(commit);

        int rc = bmqp::ProtocolUtil::encodeMessage(
            error,
            &input,
            clusterMessage,
            bmqp::EncodingType::e_JSON,
            bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(rc, 0);

        bsl::cout << "Input size: " << input.length() << endl;

        rc = bmqp::Compression_Impl::compressZlib(
            &compressed,
            &bufferFactory,
            input,
            -1,
            &error,
            bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(rc, 0);
        bsl::cout << "Compressed size: " << compressed.length() << endl;

        rc = bmqp::Compression_Impl::decompressZlib(
            &decompressed,
            &bufferFactory,
            compressed,
            &error,
            bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(rc, 0);
        BMQTST_ASSERT_EQ(bdlbb::BlobUtil::compare(decompressed, input), 0);
        bsl::cout << "Decompressed size: " << decompressed.length() << endl;
    }
}

static void test3_compression_decompression_none()
// ------------------------------------------------------------------------
// TEST USING bmqt::CompressionAlgorithmType::e_NONE ALGORITHM TYPE
//
// Concerns:
//   Check edge case where specified compression algorithm is
//   bmqt::CompressionAlgorithmType::e_NONE.
//
// Plan:
//   - Create a string, compress it, decompress it and compare the
//     decompressed message with the original string.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("NONE TYPE ALGORITHM TEST");
    {
        PV("BUFFERS WITH NONEMPTY STRINGS");

        struct Test {
            int         d_line;
            const char* d_data;
            const char* d_expected;
        } k_DATA[] = {{L_, "Hello World", "Hello World"},
                      {L_, "HelloHello", "HelloHello"},
                      {L_, "abcdefghij", "abcdefghij"},
                      {L_,
                       "Hello Hello Hello Hello Hello",
                       "Hello Hello Hello "
                       "Hello Hello"}};

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test& test = k_DATA[idx];

            PVV(test.d_line << "'" << test.d_data << "'");
            bsls::Types::Int64 compressionTime   = 0;
            bsls::Types::Int64 decompressionTime = 0;
            bsl::string        inputString(test.d_data,
                                    bmqtst::TestHelperUtil::allocator());
            eZlibCompressDecompressHelper(
                &compressionTime,
                &decompressionTime,
                inputString,
                test.d_expected,
                bmqt::CompressionAlgorithmType::e_NONE);
        }
    }
}

// ============================================================================
//                              PERFORMANCE TESTS
// ----------------------------------------------------------------------------

BSLA_MAYBE_UNUSED
static void testN1_performanceCompressionDecompressionDefault()
// ------------------------------------------------------------------------
// PERFORMANCE: Perform compression and decompression.
//
// Concerns:
//   - Test the performance of bmqp::Compression_Impl::compressZlib(
//                                   bdlbb::Blob              *output,
//                                   bdlbb::BlobBufferFactory *factory,
//                                   const bdlbb::Blob&        input,
//                                   int                       level,
//                                   bsl::ostream             *errorStream,
//                                   bslma::Allocator         *allocator);
//     in a single thread environment.
//   - Test the performance of bmqp::Compression_Impl::decompressZlib(
//                                   bdlbb::Blob              *output,
//                                   bdlbb::BlobBufferFactory *factory,
//                                   const bdlbb::Blob&        input,
//                                   bsl::ostream             *errorStream,
//                                   bslma::Allocator         *allocator));
//     in a single thread environment.
//
// Plan:
//   - Time a large number of compressions, decompressions for buffers of
//     strings with various sizes in a single thread and take the average.
//
// Testing:
//   Performance of compressing and decompressing using the default
//   implementation.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // The default allocator check fails in this test case because the
    // printTable method utilizes the global allocator.

    bmqtst::TestHelper::printTestName(
        "PERFORMANCE: COMPRESS/DECOMPRESS ON BUFFER");

    const int k_NUM_ITERS = 1;

    bsl::vector<bsl::string> data(bmqtst::TestHelperUtil::allocator());
    populateData(&data);

    // Measure calculation time and report
    bsl::vector<TableRecord> tableRecords(bmqtst::TestHelperUtil::allocator());
    for (unsigned i = 0; i < data.size(); ++i) {
        const int length = data[i].size();

        bsl::cout << "-----------------------\n"
                  << " SIZE       = " << bmqu::PrintUtil::prettyBytes(length)
                  << '\n'
                  << " ITERATIONS = " << k_NUM_ITERS << '\n';

        //=====================================================================
        //                   Compression, Decompression

        // <time>
        bsls::Types::Int64 compressionTotalTime   = 0,
                           decompressionTotalTime = 0;
        for (unsigned int l = 0; l < k_NUM_ITERS; ++l) {
            bsls::Types::Int64 compressionTime   = 0;
            bsls::Types::Int64 decompressionTime = 0;
            eZlibCompressDecompressHelper(&compressionTime,
                                          &decompressionTime,
                                          data[i]);
            compressionTotalTime += compressionTime;
            decompressionTotalTime += decompressionTime;
        }
        // </time>

        //=====================================================================
        //                            Report
        TableRecord record;
        record.d_size              = length;
        record.d_compressionTime   = compressionTotalTime / k_NUM_ITERS;
        record.d_decompressionTime = decompressionTotalTime / k_NUM_ITERS;
        tableRecords.push_back(record);

        bsl::cout
            << "Average Time:\n"
            << "  Compression   : "
            << bmqu::PrintUtil::prettyTimeInterval(record.d_compressionTime)
            << '\n'
            << "  Decompression : "
            << bmqu::PrintUtil::prettyTimeInterval(record.d_decompressionTime)
            << "\n\n";
    }

    // Print performance comparison table
    bsl::vector<bsl::string> headerCols(bmqtst::TestHelperUtil::allocator());
    headerCols.emplace_back("Payload Size");
    headerCols.emplace_back("Compression");
    headerCols.emplace_back("Decompression");

    printTable(bsl::cout, headerCols, tableRecords);
}

BSLA_MAYBE_UNUSED
static void testN2_calculateThroughput()
// ------------------------------------------------------------------------
// BENCHMARK: PERFORM COMPRESSION DECOMPRESSION
//
// Concerns:
//   Test the throughput (GB/s) of Compression and decompression using
//   Zlib implementations in a single thread environment.
//
// Plan:
//   - Time a large number of compressions and decompressions using
//     Zlib implementation for a buffer of fixed size in a single thread
//     and take the average.  Calculate the throughput using its average.
//
// Testing:
//   Throughput (GB/s) of Compression and decompression of Zlib
//   implementation in a single thread environment.
// ------------------------------------------------------------------------
{
    size_t                    length      = 1024;     // 1 Ki
    const bsls::Types::Uint64 k_NUM_ITERS = 1000000;  // 1 M

    bsl::string buffer_data("", bmqtst::TestHelperUtil::allocator());
    generateRandomString(&buffer_data, length);
    // <time>
    bsls::Types::Int64 compressionTotalTime = 0, decompressionTotalTime = 0;
    for (unsigned int l = 0; l < k_NUM_ITERS; ++l) {
        bsls::Types::Int64 compressionTime   = 0;
        bsls::Types::Int64 decompressionTime = 0;
        eZlibCompressDecompressHelper(&compressionTime,
                                      &decompressionTime,
                                      buffer_data);
        compressionTotalTime += compressionTime;
        decompressionTotalTime += decompressionTime;
    }
    // </time>

    cout << "=========================\n";
    cout << "For a payload of length " << bmqu::PrintUtil::prettyBytes(length)
         << ", completed "
         << bmqu::PrintUtil::prettyNumber(
                static_cast<bsls::Types::Int64>(k_NUM_ITERS))
         << " compression iterations in "
         << bmqu::PrintUtil::prettyTimeInterval(compressionTotalTime) << ".\n"
         << "Above implies that 1 compression iteration was calculated in "
         << bmqu::PrintUtil::prettyTimeInterval(compressionTotalTime /
                                                k_NUM_ITERS)
         << ".\nIn other words: "
         << bmqu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                (k_NUM_ITERS * bdlt::TimeUnitRatio::k_NS_PER_S) /
                compressionTotalTime))
         << " iterations per second.\n"
         << "Compression throughput: "
         << bmqu::PrintUtil::prettyBytes(
                (k_NUM_ITERS * bdlt::TimeUnitRatio::k_NS_PER_S * length) /
                compressionTotalTime)
         << " per second.\n"
         << endl;

    cout << "=========================\n";
    cout << "For a payload of length " << bmqu::PrintUtil::prettyBytes(length)
         << ", completed "
         << bmqu::PrintUtil::prettyNumber(
                static_cast<bsls::Types::Int64>(k_NUM_ITERS))
         << " decompression iterations in "
         << bmqu::PrintUtil::prettyTimeInterval(decompressionTotalTime)
         << ".\nAbove implies that 1 decompression iteration was calculated "
         << "in "
         << bmqu::PrintUtil::prettyTimeInterval(decompressionTotalTime /
                                                k_NUM_ITERS)
         << ".\nIn other words: "
         << bmqu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                (k_NUM_ITERS * bdlt::TimeUnitRatio::k_NS_PER_S) /
                decompressionTotalTime))
         << " iterations per second.\n"
         << "Decompression throughput: "
         << bmqu::PrintUtil::prettyBytes(
                (k_NUM_ITERS * bdlt::TimeUnitRatio::k_NS_PER_S * length) /
                decompressionTotalTime)
         << " per second.\n"
         << endl;
}

static void testN3_performanceCompressionRatio()
// ------------------------------------------------------------------------
// BENCHMARK: COMPRESSION RATIO
//
// Concerns:
//   Test the compression ratio (InputSize/CompressedSize) using
//   Zlib implementations in a single thread environment.
//
// Plan:
//   - Start from a random string of size 2 bytes and increase it in an
//     exponential manner to 1024 megabytes. Make a note of compression
//     ratio for each of the buffer sizes.
//
// Testing:
//   Compression ratio (InputSize/CompressedSize) of Zlib
//   implementation in a single thread environment.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // The default allocator check fails in this test case because the
    // printTable method utilizes the global allocator.

    bmqtst::TestHelper::printTestName("BENCHMARK: COMPRESSION RATIO");

    bsl::vector<bsl::string> data(bmqtst::TestHelperUtil::allocator());
    populateData(&data);

    // Measure calculation time and report per level
    for (int level = 0; level < 10; level++) {
        bsl::cout << "---------------------\n"
                  << " LEVEL = " << level << '\n'
                  << "---------------------\n";
        bsl::vector<CompressionRatioRecord> tableRecords(
            bmqtst::TestHelperUtil::allocator());
        for (unsigned i = 0; i < data.size(); ++i) {
            const int length = data[i].size();

            bsl::cout << "---------------------\n"
                      << " SIZE = " << bmqu::PrintUtil::prettyBytes(length)
                      << '\n'
                      << "---------------------\n";

            //===============================================================//
            //                   Compression, Decompression

            // <Size>
            bsls::Types::Int64 inputSize      = 0;
            bsls::Types::Int64 compressedSize = 0;
            eZlibCompressionRatioHelper(&inputSize,
                                        &compressedSize,
                                        data[i],
                                        level);
            // </Size>

            //===============================================================//
            //                            Report
            CompressionRatioRecord record;
            record.d_inputSize        = inputSize;
            record.d_compressedSize   = compressedSize;
            record.d_compressionRatio = static_cast<double>(inputSize) /
                                        compressedSize;
            tableRecords.push_back(record);

            bsl::cout << "Input Size        : "
                      << bmqu::PrintUtil::prettyBytes(record.d_inputSize)
                      << '\n'
                      << "Compressed Size   : "
                      << bmqu::PrintUtil::prettyBytes(record.d_compressedSize)
                      << '\n'
                      << "Compression Ratio : " << record.d_compressionRatio
                      << "\n\n";
        }
        // Print compression ratio comparison table
        bsl::vector<bsl::string> headerCols(
            bmqtst::TestHelperUtil::allocator());
        headerCols.emplace_back("Input Size");
        headerCols.emplace_back("Compressed Size");
        headerCols.emplace_back("Compression Ratio");

        printTable(bsl::cout, headerCols, tableRecords);
    }
}

// Begin Benchmarking Tests
#ifdef BMQTST_BENCHMARK_ENABLED
static void testN1_performanceCompressionDecompressionDefault_GoogleBenchmark(
    benchmark::State& state)
// ------------------------------------------------------------------------
// PERFORMANCE: Perform compression and decompression.
//
// Concerns:
//   - Test the performance of bmqp::Compression_Impl::compressZlib()
//     in a single thread environment.
//   - Test the performance of bmqp::Compression_Impl::decompressZlib()
//     in a single thread environment.
//
// Plan:
//   - Time a large number of compressions, decompressions for buffers of
//     strings with various sizes in a single thread and take the average.
//
// Testing:
//   Performance of compressing and decompressing using the default
//   implementation.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // The default allocator check fails in this test case because the
    // printTable method utilizes the global allocator.

    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK PERFORMANCE: "
                                      "COMPRESS/DECOMPRESS ON BUFFER");
    const int   length = state.range(0);
    bsl::string str("", bmqtst::TestHelperUtil::allocator());
    generateRandomString(&str, length);

    //=====================================================================
    //                   Compression, Decompression

    bsls::Types::Int64 compressionTotal = 0, decompressionTotal = 0;
    // <time>
    for (auto _ : state) {
        eZlibCompressDecompressHelper(&compressionTotal,
                                      &decompressionTotal,
                                      str);
    }
    // </time>
}

static void testN2_calculateThroughput_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// BENCHMARK: PERFORM COMPRESSION DECOMPRESSION
//
// Concerns:
//   Test the throughput (GB/s) of Compression and decompression using
//   Zlib implementations in a single thread environment.
//
// Plan:
//   - Time a large number of compressions and decompressions using
//     Zlib implementation for a buffer of fixed size in a single thread
//     and take the average.  Calculate the throughput using its average.
//
// Testing:
//   Throughput (GB/s) of Compression and decompression of Zlib
//   implementation in a single thread environment.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK THROUGHPUT: "
                                      "COMPRESS/DECOMPRESS ON BUFFER");
    size_t length = 1024;  // 1 Ki

    bsl::string buffer_data("", bmqtst::TestHelperUtil::allocator());
    generateRandomString(&buffer_data, length);
    // <time>
    for (unsigned int l = 0; l < state.range(0); ++l) {
        bsls::Types::Int64 compressionTotal = 0, decompressionTotal = 0;
        for (auto _ : state) {
            eZlibCompressDecompressHelper(&compressionTotal,
                                          &decompressionTotal,
                                          buffer_data);
        }
    }
    // </time>
}
#endif  // BMQTST_BENCHMARK_ENABLED
// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_breathingTest(); break;
    case 2: test2_compression_cluster_message(); break;
    case 3: test3_compression_decompression_none(); break;
    case -1:
        BMQTST_BENCHMARK_WITH_ARGS(
            testN1_performanceCompressionDecompressionDefault,
            Unit(benchmark::kMillisecond)
                ->RangeMultiplier(2)
                ->Range(2, 1073741824));  // 2^30
        break;
    case -2:
        BMQTST_BENCHMARK_WITH_ARGS(testN2_calculateThroughput,
                                   RangeMultiplier(10)
                                       ->Range(100, 1000000)
                                       ->Unit(benchmark::kMillisecond));
        break;
    case -3: testN3_performanceCompressionRatio(); break;
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
