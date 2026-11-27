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
#include <m_bmqstoragewriter_journalwriter.h>
#include <m_bmqstoragewriter_util.h>

// MQB
#include <mqbs_datafileiterator.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_filestoreprotocolutil.h>
#include <mqbs_filesystemutil.h>
#include <mqbs_journalfileiterator.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbs_qlistfileiterator.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_protocol.h>
#include <bmqt_messageguid.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdls_filesystemutil.h>
#include <bsl_cstring.h>
#include <bsl_sstream.h>
#include <bsl_string.h>

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

/// Create the file at the specified `path` with a `FileHeader` of the
/// specified `fileType` followed by the appropriate per-type header, matching
/// what the tool's `createFileSet` writes.  Return true on success.
bool createFile(const bsl::string& path, mqbs::FileType::Enum fileType)
{
    bdls::FilesystemUtil::FileDescriptor fd = bdls::FilesystemUtil::open(
        path.c_str(),
        bdls::FilesystemUtil::e_CREATE,
        bdls::FilesystemUtil::e_READ_WRITE);
    if (fd == bdls::FilesystemUtil::k_INVALID_FD) {
        return false;
    }

    mqbs::FileHeader fh;
    fh.setFileType(fileType).setPartitionId(0);
    bool ok = bdls::FilesystemUtil::write(fd, &fh, sizeof(fh)) ==
              static_cast<int>(sizeof(fh));

    if (ok && fileType == mqbs::FileType::e_DATA) {
        mqbs::DataFileHeader dfh;
        dfh.setFileKey(mqbu::StorageKey::k_NULL_KEY);
        ok = bdls::FilesystemUtil::write(fd, &dfh, sizeof(dfh)) ==
             static_cast<int>(sizeof(dfh));
    }
    else if (ok && fileType == mqbs::FileType::e_JOURNAL) {
        mqbs::JournalFileHeader jfh;
        ok = bdls::FilesystemUtil::write(fd, &jfh, sizeof(jfh)) ==
             static_cast<int>(sizeof(jfh));
    }
    else if (ok) {
        mqbs::QlistFileHeader qfh;
        ok = bdls::FilesystemUtil::write(fd, &qfh, sizeof(qfh)) ==
             static_cast<int>(sizeof(qfh));
    }

    bdls::FilesystemUtil::close(fd);
    return ok;
}

bdls::FilesystemUtil::FileDescriptor openForAppend(const bsl::string& path)
{
    return bdls::FilesystemUtil::open(path.c_str(),
                                      bdls::FilesystemUtil::e_OPEN,
                                      bdls::FilesystemUtil::e_READ_WRITE);
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
//   Empty and malformed input are handled without writing records.
//
// Testing:
//   processJournalInput
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

    bsl::string dir(alloc);
    BMQTST_ASSERT_EQ(bdls::FilesystemUtil::createTemporaryDirectory(&dir,
                                                                    "bmqsw"),
                     0);

    bsl::string journalPath(dir, alloc);
    journalPath.append("/j.bmq_journal");
    bsl::string dataPath(dir, alloc);
    dataPath.append("/j.bmq_data");
    bsl::string qlistPath(dir, alloc);
    qlistPath.append("/j.bmq_qlist");

    BMQTST_ASSERT(createFile(journalPath, mqbs::FileType::e_JOURNAL));
    BMQTST_ASSERT(createFile(dataPath, mqbs::FileType::e_DATA));
    BMQTST_ASSERT(createFile(qlistPath, mqbs::FileType::e_QLIST));

    QueueCache cache(alloc);

    // Not valid JSON.
    {
        bdls::FilesystemUtil::FileDescriptor jFd = openForAppend(journalPath);
        bdls::FilesystemUtil::FileDescriptor dFd = openForAppend(dataPath);
        bdls::FilesystemUtil::FileDescriptor qFd = openForAppend(qlistPath);

        bsl::istringstream iss(bsl::string("not json", alloc), alloc);
        BMQTST_ASSERT_NE(processJournalInput(cache, jFd, dFd, qFd, iss, alloc),
                         0);

        bdls::FilesystemUtil::close(jFd);
        bdls::FilesystemUtil::close(dFd);
        bdls::FilesystemUtil::close(qFd);
    }

    // Valid JSON, empty Records array: succeeds, writes nothing.
    {
        bdls::FilesystemUtil::FileDescriptor jFd = openForAppend(journalPath);
        bdls::FilesystemUtil::FileDescriptor dFd = openForAppend(dataPath);
        bdls::FilesystemUtil::FileDescriptor qFd = openForAppend(qlistPath);

        bsl::istringstream iss(bsl::string("{\"Records\":[]}", alloc), alloc);
        BMQTST_ASSERT_EQ(processJournalInput(cache, jFd, dFd, qFd, iss, alloc),
                         0);

        bdls::FilesystemUtil::close(jFd);
        bdls::FilesystemUtil::close(dFd);
        bdls::FilesystemUtil::close(qFd);
    }

    // Journal still only holds its header (no records appended).
    BMQTST_ASSERT_EQ(
        bdls::FilesystemUtil::getFileSize(journalPath),
        static_cast<bsls::Types::Int64>(sizeof(mqbs::FileHeader) +
                                        sizeof(mqbs::JournalFileHeader)));

    bdls::FilesystemUtil::remove(dir, true);
}

