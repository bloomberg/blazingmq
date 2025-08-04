// Copyright 2025 Bloomberg Finance L.P.
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
#include <m_bmqstoragetool_printer.h>

// bmqstoragetool
#include <m_bmqstoragetool_messagedetails.h>
#include <m_bmqstoragetool_recordprinter.h>

// BMQ
#include <bmqu_alignedprinter.h>
#include <bmqu_jsonprinter.h>

// MQB
#include <mqbs_filestoreprotocol.h>
#include <mqbs_filestoreprotocolprinter.h>

// BDE
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bsl_iomanip.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_vector.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// CONVENIENCE
using namespace RecordPrinter;

// =============
// class Printer
// =============

Printer::~Printer()
{
    // NOTHING
}

namespace {

// FREE FUNCTIONS

template <typename PRINTER_TYPE>
void printMessageDetails(bsl::ostream&         os,
                         const MessageDetails& details,
                         bslma::Allocator*     allocator)
{
    RecordDetailsPrinter<PRINTER_TYPE> printer(os, allocator);

    // Print message record
    printer.printRecordDetails(details.messageRecord());

    // Print confirmation records
    const bsl::vector<RecordDetails<mqbs::ConfirmRecord> >& confirmRecords =
        details.confirmRecords();
    if (!confirmRecords.empty()) {
        typename bsl::vector<
            RecordDetails<mqbs::ConfirmRecord> >::const_iterator it =
            confirmRecords.begin();
        for (; it != confirmRecords.end(); ++it) {
            RecordPrinter::printDelimeter<PRINTER_TYPE>(os);
            printer.printRecordDetails(*it);
        }
    }

    // Print deletion record
    if (details.deleteRecord().has_value()) {
        RecordPrinter::printDelimeter<PRINTER_TYPE>(os);
        printer.printRecordDetails(details.deleteRecord().value());
    }
}

/// Helper to print data file meta data
template <typename PRINTER_TYPE1, typename PRINTER_TYPE2>
void printDataFileMeta(bsl::ostream&                 ostream,
                       const mqbs::DataFileIterator* dataFile_p,
                       bslma::Allocator*             allocator)
{
    BSLS_ASSERT_SAFE(dataFile_p && dataFile_p->isValid());

    const bsl::vector<const char*> fields = {"BlazingMQ File Header",
                                             "Data File Header"};

    PRINTER_TYPE1 printer(ostream, &fields);
    {
        bmqu::MemOutStream s(allocator);
        s << '\n';
        mqbs::FileStoreProtocolPrinter::printFileHeader<PRINTER_TYPE2>(
            s,
            *dataFile_p->mappedFileDescriptor());
        printer << s.str();
    }
    {
        bmqu::MemOutStream s(allocator);
        s << '\n';
        mqbs::FileStoreProtocolPrinter::printDataFileHeader<PRINTER_TYPE2>(
            s,
            dataFile_p->header());
        printer << s.str();
    }
}

/// Helper to print journal file meta data
template <typename PRINTER_TYPE1, typename PRINTER_TYPE2>
void printJournalFileMeta(bsl::ostream&                    ostream,
                          const mqbs::JournalFileIterator* journalFile_p,
                          bslma::Allocator*                allocator)
{
    BSLS_ASSERT_SAFE(journalFile_p && journalFile_p->isValid());

    const bsl::vector<const char*> fields = {"BlazingMQ File Header",
                                             "Journal File Header",
                                             "Journal SyncPoint"};

    PRINTER_TYPE1 printer(ostream, &fields);
    {
        bmqu::MemOutStream s(allocator);
        s << '\n';
        mqbs::FileStoreProtocolPrinter::printFileHeader<PRINTER_TYPE2>(
            s,
            *journalFile_p->mappedFileDescriptor());
        printer << s.str();
    }
    {
        bmqu::MemOutStream s(allocator);
        s << '\n';
        mqbs::FileStoreProtocolPrinter::printJournalFileHeader<PRINTER_TYPE2>(
            s,
            journalFile_p->header(),
            *journalFile_p->mappedFileDescriptor(),
            allocator);
        printer << s.str();
    }

    {
        bmqu::MemOutStream s(allocator);
        s << '\n';
        {
            // Print journal-specific fields
            bsl::vector<const char*> fieldsSyncPoint(allocator);
            fieldsSyncPoint.reserve(12);
            fieldsSyncPoint.push_back("Last Valid Record Offset");
            fieldsSyncPoint.push_back("Record Type");
            fieldsSyncPoint.push_back("Record Timestamp");
            fieldsSyncPoint.push_back("Record Epoch");
            fieldsSyncPoint.push_back("Last Valid SyncPoint Offset");
            fieldsSyncPoint.push_back("SyncPoint Timestamp");
            fieldsSyncPoint.push_back("SyncPoint Epoch");
            fieldsSyncPoint.push_back("SyncPoint SeqNum");
            fieldsSyncPoint.push_back("SyncPoint Primary NodeId");
            fieldsSyncPoint.push_back("SyncPoint Primary LeaseId");
            fieldsSyncPoint.push_back("SyncPoint DataFileOffset (DWORDS)");
            fieldsSyncPoint.push_back("SyncPoint QlistFileOffset (WORDS)");

            PRINTER_TYPE2       p(s, &fieldsSyncPoint);
            bsls::Types::Uint64 lastRecPos =
                journalFile_p->lastRecordPosition();
            p << lastRecPos;
            if (0 == lastRecPos) {
                // No valid record
                p << "** NA **";
                p << "** NA **";
            }
            else {
                mqbs::OffsetPtr<const mqbs::RecordHeader> recHeader(
                    journalFile_p->mappedFileDescriptor()->block(),
                    lastRecPos);
                p << recHeader->type();
                bdlt::Datetime      datetime;
                bsls::Types::Uint64 epochValue = recHeader->timestamp();
                const int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime,
                                                                   epochValue);
                if (0 != rc) {
                    p << 0;
                }
                else {
                    p << datetime;
                }
                p << epochValue;
            }

            const bsls::Types::Uint64 syncPointPos =
                journalFile_p->lastSyncPointPosition();

            p << syncPointPos;
            if (0 == syncPointPos) {
                // No valid syncPoint
                p << "** NA **";
                p << "** NA **";
                p << "** NA **";
                p << "** NA **";
                p << "** NA **";
                p << "** NA **";
            }
            else {
                const mqbs::JournalOpRecord& syncPt =
                    journalFile_p->lastSyncPoint();

                BSLS_ASSERT_OPT(mqbs::JournalOpType::e_SYNCPOINT ==
                                syncPt.type());

                bsls::Types::Uint64 epochValue = syncPt.header().timestamp();
                bdlt::Datetime      datetime;
                const int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime,
                                                                   epochValue);
                if (0 != rc) {
                    p << 0;
                }
                else {
                    p << datetime;
                }
                p << epochValue;

                p << syncPt.sequenceNum() << syncPt.primaryNodeId()
                  << syncPt.primaryLeaseId() << syncPt.dataFileOffsetDwords()
                  << syncPt.qlistFileOffsetWords();
            }
        }
        printer << s.str();
    }
}

