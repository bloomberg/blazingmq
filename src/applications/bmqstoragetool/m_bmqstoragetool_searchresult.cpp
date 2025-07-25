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
#include <m_bmqstoragetool_compositesequencenumber.h>
#include <m_bmqstoragetool_parameters.h>
#include <m_bmqstoragetool_recordprinter.h>
#include <m_bmqstoragetool_searchresult.h>

// MQB
#include <mqbs_filestoreprotocolprinter.h>
#include <mqbs_filestoreprotocolutil.h>

// BMQ
#include <bmqu_alignedprinter.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bsl_algorithm.h>
#include <bsl_cmath.h>
#include <bsl_cstddef.h>
#include <bsl_memory.h>
#include <bsl_utility.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

namespace {

// Helper to calculate outstanding ratio
bsl::pair<bsl::size_t, int>
calculateOutstandingRatio(bsl::size_t totalMessagesCount,
                          bsl::size_t deletedMessagesCount)
{
    BSLS_ASSERT_SAFE(totalMessagesCount > 0);
    BSLS_ASSERT_SAFE(totalMessagesCount >= deletedMessagesCount);
    bsl::size_t outstandingMessagesCount = totalMessagesCount -
                                           deletedMessagesCount;
    int ratio = static_cast<int>(bsl::floor(
        float(outstandingMessagesCount) / float(totalMessagesCount) * 100.0f +
        0.5f));
    return bsl::make_pair(outstandingMessagesCount, ratio);
}

}  // close unnamed namespace

// ==================
// class SearchResult
// ==================

SearchResult::~SearchResult()
{
    // NOTHING
}

// ===========================
// class SearchResultDecorator
// ===========================

SearchResultDecorator::SearchResultDecorator(
    const bsl::shared_ptr<SearchResult>& component,
    bslma::Allocator*                    allocator)
: d_searchResult(component)
, d_allocator_p(allocator)
{
}

bool SearchResultDecorator::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    return d_searchResult->processMessageRecord(record,
                                                recordIndex,
                                                recordOffset);
}

bool SearchResultDecorator::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    return d_searchResult->processConfirmRecord(record,
                                                recordIndex,
                                                recordOffset);
}

bool SearchResultDecorator::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    return d_searchResult->processDeletionRecord(record,
                                                 recordIndex,
                                                 recordOffset);
}

bool SearchResultDecorator::processQueueOpRecord(
    const mqbs::QueueOpRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    return d_searchResult->processQueueOpRecord(record,
                                                recordIndex,
                                                recordOffset);
}

bool SearchResultDecorator::processJournalOpRecord(
    const mqbs::JournalOpRecord& record,
    bsls::Types::Uint64          recordIndex,
    bsls::Types::Uint64          recordOffset)
{
    return d_searchResult->processJournalOpRecord(record,
                                                  recordIndex,
                                                  recordOffset);
}

void SearchResultDecorator::outputResult()
{
    d_searchResult->outputResult();
}

void SearchResultDecorator::outputResult(const GuidsList& guidFilter)
{
    d_searchResult->outputResult(guidFilter);
}

const bsl::shared_ptr<Printer>& SearchResultDecorator::printer() const
{
    return d_searchResult->printer();
}

bool SearchResultDecorator::hasCache() const
{
    return d_searchResult->hasCache();
}

// ====================================
// class SearchResultTimestampDecorator
// ====================================

bool SearchResultTimestampDecorator::stop(
    const bsls::Types::Uint64 timestamp) const
{
    return timestamp >= d_timestampLt && !SearchResultDecorator::hasCache();
}

SearchResultTimestampDecorator::SearchResultTimestampDecorator(
    const bsl::shared_ptr<SearchResult>& component,
    const bsls::Types::Uint64            timestampLt,
    bslma::Allocator*                    allocator)
: SearchResultDecorator(component, allocator)
, d_timestampLt(timestampLt)
{
}

bool SearchResultTimestampDecorator::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    return SearchResultDecorator::processMessageRecord(record,
                                                       recordIndex,
                                                       recordOffset) ||
           stop(record.header().timestamp());
}

bool SearchResultTimestampDecorator::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    return SearchResultDecorator::processConfirmRecord(record,
                                                       recordIndex,
                                                       recordOffset) ||
           stop(record.header().timestamp());
}

bool SearchResultTimestampDecorator::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    return SearchResultDecorator::processDeletionRecord(record,
                                                        recordIndex,
                                                        recordOffset) ||
           stop(record.header().timestamp());
}

// =================================
// class SearchResultOffsetDecorator
// =================================

bool SearchResultOffsetDecorator::stop(const bsls::Types::Uint64 offset) const
{
    return offset >= d_offsetLt && !SearchResultDecorator::hasCache();
}

SearchResultOffsetDecorator::SearchResultOffsetDecorator(
    const bsl::shared_ptr<SearchResult>& component,
    const bsls::Types::Uint64            offsetLt,
    bslma::Allocator*                    allocator)
: SearchResultDecorator(component, allocator)
, d_offsetLt(offsetLt)
{
    // NOTHING
}

bool SearchResultOffsetDecorator::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    return SearchResultDecorator::processMessageRecord(record,
                                                       recordIndex,
                                                       recordOffset) ||
           stop(recordOffset);
}

bool SearchResultOffsetDecorator::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    return SearchResultDecorator::processConfirmRecord(record,
                                                       recordIndex,
                                                       recordOffset) ||
           stop(recordOffset);
}

bool SearchResultOffsetDecorator::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    return SearchResultDecorator::processDeletionRecord(record,
                                                        recordIndex,
                                                        recordOffset) ||
           stop(recordOffset);
}

// =========================================
// class SearchResultSequenceNumberDecorator
// =========================================

