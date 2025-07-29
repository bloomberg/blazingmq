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

#ifndef INCLUDED_M_BMQSTORAGETOOL_SEARCHRESULT
#define INCLUDED_M_BMQSTORAGETOOL_SEARCHRESULT

//@PURPOSE: Provide a logic of search and output results.
//
//@CLASSES:
// m_bmqstoragetool::SearchResult: an interface for search processors.
// m_bmqstoragetool::SearchShortResult: provides logic to handle and output
//  short result (only GUIDs).
// m_bmqstoragetool::SearchDetailResult: provides
//  logic to handle and output detail result.
// m_bmqstoragetool::SearchExactMatchResult: provides logic to handle and
//  output exact match result (sequence numbers and offsets).
// m_bmqstoragetool::SearchResultDecorator: provides a base decorator for
//  search processor.
// m_bmqstoragetool::SearchResultTimestampDecorator:
//  provides decorator to handle timestamps.
// m_bmqstoragetool::SearchResultOffsetDecorator:
//  provides decorator to handle offsets.
// m_bmqstoragetool::SearchResultSequenceNumberDecorator:
//  provides decorator to handle composite sequence numbers.
// m_bmqstoragetool::SearchAllDecorator: provides decorator to handle all
//  messages.
// m_bmqstoragetool::SearchOutstandingDecorator: provides decorator
//  to handle outstanding or confirmed messages.
// m_bmqstoragetool::SearchPartiallyConfirmedDecorator: provides decorator to
//  handle partially confirmed messages.
// m_bmqstoragetool::SearchGuidDecorator:
//  provides decorator to handle search of given GUIDs.
// m_bmqstoragetool::SearchOffsetDecorator:
//  provides decorator to handle search of given offsets.
// m_bmqstoragetool::SearchSequenceNumberDecorator:
//  provides decorator to handle search of given composite sequence numbers.
// m_bmqstoragetool::SummaryProcessor: provides logic to collect summary of
//  journal file.
//
//@DESCRIPTION: 'SearchResult' interface and implementation classes provide
// a logic of search and output results.

// bmqstoragetool
#include <m_bmqstoragetool_compositesequencenumber.h>
#include <m_bmqstoragetool_filters.h>
#include <m_bmqstoragetool_messagedetails.h>
#include <m_bmqstoragetool_parameters.h>
#include <m_bmqstoragetool_payloaddumper.h>
#include <m_bmqstoragetool_printer.h>

// MQB
#include <mqbs_filestoreprotocolprinter.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// ==================
// class SearchResult
// ==================

/// This class provides an interface for search processors.
class SearchResult {
  protected:
    // PROTECTED TYPES

    typedef bsl::list<bmqt::MessageGUID> GuidsList;
    // List of message guids.
    typedef bsl::unordered_map<bmqt::MessageGUID, GuidsList::iterator>
        GuidsMap;
    // Hash map of message guids to iterators of GuidsList.

  public:
    // CREATORS

    /// Destructor
    virtual ~SearchResult();

    // MANIPULATORS

    /// Process `message` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    virtual bool processMessageRecord(const mqbs::MessageRecord& record,
                                      bsls::Types::Uint64        recordIndex,
                                      bsls::Types::Uint64 recordOffset) = 0;
    /// Process `confirm` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    virtual bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                                      bsls::Types::Uint64        recordIndex,
                                      bsls::Types::Uint64 recordOffset) = 0;
    /// Process `deletion` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    virtual bool processDeletionRecord(const mqbs::DeletionRecord& record,
                                       bsls::Types::Uint64         recordIndex,
                                       bsls::Types::Uint64 recordOffset) = 0;
    /// Process `queueOp` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    virtual bool processQueueOpRecord(const mqbs::QueueOpRecord& record,
                                      bsls::Types::Uint64        recordIndex,
                                      bsls::Types::Uint64 recordOffset) = 0;
    /// Process `journalOp` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    virtual bool processJournalOpRecord(const mqbs::JournalOpRecord& record,
                                        bsls::Types::Uint64 recordIndex,
                                        bsls::Types::Uint64 recordOffset) = 0;

    /// Output result of a search.
    virtual void outputResult() = 0;
    /// Output result of a search filtered by the specified GUIDs filter.
    virtual void outputResult(const GuidsList& guidFilter) = 0;

    /// Return `false` if all required data is processed, e.g. all given GUIDs
    /// are output and search could be stopped. Return `true` to indicate that
    /// there is incomplete data.
    virtual bool hasCache() const { return false; }

    // ACCESSORS

    /// Return a reference to the non-modifiable printer
    virtual const bsl::shared_ptr<Printer>& printer() const = 0;
};

// =======================
// class SearchShortResult
// =======================

/// This class provides logic to handle and output short result (message
/// GUIDs).
class SearchShortResult : public SearchResult {
  private:
    // PRIVATE TYPES

    typedef bsl::pair<bmqt::MessageGUID, bsls::Types::Uint64> GuidData;
    // Pair that represents guid short result.
    typedef bsl::list<GuidData> GuidDataList;
    // List iterator for guid short result.
    typedef bsl::list<GuidData>::iterator GuidDataListIt;
    // List iterator for guid short result.
    typedef bsl::unordered_map<bmqt::MessageGUID, GuidDataListIt> GuidDataMap;
    // Hash map of message guids to iterators pointing to coresponding GuidData
    // objects in a list.

    // PRIVATE DATA

    const bsl::shared_ptr<Printer> d_printer;
    // Pointer to 'Printer' instance.
    Parameters::ProcessRecordTypes d_processRecordTypes;
    // Record types to process
    const bslma::ManagedPtr<PayloadDumper> d_payloadDumper;
    // Pointer to 'PayloadDumper' instance.
    const bool d_printImmediately;
    // If 'true', print message guid as soon as it is received (usually when
    // 'message' record received) to save memory. If 'false', message guid
    // remains stored in guid list for further processing.
    const bool d_eraseDeleted;
    // If 'true', erase data from guid list when 'deleted' record is received
    // to save memory. If 'false', message data remains stored in guid list for
    // further processing.
    const bool d_printOnDelete;
    // If 'true', print message guid when 'deleted' record is received.
    bsls::Types::Uint64 d_printedMessagesCount;
    // Counter of already output (printed) messages.
    bsls::Types::Uint64 d_printedQueueOpCount;
    // Counter of already output (printed) QueueOp records.
    bsls::Types::Uint64 d_printedJournalOpCount;
    // Counter of already output (printed) JournalOp records.
    GuidDataMap d_guidMap;
    // Map to store guid and list iterator, for fast searching by guid.
    bsl::list<GuidData> d_guidList;
    // List to store ordered guid data to preserve messages order for output.

    // PRIVATE MANIPULATORS

    void outputGuidData(const GuidData& guidData);
    // Output result in short format (only GUIDs).

  public:
    // CREATORS

    /// Constructor using the specified `printer`, `processRecordTypes`,
    /// `payloadDumper`, `printImmediately`, `eraseDeleted`, `printOnDelete`
    /// and `allocator`.
    explicit SearchShortResult(
        const bsl::shared_ptr<Printer>&       printer,
        const Parameters::ProcessRecordTypes& processRecordTypes,
        bslma::ManagedPtr<PayloadDumper>&     payloadDumper,
        bool                                  printImmediately = true,
        bool                                  eraseDeleted     = false,
        bool                                  printOnDelete    = false,
        bslma::Allocator*                     allocator        = 0);

    // ACCESSORS

    bslma::Allocator* allocator() const;

    // MANIPULATORS

    /// Process `message` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `confirm` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `deletion` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `queueOp` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processQueueOpRecord(const mqbs::QueueOpRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `journalOp` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processJournalOpRecord(const mqbs::JournalOpRecord& record,
                                bsls::Types::Uint64          recordIndex,
                                bsls::Types::Uint64          recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Output result of a search.
    void outputResult() BSLS_KEYWORD_OVERRIDE;
    /// Output result of a search filtered by the specified GUIDs filter.
    void outputResult(const GuidsList& guidFilter) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return 'false' if all required data is processed, e.g. all given GUIDs
    /// are output and search could be stopped. Return 'true' to indicate that
    /// there is incomplete data.
    bool hasCache() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference to the non-modifiable printer
    const bsl::shared_ptr<Printer>& printer() const BSLS_KEYWORD_OVERRIDE;
};

