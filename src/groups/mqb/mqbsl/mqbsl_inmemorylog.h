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

// mqbsl_inmemorylog.h                                                -*-C++-*-
#ifndef INCLUDED_MQBSL_INMEMORYLOG
#define INCLUDED_MQBSL_INMEMORYLOG

//@PURPOSE: Provide a mechanism for reading and writing to an in-memory log.
//
//@CLASSES:
//  mqbsl::InMemoryLogFactory: Class used to create in-memory log instances
//  mqbsl::InMemoryLog:        Mechanism for a read-write in-memory log.
//
//@SEE_ALSO:
//  mqbsi::Log
//  mqbsi::LogFactory
//
//@DESCRIPTION: 'mqbsl::InMemoryLog' is a mechanism for reading and writing to
// an in-memory log.  For efficiency, it is intended to be used solely in an
// append-only fashion.  'mqbsl::InMemoryLogFactory' is a concrete
// implementation of the 'mqbsi::LogFactory' protocol used to create in-memory
// log instances.
//
/// Thread Safety
///-------------
// This component is *NOT* thread safe.

// MQB

#include <mqbsi_log.h>

// MWC
#include <mwcu_blob.h>

// BDE
#include <bsl_memory.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlbb {
class BlobBufferFactory;
}

namespace mqbsl {

// ========================
// class InMemoryLogFactory
// ========================

/// Factory used to create in-memory log instances.
class InMemoryLogFactory BSLS_KEYWORD_FINAL : public mqbsi::LogFactory {
  private:
    // DATA
    bslma::Allocator*         d_allocator_p;
    bdlbb::BlobBufferFactory* d_bufferFactory_p;

  private:
    // NOT IMPLEMENTED
    InMemoryLogFactory(const InMemoryLogFactory&) BSLS_KEYWORD_DELETED;
    InMemoryLogFactory&
    operator=(const InMemoryLogFactory&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Constructor of a `mqbsl::InMemoryLogFactory` object, using the
    /// specified `allocator` and `bufferFactory` to supply memory.
    InMemoryLogFactory(bslma::Allocator*         allocator,
                       bdlbb::BlobBufferFactory* bufferFactory);

    /// Destructor.
    virtual ~InMemoryLogFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Create a new log using the specified `config`.
    virtual bslma::ManagedPtr<mqbsi::Log>
    create(const mqbsi::LogConfig& config) BSLS_KEYWORD_OVERRIDE;
};

// =================
// class InMemoryLog
// =================

/// This class provides a mechanism for reading and writing to an in-memory
/// log.
class InMemoryLog BSLS_KEYWORD_FINAL : public mqbsi::Log {
  private:
    // PRIVATE TYPES
    typedef mqbsi::LogOpResult LogOpResult;
    typedef mqbsi::LogConfig   LogConfig;

    struct LogState {
        enum Enum {
            // enum representing the state of the log
            e_NON_EXISTENT     = 0,
            e_OPENED_READONLY  = 1,
            e_OPENED_READWRITE = 2,
            e_CLOSED           = 3
        };
    };

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use

    bool d_isOpened;
    // Whether the log is opened.

    bsls::Types::Int64 d_totalNumBytes;
    // Total number of bytes in the log

    bsls::Types::Int64 d_outstandingNumBytes;
    // Number of outstanding bytes in the log.
    // Note that it is the onus of the user to
    // invoke 'updateOutstandingNumBytes' properly
    // before overwriting an existing record, since
    // a 'write()' operation will always increment
    // this value by exactly the number of bytes
    // written, regardless of whether an existing
    // record is overwritten.

    Offset d_currentOffset;
    // Current offset of the log's internal write
    // position

    LogConfig d_config;
    // Config of the log

    LogState::Enum d_logState;
    // Current state of the log

    bsl::vector<bdlbb::Blob> d_records;
    // Vector of log records

    bdlbb::BlobBufferFactory* d_blobBufferFactory_p;
    // BlobBufferFactory to use, held not owned

