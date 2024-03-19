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

#include <m_bmqstoragetool_parameters.h>

// BMQ
#include <bmqt_messageguid.h>
#include <bmqt_queueflags.h>

// MQB
#include <mqbc_clusterstateledgerutil.h>
#include <mqbc_incoreclusterstateledgeriterator.h>
#include <mqbmock_logidgenerator.h>
#include <mqbs_filestoreprotocolutil.h>
#include <mqbsl_ledger.h>
#include <mqbsl_memorymappedondisklog.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <bdlb_chartype.h>
#include <bdlb_string.h>
#include <bdls_filesystemutil.h>
#include <bdls_pathutil.h>
#include <bdlt_timeunitratio.h>
#include <bsl_iostream.h>
#include <bsl_sstream.h>
#include <bsl_stdexcept.h>
#include <bslim_printer.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

namespace {
// Validation helper
// StorageKey class does not have validator similar to
// MessageGUID::isValidHexRepresentation so implement own validator
bool isValidQueueKeyHexRepresentation(const char* queueKeyBuf)
{
    const size_t queueKeyHexLength =
        mqbu::StorageKey::e_KEY_LENGTH_BINARY *
        2;  // one byte is represented by two hex symbols
    if (bsl::strlen(queueKeyBuf) != queueKeyHexLength)
        return false;  // RETURN

    for (size_t i = 0; i < queueKeyHexLength; ++i) {
        if (!bsl::isxdigit(queueKeyBuf[i]) ||
            bdlb::CharType::isLower(queueKeyBuf[i])) {
            return false;  // RETURN
        }
    }
    return true;
}

}  // close unnamed namespace

// ==========================
// class CommandLineArguments
// ==========================

CommandLineArguments::CommandLineArguments(bslma::Allocator* allocator)
: d_timestampGt(0)
, d_timestampLt(0)
, d_journalPath(allocator)
, d_journalFile(allocator)
, d_dataFile(allocator)
, d_cslFile(allocator)
, d_guid(allocator)
, d_queueKey(allocator)
, d_queueName(allocator)
, d_dumpLimit(0)
, d_details(false)
, d_dumpPayload(false)
, d_summary(false)
, d_outstanding(false)
, d_confirmed(false)
, d_partiallyConfirmed(false)
{
}

bool CommandLineArguments::validate(bsl::string* error)
{
    mwcu::MemOutStream ss;

    if (d_journalPath.empty() && d_journalFile.empty()) {
        ss << "Niether journal path nor journal file are specified\n";
    }
    else if (!d_journalPath.empty()) {
        if (d_journalFile.empty() && d_dataFile.empty()) {
            // Try to find files by path
            bsl::vector<bsl::string> result;
            bdls::FilesystemUtil::findMatchingPaths(&result,
                                                    d_journalPath.c_str());
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
    if (d_timestampLt < 0 || d_timestampGt < 0 ||
        (d_timestampLt > 0 && d_timestampGt >= d_timestampLt)) {
        ss << "Invalid timestamp range specified\n";
    }
    if (!d_guid.empty() &&
        (!d_queueKey.empty() || !d_queueName.empty() || d_outstanding ||
         d_confirmed || d_partiallyConfirmed || d_timestampGt > 0 ||
         d_timestampLt > 0 || d_summary)) {
        ss << "Giud filter can't be combined with any other filters, as it is "
              "specific enough to find a particular message\n";
    }
    if (d_summary &&
        (d_outstanding || d_confirmed || d_partiallyConfirmed || d_details)) {
        ss << "'--summary' can't be combined with '--outstanding', "
              "'--confirmed', '--partially-confirmed' and '--details' "
              "options, as it is "
              "calculates and outputs statistics\n";
    }

    if (d_outstanding + d_confirmed + d_partiallyConfirmed > 1) {
        ss << "These filter flags can't be specified together: outstanding, "
              "confirmed, partially-confirmed. You can specify only one of "
              "them\n";
    }

    bsl::vector<bsl::string>::const_iterator it = d_guid.cbegin();
    for (; it != d_guid.cend(); ++it) {
        if (!bmqt::MessageGUID::isValidHexRepresentation(it->c_str())) {
            ss << *it << " is not a valid GUID\n";
        }
    }

    it = d_queueKey.cbegin();
    for (; it != d_queueKey.cend(); ++it) {
        if (!isValidQueueKeyHexRepresentation(it->c_str())) {
            ss << *it << " is not a valid Queue Key\n";
        }
    }

    if (d_dumpLimit <= 0)
        ss << "Dump limit must be positive value greater than zero.\n";

    error->assign(ss.str().data(), ss.str().length());
    return error->empty();
}

Parameters::Parameters(bslma::Allocator* allocator)
: d_queueMap(allocator)
, d_timestampGt(0)
, d_timestampLt(0)
, d_guid(allocator)
, d_queueKey(allocator)
, d_queueName(allocator)
, d_dumpLimit(0)
, d_details(false)
, d_dumpPayload(false)
, d_summary(false)
, d_outstanding(false)
, d_confirmed(false)
, d_partiallyConfirmed(false)
{
}

Parameters::Parameters(const CommandLineArguments& arguments,
                       bslma::Allocator*           allocator)
: d_queueMap(allocator)
, d_timestampGt(arguments.d_timestampGt)
, d_timestampLt(arguments.d_timestampLt)
, d_guid(arguments.d_guid, allocator)
, d_queueKey(arguments.d_queueKey, allocator)
, d_queueName(arguments.d_queueName, allocator)
, d_dumpLimit(arguments.d_dumpLimit)
, d_details(arguments.d_details)
, d_dumpPayload(arguments.d_dumpPayload)
, d_summary(arguments.d_summary)
, d_outstanding(arguments.d_outstanding)
, d_confirmed(arguments.d_confirmed)
, d_partiallyConfirmed(arguments.d_partiallyConfirmed)
{
}

void Parameters::validateQueueNames(bslma::Allocator* allocator) const
{
    // Validate given queue names agains existing in csl file
    mwcu::MemOutStream                       ss(allocator);
    mqbu::StorageKey                         key;
    bsl::vector<bsl::string>::const_iterator it = d_queueName.cbegin();
    for (; it != d_queueName.cend(); ++it) {
        if (!d_queueMap.findKeyByUri(&key, *it)) {
            ss << "Queue name: '" << *it << "' is not found in Csl file."
               << bsl::endl;
        }
    }
    if (!ss.isEmpty()) {
        throw bsl::runtime_error(ss.str());
    }
}

}  // close package namespace
}  // close enterprise namespace
