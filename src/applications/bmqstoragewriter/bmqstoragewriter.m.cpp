// Copyright 2024 Bloomberg Finance L.P.
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

#include <m_bmqstoragewriter_cslwriter.h>
#include <m_bmqstoragewriter_journalwriter.h>

// MQB
#include <mqbs_filestoreprotocol.h>
#include <mqbu_storagekey.h>

// BDE
#include <balcl_commandline.h>
#include <bdls_filesystemutil.h>
#include <bsl_cstring.h>
#include <bsl_fstream.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bslma_default.h>

using namespace BloombergLP;

namespace {

// ============================================================================
//                              FILE CREATION
// ============================================================================

bsl::string buildFilePath(const bsl::string& outputDir,
                          const bsl::string& prefix,
                          int                partitionId,
                          const char*        extension,
                          bslma::Allocator*  allocator)
{
    bsl::string path(allocator);
    path.append(outputDir);
    if (!outputDir.empty() && *(outputDir.rbegin()) != '/') {
        path.append(1, '/');
    }
    path.append(prefix);
    path.append(mqbs::FileStoreProtocol::k_COMMON_FILE_PREFIX);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d", partitionId);
    path.append(buf);
    path.append(extension);
    return path;
}

int writeFileHeader(bdls::FilesystemUtil::FileDescriptor fd,
                    mqbs::FileType::Enum                 fileType,
                    int                                  partitionId)
{
    mqbs::FileHeader fh;
    bsl::memset(&fh, 0, sizeof(fh));
    new (&fh) mqbs::FileHeader();
    fh.setFileType(fileType).setPartitionId(partitionId);

    if (bdls::FilesystemUtil::write(fd, &fh, sizeof(fh)) != sizeof(fh)) {
        return -1;
    }

    if (fileType == mqbs::FileType::e_DATA) {
        mqbs::DataFileHeader dfh;
        bsl::memset(&dfh, 0, sizeof(dfh));
        new (&dfh) mqbs::DataFileHeader();
        dfh.setFileKey(mqbu::StorageKey::k_NULL_KEY);
        if (bdls::FilesystemUtil::write(fd, &dfh, sizeof(dfh)) !=
            sizeof(dfh)) {
            return -1;
        }
    }
    else if (fileType == mqbs::FileType::e_JOURNAL) {
        mqbs::JournalFileHeader jfh;
        bsl::memset(&jfh, 0, sizeof(jfh));
        new (&jfh) mqbs::JournalFileHeader();
        if (bdls::FilesystemUtil::write(fd, &jfh, sizeof(jfh)) !=
            sizeof(jfh)) {
            return -1;
        }
    }
    else {
        mqbs::QlistFileHeader qfh;
        bsl::memset(&qfh, 0, sizeof(qfh));
        new (&qfh) mqbs::QlistFileHeader();
        if (bdls::FilesystemUtil::write(fd, &qfh, sizeof(qfh)) !=
            sizeof(qfh)) {
            return -1;
        }
    }

    return 0;
}

int createFileSet(const bsl::string& outputDir,
                  const bsl::string& prefix,
                  int                partitionId,
                  bslma::Allocator*  allocator)
{
    if (!bdls::FilesystemUtil::isDirectory(outputDir)) {
        bsl::cerr << "Error: '" << outputDir << "' is not a directory\n";
        return -1;
    }

    const bsl::string dataPath = buildFilePath(
        outputDir,
        prefix,
        partitionId,
        mqbs::FileStoreProtocol::k_DATA_FILE_EXTENSION,
        allocator);
    const bsl::string journalPath = buildFilePath(
        outputDir,
        prefix,
        partitionId,
        mqbs::FileStoreProtocol::k_JOURNAL_FILE_EXTENSION,
        allocator);
    const bsl::string qlistPath = buildFilePath(
        outputDir,
        prefix,
        partitionId,
        mqbs::FileStoreProtocol::k_QLIST_FILE_EXTENSION,
        allocator);

    struct FileSpec {
        const char*          d_label;
        const bsl::string&   d_path;
        mqbs::FileType::Enum d_type;
    };

    const FileSpec files[] = {
        {"data", dataPath, mqbs::FileType::e_DATA},
        {"journal", journalPath, mqbs::FileType::e_JOURNAL},
        {"qlist", qlistPath, mqbs::FileType::e_QLIST}};

    for (int i = 0; i < 3; ++i) {
        if (bdls::FilesystemUtil::exists(files[i].d_path)) {
            continue;
        }

        bdls::FilesystemUtil::FileDescriptor fd = bdls::FilesystemUtil::open(
            files[i].d_path.c_str(),
            bdls::FilesystemUtil::e_CREATE_PRIVATE,
            bdls::FilesystemUtil::e_READ_WRITE);
        if (fd == bdls::FilesystemUtil::k_INVALID_FD) {
            bsl::cerr << "Error: cannot create file: " << files[i].d_path
                      << "\n";
            return -1;
        }

        int rc = writeFileHeader(fd, files[i].d_type, partitionId);
        bdls::FilesystemUtil::close(fd);

        if (rc != 0) {
            bsl::cerr << "Error: cannot write " << files[i].d_label
                      << " header\n";
            return -1;
        }

        bsl::cout << "Created " << files[i].d_label << ": " << files[i].d_path
                  << "\n";
    }

    return 0;
}

// ============================================================================
//                              WRITE COMMAND
// ============================================================================

int writeCommand(const bsl::string& inputFile,
                 const bsl::string& cslInput,
                 const bsl::string& outputDir,
                 const bsl::string& prefix,
                 int                partitionId,
                 bslma::Allocator*  allocator)
{
    // Ensure output files exist (create if needed)
    int rc = createFileSet(outputDir, prefix, partitionId, allocator);
    if (rc != 0) {
        return rc;
    }

    // Process CSL input (populate queue cache + write CSL file)
    m_bmqstoragewriter::QueueCache cache(allocator);
    if (!cslInput.empty()) {
        bsl::ifstream cslStream(cslInput.c_str());
        if (!cslStream.is_open()) {
            bsl::cerr << "Error: cannot open CSL input: " << cslInput << "\n";
            return -1;
        }

        // Open/create CSL output file
        bsl::string cslPath(outputDir, allocator);
        if (!cslPath.empty() && *(cslPath.rbegin()) != '/') {
            cslPath.append(1, '/');
        }
        cslPath.append(prefix);
        cslPath.append("csl.bmq_csl");

        bool newFile = !bdls::FilesystemUtil::exists(cslPath);
        bdls::FilesystemUtil::FileDescriptor cslFd =
            bdls::FilesystemUtil::open(cslPath.c_str(),
                                       bdls::FilesystemUtil::e_CREATE,
                                       bdls::FilesystemUtil::e_READ_WRITE);
        if (cslFd == bdls::FilesystemUtil::k_INVALID_FD) {
            bsl::cerr << "Error: cannot open CSL file: " << cslPath << "\n";
            return -1;
        }

        rc = m_bmqstoragewriter::processCslInput(&cache,
                                                 cslFd,
                                                 newFile,
                                                 cslStream,
                                                 allocator);
        bdls::FilesystemUtil::close(cslFd);
        if (rc != 0) {
            return rc;
        }
    }

    // Process journal input
    if (!inputFile.empty()) {
        bsl::ifstream jStream(inputFile.c_str());
        if (!jStream.is_open()) {
            bsl::cerr << "Error: cannot open journal input: " << inputFile
                      << "\n";
            return -1;
        }

        bsl::string journalPath = buildFilePath(
            outputDir,
            prefix,
            partitionId,
            mqbs::FileStoreProtocol::k_JOURNAL_FILE_EXTENSION,
            allocator);
        bsl::string dataPath = buildFilePath(
            outputDir,
            prefix,
            partitionId,
            mqbs::FileStoreProtocol::k_DATA_FILE_EXTENSION,
            allocator);
        bsl::string qlistPath = buildFilePath(
            outputDir,
            prefix,
            partitionId,
            mqbs::FileStoreProtocol::k_QLIST_FILE_EXTENSION,
            allocator);

        bdls::FilesystemUtil::FileDescriptor journalFd =
            bdls::FilesystemUtil::open(journalPath.c_str(),
                                       bdls::FilesystemUtil::e_OPEN,
                                       bdls::FilesystemUtil::e_READ_WRITE);
        bdls::FilesystemUtil::FileDescriptor dataFd =
            bdls::FilesystemUtil::open(dataPath.c_str(),
                                       bdls::FilesystemUtil::e_OPEN,
                                       bdls::FilesystemUtil::e_READ_WRITE);
        bdls::FilesystemUtil::FileDescriptor qlistFd =
            bdls::FilesystemUtil::open(qlistPath.c_str(),
                                       bdls::FilesystemUtil::e_OPEN,
                                       bdls::FilesystemUtil::e_READ_WRITE);

        if (journalFd == bdls::FilesystemUtil::k_INVALID_FD ||
            dataFd == bdls::FilesystemUtil::k_INVALID_FD ||
            qlistFd == bdls::FilesystemUtil::k_INVALID_FD) {
            bsl::cerr << "Error: cannot open output files\n";
            if (journalFd != bdls::FilesystemUtil::k_INVALID_FD)
                bdls::FilesystemUtil::close(journalFd);
            if (dataFd != bdls::FilesystemUtil::k_INVALID_FD)
                bdls::FilesystemUtil::close(dataFd);
            if (qlistFd != bdls::FilesystemUtil::k_INVALID_FD)
                bdls::FilesystemUtil::close(qlistFd);
            return -1;
        }

        rc = m_bmqstoragewriter::processJournalInput(cache,
                                                     journalFd,
                                                     dataFd,
                                                     qlistFd,
                                                     jStream,
                                                     allocator);

        bdls::FilesystemUtil::close(journalFd);
        bdls::FilesystemUtil::close(dataFd);
        bdls::FilesystemUtil::close(qlistFd);

        if (rc != 0) {
            return rc;
        }
    }

    return 0;
}

}  // close unnamed namespace