bool SearchResultSequenceNumberDecorator::stop(
    const CompositeSequenceNumber& sequenceNumber) const
{
    return d_sequenceNumberLt <= sequenceNumber &&
           !SearchResultDecorator::hasCache();
}

SearchResultSequenceNumberDecorator::SearchResultSequenceNumberDecorator(
    const bsl::shared_ptr<SearchResult>& component,
    const CompositeSequenceNumber&       sequenceNumberLt,
    bslma::Allocator*                    allocator)
: SearchResultDecorator(component, allocator)
, d_sequenceNumberLt(sequenceNumberLt)
{
    // NOTHING
}

bool SearchResultSequenceNumberDecorator::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    return SearchResultDecorator::processMessageRecord(record,
                                                       recordIndex,
                                                       recordOffset) ||
           stop(CompositeSequenceNumber(record.header().primaryLeaseId(),
                                        record.header().sequenceNumber()));
}

bool SearchResultSequenceNumberDecorator::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    return SearchResultDecorator::processConfirmRecord(record,
                                                       recordIndex,
                                                       recordOffset) ||
           stop(CompositeSequenceNumber(record.header().primaryLeaseId(),
                                        record.header().sequenceNumber()));
}

bool SearchResultSequenceNumberDecorator::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    return SearchResultDecorator::processDeletionRecord(record,
                                                        recordIndex,
                                                        recordOffset) ||
           stop(CompositeSequenceNumber(record.header().primaryLeaseId(),
                                        record.header().sequenceNumber()));
}

// =======================
// class SearchShortResult
// =======================

SearchShortResult::SearchShortResult(
    const bsl::shared_ptr<Printer>&       printer,
    const Parameters::ProcessRecordTypes& processRecordTypes,
    bslma::ManagedPtr<PayloadDumper>&     payloadDumper,
    bool                                  printImmediately,
    bool                                  eraseDeleted,
    bool                                  printOnDelete,
    bslma::Allocator*                     allocator)
: d_printer(printer)
, d_processRecordTypes(processRecordTypes)
, d_payloadDumper(payloadDumper)
, d_printImmediately(printImmediately)
, d_eraseDeleted(eraseDeleted)
, d_printOnDelete(printOnDelete)
, d_printedMessagesCount(0)
, d_printedQueueOpCount(0)
, d_printedJournalOpCount(0)
, d_guidMap(allocator)
, d_guidList(allocator)
{
    // NOTHING
}

bslma::Allocator* SearchShortResult::allocator() const
{
    return d_guidMap.get_allocator().mechanism();
}

bool SearchShortResult::processMessageRecord(
    const mqbs::MessageRecord& record,
    BSLA_UNUSED bsls::Types::Uint64 recordIndex,
    BSLA_UNUSED bsls::Types::Uint64 recordOffset)
{
    GuidData guidData = bsl::make_pair(record.messageGUID(),
                                       record.messageOffsetDwords());

    if (d_printImmediately) {
        outputGuidData(guidData);
    }
    else {
        d_guidMap[record.messageGUID()] = d_guidList.insert(d_guidList.cend(),
                                                            guidData);
    }

    return false;
}

bool SearchShortResult::processConfirmRecord(
    BSLA_UNUSED const mqbs::ConfirmRecord& record,
    BSLA_UNUSED bsls::Types::Uint64 recordIndex,
    BSLA_UNUSED bsls::Types::Uint64 recordOffset)
{
    return false;
}

bool SearchShortResult::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    BSLA_UNUSED bsls::Types::Uint64 recordIndex,
    BSLA_UNUSED bsls::Types::Uint64 recordOffset)
{
    if (!d_printImmediately && (d_printOnDelete || d_eraseDeleted)) {
        GuidDataMap::iterator it = d_guidMap.find(record.messageGUID());
        if (it != d_guidMap.end()) {
            if (d_printOnDelete) {
                outputGuidData(*it->second);
            }
            if (d_eraseDeleted) {
                d_guidList.erase(it->second);
                d_guidMap.erase(it);
            }
        }
    }
    return false;
}

bool SearchShortResult::processQueueOpRecord(const mqbs::QueueOpRecord& record,
                                             bsls::Types::Uint64 recordIndex,
                                             bsls::Types::Uint64 recordOffset)
{
    RecordDetails<mqbs::QueueOpRecord> details(record,
                                               recordIndex,
                                               recordOffset,
                                               allocator());

    printer()->printQueueOpRecord(details);
    d_printedQueueOpCount++;
    return false;
}

bool SearchShortResult::processJournalOpRecord(
    const mqbs::JournalOpRecord& record,
    bsls::Types::Uint64          recordIndex,
    bsls::Types::Uint64          recordOffset)
{
    RecordDetails<mqbs::JournalOpRecord> details(record,
                                                 recordIndex,
                                                 recordOffset,
                                                 allocator());
    printer()->printJournalOpRecord(details);
    d_printedJournalOpCount++;
    return false;
}

void SearchShortResult::outputResult()
{
    if (!d_printOnDelete) {
        // Print results that were not printed on Delete record processing
        bsl::list<GuidData>::const_iterator it = d_guidList.cbegin();
        for (; it != d_guidList.cend(); ++it) {
            outputGuidData(*it);
        }
    }

    d_printer->printFooter(d_printedMessagesCount,
                           d_printedQueueOpCount,
                           d_printedJournalOpCount,
                           d_processRecordTypes);
}

