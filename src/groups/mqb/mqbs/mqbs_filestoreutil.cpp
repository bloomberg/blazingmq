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

// mqbs_filestoreutil.cpp                                             -*-C++-*-
#include <mqbs_filestoreutil.h>

#include <mqbscm_version.h>
// MQB
#include <mqbs_datafileiterator.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_filestoreprotocolutil.h>
#include <mqbs_filesystemutil.h>
#include <mqbs_journalfileiterator.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbs_offsetptr.h>
#include <mqbs_qlistfileiterator.h>
#include <mqbu_storagekey.h>

// MWC
#include <mwctsk_alarmlog.h>
#include <mwcu_memoutstream.h>
#include <mwcu_stringutil.h>

// BDE
#include <bdlb_scopeexit.h>
#include <bdlb_string.h>
#include <bdlf_bind.h>
#include <bdls_filesystemutil.h>
#include <bdlt_currenttime.h>
#include <bdlt_datetimeutil.h>
#include <bdlt_epochutil.h>
#include <bsl_cerrno.h>
#include <bsl_cstddef.h>
#include <bsl_cstring.h>
#include <bsl_ctime.h>
#include <bsl_iostream.h>
#include <bslim_printer.h>
#include <bsls_assert.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

// SYS
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace BloombergLP {
namespace mqbs {

namespace {

/// Append the specified `value` to `result` in YYYYMMDD_HHMMSS format.
void appendFormattedDatetime(bsl::string* result, const bdlt::Datetime& value)
{
    enum {
        e_BUFFER_SIZE = 16  // includes null-character
    };

    char           buffer[e_BUFFER_SIZE];
    struct bsl::tm timeStruct = bdlt::DatetimeUtil::convertToTm(value);
    bsl::strftime(buffer, e_BUFFER_SIZE, "%G%m%d_%H%M%S", &timeStruct);
    result->append(buffer);
}

/// Populate the specified `filename` with the well known BlazingMQ file
/// name format using the specified `basePath`, `partitionId`, `datetime`
/// and `extension`.  Note that the format is:
///
///     `/basePath/bmq_G.YYYYMMDD_HHMMSS.extension`
///
/// where `G` is partitionIp, `YYYYMMDD_HHMMSS` is derived from `datetime`.
void createFileName(bsl::string*             filename,
                    const bslstl::StringRef& basePath,
                    int                      partitionId,
                    const bdlt::Datetime&    datetime,
                    const char*              extension)
{
    filename->clear();
    filename->append(basePath);
    if (*(filename->rbegin()) != '/') {
        filename->append(1, '/');
    }

    filename->append(FileStoreProtocol::k_COMMON_FILE_PREFIX);
    mwcu::MemOutStream osstr;
    osstr << partitionId;
    filename->append(osstr.str().data(), osstr.str().length());
    filename->append(".");
    appendFormattedDatetime(filename, datetime);
    filename->append(extension);
}

int openFileSet(bsl::ostream&         errorDescription,
                const FileStoreSet&   fileSet,
                bool                  readOnly,
                bool                  prefaultPages,
                MappedFileDescriptor* journalFd = 0,
                MappedFileDescriptor* dataFd    = 0,
                MappedFileDescriptor* qlistFd   = 0)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(journalFd || dataFd || qlistFd);

    enum {
        rc_SUCCESS              = 0,
        rc_UNKNOWN              = -1,
        rc_INVALID_FILE_SIZE    = -2,
        rc_JOURNAL_OPEN_FAILURE = -3,
        rc_DATA_OPEN_FAILURE    = -4,
        rc_QLIST_OPEN_FAILURE   = -5
    };

    if ((journalFd && fileSet.journalFileSize() == 0) ||
        (dataFd && fileSet.dataFileSize() == 0) ||
        (qlistFd && fileSet.qlistFileSize() == 0)) {
        errorDescription << "At least one of JOURNAL/DATA/QLIST specified "
                         << "file size is zero. JOURNAL/DATA/QLIST file "
                         << "sizes: " << fileSet.journalFileSize() << "/"
                         << fileSet.dataFileSize() << "/"
                         << fileSet.qlistFileSize() << " respectively.";
        return rc_INVALID_FILE_SIZE;  // RETURN
    }

    int rc = rc_UNKNOWN;
    if (journalFd) {
        rc = FileSystemUtil::open(journalFd,
                                  fileSet.journalFile().c_str(),
                                  fileSet.journalFileSize(),
                                  readOnly,
                                  errorDescription,
                                  prefaultPages);
        if (0 != rc) {
            return 10 * rc + rc_JOURNAL_OPEN_FAILURE;  // RETURN
        }
    }

    if (dataFd) {
        rc = FileSystemUtil::open(dataFd,
                                  fileSet.dataFile().c_str(),
                                  fileSet.dataFileSize(),
                                  readOnly,
                                  errorDescription,
                                  prefaultPages);

        if (0 != rc) {
            if (journalFd) {
                FileSystemUtil::close(journalFd);  // ignore rc
            }
            return 10 * rc + rc_DATA_OPEN_FAILURE;  // RETURN
        }
    }

    if (qlistFd) {
        rc = FileSystemUtil::open(qlistFd,
                                  fileSet.qlistFile().c_str(),
                                  fileSet.qlistFileSize(),
                                  readOnly,
                                  errorDescription,
                                  prefaultPages);

        if (0 != rc) {
            if (journalFd) {
                FileSystemUtil::close(journalFd);  // ignore rc
            }
            if (dataFd) {
                FileSystemUtil::close(dataFd);  // ignore rc
            }
            return 10 * rc + rc_QLIST_OPEN_FAILURE;  // RETURN
        }
    }

    // Indicate to the OS not to dump these mappings in the core file.
    if (journalFd) {
        FileSystemUtil::disableDump(journalFd->mapping(),
                                    journalFd->mappingSize());
    }

    if (dataFd) {
        FileSystemUtil::disableDump(dataFd->mapping(), dataFd->mappingSize());
    }

    if (qlistFd) {
        FileSystemUtil::disableDump(qlistFd->mapping(),
                                    qlistFd->mappingSize());
    }

    return rc_SUCCESS;
}

void closeAndDeleteFileSet(const FileStoreSet&   fileSet,
                           bool                  deleteOnFailure,
                           MappedFileDescriptor* journalFd = 0,
                           MappedFileDescriptor* dataFd    = 0,
                           MappedFileDescriptor* qlistFd   = 0)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(journalFd || dataFd || qlistFd);

    if (journalFd) {
        FileSystemUtil::close(journalFd);
    }
    if (dataFd) {
        FileSystemUtil::close(dataFd);
    }
    if (qlistFd) {
        FileSystemUtil::close(qlistFd);
    }

    if (deleteOnFailure) {
        // Even if only one file encountered failure, we will remove all files.

        bdls::FilesystemUtil::remove(fileSet.journalFile());
        bdls::FilesystemUtil::remove(fileSet.dataFile());
        bdls::FilesystemUtil::remove(fileSet.qlistFile());
    }
}

}  // close unnamed namespace

