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

// mqbsl_ledger.h                                                     -*-C++-*-
#ifndef INCLUDED_MQBSL_LEDGER
#define INCLUDED_MQBSL_LEDGER

//@PURPOSE: Provide a mechanism to store application-defined records.
//
//@CLASSES:
//  mqbsl::Ledger: Mechanism to store application-defined records.
//
//@DESCRIPTION: 'mqbsl::Ledger' is a mechanism to store application-defined
// records.  It owns multiple 'mqbsi::Log' instances, and records may span
// across these multiple instances.
//
/// Thread Safety
///-------------
// This component is *NOT* thread safe.

// MQB

#include <mqbsi_ledger.h>
#include <mqbsi_log.h>
#include <mqbu_storagekey.h>

#include <bmqc_orderedhashmap.h>
#include <bmqu_blob.h>

// BDE
#include <ball_log.h>
#include <bsl_memory.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_istriviallycopyable.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace mqbsl {

// ============
// class Ledger
// ============

/// Provide a mechanism to store application-defined records.  An object of
/// this class owns multiple `mqbsi::Log` instances, and records may span
/// across these multiple instances.
class Ledger BSLS_KEYWORD_FINAL : public mqbsi::Ledger {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBSL.LEDGER");

  public:
    // TYPES
    typedef mqbsi::Ledger::LogSp LogSp;
    typedef mqbsi::Ledger::Logs  Logs;

  private:
    // PRIVATE TYPES
    typedef mqbsi::LedgerRecordId LedgerRecordId;
    typedef mqbsi::LedgerOpResult LedgerOpResult;
    typedef mqbsi::LogOpResult    LogOpResult;
    typedef mqbsi::LogConfig      LogConfig;
    typedef mqbsi::Log            Log;
    typedef Log::Offset           Offset;

    typedef bmqc::OrderedHashMap<mqbu::StorageKey, LogSp> LogsMap;
    typedef LogsMap::iterator                             LogsMapIt;
    typedef LogsMap::const_iterator                       LogsMapCIt;

    struct LedgerState {
        enum Enum {
            // enum representing the state of the ledger
            e_OPENED = 0,
            e_CLOSED = 1
        };
    };

  private:
    // DATA
    bool d_isFirstOpen;  // Whether the ledger will be opened
                         // for the first time.

    bool d_isReadOnly;  // Whether the ledger is in read-only
                        // mode.

    mqbsi::LedgerConfig d_config;  // Configuration of this instance
                                   // provided upon construction

    bsls::Types::Int64 d_totalNumBytes;
    // Total number of bytes in the ledger

    bsls::Types::Int64 d_outstandingNumBytes;
    // Number of outstanding bytes in the
    // ledger. E.g.: Note that it is the
    // onus of the user to invoke
    // 'updateOutstandingNumBytes' properly
    // before or after overwriting an
    // existing record, since a 'write()'
    // operation will always increment this
    // value.

    bool d_supportsAliasing;
    // Flag indicating whether this ledger
    // supports aliasing

    LogsMap d_logs;  // Map of the logs held by this ledger,
                     // always in sync with the list of logs
                     // 'd_logList'

    Logs d_logList;  // List of the logs held by this
                     // ledger, always in sync with the map
                     // of logs 'd_logs'

    LedgerState::Enum d_state;  // State of this ledger (open, closed,
                                // etc.)

    bslma::Allocator* d_allocator_p;  // Allocator used to supply memory

  private:
    // PRIVATE MANIPULATORS

    /// When `open()` fails, this method can be invoked to reset the
    /// specified `state` to `LedgerState::e_CLOSED`, and to close all logs
    /// in the specified `logs`.
    static void openFailureCleanup(LedgerState::Enum* state, LogsMap* logs);

    /// Re-open the ledger in the mode according to the specified `flags`,
    /// and return 0 on success or a non-zero value otherwise.  Note that
    /// this method is only invoked when the ledger has been opened at least
    /// once before.
    int reopen(int flags);

    /// Create a new log using the specified `logId`, `logName`, and the
    /// internal ledger config.
    bsl::shared_ptr<mqbsi::Log> createLog(const mqbu::StorageKey& logId,
                                          const bsl::string&      logName);

    /// Insert the specified `log` to the list and map of logs maintained by
    /// this object and return true if successful and false otherwise (e.g.,
    /// a log with the same `logId` already exists).
    bool insert(const LogSp& log);

    /// Create a new log, add it to the ledger, and populate the specified
    /// `logPtr`.  Return true if was able to successfully add a new log,
    /// and false otherwise.
    bool addNew(LogSp* logPtr);

