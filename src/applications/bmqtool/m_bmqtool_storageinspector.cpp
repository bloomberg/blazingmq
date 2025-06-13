// Copyright 2015-2023 Bloomberg Finance L.P.
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

// m_bmqtool_storageinspector.cpp                                     -*-C++-*-
#include <m_bmqtool_storageinspector.h>

// BMQTOOL
#include <m_bmqtool_inpututil.h>

// MQB
#include <mqbs_filestoreprotocolprinter.h>
#include <mqbs_filestoreprotocolutil.h>
#include <mqbs_filesystemutil.h>
#include <mqbs_offsetptr.h>

// BMQ
#include <bmqp_messageproperties.h>
#include <bmqp_optionsview.h>
#include <bmqp_protocol.h>

// BMQ
#include <bmqu_alignedprinter.h>
#include <bmqu_memoutstream.h>
#include <bmqu_outstreamformatsaver.h>
#include <bmqu_stringutil.h>

// BDE
#include <ball_log.h>
#include <bdlb_print.h>
#include <bdlbb_blob.h>
#include <bdls_filesystemutil.h>
#include <bdlt_datetime.h>
#include <bdlt_epochutil.h>
#include <bsl_algorithm.h>
#include <bsla_annotations.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>

namespace BloombergLP {
namespace m_bmqtool {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("STORAGEINSPECTOR");

using namespace mqbs::FileStoreProtocolPrinter;

/// Decode the specified `jsonInput` into the specified `command` object of
/// type `CMD`.  Verify that all parameters in the `specified `requiredArgs'
/// (a comma separated list of names) are present in `jsonInput`.  Return
/// true on success and false on error.
template <typename CMD>
bool parseCommand(CMD* command, const bsl::string& jsonInput)
{
    bsl::istringstream     is(jsonInput);
    baljsn::DecoderOptions options;
    options.setSkipUnknownElements(true);
    baljsn::Decoder decoder;
    int             rc = decoder.decode(is, command, options);
    if (rc != 0) {
        BALL_LOG_ERROR << "Unable to decode: " << jsonInput << bsl::endl
                       << decoder.loggedMessages();
        return false;  // RETURN
    }

    return true;
}

template <typename ITER>
bool resetIterator(mqbs::MappedFileDescriptor* mfd,
                   ITER*                       iter,
                   const char*                 filename)
{
    if (!bdls::FilesystemUtil::isRegularFile(filename)) {
        BALL_LOG_ERROR << "File [" << filename << "] is not a regular file.";
        return false;  // RETURN
    }

    // 1) Open
    bmqu::MemOutStream errorDesc;
    int                rc = mqbs::FileSystemUtil::open(
        mfd,
        filename,
        bdls::FilesystemUtil::getFileSize(filename),
        true,  // read only
        errorDesc);
    if (0 != rc) {
        BALL_LOG_ERROR << "Failed to open file [" << filename << "] rc: " << rc
                       << ", error: " << errorDesc.str();
        return false;  // RETURN
    }

    // 2) Basic sanity check
    rc = mqbs::FileStoreProtocolUtil::hasBmqHeader(*mfd);
    if (0 != rc) {
        BALL_LOG_ERROR << "Missing BlazingMQ header from file [" << filename
                       << "] rc: " << rc;
        mqbs::FileSystemUtil::close(mfd);
        return false;  // RETURN
    }

    // 3) Load iterator and check
    rc = iter->reset(mfd, mqbs::FileStoreProtocolUtil::bmqHeader(*mfd));
    if (0 != rc) {
        BALL_LOG_ERROR << "Failed to create iterator for file [" << filename
                       << "] rc: " << rc;
        mqbs::FileSystemUtil::close(mfd);
        return false;  // RETURN
    }

    BSLS_ASSERT_OPT(iter->isValid());
    return true;  // RETURN
}

/// Iterate to the next position (record) in the file currently
/// opened by the iterator.  Verify that the file and iterator are in a
/// valid state; reset them if this is not the case.  Pass parameters to
/// indicate how many records to skip and whether that is forward
/// or backwards.
template <typename CHOICE, typename ITER>
void iterateNextPosition(CHOICE&                     choice,
                         mqbs::MappedFileDescriptor* mfd,
                         ITER*                       iter,
                         const char*                 filename)
{
    bsls::Types::Uint64 skip    = 0;
    bool                reverse = false;
    bool                verbose = false;

    switch (choice.selectionId()) {
    case CHOICE::SELECTION_ID_N: {
        reverse = false;
        skip    = choice.n();
    } break;
    case CHOICE::SELECTION_ID_NEXT: {
        reverse = false;
        skip    = choice.next();
    } break;
    case CHOICE::SELECTION_ID_P: {
        reverse = true;
        skip    = choice.p();
    } break;
    case CHOICE::SELECTION_ID_PREV: {
        reverse = true;
        skip    = choice.prev();
    } break;
    case CHOICE::SELECTION_ID_RECORD: {
        skip = choice.record();

        if (skip >= iter->recordIndex()) {
            // If the skip is greater than our current index, than we are
            // iterating forward to reach the specified index.
            skip -= iter->recordIndex();
            reverse = false;
        }
        else {
            skip    = iter->recordIndex() - skip;
            reverse = true;
        }
    } break;
    case CHOICE::SELECTION_ID_R: {
        skip = choice.r();

        if (skip >= iter->recordIndex()) {
            // If the skip is greater than our current index, than we are
            // iterating forward to reach the specified index.
            skip -= iter->recordIndex();
            reverse = false;
        }
        else {
            skip    = iter->recordIndex() - skip;
            reverse = true;
        }
    } break;
    case CHOICE::SELECTION_ID_LIST: {
        int list = choice.list();
        verbose  = true;
        if (list >= 0) {
            reverse = false;
        }
        else {
            reverse = true;
            list *= -1;
        }
        skip = list;
    } break;
    case CHOICE::SELECTION_ID_L: {
        int list = choice.l();
        verbose  = true;
        if (list >= 0) {
            reverse = false;
        }
        else {
            reverse = true;
            list *= -1;
        }
        skip = list;
    } break;
    default:
        BALL_LOG_ERROR << "Unsupported choice: " << choice.selectionId();
        return;  // RETURN
    }

    if (!iter->isValid()) {
        // If the iterator is invalid, it means that we have cleared it at some
        // point, so we want to reset the state.
        if (!resetIterator(mfd, iter, filename)) {
            BALL_LOG_ERROR << "Iterator is invalid.  File may be corrupt.";
            return;  // RETURN
        }
    }

    if (iter->isReverseMode() != reverse) {
        iter->flipDirection();
    }

    bmqu::MemOutStream oss;
    while (skip > 0) {
        if (iter->hasRecordSizeRemaining() == false) {
            if (verbose) {
                BALL_LOG_INFO << oss.str() << bsl::endl;
            }
            BALL_LOG_INFO << "Ran out of records while iterating.";
            return;  // RETURN
        }

        int rc = iter->nextRecord();
        if (rc <= 0) {
            BALL_LOG_ERROR << "Iteration aborted (exit status " << rc << ")."
                           << "  State has been cleared.";
            return;  // RETURN
        }
        else if (verbose) {
            // In verbose mode, we print a summary of each record as we
            // encounter it.
            oss << *iter;
        }
        skip--;
    }

    if (iter->firstRecordPosition() > iter->recordOffset()) {
        BALL_LOG_ERROR << "Cannot print record as it is a header.";
        return;  // RETURN
    }
    else if (verbose) {
        BALL_LOG_INFO << oss.str() << bsl::endl;
    }
    else {
        // Print out record.  We can only do this safely because 'nextRecord'
        // returned 1.
        printIterator(*iter);
    }
}

}  // close unnamed namespace

// ----------------------
// class StorageInspector
// ----------------------

void StorageInspector::printHelp() const
{
    BALL_LOG_INFO << bsl::endl
                  << "Commands:" << bsl::endl
                  << "  open path=\"\"" << bsl::endl
                  << "  close" << bsl::endl
                  << "  queues" << bsl::endl
                  << "  metadata" << bsl::endl
                  << "  dump uri=\"\" (deleted=false) (messages=false)"
                  << bsl::endl
                  << "  help" << bsl::endl
                  << "  quit" << bsl::endl
                  << "  bye" << bsl::endl
                  << "\nData Commands:" << bsl::endl
                  << "  d n/next=1" << bsl::endl
                  << "  d p/prev=1" << bsl::endl
                  << "  d r/record=1" << bsl::endl
                  << "  d l/list=1" << bsl::endl
                  << "\nQlist Commands:" << bsl::endl
                  << "  q n/next=1" << bsl::endl
                  << "  q p/prev=1" << bsl::endl
                  << "  q r/record=1" << bsl::endl
                  << "  q l/list=1" << bsl::endl
                  << "\nJournal Commands:" << bsl::endl
                  << "  j n/next=1" << bsl::endl
                  << "  j p/prev=1" << bsl::endl
                  << "  j r/record=1" << bsl::endl
                  << "  j l/list=1" << bsl::endl
                  << "  j type={\"message\", \"confirm\", \"delete\","
                  << " \"qop\", \"jop\"}" << bsl::endl
                  << "  j dump=\"payload\"" << bsl::endl;
}

void StorageInspector::processCommand(const OpenStorageCommand& command)
{
    if (d_isOpen) {
        BALL_LOG_ERROR << "A storage is already open.";
        return;  // RETURN
    }

    if (2 >= command.path().size()) {
        BALL_LOG_ERROR << "Invalid path provided in command.";
        return;  // RETURN
    }

    const bsl::string& path = command.path();
    if ('*' == path[path.length() - 1]) {
        // Wild card => entire storage

        d_dataFile.assign(path.substr(0, path.length() - 1));
        d_dataFile.append(mqbs::FileStoreProtocol::k_DATA_FILE_EXTENSION);
        d_journalFile.assign(path.substr(0, path.length() - 1));
        d_journalFile.append(
            mqbs::FileStoreProtocol::k_JOURNAL_FILE_EXTENSION);
        d_qlistFile.assign(path.substr(0, path.length() - 1));
        d_qlistFile.append(mqbs::FileStoreProtocol::k_QLIST_FILE_EXTENSION);

        // Data file
        if (!resetIterator(&d_dataFd, &d_dataFileIter, d_dataFile.c_str())) {
            d_dataFile.clear();
            return;  // RETURN
        }

        // Journal file
        if (!resetIterator(&d_journalFd,
                           &d_journalFileIter,
                           d_journalFile.c_str())) {
            d_dataFile.clear();
            d_dataFileIter.clear();
            mqbs::FileSystemUtil::close(&d_dataFd);
            return;  // RETURN
        }

        // Qlist file
        if (!resetIterator(&d_qlistFd,
                           &d_qlistFileIter,
                           d_qlistFile.c_str())) {
            d_dataFile.clear();
            d_journalFile.clear();
            d_dataFileIter.clear();
            mqbs::FileSystemUtil::close(&d_dataFd);
            d_journalFileIter.clear();
            mqbs::FileSystemUtil::close(&d_journalFd);
            return;  // RETURN
        }

        BALL_LOG_INFO << "Data file: [" << d_dataFile << "] Journal file: ["
                      << d_journalFile << "] Qlist file: [" << d_qlistFile
                      << "]";
    }
    else if (bmqu::StringUtil::endsWith(
                 path,
                 mqbs::FileStoreProtocol::k_DATA_FILE_EXTENSION)) {
        if (!resetIterator(&d_dataFd, &d_dataFileIter, path.c_str())) {
            return;  // RETURN
        }

        d_dataFile = path;
    }
    else if (bmqu::StringUtil::endsWith(
                 path,
                 mqbs::FileStoreProtocol::k_JOURNAL_FILE_EXTENSION)) {
        if (!resetIterator(&d_journalFd, &d_journalFileIter, path.c_str())) {
            return;  // RETURN
        }

        d_journalFile = path;
    }
    else if (bmqu::StringUtil::endsWith(
                 path,
                 mqbs::FileStoreProtocol::k_QLIST_FILE_EXTENSION)) {
        if (!resetIterator(&d_qlistFd, &d_qlistFileIter, path.c_str())) {
            return;  // RETURN
        }

        d_qlistFile = path;
    }
    else {
        BALL_LOG_ERROR << "Invalid file extension in specified path [" << path
                       << "]";
        return;  // RETURN
    }

    d_isOpen = true;
}

void StorageInspector::processCommand(
    BSLA_UNUSED const CloseStorageCommand& command)
{
    if (!d_isOpen) {
        BALL_LOG_ERROR << "No storage is open";
        return;  // RETURN
    }

    if (d_dataFd.isValid()) {
        d_dataFileIter.clear();
        mqbs::FileSystemUtil::close(&d_dataFd);
    }

    if (d_journalFd.isValid()) {
        d_journalFileIter.clear();
        mqbs::FileSystemUtil::close(&d_journalFd);
    }

    if (d_qlistFd.isValid()) {
        d_qlistFileIter.clear();
        mqbs::FileSystemUtil::close(&d_qlistFd);
        d_qlistFileRead = false;
        d_queues.clear();
    }

    d_isOpen = false;
}

void StorageInspector::processCommand(
    BSLA_UNUSED const MetadataCommand& command)
{
    bool x;

    // Print meta data of all open files

    if (d_dataFd.isValid()) {
        x = resetIterator(&d_dataFd, &d_dataFileIter, d_dataFile.c_str());
        BSLS_ASSERT_OPT(x);
        BALL_LOG_INFO << "Details of data file: \n"
                      << d_dataFd << " " << d_dataFileIter.header();
    }

    if (d_qlistFd.isValid()) {
        x = resetIterator(&d_qlistFd, &d_qlistFileIter, d_qlistFile.c_str());
        BSLS_ASSERT_OPT(x);
        BALL_LOG_INFO << "Details of qlist file: \n"
                      << d_qlistFd << " " << d_qlistFileIter.header();
    }

    if (d_journalFd.isValid()) {
        x = resetIterator(&d_journalFd,
                          &d_journalFileIter,
                          d_journalFile.c_str());
        BSLS_ASSERT_OPT(x);
        BALL_LOG_INFO_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM << "Details of journal file: \n";
            BALL_LOG_OUTPUT_STREAM << d_journalFd;
            BALL_LOG_OUTPUT_STREAM << "Journal File Header: \n";
            printJournalFileHeader<bmqu::AlignedPrinter>(
                BALL_LOG_OUTPUT_STREAM,
                d_journalFileIter.header(),
                d_journalFd);
            BALL_LOG_OUTPUT_STREAM << "\n";

            // Print journal-specific fields
            BALL_LOG_OUTPUT_STREAM << "Journal SyncPoint:\n";
            bsl::vector<bsl::string> fields;
            fields.push_back("Last Valid Record Offset");
            fields.push_back("Record Type");
            fields.push_back("Record Timestamp");
            fields.push_back("Record Epoch");
            fields.push_back("Last Valid SyncPoint Offset");
            fields.push_back("SyncPoint Timestamp");
            fields.push_back("SyncPoint Epoch");
            fields.push_back("SyncPoint SeqNum");
            fields.push_back("SyncPoint Primary NodeId");
            fields.push_back("SyncPoint Primary LeaseId");
            fields.push_back("SyncPoint DataFileOffset (DWORDS)");
            fields.push_back("SyncPoint QlistFileOffset (WORDS)");

            bmqu::AlignedPrinter printer(BALL_LOG_OUTPUT_STREAM, fields);
            bsls::Types::Uint64  lastRecPos =
                d_journalFileIter.lastRecordPosition();
            printer << lastRecPos;
            if (0 == lastRecPos) {
                // No valid record
                printer << "** NA **"
                        << "** NA **";
            }
            else {
                mqbs::OffsetPtr<const mqbs::RecordHeader> recHeader(
                    d_journalFd.block(),
                    lastRecPos);
                printer << recHeader->type();
                bdlt::Datetime      datetime;
                bsls::Types::Uint64 epochValue = recHeader->timestamp();
                int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime,
                                                             epochValue);
                if (0 != rc) {
                    printer << 0;
                }
                else {
                    printer << datetime;
                }
                printer << epochValue;
            }

            bsls::Types::Uint64 syncPointPos =
                d_journalFileIter.lastSyncPointPosition();

            printer << syncPointPos;
            if (0 == syncPointPos) {
                // No valid syncPoint
                printer << "** NA **"
                        << "** NA **"
                        << "** NA **"
                        << "** NA **"
                        << "** NA **"
                        << "** NA **";
            }
            else {
                const mqbs::JournalOpRecord& syncPt =
                    d_journalFileIter.lastSyncPoint();

                BSLS_ASSERT_OPT(mqbs::JournalOpType::e_SYNCPOINT ==
                                syncPt.type());

                bsls::Types::Uint64 epochValue = syncPt.header().timestamp();
                bdlt::Datetime      datetime;
                int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime,
                                                             epochValue);
                if (0 != rc) {
                    printer << 0;
                }
                else {
                    printer << datetime;
                }
                printer << epochValue;

                printer << syncPt.sequenceNum() << syncPt.primaryNodeId()
                        << syncPt.primaryLeaseId()
                        << syncPt.dataFileOffsetDwords()
                        << syncPt.qlistFileOffsetWords();
            }
        }
    }
}

void StorageInspector::processCommand(
    BSLA_UNUSED const ListQueuesCommand& command)
{
    if (!d_qlistFd.isValid()) {
        BALL_LOG_INFO << "Qlist file is not open.";
        return;  // RETURN
    }

    if (!d_qlistFileIter.isValid()) {
        // Should not happen
        BALL_LOG_INFO << "Qlist file iterator is invalid.";
        return;  // RETURN
    }

    bool x = resetIterator(&d_qlistFd, &d_qlistFileIter, d_qlistFile.c_str());
    BSLS_ASSERT_OPT(x);

    readQueuesIfNeeded();

    // Qlist file has been read
    BALL_LOG_INFO << "Total number of queue instances: " << d_queues.size();

    QueuesMap::const_iterator cit  = d_queues.begin();
    unsigned int              qnum = 0;
    for (; cit != d_queues.end(); ++cit) {
        ++qnum;
        BALL_LOG_INFO_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM << "Queue #" << qnum << "\n";
            const QueueRecord& qr = cit->second;

            bsl::vector<bsl::string> fields;
            fields.push_back("Queue URI");
            fields.push_back("QueueKey");
            fields.push_back("Number of AppIds");

            bmqu::AlignedPrinter printer(BALL_LOG_OUTPUT_STREAM, fields);
            printer << cit->first << qr.d_queueKey << qr.d_appIds.size();

            // 'printer' not to be used beyond this point

            const bsl::vector<AppIdRecord>& appRecs = qr.d_appIds;
            for (unsigned int i = 0; i < appRecs.size(); ++i) {
                const AppIdRecord& ar = appRecs[i];
                BALL_LOG_OUTPUT_STREAM << "        AppId #" << i + 1 << "\n";
                bsl::vector<bsl::string> f;
                f.push_back("AppId");
                f.push_back("AppKey");

                const int            indent = 8;
                bmqu::AlignedPrinter p(BALL_LOG_OUTPUT_STREAM, f, indent);
                p << ar.d_appId << ar.d_appKey;
            }

            BALL_LOG_OUTPUT_STREAM << "\n";
        }
    }
}

