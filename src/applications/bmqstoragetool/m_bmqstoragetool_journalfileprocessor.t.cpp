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
#include <m_bmqstoragetool_commandprocessorfactory.h>
#include <m_bmqstoragetool_journalfileprocessor.h>
#include <m_bmqstoragetool_testutils.h>

// BMQ
#include <bmqt_messageguid.h>

// MQB
#include <mqbs_filestoreprotocol.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbs_memoryblock.h>
#include <mqbs_offsetptr.h>
#include <mqbu_messageguidutil.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_list.h>
#include <bsl_utility.h>
#include <bslma_default.h>
#include <bsls_alignedbuffer.h>

// GMOCK
#include <gmock/gmock.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace m_bmqstoragetool;
using namespace bsl;
using namespace mqbs;
using namespace ::testing;
using namespace TestUtils;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

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
    mwctst::TestHelper::printTestName("BREATHING TEST");

    // Simulate journal file
    const size_t    k_NUM_RECORDS = 15;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(k_NUM_RECORDS, s_allocator_p);
    journalFile.addAllTypesRecords(&records);

    // Prepare parameters
    Parameters params(s_allocator_p);
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(journalFile),
        s_allocator_p);

    // Run search
    mwcu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output with list of message GUIDs in Journal file
    mwcu::MemOutStream                  expectedStream(s_allocator_p);
    bsl::list<NodeType>::const_iterator recordIter         = records.begin();
    bsl::size_t                         foundMessagesCount = 0;
    while (recordIter++ != records.end()) {
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

    ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    mwctst::TestHelper::printTestName("SEARCH GUID");

    // Simulate journal file
    const size_t    k_NUM_RECORDS = 15;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(k_NUM_RECORDS, s_allocator_p);
    journalFile.addAllTypesRecords(&records);

    // Prepare parameters
    Parameters params(s_allocator_p);
    // Get list of message GUIDs for searching
    bsl::vector<bsl::string>&           searchGuids = params.d_guid;
    bsl::list<NodeType>::const_iterator recordIter  = records.begin();
    bsl::size_t                         msgCnt      = 0;
    while (recordIter++ != records.end()) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            if (msgCnt++ % 2 != 0)
                continue;  // Skip odd messages for test purposes
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            mwcu::MemOutStream ss(s_allocator_p);
            ss << msg.messageGUID();
            searchGuids.push_back(bsl::string(ss.str(), s_allocator_p));
        }
    }
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(journalFile),
        s_allocator_p);

    // Run search
    mwcu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output
    mwcu::MemOutStream                       expectedStream(s_allocator_p);
    bsl::vector<bsl::string>::const_iterator guidIt = searchGuids.cbegin();
    for (; guidIt != searchGuids.cend(); ++guidIt) {
        expectedStream << (*guidIt) << bsl::endl;
    }
    expectedStream << searchGuids.size() << " message GUID(s) found."
                   << bsl::endl;

    ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    mwctst::TestHelper::printTestName("SEARCH NON EXISTING GUID");

    // Simulate journal file
    const size_t    k_NUM_RECORDS = 15;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(k_NUM_RECORDS, s_allocator_p);
    journalFile.addAllTypesRecords(&records);

    // Prepare parameters
    Parameters params(s_allocator_p);
    // Get list of message GUIDs for searching
    bsl::vector<bsl::string>& searchGuids = params.d_guid;
    bmqt::MessageGUID         guid;
    for (int i = 0; i < 2; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        mwcu::MemOutStream ss(s_allocator_p);
        ss << guid;
        searchGuids.push_back(bsl::string(ss.str(), s_allocator_p));
    }

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(journalFile),
        s_allocator_p);

    // Run search
    mwcu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output
    mwcu::MemOutStream expectedStream(s_allocator_p);
    expectedStream << "No message GUID found." << bsl::endl;

    expectedStream << bsl::endl
                   << "The following 2 GUID(s) not found:" << bsl::endl;
    expectedStream << searchGuids[0] << bsl::endl
                   << searchGuids[1] << bsl::endl;

    ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    mwctst::TestHelper::printTestName("SEARCH EXISTING AND NON EXISTING GUID");

    // Simulate journal file
    const size_t    k_NUM_RECORDS = 15;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(k_NUM_RECORDS, s_allocator_p);
    journalFile.addAllTypesRecords(&records);

    // Prepare parameters
    Parameters params(s_allocator_p);

    // Get list of message GUIDs for searching
    bsl::vector<bsl::string>& searchGuids = params.d_guid;

    // Get two existing message GUIDs
    bsl::list<NodeType>::const_iterator recordIter = records.begin();
    size_t                              msgCnt     = 0;
    while (recordIter++ != records.end()) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            if (msgCnt++ == 2)
                break;  // Take two GUIDs
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            mwcu::MemOutStream ss(s_allocator_p);
            ss << msg.messageGUID();
            searchGuids.push_back(bsl::string(ss.str(), s_allocator_p));
        }
    }

    // Get two non existing message GUIDs
    bmqt::MessageGUID guid;
    for (int i = 0; i < 2; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        mwcu::MemOutStream ss(s_allocator_p);
        ss << guid;
        searchGuids.push_back(bsl::string(ss.str(), s_allocator_p));
    }

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(journalFile),
        s_allocator_p);

    // Run search
    mwcu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output
    mwcu::MemOutStream expectedStream(s_allocator_p);
    expectedStream << searchGuids[0] << bsl::endl
                   << searchGuids[1] << bsl::endl;
    expectedStream << "2 message GUID(s) found." << bsl::endl;
    expectedStream << bsl::endl
                   << "The following 2 GUID(s) not found:" << bsl::endl;
    expectedStream << searchGuids[2] << bsl::endl
                   << searchGuids[3] << bsl::endl;

    ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    mwctst::TestHelper::printTestName("SEARCH OUTSTANDING MESSAGES TEST");

    // Simulate journal file
    const size_t    k_NUM_RECORDS = 15;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(k_NUM_RECORDS, s_allocator_p);
    GuidVectorType  outstandingGUIDS(s_allocator_p);
    journalFile.addJournalRecordsWithOutstandingAndConfirmedMessages(
        &records,
        &outstandingGUIDS,
        true);

    // Configure parameters to search outstanding messages
    Parameters params(s_allocator_p);
    params.d_outstanding = true;
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(journalFile),
        s_allocator_p);

    // Run search
    mwcu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output
    mwcu::MemOutStream             expectedStream(s_allocator_p);
    GuidVectorType::const_iterator guidIt = outstandingGUIDS.cbegin();
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

    ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    mwctst::TestHelper::printTestName("SEARCH CONFIRMED MESSAGES TEST");

    // Simulate journal file
    const size_t    k_NUM_RECORDS = 15;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(k_NUM_RECORDS, s_allocator_p);
    GuidVectorType  confirmedGUIDS(s_allocator_p);
    journalFile.addJournalRecordsWithOutstandingAndConfirmedMessages(
        &records,
        &confirmedGUIDS,
        false);

    // Configure parameters to search confirmed messages
    Parameters params(s_allocator_p);
    params.d_confirmed = true;
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(journalFile),
        s_allocator_p);

    // Run search
    mwcu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output
    mwcu::MemOutStream             expectedStream(s_allocator_p);
    GuidVectorType::const_iterator guidIt = confirmedGUIDS.cbegin();
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

    ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    mwctst::TestHelper::printTestName(
        "SEARCH PARTIALLY CONFIRMED MESSAGES TEST");

    // Simulate journal file
    // k_NUM_RECORDS must be multiple 3 plus one to cover all combinations
    // (confirmed, deleted, not confirmed)
    const size_t    k_NUM_RECORDS = 16;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(k_NUM_RECORDS, s_allocator_p);
    GuidVectorType  partiallyConfirmedGUIDS(s_allocator_p);
    journalFile.addJournalRecordsWithPartiallyConfirmedMessages(
        &records,
        &partiallyConfirmedGUIDS);

    // Configure parameters to search partially confirmed messages
    Parameters params(s_allocator_p);
    params.d_partiallyConfirmed = true;
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(journalFile),
        s_allocator_p);

    // Run search
    mwcu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output
    mwcu::MemOutStream             expectedStream(s_allocator_p);
    GuidVectorType::const_iterator guidIt = partiallyConfirmedGUIDS.cbegin();
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

    ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    mwctst::TestHelper::printTestName("SEARCH MESSAGES BY QUEUE KEY TEST");

    // Simulate journal file
    const size_t    k_NUM_RECORDS = 15;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(k_NUM_RECORDS, s_allocator_p);
    const char*     queueKey1 = "ABCDE12345";
    const char*     queueKey2 = "12345ABCDE";
    GuidVectorType  queueKey1GUIDS(s_allocator_p);
    journalFile.addJournalRecordsWithTwoQueueKeys(&records,
                                                  &queueKey1GUIDS,
                                                  queueKey1,
                                                  queueKey2);

    // Configure parameters to search messages by queueKey1
    Parameters params(s_allocator_p);
    params.d_queueKey.push_back(queueKey1);
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(journalFile),
        s_allocator_p);

    // Run search
    mwcu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output
    mwcu::MemOutStream             expectedStream(s_allocator_p);
    GuidVectorType::const_iterator guidIt = queueKey1GUIDS.cbegin();
    for (; guidIt != queueKey1GUIDS.cend(); ++guidIt) {
        outputGuidString(expectedStream, *guidIt);
    }
    size_t foundMessagesCount = queueKey1GUIDS.size();
    expectedStream << foundMessagesCount << " message GUID(s) found."
                   << bsl::endl;

    ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    mwctst::TestHelper::printTestName("SEARCH MESSAGES BY QUEUE NAME TEST");

    // Simulate journal file
    const size_t    k_NUM_RECORDS = 15;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(k_NUM_RECORDS, s_allocator_p);
    const char*     queueKey1 = "ABCDE12345";
    const char*     queueKey2 = "12345ABCDE";
    GuidVectorType  queueKey1GUIDS(s_allocator_p);
    journalFile.addJournalRecordsWithTwoQueueKeys(&records,
                                                  &queueKey1GUIDS,
                                                  queueKey1,
                                                  queueKey2);

    // Configure parameters to search messages by 'queue1' name
    bmqp_ctrlmsg::QueueInfo queueInfo(s_allocator_p);
    queueInfo.uri() = "queue1";
    mqbu::StorageKey key(mqbu::StorageKey::HexRepresentation(), queueKey1);
    for (int i = 0; i < mqbu::StorageKey::e_KEY_LENGTH_BINARY; i++) {
        queueInfo.key().push_back(key.data()[i]);
    }
    QueueMap qMap(s_allocator_p);

    Parameters params(s_allocator_p);
    params.d_queueName.push_back("queue1");
    params.d_queueMap.insert(queueInfo);

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(journalFile),
        s_allocator_p);

    // Run search
    mwcu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output
    mwcu::MemOutStream             expectedStream(s_allocator_p);
    GuidVectorType::const_iterator guidIt = queueKey1GUIDS.cbegin();
    for (; guidIt != queueKey1GUIDS.cend(); ++guidIt) {
        outputGuidString(expectedStream, *guidIt);
    }
    size_t foundMessagesCount = queueKey1GUIDS.size();
    expectedStream << foundMessagesCount << " message GUID(s) found."
                   << bsl::endl;

    ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    mwctst::TestHelper::printTestName(
        "SEARCH MESSAGES BY QUEUE NAME AND QUEUE KEY TEST");

    // Simulate journal file
    const size_t    k_NUM_RECORDS = 15;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(k_NUM_RECORDS, s_allocator_p);
    const char*     queueKey1 = "ABCDE12345";
    const char*     queueKey2 = "12345ABCDE";
    GuidVectorType  queueKey1GUIDS(s_allocator_p);
    journalFile.addJournalRecordsWithTwoQueueKeys(&records,
                                                  &queueKey1GUIDS,
                                                  queueKey1,
                                                  queueKey2,
                                                  true);

    // Configure parameters to search messages by 'queue1' name and queueKey2
    // key.
    bmqp_ctrlmsg::QueueInfo queueInfo(s_allocator_p);
    queueInfo.uri() = "queue1";
    mqbu::StorageKey key(mqbu::StorageKey::HexRepresentation(), queueKey1);
    for (int i = 0; i < mqbu::StorageKey::e_KEY_LENGTH_BINARY; i++) {
        queueInfo.key().push_back(key.data()[i]);
    }
    QueueMap qMap(s_allocator_p);

    Parameters params(s_allocator_p);
    params.d_queueName.push_back("queue1");
    params.d_queueMap.insert(queueInfo);
    params.d_queueKey.push_back(queueKey2);

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(journalFile),
        s_allocator_p);

    // Run search
    mwcu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output
    mwcu::MemOutStream             expectedStream(s_allocator_p);
    GuidVectorType::const_iterator guidIt = queueKey1GUIDS.cbegin();
    for (; guidIt != queueKey1GUIDS.cend(); ++guidIt) {
        outputGuidString(expectedStream, *guidIt);
    }
    size_t foundMessagesCount = queueKey1GUIDS.size();
    expectedStream << foundMessagesCount << " message GUID(s) found."
                   << bsl::endl;

    ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    mwctst::TestHelper::printTestName("SEARCH MESSAGES BY TIMESTAMP TEST");

    // Simulate journal file
    const size_t    k_NUM_RECORDS = 50;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(k_NUM_RECORDS, s_allocator_p);
    journalFile.addAllTypesRecords(&records);
    const bsls::Types::Uint64 ts1 = 10 * journalFile.timestampIncrement();
    const bsls::Types::Uint64 ts2 = 40 * journalFile.timestampIncrement();

    // Configure parameters to search messages by timestamps
    Parameters params(s_allocator_p);
    params.d_timestampGt = ts1;
    params.d_timestampLt = ts2;
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(journalFile),
        s_allocator_p);

    // Get GUIDs of messages with matching timestamps and prepare expected
    // output
    mwcu::MemOutStream expectedStream(s_allocator_p);

    bsl::list<NodeType>::const_iterator recordIter = records.begin();
    bsl::size_t                         msgCnt     = 0;
    while (recordIter++ != records.end()) {
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
    mwcu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    mwctst::TestHelper::printTestName("PRINT MESSAGE DETAILS TEST");

    s_ignoreCheckDefAlloc = true;
    // Disable default allocator check for this test until we can debug
    // it on AIX/Solaris

    // Simulate journal file
    const size_t    k_NUM_RECORDS = 15;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(k_NUM_RECORDS, s_allocator_p);
    GuidVectorType  confirmedGUIDS(s_allocator_p);
    journalFile.addJournalRecordsWithOutstandingAndConfirmedMessages(
        &records,
        &confirmedGUIDS,
        false);

    // Configure parameters to print message details
    Parameters params(s_allocator_p);
    params.d_details = true;
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(journalFile),
        s_allocator_p);

    // Run search
    mwcu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    // Check that substrings are present in resultStream in correct order
    bsl::string resultString(resultStream.str(), s_allocator_p);
    size_t      startIdx             = 0;
    const char* messageRecordCaption = "MESSAGE Record";
    const char* confirmRecordCaption = "CONFIRM Record";
    const char* deleteRecordCaption  = "DELETE Record";
    for (size_t i = 0; i < confirmedGUIDS.size(); i++) {
        // Check Message type
        size_t foundIdx = resultString.find(messageRecordCaption, startIdx);
        ASSERT_D(messageRecordCaption, (foundIdx != bsl::string::npos));
        ASSERT_D(messageRecordCaption, (foundIdx >= startIdx));
        startIdx = foundIdx + bsl::strlen(messageRecordCaption);

        // Check GUID
        mwcu::MemOutStream ss(s_allocator_p);
        outputGuidString(ss, confirmedGUIDS.at(i));
        bsl::string guidStr(ss.str(), s_allocator_p);
        foundIdx = resultString.find(guidStr, startIdx);
        ASSERT_D(guidStr, (foundIdx != bsl::string::npos));
        ASSERT_D(guidStr, (foundIdx >= startIdx));
        startIdx = foundIdx + guidStr.length();

        // Check Confirm type
        foundIdx = resultString.find(confirmRecordCaption, startIdx);
        ASSERT_D(confirmRecordCaption, (foundIdx != bsl::string::npos));
        ASSERT_D(confirmRecordCaption, (foundIdx >= startIdx));
        startIdx = foundIdx + bsl::strlen(messageRecordCaption);

        // Check Delete type
        foundIdx = resultString.find(deleteRecordCaption, startIdx);
        ASSERT_D(deleteRecordCaption, (foundIdx != bsl::string::npos));
        ASSERT_D(deleteRecordCaption, (foundIdx >= startIdx));
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
    mwctst::TestHelper::printTestName(
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
    bsl::vector<unsigned int> messageOffsets(s_allocator_p);
    char*                     pd = addDataRecords(s_allocator_p,
                              &mfdData,
                              &fileHeader,
                              MESSAGES,
                              k_NUM_MSGS,
                              messageOffsets);
    ASSERT(pd != 0);
    ASSERT_GT(mfdData.fileSize(), 0ULL);
    // Create data file iterator
    DataFileIterator dataIt(&mfdData, fileHeader);

    // Simulate journal file
    const size_t k_NUM_RECORDS =
        k_NUM_MSGS * 2;  // k_NUM_MSGS records + k_NUM_MSGS deletion records

    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(k_NUM_RECORDS, s_allocator_p);
    GuidVectorType  confirmedGUIDS(s_allocator_p);
    journalFile.addJournalRecordsWithConfirmedMessagesWithDifferentOrder(
        &records,
        &confirmedGUIDS,
        k_NUM_MSGS,
        messageOffsets);

    // Configure parameters to search confirmed messages GUIDs with dumping
    // messages payload.
    Parameters params(s_allocator_p);
    params.d_confirmed   = true;
    params.d_dumpPayload = true;
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(journalFile),
        s_allocator_p);
    EXPECT_CALL(static_cast<FileManagerMock&>(*fileManager),
                dataFileIterator())
        .WillRepeatedly(Return(&dataIt));

    // Run search
    mwcu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    // Prepare expected data
    bsl::string              resultString(resultStream.str(), s_allocator_p);
    size_t                   startIdx = 0;
    bsl::vector<bsl::string> expectedPayloadSubstring(s_allocator_p);
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
        mwcu::MemOutStream ss(s_allocator_p);
        outputGuidString(ss, guid);
        bsl::string guidStr(ss.str(), s_allocator_p);
        size_t      foundIdx = resultString.find(guidStr, startIdx);

        ASSERT_D(guidStr, (foundIdx != bsl::string::npos));
        ASSERT_D(guidStr, (foundIdx >= startIdx));

        startIdx = foundIdx + guidStr.length();

        // Check payload dump substring
        bsl::string dumpStr = expectedPayloadSubstring[i];
        foundIdx            = resultString.find(dumpStr, startIdx);

        ASSERT_D(dumpStr, (foundIdx != bsl::string::npos));
        ASSERT_D(guidStr, (foundIdx >= startIdx));
        startIdx = foundIdx + dumpStr.length();
    }

    s_allocator_p->deallocate(pd);
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
    mwctst::TestHelper::printTestName("OUTPUT SUMMARY TEST");

    // Simulate journal file
    const size_t    k_NUM_RECORDS = 15;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(k_NUM_RECORDS, s_allocator_p);
    GuidVectorType  partiallyConfirmedGUIDS(s_allocator_p);
    journalFile.addJournalRecordsWithPartiallyConfirmedMessages(
        &records,
        &partiallyConfirmedGUIDS);

    // Configure parameters to output summary
    Parameters params(s_allocator_p);
    params.d_summary = true;
    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(journalFile),
        s_allocator_p);

    // Run search
    mwcu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output
    mwcu::MemOutStream expectedStream(s_allocator_p);
    expectedStream
        << "5 message(s) found.\nNumber of confirmed messages: 3\nNumber of "
           "partially confirmed messages: 2\n"
           "Number of outstanding messages: 2\nOutstanding ratio: 40% (2/5)\n";

    bsl::string res(resultStream.str(), s_allocator_p);
    ASSERT(res.starts_with(expectedStream.str()));
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
    mwctst::TestHelper::printTestName("TIMESTAMP SEARCH TEST");

    // Simulate journal file
    const size_t    k_NUM_RECORDS = 50;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(k_NUM_RECORDS, s_allocator_p);
    journalFile.addAllTypesRecords(&records);

    struct ResultChecker {
        static void check(mqbs::JournalFileIterator& it,
                          const bsls::Types::Uint64& ts)
        {
            ASSERT_GT(it.recordHeader().timestamp(), ts);
            ASSERT(!it.isReverseMode());
            // Check previous record
            it.flipDirection();
            ASSERT_EQ(it.nextRecord(), 1);
            ASSERT_LE(it.recordHeader().timestamp(), ts);
            // Set 'it' to its original state
            it.flipDirection();
            ASSERT_EQ(it.nextRecord(), 1);
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
        ASSERT_EQ(journalFileIt.nextRecord(), 1);
        ASSERT_EQ(m_bmqstoragetool::moveToLowerBound(&journalFileIt, ts), 1);
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
        ASSERT_EQ(journalFileIt.nextRecord(), 1);
        ASSERT_EQ(journalFileIt.advance(k_NUM_RECORDS / 2), 1);

        // Find record with lower timestamp than the record pointed by the
        // specified iterator, which is initially forward
        ASSERT_GT(journalFileIt.recordHeader().timestamp(), ts1);
        ASSERT_EQ(m_bmqstoragetool::moveToLowerBound(&journalFileIt, ts1), 1);
        ResultChecker::check(journalFileIt, ts1);

        // Find record with higher timestamp than the record pointed by the
        // specified iterator, which is initially forward
        ASSERT_LT(journalFileIt.recordHeader().timestamp(), ts2);
        ASSERT_EQ(m_bmqstoragetool::moveToLowerBound(&journalFileIt, ts2), 1);
        ResultChecker::check(journalFileIt, ts2);

        // Find record with lower timestamp than the record pointed by the
        // specified iterator, which is initially backward
        ASSERT_GT(journalFileIt.recordHeader().timestamp(), ts1);
        journalFileIt.flipDirection();
        ASSERT(journalFileIt.isReverseMode());
        ASSERT_EQ(m_bmqstoragetool::moveToLowerBound(&journalFileIt, ts1), 1);
        ResultChecker::check(journalFileIt, ts1);

        // Find record with higher timestamp than the record pointed by the
        // specified iterator, which is initially backward
        ASSERT_LT(journalFileIt.recordHeader().timestamp(), ts2);
        journalFileIt.flipDirection();
        ASSERT(journalFileIt.isReverseMode());
        ASSERT_EQ(m_bmqstoragetool::moveToLowerBound(&journalFileIt, ts2), 1);
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
        ASSERT_EQ(journalFileIt.nextRecord(), 1);
        ASSERT_EQ(m_bmqstoragetool::moveToLowerBound(&journalFileIt, ts), 0);
        ASSERT_EQ(journalFileIt.recordIndex(), k_NUM_RECORDS - 1);
        ASSERT_LT(journalFileIt.recordHeader().timestamp(), ts);
        ASSERT(!journalFileIt.isReverseMode());
    }

    {
        // Timestamp less than first record in the file
        const bsls::Types::Uint64 ts = journalFile.timestampIncrement() / 2;
        mqbs::JournalFileIterator journalFileIt(
            &journalFile.mappedFileDescriptor(),
            journalFile.fileHeader(),
            false);
        // Move the iterator to the beginning of the file
        ASSERT_EQ(journalFileIt.nextRecord(), 1);
        ASSERT_EQ(m_bmqstoragetool::moveToLowerBound(&journalFileIt, ts), 1);
        ASSERT_EQ(journalFileIt.recordIndex(), 0U);
        ASSERT_GT(journalFileIt.recordHeader().timestamp(), ts);
        ASSERT(!journalFileIt.isReverseMode());
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

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
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