// --------------------
// struct FileStoreUtil
// --------------------

// PRIVATE CLASS METHODS
int FileStoreUtil::findFileStoreSetsFromPaths(
    FileSetMap*                     fileSetMap,
    const bsl::vector<bsl::string>& files,
    bool                            loadSize)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fileSetMap);

    enum {
        rc_SUCCESS                = 0,
        rc_FILE_EXTENSION_UNKNOWN = -1  // Failed to recognize file
        // extension
        ,
        rc_TIMESTAMP_EXTRACTION_FAILURE = -2  // Failed to extract timestamp
                                              // from file name
    };

    int localRC  = rc_SUCCESS;
    int returnRC = rc_SUCCESS;

    if (loadSize) {
        // For every file in the output, also populate the corresponding file
        // size
        for (bsl::vector<bsl::string>::size_type i = 0; i < files.size();
             ++i) {
            bsl::string timestamp;
            localRC = extractTimestamp(&timestamp, files[i]);
            if (localRC != 0) {
                returnRC = rc_TIMESTAMP_EXTRACTION_FAILURE;
                BALL_LOG_ERROR
                    << "Failed to extract timestamp from file name:"
                    << " [" << files[i] << "], rc: "
                    << (10 * localRC + rc_TIMESTAMP_EXTRACTION_FAILURE)
                    << ". File will be skipped from the result.";
                continue;  // CONTINUE
            }

            if (hasDataFileExtension(files[i])) {
                (*fileSetMap)[timestamp].setDataFile(files[i]).setDataFileSize(
                    bdls::FilesystemUtil::getFileSize(files[i].c_str()));
            }
            else if (hasQlistFileExtension(files[i])) {
                (*fileSetMap)[timestamp]
                    .setQlistFile(files[i])
                    .setQlistFileSize(
                        bdls::FilesystemUtil::getFileSize(files[i].c_str()));
            }
            else if (hasJournalFileExtension(files[i])) {
                (*fileSetMap)[timestamp]
                    .setJournalFile(files[i])
                    .setJournalFileSize(
                        bdls::FilesystemUtil::getFileSize(files[i].c_str()));
            }
            else {
                localRC  = rc_FILE_EXTENSION_UNKNOWN;
                returnRC = 10 * returnRC + rc_FILE_EXTENSION_UNKNOWN;
                BALL_LOG_ERROR << "Invalid file extension for file ["
                               << files[i] << "]. File will be skipped from "
                               << "result.";
            }
        }
    }
    else {
        // For every file in the output, populate only the file name
        for (bsl::vector<bsl::string>::size_type i = 0; i < files.size();
             ++i) {
            bsl::string timestamp;
            localRC = extractTimestamp(&timestamp, files[i]);
            if (localRC != 0) {
                returnRC = rc_TIMESTAMP_EXTRACTION_FAILURE;
                BALL_LOG_ERROR
                    << "Failed to extract timestamp from file name:"
                    << " [" << files[i] << "], rc: "
                    << (10 * localRC + rc_TIMESTAMP_EXTRACTION_FAILURE)
                    << ". File will be skipped from the result.";
                continue;  // CONTINUE
            }

            if (hasDataFileExtension(files[i])) {
                (*fileSetMap)[timestamp].setDataFile(files[i]);
            }
            else if (hasQlistFileExtension(files[i])) {
                (*fileSetMap)[timestamp].setQlistFile(files[i]);
            }
            else if (hasJournalFileExtension(files[i])) {
                (*fileSetMap)[timestamp].setJournalFile(files[i]);
            }
            else {
                localRC  = rc_FILE_EXTENSION_UNKNOWN;
                returnRC = 10 * returnRC + localRC;
                BALL_LOG_ERROR << "Invalid file extension for file ["
                               << files[i] << "]. File will be skipped from "
                               << "result.";
            }
        }
    }

    return returnRC;
}

