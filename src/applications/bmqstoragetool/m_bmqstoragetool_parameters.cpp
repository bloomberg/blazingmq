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

#include "m_bmqstoragetool_compositesequencenumber.h"
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

// BMQ
#include <bmqu_memoutstream.h>

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

bool isValidSequenceNumber(bsl::ostream& error, const bsl::string& seqNumStr)
{
    CompositeSequenceNumber seqNum;
    bool                    success = false;
    seqNum.fromString(&success, error, seqNumStr);
    return success;
}

}  // close unnamed namespace

// ==========================
// class CommandLineArguments
// ==========================

const char* CommandLineArguments::k_MESSAGE_TYPE      = "message";
const char* CommandLineArguments::k_QUEUEOP_TYPE      = "queue-op";
const char* CommandLineArguments::k_JOURNALOP_TYPE    = "journal-op";
const char* CommandLineArguments::k_CSL_SNAPSHOT_TYPE = "snapshot";
const char* CommandLineArguments::k_CSL_UPDATE_TYPE   = "update";
const char* CommandLineArguments::k_CSL_COMMIT_TYPE   = "commit";
const char* CommandLineArguments::k_CSL_ACK_TYPE      = "ack";
const char* CommandLineArguments::k_HUMAN_MODE        = "human";
const char* CommandLineArguments::k_JSON_PRETTY_MODE  = "json-pretty";
const char* CommandLineArguments::k_JSON_LINE_MODE    = "json-line";

CommandLineArguments::CommandLineArguments(bslma::Allocator* allocator)
: d_recordType(allocator)
, d_cslRecordType(allocator)
, d_timestampGt(0)
, d_timestampLt(0)
, d_seqNumGt(allocator)
, d_seqNumLt(allocator)
, d_offsetGt(0)
, d_offsetLt(0)
, d_journalPath(allocator)
, d_journalFile(allocator)
, d_dataFile(allocator)
, d_cslFile(allocator)
, d_cslFromBegin(false)
, d_printMode(allocator)
, d_guid(allocator)
, d_seqNum(allocator)
, d_offset(allocator)
, d_queueKey(allocator)
, d_queueName(allocator)
, d_dumpLimit(0)
, d_details(false)
, d_dumpPayload(false)
, d_summary(false)
, d_outstanding(false)
, d_confirmed(false)
, d_partiallyConfirmed(false)
, d_minRecordsPerQueue(0)
, d_cslSummaryQueuesLimit(0)
{
    // NOTHING
}

bool CommandLineArguments::validate(bsl::string*      error_p,
                                    bslma::Allocator* allocator)
{
    bmqu::MemOutStream ss(allocator);

    // Determine the mode: journal or CSL iteration.
    bool validMode = true;
    if (d_recordType.size() > 0 && d_cslRecordType.size() > 0) {
        ss << "Either record type(s) or CSL record type(s) can be passed. "
              "Both passed.\n";
        validMode = false;
    }

    if (validMode) {
        if (!d_cslFile.empty() &&
            (d_journalPath.empty() && d_journalFile.empty())) {
            // Validate CSL mode args
            validateCslModeArgs(ss, allocator);
        }
        else {
            validateJournalModeArgs(ss, allocator);
        }
    }

    error_p->assign(ss.str().data(), ss.str().length());
    return error_p->empty();
}