// ========================
// class SearchDetailResult
// ========================

/// This class provides logic to handle and output detail result.
class SearchDetailResult : public SearchResult {
  private:
    // PRIVATE TYPES

    typedef bsl::list<MessageDetails> DetailsList;
    // List of message details.
    typedef bsl::unordered_map<bmqt::MessageGUID, DetailsList::iterator>
        DetailsMap;
    // Hash map of message guids to message details.

    // PRIVATE DATA

    Parameters::ProcessRecordTypes d_processRecordTypes;
    // Record types to process
    const QueueMap& d_queueMap;
    // Reference to 'QueueMap' instance.
    const bsl::shared_ptr<Printer> d_printer;
    // Pointer to 'Printer' instance.
    const bslma::ManagedPtr<PayloadDumper> d_payloadDumper;
    // Pointer to 'PayloadDumper' instance.
    const bool d_printImmediately;
    // If true, print message details as soon as it is complete (usually when
    // 'deleted' record received) to save memory. If false, message data
    // remains stored in MessagesDetails for further processing.
    const bool d_eraseDeleted;
    // If true, erase data from MessagesDetails when 'deleted' record is
    // received to save memory. If false, message data remains stored in
    // MessagesDetails for further processing.
    const bool d_cleanUnprinted;
    // If true, clean remaining data in MessagesDetails before printing final
    // result.
    bsls::Types::Uint64 d_printedMessagesCount;
    // Printed messages count.
    bsls::Types::Uint64 d_printedQueueOpCount;
    // Printed QueueOp records count.
    bsls::Types::Uint64 d_printedJournalOpCount;
    // Printed JournalOp records count.
    DetailsList d_messageDetailsList;
    // List of message details to preserve messages order for output.
    DetailsMap d_messageDetailsMap;
    // Hash map of message guids to message details for fast access.

    bslma::Allocator* d_allocator_p;
    // Allocator used inside the class.

    // PRIVATE MANIPULATORS

    void addMessageDetails(const mqbs::MessageRecord& record,
                           bsls::Types::Uint64        recordIndex,
                           bsls::Types::Uint64        recordOffset);
    // Add message details into internal storage with the specified 'record',
    // 'recordIndex' and 'recordOffset'.
    void deleteMessageDetails(DetailsMap::iterator iterator);
    // Delete message details into internal storage with the specified
    // 'iterator'.
    void outputMessageDetails(const MessageDetails& messageDetails);
    // Output message details with the specified 'messageDetails'.

  public:
    // CREATORS

    /// Constructor using the specified `printer`, `processRecordTypes`,
    /// `queueMap`, `payloadDumper`, `printImmediately`, `eraseDeleted`,
    /// `cleanUnprinted` and `allocator`.
    SearchDetailResult(
        const bsl::shared_ptr<Printer>&       printer,
        const Parameters::ProcessRecordTypes& processRecordTypes,
        const QueueMap&                       queueMap,
        bslma::ManagedPtr<PayloadDumper>&     payloadDumper,
        bool                                  printImmediately = true,
        bool                                  eraseDeleted     = true,
        bool                                  cleanUnprinted   = false,
        bslma::Allocator*                     allocator        = 0);

    // MANIPULATORS

    /// Process `message` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `confirm` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `deletion` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `queueOp` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processQueueOpRecord(const mqbs::QueueOpRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `journalOp` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processJournalOpRecord(const mqbs::JournalOpRecord& record,
                                bsls::Types::Uint64          recordIndex,
                                bsls::Types::Uint64          recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Output result of a search.
    void outputResult() BSLS_KEYWORD_OVERRIDE;
    /// Output result of a search filtered by the specified GUIDs filter.
    void outputResult(const GuidsList& guidFilter) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return 'false' if all required data is processed, e.g. all given GUIDs
    /// are output and search could be stopped. Return 'true' to indicate that
    /// there is incomplete data.
    bool hasCache() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference to the non-modifiable printer
    const bsl::shared_ptr<Printer>& printer() const BSLS_KEYWORD_OVERRIDE;
};

// ============================
// class SearchExactMatchResult
// ============================
///
/// This class provides logic to handle and output exact match result (only
/// one record that matched sequince number or offset).
class SearchExactMatchResult : public SearchResult {
  private:
    // PRIVATE DATA

    const bsl::shared_ptr<Printer> d_printer;
    // Pointer to 'Printer' instance.
    Parameters::ProcessRecordTypes d_processRecordTypes;
    // Record types to process
    bool d_isDetail;
    // If 'true', output detail result, otherwise output short result.
    const QueueMap& d_queueMap;
    // Reference to 'QueueMap' instance.
    const bslma::ManagedPtr<PayloadDumper> d_payloadDumper;
    // Pointer to 'PayloadDumper' instance.
    bsls::Types::Uint64 d_printedMessagesCount;
    // Counter of already output (printed) messages.
    bsls::Types::Uint64 d_printedConfirmCount;
    // Counter of already output (printed) messages.
    bsls::Types::Uint64 d_printedDeletionCount;
    // Counter of already output (printed) messages.
    bsls::Types::Uint64 d_printedQueueOpCount;
    // Counter of already output (printed) QueueOp records.
    bsls::Types::Uint64 d_printedJournalOpCount;
    // Counter of already output (printed) JournalOp records.
    bslma::Allocator* d_allocator_p;
    // Allocator used inside the class.

  public:
    // CREATORS

    /// Constructor using the specified `printer`, `processRecordTypes`,
    /// `queueMap`, `payloadDumper`,
    explicit SearchExactMatchResult(
        const bsl::shared_ptr<Printer>&       printer,
        const Parameters::ProcessRecordTypes& processRecordTypes,
        bool                                  isDetail,
        const QueueMap&                       queueMap,
        bslma::ManagedPtr<PayloadDumper>&     payloadDumper,
        bslma::Allocator*                     allocator);

    // MANIPULATORS

    /// Process `message` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `confirm` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `deletion` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `queueOp` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processQueueOpRecord(const mqbs::QueueOpRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `journalOp` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processJournalOpRecord(const mqbs::JournalOpRecord& record,
                                bsls::Types::Uint64          recordIndex,
                                bsls::Types::Uint64          recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Output result of a search.
    void outputResult() BSLS_KEYWORD_OVERRIDE;
    /// Output result of a search filtered by the specified GUIDs filter.
    void outputResult(const GuidsList& guidFilter) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return a reference to the non-modifiable printer
    const bsl::shared_ptr<Printer>& printer() const BSLS_KEYWORD_OVERRIDE;
};

// ===========================
// class SearchResultDecorator
// ===========================

/// This class provides a base decorator that handles
/// given `component`.
class SearchResultDecorator : public SearchResult {
  protected:
    bsl::shared_ptr<SearchResult> d_searchResult;
    // Pointer to object that is decorated.
    bslma::Allocator* d_allocator_p;
    // Pointer to allocator that is used inside the class.

  public:
    // CREATORS

    /// Constructor using the specified `component` and `allocator`.
    SearchResultDecorator(const bsl::shared_ptr<SearchResult>& component,
                          bslma::Allocator*                    allocator);

    // MANIPULATORS

    /// Process `message` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `confirm` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `deletion` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `queueOp` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processQueueOpRecord(const mqbs::QueueOpRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `journalOp` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processJournalOpRecord(const mqbs::JournalOpRecord& record,
                                bsls::Types::Uint64          recordIndex,
                                bsls::Types::Uint64          recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Output result of a search.
    void outputResult() BSLS_KEYWORD_OVERRIDE;
    /// Output result of a search filtered by the specified GUIDs filter.
    void outputResult(const GuidsList& guidFilter) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return 'false' if all required data is processed, e.g. all given GUIDs
    /// are output and search could be stopped. Return 'true' to indicate that
    /// there is incomplete data.
    bool hasCache() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference to the non-modifiable printer
    const bsl::shared_ptr<Printer>& printer() const BSLS_KEYWORD_OVERRIDE;
};

// ====================================
// class SearchResultTimestampDecorator
// ====================================

/// This class provides decorator to handle timestamps.
class SearchResultTimestampDecorator : public SearchResultDecorator {
  private:
    const bsls::Types::Uint64 d_timestampLt;
    // Higher bound timestamp.

    // ACCESSORS

    /// Return 'true' if the specified 'timestamp' is greater than
    /// 'd_timestampLt' and internal cache is empty.
    bool stop(const bsls::Types::Uint64 timestamp) const;

  public:
    // CREATORS

    /// Constructor using the specified `component`, `timestampLt` and
    /// `allocator`.
    SearchResultTimestampDecorator(
        const bsl::shared_ptr<SearchResult>& component,
        const bsls::Types::Uint64            timestampLt,
        bslma::Allocator*                    allocator);

    // MANIPULATORS

    /// Process `message` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `confirm` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `deletion` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
};

// =================================
// class SearchResultOffsetDecorator
// =================================

/// This class provides decorator to handle offsets.
class SearchResultOffsetDecorator : public SearchResultDecorator {
  private:
    /// Higher bound offset.
    const bsls::Types::Uint64 d_offsetLt;

    // ACCESSORS

    /// Return 'true' if the specified 'offset' is greater than
    /// 'd_offsetLt' and internal cache is empty.
    bool stop(const bsls::Types::Uint64 offset) const;

  public:
    // CREATORS

    /// Constructor using the specified `component`, `offsetLt` and
    /// `allocator`.
    SearchResultOffsetDecorator(const bsl::shared_ptr<SearchResult>& component,
                                const bsls::Types::Uint64            offsetLt,
                                bslma::Allocator* allocator);

    // MANIPULATORS

    /// Process `message` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `confirm` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `deletion` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
};

// =========================================
// class SearchResultSequenceNumberDecorator
// =========================================

/// This class provides decorator to handle composite sequence numbers.
class SearchResultSequenceNumberDecorator : public SearchResultDecorator {
  private:
    /// Higher bound sequence number.
    const CompositeSequenceNumber d_sequenceNumberLt;

    // ACCESSORS

    /// Return 'true' if the specified 'sequenceNumber' is greater than
    /// 'sequenceNumberLt' and internal cache is empty.
    bool stop(const CompositeSequenceNumber& sequenceNumber) const;

  public:
    // CREATORS

    /// Constructor using the specified `component`, `seqNumberLt` and
    /// `allocator`.
    SearchResultSequenceNumberDecorator(
        const bsl::shared_ptr<SearchResult>& component,
        const CompositeSequenceNumber&       seqNumberLt,
        bslma::Allocator*                    allocator);

    // MANIPULATORS

    /// Process `message` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `confirm` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `deletion` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
};

// ========================
// class SearchAllDecorator
// ========================

/// This class provides decorator to handle all found messages.
class SearchAllDecorator : public SearchResultDecorator {
  public:
    // CREATORS

    /// Constructor using the specified `component` and `allocator`.
    SearchAllDecorator(const bsl::shared_ptr<SearchResult>& component,
                       bslma::Allocator*                    allocator);

    // MANIPULATORS

    /// Process `message` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
};

// ================================
// class SearchOutstandingDecorator
// ================================

/// This class provides decorator to handle outstanding or unconfirmed
/// messages.
class SearchOutstandingDecorator : public SearchResultDecorator {
  private:
    // PRIVATE DATA

    bsls::Types::Uint64 d_foundMessagesCount;
    // Counter of found messages.
    bsls::Types::Uint64 d_deletedMessagesCount;
    // Counter of deleted messages.
    bsl::unordered_set<bmqt::MessageGUID> d_guids;
    // Set of found non-deleted message GUIDs.

  public:
    // CREATORS

    /// Constructor using the specified `component`, `ostream` and `allocator`.
    SearchOutstandingDecorator(const bsl::shared_ptr<SearchResult>& component,
                               bslma::Allocator*                    allocator);

    // MANIPULATORS

    /// Process `message` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `deletion` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Output result of a search.
    void outputResult() BSLS_KEYWORD_OVERRIDE;
};

// =======================================
// class SearchPartiallyConfirmedDecorator
// =======================================

/// This class provides decorator to handle partially confirmed messages.
class SearchPartiallyConfirmedDecorator : public SearchResultDecorator {
  private:
    // PRIVATE DATA

