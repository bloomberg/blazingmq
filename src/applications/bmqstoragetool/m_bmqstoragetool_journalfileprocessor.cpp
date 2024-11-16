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

// BMQ
#include <bmqu_alignedprinter.h>
#include <bmqu_memoutstream.h>
#include <bmqu_outstreamformatsaver.h>
#include <bmqu_stringutil.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

/// Move the journal iterator pointed by the specified 'jit' to the first
/// message whose timestamp is more then the specified 'timestamp'.  Return '1'
/// on success, '0' if there are no such records or negative value if an error
/// was encountered.  Note that if this method returns < 0, the specified 'jit'
/// is invalidated.  Behavior is undefined unless last call to `nextRecord` or
/// 'advance' returned '1' and the iterator points to a valid record.
int moveToLowerBound(mqbs::JournalFileIterator* jit,
                     const bsls::Types::Uint64& timestamp)
{
    // PRECONDITIONS
    BSLS_ASSERT(jit);

    int                rc         = 1;
    const unsigned int recordSize = jit->header().recordWords() *
                                    bmqp::Protocol::k_WORD_SIZE;
    const bsls::Types::Uint64 recordsNumber = (jit->lastRecordPosition() -
                                               jit->firstRecordPosition()) /
                                              recordSize;
    bsls::Types::Uint64 left  = 0;
    bsls::Types::Uint64 right = recordsNumber;
    while (right > left + 1) {
        const bool goBackwards = jit->recordHeader().timestamp() > timestamp;
        if (goBackwards != jit->isReverseMode()) {
            jit->flipDirection();
        }
        if (goBackwards) {
            if (jit->recordIndex() == 0) {
                break;  // BREAK
            }
            right = jit->recordIndex();
        }
        else {
            if (jit->recordIndex() == recordsNumber) {
                break;  // BREAK
            }
            left = jit->recordIndex();
        }
        rc = jit->advance(bsl::max((right - left) / 2, 1ULL));
        if (rc != 1) {
            return rc;  // RETURN
        }
    }
    if (jit->isReverseMode()) {
        jit->flipDirection();
    }
    if (jit->recordHeader().timestamp() <= timestamp) {
        if (jit->recordIndex() < recordsNumber) {
            rc = jit->nextRecord();
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
    bslma::ManagedPtr<FileManager>&      fileManager,
    const bsl::shared_ptr<SearchResult>& searchResult_p,
    bsl::ostream&                        ostream,
    bslma::Allocator*                    allocator)
: d_parameters(params)
, d_fileManager(fileManager)
, d_ostream(ostream)
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
                continue;  // CONTINUE
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
        } else {
            stopSearch = d_searchResult_p->processOtherRecord(
                iter->recordType());
        }
    }
}

}  // close package namespace
}  // close enterprise namespace
