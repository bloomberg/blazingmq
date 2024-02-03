// Copyright 2014-2023 Bloomberg Finance L.P.
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

// m_bmqstoragetool_searchprocessor.cpp -*-C++-*-

// bmqstoragetool
#include <m_bmqstoragetool_searchprocessor.h>
#include <m_bmqstoragetool_searchresult.h>

// BDE
#include <bdls_filesystemutil.h>
#include <bsl_iostream.h>
#include <bsls_assert.h>

// MQB
#include <mqbs_filestoreprotocolprinter.h>
#include <mqbs_filestoreprotocolutil.h>
#include <mqbs_filesystemutil.h>
#include <mqbs_offsetptr.h>

// MWC
#include <mwcu_alignedprinter.h>
#include <mwcu_memoutstream.h>
#include <mwcu_outstreamformatsaver.h>
#include <mwcu_stringutil.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

namespace {

/// Move the journal iterator pointed by the specified 'it' to the first
/// message whose timestamp is more then the specified 'timestamp'.  Returns
/// '1' on success, '0" if there are no such records (also the iterator is
/// invalidated).  Behavior is undefined unless last call to `nextRecord`
/// returned 1 and the iterator points to a valid record.
static int moveToLowerBound(mqbs::JournalFileIterator* it,
                            const bsls::Types::Uint64& timestamp)
{
    int                rc         = 1;
    const unsigned int recordSize = it->header().recordWords() *
                                    bmqp::Protocol::k_WORD_SIZE;
    const bsls::Types::Uint64 recordsNumber = (it->lastRecordPosition() -
                                               it->firstRecordPosition()) /
                                              recordSize;
    bsls::Types::Uint64 left  = 0;
    bsls::Types::Uint64 right = recordsNumber;
    while (right > left + 1) {
        const bool goBackwards = it->recordHeader().timestamp() > timestamp;
        if (goBackwards != it->isReverseMode()) {
            it->flipDirection();
        }
        if (goBackwards) {
            if (it->recordIndex() == 0) {
                break;
            }
            right = it->recordIndex();
        }
        else {
            if (it->recordIndex() == recordsNumber) {
                break;
            }
            left = it->recordIndex();
        }
        rc = it->advance(bsl::max((right - left) / 2, 1ULL));
        if (rc != 1) {
            return rc;  // RETURN
        }
    }
    if (it->isReverseMode()) {
        it->flipDirection();
    }
    if (it->recordHeader().timestamp() <= timestamp) {
        if (it->recordIndex() < recordsNumber) {
            rc = it->nextRecord();
        }
        else {
            // It's the last record, so there are no messages with timestamp
            // greater than the specified 'ts' in the file.
            rc = 0;
        }
    }

    return rc;  // RETURN
}

}  // close unnamed namespace

// =====================
// class SearchProcessor
// =====================

// CREATORS

SearchProcessor::SearchProcessor(bsl::unique_ptr<Parameters> params,
                                 bslma::Allocator*           allocator)
: CommandProcessor(bsl::move(params))
, d_allocator_p(bslma::Default::allocator(allocator))
{
    // NOTHING
}

