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

// mqbsi_log.h                                                        -*-C++-*-
#ifndef INCLUDED_MQBSI_LOG
#define INCLUDED_MQBSI_LOG

//@PURPOSE: Provide an interface for reading and writing log records.
//
//@CLASSES:
//  mqbsi::LogOpResult:    Result code of a log operation.
//  mqbsi::LogFactory:     Factory to create log instances
//  mqbsi::LogConfig:      VST representing the configuration of a log.
//  mqbsi::LogIdGenerator: Generator of unique log IDs
//  mqbsi::Log:            Interface for reading and writing log records.
//
//@DESCRIPTION: 'mqbsi::Log' is an interface for reading and writing log
// records.  For efficiency, it is intended to be used solely in an append-only
// fashion.
//
/// Thread Safety
///-------------
// Components implementing the 'mqbsi::Log' interface are *NOT* required to be
// thread safe.

// MQB

#include <mqbu_storagekey.h>

// MWC
#include <mwcu_blob.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbsi {

class LogConfig;
class Log;

// ================
// class LogFactory
// ================

/// Factory used to create log instances.
class LogFactory {
  public:
    // CREATORS

    /// Destructor
    virtual ~LogFactory();

    // MANIPULATORS

    /// Create a new log using the specified `config`.
    virtual bslma::ManagedPtr<Log> create(const LogConfig& config) = 0;
};

// ==================
// struct LogOpResult
// ==================

/// This enum represents the result of a log operation.
struct LogOpResult {
    // TYPES
    enum Enum {
        // Generic
        // - - - -
        e_SUCCESS = 0
        // Operation was successful

        /// Unknown result
        ,
        e_UNKNOWN = -1

        /// Operation is *not* supported for this type of log
        ,
        e_UNSUPPORTED_OPERATION = -2

        /// Log was already opened
        ,
        e_LOG_ALREADY_OPENED = -3

        /// Log was already closed
        ,
        e_LOG_ALREADY_CLOSED = -4

        /// Read-only contract is violated
        ,
        e_LOG_READONLY = -5

        // File specific, only relevant to on-disk logs
        // - - - - - - - - - - - - - - - - - - - - - -

        /// File does not exist
        ,
        e_FILE_NOT_EXIST = -6

        /// Error when opening the file
        ,
        e_FILE_OPEN_FAILURE = -7

        /// Error when closing the file
        ,
        e_FILE_CLOSE_FAILURE = -8

        /// Error when growing the size of the file
        ,
        e_FILE_GROW_FAILURE = -9

        /// Error when truncating the file
        ,
        e_FILE_TRUNCATE_FAILURE = -10

        /// Error when seeking in the file
        ,
        e_FILE_SEEK_FAILURE = -11

        /// Error when flushing file changes to disk
        ,
        e_FILE_FLUSH_FAILURE = -12

        /// Error when synchronizing the memory map back to the filesystem
        ,
        e_FILE_MSYNC_FAILURE = -13

        // Range checks for log, record, and blob
        // - - - - - - - - - - - - - - - - - - -

        /// The offset input argument is out of range of valid offsets
        ,
        e_OFFSET_OUT_OF_RANGE = -14

        /// Operation would result in the log exceeding the configured
        /// maximum size
        ,
        e_REACHED_END_OF_LOG = -15

        /// Operation would exceed the end of the record
        ,
        e_REACHED_END_OF_RECORD = -16

        /// An invalid blob section was provided
        ,
        e_INVALID_BLOB_SECTION = -17

        // Byte read/write
        // - - - - - - - -

        /// Failed to read bytes from the log
        ,
        e_BYTE_READ_FAILURE = -18

