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

#include <m_bmqstoragetool_cslprinter.h>

// bmqstoragetool
#include <m_bmqstoragetool_parameters.h>

// BMQ
#include <bmqu_alignedprinter.h>
#include <bmqu_jsonprinter.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bslim_printer.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

namespace {

// Helper to convert record to json string (escaped single line)
template <typename RECORD_TYPE>
bsl::string recordToJsonString(const RECORD_TYPE* record,
                               bslma::Allocator*  allocator,
                               bool               removeTrailingQuotes = true)
{
    bmqu::MemOutStream recStr(allocator);
    // Print record in one line string
    record->print(recStr, -1, -1);

    // Escape the string
    bmqu::MemOutStream escapedStr(allocator);
    escapedStr << bsl::quoted(recStr.str());

    bsl::string resultStr = escapedStr.str();
    // Remove leading and trailing quotes with '\n' character
    if (removeTrailingQuotes && !resultStr.empty() &&
        resultStr.front() == '"' && resultStr.back() == '"') {
        resultStr = resultStr.substr(1, resultStr.size() - 3);
    }

    return resultStr;
}

// Helper to print queue info in json format
void printQueueInfo(bsl::ostream&     ostream,
                    const QueueMap&   queueMap,
                    unsigned int      queuesLimit,
                    bslma::Allocator* allocator)
{
    const bsl::vector<bmqp_ctrlmsg::QueueInfo>& queueInfos =
        queueMap.queueInfos();
    if (!queueInfos.empty()) {
        ostream << ",\n";
        bsl::vector<bmqp_ctrlmsg::QueueInfo>::const_iterator itEnd =
            queueInfos.cend();
        if (queueInfos.size() > queuesLimit) {
            ostream << "    \"First" << queuesLimit << "Queues\": [";
            itEnd = queueInfos.cbegin() + queuesLimit;
        }
        else {
            ostream << "    \"Queues\": [";
        }
        bsl::vector<bmqp_ctrlmsg::QueueInfo>::const_iterator it =
            queueInfos.cbegin();
        for (; it != itEnd; ++it) {
            if (it != queueInfos.cbegin()) {
                ostream << ",";
            }
            bsl::string recStr = recordToJsonString(it, allocator, false);

            ostream << "\n      " << recStr;
        }
        ostream << "\n    ]";
    }
    else {
        ostream << "    \"Queues\": []";
    }
}

}  // close unnamed namespace

// =============================
// class HumanReadableCslPrinter
// =============================

class HumanReadableCslPrinter : public CslPrinter {
  private:
    // PRIVATE DATA

    bsl::ostream&     d_ostream;
    bslma::Allocator* d_allocator_p;

  public:
    // CREATORS
    HumanReadableCslPrinter(bsl::ostream& os, bslma::Allocator* allocator)
    : d_ostream(os)
    , d_allocator_p(allocator)
    {
        BSLS_ASSERT(os.good());
    }

    ~HumanReadableCslPrinter() BSLS_KEYWORD_OVERRIDE { d_ostream << '\n'; }

    // ACCESSORS

    void printShortResult(const mqbc::ClusterStateRecordHeader& header,
                          const mqbsi::LedgerRecordId&          recordId) const
        BSLS_KEYWORD_OVERRIDE
    {
        bslim::Printer printer(&d_ostream, 0, -1);
        printer.start();
        printer.printAttribute("recordType", header.recordType());
        printer.printAttribute("electorTerm", header.electorTerm());
        printer.printAttribute("sequenceNumber", header.sequenceNumber());
        printer.printAttribute("timestamp", header.timestamp());
        printer.end();

        d_ostream << recordId << '\n';
    }

    void printDetailResult(const bmqp_ctrlmsg::ClusterMessage&   record,
                           const mqbc::ClusterStateRecordHeader& header,
                           const mqbsi::LedgerRecordId& recordId) const
        BSLS_KEYWORD_OVERRIDE
    {
        // Print aligned delimiter
        bsl::string aiignedDelimiter(
            bsl::strlen(
                mqbc::ClusterStateRecordType::toAscii(header.recordType())),
            '=',
            d_allocator_p);
        d_ostream << "===========================" << aiignedDelimiter
                  << "\n\n";

        bmqu::MemOutStream recordStream(d_allocator_p);
        record.print(recordStream, 2, 2);

        CslRecordPrinter<bmqu::AlignedPrinter> printer(d_ostream,
                                                       d_allocator_p);
        printer.printRecordDetails(recordStream.str(), header, recordId);

        // d_ostream << '\n';
    }

