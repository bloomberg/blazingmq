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
#include <m_bmqstoragetool_searchresult.h>

// BDE
#include <bdls_filesystemutil.h>
#include <bsl_iostream.h>
#include <bsls_assert.h>

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

SearchProcessor::SearchProcessor(const bsl::shared_ptr<Parameters>& params,
                                 bsl::string&      journalFile,
                                 bslma::Allocator* allocator)
: CommandProcessor(params)
, d_dataFile(allocator)
, d_journalFile(journalFile, allocator)
, d_allocator_p(bslma::Default::allocator(allocator))
{
    // NOTHING
}

SearchProcessor::SearchProcessor(const bsl::shared_ptr<Parameters>& params,
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

SearchProcessor::SearchProcessor(const bsl::shared_ptr<Parameters>& params)
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
    // d_parameters->print(ostream);

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

    // TODO: why unique_ptr doesn't support deleter in reset()
    // bsl::unique_ptr<SearchResult> searchResult_p;
    bsl::shared_ptr<SearchResult> searchResult_p;
    if (!d_parameters->guid().empty()) {
        searchResult_p.reset(new (*d_allocator_p)
                                 SearchGuidResult(ostream,
                                                  d_parameters->details(),
                                                  d_parameters->guid(),
                                                  d_allocator_p),
                             d_allocator_p);
    }
    else if (d_parameters->outstanding()) {
        searchResult_p.reset(new (*d_allocator_p) SearchOutstandingResult(
                                 ostream,
                                 d_parameters->details(),
                                 d_allocator_p),
                             d_allocator_p);
    }
    else if (d_parameters->confirmed()) {
        searchResult_p.reset(new (*d_allocator_p)
                                 SearchConfirmedResult(ostream,
                                                       d_parameters->details(),
                                                       d_allocator_p),
                             d_allocator_p);
    }
    else if (d_parameters->partiallyConfirmed()) {
        searchResult_p.reset(
            new (*d_allocator_p)
                SearchPartiallyConfirmedResult(ostream,
                                               d_parameters->details(),
                                               d_allocator_p),
            d_allocator_p);
    }
    else {
        searchResult_p.reset(new (*d_allocator_p)
                                 SearchAllResult(ostream,
                                                 d_parameters->details(),
                                                 d_allocator_p),
                             d_allocator_p);
    }
    BSLS_ASSERT(searchResult_p);

    bool stopSearch = false;

    // Iterate through all Journal file records
    mqbs::JournalFileIterator* iter = &d_journalFileIter;
    while (true) {
        if (stopSearch || !iter->hasRecordSizeRemaining()) {
            searchResult_p->outputResult();
            return;  // RETURN
        }

        int rc = iter->nextRecord();
        if (rc <= 0) {
            ostream << "Iteration aborted (exit status " << rc << ").";
            return;  // RETURN
        }
        // MessageRecord
        else if (iter->recordType() == mqbs::RecordType::e_MESSAGE) {
            const mqbs::MessageRecord& message = iter->asMessageRecord();
            stopSearch = searchResult_p->processMessageRecord(message);
        }
        // ConfirmRecord
        else if (iter->recordType() == mqbs::RecordType::e_CONFIRM) {
            const mqbs::ConfirmRecord& record = iter->asConfirmRecord();
            stopSearch = searchResult_p->processConfirmRecord(record);
        }
        // DeletionRecord
        else if (iter->recordType() == mqbs::RecordType::e_DELETION) {
            const mqbs::DeletionRecord& record = iter->asDeletionRecord();
            stopSearch = searchResult_p->processDeletionRecord(record);
        }
    }
}

}  // close package namespace
}  // close enterprise namespace