// CLASS METHODS
int FileStoreUtil::closePartitionSet(MappedFileDescriptor* dataFileMfd,
                                     MappedFileDescriptor* journalFileMfd,
                                     MappedFileDescriptor* qlistFileMfd)
{
    // PRECONDTIONS
    BSLS_ASSERT_SAFE(dataFileMfd);
    BSLS_ASSERT_SAFE(journalFileMfd);
    BSLS_ASSERT_SAFE(qlistFileMfd);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                  = 0,
        rc_ERROR_CLOSE_DATA_FILE    = -1,
        rc_ERROR_CLOSE_JOURNAL_FILE = -2,
        rc_ERROR_CLOSE_QLIST_FILE   = -3
    };

    int rc = rc_SUCCESS;

    if (FileSystemUtil::close(dataFileMfd) != 0) {
        rc = 10 * rc + rc_ERROR_CLOSE_DATA_FILE;
    }
    if (FileSystemUtil::close(journalFileMfd) != 0) {
        rc = 10 * rc + rc_ERROR_CLOSE_JOURNAL_FILE;
    }
    if (FileSystemUtil::close(qlistFileMfd) != 0) {
        rc = 10 * rc + rc_ERROR_CLOSE_QLIST_FILE;
    }

    return rc;
}

void FileStoreUtil::createDataFileName(bsl::string*             filename,
                                       const bslstl::StringRef& basePath,
                                       int                      partitionId,
                                       const bdlt::Datetime&    datetime)
{
    createFileName(filename,
                   basePath,
                   partitionId,
                   datetime,
                   FileStoreProtocol::k_DATA_FILE_EXTENSION);
}

void FileStoreUtil::createJournalFileName(bsl::string*             filename,
                                          const bslstl::StringRef& basePath,
                                          int                      partitionId,
                                          const bdlt::Datetime&    datetime)
{
    createFileName(filename,
                   basePath,
                   partitionId,
                   datetime,
                   FileStoreProtocol::k_JOURNAL_FILE_EXTENSION);
}

void FileStoreUtil::createQlistFileName(bsl::string*             filename,
                                        const bslstl::StringRef& basePath,
                                        int                      partitionId,
                                        const bdlt::Datetime&    datetime)
{
    createFileName(filename,
                   basePath,
                   partitionId,
                   datetime,
                   FileStoreProtocol::k_QLIST_FILE_EXTENSION);
}

bool FileStoreUtil::hasDataFileExtension(const bsl::string& filename)
{
    return mwcu::StringUtil::endsWith(
        filename,
        FileStoreProtocol::k_DATA_FILE_EXTENSION);
}

bool FileStoreUtil::hasJournalFileExtension(const bsl::string& filename)
{
    return mwcu::StringUtil::endsWith(
        filename,
        FileStoreProtocol::k_JOURNAL_FILE_EXTENSION);
}

bool FileStoreUtil::hasQlistFileExtension(const bsl::string& filename)
{
    return mwcu::StringUtil::endsWith(
        filename,
        FileStoreProtocol::k_QLIST_FILE_EXTENSION);
}

int FileStoreUtil::createFilePattern(bsl::string*             pattern,
                                     const bslstl::StringRef& basePath,
                                     int                      partitionId)
{
    // Pattern to create: '/basePath/bmq_x.*_*.bmq_*' where 'x' is partitionId

    enum {
        rc_SUCCESS              = 0,
        rc_INVALID_PARTITION_ID = -1,
        rc_INVALID_BASE_PATH    = -2
    };

    if (0 > partitionId) {
        return rc_INVALID_PARTITION_ID;  // RETURN
    }

    if (basePath.isEmpty()) {
        return rc_INVALID_BASE_PATH;  // RETURN
    }

    bsl::string& p = *pattern;  // for convenience
    p.clear();
    p.append(basePath);
    if ('/' != p[p.length() - 1]) {
        p.append(1, '/');
    }

    mwcu::MemOutStream osstr;
    osstr << partitionId;

    p.append(FileStoreProtocol::k_COMMON_FILE_PREFIX);
    p.append(osstr.str().data(), osstr.str().length());
    p.append(".*_*");
    p.append(FileStoreProtocol::k_COMMON_FILE_EXTENSION_PREFIX);
    p.append(1, '*');

    return rc_SUCCESS;
}

int FileStoreUtil::createFilePattern(bsl::string*             pattern,
                                     const bslstl::StringRef& basePath)
{
    // Pattern to create: `/basePath/bmq_*.*_*.bmq_*`

    enum { rc_SUCCESS = 0, rc_INVALID_BASE_PATH = -1 };

    if (basePath.isEmpty()) {
        return rc_INVALID_BASE_PATH;  // RETURN
    }

    bsl::string& p = *pattern;  // for convenience

    p.clear();
    p.append(basePath);
    if ('/' != p[p.length() - 1]) {
        p.append(1, '/');
    }

    p.append(FileStoreProtocol::k_COMMON_FILE_PREFIX);
    p.append("*.*_*");
    p.append(FileStoreProtocol::k_COMMON_FILE_EXTENSION_PREFIX);
    p.append(1, '*');

    return rc_SUCCESS;
}

