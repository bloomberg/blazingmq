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
#include <m_bmqstoragetool_searchprocessor.h>

// BDE
#include <bdls_filesystemutil.h>
#include <bsl_iostream.h>

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

// TODO: remove
template <typename ITER>
bool resetIterator(mqbs::MappedFileDescriptor* mfd,
                   ITER*                       iter,
                   const char*                 filename,
                   bsl::ostream&               errorDescription)
{
    if (!bdls::FilesystemUtil::isRegularFile(filename)) {
        errorDescription << "File [" << filename << "] is not a regular file.";
        return false;  // RETURN
    }

    // 1) Open
    mwcu::MemOutStream errorDesc;
    int                rc = mqbs::FileSystemUtil::open(
        mfd,
        filename,
        bdls::FilesystemUtil::getFileSize(filename),
        true,  // read only
        errorDesc);
    if (0 != rc) {
        errorDescription << "Failed to open file [" << filename
                         << "] rc: " << rc << ", error: " << errorDesc.str();
        return false;  // RETURN
    }

    // 2) Basic sanity check
    rc = mqbs::FileStoreProtocolUtil::hasBmqHeader(*mfd);
    if (0 != rc) {
        errorDescription << "Missing BlazingMQ header from file [" << filename
                         << "] rc: " << rc;
        mqbs::FileSystemUtil::close(mfd);
        return false;  // RETURN
    }

    // 3) Load iterator and check
    rc = iter->reset(mfd, mqbs::FileStoreProtocolUtil::bmqHeader(*mfd));
    if (0 != rc) {
        errorDescription << "Failed to create iterator for file [" << filename
                         << "] rc: " << rc;
        mqbs::FileSystemUtil::close(mfd);
        return false;  // RETURN
    }

    BSLS_ASSERT_OPT(iter->isValid());
    return true;  // RETURN
}

}  // close unnamed namespace

// =====================
// class SearchParameters
// =====================

SearchParameters::SearchParameters()
: searchGuids()
, searchOutstanding(false)
{
    // NOTHING
}

SearchParameters::SearchParameters(bslma::Allocator* allocator)
: searchGuids(allocator)
, searchOutstanding(false)
{
    // NOTHING
}

// =====================
// class SearchProcessor
// =====================

// CREATORS

SearchProcessor::SearchProcessor(const Parameters& params,
                                 bsl::string&      journalFile,
                                 bslma::Allocator* allocator)
: CommandProcessor(params)
, d_dataFile(allocator)
, d_journalFile(journalFile, allocator)
, d_searchParameters(allocator)
{
    // NOTHING
}

SearchProcessor::SearchProcessor(const Parameters&          params,
                                 mqbs::JournalFileIterator& journalFileIter,
                                 SearchParameters&          searchParams,
                                 bslma::Allocator*          allocator)
: CommandProcessor(params)
, d_dataFile(allocator)
, d_journalFile(allocator)
, d_journalFileIter(journalFileIter)
, d_searchParameters(searchParams)
{
    // NOTHING
}

SearchProcessor::SearchProcessor(const Parameters& params)
: CommandProcessor(params)
{
}

SearchProcessor::~SearchProcessor()
{
    d_dataFileIter.clear();
    d_journalFileIter.clear();

    if (d_dataFd.isValid()) {
        mqbs::FileSystemUtil::close(&d_dataFd);
    }

    if (d_journalFd.isValid()) {
        mqbs::FileSystemUtil::close(&d_journalFd);
    }
}