    bsls::Types::Uint64 d_foundMessagesCount;
    // Counter of found messages.
    bsls::Types::Uint64 d_deletedMessagesCount;
    // Counter of deleted messages.
    GuidsList d_guidsList;
    // List of message guids to retain the order thir original order.
    GuidsMap d_notConfirmedGuids;
    // Map guid -> iterator to d_guidsList. Messages stored here have neither
    // confirmation messages no delete message associated with them.
    GuidsMap d_partiallyConfirmedGuids;
    // Map guid -> iterator to d_guidsList list of messages. Messages stored
    // here have at leas one confirmation message and no delete message
    // associated with them.

  public:
    // CREATORS

    /// Constructor using the specified `component`, `ostream` and `allocator`.
    SearchPartiallyConfirmedDecorator(
        const bsl::shared_ptr<SearchResult>& component,
        bslma::Allocator*                    allocator);

    // MANIPULATORS

    /// Process `message` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `confirm` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `deletion` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Output result of a search.
    void outputResult() BSLS_KEYWORD_OVERRIDE;
};

// =========================
// class SearchGuidDecorator
// =========================

/// This class provides decorator to handle search of given GUIDs.
class SearchGuidDecorator : public SearchResultDecorator {
  private:
    // PRIVATE DATA
    bool d_withDetails;
    // If 'true', output detailed result, output short one otherwise.
    GuidsMap d_guidsMap;
    // Map guid -> guids list iterator.
    GuidsList d_guids;
    // List to store ordered guid data to preserve messages order for output.

  public:
    // CREATORS

    /// Constructor using the specified `component`, `guids`, `ostream`,
    /// `withDetails` and `allocator`.
    SearchGuidDecorator(const bsl::shared_ptr<SearchResult>& component,
                        const bsl::vector<bsl::string>&      guids,
                        bool                                 withDetails,
                        bslma::Allocator*                    allocator);

    // MANIPULATORS

    /// Process `message` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `deletion` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Output result of a search.
    void outputResult() BSLS_KEYWORD_OVERRIDE;
};

// ===========================
// class SearchOffsetDecorator
// ===========================

/// This class provides decorator to handle search of given offsets.
class SearchOffsetDecorator : public SearchResultDecorator {
  private:
    // PRIVATE DATA
    /// List of offsets to search for.
    bsl::vector<bsls::Types::Int64> d_offsets;
    /// If 'true', output detailed result, output short one otherwise.
    bool d_withDetails;

  public:
    // CREATORS

    /// Constructor using the specified `component`, `offsets`, `ostream`,
    /// `withDetails` and `allocator`.
    SearchOffsetDecorator(const bsl::shared_ptr<SearchResult>&   component,
                          const bsl::vector<bsls::Types::Int64>& offsets,
                          bool                                   withDetails,
                          bslma::Allocator*                      allocator);

    // MANIPULATORS

    /// Process `message` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `confirm` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `deletion` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `queueOp` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processQueueOpRecord(const mqbs::QueueOpRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `journalOp` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processJournalOpRecord(const mqbs::JournalOpRecord& record,
                                bsls::Types::Uint64          recordIndex,
                                bsls::Types::Uint64          recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Output result of a search.
    void outputResult() BSLS_KEYWORD_OVERRIDE;
};

// ===================================
// class SearchSequenceNumberDecorator
// ===================================

/// This class provides decorator to handle search of given composite sequence
/// numbers.
class SearchSequenceNumberDecorator : public SearchResultDecorator {
  private:
    // PRIVATE DATA
    bsl::vector<CompositeSequenceNumber> d_seqNums;
    // List of composite sequence numbers to search for.
    bool d_withDetails;
    // If 'true', output detailed result, output short one otherwise.

    // PRIVATE ACCESSORS
    bool
    isSequenceNumberFound(const CompositeSequenceNumber& sequenceNumber) const;
    // Return 'true' if the specified 'sequenceNumber' is found in d_seqNums.

  public:
    // CREATORS

    /// Constructor using the specified `component`, `seqNums`, `ostream`,
    /// `withDetails` and `allocator`.
    SearchSequenceNumberDecorator(
        const bsl::shared_ptr<SearchResult>&        component,
        const bsl::vector<CompositeSequenceNumber>& seqNums,
        bool                                        withDetails,
        bslma::Allocator*                           allocator);