int FileStoreUtil::create(bsl::ostream&            errorDescription,
                          FileSetSp*               fileSetSp,
                          FileStore*               fileStore,
                          int                      partitionId,
                          const DataStoreConfig&   dataStoreConfig,
                          const bslstl::StringRef& partitionDesc,
                          bool                     needQList,
                          bslma::Allocator*        allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fileSetSp);
    BSLS_ASSERT_SAFE(partitionId >= 0);

    FileSetSp result;
    result.createInplace(allocator, fileStore, allocator);

    bdlt::Datetime now       = bdlt::CurrentTime::utc();
    int            increment = 0;

    do {
        // Increment 'now' by 1 second everytime there is a clash of at least
        // 1 file name.

        now.addSeconds(increment++);
        createDataFileName(&result->d_dataFileName,
                           dataStoreConfig.location(),
                           partitionId,
                           now);

        createJournalFileName(&result->d_journalFileName,
                              dataStoreConfig.location(),
                              partitionId,
                              now);

        if (needQList) {
            createQlistFileName(&result->d_qlistFileName,
                                dataStoreConfig.location(),
                                partitionId,
                                now);
        }
    } while (
        bdls::FilesystemUtil::exists(result->d_dataFileName) ||
        bdls::FilesystemUtil::exists(result->d_journalFileName) ||
        (needQList && bdls::FilesystemUtil::exists(result->d_qlistFileName)));

    BSLS_ASSERT_SAFE(!bdls::FilesystemUtil::exists(result->d_dataFileName));
    BSLS_ASSERT_SAFE(!bdls::FilesystemUtil::exists(result->d_journalFileName));
    if (needQList) {
        BSLS_ASSERT_SAFE(
            !bdls::FilesystemUtil::exists(result->d_qlistFileName));
    }

    // Open, mmap and grow files (delete created files on failure)
    FileStoreSet fs;
    fs.setDataFile(result->d_dataFileName)
        .setDataFileSize(dataStoreConfig.maxDataFileSize())
        .setJournalFile(result->d_journalFileName)
        .setJournalFileSize(dataStoreConfig.maxJournalFileSize());
    if (needQList) {
        fs.setQlistFile(result->d_qlistFileName)
            .setQlistFileSize(dataStoreConfig.maxQlistFileSize());
    }

    mwcu::MemOutStream errorDesc;
    int                rc = openFileSetWriteMode(errorDesc,
                                  fs,
                                  dataStoreConfig.hasPreallocate(),
                                  true,  // delete on failure
                                  &result->d_journalFile,
                                  &result->d_dataFile,
                                  needQList ? &result->d_qlistFile : 0,
                                  dataStoreConfig.hasPrefaultPages());

    if (0 != rc) {
        errorDescription << partitionDesc << " Failed to open file set in "
                         << "read mode. Reason: " << errorDesc.str();
        return rc;  // RETURN
    }

    // Local refs for convenience
    MappedFileDescriptor& dataFile    = result->d_dataFile;
    bsls::Types::Uint64&  dataFilePos = result->d_dataFilePosition;

    MappedFileDescriptor& journal    = result->d_journalFile;
    bsls::Types::Uint64&  journalPos = result->d_journalFilePosition;

    MappedFileDescriptor& qlistFile    = result->d_qlistFile;
    bsls::Types::Uint64&  qlistFilePos = result->d_qlistFilePosition;

    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM << partitionDesc << "Created data file ["
                               << result->d_dataFileName << "], journal file ["
                               << result->d_journalFileName << "]";
        if (needQList) {
            BALL_LOG_OUTPUT_STREAM << ", qlist file ["
                                   << result->d_qlistFileName << "]";
        }
    }

    // Add BlazingMQ header and file-specific headers in the active files

    // Data file -- append BlazingMQ header
    OffsetPtr<FileHeader> fh(dataFile.block(), dataFilePos);

    new (fh.get()) FileHeader();
    fh->setFileType(FileType::e_DATA).setPartitionId(partitionId);
    dataFilePos = sizeof(FileHeader);

    // Data file -- append DataFileHeader

    result->d_dataFileKey = mqbu::StorageKey::k_NULL_KEY;
    // explicitly initialize to null since this field is unused for now.

    OffsetPtr<DataFileHeader> dfh(dataFile.block(), dataFilePos);
    new (dfh.get()) DataFileHeader();
    dfh->setFileKey(result->d_dataFileKey);
    dataFilePos += sizeof(DataFileHeader);

    result->d_outstandingBytesData += dataFilePos;

    // Journal file -- append BlazingMQ header
    fh.reset(journal.block(), journalPos);
    new (fh.get()) FileHeader();
    fh->setFileType(FileType::e_JOURNAL).setPartitionId(partitionId);
    journalPos += sizeof(FileHeader);

    // Journal file -- append JournalFileHeader
    OffsetPtr<JournalFileHeader> jfh(journal.block(), journalPos);
    new (jfh.get()) JournalFileHeader();  // Default values are fine
    journalPos += sizeof(JournalFileHeader);

    result->d_outstandingBytesJournal += journalPos;

    if (needQList) {
        // Qlist file -- append BlazingMQ header
        fh.reset(qlistFile.block(), qlistFilePos);

        new (fh.get()) FileHeader();
        fh->setFileType(FileType::e_QLIST).setPartitionId(partitionId);
        qlistFilePos += sizeof(FileHeader);

        // Qlist file -- append QlistFileHeader
        OffsetPtr<QlistFileHeader> qfh(qlistFile.block(), qlistFilePos);
        new (qfh.get()) QlistFileHeader();
        qlistFilePos += sizeof(QlistFileHeader);

        result->d_outstandingBytesQlist += qlistFilePos;
    }

    *fileSetSp = result;

    return 0;
}

