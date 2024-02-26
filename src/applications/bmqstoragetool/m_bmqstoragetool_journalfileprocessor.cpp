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

// bmqstoragetool
#include <m_bmqstoragetool_journalfileprocessor.h>

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

/// Move the journal iterator pointed by the specified 'it' to the first
/// message whose timestamp is more then the specified 'timestamp'.  Returns
/// '1' on success, '0" if there are no such records (also the iterator is
/// invalidated).  Behavior is undefined unless last call to `nextRecord`
/// returned 1 and the iterator points to a valid record.
int moveToLowerBound(mqbs::JournalFileIterator* it,
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
            // greater than the specified 'timestamp' in the file.
            rc = 0;
        }
    }

    return rc;  // RETURN
}

// ==========================
// class JournalFileProcessor
// ==========================

// CREATORS

JournalFileProcessor::JournalFileProcessor(
    const Parameters*                    params,
    const bsl::shared_ptr<FileManager>&  fileManager,
    const bsl::shared_ptr<SearchResult>& searchResult_p,
    bsl::ostream&                        ostream,
    bslma::Allocator*                    allocator)
: CommandProcessor(params, fileManager, ostream)
, d_searchResult_p(searchResult_p)
, d_allocator_p(bslma::Default::allocator(allocator))
{
    // NOTHING
}

void JournalFileProcessor::process()
{
    Filters filters(d_parameters->d_queueKey,
                    d_parameters->d_queueName,
                    d_parameters->d_queueMap,
                    d_parameters->d_timestampGt,
                    d_parameters->d_timestampLt,
                    d_ostream,
                    d_allocator_p);

    bool stopSearch          = false;
    bool needTimestampSearch = d_parameters->d_timestampGt > 0;

    // Iterate through all Journal file records
    mqbs::JournalFileIterator* iter = d_fileManager->journalFileIterator();
    while (true) {
        if (stopSearch || !iter->hasRecordSizeRemaining()) {
            d_searchResult_p->outputResult();
            return;  // RETURN
        }
        int rc = iter->nextRecord();
        if (rc <= 0) {
            d_ostream << "Iteration aborted (exit status " << rc << ").";
            return;  // RETURN
        }
        if (needTimestampSearch) {
            rc = moveToLowerBound(iter, d_parameters->d_timestampGt);
            if (rc == 0) {
                stopSearch = true;
                continue;
            }
            else if (rc < 0) {
                d_ostream << "Binary search by timesamp aborted (exit status "
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
                stopSearch = d_searchResult_p->processMessageRecord(
                    record,
                    iter->recordIndex(),
                    iter->recordOffset());
            }
        }
        // ConfirmRecord
        else if (iter->recordType() == mqbs::RecordType::e_CONFIRM) {
            const mqbs::ConfirmRecord& record = iter->asConfirmRecord();
            stopSearch = d_searchResult_p->processConfirmRecord(
                record,
                iter->recordIndex(),
                iter->recordOffset());
        }
        // DeletionRecord
        else if (iter->recordType() == mqbs::RecordType::e_DELETION) {
            const mqbs::DeletionRecord& record = iter->asDeletionRecord();
            stopSearch = d_searchResult_p->processDeletionRecord(
                record,
                iter->recordIndex(),
                iter->recordOffset());
        }
    }
}

}  // close package namespace
}  // close enterprise namespace