void StorageInspector::processCommand(const DumpQueueCommand& command)
{
    // Validate command parameters ...
    if (command.uri().empty()) {
        BALL_LOG_ERROR << "'uri' is mandatory";
        return;  // RETURN
    }

    // Specifying queue key is optional, but if specified, its length must be
    // as expected.

    if (!command.key().empty() &&
        command.key().length() < mqbu::StorageKey::e_KEY_LENGTH_HEX) {
        BALL_LOG_ERROR << "'key' length must be "
                       << mqbu::StorageKey::e_KEY_LENGTH_HEX << " characters.";
        return;  // RETURN
    }

    if (!d_dataFd.isValid() || !d_qlistFd.isValid() ||
        !d_journalFd.isValid()) {
        BALL_LOG_ERROR << "All three files need to be opened.";
        return;  // RETURN
    }

    bool x = resetIterator(&d_qlistFd, &d_qlistFileIter, d_qlistFile.c_str());
    BSLS_ASSERT_OPT(x);

    x = resetIterator(&d_journalFd, &d_journalFileIter, d_journalFile.c_str());
    BSLS_ASSERT_OPT(x);

    x = resetIterator(&d_dataFd, &d_dataFileIter, d_dataFile.c_str());
    BSLS_ASSERT_OPT(x);

    readQueuesIfNeeded();

    mqbu::StorageKey queueKey;
    if (!command.key().empty()) {
        queueKey.fromHex(command.key().c_str());
    }

    // Find the specified uri/key match.

    IterPair itp = d_queues.equal_range(command.uri());
    if (itp.first == itp.second) {
        BALL_LOG_ERROR << "Queue URI not found";
        return;  // RETURN
    }

    bool foundQueueKey = false;
    if (!queueKey.isNull()) {
        for (QueuesMapIter iter = itp.first; iter != itp.second; ++iter) {
            if (iter->second.d_queueKey == queueKey) {
                foundQueueKey = true;
                break;
            }
        }
    }
    else {
        // No queue key has been specified, just pick up the first entry.

        foundQueueKey = true;
        queueKey      = itp.first->second.d_queueKey;

        QueuesMapIter temp(itp.first);
        ++temp;
        if (temp != itp.second) {
            BALL_LOG_WARN << "Several records with different queue keys exist "
                          << "for the specified queue uri. Choosing record "
                          << "with queueKey [" << queueKey << "].\n";
        }
    }

    if (!foundQueueKey) {
        BALL_LOG_ERROR << "Queue URI/Key combo not found";
        return;  // RETURN
    }

    BALL_LOG_INFO << "Printing all records for queue [" << command.uri()
                  << "] and queueKey [" << queueKey << "].\n";

    // TODO: Print all the registered groupIds, as well as currently associated
    // handle and 'creationTimesamp'.  At (storage level) all we'll have to do
    // is print the optional groupId (or more generically, all options from the
    // message)

    // Then iterate over journal file (in sync with data file).

    while (1 == d_journalFileIter.nextRecord()) {
        mqbs::RecordType::Enum recordType = d_journalFileIter.recordType();
        switch (recordType) {
        case mqbs::RecordType::e_MESSAGE: {
            const mqbs::MessageRecord& r = d_journalFileIter.asMessageRecord();
            if (r.queueKey() == queueKey) {
                bool failure = false;
                BALL_LOG_INFO_BLOCK
                {
                    BALL_LOG_OUTPUT_STREAM << "MessageRecord: \n";
                    printRecord(BALL_LOG_OUTPUT_STREAM, r);
                }
                while (
                    d_dataFileIter.recordOffset() !=
                    (r.messageOffsetDwords() * bmqp::Protocol::k_DWORD_SIZE)) {
                    int rc = d_dataFileIter.nextRecord();
                    if (rc != 1) {
                        BALL_LOG_ERROR
                            << "Failed to retrieve message from DATA "
                            << "file rc: " << rc;
                        failure = true;
                        break;  // BREAK
                    }
                }

                if (!failure) {
                    printIterator(d_dataFileIter);
                }
            }
        } break;
        case mqbs::RecordType::e_CONFIRM: {
            const mqbs::ConfirmRecord& r = d_journalFileIter.asConfirmRecord();
            if (r.queueKey() == queueKey) {
                BALL_LOG_INFO_BLOCK
                {
                    BALL_LOG_OUTPUT_STREAM << "ConfirmRecord: \n";
                    printRecord(BALL_LOG_OUTPUT_STREAM, r);
                }
            }
        } break;
        case mqbs::RecordType::e_DELETION: {
            const mqbs::DeletionRecord& r =
                d_journalFileIter.asDeletionRecord();
            if (r.queueKey() == queueKey) {
                BALL_LOG_INFO_BLOCK
                {
                    BALL_LOG_OUTPUT_STREAM << "DeletionRecord: \n";
                    printRecord(BALL_LOG_OUTPUT_STREAM, r);
                }
            }
        } break;
        case mqbs::RecordType::e_QUEUE_OP: {
            const mqbs::QueueOpRecord& r = d_journalFileIter.asQueueOpRecord();
            if (r.queueKey() == queueKey) {
                BALL_LOG_INFO_BLOCK
                {
                    BALL_LOG_INFO << "QueueOpRecord: \n";
                    printRecord(BALL_LOG_OUTPUT_STREAM, r);
                }
            }
        } break;
        case mqbs::RecordType::e_JOURNAL_OP: break;
        case mqbs::RecordType::e_UNDEFINED:
        default: BALL_LOG_ERROR << "Unexpected record type: " << recordType;
        }
    }
}