static void test2_roundTrip()
// ------------------------------------------------------------------------
// ROUND TRIP TEST
//
// Concerns:
//   Records written from JSON are read back by the broker's own file
//   iterators with the same field values, and derived data/qlist offsets are
//   correct.
//
// Testing:
//   processJournalInput
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ROUND TRIP TEST");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

    bsl::string dir(alloc);
    BMQTST_ASSERT_EQ(bdls::FilesystemUtil::createTemporaryDirectory(&dir,
                                                                    "bmqsw"),
                     0);

    bsl::string journalPath(dir, alloc);
    journalPath.append("/j.bmq_journal");
    bsl::string dataPath(dir, alloc);
    dataPath.append("/j.bmq_data");
    bsl::string qlistPath(dir, alloc);
    qlistPath.append("/j.bmq_qlist");

    BMQTST_ASSERT(createFile(journalPath, mqbs::FileType::e_JOURNAL));
    BMQTST_ASSERT(createFile(dataPath, mqbs::FileType::e_DATA));
    BMQTST_ASSERT(createFile(qlistPath, mqbs::FileType::e_QLIST));

    const char* k_QUEUE_KEY_HEX = "0102030405";
    const char* k_APP_KEY_HEX   = "0A0B0C0D0E";
    const char* k_GUID_HEX      = "00000000000000000000000000ABCDEF";
    const char* k_URI           = "bmq://foo.bar/baz";
    const char* k_APP_ID        = "myapp";
    const char* k_PAYLOAD_HEX   = "DEADBEEF01";  // 5 bytes

    // Populate the cache so the QUEUE_OP CREATION emits a qlist record with
    // the queue uri and appId.
    QueueCache       cache(alloc);
    mqbu::StorageKey queueKey = keyFromHex(
        bsl::string(k_QUEUE_KEY_HEX, alloc));
    {
        QueueCacheEntry entry(alloc);
        entry.d_uri = k_URI;
        AppIdInfo app(alloc);
        app.d_appId  = k_APP_ID;
        app.d_appKey = keyFromHex(bsl::string(k_APP_KEY_HEX, alloc));
        entry.d_appIds.push_back(app);
        cache[queueKey] = entry;
    }

    // Offsets order processing: QUEUE_OP, MESSAGE, CONFIRM, DELETION and
    // finally JOURNAL_OP so its derived offsets follow the data/qlist writes.
    bmqu::MemOutStream json(alloc);
    json << "{\"Records\":["
         << "{\"RecordType\":\"QUEUE_OP\",\"Offset\":\"100\","
         << "\"PrimaryLeaseId\":\"1\",\"SequenceNumber\":\"10\","
         << "\"Epoch\":\"111\",\"QueueKey\":\"" << k_QUEUE_KEY_HEX << "\","
         << "\"QueueOpType\":\"CREATION\"},"
         << "{\"RecordType\":\"MESSAGE\",\"Offset\":\"200\","
         << "\"PrimaryLeaseId\":\"1\",\"SequenceNumber\":\"11\","
         << "\"Epoch\":\"222\",\"RefCount\":\"3\",\"QueueKey\":\""
         << k_QUEUE_KEY_HEX << "\",\"GUID\":\"" << k_GUID_HEX << "\","
         << "\"Crc32c\":\"12345\",\"Payload\":\"" << k_PAYLOAD_HEX << "\"},"
         << "{\"RecordType\":\"CONFIRM\",\"Offset\":\"300\","
         << "\"PrimaryLeaseId\":\"1\",\"SequenceNumber\":\"12\","
         << "\"Epoch\":\"333\",\"QueueKey\":\"" << k_QUEUE_KEY_HEX << "\","
         << "\"AppKey\":\"" << k_APP_KEY_HEX << "\",\"GUID\":\"" << k_GUID_HEX
         << "\"},"
         << "{\"RecordType\":\"DELETION\",\"Offset\":\"400\","
         << "\"PrimaryLeaseId\":\"1\",\"SequenceNumber\":\"13\","
         << "\"Epoch\":\"444\",\"QueueKey\":\"" << k_QUEUE_KEY_HEX << "\","
         << "\"GUID\":\"" << k_GUID_HEX
         << "\",\"DeletionFlag\":\"TTL_EXPIRATION\"},"
         << "{\"RecordType\":\"JOURNAL_OP\",\"Offset\":\"500\","
         << "\"PrimaryLeaseId\":\"1\",\"SequenceNumber\":\"14\","
         << "\"Epoch\":\"555\",\"JournalOpType\":\"SYNCPOINT\","
         << "\"SyncPointType\":\"REGULAR\",\"SyncPtPrimaryLeaseId\":\"1\","
         << "\"SyncPtSequenceNumber\":\"14\",\"PrimaryNodeId\":\"7\"}"
         << "]}";

    const bsls::Types::Int64 dataStart = sizeof(mqbs::FileHeader) +
                                         sizeof(mqbs::DataFileHeader);
    const bsls::Types::Int64 qlistStart = sizeof(mqbs::FileHeader) +
                                          sizeof(mqbs::QlistFileHeader);

    {
        bdls::FilesystemUtil::FileDescriptor jFd = openForAppend(journalPath);
        bdls::FilesystemUtil::FileDescriptor dFd = openForAppend(dataPath);
        bdls::FilesystemUtil::FileDescriptor qFd = openForAppend(qlistPath);

        bsl::istringstream iss(bsl::string(json.str(), alloc), alloc);
        BMQTST_ASSERT_EQ(processJournalInput(cache, jFd, dFd, qFd, iss, alloc),
                         0);

        bdls::FilesystemUtil::close(jFd);
        bdls::FilesystemUtil::close(dFd);
        bdls::FilesystemUtil::close(qFd);
    }

    const bsls::Types::Int64 dataEnd = bdls::FilesystemUtil::getFileSize(
        dataPath);
    const bsls::Types::Int64 qlistEnd = bdls::FilesystemUtil::getFileSize(
        qlistPath);

    // -----------------------------------------------------------------
    // Read back the journal records.
    // -----------------------------------------------------------------
    mqbs::MappedFileDescriptor jMfd;
    bmqu::MemOutStream         err(alloc);
    BMQTST_ASSERT_EQ(mqbs::FileSystemUtil::open(
                         &jMfd,
                         journalPath.c_str(),
                         bdls::FilesystemUtil::getFileSize(journalPath),
                         true,
                         err),
                     0);
    BMQTST_ASSERT_EQ(mqbs::FileStoreProtocolUtil::hasBmqHeader(jMfd), 0);

    mqbs::JournalFileIterator jit;
    BMQTST_ASSERT_EQ(jit.reset(&jMfd,
                               mqbs::FileStoreProtocolUtil::bmqHeader(jMfd)),
                     0);

    // Record 1: QUEUE_OP CREATION.
    BMQTST_ASSERT_EQ(jit.nextRecord(), 1);
    BMQTST_ASSERT_EQ(jit.recordType(), mqbs::RecordType::e_QUEUE_OP);
    {
        const mqbs::QueueOpRecord& r = jit.asQueueOpRecord();
        BMQTST_ASSERT_EQ(r.type(), mqbs::QueueOpType::e_CREATION);
        BMQTST_ASSERT_EQ(r.queueKey(), queueKey);
        BMQTST_ASSERT_EQ(r.header().sequenceNumber(), 10ULL);
        // The queue uri record was written at the start of the qlist file.
        BMQTST_ASSERT_EQ(r.queueUriRecordOffsetWords() *
                             bmqp::Protocol::k_WORD_SIZE,
                         static_cast<unsigned int>(qlistStart));
    }

    // Record 2: MESSAGE.
    BMQTST_ASSERT_EQ(jit.nextRecord(), 1);
    BMQTST_ASSERT_EQ(jit.recordType(), mqbs::RecordType::e_MESSAGE);
    unsigned int msgDataOffsetDwords = 0;
    {
        const mqbs::MessageRecord& r = jit.asMessageRecord();
        BMQTST_ASSERT_EQ(r.queueKey(), queueKey);
        BMQTST_ASSERT_EQ(r.refCount(), 3u);
        BMQTST_ASSERT_EQ(r.crc32c(), 12345u);
        bmqt::MessageGUID guid;
        guid.fromHex(k_GUID_HEX);
        BMQTST_ASSERT_EQ(r.messageGUID(), guid);
        // Message data begins at the start of the data file.
        BMQTST_ASSERT_EQ(r.messageOffsetDwords() *
                             bmqp::Protocol::k_DWORD_SIZE,
                         static_cast<unsigned int>(dataStart));
        msgDataOffsetDwords = r.messageOffsetDwords();
    }

    // Record 3: CONFIRM.
    BMQTST_ASSERT_EQ(jit.nextRecord(), 1);
    BMQTST_ASSERT_EQ(jit.recordType(), mqbs::RecordType::e_CONFIRM);
    {
        const mqbs::ConfirmRecord& r = jit.asConfirmRecord();
        BMQTST_ASSERT_EQ(r.queueKey(), queueKey);
        BMQTST_ASSERT_EQ(r.appKey(),
                         keyFromHex(bsl::string(k_APP_KEY_HEX, alloc)));
    }

    // Record 4: DELETION.
    BMQTST_ASSERT_EQ(jit.nextRecord(), 1);
    BMQTST_ASSERT_EQ(jit.recordType(), mqbs::RecordType::e_DELETION);
    {
        const mqbs::DeletionRecord& r = jit.asDeletionRecord();
        BMQTST_ASSERT_EQ(r.queueKey(), queueKey);
        BMQTST_ASSERT_EQ(r.deletionRecordFlag(),
                         mqbs::DeletionRecordFlag::e_TTL_EXPIRATION);
    }

    // Record 5: JOURNAL_OP SYNCPOINT with derived offsets.
    BMQTST_ASSERT_EQ(jit.nextRecord(), 1);
    BMQTST_ASSERT_EQ(jit.recordType(), mqbs::RecordType::e_JOURNAL_OP);
    {
        const mqbs::JournalOpRecord& r = jit.asJournalOpRecord();
        BMQTST_ASSERT_EQ(r.syncPointType(), mqbs::SyncPointType::e_REGULAR);
        BMQTST_ASSERT_EQ(r.primaryNodeId(), 7);
        // Offsets recorded after the message and queue-op writes.
        BMQTST_ASSERT_EQ(r.dataFileOffsetDwords() *
                             bmqp::Protocol::k_DWORD_SIZE,
                         static_cast<unsigned int>(dataEnd));
        BMQTST_ASSERT_EQ(r.qlistFileOffsetWords() *
                             bmqp::Protocol::k_WORD_SIZE,
                         static_cast<unsigned int>(qlistEnd));
    }

    BMQTST_ASSERT_EQ(jit.nextRecord(), 0);
    mqbs::FileSystemUtil::close(&jMfd);

    // -----------------------------------------------------------------
    // Read back the data record payload.
    // -----------------------------------------------------------------
    mqbs::MappedFileDescriptor dMfd;
    BMQTST_ASSERT_EQ(
        mqbs::FileSystemUtil::open(&dMfd,
                                   dataPath.c_str(),
                                   bdls::FilesystemUtil::getFileSize(dataPath),
                                   true,
                                   err),
        0);
    BMQTST_ASSERT_EQ(mqbs::FileStoreProtocolUtil::hasBmqHeader(dMfd), 0);

    mqbs::DataFileIterator dit;
    BMQTST_ASSERT_EQ(dit.reset(&dMfd,
                               mqbs::FileStoreProtocolUtil::bmqHeader(dMfd)),
                     0);
    BMQTST_ASSERT_EQ(dit.nextRecord(), 1);
    {
        const char*  data   = 0;
        unsigned int length = 0;
        dit.loadApplicationData(&data, &length);
        BMQTST_ASSERT_EQ(length, 5u);
        const unsigned char expected[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01};
        BMQTST_ASSERT_EQ(bsl::memcmp(data, expected, 5), 0);
        BMQTST_ASSERT_EQ(
            dit.recordOffset(),
            static_cast<bsls::Types::Uint64>(msgDataOffsetDwords *
                                             bmqp::Protocol::k_DWORD_SIZE));
    }
    mqbs::FileSystemUtil::close(&dMfd);

    // -----------------------------------------------------------------
    // Read back the qlist record uri and appId.
    // -----------------------------------------------------------------
    mqbs::MappedFileDescriptor qMfd;
    BMQTST_ASSERT_EQ(mqbs::FileSystemUtil::open(
                         &qMfd,
                         qlistPath.c_str(),
                         bdls::FilesystemUtil::getFileSize(qlistPath),
                         true,
                         err),
                     0);
    BMQTST_ASSERT_EQ(mqbs::FileStoreProtocolUtil::hasBmqHeader(qMfd), 0);

    mqbs::QlistFileIterator qit;
    BMQTST_ASSERT_EQ(qit.reset(&qMfd,
                               mqbs::FileStoreProtocolUtil::bmqHeader(qMfd)),
                     0);
    BMQTST_ASSERT_EQ(qit.nextRecord(), 1);
    {
        const char*  uri    = 0;
        unsigned int uriLen = 0;
        qit.loadQueueUri(&uri, &uriLen);
        BMQTST_ASSERT_EQ(bsl::string(uri, uriLen, alloc),
                         bsl::string(k_URI, alloc));

        BMQTST_ASSERT_EQ(qit.numAppIds(), 1u);
        bsl::vector<mqbs::QlistFileIterator::AppIdLengthPair> appIds(alloc);
        qit.loadAppIds(&appIds);
        BMQTST_ASSERT_EQ(appIds.size(), 1u);
        BMQTST_ASSERT_EQ(bsl::string(appIds[0].first, appIds[0].second, alloc),
                         bsl::string(k_APP_ID, alloc));
    }
    mqbs::FileSystemUtil::close(&qMfd);

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
