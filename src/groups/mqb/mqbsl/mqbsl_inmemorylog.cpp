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

// mqbsl_inmemorylog.cpp                                              -*-C++-*-
#include <mqbsl_inmemorylog.h>

#include <mqbscm_version.h>
// BDE
#include <bdlbb_blobutil.h>
#include <bslma_allocator.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbsl {

// ------------------------
// class InMemoryLogFactory
// ------------------------

// CREATORS
InMemoryLogFactory::InMemoryLogFactory(bslma::Allocator*         allocator,
                                       bdlbb::BlobBufferFactory* bufferFactory)
: d_allocator_p(allocator)
, d_bufferFactory_p(bufferFactory)
{
    // NOTHING
}

InMemoryLogFactory::~InMemoryLogFactory()
{
    // NOTHING
}

// MANIPULATORS
bslma::ManagedPtr<mqbsi::Log>
InMemoryLogFactory::create(const mqbsi::LogConfig& config)
{
    bslma::ManagedPtr<mqbsi::Log> log(
        new (*d_allocator_p)
            InMemoryLog(config, d_bufferFactory_p, d_allocator_p),
        d_allocator_p);
    return log;
}

// -----------------
// class InMemoryLog
// -----------------

mqbsi::Log::Offset InMemoryLog::writeImpl(const bdlbb::Blob& entry)
{
    if (d_logState != LogState::e_OPENED_READWRITE) {
        return LogOpResult::e_UNSUPPORTED_OPERATION;  // RETURN
    }

    const bool isAppend = static_cast<UnsignedOffset>(d_currentOffset) ==
                          d_records.size();

    const int                length = entry.length();
    const bsls::Types::Int64 numBytesDelta =
        isAppend ? length : length - d_records[d_currentOffset].length();
    if (d_totalNumBytes + numBytesDelta > d_config.maxSize()) {
        return LogOpResult::e_REACHED_END_OF_LOG;  // RETURN
    }

    if (isAppend) {
        d_records.push_back(entry);
    }
    else {
        d_records[d_currentOffset] = entry;
    }

    ++d_currentOffset;
    d_outstandingNumBytes += length;
    d_totalNumBytes += numBytesDelta;

    return d_currentOffset - 1;
}

int InMemoryLog::validateRead(int length, Offset offset) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(offset >= 0);
    BSLS_ASSERT_SAFE(length >= 0);

    if (d_logState != LogState::e_OPENED_READWRITE &&
        d_logState != LogState::e_OPENED_READONLY) {
        return LogOpResult::e_UNSUPPORTED_OPERATION;  // RETURN
    }

    if (static_cast<UnsignedOffset>(offset) > d_records.size()) {
        return LogOpResult::e_OFFSET_OUT_OF_RANGE;  // RETURN
    }

    if (length > d_records[offset].length()) {
        return LogOpResult::e_REACHED_END_OF_RECORD;  // RETURN
    }

    return LogOpResult::e_SUCCESS;
}

InMemoryLog::InMemoryLog(const LogConfig&          config,
                         bdlbb::BlobBufferFactory* blobBufferFactory,
                         bslma::Allocator*         allocator)
: d_allocator_p(allocator)
, d_isOpened(false)
, d_totalNumBytes(0)
, d_outstandingNumBytes(0)
, d_currentOffset(0)
, d_config(config)
, d_logState(LogState::e_NON_EXISTENT)
, d_records(allocator)
, d_blobBufferFactory_p(blobBufferFactory)
{
    // NOTHING
}

InMemoryLog::~InMemoryLog()
{
    // NOTHING
}

int InMemoryLog::open(int flags)
{
    // Note that the e_CREATE_IF_MISSING flag is ignored for an in-memory log.

    if (d_logState == LogState::e_OPENED_READONLY ||
        d_logState == LogState::e_OPENED_READWRITE) {
        return LogOpResult::e_LOG_ALREADY_OPENED;  // RETURN
    }

    d_logState = (flags & e_READ_ONLY) ? LogState::e_OPENED_READONLY
                                       : LogState::e_OPENED_READWRITE;

    // Re-calibrate outstandingNumBytes and currentOffset
    d_outstandingNumBytes = d_totalNumBytes;
    d_currentOffset       = d_records.size();

    d_isOpened = true;
    return LogOpResult::e_SUCCESS;
}

