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

/// Move the journal iterator pointed by the specified 'it' FORWARD to the
/// first message whose timestamp is more then the specified 'timestamp'.
/// Behavior is undefined unless last call to `nextRecord` returned 1 and the
/// iterator points to the first record.
static int moveToLowerBound(mqbs::JournalFileIterator* it,
                            const bsls::Types::Uint64& timestamp)
{
    int rc = 0;
    const unsigned int recordSize = it->header().recordWords() *
                                    bmqp::Protocol::k_WORD_SIZE;
    bsls::Types::Uint64 left  = it->recordIndex();
    bsls::Types::Uint64 right = (it->lastRecordPosition() -
                                 it->firstRecordPosition()) /
                                recordSize;
    while (right > left + 1) {
        rc = it->advance((right - left) / 2);
        if (rc != 1) {
            return rc;
        }
        const bool goBackwards = it->recordHeader().timestamp() > timestamp;
        if (goBackwards != it->isReverseMode()) {
            it->flipDirection();
        }
        if (goBackwards) {
            right = it->recordIndex();
        } else {
            left = it->recordIndex();
        }

    }
    if (it->isReverseMode()) {
        it->flipDirection();
    }
    if (it->recordHeader().timestamp() <= timestamp) {
        it->nextRecord();
    }

//    {
//        // Debug information
//        bsl::cout << "Found :\n"
//                  << "record index : " << it->recordIndex() << "\n"
//                  << "   timestamp : " << it->recordHeader().timestamp()
//                  << bsl::endl;
//        it->flipDirection();
//        it->nextRecord();
//        bsl::cout << "Previous :\n"
//                  << "record index : " << it->recordIndex() << "\n"
//                  << "   timestamp : " << it->recordHeader().timestamp()
//                  << bsl::endl;
//        it->flipDirection();
//        it->nextRecord();
//    }

    return rc;
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

    // TODO: consider to introduce SearchResultFactory and move all logic there
    // TODO: why unique_ptr doesn't support deleter in reset()
    // bsl::unique_ptr<SearchResult> searchResult_p;
    bsl::shared_ptr<SearchResult> searchResult_p;
    if (!d_parameters->guid().empty()) {
        searchResult_p.reset(new (*d_allocator_p)
                                 SearchGuidResult(ostream,
                                                  d_parameters->details(),
                                                  d_parameters->dumpPayload(),
                                                  d_parameters->dumpLimit(),
                                                  d_parameters->dataFile(),
                                                  d_parameters->queueMap(),
                                                  d_parameters->guid(),
                                                  filters,
                                                  d_allocator_p),
                             d_allocator_p);
    }
    else if (d_parameters->outstanding()) {
        searchResult_p.reset(new (*d_allocator_p) SearchOutstandingResult(
                                 ostream,
                                 d_parameters->details(),
                                 d_parameters->dumpPayload(),
                                 d_parameters->dumpLimit(),
                                 d_parameters->dataFile(),
                                 d_parameters->queueMap(),
                                 filters,
                                 d_allocator_p),
                             d_allocator_p);
    }
    else if (d_parameters->confirmed()) {
        searchResult_p.reset(new (*d_allocator_p) SearchConfirmedResult(
                                 ostream,
                                 d_parameters->details(),
                                 d_parameters->dumpPayload(),
                                 d_parameters->dumpLimit(),
                                 d_parameters->dataFile(),
                                 d_parameters->queueMap(),
                                 filters,
                                 d_allocator_p),
                             d_allocator_p);
    }
    else if (d_parameters->partiallyConfirmed()) {
        searchResult_p.reset(
            new (*d_allocator_p)
                SearchPartiallyConfirmedResult(ostream,
                                               d_parameters->details(),
                                               d_parameters->dumpPayload(),
                                               d_parameters->dumpLimit(),
                                               d_parameters->dataFile(),
                                               d_parameters->queueMap(),
                                               filters,
                                               d_allocator_p),
            d_allocator_p);
    }
    else {
        searchResult_p.reset(new (*d_allocator_p)
                                 SearchAllResult(ostream,
                                                 d_parameters->details(),
                                                 d_parameters->dumpPayload(),
                                                 d_parameters->dumpLimit(),
                                                 d_parameters->dataFile(),
                                                 d_parameters->queueMap(),
                                                 filters,
                                                 d_allocator_p),
                             d_allocator_p);
    }
    BSLS_ASSERT(searchResult_p);

    bool stopSearch = false;
    bool needTimestampSearch = d_parameters->timestampGt() > 0;

    // Iterate through all Journal file records
    mqbs::JournalFileIterator* iter = d_parameters->journalFile()->iterator();
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
            if (rc <= 0) {
                ostream << "Binary search by timesamp aborted (exit status "
                        << rc << ").";
                return;  // RETURN
            }
            needTimestampSearch = false;
        }
        // MessageRecord
        if (iter->recordType() == mqbs::RecordType::e_MESSAGE) {
            const mqbs::MessageRecord& record = iter->asMessageRecord();
            stopSearch = searchResult_p->processMessageRecord(
                record,
                iter->recordIndex(),
                iter->recordOffset());
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
