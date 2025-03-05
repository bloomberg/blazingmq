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

// bmqstoragetool
#include <m_bmqstoragetool_cslprinter.h>
#include <m_bmqstoragetool_recordprinter.h>

// BMQ
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbsi_ledger.h>

// BDE
#include <bdljsn_jsonutil.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace m_bmqstoragetool;
// using namespace bsl;
// using namespace mqbs;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_humanReadableShortResultTest()
// ------------------------------------------------------------------------
// HUMAN READABLE SHORT RESULT TEST
//
// Concerns:
//   Exercise the output of the HumanReadablePrinter::printShortResult().
//
// Testing:
//   HumanReadablePrinter::printShortResult
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HUMAN SHORT RESULT TEST");

    // Create human printer
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<CslPrinter> printer = createCslPrinter(
        Parameters::PrintMode::e_HUMAN,
        resultStream,
        bmqtst::TestHelperUtil::allocator());

    // Create record data
    mqbc::ClusterStateRecordHeader header;
    header.setHeaderWords(8)
        .setRecordType(mqbc::ClusterStateRecordType::e_SNAPSHOT)
        .setLeaderAdvisoryWords(10)
        .setElectorTerm(1)
        .setSequenceNumber(2)
        .setTimestamp(123456789);
    mqbsi::LedgerRecordId recordId;
    mqbu::StorageKey      storageKey(1);
    recordId.setLogId(storageKey).setOffset(2);

    // Print short result
    printer->printShortResult(header, recordId);

    // Prepare expected output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    {
        bslim::Printer printer(&expectedStream, 0, -1);
        printer.start();
        printer.printAttribute("recordType", header.recordType());
        printer.printAttribute("electorTerm", header.electorTerm());
        printer.printAttribute("sequenceNumber", header.sequenceNumber());
        printer.printAttribute("timestamp", header.timestamp());
        printer.end();
        expectedStream << recordId << '\n';
    }

    BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
}

static void test2_humanReadableDetailResultTest()
// ------------------------------------------------------------------------
// HUMAN READABLE DETAIL RESULT TEST
//
// Concerns:
//   Exercise the output of the HumanReadablePrinter::printDetailResult().
//
// Testing:
//   HumanReadablePrinter::printDetailResult
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HUMAN DETAIL RESULT TEST");

    // Ignore check default allocator due to issue with bslim::Printer::print()
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    // Create human printer
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<CslPrinter> printer = createCslPrinter(
        Parameters::PrintMode::e_HUMAN,
        resultStream,
        bmqtst::TestHelperUtil::allocator());

    // Create record data
    bmqp_ctrlmsg::ClusterMessage   record;
    mqbc::ClusterStateRecordHeader header;
    header.setHeaderWords(8)
        .setRecordType(mqbc::ClusterStateRecordType::e_SNAPSHOT)
        .setLeaderAdvisoryWords(10)
        .setElectorTerm(1)
        .setSequenceNumber(2)
        .setTimestamp(123456789);
    mqbsi::LedgerRecordId recordId;
    mqbu::StorageKey      storageKey(1);
    recordId.setLogId(storageKey).setOffset(2);

    // Print detail result
    printer->printDetailResult(record, header, recordId);

    // Prepare expected output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    {
        expectedStream << "===================================" << "\n\n";

        bmqu::MemOutStream recordStream(bmqtst::TestHelperUtil::allocator());
        record.print(recordStream, 2, 2);

        CslRecordPrinter<bmqu::AlignedPrinter> printer(
            expectedStream,
            bmqtst::TestHelperUtil::allocator());
        printer.printRecordDetails(recordStream.str(), header, recordId);
    }

    BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
}

