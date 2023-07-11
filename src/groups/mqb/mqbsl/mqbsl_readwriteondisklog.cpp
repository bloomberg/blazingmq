// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mqbsl_readwriteondisklog.cpp                                       -*-C++-*-
#include <mqbsl_readwriteondisklog.h>

#include <mqbscm_version.h>
// MQB
#include <mqbs_filesystemutil.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <bdlb_scopeexit.h>
#include <bdlbb_blobutil.h>
#include <bdlf_bind.h>
#include <bdls_filesystemutil.h>
#include <bsl_algorithm.h>  // for bsl::max, bsl::min
#include <bsl_limits.h>
#include <bslma_allocator.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>

// SYS
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace BloombergLP {
namespace mqbsl {

namespace {

/// When `open()` fails, this method can be invoked to set the specified
/// `isOpenFlag` to false, close the specified `fd` and set it to the
/// specified `invalidFd`.
void openFailureCleanup(bool* isOpenFlag, int* fd, int invalidFd)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isOpenFlag);
    BSLS_ASSERT_SAFE(fd);

    *isOpenFlag = false;
    ::close(*fd);
    *fd = invalidFd;
}

}  // close unnamed namespace

// -------------------------------
// class ReadWriteOnDiskLogFactory
// -------------------------------

// CREATORS
ReadWriteOnDiskLogFactory::ReadWriteOnDiskLogFactory(
    bslma::Allocator* allocator)
: d_allocator_p(allocator)
{
    // NOTHING
}

ReadWriteOnDiskLogFactory::~ReadWriteOnDiskLogFactory()
{
    // NOTHING
}

// MANIPULATORS
bslma::ManagedPtr<mqbsi::Log>
ReadWriteOnDiskLogFactory::create(const mqbsi::LogConfig& config)
{
    bslma::ManagedPtr<mqbsi::Log> log(new (*d_allocator_p)
                                          ReadWriteOnDiskLog(config),
                                      d_allocator_p);
    return log;
}

// ------------------------
// class ReadWriteOnDiskLog
// ------------------------

const int mqbsl::ReadWriteOnDiskLog::k_INVALID_FD = -1;

int ReadWriteOnDiskLog::seekImpl(Offset offset) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(offset >= 0);

    const off_t retOffset = ::lseek(d_fd, offset, SEEK_SET);
    if (retOffset < 0) {
        return 100 * retOffset + LogOpResult::e_FILE_SEEK_FAILURE;  // RETURN
    }

    return LogOpResult::e_SUCCESS;
}

int ReadWriteOnDiskLog::validateRead(int length, Offset offset) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(offset >= 0);
    BSLS_ASSERT_SAFE(length >= 0);

    if (offset + length > d_totalNumBytes) {
        return LogOpResult::e_REACHED_END_OF_LOG;  // RETURN
    }

    return LogOpResult::e_SUCCESS;
}

void ReadWriteOnDiskLog::updateInternalState(int writeLength)

{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(writeLength >= 0);

    d_currentOffset += writeLength;
    d_outstandingNumBytes += writeLength;
    d_totalNumBytes = bsl::max(d_totalNumBytes, d_currentOffset);
}

void ReadWriteOnDiskLog::populateIoVectors(struct iovec*      ioVectors,
                                           int*               iovecCount,
                                           const bdlbb::Blob& entry,
                                           const mwcu::BlobPosition& offset,
                                           int                       length)
{
    int iovecCnt        = 0;
    int remainingLength = length;
    int bufIndex        = offset.buffer();
    int bufOffset       = offset.byte();

    while ((remainingLength > 0) && (iovecCnt < IOV_MAX)) {
        BSLS_ASSERT_SAFE(bufIndex < entry.numDataBuffers());

        const bdlbb::BlobBuffer& buf = entry.buffer(bufIndex);
        const int bufSize = mwcu::BlobUtil::bufferSize(entry, bufIndex);
        const int nbytes  = bsl::min(remainingLength, bufSize - bufOffset);
        ioVectors[iovecCnt].iov_base = buf.data() + bufOffset;
        ioVectors[iovecCnt].iov_len  = nbytes;

        ++bufIndex;
        ++iovecCnt;
        bufOffset = 0;
        remainingLength -= nbytes;
    }

    *iovecCount = iovecCnt;
}