int FileStoreUtil::extractTimestamp(bsl::string*       timestamp,
                                    const bsl::string& filename)
{
    // /path/to/files/bmq_x.YYYYMMDD_HHMMSS.bmq_[data|journal|qlist]

    enum {
        rc_SUCCESS                    = 0,
        rc_EXTENSION_NOT_FOUND        = -1,
        rc_INVALID_EXTENSION_POSITION = -2,
        rc_BEGINNING_POS_NOT_FOUND    = -3,
        rc_INVALID_TIMESTAMP_LENGTH   = -4,
        rc_USCORE_MISMATCH            = -5
    };

    const unsigned int k_TIMESTAMP_LENGTH = 15;
    const int          k_USCORE_POSITION  = 8;

    bsl::string::size_type endPos = filename.rfind(
        FileStoreProtocol::k_COMMON_FILE_EXTENSION_PREFIX);
    if (bsl::string::npos == endPos) {
        return rc_EXTENSION_NOT_FOUND;  // RETURN
    }

    if (1 >= endPos) {
        return rc_INVALID_TIMESTAMP_LENGTH;  // RETURN
    }

    bsl::string::size_type beginPos = filename.rfind('.', endPos - 1);
    if (bsl::string::npos == beginPos) {
        return rc_BEGINNING_POS_NOT_FOUND;  // RETURN
    }

    if (k_TIMESTAMP_LENGTH != (endPos - beginPos - 1)) {
        return rc_INVALID_TIMESTAMP_LENGTH;  // RETURN
    }

    timestamp->assign(filename, beginPos + 1, k_TIMESTAMP_LENGTH);
    if ('_' != (*timestamp)[k_USCORE_POSITION]) {
        return rc_USCORE_MISMATCH;  // RETURN
    }

    return 0;
}

void FileStoreUtil::loadCurrentFiles(FileStoreSet*  fileStoreSet,
                                     const FileSet& fileSet,
                                     bool           needQList)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fileStoreSet);

    (*fileStoreSet)
        .setDataFile(fileSet.d_dataFileName)
        .setDataFileSize(fileSet.d_dataFilePosition)
        .setJournalFile(fileSet.d_journalFileName)
        .setJournalFileSize(fileSet.d_journalFilePosition);
    if (needQList) {
        (*fileStoreSet)
            .setQlistFile(fileSet.d_qlistFileName)
            .setQlistFileSize(fileSet.d_qlistFilePosition);
    }
}

int FileStoreUtil::findFileSets(bsl::vector<FileStoreSet>* fileSets,
                                const bslstl::StringRef&   basePath,
                                int                        partitionId,
                                bool                       withSize,
                                bool                       needQList)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fileSets);

    enum {
        rc_SUCCESS                       = 0,
        rc_FILE_PATTERN_CREATION_FAILURE = -1  // Failed to create file pattern
    };

    fileSets->clear();

    bsl::string bmqFilePattern;
    int         rc = createFilePattern(&bmqFilePattern, basePath, partitionId);
    if (0 != rc) {
        return 10 * rc + rc_FILE_PATTERN_CREATION_FAILURE;  // RETURN
    }

    BALL_LOG_INFO << "Searching for files with pattern [" << bmqFilePattern
                  << "]";

    bsl::vector<bsl::string> files;
    bdls::FilesystemUtil::findMatchingPaths(&files, bmqFilePattern.c_str());

    FileSetMap fileSetMap;
    // timestamp (YYYYMMDD_HHMMSS) => FileStoreSet containing 3 files having
    //                                same timestamp in their names

    findFileStoreSetsFromPaths(&fileSetMap, files, withSize);

    // Validate 'fileSetMap'. All three files in each file set must be non
    // empty and, if applicable, have non-negative size.
    FileSetMap::const_iterator cit = fileSetMap.begin();
    while (cit != fileSetMap.end()) {
        const FileStoreSet& fileSet = cit->second;

        if (fileSet.dataFile().empty() ||
            (needQList && fileSet.qlistFile().empty()) ||
            fileSet.journalFile().empty()) {
            BALL_LOG_ERROR << "For timestamp [" << cit->first << "], at least "
                           << "one of the three files are missing. Data file ["
                           << fileSet.dataFile() << "], qlist file ["
                           << fileSet.qlistFile() << "], journal file ["
                           << fileSet.journalFile() << "]. Excluding this file"
                           << " set from the result.";

            // Erase this entry and continue
            fileSetMap.erase(cit++);
            continue;  // CONTINUE
        }

        // Can we make any assumptions on the minimum size of each file, or
        // maybe we could make the assumption that at least one file would have
        // positive file size.
        if (withSize) {
            if (fileSet.dataFileSize() == 0 ||
                (needQList && fileSet.qlistFileSize() == 0) ||
                fileSet.journalFileSize() == 0) {
                BALL_LOG_ERROR
                    << "For timestamp [" << cit->first << "], at least "
                    << "one of the three files has size of 0. Data "
                    << "file [" << fileSet.dataFile() << "], qlist "
                    << "file [" << fileSet.qlistFile() << "], journal "
                    << "file [" << fileSet.journalFile() << "]. "
                    << "Excluding this file set from the result.";

                // Erase this entry and continue
                fileSetMap.erase(cit++);
                continue;  // CONTINUE
            }
        }

        // All good with this entry 'cit'
        fileSets->push_back(fileSet);
        ++cit;
    }

    // Entries are already sorted as desired (older timestamp is first)
    // courtesy of using a map.

    return 0;
}

