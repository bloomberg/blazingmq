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

// mqbs_qlistfileiterator.t.cpp                                       -*-C++-*-
#include <mqbs_qlistfileiterator.h>

// MQB
#include <mqbs_filestore.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbs_memoryblock.h>
#include <mqbs_offsetptr.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqt_uri.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bsl_cstdlib.h>
#include <bsl_cstring.h>
#include <bsl_limits.h>
#include <bsl_vector.h>
#include <bsls_assert.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>
#include <bsl_ostream.h>

// CONVINIENCE
using namespace BloombergLP;
using namespace bsl;
using namespace mqbs;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

char* addRecords(bslma::Allocator*                                  ta,
                 MappedFileDescriptor*                              mfd,
                 FileHeader*                                        fileHeader,
                 const bsl::vector<bmqt::Uri>&                      queueUris,
                 const bsl::vector<bsl::vector<bsl::string> >&      appIdsVec,
                 const bsl::vector<mqbu::StorageKey>&               queueKeys,
                 const bsl::vector<bsl::vector<mqbu::StorageKey> >& appKeysVec)
{
    BSLS_ASSERT_SAFE(queueUris.size() == appIdsVec.size());
    BSLS_ASSERT_SAFE(queueUris.size() == queueKeys.size());
    BSLS_ASSERT_SAFE(queueUris.size() == appKeysVec.size());

    bsl::vector<unsigned int>               queueUriWords(ta);
    bsl::vector<unsigned int>               queueUriPaddings(ta);
    bsl::vector<bsl::vector<unsigned int> > appIdWordsVec(ta);
    bsl::vector<bsl::vector<unsigned int> > appIdPaddingsVec(ta);

    appIdWordsVec.resize(appIdsVec.size());
    appIdPaddingsVec.resize(appIdsVec.size());

    // Have to compute the 'totalSize' needed for the 'MemoryBlock' based on
    // the alignment.

    unsigned int numRecords = queueUris.size();
    unsigned int totalSize  = sizeof(FileHeader) + sizeof(QlistFileHeader);

    for (unsigned int i = 0; i < numRecords; i++) {
        int          padding = 0;
        int          words   = 0;
        unsigned int length  = queueUris[i].asString().length();

        totalSize += sizeof(QueueRecordHeader);
        totalSize += FileStoreProtocol::k_HASH_LENGTH;  // QueueHash

        words = bmqp::ProtocolUtil::calcNumWordsAndPadding(&padding, length);
        totalSize += padding + length;
        queueUriWords.push_back(static_cast<unsigned int>(words));
        queueUriPaddings.push_back(static_cast<unsigned int>(padding));

        const bsl::vector<bsl::string>&      appIds  = appIdsVec[i];
        const bsl::vector<mqbu::StorageKey>& appKeys = appKeysVec[i];

        BSLS_ASSERT_OPT(appIds.size() == appKeys.size());

        bsl::vector<unsigned int>& appIdWords    = appIdWordsVec[i];
        bsl::vector<unsigned int>& appIdPaddings = appIdPaddingsVec[i];

        for (size_t j = 0; j < appIds.size(); ++j) {
            length = appIds[j].length();

            if (0 == length) {
                appIdWords.push_back(0);
                appIdPaddings.push_back(0);
                continue;  // CONTINUE
            }

            totalSize += sizeof(AppIdHeader);

            padding = 0;
            words   = bmqp::ProtocolUtil::calcNumWordsAndPadding(&padding,
                                                               length);
            totalSize += padding + length;
            appIdWords.push_back(static_cast<unsigned int>(words));
            appIdPaddings.push_back(static_cast<unsigned int>(padding));

            totalSize += FileStoreProtocol::k_HASH_LENGTH;  // AppIdHash
        }

        totalSize += sizeof(unsigned int);  // magic length
    }

    // Allocate the memory for the data now.
    char* p = static_cast<char*>(ta->allocate(totalSize));

    // Create the 'MemoryBlock'.
    MemoryBlock block(p, totalSize);

    // Set the MFD
    mfd->setFd(-1);
    mfd->setBlock(block);
    mfd->setFileSize(totalSize);

    // Current position in the memory block.
    unsigned int currPos = 0;

    // Add the 'FileHeader'.
    OffsetPtr<FileHeader> fh(block, currPos);
    new (fh.get()) FileHeader();
    fh->setHeaderWords(sizeof(FileHeader) / bmqp::Protocol::k_WORD_SIZE);
    fh->setMagic1(FileHeader::k_MAGIC1);
    fh->setMagic2(FileHeader::k_MAGIC2);
    currPos += sizeof(FileHeader);

    // Add the 'QlistFileHeader'
    OffsetPtr<QlistFileHeader> qfh(block, currPos);
    new (qfh.get()) QlistFileHeader();
    currPos += sizeof(QlistFileHeader);

    for (unsigned int i = 0; i < numRecords; i++) {
        unsigned int totalRecordLength = sizeof(QueueRecordHeader);
        unsigned int uriLength         = queueUris[i].asString().length();

        totalRecordLength += FileStoreProtocol::k_HASH_LENGTH;
        totalRecordLength += uriLength;
        totalRecordLength += queueUriPaddings[i];

        const bsl::vector<unsigned int>& appIdWords    = appIdWordsVec[i];
        const bsl::vector<unsigned int>& appIdPaddings = appIdPaddingsVec[i];
        const bsl::vector<bsl::string>&  appIds        = appIdsVec[i];
        const bsl::vector<mqbu::StorageKey>& appKeys   = appKeysVec[i];

        size_t numAppIds = 0;

        for (size_t j = 0; j < appIdWords.size(); ++j) {
            if (0 != appIdWords[j]) {
                ++numAppIds;
                totalRecordLength += sizeof(AppIdHeader);
                totalRecordLength += appIds[j].length();
                totalRecordLength += appIdPaddings[j];
                totalRecordLength += FileStoreProtocol::k_HASH_LENGTH;
            }
        }

        totalRecordLength += sizeof(unsigned int);  // magic

        // Append the 'QueueRecordHeader'
        OffsetPtr<QueueRecordHeader> qrh(block, currPos);
        new (qrh.get()) QueueRecordHeader();
        qrh->setQueueUriLengthWords(queueUriWords[i]);
        qrh->setHeaderWords(sizeof(QueueRecordHeader) /
                            bmqp::Protocol::k_WORD_SIZE);
        qrh->setQueueRecordWords(totalRecordLength /
                                 bmqp::Protocol::k_WORD_SIZE);
        qrh->setNumAppIds(numAppIds);
        currPos += sizeof(QueueRecordHeader);

        // Append the 'QueueRecord'
        // 1) Append the 'QueueUri'
        OffsetPtr<char> quri(block, currPos);
        bsl::memcpy(quri.get(), queueUris[i].asString().c_str(), uriLength);
        currPos += uriLength;

        // 2) Append padding after 'QueueUri'
        bmqp::ProtocolUtil::appendPaddingRaw(quri.get() + uriLength,
                                             queueUriPaddings[i]);
        currPos += queueUriPaddings[i];

        // 3) Append 'QueueUriHash'
        OffsetPtr<char> quriHash(block, currPos);
        bsl::memcpy(quriHash.get(),
                    queueKeys[i].data(),
                    mqbu::StorageKey::e_KEY_LENGTH_BINARY);
        currPos += FileStoreProtocol::k_HASH_LENGTH;

        // 4) Append 'AppId' and 'AppKey'
        for (size_t j = 0; j < appIdWords.size(); ++j) {
            if (0 == appIdWords[j]) {
                continue;  // CONTINUE
            }

            // Append AppIdHeader
            OffsetPtr<AppIdHeader> appIdHeader(block, currPos);
            new (appIdHeader.get()) AppIdHeader();
            appIdHeader->setAppIdLengthWords(appIdWords[j]);
            currPos += sizeof(AppIdHeader);

            // App ID
            OffsetPtr<char> appIdPtr(block, currPos);
            bsl::memcpy(appIdPtr.get(), appIds[j].c_str(), appIds[j].length());
            currPos += appIds[j].length();

            // Append padding after App ID
            bmqp::ProtocolUtil::appendPaddingRaw(
                (appIdPtr.get() + appIds[j].length()),
                appIdPaddings[j]);
            currPos += appIdPaddings[j];

            // App Key
            OffsetPtr<char> appKeyPtr(block, currPos);
            bsl::memcpy(appKeyPtr.get(),
                        appKeys[j].data(),
                        mqbu::StorageKey::e_KEY_LENGTH_BINARY);
            currPos += FileStoreProtocol::k_HASH_LENGTH;
        }

        // 5) Append magic bits
        OffsetPtr<bdlb::BigEndianUint32> magic(block, currPos);
        *magic = QueueRecordHeader::k_MAGIC;
        currPos += sizeof(QueueRecordHeader::k_MAGIC);
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
//  Concerns:
//    Exercise the basic functionality of the component.
//
//  Testing:
//    Basic functionality.
// --------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    {
        // Default object
        QlistFileIterator qit;
        BMQTST_ASSERT_EQ(qit.isValid(), false);
        BMQTST_ASSERT_EQ(qit.isReverseMode(), false);
        BMQTST_ASSERT_EQ(qit.mappedFileDescriptor(),
                         static_cast<const mqbs::MappedFileDescriptor*>(0));
        BMQTST_ASSERT_EQ(qit.nextRecord(), -1);
    }

    {
        // Empty file -- forward iterator
        MappedFileDescriptor mfd;
        mfd.setMappingSize(1);

        FileHeader        fh;
        QlistFileIterator qit(&mfd, fh);
        BMQTST_ASSERT_EQ(qit.isValid(), false);
        BMQTST_ASSERT_EQ(qit.isReverseMode(), false);
        BMQTST_ASSERT_EQ(qit.mappedFileDescriptor(),
                         static_cast<const mqbs::MappedFileDescriptor*>(0));
        BMQTST_ASSERT_EQ(qit.nextRecord(), -1);
    }
}

