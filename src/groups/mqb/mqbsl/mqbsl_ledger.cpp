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

// mqbsl_ledger.cpp                                                   -*-C++-*-
#include <mqbsl_ledger.h>

#include <mqbscm_version.h>
// MQB
#include <mqbsi_ledger.h>

// BMQ
#include <bmqsys_time.h>
#include <bmqtsk_alarmlog.h>
#include <bmqu_printutil.h>

// BDE
#include <ball_log.h>
#include <bdlb_scopeexit.h>
#include <bdlf_bind.h>
#include <bdls_filesystemutil.h>
#include <bdls_pathutil.h>
#include <bdlt_datetime.h>
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bsla_annotations.h>
#include <bslmf_allocatorargt.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbsl {

namespace {

const char k_LOG_CATEGORY[] = "MQBSL.LEDGER";

/// Populate the specified `logFlags` with the appropriate log flags base on
/// the specified `ledgerFlags`.
void populateLogFlags(int* logFlags, int ledgerFlags)
{
    if (ledgerFlags & Ledger::e_READ_ONLY) {
        *logFlags |= mqbsi::Log::e_READ_ONLY;
    }
    else if (ledgerFlags & Ledger::e_CREATE_IF_MISSING) {
        *logFlags |= mqbsi::Log::e_CREATE_IF_MISSING;
    }
}

// ===================================
// struct FileLastModificationTimeLess
// ===================================

/// This struct provides a binary function for comparing two files by their
/// last modification time.
struct FileLastModificationTimeLess {
    /// Return `true` if the specified `lhs` file has an older modification
    /// time than the specified `rhs` file, and `false` otherwise.
    bool operator()(const bsl::string& lhs, const bsl::string& rhs) const;
};

// -----------------------------------
// struct FileLastModificationTimeLess
// -----------------------------------

bool FileLastModificationTimeLess::operator()(const bsl::string& lhs,
                                              const bsl::string& rhs) const
{
    BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);

    bdlt::Datetime lhsTime, rhsTime;

    int rc = bdls::FilesystemUtil::getLastModificationTime(&lhsTime, lhs);
    if (rc != 0) {
        BALL_LOG_WARN << "Failed retrieving last modification time of '" << lhs
                      << "' [rc: " << rc << "]";
        return true;  // RETURN
    }

    rc = bdls::FilesystemUtil::getLastModificationTime(&rhsTime, rhs);
    if (rc != 0) {
        BALL_LOG_WARN << "Failed retrieving last modification time of '" << rhs
                      << "' [rc: " << rc << "]";
        return false;  // RETURN
    }

    return lhsTime < rhsTime;
}

}  // close anonymous namespace

// ------------
// class Ledger
// ------------

// PRIVATE MANIPULATORS
void Ledger::openFailureCleanup(LedgerState::Enum* state, LogsMap* logs)
{
    *state = LedgerState::e_CLOSED;

    for (LogsMapIt it = logs->begin(); it != logs->end(); ++it) {
        if (it->second->isOpened()) {
            int rc = it->second->close();
            if (rc != LedgerOpResult::e_SUCCESS) {
                BALL_LOG_ERROR
                    << "Failed to close the log located at '"
                    << it->second->logConfig().location() << "' with logId = '"
                    << it->second->logConfig().logId() << "' [rc: " << rc
                    << "]";
            }
        }
    }
}

int Ledger::reopen(int flags)
{
    d_outstandingNumBytes = 0;

    int logFlags = 0;
    populateLogFlags(&logFlags, flags);

    int rc = LedgerOpResult::e_UNKNOWN;
    for (LogsMapIt it = d_logs.begin(); it != d_logs.end(); ++it) {
        rc = it->second->open(logFlags);
        if (rc != LogOpResult::e_SUCCESS) {
            return rc * 100 + LedgerOpResult::e_LOG_OPEN_FAILURE;  // RETURN
        }
        d_outstandingNumBytes += it->second->outstandingNumBytes();
    }

    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(d_outstandingNumBytes == d_totalNumBytes);

    d_state      = LedgerState::e_OPENED;
    d_isReadOnly = flags & e_READ_ONLY;
    return LogOpResult::e_SUCCESS;
}