        /// Failed to write bytes into the log
        ,
        e_BYTE_WRITE_FAILURE = -19
    };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// `LogOpResult::Enum` value.
    static bsl::ostream& print(bsl::ostream&     stream,
                               LogOpResult::Enum value,
                               int               level          = 0,
                               int               spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(LogOpResult::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(LogOpResult::Enum*       out,
                          const bslstl::StringRef& str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, LogOpResult::Enum value);

// ===============
// class LogConfig
// ===============

/// This class provides a VST representing the configuration of a log.
class LogConfig {
  private:
    // DATA
    bsls::Types::Int64 d_maxSize;  // Maximum size, in bytes, of the log

    mqbu::StorageKey d_logId;  // Log ID Fields relevant only to
                               // on-disk logs

    bsl::string d_location;  // Location of the log (full path,
                             // including log file name)

    bool d_reserveOnDisk;  // Whether to reserve the size on disk
                           // when the underlying file
                           // representing the log is grown upon
                           // opening

    bool d_prefaultPages;  // Whether to prefault page tables for
                           // a memory mapping of the underlying
                           // file representing the log

  public:
    // CREATORS

    /// Create a new `LogConfig` with the specified `maxSize` and `logId`
    /// values and the optionally specified `location`, `reserveOnDisk`, and
    /// `prefaultPages` (which are relevant only to on-disk logs).  Use the
    /// specified `allocator` to supply memory.
    LogConfig(bsls::Types::Int64      maxSize,
              const mqbu::StorageKey& logId,
              bslma::Allocator*       allocator);
    LogConfig(bsls::Types::Int64       maxSize,
              const mqbu::StorageKey&  logId,
              const bslstl::StringRef& location,
              bool                     reserveOnDisk,
              bool                     prefaultPages,
              bslma::Allocator*        allocator);

    // MANIPULATORS
    LogConfig& setMaxSize(bsls::Types::Int64 value);
    LogConfig& setLogId(const mqbu::StorageKey& value);
    LogConfig& setLocation(const bslstl::StringRef& value);
    LogConfig& setReserveOnDisk(bool value);

    /// Set the corresponding attribute to the specified `value` and return
    /// a reference offering modifiable access to this object.
    LogConfig& setPrefaultPages(bool value);

    // ACCESSORS
    bsls::Types::Int64      maxSize() const;
    const mqbu::StorageKey& logId() const;
    const bsl::string&      location() const;
    bool                    reserveOnDisk() const;

    /// Get the value of the corresponding attribute.
    bool prefaultPages() const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const LogConfig& rhs);

/// Return `true` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return false otherwise.
bool operator==(const LogConfig& lhs, const LogConfig& rhs);

// ====================
// class LogIdGenerator
// ====================

/// Generator of unique log IDs.
class LogIdGenerator {
  public:
    // CREATORS

    /// Destructor
    virtual ~LogIdGenerator();

    /// Register the specified `logId` among those generated by this object
    /// and return `true` if successful, `false` otherwise (e.g., `logId` is
    /// already registered).  The effect of this is that `logId` will not be
    /// returned by a future call to `generateLogId(...)`.
    virtual bool registerLogId(const mqbu::StorageKey& logId) = 0;

    /// Create a new log name and a new unique log ID that has not before
    /// been generated or registered by this object and load them into the
    /// specified `logName` and `logId`.
    virtual void generateLogId(bsl::string*      logName,
                               mqbu::StorageKey* logId) = 0;
};

// =========
// class Log
// =========

/// This class provides an interface for reading and writing log records.
class Log {
  public:
    // TYPES
    typedef bsls::Types::Int64  Offset;
    typedef bsls::Types::Uint64 UnsignedOffset;

    enum Enum {
        e_READ_ONLY = (1 << 0)  // Whether the log is read-only
        ,
        e_CREATE_IF_MISSING = (1 << 1)  // Whether to create the log when
                                        // opening if it does not exist
    };

    // CLASS DATA
    static const Offset k_INVALID_OFFSET = -1;  // Offset value representing
                                                // no offset, used as the
                                                // default and error value

  public:
    // CREATORS

    /// Destructor
    virtual ~Log();

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
    virtual int open(int flags) = 0;

    /// Close the log, and return 0 on success, or a negative value
    /// LogOpResult on error.
    virtual int close() = 0;

    /// Move the log's internal write position to the specified `offset`,
    /// and return 0 on success, or a negative value LogOpResult on error.
    /// Note that depending upon a log's implementation, repeatedly using
    /// `seek` to carry out random write operations may incur severe
    /// penalty.  Effort must be made to write sequentially to the log.
    /// Also note that it is the onus of the user of this component to
    /// update the number of outstanding bytes before seeking and
    /// overwriting existing bytes.
    virtual int seek(Offset offset) = 0;

    /// Increment the number of outstanding bytes in the log by the
    /// specified `value` (can be negative).
    virtual void updateOutstandingNumBytes(bsls::Types::Int64 value) = 0;

    /// Update the number of outstanding bytes in the log to the specified
    /// `value`.
    virtual void setOutstandingNumBytes(bsls::Types::Int64 value) = 0;

    /// Write the specified `length` bytes starting at the specified
    /// `offset` of the specified `entry` into the log's internal write
    /// position.  Return the offset at which the `entry` was written on
    /// success, or a negative value LogOpResult on error.  Note the number
    /// of outstanding bytes in the log will be incremented by exactly
    /// `length` bytes, regardless of whether an existing record is
    /// overwritten.  Therefore, it is the onus of the user to invoke
    /// `updateOutstandingNumBytes` properly before overwriting an existing
    /// record.
    virtual Offset write(const void* entry, int offset, int length) = 0;
    virtual Offset write(const bdlbb::Blob&        entry,
                         const mwcu::BlobPosition& offset,
                         int                       length)                                = 0;

    /// Write the specified `section` of the specified `entry` into the
    /// log's internal write position.   Return the offset at which the
    /// `entry` was written on success, or a negative value LogOpResult on
    /// error.  The number of outstanding bytes in the log will be
    /// incremented by exactly the number of bytes in the `section`,
    /// regardless of whether an existing record is overwritten.  Therefore,
    /// it is the onus of the user to invoke `updateOutstandingNumBytes`
    /// properly before overwriting an existing record.
    virtual Offset write(const bdlbb::Blob&       entry,
                         const mwcu::BlobSection& section) = 0;

    /// Flush any cached data up to the optionally specified `offset` to the
    /// underlying storing mechanism, and return 0 on success, or a negative
    /// value `LogOpResult` on error.  If `offset` is not specified, all
    /// data is flushed.
    virtual int flush(Offset offset = 0) = 0;

    // ACCESSORS

    /// Copy the specified `length` bytes starting at the specified `offset`
    /// of the log into the specified `entry`, and return 0 on success, or a
    /// negative value LogOpResult on error.  Behavior is undefined unless
    /// `entry` has space for at least `length` bytes.
    virtual int read(void* entry, int length, Offset offset) const        = 0;
    virtual int read(bdlbb::Blob* entry, int length, Offset offset) const = 0;

    /// Load into the specified `entry a reference to the specified `length'
    /// bytes starting at the specified `offset` of the log, and return 0 on
    /// success, or a negative value LogOpResult on error.  Behavior is
    /// undefined unless aliasing is supported.
    virtual int alias(void** entry, int length, Offset offset) const       = 0;
    virtual int alias(bdlbb::Blob* entry, int length, Offset offset) const = 0;

    /// Return true if this log is opened, false otherwise.
    virtual bool isOpened() const = 0;

    /// Return the total number of bytes in the log.
    virtual bsls::Types::Int64 totalNumBytes() const = 0;

    /// Return the number of outstanding bytes in the log.
    virtual bsls::Types::Int64 outstandingNumBytes() const = 0;

    /// Return the current offset of the log's internal write position.
    virtual Offset currentOffset() const = 0;

    /// Return the config of the log.
    virtual const LogConfig& logConfig() const = 0;

    /// Return true if the log supports aliasing, false otherwise.
    virtual bool supportsAliasing() const = 0;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------
// class LogConfig
// ---------------

// CREATORS
inline LogConfig::LogConfig(bsls::Types::Int64      maxSize,
                            const mqbu::StorageKey& logId,
                            bslma::Allocator*       allocator)
: d_maxSize(maxSize)
, d_logId(logId)
, d_location("", allocator)
, d_reserveOnDisk(false)
, d_prefaultPages(false)
{
    // NOTHING
}

inline LogConfig::LogConfig(bsls::Types::Int64       maxSize,
                            const mqbu::StorageKey&  logId,
                            const bslstl::StringRef& location,
                            bool                     reserveOnDisk,
                            bool                     prefaultPages,
                            bslma::Allocator*        allocator)
: d_maxSize(maxSize)
, d_logId(logId)
, d_location(location, allocator)
, d_reserveOnDisk(reserveOnDisk)
, d_prefaultPages(prefaultPages)
{
    // NOTHING
}

// MANIPULATORS
inline LogConfig& LogConfig::setMaxSize(bsls::Types::Int64 value)
{
    d_maxSize = value;
    return *this;
}

inline LogConfig& LogConfig::setLogId(const mqbu::StorageKey& value)
{
    d_logId = value;
    return *this;
}

inline LogConfig& LogConfig::setLocation(const bslstl::StringRef& value)
{
    d_location = value;
    return *this;
}

inline LogConfig& LogConfig::setReserveOnDisk(bool value)
{
    d_reserveOnDisk = value;
    return *this;
}

inline LogConfig& LogConfig::setPrefaultPages(bool value)
{
    d_prefaultPages = value;
    return *this;
}

// ACCESSORS
inline bsls::Types::Int64 LogConfig::maxSize() const
{
    return d_maxSize;
}

inline const mqbu::StorageKey& LogConfig::logId() const
{
    return d_logId;
}

inline const bsl::string& LogConfig::location() const
{
    return d_location;
}

inline bool LogConfig::reserveOnDisk() const
{
    return d_reserveOnDisk;
}

inline bool LogConfig::prefaultPages() const
{
    return d_prefaultPages;
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& mqbsi::operator<<(bsl::ostream&     stream,
                                       LogOpResult::Enum value)
{
    return LogOpResult::print(stream, value, 0, -1);
}

inline bsl::ostream& mqbsi::operator<<(bsl::ostream&    stream,
                                       const LogConfig& rhs)
{
    return rhs.print(stream, 0, -1);
}

inline bool mqbsi::operator==(const LogConfig& lhs, const LogConfig& rhs)
{
    return lhs.maxSize() == rhs.maxSize() && lhs.logId() == rhs.logId() &&
           lhs.location() == rhs.location() &&
           lhs.reserveOnDisk() == rhs.reserveOnDisk() &&
           lhs.prefaultPages() == rhs.prefaultPages();
}

}  // close enterprise namespace

#endif