static void test2_backwardIteration()
// ------------------------------------------------------------------------
// BACKWARD ITERATION
//
// Testing:
//   Backward iteration with non-zero qlist records.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BACKWARD ITERATION");

    MappedFileDescriptor mfd;
    FileHeader           fh;

    bsl::vector<bmqt::Uri> queueUris(bmqtst::TestHelperUtil::allocator());
    queueUris.push_back(bmqt::Uri("bmq://my.domain/queue1",
                                  bmqtst::TestHelperUtil::allocator()));
    queueUris.push_back(bmqt::Uri("bmq://my.domain/queue2",
                                  bmqtst::TestHelperUtil::allocator()));
    queueUris.push_back(bmqt::Uri("bmq://my.domain/queue3",
                                  bmqtst::TestHelperUtil::allocator()));
    queueUris.push_back(bmqt::Uri("bmq://my.domain/queue4",
                                  bmqtst::TestHelperUtil::allocator()));

    bsl::vector<mqbu::StorageKey> queueKeys(
        bmqtst::TestHelperUtil::allocator());
    queueKeys.push_back(
        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(), "1111111111"));
    queueKeys.push_back(
        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(), "2222222222"));
    queueKeys.push_back(
        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(), "3333333333"));
    queueKeys.push_back(
        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(), "4444444444"));

    bsl::vector<bsl::vector<bsl::string> > appIdsVec(
        bmqtst::TestHelperUtil::allocator());
    appIdsVec.resize(queueUris.size());

    for (size_t i = 0; i < appIdsVec.size(); ++i) {
        bsl::vector<bsl::string>& appIds = appIdsVec[i];

        for (size_t j = 0; j < i; ++j) {
            // The 1st queue uri (ie, 'i == 0') will have no appIds associated
            // with it.

            bmqu::MemOutStream osstr(bmqtst::TestHelperUtil::allocator());
            osstr << "AppId" << i << "_" << j << bsl::ends;
            appIds.push_back(osstr.str());
        }
    }

    bsl::vector<bsl::vector<mqbu::StorageKey> > appKeysVec(
        bmqtst::TestHelperUtil::allocator());
    appKeysVec.resize(queueUris.size());

    for (size_t i = 0; i < appKeysVec.size(); ++i) {
        bsl::vector<mqbu::StorageKey>& appKeys = appKeysVec[i];

        for (size_t j = 0; j < i; ++j) {
            bmqu::MemOutStream osstr(bmqtst::TestHelperUtil::allocator());
            osstr << j << j << j << j << j;
            osstr << j << j << j << j << j;
            osstr << bsl::ends;
            appKeys.push_back(
                mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(),
                                 osstr.str().data()));
        }
    }

    char* p = addRecords(bmqtst::TestHelperUtil::allocator(),
                         &mfd,
                         &fh,
                         queueUris,
                         appIdsVec,
                         queueKeys,
                         appKeysVec);

    QlistFileIterator it(&mfd, fh);
    BMQTST_ASSERT_EQ(it.isValid(), true);
    BMQTST_ASSERT_EQ(it.hasRecordSizeRemaining(), true);
    BMQTST_ASSERT_EQ(it.isReverseMode(), false);
    BMQTST_ASSERT_EQ(it.mappedFileDescriptor(), &mfd);

    unsigned int i      = 0;
    unsigned int offset = sizeof(FileHeader) + sizeof(QlistFileHeader);
    while (it.hasRecordSizeRemaining()) {
        BMQTST_ASSERT_EQ_D(i, it.nextRecord(), 1);

        const QueueRecordHeader* qrh = it.queueRecordHeader();
        BMQTST_ASSERT_EQ_D(i, qrh == 0, false);

        offset += qrh->queueRecordWords() * bmqp::Protocol::k_WORD_SIZE;
        i++;
    }

    BMQTST_ASSERT_EQ(it.isValid(), true);
    BMQTST_ASSERT_EQ(queueKeys.size(), i);

    it.flipDirection();

    BMQTST_ASSERT_EQ(it.isValid(), true);
    BMQTST_ASSERT_EQ(it.hasRecordSizeRemaining(), true);
    BMQTST_ASSERT_EQ(it.isReverseMode(), true);

    while (i > 0) {
        unsigned index = i - 1;

        BMQTST_ASSERT_EQ_D(i, index, it.recordIndex());

        if (index == 0) {
            BMQTST_ASSERT_EQ_D(i, false, it.hasRecordSizeRemaining());
        }
        else {
            BMQTST_ASSERT_EQ_D(i, true, it.hasRecordSizeRemaining());
        }

        const QueueRecordHeader* qrh = it.queueRecordHeader();
        BMQTST_ASSERT_EQ_D(i, false, qrh == 0);

        offset -= (qrh->queueRecordWords() * bmqp::Protocol::k_WORD_SIZE);

        BMQTST_ASSERT_EQ_D(i, offset, it.recordOffset());

        const char*  uri;
        unsigned int uriLen;
        const char*  uriHash;

        it.loadQueueUri(&uri, &uriLen);
        it.loadQueueUriHash(&uriHash);

        BMQTST_ASSERT_EQ_D(i, uriLen, queueUris[index].asString().size());

        BMQTST_ASSERT_EQ_D(
            i,
            0,
            bsl::memcmp(queueUris[index].asString().c_str(), uri, uriLen));
        BMQTST_ASSERT_EQ_D(i,
                           0,
                           bsl::memcmp(queueKeys[index].data(),
                                       uriHash,
                                       mqbu::StorageKey::e_KEY_LENGTH_BINARY));

        const bsl::vector<bsl::string>&      appIds  = appIdsVec[index];
        const bsl::vector<mqbu::StorageKey>& appKeys = appKeysVec[index];

        BSLS_ASSERT_OPT(appIds.size() == appKeys.size());

        unsigned int numAppIds = it.numAppIds();

        BMQTST_ASSERT_EQ_D(i, numAppIds, appIds.size());
        BMQTST_ASSERT_EQ_D(i, numAppIds, appKeys.size());

        typedef QlistFileIterator::AppIdLengthPair AppIdLenPair;

        bsl::vector<AppIdLenPair> appIdLenPairVector(
            bmqtst::TestHelperUtil::allocator());
        bsl::vector<const char*> appKeysVector(
            bmqtst::TestHelperUtil::allocator());

        it.loadAppIds(&appIdLenPairVector);
        it.loadAppIdHashes(&appKeysVector);

        BMQTST_ASSERT_EQ_D(i, appIdLenPairVector.size(), appKeysVector.size());

        for (size_t j = 0; j < appIdLenPairVector.size(); ++j) {
            BMQTST_ASSERT_EQ_D(i << ", " << j,
                               appIdLenPairVector[j].second,
                               appIds[j].length());

            BMQTST_ASSERT_EQ_D(i << ", " << j,
                               0,
                               bsl::memcmp(appIdLenPairVector[j].first,
                                           appIds[j].c_str(),
                                           appIds[j].length()));

            BMQTST_ASSERT_EQ_D(
                i << ", " << j,
                0,
                bsl::memcmp(appKeysVector[j],
                            appKeys[j].data(),
                            mqbu::StorageKey::e_KEY_LENGTH_BINARY));
        }

        if (index == 0) {
            BMQTST_ASSERT_EQ_D(i, 0, it.nextRecord());
        }
        else {
            BMQTST_ASSERT_EQ_D(i, it.nextRecord(), 1);
        }

        --i;
    }

    BMQTST_ASSERT_EQ(it.isValid(), false);
    BMQTST_ASSERT_EQ(0U, i);

    bmqtst::TestHelperUtil::allocator()->deallocate(p);
}