bsl::shared_ptr<mqbsi::Log> Ledger::createLog(const mqbu::StorageKey& logId,
                                              const bsl::string&      logName)
{
    mqbsi::LogConfig logConfig(d_config.maxLogSize(), logId, d_allocator_p);

    // Populate fields relevant to on-disk logs, just in case (this object has
    // no awareness of the exact type of the log being created)
    bsl::string fullPath(d_config.location());
    bdls::PathUtil::appendRaw(&fullPath, logName.c_str());

    logConfig.setLocation(fullPath)
        .setReserveOnDisk(d_config.reserveOnDisk())
        .setPrefaultPages(d_config.prefaultPages());

    // Create a new log from the config and return it
    return bsl::shared_ptr<mqbsi::Log>(
        d_config.logFactory()->create(logConfig),
        d_allocator_p);
}

bool Ledger::insert(const bsl::shared_ptr<mqbsi::Log>& logSp)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(logSp);
    BSLS_ASSERT_SAFE(d_logs.size() == d_logList.size());

    const mqbu::StorageKey& logId = logSp->logConfig().logId();
    if (d_logs.find(logId) != d_logs.end()) {
        // Log already exists
        return false;  // RETURN
    }

    d_logList.push_back(logSp);
    d_logs.insert(bsl::make_pair(logId, logSp));
    return true;
}

bool Ledger::addNew(LogSp* logSp)
{
    // Generate log ID and name
    bsl::string      logName;
    mqbu::StorageKey logId;
    d_config.logIdGenerator()->generateLogId(&logName, &logId);
    if (d_logs.find(logId) != d_logs.end()) {
        // logId already exists in this map
        return false;  // RETURN
    }

    // Create log config from ledger config, log ID, and log name
    *logSp = createLog(logId, logName);

    return insert(*logSp);
}

int Ledger::addNewAndOpen(LogSp* logSp)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(logSp);

    if (!addNew(logSp)) {
        return LedgerOpResult::e_LOG_CREATE_FAILURE;  // RETURN
    }
    BSLS_ASSERT_SAFE((*logSp) == currentLog());

    int rc = (*logSp)->open(mqbsi::Log::e_CREATE_IF_MISSING);
    if (rc != mqbsi::LogOpResult::e_SUCCESS) {
        return rc * 100 + LedgerOpResult::e_LOG_OPEN_FAILURE;  // RETURN
    }
    BSLS_ASSERT_SAFE((*logSp)->totalNumBytes() == 0);
    BSLS_ASSERT_SAFE((*logSp)->currentOffset() == 0);

    return LedgerOpResult::e_SUCCESS;
}

// PRIVATE ACCESSORS
bool Ledger::canWrite(int length) const
{
    const bool isEmpty        = d_logs.empty();
    const bool willExceedSize = currentLog()->currentOffset() + length >
                                currentLog()->logConfig().maxSize();
    return !(isEmpty || willExceedSize);
}

int Ledger::find(Log** logPtr, const mqbu::StorageKey& logId) const
{
    LogsMapCIt cit = d_logs.find(logId);
    if (cit == d_logs.end()) {
        return LedgerOpResult::e_LOG_NOT_FOUND;  // RETURN
    }

    *logPtr = cit->second.get();

    return LedgerOpResult::e_SUCCESS;
}

int Ledger::rollOverImpl(const mqbu::StorageKey& oldLogId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_isReadOnly);

    LogSp newLog;
    int   rc = addNewAndOpen(&newLog);
    if (rc != 0) {
        return rc;  // RETURN
    }

    // Invoke rollover callback
    const mqbu::StorageKey& newLogId = currentLog()->logConfig().logId();
    rc = d_config.rolloverCallback()(oldLogId, newLogId);
    if (rc != 0) {
        BMQTSK_ALARMLOG_ALARM("ROLLOVER")
            << "Rollover callback from log '" << oldLogId << "' to log '"
            << newLogId << "' failed, rc: " << rc
            << ". Aborting rollover and reverting back to old log."
            << BMQTSK_ALARMLOG_END;

        closeAndCleanup(currentLog(), d_logs.size() - 1);

        return rc * 100 + LedgerOpResult::e_LOG_ROLLOVER_CB_FAILURE;  // RETURN
    }

    return LedgerOpResult::e_SUCCESS;
}