void SearchShortResult::outputResult(const GuidsList& guidFilter)
{
    // Print only Guids from `guidFilter`
    if (!d_printOnDelete) {
        GuidsList::const_iterator it = guidFilter.cbegin();
        for (; it != guidFilter.cend(); ++it) {
            GuidDataMap::const_iterator gIt = d_guidMap.find(*it);
            if (gIt != d_guidMap.end()) {
                outputGuidData(*gIt->second);
            }
            else {
                // this should not happen
                d_printer->printGuidNotFound(*it);
            }
        }
    }

    d_printer->printFooter(d_printedMessagesCount,
                           d_printedQueueOpCount,
                           d_printedJournalOpCount,
                           d_processRecordTypes);
}

void SearchShortResult::outputGuidData(const GuidData& guidData)
{
    d_printer->printGuid(guidData.first);
    if (d_payloadDumper)
        d_payloadDumper->outputPayload(guidData.second);

    d_printedMessagesCount++;
}

bool SearchShortResult::hasCache() const
{
    return !d_guidList.empty();
}

const bsl::shared_ptr<Printer>& SearchShortResult::printer() const
{
    return d_printer;
}

// ========================
// class SearchDetailResult
// ========================

SearchDetailResult::SearchDetailResult(
    const bsl::shared_ptr<Printer>&       printer,
    const Parameters::ProcessRecordTypes& processRecordTypes,
    const QueueMap&                       queueMap,
    bslma::ManagedPtr<PayloadDumper>&     payloadDumper,
    bool                                  printImmediately,
    bool                                  eraseDeleted,
    bool                                  cleanUnprinted,
    bslma::Allocator*                     allocator)
: d_processRecordTypes(processRecordTypes)
, d_queueMap(queueMap)
, d_printer(printer)
, d_payloadDumper(payloadDumper)
, d_printImmediately(printImmediately)
, d_eraseDeleted(eraseDeleted)
, d_cleanUnprinted(cleanUnprinted)
, d_printedMessagesCount(0)
, d_printedQueueOpCount(0)
, d_printedJournalOpCount(0)
, d_messageDetailsList(allocator)
, d_messageDetailsMap(allocator)
, d_allocator_p(allocator)
{
    // NOTHING
}

bool SearchDetailResult::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    // Store record details for further output
    addMessageDetails(record, recordIndex, recordOffset);

    return false;
}

bool SearchDetailResult::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    // Store record details for further output
    DetailsMap::iterator it = d_messageDetailsMap.find(record.messageGUID());
    if (it != d_messageDetailsMap.end()) {
        it->second->addConfirmRecord(record, recordIndex, recordOffset);
    }

    return false;
}

bool SearchDetailResult::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    DetailsMap::iterator it = d_messageDetailsMap.find(record.messageGUID());
    if (it != d_messageDetailsMap.end()) {
        if (d_printImmediately) {
            // Print message details immediately
            it->second->addDeleteRecord(record, recordIndex, recordOffset);
            outputMessageDetails(*it->second);
        }
        if (d_eraseDeleted) {
            // Delete record if it is not needed anymore
            deleteMessageDetails(it);
        }
    }

    return false;
}

bool SearchDetailResult::processQueueOpRecord(
    const mqbs::QueueOpRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    bsl::optional<bmqp_ctrlmsg::QueueInfo> queueInfo =
        d_queueMap.findInfoByKey(record.queueKey());

    RecordDetails<mqbs::QueueOpRecord> details(record,
                                               recordIndex,
                                               recordOffset,
                                               d_allocator_p);

    if (queueInfo.has_value()) {
        details.d_queueUri = queueInfo->uri();
        if (!findQueueAppIdByAppKey(&details.d_appId,
                                    queueInfo->appIds(),
                                    record.appKey())) {
            details.d_appId = "** NULL **";
        }
    }

    d_printer->printQueueOpRecord(details);

    d_printedQueueOpCount++;

    return false;
}

bool SearchDetailResult::processJournalOpRecord(
    const mqbs::JournalOpRecord& record,
    bsls::Types::Uint64          recordIndex,
    bsls::Types::Uint64          recordOffset)
{
    RecordDetails<mqbs::JournalOpRecord> details(record,
                                                 recordIndex,
                                                 recordOffset,
                                                 d_allocator_p);

    d_printer->printJournalOpRecord(details);

    d_printedJournalOpCount++;

    return false;
}

void SearchDetailResult::outputResult()
{
    if (!d_cleanUnprinted) {
        DetailsList::const_iterator it = d_messageDetailsList.cbegin();
        for (; it != d_messageDetailsList.cend(); ++it) {
            outputMessageDetails(*it);
        }
    }

    d_printer->printFooter(d_printedMessagesCount,
                           d_printedQueueOpCount,
                           d_printedJournalOpCount,
                           d_processRecordTypes);
}

void SearchDetailResult::outputResult(const GuidsList& guidFilter)
{
    // Print only Guids from `guidFilter`
    if (!d_cleanUnprinted) {
        GuidsList::const_iterator it = guidFilter.cbegin();
        for (; it != guidFilter.cend(); ++it) {
            DetailsMap::const_iterator dIt = d_messageDetailsMap.find(*it);
            if (dIt != d_messageDetailsMap.end()) {
                outputMessageDetails(*dIt->second);
            }
            else {
                // this should not happen
                d_printer->printGuidNotFound(*it);
            }
        }
    }

    d_printer->printFooter(d_printedMessagesCount,
                           d_printedQueueOpCount,
                           d_printedJournalOpCount,
                           d_processRecordTypes);
}

void SearchDetailResult::addMessageDetails(const mqbs::MessageRecord& record,
                                           bsls::Types::Uint64 recordIndex,
                                           bsls::Types::Uint64 recordOffset)
{
    bsl::optional<bmqp_ctrlmsg::QueueInfo> queueInfo =
        d_queueMap.findInfoByKey(record.queueKey());

    d_messageDetailsMap.emplace(
        record.messageGUID(),
        d_messageDetailsList.insert(d_messageDetailsList.cend(),
                                    MessageDetails(record,
                                                   recordIndex,
                                                   recordOffset,
                                                   queueInfo,
                                                   d_allocator_p)));
}

