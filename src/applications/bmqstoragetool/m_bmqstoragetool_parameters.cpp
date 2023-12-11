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

// m_bmqtool_parameters.cpp                                           -*-C++-*-
#include <m_bmqstoragetool_parameters.h>

// BDE
#include <bdlb_string.h>
#include <bdls_filesystemutil.h>
#include <bdlt_timeunitratio.h>
#include <bsl_iostream.h>
#include <bsl_sstream.h>
#include <bsl_stdexcept.h>
#include <bslim_printer.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>
#include <bsls_types.h>

// BMQ
#include <bmqt_messageguid.h>
#include <bmqt_queueflags.h>

// MQB
#include <mqbs_filestoreprotocolutil.h>
#include <mqbs_filesystemutil.h>

// MWC
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// ==========================
// class CommandLineArguments
// ==========================

CommandLineArguments::CommandLineArguments(bslma::Allocator* allocator)
: d_path(allocator)
, d_journalFile(allocator)
, d_dataFile(allocator)
, d_cslFile(allocator)
, d_guid(allocator)
, d_queueKey(allocator)
, d_queueName(allocator)
, d_timestampGt(0)
, d_timestampLt(0)
, d_dumpLimit(0)
, d_dumpPayload(false)
, d_outstanding(false)
, d_confirmed(false)
, d_partiallyConfirmed(false)
{
}

bool CommandLineArguments::validate(bsl::string* error)
{
    mwcu::MemOutStream ss;

    if (d_path.empty() && d_journalFile.empty()) {
        ss << "Niether path nor journal file are specified\n";
    }
    else if (!d_path.empty()) {
        if (d_journalFile.empty() && d_dataFile.empty() && d_cslFile.empty()) {
            // Try to find files by path
            bsl::vector<bsl::string> result;
            bdls::FilesystemUtil::findMatchingPaths(&result, d_path.c_str());
            bsl::vector<bsl::string>::const_iterator it = result.cbegin();
            for (; it != result.cend(); ++it) {
                if (it->ends_with(".bmq_journal")) {
                    if (d_journalFile.empty()) {
                        d_journalFile = *it;
                    }
                    else {
                        ss << "Several journal files match the pattern, can't "
                              "define the needed one\n";
                        break;
                    }
                }
                if (it->ends_with(".bmq_data")) {
                    if (d_dataFile.empty()) {
                        d_dataFile = *it;
                    }
                    else {
                        ss << "Several data files match the pattern, can't "
                              "define the needed one\n";
                        break;
                    }
                }
                if (it->ends_with(".bmq_csl")) {
                    if (d_cslFile.empty()) {
                        d_cslFile = *it;
                    }
                    else {
                        ss << "Several CSL files match the pattern, can't "
                              "define the needed one\n";
                        break;
                    }
                }
            }
            if (d_journalFile.empty()) {
                ss << "Couldn't define a journal file, which is required\n";
            }
        }
        else {
            ss << "Both path and particular files are specified which is "
                  "controversial. Specify only one\n";
        }
    }

    // Check if the specified files exist
    if (!d_journalFile.empty() &&
        !bdls::FilesystemUtil::isRegularFile(d_journalFile)) {
        ss << "The specified journal file does not exist: " << d_journalFile
           << "\n";
    }
    if (!d_dataFile.empty() &&
        !bdls::FilesystemUtil::isRegularFile(d_dataFile)) {
        ss << "The specified data file does not exist: " << d_dataFile << "\n";
    }
    if (!d_cslFile.empty() &&
        !bdls::FilesystemUtil::isRegularFile(d_cslFile)) {
        ss << "The specified CSL file does not exist: " << d_cslFile << "\n";
    }

    // Sanity check
    if (d_dataFile.empty() && d_dumpPayload) {
        ss << "Can't dump payload, because data file is not specified\n";
    }
    if (d_cslFile.empty() && !d_queueName.empty()) {
        ss << "Can't search by queue name, because csl file is not "
              "specified\n";
    }
    if (d_timestampGt > 0 && d_timestampGt <= d_timestampLt) {
        ss << "Invalid timestamp range specified\n";
    }
    if (!d_guid.empty() &&
        (!d_queueKey.empty() || !d_queueName.empty() || d_outstanding ||
         d_confirmed || d_partiallyConfirmed || d_timestampGt > 0 ||
         d_timestampLt > 0)) {
        ss << "Giud filter can't be combined with any other filters, as it is "
              "specific enough to find a particular message\n";
    }

    if (d_outstanding + d_confirmed + d_partiallyConfirmed > 1) {
        ss << "These filter flags can't be specified together: outstanding, "
              "confirmed, partially-confirmed. You can specify only one of "
              "them\n";
    }

    bsl::vector<bsl::string>::const_iterator it = d_guid.cbegin();
    for (; it != d_guid.cend(); ++it) {
        if (!bmqt::MessageGUID::isValidHexRepresentation(it->c_str())) {
            ss << *it << "Is not a valid GUID\n";
        }
    }

    error->assign(ss.str().data(), ss.str().length());
    return error->empty();
}