    /// Create a new log, add it to the ledger, populate the specified
    /// `logPtr`, and open the log.  Return 0 if successful and a negative
    /// `mqbsi::LedgerOpResult::Enum` value otherwise.
    int addNewAndOpen(LogSp* logPtr);

    /// Return a reference offering modifiable access to the current log
    /// being written to in this ledger.
    LogSp& currentLog();

    /// Implementation of rollover from the current log being written to (if
    /// any), identified by the specified `oldLogId`, to a new one.  Return 0
    /// on success, or a non-zero `mqbsi::LedgerOpResult::Enum` otherwise.  If
    /// successful, invoke the optional `OnRolloverCb` (found in the ledger
    /// config) when finished.
    int rollOverImpl(const mqbu::StorageKey& oldLogId);

    /// Roll over the current log being written to and return 0 on success, or
    /// a non-zero `mqbsi::LedgerOpResult` otherwise.  If successful, invoke
    /// the optional `OnRolloverCb` when finished.
    int rollOver();

    template <typename RECORD, typename OFFSET>
    int writeRecordImpl(LedgerRecordId* recordId,
                        const RECORD&   record,
                        OFFSET          offset,
                        int             length);

    /// Close and cleanup the specified `log` at the specified `logIndex`.
    /// Return 0 on success, and non-zero error code otherwise.
    ///
    /// THREAD: Scheduled to run in seperate thread.
    int closeAndCleanup(const LogSp& log, const size_t logIndex);

    // PRIVATE ACCESSORS

    /// Return true if the current log being written to can accommodate
    /// writing the specified `length` bytes in the current log of this
    /// ledger, and false if it cannot and this ledger needs to rollover to
    /// the next log in order to accommodate the desired write.
    bool canWrite(int length) const;

    /// Find the log identified by the specified `logId` and populate it
    /// into the specified `log`.  Return 0 if successful or a non-zero
    /// `mqbsi::LedgerOpResult::Enum` value otherwise.
    int find(Log** log, const mqbu::StorageKey& logId) const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Ledger, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `mqbsl::Ledger` object using the specified `config` and the
    /// specified `allocator` to supply memory.
    Ledger(const mqbsi::LedgerConfig& config, bslma::Allocator* allocator);

