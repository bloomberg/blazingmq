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

// BMQ
#include <bmqt_queueflags.h>
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>

// BDE
#include <bdlb_string.h>
#include <bdls_filesystemutil.h>
#include <bdlt_timeunitratio.h>
#include <bsl_iostream.h>
#include <bsl_sstream.h>
#include <bslim_printer.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// ================
// class Parameters
// ================

bsls::Types::Int64 Parameters::timestampGt() const
{
    return d_timestampGt;
}

bsls::Types::Int64& Parameters::setTimestampGt()
{
    return d_timestampGt;
}

bsls::Types::Int64 Parameters::timestampLt() const
{
    return d_timestampLt;
}

bsls::Types::Int64& Parameters::setTimestampLt()
{
    return d_timestampLt;
}

bsl::string Parameters::path() const
{
    return d_path;
}

bsl::string& Parameters::setPath()
{
    return d_path;
}

bsl::string Parameters::journalFile() const
{
    return d_journalFile;
}

bsl::string& Parameters::setJournalFile()
{
    return d_journalFile;
}

bsl::string Parameters::dataFile() const
{
    return d_dataFile;
}

bsl::string& Parameters::setDataFile()
{
    return d_dataFile;
}

bsl::string Parameters::cslFile() const
{
    return d_cslFile;
}

bsl::string& Parameters::setCslFile()
{
    return d_cslFile;
}

bsl::vector<bsl::string> Parameters::guid() const
{
    return d_guid;
}

bsl::vector<bsl::string>& Parameters::setGuid()
{
    return d_guid;
}

bsl::vector<bsl::string> Parameters::queueKey() const
{
    return d_queueKey;
}

bsl::vector<bsl::string>& Parameters::setQueueKey()
{
    return d_queueKey;
}

bsl::vector<bsl::string> Parameters::queueName() const
{
    return d_queueName;
}

bsl::vector<bsl::string>& Parameters::setQueueName()
{
    return d_queueName;
}

int Parameters::dumpLimit() const
{
    return d_dumpLimit;
}

int& Parameters::setDumpLimit()
{
    return d_dumpLimit;
}

bool Parameters::details() const
{
    return d_details;
}

bool& Parameters::setDetails()
{
    return d_details;
}

bool Parameters::dumpPayload() const
{
    return d_dumpPayload;
}

bool& Parameters::setDumpPayload()
{
    return d_dumpPayload;
}

bool Parameters::summary() const
{
    return d_summary;
}

bool& Parameters::setSummary()
{
    return d_summary;
}

bool Parameters::outstanding() const
{
    return d_outstanding;
}

bool& Parameters::setOutstanding()
{
    return d_outstanding;
}

bool Parameters::confirmed() const
{
    return d_confirmed;
}

bool& Parameters::setConfirmed()
{
    return d_confirmed;
}

bool Parameters::partiallyConfirmed() const
{
    return d_partiallyConfirmed;
}

bool& Parameters::setPartiallyConfirmed()
{
    return d_partiallyConfirmed;
}

Parameters::Parameters(bslma::Allocator* allocator)
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

bool Parameters::validate(bsl::string* error)
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
                bsl::cout << *it << bsl::endl;
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
    if (d_guid.empty() &&
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

    error->assign(ss.str().data(), ss.str().length());
    return error->empty();
}

void Parameters::print(bsl::ostream& ss) const
{
    ss << "PARAMETERS :\n";
    ss << "path :\t" << d_path << bsl::endl;
    ss << "journal-file :\t" << d_journalFile << bsl::endl;
    ss << "data-file :\t" << d_dataFile << bsl::endl;
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

}  // close package namespace
}  // close enterprise namespace