    // MANIPULATORS

    /// Process `message` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;

    /// Process `confirm` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `deletion` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `queueOp` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processQueueOpRecord(const mqbs::QueueOpRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `journalOp` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processJournalOpRecord(const mqbs::JournalOpRecord& record,
                                bsls::Types::Uint64          recordIndex,
                                bsls::Types::Uint64          recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Output result of a search.
    void outputResult() BSLS_KEYWORD_OVERRIDE;
};

// ======================
// class SummaryProcessor
// ======================

/// This class provides logic to process summary of journal file.
class SummaryProcessor : public SearchResult {
  private:
    // PRIVATE TYPES

    typedef bsl::unordered_set<bmqt::MessageGUID> GuidsSet;
    // Set of message guids.
    typedef bsl::vector<bsls::Types::Uint64> QueueOpCountsVec;
    // Queue op counts vector.

    // PRIVATE DATA
    const bsl::shared_ptr<Printer> d_printer;
    // Reference to print manager.
    mqbs::JournalFileIterator* d_journalFile_p;
    // Pointer to journal file iterator.
    mqbs::DataFileIterator* d_dataFile_p;
    // Pointer to data file iterator.
    Parameters::ProcessRecordTypes d_processRecordTypes;
    // Record types to process
    bsls::Types::Uint64 d_foundMessagesCount;
    // Counter of found messages.
    bsls::Types::Uint64 d_deletedMessagesCount;
    // Counter of deleted messages.
    bsls::Types::Uint64 d_journalOpRecordsCount;
    // Counter of journalOp records.
    bsls::Types::Uint64 d_queueOpRecordsCount;
    // Counter of queueOp records.
    QueueOpCountsVec d_queueOpCountsVec;
    // Queue op counts vec.
    GuidsSet d_notConfirmedGuids;
    // Set of message guids. Messages stored here have neither confirmation
    // messages no delete message associated with them.
    GuidsSet d_partiallyConfirmedGuids;
    // Set of message guids. Messages stored here have at leas one confirmation
    // message and no delete message associated with them.
    bsls::Types::Uint64 d_totalRecordsCount;
    // The total number of records.
    QueueDetailsMap d_queueDetailsMap;
    // Map containing 'QueueDetails' per queue
    const QueueMap& d_queueMap;
    // Reference to 'QueueMap' instance.
    bsl::optional<bsls::Types::Uint64> d_minRecordsPerQueue;
    // Minimum number of records for the queue to be displayed its detailed
    // info
    bslma::Allocator* d_allocator_p;
    // Pointer to allocator that is used inside the class.

    // PRIVATE MANIPULATORS

    /// Prepare collected QueueDetails for further printing
    void finalizeQueueDetails();

  public:
    // CREATORS

    /// Constructor using the specified `printer`, `journalFile_p`,
    /// `dataFile_p` , `processRecordTypes`, `queueMap`, `minRecordsPerQueue`
    /// and `allocator`.
    explicit SummaryProcessor(
        const bsl::shared_ptr<Printer>&       printer,
        mqbs::JournalFileIterator*            journalFile_p,
        mqbs::DataFileIterator*               dataFile_p,
        const Parameters::ProcessRecordTypes& processRecordTypes,
        const QueueMap&                       queueMap,
        bsl::optional<bsls::Types::Uint64>    minRecordsPerQueue,
        bslma::Allocator*                     allocator);

    // MANIPULATORS

    /// Process `message` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `confirm` record with the specified `record`, `recordIndex` and
    /// `recordOffset`.
    bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `deletion` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `queueOp` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processQueueOpRecord(const mqbs::QueueOpRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Process `journalOp` record with the specified `record`, `recordIndex`
    /// and `recordOffset`.
    bool processJournalOpRecord(const mqbs::JournalOpRecord& record,
                                bsls::Types::Uint64          recordIndex,
                                bsls::Types::Uint64          recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    /// Output result of a search.
    void outputResult() BSLS_KEYWORD_OVERRIDE;
    /// Output result of a search filtered by the specified GUIDs filter.
    void outputResult(const GuidsList& guidFilter) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return a reference to the non-modifiable printer
    const bsl::shared_ptr<Printer>& printer() const BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
