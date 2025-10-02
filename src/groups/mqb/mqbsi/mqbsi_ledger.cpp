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

// mqbsi_ledger.cpp                                                   -*-C++-*-
#include <mqbsi_ledger.h>

#include <mqbscm_version.h>
// BDE
#include <bdlb_string.h>
#include <bsl_ostream.h>
#include <bslim_printer.h>
#include <bslmf_allocatorargt.h>

namespace BloombergLP {
namespace mqbsi {

// --------------------
// class LedgerRecordId
// --------------------

// CREATORS
LedgerRecordId::LedgerRecordId()
: d_logId()
, d_offset(mqbsi::Log::k_INVALID_OFFSET)
{
    // NOTHING
}

LedgerRecordId::LedgerRecordId(const mqbu::StorageKey& logId,
                               Log::Offset             offset)
: d_logId(logId)
, d_offset(offset)
{
    // NOTHING
}

LedgerRecordId& LedgerRecordId::setLogId(const mqbu::StorageKey& value)
{
    d_logId = value;
    return *this;
}

LedgerRecordId& LedgerRecordId::setOffset(Log::Offset offset)
{
    d_offset = offset;
    return *this;
}

// ACCESSORS
const mqbu::StorageKey& LedgerRecordId::logId() const
{
    return d_logId;
}

Log::Offset LedgerRecordId::offset() const
{
    return d_offset;
}

bsl::ostream& LedgerRecordId::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("logId", logId());
    printer.printAttribute("offset", offset());
    printer.end();