ReadWriteOnDiskLog::ReadWriteOnDiskLog(const mqbsi::LogConfig& config)
: d_isOpened(false)
, d_isReadOnly(false)
, d_totalNumBytes(0)
, d_outstandingNumBytes(0)
, d_currentOffset(0)
, d_config(config)
, d_fd(k_INVALID_FD)
{
    // NOTHING
}

ReadWriteOnDiskLog::~ReadWriteOnDiskLog()
{
    // NOTHING
}

int ReadWriteOnDiskLog::open(int flags)
{
    const bool alreadyExists = bdls::FilesystemUtil::exists(
        logConfig().location());
    if (!(flags & e_CREATE_IF_MISSING) && !alreadyExists) {
        return LogOpResult::e_FILE_NOT_EXIST;  // RETURN
    }

    const bool openReadOnly = flags & e_READ_ONLY;
    const int  oflag        = openReadOnly ? O_RDONLY : (O_RDWR | O_CREAT);
    d_fd                    = ::open(d_config.location().c_str(),
                  oflag,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (d_fd < 0) {
        d_fd = k_INVALID_FD;
        return LogOpResult::e_FILE_OPEN_FAILURE;  // RETURN
    }
    d_isOpened      = true;
    d_totalNumBytes = bdls::FilesystemUtil::getFileSize(d_config.location());
    d_outstandingNumBytes = d_totalNumBytes;
    bdlb::ScopeExitAny guard(bdlf::BindUtil::bind(openFailureCleanup,
                                                  &d_isOpened,
                                                  &d_fd,
                                                  k_INVALID_FD));

    // If log is writable
    int                rc = 0;
    mwcu::MemOutStream errorDescription;
    if (!openReadOnly) {
        rc = mqbs::FileSystemUtil::grow(d_fd,
                                        logConfig().maxSize(),
                                        d_config.reserveOnDisk(),
                                        errorDescription);
        if (rc != 0) {
            return 100 * rc + LogOpResult::e_FILE_GROW_FAILURE;  // RETURN
        }
    }

    // Seek the file offset back to the end of file
    const Offset endOffset = static_cast<Offset>(d_totalNumBytes);
    rc                     = seekImpl(endOffset);
    if (rc != LogOpResult::e_SUCCESS) {
        return 100 * rc + LogOpResult::e_FILE_SEEK_FAILURE;  // RETURN
    }
    d_currentOffset = endOffset;

    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(alreadyExists || d_totalNumBytes == 0);
    BSLS_ASSERT_SAFE(d_currentOffset == d_totalNumBytes);

    d_isReadOnly = openReadOnly;
    guard.release();
    return LogOpResult::e_SUCCESS;
}

int ReadWriteOnDiskLog::close()
{
    if (d_fd == k_INVALID_FD) {
        d_isOpened   = false;
        d_isReadOnly = false;
        return LogOpResult::e_SUCCESS;  // RETURN
    }

    int rc = LogOpResult::e_UNKNOWN;
    if (!d_isReadOnly) {
        rc = ::ftruncate(d_fd, d_totalNumBytes);
        if (rc != 0) {
            return 100 * rc + LogOpResult::e_FILE_TRUNCATE_FAILURE;  // RETURN
        }
    }

    rc = ::close(d_fd);
    if (rc != 0) {
        d_fd = k_INVALID_FD;
        return 100 * rc + LogOpResult::e_FILE_CLOSE_FAILURE;  // RETURN
    }

    d_fd         = k_INVALID_FD;
    d_isOpened   = false;
    d_isReadOnly = false;
    return LogOpResult::e_SUCCESS;
}

int ReadWriteOnDiskLog::seek(Offset offset)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(offset >= 0);

    if (offset > logConfig().maxSize()) {
        return LogOpResult::e_OFFSET_OUT_OF_RANGE;  // RETURN
    }

    const int rc = seekImpl(offset);
    if (rc != LogOpResult::e_SUCCESS) {
        return 100 * rc + LogOpResult::e_FILE_SEEK_FAILURE;  // RETURN
    }

    d_currentOffset = offset;

    return LogOpResult::e_SUCCESS;
}