    void
    printOffsetsNotFound(const OffsetsVec& offsets) const BSLS_KEYWORD_OVERRIDE
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

    void printCompositesNotFound(const CompositesVec& seqNums) const
        BSLS_KEYWORD_OVERRIDE
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

    void printSummaryResult(
        const CslRecordCount&                    recordCount,
        const CslUpdateChoiceMap&                updateChoiceMap,
        const QueueMap&                          queueMap,
        const Parameters::ProcessCslRecordTypes& processCslRecordTypes,
        unsigned int queuesLimit) const BSLS_KEYWORD_OVERRIDE
    {
        if (processCslRecordTypes.d_snapshot) {
            recordCount.d_snapshotCount > 0
                ? (d_ostream << '\n'
                             << recordCount.d_snapshotCount << " snapshot")
                : d_ostream << "\nNo snapshot";
            d_ostream << " record(s) found.\n";
        }
        if (processCslRecordTypes.d_update) {
            if (recordCount.d_updateCount > 0) {
                d_ostream << '\n'
                          << recordCount.d_updateCount
                          << " update record(s) found, including:" << '\n';
                bsl::vector<const char*>           fields(d_allocator_p);
                bmqp_ctrlmsg::ClusterMessageChoice clusterMessageChoice(
                    d_allocator_p);
                for (CslUpdateChoiceMap::const_iterator it =
                         updateChoiceMap.begin();
                     it != updateChoiceMap.end();
                     ++it) {
                    clusterMessageChoice.makeSelection(it->first);
                    fields.push_back(clusterMessageChoice.selectionName());
                }
                bmqu::AlignedPrinter printer(d_ostream, &fields);
                for (CslUpdateChoiceMap::const_iterator it =
                         updateChoiceMap.begin();
                     it != updateChoiceMap.end();
                     ++it) {
                    printer << it->second;
                }
            }
            else {
                d_ostream << "\nNo update record(s) found." << '\n';
            }
        }
        if (processCslRecordTypes.d_commit) {
            recordCount.d_commitCount > 0
                ? (d_ostream << '\n'
                             << recordCount.d_commitCount << " commit")
                : d_ostream << "\nNo commit";
            d_ostream << " record(s) found.\n";
        }
        if (processCslRecordTypes.d_ack) {
            recordCount.d_ackCount > 0
                ? (d_ostream << '\n'
                             << recordCount.d_ackCount << " ack")
                : d_ostream << "\nNo ack";
            d_ostream << " record(s) found.\n";
        }

        // Print queues info
        const bsl::vector<bmqp_ctrlmsg::QueueInfo>& queueInfos =
            queueMap.queueInfos();
        if (!queueInfos.empty()) {
            size_t queueInfosSize = queueInfos.size();
            d_ostream << '\n' << queueInfosSize << " Queues found:\n";
            bsl::vector<bmqp_ctrlmsg::QueueInfo>::const_iterator itEnd =
                queueInfos.cend();
            if (queueInfosSize > queuesLimit) {
                d_ostream << "Only first " << queuesLimit
                          << " queues are displayed.\n";
                itEnd = queueInfos.cbegin() + queuesLimit;
            }
            bsl::vector<bmqp_ctrlmsg::QueueInfo>::const_iterator it =
                queueInfos.cbegin();
            for (; it != itEnd; ++it) {
                d_ostream << *it << '\n';
            }
        }
    }

    void printFooter(const CslRecordCount& recordCount,
                     const Parameters::ProcessCslRecordTypes&
                         processCslRecordTypes) const BSLS_KEYWORD_OVERRIDE
    {
        if (processCslRecordTypes.d_snapshot) {
            recordCount.d_snapshotCount > 0
                ? (d_ostream << recordCount.d_snapshotCount << " snapshot")
                : d_ostream << "No snapshot";
            d_ostream << " record(s) found.\n";
        }
        if (processCslRecordTypes.d_update) {
            recordCount.d_updateCount > 0
                ? (d_ostream << recordCount.d_updateCount << " update")
                : d_ostream << "No update";
            d_ostream << " record(s) found.\n";
        }
        if (processCslRecordTypes.d_commit) {
            recordCount.d_commitCount > 0
                ? (d_ostream << recordCount.d_commitCount << " commit")
                : d_ostream << "No commit";
            d_ostream << " record(s) found.\n";
        }
        if (processCslRecordTypes.d_ack) {
            recordCount.d_ackCount > 0
                ? (d_ostream << recordCount.d_ackCount << " ack")
                : d_ostream << "No ack";
            d_ostream << " record(s) found.\n";
        }
    }
};

