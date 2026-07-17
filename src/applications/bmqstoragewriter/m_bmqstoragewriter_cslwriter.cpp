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

#include <m_bmqstoragewriter_cslwriter.h>
#include <m_bmqstoragewriter_util.h>

// MQB
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbc_clusterstateledgerutil.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocolutil.h>

// BDE
#include <baljsn_decoder.h>
#include <baljsn_decoderoptions.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlde_base64encoder.h>
#include <bdlde_hexdecoder.h>
#include <bdljsn_jsonutil.h>
#include <bsl_iostream.h>
#include <bsl_sstream.h>

namespace BloombergLP {
namespace m_bmqstoragewriter {

namespace {

using m_bmqstoragewriter::getString;
using m_bmqstoragewriter::getUint64;
using m_bmqstoragewriter::keyFromHex;

void convertKeysToBase64(bsl::string* json, bslma::Allocator* allocator)
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

            bsl::string hexVal(json->data() + valStart,
                               valEnd - valStart,
                               allocator);

            bdlde::HexDecoder hexDecoder;
            bsl::vector<char> bin(allocator);
            bin.resize(hexVal.size() / 2);
            int numOut = 0, numIn = 0;
            int rc = hexDecoder.convert(bin.data(),
                                        &numOut,
                                        &numIn,
                                        hexVal.data(),
                                        hexVal.data() + hexVal.size());
            if (rc < 0) {
                pos = valEnd + 1;
                continue;
            }
            hexDecoder.endConvert();
            bin.resize(numOut);

            bdlde::Base64Encoder b64Encoder(0);
            bsl::string          b64(allocator);
            b64.resize(bdlde::Base64Encoder::encodedLength(
                static_cast<int>(bin.size()),
                0));
            int b64Out = 0, b64In = 0;
            b64Encoder.convert(b64.begin(),
                               &b64Out,
                               &b64In,
                               bin.data(),
                               bin.data() + bin.size());
            int b64End = 0;
            b64Encoder.endConvert(b64.begin() + b64Out, &b64End);
            b64.resize(b64Out + b64End);

            json->replace(valStart, valEnd - valStart, b64);
            pos = valStart + b64.size() + 1;
        }
    }
}

mqbc::ClusterStateRecordType::Enum
cslRecordTypeFromString(const bsl::string& str)
{
    if (str == "SNAPSHOT")
        return mqbc::ClusterStateRecordType::e_SNAPSHOT;
    if (str == "UPDATE")
        return mqbc::ClusterStateRecordType::e_UPDATE;
    if (str == "COMMIT")
        return mqbc::ClusterStateRecordType::e_COMMIT;
    if (str == "ACK")
        return mqbc::ClusterStateRecordType::e_ACK;
    return mqbc::ClusterStateRecordType::e_UNDEFINED;
}

void updateCacheFromRecord(QueueCache*               cache,
                           const bdljsn::JsonObject& record,
                           bslma::Allocator*         allocator)
{
    const bdljsn::JsonObject*         advisory = 0;
    bdljsn::JsonObject::ConstIterator advIt;

    advIt = record.find("leaderAdvisory");
    if (advIt != record.end() && advIt->second.isObject()) {
        advisory = &advIt->second.theObject();
    }
    if (!advisory) {
        advIt = record.find("queueAssignmentAdvisory");
        if (advIt != record.end() && advIt->second.isObject()) {
            advisory = &advIt->second.theObject();
        }
    }
    if (!advisory) {
        return;
    }

    bdljsn::JsonObject::ConstIterator queuesIt = advisory->find("queues");
    if (queuesIt == advisory->end() || !queuesIt->second.isArray()) {
        return;
    }

    const bdljsn::JsonArray& queues = queuesIt->second.theArray();
    for (bsl::size_t q = 0; q < queues.size(); ++q) {
        if (!queues[q].isObject()) {
            continue;
        }
        const bdljsn::JsonObject& qInfo = queues[q].theObject();

        bsl::string      keyStr   = getString(qInfo, "key", allocator);
        mqbu::StorageKey queueKey = keyFromHex(keyStr);
        if (queueKey.isNull()) {
            continue;
        }

        QueueCacheEntry entry(allocator);
        entry.d_uri = getString(qInfo, "uri", allocator);

        bdljsn::JsonObject::ConstIterator appIdsIt = qInfo.find("appIds");
        if (appIdsIt != qInfo.end() && appIdsIt->second.isArray()) {
            const bdljsn::JsonArray& appIds = appIdsIt->second.theArray();
            for (bsl::size_t a = 0; a < appIds.size(); ++a) {
                if (!appIds[a].isObject()) {
                    continue;
                }
                const bdljsn::JsonObject& appObj = appIds[a].theObject();
                AppIdInfo                 info(allocator);
                info.d_appId  = getString(appObj, "appId", allocator);
                info.d_appKey = keyFromHex(
                    getString(appObj, "appKey", allocator));
                entry.d_appIds.push_back(info);
            }
        }

        (*cache)[queueKey] = entry;
    }
}