static void test3_humanReadableNotFoundTest()
// ------------------------------------------------------------------------
// HUMAN READABLE NOT FOUND TEST
//
// Concerns:
//   Exercise the output of the printOffsetsNotFound() and
//   printCompositesNotFound().
//
// Testing:
//   HumanReadablePrinter::printOffsetsNotFound
//   HumanReadablePrinter::printCompositesNotFound
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HUMAN NOT FOUND TEST");

    // Create human printer
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<CslPrinter> printer = createCslPrinter(
        Parameters::PrintMode::e_HUMAN,
        resultStream,
        bmqtst::TestHelperUtil::allocator());
    {
        // Print offsets not found
        bsl::vector<bsls::Types::Int64> offsets(
            bmqtst::TestHelperUtil::allocator());
        offsets.push_back(1);
        offsets.push_back(2);

        printer->printOffsetsNotFound(offsets);

        // Prepare expected output
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        expectedStream << "\nThe following " << offsets.size()
                       << " offset(s) not found:\n";
        bsl::vector<bsls::Types::Int64>::const_iterator it = offsets.cbegin();
        for (; it != offsets.cend(); ++it) {
            expectedStream << *it << '\n';
        }

        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
    {
        // Print composites not found
        resultStream.reset();
        bsl::vector<CompositeSequenceNumber> seqNums(
            bmqtst::TestHelperUtil::allocator());
        seqNums.push_back(CompositeSequenceNumber(1, 2));
        seqNums.push_back(CompositeSequenceNumber(3, 4));

        printer->printCompositesNotFound(seqNums);

        // Prepare expected output
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        expectedStream << "\nThe following " << seqNums.size()
                       << " sequence number(s) not found:\n";
        bsl::vector<CompositeSequenceNumber>::const_iterator it =
            seqNums.cbegin();
        for (; it != seqNums.cend(); ++it) {
            expectedStream << *it << '\n';
        }

        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

static void test4_humanReadableFooterTest()
// ------------------------------------------------------------------------
// HUMAN READABLE FOOTER TEST
//
// Concerns:
//   Exercise the output of the HumanReadablePrinter::printFooter().
//
// Testing:
//   HumanReadablePrinter::printFooter
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HUMAN FOOTER TEST");

    // Create human printer
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<CslPrinter> printer = createCslPrinter(
        Parameters::PrintMode::e_HUMAN,
        resultStream,
        bmqtst::TestHelperUtil::allocator());

    // Create record data
    CslRecordCount recordCount;
    recordCount.d_snapshotCount = 2;
    recordCount.d_updateCount   = 3;
    recordCount.d_commitCount   = 4;
    recordCount.d_ackCount      = 5;

    Parameters::ProcessCslRecordTypes processCslRecordTypes;
    processCslRecordTypes.d_snapshot = true;
    processCslRecordTypes.d_update   = true;
    processCslRecordTypes.d_commit   = true;
    processCslRecordTypes.d_ack      = true;

    // Print footer
    printer->printFooter(recordCount, processCslRecordTypes);

    // Prepare expected output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    expectedStream << recordCount.d_snapshotCount
                   << " snapshot record(s) found.\n";
    expectedStream << recordCount.d_updateCount
                   << " update record(s) found.\n";
    expectedStream << recordCount.d_commitCount
                   << " commit record(s) found.\n";
    expectedStream << recordCount.d_ackCount << " ack record(s) found.\n";

    BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
}

static void test5_humanReadableSummaryTest()
// ------------------------------------------------------------------------
// HUMAN READABLE SUMMARY TEST
//
// Concerns:
//   Exercise the output of the HumanReadablePrinter::printSummaryResult().
//
// Testing:
//   HumanReadablePrinter::printSummaryResult
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HUMAN SUMMARY TEST");

    // Create human printer
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<CslPrinter> printer = createCslPrinter(
        Parameters::PrintMode::e_HUMAN,
        resultStream,
        bmqtst::TestHelperUtil::allocator());

    // Create record data
    CslRecordCount recordCount;
    recordCount.d_snapshotCount = 2;
    recordCount.d_updateCount   = 3;
    recordCount.d_commitCount   = 4;
    recordCount.d_ackCount      = 5;

    Parameters::ProcessCslRecordTypes processCslRecordTypes;
    processCslRecordTypes.d_snapshot = true;
    processCslRecordTypes.d_update   = true;
    processCslRecordTypes.d_commit   = true;
    processCslRecordTypes.d_ack      = true;

    CslUpdateChoiceMap updateChoiceMap(bmqtst::TestHelperUtil::allocator());
    updateChoiceMap[1] = 2;
    updateChoiceMap[2] = 3;
    QueueMap                queueMap(bmqtst::TestHelperUtil::allocator());
    bmqp_ctrlmsg::QueueInfo queueInfo(bmqtst::TestHelperUtil::allocator());
    queueInfo.uri()         = "bmq://bmq.test.persistent.priority/first-queue";
    queueInfo.partitionId() = 2;
    queueInfo.key().push_back('a');
    queueMap.insert(queueInfo);
    queueInfo.uri() = "bmq://bmq.test.persistent.priority/second-queue";
    queueInfo.partitionId() = 3;
    queueInfo.key().clear();
    queueInfo.key().push_back('b');
    queueMap.insert(queueInfo);

    // Print summary
    printer->printSummaryResult(recordCount,
                                updateChoiceMap,
                                queueMap,
                                processCslRecordTypes,
                                10);

    // Prepare expected output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    expectedStream
        << "\n2 snapshot record(s) found.\n\n"
        << "3 update record(s) found, including:\n"
        << "    queueAssignmentAdvisory  : 3\n"
        << "    leaderAdvisory           : 2\n\n"
        << "4 commit record(s) found.\n\n"
        << "5 ack record(s) found.\n\n"
        << "2 Queues found:\n"
        << "[ uri = \"bmq://bmq.test.persistent.priority/second-queue\" key = "
           "[ 62 ]\n"
        << " partitionId = 3 appIds = [ ] ]\n"
        << "[ uri = \"bmq://bmq.test.persistent.priority/first-queue\" key = "
           "[ 61 ]\n"
        << " partitionId = 2 appIds = [ ] ]\n";

    BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
}