void CommandLineArguments::validateCslModeArgs(bsl::ostream&     stream,
                                               bslma::Allocator* allocator)
{
    // Validate record types
    if (d_cslRecordType.size() > 4) {
        stream << "Up to 4 types of CSL record are supported, passed: "
               << d_recordType.size() << "\n";
    }

    if (d_recordType.size() > 0) {
        stream << "--record-type is not supported when only CSL file is "
                  "passed.\n";
    }

    const bool rangeArgPresent = validateRangeArgs(stream, allocator);

    // Validate options compatibility
    if (!d_seqNum.empty() &&
        (!d_queueKey.empty() || !d_queueName.empty() || !d_offset.empty() ||
         rangeArgPresent || d_summary)) {
        stream << "Sequence number filter can't be combined with any other "
                  "filters, as it "
                  "is "
                  "specific enough to find a particular record\n";
    }
    if (!d_offset.empty() &&
        (!d_queueKey.empty() || !d_queueName.empty() || !d_seqNum.empty() ||
         rangeArgPresent || d_summary)) {
        stream
            << "Offset filter can't be combined with any other filters, as it "
               "is "
               "specific enough to find a particular record\n";
    }
    if (!d_seqNum.empty()) {
        bmqu::MemOutStream errorDescr(allocator);
        for (bsl::vector<bsl::string>::const_iterator cit = d_seqNum.begin();
             cit != d_seqNum.end();
             ++cit) {
            if (!isValidSequenceNumber(errorDescr, *cit)) {
                stream << "--seqnum: " << errorDescr.str() << "\n";
                errorDescr.reset();
            }
        }
    }
    if (!d_offset.empty()) {
        for (bsl::vector<bsls::Types::Int64>::const_iterator cit =
                 d_offset.begin();
             cit != d_offset.end();
             ++cit) {
            if (*cit < 0) {
                stream << "--offset: " << *cit << " cannot be negative\n";
            }
        }
    }

    if (d_summary && d_details) {
        stream << "'--summary' can't be combined with '--details' "
                  "options, as it "
                  "calculates and outputs statistics\n";
    }

    if (d_outstanding + d_confirmed + d_partiallyConfirmed > 1) {
        stream
            << "These filter flags can't be specified together: outstanding, "
               "confirmed, partially-confirmed. You can specify only one of "
               "them\n";
    }

    bsl::vector<bsl::string>::const_iterator it = d_queueKey.cbegin();
    for (; it != d_queueKey.cend(); ++it) {
        if (!isValidQueueKeyHexRepresentation(it->c_str())) {
            stream << *it << " is not a valid Queue Key\n";
        }
    }

    if (!d_guid.empty() || !d_dataFile.empty() || d_dumpPayload ||
        d_outstanding || d_confirmed || d_partiallyConfirmed) {
        stream
            << "--guid, --data-file, "
               "--dump-payload, --outstanding, --confirmed, "
               "--partially-confirmed options cannot be applied to CSL file "
               "and requere either --journal-path "
               "or --journal-file option.\n";
    }

    if (d_cslSummaryQueuesLimit <= 0)
        stream << "CSL summary queues limit must be positive value greater "
                  "than zero.\n";
}

void CommandLineArguments::validateJournalModeArgs(bsl::ostream&     stream,
                                                   bslma::Allocator* allocator)
{
    // Validate record types
    if (d_recordType.size() > 3) {
        stream << "Up to 3 types of record are supported, passed: "
               << d_recordType.size() << "\n";
    }

    if (d_cslRecordType.size() > 0) {
        stream << "--csl-record-type is not supported when either journal "
                  "path or journal file are passed.\n";
    }

    // Validate journal file and path
    if (d_journalPath.empty() && d_journalFile.empty()) {
        stream << "Neither journal path nor journal file are specified\n";
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
                        stream << "Several journal files match the pattern, "
                                  "can't "
                                  "define the needed one\n";
                        break;
                    }
                }
                if (it->ends_with(".bmq_data")) {
                    if (d_dataFile.empty()) {
                        d_dataFile = *it;
                    }
                    else {
                        stream
                            << "Several data files match the pattern, can't "
                               "define the needed one\n";
                        break;
                    }
                }
            }
            if (d_journalFile.empty()) {
                stream
                    << "Couldn't define a journal file, which is required\n";
            }
        }
        else {
            stream << "Both path and particular files are specified which is "
                      "controversial. Specify only one\n";
        }
    }

    // Sanity check
    if (d_dataFile.empty() && d_dumpPayload) {
        stream << "Can't dump payload, because data file is not specified\n";
    }
    if (d_cslFile.empty() && !d_queueName.empty()) {
        stream << "Can't search by queue name, because csl file is not "
                  "specified\n";
    }

    const bool rangeArgPresent = validateRangeArgs(stream, allocator);

    // Validate options compatibility
    if (!d_guid.empty() &&
        (!d_queueKey.empty() || !d_queueName.empty() || !d_seqNum.empty() ||
         !d_offset.empty() || d_outstanding || d_confirmed ||
         d_partiallyConfirmed || rangeArgPresent || d_summary)) {
        stream << "Giud filter can't be combined with any other filters, as "
                  "it is "
                  "specific enough to find a particular message\n";
    }
    if (!d_seqNum.empty() &&
        (!d_queueKey.empty() || !d_queueName.empty() || !d_guid.empty() ||
         !d_offset.empty() || d_outstanding || d_confirmed ||
         d_partiallyConfirmed || rangeArgPresent || d_summary)) {
        stream
            << "Secnum filter can't be combined with any other filters, as it "
               "is "
               "specific enough to find a particular message\n";
    }
    if (!d_offset.empty() &&
        (!d_queueKey.empty() || !d_queueName.empty() || !d_guid.empty() ||
         !d_seqNum.empty() || d_outstanding || d_confirmed ||
         d_partiallyConfirmed || rangeArgPresent || d_summary)) {
        stream
            << "Offset filter can't be combined with any other filters, as it "
               "is "
               "specific enough to find a particular message\n";
    }
    if (!d_seqNum.empty()) {
        bmqu::MemOutStream errorDescr(allocator);
        for (bsl::vector<bsl::string>::const_iterator cit = d_seqNum.begin();
             cit != d_seqNum.end();
             ++cit) {
            if (!isValidSequenceNumber(errorDescr, *cit)) {
                stream << "--seqnum: " << errorDescr.str() << "\n";
                errorDescr.reset();
            }
        }
    }
    if (!d_offset.empty()) {
        for (bsl::vector<bsls::Types::Int64>::const_iterator cit =
                 d_offset.begin();
             cit != d_offset.end();
             ++cit) {
            if (*cit < 0) {
                stream << "--offset: " << *cit << " cannot be negative\n";
            }
        }
    }

    if (d_summary &&
        (d_outstanding || d_confirmed || d_partiallyConfirmed || d_details)) {
        stream << "'--summary' can't be combined with '--outstanding', "
                  "'--confirmed', '--partially-confirmed' and '--details' "
                  "options, as it "
                  "calculates and outputs statistics\n";
    }

    if (d_outstanding + d_confirmed + d_partiallyConfirmed > 1) {
        stream
            << "These filter flags can't be specified together: outstanding, "
               "confirmed, partially-confirmed. You can specify only one of "
               "them\n";
    }

    bsl::vector<bsl::string>::const_iterator it = d_guid.cbegin();
    for (; it != d_guid.cend(); ++it) {
        if (!bmqt::MessageGUID::isValidHexRepresentation(it->c_str())) {
            stream << *it << " is not a valid GUID\n";
        }
    }

    it = d_queueKey.cbegin();
    for (; it != d_queueKey.cend(); ++it) {
        if (!isValidQueueKeyHexRepresentation(it->c_str())) {
            stream << *it << " is not a valid Queue Key\n";
        }
    }

    if (d_dumpLimit <= 0)
        stream << "Dump limit must be positive value greater than zero.\n";
}