void SearchDetailResult::deleteMessageDetails(DetailsMap::iterator iterator)
{
    // Erase record from both containers
    d_messageDetailsList.erase(iterator->second);
    d_messageDetailsMap.erase(iterator);
}

void SearchDetailResult::outputMessageDetails(
    const MessageDetails& messageDetails)
{
    d_printer->printMessage(messageDetails);
    if (d_payloadDumper)
        d_payloadDumper->outputPayload(
            messageDetails.messageRecord().d_record.messageOffsetDwords());

    d_printedMessagesCount++;
}

bool SearchDetailResult::hasCache() const
{
    return !d_messageDetailsMap.empty();
}

const bsl::shared_ptr<Printer>& SearchDetailResult::printer() const
{
    return d_printer;
}

// ============================
// class SearchExactMatchResult
// ============================

SearchExactMatchResult::SearchExactMatchResult(
    const bsl::shared_ptr<Printer>&       printer,
    const Parameters::ProcessRecordTypes& processRecordTypes,
    bool                                  isDetail,
    const QueueMap&                       queueMap,
    bslma::ManagedPtr<PayloadDumper>&     payloadDumper,
    bslma::Allocator*                     allocator)
: d_printer(printer)
, d_processRecordTypes(processRecordTypes)
, d_isDetail(isDetail)
, d_queueMap(queueMap)
, d_payloadDumper(payloadDumper)
, d_printedMessagesCount(0)
, d_printedConfirmCount(0)
, d_printedDeletionCount(0)
, d_printedQueueOpCount(0)
, d_printedJournalOpCount(0)
, d_allocator_p(allocator)
{
    // NOTHING    
}

bool SearchExactMatchResult::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    if (d_isDetail) {
        bsl::optional<bmqp_ctrlmsg::QueueInfo> queueInfo =
            d_queueMap.findInfoByKey(record.queueKey());
        MessageDetails details (record,
                        recordIndex,
                        recordOffset,
                        queueInfo,
                        d_allocator_p);
        d_printer->printMessage(details);
    } else {
        d_printer->printGuid(record.messageGUID());
    }

    if (d_payloadDumper) {
        d_payloadDumper->outputPayload(record.messageOffsetDwords());
    }

    d_printedMessagesCount++;

    return false;
}

bool SearchExactMatchResult::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    if (d_isDetail) {
        RecordDetails<mqbs::ConfirmRecord> details(record,
                                           recordIndex,
                                           recordOffset,
                                           d_allocator_p);

        bsl::optional<bmqp_ctrlmsg::QueueInfo> queueInfo =
            d_queueMap.findInfoByKey(record.queueKey());

        // TODO: move to method to avoid duplicate
        if (queueInfo.has_value()) {
            details.d_queueUri = queueInfo->uri();
            if (!findQueueAppIdByAppKey(&details.d_appId,
                                        queueInfo->appIds(),
                                        record.appKey())) {
                details.d_appId = "** NULL **";
            }
        }

        d_printer->printConfirmRecord(details);
    } else {
        d_printer->printGuid(record.messageGUID());
    }

    d_printedConfirmCount++;
    
    return false;
}

bool SearchExactMatchResult::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    if (d_isDetail) {
        RecordDetails<mqbs::DeletionRecord> details(record,
                                           recordIndex,
                                           recordOffset,
                                           d_allocator_p);

        bsl::optional<bmqp_ctrlmsg::QueueInfo> queueInfo =
            d_queueMap.findInfoByKey(record.queueKey());

        // TODO: move to method to avoid duplicate
        if (queueInfo.has_value()) {
            details.d_queueUri = queueInfo->uri();
        }

        d_printer->printDeletionRecord(details);
    } else {
        d_printer->printGuid(record.messageGUID());
    }

    d_printedDeletionCount++;

    return false;
}

bool SearchExactMatchResult::processQueueOpRecord(
    const mqbs::QueueOpRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{

    RecordDetails<mqbs::QueueOpRecord> details(record,
                                               recordIndex,
                                               recordOffset,
                                               d_allocator_p);

    bsl::optional<bmqp_ctrlmsg::QueueInfo> queueInfo =
        d_queueMap.findInfoByKey(record.queueKey());

    if (queueInfo.has_value()) {
        details.d_queueUri = queueInfo->uri();
        if (!findQueueAppIdByAppKey(&details.d_appId,
                                    queueInfo->appIds(),
                                    record.appKey())) {
            details.d_appId = "** NULL **";
        }
    }

    d_printer->printQueueOpRecord(details);

    d_printedQueueOpCount++;

    return false;
}

bool SearchExactMatchResult::processJournalOpRecord(
    const mqbs::JournalOpRecord& record,
    bsls::Types::Uint64          recordIndex,
    bsls::Types::Uint64          recordOffset)
{
    RecordDetails<mqbs::JournalOpRecord> details(record,
                                                 recordIndex,
                                                 recordOffset,
                                                 d_allocator_p);
    printer()->printJournalOpRecord(details);

    d_printedJournalOpCount++;

    return false;
}

void SearchExactMatchResult::outputResult()
{
    d_printer->printFooter(d_printedMessagesCount,
                            d_printedConfirmCount,
                            d_printedDeletionCount,
                           d_printedQueueOpCount,
                           d_printedJournalOpCount,
                           d_processRecordTypes);
}

void SearchExactMatchResult::outputResult(BSLA_UNUSED const GuidsList& guidFilter)
{
    BSLS_ASSERT_SAFE(false && "NOT SUPPORTED!");
}

const bsl::shared_ptr<Printer>& SearchExactMatchResult::printer() const
{
    return d_printer;
}

// ========================
// class SearchAllDecorator
// ========================

SearchAllDecorator::SearchAllDecorator(
    const bsl::shared_ptr<SearchResult>& component,
    bslma::Allocator*                    allocator)
: SearchResultDecorator(component, allocator)
{
    // NOTHING
}

bool SearchAllDecorator::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    SearchResultDecorator::processMessageRecord(record,
                                                recordIndex,
                                                recordOffset);
    return false;
}

