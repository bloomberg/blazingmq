// Copyright 2019-2023 Bloomberg Finance L.P.
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

// mqbc_incoreclusterstateledgeriterator.cpp                          -*-C++-*-
#include <mqbc_incoreclusterstateledgeriterator.h>

#include <mqbscm_version.h>
// MQB
#include <mqbc_clusterstateledgerutil.h>

// BMQ
#include <bmqp_protocol.h>

// BDE
#include <bdlb_scopeexit.h>
#include <bdlf_bind.h>
#include <bslim_printer.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbc {

namespace {

/// When `next()` returns a non-zero value, this method can be invoked to
/// set the specified `isValid` flag to false.
void onInvalidNext(bool* isValid)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid);

    *isValid = false;
}

}  // close unnamed namespace

// ---------------------------------------
// struct IncoreClusterStateLedgerIterator
// ---------------------------------------

// PRIVATE MANIPULATORS
void IncoreClusterStateLedgerIterator::incrementOffset(
    mqbsi::Log::Offset amount)
{
    d_currRecordId.setOffset(d_currRecordId.offset() + amount);
}

// CREATORS
IncoreClusterStateLedgerIterator::IncoreClusterStateLedgerIterator(
    const mqbsi::Ledger* ledger)
: d_firstInvocation(true)
, d_isValid(false)
, d_ledger_p(ledger)
, d_currRecordId()
, d_currRecordHeader_p()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(ledger && ledger->isOpened());
    BSLS_ASSERT_SAFE(ledger->supportsAliasing());
}

IncoreClusterStateLedgerIterator::~IncoreClusterStateLedgerIterator()
{
    // NOTHING
}

// MANIPULATORS
//   (virtual mqbc::ClusterStateLedgerIterator)
int IncoreClusterStateLedgerIterator::next()
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_END_OF_LEDGER = 1  // End of ledger is reached
        ,
        rc_SUCCESS = 0  // Success
        ,
        rc_RECORD_ALIAS_FAILURE = -1  // Failure to alias a record
        ,
        rc_INVALID_FILE_HEADER = -2  // Invalid file header
        ,
        rc_INVALID_RECORD_HEADER = -3  // Invalid record header
        ,
        rc_OUT_OF_LEDGER_RANGE = -4  // An offset outside of the ledger's
                                     // range is reached
    };

    bdlb::ScopeExitAny guard(bdlf::BindUtil::bind(onInvalidNext, &d_isValid));

    // 1. Advance pass the previous record
    if (d_firstInvocation) {
        d_firstInvocation = false;

        d_currRecordId.setLogId(d_ledger_p->currentLog()->logConfig().logId());
        d_currRecordId.setOffset(0);

        // Validate the file header
        ClusterStateFileHeader* fh;
        int rc = d_ledger_p->aliasRecord(reinterpret_cast<void**>(&fh),
                                         sizeof(ClusterStateFileHeader),
                                         d_currRecordId);
        if (rc != 0) {
            return (rc * 10) + rc_RECORD_ALIAS_FAILURE;  // RETURN
        }

        rc = ClusterStateLedgerUtil::validateFileHeader(*fh);
        if (rc != 0) {
            return (rc * 10) + rc_INVALID_FILE_HEADER;  // RETURN
        }

        incrementOffset(fh->headerWords() * bmqp::Protocol::k_WORD_SIZE);
    }
    else {
        incrementOffset(
            ClusterStateLedgerUtil::recordSize(*d_currRecordHeader_p));
    }

    if (d_currRecordId.offset() ==
        d_ledger_p->currentLog()->outstandingNumBytes()) {
        return rc_END_OF_LEDGER;  // RETURN
    }

    if (d_currRecordId.offset() >
        d_ledger_p->currentLog()->outstandingNumBytes()) {
        return rc_OUT_OF_LEDGER_RANGE;  // RETURN
    }

    // 2. Parse the new record
    int rc = d_ledger_p->aliasRecord(
        reinterpret_cast<void**>(&d_currRecordHeader_p),
        sizeof(ClusterStateRecordHeader),
        d_currRecordId);
    if (rc != 0) {
        return (rc * 10) + rc_RECORD_ALIAS_FAILURE;  // RETURN
    }

    rc = ClusterStateLedgerUtil::validateRecordHeader(*d_currRecordHeader_p);
    if (rc != 0) {
        return (rc * 10) + rc_INVALID_RECORD_HEADER;  // RETURN
    }

    guard.release();
    d_isValid = true;
    return rc_SUCCESS;
}

// ACCESSORS
//   (virtual mqbc::ClusterStateLedgerIterator)
const ClusterStateRecordHeader&
IncoreClusterStateLedgerIterator::header() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return *d_currRecordHeader_p;
}

int IncoreClusterStateLedgerIterator::loadClusterMessage(
    bmqp_ctrlmsg::ClusterMessage* message) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(message);
    BSLS_ASSERT_SAFE(isValid());

    return ClusterStateLedgerUtil::loadClusterMessage(message,
                                                      *d_ledger_p,
                                                      *d_currRecordHeader_p,
                                                      d_currRecordId);
}

bsl::ostream& IncoreClusterStateLedgerIterator::print(bsl::ostream& stream,
                                                      int           level,
                                                      int spacesPerLevel) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("headerWords", d_currRecordHeader_p->headerWords());
    printer.printAttribute("recordType", d_currRecordHeader_p->recordType());
    printer.printAttribute("leaderAdvisoryWords",
                           d_currRecordHeader_p->leaderAdvisoryWords());
    printer.printAttribute("electorTerm", d_currRecordHeader_p->electorTerm());
    printer.printAttribute("sequenceNumber",
                           d_currRecordHeader_p->sequenceNumber());
    printer.printAttribute("timestamp", d_currRecordHeader_p->timestamp());
    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
