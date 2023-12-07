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
// #include <m_bmqstoragetool_searchprocessor.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_unordered_map.h>
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
    const bsl::string d_foundGuidCaption    = " message GUID(s) found.";
    const bsl::string d_notFoundGuidCaption = "No message GUID found.";

    // DATA
    bsl::ostream&     d_ostream;
    bool              d_withDetails;
    bsl::size_t       d_totalMessagesCount;
    bsl::size_t       d_foundMessagesCount;
    bslma::Allocator* d_allocator_p;

  public:
    // CREATORS
    explicit SearchResult(bsl::ostream&     ostream,
                          bool              withDetails,
                          bslma::Allocator* allocator);
    virtual ~SearchResult() = default;

    // MANIPULATORS
    virtual bool processMessageRecord(const mqbs::MessageRecord& record);
    virtual bool processConfirmRecord(const mqbs::ConfirmRecord& record);
    virtual bool processDeletionRecord(const mqbs::DeletionRecord& record);
    virtual void outputResult();
    void         outputGuidString(const bmqt::MessageGUID& messageGUID,
                                  const bool               addNewLine = true);
    void         outputFooter();
    void         outputOutstandingRatio();
};

// =====================
// class SearchAllResult
// =====================
class SearchAllResult : public SearchResult {
  public:
    // CREATORS
    explicit SearchAllResult(bsl::ostream&     ostream,
                             bool              withDetails,
                             bslma::Allocator* allocator);

    // MANIPULATORS
    bool processMessageRecord(const mqbs::MessageRecord& record)
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
    explicit SearchGuidResult(bsl::ostream&                   ostream,
                              bool                            withDetails,
                              const bsl::vector<bsl::string>& guids,
                              bslma::Allocator*               allocator);

    // MANIPULATORS
    bool processMessageRecord(const mqbs::MessageRecord& record)
        BSLS_KEYWORD_OVERRIDE;
};

// =====================
// class SearchOutstandingResult
// =====================
class SearchOutstandingResult : public SearchResult {
  private:
    // DATA
    bsl::vector<bmqt::MessageGUID> d_outstandingGUIDS;

  public:
    // CREATORS
    explicit SearchOutstandingResult(bsl::ostream&     ostream,
                                     bool              withDetails,
                                     bslma::Allocator* allocator);

    // MANIPULATORS
    bool processMessageRecord(const mqbs::MessageRecord& record)
        BSLS_KEYWORD_OVERRIDE;
    bool processDeletionRecord(const mqbs::DeletionRecord& record)
        BSLS_KEYWORD_OVERRIDE;
    void outputResult() BSLS_KEYWORD_OVERRIDE;
};

// =====================
// class SearchConfirmedResult
// =====================
class SearchConfirmedResult : public SearchResult {
  private:
    // DATA
    bsl::vector<bmqt::MessageGUID> d_messageGUIDS;

  public:
    // CREATORS
    explicit SearchConfirmedResult(bsl::ostream&     ostream,
                                   bool              withDetails,
                                   bslma::Allocator* allocator);

    // MANIPULATORS
    bool processMessageRecord(const mqbs::MessageRecord& record)
        BSLS_KEYWORD_OVERRIDE;
    bool processDeletionRecord(const mqbs::DeletionRecord& record)
        BSLS_KEYWORD_OVERRIDE;
    void outputResult() BSLS_KEYWORD_OVERRIDE;
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
    explicit SearchPartiallyConfirmedResult(bsl::ostream&     ostream,
                                            bool              withDetails,
                                            bslma::Allocator* allocator);

    // MANIPULATORS
    bool processMessageRecord(const mqbs::MessageRecord& record)
        BSLS_KEYWORD_OVERRIDE;
    bool processConfirmRecord(const mqbs::ConfirmRecord& record)
        BSLS_KEYWORD_OVERRIDE;
    bool processDeletionRecord(const mqbs::DeletionRecord& record)
        BSLS_KEYWORD_OVERRIDE;
    void outputResult() BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace
