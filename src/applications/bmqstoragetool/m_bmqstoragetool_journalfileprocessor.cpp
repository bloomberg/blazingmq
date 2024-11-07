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
#include "m_bmqstoragetool_compositesequencenumber.h"
#include "m_bmqstoragetool_parameters.h"
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

namespace {

bsls::Types::Uint64 getValue(const mqbs::JournalFileIterator* jit, const Parameters::SearchValueType valueType)
{
    // PRECONDITIONS
    BSLS_ASSERT(jit);
    BSLS_ASSERT(valueType == Parameters::e_TIMESTAMP || valueType == Parameters::e_OFFSET);

    return (valueType == Parameters::e_TIMESTAMP) ? jit->recordHeader().timestamp() : jit->recordOffset();
}

template<typename T>
T getValue_1(const mqbs::JournalFileIterator* jit, const Parameters::SearchValueType valueType)
{
    // PRECONDITIONS
    BSLS_ASSERT(jit);
    BSLS_ASSERT(valueType == Parameters::e_TIMESTAMP || valueType == Parameters::e_OFFSET);

    return (valueType == Parameters::e_TIMESTAMP) ? jit->recordHeader().timestamp() : jit->recordOffset();
}

template<>
CompositeSequenceNumber getValue_1<CompositeSequenceNumber>(const mqbs::JournalFileIterator* jit, const Parameters::SearchValueType valueType)
{
    // PRECONDITIONS
    BSLS_ASSERT(jit);
    BSLS_ASSERT(valueType != Parameters::e_NONE);

    return CompositeSequenceNumber(jit->recordHeader().primaryLeaseId(), jit->recordHeader().sequenceNumber());
}

}  // close unnamed namespace

