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

// MQB
#include <mqbs_mappedfiledescriptor.h>
#include <mqbs_memoryblock.h>
#include <mqbs_offsetptr.h>
#include <mqbu_messageguidutil.h>

// BMQ
#include <bmqu_alignedprinter.h>
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

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

namespace {

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

}  // close unnamed namespace

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

    // Prepare parameters
    Parameters params(bmqtst::TestHelperUtil::allocator());
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    // Prepare expected output with list of message GUIDs in Journal file
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    bsl::size_t foundMessagesCount = 0;
    for (; recordIter != records.end(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            outputGuidString(expectedStream, msg.messageGUID());
            foundMessagesCount++;
        }
    }
    expectedStream << foundMessagesCount << " message GUID(s) found."
                   << bsl::endl;

    BMQTST_ASSERT_EQ(resultStream.str(), expectedStream.str());
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

    // Prepare parameters
    Parameters params(bmqtst::TestHelperUtil::allocator());
    // Get list of message GUIDs for searching
    bsl::vector<bsl::string>& searchGuids = params.d_guid;
    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    bsl::size_t msgCnt = 0;
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
        }
    }
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    // Prepare expected output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    bsl::vector<bsl::string>::const_iterator guidIt = searchGuids.cbegin();
    for (; guidIt != searchGuids.cend(); ++guidIt) {
        expectedStream << (*guidIt) << bsl::endl;
    }
    expectedStream << searchGuids.size() << " message GUID(s) found."
                   << bsl::endl;

    BMQTST_ASSERT_EQ(resultStream.str(), expectedStream.str());
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

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 15;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    journalFile.addAllTypesRecords(&records);

    // Prepare parameters
    Parameters params(bmqtst::TestHelperUtil::allocator());
    // Get list of message GUIDs for searching
    bsl::vector<bsl::string>& searchGuids = params.d_guid;
    bmqt::MessageGUID         guid;
    for (int i = 0; i < 2; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        bmqu::MemOutStream ss(bmqtst::TestHelperUtil::allocator());
        ss << guid;
        searchGuids.push_back(
            bsl::string(ss.str(), bmqtst::TestHelperUtil::allocator()));
    }

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    // Prepare expected output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    expectedStream << "No message GUID found." << bsl::endl;

    expectedStream << bsl::endl
                   << "The following 2 GUID(s) not found:" << bsl::endl;
    expectedStream << searchGuids[0] << bsl::endl
                   << searchGuids[1] << bsl::endl;

    BMQTST_ASSERT_EQ(resultStream.str(), expectedStream.str());
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

    // Simulate journal file
    const size_t                 k_NUM_RECORDS = 15;
    JournalFile::RecordsListType records(bmqtst::TestHelperUtil::allocator());
    JournalFile                  journalFile(k_NUM_RECORDS,
                            bmqtst::TestHelperUtil::allocator());
    journalFile.addAllTypesRecords(&records);

    // Prepare parameters
    Parameters params(bmqtst::TestHelperUtil::allocator());

    // Get list of message GUIDs for searching
    bsl::vector<bsl::string>& searchGuids = params.d_guid;

    // Get two existing message GUIDs
    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    size_t msgCnt = 0;
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
        }
    }

    // Get two non existing message GUIDs
    bmqt::MessageGUID guid;
    for (int i = 0; i < 2; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        bmqu::MemOutStream ss(bmqtst::TestHelperUtil::allocator());
        ss << guid;
        searchGuids.push_back(
            bsl::string(ss.str(), bmqtst::TestHelperUtil::allocator()));
    }

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    // Prepare expected output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    expectedStream << searchGuids[0] << bsl::endl
                   << searchGuids[1] << bsl::endl;
    expectedStream << "2 message GUID(s) found." << bsl::endl;
    expectedStream << bsl::endl
                   << "The following 2 GUID(s) not found:" << bsl::endl;
    expectedStream << searchGuids[2] << bsl::endl
                   << searchGuids[3] << bsl::endl;

    BMQTST_ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    Parameters params(bmqtst::TestHelperUtil::allocator());
    params.d_outstanding = true;
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    // Prepare expected output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    JournalFile::GuidVectorType::const_iterator guidIt =
        outstandingGUIDS.cbegin();
    for (; guidIt != outstandingGUIDS.cend(); ++guidIt) {
        outputGuidString(expectedStream, *guidIt);
    }

    expectedStream << outstandingGUIDS.size() << " message GUID(s) found."
                   << bsl::endl;
    const size_t messageCount     = k_NUM_RECORDS / 3;
    const float  outstandingRatio = static_cast<float>(
                                       outstandingGUIDS.size()) /
                                   static_cast<float>(messageCount) * 100.0f;
    expectedStream << "Outstanding ratio: " << outstandingRatio << "% ("
                   << outstandingGUIDS.size() << "/" << messageCount << ")"
                   << bsl::endl;

    BMQTST_ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    Parameters params(bmqtst::TestHelperUtil::allocator());
    params.d_confirmed = true;
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    // Prepare expected output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    JournalFile::GuidVectorType::const_iterator guidIt =
        confirmedGUIDS.cbegin();
    for (; guidIt != confirmedGUIDS.cend(); ++guidIt) {
        outputGuidString(expectedStream, *guidIt);
    }
    expectedStream << confirmedGUIDS.size() << " message GUID(s) found."
                   << bsl::endl;
    const size_t messageCount     = k_NUM_RECORDS / 3;
    const float  outstandingRatio = static_cast<float>(messageCount -
                                                      confirmedGUIDS.size()) /
                                   static_cast<float>(messageCount) * 100.0f;
    expectedStream << "Outstanding ratio: " << outstandingRatio << "% ("
                   << (messageCount - confirmedGUIDS.size()) << "/"
                   << messageCount << ")" << bsl::endl;

    BMQTST_ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    Parameters params(bmqtst::TestHelperUtil::allocator());
    params.d_partiallyConfirmed = true;
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    // Prepare expected output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    JournalFile::GuidVectorType::const_iterator guidIt =
        partiallyConfirmedGUIDS.cbegin();
    for (; guidIt != partiallyConfirmedGUIDS.cend(); ++guidIt) {
        outputGuidString(expectedStream, *guidIt);
    }
    expectedStream << partiallyConfirmedGUIDS.size()
                   << " message GUID(s) found." << bsl::endl;
    const size_t messageCount     = (k_NUM_RECORDS + 2) / 3;
    const float  outstandingRatio = static_cast<float>(
                                       partiallyConfirmedGUIDS.size() + 1) /
                                   static_cast<float>(messageCount) * 100.0f;
    expectedStream << "Outstanding ratio: " << outstandingRatio << "% ("
                   << partiallyConfirmedGUIDS.size() + 1 << "/" << messageCount
                   << ")" << bsl::endl;

    BMQTST_ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    Parameters params(bmqtst::TestHelperUtil::allocator());
    params.d_queueKey.push_back(queueKey1);
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    // Prepare expected output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    JournalFile::GuidVectorType::const_iterator guidIt =
        queueKey1GUIDS.cbegin();
    for (; guidIt != queueKey1GUIDS.cend(); ++guidIt) {
        outputGuidString(expectedStream, *guidIt);
    }
    size_t foundMessagesCount = queueKey1GUIDS.size();
    expectedStream << foundMessagesCount << " message GUID(s) found."
                   << bsl::endl;

    BMQTST_ASSERT_EQ(resultStream.str(), expectedStream.str());
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

    Parameters params(bmqtst::TestHelperUtil::allocator());
    params.d_queueName.push_back("queue1");
    params.d_queueMap.insert(queueInfo);

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    // Prepare expected output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    JournalFile::GuidVectorType::const_iterator guidIt =
        queueKey1GUIDS.cbegin();
    for (; guidIt != queueKey1GUIDS.cend(); ++guidIt) {
        outputGuidString(expectedStream, *guidIt);
    }
    size_t foundMessagesCount = queueKey1GUIDS.size();
    expectedStream << foundMessagesCount << " message GUID(s) found."
                   << bsl::endl;

    BMQTST_ASSERT_EQ(resultStream.str(), expectedStream.str());
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

    // Configure parameters to search messages by 'queue1' name and queueKey2
    // key.
    bmqp_ctrlmsg::QueueInfo queueInfo(bmqtst::TestHelperUtil::allocator());
    queueInfo.uri() = "queue1";
    mqbu::StorageKey key(mqbu::StorageKey::HexRepresentation(), queueKey1);
    for (int i = 0; i < mqbu::StorageKey::e_KEY_LENGTH_BINARY; i++) {
        queueInfo.key().push_back(key.data()[i]);
    }
    QueueMap qMap(bmqtst::TestHelperUtil::allocator());

    Parameters params(bmqtst::TestHelperUtil::allocator());
    params.d_queueName.push_back("queue1");
    params.d_queueMap.insert(queueInfo);
    params.d_queueKey.push_back(queueKey2);

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    // Prepare expected output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    JournalFile::GuidVectorType::const_iterator guidIt =
        queueKey1GUIDS.cbegin();
    for (; guidIt != queueKey1GUIDS.cend(); ++guidIt) {
        outputGuidString(expectedStream, *guidIt);
    }
    size_t foundMessagesCount = queueKey1GUIDS.size();
    expectedStream << foundMessagesCount << " message GUID(s) found."
                   << bsl::endl;

    BMQTST_ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    Parameters params(bmqtst::TestHelperUtil::allocator());
    params.d_range.d_timestampGt = ts1;
    params.d_range.d_timestampLt = ts2;
    params.d_range.d_type        = Parameters::Range::e_TIMESTAMP;

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Get GUIDs of messages with matching timestamps and prepare expected
    // output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    bsl::size_t msgCnt = 0;
    for (; recordIter != records.end(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            const bsls::Types::Uint64& ts = msg.header().timestamp();
            if (ts > ts1 && ts < ts2) {
                outputGuidString(expectedStream, msg.messageGUID());
                msgCnt++;
            }
        }
    }
    expectedStream << msgCnt << " message GUID(s) found." << bsl::endl;

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    BMQTST_ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    s_ignoreCheckDefAlloc = true;
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
    Parameters params(bmqtst::TestHelperUtil::allocator());
    params.d_details = true;
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    // Check that substrings are present in resultStream in correct order
    bsl::string resultString(resultStream.str(),
                             bmqtst::TestHelperUtil::allocator());
    size_t      startIdx             = 0;
    const char* messageRecordCaption = "MESSAGE Record";
    const char* confirmRecordCaption = "CONFIRM Record";
    const char* deleteRecordCaption  = "DELETE Record";
    for (size_t i = 0; i < confirmedGUIDS.size(); i++) {
        // Check Message type
        size_t foundIdx = resultString.find(messageRecordCaption, startIdx);
        BMQTST_ASSERT_D(messageRecordCaption, (foundIdx != bsl::string::npos));
        BMQTST_ASSERT_D(messageRecordCaption, (foundIdx >= startIdx));
        startIdx = foundIdx + bsl::strlen(messageRecordCaption);

        // Check GUID
        bmqu::MemOutStream ss(bmqtst::TestHelperUtil::allocator());
        outputGuidString(ss, confirmedGUIDS.at(i));
        bsl::string guidStr(ss.str(), bmqtst::TestHelperUtil::allocator());
        foundIdx = resultString.find(guidStr, startIdx);
        BMQTST_ASSERT_D(guidStr, (foundIdx != bsl::string::npos));
        BMQTST_ASSERT_D(guidStr, (foundIdx >= startIdx));
        startIdx = foundIdx + guidStr.length();

        // Check Confirm type
        foundIdx = resultString.find(confirmRecordCaption, startIdx);
        BMQTST_ASSERT_D(confirmRecordCaption, (foundIdx != bsl::string::npos));
        BMQTST_ASSERT_D(confirmRecordCaption, (foundIdx >= startIdx));
        startIdx = foundIdx + bsl::strlen(messageRecordCaption);

        // Check Delete type
        foundIdx = resultString.find(deleteRecordCaption, startIdx);
        BMQTST_ASSERT_D(deleteRecordCaption, (foundIdx != bsl::string::npos));
        BMQTST_ASSERT_D(deleteRecordCaption, (foundIdx >= startIdx));
        startIdx = foundIdx + bsl::strlen(messageRecordCaption);
    }
}

