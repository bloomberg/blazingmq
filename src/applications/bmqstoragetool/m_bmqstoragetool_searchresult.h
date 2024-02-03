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

// m_bmqstoragetool_searchresult.h -*-C++-*-
#ifndef INCLUDED_M_BMQSTORAGETOOL_SEARCHRESULT
#define INCLUDED_M_BMQSTORAGETOOL_SEARCHRESULT

//@PURPOSE: Provide a logic of search and output results.
//
//@CLASSES:
//  m_bmqstoragetool::SearchResult: an interface for search processors.
//
//@DESCRIPTION: 'SearchResult' and implementation classes provide
// a logic of search and output results.

// bmqstoragetool
#include <m_bmqstoragetool_filters.h>
#include <m_bmqstoragetool_messagedetails.h>
#include <m_bmqstoragetool_parameters.h>
#include <m_bmqstoragetool_payloaddumper.h>

// MQB
#include <mqbs_filestoreprotocolprinter.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_list.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
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
    virtual bool hasCache() const { return false; }

  public:
    virtual ~SearchResult() = default;

    // MANIPULATORS
    virtual bool processMessageRecord(const mqbs::MessageRecord& record,
                                      bsls::Types::Uint64        recordIndex,
                                      bsls::Types::Uint64 recordOffset)  = 0;
    virtual bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                                      bsls::Types::Uint64        recordIndex,
                                      bsls::Types::Uint64 recordOffset)  = 0;
    virtual bool processDeletionRecord(const mqbs::DeletionRecord& record,
                                       bsls::Types::Uint64         recordIndex,
                                       bsls::Types::Uint64 recordOffset) = 0;
    virtual void outputResult()                                          = 0;
    virtual void
    outputResult(bsl::unordered_set<bmqt::MessageGUID>& guidFilter) = 0;
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
    typedef bsl::list<GuidData>::iterator GuidListIt;
    // List iterator for guid short result.

    // PRIVATE DATA

    bsl::ostream&                        d_ostream;
    const bsl::shared_ptr<PayloadDumper> d_payloadDumper;
    const bool                           d_printImmediately;
    const bool                           d_printOnDelete;
    const bool                           d_eraseDeleted;

    bsl::size_t d_printedMessagesCount;

    bsl::unordered_map<bmqt::MessageGUID, GuidListIt> d_guidMap;
    // Map to store guid and list iterator, for fast searching by guid.
    bsl::list<GuidData> d_guidList;
    // List to store ordered guid data to preserve messages order for output.

    void outputGuidData(GuidData guidData);

  public:
    // CREATORS
    explicit SearchShortResult(bsl::ostream&                  ostream,
                               bsl::shared_ptr<PayloadDumper> payloadDumper,
                               bslma::Allocator*              allocator,
                               const bool printImmediately = true,
                               const bool printOnDelete    = false,
                               const bool eraseDeleted     = false);

    // MANIPULATORS
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    void outputResult() BSLS_KEYWORD_OVERRIDE;
    void outputResult(bsl::unordered_set<bmqt::MessageGUID>& guidFilter)
        BSLS_KEYWORD_OVERRIDE;
    bool hasCache() const BSLS_KEYWORD_OVERRIDE;
};

// ========================
// class SearchDetailResult
// ========================

/// This class provides logic to handle and output detail result.
class SearchDetailResult : public SearchResult {
  private:
    // PRIVATE TYPES
    typedef bsl::unordered_map<bmqt::MessageGUID, MessageDetails>
        MessagesDetails;
    // Type that represents map for guid and message details.

    // PRIVATE DATA

    bsl::ostream&                                    d_ostream;
    const QueueMap&                                  d_queueMap;
    const bsl::shared_ptr<PayloadDumper>             d_payloadDumper;
    bslma::Allocator*                                d_allocator_p;
    const bool                                       d_printImmediately;
    const bool                                       d_eraseDeleted;
    const bool                                       d_cleanUnprinted;
    bsl::size_t                                      d_printedMessagesCount;
    MessagesDetails                                  d_messagesDetails;
    bsl::map<bsls::Types::Uint64, bmqt::MessageGUID> d_messageIndexToGuidMap;
    // Map to store sorted indexes to preserve messages order for output.

    // MANIPULATORS
    void addMessageDetails(const mqbs::MessageRecord& record,
                           bsls::Types::Uint64        recordIndex,
                           bsls::Types::Uint64        recordOffset);
    void deleteMessageDetails(MessagesDetails::iterator iterator);
    void outputMessageDetails(const MessageDetails& messageDetails);

  public:
    // CREATORS
    explicit SearchDetailResult(bsl::ostream&                  ostream,
                                const QueueMap&                queueMap,
                                bsl::shared_ptr<PayloadDumper> payloadDumper,
                                bslma::Allocator*              allocator,
                                const bool printImmediately = true,
                                const bool eraseDeleted     = true,
                                const bool cleanUnprinted   = false);

