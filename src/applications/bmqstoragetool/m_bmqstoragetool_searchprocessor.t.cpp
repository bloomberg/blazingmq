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
#include <m_bmqstoragetool_searchprocessor.h>

// BMQ
#include <bmqt_messageguid.h>

// MQB
#include <mqbs_filestoreprotocol.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbs_memoryblock.h>
#include <mqbs_offsetptr.h>
#include <mqbu_messageguidutil.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_list.h>
#include <bsl_utility.h>
#include <bslma_default.h>
#include <bsls_alignedbuffer.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace m_bmqstoragetool;
using namespace bsl;
using namespace mqbs;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

typedef bsls::AlignedBuffer<FileStoreProtocol::k_JOURNAL_RECORD_SIZE>
    RecordBufferType;

typedef bsl::pair<RecordType::Enum, RecordBufferType> NodeType;

typedef bsl::list<NodeType> RecordsListType;

void addJournalRecords(MemoryBlock*  block,
                FileHeader*          fileHeader,
                bsls::Types::Uint64* lastRecordPos,
                bsls::Types::Uint64* lastSyncPtPos,
                RecordsListType*     records,
                unsigned int         numRecords)
{
    bsls::Types::Uint64 currPos = 0;

    OffsetPtr<FileHeader> fh(*block, currPos);
    new (fh.get()) FileHeader();
    *fileHeader = *fh;
    currPos += sizeof(FileHeader);

    OffsetPtr<JournalFileHeader> jfh(*block, currPos);
    new (jfh.get()) JournalFileHeader();  // Default values are ok
    currPos += sizeof(JournalFileHeader);

    for (unsigned int i = 1; i <= numRecords; ++i) {
        *lastRecordPos = currPos;

        unsigned int remainder = i % 5;
        if (0 == remainder) {
            bmqt::MessageGUID g;
            mqbu::MessageGUIDUtil::generateGUID(&g);
            OffsetPtr<MessageRecord> rec(*block, currPos);
            new (rec.get()) MessageRecord();
            rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
            rec->setRefCount(i % FileStoreProtocol::k_MAX_MSG_REF_COUNT_HARD)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "abcde"))
                .setFileKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "12345"))
                .setMessageOffsetDwords(i)
                .setMessageGUID(g)
                .setCrc32c(i)
                .setCompressionAlgorithmType(
                    bmqt::CompressionAlgorithmType::e_NONE)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_MESSAGE, buf));
        }
        else if (1 == remainder) {
            // ConfRec
            bmqt::MessageGUID g;
            mqbu::MessageGUIDUtil::generateGUID(&g);
            OffsetPtr<ConfirmRecord> rec(*block, currPos);
            new (rec.get()) ConfirmRecord();
            rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
            rec->setReason(ConfirmReason::e_REJECTED)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "abcde"))
                .setAppKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "appid"))
                .setMessageGUID(g)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_CONFIRM, buf));
        }
        else if (2 == remainder) {
            // DelRec
            bmqt::MessageGUID g;
            mqbu::MessageGUIDUtil::generateGUID(&g);
            OffsetPtr<DeletionRecord> rec(*block, currPos);
            new (rec.get()) DeletionRecord();
            rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
            rec->setDeletionRecordFlag(DeletionRecordFlag::e_IMPLICIT_CONFIRM)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "abcde"))
                .setMessageGUID(g)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_DELETION, buf));
        }
        else if (3 == remainder) {
            // QueueOpRec
            OffsetPtr<QueueOpRecord> rec(*block, currPos);
            new (rec.get()) QueueOpRecord();
            rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
            rec->setFlags(3)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "abcde"))
                .setAppKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "appid"))
                .setType(QueueOpType::e_PURGE)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_QUEUE_OP, buf));
        }
        else {
            // JournalOpRec.SyncPoint
            *lastSyncPtPos = currPos;
            OffsetPtr<JournalOpRecord> rec(*block, currPos);
            new (rec.get()) JournalOpRecord(JournalOpType::e_SYNCPOINT,
                                            SyncPointType::e_REGULAR,
                                            1234567,  // seqNum
                                            25,       // leaderTerm
                                            121,      // leaderNodeId
                                            8800,     // dataFilePosition
                                            100,      // qlistFilePosition
                                            RecordHeader::k_MAGIC);

            rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_JOURNAL_OP, buf));
        }

        currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    }
}

}  // close unnamed namespace



// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the component.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    bsl::string journalFile("/home/aivanov71/projects/pr/blazingmq/build/blazingmq/src/applications/bmqbrkr/localBMQ/storage/local/bmq_0.20231121_091839.bmq_journal", s_allocator_p);
    auto sp = SearchProcessor(journalFile, s_allocator_p);
    sp.process(bsl::cout);

    ASSERT(sp.getJournalFileIter().isValid());
}

static void test2_searchGuidTest()
// ------------------------------------------------------------------------
// SEARCH GUID TEST
//
// Concerns:
//   Search message by GUID in journal file.
//
// Testing:
//   SearchProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SEARCH GUID");
    
    // Simulate journal file
    unsigned int numRecords = 15;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    char*       p = static_cast<char*>(s_allocator_p->allocate(totalSize));
    MemoryBlock block(p, totalSize);
    FileHeader  fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;

    RecordsListType records(s_allocator_p);

    addJournalRecords(&block,
               &fileHeader,
               &lastRecordPos,
               &lastSyncPtPos,
               &records,
               numRecords);

    // Create JournalFileIterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);
    JournalFileIterator it(&mfd, fileHeader, false);

    // Get list of message GUIDs for searching
    SearchParameters searchParameters(s_allocator_p);
    bsl::list<NodeType>::const_iterator recordIter = records.begin();
    int msgCnt = 0;
    while(recordIter++ != records.end()) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE){
            if (msgCnt++ % 2) continue; // Skip odd messages
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(recordIter->second.buffer());
            char buf[bmqt::MessageGUID::e_SIZE_HEX];
            msg.messageGUID().toHex(buf);
            bsl::string guid(buf, s_allocator_p);
            searchParameters.searchGuids.push_back(guid);
        }
    }

    bsl::cout << searchParameters.searchGuids.size() << "GUIDs" << bsl::endl;
    for(auto& g : searchParameters.searchGuids) {
        bsl::cout << "GUID: " << g << bsl::endl;
    }

    auto searchProcessor = SearchProcessor(it, searchParameters, s_allocator_p);
    searchProcessor.process(bsl::cout);

    ASSERT(searchProcessor.getJournalFileIter().isValid());

    s_allocator_p->deallocate(p);
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
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