static void test6_prettyShortResultTest()
// ------------------------------------------------------------------------
// JSON PRETTY SHORT RESULT TEST
//
// Concerns:
//   Exercise the output of the JsonPrettyCslPrinter::printShortResult().
//
// Testing:
//   JsonPrettyCslPrinter::printShortResult
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PRETTY SHORT TEST");

    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

    // Create record data
    mqbc::ClusterStateRecordHeader header;
    header.setHeaderWords(8)
        .setRecordType(mqbc::ClusterStateRecordType::e_SNAPSHOT)
        .setLeaderAdvisoryWords(10)
        .setElectorTerm(1)
        .setSequenceNumber(2)
        .setTimestamp(123456789);
    mqbsi::LedgerRecordId recordId;
    mqbu::StorageKey      storageKey(1);
    recordId.setLogId(storageKey).setOffset(2);

    {
        // Create printer
        bsl::shared_ptr<CslPrinter> printer = createCslPrinter(
            Parameters::PrintMode::e_JSON_PRETTY,
            resultStream,
            bmqtst::TestHelperUtil::allocator());

        // Print short result
        printer->printShortResult(header, recordId);
    }

    // Prepare expected output
    expectedStream << "{\n"
                   << "  \"Records\": [\n"
                   << "    {\n"
                   << "      \"RecordType\": \"SNAPSHOT\",\n"
                   << "      \"Offset\": \"2\",\n"
                   << "      \"LogId\": \"3000000001\",\n"
                   << "      \"ElectorTerm\": \"1\",\n"
                   << "      \"SequenceNumber\": \"2\",\n"
                   << "      \"HeaderWords\": \"8\",\n"
                   << "      \"LeaderAdvisoryWords\": \"10\",\n"
                   << "      \"Timestamp\": \"29NOV1973_21:33:09.000000\",\n"
                   << "      \"Epoch\": \"123456789\"\n"
                   << "    }\n"
                   << "  ]\n"
                   << "}\n";

    bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
    bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
    const int rc = bdljsn::JsonUtil::read(&json, &error, resultStream.str());
    BMQTST_ASSERT_D(error, (rc == 0));
    BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
}

