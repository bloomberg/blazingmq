// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbs_datafileiterator.t.cpp                                        -*-C++-*-
#include <mqbs_datafileiterator.h>

// MQB
#include <mqbs_filestoreprotocol.h>
#include <mqbs_memoryblock.h>
#include <mqbs_offsetptr.h>

// BMQ
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>

// BDE
#include <bsl_cstddef.h>
#include <bsl_cstring.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_utility.h>
#include <bslma_default.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

using namespace BloombergLP;
using namespace bsl;
using namespace mqbs;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

struct Message {
    int         d_line;
    const char* d_appData_p;
    const char* d_options_p;
};

static char* addRecords(bslma::Allocator*     ta,
                        MappedFileDescriptor* mfd,
                        FileHeader*           fileHeader,
                        const Message*        messages,
                        const unsigned int    numMessages)
{
    bsls::Types::Uint64 currPos = 0;
    const unsigned int  dhSize  = sizeof(DataHeader);
    unsigned int totalSize      = sizeof(FileHeader) + sizeof(DataFileHeader);

    // Have to compute the 'totalSize' we need for the 'MemoryBlock' based on
    // the padding that we need for each record.

    for (unsigned int i = 0; i < numMessages; i++) {
        unsigned int optionsLen = bsl::strlen(messages[i].d_options_p);
        BSLS_ASSERT_OPT(0 == optionsLen % bmqp::Protocol::k_WORD_SIZE);

        unsigned int appDataLen     = bsl::strlen(messages[i].d_appData_p);
        int          appDataPadding = 0;
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
        OffsetPtr<DataHeader> dh(block, currPos);
        new (dh.get()) DataHeader();

        unsigned int optionsLen = bsl::strlen(messages[i].d_options_p);
        dh->setOptionsWords(optionsLen / bmqp::Protocol::k_WORD_SIZE);
        currPos += sizeof(DataHeader);

        char* destination = reinterpret_cast<char*>(block.base() + currPos);
        bsl::memcpy(destination, messages[i].d_options_p, optionsLen);
        currPos += optionsLen;
        destination += optionsLen;

        unsigned int appDataLen = bsl::strlen(messages[i].d_appData_p);
        int          appDataPad = 0;
        bmqp::ProtocolUtil::calcNumDwordsAndPadding(&appDataPad,
                                                    appDataLen + optionsLen +
                                                        dhSize);

        bsl::memcpy(destination, messages[i].d_appData_p, appDataLen);
        currPos += appDataLen;
        destination += appDataLen;
        bmqp::ProtocolUtil::appendPaddingDwordRaw(destination, appDataPad);
        currPos += appDataPad;

        dh->setMessageWords(dh->headerWords() +
                            ((appDataLen + appDataPad + optionsLen) /
                             bmqp::Protocol::k_WORD_SIZE));
    }

    *fileHeader = *fh;

    return p;
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Testing:
//   Basic functionality of the component
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    {
        // Default Object
        DataFileIterator it;
        ASSERT_EQ(false, it.isValid());
        ASSERT_EQ(-1, it.nextRecord());
    }
    {
        // Empty file -- forward iterator
        MappedFileDescriptor mfd;
        mfd.setMappingSize(1);

        FileHeader       fh;
        DataFileIterator it(&mfd, fh);
        ASSERT_EQ(false, it.isValid());
        ASSERT_EQ(-1, it.nextRecord());
    }
    {
        // No records
        FileHeader           fh;
        MappedFileDescriptor mfd;
        char* p = addRecords(bmqtst::TestHelperUtil::allocator(),
                             &mfd,
                             &fh,
                             NULL,  // No messages
                             0);

        DataFileIterator it(&mfd, fh);
        ASSERT_EQ(true, it.isValid());
        ASSERT_EQ(-2, it.nextRecord());

        bmqtst::TestHelperUtil::allocator()->deallocate(p);
    }
    {
        // No records - reset
        FileHeader           fh;
        MappedFileDescriptor mfd;
        char* p = addRecords(bmqtst::TestHelperUtil::allocator(),
                             &mfd,
                             &fh,
                             NULL,  // No messages
                             0);

        DataFileIterator it(&mfd, fh);
        ASSERT_EQ(true, it.isValid());

        it.reset(&mfd, fh);

        ASSERT_EQ(true, it.isValid());
        ASSERT_EQ(-2, it.nextRecord());

        bmqtst::TestHelperUtil::allocator()->deallocate(p);
    }
}

