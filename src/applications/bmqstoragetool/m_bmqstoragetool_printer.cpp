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
#include <m_bmqstoragetool_printer.h>

// bmqstoragetool
#include <m_bmqstoragetool_messagedetails.h>

// BMQ
#include <bmqu_alignedprinter.h>
#include <bmqu_jsonprinter.h>

// MQB
#include <mqbs_filestoreprotocol.h>
#include <mqbs_filestoreprotocolprinter.h>

// BDE
#include <bsl_vector.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

namespace {

// ==========================
// class RecordDetailsPrinter
// ==========================

template <typename PRINTER_TYPE>
class RecordDetailsPrinter {
  private:
    bsl::ostream&                   d_stream;
    bsl::vector<const char*>        d_fields;
    bslma::ManagedPtr<PRINTER_TYPE> d_printer_mp;
    bslma::Allocator*               d_allocator_p;

    // PRIVATE METHODS

    /// Print the specified message record `rec` to the specified
    /// `printer`.
    void
    printRecord(const MessageDetails::RecordDetails<mqbs::MessageRecord>& rec);
    /// Print the specified confirm record `rec` to the specified
    /// `printer`.
    void
    printRecord(const MessageDetails::RecordDetails<mqbs::ConfirmRecord>& rec);
    /// Print the specified delete record `rec` to the specified
    /// `printer`.
    void printRecord(
        const MessageDetails::RecordDetails<mqbs::DeletionRecord>& rec);

  public:
    // CREATORS
    RecordDetailsPrinter(bsl::ostream& stream, bslma::Allocator* allocator);

    // PUBLIC METHODS

