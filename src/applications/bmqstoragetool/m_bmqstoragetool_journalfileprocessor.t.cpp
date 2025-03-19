// Copyright 2023 Bloomberg Finance L.P.
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
#include "m_bmqstoragetool_compositesequencenumber.h"
#include "m_bmqstoragetool_parameters.h"
#include <m_bmqstoragetool_commandprocessorfactory.h>
#include <m_bmqstoragetool_filemanagermock.h>
#include <m_bmqstoragetool_journalfileprocessor.h>
#include <m_bmqstoragetool_printermock.h>
#include <m_bmqstoragetool_searchresultfactory.h>

// MQB
#include <mqbs_mappedfiledescriptor.h>
#include <mqbs_memoryblock.h>
#include <mqbs_offsetptr.h>
#include <mqbu_messageguidutil.h>

// BMQ
#include <bmqu_memoutstream.h>

// BDE
#include <bsl_list.h>
#include <bsl_utility.h>
#include <bslma_allocator.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace m_bmqstoragetool;
using namespace bsl;
using namespace mqbs;
using ::testing::_;
using ::testing::Eq;
using ::testing::Field;
using ::testing::Invoke;
using ::testing::Property;
using ::testing::Sequence;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

namespace {

const size_t k_HEADER_SIZE = sizeof(mqbs::FileHeader) +
                             sizeof(mqbs::JournalFileHeader);
// Size of a journal file header

typedef bsl::list<bmqt::MessageGUID> GuidsList;
// List of message guids.

bslma::ManagedPtr<CommandProcessor>
createCommandProcessor(const Parameters*               params,
                       const bsl::shared_ptr<Printer>& printer,
                       bslma::ManagedPtr<FileManager>& fileManager,
                       bsl::ostream&                   ostream,
                       bslma::Allocator*               allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(params);

    bslma::Allocator* alloc = bslma::Default::allocator(allocator);

    // Create payload dumper
    bslma::ManagedPtr<PayloadDumper> payloadDumper;
    if (params->d_dumpPayload) {
        payloadDumper.load(new (*alloc)
                               PayloadDumper(ostream,
                                             fileManager->dataFileIterator(),
                                             params->d_dumpLimit,
                                             alloc),
                           alloc);
    }

    // Create searchResult for given 'params'.
    bsl::shared_ptr<SearchResult> searchResult =
        SearchResultFactory::createSearchResult(params,
                                                fileManager,
                                                printer,
                                                payloadDumper,
                                                alloc);
    // Create commandProcessor.
    bslma::ManagedPtr<CommandProcessor> commandProcessor(
        new (*alloc) JournalFileProcessor(params,
                                          fileManager,
                                          searchResult,
                                          ostream,
                                          alloc),
        alloc);
    return commandProcessor;
}

/// Value semantic type representing data message parameters.
struct DataMessage {
    int         d_line;
    const char* d_appData_p;
    const char* d_options_p;
};

/// Allocate in memory storage data file and generate sequence of data
/// records using the specified arguments. Return pointer to allocated
/// memory.
char* addDataRecords(bslma::Allocator*          ta,
                     MappedFileDescriptor*      mfd,
                     FileHeader*                fileHeader,
                     const DataMessage*         messages,
                     const unsigned int         numMessages,
                     bsl::vector<unsigned int>& messageOffsets)
{
    bsls::Types::Uint64 currPos = 0;
    const unsigned int  dhSize  = sizeof(DataHeader);
    unsigned int totalSize      = sizeof(FileHeader) + sizeof(DataFileHeader);

    // Have to compute the 'totalSize' we need for the 'MemoryBlock' based on
    // the padding that we need for each record.

    for (unsigned int i = 0; i < numMessages; i++) {
        unsigned int optionsLen = static_cast<unsigned int>(
            bsl::strlen(messages[i].d_options_p));
        BSLS_ASSERT_OPT(0 == optionsLen % bmqp::Protocol::k_WORD_SIZE);

        unsigned int appDataLen = static_cast<unsigned int>(
            bsl::strlen(messages[i].d_appData_p));
        int appDataPadding = 0;
        bmqp::ProtocolUtil::calcNumDwordsAndPadding(&appDataPadding,
                                                    appDataLen + optionsLen +
                                                        dhSize);

        totalSize += dhSize + appDataLen + appDataPadding + optionsLen;
    }

    // Allocate the memory now.
    char* p = static_cast<char*>(ta->allocate(totalSize));

    // Create the 'MemoryBlock'
    MemoryBlock block(p, totalSize);

    // Set the MFD
    mfd->setFd(-1);
    mfd->setBlock(block);
    mfd->setFileSize(totalSize);

    // Add the entries to the block.
    OffsetPtr<FileHeader> fh(block, currPos);
    new (fh.get()) FileHeader();
    fh->setHeaderWords(sizeof(FileHeader) / bmqp::Protocol::k_WORD_SIZE);
    fh->setMagic1(FileHeader::k_MAGIC1);
    fh->setMagic2(FileHeader::k_MAGIC2);
    currPos += sizeof(FileHeader);

    OffsetPtr<DataFileHeader> dfh(block, currPos);
    new (dfh.get()) DataFileHeader();
    dfh->setHeaderWords(sizeof(DataFileHeader) / bmqp::Protocol::k_WORD_SIZE);
    currPos += sizeof(DataFileHeader);

    for (unsigned int i = 0; i < numMessages; i++) {
        messageOffsets.push_back(
            static_cast<unsigned int>(currPos / bmqp::Protocol::k_DWORD_SIZE));

        OffsetPtr<DataHeader> dh(block, currPos);
        new (dh.get()) DataHeader();

        unsigned int optionsLen = static_cast<unsigned int>(
            bsl::strlen(messages[i].d_options_p));
        dh->setOptionsWords(optionsLen / bmqp::Protocol::k_WORD_SIZE);
        currPos += sizeof(DataHeader);

        char* destination = reinterpret_cast<char*>(block.base() + currPos);
        bsl::memcpy(destination, messages[i].d_options_p, optionsLen);
        currPos += optionsLen;
        destination += optionsLen;

        unsigned int appDataLen = static_cast<unsigned int>(
            bsl::strlen(messages[i].d_appData_p));
        int appDataPad = 0;
        bmqp::ProtocolUtil::calcNumDwordsAndPadding(&appDataPad,
                                                    appDataLen + optionsLen +
                                                        dhSize);

        bsl::memcpy(destination, messages[i].d_appData_p, appDataLen);
        currPos += appDataLen;
        destination += appDataLen;
        bmqp::ProtocolUtil::appendPaddingDwordRaw(destination, appDataPad);
        currPos += appDataPad;

        unsigned int messageOffset = dh->headerWords() +
                                     ((appDataLen + appDataPad + optionsLen) /
                                      bmqp::Protocol::k_WORD_SIZE);
        dh->setMessageWords(messageOffset);
    }

    *fileHeader = *fh;

    return p;
}

/// Output the specified `messageGUID` as a string to the specified
/// `ostream`.
void outputGuidString(bsl::ostream&            ostream,
                      const bmqt::MessageGUID& messageGUID,
                      bool                     addNewLine = true)
{
    ostream << messageGUID;
    if (addNewLine)
        ostream << bsl::endl;
}

enum ProcessRecordTypeFlags {
    /// Do not process any record types
    e_EMPTY = 0,

