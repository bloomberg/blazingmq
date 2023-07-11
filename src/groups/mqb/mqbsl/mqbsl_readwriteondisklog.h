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

// mqbsl_readwriteondisklog.h                                         -*-C++-*-
#ifndef INCLUDED_MQBSL_READWRITEONDISKLOG
#define INCLUDED_MQBSL_READWRITEONDISKLOG

//@PURPOSE: Implements an on-disk log using read() and write() syscalls.
//
//@CLASSES:
//  mqbsl::ReadWriteOnDiskLogFactory: Factory to create read-write on-disk logs
//  mqbsl::ReadWriteOnDiskLog:        On-disk log using read/write syscalls.
//
//@SEE_ALSO:
//  mqbsi::Log
//  mqbsi::LogFactory
//  mqbsl::OnDiskLog
//
//@DESCRIPTION: 'mqbsl::ReadWriteOnDiskLog' is an implementation of an on-disk
// log using read() and write() syscalls.  For efficiency, it is intended to be
// used solely in an append-only fashion.  'mqbsl::ReadWriteOnDiskLogFactory'
// is a concrete implementation of the 'mqbsi::LogFactory' protocol used to
// create read-write on-disk logs.
//
/// Thread Safety
///-------------
// This component is *NOT* thread safe.

// MQB

#include <mqbsi_log.h>
#include <mqbsl_ondisklog.h>

// MWC
#include <mwcu_blob.h>

// BDE
#include <bdlbb_blob.h>
#include <bslma_allocator.h>
#include <bsls_keyword.h>
#include <bsls_types.h>

// SYS
#include <sys/uio.h>

namespace BloombergLP {

namespace mqbsl {

// ===============================
// class ReadWriteOnDiskLogFactory
// ===============================

/// Factory used to create read-write on-disk log instances.
class ReadWriteOnDiskLogFactory BSLS_KEYWORD_FINAL : public mqbsi::LogFactory {
  private:
    // DATA
    bslma::Allocator* d_allocator_p;

  private:
    // NOT IMPLEMENTED
    ReadWriteOnDiskLogFactory(const ReadWriteOnDiskLogFactory&)
        BSLS_KEYWORD_DELETED;
    ReadWriteOnDiskLogFactory&
    operator=(const ReadWriteOnDiskLogFactory&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Constructor of a `mqbsl::ReadWriteOnDiskLogFactory` object, using
    /// the specified `allocator` supply memory.
    explicit ReadWriteOnDiskLogFactory(bslma::Allocator* allocator);

    /// Destructor.
    virtual ~ReadWriteOnDiskLogFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Create a new log using the specified `config`.
    virtual bslma::ManagedPtr<mqbsi::Log>
    create(const mqbsi::LogConfig& config) BSLS_KEYWORD_OVERRIDE;
};

// ========================
// class ReadWriteOnDiskLog
// ========================

/// This class implements an on-disk log using read() and write() syscalls.
class ReadWriteOnDiskLog BSLS_KEYWORD_FINAL : public OnDiskLog {
  private:
    // PRIVATE TYPES
    typedef mqbsi::LogOpResult LogOpResult;
    typedef mqbsi::LogConfig   LogConfig;

    // CLASS DATA
    static const int k_INVALID_FD;  // File descriptor value representing no
                                    // file, used as the error return for
                                    // 'open'.

  private:
    // DATA
    bool d_isOpened;
    // Whether the log is opened.

    bool d_isReadOnly;
    // Whether the log is in read-only mode.

    bsls::Types::Int64 d_totalNumBytes;
    // Total number of bytes in the log.

    bsls::Types::Int64 d_outstandingNumBytes;
    // Number of outstanding bytes in the log.  Note
    // that it is the onus of the user to invoke
    // 'updateOutstandingNumBytes' properly before
    // overwriting an existing record, since a
    // 'write()' operation will always increment
    // this value by exactly the number of bytes
    // written, regardless of whether an existing
    // record is overwritten.

    Offset d_currentOffset;
    // Current offset of the log's internal write
    // position.

    mqbsi::LogConfig d_config;
    // Config of this on-disk log.

    int d_fd;
    // File descriptor to the underlying file
    // storing the log.

  private:
    // NOT IMPLEMENTED
    ReadWriteOnDiskLog(const ReadWriteOnDiskLog&) BSLS_KEYWORD_DELETED;
    ReadWriteOnDiskLog&
    operator=(const ReadWriteOnDiskLog&) BSLS_KEYWORD_DELETED;

  private:
    // PRIVATE MANIPULATORS

    /// Repositions the file offset of the underlying file descriptor to the
    /// specified `offset`, and return 0 on success, or a non-zero value on
    /// error.
    int seekImpl(Offset offset) const;

    /// Validate that the specified `length` and `offset` arguments for a
    /// `read()` or `alias()` operation are within bounds of the log.
    /// Return 0 on success or a negative value LogOpResult otherwise.
    int validateRead(int length, Offset offset) const;

    /// Increment the log's internal write position and outstanding bytes by
    /// the specified `writeLength` and update the total number of bytes in
    /// the log if it has grown to a new max.
    void updateInternalState(int writeLength);

    /// Populate the specified `ioVectors` with the specified `length` bytes
    /// starting at the specified `offset` of the specified `entry`, in
    /// preparation for a subsequent `writev` syscall on the `ioVectors`.
    /// This function returns either when all `length` bytes have been
    /// populated or when the size of `ioVectors` reaches IOV_MAX, since
    /// writev() supports only up to IOV_MAX iovecs.  The specified
    /// `iovecCount` will be populated with the number of entries in
    /// `ioVectors` which has been modified by this function.
    void populateIoVectors(struct iovec*             ioVectors,
                           int*                      iovecCount,
                           const bdlbb::Blob&        entry,
                           const mwcu::BlobPosition& offset,
                           int                       length);

  public:
    // CREATORS

    /// Create a `mqbsl::ReadWriteOnDiskLog` using the specified `config`.
    explicit ReadWriteOnDiskLog(const mqbsi::LogConfig& config);

    /// Destructor
    ~ReadWriteOnDiskLog() BSLS_KEYWORD_OVERRIDE;

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

    /// Return the config of this on-disk log.
    virtual const mqbsi::LogConfig& config() const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------------
// class ReadWriteOnDiskLog
// ------------------------

// MANIPULATORS
inline void
ReadWriteOnDiskLog::updateOutstandingNumBytes(bsls::Types::Int64 value)
{
    d_outstandingNumBytes += value;
}

inline void
ReadWriteOnDiskLog::setOutstandingNumBytes(bsls::Types::Int64 value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(value >= 0);

    d_outstandingNumBytes = value;
}

// ACCESSORS
inline bool ReadWriteOnDiskLog::isOpened() const
{
    return d_isOpened;
}

inline bsls::Types::Int64 ReadWriteOnDiskLog::totalNumBytes() const
{
    return d_totalNumBytes;
}

inline bsls::Types::Int64 ReadWriteOnDiskLog::outstandingNumBytes() const
{
    return d_outstandingNumBytes;
}

inline mqbsi::Log::Offset ReadWriteOnDiskLog::currentOffset() const
{
    return d_currentOffset;
}

inline const mqbsi::LogConfig& ReadWriteOnDiskLog::logConfig() const
{
    return d_config;
}

inline bool ReadWriteOnDiskLog::supportsAliasing() const
{
    return false;
}

inline const mqbsi::LogConfig& ReadWriteOnDiskLog::config() const
{
    return d_config;
}

}  // close package namespace
}  // close enterprise namespace

#endif