int Ledger::rollOver()
{
    LogSp& lastLog = currentLog();
    const size_t lastLogIndex = d_logs.size() - 1;

    // Flush the log and roll over
    int rc = lastLog->flush();
    if (rc != LogOpResult::e_SUCCESS) {
        return rc * 100 + LedgerOpResult::e_LOG_FLUSH_FAILURE;  // RETURN
    }

    rc = rollOverImpl(lastLog->logConfig().logId());
    if (rc != LedgerOpResult::e_SUCCESS) {
        return rc;  // RETURN
    }

    // If not keeping old logs, enqueue an event in the scheduler thread
    // to be executed right away to close and cleanup the log file.
    if (!d_config.keepOldLogs()) {
        d_config.scheduler()->scheduleEvent(
            bmqsys::Time::nowMonotonicClock(),
            bdlf::BindUtil::bind(&Ledger::closeAndCleanup,
                                 this,
                                 lastLog,
                                 lastLogIndex));
    }

    return LedgerOpResult::e_SUCCESS;
}

template <typename RECORD, typename OFFSET>
int Ledger::writeRecordImpl(LedgerRecordId* recordId,
                            const RECORD&   record,
                            OFFSET          offset,
                            int             length)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state == LedgerState::e_OPENED);
    BSLS_ASSERT_SAFE(!d_logs.empty());

    if (d_isReadOnly) {
        return LedgerOpResult::e_LEDGER_READ_ONLY;  // RETURN
    }

    bsls::Types::Int64 oldNumBytes = currentLog()->totalNumBytes();
    if (!canWrite(length)) {
        // If current log is full, create new log to hold the record.
        const int rc = rollOver();
        if (rc != LedgerOpResult::e_SUCCESS) {
            return rc;  // RETURN
        }

        // Reset local state
        oldNumBytes = 0;
    }

    // Write and update internal state
    LogSp&       currLog      = currentLog();
    const Offset recordOffset = currLog->write(record, offset, length);
    BSLS_ASSERT_SAFE(recordOffset != LogOpResult::e_REACHED_END_OF_LOG);
    if (recordOffset < 0) {
        return 100 * recordOffset +
               LedgerOpResult::e_RECORD_WRITE_FAILURE;  // RETURN
    }

    mqbu::StorageKey logId = currLog->logConfig().logId();
    recordId->setLogId(logId).setOffset(recordOffset);

    d_outstandingNumBytes += length;
    d_totalNumBytes += currLog->totalNumBytes() - oldNumBytes;

    return LedgerOpResult::e_SUCCESS;
}

int Ledger::closeAndCleanup(const LogSp& log, const size_t logIndex)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_logs.size() == d_logList.size());
    BSLS_ASSERT_SAFE(d_logs.size() > 0);

    if (d_logs.size() == 1) {
        BALL_LOG_ERROR << "Cannot erase latest log '"
                       << log->logConfig().logId()
                       << "'  because there is only one log.";
        return LedgerOpResult::e_LOG_CLEANUP_FAILURE;  // RETURN
    }

    const bsls::Types::Int64 start   = bmqsys::Time::highResolutionTimer();
    const bsl::string&       logPath = log->logConfig().location();

    LogsMapCIt cit = d_logs.find(log->logConfig().logId());
    BSLS_ASSERT_SAFE(cit != d_logs.end());
    d_totalNumBytes -= cit->second->totalNumBytes();
    d_logs.erase(cit);
    d_logList.erase(d_logList.begin() + logIndex);

    int rc = log->close();
    if (rc != LogOpResult::e_SUCCESS) {
        BALL_LOG_ERROR << "Failed to close the log " << logPath
                       << ", rc: " << rc;
        return rc * 100 + LedgerOpResult::e_LOG_CLOSE_FAILURE;  // RETURN
    }

    rc = d_config.cleanupCallback()(logPath);
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to clean up the log " << logPath
                       << ", rc: " << rc;
        return rc * 100 + LedgerOpResult::e_LOG_CLEANUP_FAILURE;  // RETURN
    }

    const bsls::Types::Int64 end = bmqsys::Time::highResolutionTimer();

    BALL_LOG_INFO << "Log '" << log->logConfig().logId()
                  << "' closed and cleaned up. Time taken: "
                  << bmqu::PrintUtil::prettyTimeInterval(end - start);

    return LedgerOpResult::e_SUCCESS;
}