void SearchProcessor::process(bsl::ostream& ostream)
{
    Filters filters(d_parameters->queueKey(),
                    d_parameters->queueName(),
                    d_parameters->queueMap(),
                    d_parameters->timestampGt(),
                    d_parameters->timestampLt(),
                    ostream,
                    d_allocator_p);

    bsl::shared_ptr<PayloadDumper> payloadDumper;
    if (d_parameters->dumpPayload())
        payloadDumper.reset(new (*d_allocator_p)
                                PayloadDumper(ostream,
                                              d_parameters->dataFileIterator(),
                                              d_parameters->dumpLimit()),
                            d_allocator_p);

    // TODO: consider to introduce SearchResultFactory and move all logic there
    // TODO: why unique_ptr doesn't support deleter in reset()
    // bsl::unique_ptr<SearchResult> searchResult_p;
    bsl::shared_ptr<SearchResult> searchResult_p;
    if (!d_parameters->guid().empty()) {
        // searchResult_p.reset(new (*d_allocator_p) SearchGuidResult(
        //                          ostream,
        //                          d_parameters->details(),
        //                          d_parameters->dumpPayload(),
        //                          d_parameters->dumpLimit(),
        //                          d_parameters->dataFileIterator(),
        //                          d_parameters->queueMap(),
        //                          d_parameters->guid(),
        //                          filters,
        //                          d_allocator_p),
        //                      d_allocator_p);
        if (d_parameters->details()) {
            // Base: Details
            searchResult_p.reset(new (*d_allocator_p) SearchDetailResult(
                                     ostream,
                                     //  filters,
                                     d_parameters->queueMap(),
                                     payloadDumper,
                                     d_allocator_p,
                                     true,
                                     true,
                                     true),
                                 d_allocator_p);
        }
        else {
            // Base: Short
            searchResult_p.reset(new (*d_allocator_p)
                                     SearchShortResult(ostream,
                                                       payloadDumper,
                                                       d_allocator_p,
                                                       true,
                                                       true,
                                                       true),
                                 d_allocator_p);
        }
        // Decorator
        searchResult_p.reset(new (*d_allocator_p)
                                 SearchGuidDecorator(searchResult_p,
                                                     d_parameters->guid(),
                                                     ostream,
                                                     d_parameters->details(),
                                                     d_allocator_p),
                             d_allocator_p);
    }
    else if (d_parameters->summary()) {
        // searchResult_p.reset(new (*d_allocator_p) SearchSummaryResult(
        //                          ostream,
        //                          d_parameters->journalFileIterator(),
        //                          d_parameters->dataFileIterator(),
        //                          d_parameters->queueMap(),
        //                          filters,
        //                          d_allocator_p),
        //                      d_allocator_p);
        searchResult_p.reset(new (*d_allocator_p) SummaryProcessor(
                                 ostream,
                                 d_parameters->journalFileIterator(),
                                 d_parameters->dataFileIterator(),
                                 d_allocator_p),
                             d_allocator_p);
    }
    else if (d_parameters->outstanding()) {
        // searchResult_p.reset(new (*d_allocator_p) SearchOutstandingResult(
        //                          ostream,
        //                          d_parameters->details(),
        //                          d_parameters->dumpPayload(),
        //                          d_parameters->dumpLimit(),
        //                          d_parameters->dataFileIterator(),
        //                          d_parameters->queueMap(),
        //                          filters,
        //                          d_allocator_p),
        //                      d_allocator_p);

        if (d_parameters->details()) {
            // Base: Details
            searchResult_p.reset(new (*d_allocator_p) SearchDetailResult(
                                     ostream,
                                     //  filters,
                                     d_parameters->queueMap(),
                                     payloadDumper,
                                     d_allocator_p,
                                     false,
                                     true,
                                     false),
                                 d_allocator_p);
        }
        else {
            // Base: Short
            searchResult_p.reset(new (*d_allocator_p)
                                     SearchShortResult(ostream,
                                                       payloadDumper,
                                                       d_allocator_p,
                                                       false,
                                                       false,
                                                       true),
                                 d_allocator_p);
        }
        // Decorator
        searchResult_p.reset(new (*d_allocator_p)
                                 SearchOutstandingDecorator(searchResult_p,
                                                            ostream,
                                                            d_allocator_p),
                             d_allocator_p);
    }
    else if (d_parameters->confirmed()) {
        // searchResult_p.reset(new (*d_allocator_p) SearchConfirmedResult(
        //                          ostream,
        //                          d_parameters->details(),
        //                          d_parameters->dumpPayload(),
        //                          d_parameters->dumpLimit(),
        //                          d_parameters->dataFileIterator(),
        //                          d_parameters->queueMap(),
        //                          filters,
        //                          d_allocator_p),
        //                      d_allocator_p);

        if (d_parameters->details()) {
            // Base: Details
            searchResult_p.reset(new (*d_allocator_p) SearchDetailResult(
                                     ostream,
                                     //  filters,
                                     d_parameters->queueMap(),
                                     payloadDumper,
                                     d_allocator_p,
                                     true,
                                     true,
                                     true),
                                 d_allocator_p);
        }
        else {
            // Base: Short
            searchResult_p.reset(new (*d_allocator_p)
                                     SearchShortResult(ostream,
                                                       payloadDumper,
                                                       d_allocator_p,
                                                       false,
                                                       true,
                                                       true),
                                 d_allocator_p);
        }
        // Decorator
        searchResult_p.reset(new (*d_allocator_p)
                                 SearchOutstandingDecorator(searchResult_p,
                                                            ostream,
                                                            d_allocator_p),
                             d_allocator_p);
    }
    else if (d_parameters->partiallyConfirmed()) {
        // searchResult_p.reset(new (*d_allocator_p)
        //                          SearchPartiallyConfirmedResult(
        //                              ostream,
        //                              d_parameters->details(),
        //                              d_parameters->dumpPayload(),
        //                              d_parameters->dumpLimit(),
        //                              d_parameters->dataFileIterator(),
        //                              d_parameters->queueMap(),
        //                              filters,
        //                              d_allocator_p),
        //                      d_allocator_p);
        if (d_parameters->details()) {
            // Base: Details
            searchResult_p.reset(new (*d_allocator_p) SearchDetailResult(
                                     ostream,
                                     //  filters,
                                     d_parameters->queueMap(),
                                     payloadDumper,
                                     d_allocator_p,
                                     false,
                                     true,
                                     false),
                                 d_allocator_p);
        }
        else {
            // Base: Short
            searchResult_p.reset(new (*d_allocator_p)
                                     SearchShortResult(ostream,
                                                       payloadDumper,
                                                       d_allocator_p,
                                                       false,
                                                       false,
                                                       true),
                                 d_allocator_p);
        }
        // Decorator
        searchResult_p.reset(
            new (*d_allocator_p)
                SearchPartiallyConfirmedDecorator(searchResult_p,
                                                  ostream,
                                                  d_allocator_p),
            d_allocator_p);
    }
    else {
        // searchResult_p.reset(new (*d_allocator_p)
        //                         SearchAllResult(ostream,
        //                                         d_parameters->details(),
        //                                         d_parameters->dumpPayload(),
        //                                         d_parameters->dumpLimit(),
        //                                         d_parameters->dataFile(),
        //                                         d_parameters->queueMap(),
        //                                         filters,
        //                                         d_allocator_p),
        //                     d_allocator_p);
        if (d_parameters->details()) {
            // Base: Details
            searchResult_p.reset(new (*d_allocator_p) SearchDetailResult(
                                     ostream,
                                     d_parameters->queueMap(),
                                     payloadDumper,
                                     d_allocator_p),
                                 d_allocator_p);
        }
        else {
            // Base: Short
            searchResult_p.reset(new (*d_allocator_p)
                                     SearchShortResult(ostream,
                                                       //  filters,
                                                       payloadDumper,
                                                       d_allocator_p),
                                 d_allocator_p);
        }
        // Decorator
        searchResult_p.reset(new (*d_allocator_p)
                                 SearchAllDecorator(searchResult_p),
                             d_allocator_p);
    }
    if (d_parameters->timestampLt() > 0) {
        searchResult_p.reset(
            new (*d_allocator_p)
                SearchResultTimestampDecorator(searchResult_p,
                                               d_parameters->timestampLt()),
            d_allocator_p);
    }
    BSLS_ASSERT(searchResult_p);

    bool stopSearch          = false;
    bool needTimestampSearch = d_parameters->timestampGt() > 0;

    // Iterate through all Journal file records
    mqbs::JournalFileIterator* iter = d_parameters->journalFileIterator();
    while (true) {
        if (stopSearch || !iter->hasRecordSizeRemaining()) {
            searchResult_p->outputResult();
            return;  // RETURN
        }
        int rc = iter->nextRecord();
        if (rc <= 0) {
            ostream << "Iteration aborted (exit status " << rc << ").";
            return;  // RETURN
        }
        if (needTimestampSearch) {
            rc = moveToLowerBound(iter, d_parameters->timestampGt());
            if (rc == 0) {
                stopSearch = true;
                continue;
            }
            else if (rc < 0) {
                ostream << "Binary search by timesamp aborted (exit status "
                        << rc << ").";
                return;  // RETURN
            }
            needTimestampSearch = false;
        }
        // MessageRecord
        if (iter->recordType() == mqbs::RecordType::e_MESSAGE) {
            const mqbs::MessageRecord& record = iter->asMessageRecord();
            // Apply filters
            if (filters.apply(record)) {
                stopSearch = searchResult_p->processMessageRecord(
                    record,
                    iter->recordIndex(),
                    iter->recordOffset());
            }
        }
        // ConfirmRecord
        else if (iter->recordType() == mqbs::RecordType::e_CONFIRM) {
            const mqbs::ConfirmRecord& record = iter->asConfirmRecord();
            stopSearch = searchResult_p->processConfirmRecord(
                record,
                iter->recordIndex(),
                iter->recordOffset());
        }
        // DeletionRecord
        else if (iter->recordType() == mqbs::RecordType::e_DELETION) {
            const mqbs::DeletionRecord& record = iter->asDeletionRecord();
            stopSearch = searchResult_p->processDeletionRecord(
                record,
                iter->recordIndex(),
                iter->recordOffset());
        }
    }
}

}  // close package namespace
}  // close enterprise namespace
