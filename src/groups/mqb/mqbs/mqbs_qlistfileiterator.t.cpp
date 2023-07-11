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
#include <mwcu_memoutstream.h>

// BDE
#include <bsl_cstdlib.h>
#include <bsl_cstring.h>
#include <bsl_limits.h>
#include <bsl_vector.h>
#include <bsls_assert.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

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
    mwctst::TestHelper::printTestName("BREATHING TEST");

    {
        // Default object
        QlistFileIterator qit;
        ASSERT_EQ(qit.isValid(), false);
        ASSERT_EQ(qit.isReverseMode(), false);
        ASSERT_EQ(qit.mappedFileDescriptor(),
                  static_cast<const mqbs::MappedFileDescriptor*>(0));
        ASSERT_EQ(qit.nextRecord(), -1);
    }

    {
        // Empty file -- forward iterator
        MappedFileDescriptor mfd;
        mfd.setMappingSize(1);

        FileHeader        fh;
        QlistFileIterator qit(&mfd, fh);
        ASSERT_EQ(qit.isValid(), false);
        ASSERT_EQ(qit.isReverseMode(), false);
        ASSERT_EQ(qit.mappedFileDescriptor(),
                  static_cast<const mqbs::MappedFileDescriptor*>(0));
        ASSERT_EQ(qit.nextRecord(), -1);
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
    mwctst::TestHelper::printTestName("BACKWARD ITERATION");

    MappedFileDescriptor mfd;
    FileHeader           fh;

    bsl::vector<bmqt::Uri> queueUris(s_allocator_p);
    queueUris.push_back(bmqt::Uri("bmq://my.domain/queue1", s_allocator_p));
    queueUris.push_back(bmqt::Uri("bmq://my.domain/queue2", s_allocator_p));
    queueUris.push_back(bmqt::Uri("bmq://my.domain/queue3", s_allocator_p));
    queueUris.push_back(bmqt::Uri("bmq://my.domain/queue4", s_allocator_p));

    bsl::vector<mqbu::StorageKey> queueKeys(s_allocator_p);
    queueKeys.push_back(
        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(), "1111111111"));
    queueKeys.push_back(
        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(), "2222222222"));
    queueKeys.push_back(
        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(), "3333333333"));
    queueKeys.push_back(
        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(), "4444444444"));

    bsl::vector<bsl::vector<bsl::string> > appIdsVec(s_allocator_p);
    appIdsVec.resize(queueUris.size());

    for (size_t i = 0; i < appIdsVec.size(); ++i) {
        bsl::vector<bsl::string>& appIds = appIdsVec[i];

        for (size_t j = 0; j < i; ++j) {
            // The 1st queue uri (ie, 'i == 0') will have no appIds associated
            // with it.

            mwcu::MemOutStream osstr(s_allocator_p);
            osstr << "AppId" << i << "_" << j << bsl::ends;
            appIds.push_back(osstr.str());
        }
    }

    bsl::vector<bsl::vector<mqbu::StorageKey> > appKeysVec(s_allocator_p);
    appKeysVec.resize(queueUris.size());

    for (size_t i = 0; i < appKeysVec.size(); ++i) {
        bsl::vector<mqbu::StorageKey>& appKeys = appKeysVec[i];

        for (size_t j = 0; j < i; ++j) {
            mwcu::MemOutStream osstr(s_allocator_p);
            osstr << j << j << j << j << j;
            osstr << j << j << j << j << j;
            osstr << bsl::ends;
            appKeys.push_back(
                mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(),
                                 osstr.str().data()));
        }
    }

    char* p = addRecords(s_allocator_p,
                         &mfd,
                         &fh,
                         queueUris,
                         appIdsVec,
                         queueKeys,
                         appKeysVec);

    QlistFileIterator it(&mfd, fh);
    ASSERT_EQ(it.isValid(), true);
    ASSERT_EQ(it.hasRecordSizeRemaining(), true);
    ASSERT_EQ(it.isReverseMode(), false);
    ASSERT_EQ(it.mappedFileDescriptor(), &mfd);

    unsigned int i      = 0;
    unsigned int offset = sizeof(FileHeader) + sizeof(QlistFileHeader);
    while (it.hasRecordSizeRemaining()) {
        ASSERT_EQ_D(i, it.nextRecord(), 1);

        const QueueRecordHeader* qrh = it.queueRecordHeader();
        ASSERT_EQ_D(i, qrh == 0, false);

        offset += qrh->queueRecordWords() * bmqp::Protocol::k_WORD_SIZE;
        i++;
    }

    ASSERT_EQ(it.isValid(), true);
    ASSERT_EQ(queueKeys.size(), i);

    it.flipDirection();

    ASSERT_EQ(it.isValid(), true);
    ASSERT_EQ(it.hasRecordSizeRemaining(), true);
    ASSERT_EQ(it.isReverseMode(), true);

    while (i > 0) {
        unsigned index = i - 1;

        ASSERT_EQ_D(i, index, it.recordIndex());

        if (index == 0) {
            ASSERT_EQ_D(i, false, it.hasRecordSizeRemaining());
        }
        else {
            ASSERT_EQ_D(i, true, it.hasRecordSizeRemaining());
        }

        const QueueRecordHeader* qrh = it.queueRecordHeader();
        ASSERT_EQ_D(i, false, qrh == 0);

        offset -= (qrh->queueRecordWords() * bmqp::Protocol::k_WORD_SIZE);

        ASSERT_EQ_D(i, offset, it.recordOffset());

        const char*  uri;
        unsigned int uriLen;
        const char*  uriHash;

        it.loadQueueUri(&uri, &uriLen);
        it.loadQueueUriHash(&uriHash);

        ASSERT_EQ_D(i, uriLen, queueUris[index].asString().size());

        ASSERT_EQ_D(
            i,
            0,
            bsl::memcmp(queueUris[index].asString().c_str(), uri, uriLen));
        ASSERT_EQ_D(i,
                    0,
                    bsl::memcmp(queueKeys[index].data(),
                                uriHash,
                                mqbu::StorageKey::e_KEY_LENGTH_BINARY));

        const bsl::vector<bsl::string>&      appIds  = appIdsVec[index];
        const bsl::vector<mqbu::StorageKey>& appKeys = appKeysVec[index];

        BSLS_ASSERT_OPT(appIds.size() == appKeys.size());

        unsigned int numAppIds = it.numAppIds();

        ASSERT_EQ_D(i, numAppIds, appIds.size());
        ASSERT_EQ_D(i, numAppIds, appKeys.size());

        typedef QlistFileIterator::AppIdLengthPair AppIdLenPair;

        bsl::vector<AppIdLenPair> appIdLenPairVector(s_allocator_p);
        bsl::vector<const char*>  appKeysVector(s_allocator_p);

        it.loadAppIds(&appIdLenPairVector);
        it.loadAppIdHashes(&appKeysVector);

        ASSERT_EQ_D(i, appIdLenPairVector.size(), appKeysVector.size());

        for (size_t j = 0; j < appIdLenPairVector.size(); ++j) {
            ASSERT_EQ_D(i << ", " << j,
                        appIdLenPairVector[j].second,
                        appIds[j].length());

            ASSERT_EQ_D(i << ", " << j,
                        0,
                        bsl::memcmp(appIdLenPairVector[j].first,
                                    appIds[j].c_str(),
                                    appIds[j].length()));

            ASSERT_EQ_D(i << ", " << j,
                        0,
                        bsl::memcmp(appKeysVector[j],
                                    appKeys[j].data(),
                                    mqbu::StorageKey::e_KEY_LENGTH_BINARY));
        }

        if (index == 0) {
            ASSERT_EQ_D(i, 0, it.nextRecord());
        }
        else {
            ASSERT_EQ_D(i, it.nextRecord(), 1);
        }

        --i;
    }

    ASSERT_EQ(it.isValid(), false);
    ASSERT_EQ(0U, i);

    s_allocator_p->deallocate(p);
}