// ================
// class Parameters
// ================

bsls::Types::Int64 Parameters::timestampGt() const
{
    return d_timestampGt;
}

bsls::Types::Int64 Parameters::timestampLt() const
{
    return d_timestampLt;
}

Parameters::FileHandler<mqbs::JournalFileIterator>* Parameters::journalFile()
{
    return &d_journalFile;
}

Parameters::FileHandler<mqbs::DataFileIterator>* Parameters::dataFile()
{
    return &d_dataFile;
}

bsl::string Parameters::cslFile()
{
    return d_cslFile;
}

bsl::vector<bsl::string> Parameters::guid() const
{
    return d_guid;
}

bsl::vector<bsl::string> Parameters::queueKey() const
{
    return d_queueKey;
}

bsl::vector<bsl::string> Parameters::queueName() const
{
    return d_queueName;
}

int Parameters::dumpLimit() const
{
    return d_dumpLimit;
}

bool Parameters::details() const
{
    return d_details;
}

bool Parameters::dumpPayload() const
{
    return d_dumpPayload;
}

bool Parameters::summary() const
{
    return d_summary;
}

bool Parameters::outstanding() const
{
    return d_outstanding;
}

bool Parameters::confirmed() const
{
    return d_confirmed;
}

bool Parameters::partiallyConfirmed() const
{
    return d_partiallyConfirmed;
}

Parameters::Parameters(const CommandLineArguments& arguments,
                       bslma::Allocator*           allocator)
: d_journalFile(arguments.d_journalFile)
, d_dataFile(arguments.d_dataFile)
, d_cslFile(arguments.d_cslFile)
, d_guid(arguments.d_guid, allocator)
, d_queueKey(arguments.d_queueKey, allocator)
, d_queueName(arguments.d_queueName, allocator)
, d_timestampGt(arguments.d_timestampGt)
, d_timestampLt(arguments.d_timestampLt)
, d_dumpLimit(arguments.d_dumpLimit)
, d_dumpPayload(arguments.d_dumpPayload)
, d_outstanding(arguments.d_outstanding)
, d_confirmed(arguments.d_confirmed)
, d_partiallyConfirmed(arguments.d_partiallyConfirmed)
{
    mwcu::MemOutStream ss;
    if ((!d_journalFile.path().empty() && !d_journalFile.resetIterator(ss)) ||
        (!d_dataFile.path().empty() && !d_dataFile.resetIterator(ss))) {
        throw bsl::runtime_error(ss.str());
    }
    // TODO: used for testing, consider better way
    if (d_journalFile.path().empty())
        d_journalFile.setIterator(nullptr);
    if (d_dataFile.path().empty())
        d_dataFile.setIterator(nullptr);
}

