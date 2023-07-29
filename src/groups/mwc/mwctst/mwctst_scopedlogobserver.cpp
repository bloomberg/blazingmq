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

// mwctst_scopedlogobserver.cpp                                       -*-C++-*-
#include <mwctst_scopedlogobserver.h>

#include <mwcscm_version.h>
// BDE
#include <ball_loggermanager.h>
#include <bdlpcre_regex.h>
#include <bsl_memory.h>
#include <bslalg_numericformatterutil.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mwctst {

namespace {

// UTILITY FUNCTIONS

/// Generate an unique log observer name for the observer with the specified
/// `observerAddress`, and load the name into the specified `observerName`.
void generateUniqueObserverName(bsl::string* observerName,
                                void*        observerAddress)
{
    // PRECONDITIONS
    BSLS_ASSERT(observerName);
    BSLS_ASSERT(observerAddress);

    observerName->resize(32);
    observerName->resize(
        bslalg::NumericFormatterUtil::toChars(
            observerName->data(),
            observerName->data() + 32,
            reinterpret_cast<unsigned long long>(observerAddress),
            16)  // base
        - observerName->data());
}

}  // close unnamed namespace

// -----------------------
// class ScopedLogObserver
// -----------------------

// CREATORS
ScopedLogObserver::ScopedLogObserver(ball::Severity::Level severity,
                                     bslma::Allocator*     allocator)
: d_severityThreshold(severity)
, d_observerName(allocator)
, d_records(allocator)
{
    // PRECONDTIONS
    BSLS_ASSERT(allocator);
    BSLS_ASSERT(ball::LoggerManager::isInitialized());

    // generate unique observer name
    generateUniqueObserverName(&d_observerName, this);

    // register this observer
    ball::LoggerManager& loggerManager = ball::LoggerManager::singleton();
    int                  rc            = loggerManager.registerObserver(
        bsl::shared_ptr<ScopedLogObserver>(this,
                                           bslstl::SharedPtrNilDeleter(),
                                           allocator),
        d_observerName);
    BSLS_ASSERT_OPT(rc == 0);
}

ScopedLogObserver::~ScopedLogObserver()
{
    // PRECONDITIONS
    BSLS_ASSERT(ball::LoggerManager::isInitialized());

    // unregister this observer
    ball::LoggerManager& loggerManager = ball::LoggerManager::singleton();
    int                  rc = loggerManager.deregisterObserver(d_observerName);
    BSLS_ASSERT_OPT(rc == 0);
}

// MANIPULATORS
ScopedLogObserver&
ScopedLogObserver::setSeverityThreshold(ball::Severity::Level value)
{
    d_severityThreshold = value;
    return *this;
}

void ScopedLogObserver::publish(
    const ball::Record&          record,
    BSLS_ANNOTATION_UNUSED const ball::Context& context)
{
    // Check if the record's severity is higher than the severity threshold
    if (record.fixedFields().severity() > d_severityThreshold) {
        return;  // RETURN
    }

    d_records.push_back(record);
}

// ACCESSORS
ball::Severity::Level ScopedLogObserver::severityThreshold() const
{
    return d_severityThreshold;
}

bool ScopedLogObserver::isEnabled() const
{
    return severityThreshold() != ball::Severity::OFF;
}

const bsl::vector<ball::Record>& ScopedLogObserver::records() const
{
    return d_records;
}

// ----------------------------
// struct ScopedLogObserverUtil
// ----------------------------

bool ScopedLogObserverUtil::recordMessageMatch(const ball::Record& record,
                                               const char*         pattern,
                                               bslma::Allocator*   allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(pattern);
    BSLS_ASSERT(allocator);

    bsl::string    error(allocator);
    size_t         errorOffset;
    bdlpcre::RegEx regex(allocator);

    int rc = regex.prepare(&error,
                           &errorOffset,
                           pattern,
                           bdlpcre::RegEx::k_FLAG_CASELESS |
                               bdlpcre::RegEx::k_FLAG_DOTMATCHESALL);
    BSLS_ASSERT(rc == 0);
    BSLS_ASSERT(regex.isPrepared() == true);

    const bslstl::StringRef& msg = record.fixedFields().message();

    rc = regex.match(msg.data(), msg.length());

    return rc == 0;
}

}  // close package namespace
}  // close enterprise namespace
