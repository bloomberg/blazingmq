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

// bmqtsk_logcleaner.cpp                                              -*-C++-*-
#include <bmqtsk_logcleaner.h>

#include <bmqscm_version.h>

#include <bmqex_bindutil.h>
#include <bmqex_executionpolicy.h>

// BDE
#include <ball_log.h>
#include <bdlf_memfn.h>
#include <bdlma_localsequentialallocator.h>
#include <bdls_filesystemutil.h>
#include <bdlt_currenttime.h>
#include <bdlt_datetime.h>
#include <bdlt_epochutil.h>
#include <bsl_cstddef.h>
#include <bsl_iostream.h>
#include <bsl_ostream.h>
#include <bsl_vector.h>
#include <bslmt_threadutil.h>
#include <bsls_assert.h>
#include <bsls_systemclocktype.h>
#include <bsls_systemtime.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("BMQTSK.LOGCLEANER");

// UTILITY FUNCTIONS

/// Return thread attributes to be used when creating a logs cleaning
/// thread.
bslmt::ThreadAttributes logsCleaningThreadAttributes()
{
    // NOTE: Use low priority scheduling thread, since the log rotation
    //       operation is a background IO cleanup job.

    bslmt::ThreadAttributes attr = bslmt::ThreadAttributes();
    attr.setInheritSchedule(false);
    attr.setSchedulingPriority(
        bslmt::ThreadUtil::getMinSchedulingPriority(attr.schedulingPolicy()));

    return attr;
}

}  // unnamed namespace

namespace bmqtsk {

// ----------------
// class LogCleaner
// ----------------

// PRIVATE MANIPULATORS
void LogCleaner::cleanLogs()
{
    // EXECUTED FROM SYSTEM EXECUTOR THREAD

    // PRECONDITIONS
    BSLS_ASSERT(d_logsCleaningContext.executor().runningInThisThread());

    bdlma::LocalSequentialAllocator<4096> localAllocator;

    const bdlt::Datetime keepThreshold =
        bdlt::EpochUtil::convertFromTimeInterval(bdlt::CurrentTime::now() -
                                                 d_maxAge);

    BALL_LOG_INFO << "Cleaning files matching '" << d_filePattern << "' "
                  << "older than " << keepThreshold << ".";

    bsl::vector<bsl::string> files(&localAllocator);
    bdls::FilesystemUtil::findMatchingPaths(&files, d_filePattern.c_str());

    int filesDeleted = 0;
    for (bsl::vector<bsl::string>::iterator it = files.begin();
         it != files.end();
         ++it) {
        bdlt::Datetime modificationTime;
        int            rc = bdls::FilesystemUtil::getLastModificationTime(
            &modificationTime,
            *it);
        if (rc != 0) {
            BALL_LOG_WARN << "Failed retrieving last modification time of '"
                          << *it << "' [rc: " << rc << "]";
            continue;  // CONTINUE
        }

        if (modificationTime < keepThreshold) {
            rc = bdls::FilesystemUtil::remove(*it);

            if (rc != 0) {
                BALL_LOG_WARN << "Failed deleting file '" << *it << "' "
                              << "[rc: " << rc << "]";
            }
            else {
                ++filesDeleted;
                BALL_LOG_TRACE << "Deleted file '" << *it << "'";
            }
        }
    }

    BALL_LOG_INFO << "Cleaning files matching '" << d_filePattern << "' "
                  << "completed, " << filesDeleted << " file(s) deleted.";
}

// CREATORS
LogCleaner::LogCleaner(bdlmt::EventScheduler* scheduler,
                       bslma::Allocator*      allocator)
: d_scheduler_p(scheduler)
, d_logsCleaningEvent()
, d_logsCleaningContext(bmqex::SystemExecutor(logsCleaningThreadAttributes(),
                                              allocator),
                        allocator)
, d_filePattern(allocator)
, d_maxAge(0)
{
    // PRECONDITIONS
    BSLS_ASSERT(scheduler);

    // start accepting work on the log cleaning execution context
    d_logsCleaningContext.start();
}

LogCleaner::~LogCleaner()
{
    stop();
}

// MANIPULATORS
int LogCleaner::start(bslstl::StringRef         pattern,
                      bsls::TimeInterval        maxAge,
                      const bsls::TimeInterval& cleanupPeriod)
{
    if (d_logsCleaningEvent) {
        // Log cleaner already started. Do nothing.
        return 0;  // RETURN
    }

    d_filePattern = pattern;
    d_maxAge      = maxAge;

    // schedule a logs cleanup immediately and then once every 'cleanupPeriod'
    // interval
    d_scheduler_p->scheduleRecurringEvent(
        &d_logsCleaningEvent,
        cleanupPeriod,
        bmqex::BindUtil::bindExecute(
            bmqex::ExecutionPolicyUtil::oneWay().neverBlocking().useExecutor(
                d_logsCleaningContext.executor()),
            bdlf::MemFnUtil::memFn(&LogCleaner::cleanLogs, this)),
        bsls::SystemTime::now(d_scheduler_p->clockType()));

    return 0;
}

void LogCleaner::stop()
{
    if (!d_logsCleaningEvent) {
        // Log cleaner already stopped. Do nothing.
        return;  // RETURN
    }

    // cancel the logs cleaning recurring event and synchronize with the
    // callback
    d_scheduler_p->cancelEventAndWait(&d_logsCleaningEvent);

    // cancel all pending log cleaning operations and synchronize with any
    // ongoing one
    d_logsCleaningContext.dropPendingJobs();
    d_logsCleaningContext.join();
}

}  // close package namespace
}  // close enterprise namespace
