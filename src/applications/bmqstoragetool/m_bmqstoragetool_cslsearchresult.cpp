// bmqstoragetool
#include <m_bmqstoragetool_cslsearchresult.h>
#include <m_bmqstoragetool_parameters.h>
#include <m_bmqstoragetool_recordprinter.h>

// BMQ
#include <bmqu_memoutstream.h>

// BDE
#include <bslim_printer.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

namespace {
// Helper to print CSL record header
void printClusterStateRecordHeader(
    bsl::ostream&                         ostream,
    const mqbc::ClusterStateRecordHeader& header,
    int                                   level          = 0,
    int                                   spacesPerLevel = -1)
{
    BSLS_ASSERT(ostream.good());

    bslim::Printer printer(&ostream, level, spacesPerLevel);
    printer.start();
    // printer.printAttribute("headerWords", header.headerWords());
    printer.printAttribute("recordType", header.recordType());
    // printer.printAttribute("leaderAdvisoryWords",
    //                     header.leaderAdvisoryWords());
    printer.printAttribute("electorTerm", header.electorTerm());
    printer.printAttribute("sequenceNumber", header.sequenceNumber());
    printer.printAttribute("timestamp", header.timestamp());
    printer.end();
}

// Helper to update record counters
void updateRecordCount(CslSearchResult::RecordCount*      recordCount_p,
                       mqbc::ClusterStateRecordType::Enum recordType)
{
    switch (recordType) {
    case mqbc::ClusterStateRecordType::e_SNAPSHOT:
        recordCount_p->d_snapshotCount++;
        break;
    case mqbc::ClusterStateRecordType::e_UPDATE:
        recordCount_p->d_updateCount++;
        break;
    case mqbc::ClusterStateRecordType::e_COMMIT:
        recordCount_p->d_commitCount++;
        break;
    case mqbc::ClusterStateRecordType::e_ACK:
        recordCount_p->d_ackCount++;
        break;
    default: BSLS_ASSERT(false && "Unknown record type");
    }
}

// Helper to print summary of search result
void outputFooter(
    bsl::ostream&                            ostream,
    const Parameters::ProcessCslRecordTypes& processCslRecordTypes,
    const CslSearchResult::RecordCount&      recordCount)
{
    BSLS_ASSERT(ostream.good());

    if (processCslRecordTypes.d_snapshot) {
        recordCount.d_snapshotCount > 0
            ? (ostream << recordCount.d_snapshotCount << " snapshot")
            : ostream << "No snapshot";
        ostream << " record(s) found.\n";
    }
    if (processCslRecordTypes.d_update) {
        recordCount.d_updateCount > 0
            ? (ostream << recordCount.d_updateCount << " update")
            : ostream << "No update";
        ostream << " record(s) found.\n";
    }
    if (processCslRecordTypes.d_commit) {
        recordCount.d_commitCount > 0
            ? (ostream << recordCount.d_commitCount << " commit")
            : ostream << "No commit";
        ostream << " record(s) found.\n";
    }
    if (processCslRecordTypes.d_ack) {
        recordCount.d_ackCount > 0
            ? (ostream << recordCount.d_ackCount << " ack")
            : ostream << "No ack";
        ostream << " record(s) found.\n";
    }
}

}  // close unnamed namespace

// =====================
// class CslSearchResult
// =====================

CslSearchResult::~CslSearchResult()
{
    // NOTHING
}

// ==================
// struct RecordCount
// ==================

CslSearchResult::RecordCount::RecordCount()
: d_snapshotCount(0)
, d_updateCount(0)
, d_commitCount(0)
, d_ackCount(0)
{
    // NOTHING
}

// ==========================
// class CslSearchShortResult
// ==========================

CslSearchShortResult::CslSearchShortResult(
    bsl::ostream&                            ostream,
    const Parameters::ProcessCslRecordTypes& processCslRecordTypes,
    bslma::Allocator*                        allocator)
: d_ostream(ostream)
, d_processCslRecordTypes(processCslRecordTypes)
, d_recordCount()
, d_allocator_p(allocator)
{
    // NOTHING
}

bool CslSearchShortResult::processRecord(
    const mqbc::ClusterStateRecordHeader& header,
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ClusterMessage& record,
    const mqbsi::LedgerRecordId&                               recordId)
{
    printClusterStateRecordHeader(d_ostream, header);
    d_ostream << recordId << '\n';

    updateRecordCount(&d_recordCount, header.recordType());

    return false;
}

void CslSearchShortResult::outputResult() const
{
    // Print summary counters
    outputFooter(d_ostream, d_processCslRecordTypes, d_recordCount);
}

// ===========================
// class CslSearchDetailResult
// ===========================

CslSearchDetailResult::CslSearchDetailResult(
    bsl::ostream&                            ostream,
    const Parameters::ProcessCslRecordTypes& processCslRecordTypes,
    bslma::Allocator*                        allocator)
: d_ostream(ostream)
, d_processCslRecordTypes(processCslRecordTypes)
, d_recordCount()
, d_allocator_p(allocator)
{
    // NOTHING
}

bool CslSearchDetailResult::processRecord(
    const mqbc::ClusterStateRecordHeader& header,
    const bmqp_ctrlmsg::ClusterMessage&   record,
    const mqbsi::LedgerRecordId&          recordId)
{
    RecordPrinter::printRecord(d_ostream,
                               record,
                               header,
                               recordId,
                               d_allocator_p);

    updateRecordCount(&d_recordCount, header.recordType());

    return false;
}

void CslSearchDetailResult::outputResult() const
{
    // Print summary counters
    outputFooter(d_ostream, d_processCslRecordTypes, d_recordCount);
}

}  // close package namespace
}  // close enterprise namespace
