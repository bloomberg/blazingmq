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

#include <m_bmqstoragewriter_journalwriter.h>
#include <m_bmqstoragewriter_util.h>

// MQB
#include <mqbs_filestoreprotocol.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_protocolutil.h>
#include <bmqt_messageguid.h>

// BDE
#include <bdljsn_json.h>
#include <bdljsn_jsonutil.h>
#include <bsl_algorithm.h>
#include <bsl_cstring.h>
#include <bsl_iostream.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace m_bmqstoragewriter {

namespace {

// ============================================================================
//                              HELPERS
// ============================================================================

using m_bmqstoragewriter::getInt;
using m_bmqstoragewriter::getString;
using m_bmqstoragewriter::getUint64;
using m_bmqstoragewriter::keyFromHex;

bmqt::MessageGUID guidFromHex(const bsl::string& hex)
{
    bmqt::MessageGUID guid;
    if (hex.size() == bmqt::MessageGUID::e_SIZE_HEX) {
        guid.fromHex(hex.c_str());
    }
    return guid;
}

int decodeHexPayload(bsl::vector<char>* out, const bsl::string& hex)
{
    if (hex.size() % 2 != 0) {
        return -1;
    }
    out->resize(hex.size() / 2);
    for (bsl::size_t i = 0; i < hex.size(); i += 2) {
        char c = hex[i];
        int  hi, lo;
        if (c >= '0' && c <= '9')
            hi = c - '0';
        else if (c >= 'A' && c <= 'F')
            hi = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f')
            hi = c - 'a' + 10;
        else
            return -1;
        c = hex[i + 1];
        if (c >= '0' && c <= '9')
            lo = c - '0';
        else if (c >= 'A' && c <= 'F')
            lo = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f')
            lo = c - 'a' + 10;
        else
            return -1;
        (*out)[i / 2] = static_cast<char>((hi << 4) | lo);
    }
    return 0;
}

// ============================================================================
//                         RECORD WRITING
// ============================================================================

int writeJournalRecord(bdls::FilesystemUtil::FileDescriptor fd,
                       const void*                          record)
{
    int written = bdls::FilesystemUtil::write(
        fd,
        record,
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
    if (written != mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE) {
        bsl::cerr << "Error: failed to write journal record\n";
        return -1;
    }
    return 0;
}

int writeDataRecord(bdls::FilesystemUtil::FileDescriptor fd,
                    bsls::Types::Uint64*                 dataPos,
                    const bsl::vector<char>&             payload)
{
    using namespace mqbs;

    int unpaddedSize = sizeof(DataHeader) + static_cast<int>(payload.size());
    int padding      = 0;
    int numDwords    = bmqp::ProtocolUtil::calcNumDwordsAndPadding(&padding,
                                                                unpaddedSize);
    int totalSize    = numDwords * bmqp::Protocol::k_DWORD_SIZE;

    bsl::vector<char> buf(totalSize, 0);

    DataHeader* dh = new (buf.data()) DataHeader();
    dh->setHeaderWords(sizeof(DataHeader) / bmqp::Protocol::k_WORD_SIZE);
    dh->setMessageWords(totalSize / bmqp::Protocol::k_WORD_SIZE);

    bsl::memcpy(buf.data() + sizeof(DataHeader),
                payload.data(),
                payload.size());

    bmqp::ProtocolUtil::appendPaddingDwordRaw(buf.data() + sizeof(DataHeader) +
                                                  payload.size(),
                                              padding);

    int written = bdls::FilesystemUtil::write(fd, buf.data(), totalSize);
    if (written != totalSize) {
        bsl::cerr << "Error: failed to write data record\n";
        return -1;
    }

    *dataPos += totalSize;
    return 0;
}

int writeQlistRecord(bdls::FilesystemUtil::FileDescriptor fd,
                     bsls::Types::Uint64*                 qlistPos,
                     const bsl::string&                   uri,
                     const mqbu::StorageKey&              queueKey,
                     const bsl::vector<AppIdInfo>&        appIds)
{
    using namespace mqbs;

    int queueUriPadding = 0;
    int queueUriWords   = bmqp::ProtocolUtil::calcNumWordsAndPadding(
        &queueUriPadding,
        static_cast<int>(uri.size()));

    int totalLen = sizeof(QueueRecordHeader) + uri.size() + queueUriPadding +
                   FileStoreProtocol::k_HASH_LENGTH;

    bsl::vector<int> appIdPaddings(appIds.size());
    bsl::vector<int> appIdWordsVec(appIds.size());
    for (bsl::size_t i = 0; i < appIds.size(); ++i) {
        appIdWordsVec[i] = bmqp::ProtocolUtil::calcNumWordsAndPadding(
            &appIdPaddings[i],
            static_cast<int>(appIds[i].d_appId.size()));
        totalLen += sizeof(AppIdHeader) + appIds[i].d_appId.size() +
                    appIdPaddings[i] + FileStoreProtocol::k_HASH_LENGTH;
    }
    totalLen += sizeof(unsigned int);  // magic

    bsl::vector<char> buf(totalLen, 0);
    char*             pos = buf.data();

    QueueRecordHeader* qrh = new (pos) QueueRecordHeader();
    qrh->setQueueUriLengthWords(queueUriWords)
        .setNumAppIds(static_cast<unsigned int>(appIds.size()))
        .setQueueRecordWords(totalLen / bmqp::Protocol::k_WORD_SIZE);
    pos += sizeof(QueueRecordHeader);

    bsl::memcpy(pos, uri.c_str(), uri.size());
    pos += uri.size();

    bmqp::ProtocolUtil::appendPaddingRaw(pos, queueUriPadding);
    pos += queueUriPadding;

    char queueHash[FileStoreProtocol::k_HASH_LENGTH] = {0};
    bsl::memcpy(queueHash,
                queueKey.data(),
                mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    bsl::memcpy(pos, queueHash, FileStoreProtocol::k_HASH_LENGTH);
    pos += FileStoreProtocol::k_HASH_LENGTH;

    for (bsl::size_t i = 0; i < appIds.size(); ++i) {
        AppIdHeader* appIdHeader = new (pos) AppIdHeader();
        appIdHeader->setAppIdLengthWords(appIdWordsVec[i]);
        pos += sizeof(AppIdHeader);

        bsl::memcpy(pos, appIds[i].d_appId.c_str(), appIds[i].d_appId.size());
        pos += appIds[i].d_appId.size();

        bmqp::ProtocolUtil::appendPaddingRaw(pos, appIdPaddings[i]);
        pos += appIdPaddings[i];

        char appHash[FileStoreProtocol::k_HASH_LENGTH] = {0};
        bsl::memcpy(appHash,
                    appIds[i].d_appKey.data(),
                    mqbu::StorageKey::e_KEY_LENGTH_BINARY);
        bsl::memcpy(pos, appHash, FileStoreProtocol::k_HASH_LENGTH);
        pos += FileStoreProtocol::k_HASH_LENGTH;
    }

    bdlb::BigEndianUint32 magic;
    magic = QueueRecordHeader::k_MAGIC;
    bsl::memcpy(pos, &magic, sizeof(magic));

    int written = bdls::FilesystemUtil::write(fd, buf.data(), totalLen);
    if (written != totalLen) {
        bsl::cerr << "Error: failed to write qlist record\n";
        return -1;
    }

    *qlistPos += totalLen;
    return 0;
}

// ============================================================================
//                         RECORD PROCESSING
// ============================================================================

int processMessage(bdls::FilesystemUtil::FileDescriptor journalFd,
                   bdls::FilesystemUtil::FileDescriptor dataFd,
                   bsls::Types::Uint64*                 dataPos,
                   const bdljsn::JsonObject&            obj,
                   bslma::Allocator*                    allocator)
{
    using namespace mqbs;

    bsl::string       payloadHex = getString(obj, "Payload", allocator);
    bsl::vector<char> payload(allocator);
    if (!payloadHex.empty()) {
        if (decodeHexPayload(&payload, payloadHex) != 0) {
            bsl::cerr << "Error: invalid hex payload\n";
            return -1;
        }
    }

    bsls::Types::Uint64 msgOffsetDwords = *dataPos /
                                          bmqp::Protocol::k_DWORD_SIZE;

    int rc = writeDataRecord(dataFd, dataPos, payload);
    if (rc != 0)
        return rc;

    MessageRecord msgRec;
    bsl::memset(&msgRec, 0, sizeof(msgRec));
    new (&msgRec) MessageRecord();
    msgRec.header()
        .setType(RecordType::e_MESSAGE)
        .setPrimaryLeaseId(getInt(obj, "PrimaryLeaseId"))
        .setSequenceNumber(getUint64(obj, "SequenceNumber"))
        .setTimestamp(getUint64(obj, "Epoch"));
    msgRec.setRefCount(getInt(obj, "RefCount"));
    msgRec.setQueueKey(keyFromHex(getString(obj, "QueueKey", allocator)));
    msgRec.setFileKey(keyFromHex(getString(obj, "FileKey", allocator)));
    msgRec.setMessageOffsetDwords(static_cast<unsigned int>(msgOffsetDwords));
    msgRec.setMessageGUID(guidFromHex(getString(obj, "GUID", allocator)));
    msgRec.setCrc32c(static_cast<unsigned int>(getUint64(obj, "Crc32c")));
    msgRec.setMagic(RecordHeader::k_MAGIC);

    return writeJournalRecord(journalFd, &msgRec);
}

int processConfirm(bdls::FilesystemUtil::FileDescriptor journalFd,
                   const bdljsn::JsonObject&            obj,
                   bslma::Allocator*                    allocator)
{
    using namespace mqbs;

    ConfirmRecord cfmRec;
    bsl::memset(&cfmRec, 0, sizeof(cfmRec));
    new (&cfmRec) ConfirmRecord();
    cfmRec.header()
        .setType(RecordType::e_CONFIRM)
        .setPrimaryLeaseId(getInt(obj, "PrimaryLeaseId"))
        .setSequenceNumber(getUint64(obj, "SequenceNumber"))
        .setTimestamp(getUint64(obj, "Epoch"));
    cfmRec.setQueueKey(keyFromHex(getString(obj, "QueueKey", allocator)));
    cfmRec.setAppKey(keyFromHex(getString(obj, "AppKey", allocator)));
    cfmRec.setMessageGUID(guidFromHex(getString(obj, "GUID", allocator)));
    cfmRec.setMagic(RecordHeader::k_MAGIC);

    return writeJournalRecord(journalFd, &cfmRec);
}

int processDeletion(bdls::FilesystemUtil::FileDescriptor journalFd,
                    const bdljsn::JsonObject&            obj,
                    bslma::Allocator*                    allocator)
{
    using namespace mqbs;

    DeletionRecord delRec;
    bsl::memset(&delRec, 0, sizeof(delRec));
    new (&delRec) DeletionRecord();
    delRec.header()
        .setType(RecordType::e_DELETION)
        .setPrimaryLeaseId(getInt(obj, "PrimaryLeaseId"))
        .setSequenceNumber(getUint64(obj, "SequenceNumber"))
        .setTimestamp(getUint64(obj, "Epoch"));
    delRec.setQueueKey(keyFromHex(getString(obj, "QueueKey", allocator)));
    delRec.setMessageGUID(guidFromHex(getString(obj, "GUID", allocator)));

    bsl::string flag = getString(obj, "DeletionFlag", allocator);
    if (flag == "TTL_EXPIRATION") {
        delRec.setDeletionRecordFlag(DeletionRecordFlag::e_TTL_EXPIRATION);
    }
    else if (flag == "IMPLICIT_CONFIRM") {
        delRec.setDeletionRecordFlag(DeletionRecordFlag::e_IMPLICIT_CONFIRM);
    }
    else if (flag == "NO_SC_QUORUM") {
        delRec.setDeletionRecordFlag(DeletionRecordFlag::e_NO_SC_QUORUM);
    }
    delRec.setMagic(RecordHeader::k_MAGIC);

    return writeJournalRecord(journalFd, &delRec);
}

int processQueueOp(bdls::FilesystemUtil::FileDescriptor journalFd,
                   bdls::FilesystemUtil::FileDescriptor qlistFd,
                   bsls::Types::Uint64*                 qlistPos,
                   const QueueCache&                    cache,
                   const bdljsn::JsonObject&            obj,
                   bslma::Allocator*                    allocator)
{
    using namespace mqbs;

    bsl::string      queueKeyStr = getString(obj, "QueueKey", allocator);
    mqbu::StorageKey queueKey    = keyFromHex(queueKeyStr);
    bsl::string      opType      = getString(obj, "QueueOpType", allocator);

    if (opType == "CREATION" || opType == "ADDITION") {
        bsl::string            uri(allocator);
        bsl::vector<AppIdInfo> appIds(allocator);

        QueueCache::const_iterator cacheIt = cache.find(queueKey);
        if (cacheIt != cache.end()) {
            uri    = cacheIt->second.d_uri;
            appIds = cacheIt->second.d_appIds;
        }
        else {
            uri = queueKeyStr;
        }

        bsls::Types::Uint64 qlistOffset = *qlistPos;

        int rc = writeQlistRecord(qlistFd, qlistPos, uri, queueKey, appIds);
        if (rc != 0)
            return rc;

        QueueOpRecord qopRec;
        bsl::memset(&qopRec, 0, sizeof(qopRec));
        new (&qopRec) QueueOpRecord();
        qopRec.header()
            .setType(RecordType::e_QUEUE_OP)
            .setPrimaryLeaseId(getInt(obj, "PrimaryLeaseId"))
            .setSequenceNumber(getUint64(obj, "SequenceNumber"))
            .setTimestamp(getUint64(obj, "Epoch"));
        qopRec.setQueueKey(queueKey);
        qopRec.setAppKey(keyFromHex(getString(obj, "AppKey", allocator)));
        qopRec.setType(opType == "CREATION" ? QueueOpType::e_CREATION
                                            : QueueOpType::e_ADDITION);
        qopRec.setQueueUriRecordOffsetWords(static_cast<unsigned int>(
            qlistOffset / bmqp::Protocol::k_WORD_SIZE));
        qopRec.setMagic(RecordHeader::k_MAGIC);

        return writeJournalRecord(journalFd, &qopRec);
    }

    // DELETION or PURGE
    QueueOpRecord qopRec;
    bsl::memset(&qopRec, 0, sizeof(qopRec));
    new (&qopRec) QueueOpRecord();
    qopRec.header()
        .setType(RecordType::e_QUEUE_OP)
        .setPrimaryLeaseId(getInt(obj, "PrimaryLeaseId"))
        .setSequenceNumber(getUint64(obj, "SequenceNumber"))
        .setTimestamp(getUint64(obj, "Epoch"));
    qopRec.setQueueKey(queueKey);
    qopRec.setAppKey(keyFromHex(getString(obj, "AppKey", allocator)));
    qopRec.setType(opType == "DELETION" ? QueueOpType::e_DELETION
                                        : QueueOpType::e_PURGE);
    qopRec.setMagic(RecordHeader::k_MAGIC);

    return writeJournalRecord(journalFd, &qopRec);
}

int processJournalOp(bdls::FilesystemUtil::FileDescriptor journalFd,
                     bsls::Types::Uint64                  dataPos,
                     bsls::Types::Uint64                  qlistPos,
                     const bdljsn::JsonObject&            obj,
                     bslma::Allocator*                    allocator)
{
    using namespace mqbs;

    JournalOpRecord jopRec;
    bsl::memset(&jopRec, 0, sizeof(jopRec));
    new (&jopRec) JournalOpRecord();
    jopRec.header()
        .setType(RecordType::e_JOURNAL_OP)
        .setPrimaryLeaseId(getInt(obj, "PrimaryLeaseId"))
        .setSequenceNumber(getUint64(obj, "SequenceNumber"))
        .setTimestamp(getUint64(obj, "Epoch"));

    bsl::string jopType = getString(obj, "JournalOpType", allocator);
    if (jopType == "SYNCPOINT") {
        jopRec.setType(JournalOpType::e_SYNCPOINT);
    }

    bsl::string spType = getString(obj, "SyncPointType", allocator);
    if (spType == "REGULAR") {
        jopRec.setSyncPointType(SyncPointType::e_REGULAR);
    }
    else if (spType == "ROLLOVER") {
        jopRec.setSyncPointType(SyncPointType::e_ROLLOVER);
    }

    jopRec.setPrimaryLeaseId(getInt(obj, "SyncPtPrimaryLeaseId"));
    jopRec.setSequenceNum(getUint64(obj, "SyncPtSequenceNumber"));
    jopRec.setPrimaryNodeId(getInt(obj, "PrimaryNodeId"));
    jopRec.setDataFileOffsetDwords(
        static_cast<unsigned int>(dataPos / bmqp::Protocol::k_DWORD_SIZE));
    jopRec.setQlistFileOffsetWords(
        static_cast<unsigned int>(qlistPos / bmqp::Protocol::k_WORD_SIZE));
    jopRec.setMagic(RecordHeader::k_MAGIC);

    return writeJournalRecord(journalFd, &jopRec);
}

}  // close unnamed namespace

// ============================================================================
//                         PUBLIC FUNCTION
// ============================================================================

int processJournalInput(const QueueCache&                    cache,
                        bdls::FilesystemUtil::FileDescriptor journalFd,
                        bdls::FilesystemUtil::FileDescriptor dataFd,
                        bdls::FilesystemUtil::FileDescriptor qlistFd,
                        bsl::istream&                        input,
                        bslma::Allocator*                    allocator)
{
    bdljsn::Json  root;
    bdljsn::Error error;
    int           rc = bdljsn::JsonUtil::read(&root, &error, input);
    if (rc != 0) {
        bsl::cerr << "Error: invalid journal JSON: " << error.message()
                  << "\n";
        return -1;
    }

    if (!root.isObject()) {
        bsl::cerr << "Error: expected JSON object at top level\n";
        return -1;
    }

    const bdljsn::JsonObject&         rootObj = root.theObject();
    bdljsn::JsonObject::ConstIterator it      = rootObj.find("Records");
    if (it == rootObj.end() || !it->second.isArray()) {
        bsl::cerr << "Error: missing or invalid 'Records' array\n";
        return -1;
    }

    const bdljsn::JsonArray& arr = it->second.theArray();

    // Get current file positions (seek to end)
    bsls::Types::Uint64 dataPos = bdls::FilesystemUtil::seek(
        dataFd,
        0,
        bdls::FilesystemUtil::e_SEEK_FROM_END);
    bsls::Types::Uint64 qlistPos = bdls::FilesystemUtil::seek(
        qlistFd,
        0,
        bdls::FilesystemUtil::e_SEEK_FROM_END);
    bdls::FilesystemUtil::seek(journalFd,
                               0,
                               bdls::FilesystemUtil::e_SEEK_FROM_END);

    int numWritten = 0;

    // bmqstoragetool may emit records in a different order than their physical
    // journal offset (it groups message records separately from journal-op and
    // queue-op records).  Sort by the original "Offset" so records are written
    // back in the source's physical order.  This matters for correctness as
    // well as fidelity: syncpoint (JOURNAL_OP) records derive their data/qlist
    // file offsets from the running write position, so any message or queue-op
    // that physically preceded them must be written first.
    bsl::vector<bsl::pair<bsls::Types::Uint64, bsl::size_t> > order(allocator);
    order.reserve(arr.size());
    for (bsl::size_t i = 0; i < arr.size(); ++i) {
        if (!arr[i].isObject()) {
            continue;
        }
        order.push_back(
            bsl::make_pair(getUint64(arr[i].theObject(), "Offset"), i));
    }
    bsl::sort(order.begin(), order.end());

    for (bsl::size_t k = 0; k < order.size(); ++k) {
        const bdljsn::JsonObject& obj = arr[order[k].second].theObject();

        bsl::string recType = getString(obj, "RecordType", allocator);

        if (recType == "MESSAGE") {
            rc = processMessage(journalFd, dataFd, &dataPos, obj, allocator);
        }
        else if (recType == "CONFIRM") {
            rc = processConfirm(journalFd, obj, allocator);
        }
        else if (recType == "DELETION") {
            rc = processDeletion(journalFd, obj, allocator);
        }
        else if (recType == "QUEUE_OP") {
            rc = processQueueOp(journalFd,
                                qlistFd,
                                &qlistPos,
                                cache,
                                obj,
                                allocator);
        }
        else if (recType == "JOURNAL_OP") {
            rc =
                processJournalOp(journalFd, dataPos, qlistPos, obj, allocator);
        }
        else {
            bsl::cerr << "Warning: unknown RecordType '" << recType
                      << "', skipping\n";
            continue;
        }

        if (rc != 0) {
            return rc;
        }
        ++numWritten;
    }

    bsl::cout << "Wrote " << numWritten << " journal record(s)\n";
    return 0;
}

}  // close package namespace
}  // close enterprise namespace