static void test13_searchMessagesWithPayloadDumpTest()
// ------------------------------------------------------------------------
// SEARCH MESSAGES WITH PAYLOAD DUMP TEST
//
// Concerns:
//   Search confirmed message in journal file and output GUIDs and payload
//   dumps. In case of confirmed messages search, message data (including dump)
//   are output immediately when 'delete' record found. Order of 'delete'
//   records can be different than order of messages. This test simulates
//   different order of 'delete' records and checks that payload dump is output
//   correctly.
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
    Parameters params(bmqtst::TestHelperUtil::allocator());
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
    Parameters params(bmqtst::TestHelperUtil::allocator());
    params.d_summary = true;
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    // Prepare expected output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());
    expectedStream << "5 message(s) found.\n";
    bsl::vector<const char*> fields(bmqtst::TestHelperUtil::allocator());
    fields.push_back("Number of partially confirmed messages");
    fields.push_back("Number of confirmed messages");
    fields.push_back("Number of outstanding messages");
    bmqu::AlignedPrinter printer(expectedStream, &fields);
    printer << 3 << 2 << 2;
    expectedStream << "Outstanding ratio: 40% (2/5)\n";

    bsl::string res(resultStream.str(), bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT(res.starts_with(expectedStream.str()));
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
        range.d_type        = Parameters::Range::e_TIMESTAMP;
        range.d_timestampGt = ts;
        LessThanLowerBoundFn lessThanLowerBoundFn(range);

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
        // Find existing timestamps starting from different places of the file
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

        Parameters::Range range;
        range.d_type        = Parameters::Range::e_TIMESTAMP;
        range.d_timestampGt = ts1;
        LessThanLowerBoundFn lessThanLowerBoundFn(range);

        BMQTST_ASSERT_EQ(
            m_bmqstoragetool::moveToLowerBound(&journalFileIt,
                                               lessThanLowerBoundFn),
            1);
        ResultChecker::check(journalFileIt, ts1);

        // Find record with higher timestamp than the record pointed by the
        // specified iterator, which is initially forward
        BMQTST_ASSERT_LT(journalFileIt.recordHeader().timestamp(), ts2);
        range.d_timestampGt = ts2;

        LessThanLowerBoundFn lessThanLowerBoundFn2(range);
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
        range.d_type        = Parameters::Range::e_TIMESTAMP;
        range.d_timestampGt = ts;
        LessThanLowerBoundFn lessThanLowerBoundFn(range);

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
        range.d_type        = Parameters::Range::e_TIMESTAMP;
        range.d_timestampGt = ts;
        LessThanLowerBoundFn lessThanLowerBoundFn(range);

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
//   Find the first message in journal file with sequence number more than the
//   specified 'valueGt' and move the specified JournalFileIterator to it.
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
        {L_, 330, 11, 30, 10},  // edge case (prev before last seqNum inside
                                // last leaseId)
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
        range.d_type     = Parameters::Range::e_SEQUENCE_NUM;
        range.d_seqNumGt = seqNumGt;
        LessThanLowerBoundFn lessThanLowerBoundFn(range);

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
        range.d_type     = Parameters::Range::e_SEQUENCE_NUM;
        range.d_seqNumGt = seqNumGt;
        LessThanLowerBoundFn lessThanLowerBoundFn(range);

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
    Parameters params(bmqtst::TestHelperUtil::allocator());
    params.d_range.d_seqNumGt = seqNumGt;
    params.d_range.d_seqNumLt = seqNumLt;
    params.d_range.d_type     = Parameters::Range::e_SEQUENCE_NUM;
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Get GUIDs of messages inside sequence numbers range and prepare expected
    // output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    bsl::size_t msgCnt = 0;
    for (; recordIter != records.end(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            const CompositeSequenceNumber seqNum(
                msg.header().primaryLeaseId(),
                msg.header().sequenceNumber());
            if (seqNumGt < seqNum && seqNum < seqNumLt) {
                outputGuidString(expectedStream, msg.messageGUID());
                msgCnt++;
            }
        }
    }
    expectedStream << msgCnt << " message GUID(s) found." << bsl::endl;

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    BMQTST_ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    const size_t k_HEADER_SIZE = sizeof(mqbs::FileHeader) +
                                 sizeof(mqbs::JournalFileHeader);
    const bsls::Types::Uint64 offsetGt =
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE * 15 + k_HEADER_SIZE;
    const bsls::Types::Uint64 offsetLt =
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE * 35 + k_HEADER_SIZE;

    // Configure parameters to search messages by offsets
    Parameters params(bmqtst::TestHelperUtil::allocator());
    params.d_range.d_offsetGt = offsetGt;
    params.d_range.d_offsetLt = offsetLt;
    params.d_range.d_type     = Parameters::Range::e_OFFSET;
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Get GUIDs of messages within offsets range and prepare expected
    // output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    bsl::size_t msgCnt = 0;
    for (; recordIter != records.end(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            const bsls::Types::Uint64& offset =
                msg.header().sequenceNumber() *
                mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
            if (offset > offsetGt && offset < offsetLt) {
                outputGuidString(expectedStream, msg.messageGUID());
                msgCnt++;
            }
        }
    }
    expectedStream << msgCnt << " message GUID(s) found." << bsl::endl;

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    BMQTST_ASSERT_EQ(resultStream.str(), expectedStream.str());
}