/// Helper to print journal file meta data
template <typename PRINTER_TYPE>
void printQueueDetails(bsl::ostream&          ostream,
                       const QueueDetailsMap& queueDetailsMap,
                       bslma::Allocator*      allocator)
{
    for (QueueDetailsMap::const_iterator it = queueDetailsMap.cbegin();
         it != queueDetailsMap.cend();
         ++it) {
        if (it != queueDetailsMap.cbegin()) {
            printDelimeter<PRINTER_TYPE>(ostream);
        }

        const mqbu::StorageKey& queueKey     = it->first;
        const QueueDetails&     details      = it->second;
        const bsl::size_t       appKeysCount = details.d_appDetailsMap.size();

        // Setup fields to be displayed
        bsl::vector<const char*> fields(allocator);
        fields.reserve(8);
        fields.push_back("Queue Key");
        if (!details.d_queueUri.empty()) {
            fields.push_back("Queue URI");
        }
        fields.push_back("Total Records");
        fields.push_back("Num Queue Op Records");
        fields.push_back("Num Message Records");
        fields.push_back("Num Confirm Records");
        if (appKeysCount > 1U) {
            fields.push_back("Num Records Per App");
        }
        fields.push_back("Num Delete Records");

        {
            PRINTER_TYPE printer(ostream, &fields);

            // Print Queue Key id: either Key or URI
            printer << queueKey;

            // Print Queue URI if it's available in CSL file
            if (!details.d_queueUri.empty()) {
                printer << details.d_queueUri;
            }

            // Print number of records of all types related to the queue
            printer << details.d_recordsNumber;
            printer << details.d_queueOpRecordsNumber;
            printer << details.d_messageRecordsNumber;
            printer << details.d_confirmRecordsNumber;

            // Print number of records per App Key/Id
            if (appKeysCount > 1U) {
                bmqu::MemOutStream ss(allocator);

                // Sort Apps by number of records ascending
                AppsData appsData(allocator);
                appsData.reserve(appKeysCount);
                for (QueueDetails::AppDetailsMap::const_iterator appIt =
                         details.d_appDetailsMap.cbegin();
                     appIt != details.d_appDetailsMap.cend();
                     ++appIt) {
                    appsData.emplace_back(appIt->second.d_recordsNumber,
                                          appIt->first);
                }
                bsl::sort(appsData.begin(), appsData.end());

                // Print number of records per App
                for (AppsData::const_reverse_iterator appIt =
                         appsData.crbegin();
                     appIt != appsData.crend();
                     ++appIt) {
                    const mqbu::StorageKey&         appKey = appIt->second;
                    const QueueDetails::AppDetails& appDetails =
                        details.d_appDetailsMap.at(appIt->second);

                    if (!appDetails.d_appId.empty()) {
                        ss << appDetails.d_appId;
                    }
                    else {
                        ss << appKey;
                    }

                    ss << "=" << appIt->first << " ";
                }
                printer << ss.str();
            }

            printer << details.d_deleteRecordsNumber;
        }
    }
}

}  // close anonymous namespace

class HumanReadablePrinter : public Printer {
  private:
    // DATA
    bsl::ostream&     d_ostream;
    bslma::Allocator* d_allocator_p;

    // PRIVATE ACCESSORS
    template <typename RECORD_TYPE>
    void
    printRecordDetails(const RecordDetails<RECORD_TYPE>& recordDetails) const;

  public:
    // CREATORS
    HumanReadablePrinter(bsl::ostream& os, bslma::Allocator* allocator);

    ~HumanReadablePrinter() BSLS_KEYWORD_OVERRIDE;

    // PUBLIC METHODS

    void
    printMessage(const MessageDetails& details) const BSLS_KEYWORD_OVERRIDE;

    void printConfirmRecord(const RecordDetails<mqbs::ConfirmRecord>& rec)
        const BSLS_KEYWORD_OVERRIDE;

    void printDeletionRecord(const RecordDetails<mqbs::DeletionRecord>& rec)
        const BSLS_KEYWORD_OVERRIDE;

