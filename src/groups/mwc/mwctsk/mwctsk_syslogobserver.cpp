// Copyright 2022-2023 Bloomberg Finance L.P.
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

// mwctsk_syslogobserver.cpp                                          -*-C++-*-
#include <mwctsk_syslogobserver.h>

#include <mwcscm_version.h>
// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <ball_record.h>
#include <bdlma_localsequentialallocator.h>

extern "C" {
#include <syslog.h>
}

namespace BloombergLP {
namespace mwctsk {

// ---------------------
// class SyslogObserver
// ---------------------

SyslogObserver::SyslogObserver(bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_formatter(allocator)
, d_severityThreshold(ball::Severity::INFO)
{
    d_formatter.disablePublishInLocalTime();  // Always use UTC time
}

SyslogObserver::~SyslogObserver()
{
    // NOTHING (required because of virtual class)
}

void SyslogObserver::enableLogging(const bsl::string& appName)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    closelog();
    openlog(appName.c_str(), LOG_ODELAY | LOG_PID, LOG_USER);
    setlogmask(LOG_MASK(LOG_EMERG) | LOG_MASK(LOG_ALERT) | LOG_MASK(LOG_CRIT) |
               LOG_MASK(LOG_ERR) | LOG_MASK(LOG_WARNING) |
               LOG_MASK(LOG_NOTICE) | LOG_MASK(LOG_INFO) |
               LOG_MASK(LOG_DEBUG));
}

void SyslogObserver::disableLogging()
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    closelog();
}

void SyslogObserver::publish(
    const ball::Record&          record,
    BSLS_ANNOTATION_UNUSED const ball::Context& context)
{
    // Check if the record's severity is higher than the severity threshold
    if (record.fixedFields().severity() > d_severityThreshold) {
        return;  // RETURN
    }

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
    mwcu::MemOutStream                    os(&localAllocator);
    d_formatter(os, record);

    ball::Severity::Level severity = static_cast<ball::Severity::Level>(
        record.fixedFields().severity());

    syslog(severity, "%s", os.str().data());
}

}  // close package namespace
}  // close enterprise namespace
