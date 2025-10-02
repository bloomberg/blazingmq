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

// mqbsi_ledger.h                                                     -*-C++-*-
#ifndef INCLUDED_MQBSI_LEDGER
#define INCLUDED_MQBSI_LEDGER

//@PURPOSE: Provide an interface to store application-defined records.
//
//@CLASSES:
//  mqbsi::LedgerRecordId: ID which uniquely identifies a ledger record.
//  mqbsi::LedgerOpResult: Result code of a ledger operation.
//  mqbsi::LedgerConfig:   VST representing the configuration of a ledger.
//  mqbsi::Ledger:         Interface to store application-defined records.
//
//@DESCRIPTION: 'mqbsi::Ledger' is an interface to store application-defined
// records.  A 'Ledger' implemenation may own multiple `Log` instances, and
// records may span across these multiple instances.
//
/// Thread Safety
///-------------
// Components implementing the 'mqbsi::Ledger' interface are *NOT* required to
// be thread safe.

// MQB

#include <mqbsi_log.h>
#include <mqbu_storagekey.h>

#include <bmqu_blob.h>

// BDE
#include <bdlb_nullablevalue.h>
#include <bdlbb_blob.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_type_traits.h>
#include <bsl_vector.h>
#include <bslh_hash.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_istriviallycopyable.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATIONS
namespace bdlmt {
class EventScheduler;
}

namespace mqbsi {

// =====================
// struct LedgerOpResult
// =====================

/// This enum represents the result of a ledger operation.
struct LedgerOpResult {
    // TYPES
    enum Enum {
        // Generic
        // - - - -
        e_SUCCESS = 0  // Operation was successful
        ,
        e_UNKNOWN                 = -1,
        e_LEDGER_NOT_EXIST        = -2,
        e_LEDGER_UNGRACEFUL_CLOSE = -3