    /// Print this object to the specified `os` stream.
    template <typename RECORD_TYPE>
    void printRecordDetails(
        const MessageDetails::RecordDetails<RECORD_TYPE>& details);
};

template <typename PRINTER_TYPE>
void printDelimeter(bsl::ostream& ostream)
{
    ostream << ",\n";
}

template <>
void printDelimeter<bmqu::AlignedPrinter>(bsl::ostream& ostream)
{
    ostream << "\n";
}

template <typename PRINTER_TYPE>
void printMessageDetails(bsl::ostream&         os,
                         const MessageDetails& details,
                         bslma::Allocator*     allocator)
{
    RecordDetailsPrinter<PRINTER_TYPE> printer(os, allocator);

    // Print message record
    printer.printRecordDetails(details.messageRecord());

    // Print confirmation records
    const bsl::vector<MessageDetails::RecordDetails<mqbs::ConfirmRecord> >&
        confirmRecords = details.confirmRecords();
    if (!confirmRecords.empty()) {
        typename bsl::vector<MessageDetails::RecordDetails<
            mqbs::ConfirmRecord> >::const_iterator it = confirmRecords.begin();
        for (; it != confirmRecords.end(); ++it) {
            printer.printRecordDetails(*it);
        }
    }

    // Print deletion record
    if (!details.deleteRecord().isNull()) {
        printer.printRecordDetails(details.deleteRecord().value());
    }
}

// Helper to print data file meta data
template <typename PRINTER_TYPE1, typename PRINTER_TYPE2>
void printDataFileMeta(bsl::ostream&                 ostream,
                       const mqbs::DataFileIterator* dataFile_p)
{
    if (!dataFile_p || !dataFile_p->isValid()) {
        return;  // RETURN
    }

    const bsl::vector<const char*> fields = {"BlazingMQ File Header",
                                             "Data File Header"};

    PRINTER_TYPE1 printer(ostream, &fields);
    {
        bsl::ostringstream s;
        mqbs::FileStoreProtocolPrinter::printFileHeader<PRINTER_TYPE2>(
            s,
            *dataFile_p->mappedFileDescriptor());
        printer << s.str();
    }
    {
        bsl::ostringstream s;
        mqbs::FileStoreProtocolPrinter::printDataFileHeader<PRINTER_TYPE2>(
            s,
            dataFile_p->header());
        printer << s.str();
    }
}

// Helper to print journal file meta data
template <typename PRINTER_TYPE1, typename PRINTER_TYPE2>
void printJournalFileMeta(bsl::ostream&                    ostream,
                          const mqbs::JournalFileIterator* journalFile_p,
                          bslma::Allocator*                allocator)
{
    if (!journalFile_p || !journalFile_p->isValid()) {
        return;  // RETURN
    }

    const bsl::vector<const char*> fields = {"BlazingMQ File Header",
                                             "Journal File Header",
                                             "Journal SyncPoint"};

    PRINTER_TYPE1 printer(ostream, &fields);
    {
        bsl::ostringstream s;
        mqbs::FileStoreProtocolPrinter::printFileHeader<PRINTER_TYPE2>(
            s,
            *journalFile_p->mappedFileDescriptor());
        printer << s.str();
    }
    {
        bsl::ostringstream s;
        mqbs::FileStoreProtocolPrinter::printJournalFileHeader<PRINTER_TYPE2>(
            s,
            journalFile_p->header(),
            *journalFile_p->mappedFileDescriptor(),
            allocator);
        printer << s.str();
    }

    {
        bsl::ostringstream s;
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
                p << "** NA **"
                  << "** NA **";
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
                p << "** NA **"
                  << "** NA **"
                  << "** NA **"
                  << "** NA **"
                  << "** NA **"
                  << "** NA **";
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

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------
// RecordDetailsPrinter
// --------------------

template <typename PRINTER_TYPE>
RecordDetailsPrinter<PRINTER_TYPE>::RecordDetailsPrinter(
    std::ostream&     stream,
    bslma::Allocator* allocator)
: d_stream(stream)
, d_fields(allocator)
, d_printer_mp()
, d_allocator_p(allocator)
{
    // NOTHING
}

template <typename PRINTER_TYPE>
template <typename RECORD_TYPE>
void RecordDetailsPrinter<PRINTER_TYPE>::printRecordDetails(
    const MessageDetails::RecordDetails<RECORD_TYPE>& details)
{
    d_fields.clear();
    d_fields.reserve(16);  // max number of fields
    d_fields.push_back("RecordType");
    d_fields.push_back("Index");
    d_fields.push_back("Offset");
    d_fields.push_back("GUID");
    d_fields.push_back("PrimaryLeaseId");
    d_fields.push_back("SequenceNumber");
    d_fields.push_back("Timestamp");
    d_fields.push_back("Epoch");
    d_fields.push_back("QueueKey");

    // It's ok to pass a vector by pointer and push elements after that as
    // we've reserved it's capacity in advance. Hense, no reallocations will
    // happen and the pointer won't get invalidated.
    d_printer_mp.load(new (*d_allocator_p) PRINTER_TYPE(d_stream, &d_fields),
                      d_allocator_p);

    *d_printer_mp << details.d_record.header().type() << details.d_recordIndex
                  << details.d_recordOffset << details.d_record.messageGUID()
                  << details.d_record.header().primaryLeaseId()
                  << details.d_record.header().sequenceNumber();

    bsls::Types::Uint64 epochValue = details.d_record.header().timestamp();
    bdlt::Datetime      datetime;
    const int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime, epochValue);
    if (0 != rc) {
        *d_printer_mp << 0;
    }
    else {
        *d_printer_mp << datetime;
    }
    *d_printer_mp << epochValue;

    bmqu::MemOutStream queueKeyStr(d_allocator_p);
    queueKeyStr << details.d_record.queueKey();
    *d_printer_mp << queueKeyStr.str();
    if (!details.d_queueUri.empty()) {
        d_fields.push_back("QueueUri");
        *d_printer_mp << details.d_queueUri;
    }

    printRecord(details);
    d_printer_mp.reset();
    printDelimeter<PRINTER_TYPE>(d_stream);
}

template <typename PRINTER_TYPE>
void RecordDetailsPrinter<PRINTER_TYPE>::printRecord(
    const MessageDetails::RecordDetails<mqbs::MessageRecord>& rec)
{
    d_fields.push_back("FileKey");
    d_fields.push_back("RefCount");
    d_fields.push_back("MsgOffsetDwords");
    d_fields.push_back("Crc32c");

    bmqu::MemOutStream fileKeyStr(d_allocator_p);
    fileKeyStr << rec.d_record.fileKey();

    *d_printer_mp << fileKeyStr.str() << rec.d_record.refCount()
                  << rec.d_record.messageOffsetDwords()
                  << rec.d_record.crc32c();
}

template <typename PRINTER_TYPE>
void RecordDetailsPrinter<PRINTER_TYPE>::printRecord(
    const MessageDetails::RecordDetails<mqbs::ConfirmRecord>& rec)
{
    d_fields.push_back("AppKey");
    if (rec.d_record.appKey().isNull()) {
        *d_printer_mp << "** NULL **";
    }
    else {
        *d_printer_mp << rec.d_record.appKey();
    }

    if (!rec.d_appId.empty()) {
        d_fields.push_back("AppId");
        *d_printer_mp << rec.d_appId;
    }
}

template <typename PRINTER_TYPE>
void RecordDetailsPrinter<PRINTER_TYPE>::printRecord(
    const MessageDetails::RecordDetails<mqbs::DeletionRecord>& rec)
{
    d_fields.push_back("DeletionFlag");
    *d_printer_mp << rec.d_record.deletionRecordFlag();
}

}  // close anonymous namespace

class HumanReadablePrinter : public Printer {
    bsl::ostream&     d_stream;
    bslma::Allocator* d_allocator_p;

