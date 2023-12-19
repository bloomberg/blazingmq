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

// bmqstoragetool
// #include <m_bmqstoragetool_searchprocessor.h>

#include <m_bmqstoragetool_filters.h>
#include <m_bmqstoragetool_messagedetails.h>
#include <m_bmqstoragetool_parameters.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bsl_vector.h>
#include <bsls_keyword.h>

// MQB
#include <mqbs_filestoreprotocolprinter.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =====================
// class SearchResult
// =====================

class SearchResult {
  protected:
    typedef bsl::unordered_map<bmqt::MessageGUID, MessageDetails>
        MessagesDetails;

    // DATA

    bsl::ostream&                                    d_ostream;
    bool                                             d_withDetails;
    bool                                             d_dumpPayload;
    unsigned int                                     d_dumpLimit;
    Parameters::FileHandler<mqbs::DataFileIterator>* d_dataFile;
    Filters&                                         d_filters;
    bsl::size_t                                      d_totalMessagesCount;
    bsl::size_t                                      d_foundMessagesCount;
    bsl::size_t                                      d_deletedMessagesCount;
    bslma::Allocator*                                d_allocator_p;

    MessagesDetails d_messagesDetails;

    bsl::map<bsls::Types::Uint64, bmqt::MessageGUID> dataRecordOffsetMap;
    // Map to store sorted data records offset for data file iterator.

    bsl::map<bsls::Types::Uint64, bmqt::MessageGUID> d_messageIndexToGuidMap;
    // Map to store sorted indexes to preserve messages order for output.

    // MANIPULATORS
    void addMessageDetails(const mqbs::MessageRecord& record,
                           bsls::Types::Uint64        recordIndex,
                           bsls::Types::Uint64        recordOffset);
    void deleteMessageDetails(MessagesDetails::iterator iterator);
    void deleteMessageDetails(const bmqt::MessageGUID& messageGUID,
                              bsls::Types::Uint64      recordIndex);

  public:
    // CREATORS
    explicit SearchResult(
        bsl::ostream&                                    ostream,
        bool                                             withDetails,
        bool                                             d_dumpPayload,
        unsigned int                                     d_dumpLimit,
        Parameters::FileHandler<mqbs::DataFileIterator>* d_dataFile,
        Filters&                                         filters,
        bslma::Allocator*                                allocator);
    virtual ~SearchResult() = default;

    // MANIPULATORS
    virtual bool processMessageRecord(const mqbs::MessageRecord& record,
                                      bsls::Types::Uint64        recordIndex,
                                      bsls::Types::Uint64        recordOffset);
    virtual bool processConfirmRecord(const mqbs::ConfirmRecord& record,
                                      bsls::Types::Uint64        recordIndex,
                                      bsls::Types::Uint64        recordOffset);
    virtual bool processDeletionRecord(const mqbs::DeletionRecord& record,
                                       bsls::Types::Uint64         recordIndex,
                                       bsls::Types::Uint64 recordOffset);
    virtual void outputResult(bool outputRatio = true);
    void         outputGuidString(const bmqt::MessageGUID& messageGUID,
                                  const bool               addNewLine = true);
    void         outputFooter();
    void         outputPayload(bsls::Types::Uint64 dataRecordOffset);
    void         outputOutstandingRatio();
};

// =====================
// class SearchAllResult
// =====================
class SearchAllResult : public SearchResult {
  public:
    // CREATORS
    explicit SearchAllResult(
        bsl::ostream&                                    ostream,
        bool                                             withDetails,
        bool                                             d_dumpPayload,
        unsigned int                                     d_dumpLimit,
        Parameters::FileHandler<mqbs::DataFileIterator>* d_dataFile,
        Filters&                                         filters,
        bslma::Allocator*                                allocator);

    // MANIPULATORS
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
};

// =====================
// class SearchGuidResult
// =====================
class SearchGuidResult : public SearchResult {
  private:
    bsl::unordered_map<bmqt::MessageGUID, bsl::string> d_guidsMap;

  public:
    // CREATORS
    explicit SearchGuidResult(
        bsl::ostream&                                    ostream,
        bool                                             withDetails,
        bool                                             d_dumpPayload,
        unsigned int                                     d_dumpLimit,
        Parameters::FileHandler<mqbs::DataFileIterator>* d_dataFile,
        const bsl::vector<bsl::string>&                  guids,
        Filters&                                         filters,
        bslma::Allocator*                                allocator);

    // MANIPULATORS
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    void outputResult(bool outputRatio = true) BSLS_KEYWORD_OVERRIDE;
};

// =====================
// class SearchOutstandingResult
// =====================
class SearchOutstandingResult : public SearchResult {
  public:
    // CREATORS
    explicit SearchOutstandingResult(
        bsl::ostream&                                    ostream,
        bool                                             withDetails,
        bool                                             d_dumpPayload,
        unsigned int                                     d_dumpLimit,
        Parameters::FileHandler<mqbs::DataFileIterator>* d_dataFile,
        Filters&                                         filters,
        bslma::Allocator*                                allocator);

    // MANIPULATORS
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    // void outputResult() BSLS_KEYWORD_OVERRIDE;
};

// =====================
// class SearchConfirmedResult
// =====================
class SearchConfirmedResult : public SearchResult {
  private:
    // DATA
    // bsl::unordered_set<bmqt::MessageGUID> d_messageGUIDS;

  public:
    // CREATORS
    explicit SearchConfirmedResult(
        bsl::ostream&                                    ostream,
        bool                                             withDetails,
        bool                                             d_dumpPayload,
        unsigned int                                     d_dumpLimit,
        Parameters::FileHandler<mqbs::DataFileIterator>* d_dataFile,
        Filters&                                         filters,
        bslma::Allocator*                                allocator);

    // MANIPULATORS
    bool processMessageRecord(const mqbs::MessageRecord& record,
                              bsls::Types::Uint64        recordIndex,
                              bsls::Types::Uint64        recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    bool processDeletionRecord(const mqbs::DeletionRecord& record,
                               bsls::Types::Uint64         recordIndex,
                               bsls::Types::Uint64         recordOffset)
        BSLS_KEYWORD_OVERRIDE;
    void outputResult(bool outputRatio = true) BSLS_KEYWORD_OVERRIDE;
};

// =====================
// class SearchPartiallyConfirmedResult
// =====================
class SearchPartiallyConfirmedResult : public SearchResult {
  private:
    // DATA
    bsl::unordered_map<bmqt::MessageGUID, size_t> d_partiallyConfirmedGUIDS;

  public:
    // CREATORS
    explicit SearchPartiallyConfirmedResult(
        bsl::ostream&                                    ostream,
        bool                                             withDetails,
        bool                                             d_dumpPayload,
        unsigned int                                     d_dumpLimit,
        Parameters::FileHandler<mqbs::DataFileIterator>* d_dataFile,
        Filters&                                         filters,
        bslma::Allocator*                                allocator);

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
    void outputResult(bool outputRatio = true) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
