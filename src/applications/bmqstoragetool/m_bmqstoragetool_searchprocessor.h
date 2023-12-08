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
#include <m_bmqstoragetool_commandprocessor.h>

// MQB
#include <mqbs_datafileiterator.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_journalfileiterator.h>
#include <mqbs_mappedfiledescriptor.h>
// #include <mqbu_storagekey.h>

// BDE
#include <bsls_keyword.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =====================
// class SearchProcessor
// =====================

class SearchProcessor : public CommandProcessor {
  private:
    enum SearchMode { k_ALL, k_LIST, k_OUTSTANDING };

    // TODO: refactor to class, move to separate file for sharing.
    // VST representing message details.
    struct MessageDetails {
        mqbs::MessageRecord              messageRecord;
        bsl::vector<mqbs::ConfirmRecord> confirmRecords;
        mqbs::DeletionRecordFlag::Enum   deleteRecordFlag;
    };

    typedef bsl::unordered_map<bmqt::MessageGUID, MessageDetails>
        MessagesDetails;

    // DATA
    bsl::string d_dataFile;

    bsl::string d_journalFile;

    mqbs::MappedFileDescriptor d_dataFd;

    mqbs::MappedFileDescriptor d_journalFd;

    mqbs::DataFileIterator d_dataFileIter;

    mqbs::JournalFileIterator d_journalFileIter;

    // MANIPULATORS
    void outputSearchResult(bsl::ostream&          ostream,
                            const SearchMode       mode,
                            const MessagesDetails& messagesDetails,
                            const bsl::size_t      messagesCount,
                            const bsl::size_t      totalMessagesCount);

    // ACCESSORS
    void outputGuidString(bsl::ostream&            ostream,
                          const bmqt::MessageGUID& messageGUID,
                          const bool               addNewLine = true);

  public:
    // CREATORS
    SearchProcessor(const bsl::shared_ptr<Parameters>& params,
                    bsl::string&                       journalFile,
                    bslma::Allocator*                  allocator);
    SearchProcessor(const bsl::shared_ptr<Parameters>& params,
                    mqbs::JournalFileIterator&         journalFileIter,
                    bslma::Allocator*                  allocator);
    explicit SearchProcessor(const bsl::shared_ptr<Parameters>& params);

    ~SearchProcessor();

    // MANIPULATORS
    void process(bsl::ostream& ostream) BSLS_KEYWORD_OVERRIDE;

    // TODO: remove
    mqbs::JournalFileIterator& getJournalFileIter()
    {
        return d_journalFileIter;
    }
};

}  // close package namespace
}  // close enterprise namespace