static void test7_prettyDetailResultTest()
// ------------------------------------------------------------------------
// JSON PRETTY DETAIL RESULT TEST
//
// Concerns:
//   Exercise the output of the JsonPrettyCslPrinter::printDetailResult().
//
// Testing:
//   JsonPrettyCslPrinter::printDetailResult
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PRETTY SHORT TEST");

    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

    // Ignore check default allocator due to issue with bslim::Printer::print()
    // used in recordToJsonString()
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    // Create record data
    bmqp_ctrlmsg::ClusterMessage record(bmqtst::TestHelperUtil::allocator());
    record.choice().makeLeaderAdvisory();

    mqbc::ClusterStateRecordHeader header;
    header.setHeaderWords(8)
        .setRecordType(mqbc::ClusterStateRecordType::e_SNAPSHOT)
        .setLeaderAdvisoryWords(10)
        .setElectorTerm(1)
        .setSequenceNumber(2)
        .setTimestamp(123456789);

    mqbsi::LedgerRecordId recordId;
    mqbu::StorageKey      storageKey(1);
    recordId.setLogId(storageKey).setOffset(2);

    {
        // Create printer
        bsl::shared_ptr<CslPrinter> printer = createCslPrinter(
            Parameters::PrintMode::e_JSON_PRETTY,
            resultStream,
            bmqtst::TestHelperUtil::allocator());

        // Print detail result
        printer->printDetailResult(record, header, recordId);
    }

    // Prepare expected output
    expectedStream << "{\n"
                   << "  \"Records\": [\n"
                   << "    {\n"
                   << "      \"RecordType\": \"SNAPSHOT\",\n"
                   << "      \"Offset\": \"2\",\n"
                   << "      \"LogId\": \"3000000001\",\n"
                   << "      \"ElectorTerm\": \"1\",\n"
                   << "      \"SequenceNumber\": \"2\",\n"
                   << "      \"HeaderWords\": \"8\",\n"
                   << "      \"LeaderAdvisoryWords\": \"10\",\n"
                   << "      \"Timestamp\": \"29NOV1973_21:33:09.000000\",\n"
                   << "      \"Epoch\": \"123456789\",\n"
                   << "      \"Record\": \"[ choice = [ leaderAdvisory = [ "
                      "sequenceNumber = [ electorTerm = 0 sequenceNumber = 0 "
                      "] partitions = [ ] queues = [ ] ] ] \"\n"
                   << "    }\n"
                   << "  ]\n"
                   << "}\n";

    bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
    bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
    const int rc = bdljsn::JsonUtil::read(&json, &error, resultStream.str());
    BMQTST_ASSERT_D(error, (rc == 0));
    BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
}

static void test8_prettySummaryTest()
// ------------------------------------------------------------------------
// JSON PRETTY SUMMARY TEST
//
// Concerns:
//   Exercise the output of the JsonPrettyCslPrinter::printSummaryResult().
//
// Testing:
//   JsonPrettyCslPrinter::printSummaryResult
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PRETTY SUMMARY TEST");

    // Ignore check default allocator due to issue with bslim::Printer::print()
    // used in recordToJsonString()
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());

    // Create record data
    CslRecordCount recordCount;
    recordCount.d_snapshotCount = 2;
    recordCount.d_updateCount   = 3;
    recordCount.d_commitCount   = 4;
    recordCount.d_ackCount      = 5;

    Parameters::ProcessCslRecordTypes processCslRecordTypes;
    processCslRecordTypes.d_snapshot = true;
    processCslRecordTypes.d_update   = true;
    processCslRecordTypes.d_commit   = true;
    processCslRecordTypes.d_ack      = true;

    CslUpdateChoiceMap updateChoiceMap(bmqtst::TestHelperUtil::allocator());
    updateChoiceMap[1] = 2;
    updateChoiceMap[2] = 3;
    QueueMap                queueMap(bmqtst::TestHelperUtil::allocator());
    bmqp_ctrlmsg::QueueInfo queueInfo(bmqtst::TestHelperUtil::allocator());
    queueInfo.uri()         = "bmq://bmq.test.persistent.priority/first-queue";
    queueInfo.partitionId() = 2;
    queueInfo.key().push_back('a');
    queueMap.insert(queueInfo);
    queueInfo.uri() = "bmq://bmq.test.persistent.priority/second-queue";
    queueInfo.partitionId() = 3;
    queueInfo.key().clear();
    queueInfo.key().push_back('b');
    queueMap.insert(queueInfo);

    {
        // Create printer
        bsl::shared_ptr<CslPrinter> printer = createCslPrinter(
            Parameters::PrintMode::e_JSON_PRETTY,
            resultStream,
            bmqtst::TestHelperUtil::allocator());

        // Print summary
        printer->printSummaryResult(recordCount,
                                    updateChoiceMap,
                                    queueMap,
                                    processCslRecordTypes,
                                    10);
    }

    // Prepare expected output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    expectedStream
        << "{\n"
        << "    \"Summary\":\n"
        << "    {\n"
        << "      \"SnapshotRecords\": \"2\",\n"
        << "      \"UpdateRecords\": \"3\",\n"
        << "      \"queueAssignmentAdvisory\": \"3\",\n"
        << "      \"leaderAdvisory\": \"2\",\n"
        << "      \"CommitRecords\": \"4\",\n"
        << "      \"AckRecords\": \"5\"\n"
        << "    },\n"
        << "    \"Queues\": [\n"
        << "      \"[ uri = "
           "\\\"bmq://bmq.test.persistent.priority/second-queue\\\" key = [ "
           "62 ] partitionId = 3 appIds = [ ] ]\",\n"
        << "      \"[ uri = "
           "\\\"bmq://bmq.test.persistent.priority/first-queue\\\" key = [ 61 "
           "] partitionId = 2 appIds = [ ] ]\"\n"
        << "    ]\n"
        << "}\n";

    BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
}