    /// Enable processing of message records
    e_MESSAGE = 1,
    /// Enable processing of Queue Op records
    e_QUEUE_OP = 2,
    /// Enable processing of Journal Op records
    e_JOURNAL_OP = 4
};

/// Helper function to instantiate test Parameters
Parameters createTestParameters(int flags = e_MESSAGE)
{
    Parameters params(
        CommandLineArguments(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());

    params.d_processRecordTypes.d_message   = flags & e_MESSAGE;
    params.d_processRecordTypes.d_queueOp   = flags & e_QUEUE_OP;
    params.d_processRecordTypes.d_journalOp = flags & e_JOURNAL_OP;

    return params;
}

/// Helper matcher to check if the message is confirmed. Return 'true' if the
/// specified 'expectedGuid' is equal to the guid of the message record of the
/// invoked MessageDetails 'arg', the 'confirmRecords' vector is not empty and
/// the 'deleteRecord' optional has value. Return 'false' otherwise.
MATCHER_P(ConfirmedGuidMatcher, expectedGuid, "")
{
    bmqu::MemOutStream ss(bmqtst::TestHelperUtil::allocator());
    outputGuidString(ss, expectedGuid);
    bsl::string guidStr(ss.str(), bmqtst::TestHelperUtil::allocator());
    *result_listener << "GUID : " << guidStr;
    return arg.messageRecord().d_record.messageGUID() == expectedGuid &&
           !arg.confirmRecords().empty() && arg.deleteRecord().has_value();
}

/// Helper matcher to check if the message is not confirmed. Return 'true' if
/// the 'deleteRecord' optional has value. Return 'false' otherwise.
MATCHER(NotConfirmedGuidMatcher, "")
{
    bmqu::MemOutStream ss(bmqtst::TestHelperUtil::allocator());
    outputGuidString(ss, arg.messageRecord().d_record.messageGUID());
    bsl::string guidStr(ss.str(), bmqtst::TestHelperUtil::allocator());
    *result_listener << "GUID : " << guidStr;
    return !arg.deleteRecord().has_value();
}

/// Helper matcher to check the offset of the record. Return 'true' if the
/// specified 'expectedOffset' is equal to the offset of the RecordDetails
/// 'arg'.
MATCHER_P(OffsetMatcher, expectedOffset, "")
{
    *result_listener << "Offset : " << arg.d_recordOffset;
    return arg.d_recordOffset == expectedOffset;
}

/// Calculate record offset
template <typename RECORD_TYPE>
bsls::Types::Uint64 recordOffset(const RECORD_TYPE& record)
{
    BSLS_ASSERT_SAFE(record.header().sequenceNumber() > 0);
    return k_HEADER_SIZE + (record.header().sequenceNumber() - 1) *
                               mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
}

/// Helper matcher to check the offset of the record. Return 'true' if the
/// specified 'expectedOffset' is equal to the offset of the RecordDetails
/// 'arg'.
MATCHER_P(SequenceNumberMatcher, expectedSequenceNumber, "")
{
    CompositeSequenceNumber seq(arg.d_record.header().primaryLeaseId(),
                                arg.d_record.header().sequenceNumber());
    *result_listener << "SequenceNumber : " << seq;
    return seq == expectedSequenceNumber;
}

}  // close unnamed namespace

namespace BloombergLP::m_bmqstoragetool {

// Printers for easier debugging. GMock will use these functions to log test
// failures.

/// Print the specified 'details' to the specified 'stream'
void PrintTo(const QueueDetails& details, bsl::ostream* stream)
{
    *stream << "\trecordsNumber : " << details.d_recordsNumber
            << "\n\tmessageRecordsNumber : " << details.d_messageRecordsNumber
            << "\n\tconfirmRecordsNumber : " << details.d_confirmRecordsNumber
            << "\n\tdeleteRecordsNumber : " << details.d_deleteRecordsNumber
            << "\n\tqueueOpRecordsNumber : " << details.d_queueOpRecordsNumber
            << "\n\tqueueUri : " << details.d_queueUri << "\n\tAppDetails:\n";
    for (const auto& a : details.d_appDetailsMap) {
        *stream << "\t\t" << a.first << " : { " << a.second.d_appId << " : "
                << a.second.d_recordsNumber << " }\n";
    }
}

/// Print the specified 'detailsMap' to the specified 'stream'
void PrintTo(const QueueDetailsMap& detailsMap, std::ostream* stream)
{
    for (const auto& d : detailsMap) {
        *stream << d.first << " :\n";
        PrintTo(d.second, stream);
    }
}

}

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the tool - output all message GUIDs
//   found in journal file.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 15;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    journalFile.addAllTypesRecords(&records);

    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());

    // Prepare parameters
    Parameters params = createTestParameters();
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    Sequence s;
    // Prepare expected output with list of message GUIDs in Journal file
    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    bsl::size_t foundMessagesCount = 0;
    for (; recordIter != records.end(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            EXPECT_CALL(*printer, printGuid(msg.messageGUID())).InSequence(s);
            foundMessagesCount++;
        }
    }
    EXPECT_CALL(
        *printer,
        printFooter(foundMessagesCount, 0, 0, params.d_processRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test2_searchGuidTest()
// ------------------------------------------------------------------------
// SEARCH GUID TEST
//
// Concerns:
//   Search messages by GUIDs in journal file and output GUIDs.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEARCH GUID");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 15;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    journalFile.addAllTypesRecords(&records);

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare parameters
    Parameters params = createTestParameters();
    // Get list of message GUIDs for searching
    bsl::vector<bsl::string>& searchGuids = params.d_guid;
    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    bsl::size_t msgCnt = 0;
    Sequence    s;
    for (; recordIter != records.end(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            if (msgCnt++ % 2 != 0)
                continue;  // Skip odd messages for test purposes
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            bmqu::MemOutStream ss(bmqtst::TestHelperUtil::allocator());
            ss << msg.messageGUID();
            searchGuids.push_back(
                bsl::string(ss.str(), bmqtst::TestHelperUtil::allocator()));
            EXPECT_CALL(*printer, printGuid(msg.messageGUID())).InSequence(s);
        }
    }
    EXPECT_CALL(
        *printer,
        printFooter(searchGuids.size(), 0, 0, params.d_processRecordTypes))
        .InSequence(s);

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());
    // Run search
    searchProcessor->process();
}

static void test3_searchNonExistingGuidTest()
// ------------------------------------------------------------------------
// SEARCH NON EXISTING GUID TEST
//
// Concerns:
//   Search messages by non existing GUIDs in journal file and output result.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEARCH NON EXISTING GUID");

    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Disable default allocator check for this test because
    // EXPECT_CALL(PrinterMock::printGuidsNotFound(const GuidsList&))
    // uses default allocator

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 15;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    journalFile.addAllTypesRecords(&records);

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare parameters
    Parameters params = createTestParameters();
    // Get list of message GUIDs for searching
    bsl::vector<bsl::string>& searchGuids = params.d_guid;
    GuidsList         notFoundGuids(bmqtst::TestHelperUtil::allocator());
    bmqt::MessageGUID guid;

    for (int i = 0; i < 2; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        bmqu::MemOutStream ss(bmqtst::TestHelperUtil::allocator());
        ss << guid;
        searchGuids.push_back(
            bsl::string(ss.str(), bmqtst::TestHelperUtil::allocator()));
        notFoundGuids.push_back(guid);
    }

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    Sequence s;
    EXPECT_CALL(*printer, printFooter(0, 0, 0, params.d_processRecordTypes))
        .InSequence(s);
    EXPECT_CALL(*printer, printGuidsNotFound(notFoundGuids)).InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test4_searchExistingAndNonExistingGuidTest()