// CREATORS
Ledger::Ledger(const mqbsi::LedgerConfig& config, bslma::Allocator* allocator)
: d_isFirstOpen(true)
, d_isReadOnly(false)
, d_config(config, allocator)
, d_totalNumBytes(0)
, d_outstandingNumBytes(0)
, d_supportsAliasing(false)
, d_logs(allocator)
, d_logList(allocator)
, d_state(LedgerState::e_CLOSED)
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(config.logFactory());
    BSLS_ASSERT_SAFE(config.logIdGenerator());
    BSLS_ASSERT_SAFE(config.validateLogCallback());
    BSLS_ASSERT_SAFE(config.rolloverCallback());
    BSLS_ASSERT_SAFE(config.cleanupCallback());
}

Ledger::~Ledger()
{
    // NOTHING
}

// MANIPULATORS
//   (virtual 'mqbsi::Ledger')
int Ledger::open(int flags)
{
    if (d_state == LedgerState::e_OPENED) {
        return LedgerOpResult::e_LEDGER_ALREADY_OPENED;  // RETURN
    }
    BSLS_ASSERT_SAFE(d_state == LedgerState::e_CLOSED);

    if (!d_isFirstOpen) {
        return reopen(flags);  // RETURN
    }

    d_state = LedgerState::e_OPENED;
    bdlb::ScopeExitAny guard(
        bdlf::BindUtil::bind(openFailureCleanup, &d_state, &d_logs));

    // Look for files matching the location and pattern specified in the config
    bsl::string pattern;
    pattern.append(d_config.location());
    if (pattern[pattern.length() - 1] != '/') {
        pattern.append(1, '/');
    }
    pattern.append(d_config.pattern());

    bsl::vector<bsl::string> files;
    bdls::FilesystemUtil::findMatchingPaths(&files, pattern.c_str());
    if (files.empty()) {
        if (!(flags & e_CREATE_IF_MISSING)) {
            return LedgerOpResult::e_LEDGER_NOT_EXIST;  // RETURN
        }

        // There are no matching files to open, create the initial log and
        // "rollover" to it
        int rc = rollOverImpl(mqbu::StorageKey());
        if (rc != 0) {
            BALL_LOG_WARN << "Failed creating the initial log [rc: " << rc
                          << "]";
            return rc;  // RETURN
        }

        d_supportsAliasing = currentLog()->supportsAliasing();
        guard.release();
        return LedgerOpResult::e_SUCCESS;  // RETURN
    }

    // Sort the files by their last modification time (in order of earliest to
    // latest) and keep only the ones that will be opened
    bsl::sort(files.begin(), files.end(), FileLastModificationTimeLess());

    if (!d_config.keepOldLogs()) {
        // Keep only the file with most recent last modification time
        files.erase(files.begin(), files.end() - 1);
    }

    // Create logs for selected files, open and insert them into internal data
    // structures
    for (bsl::vector<bsl::string>::size_type i = 0; i < files.size(); ++i) {
        const bsl::string& logFullPath = files[i];

        // Extract the logId and parse logName
        mqbu::StorageKey logId;
        int rc = d_config.extractLogIdCallback()(&logId, logFullPath);
        if (rc != 0) {
            BALL_LOG_ERROR << "Failed to extract the logID of '" << logFullPath
                           << "' [rc: " << rc << "]";
            rc = d_config.cleanupCallback()(logFullPath);
            if (rc != 0) {
                BALL_LOG_ERROR << "Failed to clean up the log '" << logFullPath
                               << "' after failing to extract "
                               << "its logID. [rc: " << rc << "]";
            }
            return LedgerOpResult::e_LOG_OPEN_FAILURE;  // RETURN
        }
        d_config.logIdGenerator()->registerLogId(logId);

        bsl::string logName;
        rc = bdls::PathUtil::getLeaf(&logName, logFullPath);
        if (rc != 0) {
            BALL_LOG_ERROR << "Failed to extract the logName of '"
                           << logFullPath << "' [rc: " << rc << "]";
            return LedgerOpResult::e_LOG_OPEN_FAILURE;  // RETURN
        }

        // Create new log, open and validate it
        bsl::shared_ptr<mqbsi::Log> log = createLog(logId, logName);

        int logFlags = 0;
        populateLogFlags(&logFlags, flags);

        rc = log->open(logFlags);
        if (rc != mqbsi::LogOpResult::e_SUCCESS) {
            BALL_LOG_ERROR << "Failed to open the log located at '"
                           << logFullPath << "' with logId = '" << logId
                           << "' [rc: " << rc << "]";
            return rc * 100 + LedgerOpResult::e_LOG_OPEN_FAILURE;  // RETURN
        }

        mqbsi::Log::Offset currOffset = 0;
        rc = d_config.validateLogCallback()(&currOffset, log);
        if (rc != mqbsi::LogOpResult::e_SUCCESS) {
            BALL_LOG_ERROR << "Log located at '" << logFullPath
                           << "' with logId = '" << logId << "' is invalid. "
                           << "[rc: " << rc << "]";
            return rc * 100 + LedgerOpResult::e_LOG_INVALID;  // RETURN
        }

        rc = log->seek(currOffset);
        if (rc != 0) {
            BALL_LOG_ERROR << "Failed to seek in the log located at '"
                           << logFullPath << "' to offset " << currOffset
                           << ". [rc: " << rc << "]";
        }

        // Insert the new log
        bool wasInserted = insert(log);
        BSLS_ASSERT_OPT(wasInserted);

        d_totalNumBytes += log->totalNumBytes();
        d_outstandingNumBytes += log->outstandingNumBytes();
    }

    if (d_logs.empty()) {
        return LedgerOpResult::e_LOG_OPEN_FAILURE;  // RETURN
    }

    d_supportsAliasing = currentLog()->supportsAliasing();
    d_isFirstOpen      = false;
    d_isReadOnly       = flags & e_READ_ONLY;
    guard.release();
    return LedgerOpResult::e_SUCCESS;
}

