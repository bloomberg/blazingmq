// Copyright 2024 Bloomberg Finance L.P.
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

// bmqstoragewriter
#include <m_bmqstoragewriter_cslwriter.h>
#include <m_bmqstoragewriter_util.h>

// MQB
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbc_clusterstateledgerutil.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqu_memoutstream.h>

// BDE
#include <baljsn_encoder.h>
#include <baljsn_encoderoptions.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlde_base64decoder.h>
#include <bdls_filesystemutil.h>
#include <bsl_cstring.h>
#include <bsl_sstream.h>
#include <bsl_string.h>
#include <bsl_vector.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace m_bmqstoragewriter;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS
// ----------------------------------------------------------------------------

namespace {

/// Decode the base64 string `b64` and return its uppercase hex form, using
/// `alloc` for allocation.  Mirrors the base64->hex conversion
/// `bmqstoragetool` applies to storage keys before printing, so the produced
/// JSON matches the tool's on-disk format that the writer consumes.
bsl::string base64ToHex(const bsl::string& b64, bslma::Allocator* alloc)
{
    bdlde::Base64Decoder decoder(true);
    bsl::vector<char>    bin(alloc);
    bin.resize(
        bdlde::Base64Decoder::maxDecodedLength(static_cast<int>(b64.size())));
    int numOut = 0, numIn = 0;
    decoder.convert(bin.data(),
                    &numOut,
                    &numIn,
                    b64.data(),
                    b64.data() + b64.size());
    int endOut = 0;
    decoder.endConvert(bin.data() + numOut, &endOut);
    bin.resize(numOut + endOut);

    static const char HEX[] = "0123456789ABCDEF";
    bsl::string       out(alloc);
    out.reserve(bin.size() * 2);
    for (bsl::size_t i = 0; i < bin.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(bin[i]);
        out.push_back(HEX[c >> 4]);
        out.push_back(HEX[c & 0x0F]);
    }
    return out;
}

/// Convert the values of the `"key"` and `"appKey"` fields in `json` from
/// base64 to hex in place, using `alloc`.  Reproduces `bmqstoragetool`'s
/// `convertKeysToHex`, i.e. the inverse of the writer's `convertKeysToBase64`.
void convertKeysToHex(bsl::string* json, bslma::Allocator* alloc)
{
    const char* keys[]    = {"\"key\"", "\"appKey\""};
    const int   keyLens[] = {5, 8};
    for (int p = 0; p < 2; ++p) {
        bsl::size_t pos = 0;
        while ((pos = json->find(keys[p], pos)) != bsl::string::npos) {
            bsl::size_t quoteStart = json->find('"', pos + keyLens[p]);
            if (quoteStart == bsl::string::npos)
                break;
            bsl::size_t valStart = quoteStart + 1;
            bsl::size_t valEnd   = json->find('"', valStart);
            if (valEnd == bsl::string::npos)
                break;
            bsl::string b64(json->data() + valStart, valEnd - valStart, alloc);
            bsl::string hex = base64ToHex(b64, alloc);
            json->replace(valStart, valEnd - valStart, hex);
            pos = valStart + hex.size() + 1;
        }
    }
}

/// A 5-byte binary storage key value.
bsl::vector<char> binKey(char b0, char b1, char b2, char b3, char b4)
{
    bsl::vector<char> v;
    v.push_back(b0);
    v.push_back(b1);
    v.push_back(b2);
    v.push_back(b3);
    v.push_back(b4);
    return v;
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
//   Malformed and empty input are handled without corrupting the file.
//
// Testing:
//   processCslInput
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

    bsl::string dir(alloc);
    BMQTST_ASSERT_EQ(bdls::FilesystemUtil::createTemporaryDirectory(&dir,
                                                                    "bmqsw"),
                     0);
    bsl::string cslPath(dir, alloc);
    cslPath.append("/c.bmq_csl");

    // Invalid JSON.
    {
        bdls::FilesystemUtil::FileDescriptor fd = bdls::FilesystemUtil::open(
            cslPath.c_str(),
            bdls::FilesystemUtil::e_CREATE,
            bdls::FilesystemUtil::e_READ_WRITE);
        BMQTST_ASSERT_NE(fd, bdls::FilesystemUtil::k_INVALID_FD);

        QueueCache         cache(alloc);
        bsl::istringstream iss(bsl::string("not json", alloc), alloc);
        BMQTST_ASSERT_NE(processCslInput(&cache, fd, true, iss, alloc), 0);
        bdls::FilesystemUtil::close(fd);
    }

    // Valid JSON, empty Records array.
    {
        bdls::FilesystemUtil::FileDescriptor fd = bdls::FilesystemUtil::open(
            cslPath.c_str(),
            bdls::FilesystemUtil::e_OPEN,
            bdls::FilesystemUtil::e_READ_WRITE);
        QueueCache         cache(alloc);
        bsl::istringstream iss(bsl::string("{\"Records\":[]}", alloc), alloc);
        BMQTST_ASSERT_EQ(processCslInput(&cache, fd, true, iss, alloc), 0);
        BMQTST_ASSERT_EQ(cache.size(), 0u);
        bdls::FilesystemUtil::close(fd);
    }

    bdls::FilesystemUtil::remove(dir, true);
}

static void test2_roundTrip()
// ------------------------------------------------------------------------
// ROUND TRIP TEST
//
// Concerns:
//   A queueAssignmentAdvisory record written from JSON is read back as a valid
//   CSL record with the expected file logId and queue contents, and the queue
//   cache is populated with the queue uri and appId.
//
// Testing:
//   processCslInput
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ROUND TRIP TEST");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

    const char*             k_URI        = "bmq://foo.bar/baz";
    const char*             k_APP_ID     = "myapp";
    const char*             k_LOG_ID_HEX = "1122334455";
    const bsl::vector<char> k_QUEUE_KEY  = binKey(1, 2, 3, 4, 5);
    const bsl::vector<char> k_APP_KEY    = binKey(10, 11, 12, 13, 14);

    // Build a ClusterMessage carrying a queueAssignmentAdvisory, exactly as
    // the broker would, then encode it to JSON the way bmqstoragetool prints
    // it (base64 keys converted to hex).  This is the "Record" the writer
    // reads.
    bmqp_ctrlmsg::ClusterMessage           msg(alloc);
    bmqp_ctrlmsg::QueueAssignmentAdvisory& qaa =
        msg.choice().makeQueueAssignmentAdvisory();
    qaa.sequenceNumber().electorTerm()    = 1;
    qaa.sequenceNumber().sequenceNumber() = 5;
    qaa.queues().resize(1);
    bmqp_ctrlmsg::QueueInfo& qi = qaa.queues()[0];
    qi.uri()                    = k_URI;
    qi.partitionId()            = 0;
    qi.key()                    = k_QUEUE_KEY;
    qi.appIds().resize(1);
    qi.appIds()[0].appId()  = k_APP_ID;
    qi.appIds()[0].appKey() = k_APP_KEY;

    bsl::string            recordJson(alloc);
    bmqu::MemOutStream     recStream(alloc);
    baljsn::Encoder        encoder(alloc);
    baljsn::EncoderOptions options;
    options.setEncodingStyle(baljsn::EncoderOptions::e_COMPACT);
    BMQTST_ASSERT_EQ(encoder.encode(recStream, msg, options), 0);
    recordJson = recStream.str();
    convertKeysToHex(&recordJson, alloc);

    bmqu::MemOutStream input(alloc);
    input << "{\"Records\":[{"
          << "\"RecordType\":\"UPDATE\",\"LogId\":\"" << k_LOG_ID_HEX << "\","
          << "\"ElectorTerm\":\"1\",\"SequenceNumber\":\"5\","
          << "\"Epoch\":\"999\",\"Record\":" << recordJson << "}]}";

    bsl::string dir(alloc);
    BMQTST_ASSERT_EQ(bdls::FilesystemUtil::createTemporaryDirectory(&dir,
                                                                    "bmqsw"),
                     0);
    bsl::string cslPath(dir, alloc);
    cslPath.append("/c.bmq_csl");

    QueueCache cache(alloc);
    {
        bdls::FilesystemUtil::FileDescriptor fd = bdls::FilesystemUtil::open(
            cslPath.c_str(),
            bdls::FilesystemUtil::e_CREATE,
            bdls::FilesystemUtil::e_READ_WRITE);
        BMQTST_ASSERT_NE(fd, bdls::FilesystemUtil::k_INVALID_FD);

        bsl::istringstream iss(bsl::string(input.str(), alloc), alloc);
        BMQTST_ASSERT_EQ(processCslInput(&cache, fd, true, iss, alloc), 0);
        bdls::FilesystemUtil::close(fd);
    }

    // -----------------------------------------------------------------
    // The queue cache is populated from the advisory.
    // -----------------------------------------------------------------
    mqbu::StorageKey queueKey;
    queueKey.fromBinary(k_QUEUE_KEY.data());

    BMQTST_ASSERT_EQ(cache.size(), 1u);
    QueueCache::const_iterator cit = cache.find(queueKey);
    BMQTST_ASSERT(cit != cache.end());
    if (cit != cache.end()) {
        BMQTST_ASSERT_EQ(cit->second.d_uri, bsl::string(k_URI, alloc));
        BMQTST_ASSERT_EQ(cit->second.d_appIds.size(), 1u);
        if (!cit->second.d_appIds.empty()) {
            BMQTST_ASSERT_EQ(cit->second.d_appIds[0].d_appId,
                             bsl::string(k_APP_ID, alloc));
            mqbu::StorageKey appKey;
            appKey.fromBinary(k_APP_KEY.data());
            BMQTST_ASSERT_EQ(cit->second.d_appIds[0].d_appKey, appKey);
        }
    }

    // -----------------------------------------------------------------
    // The CSL file header carries the input logId.
    // -----------------------------------------------------------------
    bsls::Types::Int64 fileSize = bdls::FilesystemUtil::getFileSize(cslPath);
    bsl::vector<char>  buf(static_cast<bsl::size_t>(fileSize), alloc);
    {
        bdls::FilesystemUtil::FileDescriptor fd = bdls::FilesystemUtil::open(
            cslPath.c_str(),
            bdls::FilesystemUtil::e_OPEN,
            bdls::FilesystemUtil::e_READ_ONLY);
        BMQTST_ASSERT_EQ(
            bdls::FilesystemUtil::read(fd,
                                       buf.data(),
                                       static_cast<int>(fileSize)),
            static_cast<int>(fileSize));
        bdls::FilesystemUtil::close(fd);
    }

    mqbc::ClusterStateFileHeader fileHeader;
    bsl::memcpy(&fileHeader, buf.data(), sizeof(fileHeader));
    mqbu::StorageKey expectedLogId = keyFromHex(
        bsl::string(k_LOG_ID_HEX, alloc));
    BMQTST_ASSERT_EQ(
        mqbc::ClusterStateLedgerUtil::validateFileHeader(fileHeader,
                                                         expectedLogId),
        0);
    BMQTST_ASSERT_EQ(fileHeader.fileKey(), expectedLogId);

    // -----------------------------------------------------------------
    // Exactly one well-formed record follows the header and decodes back to
    // the same advisory.
    // -----------------------------------------------------------------
    const int fileHeaderSize = fileHeader.headerWords() *
                               bmqp::Protocol::k_WORD_SIZE;

    mqbc::ClusterStateRecordHeader recHeader;
    bsl::memcpy(&recHeader, buf.data() + fileHeaderSize, sizeof(recHeader));
    BMQTST_ASSERT_EQ(
        mqbc::ClusterStateLedgerUtil::validateRecordHeader(recHeader),
        0);

    const int recSize = mqbc::ClusterStateLedgerUtil::recordSize(recHeader);
    // Header plus exactly one record consumes the whole file.
    BMQTST_ASSERT_EQ(fileHeaderSize + recSize, static_cast<int>(fileSize));

    bdlbb::PooledBlobBufferFactory factory(1024, alloc);
    bdlbb::Blob                    blob(&factory, alloc);
    bdlbb::BlobUtil::append(&blob, buf.data(), static_cast<int>(fileSize));

    bmqp_ctrlmsg::ClusterMessage decoded(alloc);
    BMQTST_ASSERT_EQ(
        mqbc::ClusterStateLedgerUtil::loadClusterMessage(&decoded,
                                                         blob,
                                                         fileHeaderSize),
        0);
    BMQTST_ASSERT(decoded.choice().isQueueAssignmentAdvisoryValue());
    if (decoded.choice().isQueueAssignmentAdvisoryValue()) {
        const bmqp_ctrlmsg::QueueAssignmentAdvisory& dqaa =
            decoded.choice().queueAssignmentAdvisory();
        BMQTST_ASSERT_EQ(dqaa.queues().size(), 1u);
        if (!dqaa.queues().empty()) {
            BMQTST_ASSERT_EQ(dqaa.queues()[0].uri(),
                             bsl::string(k_URI, alloc));
            BMQTST_ASSERT_EQ(dqaa.queues()[0].key(), k_QUEUE_KEY);
        }
    }

    bdls::FilesystemUtil::remove(dir, true);
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
    case 2: test2_roundTrip(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    // 'bsl::istream' (as used by the tool via 'bsl::ifstream') allocates from
    // the default allocator, so the default-allocator check is not applicable.
    TEST_EPILOG(bmqtst::TestHelper::e_DEFAULT);
}
