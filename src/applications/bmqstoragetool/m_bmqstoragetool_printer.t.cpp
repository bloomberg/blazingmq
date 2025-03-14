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
#include <m_bmqstoragetool_printer.h>

#include <m_bmqstoragetool_filemanagermock.h>
#include <m_bmqstoragetool_messagedetails.h>

// MQB
#include <mqbs_filestoreprotocol.h>
#include <mqbu_messageguidutil.h>

// BDE
#include <bdljsn_jsonutil.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace m_bmqstoragetool;
using namespace bsl;
using namespace mqbs;

namespace {

/// List of message details.
typedef bsl::list<MessageDetails> DetailsList;
/// Hash map of message guids to message details.
typedef bsl::unordered_map<bmqt::MessageGUID, DetailsList::iterator>
    DetailsMap;
/// Composite sequence numbers vector.
typedef bsl::vector<CompositeSequenceNumber> CompositesVec;
/// Offsets vector.
typedef bsl::vector<bsls::Types::Int64> OffsetsVec;
/// List of message guids.
typedef bsl::list<bmqt::MessageGUID> GuidsList;
/// Queue operations count vector.
typedef bsl::vector<bsls::Types::Uint64> QueueOpCountsVec;

const size_t k_HEADER_SIZE = sizeof(mqbs::FileHeader) +
                             sizeof(mqbs::JournalFileHeader);
// Size of a journal file header

/// Calculate record offset
template <typename RECORD_TYPE>
bsls::Types::Uint64 recordOffset(const RECORD_TYPE& record)
{
    BSLS_ASSERT_SAFE(record.header().sequenceNumber() > 0);
    return k_HEADER_SIZE + (record.header().sequenceNumber() - 1) *
                               mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
}

}

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_humanReadableGuidTest()
// ------------------------------------------------------------------------
// HUMAN READABLE GUID TEST
//
// Concerns:
//   Exercise the output of the HumanReadablePrinter.
//
// Testing:
//   HumanReadablePrinter::printGuid
//   HumanReadablePrinter::printGuidNotFound
//   HumanReadablePrinter::printGuidsNotFound
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HUMAN GUID TEST");

    {
        // Test printGuid

        bmqt::MessageGUID guid;
        mqbu::MessageGUIDUtil::generateGUID(&guid);

        // Create printer
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bsl::shared_ptr<Printer> printer = createPrinter(
            Parameters::PrintMode::e_HUMAN,
            resultStream,
            bmqtst::TestHelperUtil::allocator());

        printer->printGuid(guid);

        // Prepare expected output
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        expectedStream << guid << '\n';

        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test printGuidNotFound

        bmqt::MessageGUID guid;
        mqbu::MessageGUIDUtil::generateGUID(&guid);

        // Create printer
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bsl::shared_ptr<Printer> printer = createPrinter(
            Parameters::PrintMode::e_HUMAN,
            resultStream,
            bmqtst::TestHelperUtil::allocator());

        printer->printGuidNotFound(guid);

        // Prepare expected output
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        expectedStream << "Logic error : guid " << guid << " not found\n";

        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test printGuidsNotFound

        // Create printer
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bsl::shared_ptr<Printer> printer = createPrinter(
            Parameters::PrintMode::e_HUMAN,
            resultStream,
            bmqtst::TestHelperUtil::allocator());

        // Prepare argument for the test and expected output
        bsl::list<bmqt::MessageGUID> guids(
            bmqtst::TestHelperUtil::allocator());

        // Test empty GUIDs list
        printer->printGuidsNotFound(guids);
        BMQTST_ASSERT(resultStream.isEmpty());

        // Test not empty GUIDs list
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        expectedStream << "\nThe following 10 GUID(s) not found:\n";
        for (int i = 0; i < 10; ++i) {
            bmqt::MessageGUID guid;
            mqbu::MessageGUIDUtil::generateGUID(&guid);
            expectedStream << guid << '\n';
            guids.push_back(guid);
        }

        printer->printGuidsNotFound(guids);

        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

static void test2_humanReadableMessageTest()
// ------------------------------------------------------------------------
// HUMAN READABLE MESSAGE TEST
//
// Concerns:
//   Exercise the output of the HumanReadablePrinter.
//
// Testing:
//   HumanReadablePrinter::printMessage
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HUMAN MESSAGE TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 3;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    bsl::vector<bmqt::MessageGUID> expectedGuids(
        bmqtst::TestHelperUtil::allocator());
    JournalFile journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    journalFile.addJournalRecordsWithOutstandingAndConfirmedMessages(
        &records,
        &expectedGuids,
        false);

    // Prepare MessageDetails
    bslma::ManagedPtr<MessageDetails>                details;
    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    for (; recordIter != records.cend(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            details.load(
                new (*bmqtst::TestHelperUtil::allocator())
                    MessageDetails(msg,
                                   msg.header().sequenceNumber(),
                                   recordOffset(msg),
                                   bsl::nullopt,
                                   bmqtst::TestHelperUtil::allocator()),
                bmqtst::TestHelperUtil::allocator());
        }
        else if (rtype == RecordType::e_CONFIRM) {
            const ConfirmRecord& conf =
                *reinterpret_cast<const ConfirmRecord*>(
                    recordIter->second.buffer());
            details->addConfirmRecord(conf,
                                      conf.header().sequenceNumber(),
                                      recordOffset(conf));
        }
        else if (rtype == RecordType::e_DELETION) {
            const DeletionRecord& del =
                *reinterpret_cast<const DeletionRecord*>(
                    recordIter->second.buffer());
            details->addDeleteRecord(del,
                                     del.header().sequenceNumber(),
                                     recordOffset(del));
        }
    }
    const bmqt::MessageGUID& guid =
        details->messageRecord().d_record.messageGUID();

    // Create printer
    bmqu::MemOutStream       resultStream(bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<Printer> printer = createPrinter(
        Parameters::PrintMode::e_HUMAN,
        resultStream,
        bmqtst::TestHelperUtil::allocator());

    printer->printMessage(*details);

    // Prepare expected output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    expectedStream << "==============================\n\n"
                   << "    RecordType      : MESSAGE\n"
                   << "    Index           : 1\n"
                   << "    Offset          : 44\n"
                   << "    PrimaryLeaseId  : 100\n"
                   << "    SequenceNumber  : 1\n"
                   << "    Timestamp       : 01JAN1970_00:01:40.000000\n"
                   << "    Epoch           : 100\n"
                   << "    QueueKey        : 6162636465\n"
                   << "    FileKey         : 3132333435\n"
                   << "    RefCount        : 1\n"
                   << "    MsgOffsetDwords : 1\n"
                   << "    GUID            : " << guid << "\n"
                   << "    Crc32c          : 1\n\n"
                   << "    RecordType      : CONFIRM\n"
                   << "    Index           : 2\n"
                   << "    Offset          : 104\n"
                   << "    PrimaryLeaseId  : 100\n"
                   << "    SequenceNumber  : 2\n"
                   << "    Timestamp       : 01JAN1970_00:03:20.000000\n"
                   << "    Epoch           : 200\n"
                   << "    QueueKey        : 6162636465\n"
                   << "    AppKey          : 6170706964\n"
                   << "    GUID            : " << guid << "\n\n"
                   << "    RecordType      : DELETION\n"
                   << "    Index           : 3\n"
                   << "    Offset          : 164\n"
                   << "    PrimaryLeaseId  : 100\n"
                   << "    SequenceNumber  : 3\n"
                   << "    Timestamp       : 01JAN1970_00:05:00.000000\n"
                   << "    Epoch           : 300\n"
                   << "    QueueKey        : 6162636465\n"
                   << "    DeletionFlag    : IMPLICIT_CONFIRM\n"
                   << "    GUID            : " << guid << "\n\n";

    BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
}

static void test3_humanReadableRecordsTest()
// ------------------------------------------------------------------------
// HUMAN READABLE RECORDS TEST
//
// Concerns:
//   Exercise the output of the HumanReadablePrinter.
//
// Testing:
//   HumanReadablePrinter::printQueueOpRecord
//   HumanReadablePrinter::printJournalOpRecord
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HUMAN RECORDS TEST");

    {
        // Test printQueueOpRecord

        // Simulate journal file
        JournalFile journalFile(1, bmqtst::TestHelperUtil::allocator());
        JournalFile::RecordBufferType buf = journalFile.makeQueueOpRecord(100,
                                                                          1);

        const QueueOpRecord& queueOpRecord =
            *reinterpret_cast<const QueueOpRecord*>(buf.buffer());
        RecordDetails<mqbs::QueueOpRecord> queueOpDetails(
            queueOpRecord,
            12345,
            56789,
            bmqtst::TestHelperUtil::allocator());

        // Create printer
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bsl::shared_ptr<Printer> printer = createPrinter(
            Parameters::PrintMode::e_HUMAN,
            resultStream,
            bmqtst::TestHelperUtil::allocator());

        printer->printQueueOpRecord(queueOpDetails);

        // Prepare expected output
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        expectedStream << "==============================\n\n"
                       << "    RecordType      : QUEUE_OP\n"
                       << "    Index           : 12345\n"
                       << "    Offset          : 56789\n"
                       << "    PrimaryLeaseId  : 100\n"
                       << "    SequenceNumber  : 1\n"
                       << "    Timestamp       : 01JAN1970_00:01:40.000000\n"
                       << "    Epoch           : 100\n"
                       << "    QueueKey        : 6162636465\n"
                       << "    AppKey          : 6170706964\n"
                       << "    QueueOpType     : PURGE\n\n";

        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test printJournalOpRecord

        // Simulate journal file
        JournalFile journalFile(1, bmqtst::TestHelperUtil::allocator());
        JournalFile::RecordBufferType buf =
            journalFile.makeJournalOpRecord(100, 1);

        const JournalOpRecord& journalOpRecord =
            *reinterpret_cast<const JournalOpRecord*>(buf.buffer());
        RecordDetails<mqbs::JournalOpRecord> journalOpDetails(
            journalOpRecord,
            12345,
            56789,
            bmqtst::TestHelperUtil::allocator());

        // Create printer
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bsl::shared_ptr<Printer> printer = createPrinter(
            Parameters::PrintMode::e_HUMAN,
            resultStream,
            bmqtst::TestHelperUtil::allocator());

        printer->printJournalOpRecord(journalOpDetails);

        // Prepare expected output
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        expectedStream << "==============================\n\n"
                       << "    RecordType      : JOURNAL_OP\n"
                       << "    Index           : 12345\n"
                       << "    Offset          : 56789\n"
                       << "    PrimaryLeaseId  : 100\n"
                       << "    SequenceNumber  : 1\n"
                       << "    Timestamp       : 01JAN1970_00:01:40.000000\n"
                       << "    Epoch           : 100\n"
                       << "    JournalOpType   : SYNCPOINT\n"
                       << "    SyncPointType   : REGULAR\n"
                       << "    SyncPtPrimaryLeaseId: 25\n"
                       << "    SyncPtSequenceNumber: 1234567\n"
                       << "    PrimaryNodeId   : 121\n"
                       << "    DataFileOffsetDwords: 8800\n\n";

        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

void test4_humanReadableOffsetsTest()
// ------------------------------------------------------------------------
// HUMAN READABLE OFFSETS TEST
//
// Concerns:
//   Exercise the output of the HumanReadablePrinter.
//
// Testing:
//   HumanReadablePrinter::printOffsetsNotFound
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HUMAN OFFSETS TEST");

    // Create printer
    bmqu::MemOutStream       resultStream(bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<Printer> printer = createPrinter(
        Parameters::PrintMode::e_HUMAN,
        resultStream,
        bmqtst::TestHelperUtil::allocator());

    {
        // Test empty OffsetsVec

        printer->printOffsetsNotFound(
            OffsetsVec(bmqtst::TestHelperUtil::allocator()));

        BMQTST_ASSERT(resultStream.isEmpty());
    }

    {
        // Test not empty OffsetsVec

        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        expectedStream << "\nThe following 10 offset(s) not found:\n";

        OffsetsVec vec(bmqtst::TestHelperUtil::allocator());
        for (bsls::Types::Int64 i = 1; i < 11; ++i) {
            vec.push_back(i);
            expectedStream << i << "\n";
        }

        printer->printOffsetsNotFound(vec);

        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

static void test5_humanReadableCompositesTest()
// ------------------------------------------------------------------------
// HUMAN READABLE COMPOSITES TEST
//
// Concerns:
//   Exercise the output of the HumanReadablePrinter.
//
// Testing:
//   HumanReadablePrinter::printCompositesNotFound
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HUMAN COMPOSITES TEST");

    // Create printer
    bmqu::MemOutStream       resultStream(bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<Printer> printer = createPrinter(
        Parameters::PrintMode::e_HUMAN,
        resultStream,
        bmqtst::TestHelperUtil::allocator());

    {
        // Test empty CompositesVec

        printer->printCompositesNotFound(
            CompositesVec(bmqtst::TestHelperUtil::allocator()));

        BMQTST_ASSERT(resultStream.isEmpty());
    }

    {
        // Test not empty CompositesVec

        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        expectedStream << "\nThe following 10 sequence number(s) not found:\n";

        CompositesVec vec(bmqtst::TestHelperUtil::allocator());
        for (unsigned int i = 1; i < 11; ++i) {
            vec.emplace_back(CompositeSequenceNumber(i, 2lu * i));
            expectedStream << "leaseId: " << i
                           << ", sequenceNumber: " << (2lu * i) << "\n";
        }

        printer->printCompositesNotFound(vec);

        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

static void test6_humanReadableFooterTest()
// ------------------------------------------------------------------------
// HUMAN READABLE FOOTER TEST
//
// Concerns:
//   Exercise the output of the HumanReadablePrinter.
//
// Testing:
//   HumanReadablePrinter::printFooter
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HUMAN FOOTER TEST");

    // Create printer
    bmqu::MemOutStream       resultStream(bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<Printer> printer = createPrinter(
        Parameters::PrintMode::e_HUMAN,
        resultStream,
        bmqtst::TestHelperUtil::allocator());

    {
        Parameters::ProcessRecordTypes recordTypes;
        printer->printFooter(0u, 0ul, 0ul, recordTypes);
        BMQTST_ASSERT(resultStream.isEmpty());
    }

    {
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        Parameters::ProcessRecordTypes recordTypes;
        recordTypes.d_message   = true;
        recordTypes.d_queueOp   = true;
        recordTypes.d_journalOp = true;
        printer->printFooter(0u, 0ul, 0ul, recordTypes);
        expectedStream << "No message record found.\n"
                       << "No queueOp record found.\n"
                       << "No journalOp record found.\n";
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        resultStream.reset();
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        Parameters::ProcessRecordTypes recordTypes;
        recordTypes.d_message   = true;
        recordTypes.d_queueOp   = true;
        recordTypes.d_journalOp = true;
        printer->printFooter(11u, 22ul, 33ul, recordTypes);
        expectedStream << 11u << " message record(s) found.\n"
                       << 22u << " queueOp record(s) found.\n"
                       << 33u << " journalOp record(s) found.\n";
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

static void test7_humanReadableOutstandingTest()
// ------------------------------------------------------------------------
// HUMAN READABLE OUTSTANDING TEST
//
// Concerns:
//   Exercise the output of the HumanReadablePrinter.
//
// Testing:
//   HumanReadablePrinter::printOutstandingRatio
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HUMAN OUTSTANDING TEST");

    // Create printer
    bmqu::MemOutStream       resultStream(bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<Printer> printer = createPrinter(
        Parameters::PrintMode::e_HUMAN,
        resultStream,
        bmqtst::TestHelperUtil::allocator());

    {
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        printer->printOutstandingRatio(35, 7ul, 20ul);
        expectedStream << "Outstanding ratio: 35% (7/20)\n";
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

static void test8_humanReadableSummaryTest()
// ------------------------------------------------------------------------
// HUMAN READABLE SUMMARY TEST
//
// Concerns:
//   Exercise the output of the HumanReadablePrinter.
//
// Testing:
//   HumanReadablePrinter::printMessageSummary
//   HumanReadablePrinter::printQueueOpSummary
//   HumanReadablePrinter::printJournalOpSummary
//   HumanReadablePrinter::printRecordSummary
//   HumanReadablePrinter::printJournalOpSummary
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HUMAN SUMMARY TEST");

    // Create printer
    bmqu::MemOutStream       resultStream(bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<Printer> printer = createPrinter(
        Parameters::PrintMode::e_HUMAN,
        resultStream,
        bmqtst::TestHelperUtil::allocator());

    {
        // Test no message summary
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        expectedStream << "\nNo messages found.\n";
        printer->printMessageSummary(0, 1, 2, 3);
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test message summary
        resultStream.reset();
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        expectedStream << "\nTotal number of messages: 7\n"
                       << "Number of partially confirmed messages: 6\n"
                       << "Number of confirmed messages: 5\n"
                       << "Number of outstanding messages: 4\n";
        printer->printMessageSummary(7, 6, 5, 4);
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test no QueueOp summary
        resultStream.reset();
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        expectedStream << "\nNo queueOp records found.\n";
        printer->printQueueOpSummary(
            0u,
            QueueOpCountsVec(mqbs::QueueOpType::e_ADDITION + 1,
                             0,
                             bmqtst::TestHelperUtil::allocator()));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test QueueOp summary
        resultStream.reset();
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        expectedStream << "\nTotal number of queueOp records: 123\n"
                       << "    Number of 'purge' operations     : 1\n"
                       << "    Number of 'creation' operations  : 2\n"
                       << "    Number of 'deletion' operations  : 3\n"
                       << "    Number of 'addition' operations  : 4\n";

        QueueOpCountsVec vec(mqbs::QueueOpType::e_ADDITION + 1,
                             0,
                             bmqtst::TestHelperUtil::allocator());
        vec[mqbs::QueueOpType::e_PURGE]    = 1ul;
        vec[mqbs::QueueOpType::e_CREATION] = 2ul;
        vec[mqbs::QueueOpType::e_DELETION] = 3ul;
        vec[mqbs::QueueOpType::e_ADDITION] = 4ul;
        printer->printQueueOpSummary(123u, vec);
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test no JournalOp summary
        resultStream.reset();
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        expectedStream << "\nNo journalOp records found.\n";
        printer->printJournalOpSummary(0u);
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test JournalOp summary
        resultStream.reset();
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        expectedStream << "\nNumber of journalOp records: 123\n";
        printer->printJournalOpSummary(123u);
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        resultStream.reset();
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

        expectedStream << "Total number of records: 123\n"
                       << "Number of records per Queue:\n"
                       << "    Queue Key             : 5175657565\n"
                       << "    Queue URI             : QueueUri1\n"
                       << "    Total Records         : 12345\n"
                       << "    Num Queue Op Records  : 78\n"
                       << "    Num Message Records   : 12\n"
                       << "    Num Confirm Records   : 34\n"
                       << "    Num Records Per App   : AppId2=99 AppId1=9 \n"
                       << "    Num Delete Records    : 56\n";

        QueueDetailsMap map(bmqtst::TestHelperUtil::allocator());
        QueueDetails&   details =
            map.emplace(
                   mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                    "Queue1"),
                   QueueDetails(bmqtst::TestHelperUtil::allocator()))
                .first->second;
        details.d_recordsNumber        = 12345;
        details.d_messageRecordsNumber = 12;
        details.d_confirmRecordsNumber = 34;
        details.d_deleteRecordsNumber  = 56;
        details.d_queueOpRecordsNumber = 78;
        details.d_queueUri             = "QueueUri1";
        QueueDetails::AppDetails& appDetails1 =
            details.d_appDetailsMap
                .emplace(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "App1"),
                    QueueDetails::AppDetails())
                .first->second;
        appDetails1.d_appId         = "AppId1";
        appDetails1.d_recordsNumber = 9;

        QueueDetails::AppDetails& appDetails2 =
            details.d_appDetailsMap
                .emplace(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "App2"),
                    QueueDetails::AppDetails())
                .first->second;
        appDetails2.d_appId         = "AppId2";
        appDetails2.d_recordsNumber = 99;

        printer->printRecordSummary(123, map);

        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

static void test9_jsonPrettyGuidTest()
// ------------------------------------------------------------------------
// JSON PRETTY GUID TEST
//
// Concerns:
//   Exercise the output of the JsonPrettyPrinter.
//
// Testing:
//   JsonPrettyPrinter::printGuid
//   JsonPrettyPrinter::printGuidNotFound
//   JsonPrettyPrinter::printGuidsNotFound
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PRETTY GUID TEST");

    {
        // Test printGuid

        bmqt::MessageGUID guid;
        mqbu::MessageGUIDUtil::generateGUID(&guid);

        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_PRETTY,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            printer->printGuid(guid);

            // Prepare expected output
            expectedStream << "{\n"
                           << "  \"Records\": [\n"
                           << "    \"" << guid << "\"\n"
                           << "  ]\n"
                           << "}\n";
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test printGuidNotFound

        bmqt::MessageGUID guid;
        mqbu::MessageGUIDUtil::generateGUID(&guid);

        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_PRETTY,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            printer->printGuidNotFound(guid);

            // Prepare expected output
            expectedStream << "{\n"
                           << "  \"Records\": [\n"
                           << "    {\"LogicError\" : \"guid " << guid
                           << " not found\"}\n"
                           << "  ]\n"
                           << "}\n";
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test printGuid and printGuidNotFound in combination

        bmqt::MessageGUID guid1, guid2;
        mqbu::MessageGUIDUtil::generateGUID(&guid1);
        mqbu::MessageGUIDUtil::generateGUID(&guid2);

        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_PRETTY,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            printer->printGuid(guid1);
            printer->printGuidNotFound(guid2);

            // Prepare expected output
            expectedStream << "{\n"
                           << "  \"Records\": [\n"
                           << "    \"" << guid1 << "\",\n"
                           << "    {\"LogicError\" : \"guid " << guid2
                           << " not found\"}\n"
                           << "  ]\n"
                           << "}\n";
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    // Test printGuidsNotFound

    {
        // Test empty GUIDs list
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_PRETTY,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            // Prepare argument for the test and expected output
            bsl::list<bmqt::MessageGUID> guids(
                bmqtst::TestHelperUtil::allocator());

            printer->printGuidsNotFound(guids);

            // Prepare expected output
            expectedStream << "{\n  \"GuidsNotFound\": [\n  ]\n}\n";
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test not empty GUIDs list
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_PRETTY,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            // Prepare argument for the test and expected output
            bsl::list<bmqt::MessageGUID> guids(
                bmqtst::TestHelperUtil::allocator());

            expectedStream << "{\n  \"GuidsNotFound\": [\n";
            for (int i = 0; i < 10; ++i) {
                bmqt::MessageGUID guid;
                mqbu::MessageGUIDUtil::generateGUID(&guid);
                if (i != 0) {
                    expectedStream << ",\n";
                }
                expectedStream << "    \"" << guid << "\"";
                guids.push_back(guid);
            }
            expectedStream << "\n  ]\n}\n";

            printer->printGuidsNotFound(guids);
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

static void test10_jsonPrettyMessageTest()
// ------------------------------------------------------------------------
// JSON PRETTY MESSAGE TEST
//
// Concerns:
//   Exercise the output of the JsonPrettyPrinter.
//
// Testing:
//   JsonPrettyPrinter::printMessage
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PRETTY MESSAGE TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 3;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    bsl::vector<bmqt::MessageGUID> expectedGuids(
        bmqtst::TestHelperUtil::allocator());
    JournalFile journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    journalFile.addJournalRecordsWithOutstandingAndConfirmedMessages(
        &records,
        &expectedGuids,
        false);

    // Prepare MessageDetails
    bslma::ManagedPtr<MessageDetails>                details;
    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    for (; recordIter != records.cend(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            details.load(
                new (*bmqtst::TestHelperUtil::allocator())
                    MessageDetails(msg,
                                   msg.header().sequenceNumber(),
                                   recordOffset(msg),
                                   bsl::nullopt,
                                   bmqtst::TestHelperUtil::allocator()),
                bmqtst::TestHelperUtil::allocator());
        }
        else if (rtype == RecordType::e_CONFIRM) {
            const ConfirmRecord& conf =
                *reinterpret_cast<const ConfirmRecord*>(
                    recordIter->second.buffer());
            details->addConfirmRecord(conf,
                                      conf.header().sequenceNumber(),
                                      recordOffset(conf));
        }
        else if (rtype == RecordType::e_DELETION) {
            const DeletionRecord& del =
                *reinterpret_cast<const DeletionRecord*>(
                    recordIter->second.buffer());
            details->addDeleteRecord(del,
                                     del.header().sequenceNumber(),
                                     recordOffset(del));
        }
    }
    const bmqt::MessageGUID& guid =
        details->messageRecord().d_record.messageGUID();

    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

    {
        // Create printer
        bsl::shared_ptr<Printer> printer = createPrinter(
            Parameters::PrintMode::e_JSON_PRETTY,
            resultStream,
            bmqtst::TestHelperUtil::allocator());

        printer->printMessage(*details);

        // Prepare expected output
        expectedStream
            << "{\n"
            << "  \"Records\": [\n"
            << "    {\n"
            << "      \"RecordType\": \"MESSAGE\",\n"
            << "      \"Index\": \"1\",\n"
            << "      \"Offset\": \"44\",\n"
            << "      \"PrimaryLeaseId\": \"100\",\n"
            << "      \"SequenceNumber\": \"1\",\n"
            << "      \"Timestamp\": \"01JAN1970_00:01:40.000000\",\n"
            << "      \"Epoch\": \"100\",\n"
            << "      \"QueueKey\": \"6162636465\",\n"
            << "      \"FileKey\": \"3132333435\",\n"
            << "      \"RefCount\": \"1\",\n"
            << "      \"MsgOffsetDwords\": \"1\",\n"
            << "      \"GUID\": \"" << guid << "\",\n"
            << "      \"Crc32c\": \"1\"\n"
            << "    },\n"
            << "    {\n"
            << "      \"RecordType\": \"CONFIRM\",\n"
            << "      \"Index\": \"2\",\n"
            << "      \"Offset\": \"104\",\n"
            << "      \"PrimaryLeaseId\": \"100\",\n"
            << "      \"SequenceNumber\": \"2\",\n"
            << "      \"Timestamp\": \"01JAN1970_00:03:20.000000\",\n"
            << "      \"Epoch\": \"200\",\n"
            << "      \"QueueKey\": \"6162636465\",\n"
            << "      \"AppKey\": \"6170706964\",\n"
            << "      \"GUID\": \"" << guid << "\"\n"
            << "    },\n"
            << "    {\n"
            << "      \"RecordType\": \"DELETION\",\n"
            << "      \"Index\": \"3\",\n"
            << "      \"Offset\": \"164\",\n"
            << "      \"PrimaryLeaseId\": \"100\",\n"
            << "      \"SequenceNumber\": \"3\",\n"
            << "      \"Timestamp\": \"01JAN1970_00:05:00.000000\",\n"
            << "      \"Epoch\": \"300\",\n"
            << "      \"QueueKey\": \"6162636465\",\n"
            << "      \"DeletionFlag\": \"IMPLICIT_CONFIRM\",\n"
            << "      \"GUID\": \"" << guid << "\"\n"
            << "    }\n"
            << "  ]\n"
            << "}\n";
    }

    bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
    bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
    const int rc = bdljsn::JsonUtil::read(&json, &error, resultStream.str());
    BMQTST_ASSERT_D(error, (rc == 0));
    BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
}

static void test11_jsonPrettyRecordsTest()
// ------------------------------------------------------------------------
// JSON PRETTY RECORDS TEST
//
// Concerns:
//   Exercise the output of the JsonPrettyPrinter.
//
// Testing:
//   JsonPrettyPrinter::printQueueOpRecord
//   JsonPrettyPrinter::printJournalOpRecord
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PRETTY RECORDS TEST");

    {
        // Test printQueueOpRecord

        // Simulate journal file
        JournalFile journalFile(1, bmqtst::TestHelperUtil::allocator());
        JournalFile::RecordBufferType buf = journalFile.makeQueueOpRecord(100,
                                                                          1);

        const QueueOpRecord& queueOpRecord =
            *reinterpret_cast<const QueueOpRecord*>(buf.buffer());
        RecordDetails<mqbs::QueueOpRecord> queueOpDetails(
            queueOpRecord,
            12345,
            56789,
            bmqtst::TestHelperUtil::allocator());

        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_PRETTY,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            printer->printQueueOpRecord(queueOpDetails);

            // Prepare expected output
            expectedStream
                << "{\n"
                << "  \"Records\": [\n"
                << "    {\n"
                << "      \"RecordType\": \"QUEUE_OP\",\n"
                << "      \"Index\": \"12345\",\n"
                << "      \"Offset\": \"56789\",\n"
                << "      \"PrimaryLeaseId\": \"100\",\n"
                << "      \"SequenceNumber\": \"1\",\n"
                << "      \"Timestamp\": \"01JAN1970_00:01:40.000000\",\n"
                << "      \"Epoch\": \"100\",\n"
                << "      \"QueueKey\": \"6162636465\",\n"
                << "      \"AppKey\": \"6170706964\",\n"
                << "      \"QueueOpType\": \"PURGE\"\n"
                << "    }\n"
                << "  ]\n"
                << "}\n";
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test printJournalOpRecord

        // Simulate journal file
        JournalFile journalFile(1, bmqtst::TestHelperUtil::allocator());
        JournalFile::RecordBufferType buf =
            journalFile.makeJournalOpRecord(100, 1);

        const JournalOpRecord& journalOpRecord =
            *reinterpret_cast<const JournalOpRecord*>(buf.buffer());
        RecordDetails<mqbs::JournalOpRecord> journalOpDetails(
            journalOpRecord,
            12345,
            56789,
            bmqtst::TestHelperUtil::allocator());

        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_PRETTY,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            printer->printJournalOpRecord(journalOpDetails);

            // Prepare expected output
            expectedStream
                << "{\n"
                << "  \"Records\": [\n"
                << "    {\n"
                << "      \"RecordType\": \"JOURNAL_OP\",\n"
                << "      \"Index\": \"12345\",\n"
                << "      \"Offset\": \"56789\",\n"
                << "      \"PrimaryLeaseId\": \"100\",\n"
                << "      \"SequenceNumber\": \"1\",\n"
                << "      \"Timestamp\": \"01JAN1970_00:01:40.000000\",\n"
                << "      \"Epoch\": \"100\",\n"
                << "      \"JournalOpType\": \"SYNCPOINT\",\n"
                << "      \"SyncPointType\": \"REGULAR\",\n"
                << "      \"SyncPtPrimaryLeaseId\": \"25\",\n"
                << "      \"SyncPtSequenceNumber\": \"1234567\",\n"
                << "      \"PrimaryNodeId\": \"121\",\n"
                << "      \"DataFileOffsetDwords\": \"8800\"\n"
                << "    }\n"
                << "  ]\n"
                << "}\n";
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

void test12_jsonPrettyOffsetsTest()
// ------------------------------------------------------------------------
// JSON PRETTY OFFSETS TEST
//
// Concerns:
//   Exercise the output of the JsonPrettyPrinter.
//
// Testing:
//   JsonPrettyPrinter::printOffsetsNotFound
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PRETTY OFFSETS TEST");

    {
        // Test empty OffsetsVec

        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_PRETTY,
                resultStream,
                bmqtst::TestHelperUtil::allocator());
            printer->printOffsetsNotFound(
                OffsetsVec(bmqtst::TestHelperUtil::allocator()));
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(resultStream.str(),
                         "{\n  \"OffsetsNotFound\": [\n  ]\n}\n");
    }

    {
        // Test not empty OffsetsVec

        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_PRETTY,
                resultStream,
                bmqtst::TestHelperUtil::allocator());
            expectedStream << "{\n  \"OffsetsNotFound\": [";
            OffsetsVec vec(bmqtst::TestHelperUtil::allocator());
            for (bsls::Types::Int64 i = 0; i < 10; ++i) {
                vec.push_back(i);
                if (i != 0) {
                    expectedStream << ",";
                }
                expectedStream << "\n    " << i;
            }
            expectedStream << "\n  ]\n}\n";
            printer->printOffsetsNotFound(vec);
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

static void test13_jsonPrettyCompositesTest()
// ------------------------------------------------------------------------
// JSON PRETTY COMPOSITES TEST
//
// Concerns:
//   Exercise the output of the JsonPrettyPrinter.
//
// Testing:
//   JsonPrettyPrinter::printCompositesNotFound
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PRETTY COMPOSITES TEST");

    {
        // Test empty CompositesVec

        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_PRETTY,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            printer->printCompositesNotFound(
                CompositesVec(bmqtst::TestHelperUtil::allocator()));
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(resultStream.str(),
                         "{\n  \"SequenceNumbersNotFound\": [\n  ]\n}\n");
    }

    {
        // Test not empty CompositesVec

        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        {
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_PRETTY,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            // Prepare expected output
            expectedStream << "{\n  \"SequenceNumbersNotFound\": [";

            CompositesVec vec(bmqtst::TestHelperUtil::allocator());
            for (unsigned int i = 1; i < 11; ++i) {
                vec.emplace_back(CompositeSequenceNumber(i, 2lu * i));
                if (i > 1) {
                    expectedStream << ",";
                }
                expectedStream << "\n    {\"leaseId\": \"" << i
                               << "\", \"sequenceNumber\": \"" << (2lu * i)
                               << "\"}";
            }
            expectedStream << "\n  ]\n}\n";
            printer->printCompositesNotFound(vec);
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

static void test14_jsonPrettyFooterTest()
// ------------------------------------------------------------------------
// JSON PRETTY FOOTER TEST
//
// Concerns:
//   Exercise the output of the JsonPrettyPrinter.
//
// Testing:
//   JsonPrettyPrinter::printFooter
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PRETTY FOOTER TEST");

    {
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_PRETTY,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            Parameters::ProcessRecordTypes recordTypes;
            printer->printFooter(0u, 0ul, 0ul, recordTypes);
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(resultStream.str(), "{\n\n}\n");
    }

    {
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_PRETTY,
                resultStream,
                bmqtst::TestHelperUtil::allocator());
            Parameters::ProcessRecordTypes recordTypes;
            recordTypes.d_message   = true;
            recordTypes.d_queueOp   = true;
            recordTypes.d_journalOp = true;
            printer->printFooter(0u, 0ul, 0ul, recordTypes);

            // Prepare expected output
            expectedStream << "{\n"
                           << "  \"TotalMessages\": \"0\",\n"
                           << "  \"QueueOpRecords\": \"0\",\n"
                           << "  \"JournalOpRecords\": \"0\"\n"
                           << "}\n";
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_PRETTY,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            Parameters::ProcessRecordTypes recordTypes;
            recordTypes.d_message   = true;
            recordTypes.d_queueOp   = true;
            recordTypes.d_journalOp = true;
            printer->printFooter(11u, 22ul, 33ul, recordTypes);

            // Prepare expected output
            expectedStream << "{\n"
                           << "  \"TotalMessages\": \"11\",\n"
                           << "  \"QueueOpRecords\": \"22\",\n"
                           << "  \"JournalOpRecords\": \"33\"\n"
                           << "}\n";
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

static void test15_jsonPrettyOutstandingTest()
// ------------------------------------------------------------------------
// JSON PRETTY OUTSTANDING TEST
//
// Concerns:
//   Exercise the output of the JsonPrettyPrinter.
//
// Testing:
//   JsonPrettyPrinter::printOutstandingRatio
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PRETTY OUTSTANDING TEST");

    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    {
        // Create printer
        bsl::shared_ptr<Printer> printer = createPrinter(
            Parameters::PrintMode::e_JSON_PRETTY,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
        printer->printOutstandingRatio(35, 7ul, 20ul);

        // Prepare expected output
        expectedStream << "{\n"
                       << "  \"OutstandingRatio\": \"35\"\n"
                       << "}\n";
    }
    bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
    bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
    const int rc = bdljsn::JsonUtil::read(&json, &error, resultStream.str());
    BMQTST_ASSERT_D(error, (rc == 0));
    BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
}

static void test16_jsonPrettySummaryTest()
// ------------------------------------------------------------------------
// JSON PRETTY SUMMARY TEST
//
// Concerns:
//   Exercise the output of the JsonPrettyPrinter.
//
// Testing:
//   JsonPrettyPrinter::printMessageSummary
//   JsonPrettyPrinter::printQueueOpSummary
//   JsonPrettyPrinter::printJournalOpSummary
//   JsonPrettyPrinter::printRecordSummary
//   JsonPrettyPrinter::printJournalOpSummary
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PRETTY SUMMARY TEST");

    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Disable default allocator check for this test because
    // bdljsn::JsonUtil::read()
    // uses default allocator in the first two sub-tests.

    {
        // Test no message summary
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        // Prepare expected output
        expectedStream << "{\n"
                       << "  \"TotalMessagesNumber\": \"0\",\n"
                       << "  \"PartiallyConfirmedMessagesNumber\": \"1\",\n"
                       << "  \"ConfirmedMessagesNumber\": \"2\",\n"
                       << "  \"OutstandingMessagesNumber\": \"3\"\n"
                       << "}\n";
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_PRETTY,
                resultStream,
                bmqtst::TestHelperUtil::allocator());
            printer->printMessageSummary(0, 1, 2, 3);
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        int rc = bdljsn::JsonUtil::read(&json, &error, resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test QueueOp summary
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_PRETTY,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            // Prepare expected output
            expectedStream << "{\n"
                           << "  \"TotalQueueOperationsNumber\": \"123\",\n"
                           << "  \"PurgeOperationsNumber\": \"1\",\n"
                           << "  \"CreationOperationsNumber\": \"2\",\n"
                           << "  \"DeletionOperationsNumber\": \"3\",\n"
                           << "  \"AdditionOperationsNumber\": \"4\"\n"
                           << "}\n";
            QueueOpCountsVec vec(mqbs::QueueOpType::e_ADDITION + 1,
                                 0,
                                 bmqtst::TestHelperUtil::allocator());
            vec[mqbs::QueueOpType::e_PURGE]    = 1ul;
            vec[mqbs::QueueOpType::e_CREATION] = 2ul;
            vec[mqbs::QueueOpType::e_DELETION] = 3ul;
            vec[mqbs::QueueOpType::e_ADDITION] = 4ul;
            printer->printQueueOpSummary(123, vec);
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        int rc = bdljsn::JsonUtil::read(&json, &error, resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test JournalOp summary
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_PRETTY,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            // Prepare expected output
            expectedStream << "{\n  \"JournalOperationsNumber\": \"123\"\n}\n";
            printer->printJournalOpSummary(123u);
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        int rc = bdljsn::JsonUtil::read(&json, &error, resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_PRETTY,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            // Prepare expected output
            expectedStream
                << "{\n"
                << "  \"TotalRecordsNumber\": \"123\",\n"
                << "  \"PerQueueRecordsNumber\": [\n"
                << "    {\n"
                << "      \"Queue Key\": \"5175657565\",\n"
                << "      \"Queue URI\": \"QueueUri1\",\n"
                << "      \"Total Records\": \"12345\",\n"
                << "      \"Num Queue Op Records\": \"78\",\n"
                << "      \"Num Message Records\": \"12\",\n"
                << "      \"Num Confirm Records\": \"34\",\n"
                << "      \"Num Records Per App\": \"AppId2=99 AppId1=9 \",\n"
                << "      \"Num Delete Records\": \"56\"\n"
                << "    }\n"
                << "  ]\n"
                << "}\n";

            QueueDetailsMap map(bmqtst::TestHelperUtil::allocator());
            QueueDetails&   details =
                map.emplace(mqbu::StorageKey(
                                mqbu::StorageKey::BinaryRepresentation(),
                                "Queue1"),
                            QueueDetails(bmqtst::TestHelperUtil::allocator()))
                    .first->second;
            details.d_recordsNumber        = 12345;
            details.d_messageRecordsNumber = 12;
            details.d_confirmRecordsNumber = 34;
            details.d_deleteRecordsNumber  = 56;
            details.d_queueOpRecordsNumber = 78;
            details.d_queueUri             = "QueueUri1";
            QueueDetails::AppDetails& appDetails1 =
                details.d_appDetailsMap
                    .emplace(mqbu::StorageKey(
                                 mqbu::StorageKey::BinaryRepresentation(),
                                 "App1"),
                             QueueDetails::AppDetails())
                    .first->second;
            appDetails1.d_appId         = "AppId1";
            appDetails1.d_recordsNumber = 9;

            QueueDetails::AppDetails& appDetails2 =
                details.d_appDetailsMap
                    .emplace(mqbu::StorageKey(
                                 mqbu::StorageKey::BinaryRepresentation(),
                                 "App2"),
                             QueueDetails::AppDetails())
                    .first->second;
            appDetails2.d_appId         = "AppId2";
            appDetails2.d_recordsNumber = 99;

            printer->printRecordSummary(123, map);
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

static void test17_jsonLineGuidTest()
// ------------------------------------------------------------------------
// JSON LINE GUID TEST
//
// Concerns:
//   Exercise the output of the JsonLinePrinter.
//
// Testing:
//   JsonLinePrinter::printGuid
//   JsonLinePrinter::printGuidNotFound
//   JsonLinePrinter::printGuidsNotFound
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LINE GUID TEST");

    {
        // Test printGuid

        bmqt::MessageGUID guid;
        mqbu::MessageGUIDUtil::generateGUID(&guid);

        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_LINE,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            printer->printGuid(guid);

            // Prepare expected output
            expectedStream << "{\n"
                           << "  \"Records\": [\n"
                           << "    \"" << guid << "\"\n"
                           << "  ]\n"
                           << "}\n";
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test printGuidNotFound

        bmqt::MessageGUID guid;
        mqbu::MessageGUIDUtil::generateGUID(&guid);

        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_LINE,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            printer->printGuidNotFound(guid);

            // Prepare expected output
            expectedStream << "{\n"
                           << "  \"Records\": [\n"
                           << "    {\"LogicError\" : \"guid " << guid
                           << " not found\"}\n"
                           << "  ]\n"
                           << "}\n";
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test printGuid and printGuidNotFound in combination

        bmqt::MessageGUID guid1, guid2;
        mqbu::MessageGUIDUtil::generateGUID(&guid1);
        mqbu::MessageGUIDUtil::generateGUID(&guid2);

        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_LINE,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            printer->printGuid(guid1);
            printer->printGuidNotFound(guid2);

            // Prepare expected output
            expectedStream << "{\n"
                           << "  \"Records\": [\n"
                           << "    \"" << guid1 << "\",\n"
                           << "    {\"LogicError\" : \"guid " << guid2
                           << " not found\"}\n"
                           << "  ]\n"
                           << "}\n";
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    // Test printGuidsNotFound

    {
        // Test empty GUIDs list
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_LINE,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            // Prepare argument for the test and expected output
            bsl::list<bmqt::MessageGUID> guids(
                bmqtst::TestHelperUtil::allocator());

            printer->printGuidsNotFound(guids);

            // Prepare expected output
            expectedStream << "{\n  \"GuidsNotFound\": [\n  ]\n}\n";
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test not empty GUIDs list
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_LINE,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            // Prepare argument for the test and expected output
            bsl::list<bmqt::MessageGUID> guids(
                bmqtst::TestHelperUtil::allocator());

            expectedStream << "{\n  \"GuidsNotFound\": [\n";
            for (int i = 0; i < 10; ++i) {
                bmqt::MessageGUID guid;
                mqbu::MessageGUIDUtil::generateGUID(&guid);
                if (i != 0) {
                    expectedStream << ",\n";
                }
                expectedStream << "    \"" << guid << "\"";
                guids.push_back(guid);
            }
            expectedStream << "\n  ]\n}\n";

            printer->printGuidsNotFound(guids);
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

static void test18_jsonLineMessageTest()
// ------------------------------------------------------------------------
// JSON LINE MESSAGE TEST
//
// Concerns:
//   Exercise the output of the JsonLinePrinter.
//
// Testing:
//   JsonLinePrinter::printMessage
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LINE MESSAGE TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 3;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    bsl::vector<bmqt::MessageGUID> expectedGuids(
        bmqtst::TestHelperUtil::allocator());
    JournalFile journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    journalFile.addJournalRecordsWithOutstandingAndConfirmedMessages(
        &records,
        &expectedGuids,
        false);

    // Prepare MessageDetails
    bslma::ManagedPtr<MessageDetails>                details;
    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    for (; recordIter != records.cend(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            details.load(
                new (*bmqtst::TestHelperUtil::allocator())
                    MessageDetails(msg,
                                   msg.header().sequenceNumber(),
                                   recordOffset(msg),
                                   bsl::nullopt,
                                   bmqtst::TestHelperUtil::allocator()),
                bmqtst::TestHelperUtil::allocator());
        }
        else if (rtype == RecordType::e_CONFIRM) {
            const ConfirmRecord& conf =
                *reinterpret_cast<const ConfirmRecord*>(
                    recordIter->second.buffer());
            details->addConfirmRecord(conf,
                                      conf.header().sequenceNumber(),
                                      recordOffset(conf));
        }
        else if (rtype == RecordType::e_DELETION) {
            const DeletionRecord& del =
                *reinterpret_cast<const DeletionRecord*>(
                    recordIter->second.buffer());
            details->addDeleteRecord(del,
                                     del.header().sequenceNumber(),
                                     recordOffset(del));
        }
    }
    const bmqt::MessageGUID& guid =
        details->messageRecord().d_record.messageGUID();

    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

    {
        // Create printer
        bsl::shared_ptr<Printer> printer = createPrinter(
            Parameters::PrintMode::e_JSON_LINE,
            resultStream,
            bmqtst::TestHelperUtil::allocator());

        printer->printMessage(*details);

        // Prepare expected output
        expectedStream
            << "{\n"
            << "  \"Records\": [\n"
            << "    {" << "\"RecordType\": \"MESSAGE\", "
            << "\"Index\": \"1\", " << "\"Offset\": \"44\", "
            << "\"PrimaryLeaseId\": \"100\", " << "\"SequenceNumber\": \"1\", "
            << "\"Timestamp\": \"01JAN1970_00:01:40.000000\", "
            << "\"Epoch\": \"100\", " << "\"QueueKey\": \"6162636465\", "
            << "\"FileKey\": \"3132333435\", " << "\"RefCount\": \"1\", "
            << "\"MsgOffsetDwords\": \"1\", " << "\"GUID\": \"" << guid
            << "\", " << "\"Crc32c\": \"1\"" << "},\n"
            << "    {" << "\"RecordType\": \"CONFIRM\", "
            << "\"Index\": \"2\", " << "\"Offset\": \"104\", "
            << "\"PrimaryLeaseId\": \"100\", " << "\"SequenceNumber\": \"2\", "
            << "\"Timestamp\": \"01JAN1970_00:03:20.000000\", "
            << "\"Epoch\": \"200\", " << "\"QueueKey\": \"6162636465\", "
            << "\"AppKey\": \"6170706964\", " << "\"GUID\": \"" << guid << "\""
            << "},\n"
            << "    {" << "\"RecordType\": \"DELETION\", "
            << "\"Index\": \"3\", " << "\"Offset\": \"164\", "
            << "\"PrimaryLeaseId\": \"100\", " << "\"SequenceNumber\": \"3\", "
            << "\"Timestamp\": \"01JAN1970_00:05:00.000000\", "
            << "\"Epoch\": \"300\", " << "\"QueueKey\": \"6162636465\", "
            << "\"DeletionFlag\": \"IMPLICIT_CONFIRM\", " << "\"GUID\": \""
            << guid << "\"" << "}\n"
            << "  ]\n"
            << "}\n";
    }

    bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
    bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
    const int rc = bdljsn::JsonUtil::read(&json, &error, resultStream.str());
    BMQTST_ASSERT_D(error, (rc == 0));
    BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
}

static void test19_jsonLineRecordsTest()
// ------------------------------------------------------------------------
// JSON LINE RECORDS TEST
//
// Concerns:
//   Exercise the output of the JsonLinePrinter.
//
// Testing:
//   JsonLinePrinter::printQueueOpRecord
//   JsonLinePrinter::printJournalOpRecord
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LINE RECORDS TEST");

    {
        // Test printQueueOpRecord

        // Simulate journal file
        JournalFile journalFile(1, bmqtst::TestHelperUtil::allocator());
        JournalFile::RecordBufferType buf = journalFile.makeQueueOpRecord(100,
                                                                          1);

        const QueueOpRecord& queueOpRecord =
            *reinterpret_cast<const QueueOpRecord*>(buf.buffer());
        RecordDetails<mqbs::QueueOpRecord> queueOpDetails(
            queueOpRecord,
            12345,
            56789,
            bmqtst::TestHelperUtil::allocator());

        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_LINE,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            printer->printQueueOpRecord(queueOpDetails);

            // Prepare expected output
            expectedStream << "{\n"
                           << "  \"Records\": [\n"
                           << "    {" << "\"RecordType\": \"QUEUE_OP\", "
                           << "\"Index\": \"12345\", "
                           << "\"Offset\": \"56789\", "
                           << "\"PrimaryLeaseId\": \"100\", "
                           << "\"SequenceNumber\": \"1\", "
                           << "\"Timestamp\": \"01JAN1970_00:01:40.000000\", "
                           << "\"Epoch\": \"100\", "
                           << "\"QueueKey\": \"6162636465\", "
                           << "\"AppKey\": \"6170706964\", "
                           << "\"QueueOpType\": \"PURGE\"" << "}\n"
                           << "  ]\n"
                           << "}\n";
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test printJournalOpRecord

        // Simulate journal file
        JournalFile journalFile(1, bmqtst::TestHelperUtil::allocator());
        JournalFile::RecordBufferType buf =
            journalFile.makeJournalOpRecord(100, 1);

        const JournalOpRecord& journalOpRecord =
            *reinterpret_cast<const JournalOpRecord*>(buf.buffer());
        RecordDetails<mqbs::JournalOpRecord> journalOpDetails(
            journalOpRecord,
            12345,
            56789,
            bmqtst::TestHelperUtil::allocator());

        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_LINE,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            printer->printJournalOpRecord(journalOpDetails);

            // Prepare expected output
            expectedStream << "{\n"
                           << "  \"Records\": [\n"
                           << "    {" << "\"RecordType\": \"JOURNAL_OP\", "
                           << "\"Index\": \"12345\", "
                           << "\"Offset\": \"56789\", "
                           << "\"PrimaryLeaseId\": \"100\", "
                           << "\"SequenceNumber\": \"1\", "
                           << "\"Timestamp\": \"01JAN1970_00:01:40.000000\", "
                           << "\"Epoch\": \"100\", "
                           << "\"JournalOpType\": \"SYNCPOINT\", "
                           << "\"SyncPointType\": \"REGULAR\", "
                           << "\"SyncPtPrimaryLeaseId\": \"25\", "
                           << "\"SyncPtSequenceNumber\": \"1234567\", "
                           << "\"PrimaryNodeId\": \"121\", "
                           << "\"DataFileOffsetDwords\": \"8800\"" << "}\n"
                           << "  ]\n"
                           << "}\n";
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

void test20_jsonLineOffsetsTest()
// ------------------------------------------------------------------------
// JSON LINE OFFSETS TEST
//
// Concerns:
//   Exercise the output of the JsonLinePrinter.
//
// Testing:
//   JsonLinePrinter::printOffsetsNotFound
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LINE OFFSETS TEST");

    {
        // Test empty OffsetsVec

        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_LINE,
                resultStream,
                bmqtst::TestHelperUtil::allocator());
            printer->printOffsetsNotFound(
                OffsetsVec(bmqtst::TestHelperUtil::allocator()));
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(resultStream.str(),
                         "{\n  \"OffsetsNotFound\": [\n  ]\n}\n");
    }

    {
        // Test not empty OffsetsVec

        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_LINE,
                resultStream,
                bmqtst::TestHelperUtil::allocator());
            expectedStream << "{\n  \"OffsetsNotFound\": [";
            OffsetsVec vec(bmqtst::TestHelperUtil::allocator());
            for (bsls::Types::Int64 i = 0; i < 10; ++i) {
                vec.push_back(i);
                if (i != 0) {
                    expectedStream << ",";
                }
                expectedStream << "\n    " << i;
            }
            expectedStream << "\n  ]\n}\n";
            printer->printOffsetsNotFound(vec);
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

static void test21_jsonLineCompositesTest()
// ------------------------------------------------------------------------
// JSON LINE COMPOSITES TEST
//
// Concerns:
//   Exercise the output of the JsonLinePrinter.
//
// Testing:
//   JsonLinePrinter::printCompositesNotFound
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LINE COMPOSITES TEST");

    {
        // Test empty CompositesVec

        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_LINE,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            printer->printCompositesNotFound(
                CompositesVec(bmqtst::TestHelperUtil::allocator()));
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(resultStream.str(),
                         "{\n  \"SequenceNumbersNotFound\": [\n  ]\n}\n");
    }

    {
        // Test not empty CompositesVec

        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        {
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_LINE,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            // Prepare expected output
            expectedStream << "{\n  \"SequenceNumbersNotFound\": [";

            CompositesVec vec(bmqtst::TestHelperUtil::allocator());
            for (unsigned int i = 1; i < 11; ++i) {
                vec.emplace_back(CompositeSequenceNumber(i, 2lu * i));
                if (i > 1) {
                    expectedStream << ",";
                }
                expectedStream << "\n    {\"leaseId\": \"" << i
                               << "\", \"sequenceNumber\": \"" << (2lu * i)
                               << "\"}";
            }
            expectedStream << "\n  ]\n}\n";
            printer->printCompositesNotFound(vec);
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

static void test22_jsonLineFooterTest()
// ------------------------------------------------------------------------
// JSON LINE FOOTER TEST
//
// Concerns:
//   Exercise the output of the JsonLinePrinter.
//
// Testing:
//   JsonLinePrinter::printFooter
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LINE FOOTER TEST");

    {
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_LINE,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            Parameters::ProcessRecordTypes recordTypes;
            printer->printFooter(0u, 0ul, 0ul, recordTypes);
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(resultStream.str(), "{\n\n}\n");
    }

    {
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_LINE,
                resultStream,
                bmqtst::TestHelperUtil::allocator());
            Parameters::ProcessRecordTypes recordTypes;
            recordTypes.d_message   = true;
            recordTypes.d_queueOp   = true;
            recordTypes.d_journalOp = true;
            printer->printFooter(0u, 0ul, 0ul, recordTypes);

            // Prepare expected output
            expectedStream << "{\n"
                           << "  \"TotalMessages\": \"0\",\n"
                           << "  \"QueueOpRecords\": \"0\",\n"
                           << "  \"JournalOpRecords\": \"0\"\n"
                           << "}\n";
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_LINE,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            Parameters::ProcessRecordTypes recordTypes;
            recordTypes.d_message   = true;
            recordTypes.d_queueOp   = true;
            recordTypes.d_journalOp = true;
            printer->printFooter(11u, 22ul, 33ul, recordTypes);

            // Prepare expected output
            expectedStream << "{\n"
                           << "  \"TotalMessages\": \"11\",\n"
                           << "  \"QueueOpRecords\": \"22\",\n"
                           << "  \"JournalOpRecords\": \"33\"\n"
                           << "}\n";
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

static void test23_jsonLineOutstandingTest()
// ------------------------------------------------------------------------
// JSON LINE OUTSTANDING TEST
//
// Concerns:
//   Exercise the output of the JsonLinePrinter.
//
// Testing:
//   JsonLinePrinter::printOutstandingRatio
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LINE OUTSTANDING TEST");

    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    {
        // Create printer
        bsl::shared_ptr<Printer> printer = createPrinter(
            Parameters::PrintMode::e_JSON_LINE,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
        printer->printOutstandingRatio(35, 7ul, 20ul);

        // Prepare expected output
        expectedStream << "{\n"
                       << "  \"OutstandingRatio\": \"35\"\n"
                       << "}\n";
    }
    bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
    bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
    const int rc = bdljsn::JsonUtil::read(&json, &error, resultStream.str());
    BMQTST_ASSERT_D(error, (rc == 0));
    BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
}

static void test24_jsonLineSummaryTest()
// ------------------------------------------------------------------------
// JSON LINE SUMMARY TEST
//
// Concerns:
//   Exercise the output of the JsonLinePrinter.
//
// Testing:
//   JsonLinePrinter::printMessageSummary
//   JsonLinePrinter::printQueueOpSummary
//   JsonLinePrinter::printJournalOpSummary
//   JsonLinePrinter::printRecordSummary
//   JsonLinePrinter::printJournalOpSummary
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LINE SUMMARY TEST");

    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Disable default allocator check for this test because
    // bdljsn::JsonUtil::read()
    // uses default allocator in the first two sub-tests.

    {
        // Test no message summary
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        // Prepare expected output
        expectedStream << "{\n"
                       << "  \"TotalMessagesNumber\": \"0\",\n"
                       << "  \"PartiallyConfirmedMessagesNumber\": \"1\",\n"
                       << "  \"ConfirmedMessagesNumber\": \"2\",\n"
                       << "  \"OutstandingMessagesNumber\": \"3\"\n"
                       << "}\n";
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_LINE,
                resultStream,
                bmqtst::TestHelperUtil::allocator());
            printer->printMessageSummary(0, 1, 2, 3);
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        int rc = bdljsn::JsonUtil::read(&json, &error, resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test QueueOp summary
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_LINE,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            // Prepare expected output
            expectedStream << "{\n"
                           << "  \"TotalQueueOperationsNumber\": \"123\",\n"
                           << "  \"PurgeOperationsNumber\": \"1\",\n"
                           << "  \"CreationOperationsNumber\": \"2\",\n"
                           << "  \"DeletionOperationsNumber\": \"3\",\n"
                           << "  \"AdditionOperationsNumber\": \"4\"\n"
                           << "}\n";
            QueueOpCountsVec vec(mqbs::QueueOpType::e_ADDITION + 1,
                                 0,
                                 bmqtst::TestHelperUtil::allocator());
            vec[mqbs::QueueOpType::e_PURGE]    = 1ul;
            vec[mqbs::QueueOpType::e_CREATION] = 2ul;
            vec[mqbs::QueueOpType::e_DELETION] = 3ul;
            vec[mqbs::QueueOpType::e_ADDITION] = 4ul;
            printer->printQueueOpSummary(123, vec);
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        int rc = bdljsn::JsonUtil::read(&json, &error, resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        // Test JournalOp summary
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_LINE,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            // Prepare expected output
            expectedStream << "{\n  \"JournalOperationsNumber\": \"123\"\n}\n";
            printer->printJournalOpSummary(123u);
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        int rc = bdljsn::JsonUtil::read(&json, &error, resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }

    {
        bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
        {
            // Create printer
            bsl::shared_ptr<Printer> printer = createPrinter(
                Parameters::PrintMode::e_JSON_LINE,
                resultStream,
                bmqtst::TestHelperUtil::allocator());

            // Prepare expected output
            expectedStream
                << "{\n"
                << "  \"TotalRecordsNumber\": \"123\",\n"
                << "  \"PerQueueRecordsNumber\": [\n"
                << "    {" << "\"Queue Key\": \"5175657565\", "
                << "\"Queue URI\": \"QueueUri1\", "
                << "\"Total Records\": \"12345\", "
                << "\"Num Queue Op Records\": \"78\", "
                << "\"Num Message Records\": \"12\", "
                << "\"Num Confirm Records\": \"34\", "
                << "\"Num Records Per App\": \"AppId2=99 AppId1=9 \", "
                << "\"Num Delete Records\": \"56\"" << "}\n"
                << "  ]\n"
                << "}\n";

            QueueDetailsMap map(bmqtst::TestHelperUtil::allocator());
            QueueDetails&   details =
                map.emplace(mqbu::StorageKey(
                                mqbu::StorageKey::BinaryRepresentation(),
                                "Queue1"),
                            QueueDetails(bmqtst::TestHelperUtil::allocator()))
                    .first->second;
            details.d_recordsNumber        = 12345;
            details.d_messageRecordsNumber = 12;
            details.d_confirmRecordsNumber = 34;
            details.d_deleteRecordsNumber  = 56;
            details.d_queueOpRecordsNumber = 78;
            details.d_queueUri             = "QueueUri1";
            QueueDetails::AppDetails& appDetails1 =
                details.d_appDetailsMap
                    .emplace(mqbu::StorageKey(
                                 mqbu::StorageKey::BinaryRepresentation(),
                                 "App1"),
                             QueueDetails::AppDetails())
                    .first->second;
            appDetails1.d_appId         = "AppId1";
            appDetails1.d_recordsNumber = 9;

            QueueDetails::AppDetails& appDetails2 =
                details.d_appDetailsMap
                    .emplace(mqbu::StorageKey(
                                 mqbu::StorageKey::BinaryRepresentation(),
                                 "App2"),
                             QueueDetails::AppDetails())
                    .first->second;
            appDetails2.d_appId         = "AppId2";
            appDetails2.d_recordsNumber = 99;

            printer->printRecordSummary(123, map);
        }
        bdljsn::Json  json(bmqtst::TestHelperUtil::allocator());
        bdljsn::Error error(bmqtst::TestHelperUtil::allocator());
        const int     rc = bdljsn::JsonUtil::read(&json,
                                              &error,
                                              resultStream.str());
        BMQTST_ASSERT_D(error, (rc == 0));
        BMQTST_ASSERT_EQ(expectedStream.str(), resultStream.str());
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_humanReadableGuidTest(); break;
    case 2: test2_humanReadableMessageTest(); break;
    case 3: test3_humanReadableRecordsTest(); break;
    case 4: test4_humanReadableOffsetsTest(); break;
    case 5: test5_humanReadableCompositesTest(); break;
    case 6: test6_humanReadableFooterTest(); break;
    case 7: test7_humanReadableOutstandingTest(); break;
    case 8: test8_humanReadableSummaryTest(); break;
    case 9: test9_jsonPrettyGuidTest(); break;
    case 10: test10_jsonPrettyMessageTest(); break;
    case 11: test11_jsonPrettyRecordsTest(); break;
    case 12: test12_jsonPrettyOffsetsTest(); break;
    case 13: test13_jsonPrettyCompositesTest(); break;
    case 14: test14_jsonPrettyFooterTest(); break;
    case 15: test15_jsonPrettyOutstandingTest(); break;
    case 16: test16_jsonPrettySummaryTest(); break;
    case 17: test17_jsonLineGuidTest(); break;
    case 18: test18_jsonLineMessageTest(); break;
    case 19: test19_jsonLineRecordsTest(); break;
    case 20: test20_jsonLineOffsetsTest(); break;
    case 21: test21_jsonLineCompositesTest(); break;
    case 22: test22_jsonLineFooterTest(); break;
    case 23: test23_jsonLineOutstandingTest(); break;
    case 24: test24_jsonLineSummaryTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