  private:
    // NOT IMPLEMENTED
    InMemoryLog(const InMemoryLog&) BSLS_KEYWORD_DELETED;
    InMemoryLog& operator=(const InMemoryLog&) BSLS_KEYWORD_DELETED;

  private:
    // PRIVATE MANIPULATORS

    /// Write the specified `entry` into the log's internal write position.
    /// The number of outstanding bytes in the log will be incremented by
    /// the length of the `entry`.  Return the offset at which the `entry`
    /// was written on success, or a negative value on error.
    Offset writeImpl(const bdlbb::Blob& entry);

    // PRIVATE ACCESSORS

    /// Validate that the specified `length` and `offset` arguments for a
    /// `read()` or `alias()` operation are within bounds of the log.
    /// Return 0 on success or a negative value LogOpResult otherwise.
    int validateRead(int length, Offset offset) const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(InMemoryLog, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an instance of in-memory log having the specified `config`
    /// and using the specified `blobBufferFactory`.  Memory allocations are
    /// performed using the optionally specified `allocator`.
    InMemoryLog(const LogConfig&          config,
                bdlbb::BlobBufferFactory* blobBufferFactory,
                bslma::Allocator*         allocator);

    /// Destructor
    ~InMemoryLog() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Open the log in the mode according to the specified `flags`, and
    /// return 0 on success or a negative value LogOpResult otherwise.  The
    /// `flags` must include exactly zero or one of the following modes:
    /// e_READ_ONLY, or e_CREATE_IF_MISSING (setting both e_READ_ONLY and
    /// e_CREATE_IF_MISSING to true does not make sense).  If e_READ_ONLY is
    /// true, open the log in read-only mode.  If e_CREATE_IF_MISSING is
    /// true, create the log if it does not exist.  Else, return error if it
    /// does not exist.  Note that if e_READ_ONLY is true, `write()` and
    /// `seek()` operations will return failure.  As an additional
    /// guarantee, upon successful completion of `open()`, `currentOffset()`
    /// must point to the end of the log, while `totalNumBytes()` and
    /// `outstandingNumBytes()` must be equal to the size of the log.
    virtual int open(int flags) BSLS_KEYWORD_OVERRIDE;

    /// Close the log, and return 0 on success, or a negative value
    /// LogOpResult on error.
    virtual int close() BSLS_KEYWORD_OVERRIDE;

    /// Move the log's internal write position to the specified `offset`,
    /// and return 0 on success, or a negative value LogOpResult on error.
    /// Note that depending upon a log's implementation, repeatedly using
    /// `seek` to carry out random write operations may incur severe
    /// penalty.  Effort must be made to write sequentially to the log.
    /// Also note that it is the onus of the user of this component to
    /// update the number of outstanding bytes before seeking and
    /// overwriting existing bytes.
    virtual int seek(Offset offset) BSLS_KEYWORD_OVERRIDE;

    /// Increment the number of outstanding bytes in the log by the
    /// specified `value` (can be negative).
    virtual void
    updateOutstandingNumBytes(bsls::Types::Int64 value) BSLS_KEYWORD_OVERRIDE;

    /// Update the number of outstanding bytes in the log to the specified
    /// `value`.
    virtual void
    setOutstandingNumBytes(bsls::Types::Int64 value) BSLS_KEYWORD_OVERRIDE;

    virtual Offset
    write(const void* entry, int offset, int length) BSLS_KEYWORD_OVERRIDE;

    /// Write the specified `length` bytes starting at the specified
    /// `offset` of the specified `entry` into the log's internal write
    /// position.  Return the offset at which the `entry` was written on
    /// success, or a negative value LogOpResult on error.  Note the number
    /// of outstanding bytes in the log will be incremented by exactly
    /// `length` bytes, regardless of whether an existing record is
    /// overwritten.  Therefore, it is the onus of the user to invoke
    /// `updateOutstandingNumBytes` properly before overwriting an existing
    /// record.
    virtual Offset write(const bdlbb::Blob&        entry,
                         const mwcu::BlobPosition& offset,
                         int length) BSLS_KEYWORD_OVERRIDE;

    /// Write the specified `section` of the specified `entry` into the
    /// log's internal write position.   Return the offset at which the
    /// `entry` was written on success, or a negative value LogOpResult on
    /// error.  The number of outstanding bytes in the log will be
    /// incremented by exactly the number of bytes in the `section`,
    /// regardless of whether an existing record is overwritten.  Therefore,
    /// it is the onus of the user to invoke `updateOutstandingNumBytes`
    /// properly before overwriting an existing record.
    virtual Offset
    write(const bdlbb::Blob&       entry,
          const mwcu::BlobSection& section) BSLS_KEYWORD_OVERRIDE;

    /// Flush any cached data up to the optionally specified `offset` to the
    /// underlying storing mechanism, and return 0 on success, or a negative
    /// value `mqbsi::LogOpResult` on error.  If `offset` is not specified,
    /// all data is flushed.
    virtual int flush(Offset offset = 0) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    virtual int
    read(void* entry, int length, Offset offset) const BSLS_KEYWORD_OVERRIDE;

    /// Copy the specified `length` bytes starting at the specified `offset`
    /// of the log into the specified `entry`, and return 0 on success, or a
    /// negative value LogOpResult on error.  Behavior is undefined unless
    /// `entry` has space for at least `length` bytes.
    virtual int read(bdlbb::Blob* entry,
                     int          length,
                     Offset       offset) const BSLS_KEYWORD_OVERRIDE;

    virtual int
    alias(void** entry, int length, Offset offset) const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `entry a reference to the specified `length'
    /// bytes starting at the specified `offset` of the log, and return 0 on
    /// success, or a negative value LogOpResult on error.  Behavior is
    /// undefined unless aliasing is supported.
    virtual int alias(bdlbb::Blob* entry,
                      int          length,
                      Offset       offset) const BSLS_KEYWORD_OVERRIDE;

    /// Return true if this log is opened, false otherwise.
    virtual bool isOpened() const BSLS_KEYWORD_OVERRIDE;

    /// Return the total number of bytes in the log.
    virtual bsls::Types::Int64 totalNumBytes() const BSLS_KEYWORD_OVERRIDE;

    /// Return the number of outstanding bytes in the log.
    virtual bsls::Types::Int64
    outstandingNumBytes() const BSLS_KEYWORD_OVERRIDE;

    /// Return the current offset of the log's internal write position.
    virtual Offset currentOffset() const BSLS_KEYWORD_OVERRIDE;

    /// Return the config of the log.
    virtual const LogConfig& logConfig() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if the log supports aliasing, false otherwise.
    virtual bool supportsAliasing() const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------
// class InMemoryLog
// -----------------

// MANIPULATORS
inline void InMemoryLog::updateOutstandingNumBytes(bsls::Types::Int64 value)
{
    d_outstandingNumBytes += value;
}

inline void InMemoryLog::setOutstandingNumBytes(bsls::Types::Int64 value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(value >= 0);

    d_outstandingNumBytes = value;
}

// ACCESSORS
inline bool InMemoryLog::isOpened() const
{
    return d_isOpened;
}

inline bsls::Types::Int64 InMemoryLog::totalNumBytes() const
{
    return d_totalNumBytes;
}

inline bsls::Types::Int64 InMemoryLog::outstandingNumBytes() const
{
    return d_outstandingNumBytes;
}

inline mqbsi::Log::Offset InMemoryLog::currentOffset() const
{
    return d_currentOffset;
}

inline const mqbsi::LogConfig& InMemoryLog::logConfig() const
{
    return d_config;
}

inline bool InMemoryLog::supportsAliasing() const
{
    return true;
}

}  // close package namespace
}  // close enterprise namespace

#endif