mqbsi::Log::Offset
ReadWriteOnDiskLog::write(const void* entry, int offset, int length)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(entry);
    BSLS_ASSERT_SAFE(offset >= 0);
    BSLS_ASSERT_SAFE(length >= 0);

    const Offset oldOffset = d_currentOffset;
    if (length == 0) {
        return oldOffset;  // RETURN
    }
    if (oldOffset + length > logConfig().maxSize()) {
        return LogOpResult::e_REACHED_END_OF_LOG;  // RETURN
    }

    int rc              = LogOpResult::e_UNKNOWN;
    int remainingLength = length;
    do {
        rc = ::write(d_fd,
                     static_cast<const char*>(entry) + offset +
                         (length - remainingLength),
                     remainingLength);
        remainingLength -= rc;
    } while (rc > 0 && remainingLength > 0);

    if (rc < 0) {
        return LogOpResult::e_BYTE_WRITE_FAILURE;  // RETURN
    }

    updateInternalState(length);

    return oldOffset;
}

mqbsi::Log::Offset ReadWriteOnDiskLog::write(const bdlbb::Blob&        entry,
                                             const mwcu::BlobPosition& offset,
                                             int                       length)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(length >= 0);
    BSLS_ASSERT_SAFE(offset.buffer() >= 0);

    const Offset oldOffset = d_currentOffset;
    if ((length == 0) || (entry.numDataBuffers() == 0)) {
        return oldOffset;  // RETURN
    }
    if (oldOffset + length > logConfig().maxSize()) {
        return LogOpResult::e_REACHED_END_OF_LOG;  // RETURN
    }

    // writev() supports up to IOV_MAX iovecs, so batch it up
    struct iovec ioVectors[IOV_MAX];

    int                rc;
    int                remainingLength = length;
    mwcu::BlobPosition currOffset      = offset;
    mwcu::BlobPosition nextCurrOffset;
    do {
        int iovecCount = 0;
        populateIoVectors(ioVectors,
                          &iovecCount,
                          entry,
                          currOffset,
                          remainingLength);

        // Write Scatter/Gather IO
        const int numWritten = ::writev(d_fd, ioVectors, iovecCount);
        if (numWritten == -1) {
            rc = errno;
            return LogOpResult::e_BYTE_WRITE_FAILURE;  // RETURN
        }

        rc = mwcu::BlobUtil::findOffset(&nextCurrOffset,
                                        entry,
                                        currOffset,
                                        numWritten);
        BSLS_ASSERT_SAFE(rc == 0);

        currOffset = nextCurrOffset;
        remainingLength -= numWritten;
    } while (remainingLength > 0);

    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(remainingLength == 0);

    updateInternalState(length);

    return oldOffset;
}

mqbsi::Log::Offset ReadWriteOnDiskLog::write(const bdlbb::Blob&       entry,
                                             const mwcu::BlobSection& section)
{
    int length;
    int rc = mwcu::BlobUtil::sectionSize(&length, entry, section);
    if (rc != 0) {
        return LogOpResult::e_INVALID_BLOB_SECTION;  // RETURN
    }

    return write(entry, section.start(), length);
}

int ReadWriteOnDiskLog::flush(BSLS_ANNOTATION_UNUSED Offset offset)
{
    // TBD: Consider using fdatasync() for Linux and Solaris, and
    // fcntl(F_FULLFSYNC) for OSX instead. Ideally, we should add a
    // mqbs::FilesystemUtil::flushToDisk(...) routine, which hides above
    // details.

    if (d_totalNumBytes == 0) {
        return LogOpResult::e_SUCCESS;  // RETURN
    }

    const int rc = ::fsync(d_fd);
    if (rc != 0) {
        return LogOpResult::e_FILE_FLUSH_FAILURE;  // RETURN
    }

    return LogOpResult::e_SUCCESS;
}