int Ledger::close()
{
    if (d_state == LedgerState::e_CLOSED) {
        return LedgerOpResult::e_LEDGER_ALREADY_CLOSED;  // RETURN
    }

    int rc = LedgerOpResult::e_UNKNOWN;
    for (LogsMapIt it = d_logs.begin(); it != d_logs.end(); ++it) {
        if (it->second->isOpened()) {
            rc = it->second->close();
            if (rc != LogOpResult::e_SUCCESS) {
                return rc * 100 + LedgerOpResult::e_LEDGER_UNGRACEFUL_CLOSE;
                // RETURN
            }
        }
    }

    d_state      = LedgerState::e_CLOSED;
    d_isReadOnly = false;
    return LedgerOpResult::e_SUCCESS;
}

int Ledger::updateOutstandingNumBytes(const mqbu::StorageKey& logId,
                                      bsls::Types::Int64      value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state == LedgerState::e_OPENED);

    Log* log = 0;
    int  rc  = find(&log, logId);
    if (rc != LedgerOpResult::e_SUCCESS) {
        return rc;  // RETURN
    }

    log->updateOutstandingNumBytes(value);
    d_outstandingNumBytes += value;

    return LedgerOpResult::e_SUCCESS;
}

int Ledger::setOutstandingNumBytes(const mqbu::StorageKey& logId,
                                   bsls::Types::Int64      value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state == LedgerState::e_OPENED);

    Log* log = 0;
    int  rc  = find(&log, logId);
    if (rc != LedgerOpResult::e_SUCCESS) {
        return rc;  // RETURN
    }

    const bsls::Types::Int64 oldNumBytes = log->outstandingNumBytes();
    log->setOutstandingNumBytes(value);
    d_outstandingNumBytes += (log->outstandingNumBytes() - oldNumBytes);

    return LedgerOpResult::e_SUCCESS;
}

int Ledger::writeRecord(LedgerRecordId* recordId,
                        const void*     record,
                        int             offset,
                        int             length)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state == LedgerState::e_OPENED);
    BSLS_ASSERT_SAFE(recordId);
    BSLS_ASSERT_SAFE(record);
    BSLS_ASSERT_SAFE(offset >= 0);
    BSLS_ASSERT_SAFE(length >= 0);
    BSLS_ASSERT_SAFE(!d_logs.empty());

    return writeRecordImpl(recordId, record, offset, length);
}

int Ledger::writeRecord(LedgerRecordId*           recordId,
                        const bdlbb::Blob&        record,
                        const bmqu::BlobPosition& offset,
                        int                       length)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state == LedgerState::e_OPENED);
    BSLS_ASSERT_SAFE(recordId);
    BSLS_ASSERT_SAFE(offset.byte() >= 0);
    BSLS_ASSERT_SAFE(offset.buffer() >= 0);
    BSLS_ASSERT_SAFE(length >= 0);
    BSLS_ASSERT_SAFE(!d_logs.empty());

    return writeRecordImpl(recordId, record, offset, length);
}