int main(int argc, const char* argv[])
{
    bslma::Allocator* allocator = bslma::Default::allocator();

    bsl::string inputFile(allocator);
    bsl::string cslInput(allocator);
    bsl::string outputDir(".", allocator);
    bsl::string prefix("bmq_", allocator);
    int         partitionId = 0;

    // clang-format off
    balcl::OptionInfo specTable[] = {
        {
            "i|input",
            "input",
            "Journal JSON input file (from storagetool)",
            balcl::TypeInfo(&inputFile),
            balcl::OccurrenceInfo(bsl::string(allocator))
        },
        {
            "c|csl-input",
            "cslInput",
            "CSL JSON input file (from storagetool)",
            balcl::TypeInfo(&cslInput),
            balcl::OccurrenceInfo(bsl::string(allocator))
        },
        {
            "o|output-dir",
            "outputDir",
            "Output directory (default: .)",
            balcl::TypeInfo(&outputDir),
            balcl::OccurrenceInfo(bsl::string(".", allocator))
        },
        {
            "p|partition",
            "partitionId",
            "Partition ID (default: 0)",
            balcl::TypeInfo(&partitionId),
            balcl::OccurrenceInfo(0)
        },
        {
            "prefix",
            "prefix",
            "Filename prefix (default: bmq_)",
            balcl::TypeInfo(&prefix),
            balcl::OccurrenceInfo(bsl::string("bmq_", allocator))
        }
    };
    // clang-format on

    if (argc < 2) {
        balcl::CommandLine cmdLine(specTable, 5, allocator);
        cmdLine.printUsage();
        return 0;
    }

    balcl::CommandLine cmdLine(specTable, 5, allocator);
    if (cmdLine.parse(argc, argv) != 0) {
        return 1;
    }

    return writeCommand(inputFile,
                        cslInput,
                        outputDir,
                        prefix,
                        partitionId,
                        allocator);
}