int ReadWriteOnDiskLog::read(void* entry, int length, Offset offset) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(entry);

    int rc = validateRead(length, offset);
    if (rc != LogOpResult::e_SUCCESS) {
        return rc;  // RETURN
    }

    int numRead = 0;
    do {
        rc = ::pread(d_fd,
                     static_cast<char*>(entry) + numRead,
                     length,
                     offset + numRead);
        if (rc < 0) {
            return LogOpResult::e_BYTE_READ_FAILURE;  // RETURN
        }
        numRead += rc;
        length -= rc;
    } while (length > 0);

    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(length == 0);

    return LogOpResult::e_SUCCESS;
}

int ReadWriteOnDiskLog::read(bdlbb::Blob* entry,
                             int          length,
                             Offset       offset) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(entry);

    if (length == 0) {
        return LogOpResult::e_SUCCESS;
    }

    int rc = validateRead(length, offset);
    if (rc != LogOpResult::e_SUCCESS) {
        return rc;  // RETURN
    }

    rc = seekImpl(offset);
    if (rc != LogOpResult::e_SUCCESS) {
        return 100 * rc + LogOpResult::e_FILE_SEEK_FAILURE;  // RETURN
    }

    // Ensure blob has enough space for the intended length
    mwcu::BlobUtil::reserve(entry, length);

    // Read Scatter/Gather IO. readv() supports up to IOV_MAX iovecs, so batch
    // it up.
    struct iovec       ioVectors[IOV_MAX];
    const int          numDataBuffers  = entry->numDataBuffers();
    int                remainingLength = length;
    mwcu::BlobPosition currOffset(0, 0);
    mwcu::BlobPosition nextCurrOffset;

    while (remainingLength > 0) {
        const int currBufIdx    = currOffset.buffer();
        const int currBufOffset = currOffset.byte();

        // i.e. Case j == 0
        const bdlbb::BlobBuffer& buf0 = entry->buffer(currBufIdx);
        ioVectors[0].iov_base         = buf0.data() + currBufOffset;
        ioVectors[0].iov_len = mwcu::BlobUtil::bufferSize(*entry, currBufIdx) -
                               currBufOffset;

        const int iovecCount = bsl::min(numDataBuffers - currBufIdx, IOV_MAX);
        for (int j = 1; j < iovecCount; ++j) {
            const bdlbb::BlobBuffer& buf = entry->buffer(currBufIdx + j);
            ioVectors[j].iov_base        = buf.data();
            ioVectors[j].iov_len         = mwcu::BlobUtil::bufferSize(*entry,
                                                              currBufIdx + j);
        }

        const int numRead = ::readv(d_fd, ioVectors, iovecCount);
        if (numRead == -1) {
            return LogOpResult::e_BYTE_READ_FAILURE;  // RETURN
        }

        rc = mwcu::BlobUtil::findOffset(&nextCurrOffset,
                                        *entry,
                                        currOffset,
                                        numRead);
        BSLS_ASSERT_SAFE(rc == 0);

        currOffset = nextCurrOffset;
        remainingLength -= numRead;
    }

    // POSTCONDITIONS
    BSLS_ASSERT(remainingLength == 0);

    rc = seekImpl(d_currentOffset);
    if (rc != LogOpResult::e_SUCCESS) {
        return 100 * rc + LogOpResult::e_FILE_SEEK_FAILURE;  // RETURN
    }

    return LogOpResult::e_SUCCESS;
}

int ReadWriteOnDiskLog::alias(BSLS_ANNOTATION_UNUSED void** entry,
                              BSLS_ANNOTATION_UNUSED int    length,
                              BSLS_ANNOTATION_UNUSED Offset offset) const
{
    BSLS_ASSERT_OPT(false && "Aliasing is not supported for"
                             "ReadWriteOnDiskLog");

    return LogOpResult::e_UNSUPPORTED_OPERATION;
}

int ReadWriteOnDiskLog::alias(BSLS_ANNOTATION_UNUSED bdlbb::Blob* entry,
                              BSLS_ANNOTATION_UNUSED int          length,
                              BSLS_ANNOTATION_UNUSED Offset       offset) const
{
    BSLS_ASSERT_OPT(false && "Aliasing is not supported for"
                             "ReadWriteOnDiskLog");

    return LogOpResult::e_UNSUPPORTED_OPERATION;
}

}  // close package namespace
}  // close enterprise namespace