void StorageInspector::processCommand(const DataCommand& command)
{
    if (!d_dataFd.isValid()) {
        BALL_LOG_ERROR << "You must open a data file to use that command.";
        return;  // RETURN
    }

    iterateNextPosition(command.choice(),
                        &d_dataFd,
                        &d_dataFileIter,
                        d_dataFile.c_str());
}

void StorageInspector::processCommand(const QlistCommand& command)
{
    if (!d_qlistFd.isValid()) {
        BALL_LOG_ERROR << "You must open a qlist file to use that command.";
        return;  // RETURN
    }

    iterateNextPosition(command.choice(),
                        &d_qlistFd,
                        &d_qlistFileIter,
                        d_qlistFile.c_str());
}

void StorageInspector::processCommand(JournalCommand& command)
{
    if (!d_journalFd.isValid()) {
        BALL_LOG_ERROR << "You must open a journal file to use that command.";
        return;  // RETURN
    }

    mqbs::JournalFileIterator* iter   = &d_journalFileIter;
    JournalCommandChoice&      choice = command.choice();
    if (choice.selectionId() == JournalCommandChoice::SELECTION_ID_DUMP) {
        if (choice.dump() != "payload") {
            BALL_LOG_ERROR << "Unknown dump option: " << choice.dump();
            return;  // RETURN
        }
        else if (iter->recordType() != mqbs::RecordType::e_MESSAGE) {
            BALL_LOG_ERROR << "Only printing message is supported (received "
                           << iter->recordType() << ")";
            return;  // RETURN
        }
        else if (!d_dataFd.isValid()) {
            BALL_LOG_ERROR << "You must open a data file to use that command.";
            return;  // RETURN
        }

        const mqbs::FileHeader& fh = mqbs::FileStoreProtocolUtil::bmqHeader(
            d_dataFd);
        int rc = d_dataFileIter.reset(&d_dataFd, fh);
        if (rc < 0) {
            BALL_LOG_ERROR << "Failed to reset data file iterator (exit rc = "
                           << rc << ")";
            return;  // RETURN
        }

        const mqbs::MessageRecord& record = iter->asMessageRecord();
        const unsigned int messageOffset  = record.messageOffsetDwords() *
                                           bmqp::Protocol::k_DWORD_SIZE;

        while (d_dataFileIter.recordOffset() != messageOffset) {
            rc = d_dataFileIter.nextRecord();
            if (rc != 1) {
                BALL_LOG_ERROR
                    << "Failed to iterate data file (exit status=" << rc
                    << ")";
                return;  // RETURN
            }
        }

        printIterator(d_dataFileIter);
        return;  // RETURN
    }
    else if (choice.selectionId() == JournalCommandChoice::SELECTION_ID_TYPE) {
        mqbs::RecordType::Enum recordType;
        switch (choice.type()) {
        case JournalCommandChoiceType::CONFIRM: {
            recordType = mqbs::RecordType::e_CONFIRM;
        } break;
        case JournalCommandChoiceType::DELETE: {
            recordType = mqbs::RecordType::e_DELETION;
        } break;
        case JournalCommandChoiceType::JOP: {
            recordType = mqbs::RecordType::e_JOURNAL_OP;
        } break;
        case JournalCommandChoiceType::MESSAGE: {
            recordType = mqbs::RecordType::e_MESSAGE;
        } break;
        case JournalCommandChoiceType::QOP: {
            recordType = mqbs::RecordType::e_QUEUE_OP;
        } break;
        default:
            BALL_LOG_ERROR << "Invalid record type: " << choice.type();
            return;  // RETURN
        }

        if (iter->isReverseMode()) {
            iter->flipDirection();
        }

        while (true) {
            if (!iter->hasRecordSizeRemaining()) {
                BALL_LOG_ERROR << "Ran out of records while iterating.";
                return;  // RETURN
            }
            int rc = iter->nextRecord();
            if (rc <= 0) {
                BALL_LOG_ERROR << "Iteration aborted (exit status " << rc
                               << ").";
                return;  // RETURN
            }
            else if (iter->recordType() == recordType) {
                // We let the fall through to 'iterateNextPosition' print out
                // the record at this index for us by converting the choice to
                // a next choice with a skip of zero.
                choice.makeNext(0);
                break;  // BREAK
            }
        }
        // Fall through
    }

    iterateNextPosition(choice, &d_journalFd, iter, d_journalFile.c_str());
}