    return stream;
}

// ---------------------
// struct LedgerOpResult
// ---------------------

bsl::ostream& LedgerOpResult::print(bsl::ostream&        stream,
                                    LedgerOpResult::Enum value,
                                    int                  level,
                                    int                  spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << LedgerOpResult::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* LedgerOpResult::toAscii(LedgerOpResult::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(SUCCESS)
        CASE(UNKNOWN)
        CASE(LEDGER_NOT_EXIST)
        CASE(LEDGER_UNGRACEFUL_CLOSE)
        CASE(LEDGER_READ_ONLY)
        CASE(LOG_CREATE_FAILURE)
        CASE(LOG_OPEN_FAILURE)
        CASE(LOG_CLOSE_FAILURE)
        CASE(LOG_FLUSH_FAILURE)
        CASE(LOG_ROLLOVER_CB_FAILURE)
        CASE(LOG_CLEANUP_FAILURE)
        CASE(LOG_NOT_FOUND)
        CASE(LOG_INVALID)
        CASE(RECORD_WRITE_FAILURE)
        CASE(RECORD_READ_FAILURE)
        CASE(RECORD_ALIAS_FAILURE)
        CASE(ALIAS_NOT_SUPPORTED)
        CASE(INVALID_BLOB_SECTION)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool LedgerOpResult::fromAscii(LedgerOpResult::Enum*    out,
                               const bslstl::StringRef& str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(LedgerOpResult::e_##M),        \
                                       str.data(),                            \
                                       static_cast<int>(str.length()))) {     \
        *out = LedgerOpResult::e_##M;                                         \
        return true;                                                          \
    }

    CHECKVALUE(SUCCESS)
    CHECKVALUE(UNKNOWN)
    CHECKVALUE(LEDGER_NOT_EXIST)
    CHECKVALUE(LEDGER_UNGRACEFUL_CLOSE)
    CHECKVALUE(LEDGER_READ_ONLY)
    CHECKVALUE(LOG_CREATE_FAILURE)
    CHECKVALUE(LOG_OPEN_FAILURE)
    CHECKVALUE(LOG_CLOSE_FAILURE)
    CHECKVALUE(LOG_FLUSH_FAILURE)
    CHECKVALUE(LOG_ROLLOVER_CB_FAILURE)
    CHECKVALUE(LOG_CLEANUP_FAILURE)
    CHECKVALUE(LOG_NOT_FOUND)
    CHECKVALUE(LOG_INVALID)
    CHECKVALUE(RECORD_WRITE_FAILURE)
    CHECKVALUE(RECORD_READ_FAILURE)
    CHECKVALUE(RECORD_ALIAS_FAILURE)
    CHECKVALUE(ALIAS_NOT_SUPPORTED)
    CHECKVALUE(INVALID_BLOB_SECTION)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// ------------------
// class LedgerConfig
// ------------------

LedgerConfig::LedgerConfig(bslma::Allocator* allocator)
: d_location(allocator)
, d_pattern(allocator)
, d_maxLogSize(0)
, d_reserveOnDisk(false)
, d_prefaultPages(false)
, d_keepOldLogs(false)
, d_logFactory_sp()
, d_logIdGenerator_sp()
, d_scheduler_p()
, d_extractLogIdCallback(bsl::allocator_arg, allocator)
, d_validateLogCallback(bsl::allocator_arg, allocator)
, d_rolloverCallback(bsl::allocator_arg, allocator)
, d_cleanupCallback(bsl::allocator_arg, allocator)
{
    // NOTHING
}

LedgerConfig::LedgerConfig(const LedgerConfig& other,
                           bslma::Allocator*   allocator)
: d_location(other.d_location, allocator)
, d_pattern(other.d_pattern, allocator)
, d_maxLogSize(other.d_maxLogSize)
, d_reserveOnDisk(other.d_reserveOnDisk)
, d_prefaultPages(other.d_prefaultPages)
, d_keepOldLogs(other.d_keepOldLogs)
, d_logFactory_sp(other.d_logFactory_sp)
, d_logIdGenerator_sp(other.d_logIdGenerator_sp)
, d_scheduler_p(other.d_scheduler_p)
, d_extractLogIdCallback(bsl::allocator_arg,
                         allocator,
                         other.d_extractLogIdCallback)
, d_validateLogCallback(bsl::allocator_arg,
                        allocator,
                        other.d_validateLogCallback)
, d_rolloverCallback(bsl::allocator_arg, allocator, other.d_rolloverCallback)
, d_cleanupCallback(bsl::allocator_arg, allocator, other.d_cleanupCallback)
{
    // NOTHING
}

// MANIPULATORS
LedgerConfig& LedgerConfig::setLocation(const bsl::string& value)
{
    d_location = value;
    return *this;
}

LedgerConfig& LedgerConfig::setPattern(const bsl::string& value)
{
    d_pattern = value;
    return *this;
}

LedgerConfig& LedgerConfig::setMaxLogSize(const bsls::Types::Int64 value)
{
    d_maxLogSize = value;
    return *this;
}

LedgerConfig& LedgerConfig::setReserveOnDisk(bool value)
{
    d_reserveOnDisk = value;
    return *this;
}

LedgerConfig& LedgerConfig::setPrefaultPages(bool value)
{
    d_prefaultPages = value;
    return *this;
}

LedgerConfig& LedgerConfig::setKeepOldLogs(bool value)
{
    d_keepOldLogs = value;
    return *this;
}

LedgerConfig&
LedgerConfig::setLogFactory(const bsl::shared_ptr<mqbsi::LogFactory>& value)
{
    d_logFactory_sp = value;
    return *this;
}

LedgerConfig& LedgerConfig::setLogIdGenerator(
    const bsl::shared_ptr<mqbsi::LogIdGenerator>& value)
{
    d_logIdGenerator_sp = value;
    return *this;
}

LedgerConfig& LedgerConfig::setScheduler(bdlmt::EventScheduler* value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(value);
    BSLS_ASSERT_SAFE(value->clockType() == bsls::SystemClockType::e_MONOTONIC);

    d_scheduler_p = value;
    return *this;
}

LedgerConfig&
LedgerConfig::setExtractLogIdCallback(const ExtractLogIdCb& value)
{
    d_extractLogIdCallback = value;
    return *this;
}

LedgerConfig& LedgerConfig::setValidateLogCallback(const ValidateLogCb& value)
{
    d_validateLogCallback = value;
    return *this;
}

LedgerConfig& LedgerConfig::setRolloverCallback(const OnRolloverCb& value)
{
    d_rolloverCallback = value;
    return *this;
}

LedgerConfig& LedgerConfig::setCleanupCallback(const CleanupCb& value)
{
    d_cleanupCallback = value;
    return *this;
}

// ACCESSORS
const bsl::string& LedgerConfig::location() const
{
    return d_location;
}

const bsl::string& LedgerConfig::pattern() const
{
    return d_pattern;
}

bsls::Types::Int64 LedgerConfig::maxLogSize() const
{
    return d_maxLogSize;
}

bool LedgerConfig::reserveOnDisk() const
{
    return d_reserveOnDisk;
}

bool LedgerConfig::prefaultPages() const
{
    return d_prefaultPages;
}

bool LedgerConfig::keepOldLogs() const
{
    return d_keepOldLogs;
}

mqbsi::LogFactory* LedgerConfig::logFactory() const
{
    return d_logFactory_sp.get();
}

mqbsi::LogIdGenerator* LedgerConfig::logIdGenerator() const
{
    return d_logIdGenerator_sp.get();
}

bdlmt::EventScheduler* LedgerConfig::scheduler() const
{
    return d_scheduler_p;
}

const LedgerConfig::ExtractLogIdCb& LedgerConfig::extractLogIdCallback() const
{
    return d_extractLogIdCallback;
}

const LedgerConfig::ValidateLogCb& LedgerConfig::validateLogCallback() const
{
    return d_validateLogCallback;
}

const LedgerConfig::OnRolloverCb& LedgerConfig::rolloverCallback() const
{
    return d_rolloverCallback;
}

const LedgerConfig::CleanupCb& LedgerConfig::cleanupCallback() const
{
    return d_cleanupCallback;
}

bsl::ostream&
LedgerConfig::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("location", location());
    printer.printAttribute("pattern", pattern());
    printer.printAttribute("maxLogSize", maxLogSize());

    return stream;
}

// ------------
// class Ledger
// ------------

Ledger::~Ledger()
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace
