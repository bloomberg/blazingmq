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
#include <baljsn_encoder.h>
#include <baljsn_encoderoptions.h>
#include <bdlde_base64decoder.h>
#include <bsl_cstring.h>
#include <bsl_iomanip.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_sstream.h>
#include <bsl_vector.h>
#include <bslim_printer.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

namespace {

bsl::string base64ToHex(const bsl::string& base64, bslma::Allocator* allocator)
{
    bdlde::Base64Decoder decoder(true);
    bsl::vector<char>    bin(allocator);
    bin.resize(bdlde::Base64Decoder::maxDecodedLength(
        static_cast<int>(base64.size())));

    int numOut = 0;
    int numIn  = 0;
    int rc     = decoder.convert(bin.data(),
                             &numOut,
                             &numIn,
                             base64.data(),
                             base64.data() + base64.size());
    if (rc < 0) {
        return base64;
    }
    int endOut = 0;
    rc         = decoder.endConvert(bin.data() + numOut, &endOut);
    if (rc < 0) {
        return base64;
    }
    bin.resize(numOut + endOut);

    static const char HEX[] = "0123456789ABCDEF";
    bsl::string       result(allocator);
    result.reserve(bin.size() * 2);
    for (bsl::size_t i = 0; i < bin.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(bin[i]);
        result.push_back(HEX[c >> 4]);
        result.push_back(HEX[c & 0x0F]);
    }
    return result;
}

void convertKeysToHex(bsl::string* json, bslma::Allocator* allocator)
{
    const char* keys[]    = {"\"key\"", "\"appKey\""};
    const int   keyLens[] = {5, 8};

    for (int p = 0; p < 2; ++p) {
        bsl::size_t pos = 0;
        while ((pos = json->find(keys[p], pos)) != bsl::string::npos) {
            bsl::size_t quoteStart = json->find('"', pos + keyLens[p]);
            if (quoteStart == bsl::string::npos) {
                break;
            }
            bsl::size_t valStart = quoteStart + 1;
            bsl::size_t valEnd   = json->find('"', valStart);
            if (valEnd == bsl::string::npos) {
                break;
            }
            bsl::string b64(json->data() + valStart,
                            valEnd - valStart,
                            allocator);
            bsl::string hex = base64ToHex(b64, allocator);
            json->replace(valStart, valEnd - valStart, hex);
            pos = valStart + hex.size() + 1;
        }
    }
}

// Helper to convert record to JSON using baljsn encoder.
// Produces a JSON object string (starts with '{') that the JsonPrinter
// will emit unquoted as a nested object.
template <typename RECORD_TYPE>
bsl::string recordToJsonString(const RECORD_TYPE* record,
                               bslma::Allocator*  allocator,
                               bool               pretty)
{
    bmqu::MemOutStream os(allocator);

    baljsn::Encoder        encoder(allocator);
    baljsn::EncoderOptions options;
    if (pretty) {
        options.setEncodingStyle(baljsn::EncoderOptions::e_PRETTY);
        options.setInitialIndentLevel(3);
        options.setSpacesPerLevel(2);
    }
    else {
        options.setEncodingStyle(baljsn::EncoderOptions::e_COMPACT);
    }

    const int rc = encoder.encode(os, *record, options);
    if (rc != 0) {
        // Encoder failed (e.g., on empty choice message). Fall back to
        // print() method and escape the result as a string. Use bsl::quoted
        // to escape special characters (quotes, backslashes, newlines), then
        // strip the outer quotes added by quoted() to keep just the escaped
        // content.
        bmqu::MemOutStream recStr(allocator);
        record->print(recStr, -1, -1);
        bmqu::MemOutStream escapedStr(allocator);
        escapedStr << bsl::quoted(recStr.str());
        bsl::string resultStr = escapedStr.str();
        if (!resultStr.empty() && resultStr.front() == '"' &&
            resultStr.back() == '"') {
            resultStr = resultStr.substr(1, resultStr.size() - 2);
        }
        return resultStr;
    }

    bsl::string result(os.str(), allocator);
    convertKeysToHex(&result, allocator);
    bsl::size_t start = result.find_first_not_of(" \n");
    if (start != 0 && start != bsl::string::npos) {
        result.erase(0, start);
    }
    return result;
}

// Helper to print queue info in json format
void printQueueInfo(bsl::ostream&     ostream,
                    const QueueMap&   queueMap,
                    unsigned int      queuesLimit,
                    bslma::Allocator* allocator)
{
    const QueueMap::QueueInfos& queueInfos = queueMap.queueInfos();
    if (!queueInfos.empty()) {
        ostream << ",\n";
        QueueMap::QueueInfos::const_iterator itEnd = queueInfos.cend();
        if (queueInfos.size() > queuesLimit) {
            ostream << "    \"First" << queuesLimit << "Queues\": [";
            itEnd = queueInfos.cbegin() + queuesLimit;
        }
        else {
            ostream << "    \"Queues\": [";
        }
        QueueMap::QueueInfos::const_iterator it = queueInfos.cbegin();
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

// ================
// class CslPrinter
// ================
CslPrinter::~CslPrinter()
{
    // NOTHING
}

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
    explicit HumanReadableCslPrinter(bsl::ostream&     os,
                                     bslma::Allocator* allocator = 0);

    ~HumanReadableCslPrinter() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    void printShortResult(const bmqp_ctrlmsg::ClusterMessage&   record,
                          const mqbc::ClusterStateRecordHeader& header,
                          const mqbsi::LedgerRecordId&          recordId) const
        BSLS_KEYWORD_OVERRIDE;

    void printDetailResult(const bmqp_ctrlmsg::ClusterMessage&   record,
                           const mqbc::ClusterStateRecordHeader& header,
                           const mqbsi::LedgerRecordId& recordId) const
        BSLS_KEYWORD_OVERRIDE;

    void printOffsetsNotFound(const OffsetsVec& offsets) const
        BSLS_KEYWORD_OVERRIDE;

    void printCompositesNotFound(const CompositesVec& seqNums) const
        BSLS_KEYWORD_OVERRIDE;

    void printSummaryResult(
        const CslRecordCount&                    recordCount,
        const CslUpdateChoiceMap&                updateChoiceMap,
        const QueueMap&                          queueMap,
        const Parameters::ProcessCslRecordTypes& processCslRecordTypes,
        unsigned int queuesLimit) const BSLS_KEYWORD_OVERRIDE;

    void printFooter(const CslRecordCount& recordCount,
                     const Parameters::ProcessCslRecordTypes&
                         processCslRecordTypes) const BSLS_KEYWORD_OVERRIDE;
};

// CREATORS
HumanReadableCslPrinter::HumanReadableCslPrinter(bsl::ostream&     os,
                                                 bslma::Allocator* allocator)
: d_ostream(os)
, d_allocator_p(allocator)
{
    BSLS_ASSERT(os.good());
}

HumanReadableCslPrinter::~HumanReadableCslPrinter()
{
    d_ostream << '\n';
}

// ACCESSORS
void HumanReadableCslPrinter::printShortResult(
    BSLA_MAYBE_UNUSED const bmqp_ctrlmsg::ClusterMessage& record,
    const mqbc::ClusterStateRecordHeader&                 header,
    const mqbsi::LedgerRecordId&                          recordId) const
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

void HumanReadableCslPrinter::printDetailResult(
    const bmqp_ctrlmsg::ClusterMessage&   record,
    const mqbc::ClusterStateRecordHeader& header,
    const mqbsi::LedgerRecordId&          recordId) const
{
    // Print aligned delimiter
    bsl::string aiignedDelimiter(
        bsl::strlen(
            mqbc::ClusterStateRecordType::toAscii(header.recordType())),
        '=',
        d_allocator_p);
    d_ostream << "===========================" << aiignedDelimiter << "\n\n";

    bmqu::MemOutStream recordStream(d_allocator_p);
    record.print(recordStream, 2, 2);

    CslRecordPrinter<bmqu::AlignedPrinter> printer(d_ostream, d_allocator_p);
    printer.printRecordDetails(recordStream.str(), header, recordId);
}

void HumanReadableCslPrinter::printOffsetsNotFound(
    const OffsetsVec& offsets) const
{
    if (!offsets.empty()) {
        d_ostream << "\nThe following " << offsets.size()
                  << " offset(s) not found:\n";
        for (OffsetsVec::const_iterator it = offsets.cbegin();
             it != offsets.cend();
             ++it) {
            d_ostream << *it << '\n';
        }
    }
}

void HumanReadableCslPrinter::printCompositesNotFound(
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

void HumanReadableCslPrinter::printSummaryResult(
    const CslRecordCount&                    recordCount,
    const CslUpdateChoiceMap&                updateChoiceMap,
    const QueueMap&                          queueMap,
    const Parameters::ProcessCslRecordTypes& processCslRecordTypes,
    unsigned int                             queuesLimit) const
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

void HumanReadableCslPrinter::printFooter(
    const CslRecordCount&                    recordCount,
    const Parameters::ProcessCslRecordTypes& processCslRecordTypes) const
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

    void openBraceIfNotOpen(const std::string& fieldName) const;

    void closeBraceIfOpen() const;

  public:
    // CREATORS

    explicit JsonCslPrinter(bsl::ostream& os, bslma::Allocator* allocator = 0);

    ~JsonCslPrinter() BSLS_KEYWORD_OVERRIDE;

    // PUBLIC METHODS

    void printOffsetsNotFound(const OffsetsVec& offsets) const
        BSLS_KEYWORD_OVERRIDE;

    void printCompositesNotFound(const CompositesVec& seqNums) const
        BSLS_KEYWORD_OVERRIDE;

    void printFooter(const CslRecordCount&   recordCount,
                     BSLA_MAYBE_UNUSED const Parameters::ProcessCslRecordTypes&
                         processCslRecordTypes) const BSLS_KEYWORD_OVERRIDE;
};

// PROTECTED METHODS

void JsonCslPrinter::openBraceIfNotOpen(const std::string& fieldName) const
{
    if (!d_braceOpen) {
        d_ostream << "  \"" << fieldName << "\": [\n";
        d_braceOpen = true;
    }
    else {
        d_ostream << ",\n";
    }
}

void JsonCslPrinter::closeBraceIfOpen() const
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

// CREATORS

JsonCslPrinter::JsonCslPrinter(bsl::ostream& os, bslma::Allocator* allocator)
: d_ostream(os)
, d_allocator_p(allocator)
, d_braceOpen(false)
, d_firstRow(true)
{
    d_ostream << "{\n";
}

JsonCslPrinter::~JsonCslPrinter()
{
    if (d_braceOpen) {
        d_ostream << "\n  ]";
        d_braceOpen = false;
    }
    d_ostream << "\n}\n";
}

// PUBLIC METHODS

void JsonCslPrinter::printOffsetsNotFound(const OffsetsVec& offsets) const
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

void JsonCslPrinter::printCompositesNotFound(
    const CompositesVec& seqNums) const
{
    closeBraceIfOpen();
    d_ostream << "  \"SequenceNumbersNotFound\": [";
    CompositesVec::const_iterator it = seqNums.cbegin();
    for (; it != seqNums.cend(); ++it) {
        if (it != seqNums.cbegin()) {
            d_ostream << ',';
        }

        d_ostream << "\n    {\"leaseId\": " << it->leaseId()
                  << ", \"sequenceNumber\": " << it->sequenceNumber() << "}";
    }
    d_ostream << "\n  ]";
}

void JsonCslPrinter::printFooter(
    const CslRecordCount&   recordCount,
    BSLA_MAYBE_UNUSED const Parameters::ProcessCslRecordTypes&
                            processCslRecordTypes) const
{
    closeBraceIfOpen();
    d_ostream << "  \"SnapshotRecords\": \"" << recordCount.d_snapshotCount
              << "\",\n";
    d_ostream << "  \"UpdateRecords\": \"" << recordCount.d_updateCount
              << "\",\n";
    d_ostream << "  \"CommitRecords\": \"" << recordCount.d_commitCount
              << "\",\n";
    d_ostream << "  \"AckRecords\": \"" << recordCount.d_ackCount << "\"";
}

// ==========================
// class JsonPrettyCslPrinter
// ==========================

class JsonPrettyCslPrinter : public JsonCslPrinter {
  public:
    // CREATORS
    JsonPrettyCslPrinter(bsl::ostream& os, bslma::Allocator* allocator);

    ~JsonPrettyCslPrinter() BSLS_KEYWORD_OVERRIDE;

    // PUBLIC METHODS

    void printShortResult(const bmqp_ctrlmsg::ClusterMessage&   record,
                          const mqbc::ClusterStateRecordHeader& header,
                          const mqbsi::LedgerRecordId&          recordId) const
        BSLS_KEYWORD_OVERRIDE;

    void printDetailResult(const bmqp_ctrlmsg::ClusterMessage&   record,
                           const mqbc::ClusterStateRecordHeader& header,
                           const mqbsi::LedgerRecordId& recordId) const
        BSLS_KEYWORD_OVERRIDE;

    void printSummaryResult(
        const CslRecordCount&     recordCount,
        const CslUpdateChoiceMap& updateChoiceMap,
        const QueueMap&           queueMap,
        BSLA_MAYBE_UNUSED const   Parameters::ProcessCslRecordTypes&
                                  processCslRecordTypes,
        unsigned int              queuesLimit) const BSLS_KEYWORD_OVERRIDE;
};

// CREATORS

JsonPrettyCslPrinter::JsonPrettyCslPrinter(bsl::ostream&     os,
                                           bslma::Allocator* allocator)
: JsonCslPrinter(os, allocator)
{
    // NOTHING
}

JsonPrettyCslPrinter::~JsonPrettyCslPrinter()
{
    // NOTHING
}

// PUBLIC METHODS

void JsonPrettyCslPrinter::printShortResult(
    BSLA_MAYBE_UNUSED const bmqp_ctrlmsg::ClusterMessage& record,
    const mqbc::ClusterStateRecordHeader&                 header,
    const mqbsi::LedgerRecordId&                          recordId) const
{
    openBraceIfNotOpen("Records");

    CslRecordPrinter<bmqu::JsonPrinter<true, true, 4, 6> > printer(
        d_ostream,
        d_allocator_p);
    printer.printRecordDetails("", header, recordId);
}

void JsonPrettyCslPrinter::printDetailResult(
    const bmqp_ctrlmsg::ClusterMessage&   record,
    const mqbc::ClusterStateRecordHeader& header,
    const mqbsi::LedgerRecordId&          recordId) const
{
    openBraceIfNotOpen("Records");

    bsl::string recStr = recordToJsonString(&record, d_allocator_p, true);

    CslRecordPrinter<bmqu::JsonPrinter<true, true, 4, 6> > printer(
        d_ostream,
        d_allocator_p);

    printer.printRecordDetails(recStr, header, recordId);
}

void JsonPrettyCslPrinter::printSummaryResult(
    const CslRecordCount&     recordCount,
    const CslUpdateChoiceMap& updateChoiceMap,
    const QueueMap&           queueMap,
    BSLA_MAYBE_UNUSED const   Parameters::ProcessCslRecordTypes&
                              processCslRecordTypes,
    unsigned int              queuesLimit) const
{
    d_ostream << "    \"Summary\":\n";

    CslRecordPrinter<bmqu::JsonPrinter<true, true, 4, 6> > printer(
        d_ostream,
        d_allocator_p);

    printer.printRecordsSummary(recordCount, updateChoiceMap);

    printQueueInfo(d_ostream, queueMap, queuesLimit, d_allocator_p);
}

// ========================
// class JsonLineCslPrinter
// ========================

class JsonLineCslPrinter : public JsonCslPrinter {
  public:
    // CREATORS
    JsonLineCslPrinter(bsl::ostream& os, bslma::Allocator* allocator);

    ~JsonLineCslPrinter() BSLS_KEYWORD_OVERRIDE;

    // PUBLIC METHODS
    void printShortResult(const bmqp_ctrlmsg::ClusterMessage&   record,
                          const mqbc::ClusterStateRecordHeader& header,
                          const mqbsi::LedgerRecordId&          recordId) const
        BSLS_KEYWORD_OVERRIDE;

    void printDetailResult(const bmqp_ctrlmsg::ClusterMessage&   record,
                           const mqbc::ClusterStateRecordHeader& header,
                           const mqbsi::LedgerRecordId& recordId) const
        BSLS_KEYWORD_OVERRIDE;

    void printSummaryResult(
        const CslRecordCount&     recordCount,
        const CslUpdateChoiceMap& updateChoiceMap,
        const QueueMap&           queueMap,
        BSLA_MAYBE_UNUSED const   Parameters::ProcessCslRecordTypes&
                                  processCslRecordTypes,
        unsigned int              queuesLimit) const BSLS_KEYWORD_OVERRIDE;
};

// CREATORS
JsonLineCslPrinter::JsonLineCslPrinter(bsl::ostream&     os,
                                       bslma::Allocator* allocator)
: JsonCslPrinter(os, allocator)
{
    // NOTHING
}

JsonLineCslPrinter::~JsonLineCslPrinter()
{
}

// PUBLIC METHODS
void JsonLineCslPrinter::printShortResult(
    BSLA_MAYBE_UNUSED const bmqp_ctrlmsg::ClusterMessage& record,
    const mqbc::ClusterStateRecordHeader&                 header,
    const mqbsi::LedgerRecordId&                          recordId) const
{
    openBraceIfNotOpen("Records");

    CslRecordPrinter<bmqu::JsonPrinter<false, true, 4, 6> > printer(
        d_ostream,
        d_allocator_p);
    printer.printRecordDetails("", header, recordId);
}

void JsonLineCslPrinter::printDetailResult(
    const bmqp_ctrlmsg::ClusterMessage&   record,
    const mqbc::ClusterStateRecordHeader& header,
    const mqbsi::LedgerRecordId&          recordId) const
{
    openBraceIfNotOpen("Records");

    bsl::string recStr = recordToJsonString(&record, d_allocator_p, false);

    CslRecordPrinter<bmqu::JsonPrinter<false, true, 4, 6> > printer(
        d_ostream,
        d_allocator_p);

    printer.printRecordDetails(recStr, header, recordId);
}

void JsonLineCslPrinter::printSummaryResult(
    const CslRecordCount&     recordCount,
    const CslUpdateChoiceMap& updateChoiceMap,
    const QueueMap&           queueMap,
    BSLA_MAYBE_UNUSED const   Parameters::ProcessCslRecordTypes&
                              processCslRecordTypes,
    unsigned int              queuesLimit) const
{
    d_ostream << "    \"Summary\": ";

    CslRecordPrinter<bmqu::JsonPrinter<false, true, 0, 6> > printer(
        d_ostream,
        d_allocator_p);

    printer.printRecordsSummary(recordCount, updateChoiceMap);

    printQueueInfo(d_ostream, queueMap, queuesLimit, d_allocator_p);
}

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
    else {
        BSLS_ASSERT(false && "Unknown printer mode");
    }
    return printer;
}

}  // close package namespace
}  // close enterprise namespace