static void test3_iteratorWithNoRecords()
// ------------------------------------------------------------------------
// ITERATOR WITH NO RECORDS
//
// Testing:
//   Iterator with no records.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ITERATOR WITH NO RECORDS");

    MappedFileDescriptor mfd;
    FileHeader           fh;

    char* p = addRecords(
        bmqtst::TestHelperUtil::allocator(),
        &mfd,
        &fh,
        bsl::vector<bmqt::Uri>(bmqtst::TestHelperUtil::allocator()),
        bsl::vector<bsl::vector<bsl::string> >(
            bmqtst::TestHelperUtil::allocator()),
        bsl::vector<mqbu::StorageKey>(bmqtst::TestHelperUtil::allocator()),
        bsl::vector<bsl::vector<mqbu::StorageKey> >(
            bmqtst::TestHelperUtil::allocator()));

    QlistFileIterator it(&mfd, fh);
    BMQTST_ASSERT_EQ(it.isValid(), true);
    BMQTST_ASSERT_EQ(it.isReverseMode(), false);
    BMQTST_ASSERT_EQ(it.hasRecordSizeRemaining(), false);
    BMQTST_ASSERT_EQ(it.mappedFileDescriptor(), &mfd);

    BMQTST_ASSERT_EQ(it.nextRecord(), -3);
    BMQTST_ASSERT_EQ(it.isValid(), false);

    BMQTST_ASSERT_EQ(it.reset(&mfd, fh), 0);
    BMQTST_ASSERT_EQ(it.isValid(), true);

    it.flipDirection();

    BMQTST_ASSERT_EQ(it.isValid(), true);
    BMQTST_ASSERT_EQ(it.isReverseMode(), true);
    BMQTST_ASSERT_EQ(it.hasRecordSizeRemaining(), false);
    BMQTST_ASSERT_EQ(it.mappedFileDescriptor(), &mfd);

    // Should be at end, as the iterator is hovering over the
    // 'QlistFileHeader'.
    BMQTST_ASSERT_EQ(it.nextRecord(), 0);
    BMQTST_ASSERT_EQ(it.isValid(), false);

    bmqtst::TestHelperUtil::allocator()->deallocate(p);
}