// ------------------------------------------------------------------------
// SEARCH EXISTING AND NON EXISTING GUID TEST
//
// Concerns:
//   Search messages by existing and non existing GUIDs in journal file and
//   output result.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEARCH EXISTING AND NON EXISTING GUID");

    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Disable default allocator check for this test because
    // EXPECT_CALL(PrinterMock::printGuidsNotFound(const GuidsList&))
    // uses default allocator

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 15;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    journalFile.addAllTypesRecords(&records);

    // Prepare parameters
    Parameters params = createTestParameters();

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Get list of message GUIDs for searching
    bsl::vector<bsl::string>& searchGuids = params.d_guid;

    // Get two existing message GUIDs
    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    size_t   msgCnt        = 0;
    size_t   foundGuidsCnt = 0;
    Sequence s;
    for (; recordIter != records.end(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            if (msgCnt++ == 2)
                break;  // Take two GUIDs
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            bmqu::MemOutStream ss(bmqtst::TestHelperUtil::allocator());
            ss << msg.messageGUID();
            searchGuids.push_back(
                bsl::string(ss.str(), bmqtst::TestHelperUtil::allocator()));
            ++foundGuidsCnt;
            EXPECT_CALL(*printer, printGuid(msg.messageGUID())).InSequence(s);
        }
    }

    // Get two non existing message GUIDs
    GuidsList         notFoundGuids(bmqtst::TestHelperUtil::allocator());
    bmqt::MessageGUID guid;
    for (int i = 0; i < 2; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        bmqu::MemOutStream ss(bmqtst::TestHelperUtil::allocator());
        ss << guid;
        searchGuids.push_back(
            bsl::string(ss.str(), bmqtst::TestHelperUtil::allocator()));
        notFoundGuids.push_back(guid);
    }

    EXPECT_CALL(*printer,
                printFooter(foundGuidsCnt, 0, 0, params.d_processRecordTypes))
        .InSequence(s);
    EXPECT_CALL(*printer, printGuidsNotFound(notFoundGuids)).InSequence(s);

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    // Run search
    searchProcessor->process();
}

static void test5_searchOutstandingMessagesTest()
// ------------------------------------------------------------------------
// SEARCH OUTSTANDING MESSAGES TEST
//
// Concerns:
//   Search outstanding (not deleted) messages and output GUIDs.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEARCH OUTSTANDING MESSAGES TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 15;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    JournalFile::GuidVectorType  outstandingGUIDS(
        bmqtst::TestHelperUtil::allocator());
    journalFile.addJournalRecordsWithOutstandingAndConfirmedMessages(
        &records,
        &outstandingGUIDS,
        true);

    // Configure parameters to search outstanding messages
    Parameters params    = createTestParameters();
    params.d_outstanding = true;

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    const size_t messageCount     = k_NUM_RECORDS / 3;
    const int    outstandingRatio = static_cast<int>(bsl::floor(
        float(outstandingGUIDS.size()) / float(messageCount) * 100.0f + 0.5f));

    JournalFile::GuidVectorType::const_iterator guidIt =
        outstandingGUIDS.cbegin();

    Sequence s;
    for (; guidIt != outstandingGUIDS.cend(); ++guidIt) {
        EXPECT_CALL(*printer, printGuid(*guidIt)).InSequence(s);
    }
    EXPECT_CALL(*printer,
                printFooter(outstandingGUIDS.size(),
                            0,
                            0,
                            params.d_processRecordTypes))
        .InSequence(s);
    EXPECT_CALL(*printer,
                printOutstandingRatio(outstandingRatio,
                                      outstandingGUIDS.size(),
                                      messageCount))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test6_searchConfirmedMessagesTest()
// ------------------------------------------------------------------------
// SEARCH CONFIRMED MESSAGES TEST
//
// Concerns:
//   Search confirmed (deleted) messages  in journal file and output GUIDs.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEARCH CONFIRMED MESSAGES TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 15;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    JournalFile::GuidVectorType  confirmedGUIDS(
        bmqtst::TestHelperUtil::allocator());
    journalFile.addJournalRecordsWithOutstandingAndConfirmedMessages(
        &records,
        &confirmedGUIDS,
        false);

    // Configure parameters to search confirmed messages
    Parameters params  = createTestParameters();
    params.d_confirmed = true;

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    const size_t messageCount     = k_NUM_RECORDS / 3;
    const size_t outstandingCount = messageCount - confirmedGUIDS.size();
    const int    outstandingRatio = static_cast<int>(bsl::floor(
        float(outstandingCount) / float(messageCount) * 100.0f + 0.5f));
    JournalFile::GuidVectorType::const_iterator guidIt =
        confirmedGUIDS.cbegin();

    Sequence s;
    for (; guidIt != confirmedGUIDS.cend(); ++guidIt) {
        EXPECT_CALL(*printer, printGuid(*guidIt)).InSequence(s);
    }
    EXPECT_CALL(
        *printer,
        printFooter(confirmedGUIDS.size(), 0, 0, params.d_processRecordTypes))
        .InSequence(s);
    EXPECT_CALL(*printer,
                printOutstandingRatio(outstandingRatio,
                                      outstandingCount,
                                      messageCount))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test7_searchPartiallyConfirmedMessagesTest()
// ------------------------------------------------------------------------
// SEARCH PARTIALLY CONFIRMED MESSAGES TEST
//
// Concerns:
//   Search partially confirmed (at least one confirm) messages in journal
//   file and output GUIDs.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "SEARCH PARTIALLY CONFIRMED MESSAGES TEST");

    // Simulate journal file
    // k_NUM_RECORDS must be multiple 3 plus one to cover all combinations
    // (confirmed, deleted, not confirmed)
    const size_t                 k_NUM_RECORDS = 16;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    JournalFile::GuidVectorType  partiallyConfirmedGUIDS(
        bmqtst::TestHelperUtil::allocator());
    journalFile.addJournalRecordsWithPartiallyConfirmedMessages(
        &records,
        &partiallyConfirmedGUIDS);

    // Configure parameters to search partially confirmed messages
    Parameters params           = createTestParameters();
    params.d_partiallyConfirmed = true;

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    const size_t messageCount     = (k_NUM_RECORDS + 2) / 3;
    const size_t outstandingCount = partiallyConfirmedGUIDS.size() + 1;
    const int    outstandingRatio = static_cast<int>(bsl::floor(
        float(outstandingCount) / float(messageCount) * 100.0f + 0.5f));
    JournalFile::GuidVectorType::const_iterator guidIt =
        partiallyConfirmedGUIDS.cbegin();

    Sequence s;
    for (; guidIt != partiallyConfirmedGUIDS.cend(); ++guidIt) {
        EXPECT_CALL(*printer, printGuid(*guidIt)).InSequence(s);
    }
    EXPECT_CALL(*printer,
                printFooter(partiallyConfirmedGUIDS.size(),
                            0,
                            0,
                            params.d_processRecordTypes))
        .InSequence(s);
    EXPECT_CALL(*printer,
                printOutstandingRatio(outstandingRatio,
                                      outstandingCount,
                                      messageCount))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test8_searchMessagesByQueueKeyTest()
