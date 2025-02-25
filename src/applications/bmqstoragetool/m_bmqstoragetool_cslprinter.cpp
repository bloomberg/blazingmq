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
#include <m_bmqstoragetool_messagedetails.h>
#include <m_bmqstoragetool_parameters.h>
#include <m_bmqstoragetool_recordprinter.h>

// BMQ
#include <bmqu_alignedprinter.h>
#include <bmqu_jsonprinter.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bslim_printer.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

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

        RecordPrinter::RecordDetailsPrinter<bmqu::AlignedPrinter> printer(
            d_ostream,
            d_allocator_p);
        printer.printCslRecordDetails(record, header, recordId);
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
        // TODO: consider to enhance printFooter with optional updateChoiceMap
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
            d_ostream << '\n' << queueInfosSize << " Queues found:" << '\n';
            if (queueInfosSize > queuesLimit) {
                d_ostream << "Only first " << queuesLimit
                          << " queues are displayed." << '\n';
            }
            bsl::vector<bmqp_ctrlmsg::QueueInfo>::const_iterator it =
                queueInfos.cbegin();
            for (; it != queueInfos.cend(); ++it) {
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
    // else if (mode == Parameters::e_JSON_PRETTY) {
    //     printer.load(new (*allocator) JsonPrettyPrinter(stream, allocator),
    //                 allocator);
    // }
    // else if (mode == Parameters::e_JSON_LINE) {
    //     printer.load(new (*allocator) JsonLinePrinter(stream, allocator),
    //                 allocator);
    // }
    return printer;
}

}  // close package namespace
}  // close enterprise namespace