    // MANIPULATORS
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    void outputResult() BSLS_KEYWORD_OVERRIDE;
    void outputResult(bsl::unordered_set<bmqt::MessageGUID>& guidFilter)
        BSLS_KEYWORD_OVERRIDE;
    bool hasCache() const BSLS_KEYWORD_OVERRIDE;
};

// ===========================
// class SearchResultDecorator
// ===========================

/// This class provides a base decorator that handles
/// given 'component'.
class SearchResultDecorator : public SearchResult {
  protected:
    bsl::shared_ptr<SearchResult> d_searchResult;

  public:
    // CREATORS
    SearchResultDecorator(const bsl::shared_ptr<SearchResult> component);

    // MANIPULATORS
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    void outputResult() BSLS_KEYWORD_OVERRIDE;
    void outputResult(bsl::unordered_set<bmqt::MessageGUID>& guidFilter)
        BSLS_KEYWORD_OVERRIDE;
};

// ====================================
// class SearchResultTimestampDecorator
// ====================================
class SearchResultTimestampDecorator : public SearchResultDecorator {
  private:
    const bsls::Types::Uint64 d_timestampLt;

    // ACCESSORS
    bool stop(bsls::Types::Uint64 timestamp) const;

  public:
    // CREATORS
    SearchResultTimestampDecorator(
        const bsl::shared_ptr<SearchResult> component,
        const bsls::Types::Uint64           timestampLt);

    // MANIPULATORS
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
};

// ========================
// class SearchAllDecorator
// ========================
class SearchAllDecorator : public SearchResultDecorator {
  public:
    // CREATORS
    explicit SearchAllDecorator(const bsl::shared_ptr<SearchResult> component);

    // MANIPULATORS
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
};

// ================================
// class SearchOutstandingDecorator
// ================================
class SearchOutstandingDecorator : public SearchResultDecorator {
  private:
    // PRIVATE DATA

    bsl::ostream&                         d_ostream;
    bsl::size_t                           d_foundMessagesCount;
    bsl::size_t                           d_deletedMessagesCount;
    bsl::unordered_set<bmqt::MessageGUID> d_guids;

  public:
    // CREATORS

    explicit SearchOutstandingDecorator(
        const bsl::shared_ptr<SearchResult> component,
        bsl::ostream&                       ostream,
        bslma::Allocator*                   allocator);

    // MANIPULATORS

    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    void outputResult() BSLS_KEYWORD_OVERRIDE;
};

// =======================================
// class SearchPartiallyConfirmedDecorator
// =======================================
class SearchPartiallyConfirmedDecorator : public SearchResultDecorator {
  private:
    // PRIVATE DATA

    bsl::ostream&                                 d_ostream;
    bsl::size_t                                   d_foundMessagesCount;
    bsl::size_t                                   d_deletedMessagesCount;
    bsl::unordered_map<bmqt::MessageGUID, size_t> d_partiallyConfirmedGUIDS;

  public:
    // CREATORS
    explicit SearchPartiallyConfirmedDecorator(
        const bsl::shared_ptr<SearchResult> component,
        bsl::ostream&                       ostream,
        bslma::Allocator*                   allocator);

    // MANIPULATORS
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    void outputResult() BSLS_KEYWORD_OVERRIDE;
};

// =========================
// class SearchGuidDecorator
// =========================
class SearchGuidDecorator : public SearchResultDecorator {
  private:
    // PRIVATE TYPES
    typedef bsl::list<bsl::string>::iterator GuidListIt;

    // PRIVATE DATA
    bsl::ostream&                                     d_ostream;
    bool                                              d_withDetails;
    bsl::unordered_map<bmqt::MessageGUID, GuidListIt> d_guidsMap;
    bsl::list<bsl::string>                            d_guids;

  public:
    // CREATORS
    explicit SearchGuidDecorator(const bsl::shared_ptr<SearchResult> component,
                                 const bsl::vector<bsl::string>&     guids,
                                 bsl::ostream&                       ostream,
                                 bool              withDetails,
                                 bslma::Allocator* allocator);

    // MANIPULATORS
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    void outputResult() BSLS_KEYWORD_OVERRIDE;
};

// ======================
// class SummaryProcessor
// ======================
class SummaryProcessor : public SearchResult {
  private:
    // PRIVATE DATA
    bsl::ostream&                                 d_ostream;
    mqbs::JournalFileIterator*                    d_journalFile_p;
    mqbs::DataFileIterator*                       d_dataFile_p;
    bsl::size_t                                   d_foundMessagesCount;
    bsl::size_t                                   d_deletedMessagesCount;
    bsl::unordered_map<bmqt::MessageGUID, size_t> d_partiallyConfirmedGUIDS;

  public:
    // CREATORS
    explicit SummaryProcessor(bsl::ostream&              ostream,
                              mqbs::JournalFileIterator* journalFile_p,
                              mqbs::DataFileIterator*    dataFile_p,
                              bslma::Allocator*          allocator);

    // MANIPULATORS
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    void outputResult() BSLS_KEYWORD_OVERRIDE;
    void outputResult(bsl::unordered_set<bmqt::MessageGUID>& guidFilter)
        BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