void FileStoreUtil::deleteArchiveFiles(int                partitionId,
                                       const bsl::string& archiveLocation,
                                       int                maxArchivedFileSets,
                                       const bsl::string& cluster)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(!archiveLocation.empty());
    BSLS_ASSERT_SAFE(0 <= maxArchivedFileSets);
    BSLS_ASSERT_SAFE(bdls::FilesystemUtil::exists(archiveLocation));
    BSLS_ASSERT_SAFE(bdls::FilesystemUtil::isDirectory(archiveLocation));

    bsl::string pattern;
    int         rc = createFilePattern(&pattern, archiveLocation, partitionId);
    if (0 != rc) {
        BALL_LOG_ERROR << cluster
                       << ": Failed to create file pattern for partitionId"
                       << " " << partitionId
                       << " while attemping to clean up archived storage "
                       << "files at [" << archiveLocation
                       << "].  Skipping this partition.";
        return;  // RETURN
    }

    const int numFilesToKeep =
        maxArchivedFileSets *
        mqbs::FileStoreProtocol::k_NUM_FILES_PER_PARTITION;

    bsl::vector<bsl::string> archivedFiles;
    bdls::FilesystemUtil::findMatchingPaths(&archivedFiles, pattern.c_str());

    if (numFilesToKeep >= static_cast<int>(archivedFiles.size())) {
        return;  // RETURN
    }

    // Sort 'archivedFiles' as per their names.  Note that file names contain
    // creation date and timestamp in their names and operator< on the string
    // will result in the desired sorting order, which is oldest to newest
    // file.

    bsl::sort(archivedFiles.begin(), archivedFiles.end());

    const unsigned int numFilesToDelete = archivedFiles.size() -
                                          numFilesToKeep;

    BALL_LOG_INFO << cluster << ": PartitionId [" << partitionId
                  << "], deleting " << numFilesToDelete << " files.";

    for (unsigned int i = 0; i < numFilesToDelete; ++i) {
        rc = bdls::FilesystemUtil::remove(archivedFiles[i]);
        if (0 != rc) {
            MWCTSK_ALARMLOG_ALARM("FILE_IO")
                << cluster << ": Failed to remove [" << archivedFiles[i]
                << "] file during archived storage cleanup for "
                << "PartitionId [" << partitionId << "], rc: " << rc
                << MWCTSK_ALARMLOG_END;
            continue;  // CONTINUE
        }

        BALL_LOG_INFO << cluster << ": Removed file [" << archivedFiles[i]
                      << "] during archived storage cleanup for "
                      << "PartitionId [" << partitionId << "].";
    }
}

void FileStoreUtil::deleteArchiveFiles(
    const mqbcfg::PartitionConfig& partitionCfg,
    const bsl::string&             cluster)
{
    const bsl::string& archiveLocation = partitionCfg.archiveLocation();
    BSLS_ASSERT_SAFE(!archiveLocation.empty());

    if (!bdls::FilesystemUtil::exists(archiveLocation)) {
        MWCTSK_ALARMLOG_ALARM("MISSING_FILE_OR_DIRECTORY")
            << cluster << ": Archive storage location [" << archiveLocation
            << "] no longer exists." << MWCTSK_ALARMLOG_END;
        return;  // RETURN
    }

    if (!bdls::FilesystemUtil::isDirectory(archiveLocation)) {
        MWCTSK_ALARMLOG_ALARM("MISSING_FILE_OR_DIRECTORY")
            << cluster << ": Archive storage location [" << archiveLocation
            << "] is not a directory." << MWCTSK_ALARMLOG_END;

        return;  // RETURN
    }

    const unsigned int numPartitions = static_cast<unsigned int>(
        partitionCfg.numPartitions());

    for (unsigned int pid = 0; pid < numPartitions; ++pid) {
        deleteArchiveFiles(pid,
                           archiveLocation,
                           partitionCfg.maxArchivedFileSets(),
                           cluster);
    }
}

int FileStoreUtil::openFileSetReadMode(bsl::ostream&         errorDescription,
                                       const FileStoreSet&   fileSet,
                                       MappedFileDescriptor* journalFd,
                                       MappedFileDescriptor* dataFd,
                                       MappedFileDescriptor* qlistFd)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(journalFd || dataFd || qlistFd);

    return openFileSet(errorDescription,
                       fileSet,
                       true,   // readOnly
                       false,  // prefaultPages
                       journalFd,
                       dataFd,
                       qlistFd);
}

int FileStoreUtil::openFileSetWriteMode(bsl::ostream&         errorDescription,
                                        const FileStoreSet&   fileSet,
                                        bool                  preallocate,
                                        bool                  deleteOnFailure,
                                        MappedFileDescriptor* journalFd,
                                        MappedFileDescriptor* dataFd,
                                        MappedFileDescriptor* qlistFd,
                                        bool                  prefaultPages)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(journalFd || dataFd || qlistFd);

    enum {
        rc_SUCCESS              = 0,
        rc_FILE_OPEN_FAILURE    = -1,
        rc_JOURNAL_GROW_FAILURE = -2,
        rc_DATA_GROW_FAILURE    = -3,
        rc_QLIST_GROW_FAILURE   = -4
    };

    bdlb::ScopeExitAny failureGuard(bdlf::BindUtil::bind(closeAndDeleteFileSet,
                                                         fileSet,
                                                         deleteOnFailure,
                                                         journalFd,
                                                         dataFd,
                                                         qlistFd));

    int rc = openFileSet(errorDescription,
                         fileSet,
                         false,  // readOnly
                         prefaultPages,
                         journalFd,
                         dataFd,
                         qlistFd);

    if (0 != rc) {
        return 10 * rc + rc_FILE_OPEN_FAILURE;  // RETURN
    }

    // Grow the files, pre-allocating if requested.
    mwcu::MemOutStream errorDesc;
    if (journalFd) {
        rc = FileSystemUtil::grow(journalFd, preallocate, errorDesc);
        if (0 != rc) {
            errorDescription << "Failed to grow journal file ["
                             << fileSet.journalFile() << "], rc: " << rc
                             << ", error: " << errorDesc.str();

            return 10 * rc + rc_JOURNAL_GROW_FAILURE;  // RETURN
        }
    }

    if (dataFd) {
        rc = FileSystemUtil::grow(dataFd, preallocate, errorDesc);
        if (0 != rc) {
            errorDescription << "Failed to grow data file ["
                             << fileSet.dataFile() << "], rc: " << rc
                             << ", error: " << errorDesc.str();

            return 10 * rc + rc_DATA_GROW_FAILURE;  // RETURN
        }
    }

    if (qlistFd) {
        rc = FileSystemUtil::grow(qlistFd, preallocate, errorDesc);
        if (0 != rc) {
            errorDescription << "Failed to grow qlist file ["
                             << fileSet.qlistFile() << "], rc: " << rc
                             << ", error: " << errorDesc.str();

            return 10 * rc + rc_QLIST_GROW_FAILURE;  // RETURN
        }
    }

    failureGuard.release();
    return rc_SUCCESS;
}

