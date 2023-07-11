// Copyright 2017-2023 Bloomberg Finance L.P.
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

// mwctsk_alarmlog.cpp                                                -*-C++-*-
#include <mwctsk_alarmlog.h>

#include <mwcscm_version.h>

// MWC
#include <mwcu_memoutstream.h>
#include <mwcu_stringutil.h>

// BDE
#include <ball_severity.h>
#include <bdlt_timeunitratio.h>
#include <bsl_iostream.h>
#include <bslma_allocator.h>
#include <bslmt_lockguard.h>
#include <bsls_annotation.h>
#include <bsls_performancehint.h>
#include <bsls_timeutil.h>

namespace BloombergLP {
namespace mwctsk {

namespace {
const int k_ALARM_INTERVAL = 60;
// Minimum interval, in seconds, between two alarms from the same
// source.

/// Generate the alarm for the specified `category` with the specified
/// `message` by writing to stderr.
void generateAlarm(const bslstl::StringRef& category,
                   const bslstl::StringRef& message)
{
    bsl::cerr << category << ": " << message << '\n' << bsl::flush;
}

}  // close unnamed namespace

// --------------
// class AlarmLog
// --------------

AlarmLog::AlarmLog(bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_lastAlarmTime(allocator)
{
    // NOTHING
}

AlarmLog::~AlarmLog()
{
    // NOTHING (required because of virtual class)
}

void AlarmLog::publish(const ball::Record&          record,
                       BSLS_ANNOTATION_UNUSED const ball::Context& context)
{
    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(
            (record.fixedFields().severity() > ball::Severity::ERROR) ||
            (record.customFields().length() == 0))) {
        // We are only interested in ERROR severity, and only the ones which
        // have a userField (the alarm type) set.
        return;  // RETURN
    }

    BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);

    const bsl::string& alarmType = record.customFields()[0].theString();

    // Raw version used, no throttling
    if (mwcu::StringUtil::startsWith(alarmType, "RAW_")) {
        // We are stripping out the 'RAW_' prefix from the alarm identification
        // string.
        const bslstl::StringRef categoryStr(alarmType.c_str() + 4,
                                            alarmType.length() - 4);
        generateAlarm(categoryStr, record.fixedFields().message());
        return;  // RETURN
    }

    // Build the log source unique identifier (used for rate-limiting alarms)
    // by concatenating the file and the line.
    mwcu::MemOutStream ss(d_allocator_p);
    ss << record.fixedFields().fileName() << ":"
       << record.fixedFields().lineNumber() << bsl::ends;

    bsls::Types::Int64& last = d_lastAlarmTime[ss.str()];
    bsls::Types::Int64  now  = bsls::TimeUtil::getTimer();

    if ((last == 0) ||
        ((now - last) >=
         k_ALARM_INTERVAL * bdlt::TimeUnitRatio::k_NANOSECONDS_PER_SECOND)) {
        last = now;  // Update alarm last time
        generateAlarm(alarmType, record.fixedFields().message());
    }
}

}  // close package namespace
}  // close enterprise namespace