void SearchProcessor::process(bsl::ostream& ostream)
{
    // ostream << "SearchProcessor::process()\n";
    // d_parameters.print(ostream);

    // TODO: remove - Initialize journal file iterator from real file
    if (!d_journalFileIter.isValid()) {
        if (!resetIterator(&d_journalFd,
                           &d_journalFileIter,
                           d_journalFile.c_str(),
                           ostream)) {
            return;  // RETURN
        }
        ostream << "Created Journal iterator successfully" << bsl::endl;
    }

    SearchMode mode = d_searchParameters.searchGuids.empty()
                          ? SearchMode::k_ALL
                          : SearchMode::k_LIST;
    if (d_searchParameters.searchOutstanding)
        mode = SearchMode::k_OUTSTANDING;

    bsl::size_t     foundMessagesCount = 0;
    bsl::size_t     totalMessagesCount = 0;
    MessagesDetails messagesDetails;

    // Build MessageGUID->StrGUID Map
    bsl::unordered_map<bmqt::MessageGUID, bsl::string> guidsMap;
    if (mode == SearchMode::k_LIST) {
        for (const auto& guidStr : d_searchParameters.searchGuids) {
            bmqt::MessageGUID guid;
            guidsMap[guid.fromHex(guidStr.c_str())] = guidStr;
        }
    }

    // Iterate through Journal file records
    mqbs::JournalFileIterator* iter = &d_journalFileIter;
    while (true) {
        if (!iter->hasRecordSizeRemaining()) {
            // End of journal file reached, return...
            outputSearchResult(ostream,
                               mode,
                               messagesDetails,
                               foundMessagesCount,
                               totalMessagesCount);
            return;  // RETURN
        }

        int rc = iter->nextRecord();
        if (rc <= 0) {
            ostream << "Iteration aborted (exit status " << rc << ").";
            return;  // RETURN
        }
        else if (iter->recordType() == mqbs::RecordType::e_MESSAGE) {
            totalMessagesCount++;
            const mqbs::MessageRecord& message = iter->asMessageRecord();
            switch (mode) {
            case SearchMode::k_ALL:
                outputGuidString(ostream, message.messageGUID());
                foundMessagesCount++;
                break;
            case SearchMode::k_LIST:
                if (auto foundGUID = guidsMap.find(message.messageGUID());
                    foundGUID != guidsMap.end()) {
                    // Output result and remove processed GUID from map
                    ostream << foundGUID->second << bsl::endl;
                    guidsMap.erase(foundGUID);
                    foundMessagesCount++;
                    if (guidsMap.empty()) {
                        // All GUIDs are found, return...
                        outputSearchResult(ostream,
                                           mode,
                                           messagesDetails,
                                           foundMessagesCount,
                                           totalMessagesCount);
                        return;  // RETURN
                    }
                }
                break;
            case SearchMode::k_OUTSTANDING:
                MessageDetails messageDetails;
                messageDetails.messageRecord           = message;
                messagesDetails[message.messageGUID()] = messageDetails;
                break;
            }
        }
        else if (iter->recordType() == mqbs::RecordType::e_DELETION) {
            const mqbs::DeletionRecord& deletion = iter->asDeletionRecord();
            if (mode == SearchMode::k_OUTSTANDING) {
                if (auto foundGUID = messagesDetails.find(
                        deletion.messageGUID());
                    foundGUID != messagesDetails.end()) {
                    // Message is not outstanding, remove it.
                    messagesDetails.erase(foundGUID);
                }
                else {
                    foundMessagesCount++;
                }
            }
        }
    }
}

void SearchProcessor::outputGuidString(bsl::ostream&            ostream,
                                       const bmqt::MessageGUID& messageGUID,
                                       const bool               addNewLine)
{
    char buf[bmqt::MessageGUID::e_SIZE_HEX];
    messageGUID.toHex(buf);
    ostream.write(buf, bmqt::MessageGUID::e_SIZE_HEX);
    if (addNewLine)
        ostream << bsl::endl;
}

void SearchProcessor::outputSearchResult(
    bsl::ostream&          ostream,
    const SearchMode       mode,
    const MessagesDetails& messagesDetails,
    const bsl::size_t      messagesCount,
    const bsl::size_t      totalMessagesCount)
{
    const bsl::string foundCaption    = " message GUID(s) found.";
    const bsl::string notFoundCaption = "No message GUID found.";

    // Helper lambdas
    auto outputFooter = [&]() {
        messagesCount > 0 ? (ostream << messagesCount << foundCaption)
                          : ostream << notFoundCaption;
        ostream << bsl::endl;
    };

    switch (mode) {
    case SearchMode::k_ALL:
    case SearchMode::k_LIST: outputFooter(); break;
    case SearchMode::k_OUTSTANDING:
        auto outstandingMessagesCount = messagesDetails.size();
        if (outstandingMessagesCount > 0) {
            for (const auto& messageDetails : messagesDetails) {
                outputGuidString(ostream, messageDetails.first);
            }
            outputFooter();
            ostream << "Outstanding ratio: "
                    << float(outstandingMessagesCount) / totalMessagesCount *
                           100.0
                    << "%" << bsl::endl;
        }
        else
            outputFooter();
        break;
    }
}

}  // close package namespace
}  // close enterprise namespace