void Parameters::print(bsl::ostream& ss) const
{
    ss << "PARAMETERS :\n";
    ss << "journal-file :\t" << d_journalFile.path() << bsl::endl;
    ss << "data-file :\t" << d_dataFile.path() << bsl::endl;
    ss << "csl-file :\t" << d_cslFile << bsl::endl;
    ss << "guids :\n";
    for (bsl::size_t i = 0; i < d_guid.size(); ++i) {
        ss << "[" << i << "] :\t" << d_guid[i] << bsl::endl;
    }
    ss << "queue names:\n";
    for (bsl::size_t i = 0; i < d_queueName.size(); ++i) {
        ss << "[" << i << "] :\t" << d_queueName[i] << bsl::endl;
    }
    ss << "queue key:\n";
    for (bsl::size_t i = 0; i < d_queueKey.size(); ++i) {
        ss << "[" << i << "] :\t" << d_queueKey[i] << bsl::endl;
    }
    ss << "timestamp-gt :\t" << d_timestampGt << bsl::endl;
    ss << "timestamp-lt :\t" << d_timestampLt << bsl::endl;
    ss << "outstanding :\t" << d_outstanding << bsl::endl;
    ss << "confirmed :\t" << d_confirmed << bsl::endl;
    ss << "partiallyConfirmed :\t" << d_partiallyConfirmed << bsl::endl;
    ss << "details :\t" << d_details << bsl::endl;
    ss << "dump-payload :\t" << d_dumpPayload << bsl::endl;
    ss << "dump-limit :\t" << d_dumpLimit << bsl::endl;
    ss << "summary :\t" << d_summary << bsl::endl;
}

template <typename ITER>
Parameters::FileHandler<ITER>::FileHandler(const bsl::string& path,
                                           bslma::Allocator*  allocator)
: d_path(path, allocator)
{
}

template <typename ITER>
Parameters::FileHandler<ITER>::~FileHandler()
{
    d_iter.clear();
    if (d_mfd.isValid()) {
        mqbs::FileSystemUtil::close(&d_mfd);
    }
}

template <typename ITER>
bsl::string Parameters::FileHandler<ITER>::path() const
{
    return d_path;
}

template <typename ITER>
bool Parameters::FileHandler<ITER>::resetIterator(
    std::ostream& errorDescription)
{
    // 1) Open
    mwcu::MemOutStream errorDesc;
    int                rc = mqbs::FileSystemUtil::open(
        &d_mfd,
        d_path.c_str(),
        bdls::FilesystemUtil::getFileSize(d_path),
        true,  // read only
        errorDesc);
    if (0 != rc) {
        errorDescription << "Failed to open file [" << d_path << "] rc: " << rc
                         << ", error: " << errorDesc.str() << "\n";
        return false;  // RETURN
    }

    // 2) Basic sanity check
    rc = mqbs::FileStoreProtocolUtil::hasBmqHeader(d_mfd);
    if (0 != rc) {
        errorDescription << "Missing BlazingMQ header from file [" << d_path
                         << "] rc: " << rc << "\n";
        mqbs::FileSystemUtil::close(&d_mfd);
        return false;  // RETURN
    }

    // 3) Load iterator and check
    rc = d_iter.reset(&d_mfd, mqbs::FileStoreProtocolUtil::bmqHeader(d_mfd));
    if (0 != rc) {
        errorDescription << "Failed to create iterator for file [" << d_path
                         << "] rc: " << rc << "\n";
        mqbs::FileSystemUtil::close(&d_mfd);
        return false;  // RETURN
    }

    BSLS_ASSERT_OPT(d_iter.isValid());
    return true;  // RETURN
}

template <typename ITER>
ITER* Parameters::FileHandler<ITER>::iterator()
{
    return &d_iter;
}

// TODO: used for testing, consider better way
template <typename ITER>
void Parameters::FileHandler<ITER>::setIterator(ITER* iter)
{
    if (iterator() != nullptr && iter != nullptr)
        d_iter = *iter;
}

}  // close package namespace
}  // close enterprise namespace