static void test3_iteratorWithNoRecords()
// ------------------------------------------------------------------------
// ITERATOR WITH NO RECORDS
//
// Testing:
//   Iterator with no records.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ITERATOR WITH NO RECORDS");

    MappedFileDescriptor mfd;
    FileHeader           fh;

    char* p = addRecords(
        s_allocator_p,
        &mfd,
        &fh,
        bsl::vector<bmqt::Uri>(s_allocator_p),
        bsl::vector<bsl::vector<bsl::string> >(s_allocator_p),
        bsl::vector<mqbu::StorageKey>(s_allocator_p),
        bsl::vector<bsl::vector<mqbu::StorageKey> >(s_allocator_p));

    QlistFileIterator it(&mfd, fh);
    ASSERT_EQ(it.isValid(), true);
    ASSERT_EQ(it.isReverseMode(), false);
    ASSERT_EQ(it.hasRecordSizeRemaining(), false);
    ASSERT_EQ(it.mappedFileDescriptor(), &mfd);

    ASSERT_EQ(it.nextRecord(), -3);
    ASSERT_EQ(it.isValid(), false);

    ASSERT_EQ(it.reset(&mfd, fh), 0);
    ASSERT_EQ(it.isValid(), true);

    it.flipDirection();

    ASSERT_EQ(it.isValid(), true);
    ASSERT_EQ(it.isReverseMode(), true);
    ASSERT_EQ(it.hasRecordSizeRemaining(), false);
    ASSERT_EQ(it.mappedFileDescriptor(), &mfd);

    // Should be at end, as the iterator is hovering over the
    // 'QlistFileHeader'.
    ASSERT_EQ(it.nextRecord(), 0);
    ASSERT_EQ(it.isValid(), false);

    s_allocator_p->deallocate(p);
}

