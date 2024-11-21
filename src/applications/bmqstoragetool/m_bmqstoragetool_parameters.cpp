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

}  // close unnamed namespace

// ==========================
// class CommandLineArguments
// ==========================

const char* CommandLineArguments::k_MESSAGE_TYPE = "message";
const char* CommandLineArguments::k_QUEUEOP_TYPE = "queue-op";
const char* CommandLineArguments::k_JOURNAL_TYPE = "journal-op";

CommandLineArguments::CommandLineArguments(bslma::Allocator* allocator)
: d_recordType(allocator)
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
{
}

bool CommandLineArguments::validate(bsl::string*      error,
                                    bslma::Allocator* allocator)
{
    bmqu::MemOutStream ss(allocator);

    if (d_recordType.size() > 3) {
        ss << "Up to 3 types of record are supported, passed: "
           << d_recordType.size() << "\n";
    }

    if (d_journalPath.empty() && d_journalFile.empty()) {
        ss << "Neither journal path nor journal file are specified\n";
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
    if (!d_seqNumLt.empty() || !d_seqNumGt.empty()) {
        bmqu::MemOutStream      errorDescr(allocator);
        CompositeSequenceNumber seqNumLt, seqNumGt;
        if (!d_seqNumLt.empty()) {
            seqNumLt.fromString(errorDescr, d_seqNumLt);
            if (!seqNumLt.isSet()) {
                ss << "--seqnum-lt: " << errorDescr.str() << "\n";
                errorDescr.reset();
            }
        }
        if (!d_seqNumGt.empty()) {
            seqNumGt.fromString(errorDescr, d_seqNumGt);
            if (!seqNumGt.isSet()) {
                ss << "--seqnum-gt: " << errorDescr.str() << "\n";
            }
        }

        if (seqNumLt.isSet() && seqNumGt.isSet()) {
            if (seqNumLt <= seqNumGt) {
                ss << "Invalid sequence number range specified\n";
            }
        }
    }
    if (d_offsetLt < 0 || d_offsetGt < 0 ||
        (d_offsetLt > 0 && d_offsetGt >= d_offsetLt)) {
        ss << "Invalid offset range specified\n";
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
        ss << "Only one range type can be selected: timestamp, seqnum or "
              "offset\n";
    }

    if (!d_guid.empty() &&
        (!d_queueKey.empty() || !d_queueName.empty() || !d_seqNum.empty() ||
         !d_offset.empty() || d_outstanding || d_confirmed ||
         d_partiallyConfirmed || rangesCnt > 0 || d_summary)) {
        ss << "Giud filter can't be combined with any other filters, as it is "
              "specific enough to find a particular message\n";
    }
    if (!d_seqNum.empty() &&
        (!d_queueKey.empty() || !d_queueName.empty() || !d_guid.empty() ||
         !d_offset.empty() || d_outstanding || d_confirmed ||
         d_partiallyConfirmed || rangesCnt > 0 || d_summary)) {
        ss << "Secnum filter can't be combined with any other filters, as it "
              "is "
              "specific enough to find a particular message\n";
    }
    if (!d_offset.empty() &&
        (!d_queueKey.empty() || !d_queueName.empty() || !d_guid.empty() ||
         !d_seqNum.empty() || d_outstanding || d_confirmed ||
         d_partiallyConfirmed || rangesCnt > 0 || d_summary)) {
        ss << "Offset filter can't be combined with any other filters, as it "
              "is "
              "specific enough to find a particular message\n";
    }
    if (!d_seqNum.empty()) {
        CompositeSequenceNumber seqNum;
        bmqu::MemOutStream      errorDescr(allocator);
        for (bsl::vector<bsl::string>::const_iterator cit = d_seqNum.begin();
             cit != d_seqNum.end();
             ++cit) {
            seqNum.fromString(errorDescr, *cit);
            if (!seqNum.isSet()) {
                ss << "--seqnum: " << errorDescr.str() << "\n";
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
                ss << "--offset: " << *cit << " cannot be negative\n";
            }
        }
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

Parameters::Range::Range()
: d_type(Range::e_NONE)
, d_timestampGt(0)
, d_timestampLt(0)
, d_offsetGt(0)
, d_offsetLt(0)
, d_seqNumGt()
, d_seqNumLt()
{
    // NOTHING
}

Parameters::ProcessRecordTypes::ProcessRecordTypes(bool enableDefault)
: d_message(enableDefault)
, d_queueOp(false)
, d_journalOp(false)
{
    // NOTHING
}

Parameters::Parameters(bslma::Allocator* allocator)
: d_processRecordTypes()
, d_queueMap(allocator)
, d_range()
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
{
    // NOTHING
}

Parameters::Parameters(const CommandLineArguments& arguments,
                       bslma::Allocator*           allocator)
: d_processRecordTypes(false)
, d_queueMap(allocator)
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
{
    // Set record types to process
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
        else {
            d_processRecordTypes.d_journalOp = true;
        }
    }

    // Set search range type and values if present
    if (arguments.d_timestampLt || arguments.d_timestampGt) {
        d_range.d_type        = Range::e_TIMESTAMP;
        d_range.d_timestampLt = static_cast<bsls::Types::Uint64>(
            arguments.d_timestampLt);
        d_range.d_timestampGt = static_cast<bsls::Types::Uint64>(
            arguments.d_timestampGt);
    }
    else if (arguments.d_offsetLt || arguments.d_offsetGt) {
        d_range.d_type     = Range::e_OFFSET;
        d_range.d_offsetLt = static_cast<bsls::Types::Uint64>(
            arguments.d_offsetLt);
        d_range.d_offsetGt = static_cast<bsls::Types::Uint64>(
            arguments.d_offsetGt);
    }
    else if (!arguments.d_seqNumLt.empty() || !arguments.d_seqNumGt.empty()) {
        d_range.d_type = Range::e_SEQUENCE_NUM;
        bmqu::MemOutStream errorDescr(allocator);
        if (!arguments.d_seqNumLt.empty()) {
            d_range.d_seqNumLt.fromString(errorDescr, arguments.d_seqNumLt);
        }
        if (!arguments.d_seqNumGt.empty()) {
            d_range.d_seqNumGt.fromString(errorDescr, arguments.d_seqNumGt);
        }
    }

    // Set specific sequence numbers if present
    if (!arguments.d_seqNum.empty()) {
        CompositeSequenceNumber seqNum;
        bmqu::MemOutStream      errorDescr(allocator);
        for (bsl::vector<bsl::string>::const_iterator cit =
                 arguments.d_seqNum.begin();
             cit != arguments.d_seqNum.end();
             ++cit) {
            seqNum.fromString(errorDescr, *cit);
            d_seqNum.push_back(seqNum);
        }
    }
}

void Parameters::validateQueueNames(bslma::Allocator* allocator) const
{
    // Validate given queue names agains existing in csl file
    bmqu::MemOutStream                       ss(allocator);
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
