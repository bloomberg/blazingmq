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
// class SearchProcessor
// =====================

// CREATORS

SearchProcessor::SearchProcessor(const Parameters& params,
                                 bsl::string&      journalFile,
                                 bslma::Allocator* allocator)
: CommandProcessor(params)
, d_dataFile(allocator)
, d_journalFile(journalFile, allocator)
, d_allocator_p(bslma::Default::allocator(allocator))
{
    // NOTHING
}

SearchProcessor::SearchProcessor(const Parameters&          params,
                                 mqbs::JournalFileIterator& journalFileIter,
                                 bslma::Allocator*          allocator)
: CommandProcessor(params)
, d_dataFile(allocator)
, d_journalFile(allocator)
, d_journalFileIter(journalFileIter)
, d_allocator_p(bslma::Default::allocator(allocator))
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

    SearchMode mode = SearchMode::k_ALL;
    if (!d_parameters.guid().empty())
        mode = SearchMode::k_LIST;
    else if (d_parameters.outstanding())
        mode = SearchMode::k_OUTSTANDING;
    else if (d_parameters.confirmed())
        mode = SearchMode::k_CONFIRMED;
    else if (d_parameters.partiallyConfirmed())
        mode = SearchMode::k_PARTIALLY_CONFIRMED;

    bsl::size_t     foundMessagesCount = 0;
    bsl::size_t     totalMessagesCount = 0;
    MessagesDetails messagesDetails;

    // Build MessageGUID->StrGUID Map
    bsl::unordered_map<bmqt::MessageGUID, bsl::string> guidsMap(d_allocator_p);
    if (mode == SearchMode::k_LIST) {
        for (const auto& guidStr : d_parameters.guid()) {
            bmqt::MessageGUID guid;
            guidsMap[guid.fromHex(guidStr.c_str())] = guidStr;
        }
    }

    // Iterate through all Journal file records
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
        // MessageRecord
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
                    // Output result immediately and remove processed GUID from
                    // map
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
            default:
                MessageDetails messageDetails;
                // messageDetails.messageRecord           = message;  // TODO:
                // only when details needed
                messagesDetails[message.messageGUID()] = messageDetails;
                break;
            }
        }
        // ConfirmRecord
        else if (iter->recordType() == mqbs::RecordType::e_CONFIRM) {
            const mqbs::ConfirmRecord& confirm = iter->asConfirmRecord();
            if (auto foundGUID = messagesDetails.find(confirm.messageGUID());
                foundGUID != messagesDetails.end()) {
                if (mode == SearchMode::k_PARTIALLY_CONFIRMED) {
                    // foundGUID->second.confirmRecords.push_back(confirm); //
                    // TODO: only when details needed
                    if (!foundGUID->second.partiallyConfirmed) {
                        foundGUID->second.partiallyConfirmed = true;
                    }
                }
            }
        }
        // DeletionRecord
        else if (iter->recordType() == mqbs::RecordType::e_DELETION) {
            const mqbs::DeletionRecord& deletion = iter->asDeletionRecord();
            if (mode == SearchMode::k_OUTSTANDING ||
                mode == SearchMode::k_CONFIRMED ||
                mode == SearchMode::k_PARTIALLY_CONFIRMED) {
                if (auto foundGUID = messagesDetails.find(
                        deletion.messageGUID());
                    foundGUID != messagesDetails.end()) {
                    if (mode == SearchMode::k_CONFIRMED) {
                        // Output found message data immediately.
                        outputGuidString(ostream, deletion.messageGUID());
                        foundMessagesCount++;
                    }
                    // Message is confirmed (not outstanding and not partialy
                    // confirmed), remove it.
                    messagesDetails.erase(foundGUID);
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

    bsl::size_t foundMessagesCount = messagesCount;
    // Helper lambdas
    auto outputFooter = [&](bool outputRatio = false) {
        foundMessagesCount > 0
            ? (ostream << foundMessagesCount << foundCaption)
            : ostream << notFoundCaption;
        ostream << bsl::endl;
        if (outputRatio && foundMessagesCount) {
            ostream << "Outstanding ratio: "
                    << float(foundMessagesCount) / totalMessagesCount * 100.0
                    << "%" << bsl::endl;
        }
    };

    switch (mode) {
    case SearchMode::k_ALL:
    case SearchMode::k_LIST: outputFooter(); break;
    case SearchMode::k_OUTSTANDING: {
        foundMessagesCount = messagesDetails.size();
        for (const auto& messageDetails : messagesDetails) {
            outputGuidString(ostream, messageDetails.first);
        }
        outputFooter(true);
        break;
    }
    case SearchMode::k_CONFIRMED: outputFooter(true); break;
    case SearchMode::k_PARTIALLY_CONFIRMED: {
        foundMessagesCount = 0;
        for (const auto& messageDetails : messagesDetails) {
            if (messageDetails.second.partiallyConfirmed) {
                outputGuidString(ostream, messageDetails.first);
                foundMessagesCount++;
            }
        }
        outputFooter(true);
        break;
    }
    }
}

}  // close package namespace
}  // close enterprise namespace
