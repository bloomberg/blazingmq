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
#include <bsl_iostream.h>
#include <bdls_filesystemutil.h>

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
template <typename ITER>
bool resetIterator(mqbs::MappedFileDescriptor* mfd,
                   ITER*                       iter,
                   const char*                 filename,
                   bsl::ostream& errorDescription)
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
        errorDescription << "Failed to open file [" << filename << "] rc: " << rc
                       << ", error: " << errorDesc.str();
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

} // close unnamed namespace


// =====================
// class SearchParameters
// =====================

SearchParameters::SearchParameters()
: searchGuids()
{
    // NOTHING
}

SearchParameters::SearchParameters(bslma::Allocator* allocator)
: searchGuids(allocator)
{
    // NOTHING
}

// =====================
// class SearchProcessor
// =====================

// CREATORS
SearchProcessor::SearchProcessor() 
: d_dataFile()
, d_journalFile()
, d_searchParameters()
{
    // NOTHING
}

SearchProcessor::SearchProcessor(bslma::Allocator* allocator) 
: d_dataFile(allocator)
, d_journalFile(allocator)
, d_searchParameters(allocator)
{
    // NOTHING
}

SearchProcessor::SearchProcessor(bsl::string& journalFile, bslma::Allocator* allocator) 
: d_dataFile(allocator)
, d_journalFile(journalFile, allocator)
, d_searchParameters(allocator)
{
    // NOTHING
}

SearchProcessor::SearchProcessor(mqbs::JournalFileIterator& journalFileIter, SearchParameters& params, bslma::Allocator* allocator) 
: d_dataFile(allocator)
, d_journalFile(allocator)
, d_journalFileIter(journalFileIter)
, d_searchParameters(params)
{
    // NOTHING
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

void SearchProcessor::process(bsl::ostream& ostream) {
    // TODO: remove - Initialize journal file iterator
    if (!d_journalFileIter.isValid()) {
        if (!resetIterator(&d_journalFd,
                            &d_journalFileIter,
                            d_journalFile.c_str(), ostream)) {
            return;  // RETURN
        }
        ostream << "Created Journal iterator successfully" << bsl::endl;
    }

    if (!d_searchParameters.searchGuids.empty()) {
        // Search by GUIDs
        // TODO: use bsl::transform
        // auto transformFunc = [](bsl::string item) {
        //     bmqt::MessageGUID guid;
        //     return guid.fromHex(item.c_str());
        // };
        // bsl::transform(d_searchParameters.searchGuids.begin(), d_searchParameters.searchGuids.end(), bsl::back_inserter(guids), transformFunc);
        bsl::unordered_map<bmqt::MessageGUID, bsl::string> guids;
        for (auto& guidStr : d_searchParameters.searchGuids) {
            bmqt::MessageGUID guid;
            guids[guid.fromHex(guidStr.c_str())] = guidStr; 

        }
        mqbs::JournalFileIterator* iter   = &d_journalFileIter;
        while (true) {
            if (!iter->hasRecordSizeRemaining()) {
                ostream << "Ran out of records while iterating.";
                return;  // RETURN
            }
            int rc = iter->nextRecord();
            if (rc <= 0) {
                ostream << "Iteration aborted (exit status " << rc
                               << ").";
                return;  // RETURN
            }
            else if (iter->recordType() == mqbs::RecordType::e_MESSAGE) {
                const mqbs::MessageRecord& message = iter->asMessageRecord();
                 if (auto search = guids.find(message.messageGUID()); search != guids.end()) {
                    bsl::cout << "FOUND: " << guids[message.messageGUID()] << bsl::endl;
                 }
                 else {
                    bsl::cout << "NOT FOUND: " << bsl::endl;
                 }
            }
        }
    }
}

}  // close package namespace
}  // close enterprise namespace
