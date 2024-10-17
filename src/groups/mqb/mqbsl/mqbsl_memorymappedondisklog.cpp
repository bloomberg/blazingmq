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

// mqbsl_memorymappedondisklog.cpp                                    -*-C++-*-
#include <mqbsl_memorymappedondisklog.h>

#include <mqbscm_version.h>
// MQB
#include <mqbs_filesystemutil.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_scopeexit.h>
#include <bdlbb_blobutil.h>
#include <bdlf_bind.h>
#include <bdls_filesystemutil.h>
#include <bsl_algorithm.h>  // for bsl::max
#include <bsl_cstring.h>    // for bsl::memcpy
#include <bsl_memory.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>

// SYS
#include <sys/mman.h>  // for msync

namespace BloombergLP {
namespace mqbsl {

namespace {

/// When `open()` fails, this method can be invoked to close the specified
/// `mfd` and set the specified `isOpenFlag` to false.
void openFailureCleanup(mqbs::MappedFileDescriptor* mfd, bool* isOpenFlag)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(mfd);
    BSLS_ASSERT_SAFE(isOpenFlag);

    mqbs::FileSystemUtil::close(mfd);
    *isOpenFlag = false;
}

}  // close unnamed namespace

// ----------------------------------
// class MemoryMappedOnDiskLogFactory
// ----------------------------------

// CREATORS
MemoryMappedOnDiskLogFactory::MemoryMappedOnDiskLogFactory(
    bslma::Allocator* allocator)
: d_allocator_p(allocator)
{
    // NOTHING
}

MemoryMappedOnDiskLogFactory::~MemoryMappedOnDiskLogFactory()
{
    // NOTHING
}

// MANIPULATORS
bslma::ManagedPtr<mqbsi::Log>
MemoryMappedOnDiskLogFactory::create(const mqbsi::LogConfig& config)
{
    bslma::ManagedPtr<mqbsi::Log> log(new (*d_allocator_p)
                                          MemoryMappedOnDiskLog(config),
                                      d_allocator_p);
    return log;
}

// ---------------------------
// class MemoryMappedOnDiskLog
// ---------------------------

// PRIVATE MANIPULATORS
int MemoryMappedOnDiskLog::validateRead(int length, Offset offset) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(offset >= 0);
    BSLS_ASSERT_SAFE(length >= 0);

    if (!d_isOpened) {
        return LogOpResult::e_UNSUPPORTED_OPERATION;  // RETURN
    }

    if (offset > d_totalNumBytes) {
        return LogOpResult::e_OFFSET_OUT_OF_RANGE;  // RETURN
    }

    if (offset + length > d_totalNumBytes) {
        return LogOpResult::e_REACHED_END_OF_LOG;  // RETURN
    }

    return LogOpResult::e_SUCCESS;
}

void MemoryMappedOnDiskLog::updateInternalState(int writeLength)

{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(writeLength >= 0);

    d_currentOffset += writeLength;
    d_outstandingNumBytes += writeLength;
    d_totalNumBytes = bsl::max(d_totalNumBytes, d_currentOffset);
}

MemoryMappedOnDiskLog::MemoryMappedOnDiskLog(const mqbsi::LogConfig& config)
: d_isOpened(false)
, d_isReadOnly(false)
, d_totalNumBytes(0)
, d_outstandingNumBytes(0)
, d_currentOffset(0)
, d_config(config)
, d_mfd()
{
    // NOTHING
}

MemoryMappedOnDiskLog::~MemoryMappedOnDiskLog()
{
    // NOTHING
}

int MemoryMappedOnDiskLog::open(int flags)
{
    const bool alreadyExists = bdls::FilesystemUtil::exists(
        logConfig().location());
    if (!(flags & e_CREATE_IF_MISSING) && !alreadyExists) {
        return LogOpResult::e_FILE_NOT_EXIST;  // RETURN
    }

    bmqu::MemOutStream errorDescription;
    const bool         openReadOnly = flags & e_READ_ONLY;
    int                rc           = mqbs::FileSystemUtil::open(&d_mfd,
                                        logConfig().location().c_str(),
                                        logConfig().maxSize(),
                                        openReadOnly,
                                        errorDescription,
                                        logConfig().prefaultPages());
    if (rc != 0) {
        return 100 * rc + LogOpResult::e_FILE_OPEN_FAILURE;  // RETURN
    }
    d_isOpened      = true;
    d_totalNumBytes = bdls::FilesystemUtil::getFileSize(d_config.location());
    d_outstandingNumBytes = d_totalNumBytes;

    bdlb::ScopeExitAny guard(
        bdlf::BindUtil::bind(openFailureCleanup, &d_mfd, &d_isOpened));

    // If log is writable
    if (!openReadOnly) {
        rc = mqbs::FileSystemUtil::grow(&d_mfd,
                                        logConfig().reserveOnDisk(),
                                        errorDescription);
        if (rc != 0) {
            return 100 * rc + LogOpResult::e_FILE_GROW_FAILURE;  // RETURN
        }
    }

    // Seek the file offset back to the end of file
    const Offset endOffset = static_cast<Offset>(d_totalNumBytes);
    rc                     = seek(endOffset);
    if (rc != LogOpResult::e_SUCCESS) {
        return 100 * rc + LogOpResult::e_FILE_SEEK_FAILURE;  // RETURN
    }

    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(alreadyExists || d_totalNumBytes == 0);
    BSLS_ASSERT_SAFE(d_currentOffset == d_totalNumBytes);

    d_isReadOnly = openReadOnly;
    guard.release();
    return LogOpResult::e_SUCCESS;
}