int FileStoreUtil::validateFileSet(const MappedFileDescriptor& journalFd,
                                   const MappedFileDescriptor& dataFd,
                                   const MappedFileDescriptor& qlistFd)
{
    enum {
        rc_SUCCESS         = 0,
        rc_UNKNOWN         = -1,
        rc_INVALID_JOURNAL = -2,
        rc_INVALID_DATA    = -3,
        rc_INVALID_QLIST   = -4
    };

    int rc = rc_UNKNOWN;
    if (journalFd.isValid()) {
        rc = FileStoreProtocolUtil::hasBmqHeader(journalFd);
        if (0 != rc) {
            return 10 * rc + rc_INVALID_JOURNAL;  // RETURN
        }
    }

    if (dataFd.isValid()) {
        rc = FileStoreProtocolUtil::hasBmqHeader(dataFd);
        if (0 != rc) {
            return 10 * rc + rc_INVALID_DATA;  // RETURN
        }
    }

    if (qlistFd.isValid()) {
        rc = FileStoreProtocolUtil::hasBmqHeader(qlistFd);
        if (0 != rc) {
            return 10 * rc + rc_INVALID_QLIST;  // RETURN
        }
    }

    return rc_SUCCESS;
}

int FileStoreUtil::openRecoveryFileSet(bsl::ostream&         errorDescription,
                                       MappedFileDescriptor* journalFd,
                                       FileStoreSet*         recoveryFileSet,
                                       int                   partitionId,
                                       int                   numSetsToCheck,
                                       const mqbs::DataStoreConfig& config,
                                       bool                         readOnly,
                                       bool                  isFSMWorkflow,
                                       MappedFileDescriptor* dataFd,
                                       MappedFileDescriptor* qlistFd)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(journalFd);
    BSLS_ASSERT_SAFE(recoveryFileSet);
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(0 < numSetsToCheck);
    BSLS_ASSERT_SAFE(!config.location().isEmpty());
    BSLS_ASSERT_SAFE(!config.archiveLocation().isEmpty());

    enum {
        rc_NO_FILE_SETS_TO_RECOVER = 1  // Special rc, do not change
        ,
        rc_SUCCESS                    = 0,
        rc_FILE_SET_RETRIEVAL_FAILURE = -1  // Failed to retrieve file sets
        ,
        rc_RECOVERY_SET_RETRIEVAL_FAILURE = -2  // Failed to retrieve file set
                                                // from which recovery could be
                                                // performed
    };

    const int                 k_INVALID_INDEX = -1;
    bsl::vector<FileStoreSet> fileSets;

    int rc = findFileSets(&fileSets,
                          config.location(),
                          partitionId,
                          true,  // withSize
                          qlistFd != 0);
    if (0 != rc) {
        errorDescription << "Failed to retrieve candidate file sets for "
                         << "recovery, rc: " << rc;
        return 10 * rc + rc_FILE_SET_RETRIEVAL_FAILURE;  // RETURN
    }

    BALL_LOG_INFO << "PartitionId [" << partitionId << "]: Number of file "
                  << "sets found for potential recovery: " << fileSets.size();

    if (fileSets.empty()) {
        // Return special rc indicating that no sets were found.
        return rc_NO_FILE_SETS_TO_RECOVER;  // RETURN
    }

    int              recoveryIndex = k_INVALID_INDEX;
    bsl::vector<int> archivingIndices;

    for (int i = (fileSets.size() - 1); i >= 0; --i) {
        if (numSetsToCheck-- < 0) {
            break;  // BREAK
        }

        const FileStoreSet& fs = fileSets[i];

        BALL_LOG_INFO << "PartitionId [" << partitionId << "]"
                      << ": Checking file set: " << fs;

        mwcu::MemOutStream errorDesc;
        if (readOnly) {
            rc =
                openFileSetReadMode(errorDesc, fs, journalFd, dataFd, qlistFd);
        }
        else {
            rc = openFileSetWriteMode(errorDesc,
                                      fs,
                                      config.hasPreallocate(),
                                      false,  // deleteOnFailure
                                      journalFd,
                                      dataFd,
                                      qlistFd,
                                      config.hasPrefaultPages());
        }

        if (rc != 0) {
            BALL_LOG_WARN << "PartitionId [" << partitionId
                          << "]: file set: " << fs
                          << " failed to open. Reason: " << errorDesc.str()
                          << ", rc: " << rc;
            archivingIndices.push_back(i);
            continue;  // CONTINUE
        }

        rc = validateFileSet(*journalFd,
                             dataFd ? *dataFd : MappedFileDescriptor(),
                             qlistFd ? *qlistFd : MappedFileDescriptor());

        if (rc != 0) {
            // Close this set before checking others, if any.

            BALL_LOG_ERROR << "PartitionId [" << partitionId
                           << "]: file set: " << fs
                           << " validation failed, rc: " << rc;
            FileSystemUtil::close(journalFd);
            if (dataFd) {
                FileSystemUtil::close(dataFd);
            }
            if (qlistFd) {
                FileSystemUtil::close(qlistFd);
            }
            archivingIndices.push_back(i);
            continue;  // CONTINUE
        }

        // Files have now been opened and basic validation has been performed.
        rc = FileStoreProtocolUtil::hasValidFirstSyncPointRecord(*journalFd);
        if (isFSMWorkflow || 0 == rc) {
            recoveryIndex = i;
            break;  // BREAK
        }
        else {
            // No valid first sync point record in this file.
            BALL_LOG_INFO << "PartitionId [" << partitionId << "]"
                          << ": No valid first sync point found in journal"
                          << "file [" << fs.journalFile() << "], rc: " << rc;

            if ((fileSets.size() == 1) || (numSetsToCheck == 0)) {
                // In case there is only 1 recoverable set or this is our last
                // attempt to check the file set, *and* journal of that set
                // does not have a valid first sync point record, it is still
                // ok to go ahead and recover messages from it.  TBD: perhaps
                // we could take a flag here.

                recoveryIndex = i;
                break;  // BREAK
            }

            // There are more file sets to be checked.  Close this one out
            // before checking them.
            FileSystemUtil::close(journalFd);
            if (dataFd) {
                FileSystemUtil::close(dataFd);
            }
            if (qlistFd) {
                FileSystemUtil::close(qlistFd);
            }
            archivingIndices.push_back(i);
            continue;  // CONTINUE
        }
    }

    if (recoveryIndex == k_INVALID_INDEX) {
        errorDescription << "Failed to retrieve any file set from which "
                         << "recovery could be performed.";
        return rc_RECOVERY_SET_RETRIEVAL_FAILURE;  // RETURN
    }

    // Found a recoverable set.  Archive the remaining file sets.
    BALL_LOG_INFO << "PartitionId [" << partitionId << "]: archiving "
                  << archivingIndices.size() << " file sets.";

    for (unsigned int i = 0; i < archivingIndices.size(); ++i) {
        if (archivingIndices[i] == recoveryIndex) {
            // Should never occur.
            continue;  // CONTINUE
        }

        const FileStoreSet& archivingFileSet = fileSets[archivingIndices[i]];
        rc = FileSystemUtil::move(archivingFileSet.dataFile(),
                                  config.archiveLocation());
        if (rc != 0) {
            BALL_LOG_WARN << "PartitionId [" << partitionId << "]: Failed to "
                          << "archive data file ["
                          << archivingFileSet.dataFile() << "], rc: " << rc;
        }

        rc = FileSystemUtil::move(archivingFileSet.qlistFile(),
                                  config.archiveLocation());
        if (0 != rc) {
            BALL_LOG_WARN << "PartitionId [" << partitionId << "]: Failed to "
                          << "archive qlist file ["
                          << archivingFileSet.qlistFile() << "], rc: " << rc;
        }

        rc = FileSystemUtil::move(archivingFileSet.journalFile(),
                                  config.archiveLocation());
        if (0 != rc) {
            BALL_LOG_WARN << "PartitionId [" << partitionId << "]: Failed to "
                          << "archive journal file ["
                          << archivingFileSet.journalFile() << "], rc: " << rc;
        }
    }

    *recoveryFileSet = fileSets[recoveryIndex];
    return rc_SUCCESS;
}