static void test19_searchQueueOpRecords()
// ------------------------------------------------------------------------
// SEARCH QUEUE OP RECORDS
//
// Concerns:
//   Search queueOP records by offsets range in journal file and output result.
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
    const size_t k_HEADER_SIZE = sizeof(mqbs::FileHeader) +
                                 sizeof(mqbs::JournalFileHeader);
    const bsls::Types::Uint64 offsetGt =
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE * 15 + k_HEADER_SIZE;
    const bsls::Types::Uint64 offsetLt =
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE * 35 + k_HEADER_SIZE;

    // Configure parameters to search queueOp records by offsets
    Parameters params(bmqtst::TestHelperUtil::allocator());
    params.d_processRecordTypes.d_message = false;
    params.d_processRecordTypes.d_queueOp = true;
    params.d_range.d_offsetGt             = offsetGt;
    params.d_range.d_offsetLt             = offsetLt;
    params.d_range.d_type                 = Parameters::Range::e_OFFSET;
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Get queueOp records content within offsets range and prepare expected
    // output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    bsl::size_t recCnt = 0;
    for (; recordIter != records.end(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_QUEUE_OP) {
            const QueueOpRecord& queueOp =
                *reinterpret_cast<const QueueOpRecord*>(
                    recordIter->second.buffer());
            const bsls::Types::Uint64& offset =
                queueOp.header().sequenceNumber() *
                mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
            if (offset > offsetGt && offset < offsetLt) {
                expectedStream << queueOp << '\n';
                recCnt++;
            }
        }
    }
    expectedStream << recCnt << " queueOp record(s) found.\n";

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    BMQTST_ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    const size_t k_HEADER_SIZE = sizeof(mqbs::FileHeader) +
                                 sizeof(mqbs::JournalFileHeader);
    const bsls::Types::Uint64 offsetGt =
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE * 15 + k_HEADER_SIZE;
    const bsls::Types::Uint64 offsetLt =
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE * 35 + k_HEADER_SIZE;

    // Configure parameters to search journalOp records by offsets
    Parameters params(bmqtst::TestHelperUtil::allocator());
    params.d_processRecordTypes.d_message   = false;
    params.d_processRecordTypes.d_journalOp = true;
    params.d_range.d_offsetGt               = offsetGt;
    params.d_range.d_offsetLt               = offsetLt;
    params.d_range.d_type                   = Parameters::Range::e_OFFSET;
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Get journalOp records content within offsets range and prepare expected
    // output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    bsl::size_t recCnt = 0;
    for (; recordIter != records.end(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_JOURNAL_OP) {
            const JournalOpRecord& journalOp =
                *reinterpret_cast<const JournalOpRecord*>(
                    recordIter->second.buffer());
            const bsls::Types::Uint64& offset =
                journalOp.header().sequenceNumber() *
                mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
            if (offset > offsetGt && offset < offsetLt) {
                expectedStream << journalOp << '\n';
                recCnt++;
            }
        }
    }
    expectedStream << recCnt << " journalOp record(s) found.\n";

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    BMQTST_ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    const size_t k_HEADER_SIZE = sizeof(mqbs::FileHeader) +
                                 sizeof(mqbs::JournalFileHeader);
    const bsls::Types::Uint64 offsetGt =
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE * 15 + k_HEADER_SIZE;
    const bsls::Types::Uint64 offsetLt =
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE * 35 + k_HEADER_SIZE;

    // Configure parameters to search journalOp records by offsets
    Parameters params(bmqtst::TestHelperUtil::allocator());
    params.d_processRecordTypes.d_message   = true;
    params.d_processRecordTypes.d_queueOp   = true;
    params.d_processRecordTypes.d_journalOp = true;
    params.d_range.d_offsetGt               = offsetGt;
    params.d_range.d_offsetLt               = offsetLt;
    params.d_range.d_type                   = Parameters::Range::e_OFFSET;
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Get all records content within offsets range and prepare expected
    // output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    bsl::size_t msgCnt       = 0;
    bsl::size_t queueOpCnt   = 0;
    bsl::size_t journalOpCnt = 0;
    for (; recordIter != records.end(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            const bsls::Types::Uint64& offset =
                msg.header().sequenceNumber() *
                mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
            if (offset > offsetGt && offset < offsetLt) {
                outputGuidString(expectedStream, msg.messageGUID());
                msgCnt++;
            }
        }
        if (rtype == RecordType::e_QUEUE_OP) {
            const QueueOpRecord& queueOp =
                *reinterpret_cast<const QueueOpRecord*>(
                    recordIter->second.buffer());
            const bsls::Types::Uint64& offset =
                queueOp.header().sequenceNumber() *
                mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
            if (offset > offsetGt && offset < offsetLt) {
                expectedStream << queueOp << '\n';
                queueOpCnt++;
            }
        }
        if (rtype == RecordType::e_JOURNAL_OP) {
            const JournalOpRecord& journalOp =
                *reinterpret_cast<const JournalOpRecord*>(
                    recordIter->second.buffer());
            const bsls::Types::Uint64& offset =
                journalOp.header().sequenceNumber() *
                mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
            if (offset > offsetGt && offset < offsetLt) {
                expectedStream << journalOp << '\n';
                journalOpCnt++;
            }
        }
    }
    expectedStream << msgCnt << " message GUID(s) found." << bsl::endl;
    expectedStream << queueOpCnt << " queueOp record(s) found.\n";
    expectedStream << journalOpCnt << " journalOp record(s) found.\n";

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    BMQTST_ASSERT_EQ(resultStream.str(), expectedStream.str());
}