    void printQueueOpRecord(const RecordDetails<mqbs::QueueOpRecord>& rec)
        const BSLS_KEYWORD_OVERRIDE;

    void printJournalOpRecord(const RecordDetails<mqbs::JournalOpRecord>& rec)
        const BSLS_KEYWORD_OVERRIDE;

    void printGuidNotFound(const bmqt::MessageGUID& guid) const
        BSLS_KEYWORD_OVERRIDE;

    void printGuid(const bmqt::MessageGUID& guid) const BSLS_KEYWORD_OVERRIDE;

    void printFooter(bsls::Types::Uint64                   foundMessagesCount,
                     bsls::Types::Uint64                   foundQueueOpCount,
                     bsls::Types::Uint64                   foundJournalOpCount,
                     const Parameters::ProcessRecordTypes& processRecordTypes)
        const BSLS_KEYWORD_OVERRIDE;

    void printExactMatchFooter(
        bsls::Types::Uint64                   foundMessagesCount,
        bsls::Types::Uint64                   foundConfirmCount,
        bsls::Types::Uint64                   foundDeletionCount,
        bsls::Types::Uint64                   foundQueueOpCount,
        bsls::Types::Uint64                   foundJournalOpCount,
        const Parameters::ProcessRecordTypes& processRecordTypes) const
        BSLS_KEYWORD_OVERRIDE;

    void printOutstandingRatio(int         ratio,
                               bsl::size_t outstandingMessagesCount,
                               bsl::size_t totalMessagesCount) const
        BSLS_KEYWORD_OVERRIDE;

    void printMessageSummary(bsl::size_t totalMessagesCount,
                             bsl::size_t partiallyConfirmedCount,
                             bsl::size_t confirmedCount,
                             bsl::size_t outstandingCount) const
        BSLS_KEYWORD_OVERRIDE;

    void printQueueOpSummary(bsls::Types::Uint64     queueOpRecordsCount,
                             const QueueOpCountsVec& queueOpCountsVec) const
        BSLS_KEYWORD_OVERRIDE;

    void printJournalOpSummary(bsls::Types::Uint64 journalOpRecordsCount) const
        BSLS_KEYWORD_OVERRIDE;

    void printRecordSummary(bsls::Types::Uint64    totalRecordsCount,
                            const QueueDetailsMap& queueDetailsMap) const
        BSLS_KEYWORD_OVERRIDE;

    void printJournalFileMeta(const mqbs::JournalFileIterator* journalFile_p)
        const BSLS_KEYWORD_OVERRIDE;

    void printDataFileMeta(const mqbs::DataFileIterator* dataFile_p) const
        BSLS_KEYWORD_OVERRIDE;

    void
    printGuidsNotFound(const GuidsList& guids) const BSLS_KEYWORD_OVERRIDE;

    void printOffsetsNotFound(const OffsetsVec& offsets) const
        BSLS_KEYWORD_OVERRIDE;

    void printCompositesNotFound(const CompositesVec& seqNums) const
        BSLS_KEYWORD_OVERRIDE;
};

// CREATORS
HumanReadablePrinter::HumanReadablePrinter(bsl::ostream&     os,
                                           bslma::Allocator* allocator)
: d_ostream(os)
, d_allocator_p(allocator)
{
}

HumanReadablePrinter::~HumanReadablePrinter()
{
    d_ostream << "\n";
}

// PUBLIC METHODS

void HumanReadablePrinter::printMessage(const MessageDetails& details) const
{
    d_ostream << "==============================\n\n";
    printMessageDetails<bmqu::AlignedPrinter>(d_ostream,
                                              details,
                                              d_allocator_p);
    d_ostream << "\n";
}

void HumanReadablePrinter::printConfirmRecord(
    const RecordDetails<mqbs::ConfirmRecord>& rec) const
{
    printRecordDetails(rec);
}

void HumanReadablePrinter::printDeletionRecord(
    const RecordDetails<mqbs::DeletionRecord>& rec) const
{
    printRecordDetails(rec);
}

void HumanReadablePrinter::printQueueOpRecord(
    const RecordDetails<mqbs::QueueOpRecord>& rec) const
{
    printRecordDetails(rec);
}

void HumanReadablePrinter::printJournalOpRecord(
    const RecordDetails<mqbs::JournalOpRecord>& rec) const
{
    printRecordDetails(rec);
}

void HumanReadablePrinter::printGuidNotFound(
    const bmqt::MessageGUID& guid) const
{
    d_ostream << "Logic error : guid " << guid << " not found\n";
}

void HumanReadablePrinter::printGuid(const bmqt::MessageGUID& guid) const
{
    d_ostream << guid << "\n\n";
}

void HumanReadablePrinter::printFooter(
    bsls::Types::Uint64                   foundMessagesCount,
    bsls::Types::Uint64                   foundQueueOpCount,
    bsls::Types::Uint64                   foundJournalOpCount,
    const Parameters::ProcessRecordTypes& processRecordTypes) const
{
    if (processRecordTypes.d_message) {
        foundMessagesCount > 0
            ? (d_ostream << foundMessagesCount << " message record(s)")
            : d_ostream << "No message record";
        d_ostream << " found.\n";
    }
    if (processRecordTypes.d_queueOp) {
        foundQueueOpCount > 0
            ? (d_ostream << foundQueueOpCount << " queueOp record(s)")
            : d_ostream << "No queueOp record";
        d_ostream << " found.\n";
    }
    if (processRecordTypes.d_journalOp) {
        foundJournalOpCount > 0
            ? (d_ostream << foundJournalOpCount << " journalOp record(s)")
            : d_ostream << "No journalOp record";
        d_ostream << " found.\n";
    }
}