// ====================
// class JsonCslPrinter
// ====================

class JsonCslPrinter : public CslPrinter {
  protected:
    // PROTECTED DATA

    bsl::ostream&     d_ostream;
    bslma::Allocator* d_allocator_p;
    mutable bool      d_braceOpen;
    mutable bool      d_firstRow;

    // PROTECTED METHODS

    void openBraceIfNotOpen(const std::string& fieldName) const
    {
        if (!d_braceOpen) {
            d_ostream << "  \"" << fieldName << "\": [\n";
            d_braceOpen = true;
        }
        else {
            d_ostream << ",\n";
        }
    }

    void closeBraceIfOpen() const
    {
        if (d_braceOpen) {
            d_ostream << "\n  ]";
            d_braceOpen = false;
            d_firstRow  = false;
        }
        if (!d_firstRow) {
            d_ostream << ",\n";
        }
        else {
            d_firstRow = false;
        }
    }

  public:
    // CREATORS

    JsonCslPrinter(bsl::ostream& os, bslma::Allocator* allocator)
    : d_ostream(os)
    , d_allocator_p(allocator)
    , d_braceOpen(false)
    , d_firstRow(true)
    {
        d_ostream << "{\n";
    }

    ~JsonCslPrinter() BSLS_KEYWORD_OVERRIDE
    {
        if (d_braceOpen) {
            d_ostream << "\n  ]";
            d_braceOpen = false;
        }
        d_ostream << "\n}\n";
    }

    // PUBLIC METHODS

    void
    printOffsetsNotFound(const OffsetsVec& offsets) const BSLS_KEYWORD_OVERRIDE
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

    void printCompositesNotFound(const CompositesVec& seqNums) const
        BSLS_KEYWORD_OVERRIDE
    {
        closeBraceIfOpen();
        d_ostream << "  \"SequenceNumbersNotFound\": [";
        CompositesVec::const_iterator it = seqNums.cbegin();
        for (; it != seqNums.cend(); ++it) {
            if (it != seqNums.cbegin()) {
                d_ostream << ',';
            }

            d_ostream << "\n    {\"leaseId\": " << it->leaseId()
                      << ", \"sequenceNumber\": " << it->sequenceNumber()
                      << "}";
        }
        d_ostream << "\n  ]";
    }

    void
    printFooter(const CslRecordCount&        recordCount,
                BSLS_ANNOTATION_UNUSED const Parameters::ProcessCslRecordTypes&
                    processCslRecordTypes) const BSLS_KEYWORD_OVERRIDE
    {
        closeBraceIfOpen();
        d_ostream << "  \"SnapshotRecords\": " << recordCount.d_snapshotCount
                  << ",\n";
        d_ostream << "  \"UpdateRecords\": " << recordCount.d_updateCount
                  << ",\n";
        d_ostream << "  \"CommitRecords\": " << recordCount.d_commitCount
                  << ",\n";
        d_ostream << "  \"AckRecords\": " << recordCount.d_ackCount;
    }
};

// ==========================
// class JsonPrettyCslPrinter
// ==========================

class JsonPrettyCslPrinter : public JsonCslPrinter {
  public:
    // CREATORS
    JsonPrettyCslPrinter(bsl::ostream& os, bslma::Allocator* allocator)
    : JsonCslPrinter(os, allocator)
    {
        // NOTHING
    }

    ~JsonPrettyCslPrinter() BSLS_KEYWORD_OVERRIDE {}

    // PUBLIC METHODS

    void printShortResult(const mqbc::ClusterStateRecordHeader& header,
                          const mqbsi::LedgerRecordId&          recordId) const
        BSLS_KEYWORD_OVERRIDE
    {
        openBraceIfNotOpen("Records");

        CslRecordPrinter<bmqu::JsonPrinter<true, true, 4, 6> > printer(
            d_ostream,
            d_allocator_p);
        printer.printRecordDetails("", header, recordId);
    }

    void printDetailResult(const bmqp_ctrlmsg::ClusterMessage&   record,
                           const mqbc::ClusterStateRecordHeader& header,
                           const mqbsi::LedgerRecordId& recordId) const
        BSLS_KEYWORD_OVERRIDE
    {
        openBraceIfNotOpen("Records");

        // Print record.
        // Since `record` uses `bslim::Printer` to print its objects hierarchy,
        // it is not easy (and error prone) to do the same for json printer
        // without changing nested `record` objects.
        // So, we will use the output of `print` method and store it in json as
        // escaped string.
        bsl::string recStr = recordToJsonString(&record, d_allocator_p);

        CslRecordPrinter<bmqu::JsonPrinter<true, true, 4, 6> > printer(
            d_ostream,
            d_allocator_p);

        printer.printRecordDetails(recStr, header, recordId);
    }