int FileStoreUtil::loadIterators(bsl::ostream&               errorDescription,
                                 JournalFileIterator*        jit,
                                 DataFileIterator*           dit,
                                 QlistFileIterator*          qit,
                                 const MappedFileDescriptor& journalFd,
                                 const MappedFileDescriptor& dataFd,
                                 const MappedFileDescriptor& qlistFd,
                                 const FileStoreSet&         fileSet,
                                 bool                        needQList,
                                 bool                        needData)
{
    BSLS_ASSERT_SAFE(jit);
    BSLS_ASSERT_SAFE(journalFd.isValid());

    if (needData) {
        BSLS_ASSERT_SAFE(dit);
        BSLS_ASSERT_SAFE(dataFd.isValid());
    }

    if (needQList) {
        BSLS_ASSERT_SAFE(qit);
        BSLS_ASSERT_SAFE(qlistFd.isValid());
    }

    enum {
        rc_SUCCESS                       = 0,
        rc_JOURNAL_FILE_ITERATOR_FAILURE = -1,
        rc_DATA_FILE_ITERATOR_FAILURE    = -2,
        rc_QLIST_FILE_ITERATOR_FAILURE   = -3
    };

    int rc = jit->reset(&journalFd,
                        FileStoreProtocolUtil::bmqHeader(journalFd),
                        true);  // reverse mode
    if (0 != rc) {
        errorDescription << "Failed to create journal iterator for ["
                         << fileSet.journalFile() << "], rc: " << rc;

        return rc_JOURNAL_FILE_ITERATOR_FAILURE;  // RETURN
    }
    BSLS_ASSERT_SAFE(jit->isValid());

    if (needQList) {
        rc = qit->reset(&qlistFd, FileStoreProtocolUtil::bmqHeader(qlistFd));
        if (0 != rc) {
            errorDescription << "Failed to create qlist iterator for ["
                             << fileSet.qlistFile() << "], rc: " << rc;

            return rc_QLIST_FILE_ITERATOR_FAILURE;  // RETURN
        }
        BSLS_ASSERT_SAFE(qit->isValid());
    }

    if (needData) {
        rc = dit->reset(&dataFd, FileStoreProtocolUtil::bmqHeader(dataFd));
        if (0 != rc) {
            errorDescription << "Failed to create data iterator for ["
                             << fileSet.dataFile() << "], rc: " << rc;

            return rc_DATA_FILE_ITERATOR_FAILURE;  // RETURN
        }
        BSLS_ASSERT_SAFE(dit->isValid());
    }

    return rc_SUCCESS;
}

}  // close package namespace
}  // close enterprise namespace