// ================================
// class SearchOutstandingDecorator
// ================================
SearchOutstandingDecorator::SearchOutstandingDecorator(
    const bsl::shared_ptr<SearchResult>& component,
    bslma::Allocator*                    allocator)
: SearchResultDecorator(component, allocator)
, d_foundMessagesCount(0)
, d_deletedMessagesCount(0)
, d_guids(allocator)
{
    // NOTHING
}

bool SearchOutstandingDecorator::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    SearchResultDecorator::processMessageRecord(record,
                                                recordIndex,
                                                recordOffset);
    d_guids.insert(record.messageGUID());
    d_foundMessagesCount++;
    return false;
}

bool SearchOutstandingDecorator::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    SearchResultDecorator::processDeletionRecord(record,
                                                 recordIndex,
                                                 recordOffset);
    bsl::unordered_set<bmqt::MessageGUID>::iterator it = d_guids.find(
        record.messageGUID());
    if (it != d_guids.end()) {
        d_guids.erase(it);
        d_deletedMessagesCount++;
    }

    return false;
}

void SearchOutstandingDecorator::outputResult()
{
    SearchResultDecorator::outputResult();
    if (d_foundMessagesCount > 0) {
        bsl::pair<bsl::size_t, int> outstanding = calculateOutstandingRatio(
            d_foundMessagesCount,
            d_deletedMessagesCount);
        printer()->printOutstandingRatio(outstanding.second,
                                         outstanding.first,
                                         d_foundMessagesCount);
    }
}

// =======================================
// class SearchPartiallyConfirmedDecorator
// =======================================

SearchPartiallyConfirmedDecorator::SearchPartiallyConfirmedDecorator(
    const bsl::shared_ptr<SearchResult>& component,
    bslma::Allocator*                    allocator)
: SearchResultDecorator(component, allocator)
, d_foundMessagesCount(0)
, d_deletedMessagesCount(0)
, d_guidsList(allocator)
, d_notConfirmedGuids(allocator)
, d_partiallyConfirmedGuids(allocator)
{
    // NOTHING
}

bool SearchPartiallyConfirmedDecorator::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    SearchResultDecorator::processMessageRecord(record,
                                                recordIndex,
                                                recordOffset);
    d_notConfirmedGuids.emplace(record.messageGUID(),
                                d_guidsList.insert(d_guidsList.cend(),
                                                   record.messageGUID()));
    d_foundMessagesCount++;
    return false;
}

bool SearchPartiallyConfirmedDecorator::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    SearchResultDecorator::processConfirmRecord(record,
                                                recordIndex,
                                                recordOffset);
    GuidsMap::iterator it = d_notConfirmedGuids.find(record.messageGUID());
    if (it != d_notConfirmedGuids.end()) {
        // Message is partially confirmed, move it to the dedicated map.
        d_partiallyConfirmedGuids.emplace(*it);
        d_notConfirmedGuids.erase(it);
    }

    return false;
}

bool SearchPartiallyConfirmedDecorator::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    SearchResultDecorator::processDeletionRecord(record,
                                                 recordIndex,
                                                 recordOffset);
    GuidsMap::iterator it = d_partiallyConfirmedGuids.find(
        record.messageGUID());
    if (it != d_partiallyConfirmedGuids.end()) {
        // Message is confirmed, remove it.
        d_guidsList.erase(it->second);
        d_partiallyConfirmedGuids.erase(it);
        d_deletedMessagesCount++;
    }

    return false;
}

void SearchPartiallyConfirmedDecorator::outputResult()
{
    GuidsMap::const_iterator it = d_notConfirmedGuids.cbegin();
    for (; it != d_notConfirmedGuids.cend(); ++it) {
        d_guidsList.erase(it->second);
    }
    SearchResultDecorator::outputResult(d_guidsList);

    if (d_foundMessagesCount > 0) {
        bsl::pair<bsl::size_t, int> outstanding = calculateOutstandingRatio(
            d_foundMessagesCount,
            d_deletedMessagesCount);
        printer()->printOutstandingRatio(outstanding.second,
                                         outstanding.first,
                                         d_foundMessagesCount);
    }
}

// =========================
// class SearchGuidDecorator
// =========================
SearchGuidDecorator::SearchGuidDecorator(
    const bsl::shared_ptr<SearchResult>& component,
    const bsl::vector<bsl::string>&      guids,
    bool                                 withDetails,
    bslma::Allocator*                    allocator)
: SearchResultDecorator(component, allocator)
, d_withDetails(withDetails)
, d_guidsMap(allocator)
, d_guids(allocator)
{
    // Build MessageGUID->StrGUID Map
    bsl::vector<bsl::string>::const_iterator it = guids.cbegin();
    for (; it != guids.cend(); ++it) {
        bmqt::MessageGUID guid;
        guid.fromHex(it->c_str());
        d_guidsMap.emplace(guid, d_guids.insert(d_guids.cend(), guid));
    }
}