static void test9_lineShortResultTest()
// ------------------------------------------------------------------------
// JSON LINE SHORT RESULT TEST
//
// Concerns:
//   Exercise the output of the JsonLineCslPrinter::printShortResult().
//
// Testing:
//   JsonLineCslPrinter::printShortResult
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LINE SHORT TEST");

    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

    // Create record data
    mqbc::ClusterStateRecordHeader header;
    header.setHeaderWords(8)
        .setRecordType(mqbc::ClusterStateRecordType::e_SNAPSHOT)
        .setLeaderAdvisoryWords(10)
        .setElectorTerm(1)
        .setSequenceNumber(2)
        .setTimestamp(123456789);
    mqbsi::LedgerRecordId recordId;
    mqbu::StorageKey      storageKey(1);
    recordId.setLogId(storageKey).setOffset(2);

    {
        // Create printer
        bsl::shared_ptr<CslPrinter> printer = createCslPrinter(
            Parameters::PrintMode::e_JSON_LINE,
            resultStream,
            bmqtst::TestHelperUtil::allocator());

        // Print short result
        printer->printShortResult(header, recordId);
    }

    // Prepare expected output
    expectedStream
        << "{\n"
        << "  \"Records\": [\n"
        << "    {\"RecordType\": \"SNAPSHOT\", \"Offset\": \"2\", \"LogId\": "
           "\"3000000001\", \"ElectorTerm\": \"1\", \"SequenceNumber\": "
           "\"2\", \"HeaderWords\": \"8\", \"LeaderAdvisoryWords\": \"10\", "
           "\"Timestamp\": \"29NOV1973_21:33:09.000000\", \"Epoch\": "
           "\"123456789\"}\n"
        << "  ]\n"
        << "}\n";

    bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
    bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
    const int rc = bdljsn::JsonUtil::read(&json, &error, resultStream.str());
    BMQTST_ASSERT_D(error, (rc == 0));
    BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
}

static void test10_lineDetailResultTest()
// ------------------------------------------------------------------------
// JSON LINE DETAIL RESULT TEST
//
// Concerns:
//   Exercise the output of the JsonLineCslPrinter::printDetailResult().
//
// Testing:
//   JsonLineCslPrinter::printDetailResult
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LINE SHORT TEST");

    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

    // Ignore check default allocator due to issue with bslim::Printer::print()
    // used in recordToJsonString()
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    // Create record data
    bmqp_ctrlmsg::ClusterMessage record(bmqtst::TestHelperUtil::allocator());
    record.choice().makeLeaderAdvisory();

    mqbc::ClusterStateRecordHeader header;
    header.setHeaderWords(8)
        .setRecordType(mqbc::ClusterStateRecordType::e_SNAPSHOT)
        .setLeaderAdvisoryWords(10)
        .setElectorTerm(1)
        .setSequenceNumber(2)
        .setTimestamp(123456789);

    mqbsi::LedgerRecordId recordId;
    mqbu::StorageKey      storageKey(1);
    recordId.setLogId(storageKey).setOffset(2);

    {
        // Create printer
        bsl::shared_ptr<CslPrinter> printer = createCslPrinter(
            Parameters::PrintMode::e_JSON_LINE,
            resultStream,
            bmqtst::TestHelperUtil::allocator());

        // Print detail result
        printer->printDetailResult(record, header, recordId);
    }

    // Prepare expected output
    expectedStream
        << "{\n"
        << "  \"Records\": [\n"
        << "    {\"RecordType\": \"SNAPSHOT\", \"Offset\": \"2\", \"LogId\": "
           "\"3000000001\", \"ElectorTerm\": \"1\", \"SequenceNumber\": "
           "\"2\", \"HeaderWords\": \"8\", \"LeaderAdvisoryWords\": \"10\", "
           "\"Timestamp\": \"29NOV1973_21:33:09.000000\", \"Epoch\": "
           "\"123456789\", \"Record\": \"[ choice = [ leaderAdvisory = [ "
           "sequenceNumber = [ electorTerm = 0 sequenceNumber = 0 ] "
           "partitions = [ ] queues = [ ] ] ] \"}\n"
        << "  ]\n"
        << "}\n";

    bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
    bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
    const int rc = bdljsn::JsonUtil::read(&json, &error, resultStream.str());
    BMQTST_ASSERT_D(error, (rc == 0));
    BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
}

