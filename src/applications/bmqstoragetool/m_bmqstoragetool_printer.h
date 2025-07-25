// Copyright 2025 Bloomberg Finance L.P.
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

#ifndef INCLUDED_M_BMQSTORAGETOOL_PRINTER_H
#define INCLUDED_M_BMQSTORAGETOOL_PRINTER_H

//@PURPOSE: Provide Printer class for printing storage files.
//
//@CLASSES:
//  Printer: provides methods to print storage files.
//
//@DESCRIPTION: Interface class to print storage files.

// bmqstoragetool
#include <m_bmqstoragetool_messagedetails.h>
#include <m_bmqstoragetool_parameters.h>

// BMQ
#include <bmqt_messageguid.h>

// BDE
#include <bsl_cstddef.h>
#include <bsl_list.h>
#include <bsl_map.h>
#include <bsl_memory.h>
#include <bsl_vector.h>
#include <bslma_managedptr.h>

namespace BloombergLP {

namespace m_bmqstoragetool {

// =============
// class Printer
// =============

class Printer {
  protected:
    // PROTECTED TYPES

    /// List of message guids.
    typedef bsl::list<bmqt::MessageGUID> GuidsList;
    /// Queue operations count vector.
    typedef bsl::vector<bsls::Types::Uint64> QueueOpCountsVec;
    /// Offsets vector.
    typedef bsl::vector<bsls::Types::Int64> OffsetsVec;
    /// Composite sequence numbers vector.
    typedef bsl::vector<CompositeSequenceNumber> CompositesVec;

  public:
    // CREATORS

    virtual ~Printer();

    // PUBLIC METHODS

    /// Print the specified message details `details`.
    virtual void printMessage(const MessageDetails& details) const = 0;

    /// Print the specified Confirm record `record`.
    virtual void printConfirmRecord(
        const RecordDetails<mqbs::ConfirmRecord>& record) const = 0;

    /// Print the specified Confirm record `record`.
    virtual void printDeletionRecord(
        const RecordDetails<mqbs::DeletionRecord>& record) const = 0;

    /// Print the specified queueOp record `record`.
    virtual void printQueueOpRecord(
        const RecordDetails<mqbs::QueueOpRecord>& record) const = 0;

    /// Print the specified journalOp record `record`.
    virtual void printJournalOpRecord(
        const RecordDetails<mqbs::JournalOpRecord>& record) const = 0;

    /// Print the specified message guid `guid`.
    virtual void printGuid(const bmqt::MessageGUID& guid) const = 0;

    /// Print not found message guid `guid`.
    virtual void printGuidNotFound(const bmqt::MessageGUID& guid) const = 0;

    /// Print not found message guids `guids`.
    virtual void printGuidsNotFound(const GuidsList& guids) const = 0;

    /// Print not found message offsets `offsets`.
    virtual void printOffsetsNotFound(const OffsetsVec& offsets) const = 0;

    /// Print not found composite sequence numbers `seqNums`.
    virtual void
    printCompositesNotFound(const CompositesVec& seqNums) const = 0;

    /// Print footer with the specified numbers `foundMessagesCount`,
    /// `foundQueueOpCount` and `foundJournalOpCount` if the corresponding
    /// flags are set in the specified `processRecordTypes`.
    virtual void printFooter(
        bsls::Types::Uint64                   foundMessagesCount,
        bsls::Types::Uint64                   foundQueueOpCount,
        bsls::Types::Uint64                   foundJournalOpCount,
        const Parameters::ProcessRecordTypes& processRecordTypes) const = 0;

    /// Print footer with the specified numbers `foundMessagesCount`, `foundConfirmCount`, `foundDeletionCount`,
    /// `foundQueueOpCount` and `foundJournalOpCount` if the corresponding
    /// flags are set in the specified `processRecordTypes`.
    virtual void printFooter(
        bsls::Types::Uint64                   foundMessagesCount,
        bsls::Types::Uint64                   foundConfirmCount,
        bsls::Types::Uint64                   foundDeletionCount,
        bsls::Types::Uint64                   foundQueueOpCount,
        bsls::Types::Uint64                   foundJournalOpCount,
        const Parameters::ProcessRecordTypes& processRecordTypes) const = 0;

    /// Print the outstanding ratio.
    virtual void
    printOutstandingRatio(int         ratio,
                          bsl::size_t outstandingMessagesCount,
                          bsl::size_t totalMessagesCount) const = 0;

    /// Print summary of all messages including the `totalMessagesCount`, the
    /// `partiallyConfirmedCount`, the `confirmedCount` and the
    /// `outstandingCount`.
    virtual void printMessageSummary(bsl::size_t totalMessagesCount,
                                     bsl::size_t partiallyConfirmedCount,
                                     bsl::size_t confirmedCount,
                                     bsl::size_t outstandingCount) const = 0;

    /// Print summary of all queue operations including number of all queueOp
    /// records `queueOpRecordsCount` and the number of records of each QueueOp
    /// type from the `queueOpCountsVec`.
    virtual void
    printQueueOpSummary(bsls::Types::Uint64     queueOpRecordsCount,
                        const QueueOpCountsVec& queueOpCountsVec) const = 0;

    /// Print number of all journal operations `journalOpRecordsCount`.
    virtual void
    printJournalOpSummary(bsls::Types::Uint64 journalOpRecordsCount) const = 0;

    /// Print summary of all records including `totalRecordsCount` and detailed
    /// summary of records for each queue `queueDetailsMap`.
    virtual void
    printRecordSummary(bsls::Types::Uint64    totalRecordsCount,
                       const QueueDetailsMap& queueDetailsMap) const = 0;

    /// Print metadata of the specified journal file `journalFile_p`.
    virtual void printJournalFileMeta(
        const mqbs::JournalFileIterator* journalFile_p) const = 0;

    /// Print metadata of the specified data file `dataFile_p`.
    virtual void
    printDataFileMeta(const mqbs::DataFileIterator* dataFile_p) const = 0;
};

/// Create an instance of printer to print data to the specified 'stream'
/// according to the specified 'mode' using the specified 'allocator'.
bsl::shared_ptr<Printer> createPrinter(Parameters::PrintMode mode,
                                       std::ostream&         stream,
                                       bslma::Allocator*     allocator);

}  // close package namespace

}  // close enterprise namespace

#endif  // INCLUDED_M_BMQSTORAGETOOL_PRINTER_H
