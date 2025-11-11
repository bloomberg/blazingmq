// Copyright 2014-2025 Bloomberg Finance L.P.
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

#ifndef INCLUDED_M_BMQSTORAGETOOL_RECORDPRINTER
#define INCLUDED_M_BMQSTORAGETOOL_RECORDPRINTER

//@PURPOSE: Provide utilities for printing journal file records for different
// formats.
//
//@CLASSES: m_bmqstoragetool::RecordPrinter::RecordDetailsPrinter
//
//@DESCRIPTION: 'm_bmqstoragetool::RecordPrinter' namespace provides classes
// and methods
// for printing journal file records details. These methods are enhanced
// versions of
// mqbs::FileStoreProtocolPrinter::printRecord() methods.

// BMQ
#include <m_bmqstoragetool_messagedetails.h>

// BDE
#include <bslma_allocator.h>

// MQB
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbs_filestoreprotocolprinter.h>
#include <mqbsi_ledger.h>

// BDE
#include <bsl_ostream.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =======================
// namespace RecordPrinter
// =======================

namespace RecordPrinter {

typedef bsl::vector<bsl::pair<bsls::Types::Uint64, mqbu::StorageKey> >
    AppsData;
// Vector of pairs of record counts and AppKey's

// ==========================
// class RecordDetailsPrinter
// ==========================

template <typename PRINTER_TYPE>
class RecordDetailsPrinter {
  private:
    bsl::ostream&                   d_ostream;
    bsl::vector<const char*>        d_fields;
    bslma::ManagedPtr<PRINTER_TYPE> d_printer_mp;
    bslma::Allocator*               d_allocator_p;

    // PRIVATE METHODS

    /// Print the specified message record `rec`.
    void printRecord(const RecordDetails<mqbs::MessageRecord>& rec);
    /// Print the specified confirm record `rec`.
    void printRecord(const RecordDetails<mqbs::ConfirmRecord>& rec);
    /// Print the specified delete record `rec`.
    void printRecord(const RecordDetails<mqbs::DeletionRecord>& rec);
    /// Print the specified queue operation record `rec`.
    void printRecord(const RecordDetails<mqbs::QueueOpRecord>& rec);
    /// Print the specified journal operation record `rec`.
    void printRecord(const RecordDetails<mqbs::JournalOpRecord>& rec);

    template <typename RECORD_TYPE>
    void printAppInfo(const RecordDetails<RECORD_TYPE>& rec);

    template <typename RECORD_TYPE>
    void printQueueInfo(const RecordDetails<RECORD_TYPE>& rec);

  public:
    // CREATORS
    RecordDetailsPrinter(bsl::ostream& stream, bslma::Allocator* allocator);

    // PUBLIC METHODS