int MemoryMappedOnDiskLog::close()
{
    if (!d_isOpened) {
        return LogOpResult::e_LOG_ALREADY_CLOSED;  // RETURN
    }

    int rc = LogOpResult::e_UNKNOWN;
    if (!d_isReadOnly) {
        bmqu::MemOutStream errorDescription;
        rc = mqbs::FileSystemUtil::truncate(&d_mfd,
                                            d_totalNumBytes,
                                            errorDescription);
        if (rc != 0) {
            return 100 * rc + LogOpResult::e_FILE_TRUNCATE_FAILURE;  // RETURN
        }
    }

    rc = mqbs::FileSystemUtil::close(&d_mfd);
    if (rc != 0) {
        return 100 * rc + LogOpResult::e_FILE_CLOSE_FAILURE;  // RETURN
    }

    d_isOpened = false;

    return LogOpResult::e_SUCCESS;
}

int MemoryMappedOnDiskLog::seek(Offset offset)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(offset >= 0);

    if (!d_isOpened) {
        return LogOpResult::e_UNSUPPORTED_OPERATION;  // RETURN
    }
    if (offset > logConfig().maxSize()) {
        return LogOpResult::e_OFFSET_OUT_OF_RANGE;  // RETURN
    }

    d_currentOffset = offset;

    return LogOpResult::e_SUCCESS;
}

mqbsi::Log::Offset
MemoryMappedOnDiskLog::write(const void* entry, int offset, int length)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(entry);
    BSLS_ASSERT_SAFE(offset >= 0);
    BSLS_ASSERT_SAFE(length >= 0);

    if (!d_isOpened) {
        return LogOpResult::e_UNSUPPORTED_OPERATION;  // RETURN
    }

    const Offset oldOffset = d_currentOffset;
    if (oldOffset + length > logConfig().maxSize()) {
        return LogOpResult::e_REACHED_END_OF_LOG;  // RETURN
    }

    bsl::memcpy(d_mfd.mapping() + oldOffset,
                static_cast<const char*>(entry) + offset,
                length);

    updateInternalState(length);

    return oldOffset;
}

mqbsi::Log::Offset
MemoryMappedOnDiskLog::write(const bdlbb::Blob&        entry,
                             const bmqu::BlobPosition& offset,
                             int                       length)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(length >= 0);

    if (!d_isOpened) {
        return LogOpResult::e_UNSUPPORTED_OPERATION;  // RETURN
    }

    const Offset oldOffset = d_currentOffset;
    if (oldOffset + length > logConfig().maxSize()) {
        return LogOpResult::e_REACHED_END_OF_LOG;  // RETURN
    }

    int rc = bmqu::BlobUtil::readNBytes(d_mfd.mapping() + oldOffset,
                                        entry,
                                        offset,
                                        length);
    if (rc != 0) {
        return LogOpResult::e_BYTE_WRITE_FAILURE;  // RETURN
    }

    updateInternalState(length);

    return oldOffset;
}

mqbsi::Log::Offset
MemoryMappedOnDiskLog::write(const bdlbb::Blob&       entry,
                             const bmqu::BlobSection& section)
{
    int length;
    int rc = bmqu::BlobUtil::sectionSize(&length, entry, section);
    if (rc != 0) {
        return LogOpResult::e_INVALID_BLOB_SECTION;  // RETURN
    }

    return write(entry, section.start(), length);
}

int MemoryMappedOnDiskLog::flush(Offset offset)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(offset >= 0);
    BSLS_ASSERT_SAFE(offset <= d_currentOffset);

    if (!d_isOpened) {
        return LogOpResult::e_UNSUPPORTED_OPERATION;  // RETURN
    }

    // no-op
    if (d_currentOffset == 0) {
        return LogOpResult::e_SUCCESS;  // RETURN
    }

    if (offset == 0) {
        offset = d_currentOffset;
    }

    const int rc = ::msync(d_mfd.mapping(), offset, MS_SYNC);
    if (rc != 0) {
        return LogOpResult::e_FILE_MSYNC_FAILURE;  // RETURN
    }

    return LogOpResult::e_SUCCESS;
}

int MemoryMappedOnDiskLog::read(void* entry, int length, Offset offset) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(entry);

    int rc = validateRead(length, offset);
    if (rc != LogOpResult::e_SUCCESS) {
        return rc;  // RETURN
    }

    bsl::memcpy(entry, d_mfd.mapping() + offset, length);

    return LogOpResult::e_SUCCESS;
}

int MemoryMappedOnDiskLog::read(bdlbb::Blob* entry,
                                int          length,
                                Offset       offset) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(entry);

    int rc = validateRead(length, offset);
    if (rc != LogOpResult::e_SUCCESS) {
        return rc;  // RETURN
    }

    bdlbb::BlobUtil::append(entry, d_mfd.mapping(), offset, length);

    return LogOpResult::e_SUCCESS;
}

int MemoryMappedOnDiskLog::alias(void** entry, int length, Offset offset) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(entry);

    int rc = validateRead(length, offset);
    if (rc != LogOpResult::e_SUCCESS) {
        return rc;  // RETURN
    }

    *entry = d_mfd.mapping() + offset;

    return LogOpResult::e_SUCCESS;
}

int MemoryMappedOnDiskLog::alias(bdlbb::Blob* entry,
                                 int          length,
                                 Offset       offset) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(entry);

    int rc = validateRead(length, offset);
    if (rc != LogOpResult::e_SUCCESS) {
        return rc;  // RETURN
    }

    bsl::shared_ptr<char> entryBufferSp(d_mfd.mapping() + offset,
                                        bslstl::SharedPtrNilDeleter());
    bdlbb::BlobBuffer     entryBlobBuffer(entryBufferSp, length);
    entry->appendDataBuffer(entryBlobBuffer);

    return LogOpResult::e_SUCCESS;
}

}  // close package namespace
}  // close enterprise namespace