    void printSummaryResult(
        const CslRecordCount&        recordCount,
        const CslUpdateChoiceMap&    updateChoiceMap,
        const QueueMap&              queueMap,
        BSLS_ANNOTATION_UNUSED const Parameters::ProcessCslRecordTypes&
                                     processCslRecordTypes,
        unsigned int                 queuesLimit) const BSLS_KEYWORD_OVERRIDE
    {
        d_ostream << "    \"Summary\":\n";

        CslRecordPrinter<bmqu::JsonPrinter<true, true, 4, 6> > printer(
            d_ostream,
            d_allocator_p);

        printer.printRecordsSummary(recordCount, updateChoiceMap);

        printQueueInfo(d_ostream, queueMap, queuesLimit, d_allocator_p);
    }
};

// ========================
// class JsonLineCslPrinter
// ========================

class JsonLineCslPrinter : public JsonCslPrinter {
  public:
    // CREATORS
    JsonLineCslPrinter(bsl::ostream& os, bslma::Allocator* allocator)
    : JsonCslPrinter(os, allocator)
    {
        // NOTHING
    }

    ~JsonLineCslPrinter() BSLS_KEYWORD_OVERRIDE {}

    // PUBLIC METHODS
    void printShortResult(const mqbc::ClusterStateRecordHeader& header,
                          const mqbsi::LedgerRecordId&          recordId) const
        BSLS_KEYWORD_OVERRIDE
    {
        openBraceIfNotOpen("Records");

        CslRecordPrinter<bmqu::JsonPrinter<false, true, 4, 6> > printer(
            d_ostream,
            d_allocator_p);
        printer.printRecordDetails("", header, recordId);
    }

    void printDetailResult(const bmqp_ctrlmsg::ClusterMessage&   record,
                           const mqbc::ClusterStateRecordHeader& header,
                           const mqbsi::LedgerRecordId& recordId) const
        BSLS_KEYWORD_OVERRIDE
    {
        openBraceIfNotOpen("Records");

        // Print record.
        // Since `record` uses `bslim::Printer` to print its objects hierarchy,
        // it is not easy (and error prone) to do the same for json printer
        // without changing nested `record` objects.
        // So, we will use the output of `print` method and store it in json as
        // escaped string.
        bsl::string recStr = recordToJsonString(&record, d_allocator_p);

        CslRecordPrinter<bmqu::JsonPrinter<false, true, 4, 6> > printer(
            d_ostream,
            d_allocator_p);

        printer.printRecordDetails(recStr, header, recordId);
    }

    void printSummaryResult(
        const CslRecordCount&        recordCount,
        const CslUpdateChoiceMap&    updateChoiceMap,
        const QueueMap&              queueMap,
        BSLS_ANNOTATION_UNUSED const Parameters::ProcessCslRecordTypes&
                                     processCslRecordTypes,
        unsigned int                 queuesLimit) const BSLS_KEYWORD_OVERRIDE
    {
        d_ostream << "    \"Summary\": ";

        CslRecordPrinter<bmqu::JsonPrinter<false, true, 0, 6> > printer(
            d_ostream,
            d_allocator_p);

        printer.printRecordsSummary(recordCount, updateChoiceMap);

        printQueueInfo(d_ostream, queueMap, queuesLimit, d_allocator_p);
    }
};

bsl::shared_ptr<CslPrinter> createCslPrinter(Parameters::PrintMode mode,
                                             std::ostream&         stream,
                                             bslma::Allocator*     allocator)
{
    bsl::shared_ptr<CslPrinter> printer;
    if (mode == Parameters::e_HUMAN) {
        printer.load(new (*allocator)
                         HumanReadableCslPrinter(stream, allocator),
                     allocator);
    }
    else if (mode == Parameters::e_JSON_PRETTY) {
        printer.load(new (*allocator) JsonPrettyCslPrinter(stream, allocator),
                     allocator);
    }
    else if (mode == Parameters::e_JSON_LINE) {
        printer.load(new (*allocator) JsonLineCslPrinter(stream, allocator),
                     allocator);
    }
    return printer;
}

}  // close package namespace
}  // close enterprise namespace