bool CommandLineArguments::validateRangeArgs(bsl::ostream&     error,
                                             bslma::Allocator* allocator) const
{
    if (d_timestampLt < 0 || d_timestampGt < 0 ||
        (d_timestampLt > 0 && d_timestampGt >= d_timestampLt)) {
        error << "Invalid timestamp range specified\n";
    }

    if (!d_seqNumLt.empty() || !d_seqNumGt.empty()) {
        bmqu::MemOutStream      errorDescr(allocator);
        CompositeSequenceNumber seqNumLt, seqNumGt;
        bool                    successLt = false;
        if (!d_seqNumLt.empty()) {
            seqNumLt.fromString(&successLt, errorDescr, d_seqNumLt);
            if (successLt) {
                error << "--seqnum-lt: " << errorDescr.str() << "\n";
                errorDescr.reset();
            }
        }

        bool successGt = false;
        if (!d_seqNumGt.empty()) {
            seqNumGt.fromString(&successGt, errorDescr, d_seqNumGt);
            if (successGt) {
                error << "--seqnum-gt: " << errorDescr.str() << "\n";
            }
        }

        if (successLt && successGt) {
            if (seqNumLt <= seqNumGt) {
                error << "Invalid sequence number range specified\n";
            }
        }
    }

    if (d_offsetLt < 0 || d_offsetGt < 0 ||
        (d_offsetLt > 0 && d_offsetGt >= d_offsetLt)) {
        error << "Invalid offset range specified\n";
    }

    // Check that only one range type is selected
    bsl::size_t rangesCnt = 0;
    if (d_timestampLt || d_timestampGt) {
        rangesCnt++;
    }
    if (!d_seqNumLt.empty() || !d_seqNumGt.empty()) {
        rangesCnt++;
    }
    if (d_offsetLt || d_offsetGt) {
        rangesCnt++;
    }

    if (rangesCnt > 1) {
        error << "Only one range type can be selected: timestamp, seqnum or "
                 "offset\n";
    }

    return rangesCnt > 0;
}

bool CommandLineArguments::isValidRecordType(const bsl::string* recordType,
                                             bsl::ostream&      stream)
{
    if (*recordType != k_MESSAGE_TYPE && *recordType != k_QUEUEOP_TYPE &&
        *recordType != k_JOURNALOP_TYPE) {
        stream << "--record-type invalid: " << *recordType << bsl::endl;

        return false;  // RETURN
    }

    return true;
}

bool CommandLineArguments::isValidCslRecordType(
    const bsl::string* cslRecordType,
    bsl::ostream&      stream)
{
    if (*cslRecordType != k_CSL_SNAPSHOT_TYPE &&
        *cslRecordType != k_CSL_UPDATE_TYPE &&
        *cslRecordType != k_CSL_COMMIT_TYPE &&
        *cslRecordType != k_CSL_ACK_TYPE) {
        stream << "--csl-record-type invalid: " << *cslRecordType << bsl::endl;
        return false;  // RETURN
    }

    return true;
}

bool CommandLineArguments::isValidPrintMode(const bsl::string* printMode,
                                            bsl::ostream&      stream)
{
    if (*printMode != k_HUMAN_MODE && *printMode != k_JSON_PRETTY_MODE &&
        *printMode != k_JSON_LINE_MODE) {
        stream << "--print-mode invalid: " << *printMode << bsl::endl;

        return false;  // RETURN
    }

    return true;
}

bool CommandLineArguments::isValidFileName(const bsl::string* fileName,
                                           bsl::ostream&      stream)
{
    if (!bdls::FilesystemUtil::isRegularFile(*fileName, true)) {
        stream << "The specified file does not exist: " << *fileName
               << bsl::endl;

        return false;  // RETURN
    }

    return true;
}

Parameters::Range::Range()
: d_timestampGt()
, d_timestampLt()
, d_offsetGt()
, d_offsetLt()
, d_seqNumGt()
, d_seqNumLt()
{
    // NOTHING
}

Parameters::ProcessRecordTypes::ProcessRecordTypes()
: d_message(false)
, d_queueOp(false)
, d_journalOp(false)
{
    // NOTHING
}

bool Parameters::ProcessRecordTypes::operator==(
    ProcessRecordTypes const& other) const
{
    return d_message == other.d_message && d_queueOp == other.d_queueOp &&
           d_journalOp == other.d_journalOp;
}

Parameters::ProcessCslRecordTypes::ProcessCslRecordTypes()
: d_snapshot(false)
, d_update(false)
, d_commit(false)
, d_ack(false)
{
    // NOTHING
}

bool Parameters::ProcessCslRecordTypes::operator==(
    ProcessCslRecordTypes const& other) const
{
    return d_snapshot == other.d_snapshot && d_update == other.d_update &&
           d_commit == other.d_commit && d_ack == other.d_ack;
}

Parameters::Parameters(const CommandLineArguments& arguments,
                       bslma::Allocator*           allocator)