void HumanReadablePrinter::printExactMatchFooter(
    bsls::Types::Uint64                   foundMessagesCount,
    bsls::Types::Uint64                   foundConfirmCount,
    bsls::Types::Uint64                   foundDeletionCount,
    bsls::Types::Uint64                   foundQueueOpCount,
    bsls::Types::Uint64                   foundJournalOpCount,
    const Parameters::ProcessRecordTypes& processRecordTypes) const
{
    if (processRecordTypes.d_message) {
        foundConfirmCount > 0
            ? (d_ostream << foundConfirmCount << " confirm record(s)")
            : d_ostream << "No confirm record";
        d_ostream << " found.\n";
        foundDeletionCount > 0
            ? (d_ostream << foundDeletionCount << " deletion record(s)")
            : d_ostream << "No deletion record";
        d_ostream << " found.\n";
    }

    printFooter(foundMessagesCount,
                foundQueueOpCount,
                foundJournalOpCount,
                processRecordTypes);
}

void HumanReadablePrinter::printOutstandingRatio(
    int         ratio,
    bsl::size_t outstandingMessagesCount,
    bsl::size_t totalMessagesCount) const
{
    d_ostream << "Outstanding ratio: " << ratio << "% ("
              << outstandingMessagesCount << "/" << totalMessagesCount << ")"
              << '\n';
}

void HumanReadablePrinter::printMessageSummary(
    bsl::size_t totalMessagesCount,
    bsl::size_t partiallyConfirmedCount,
    bsl::size_t confirmedCount,
    bsl::size_t outstandingCount) const
{
    if (totalMessagesCount == 0) {
        d_ostream << "\nNo messages found.\n";
    }
    else {
        d_ostream << "\nTotal number of messages: " << totalMessagesCount
                  << '\n';
        d_ostream << "Number of partially confirmed messages: "
                  << partiallyConfirmedCount << '\n';
        d_ostream << "Number of confirmed messages: " << confirmedCount
                  << '\n';
        d_ostream << "Number of outstanding messages: " << outstandingCount
                  << '\n';
    }
}

void HumanReadablePrinter::printQueueOpSummary(
    bsls::Types::Uint64     queueOpRecordsCount,
    const QueueOpCountsVec& queueOpCountsVec) const
{
    BSLS_ASSERT_SAFE(queueOpCountsVec.size() > mqbs::QueueOpType::e_ADDITION);
    if (queueOpRecordsCount == 0) {
        d_ostream << "\nNo queueOp records found.\n";
    }
    else {
        d_ostream << "\nTotal number of queueOp records: "
                  << queueOpRecordsCount << '\n';

        bsl::vector<const char*> fields(d_allocator_p);
        fields.reserve(4);
        fields.push_back("Number of 'purge' operations");
        fields.push_back("Number of 'creation' operations");
        fields.push_back("Number of 'deletion' operations");
        fields.push_back("Number of 'addition' operations");
        bmqu::AlignedPrinter printer(d_ostream, &fields);
        printer << queueOpCountsVec[mqbs::QueueOpType::e_PURGE]
                << queueOpCountsVec[mqbs::QueueOpType::e_CREATION]
                << queueOpCountsVec[mqbs::QueueOpType::e_DELETION]
                << queueOpCountsVec[mqbs::QueueOpType::e_ADDITION];
    }
}

void HumanReadablePrinter::printJournalOpSummary(
    bsls::Types::Uint64 journalOpRecordsCount) const
{
    if (journalOpRecordsCount == 0) {
        d_ostream << "\nNo journalOp records found." << '\n';
    }
    else {
        d_ostream << "\nNumber of journalOp records: " << journalOpRecordsCount
                  << '\n';
    }
}

void HumanReadablePrinter::printRecordSummary(
    bsls::Types::Uint64    totalRecordsCount,
    const QueueDetailsMap& queueDetailsMap) const
{
    d_ostream << "Total number of records: " << totalRecordsCount << "\n";

    // Print information per Queue:
    if (!queueDetailsMap.empty()) {
        d_ostream << "Number of records per Queue:\n";
        printQueueDetails<bmqu::AlignedPrinter>(d_ostream,
                                                queueDetailsMap,
                                                d_allocator_p);
    }
}

void HumanReadablePrinter::printJournalFileMeta(
    const mqbs::JournalFileIterator* journalFile_p) const
{
    d_ostream << "\nDetails of journal file:\n";
    m_bmqstoragetool::printJournalFileMeta<bmqu::AlignedPrinter,
                                           bmqu::AlignedPrinter>(
        d_ostream,
        journalFile_p,
        d_allocator_p);
}

void HumanReadablePrinter::printDataFileMeta(
    const mqbs::DataFileIterator* dataFile_p) const
{
    d_ostream << "\nDetails of data file: \n";
    m_bmqstoragetool::printDataFileMeta<bmqu::AlignedPrinter,
                                        bmqu::AlignedPrinter>(d_ostream,
                                                              dataFile_p,
                                                              d_allocator_p);
}

void HumanReadablePrinter::printGuidsNotFound(const GuidsList& guids) const
{
    // Print non found GUIDs
    if (!guids.empty()) {
        d_ostream << "\nThe following " << guids.size()
                  << " GUID(s) not found:\n";
        GuidsList::const_iterator it = guids.cbegin();
        for (; it != guids.cend(); ++it) {
            d_ostream << *it << '\n';
        }
    }
}