static void test2_forwardIteration()
// ------------------------------------------------------------------------
// Testing:
//   Forward iteration.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("FORWARD ITERATION");

    const Message MESSAGES[] = {
        {
            L_,
            "APP_DATA_APP_DATA_APP_DATA",
            "OPTIONS_OPTIONS_"  // Word aligned
        },
        {
            L_,
            "APP_DATA_APP_DATA_APP_DATA_APP_DATA_APP_DATA",
            "OPTIONS_OPTIONS_OPTIONS_OPTIONS_"  // Word aligned
        },
        {L_,
         "APP_DATA_APP_DATA_APP_DATA_APP_DATA_APP_DATA_APP_DATA_APP",
         "OPTIONS_OPTIONS_OPTIONS_OPTIONS_OPTIONS_OPTIONS_OPTIONS_"},
        {L_, "APP", ""},
        {L_, "APP_DATA_APP_DATA_APP_DATA_APP_DATA_APP_DATA_APP_DATA", ""},
        {L_, "A", "OPTIONS_OPTIONS_OPTIONS_OPTIONS_OPTIONS_OPTIONS_OPTIONS_"},
    };

    const unsigned int k_NUM_MSGS = sizeof(MESSAGES) / sizeof(*MESSAGES);

    FileHeader           fileHeader;
    MappedFileDescriptor mfd;
    char*                p = addRecords(bmqtst::TestHelperUtil::allocator(),
                         &mfd,
                         &fileHeader,
                         MESSAGES,
                         k_NUM_MSGS);

    ASSERT(p != 0);
    ASSERT_GT(mfd.fileSize(), 0ULL);

    // Create iterator
    DataFileIterator it(&mfd, fileHeader);

    ASSERT_EQ(it.hasRecordSizeRemaining(), true);
    ASSERT_EQ(it.isValid(), true);
    ASSERT_EQ(it.mappedFileDescriptor(), &mfd);
    ASSERT_EQ(it.isReverseMode(), false);

    unsigned int i      = 0;
    unsigned int offset = sizeof(FileHeader) + sizeof(DataFileHeader);
    ASSERT_EQ(offset, it.firstRecordPosition());

    while (it.nextRecord() == 1) {
        ASSERT_EQ_D(i, it.recordOffset(), offset);
        ASSERT_EQ_D(i, it.recordIndex(), i);

        const DataHeader& dh     = it.dataHeader();
        const char*       data   = 0;
        unsigned int      length = 0;
        it.loadApplicationData(&data, &length);

        ASSERT_EQ_D(i, bsl::strlen(MESSAGES[i].d_appData_p), length);
        ASSERT_EQ_D(i, bsl::memcmp(data, MESSAGES[i].d_appData_p, length), 0);

        it.loadOptions(&data, &length);
        ASSERT_EQ_D(i, bsl::strlen(MESSAGES[i].d_options_p), length);
        ASSERT_EQ_D(i, bsl::memcmp(data, MESSAGES[i].d_options_p, length), 0);

        offset += (dh.messageWords() * bmqp::Protocol::k_WORD_SIZE);
        ++i;
    }

    ASSERT_EQ(i, k_NUM_MSGS);

    ASSERT(it.nextRecord() != 1);
    ASSERT(!it.isValid());

    bmqtst::TestHelperUtil::allocator()->deallocate(p);
}

static void test3_reverseIteration()
// ------------------------------------------------------------------------
// Testing:
//   Reverse iteration.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("REVERSE ITERATION");

    const Message MESSAGES[] = {
        {
            L_,
            "APP_DATA_APP_DATA_APP_DATA",
            "OPTIONS_OPTIONS_"  // Word aligned
        },
        {
            L_,
            "APP_DATA_APP_DATA_APP_DATA_APP_DATA_APP_DATA",
            "OPTIONS_OPTIONS_OPTIONS_OPTIONS_"  // Word aligned
        },
        {L_,
         "APP_DATA_APP_DATA_APP_DATA_APP_DATA_APP_DATA_APP_DATA_APP",
         "OPTIONS_OPTIONS_OPTIONS_OPTIONS_OPTIONS_OPTIONS_OPTIONS_"},
        {L_, "APP", ""},
        {L_, "APP_DATA_APP_DATA_APP_DATA_APP_DATA_APP_DATA_APP_DATA", ""},
        {L_, "A", "OPTIONS_OPTIONS_OPTIONS_OPTIONS_OPTIONS_OPTIONS_OPTIONS_"},
    };

    const unsigned int k_NUM_MSGS = sizeof(MESSAGES) / sizeof(*MESSAGES);

    FileHeader           fileHeader;
    MappedFileDescriptor mfd;
    char*                p = addRecords(bmqtst::TestHelperUtil::allocator(),
                         &mfd,
                         &fileHeader,
                         MESSAGES,
                         k_NUM_MSGS);

    ASSERT(p != 0);
    ASSERT_GT(mfd.fileSize(), 0ULL);

    // Create iterator
    DataFileIterator it(&mfd, fileHeader);

    ASSERT_EQ(it.hasRecordSizeRemaining(), true);
    ASSERT_EQ(it.isValid(), true);
    ASSERT_EQ(it.mappedFileDescriptor(), &mfd);
    ASSERT_EQ(it.isReverseMode(), false);

    it.flipDirection();

    ASSERT_EQ(it.isReverseMode(), true);
    ASSERT_EQ(it.hasRecordSizeRemaining(), false);

    it.flipDirection();

    // Iterate to the second-last record in the file.  We want to leave one
    // record so that we can call flipDirection() after that.

    while (it.hasRecordSizeRemaining()) {
        it.nextRecord();
    }

    // 'it' is now pointing to 'D' (the 2nd-last record).
    ASSERT(it.isValid());
    ASSERT(!it.isReverseMode());

    it.flipDirection();

    ASSERT(it.isValid());
    ASSERT(it.isReverseMode());
    ASSERT(it.hasRecordSizeRemaining());

    // Start iterating the records in the reverse direction.  Note that we
    // will start with record 'D', which is 6th record, and thus, at index
    // 5.

    unsigned int i = 0;
    while (it.nextRecord() == 1) {
        ASSERT_EQ_D(i, k_NUM_MSGS - i - 2, it.recordIndex());

        const char*  data;
        unsigned int length;
        it.loadApplicationData(&data, &length);

        int rc = bsl::memcmp(data,
                             MESSAGES[k_NUM_MSGS - i - 2].d_appData_p,
                             length);
        ASSERT_EQ_D(i, rc, 0);

        it.loadOptions(&data, &length);
        rc = bsl::memcmp(data,
                         MESSAGES[k_NUM_MSGS - i - 2].d_options_p,
                         length);
        ASSERT_EQ_D(i, rc, 0);

        ++i;
    }

    ASSERT_EQ(i, k_NUM_MSGS - 1);
    ASSERT_EQ(it.isValid(), false);

    bmqtst::TestHelperUtil::allocator()->deallocate(p);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_reverseIteration(); break;
    case 2: test2_forwardIteration(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