/// Move the journal iterator pointed by the specified 'jit' to the first
/// record whose value (timestamp or offset) is more then the specified 'valueGt'.  Return '1'
/// on success, '0' if there are no such records or negative value if an error
/// was encountered.  Note that if this method returns < 0, the specified 'jit'
/// is invalidated.  Behavior is undefined unless last call to `nextRecord` or
/// 'advance' returned '1' and the iterator points to a valid record.
int moveToLowerBound(mqbs::JournalFileIterator* jit,
                     const Parameters::SearchValueType valueType,
                     const bsls::Types::Uint64& valueGt)
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
        const bool goBackwards = getValue(jit, valueType) > valueGt;
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
    if (getValue(jit, valueType) <= valueGt) {
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

template<typename T>
int moveToLower(mqbs::JournalFileIterator* jit,
                     const Parameters::SearchValueType valueType,
                     const T& valueGt)
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
        const bool goBackwards = valueGt < getValue_1<T>(jit, valueType);
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
    if (getValue_1<T>(jit, valueType) <= valueGt) {
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

int moveToLowerSeqNumber(mqbs::JournalFileIterator* jit,
                     const CompositeSequenceNumber& seqNumGt)
{
    // PRECONDITIONS
    BSLS_ASSERT(jit);
    BSLS_ASSERT(!seqNumGt.isUnset());

    int                rc         = 1;
    const unsigned int recordSize = jit->header().recordWords() *
                                    bmqp::Protocol::k_WORD_SIZE;
    const bsls::Types::Uint64 recordsNumber = (jit->lastRecordPosition() -
                                               jit->firstRecordPosition()) /
                                              recordSize;
    bsls::Types::Uint64 left  = 0;
    bsls::Types::Uint64 right = recordsNumber;
    while (right > left + 1) {
        CompositeSequenceNumber seqNum(jit->recordHeader().primaryLeaseId(), jit->recordHeader().sequenceNumber());
        const bool goBackwards = seqNumGt < seqNum;
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
    CompositeSequenceNumber seqNum(jit->recordHeader().primaryLeaseId(), jit->recordHeader().sequenceNumber());
    if (seqNum <= seqNumGt) {
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

int moveToLowerSeqNumber_2(mqbs::JournalFileIterator* jit,
                     const CompositeSequenceNumber& seqNumGt)
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

    bsl::cout << "right_1: " << right << "\n";
    
    // First, find required leaseId range

    // Check edge condition to skip search left edge
    if (seqNumGt.leaseId() == 1) {
        right = left;
    }
    unsigned int leftLeaseId = bsl::max(seqNumGt.leaseId() - 1, 1u);
    // unsigned int rightLeaseId = seqNumGt.leaseId() + 1;

    ////////////////////////////////////////////////////////////////////////
    // Find left position
    while (right > left + 1) {
        // const bool goBackwards = jit->recordHeader().primaryLeaseId() > seqNumGt.leaseId();
        const bool goBackwards = jit->recordHeader().primaryLeaseId() > leftLeaseId;
        bsl::cout << "goBackwards: " << jit->recordHeader().primaryLeaseId() << " : " << seqNumGt.leaseId() << "\n";

        if (goBackwards != jit->isReverseMode()) {
            jit->flipDirection();
        }
        if (goBackwards) {
            if (jit->recordIndex() == 0) {
                break;  // BREAK
            }
            right = jit->recordIndex();
            bsl::cout << "right: " << right << "\n";
        }
        else {
            if (jit->recordIndex() == recordsNumber) {
                break;  // BREAK
            }
            left = jit->recordIndex();
            bsl::cout << "left: " << left << "\n";
        }
        rc = jit->advance(bsl::max((right - left) / 2, 1ULL));
        if (rc != 1) {
            return rc;  // RETURN
        }
    }

    bsl::cout << "Found Left index: " << jit->recordIndex() << "\n";
    bsl::cout << jit->recordHeader() << '\n';
    
    if (jit->isReverseMode()) {
        jit->flipDirection();
    }

    // Move inside range
    if (jit->recordHeader().primaryLeaseId() < seqNumGt.leaseId()) {
        if (jit->recordIndex() < recordsNumber) {
            rc = jit->nextRecord();
            if (rc != 1) {
                return rc;  // RETURN
            }
        }
        else {
            // TODO
            return 0;  // RETURN
        } 
    }

    left = jit->recordIndex();

    bsls::Types::Uint64 foundLeft = left;
    right = recordsNumber;

    ////////////////////////////////////////////////////////////////////////
    // Search right
    while (right > left + 1) {
        const bool goBackwards = jit->recordHeader().primaryLeaseId() > seqNumGt.leaseId();
        // const bool goBackwards = jit->recordHeader().primaryLeaseId() > leftLeaseId;
        bsl::cout << "goBackwards: " << jit->recordHeader().primaryLeaseId() << " : " << seqNumGt.leaseId() << "\n";

        if (goBackwards != jit->isReverseMode()) {
            jit->flipDirection();
        }
        if (goBackwards) {
            if (jit->recordIndex() == 0) {
                break;  // BREAK
            }
            right = jit->recordIndex();
            bsl::cout << "right: " << right << "\n";
        }
        else {
            if (jit->recordIndex() == recordsNumber) {
                break;  // BREAK
            }
            left = jit->recordIndex();
            bsl::cout << "left: " << left << "\n";
        }
        rc = jit->advance(bsl::max((right - left) / 2, 1ULL));
        if (rc != 1) {
            return rc;  // RETURN
        }
    }

    bsl::cout << "Found Right index: " << jit->recordIndex() << "\n";
    bsl::cout << jit->recordHeader() << '\n';

    // Move inside range
    if (jit->recordHeader().primaryLeaseId() > seqNumGt.leaseId()) {

        if (jit->recordIndex() != 0) {
            if (!jit->isReverseMode()) {
                jit->flipDirection();
            }
            rc = jit->nextRecord();
            if (rc != 1) {
                return rc;  // RETURN
            }
        }
        else {
            // TODO
            return 0;  // RETURN
        } 
    }
    
    bsl::cout << "Final Right: " << jit->recordHeader() << '\n';
    
    if (jit->isReverseMode()) {
        jit->flipDirection();
    }

    // Check edge case (right position)
    if (jit->recordHeader().sequenceNumber() == seqNumGt.sequenceNumber())
    {
        // Move to next record
        if (jit->recordIndex() < recordsNumber) {
            // if (!jit->isReverseMode()) {
            //     jit->flipDirection();
            // }
            rc = jit->nextRecord();
            bsl::cout << "Edge case Right: " << jit->recordHeader() << '\n';
            return rc;  // RETURN
        }
        else {
            // TODO
            return 0;  // RETURN
        } 
    }

    ////////////////////////////////////////////////////////////////////////
    // Search inside leaqseId
    left = foundLeft;
    right = jit->recordIndex();
    while (right > left + 1) {
        const bool goBackwards = jit->recordHeader().sequenceNumber() > seqNumGt.sequenceNumber();
        bsl::cout << "goBackwards: " << jit->recordHeader().sequenceNumber() << " : " << seqNumGt.sequenceNumber() << "\n";

        if (goBackwards != jit->isReverseMode()) {
            jit->flipDirection();
        }
        if (goBackwards) {
            if (jit->recordIndex() == 0) {
                break;  // BREAK
            }
            right = jit->recordIndex();
            bsl::cout << "right: " << right << "\n";
        }
        else {
            if (jit->recordIndex() == recordsNumber) {
                break;  // BREAK
            }
            left = jit->recordIndex();
            bsl::cout << "left: " << left << "\n";
        }
        rc = jit->advance(bsl::max((right - left) / 2, 1ULL));
        if (rc != 1) {
            return rc;  // RETURN
        }
    }

    bsl::cout << "Found index: " << jit->recordIndex() << "\n";
    bsl::cout << jit->recordHeader() << '\n';
    
    if (jit->isReverseMode()) {
        jit->flipDirection();
    }
    if (jit->recordHeader().sequenceNumber() <= seqNumGt.sequenceNumber()) {
    // if (getValue(jit, valueType) <= valueGt) {
        if (jit->recordIndex() < recordsNumber) {
            rc = jit->nextRecord();
        }
        else {
            // It's the last record, so there are no messages with timestamp
            // greater than the specified 'timestamp' in the file.
            rc = 0;
        }
    }

    return rc;
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
                    d_parameters->d_valueType,
                    d_parameters->d_valueGt,
                    d_parameters->d_valueLt,
                    d_parameters->d_seqNumGt,
                    d_parameters->d_seqNumLt,
                    d_allocator_p);

    bool stopSearch          = false;
    bool needMoveToLowerBound = d_parameters->d_valueGt > 0 || !d_parameters->d_seqNumGt.isUnset();

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

        if (needMoveToLowerBound) {
            // rc = moveToLowerBound(iter, d_parameters->d_valueType, d_parameters->d_valueGt);
            if (d_parameters->d_valueType == Parameters::e_SEQUENCE_NUM) {
                rc = moveToLower<CompositeSequenceNumber>(iter, d_parameters->d_valueType, d_parameters->d_seqNumGt);
            } else {
                rc = moveToLower<bsls::Types::Uint64>(iter, d_parameters->d_valueType, d_parameters->d_valueGt);
            }
            if (rc == 0) {
                stopSearch = true;
                continue;  // CONTINUE
            }
            else if (rc < 0) {
                d_ostream << "Binary search by timesamp aborted (exit status "
                          << rc << ").";
                return;  // RETURN
            }
            needMoveToLowerBound = false;
        }

        bsl::cout << iter->recordHeader() << '\n';
        // continue;
        
        // MessageRecord
        if (iter->recordType() == mqbs::RecordType::e_MESSAGE) {
            const mqbs::MessageRecord& record = iter->asMessageRecord();

            // Apply filters
            if (filters.apply(record, iter->recordOffset())) {
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