void HumanReadablePrinter::printOffsetsNotFound(
    const OffsetsVec& offsets) const
{
    if (!offsets.empty()) {
        d_ostream << "\nThe following " << offsets.size()
                  << " offset(s) not found:\n";
        OffsetsVec::const_iterator it = offsets.cbegin();
        for (; it != offsets.cend(); ++it) {
            d_ostream << *it << '\n';
        }
    }
}

void HumanReadablePrinter::printCompositesNotFound(
    const CompositesVec& seqNums) const
{
    if (!seqNums.empty()) {
        d_ostream << "\nThe following " << seqNums.size()
                  << " sequence number(s) not found:\n";
        CompositesVec::const_iterator it = seqNums.cbegin();
        for (; it != seqNums.cend(); ++it) {
            d_ostream << *it << '\n';
        }
    }
}

template <typename RECORD_TYPE>
void HumanReadablePrinter::printRecordDetails(
    const RecordDetails<RECORD_TYPE>& recordDetails) const
{
    d_ostream << "==============================\n\n";
    RecordDetailsPrinter<bmqu::AlignedPrinter> printer(d_ostream,
                                                       d_allocator_p);
    printer.printRecordDetails(recordDetails);
    d_ostream << "\n";
}

class JsonPrinter : public Printer {
  protected:
    bsl::ostream&     d_ostream;
    bslma::Allocator* d_allocator_p;
    mutable bool      d_braceOpen;
    mutable bool      d_firstRow;

    void openBraceIfNotOpen(const std::string& fieldName) const;

    void closeBraceIfOpen() const;

  public:
    // CREATORS
    JsonPrinter(bsl::ostream& os, bslma::Allocator* allocator);

    ~JsonPrinter() BSLS_KEYWORD_OVERRIDE;

    // PUBLIC METHODS

    void printGuid(const bmqt::MessageGUID& guid) const BSLS_KEYWORD_OVERRIDE;

    void printGuidNotFound(const bmqt::MessageGUID& guid) const
        BSLS_KEYWORD_OVERRIDE;

    void printFooter(bsls::Types::Uint64                   foundMessagesCount,
                     bsls::Types::Uint64                   foundQueueOpCount,
                     bsls::Types::Uint64                   foundJournalOpCount,
                     const Parameters::ProcessRecordTypes& processRecordTypes)
        const BSLS_KEYWORD_OVERRIDE;

    void printExactMatchFooter(
        bsls::Types::Uint64                   foundMessagesCount,
        bsls::Types::Uint64                   foundConfirmCount,
        bsls::Types::Uint64                   foundDeletionCount,
        bsls::Types::Uint64                   foundQueueOpCount,
        bsls::Types::Uint64                   foundJournalOpCount,
        const Parameters::ProcessRecordTypes& processRecordTypes) const
        BSLS_KEYWORD_OVERRIDE;

    void printOutstandingRatio(int         ratio,
                               bsl::size_t outstandingMessagesCount,
                               BSLA_UNUSED bsl::size_t totalMessagesCount)
        const BSLS_KEYWORD_OVERRIDE;

    void printMessageSummary(bsl::size_t totalMessagesCount,
                             bsl::size_t partiallyConfirmedCount,
                             bsl::size_t confirmedCount,
                             bsl::size_t outstandingCount) const
        BSLS_KEYWORD_OVERRIDE;

    void printQueueOpSummary(bsls::Types::Uint64     queueOpRecordsCount,
                             const QueueOpCountsVec& queueOpCountsVec) const
        BSLS_KEYWORD_OVERRIDE;

    void printJournalOpSummary(bsls::Types::Uint64 journalOpRecordsCount) const
        BSLS_KEYWORD_OVERRIDE;

    void
    printGuidsNotFound(const GuidsList& guids) const BSLS_KEYWORD_OVERRIDE;

    void printOffsetsNotFound(const OffsetsVec& offsets) const
        BSLS_KEYWORD_OVERRIDE;

    void printCompositesNotFound(const CompositesVec& seqNums) const
        BSLS_KEYWORD_OVERRIDE;
};

// PROTECTED METHODS

void JsonPrinter::openBraceIfNotOpen(const std::string& fieldName) const
{
    if (!d_braceOpen) {
        d_ostream << "  \"" << fieldName << "\": [\n";
        d_braceOpen = true;
    }
    else {
        RecordPrinter::printDelimeter<void>(d_ostream);
    }
}

void JsonPrinter::closeBraceIfOpen() const
{
    if (d_braceOpen) {
        d_ostream << "\n  ]";
        d_braceOpen = false;
        d_firstRow  = false;
    }
    if (!d_firstRow) {
        RecordPrinter::printDelimeter<void>(d_ostream);
    }
    else {
        d_firstRow = false;
    }
}

// CREATORS

JsonPrinter::JsonPrinter(bsl::ostream& os, bslma::Allocator* allocator)
: d_ostream(os)
, d_allocator_p(allocator)
, d_braceOpen(false)
, d_firstRow(true)
{
    d_ostream << "{\n";
}

JsonPrinter::~JsonPrinter()
{
    if (d_braceOpen) {
        d_ostream << "\n  ]";
        d_braceOpen = false;
    }
    d_ostream << "\n}\n";
}

// PUBLIC METHODS

void JsonPrinter::printGuid(const bmqt::MessageGUID& guid) const
{
    openBraceIfNotOpen("Records");
    d_ostream << bsl::setw(4) << ' ' << "\"" << guid << "\"";
}

void JsonPrinter::printGuidNotFound(const bmqt::MessageGUID& guid) const
{
    openBraceIfNotOpen("Records");
    d_ostream << bsl::setw(4) << ' ' << "{\"LogicError\" : \"guid " << guid
              << " not found\"}";
}

void JsonPrinter::printFooter(
    bsls::Types::Uint64                   foundMessagesCount,
    bsls::Types::Uint64                   foundQueueOpCount,
    bsls::Types::Uint64                   foundJournalOpCount,
    const Parameters::ProcessRecordTypes& processRecordTypes) const
{
    if (processRecordTypes.d_message) {
        closeBraceIfOpen();
        d_ostream << "  \"MessageRecords\": \"" << foundMessagesCount << "\"";
    }
    if (processRecordTypes.d_queueOp) {
        closeBraceIfOpen();
        d_ostream << "  \"QueueOpRecords\": \"" << foundQueueOpCount << "\"";
    }
    if (processRecordTypes.d_journalOp) {
        closeBraceIfOpen();
        d_ostream << "  \"JournalOpRecords\": \"" << foundJournalOpCount
                  << "\"";
    }
}

void JsonPrinter::printExactMatchFooter(
    bsls::Types::Uint64                   foundMessagesCount,
    bsls::Types::Uint64                   foundConfirmCount,
    bsls::Types::Uint64                   foundDeletionCount,
    bsls::Types::Uint64                   foundQueueOpCount,
    bsls::Types::Uint64                   foundJournalOpCount,
    const Parameters::ProcessRecordTypes& processRecordTypes) const
{
    if (processRecordTypes.d_message) {
        closeBraceIfOpen();
        d_ostream << "  \"ConfirmRecords\": \"" << foundConfirmCount << "\"";
        RecordPrinter::printDelimeter<void>(d_ostream);
        d_ostream << "  \"DeletionRecords\": \"" << foundDeletionCount << "\"";
    }

    printFooter(foundMessagesCount,
                foundQueueOpCount,
                foundJournalOpCount,
                processRecordTypes);
}

void JsonPrinter::printOutstandingRatio(
    int         ratio,
    bsl::size_t outstandingMessagesCount,
    BSLA_UNUSED bsl::size_t totalMessagesCount) const
{
    closeBraceIfOpen();
    d_ostream << "  \"OutstandingRatio\": \"" << ratio
              << "\",\n  \"OutstandingMessages\": \""
              << outstandingMessagesCount << "\"";
}

void JsonPrinter::printMessageSummary(bsl::size_t totalMessagesCount,
                                      bsl::size_t partiallyConfirmedCount,
                                      bsl::size_t confirmedCount,
                                      bsl::size_t outstandingCount) const
{
    closeBraceIfOpen();
    d_ostream << "  \"TotalMessagesNumber\": \"" << totalMessagesCount
              << "\",\n  \"PartiallyConfirmedMessagesNumber\": \""
              << partiallyConfirmedCount
              << "\",\n  \"ConfirmedMessagesNumber\": \"" << confirmedCount
              << "\",\n  \"OutstandingMessagesNumber\": \"" << outstandingCount
              << "\"";
}

void JsonPrinter::printQueueOpSummary(
    bsls::Types::Uint64     queueOpRecordsCount,
    const QueueOpCountsVec& queueOpCountsVec) const
{
    BSLS_ASSERT_SAFE(queueOpCountsVec.size() > mqbs::QueueOpType::e_ADDITION);
    closeBraceIfOpen();
    bsl::vector<const char*> fields(d_allocator_p);
    fields.reserve(5);
    fields.push_back("TotalQueueOperationsNumber");
    fields.push_back("PurgeOperationsNumber");
    fields.push_back("CreationOperationsNumber");
    fields.push_back("DeletionOperationsNumber");
    fields.push_back("AdditionOperationsNumber");

    bmqu::JsonPrinter<true, false, 0, 2> printer(d_ostream, &fields);
    printer << queueOpRecordsCount
            << queueOpCountsVec[mqbs::QueueOpType::e_PURGE]
            << queueOpCountsVec[mqbs::QueueOpType::e_CREATION]
            << queueOpCountsVec[mqbs::QueueOpType::e_DELETION]
            << queueOpCountsVec[mqbs::QueueOpType::e_ADDITION];
}

void JsonPrinter::printJournalOpSummary(
    bsls::Types::Uint64 journalOpRecordsCount) const
{
    closeBraceIfOpen();
    d_ostream << "  \"JournalOperationsNumber\": \"" << journalOpRecordsCount
              << "\"";
}

void JsonPrinter::printGuidsNotFound(const GuidsList& guids) const
{
    closeBraceIfOpen();
    d_ostream << "  \"GuidsNotFound\": [";
    GuidsList::const_iterator it = guids.cbegin();
    for (; it != guids.cend(); ++it) {
        if (it != guids.cbegin()) {
            d_ostream << ',';
        }
        d_ostream << '\n' << bsl::setw(4) << ' ' << '\"' << *it << "\"";
    }
    d_ostream << "\n  ]";
}

void JsonPrinter::printOffsetsNotFound(const OffsetsVec& offsets) const
{
    closeBraceIfOpen();
    d_ostream << "  \"OffsetsNotFound\": [";
    OffsetsVec::const_iterator it = offsets.cbegin();
    for (; it != offsets.cend(); ++it) {
        if (it != offsets.cbegin()) {
            d_ostream << ",";
        }
        d_ostream << "\n    " << *it;
    }
    d_ostream << "\n  ]";
}

void JsonPrinter::printCompositesNotFound(const CompositesVec& seqNums) const
{
    closeBraceIfOpen();
    d_ostream << "  \"SequenceNumbersNotFound\": [";
    CompositesVec::const_iterator it = seqNums.cbegin();
    for (; it != seqNums.cend(); ++it) {
        if (it != seqNums.cbegin()) {
            d_ostream << ',';
        }

        d_ostream << "\n    {\"leaseId\": \"" << it->leaseId()
                  << "\", \"sequenceNumber\": \"" << it->sequenceNumber()
                  << "\"}";
    }
    d_ostream << "\n  ]";
}

class JsonPrettyPrinter : public JsonPrinter {
  private:
    // PRIVATE ACCESSORS

    template <typename RECORD_TYPE>
    void
    printRecordDetails(const RecordDetails<RECORD_TYPE>& recordDetails) const;

  public:
    // CREATORS
    JsonPrettyPrinter(bsl::ostream& os, bslma::Allocator* allocator);

    ~JsonPrettyPrinter() BSLS_KEYWORD_OVERRIDE;

    // PUBLIC METHODS

    void
    printMessage(const MessageDetails& details) const BSLS_KEYWORD_OVERRIDE;

    void printConfirmRecord(const RecordDetails<mqbs::ConfirmRecord>& rec)
        const BSLS_KEYWORD_OVERRIDE;

    void printDeletionRecord(const RecordDetails<mqbs::DeletionRecord>& rec)
        const BSLS_KEYWORD_OVERRIDE;

    void printQueueOpRecord(const RecordDetails<mqbs::QueueOpRecord>& rec)
        const BSLS_KEYWORD_OVERRIDE;

    void printJournalOpRecord(const RecordDetails<mqbs::JournalOpRecord>& rec)
        const BSLS_KEYWORD_OVERRIDE;

    void printJournalFileMeta(const mqbs::JournalFileIterator* journalFile_p)
        const BSLS_KEYWORD_OVERRIDE;

    void printDataFileMeta(const mqbs::DataFileIterator* dataFile_p) const
        BSLS_KEYWORD_OVERRIDE;

    void printRecordSummary(bsls::Types::Uint64    totalRecordsCount,
                            const QueueDetailsMap& queueDetailsMap) const
        BSLS_KEYWORD_OVERRIDE;
};

// CREATORS
JsonPrettyPrinter::JsonPrettyPrinter(bsl::ostream&     os,
                                     bslma::Allocator* allocator)
: JsonPrinter(os, allocator)
{
    // NOTHING
}

JsonPrettyPrinter::~JsonPrettyPrinter()
{
    // NOTHING
}

// PUBLIC METHODS

void JsonPrettyPrinter::printMessage(const MessageDetails& details) const
{
    openBraceIfNotOpen("Records");
    printMessageDetails<bmqu::JsonPrinter<true, true, 4, 6> >(d_ostream,
                                                              details,
                                                              d_allocator_p);
}

void JsonPrettyPrinter::printConfirmRecord(
    const RecordDetails<mqbs::ConfirmRecord>& rec) const
{
    printRecordDetails(rec);
}

void JsonPrettyPrinter::printDeletionRecord(
    const RecordDetails<mqbs::DeletionRecord>& rec) const
{
    printRecordDetails(rec);
}

void JsonPrettyPrinter::printQueueOpRecord(
    const RecordDetails<mqbs::QueueOpRecord>& rec) const
{
    printRecordDetails(rec);
}

void JsonPrettyPrinter::printJournalOpRecord(
    const RecordDetails<mqbs::JournalOpRecord>& rec) const
{
    printRecordDetails(rec);
}

void JsonPrettyPrinter::printJournalFileMeta(
    const mqbs::JournalFileIterator* journalFile_p) const
{
    closeBraceIfOpen();
    d_ostream << "  \"JournalFileDetails\":\n";
    m_bmqstoragetool::printJournalFileMeta<
        bmqu::JsonPrinter<true, true, 2, 4>,
        bmqu::JsonPrinter<true, true, 4, 6> >(d_ostream,
                                              journalFile_p,
                                              d_allocator_p);
}

void JsonPrettyPrinter::printDataFileMeta(
    const mqbs::DataFileIterator* dataFile_p) const
{
    closeBraceIfOpen();
    d_ostream << "  \"DataFileDetails\":\n";
    m_bmqstoragetool::printDataFileMeta<bmqu::JsonPrinter<true, true, 2, 4>,
                                        bmqu::JsonPrinter<true, true, 4, 6> >(
        d_ostream,
        dataFile_p,
        d_allocator_p);
}

void JsonPrettyPrinter::printRecordSummary(
    bsls::Types::Uint64    totalRecordsCount,
    const QueueDetailsMap& queueDetailsMap) const
{
    closeBraceIfOpen();
    d_ostream << "  \"TotalRecordsNumber\": \"" << totalRecordsCount
              << "\",\n";

    // Print information per Queue:
    d_ostream << "  \"PerQueueRecordsNumber\": [\n";
    printQueueDetails<bmqu::JsonPrinter<true, true, 4, 6> >(d_ostream,
                                                            queueDetailsMap,
                                                            d_allocator_p);
    d_ostream << "\n  ]";
}

template <typename RECORD_TYPE>
void JsonPrettyPrinter::printRecordDetails(
    const RecordDetails<RECORD_TYPE>& recordDetails) const
{
    openBraceIfNotOpen("Records");
    RecordDetailsPrinter<bmqu::JsonPrinter<true, true, 4, 6> > printer(
        d_ostream,
        d_allocator_p);
    printer.printRecordDetails(recordDetails);
}

class JsonLinePrinter : public JsonPrinter {
  private:
    // PRIVATE ACCESSORS

