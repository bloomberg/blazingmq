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
#include <m_bmqstoragetool_recordprinter.h>
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

    void printShortResult(const mqbc::ClusterStateRecordHeader& header, const mqbsi::LedgerRecordId& recordId) const BSLS_KEYWORD_OVERRIDE
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

    void printDetailResult(const bmqp_ctrlmsg::ClusterMessage& record, const mqbc::ClusterStateRecordHeader& header, const mqbsi::LedgerRecordId& recordId) const BSLS_KEYWORD_OVERRIDE
    {
        d_ostream << "===================================\n\n";

        RecordPrinter::RecordDetailsPrinter<bmqu::AlignedPrinter> printer(d_ostream, d_allocator_p);
        printer.printCslRecordDetails(record, header, recordId);
    }

    void printFooter(bsls::Types::Uint64 snapshotCount, bsls::Types::Uint64 updateCount, bsls::Types::Uint64 commitCount, bsls::Types::Uint64 ackCount,
        const Parameters::ProcessCslRecordTypes& processCslRecordTypes) const BSLS_KEYWORD_OVERRIDE
    {
        if (processCslRecordTypes.d_snapshot) {
            snapshotCount > 0
                ? (d_ostream << snapshotCount << " snapshot")
                : d_ostream << "No snapshot";
                d_ostream << " record(s) found.\n";
        }
        if (processCslRecordTypes.d_update) {
            updateCount > 0
                ? (d_ostream << updateCount << " update")
                : d_ostream << "No update";
                d_ostream << " record(s) found.\n";
        }
        if (processCslRecordTypes.d_commit) {
            commitCount > 0
                ? (d_ostream << commitCount << " commit")
                : d_ostream << "No commit";
                d_ostream << " record(s) found.\n";
        }
        if (processCslRecordTypes.d_ack) {
            ackCount > 0
                ? (d_ostream << ackCount << " ack")
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
        printer.load(new (*allocator) HumanReadableCslPrinter(stream, allocator),
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