static void test22_searchQueueOpRecordsByOffset()
// ------------------------------------------------------------------------
// SEARCH QUEUE OP RECORDS BY OFFSET
//
// Concerns:
//   Search queueOP records by exact offsets in journal file and output result.
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
    const size_t k_HEADER_OFFSET = sizeof(mqbs::FileHeader) / 2;

    // Configure parameters to search queueOp records
    Parameters params(bmqtst::TestHelperUtil::allocator());
    params.d_processRecordTypes.d_message = false;
    params.d_processRecordTypes.d_queueOp = true;

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Get queueOp records content and prepare expected
    // output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    bsl::size_t resCnt     = 0;
    bsl::size_t queueOpCnt = 0;
    for (; recordIter != records.end(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_QUEUE_OP) {
            const QueueOpRecord& queueOp =
                *reinterpret_cast<const QueueOpRecord*>(
                    recordIter->second.buffer());
            const bsls::Types::Uint64& offset =
                queueOp.header().sequenceNumber() *
                    mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE -
                k_HEADER_OFFSET;
            if (queueOpCnt++ % 3 == 0) {
                params.d_offset.push_back(offset);
                expectedStream << queueOp << '\n';
                resCnt++;
            }
        }
    }
    expectedStream << resCnt << " queueOp record(s) found.\n";

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    BMQTST_ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    Parameters params(bmqtst::TestHelperUtil::allocator());
    params.d_processRecordTypes.d_message   = false;
    params.d_processRecordTypes.d_journalOp = true;

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(journalFile),
        bmqtst::TestHelperUtil::allocator());

    // Get journalOp records content and prepare expected
    // output
    bmqu::MemOutStream expectedStream(bmqtst::TestHelperUtil::allocator());

    bsl::list<JournalFile::NodeType>::const_iterator recordIter =
        records.begin();
    bsl::size_t resCnt = 0;
    bsl::size_t jOpCnt = 0;
    for (; recordIter != records.end(); ++recordIter) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_JOURNAL_OP) {
            const JournalOpRecord& journalOp =
                *reinterpret_cast<const JournalOpRecord*>(
                    recordIter->second.buffer());
            if (jOpCnt++ % 3 == 0) {
                params.d_seqNum.emplace_back(
                    journalOp.header().primaryLeaseId(),
                    journalOp.header().sequenceNumber());
                expectedStream << journalOp << '\n';
                resCnt++;
            }
            jOpCnt++;
        }
    }
    expectedStream << resCnt << " journalOp record(s) found.\n";

    // Run search
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(
            &params,
            fileManager,
            resultStream,
            bmqtst::TestHelperUtil::allocator());
    searchProcessor->process();

    BMQTST_ASSERT_EQ(resultStream.str(), expectedStream.str());
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
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
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