// ------------------------------------------------------------------------
// SEARCH MESSAGES BY QUEUE KEY TEST
//
// Concerns:
//   Search messages by queue key in journal
//   file and output GUIDs.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEARCH MESSAGES BY QUEUE KEY TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 15;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    const char*                  queueKey1 = "ABCDE12345";
    const char*                  queueKey2 = "12345ABCDE";
    JournalFile::GuidVectorType  queueKey1GUIDS(
        bmqtst::TestHelperUtil::allocator());
    journalFile.addJournalRecordsWithTwoQueueKeys(&records,
                                                  &queueKey1GUIDS,
                                                  queueKey1,
                                                  queueKey2);

    // Configure parameters to search messages by queueKey1
    Parameters params = createTestParameters();
    params.d_queueKey.push_back(queueKey1);

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    JournalFile::GuidVectorType::const_iterator guidIt =
        queueKey1GUIDS.cbegin();
    Sequence s;
    for (; guidIt != queueKey1GUIDS.cend(); ++guidIt) {
        EXPECT_CALL(*printer, printGuid(*guidIt)).InSequence(s);
    }
    EXPECT_CALL(
        *printer,
        printFooter(queueKey1GUIDS.size(), 0, 0, params.d_processRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test9_searchMessagesByQueueNameTest()
// ------------------------------------------------------------------------
// SEARCH MESSAGES BY QUEUE NAME TEST
//
// Concerns:
//   Search messages by queue name in journal
//   file and output GUIDs.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEARCH MESSAGES BY QUEUE NAME TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 15;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    const char*                  queueKey1 = "ABCDE12345";
    const char*                  queueKey2 = "12345ABCDE";
    JournalFile::GuidVectorType  queueKey1GUIDS(
        bmqtst::TestHelperUtil::allocator());
    journalFile.addJournalRecordsWithTwoQueueKeys(&records,
                                                  &queueKey1GUIDS,
                                                  queueKey1,
                                                  queueKey2);

    // Configure parameters to search messages by 'queue1' name
    bmqp_ctrlmsg::QueueInfo queueInfo(bmqtst::TestHelperUtil::allocator());
    queueInfo.uri() = "queue1";
    mqbu::StorageKey key(mqbu::StorageKey::HexRepresentation(), queueKey1);
    for (int i = 0; i < mqbu::StorageKey::e_KEY_LENGTH_BINARY; i++) {
        queueInfo.key().push_back(key.data()[i]);
    }
    QueueMap qMap(bmqtst::TestHelperUtil::allocator());

    Parameters params = createTestParameters();
    params.d_queueName.push_back("queue1");
    params.d_queueMap.insert(queueInfo);

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    JournalFile::GuidVectorType::const_iterator guidIt =
        queueKey1GUIDS.cbegin();
    Sequence s;
    for (; guidIt != queueKey1GUIDS.cend(); ++guidIt) {
        EXPECT_CALL(*printer, printGuid(*guidIt)).InSequence(s);
    }
    EXPECT_CALL(
        *printer,
        printFooter(queueKey1GUIDS.size(), 0, 0, params.d_processRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test10_searchMessagesByQueueNameAndQueueKeyTest()
// ------------------------------------------------------------------------
// SEARCH MESSAGES BY QUEUE NAME AND QUEUE KEY TEST
//
// Concerns:
//   Search messages by queue name and queue key in journal
//   file and output GUIDs.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "SEARCH MESSAGES BY QUEUE NAME AND QUEUE KEY TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 15;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    const char*                  queueKey1 = "ABCDE12345";
    const char*                  queueKey2 = "12345ABCDE";
    JournalFile::GuidVectorType  queueKey1GUIDS(
        bmqtst::TestHelperUtil::allocator());
    journalFile.addJournalRecordsWithTwoQueueKeys(&records,
                                                  &queueKey1GUIDS,
                                                  queueKey1,
                                                  queueKey2,
                                                  true);

    // Configure parameters to search messages by 'queue1' name and
    // queueKey2 key.
    bmqp_ctrlmsg::QueueInfo queueInfo(bmqtst::TestHelperUtil::allocator());
    queueInfo.uri() = "queue1";
    mqbu::StorageKey key(mqbu::StorageKey::HexRepresentation(), queueKey1);
    for (int i = 0; i < mqbu::StorageKey::e_KEY_LENGTH_BINARY; i++) {
        queueInfo.key().push_back(key.data()[i]);
    }
    QueueMap qMap(bmqtst::TestHelperUtil::allocator());

    Parameters params = createTestParameters();
    params.d_queueName.push_back("queue1");
    params.d_queueMap.insert(queueInfo);
    params.d_queueKey.push_back(queueKey2);

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    JournalFile::GuidVectorType::const_iterator guidIt =
        queueKey1GUIDS.cbegin();
    Sequence s;
    for (; guidIt != queueKey1GUIDS.cend(); ++guidIt) {
        EXPECT_CALL(*printer, printGuid(*guidIt)).InSequence(s);
    }
    EXPECT_CALL(
        *printer,
        printFooter(queueKey1GUIDS.size(), 0, 0, params.d_processRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test11_searchMessagesByTimestamp()
// ------------------------------------------------------------------------
// SEARCH MESSAGES BY TIMESTAMP TEST
//
// Concerns:
//   Search messages by timestamp in journal file and output GUIDs.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEARCH MESSAGES BY TIMESTAMP TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 50;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    journalFile.addAllTypesRecords(&records);
    const bsls::Types::Uint64 ts1 = 10 * journalFile.timestampIncrement();
    const bsls::Types::Uint64 ts2 = 40 * journalFile.timestampIncrement();

    // Configure parameters to search messages by timestamps
    Parameters params            = createTestParameters();
    params.d_range.d_timestampGt = ts1;
    params.d_range.d_timestampLt = ts2;

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    bsl::size_t msgCnt = 0;

    Sequence s;
    for (; recordIter != records.end(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            const bsls::Types::Uint64& ts = msg.header().timestamp();
            // Get GUIDs of messages with matching timestamps
            if (ts > ts1 && ts < ts2) {
                EXPECT_CALL(*printer, printGuid(msg.messageGUID()))
                    .InSequence(s);
                msgCnt++;
            }
        }
    }
    EXPECT_CALL(*printer,
                printFooter(msgCnt, 0, 0, params.d_processRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test12_printMessagesDetailsTest()
// ------------------------------------------------------------------------
// PRINT MESSAGE DETAILS TEST
//
// Concerns:
//   Search messages in journal file and output message details.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PRINT MESSAGE DETAILS TEST");

#if defined(BSLS_PLATFORM_OS_SOLARIS)
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Disable default allocator check for this test until we can debug
    // it on Solaris
#endif

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 15;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    JournalFile::GuidVectorType  confirmedGUIDS(
        bmqtst::TestHelperUtil::allocator());
    journalFile.addJournalRecordsWithOutstandingAndConfirmedMessages(
        &records,
        &confirmedGUIDS,
        false);

    // Configure parameters to print message details
    Parameters params = createTestParameters();
    params.d_details  = true;

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    Sequence s;

    JournalFile::GuidVectorType::const_iterator it = confirmedGUIDS.cbegin();
    for (; it != confirmedGUIDS.cend(); ++it) {
        EXPECT_CALL(*printer, printMessage(ConfirmedGuidMatcher(*it)))
            .InSequence(s);
    }
    EXPECT_CALL(*printer, printMessage(NotConfirmedGuidMatcher()))
        .Times(2)
        .InSequence(s);
    EXPECT_CALL(*printer, printFooter(5, 0, 0, params.d_processRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test13_searchMessagesWithPayloadDumpTest()
// ------------------------------------------------------------------------
// SEARCH MESSAGES WITH PAYLOAD DUMP TEST
//
// Concerns:
//   Search confirmed message in journal file and output GUIDs and payload
//   dumps. In case of confirmed messages search, message data (including
//   dump) are output immediately when 'delete' record found. Order of
//   'delete' records can be different than order of messages. This test
//   simulates different order of 'delete' records and checks that payload
//   dump is output correctly.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "SEARCH MESSAGES WITH PAYLOAD DUMP TEST");

    // Simulate data file
    const DataMessage MESSAGES[] = {
        {
            L_,
            "APP_DATA_APP_DATA_APP_DATA_1",
            ""  //"OPTIONS_OPTIONS_"  // Word aligned
        },
        {
            L_,
            "APP_DATA_APP_DATA_APP_DATA_APP_DATA_APP_DATA_2",
            ""  // OPTIONS_OPTIONS_OPTIONS_OPTIONS_"  // Word aligned
        },
        {
            L_,
            "APP_DATA_APP_DATA_APP_DATA_APP_DATA_APP_DATA_APP_DATA_3",
            ""  // OPTIONS_OPTIONS_OPTIONS_OPTIONS_OPTIONS_OPTIONS_OPTIONS_"
        },
        {
            L_,
            "APP_DATA_APP_DATA_APP_DATA_4",
            ""  //"OPTIONS_OPTIONS_"  // Word aligned
        },
    };

    const unsigned int k_NUM_MSGS = sizeof(MESSAGES) / sizeof(*MESSAGES);

    FileHeader                fileHeader;
    MappedFileDescriptor      mfdData;
    bsl::vector<unsigned int> messageOffsets(
        bmqtst::TestHelperUtil::allocator());
    char* pd = addDataRecords(bmqtst::TestHelperUtil::allocator(),
                              &mfdData,
                              &fileHeader,
                              MESSAGES,
                              k_NUM_MSGS,
                              messageOffsets);
    BMQTST_ASSERT(pd != 0);
    BMQTST_ASSERT_GT(mfdData.fileSize(), 0ULL);
    // Create data file iterator
    DataFileIterator dataIt(&mfdData, fileHeader);

    // Simulate journal file
    const size_t k_NUM_RECORDS =
        k_NUM_MSGS * 2;  // k_NUM_MSGS records + k_NUM_MSGS deletion records

    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    JournalFile::GuidVectorType  confirmedGUIDS(
        bmqtst::TestHelperUtil::allocator());
    journalFile.addJournalRecordsWithConfirmedMessagesWithDifferentOrder(
        &records,
        &confirmedGUIDS,
        k_NUM_MSGS,
        messageOffsets);

    // Configure parameters to search confirmed messages GUIDs with dumping
    // messages payload.
    Parameters params    = createTestParameters();
    params.d_confirmed   = true;
    params.d_dumpPayload = true;
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());
    EXPECT_CALL(static_cast<FileManagerMock&>(*fileManager),
                dataFileIterator())
        .WillRepeatedly(testing::Return(&dataIt));

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    // Prepare expected data
    bsl::string              resultString(resultStream.str(),
                             bmqtst::TestHelperUtil::allocator());
    size_t                   startIdx = 0;
    bsl::vector<bsl::string> expectedPayloadSubstring(
        bmqtst::TestHelperUtil::allocator());
    expectedPayloadSubstring.push_back("DATA_1");
    expectedPayloadSubstring.push_back("DATA_3");
    expectedPayloadSubstring.push_back("DATA_2");
    expectedPayloadSubstring.push_back("DATA_4");

    // Change GUIDs order for 2nd and 3rd messages as it was done in
    // 'addJournalRecordsWithConfirmedMessagesWithDifferentOrder()'
    bsl::swap(confirmedGUIDS[1], confirmedGUIDS[2]);

    // Check that substrings are present in resultStream in correct order
    for (unsigned int i = 0; i < k_NUM_MSGS; i++) {
        // Check GUID
        bmqt::MessageGUID  guid = confirmedGUIDS.at(i);
        bmqu::MemOutStream ss(bmqtst::TestHelperUtil::allocator());
        outputGuidString(ss, guid);
        bsl::string guidStr(ss.str(), bmqtst::TestHelperUtil::allocator());
        size_t      foundIdx = resultString.find(guidStr, startIdx);

        BMQTST_ASSERT_D(guidStr, (foundIdx != bsl::string::npos));
        BMQTST_ASSERT_D(guidStr, (foundIdx >= startIdx));

        startIdx = foundIdx + guidStr.length();

        // Check payload dump substring
        bsl::string dumpStr = expectedPayloadSubstring[i];
        foundIdx            = resultString.find(dumpStr, startIdx);

        BMQTST_ASSERT_D(dumpStr, (foundIdx != bsl::string::npos));
        BMQTST_ASSERT_D(guidStr, (foundIdx >= startIdx));
        startIdx = foundIdx + dumpStr.length();
    }

    bmqtst::TestHelperUtil::allocator()->deallocate(pd);
}

static void test14_summaryTest()
// ------------------------------------------------------------------------
// OUTPUT SUMMARY TEST
//
// Concerns:
//   Search messages in journal file and output summary.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("OUTPUT SUMMARY TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 15;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    JournalFile::GuidVectorType  partiallyConfirmedGUIDS(
        bmqtst::TestHelperUtil::allocator());
    journalFile.addJournalRecordsWithPartiallyConfirmedMessages(
        &records,
        &partiallyConfirmedGUIDS);

    // Configure parameters to output summary
    Parameters params = createTestParameters();
    params.d_summary  = true;

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    Sequence s;
    EXPECT_CALL(*printer,
                printMessageSummary(5, partiallyConfirmedGUIDS.size(), 3, 2))
        .InSequence(s);
    EXPECT_CALL(*printer, printOutstandingRatio(40, 2, 5)).InSequence(s);
    EXPECT_CALL(*printer,
                printRecordSummary(
                    15,
                    QueueDetailsMap(bmqtst::TestHelperUtil::allocator())))
        .InSequence(s);
    EXPECT_CALL(*printer, printJournalFileMeta).InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test15_timestampSearchTest()
// ------------------------------------------------------------------------
// TIMESTAMP SEARCH TEST
//
// Concerns:
//   Find the first message in journal file with timestamp more than the
//   specified 'ts' and move the specified JournalFileIterator to it.
//
// Testing:
//   m_bmqstoragetool::moveToLowerBound()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("TIMESTAMP SEARCH TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 50;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    journalFile.addAllTypesRecords(&records);

    struct ResultChecker {
        static void check(mqbs::JournalFileIterator& it,
                          const bsls::Types::Uint64& ts)
        {
            BMQTST_ASSERT_GT(it.recordHeader().timestamp(), ts);
            BMQTST_ASSERT(!it.isReverseMode());
            // Check previous record
            it.flipDirection();
            BMQTST_ASSERT_EQ(it.nextRecord(), 1);
            BMQTST_ASSERT_LE(it.recordHeader().timestamp(), ts);
            // Set 'it' to its original state
            it.flipDirection();
            BMQTST_ASSERT_EQ(it.nextRecord(), 1);
        }
    };

    {
        // Find existing timestamp
        const bsls::Types::Uint64 ts = k_NUM_RECORDS / 2 *
                                       journalFile.timestampIncrement();
        mqbs::JournalFileIterator journalFileIt(
            &journalFile.mappedFileDescriptor(),
            journalFile.fileHeader(),
            false);
        // Move the iterator to the beginning of the file
        BMQTST_ASSERT_EQ(journalFileIt.nextRecord(), 1);

        Parameters::Range range;
        range.d_timestampGt = ts;
        MoreThanLowerBoundFn lessThanLowerBoundFn(range);

        BMQTST_ASSERT_EQ(
            m_bmqstoragetool::moveToLowerBound(&journalFileIt,
                                               lessThanLowerBoundFn),
            1);
        BMQTST_ASSERT_EQ(journalFileIt.nextRecord(), 1);
        BMQTST_ASSERT_EQ(
            m_bmqstoragetool::moveToLowerBound(&journalFileIt,
                                               lessThanLowerBoundFn),
            1);
        ResultChecker::check(journalFileIt, ts);
    }

    {
        // Find existing timestamps starting from different places of the
        // file
        const bsls::Types::Uint64 ts1 = 10 * journalFile.timestampIncrement();
        const bsls::Types::Uint64 ts2 = 40 * journalFile.timestampIncrement();
        mqbs::JournalFileIterator journalFileIt(
            &journalFile.mappedFileDescriptor(),
            journalFile.fileHeader(),
            false);

        // Move the iterator to the center of the file
        BMQTST_ASSERT_EQ(journalFileIt.nextRecord(), 1);
        BMQTST_ASSERT_EQ(journalFileIt.advance(k_NUM_RECORDS / 2), 1);

        // Find record with lower timestamp than the record pointed by the
        // specified iterator, which is initially forward
        BMQTST_ASSERT_GT(journalFileIt.recordHeader().timestamp(), ts1);

        Parameters::Range range1;
        range1.d_timestampGt = ts1;
        MoreThanLowerBoundFn lessThanLowerBoundFn(range1);

        BMQTST_ASSERT_EQ(
            m_bmqstoragetool::moveToLowerBound(&journalFileIt,
                                               lessThanLowerBoundFn),
            1);
        ResultChecker::check(journalFileIt, ts1);

        // Find record with higher timestamp than the record pointed by the
        // specified iterator, which is initially forward
        BMQTST_ASSERT_LT(journalFileIt.recordHeader().timestamp(), ts2);
        Parameters::Range range2;
        range2.d_timestampGt = ts2;

        MoreThanLowerBoundFn lessThanLowerBoundFn2(range2);
        BMQTST_ASSERT_EQ(
            m_bmqstoragetool::moveToLowerBound(&journalFileIt,
                                               lessThanLowerBoundFn2),
            1);
        ResultChecker::check(journalFileIt, ts2);

        // Find record with lower timestamp than the record pointed by the
        // specified iterator, which is initially backward
        BMQTST_ASSERT_GT(journalFileIt.recordHeader().timestamp(), ts1);
        journalFileIt.flipDirection();
        BMQTST_ASSERT(journalFileIt.isReverseMode());
        BMQTST_ASSERT_EQ(
            m_bmqstoragetool::moveToLowerBound(&journalFileIt,
                                               lessThanLowerBoundFn),
            1);
        ResultChecker::check(journalFileIt, ts1);

        // Find record with higher timestamp than the record pointed by the
        // specified iterator, which is initially backward
        BMQTST_ASSERT_LT(journalFileIt.recordHeader().timestamp(), ts2);
        journalFileIt.flipDirection();
        BMQTST_ASSERT(journalFileIt.isReverseMode());
        BMQTST_ASSERT_EQ(
            m_bmqstoragetool::moveToLowerBound(&journalFileIt,
                                               lessThanLowerBoundFn2),
            1);
        ResultChecker::check(journalFileIt, ts2);
    }

    {
        // Timestamp more than last record in the file
        const bsls::Types::Uint64 ts = k_NUM_RECORDS * 2 *
                                       journalFile.timestampIncrement();
        mqbs::JournalFileIterator journalFileIt(
            &journalFile.mappedFileDescriptor(),
            journalFile.fileHeader(),
            false);
        // Move the iterator to the beginning of the file
        BMQTST_ASSERT_EQ(journalFileIt.nextRecord(), 1);

        Parameters::Range range;
        range.d_timestampGt = ts;
        MoreThanLowerBoundFn lessThanLowerBoundFn(range);

        BMQTST_ASSERT_EQ(
            m_bmqstoragetool::moveToLowerBound(&journalFileIt,
                                               lessThanLowerBoundFn),
            0);
        BMQTST_ASSERT_EQ(journalFileIt.recordIndex(), k_NUM_RECORDS - 1);
        BMQTST_ASSERT_LT(journalFileIt.recordHeader().timestamp(), ts);
        BMQTST_ASSERT(!journalFileIt.isReverseMode());
    }

    {
        // Timestamp less than first record in the file
        const bsls::Types::Uint64 ts = journalFile.timestampIncrement() / 2;
        mqbs::JournalFileIterator journalFileIt(
            &journalFile.mappedFileDescriptor(),
            journalFile.fileHeader(),
            false);
        // Move the iterator to the beginning of the file
        BMQTST_ASSERT_EQ(journalFileIt.nextRecord(), 1);

        Parameters::Range range;
        range.d_timestampGt = ts;
        MoreThanLowerBoundFn lessThanLowerBoundFn(range);

        BMQTST_ASSERT_EQ(
            m_bmqstoragetool::moveToLowerBound(&journalFileIt,
                                               lessThanLowerBoundFn),
            1);
        BMQTST_ASSERT_EQ(journalFileIt.recordIndex(), 0U);
        BMQTST_ASSERT_GT(journalFileIt.recordHeader().timestamp(), ts);
        BMQTST_ASSERT(!journalFileIt.isReverseMode());
    }
}

static void test16_sequenceNumberLowerBoundTest()
// ------------------------------------------------------------------------
// MOVE TO SEQUENCE NUMBER LOWER BOUND TEST
//
// Concerns:
//   Find the first message in journal file with sequence number more than
//   the specified 'valueGt' and move the specified JournalFileIterator to
//   it.
//
// Testing:
//   m_bmqstoragetool::moveToLowerBound()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "MOVE TO SEQUENCE NUMBER LOWER BOUND TEST");

    struct Test {
        int                 d_line;
        size_t              d_numRecords;
        size_t              d_numRecordsWithSameLeaseId;
        unsigned int        d_leaseIdGt;
        bsls::Types::Uint64 d_seqNumberGt;
    } k_DATA[] = {
        {L_, 32, 4, 3, 2},
        {L_, 3, 2, 1, 2},
        {L_, 300, 10, 3, 2},
        {L_, 300, 11, 3, 2},
        {L_, 300, 11, 3, 1},    // edge case (first seqNum inside leaseId)
        {L_, 300, 11, 3, 11},   // edge case (last seqNum inside leaseId)
        {L_, 300, 11, 1, 1},    // edge case (left seqNum edge inside first
                                // leaseId)
        {L_, 330, 11, 30, 10},  // edge case (prev before last seqNum
                                // inside last leaseId)
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        // Simulate journal file
        JournalFile::RecordsListType records(
            bmqtst::TestHelperUtil::allocator());
        JournalFile journalFile(test.d_numRecords,
                                bmqtst::TestHelperUtil::allocator());
        journalFile.addMultipleTypesRecordsWithMultipleLeaseId(
            &records,
            test.d_numRecordsWithSameLeaseId);

        mqbs::JournalFileIterator journalFileIt(
            &journalFile.mappedFileDescriptor(),
            journalFile.fileHeader(),
            false);

        CompositeSequenceNumber seqNumGt(test.d_leaseIdGt, test.d_seqNumberGt);
        unsigned int            expectedLeaseId =
            test.d_leaseIdGt +
            (test.d_seqNumberGt == test.d_numRecordsWithSameLeaseId ? 1 : 0);
        bsls::Types::Uint64 expectedSeqNumber =
            test.d_seqNumberGt == test.d_numRecordsWithSameLeaseId
                ? 1
                : (test.d_seqNumberGt + 1);

        // Move the iterator to the beginning of the file
        BMQTST_ASSERT_EQ(journalFileIt.nextRecord(), 1);

        Parameters::Range range;
        range.d_seqNumGt = seqNumGt;
        MoreThanLowerBoundFn lessThanLowerBoundFn(range);

        BMQTST_ASSERT_EQ_D(
            test.d_line,
            m_bmqstoragetool::moveToLowerBound(&journalFileIt,
                                               lessThanLowerBoundFn),
            1);
        BMQTST_ASSERT_EQ_D(test.d_line,
                           journalFileIt.recordHeader().primaryLeaseId(),
                           expectedLeaseId);
        BMQTST_ASSERT_EQ_D(test.d_line,
                           journalFileIt.recordHeader().sequenceNumber(),
                           expectedSeqNumber);
    }

    // Edge case: not in the range (greater then the last record)
    {
        const size_t                 k_NUM_RECORDS = 30;
        JournalFile::RecordsListType records(
            bmqtst::TestHelperUtil::allocator());
        JournalFile journalFile(k_NUM_RECORDS,
                                bmqtst::TestHelperUtil::allocator());
        journalFile.addMultipleTypesRecordsWithMultipleLeaseId(&records,
                                                               k_NUM_RECORDS);

        mqbs::JournalFileIterator journalFileIt(
            &journalFile.mappedFileDescriptor(),
            journalFile.fileHeader(),
            false);

        // Move the iterator to the beginning of the file
        BMQTST_ASSERT_EQ(journalFileIt.nextRecord(), 1);

        CompositeSequenceNumber seqNumGt(1, k_NUM_RECORDS);
        Parameters::Range       range;
        range.d_seqNumGt = seqNumGt;
        MoreThanLowerBoundFn lessThanLowerBoundFn(range);

        BMQTST_ASSERT_EQ(
            m_bmqstoragetool::moveToLowerBound(&journalFileIt,
                                               lessThanLowerBoundFn),
            0);
        BMQTST_ASSERT_EQ(journalFileIt.recordHeader().primaryLeaseId(), 1u);
        BMQTST_ASSERT_EQ(journalFileIt.recordHeader().sequenceNumber(),
                         k_NUM_RECORDS);
    }
}

static void test17_searchMessagesBySequenceNumbersRange()
// ------------------------------------------------------------------------
// SEARCH MESSAGES BY SEQUENCE NUMBERS RANGE TEST
//
// Concerns:
//   Search messages by sequence numbers range in journal file and output
//   GUIDs.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "SEARCH MESSAGES BY SEQUENCE NUMBERS RANGE TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 100;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    journalFile.addMultipleTypesRecordsWithMultipleLeaseId(&records, 10);
    const CompositeSequenceNumber seqNumGt(3, 3);
    const CompositeSequenceNumber seqNumLt(4, 6);

    // Configure parameters to search messages by sequence number range
    Parameters params         = createTestParameters();
    params.d_range.d_seqNumGt = seqNumGt;
    params.d_range.d_seqNumLt = seqNumLt;

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    // Get GUIDs of messages inside sequence numbers range and prepare
    // expected calls
    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    bsl::size_t msgCnt = 0;
    Sequence    s;
    for (; recordIter != records.end(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            const CompositeSequenceNumber seqNum(
                msg.header().primaryLeaseId(),
                msg.header().sequenceNumber());
            if (seqNumGt < seqNum && seqNum < seqNumLt) {
                EXPECT_CALL(*printer, printGuid(msg.messageGUID()))
                    .InSequence(s);
                msgCnt++;
            }
        }
    }

    EXPECT_CALL(*printer,
                printFooter(msgCnt, 0, 0, params.d_processRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test18_searchMessagesByOffsetsRange()
// ------------------------------------------------------------------------
// SEARCH MESSAGES BY OFFSETS RANGE TEST
//
// Concerns:
//   Search messages by offsets range in journal file and output GUIDs.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEARCH MESSAGES BY OFFSETS RANGE TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 50;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    journalFile.addAllTypesRecords(&records);
    const bsls::Types::Uint64 offsetGt =
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE * 15 + k_HEADER_SIZE;
    const bsls::Types::Uint64 offsetLt =
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE * 35 + k_HEADER_SIZE;

    // Configure parameters to search messages by offsets
    Parameters params         = createTestParameters();
    params.d_range.d_offsetGt = offsetGt;
    params.d_range.d_offsetLt = offsetLt;

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    // Get GUIDs of messages within offsets range and prepare expected
    // calls
    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    bsl::size_t msgCnt = 0;
    Sequence    s;
    for (; recordIter != records.end(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            const bsls::Types::Uint64 offset = recordOffset(msg);
            if (offset > offsetGt && offset < offsetLt) {
                EXPECT_CALL(*printer, printGuid(msg.messageGUID()))
                    .InSequence(s);
                msgCnt++;
            }
        }
    }

    EXPECT_CALL(*printer,
                printFooter(msgCnt, 0, 0, params.d_processRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test19_searchQueueOpRecords()
// ------------------------------------------------------------------------
// SEARCH QUEUE OP RECORDS
//
// Concerns:
//   Search queueOP records by offsets range in journal file and output
//   result.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEARCH QUEUE OP RECORDS TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 50;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    journalFile.addAllTypesRecords(&records);
    const bsls::Types::Uint64 offsetGt =
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE * 15 + k_HEADER_SIZE;
    const bsls::Types::Uint64 offsetLt =
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE * 35 + k_HEADER_SIZE;

    // Configure parameters to search queueOp records by offsets
    Parameters params         = createTestParameters(e_QUEUE_OP);
    params.d_range.d_offsetGt = offsetGt;
    params.d_range.d_offsetLt = offsetLt;

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    // Get queueOp records content within offsets range and prepare
    // expected calls
    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.cbegin();
    Sequence            s;
    bsls::Types::Uint64 queueOpCount = 0;
    for (; recordIter != records.cend(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_QUEUE_OP) {
            const QueueOpRecord& queueOp =
                *reinterpret_cast<const QueueOpRecord*>(
                    recordIter->second.buffer());
            const bsls::Types::Uint64 offset = recordOffset(queueOp);
            if (offset > offsetGt && offset < offsetLt) {
                ++queueOpCount;
                EXPECT_CALL(*printer,
                            printQueueOpRecord(OffsetMatcher(offset)))
                    .InSequence(s);
            }
        }
    }

    EXPECT_CALL(*printer,
                printFooter(0, queueOpCount, 0, params.d_processRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test20_searchJournalOpRecords()
// ------------------------------------------------------------------------
// SEARCH JOURNAL OP RECORDS
//
// Concerns:
//   Search journalOP records by offsets range in journal file and output
//   result.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEARCH JOURNAL OP RECORDS TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 50;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    journalFile.addAllTypesRecords(&records);
    const bsls::Types::Uint64 offsetGt =
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE * 15 + k_HEADER_SIZE;
    const bsls::Types::Uint64 offsetLt =
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE * 35 + k_HEADER_SIZE;

    // Configure parameters to search journalOp records by offsets
    Parameters params         = createTestParameters(e_JOURNAL_OP);
    params.d_range.d_offsetGt = offsetGt;
    params.d_range.d_offsetLt = offsetLt;

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    // Get journalOp records content within offsets range and prepare
    // expected calls
    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.cbegin();
    Sequence            s;
    bsls::Types::Uint64 journalOpCount = 0;
    for (; recordIter != records.cend(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_JOURNAL_OP) {
            const JournalOpRecord& journalOp =
                *reinterpret_cast<const JournalOpRecord*>(
                    recordIter->second.buffer());
            const bsls::Types::Uint64 offset = recordOffset(journalOp);
            if (offset > offsetGt && offset < offsetLt) {
                ++journalOpCount;
                EXPECT_CALL(*printer,
                            printJournalOpRecord(OffsetMatcher(offset)))
                    .InSequence(s);
            }
        }
    }

    EXPECT_CALL(*printer,
                printFooter(0, 0, journalOpCount, params.d_processRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test21_searchAllTypesRecords()
// ------------------------------------------------------------------------
// SEARCH ALL TYPES RECORDS
//
// Concerns:
//   Search all types records by offsets range in journal file and output
//   result.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEARCH ALL TYPES RECORDS TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 50;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    journalFile.addAllTypesRecords(&records);
    const bsls::Types::Uint64 offsetGt =
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE * 15 + k_HEADER_SIZE;
    const bsls::Types::Uint64 offsetLt =
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE * 35 + k_HEADER_SIZE;

    // Configure parameters to search journalOp records by offsets
    Parameters params         = createTestParameters(e_MESSAGE | e_QUEUE_OP |
                                             e_JOURNAL_OP);
    params.d_range.d_offsetGt = offsetGt;
    params.d_range.d_offsetLt = offsetLt;

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    // Get all records content within offsets range and prepare expected
    // output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.cbegin();
    bsl::size_t msgCnt       = 0;
    bsl::size_t queueOpCnt   = 0;
    bsl::size_t journalOpCnt = 0;
    Sequence    s;
    for (; recordIter != records.cend(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            const bsls::Types::Uint64 offset = recordOffset(msg);
            if (offset > offsetGt && offset < offsetLt) {
                msgCnt++;
                EXPECT_CALL(*printer, printGuid(msg.messageGUID()))
                    .InSequence(s);
            }
        }
        else if (rtype == RecordType::e_QUEUE_OP) {
            const QueueOpRecord& queueOp =
                *reinterpret_cast<const QueueOpRecord*>(
                    recordIter->second.buffer());
            const bsls::Types::Uint64 offset = recordOffset(queueOp);
            if (offset > offsetGt && offset < offsetLt) {
                queueOpCnt++;
                EXPECT_CALL(*printer,
                            printQueueOpRecord(OffsetMatcher(offset)))
                    .InSequence(s);
            }
        }
        else if (rtype == RecordType::e_JOURNAL_OP) {
            const JournalOpRecord& journalOp =
                *reinterpret_cast<const JournalOpRecord*>(
                    recordIter->second.buffer());
            const bsls::Types::Uint64 offset = recordOffset(journalOp);
            if (offset > offsetGt && offset < offsetLt) {
                journalOpCnt++;
                EXPECT_CALL(*printer,
                            printJournalOpRecord(OffsetMatcher(offset)))
                    .InSequence(s);
            }
        }
    }

    EXPECT_CALL(*printer,
                printFooter(msgCnt,
                            queueOpCnt,
                            journalOpCnt,
                            params.d_processRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test22_searchQueueOpRecordsByOffset()
// ------------------------------------------------------------------------
// SEARCH QUEUE OP RECORDS BY OFFSET
//
// Concerns:
//   Search queueOP records by exact offsets in journal file and output
//   result.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "SEARCH QUEUE OP RECORDS BY OFFSET TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 50;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    journalFile.addAllTypesRecords(&records);

    // Configure parameters to search queueOp records
    Parameters params = createTestParameters(e_QUEUE_OP);

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Get queueOp records content and prepare expected
    // calls
    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.cbegin();
    bsl::size_t resCnt     = 0;
    bsl::size_t queueOpCnt = 0;
    Sequence    s;
    for (; recordIter != records.cend(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_QUEUE_OP) {
            const QueueOpRecord& queueOp =
                *reinterpret_cast<const QueueOpRecord*>(
                    recordIter->second.buffer());
            const bsls::Types::Uint64 offset = recordOffset(queueOp);
            if (queueOpCnt++ % 3 == 0) {
                params.d_offset.push_back(offset);
                EXPECT_CALL(*printer,
                            printQueueOpRecord(OffsetMatcher(offset)))
                    .InSequence(s);
                resCnt++;
            }
        }
    }

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    EXPECT_CALL(*printer,
                printFooter(0, resCnt, 0, params.d_processRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test23_searchJournalOpRecordsBySeqNumber()
// ------------------------------------------------------------------------
// SEARCH JOURNAL OP RECORDS BY SEQUENCE NUMBER
//
// Concerns:
//   Search journalOP records by exact sequence numbers in journal file and
//   output result.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "SEARCH JOURNAL OP RECORDS BY SEQUENCE NUMBER TEST");

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 50;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    journalFile.addAllTypesRecords(&records);

    // Configure parameters to search journalOp
    Parameters params = createTestParameters(e_JOURNAL_OP);

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Get journalOp records content and prepare expected
    // calls

    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    bsl::size_t resCnt = 0;
    bsl::size_t jOpCnt = 0;
    Sequence    s;
    for (; recordIter != records.end(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_JOURNAL_OP) {
            const JournalOpRecord& journalOp =
                *reinterpret_cast<const JournalOpRecord*>(
                    recordIter->second.buffer());
            if (jOpCnt++ % 3 == 0) {
                CompositeSequenceNumber expectedComposite(
                    journalOp.header().primaryLeaseId(),
                    journalOp.header().sequenceNumber());
                params.d_seqNum.emplace_back(expectedComposite);
                EXPECT_CALL(*printer,
                            printJournalOpRecord(
                                SequenceNumberMatcher(expectedComposite)))
                    .InSequence(s);
                resCnt++;
            }
        }
    }

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    EXPECT_CALL(*printer,
                printFooter(0, 0, resCnt, params.d_processRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test24_summaryWithQueueDetailsTest()
// ------------------------------------------------------------------------
// OUTPUT SUMMARY TEST
//
// Concerns:
//   Search messages in journal file and output summary.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "OUTPUT SUMMARY WITH QUEUE DETAILS TEST");

    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Disable default allocator check for this test because
    // EXPECT_CALL(PrinterMock::printRecordSummary(bsls::Types::Uint64,
    //                                             const QueueDetailsMap&))
    // uses default allocator

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 15;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    JournalFile::GuidVectorType  partiallyConfirmedGUIDS(
        bmqtst::TestHelperUtil::allocator());
    journalFile.addJournalRecordsWithPartiallyConfirmedMessages(
        &records,
        &partiallyConfirmedGUIDS);

    // Configure parameters to output summary
    Parameters params = createTestParameters();

    params.d_summary            = true;
    params.d_minRecordsPerQueue = 0;

    // Create printer mock
    bsl::shared_ptr<PrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) PrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    // Prepare expected QueueDetails
    QueueDetailsMap m(bmqtst::TestHelperUtil::allocator());
    QueueDetails&   q =
        m.emplace(mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                   "abcde"),
                  QueueDetails(bmqtst::TestHelperUtil::allocator()))
            .first->second;
    q.d_recordsNumber        = 15;
    q.d_messageRecordsNumber = 5;
    q.d_confirmRecordsNumber = 5;
    q.d_deleteRecordsNumber  = 5;
    q.d_queueOpRecordsNumber = 0;
    QueueDetails::AppDetails& ad =
        q.d_appDetailsMap
            .emplace(mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                      "appid"),
                     QueueDetails::AppDetails())
            .first->second;
    ad.d_recordsNumber = 5;

    Sequence s;
    EXPECT_CALL(*printer,
                printMessageSummary(5, partiallyConfirmedGUIDS.size(), 3, 2))
        .InSequence(s);
    EXPECT_CALL(*printer, printOutstandingRatio(40, 2, 5)).InSequence(s);
    EXPECT_CALL(*printer, printRecordSummary(15, m)).InSequence(s);
    EXPECT_CALL(*printer, printJournalFileMeta).InSequence(s);

    // Run search
    searchProcessor->process();
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    ::testing::GTEST_FLAG(throw_on_failure) = true;
    ::testing::InitGoogleMock(&argc, argv);
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_breathingTest(); break;
    case 2: test2_searchGuidTest(); break;
    case 3: test3_searchNonExistingGuidTest(); break;
    case 4: test4_searchExistingAndNonExistingGuidTest(); break;
    case 5: test5_searchOutstandingMessagesTest(); break;
    case 6: test6_searchConfirmedMessagesTest(); break;
    case 7: test7_searchPartiallyConfirmedMessagesTest(); break;
    case 8: test8_searchMessagesByQueueKeyTest(); break;
    case 9: test9_searchMessagesByQueueNameTest(); break;
    case 10: test10_searchMessagesByQueueNameAndQueueKeyTest(); break;
    case 11: test11_searchMessagesByTimestamp(); break;
    case 12: test12_printMessagesDetailsTest(); break;
    case 13: test13_searchMessagesWithPayloadDumpTest(); break;
    case 14: test14_summaryTest(); break;
    case 15: test15_timestampSearchTest(); break;
    case 16: test16_sequenceNumberLowerBoundTest(); break;
    case 17: test17_searchMessagesBySequenceNumbersRange(); break;
    case 18: test18_searchMessagesByOffsetsRange(); break;
    case 19: test19_searchQueueOpRecords(); break;
    case 20: test20_searchJournalOpRecords(); break;
    case 21: test21_searchAllTypesRecords(); break;
    case 22: test22_searchQueueOpRecordsByOffset(); break;
    case 23: test23_searchJournalOpRecordsBySeqNumber(); break;
    case 24: test24_summaryWithQueueDetailsTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