bool SearchGuidDecorator::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    GuidsMap::iterator it = d_guidsMap.find(record.messageGUID());
    if (it != d_guidsMap.end()) {
        SearchResultDecorator::processMessageRecord(record,
                                                    recordIndex,
                                                    recordOffset);
        // Remove processed GUID from map.
        d_guids.erase(it->second);
        d_guidsMap.erase(it);
    }

    // return true (stop search) if no detail is needed and map is empty.
    return (!d_withDetails && d_guidsMap.empty());
}

bool SearchGuidDecorator::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    SearchResultDecorator::processDeletionRecord(record,
                                                 recordIndex,
                                                 recordOffset);
    // return true (stop search) when details needed and search is done
    // (map is empty).
    return (d_withDetails && d_guidsMap.empty());
}

void SearchGuidDecorator::outputResult()
{
    SearchResultDecorator::outputResult();
    if (!d_guids.empty()) {
        // Print not found guids
        printer()->printGuidsNotFound(d_guids);
    }
}

// ===========================
// class SearchOffsetDecorator
// ===========================
SearchOffsetDecorator::SearchOffsetDecorator(
    const bsl::shared_ptr<SearchResult>&   component,
    const bsl::vector<bsls::Types::Int64>& offsets,
    bool                                   withDetails,
    bslma::Allocator*                      allocator)
: SearchResultDecorator(component, allocator)
, d_offsets(offsets, allocator)
, d_withDetails(withDetails)
{
    // NOTHING
}

bool SearchOffsetDecorator::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    bsl::vector<bsls::Types::Int64>::const_iterator it =
        bsl::find(d_offsets.cbegin(), d_offsets.cend(), recordOffset);
    if (it != d_offsets.cend()) {
        SearchResultDecorator::processMessageRecord(record,
                                                    recordIndex,
                                                    recordOffset);
        // Remove processed offset.
        d_offsets.erase(it);
    }

    // return true (stop search) when search is done
    // (d_offsets is empty).
    return (d_offsets.empty());
}

bool SearchOffsetDecorator::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    bsl::vector<bsls::Types::Int64>::const_iterator it =
        bsl::find(d_offsets.cbegin(), d_offsets.cend(), recordOffset);
    if (it != d_offsets.cend()) {
        SearchResultDecorator::processConfirmRecord(record,
                                                    recordIndex,
                                                    recordOffset);
        // Remove processed offset.
        d_offsets.erase(it);
    }

    // return true (stop search) when search is done
    // (d_offsets is empty).
    return (d_offsets.empty());
}

bool SearchOffsetDecorator::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    bsl::vector<bsls::Types::Int64>::const_iterator it =
        bsl::find(d_offsets.cbegin(), d_offsets.cend(), recordOffset);
    if (it != d_offsets.cend()) {
        SearchResultDecorator::processDeletionRecord(record,
                                                    recordIndex,
                                                    recordOffset);
        // Remove processed offset.
        d_offsets.erase(it);
    }

    // return true (stop search) when search is done
    // (d_offsets is empty).
    return (d_offsets.empty());
}

bool SearchOffsetDecorator::processQueueOpRecord(
    const mqbs::QueueOpRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    bsl::vector<bsls::Types::Int64>::const_iterator it =
        bsl::find(d_offsets.cbegin(), d_offsets.cend(), recordOffset);
    if (it != d_offsets.cend()) {
        SearchResultDecorator::processQueueOpRecord(record,
                                                    recordIndex,
                                                    recordOffset);
        // Remove processed offset.
        d_offsets.erase(it);
    }

    // return true (stop search) if search is done (d_offsets is empty).
    return d_offsets.empty();
}

bool SearchOffsetDecorator::processJournalOpRecord(
    const mqbs::JournalOpRecord& record,
    bsls::Types::Uint64          recordIndex,
    bsls::Types::Uint64          recordOffset)
{
    bsl::vector<bsls::Types::Int64>::const_iterator it =
        bsl::find(d_offsets.cbegin(), d_offsets.cend(), recordOffset);
    if (it != d_offsets.cend()) {
        SearchResultDecorator::processJournalOpRecord(record,
                                                      recordIndex,
                                                      recordOffset);
        // Remove processed offset.
        d_offsets.erase(it);
    }

    // return true (stop search) if search is done (d_offsets is empty).
    return d_offsets.empty();
}

void SearchOffsetDecorator::outputResult()
{
    SearchResultDecorator::outputResult();
    // Print non found offsets
    if (!d_offsets.empty()) {
        printer()->printOffsetsNotFound(d_offsets);
    }
}

// ===================================
// class SearchSequenceNumberDecorator
// ===================================
SearchSequenceNumberDecorator::SearchSequenceNumberDecorator(
    const bsl::shared_ptr<SearchResult>&        component,
    const bsl::vector<CompositeSequenceNumber>& seqNums,
    bool                                        withDetails,
    bslma::Allocator*                           allocator)
: SearchResultDecorator(component, allocator)
, d_seqNums(seqNums, allocator)
, d_withDetails(withDetails)
{
    // NOTHING
}

bool SearchSequenceNumberDecorator::isSequenceNumberFound(
        const CompositeSequenceNumber& sequenceNumber) const
{
    return bsl::find(d_seqNums.cbegin(), d_seqNums.cend(), sequenceNumber) != d_seqNums.cend();
}

bool SearchSequenceNumberDecorator::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    CompositeSequenceNumber seqNum(record.header().primaryLeaseId(),
                                   record.header().sequenceNumber());
    bsl::vector<CompositeSequenceNumber>::const_iterator it =
        bsl::find(d_seqNums.cbegin(), d_seqNums.cend(), seqNum);
    if (it != d_seqNums.cend()) {
        SearchResultDecorator::processMessageRecord(record,
                                                    recordIndex,
                                                    recordOffset);
        // Remove processed sequence number.
        d_seqNums.erase(it);
    }

    // return true (stop search) when search is done
    // (d_seqNums is empty).
    return d_seqNums.empty();
}