  public:
    // CREATORS
    HumanReadablePrinter(bsl::ostream& os, bslma::Allocator* allocator)
    : d_stream(os)
    , d_allocator_p(allocator)
    {
    }

    ~HumanReadablePrinter() BSLS_KEYWORD_OVERRIDE { d_stream << "\n"; }

    // PUBLIC METHODS

    void
    printMessage(const MessageDetails& details) const BSLS_KEYWORD_OVERRIDE
    {
        d_stream << "==============================\n\n";
        printMessageDetails<bmqu::AlignedPrinter>(d_stream,
                                                  details,
                                                  d_allocator_p);
    }

    void printGuidNotFound(const bmqt::MessageGUID& guid) const
        BSLS_KEYWORD_OVERRIDE
    {
        d_stream << "Logic error : guid " << guid << " not found\n";
    }

    void printGuid(const bmqt::MessageGUID& guid) const BSLS_KEYWORD_OVERRIDE
    {
        d_stream << guid << '\n';
    }

    void
    printFooter(bsl::size_t foundMessagesCount) const BSLS_KEYWORD_OVERRIDE
    {
        if (foundMessagesCount > 0) {
            d_stream << foundMessagesCount << " message(s) found.";
        }
        else {
            d_stream << "No messages found.";
        }
        d_stream << '\n';
    }

    void printOutstandingRatio(const OutstandingPrintBundle& bundle) const
        BSLS_KEYWORD_OVERRIDE
    {
        d_stream << "Outstanding ratio: " << bundle.d_ratio << "% ("
                 << bundle.d_outstandingMessages << "/"
                 << bundle.d_totalMessagesCount << ")" << '\n';
    }

    void printSummary(bsl::size_t foundMessagesCount,
                      bsl::size_t deletedMessagesCount,
                      bsl::size_t partiallyConfirmedCount) const
        BSLS_KEYWORD_OVERRIDE
    {
        d_stream << "Number of confirmed messages: " << deletedMessagesCount
                 << '\n';
        d_stream << "Number of partially confirmed messages: "
                 << partiallyConfirmedCount << '\n';
        d_stream << "Number of outstanding messages: "
                 << (foundMessagesCount - deletedMessagesCount) << '\n';
    }

    void printJournalFileMeta(const mqbs::JournalFileIterator* journalFile_p,
                              bslma::Allocator*                allocator) const
        BSLS_KEYWORD_OVERRIDE
    {
        d_stream << "\nDetails of journal file:\n";
        m_bmqstoragetool::printJournalFileMeta<bmqu::AlignedPrinter,
                                               bmqu::AlignedPrinter>(
            d_stream,
            journalFile_p,
            allocator);
        printDelimeter<bmqu::AlignedPrinter>(d_stream);
    }