static void test11_lineSummaryTest()
// ------------------------------------------------------------------------
// JSON PRETTY SUMMARY TEST
//
// Concerns:
//   Exercise the output of the JsonLineCslPrinter::printSummaryResult().
//
// Testing:
//   JsonLineCslPrinter::printSummaryResult
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LINE SUMMARY TEST");

    // Ignore check default allocator due to issue with bslim::Printer::print()
    // used in recordToJsonString()
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());

    // Create record data
    CslRecordCount recordCount;
    recordCount.d_snapshotCount = 2;
    recordCount.d_updateCount   = 3;
    recordCount.d_commitCount   = 4;
    recordCount.d_ackCount      = 5;

    Parameters::ProcessCslRecordTypes processCslRecordTypes;
    processCslRecordTypes.d_snapshot = true;
    processCslRecordTypes.d_update   = true;
    processCslRecordTypes.d_commit   = true;
    processCslRecordTypes.d_ack      = true;

    CslUpdateChoiceMap updateChoiceMap(bmqtst::TestHelperUtil::allocator());
    updateChoiceMap[1] = 2;
    updateChoiceMap[2] = 3;
    QueueMap                queueMap(bmqtst::TestHelperUtil::allocator());
    bmqp_ctrlmsg::QueueInfo queueInfo(bmqtst::TestHelperUtil::allocator());
    queueInfo.uri()         = "bmq://bmq.test.persistent.priority/first-queue";
    queueInfo.partitionId() = 2;
    queueInfo.key().push_back('a');
    queueMap.insert(queueInfo);
    queueInfo.uri() = "bmq://bmq.test.persistent.priority/second-queue";
    queueInfo.partitionId() = 3;
    queueInfo.key().clear();
    queueInfo.key().push_back('b');
    queueMap.insert(queueInfo);

    {
        // Create printer
        bsl::shared_ptr<CslPrinter> printer = createCslPrinter(
            Parameters::PrintMode::e_JSON_LINE,
            resultStream,
            bmqtst::TestHelperUtil::allocator());

        // Print summary
        printer->printSummaryResult(recordCount,
                                    updateChoiceMap,
                                    queueMap,
                                    processCslRecordTypes,
                                    10);
    }

    // Prepare expected output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    expectedStream
        << "{\n"
        << "    \"Summary\": {\"SnapshotRecords\": \"2\", \"UpdateRecords\": "
           "\"3\", \"queueAssignmentAdvisory\": \"3\", \"leaderAdvisory\": "
           "\"2\", \"CommitRecords\": \"4\", \"AckRecords\": \"5\"},\n"
        << "    \"Queues\": [\n"
        << "      \"[ uri = "
           "\\\"bmq://bmq.test.persistent.priority/second-queue\\\" key = [ "
           "62 ] partitionId = 3 appIds = [ ] ]\",\n"
        << "      \"[ uri = "
           "\\\"bmq://bmq.test.persistent.priority/first-queue\\\" key = [ 61 "
           "] partitionId = 2 appIds = [ ] ]\"\n"
        << "    ]\n"
        << "}\n";

    BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_humanReadableShortResultTest(); break;
    case 2: test2_humanReadableDetailResultTest(); break;
    case 3: test3_humanReadableNotFoundTest(); break;
    case 4: test4_humanReadableFooterTest(); break;
    case 5: test5_humanReadableSummaryTest(); break;
    case 6: test6_prettyShortResultTest(); break;
    case 7: test7_prettyDetailResultTest(); break;
    case 8: test8_prettySummaryTest(); break;
    case 9: test9_lineShortResultTest(); break;
    case 10: test10_lineDetailResultTest(); break;
    case 11: test11_lineSummaryTest(); break;
    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