void StorageInspector::readQueuesIfNeeded()
{
    // Shorter ref for convenience
    mqbs::QlistFileIterator& it = d_qlistFileIter;

    if (!d_qlistFileRead) {
        // Read qlist file
        BALL_LOG_INFO << "Reading qlist file.";
        int rc;
        while (1 == (rc = it.nextRecord())) {
            BSLS_ASSERT_OPT(it.isValid());

            const char*  uri       = 0;
            const char*  qkey      = 0;
            unsigned int uriLen    = 0;
            unsigned int numAppIds = it.queueRecordHeader()->numAppIds();

            it.loadQueueUri(&uri, &uriLen);
            it.loadQueueUriHash(&qkey);

            bsl::vector<bsl::pair<bsl::string, unsigned int> > appIdLenPairs;
            bsl::vector<bsl::string>                           appIdHashes;

            if (0 != numAppIds) {
                it.loadAppIds(&appIdLenPairs);
                it.loadAppIdHashes(&appIdHashes);
            }

            bsl::string      queueUri(uri, uriLen);
            mqbu::StorageKey queueKey(mqbu::StorageKey::BinaryRepresentation(),
                                      qkey);
            IterPair         itp = d_queues.equal_range(queueUri);
            if (itp.first == itp.second) {
                // Queue URI seen for the 1st time.

                QueueRecord qr;
                qr.d_queueKey = queueKey;

                for (size_t n = 0; n < numAppIds; ++n) {
                    AppIdRecord ar;
                    ar.d_appId.assign(appIdLenPairs[n].first,
                                      appIdLenPairs[n].second);
                    ar.d_appKey.fromBinary(appIdHashes[n].data());
                    qr.d_appIds.push_back(ar);
                }

                d_queues.insert(bsl::make_pair(queueUri, qr));
            }
            else {
                // At least one record with same queue uri seen before.  Use
                // 'queueKey' to check if we need to create a new entry for
                // this uri, or update an existing one.

                bool makeNewEntry = true;
                for (QueuesMapIter iter = itp.first; iter != itp.second;
                     ++iter) {
                    if (iter->second.d_queueKey != queueKey) {
                        continue;  // CONTINUE
                    }

                    // Got it.  Since we are encountering same uri with same
                    // queue key again, this must be an ADDITION record,
                    // 'numAppIds' must be non-zero, and the appId/appKey pairs
                    // listed in this record must be unique to the ones already
                    // present.  If there is a clash, raise a warning and move
                    // on.

                    makeNewEntry = false;

                    if (0 == numAppIds) {
                        BALL_LOG_ERROR
                            << "For queue [" << iter->first << "], queueKey ["
                            << queueKey << "], encountered "
                            << "another record (presumably QueueOp.ADDITION) "
                            << "but with no appId/appKey pairs, which is an "
                            << "error.";
                        break;  // BREAK
                    }

                    QueueRecord&                  qr        = iter->second;
                    bsl::vector<AppIdRecord>&     appIdsRec = qr.d_appIds;
                    bsl::vector<bsl::string>      newAppIds;
                    bsl::vector<mqbu::StorageKey> newAppKeys;

                    for (size_t n = 0; n < appIdsRec.size(); ++n) {
                        const AppIdRecord& ar = appIdsRec[n];

                        for (size_t nn = 0; nn < appIdLenPairs.size(); ++nn) {
                            const bsl::string newAppId(
                                appIdLenPairs[nn].first,
                                appIdLenPairs[nn].second);
                            const mqbu::StorageKey newAppKey(
                                mqbu::StorageKey::BinaryRepresentation(),
                                appIdHashes[nn].data());

                            if (ar.d_appId == newAppId) {
                                BALL_LOG_ERROR
                                    << "For queue [" << iter->first
                                    << "], queueKey [" << queueKey
                                    << "], same AppId [" << ar.d_appId
                                    << "] encountered again. Exisitng appKey ["
                                    << ar.d_appKey << "], new appKey ["
                                    << newAppKey << "]. Ignoring this appId/"
                                    << "appKey pair.";
                                continue;  // CONTINUE
                            }

                            if (ar.d_appKey == newAppKey) {
                                BALL_LOG_ERROR
                                    << "For queue [" << iter->first
                                    << "], queueKey [" << queueKey
                                    << "], same AppKey [" << ar.d_appKey
                                    << "] encountered again. Existing appId ["
                                    << ar.d_appId << "], new appId ["
                                    << newAppId << "]. Ignoring this appId/"
                                    << "appKey pair.";
                                continue;  // CONTINUE
                            }
                            newAppIds.push_back(newAppId);
                            newAppKeys.push_back(newAppKey);
                        }
                    }

                    // AppId/AppKey dedup check complete.  Add newly retrieved
                    // AppId/AppKey pairs to this queue's record.

                    for (size_t n = 0; n < newAppIds.size(); ++n) {
                        appIdsRec.push_back(
                            AppIdRecord(newAppIds[n], newAppKeys[n]));
                    }

                    break;  // BREAK
                }

                if (!makeNewEntry) {
                    // An entry with same uri and queue key was encountered and
                    // updated accordingly with new appId/appKey pairs.

                    continue;  // CONTINUE
                }

                // An entry with same uri but different queue key was
                // encountered.  Need to insert a new entry in 'd_queues'.

                QueueRecord qr;
                qr.d_queueKey = queueKey;

                for (size_t n = 0; n < numAppIds; ++n) {
                    AppIdRecord ar;
                    ar.d_appId.assign(appIdLenPairs[n].first,
                                      appIdLenPairs[n].second);
                    ar.d_appKey.fromBinary(appIdHashes[n].data());
                    qr.d_appIds.push_back(ar);
                }

                d_queues.insert(bsl::make_pair(queueUri, qr));
            }
        }  // while
        BALL_LOG_INFO << "Finished reading qlist file with rc: " << rc;
        d_qlistFileRead = true;
    }
}