static void test4_iteratorAppKeys()
// ------------------------------------------------------------------------
// ITERATOR APPKEYS
//
// Testing:
//   Iterator with records both having and not having appKeys.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ITERATOR APPKEYS");

    MappedFileDescriptor mfd;
    FileHeader           fh;

    bsl::vector<bmqt::Uri> queueUris(s_allocator_p);
    queueUris.push_back(bmqt::Uri("bmq://my.domain/queue1", s_allocator_p));
    queueUris.push_back(bmqt::Uri("bmq://my.domain/queue2", s_allocator_p));
    queueUris.push_back(bmqt::Uri("bmq://my.domain/queue3", s_allocator_p));
    queueUris.push_back(bmqt::Uri("bmq://my.domain/queue4", s_allocator_p));

    bsl::vector<mqbu::StorageKey> queueKeys(s_allocator_p);
    queueKeys.push_back(
        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(), "1111111111"));
    queueKeys.push_back(
        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(), "2222222222"));
    queueKeys.push_back(
        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(), "3333333333"));
    queueKeys.push_back(
        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(), "4444444444"));

    bsl::vector<bsl::vector<bsl::string> > appIdsVec(s_allocator_p);
    appIdsVec.resize(queueUris.size());

    for (size_t i = 0; i < appIdsVec.size(); ++i) {
        bsl::vector<bsl::string>& appIds = appIdsVec[i];

        for (size_t j = 0; j < i; ++j) {
            // The 1st queue uri (ie, 'i == 0') will have no appIds
            // associated with it.

            mwcu::MemOutStream osstr(s_allocator_p);
            osstr << "AppId" << i << "_" << j << bsl::ends;
            appIds.push_back(osstr.str());
        }
    }

    bsl::vector<bsl::vector<mqbu::StorageKey> > appKeysVec(s_allocator_p);
    appKeysVec.resize(queueUris.size());

    for (size_t i = 0; i < appKeysVec.size(); ++i) {
        bsl::vector<mqbu::StorageKey>& appKeys = appKeysVec[i];

        for (size_t j = 0; j < i; ++j) {
            mwcu::MemOutStream osstr(s_allocator_p);
            osstr << j << j << j << j << j;
            osstr << j << j << j << j << j;
            osstr << bsl::ends;
            appKeys.push_back(
                mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(),
                                 osstr.str().data()));
        }
    }

    char* p = addRecords(s_allocator_p,
                         &mfd,
                         &fh,
                         queueUris,
                         appIdsVec,
                         queueKeys,
                         appKeysVec);
    ASSERT(p != 0);

    QlistFileIterator it(&mfd, fh);
    ASSERT_EQ(it.isValid(), true);
    ASSERT_EQ(it.hasRecordSizeRemaining(), true);
    ASSERT_EQ(it.isReverseMode(), false);
    ASSERT_EQ(it.mappedFileDescriptor(), &mfd);

    unsigned int i      = 0;
    unsigned int offset = sizeof(FileHeader) + sizeof(QlistFileHeader);
    while (it.hasRecordSizeRemaining()) {
        ASSERT_EQ_D(i, 1, it.nextRecord());
        ASSERT_EQ_D(i, offset, it.recordOffset());
        ASSERT_EQ_D(i, i, it.recordIndex());

        const QueueRecordHeader* qrh = it.queueRecordHeader();
        ASSERT_EQ_D(i, false, qrh == 0);

        const char*  uri;
        unsigned int uriLen;
        const char*  uriHash;

        it.loadQueueUri(&uri, &uriLen);
        it.loadQueueUriHash(&uriHash);

        ASSERT_EQ_D(i, queueUris[i].asString().size(), uriLen);

        ASSERT_EQ_D(i,
                    0,
                    bsl::memcmp(queueUris[i].asString().c_str(), uri, uriLen));
        ASSERT_EQ_D(i,
                    0,
                    bsl::memcmp(uriHash,
                                queueKeys[i].data(),
                                mqbu::StorageKey::e_KEY_LENGTH_BINARY));

        const bsl::vector<bsl::string>&      appIds  = appIdsVec[i];
        const bsl::vector<mqbu::StorageKey>& appKeys = appKeysVec[i];

        BSLS_ASSERT_OPT(appIds.size() == appKeys.size());

        unsigned int numAppIds = it.numAppIds();

        ASSERT_EQ_D(i, numAppIds, appIds.size());
        ASSERT_EQ_D(i, numAppIds, appKeys.size());

        typedef QlistFileIterator::AppIdLengthPair AppIdLenPair;

        bsl::vector<AppIdLenPair> appIdLenPairVector(s_allocator_p);
        bsl::vector<const char*>  appKeysVector(s_allocator_p);

        it.loadAppIds(&appIdLenPairVector);
        it.loadAppIdHashes(&appKeysVector);

        ASSERT_EQ_D(i, appIdLenPairVector.size(), appKeysVector.size());

        for (size_t j = 0; j < appIdLenPairVector.size(); ++j) {
            ASSERT_EQ_D(i << ", " << j,
                        appIdLenPairVector[j].second,
                        appIds[j].length());
            ASSERT_EQ_D(i << ", " << j,
                        0,
                        bsl::memcmp(appIdLenPairVector[j].first,
                                    appIds[j].c_str(),
                                    appIds[j].length()));
            ASSERT_EQ_D(i << ", " << j,
                        0,
                        bsl::memcmp(appKeysVector[j],
                                    appKeys[j].data(),
                                    mqbu::StorageKey::e_KEY_LENGTH_BINARY));
        }

        offset += qrh->queueRecordWords() * bmqp::Protocol::k_WORD_SIZE;
        ++i;
    }

    ASSERT_EQ(it.isValid(), true);
    ASSERT_EQ(it.hasRecordSizeRemaining(), false);
    ASSERT_EQ(it.nextRecord(), -3);
    ASSERT_EQ(queueUris.size(), i);

    s_allocator_p->deallocate(p);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    bmqt::UriParser::initialize(s_allocator_p);

    switch (_testCase) {
    case 0:
    case 4: test4_iteratorAppKeys(); break;
    case 3: test3_iteratorWithNoRecords(); break;
    case 2: test2_backwardIteration(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    bmqt::UriParser::shutdown();

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_GBL_ALLOC);
    // NOTE: for some reason the default allcoator verification never
    // succeeds.
}