bool SearchSequenceNumberDecorator::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    CompositeSequenceNumber seqNum(record.header().primaryLeaseId(),
                                   record.header().sequenceNumber());
    bsl::vector<CompositeSequenceNumber>::const_iterator it =
        bsl::find(d_seqNums.cbegin(), d_seqNums.cend(), seqNum);
    if (it != d_seqNums.cend()) {    
         SearchResultDecorator::processConfirmRecord(record,
                                                    recordIndex,
                                                    recordOffset);
        // Remove processed sequence number.
        d_seqNums.erase(it);
    }

    // return true (stop search) when search is done
    // (d_seqNums is empty).
    return (d_seqNums.empty());
}

bool SearchSequenceNumberDecorator::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    CompositeSequenceNumber seqNum(record.header().primaryLeaseId(),
                                   record.header().sequenceNumber());
    bsl::vector<CompositeSequenceNumber>::const_iterator it =
        bsl::find(d_seqNums.cbegin(), d_seqNums.cend(), seqNum);
    if (it != d_seqNums.cend()) {
        SearchResultDecorator::processDeletionRecord(record,
                                                    recordIndex,
                                                    recordOffset);
        // Remove processed sequence number.
        d_seqNums.erase(it);
    }

    // return true (stop search) when details needed and search is done
    // (d_seqNums is empty).
    return (d_seqNums.empty());
}

bool SearchSequenceNumberDecorator::processQueueOpRecord(
    const mqbs::QueueOpRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    CompositeSequenceNumber seqNum(record.header().primaryLeaseId(),
                                   record.header().sequenceNumber());
    bsl::vector<CompositeSequenceNumber>::const_iterator it =
        bsl::find(d_seqNums.cbegin(), d_seqNums.cend(), seqNum);
    if (it != d_seqNums.cend()) {
        SearchResultDecorator::processQueueOpRecord(record,
                                                    recordIndex,
                                                    recordOffset);
        // Remove processed sequence number.
        d_seqNums.erase(it);
    }

    // return true (stop search) if search is done (d_offsets is empty).
    return d_seqNums.empty();
}

bool SearchSequenceNumberDecorator::processJournalOpRecord(
    const mqbs::JournalOpRecord& record,
    bsls::Types::Uint64          recordIndex,
    bsls::Types::Uint64          recordOffset)
{
    CompositeSequenceNumber seqNum(record.header().primaryLeaseId(),
                                   record.header().sequenceNumber());
    bsl::vector<CompositeSequenceNumber>::const_iterator it =
        bsl::find(d_seqNums.cbegin(), d_seqNums.cend(), seqNum);
    if (it != d_seqNums.cend()) {
        SearchResultDecorator::processJournalOpRecord(record,
                                                      recordIndex,
                                                      recordOffset);
        // Remove processed sequence number.
        d_seqNums.erase(it);
    }

    // return true (stop search) if search is done (d_offsets is empty).
    return d_seqNums.empty();
}

void SearchSequenceNumberDecorator::outputResult()
{
    SearchResultDecorator::outputResult();
    if (!d_seqNums.empty()) {
        // Print not found sequence numbers
        printer()->printCompositesNotFound(d_seqNums);
    }
}

// ======================
// class SummaryProcessor
// ======================

SummaryProcessor::SummaryProcessor(
    const bsl::shared_ptr<Printer>&       printer,
    mqbs::JournalFileIterator*            journalFile_p,
    mqbs::DataFileIterator*               dataFile_p,
    const Parameters::ProcessRecordTypes& processRecordTypes,
    const QueueMap&                       queueMap,
    bsl::optional<bsls::Types::Uint64>    minRecordsPerQueue,
    bslma::Allocator*                     allocator)
: d_printer(printer)
, d_journalFile_p(journalFile_p)
, d_dataFile_p(dataFile_p)
, d_processRecordTypes(processRecordTypes)
, d_foundMessagesCount(0)
, d_deletedMessagesCount(0)
, d_journalOpRecordsCount(0)
, d_queueOpRecordsCount(0)
, d_queueOpCountsVec(mqbs::QueueOpType::e_ADDITION + 1, 0, allocator)
, d_notConfirmedGuids(allocator)
, d_partiallyConfirmedGuids(allocator)
, d_totalRecordsCount(0)
, d_queueDetailsMap(allocator)
, d_queueMap(queueMap)
, d_minRecordsPerQueue(minRecordsPerQueue)
, d_allocator_p(allocator)
{
    // NOTHING
}

bool SummaryProcessor::processMessageRecord(
    const mqbs::MessageRecord& record,
    BSLA_UNUSED bsls::Types::Uint64 recordIndex,
    BSLA_UNUSED bsls::Types::Uint64 recordOffset)
{
    d_totalRecordsCount++;

    if (d_processRecordTypes.d_message) {
        d_notConfirmedGuids.emplace(record.messageGUID());
        d_foundMessagesCount++;
    }

    if (d_minRecordsPerQueue.has_value()) {
        QueueDetails& details = d_queueDetailsMap
                                    .emplace(record.queueKey(),
                                             QueueDetails(d_allocator_p))
                                    .first->second;
        details.d_recordsNumber++;
        details.d_messageRecordsNumber++;
    }

    return false;
}

