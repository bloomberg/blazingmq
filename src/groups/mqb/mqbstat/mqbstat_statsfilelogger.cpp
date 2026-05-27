// Copyright 2026 Bloomberg Finance L.P.
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

#include <mqbstat_statsfilelogger.h>

#include <mqbscm_version.h>
// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>

// BDE
#include <ball_context.h>
#include <ball_log.h>
#include <ball_logfilecleanerutil.h>
#include <ball_recordattributes.h>
#include <ball_recordstringformatter.h>
#include <ball_transmission.h>
#include <bdls_processutil.h>
#include <bdlt_datetime.h>
#include <bdlt_epochutil.h>
#include <bsl_ctime.h>
#include <bsl_ostream.h>
#include <bslma_allocator.h>
#include <bslmt_threadutil.h>
#include <bsls_assert.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqbstat {

namespace {

const char k_LOG_CATEGORY[] = "MQBSTAT.STATSFILELOGGER";

}  // close unnamed namespace

// --------------------
// class StatsFileLogger
// --------------------

StatsFileLogger::StatsFileLogger(bsl::string_view       filePattern,
                                 bdlmt::EventScheduler* eventScheduler,
                                 bslma::Allocator*      allocator)
: d_filePattern(filePattern, allocator)
, d_statsLogFile(allocator)
, d_statLogCleaner(eventScheduler, allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(eventScheduler);
    BSLS_ASSERT_SAFE(eventScheduler->clockType() ==
                     bsls::SystemClockType::e_MONOTONIC);
}

void StatsFileLogger::start()
{
    const mqbcfg::StatsPrinterConfig& printer =
        mqbcfg::BrokerConfig::get().stats().printer();

    // Configure the stats dump log file
    d_statsLogFile.enableFileLogging(d_filePattern.c_str());
    d_statsLogFile.rotateOnSize(printer.rotateBytes() / 1024);
    d_statsLogFile.rotateOnTimeInterval(
        bdlt::DatetimeInterval(printer.rotateDays()));
    d_statsLogFile.setLogFileFunctor(ball::RecordStringFormatter("%m\n"));

    // LogCleanup
    if (printer.maxAgeDays() <= 0 || d_filePattern.empty()) {
        BALL_LOG_INFO << "StatLogCleaning is *disabled* "
                      << "[reason: either 'maxAgeDays' is set to 0 in config "
                      << "or file pattern is empty]";
        return;  // RETURN
    }

    bsl::string cleanupPattern;
    ball::LogFileCleanerUtil::logPatternToFilePattern(&cleanupPattern,
                                                      d_filePattern);
    bsls::TimeInterval maxAge(0, 0);
    maxAge.addDays(printer.maxAgeDays());

    int rc = d_statLogCleaner.start(cleanupPattern, maxAge);
    if (rc != 0) {
        BALL_LOG_ERROR << "#STATLOG_CLEANING "
                       << "Failed to start log cleaning of '" << cleanupPattern
                       << "' [rc: " << rc << "]";
    }
}

void StatsFileLogger::stop()
{
    d_statLogCleaner.stop();
}

void StatsFileLogger::logStats(const PrinterCb& printerCb)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(printerCb);

    ball::Record            record;
    ball::RecordAttributes& attributes = record.fixedFields();
    bdlt::Datetime          now;
    bdlt::EpochUtil::convertFromTimeT(&now, time(0));
    attributes.setTimestamp(now);
    attributes.setProcessID(bdls::ProcessUtil::getProcessId());
    attributes.setThreadID(bslmt::ThreadUtil::selfIdAsUint64());
    attributes.setFileName(__FILE__);
    attributes.setLineNumber(__LINE__);
    attributes.setCategory(k_LOG_CATEGORY);
    attributes.setSeverity(ball::Severity::e_INFO);

    attributes.clearMessage();
    bsl::ostream os(&attributes.messageStreamBuf());

    printerCb(os);

    d_statsLogFile.publish(
        record,
        ball::Context(ball::Transmission::e_MANUAL_PUBLISH, 0, 1));
}

}  // close package namespace
}  // close enterprise namespace