    void printDataFileMeta(const mqbs::DataFileIterator* dataFile_p) const
        BSLS_KEYWORD_OVERRIDE
    {
        d_stream << "\nDetails of data file: \n";
        m_bmqstoragetool::printDataFileMeta<bmqu::AlignedPrinter,
                                            bmqu::AlignedPrinter>(d_stream,
                                                                  dataFile_p);
    }

    void printGuidsNotFound(const GuidsList& guids) const BSLS_KEYWORD_OVERRIDE
    {
        // Print non found GUIDs
        if (!guids.empty()) {
            d_stream << '\n'
                     << "The following " << guids.size()
                     << " GUID(s) not found:" << '\n';
            GuidsList::const_iterator it = guids.cbegin();
            for (; it != guids.cend(); ++it) {
                d_stream << *it << '\n';
            }
        }
    }
};

class JsonPrinter : public Printer {
  protected:
    bsl::ostream&     d_stream;
    bslma::Allocator* d_allocator_p;
    mutable bool      d_braceOpen;

  public:
    // CREATORS
    JsonPrinter(bsl::ostream& os, bslma::Allocator* allocator)
    : d_stream(os)
    , d_allocator_p(allocator)
    , d_braceOpen(false)
    {
        d_stream << "{\n";
    }

    ~JsonPrinter() BSLS_KEYWORD_OVERRIDE
    {
        if (d_braceOpen) {
            d_stream << "  ]\n";
            d_braceOpen = false;
        }
        d_stream << "\n}\n";
    }

    // PUBLIC METHODS

    void printGuid(const bmqt::MessageGUID& guid) const BSLS_KEYWORD_OVERRIDE
    {
        if (!d_braceOpen) {
            d_stream << bsl::setw(2) << ' ' << "\"GuidsFound\": [\n";
            d_braceOpen = true;
        }
        d_stream << bsl::setw(4) << ' ' << "\"" << guid << "\",\n";
    }

    void printGuidNotFound(const bmqt::MessageGUID& guid) const
        BSLS_KEYWORD_OVERRIDE
    {
        d_stream << bsl::setw(4) << ' ' << "{\"Logic error\" : \"guid " << guid
                 << " not found\"},\n";
    }

    void printFooter(BSLS_ANNOTATION_UNUSED bsl::size_t foundMessagesCount)
        const BSLS_KEYWORD_OVERRIDE
    {
        if (d_braceOpen) {
            d_stream << "  ],\n";
            d_braceOpen = false;
        }
        if (foundMessagesCount > 0) {
            d_stream << "  \"TotalMessages\": " << foundMessagesCount << ",\n";
        }
    }

    void printOutstandingRatio(const OutstandingPrintBundle& bundle) const
        BSLS_KEYWORD_OVERRIDE
    {
        if (d_braceOpen) {
            d_stream << "  ],\n";
            d_braceOpen = false;
        }
        d_stream << "  \"OutstandingRatio\": " << bundle.d_ratio
                 << ",\n  \"OutstandingMessages\": "
                 << bundle.d_outstandingMessages << ",\n";
    }

    void printSummary(bsl::size_t foundMessagesCount,
                      bsl::size_t deletedMessagesCount,
                      bsl::size_t partiallyConfirmedCount) const
        BSLS_KEYWORD_OVERRIDE
    {
        if (d_braceOpen) {
            d_stream << "  ],\n";
            d_braceOpen = false;
        }
        d_stream << "  \"Number of confirmed messages\": "
                 << deletedMessagesCount
                 << ",\n  \"Number of partially confirmed messages\": "
                 << partiallyConfirmedCount
                 << ",\n  \"Number of outstanding messages\": "
                 << (foundMessagesCount - deletedMessagesCount) << '\n';
    }

    void printGuidsNotFound(const GuidsList& guids) const BSLS_KEYWORD_OVERRIDE
    {
        if (d_braceOpen) {
            d_stream << "  ],\n";
            d_braceOpen = false;
        }
        if (!guids.empty()) {
            d_stream << "  \"Not found GUIDs\": [\n";
            GuidsList::const_iterator it = guids.cbegin();
            for (; it != guids.cend(); ++it) {
                d_stream << bsl::setw(4) << ' ' << '\"' << *it << "\",\n";
            }
            d_stream << "],\n";
        }
    }
};

class JsonPrettyPrinter : public JsonPrinter {
  public:
    // CREATORS
    JsonPrettyPrinter(bsl::ostream& os, bslma::Allocator* allocator)
    : JsonPrinter(os, allocator)
    {
    }

    ~JsonPrettyPrinter() BSLS_KEYWORD_OVERRIDE {}

    // PUBLIC METHODS

    void
    printMessage(const MessageDetails& details) const BSLS_KEYWORD_OVERRIDE
    {
        if (!d_braceOpen) {
            d_stream << bsl::setw(2) << ' ' << "\"messages found\": [\n";
            d_braceOpen = true;
        }
        printMessageDetails<bmqu::JsonPrinter<true, 4, 6> >(d_stream,
                                                            details,
                                                            d_allocator_p);
    }

    void printJournalFileMeta(const mqbs::JournalFileIterator* journalFile_p,
                              bslma::Allocator*                allocator) const
        BSLS_KEYWORD_OVERRIDE
    {
        d_stream << "  \"Details of journal file\":\n";
        m_bmqstoragetool::printJournalFileMeta<bmqu::JsonPrinter<true, 2, 4>,
                                               bmqu::JsonPrinter<true, 4, 6> >(
            d_stream,
            journalFile_p,
            allocator);
        printDelimeter<void>(d_stream);
    }

    void printDataFileMeta(const mqbs::DataFileIterator* dataFile_p) const
        BSLS_KEYWORD_OVERRIDE
    {
        d_stream << "  \"Details of data file\":\n";
        m_bmqstoragetool::printDataFileMeta<bmqu::JsonPrinter<true, 2, 4>,
                                            bmqu::JsonPrinter<true, 4, 6> >(
            d_stream,
            dataFile_p);
        d_stream << ",\n";
    }
};

class JsonLinePrinter : public JsonPrinter {
  public:
    // CREATORS
    JsonLinePrinter(bsl::ostream& os, bslma::Allocator* allocator)
    : JsonPrinter(os, allocator)
    {
    }

    ~JsonLinePrinter() BSLS_KEYWORD_OVERRIDE {}

    // PUBLIC METHODS

    void
    printMessage(const MessageDetails& details) const BSLS_KEYWORD_OVERRIDE
    {
        if (!d_braceOpen) {
            d_stream << bsl::setw(2) << ' ' << "\"messages found\": [\n";
            d_braceOpen = true;
        }
        printMessageDetails<bmqu::JsonPrinter<false, 4, 6> >(d_stream,
                                                             details,
                                                             d_allocator_p);
        d_stream << ",\n";
    }

    void printJournalFileMeta(const mqbs::JournalFileIterator* journalFile_p,
                              bslma::Allocator*                allocator) const
        BSLS_KEYWORD_OVERRIDE
    {
        d_stream << "  \"Details of journal file\":\n";
        m_bmqstoragetool::printJournalFileMeta<
            bmqu::JsonPrinter<true, 2, 4>,
            bmqu::JsonPrinter<false, 0, 0> >(d_stream,
                                             journalFile_p,
                                             allocator);
        printDelimeter<void>(d_stream);
    }

    void printDataFileMeta(const mqbs::DataFileIterator* dataFile_p) const
        BSLS_KEYWORD_OVERRIDE
    {
        d_stream << "  \"Details of data file\": \n";
        m_bmqstoragetool::printDataFileMeta<bmqu::JsonPrinter<true, 2, 4>,
                                            bmqu::JsonPrinter<false, 0, 0> >(
            d_stream,
            dataFile_p);
        d_stream << ",\n";
    }
};

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