: d_cslMode(false)
, d_processRecordTypes()
, d_processCslRecordTypes()
, d_queueMap(allocator)
, d_printMode(e_HUMAN)
, d_range()
, d_guid(arguments.d_guid, allocator)
, d_seqNum(allocator)
, d_offset(arguments.d_offset, allocator)
, d_queueKey(arguments.d_queueKey, allocator)
, d_queueName(arguments.d_queueName, allocator)
, d_dumpLimit(arguments.d_dumpLimit)
, d_details(arguments.d_details)
, d_dumpPayload(arguments.d_dumpPayload)
, d_summary(arguments.d_summary)
, d_outstanding(arguments.d_outstanding)
, d_confirmed(arguments.d_confirmed)
, d_partiallyConfirmed(arguments.d_partiallyConfirmed)
, d_cslSummaryQueuesLimit(arguments.d_cslSummaryQueuesLimit)
{
    // Determine processing mode: process Journal or CSL file
    if (!arguments.d_cslFile.empty() &&
        (arguments.d_journalPath.empty() && arguments.d_journalFile.empty())) {
        d_cslMode = true;
    }

    // Check print mode
    if (arguments.d_printMode == CommandLineArguments::k_JSON_PRETTY_MODE) {
        d_printMode = e_JSON_PRETTY;
    }
    else if (arguments.d_printMode == CommandLineArguments::k_JSON_LINE_MODE) {
        d_printMode = e_JSON_LINE;
    }
    if (d_printMode != e_HUMAN && d_dumpPayload) {
        BSLS_ASSERT(false && "Payload dumping is not supported for Json mode");
    }

    // Set record types to process
    if (d_cslMode) {
        if (arguments.d_cslRecordType.empty()) {
            // Set all CSL record types to process by default.
            d_processCslRecordTypes.d_snapshot = true;
            d_processCslRecordTypes.d_update   = true;
            d_processCslRecordTypes.d_commit   = true;
            d_processCslRecordTypes.d_ack      = true;
        }
        else {
            for (bsl::vector<bsl::string>::const_iterator cit =
                     arguments.d_cslRecordType.begin();
                 cit != arguments.d_cslRecordType.end();
                 ++cit) {
                if (*cit == CommandLineArguments::k_CSL_SNAPSHOT_TYPE) {
                    d_processCslRecordTypes.d_snapshot = true;
                }
                else if (*cit == CommandLineArguments::k_CSL_UPDATE_TYPE) {
                    d_processCslRecordTypes.d_update = true;
                }
                else if (*cit == CommandLineArguments::k_CSL_COMMIT_TYPE) {
                    d_processCslRecordTypes.d_commit = true;
                }
                else if (*cit == CommandLineArguments::k_CSL_ACK_TYPE) {
                    d_processCslRecordTypes.d_ack = true;
                }
                else {
                    BSLS_ASSERT(false && "Unknown CSL record type");
                }
            }
        }
    }
    else {
        if (arguments.d_recordType.empty()) {
            d_processRecordTypes.d_message = true;
        }
        else {
            for (bsl::vector<bsl::string>::const_iterator cit =
                     arguments.d_recordType.begin();
                 cit != arguments.d_recordType.end();
                 ++cit) {
                if (*cit == CommandLineArguments::k_MESSAGE_TYPE) {
                    d_processRecordTypes.d_message = true;
                }
                else if (*cit == CommandLineArguments::k_QUEUEOP_TYPE) {
                    d_processRecordTypes.d_queueOp = true;
                }
                else if (*cit == CommandLineArguments::k_JOURNALOP_TYPE) {
                    d_processRecordTypes.d_journalOp = true;
                }
                else {
                    BSLS_ASSERT(false && "Unknown journal record type");
                }
            }
        }
    }

    // Set search range type and values if present
    // TODO: check each value separately
    if (arguments.d_timestampLt || arguments.d_timestampGt) {
        d_range.d_timestampLt = static_cast<bsls::Types::Uint64>(
            arguments.d_timestampLt);
        d_range.d_timestampGt = static_cast<bsls::Types::Uint64>(
            arguments.d_timestampGt);
    }
    else if (arguments.d_offsetLt || arguments.d_offsetGt) {
        d_range.d_offsetLt = static_cast<bsls::Types::Uint64>(
            arguments.d_offsetLt);
        d_range.d_offsetGt = static_cast<bsls::Types::Uint64>(
            arguments.d_offsetGt);
    }
    else if (!arguments.d_seqNumLt.empty() || !arguments.d_seqNumGt.empty()) {
        bmqu::MemOutStream errorDescr(allocator);
        bool               success = false;
        if (!arguments.d_seqNumLt.empty()) {
            d_range.d_seqNumLt = CompositeSequenceNumber().fromString(
                &success,
                errorDescr,
                arguments.d_seqNumLt);
            BSLS_ASSERT(success);
        }
        if (!arguments.d_seqNumGt.empty()) {
            d_range.d_seqNumGt = CompositeSequenceNumber().fromString(
                &success,
                errorDescr,
                arguments.d_seqNumGt);
            BSLS_ASSERT(success);
        }
    }

    // Set specific sequence numbers if present
    if (!arguments.d_seqNum.empty()) {
        CompositeSequenceNumber seqNum;
        bmqu::MemOutStream      errorDescr(allocator);
        bool                    success = false;
        for (bsl::vector<bsl::string>::const_iterator cit =
                 arguments.d_seqNum.begin();
             cit != arguments.d_seqNum.end();
             ++cit) {
            seqNum.fromString(&success, errorDescr, *cit);
            BSLS_ASSERT(success);
            d_seqNum.push_back(seqNum);
        }
    }

    if (arguments.d_minRecordsPerQueue > 0) {
        // Use the provided value if records limit is not default and is valid
        d_minRecordsPerQueue = arguments.d_minRecordsPerQueue;
    }
}

void Parameters::validateQueueNames(bslma::Allocator* allocator) const
{
    // Validate given queue names agains existing in csl file
    bmqu::MemOutStream                       ss(allocator);
    mqbu::StorageKey                         key;
    bsl::vector<bsl::string>::const_iterator it = d_queueName.cbegin();
    for (; it != d_queueName.cend(); ++it) {
        if (!d_queueMap.findKeyByUri(*it).has_value()) {
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