bool SummaryProcessor::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    BSLA_UNUSED bsls::Types::Uint64 recordIndex,
    BSLA_UNUSED bsls::Types::Uint64 recordOffset)
{
    d_totalRecordsCount++;

    if (d_processRecordTypes.d_message) {
        GuidsSet::iterator it = d_notConfirmedGuids.find(record.messageGUID());
        if (it != d_notConfirmedGuids.end()) {
            // Message is partially confirmed, move it to the dedicated set.
            d_partiallyConfirmedGuids.emplace(*it);
            d_notConfirmedGuids.erase(it);
        }
    }

    if (d_minRecordsPerQueue.has_value()) {
        QueueDetails& details = d_queueDetailsMap
                                    .emplace(record.queueKey(),
                                             QueueDetails(d_allocator_p))
                                    .first->second;
        details.d_recordsNumber++;
        details.d_appDetailsMap[record.appKey()].d_recordsNumber++;
        details.d_confirmRecordsNumber++;
    }

    return false;
}

bool SummaryProcessor::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    BSLA_UNUSED bsls::Types::Uint64 recordIndex,
    BSLA_UNUSED bsls::Types::Uint64 recordOffset)
{
    d_totalRecordsCount++;

    if (d_processRecordTypes.d_message) {
        GuidsSet::iterator it = d_partiallyConfirmedGuids.find(
            record.messageGUID());
        if (it != d_partiallyConfirmedGuids.end()) {
            // Message is confirmed, remove it.
            d_partiallyConfirmedGuids.erase(it);
            d_deletedMessagesCount++;
        }
    }

    if (d_minRecordsPerQueue.has_value()) {
        QueueDetails& details = d_queueDetailsMap
                                    .emplace(record.queueKey(),
                                             QueueDetails(d_allocator_p))
                                    .first->second;
        details.d_recordsNumber++;
        details.d_deleteRecordsNumber++;
    }

    return false;
}

bool SummaryProcessor::processQueueOpRecord(
    const mqbs::QueueOpRecord& record,
    BSLA_UNUSED bsls::Types::Uint64 recordIndex,
    BSLA_UNUSED bsls::Types::Uint64 recordOffset)
{
    d_totalRecordsCount++;

    if (d_processRecordTypes.d_queueOp) {
        d_queueOpRecordsCount++;
        BSLS_ASSERT_SAFE(record.type() < d_queueOpCountsVec.size());
        d_queueOpCountsVec[record.type()]++;
    }

    if (d_minRecordsPerQueue.has_value()) {
        QueueDetails& details = d_queueDetailsMap
                                    .emplace(record.queueKey(),
                                             QueueDetails(d_allocator_p))
                                    .first->second;
        details.d_recordsNumber++;
        details.d_queueOpRecordsNumber++;
    }

    return false;
}

bool SummaryProcessor::processJournalOpRecord(
    BSLA_UNUSED const mqbs::JournalOpRecord& record,
    BSLA_UNUSED bsls::Types::Uint64 recordIndex,
    BSLA_UNUSED bsls::Types::Uint64 recordOffset)
{
    d_totalRecordsCount++;

    if (d_processRecordTypes.d_journalOp) {
        d_journalOpRecordsCount++;
    }

    return false;
}

void SummaryProcessor::finalizeQueueDetails()
{
    // Filter queue records map by records number
    for (QueueDetailsMap::iterator it = d_queueDetailsMap.begin();
         it != d_queueDetailsMap.end();) {
        const mqbu::StorageKey& queueKey = it->first;
        QueueDetails&           details  = it->second;
        // Skip this queue if the number of records for this queue is smaller
        // than threshold
        if (details.d_recordsNumber < d_minRecordsPerQueue.value()) {
            it = d_queueDetailsMap.erase(it);
        }
        else {
            // Check if queueInfo is present for queue key
            bsl::optional<bmqp_ctrlmsg::QueueInfo> queueInfo =
                d_queueMap.findInfoByKey(queueKey);
            if (queueInfo.has_value()) {
                // Set required fields
                details.d_queueUri = queueInfo->uri();
                for (QueueDetails::AppDetailsMap::iterator appIt =
                         details.d_appDetailsMap.begin();
                     appIt != details.d_appDetailsMap.end();
                     ++appIt) {
                    findQueueAppIdByAppKey(&appIt->second.d_appId,
                                           queueInfo->appIds(),
                                           appIt->first);
                }
            }
            ++it;
        }
    }
}

void SummaryProcessor::outputResult()
{
    if (d_processRecordTypes.d_message) {
        printer()->printMessageSummary(d_foundMessagesCount,
                                       d_partiallyConfirmedGuids.size(),
                                       d_deletedMessagesCount,
                                       d_foundMessagesCount -
                                           d_deletedMessagesCount);

        if (d_foundMessagesCount > 0) {
            bsl::pair<bsl::size_t, int> outstanding =
                calculateOutstandingRatio(d_foundMessagesCount,
                                          d_deletedMessagesCount);
            printer()->printOutstandingRatio(outstanding.second,
                                             outstanding.first,
                                             d_foundMessagesCount);
        }
    }

    if (d_processRecordTypes.d_queueOp) {
        printer()->printQueueOpSummary(d_queueOpRecordsCount,
                                       d_queueOpCountsVec);
    }

    if (d_processRecordTypes.d_journalOp) {
        printer()->printJournalOpSummary(d_journalOpRecordsCount);
    }

    finalizeQueueDetails();
    printer()->printRecordSummary(d_totalRecordsCount, d_queueDetailsMap);

    // Print meta data of opened files
    printer()->printJournalFileMeta(d_journalFile_p);
    if (d_dataFile_p && d_dataFile_p->isValid()) {
        printer()->printDataFileMeta(d_dataFile_p);
    }
}

void SummaryProcessor::outputResult(BSLA_UNUSED const GuidsList& guidFilter)
{
    outputResult();
}

const bsl::shared_ptr<Printer>& SummaryProcessor::printer() const
{
    return d_printer;
}

}  // close package namespace
}  // close enterprise namespace