int InMemoryLog::close()
{
    if (d_logState == LogState::e_NON_EXISTENT) {
        return LogOpResult::e_FILE_NOT_EXIST;  // RETURN
    }

    if (d_logState == LogState::e_CLOSED) {
        return LogOpResult::e_LOG_ALREADY_CLOSED;  // RETURN
    }

    d_logState = LogState::e_CLOSED;

    d_isOpened = false;
    return LogOpResult::e_SUCCESS;
}

int InMemoryLog::seek(Offset offset)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(offset >= 0);

    if (d_logState != LogState::e_OPENED_READWRITE) {
        return LogOpResult::e_UNSUPPORTED_OPERATION;  // RETURN
    }

    if (static_cast<UnsignedOffset>(offset) > d_records.size()) {
        return LogOpResult::e_OFFSET_OUT_OF_RANGE;  // RETURN
    }

    d_currentOffset = offset;

    return LogOpResult::e_SUCCESS;
}

mqbsi::Log::Offset
InMemoryLog::write(const void* entry, int offset, int length)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(entry);
    BSLS_ASSERT_SAFE(offset >= 0);
    BSLS_ASSERT_SAFE(length >= 0);

    bdlbb::Blob blob(d_blobBufferFactory_p, d_allocator_p);
    bdlbb::BlobUtil::append(&blob,
                            static_cast<const char*>(entry),
                            offset,
                            length);

    return writeImpl(blob);
}

mqbsi::Log::Offset InMemoryLog::write(const bdlbb::Blob&        entry,
                                      const mwcu::BlobPosition& offset,
                                      int                       length)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(length >= 0);

    bdlbb::Blob blob(d_blobBufferFactory_p, d_allocator_p);
    mwcu::BlobUtil::appendBlobFromIndex(&blob,
                                        entry,
                                        offset.buffer(),
                                        offset.byte(),
                                        length);

    return writeImpl(blob);
}

mqbsi::Log::Offset InMemoryLog::write(const bdlbb::Blob&       entry,
                                      const mwcu::BlobSection& section)
{
    int length;
    int rc = mwcu::BlobUtil::sectionSize(&length, entry, section);
    if (rc != 0) {
        return LogOpResult::e_INVALID_BLOB_SECTION;  // RETURN
    }

    return write(entry, section.start(), length);
}

int InMemoryLog::flush(BSLS_ANNOTATION_UNUSED Offset offset)
{
    return LogOpResult::e_SUCCESS;
}

int InMemoryLog::read(void* entry, int length, Offset offset) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(entry);

    int rc = validateRead(length, offset);
    if (rc != LogOpResult::e_SUCCESS) {
        return rc;  // RETURN
    }

    mwcu::BlobUtil::readNBytes(static_cast<char*>(entry),
                               d_records[offset],
                               mwcu::BlobPosition(),
                               length);

    return LogOpResult::e_SUCCESS;
}

int InMemoryLog::read(bdlbb::Blob* entry, int length, Offset offset) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(entry);

    int rc = validateRead(length, offset);
    if (rc != LogOpResult::e_SUCCESS) {
        return rc;  // RETURN
    }

    bdlbb::BlobUtil::copy(entry, 0, d_records[offset], 0, length);

    return LogOpResult::e_SUCCESS;
}

int InMemoryLog::alias(BSLS_ANNOTATION_UNUSED void** entry,
                       BSLS_ANNOTATION_UNUSED int    length,
                       BSLS_ANNOTATION_UNUSED Offset offset) const
{
    return LogOpResult::e_UNSUPPORTED_OPERATION;
}

int InMemoryLog::alias(bdlbb::Blob* entry, int length, Offset offset) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(entry);

    int rc = validateRead(length, offset);
    if (rc != LogOpResult::e_SUCCESS) {
        return rc;  // RETURN
    }

    bdlbb::BlobUtil::append(entry, d_records[offset], 0, length);

    return LogOpResult::e_SUCCESS;
}

}  // close package namespace
}  // close enterprise namespace
