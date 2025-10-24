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

// bmqimp_queue.cpp                                                   -*-C++-*-
#include <bmqimp_queue.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqimp_stat.h>
#include <bmqp_protocol.h>
#include <bmqt_queueflags.h>
#include <bmqu_memoutstream.h>

#include <bmqst_statcontext.h>
#include <bmqst_statutil.h>
#include <bmqst_table.h>

// BDE
#include <bdlb_print.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_iostream.h>
#include <bsl_ostream.h>
#include <bslim_printer.h>
#include <bslma_allocator.h>

namespace BloombergLP {
namespace bmqimp {

namespace {
/// Name of the stat context to create (holding all queues statistics)
const char k_STAT_NAME[] = "queues";

/// Factor governing the precision for compression ratio stat.  For example,
/// a value of 10000 would lead to 3 digit decimal precision for the
/// reported compression ratio stat.  Note, this is needed as the
/// compression ratio is typically a double, but in the current version,
/// stat context only handles Int64, therefore we scale it up at reporting
/// time and scale it back down at print time.
const double k_COMPRESSION_RATIO_PRECISION_FACTOR = 10000.;

/// Inverse of `k_COMPRESSION_RATIO_PRECISION_FACTOR` for fast multiplication.
const double k_COMPRESSION_RATIO_PRECISION_FACTOR_INV =
    1 / k_COMPRESSION_RATIO_PRECISION_FACTOR;

enum {
    k_STAT_IN = 0  // value = bytes received ; increments =
                   // messages received
    ,
    k_STAT_OUT = 1  // value = bytes sent     ; increments =
                    // messages sent
    ,
    k_STAT_COMPRESSION_RATIO = 2  // value = sum of all compression ratio for
                                  // compressed packed messages
};

double
calculateCompressionRatio(const bmqst::StatValue&                   value,
                          const bmqst::StatValue::SnapshotLocation& start)
{
    // All arithmetic here is done in `double`s, so we'll convert the `Int64`s
    // from the stats layer into `double`s explicitly.

    const double messageCount = static_cast<double>(
        bmqst::StatUtil::increments(value, start));
    if (messageCount == 0.0) {
        return 0.0;  // RETURN
    }

    const double ratioSum = static_cast<double>(
        bmqst::StatUtil::value(value, start));

    return (ratioSum / messageCount) *
           k_COMPRESSION_RATIO_PRECISION_FACTOR_INV;
}

double
calculateCompressionRatio(const bmqst::StatValue&                   value,
                          const bmqst::StatValue::SnapshotLocation& start,
                          const bmqst::StatValue::SnapshotLocation& endPlus)
{
    // All arithmetic here is done in `double`s, so we'll convert the `Int64`s
    // from the stats layer into `doubles` explicitly.

    const double messageCount = static_cast<double>(
        bmqst::StatUtil::incrementsDifference(value, start, endPlus));
    if (messageCount == 0.0) {
        return 0.0;  // RETURN
    }

    const double ratioSum = static_cast<double>(
        bmqst::StatUtil::valueDifference(value, start, endPlus));

    return (ratioSum / messageCount) *
           k_COMPRESSION_RATIO_PRECISION_FACTOR_INV;
}

}  // close unnamed namespace

/// Force variable/symbol definition so that it can be used in other files
const int Queue::k_INVALID_QUEUE_ID;

// -----------------
// struct QueueState
// -----------------

bsl::ostream& QueueState::print(bsl::ostream&    stream,
                                QueueState::Enum value,
                                int              level,
                                int              spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << QueueState::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* QueueState::toAscii(QueueState::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(OPENING_OPN)
        CASE(OPENING_CFG)
        CASE(REOPENING_OPN)
        CASE(REOPENING_CFG)
        CASE(OPENED)
        CASE(CLOSING_CFG)
        CASE(CLOSING_CLS)
        CASE(CLOSED)
        CASE(PENDING)
        CASE(OPENING_OPN_EXPIRED)
        CASE(OPENING_CFG_EXPIRED)
        CASE(CLOSING_CFG_EXPIRED)
        CASE(CLOSING_CLS_EXPIRED)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// ---------------------
// struct QueueStatsUtil
// ---------------------

void QueueStatsUtil::initializeStats(
    Stat*                                     stat,
    bmqst::StatContext*                       rootStatContext,
    const bmqst::StatValue::SnapshotLocation& start,
    const bmqst::StatValue::SnapshotLocation& end,
    bslma::Allocator*                         allocator)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    // Create the queues stat context
    // ------------------------------
    bmqst::StatContextConfiguration config(k_STAT_NAME, &localAllocator);
    config.isTable(true);
    config.value("in").value("out").value("compression_ratio");
    stat->d_statContext_mp = rootStatContext->addSubcontext(config);

    // Create table (with Delta stats)
    // -------------------------------
    bmqst::TableSchema& schema = stat->d_table.schema();
    schema.addDefaultIdColumn("id");

    schema.addColumn("in_bytes", k_STAT_IN, bmqst::StatUtil::value, start);
    schema.addColumn("in_messages",
                     k_STAT_IN,
                     bmqst::StatUtil::increments,
                     start);
    schema.addColumn("in_bytes_delta",
                     k_STAT_IN,
                     bmqst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("in_messages_delta",
                     k_STAT_IN,
                     bmqst::StatUtil::incrementsDifference,
                     start,
                     end);

    schema.addColumn("out_bytes", k_STAT_OUT, bmqst::StatUtil::value, start);
    schema.addColumn("out_messages",
                     k_STAT_OUT,
                     bmqst::StatUtil::increments,
                     start);
    schema.addColumn("out_bytes_delta",
                     k_STAT_OUT,
                     bmqst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("out_messages_delta",
                     k_STAT_OUT,
                     bmqst::StatUtil::incrementsDifference,
                     start,
                     end);
    schema.addColumn("out_compression_ratio",
                     k_STAT_COMPRESSION_RATIO,
                     calculateCompressionRatio,
                     start);
    schema.addColumn("out_compression_ratio_delta",
                     k_STAT_COMPRESSION_RATIO,
                     calculateCompressionRatio,
                     start,
                     end);

    // Configure records
    bmqst::TableRecords& records = stat->d_table.records();
    records.setContext(stat->d_statContext_mp.get());
    records.setFilter(&StatUtil::filterDirect);

    // Create the tip
    stat->d_tip.setTable(&stat->d_table);
    stat->d_tip.setColumnGroup("");
    stat->d_tip.addColumn("id", "").justifyLeft();

    stat->d_tip.setColumnGroup("In");
    stat->d_tip.addColumn("in_bytes_delta", "bytes (delta)")
        .zeroString("")
        .printAsMemory();
    stat->d_tip.addColumn("in_bytes", "bytes").zeroString("").printAsMemory();
    stat->d_tip.addColumn("in_messages_delta", "messages (delta)")
        .zeroString("");
    stat->d_tip.addColumn("in_messages", "messages").zeroString("");

    stat->d_tip.setColumnGroup("Out");
    stat->d_tip.addColumn("out_bytes_delta", "bytes (delta)")
        .zeroString("")
        .printAsMemory();
    stat->d_tip.addColumn("out_bytes", "bytes").zeroString("").printAsMemory();
    stat->d_tip.addColumn("out_messages_delta", "messages (delta)")
        .zeroString("");
    stat->d_tip.addColumn("out_messages", "messages").zeroString("");

    stat->d_tip.setColumnGroup("Compression Ratio");
    stat->d_tip.addColumn("out_compression_ratio_delta", "delta")
        .zeroString("")
        .setPrecision(3);
    stat->d_tip.addColumn("out_compression_ratio", "total")
        .zeroString("")
        .setPrecision(3);

    // Create the table (without Delta stats)
    // --------------------------------------
    // We always use current snapshot for this
    bmqst::StatValue::SnapshotLocation loc(0, 0);

    bmqst::TableSchema& schemaNoDelta = stat->d_tableNoDelta.schema();
    schemaNoDelta.addDefaultIdColumn("id");

    schemaNoDelta.addColumn("in_bytes",
                            k_STAT_IN,
                            bmqst::StatUtil::value,
                            loc);
    schemaNoDelta.addColumn("in_messages",
                            k_STAT_IN,
                            bmqst::StatUtil::increments,
                            loc);

    schemaNoDelta.addColumn("out_bytes",
                            k_STAT_OUT,
                            bmqst::StatUtil::value,
                            loc);
    schemaNoDelta.addColumn("out_messages",
                            k_STAT_OUT,
                            bmqst::StatUtil::increments,
                            loc);
    schemaNoDelta.addColumn("out_compression_ratio",
                            k_STAT_COMPRESSION_RATIO,
                            calculateCompressionRatio,
                            loc);
    // Configure records
    bmqst::TableRecords& recordsNoDelta = stat->d_tableNoDelta.records();
    recordsNoDelta.setContext(stat->d_statContext_mp.get());
    recordsNoDelta.setFilter(&StatUtil::filterDirect);

    // Create the tip
    stat->d_tipNoDelta.setTable(&stat->d_tableNoDelta);
    stat->d_tipNoDelta.setColumnGroup("");
    stat->d_tipNoDelta.addColumn("id", "").justifyLeft();

    stat->d_tipNoDelta.setColumnGroup("In");
    stat->d_tipNoDelta.addColumn("in_bytes", "bytes")
        .zeroString("")
        .printAsMemory();
    stat->d_tipNoDelta.addColumn("in_messages", "messages").zeroString("");

    stat->d_tipNoDelta.setColumnGroup("Out");
    stat->d_tipNoDelta.addColumn("out_bytes", "bytes")
        .zeroString("")
        .printAsMemory();
    stat->d_tipNoDelta.addColumn("out_messages", "messages").zeroString("");
    stat->d_tipNoDelta.addColumn("out_compression_ratio", "compression ratio")
        .zeroString("")
        .setPrecision(3);
}

// -----------
// class Queue
// -----------

Queue::Queue(bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_state(static_cast<int>(QueueState::e_CLOSED))
, d_handleParameters(allocator)
, d_atMostOnce(false)
, d_hasMultipleSubStreams(false)
, d_uri(allocator)
, d_options(allocator)
, d_pendingConfigureId(k_INVALID_CONFIGURE_ID)
, d_requestGroupId()
, d_correlationId()
, d_stats_mp(0)
, d_isSuspended(false)
, d_isOldStyle(true)
, d_isSuspendedWithBroker(false)
, d_schemaGenerator(allocator)
, d_config(allocator)
, d_registeredInternalSubscriptionIds(allocator)
{
    d_handleParameters.uri()   = "";
    d_handleParameters.flags() = bmqt::QueueFlagsUtil::empty();
    d_handleParameters.qId()   = static_cast<unsigned int>(k_INVALID_QUEUE_ID);
    d_handleParameters.readCount()  = 0;
    d_handleParameters.writeCount() = 0;
    d_handleParameters.adminCount() = 0;

    d_options.setMaxUnconfirmedMessages(0)
        .setMaxUnconfirmedBytes(0)
        .setConsumerPriority(bmqp::Protocol::k_CONSUMER_PRIORITY_INVALID);
}

void Queue::registerStatContext(bmqst::StatContext* parentStatContext)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_uri.isValid() &&
        "Cannot call registerStatContext on an invalid queue URI");
    // This method should only be called on a valid queue, with an URI
    BSLS_ASSERT_SAFE(d_stats_mp == 0 && "Stats already initialized");
    BSLS_ASSERT_SAFE(d_state == static_cast<int>(QueueState::e_OPENED) &&
                     "Queue must be opened before registerStatContext()");

    // Create subContext
    bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);

    d_stats_mp = parentStatContext->addSubcontext(
        bmqst::StatContextConfiguration(d_uri.asString(), &localAllocator));
}

void Queue::statUpdateOnMessage(int size, bool isOut)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_stats_mp.get() &&
                     "registerStatContext() has not been called");