    /// Print journal record details.
    template <typename RECORD_TYPE>
    void printRecordDetails(const RecordDetails<RECORD_TYPE>& rec);
};

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
: d_ostream(stream)
, d_fields(allocator)
, d_printer_mp()
, d_allocator_p(allocator)
{
    // NOTHING
}

template <typename PRINTER_TYPE>
template <typename RECORD_TYPE>
void RecordDetailsPrinter<PRINTER_TYPE>::printAppInfo(
    const RecordDetails<RECORD_TYPE>& rec)
{
    d_fields.push_back("AppKey");
    bmqu::MemOutStream appKeyStr(d_allocator_p);
    if (rec.d_record.appKey().isNull()) {
        appKeyStr << "** NULL **";
    }
    else {
        appKeyStr << rec.d_record.appKey();
    }
    *d_printer_mp << appKeyStr.str();

    if (!rec.d_appId.empty()) {
        d_fields.push_back("AppId");
        *d_printer_mp << rec.d_appId;
    }
}

template <typename PRINTER_TYPE>
template <typename RECORD_TYPE>
void RecordDetailsPrinter<PRINTER_TYPE>::printQueueInfo(
    const RecordDetails<RECORD_TYPE>& rec)
{
    d_fields.push_back("QueueKey");
    bmqu::MemOutStream queueKeyStr(d_allocator_p);
    queueKeyStr << rec.d_record.queueKey();
    *d_printer_mp << queueKeyStr.str();
    if (!rec.d_queueUri.empty()) {
        d_fields.push_back("QueueUri");
        *d_printer_mp << rec.d_queueUri;
    }
}

template <typename PRINTER_TYPE>
void printDelimeter(bsl::ostream& ostream)
{
    ostream << ",\n";
}

template <>
inline void printDelimeter<bmqu::AlignedPrinter>(bsl::ostream& ostream)
{
    ostream << "\n";
}

template <typename PRINTER_TYPE>
template <typename RECORD_TYPE>
void RecordDetailsPrinter<PRINTER_TYPE>::printRecordDetails(
    const RecordDetails<RECORD_TYPE>& details)
{
    d_fields.clear();
    d_fields.reserve(14);  // max number of fields
    d_fields.push_back("RecordType");
    d_fields.push_back("Index");
    d_fields.push_back("Offset");
    d_fields.push_back("PrimaryLeaseId");
    d_fields.push_back("SequenceNumber");
    d_fields.push_back("Timestamp");
    d_fields.push_back("Epoch");

    // It's ok to pass a vector by pointer and push elements after that as
    // we've reserved it's capacity in advance. Hense, no reallocations will
    // happen and the pointer won't get invalidated.
    d_printer_mp.load(new (*d_allocator_p) PRINTER_TYPE(d_ostream, &d_fields),
                      d_allocator_p);

    *d_printer_mp << details.d_record.header().type() << details.d_recordIndex
                  << details.d_recordOffset
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

    printRecord(details);
    d_printer_mp.reset();
}

template <typename PRINTER_TYPE>
void RecordDetailsPrinter<PRINTER_TYPE>::printRecord(
    const RecordDetails<mqbs::MessageRecord>& rec)
{
    printQueueInfo(rec);

    d_fields.push_back("FileKey");
    d_fields.push_back("RefCount");
    d_fields.push_back("MsgOffsetDwords");
    d_fields.push_back("GUID");
    d_fields.push_back("Crc32c");

    bmqu::MemOutStream fileKeyStr(d_allocator_p);
    fileKeyStr << rec.d_record.fileKey();

    *d_printer_mp << fileKeyStr.str() << rec.d_record.refCount()
                  << rec.d_record.messageOffsetDwords()
                  << rec.d_record.messageGUID() << rec.d_record.crc32c();
}

template <typename PRINTER_TYPE>
void RecordDetailsPrinter<PRINTER_TYPE>::printRecord(
    const RecordDetails<mqbs::ConfirmRecord>& rec)
{
    printQueueInfo(rec);
    printAppInfo(rec);
    d_fields.push_back("GUID");
    *d_printer_mp << rec.d_record.messageGUID();
}

template <typename PRINTER_TYPE>
void RecordDetailsPrinter<PRINTER_TYPE>::printRecord(
    const RecordDetails<mqbs::DeletionRecord>& rec)
{
    printQueueInfo(rec);
    d_fields.push_back("DeletionFlag");
    d_fields.push_back("GUID");
    *d_printer_mp << rec.d_record.deletionRecordFlag()
                  << rec.d_record.messageGUID();
}

template <typename PRINTER_TYPE>
void RecordDetailsPrinter<PRINTER_TYPE>::printRecord(
    const RecordDetails<mqbs::QueueOpRecord>& rec)
{
    printQueueInfo(rec);
    printAppInfo(rec);
    d_fields.push_back("QueueOpType");
    *d_printer_mp << rec.d_record.type();

    if (mqbs::QueueOpType::e_CREATION == rec.d_record.type() ||
        mqbs::QueueOpType::e_ADDITION == rec.d_record.type()) {
        d_fields.push_back("QLIST OffsetWords");
        *d_printer_mp << rec.d_record.queueUriRecordOffsetWords();
    }
}

template <typename PRINTER_TYPE>
void RecordDetailsPrinter<PRINTER_TYPE>::printRecord(
    const RecordDetails<mqbs::JournalOpRecord>& rec)
{
    d_fields.push_back("JournalOpType");
    if (mqbs::JournalOpType::e_SYNCPOINT == rec.d_record.type()) {
        d_fields.push_back("SyncPointType");
        d_fields.push_back("SyncPtPrimaryLeaseId");
        d_fields.push_back("SyncPtSequenceNumber");
        d_fields.push_back("PrimaryNodeId");
        d_fields.push_back("DataFileOffsetDwords");
        const mqbs::JournalOpRecord::SyncPointData& spd = rec.d_record.syncPointData();
        *d_printer_mp << rec.d_record.type() << rec.d_record.syncPointType()
                  << spd.primaryLeaseId()
                  << spd.sequenceNum() << spd.primaryNodeId()
                  << spd.dataFileOffsetDwords();
    } else if (mqbs::JournalOpType::e_UPDATE_STORAGE_SIZE ==
               rec.d_record.type()) {
        d_fields.push_back("MaxJournalFileSize");
        d_fields.push_back("MaxDataFileSize");
        d_fields.push_back("MaxQlistFileSize");
        const mqbs::JournalOpRecord::UpdateStorageSizeData& ussd = rec.d_record.updateStorageSizeData();
        *d_printer_mp << ussd.maxJournalFileSize()
                  << ussd.maxDataFileSize()
                  << ussd.maxQlistFileSize();
    }
}

}  // close namespace RecordPrinter

}  // close package namespace
}  // close enterprise namespace

#endif