static void test4_iteratorAppKeys()
// ------------------------------------------------------------------------
// ITERATOR APPKEYS
//
// Testing:
//   Iterator with records both having and not having appKeys.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ITERATOR APPKEYS");

    MappedFileDescriptor mfd;
    FileHeader           fh;

    bsl::vector<bmqt::Uri> queueUris(bmqtst::TestHelperUtil::allocator());
    queueUris.push_back(bmqt::Uri("bmq://my.domain/queue1",
                                  bmqtst::TestHelperUtil::allocator()));
    queueUris.push_back(bmqt::Uri("bmq://my.domain/queue2",
                                  bmqtst::TestHelperUtil::allocator()));
    queueUris.push_back(bmqt::Uri("bmq://my.domain/queue3",
                                  bmqtst::TestHelperUtil::allocator()));
    queueUris.push_back(bmqt::Uri("bmq://my.domain/queue4",
                                  bmqtst::TestHelperUtil::allocator()));

    bsl::vector<mqbu::StorageKey> queueKeys(
        bmqtst::TestHelperUtil::allocator());
    queueKeys.push_back(
        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(), "1111111111"));
    queueKeys.push_back(
        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(), "2222222222"));
    queueKeys.push_back(
        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(), "3333333333"));
    queueKeys.push_back(
        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(), "4444444444"));

    bsl::vector<bsl::vector<bsl::string> > appIdsVec(
        bmqtst::TestHelperUtil::allocator());
    appIdsVec.resize(queueUris.size());

    for (size_t i = 0; i < appIdsVec.size(); ++i) {
        bsl::vector<bsl::string>& appIds = appIdsVec[i];

        for (size_t j = 0; j < i; ++j) {
            // The 1st queue uri (ie, 'i == 0') will have no appIds
            // associated with it.

            bmqu::MemOutStream osstr(bmqtst::TestHelperUtil::allocator());
            osstr << "AppId" << i << "_" << j << bsl::ends;
            appIds.push_back(osstr.str());
        }
    }

    bsl::vector<bsl::vector<mqbu::StorageKey> > appKeysVec(
        bmqtst::TestHelperUtil::allocator());
    appKeysVec.resize(queueUris.size());

    for (size_t i = 0; i < appKeysVec.size(); ++i) {
        bsl::vector<mqbu::StorageKey>& appKeys = appKeysVec[i];

        for (size_t j = 0; j < i; ++j) {
            bmqu::MemOutStream osstr(bmqtst::TestHelperUtil::allocator());
            osstr << j << j << j << j << j;
            osstr << j << j << j << j << j;
            osstr << bsl::ends;
            appKeys.push_back(
                mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(),
                                 osstr.str().data()));
        }
    }

    char* p = addRecords(bmqtst::TestHelperUtil::allocator(),
                         &mfd,
                         &fh,
                         queueUris,
                         appIdsVec,
                         queueKeys,
                         appKeysVec);
    BMQTST_ASSERT(p != 0);

    QlistFileIterator it(&mfd, fh);
    BMQTST_ASSERT_EQ(it.isValid(), true);
    BMQTST_ASSERT_EQ(it.hasRecordSizeRemaining(), true);
    BMQTST_ASSERT_EQ(it.isReverseMode(), false);
    BMQTST_ASSERT_EQ(it.mappedFileDescriptor(), &mfd);

    unsigned int i      = 0;
    unsigned int offset = sizeof(FileHeader) + sizeof(QlistFileHeader);
    while (it.hasRecordSizeRemaining()) {
        BMQTST_ASSERT_EQ_D(i, 1, it.nextRecord());
        BMQTST_ASSERT_EQ_D(i, offset, it.recordOffset());
        BMQTST_ASSERT_EQ_D(i, i, it.recordIndex());

        const QueueRecordHeader* qrh = it.queueRecordHeader();
        BMQTST_ASSERT_EQ_D(i, false, qrh == 0);

        const char*  uri;
        unsigned int uriLen;
        const char*  uriHash;

        it.loadQueueUri(&uri, &uriLen);
        it.loadQueueUriHash(&uriHash);

        BMQTST_ASSERT_EQ_D(i, queueUris[i].asString().size(), uriLen);

        BMQTST_ASSERT_EQ_D(
            i,
            0,
            bsl::memcmp(queueUris[i].asString().c_str(), uri, uriLen));
        BMQTST_ASSERT_EQ_D(i,
                           0,
                           bsl::memcmp(uriHash,
                                       queueKeys[i].data(),
                                       mqbu::StorageKey::e_KEY_LENGTH_BINARY));

        const bsl::vector<bsl::string>&      appIds  = appIdsVec[i];
        const bsl::vector<mqbu::StorageKey>& appKeys = appKeysVec[i];

        BSLS_ASSERT_OPT(appIds.size() == appKeys.size());

        unsigned int numAppIds = it.numAppIds();

        BMQTST_ASSERT_EQ_D(i, numAppIds, appIds.size());
        BMQTST_ASSERT_EQ_D(i, numAppIds, appKeys.size());

        typedef QlistFileIterator::AppIdLengthPair AppIdLenPair;

        bsl::vector<AppIdLenPair> appIdLenPairVector(
            bmqtst::TestHelperUtil::allocator());
        bsl::vector<const char*> appKeysVector(
            bmqtst::TestHelperUtil::allocator());

        it.loadAppIds(&appIdLenPairVector);
        it.loadAppIdHashes(&appKeysVector);

        BMQTST_ASSERT_EQ_D(i, appIdLenPairVector.size(), appKeysVector.size());

        for (size_t j = 0; j < appIdLenPairVector.size(); ++j) {
            BMQTST_ASSERT_EQ_D(i << ", " << j,
                               appIdLenPairVector[j].second,
                               appIds[j].length());
            BMQTST_ASSERT_EQ_D(i << ", " << j,
                               0,
                               bsl::memcmp(appIdLenPairVector[j].first,
                                           appIds[j].c_str(),
                                           appIds[j].length()));
            BMQTST_ASSERT_EQ_D(
                i << ", " << j,
                0,
                bsl::memcmp(appKeysVector[j],
                            appKeys[j].data(),
                            mqbu::StorageKey::e_KEY_LENGTH_BINARY));
        }

        offset += qrh->queueRecordWords() * bmqp::Protocol::k_WORD_SIZE;
        ++i;
    }

    BMQTST_ASSERT_EQ(it.isValid(), true);
    BMQTST_ASSERT_EQ(it.hasRecordSizeRemaining(), false);
    BMQTST_ASSERT_EQ(it.nextRecord(), -3);
    BMQTST_ASSERT_EQ(queueUris.size(), i);

    bmqtst::TestHelperUtil::allocator()->deallocate(p);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    switch (_testCase) {
    case 0:
    case 4: test4_iteratorAppKeys(); break;
    case 3: test3_iteratorWithNoRecords(); break;
    case 2: test2_backwardIteration(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqt::UriParser::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
    // NOTE: for some reason the default allcoator verification never
    // succeeds.
}