        // File specific
        // - - - - - - - - - - - - - - - - - - - - - -
        ,
        e_LEDGER_READ_ONLY        = -6,
        e_LOG_CREATE_FAILURE      = -7,
        e_LOG_OPEN_FAILURE        = -8,
        e_LOG_CLOSE_FAILURE       = -9,
        e_LOG_FLUSH_FAILURE       = -10,
        e_LOG_ROLLOVER_CB_FAILURE = -11,
        e_LOG_CLEANUP_FAILURE     = -12,
        e_LOG_NOT_FOUND           = -13,
        e_LOG_INVALID             = -14,
        e_RECORD_WRITE_FAILURE    = -15,
        e_RECORD_READ_FAILURE     = -16,
        e_RECORD_ALIAS_FAILURE    = -17,
        e_ALIAS_NOT_SUPPORTED     = -18,
        e_INVALID_BLOB_SECTION    = -19
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
    /// `LedgerOpResult::Enum` value.
    static bsl::ostream& print(bsl::ostream&        stream,
                               LedgerOpResult::Enum value,
                               int                  level          = 0,
                               int                  spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(LedgerOpResult::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(LedgerOpResult::Enum*    out,
                          const bslstl::StringRef& str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, LedgerOpResult::Enum value);

// ====================
// class LedgerRecordId
// ====================

/// An ID which uniquely identifies a record in the ledger.  Note that this
/// type is not opaque to higher components, and they can also create a
/// valid `LedgerRecordId` instance given a logId and offset.
class LedgerRecordId {
  private:
    // DATA
    mqbu::StorageKey d_logId;
    // The log in which the entry exists in.

    Log::Offset d_offset;
    // The offset in the log at which the entry exists in.

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(LedgerRecordId,
                                   bslmf::IsBitwiseEqualityComparable)
    BSLMF_NESTED_TRAIT_DECLARATION(LedgerRecordId, bsl::is_trivially_copyable)

    // CREATORS

    /// Create a `LedgerRecordId` object with null logId and invalid offset.
    LedgerRecordId();

    /// Create a `LedgerRecordId` object having the specified `logId` and
    /// `offset`.
    LedgerRecordId(const mqbu::StorageKey& logId, Log::Offset offset);

    // MANIPULATORS
    LedgerRecordId& setLogId(const mqbu::StorageKey& value);

    /// Set the corresponding attribute to the specified `value` and return
    /// a reference offering modifiable access to this object.
    LedgerRecordId& setOffset(Log::Offset value);

    // ACCESSORS
    const mqbu::StorageKey& logId() const;

    /// Get the value of the corresponding attribute.
    Log::Offset offset() const;

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

// FREE FUNCTIONS

/// Apply the specified `hashAlgo` to the specified `key`.
template <class HASH_ALGORITHM>
void hashAppend(HASH_ALGORITHM& hashAlgo, const LedgerRecordId& key);

// FREE OPERATORS

/// Return `true` if the specified `lhs` object contains the value of the
/// same type as contained in the specified `rhs` object and the value
/// itself is the same in both objects, return false otherwise.
bool operator==(const LedgerRecordId& lhs, const LedgerRecordId& rhs);

/// Return `false` if `rhs` object contains the value of the same type as
/// contained in `lhs` object and the value itself is the same in both
/// objects, return `true` otherwise.
bool operator!=(const LedgerRecordId& lhs, const LedgerRecordId& rhs);

/// Operator used to allow comparison between the specified `lhs` and `rhs`
/// `LedgerRecordId` objects so that `LedgerRecordId` can be used as key in
/// a map.  Note that criteria for comparison is implementation defined, and
/// result of this routine does not in any way signify the order of creation
/// of records in the ledgers to which `lhs` and `rhs` instances belong.
bool operator<(const LedgerRecordId& lhs, const LedgerRecordId& rhs);

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const LedgerRecordId& value);

// ==================
// class LedgerConfig
// ==================

/// This class provides a VST representing the configuration of a ledger.
class LedgerConfig {
  public:
    // TYPES

    /// Callback invoked to load the ID of an existing log located at the
    /// specified `logPath` into the specfied `logId`, before opening the
    /// log.  Return 0 on success or a non-zero error value otherwise.
    typedef bsl::function<int(mqbu::StorageKey*  logId,
                              const bsl::string& logPath)>
        ExtractLogIdCb;

    /// Callback invoked to validate the specified `log` before beginning to
    /// use the log in earnest and populate the specified `offset` with the
    /// current offset of the log.  Return 0 on success and non-zero error
    /// value otherwise.
    typedef bsl::function<int(Log::Offset*                       offset,
                              const bsl::shared_ptr<mqbsi::Log>& log)>
        ValidateLogCb;

    /// Callback invoked upon internal rollover to a new log, providing the
    /// specified `oldLogId` (if any) and `newLogId`.  Return 0 on success
    /// and non-zero error value otherwise.
    typedef bsl::function<int(const mqbu::StorageKey& oldLogId,
                              const mqbu::StorageKey& newLogId)>
        OnRolloverCb;

    /// Callback invoked to perform cleanup after closing (or failing to
    /// open/validate) a log, providing the specified `logPath` location of
    /// the log.  Return 0 on success and non-zero error value otherwise.
    /// Note that if the `keepOldLogs` flag is false, this callback will be
    /// invoked by the ledger after it closes the old log during rollover.
    typedef bsl::function<int(const bsl::string& logPath)> CleanupCb;

  private:
    // DATA
    bsl::string d_location;
    // Location of files

    bsl::string d_pattern;
    // String representing the pattern for
    // files included in the ledger.

    bsls::Types::Int64 d_maxLogSize;
    // Maximum size, in bytes, of a log in
    // this ledger.

    bool d_reserveOnDisk;
    // Flag indicating whether files should
    // be preallocated/reserved on disk.

    bool d_prefaultPages;
    // Flag indicating whether to populate
    // (prefault) page tables for a
    // mapping.

    bool d_keepOldLogs;
    // Flag indicating whether to open old
    // logs and keep old logs open when
    // rolling over to a new log.

    bsl::shared_ptr<mqbsi::LogFactory> d_logFactory_sp;
    // Pointer to log factory used to
    // create logs.

    bsl::shared_ptr<mqbsi::LogIdGenerator> d_logIdGenerator_sp;
    // Pointer to generator of log ids.

    bdlmt::EventScheduler* d_scheduler_p;

    ExtractLogIdCb d_extractLogIdCallback;
    // Callback invoked to extract the ID
    // of a log.

    ValidateLogCb d_validateLogCallback;
    // Callback invoked to validate a log.

    OnRolloverCb d_rolloverCallback;
    // Callback invoked upon internal
    // rollover to a new log.

    CleanupCb d_cleanupCallback;
    // Callback invoked after closing a log
    // to perform additional cleanup.

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(LedgerConfig, bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Create a new `LedgerConfig` object.  Use the specified `allocator`
    /// to supply memory.
    explicit LedgerConfig(bslma::Allocator* allocator);

    /// Copy constructor from the specified `other` using the specified
    /// `allocator` to supply memory.
    LedgerConfig(const LedgerConfig& other, bslma::Allocator* allocator);

    // MANIPULATORS
    LedgerConfig& setLocation(const bsl::string& value);
    LedgerConfig& setPattern(const bsl::string& value);
    LedgerConfig& setMaxLogSize(const bsls::Types::Int64 value);
    LedgerConfig& setReserveOnDisk(bool value);
    LedgerConfig& setPrefaultPages(bool value);
    LedgerConfig& setKeepOldLogs(bool value);
    LedgerConfig&
    setLogFactory(const bsl::shared_ptr<mqbsi::LogFactory>& value);
    LedgerConfig&
    setLogIdGenerator(const bsl::shared_ptr<mqbsi::LogIdGenerator>& value);
    LedgerConfig& setScheduler(bdlmt::EventScheduler* value);
    LedgerConfig& setExtractLogIdCallback(const ExtractLogIdCb& value);
    LedgerConfig& setValidateLogCallback(const ValidateLogCb& value);
    LedgerConfig& setRolloverCallback(const OnRolloverCb& value);

    /// Set the corresponding attribute to the specified `value` and return
    /// a reference offering modifiable access to this object.
    LedgerConfig& setCleanupCallback(const CleanupCb& value);

    // ACCESSORS
    const bsl::string&     location() const;
    const bsl::string&     pattern() const;
    bsls::Types::Int64     maxLogSize() const;
    bool                   reserveOnDisk() const;
    bool                   prefaultPages() const;
    bool                   keepOldLogs() const;
    mqbsi::LogFactory*     logFactory() const;
    mqbsi::LogIdGenerator* logIdGenerator() const;
    bdlmt::EventScheduler* scheduler() const;
    const ExtractLogIdCb&  extractLogIdCallback() const;
    const ValidateLogCb&   validateLogCallback() const;
    const OnRolloverCb&    rolloverCallback() const;

    /// Get the value of the corresponding attribute.
    const CleanupCb& cleanupCallback() const;

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
bsl::ostream& operator<<(bsl::ostream& stream, const LedgerConfig& rhs);

/// Return `true` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return false otherwise.
bool operator==(const LedgerConfig& lhs, const LedgerConfig& rhs);

// ============
// class Ledger
// ============

/// This interface provides a way to store application-defined records.  A
/// `Ledger` implemenation may own multiple `Log` instances, and records may
/// span across these multiple instances.
class Ledger {
  public:
    typedef bsl::shared_ptr<Log> LogSp;
    typedef bsl::vector<LogSp>   Logs;

    enum Enum {
        e_READ_ONLY = (1 << 0)  // Whether the ledger is read-only
        ,
        e_CREATE_IF_MISSING = (1 << 1)  // Whether to create the ledger when
                                        // opening if it does not exist
    };

  public:
    // CREATORS

    /// Destructor
    virtual ~Ledger();

    // MANIPULATORS

    /// Open the ledger in the mode according to the specified `flags`, and
    /// return 0 on success or a non-zero value otherwise.  The `flags` must
    /// include exactly zero or one of the following modes: e_READ_ONLY, or
    /// e_CREATE_IF_MISSING (setting both e_READ_ONLY and
    /// e_CREATE_IF_MISSING to true does not make sense).  If e_READ_ONLY is
    /// true, open the ledger in read-only mode.  If e_CREATE_IF_MISSING is
    /// true, create the ledger if it does not exist.  Else, return error if
    /// it does not exist.  Note that if e_READ_ONLY is true,
    /// `writeRecord()` will return failure.
    virtual int open(int flags) = 0;

    /// Close the ledger, and return 0 on success or a non-zero value
    /// otherwise.
    virtual int close() = 0;

    /// Increment the number of outstanding bytes in the log having the
    /// specified `logId` by the specified `value` (can be negative).
    /// Return 0 on success or a non-zero value otherwise.
    virtual int updateOutstandingNumBytes(const mqbu::StorageKey& logId,
                                          bsls::Types::Int64      value) = 0;

    /// Set the number of outstanding bytes in the log having the specified
    /// `logId` to the specified `value`.  Return 0 on success or a non-zero
    /// value otherwise.
    virtual int setOutstandingNumBytes(const mqbu::StorageKey& logId,
                                       bsls::Types::Int64      value) = 0;

    /// Write the specified `record` starting at the specified `offset` and of
    /// the specified `length` into this ledger and load into `recordId` an
    /// identifier which can be used to retrieve the record later.  Return 0 on
    /// success and a non zero value otherwise.  The implementation must also
    /// adjust outstanding num bytes of the corresponding log.
    virtual int writeRecord(LedgerRecordId* recordId,
                            const void*     record,
                            int             offset,
                            int             length) = 0;

    virtual int writeRecord(LedgerRecordId*           recordId,
                            const bdlbb::Blob&        record,
                            const bmqu::BlobPosition& offset,
                            int                       length) = 0;

    /// Write the specified `section` of the specified `record` into this
    /// ledger and load into `recordId` an identifier which can be used to
    /// retrieve the record later.  Return 0 on success and a descriptive
    /// `mqbsi::LedgerOpResult` error code value otherwise, and accordingly
    /// adjust the total outstanding number of bytes if successful.
    virtual int writeRecord(LedgerRecordId*          recordId,
                            const bdlbb::Blob&       record,
                            const bmqu::BlobSection& section) = 0;

    /// Flush any cached data in this ledger to the underlying storage
    /// mechanism, and return 0 on success, or a non-zero value on error.
    virtual int flush() = 0;

    // ACCESSORS

    /// Copy the specified `length` bytes from the specified `recordId` in
    /// this ledger into the specified `entry`, and return 0 on success, or
    /// a non-zero value on error.  Behavior is undefined unless `entry` has
    /// space for at least `length` bytes.
    virtual int readRecord(void*                 entry,
                           int                   length,
                           const LedgerRecordId& recordId) const = 0;
    virtual int readRecord(bdlbb::Blob*          entry,
                           int                   length,
                           const LedgerRecordId& recordId) const = 0;

    /// Load into the specified `entry` a reference to the specified
    /// `length` bytes from the specified `recordId` in this ledger, and
    /// return 0 on success, or a non-zero value on error.  Behavior is
    /// undefined unless aliasing is supported.
    virtual int aliasRecord(void**                entry,
                            int                   length,
                            const LedgerRecordId& recordId) const = 0;
    virtual int aliasRecord(bdlbb::Blob*          entry,
                            int                   length,
                            const LedgerRecordId& recordId) const = 0;

    /// Return true if this ledger is opened, false otherwise.
    virtual bool isOpened() const = 0;

    /// Return true if this ledger supports aliasing, false otherwise.
    virtual bool supportsAliasing() const = 0;

    /// Return the number of logs in this ledger.
    virtual size_t numLogs() const = 0;

    /// Return the list of logs inside the ledger.  Note that the last log
    /// instance in the returned list is the "current" one (i.e., the one
    /// to which next record will be written).
    virtual const Logs& logs() const = 0;

    /// Return a reference not offering modifiable access to the current log
    /// being written to in this ledger.
    virtual const LogSp& currentLog() const = 0;

    /// If the optionally specified `logId` is null (`isNull()` returns
    /// true), return the number of outstanding bytes across all log
    /// instances in this ledger.  Else, return the number of outstanding
    /// bytes in the log instance identified by `logId`.
    virtual bsls::Types::Int64 outstandingNumBytes(
        const mqbu::StorageKey& logId = mqbu::StorageKey()) const = 0;

    /// If the optionally specified `logId` is null (`isNull()` returns
    /// true), return the number of bytes across all log instances that this
    /// ledger.  Else, return the number of bytes in the log instance
    /// identified by `logId`.
    virtual bsls::Types::Int64 totalNumBytes(
        const mqbu::StorageKey& logId = mqbu::StorageKey()) const = 0;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------
// class LedgerRecordId
// --------------------

}  // close package namespace

// FREE FUNCTIONS
template <class HASH_ALGORITHM>
inline void hashAppend(HASH_ALGORITHM&              hashAlgo,
                       const mqbsi::LedgerRecordId& key)
{
    using bslh::hashAppend;  // for ADL
    hashAppend(hashAlgo, key.logId());
    hashAppend(hashAlgo, key.offset());
}

// FREE OPERATORS
inline bool mqbsi::operator==(const mqbsi::LedgerRecordId& lhs,
                              const mqbsi::LedgerRecordId& rhs)
{
    return lhs.logId() == rhs.logId() && lhs.offset() == rhs.offset();
}

inline bool mqbsi::operator!=(const mqbsi::LedgerRecordId& lhs,
                              const mqbsi::LedgerRecordId& rhs)
{
    return !(lhs == rhs);
}

inline bool mqbsi::operator<(const mqbsi::LedgerRecordId& lhs,
                             const mqbsi::LedgerRecordId& rhs)
{
    if (lhs.logId() < rhs.logId()) {
        return true;  // RETURN
    }
    if (lhs.logId() == rhs.logId()) {
        if (lhs.offset() < rhs.offset()) {
            return true;  // RETURN
        }
    }

    return false;
}

inline bsl::ostream& mqbsi::operator<<(bsl::ostream&                stream,
                                       const mqbsi::LedgerRecordId& value)
{
    return value.print(stream, 0, -1);
}

// ---------------------
// struct LedgerOpResult
// ---------------------

inline bsl::ostream& mqbsi::operator<<(bsl::ostream&               stream,
                                       mqbsi::LedgerOpResult::Enum value)
{
    return mqbsi::LedgerOpResult::print(stream, value, 0, -1);
}

// ------------------
// class LedgerConfig
// ------------------

inline bsl::ostream& mqbsi::operator<<(bsl::ostream&              stream,
                                       const mqbsi::LedgerConfig& value)
{
    return value.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