    d_stats_mp->adjustValue((isOut ? k_STAT_OUT : k_STAT_IN), size);
}

void Queue::statReportCompressionRatio(double ratio)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_stats_mp.get() &&
                     "registerStatContext() has not been called");
    BSLS_ASSERT_SAFE(ratio > 0);

    const bsls::Types::Int64 value = static_cast<bsls::Types::Int64>(
        ratio * k_COMPRESSION_RATIO_PRECISION_FACTOR);
    d_stats_mp->adjustValue(k_STAT_COMPRESSION_RATIO, value);
}

void Queue::clearStatContext()
{
    d_stats_mp.clear();
}

bsl::ostream&
Queue::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bmqu::MemOutStream queueFlags(d_allocator_p);
    bmqt::QueueFlagsUtil::prettyPrint(queueFlags, d_handleParameters.flags());

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", d_uri);
    printer.printAttribute("flags", queueFlags.str());
    printer.printAttribute("atMostOnce", d_atMostOnce);
    printer.printAttribute("hasMultipleSubStreams", d_hasMultipleSubStreams);
    printer.printAttribute("id", d_handleParameters.qId());
    if (!hasDefaultSubQueueId()) {
        printer.printAttribute("subQueueId", subQueueId());
        printer.printAttribute("appId",
                               d_handleParameters.subIdInfo().value().appId());
    }
    printer.printAttribute("correlationId", d_correlationId);
    printer.printAttribute("state",
                           static_cast<QueueState::Enum>(d_state.load()));
    printer.printAttribute("options", d_options);
    printer.printAttribute("pendingConfigureId", d_pendingConfigureId);
    if (d_requestGroupId.has_value()) {
        printer.printAttribute("requestGroupId", d_requestGroupId.value());
    }
    else {
        printer.printAttribute("requestGroupId", static_cast<void*>(NULL));
    }
    printer.printAttribute("isSuspended", d_isSuspended.load());
    printer.printAttribute("isSuspendedWithBroker", d_isSuspendedWithBroker);
    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