    template <typename RECORD_TYPE>
    void
    printRecordDetails(const RecordDetails<RECORD_TYPE>& recordDetails) const;

  public:
    // CREATORS
    JsonLinePrinter(bsl::ostream& os, bslma::Allocator* allocator);

    ~JsonLinePrinter() BSLS_KEYWORD_OVERRIDE;

    // PUBLIC METHODS

    void
    printMessage(const MessageDetails& details) const BSLS_KEYWORD_OVERRIDE;

    void printConfirmRecord(const RecordDetails<mqbs::ConfirmRecord>& rec)
        const BSLS_KEYWORD_OVERRIDE;

    void printDeletionRecord(const RecordDetails<mqbs::DeletionRecord>& rec)
        const BSLS_KEYWORD_OVERRIDE;

    void printQueueOpRecord(const RecordDetails<mqbs::QueueOpRecord>& rec)
        const BSLS_KEYWORD_OVERRIDE;

    void printJournalOpRecord(const RecordDetails<mqbs::JournalOpRecord>& rec)
        const BSLS_KEYWORD_OVERRIDE;

    void printJournalFileMeta(const mqbs::JournalFileIterator* journalFile_p)
        const BSLS_KEYWORD_OVERRIDE;

    void printDataFileMeta(const mqbs::DataFileIterator* dataFile_p) const
        BSLS_KEYWORD_OVERRIDE;

    void printRecordSummary(bsls::Types::Uint64    totalRecordsCount,
                            const QueueDetailsMap& queueDetailsMap) const
        BSLS_KEYWORD_OVERRIDE;
};

// CREATORS
JsonLinePrinter::JsonLinePrinter(bsl::ostream& os, bslma::Allocator* allocator)
: JsonPrinter(os, allocator)
{
    // NOTHING
}

JsonLinePrinter::~JsonLinePrinter()
{
    // NOTHING
}

// PUBLIC METHODS

void JsonLinePrinter::printMessage(const MessageDetails& details) const
{
    openBraceIfNotOpen("Records");
    printMessageDetails<bmqu::JsonPrinter<false, true, 4, 6> >(d_ostream,
                                                               details,
                                                               d_allocator_p);
}

void JsonLinePrinter::printConfirmRecord(
    const RecordDetails<mqbs::ConfirmRecord>& rec) const
{
    printRecordDetails(rec);
}

void JsonLinePrinter::printDeletionRecord(
    const RecordDetails<mqbs::DeletionRecord>& rec) const
{
    printRecordDetails(rec);
}

void JsonLinePrinter::printQueueOpRecord(
    const RecordDetails<mqbs::QueueOpRecord>& rec) const
{
    printRecordDetails(rec);
}

void JsonLinePrinter::printJournalOpRecord(
    const RecordDetails<mqbs::JournalOpRecord>& rec) const
{
    printRecordDetails(rec);
}

void JsonLinePrinter::printJournalFileMeta(
    const mqbs::JournalFileIterator* journalFile_p) const
{
    closeBraceIfOpen();
    d_ostream << "  \"JournalFileDetails\":\n";
    m_bmqstoragetool::printJournalFileMeta<
        bmqu::JsonPrinter<true, true, 2, 4>,
        bmqu::JsonPrinter<false, true, 6, 0> >(d_ostream,
                                               journalFile_p,
                                               d_allocator_p);
}

void JsonLinePrinter::printDataFileMeta(
    const mqbs::DataFileIterator* dataFile_p) const
{
    closeBraceIfOpen();
    d_ostream << "  \"DataFileDetails\": \n";
    m_bmqstoragetool::printDataFileMeta<bmqu::JsonPrinter<true, true, 2, 4>,
                                        bmqu::JsonPrinter<false, true, 6, 0> >(
        d_ostream,
        dataFile_p,
        d_allocator_p);
}

void JsonLinePrinter::printRecordSummary(
    bsls::Types::Uint64    totalRecordsCount,
    const QueueDetailsMap& queueDetailsMap) const
{
    closeBraceIfOpen();
    d_ostream << "  \"TotalRecordsNumber\": \"" << totalRecordsCount
              << "\",\n";

    // Print information per Queue:
    d_ostream << "  \"PerQueueRecordsNumber\": [\n";
    printQueueDetails<bmqu::JsonPrinter<false, true, 4, 6> >(d_ostream,
                                                             queueDetailsMap,
                                                             d_allocator_p);
    d_ostream << "\n  ]";
}

template <typename RECORD_TYPE>
void JsonLinePrinter::printRecordDetails(
    const RecordDetails<RECORD_TYPE>& recordDetails) const
{
    openBraceIfNotOpen("Records");
    RecordDetailsPrinter<bmqu::JsonPrinter<false, true, 4, 6> > printer(
        d_ostream,
        d_allocator_p);
    printer.printRecordDetails(recordDetails);
}

bsl::shared_ptr<Printer> createPrinter(Parameters::PrintMode mode,
                                       std::ostream&         stream,
                                       bslma::Allocator*     allocator)
{
    bsl::shared_ptr<Printer> printer;
    if (mode == Parameters::e_HUMAN) {
        printer.load(new (*allocator) HumanReadablePrinter(stream, allocator),
                     allocator);
    }
    else if (mode == Parameters::e_JSON_PRETTY) {
        printer.load(new (*allocator) JsonPrettyPrinter(stream, allocator),
                     allocator);
    }
    else if (mode == Parameters::e_JSON_LINE) {
        printer.load(new (*allocator) JsonLinePrinter(stream, allocator),
                     allocator);
    }
    return printer;
}

}  // close package namespace
}  // close enterprise namespace