// CREATORS
StorageInspector::StorageInspector(bslma::Allocator* allocator)
: d_isOpen(false)
, d_queues(allocator)
, d_qlistFileRead(false)
, d_dataFile(allocator)
, d_qlistFile(allocator)
, d_journalFile(allocator)
{
    // NOTHING
}

StorageInspector::~StorageInspector()
{
    d_queues.clear();
    d_dataFileIter.clear();
    d_journalFileIter.clear();
    d_qlistFileIter.clear();

    if (d_dataFd.isValid()) {
        mqbs::FileSystemUtil::close(&d_dataFd);
    }

    if (d_journalFd.isValid()) {
        mqbs::FileSystemUtil::close(&d_journalFd);
    }

    if (d_qlistFd.isValid()) {
        mqbs::FileSystemUtil::close(&d_qlistFd);
    }
}

// MANIPULATORS
int StorageInspector::initialize()
{
    // Print the welcome banner
    BALL_LOG_INFO << "Welcome to the BlazingMQ tool storage inspector.\n"
                  << "Type 'help' to see the list of commands supported by "
                  << "storage inspector. Crl-D to quit.";
    return 0;
}

int StorageInspector::mainLoop()
{
    while (true) {
        bsl::string input;

        if (!InputUtil::getLine(&input)) {
            break;  // BREAK
        }

        if (input.empty()) {
            continue;  // CONTINUE
        }
        else {
            bsl::string verb, jsonInput;
            InputUtil::preprocessInput(&verb, &jsonInput, input);
            if ((verb == "bye") || (verb == "quit") || (verb == "help")) {
                if (jsonInput.length() > 2) {
                    BALL_LOG_ERROR << "'" << verb << "' takes no arguments.";
                }
                else if (verb == "help") {
                    printHelp();
                }
                else {
                    break;  // BREAK
                }
            }
            else if (verb == "open") {
                OpenStorageCommand command;
                if (parseCommand(&command, jsonInput)) {
                    processCommand(command);
                }
            }
            else if (verb == "close") {
                CloseStorageCommand command;
                if (parseCommand(&command, jsonInput)) {
                    processCommand(command);
                }
            }
            else if (verb == "queues") {
                ListQueuesCommand command;
                if (parseCommand(&command, jsonInput)) {
                    processCommand(command);
                }
            }
            else if (verb == "metadata") {
                MetadataCommand command;
                if (parseCommand(&command, jsonInput)) {
                    processCommand(command);
                }
            }
            else if (verb == "dump") {
                DumpQueueCommand command;
                if (parseCommand(&command, jsonInput)) {
                    processCommand(command);
                }
            }
            else if (verb == "j") {
                JournalCommand command;
                if (parseCommand(&command, jsonInput)) {
                    processCommand(command);
                }
            }
            else if (verb == "q") {
                QlistCommand command;
                if (parseCommand(&command, jsonInput)) {
                    processCommand(command);
                }
            }
            else if (verb == "d") {
                DataCommand command;
                if (parseCommand(&command, jsonInput)) {
                    processCommand(command);
                }
            }
            else {
                BALL_LOG_ERROR << "Unknown command: '" << verb
                               << "'.  Try 'help'.";
            }
        }
    }

    return 0;
}

}  // close package namespace
}  // close enterprise namespace