    /// Destructor
    ~Ledger() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual 'mqbsi::Ledger')

    /// Open the ledger in the mode according to the specified `flags`, and
    /// return 0 on success or a non-zero value otherwise.  The `flags` must
    /// include exactly zero or one of the following modes: e_READ_ONLY, or
    /// e_CREATE_IF_MISSING (setting both e_READ_ONLY and
    /// e_CREATE_IF_MISSING to true does not make sense).  If e_READ_ONLY is
    /// true, open the ledger in read-only mode.  If e_CREATE_IF_MISSING is
    /// true, create the ledger if it does not exist.  Else, return error if
    /// it does not exist.  Note that if e_READ_ONLY is true,
    /// `writeRecord()` will return failure.
    int open(int flags) BSLS_KEYWORD_OVERRIDE;

    /// Close the ledger, and return 0 on success or a non-zero value
    /// otherwise.
    int close() BSLS_KEYWORD_OVERRIDE;

    /// Increment the number of outstanding bytes in the log having the
    /// specified `logId` by the specified `value` (can be negative).
    /// Return 0 on success or a non-zero value otherwise.
    int
    updateOutstandingNumBytes(const mqbu::StorageKey& logId,
                              bsls::Types::Int64 value) BSLS_KEYWORD_OVERRIDE;

    /// Set the number of outstanding bytes in the log having the specified
    /// `logId` to the specified `value`.  Return 0 on success or a non-zero
    /// value otherwise.
    int setOutstandingNumBytes(const mqbu::StorageKey& logId,
                               bsls::Types::Int64 value) BSLS_KEYWORD_OVERRIDE;

    /// Write the specified `record` starting at the specified `offset`
    /// and of the specified `length` into this ledger and load into `recordId`
    /// an identifier which can be used to retrieve the record later.
    /// Return 0 on success, 1 on rollover success, and a non zero value
    /// otherwise.  The implementation must also adjust outstanding num bytes
    /// of the corresponding log.
    int writeRecord(LedgerRecordId* recordId,
                    const void*     record,
                    int             offset,
                    int             length) BSLS_KEYWORD_OVERRIDE;
    int writeRecord(LedgerRecordId*           recordId,
                    const bdlbb::Blob&        record,
                    const bmqu::BlobPosition& offset,
                    int                       length) BSLS_KEYWORD_OVERRIDE;

    /// Write the specified `section` of the specified `record` into this
    /// ledger and load into the specified `recordId` an identifier which
    /// can be used to retrieve the record later.  Return 0 on success, 1 on
    /// rollover success, and a non zero value otherwise.  The implementation
    /// must also adjust outstanding num bytes of the corresponding log.
    int writeRecord(LedgerRecordId*          recordId,
                    const bdlbb::Blob&       record,
                    const bmqu::BlobSection& section) BSLS_KEYWORD_OVERRIDE;

    /// Flush any cached data in this ledger to the underlying storing
    /// mechanism, and return 0 on success, or a non-zero value on error.
    int flush() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual 'mqbsi::Ledger')
    int readRecord(void*                 entry,
                   int                   length,
                   const LedgerRecordId& recordId) const BSLS_KEYWORD_OVERRIDE;

    /// Copy the specified `length` bytes from the specified `recordId` in
    /// this ledger into the specified `entry`, and return 0 on success, or
    /// a non-zero value on error.  Behavior is undefined unless `entry` has
    /// space for at least `length` bytes.
    int readRecord(bdlbb::Blob*          entry,
                   int                   length,
                   const LedgerRecordId& recordId) const BSLS_KEYWORD_OVERRIDE;

    int aliasRecord(void** entry, int length, const LedgerRecordId& recordId)
        const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `entry` a reference to the specified
    /// `length` bytes from the specified `recordId` in this ledger, and
    /// return 0 on success, or a non-zero value on error.  Behavior is
    /// undefined unless aliasing is supported.
    int
    aliasRecord(bdlbb::Blob*          entry,
                int                   length,
                const LedgerRecordId& recordId) const BSLS_KEYWORD_OVERRIDE;

    /// Return true if this ledger is opened, false otherwise.
    bool isOpened() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if this ledger supports aliasing, false otherwise.
    bool supportsAliasing() const BSLS_KEYWORD_OVERRIDE;

    /// Return the number of logs in this ledger.
    size_t numLogs() const BSLS_KEYWORD_OVERRIDE;

    /// Return the list of logs inside the ledger.  Note that the last log
    /// instance in the returned list is the "current" one (i.e., the one
    /// to which next record will be written).
    const Logs& logs() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference not offering modifiable access to the current log
    /// being written to in this ledger.
    const LogSp& currentLog() const BSLS_KEYWORD_OVERRIDE;

    /// If the optionally specified `logId` is null (`isNull()` returns
    /// true), return the number of outstanding bytes across all log
    /// instances in this ledger.  Else, return the number of outstanding
    /// bytes in the log instance identified by `logId`.  The behavior is
    /// undefined unless there is a log instance identified by `logId` in
    /// this ledger.
    bsls::Types::Int64
    outstandingNumBytes(const mqbu::StorageKey& logId =
                            mqbu::StorageKey()) const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Int64
    totalNumBytes(const mqbu::StorageKey& logId = mqbu::StorageKey()) const
        BSLS_KEYWORD_OVERRIDE;
    // If the optionally specified 'logId' is null ('isNull()' returns
    // true), return the number of bytes across all log instances that this
    // ledger.  Else, return the number of bytes in the log instance
    // identified by 'logId'.  The behavior is undefined unless there is a
    // log instance identified by 'logId' in this ledger.
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------
// class Ledger
// ------------

// PRIVATE MANIPULATORS
inline Ledger::LogSp& Ledger::currentLog()
{
    // Note that when the ledger is opened, it is guaranteed to have at least
    // one log.
    return (--d_logs.end())->second;
}

// ACCESSORS
inline bool Ledger::isOpened() const
{
    return d_state == LedgerState::e_OPENED;
}

inline bool Ledger::supportsAliasing() const
{
    return d_supportsAliasing;
}

inline size_t Ledger::numLogs() const
{
    return d_logs.size();
}

inline const Ledger::Logs& Ledger::logs() const
{
    return d_logList;
}

inline const Ledger::LogSp& Ledger::currentLog() const
{
    return const_cast<Ledger*>(this)->currentLog();
}

inline bsls::Types::Int64
Ledger::outstandingNumBytes(const mqbu::StorageKey& logId) const
{
    if (logId.isNull()) {
        return d_outstandingNumBytes;  // RETURN
    }

    LogsMap::const_iterator citer = d_logs.find(logId);
    BSLS_ASSERT_SAFE(citer != d_logs.end());

    return citer->second->outstandingNumBytes();
}

inline bsls::Types::Int64
Ledger::totalNumBytes(const mqbu::StorageKey& logId) const
{
    if (logId.isNull()) {
        return d_totalNumBytes;  // RETURN
    }

    LogsMap::const_iterator citer = d_logs.find(logId);
    BSLS_ASSERT_SAFE(citer != d_logs.end());

    return citer->second->totalNumBytes();
}

}  // close package namespace
}  // close enterprise namespace

#endif