int Ledger::writeRecord(LedgerRecordId*          recordId,
                        const bdlbb::Blob&       record,
                        const bmqu::BlobSection& section)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state == LedgerState::e_OPENED);
    BSLS_ASSERT_SAFE(recordId);
    BSLS_ASSERT_SAFE(!d_logs.empty());

    int length;
    int rc = bmqu::BlobUtil::sectionSize(&length, record, section);
    if (rc != 0) {
        return LedgerOpResult::e_INVALID_BLOB_SECTION;  // RETURN
    }

    return writeRecord(recordId, record, section.start(), length);
}

int Ledger::flush()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state == LedgerState::e_OPENED);
    BSLS_ASSERT_SAFE(!d_logs.empty());

    // TBD: Maybe handle failure to flush by adding to "dirty list" and trying
    //       to flush again at the next call to 'flush'
    int rc = currentLog()->flush();
    if (rc != LogOpResult::e_SUCCESS) {
        return 100 * rc +
               mqbsi::LedgerOpResult::e_LOG_FLUSH_FAILURE;  // RETURN
    }

    return LedgerOpResult::e_SUCCESS;
}

// ACCESSORS
//   (virtual 'mqbsi::Ledger')
int Ledger::readRecord(void*                 entry,
                       int                   length,
                       const LedgerRecordId& recordId) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state == LedgerState::e_OPENED);
    BSLS_ASSERT_SAFE(entry);
    BSLS_ASSERT_SAFE(length >= 0);

    Log* log = 0;
    int  rc  = find(&log, recordId.logId());
    if (rc != LedgerOpResult::e_SUCCESS) {
        return rc;  // RETURN
    }

    rc = log->read(entry, length, recordId.offset());
    if (rc != LogOpResult::e_SUCCESS) {
        return 100 * rc + LedgerOpResult::e_RECORD_READ_FAILURE;  // RETURN
    }

    return LedgerOpResult::e_SUCCESS;
}

int Ledger::readRecord(bdlbb::Blob*          entry,
                       int                   length,
                       const LedgerRecordId& recordId) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state == LedgerState::e_OPENED);
    BSLS_ASSERT_SAFE(entry);
    BSLS_ASSERT_SAFE(length >= 0);

    Log* log = 0;
    int  rc  = find(&log, recordId.logId());
    if (rc != LedgerOpResult::e_SUCCESS) {
        return rc;  // RETURN
    }

    rc = log->read(entry, length, recordId.offset());
    if (rc != LogOpResult::e_SUCCESS) {
        return 100 * rc + LedgerOpResult::e_RECORD_READ_FAILURE;  // RETURN
    }

    return LedgerOpResult::e_SUCCESS;
}

int Ledger::aliasRecord(void**                entry,
                        int                   length,
                        const LedgerRecordId& recordId) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state == LedgerState::e_OPENED);
    BSLS_ASSERT_SAFE(entry);
    BSLS_ASSERT_SAFE(length >= 0);

    if (!d_supportsAliasing) {
        return LedgerOpResult::e_ALIAS_NOT_SUPPORTED;  // RETURN
    }

    Log* log = 0;
    int  rc  = find(&log, recordId.logId());
    if (rc != LedgerOpResult::e_SUCCESS) {
        return rc;  // RETURN
    }

    rc = log->alias(entry, length, recordId.offset());
    if (rc != LogOpResult::e_SUCCESS) {
        return 100 * rc + LedgerOpResult::e_RECORD_ALIAS_FAILURE;  // RETURN
    }

    return LedgerOpResult::e_SUCCESS;
}

int Ledger::aliasRecord(bdlbb::Blob*          entry,
                        int                   length,
                        const LedgerRecordId& recordId) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state == LedgerState::e_OPENED);
    BSLS_ASSERT_SAFE(entry);
    BSLS_ASSERT_SAFE(length >= 0);

    if (!d_supportsAliasing) {
        return LedgerOpResult::e_ALIAS_NOT_SUPPORTED;  // RETURN
    }

    Log* log = 0;
    int  rc  = find(&log, recordId.logId());
    if (rc != LedgerOpResult::e_SUCCESS) {
        return rc;  // RETURN
    }

    rc = log->alias(entry, length, recordId.offset());
    if (rc != LogOpResult::e_SUCCESS) {
        return 100 * rc + LedgerOpResult::e_RECORD_ALIAS_FAILURE;  // RETURN
    }

    return LedgerOpResult::e_SUCCESS;
}

}  // close package namespace
}  // close enterprise namespace