int writeCslRecord(bdls::FilesystemUtil::FileDescriptor fd,
                   const bdljsn::JsonObject&            rec,
                   bslma::Allocator*                    allocator)
{
    bsl::string         recType     = getString(rec, "RecordType", allocator);
    bsls::Types::Uint64 electorTerm = getUint64(rec, "ElectorTerm");
    bsls::Types::Uint64 seqNum      = getUint64(rec, "SequenceNumber");
    bsls::Types::Uint64 epoch       = getUint64(rec, "Epoch");

    bdljsn::JsonObject::ConstIterator recIt = rec.find("Record");
    if (recIt == rec.end() || !recIt->second.isObject()) {
        bsl::cerr << "Error: CSL record missing 'Record' field\n";
        return -1;
    }

    bsl::string recordJson(allocator);
    bdljsn::JsonUtil::write(&recordJson, recIt->second);
    convertKeysToBase64(&recordJson, allocator);

    bmqp_ctrlmsg::ClusterMessage clusterMessage(allocator);
    baljsn::DecoderOptions       decoderOptions;
    decoderOptions.setSkipUnknownElements(true);
    baljsn::Decoder decoder;

    bsl::istringstream iss(recordJson);
    int rc = decoder.decode(iss, &clusterMessage, decoderOptions);
    if (rc != 0) {
        bsl::cerr << "Error: failed to decode CSL Record JSON: "
                  << decoder.loggedMessages() << "\n";
        return -1;
    }

    bmqp_ctrlmsg::LeaderMessageSequence lms;
    lms.electorTerm()    = electorTerm;
    lms.sequenceNumber() = seqNum;

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, allocator);
    bdlbb::Blob                    blob(&bufferFactory, allocator);

    rc = mqbc::ClusterStateLedgerUtil::appendRecord(
        &blob,
        clusterMessage,
        lms,
        epoch,
        cslRecordTypeFromString(recType));
    if (rc != 0) {
        bsl::cerr << "Error: appendRecord failed (rc=" << rc << ")\n";
        return -1;
    }

    int               totalLen = blob.length();
    bsl::vector<char> buf(totalLen, '\0', allocator);
    bdlbb::BlobUtil::copy(buf.data(), blob, 0, totalLen);

    int written = bdls::FilesystemUtil::write(fd, buf.data(), totalLen);
    if (written != totalLen) {
        bsl::cerr << "Error: failed to write CSL record to file\n";
        return -1;
    }

    return 0;
}

}  // close unnamed namespace

int processCslInput(QueueCache*                          cache,
                    bdls::FilesystemUtil::FileDescriptor cslFd,
                    bool                                 newFile,
                    bsl::istream&                        input,
                    bslma::Allocator*                    allocator)
{
    bdljsn::Json  root;
    bdljsn::Error error;
    int           rc = bdljsn::JsonUtil::read(&root, &error, input);
    if (rc != 0) {
        bsl::cerr << "Error: invalid CSL JSON: " << error.message() << "\n";
        return -1;
    }

    if (!root.isObject()) {
        bsl::cerr << "Error: expected JSON object at top level\n";
        return -1;
    }

    const bdljsn::JsonObject&         rootObj = root.theObject();
    bdljsn::JsonObject::ConstIterator it      = rootObj.find("Records");
    if (it == rootObj.end() || !it->second.isArray()) {
        bsl::cerr << "Error: missing or invalid 'Records' array in CSL\n";
        return -1;
    }

    const bdljsn::JsonArray& arr = it->second.theArray();
    mqbu::StorageKey         existingLogId;
    int                      numWritten = 0;

    for (bsl::size_t i = 0; i < arr.size(); ++i) {
        if (!arr[i].isObject()) {
            continue;
        }
        const bdljsn::JsonObject& rec = arr[i].theObject();

        bdljsn::JsonObject::ConstIterator recIt = rec.find("Record");
        if (recIt == rec.end() || !recIt->second.isObject()) {
            continue;
        }

        mqbu::StorageKey logId = keyFromHex(
            getString(rec, "LogId", allocator));

        if (newFile) {
            mqbc::ClusterStateFileHeader cslFileHeader;
            cslFileHeader.setFileKey(logId);
            bdls::FilesystemUtil::write(cslFd,
                                        &cslFileHeader,
                                        sizeof(cslFileHeader));
            existingLogId = logId;
            newFile       = false;
        }
        else if (existingLogId.isNull()) {
            mqbc::ClusterStateFileHeader hdr;
            bdls::FilesystemUtil::read(cslFd, &hdr, sizeof(hdr));
            existingLogId = hdr.fileKey();
            bdls::FilesystemUtil::seek(cslFd,
                                       0,
                                       bdls::FilesystemUtil::e_SEEK_FROM_END);
            if (logId != existingLogId) {
                bsl::cerr << "Warning: input LogId " << logId
                          << " differs from existing " << existingLogId
                          << "; using existing\n";
            }
            logId = existingLogId;
        }

        updateCacheFromRecord(cache, recIt->second.theObject(), allocator);

        rc = writeCslRecord(cslFd, rec, allocator);
        if (rc != 0) {
            return rc;
        }
        ++numWritten;
    }

    bsl::cout << "Wrote " << numWritten << " CSL record(s), cache has "
              << cache->size() << " queue(s)\n";
    return 0;
}

}  // close package namespace
}  // close enterprise namespace
