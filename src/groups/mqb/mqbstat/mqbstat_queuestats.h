// Copyright 2017-2023 Bloomberg Finance L.P.
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

// mqbstat_queuestats.h                                               -*-C++-*-
#ifndef INCLUDED_MQBSTAT_QUEUESTATS
#define INCLUDED_MQBSTAT_QUEUESTATS

//@PURPOSE: Provide mechanism to keep track of Queue statistics.
//
//@CLASSES:
//  mqbstat::QueueStatsDomain: Mechanism for statistics of a queue (domain)
//  mqbstat::QueueStatsClient: Mechanism for statistics of a queue (client)
//  mqbstat::QueueStatsUtil:   Utilities to initialize statistics
//
//@DESCRIPTION: 'mqbstat::QueueStatsDomain' provides a mechanism to keep track
// of individual overall statistics of a queue at the domain level while
// 'mqbstat::QueueStatsClient' provides a mechanism to keep track of individual
// overall statistics of a queue at the client level.
// 'mqbstat::QueueStatsUtil' is a utility namespace exposing methods to
// initialize the stat contexts and associated objects.

// BMQ
#include <bmqt_uri.h>

#include <bmqst_basictableinfoprovider.h>
#include <bmqst_table.h>
#include <bmqst_tablerecords.h>

// BDE
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_cpp11.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqst {
class StatContext;
}
namespace mqbi {
class Domain;
}

namespace mqbstat {

// ======================
// class QueueStatsDomain
// ======================

/// Mechanism to keep track of individual overall statistics of a queue in a
/// domain.
class QueueStatsDomain {
  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(QueueStatsDomain, bslma::UsesBslmaAllocator)

    // TYPES

    /// Enum representing the various type of events for which statistics
    /// are monitored.
    struct EventType {
        // TYPES
        enum Enum {
            e_ADD_MESSAGE,
            e_DEL_MESSAGE,
            e_GC_MESSAGE,
            e_PUT,
            e_PUSH,
            e_ACK,
            e_ACK_TIME,
            e_NACK,
            e_CONFIRM,
            e_CONFIRM_TIME,
            e_REJECT,
            e_QUEUE_TIME,
            e_PURGE,
            e_CHANGE_ROLE,
            e_CFG_MSGS,
            e_CFG_BYTES,
            e_NO_SC_MESSAGE,
            e_UPDATE_HISTORY
        };
    };

    /// Enum representing the various type of stats that can be obtained
    /// from this object.
    struct Stat {
        // TYPES
        enum Enum {
            e_NB_PRODUCER,
            e_NB_CONSUMER,
            e_MESSAGES_CURRENT,
            e_MESSAGES_MAX,
            e_MESSAGES_UTILIZATION_MAX,
            e_BYTES_CURRENT,
            e_BYTES_MAX,
            e_BYTES_UTILIZATION_MAX,
            e_PUT_MESSAGES_DELTA,
            e_PUT_BYTES_DELTA,
            e_PUT_MESSAGES_ABS,
            e_PUT_BYTES_ABS,
            e_PUSH_MESSAGES_DELTA,
            e_PUSH_BYTES_DELTA,
            e_PUSH_MESSAGES_ABS,
            e_PUSH_BYTES_ABS,
            e_ACK_DELTA,
            e_ACK_ABS,
            e_ACK_TIME_AVG,
            e_ACK_TIME_MAX,
            e_NACK_DELTA,
            e_NACK_ABS,
            e_CONFIRM_DELTA,
            e_CONFIRM_ABS,
            e_CONFIRM_TIME_AVG,
            e_CONFIRM_TIME_MAX,
            e_REJECT_ABS,
            e_REJECT_DELTA,
            e_QUEUE_TIME_AVG,
            e_QUEUE_TIME_MAX,
            e_GC_MSGS_DELTA,
            e_GC_MSGS_ABS,
            e_ROLE,
            e_CFG_MSGS,
            e_CFG_BYTES,
            e_NO_SC_MSGS_DELTA,
            e_NO_SC_MSGS_ABS,
            e_HISTORY_ABS
        };

        /// Return the non-modifiable string description corresponding to
        /// the specified enumeration `value`.
        static const char* toString(Stat::Enum value);
    };

    struct Role {
        enum Enum { e_UNKNOWN, e_PRIMARY, e_REPLICA, e_PROXY };

        // CLASS METHODS

        /// Write the string representation of the specified enumeration
        /// `value` to the specified output `stream`, and return a reference
        /// to `stream`.  Optionally specify an initial indentation `level`,
        /// whose absolute value is incremented recursively for nested
        /// objects.  If `level` is specified, optionally specify
        /// `spacesPerLevel`, whose absolute value indicates the number of
        /// spaces per indentation level for this and all of its nested
        /// objects.  If `level` is negative, suppress indentation of the
        /// first line.  If `spacesPerLevel` is negative, format the entire
        /// output on one line, suppressing all but the initial indentation
        /// (as governed by `level`).  See `toAscii` for what constitutes
        /// the string representation of a `Role::Enum` value.
        static bsl::ostream& print(bsl::ostream& stream,
                                   Role::Enum    value,
                                   int           level          = 0,
                                   int           spacesPerLevel = 4);

        /// Return the non-modifiable string representation corresponding to
        /// the specified enumeration `value`, if it exists, and a unique
        /// (error) string otherwise.  The string representation of `value`
        /// matches its corresponding enumerator name with the `e_` prefix
        /// elided.  Note that specifying a `value` that does not match any
        /// of the enumerators will result in a string representation that
        /// is distinct from any of those corresponding to the enumerators,
        /// but is otherwise unspecified.
        static const char* toAscii(Role::Enum value);
    };

  private:
    // PRIVATE TYPE
    typedef bslma::ManagedPtr<bmqst::StatContext> StatSubContextMp;

    // PRIVATE DATA
    /// Allocator to use
    bslma::Allocator* d_allocator_p;

    /// StatContext
    bslma::ManagedPtr<bmqst::StatContext> d_statContext_mp;

    /// List of per-appId subcontexts stored as managed pointers.
    /// Note: `bmqst::StatContext` interface allocates subcontexts as
    ///       managed pointers.  We are not able to store managed pointers
    ///       in a collection that might reallocate and copy its elements,
    ///       since ManagedPtr implementation on Solaris is constraining.
    ///       This is why list is used to store managed pointers.  But we
    ///       also want to perform fast lookups to subcontexts, and for this
    ///       we have `d_subContextsLookup` table that points to raw pointers
    ///       to subcontexts.  These both fields must be kept in sync during
    ///       reconfiguration.
    /// TODO: use one bsl::unordered_map to store and lookup if Solaris support
    ///       is stopped.
    bsl::list<StatSubContextMp> d_subContextsHolder;

    /// Lookup table for per-appId subcontexts.  Managed pointers to these
    /// subcontexts must be held in `d_subContextsHolder`.
    bsl::unordered_map<bsl::string, bmqst::StatContext*> d_subContextsLookup;

  private:
    // NOT IMPLEMENTED
    QueueStatsDomain(const QueueStatsDomain&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    QueueStatsDomain& operator=(const QueueStatsDomain&) BSLS_CPP11_DELETED;

  public:
    // CLASS METHODS

    /// Get the value of the specified `stat` reported to the queue
    /// represented by its associated specified `context` as the difference
    /// between the latest snapshot-ed value (i.e., `snapshotId == 0`) and
    /// the value that was recorded at the specified `snapshotId` snapshots
    /// ago.  The negative `snapshotId == -1` means that the oldest available
    /// snapshot should be used, while other negative values are not supported.
    ///
    /// THREAD: This method can only be invoked from the `snapshot` thread.
    static bsls::Types::Int64 getValue(const bmqst::StatContext& context,
                                       int                       snapshotId,
                                       const Stat::Enum&         stat);

    // CREATORS

    /// Create a new object in an uninitialized state, using the specified
    /// `allocator` for any memory allocations.
    explicit QueueStatsDomain(bslma::Allocator* allocator);

    // MANIPULATORS

    /// Initialize this object for the queue with the specified `uri`, and
    /// register it as a subcontext of the specified `domainStatContext`
    /// (which correspond to the domain-level stat context this queue is
    /// part of).
    void initialize(const bmqt::Uri& uri, mqbi::Domain* domain);

    /// Set the reader count to the specified `readerCount`.  Return the
    /// `QueueStatsDomain` object.
    QueueStatsDomain& setReaderCount(int readerCount);

    /// Set the writer count to the specified `writerCount`.  Return the
    /// `QueueStatsDomain` object.
    QueueStatsDomain& setWriterCount(int writerCount);

    /// Update statistics for the event of the specified `type` and with the
    /// specified `value`.  Depending on the `type`, `value` can represent
    /// the number of bytes, a counter, ...
    template <EventType::Enum type>
    void onEvent(bsls::Types::Int64 value);

    /// Update statistics for the event of the specified `type` and with the
    /// specified `value` for the specified `appId`.  Depending on the `type`,
    /// `value` can represent the number of bytes, a counter, ...
    void onEvent(EventType::Enum    type,
                 bsls::Types::Int64 value,
                 const bsl::string& appId);

    /// Update subcontexts in case of domain reconfigure with the given list of
    /// AppIds.
    void updateDomainAppIds(const bsl::vector<bsl::string>& appIds);

    /// Return a pointer to the statcontext.
    bmqst::StatContext* statContext();
};

// FREE OPERATORS

/// Write the string representation of the specified enumeration `value` to
/// the specified output `stream` in a single-line format, and return a
/// reference to `stream`.  See `toAscii` for what constitutes the string
/// representation of a `Role::Enum` value.  Note that this
/// method has the same behavior as
/// ```
/// mqbstat::QueueStatsDomain::Role::print(stream, value, 0, -1);
/// ```
bsl::ostream& operator<<(bsl::ostream&                stream,
                         QueueStatsDomain::Role::Enum value);

// ======================
// class QueueStatsClient
// ======================

/// Mechanism to keep track of individual overall statistics of a queue in a
/// client.
class QueueStatsClient {
  public:
    // TYPES

    /// Enum representing the various type of events for which statistics
    /// are monitored.
    struct EventType {
        // TYPES
        enum Enum { e_PUT, e_PUSH, e_ACK, e_CONFIRM };
    };

    /// Enum representing the various type of stats that can be obtained
    /// from this object.
    struct Stat {
        // TYPES
        enum Enum {
            e_PUT_MESSAGES_DELTA,
            e_PUT_BYTES_DELTA,
            e_PUT_MESSAGES_ABS,
            e_PUT_BYTES_ABS,
            e_PUSH_MESSAGES_DELTA,
            e_PUSH_BYTES_DELTA,
            e_PUSH_MESSAGES_ABS,
            e_PUSH_BYTES_ABS,
            e_ACK_DELTA,
            e_ACK_ABS,
            e_CONFIRM_DELTA,
            e_CONFIRM_ABS
        };
    };

  private:
    // DATA
    bslma::ManagedPtr<bmqst::StatContext> d_statContext_mp;
    // StatContext

  private:
    // NOT IMPLEMENTED
    QueueStatsClient(const QueueStatsClient&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    QueueStatsClient& operator=(const QueueStatsClient&) BSLS_CPP11_DELETED;

  public:
    // CLASS METHODS

    /// Get the value of the specified `stat` reported to the queue
    /// represented by its associated specified `context` as the difference
    /// between the latest snapshot-ed value (i.e., `snapshotId == 0`) and
    /// the value that was recorded at the specified `snapshotId` snapshots
    /// ago.
    ///
    /// THREAD: This method can only be invoked from the `snapshot` thread.
    static bsls::Types::Int64 getValue(const bmqst::StatContext& context,
                                       int                       snapshotId,
                                       const Stat::Enum&         stat);

    // CREATORS

    /// Create a new object in an uninitialized state.
    QueueStatsClient();

    // MANIPULATORS

    /// Initialize this object for the queue with the specified `uri`, and
    /// register it as a subcontext of the specified `clientStatContext`
    /// (which correspond to the client-level stat context this queue is
    /// part of), using the specified `allocator`.
    void initialize(const bmqt::Uri&    uri,
                    bmqst::StatContext* clientStatContext,
                    bslma::Allocator*   allocator);

    /// Update statistics for the event of the specified `type` and with the
    /// specified `value` (depending on the `type`, `value` can represent
    /// the number of bytes, a counter, ...
    void onEvent(EventType::Enum type, bsls::Types::Int64 value);

    /// Return a pointer to the statcontext.
    bmqst::StatContext* statContext();
};

// =====================
// struct QueueStatsUtil
// =====================

/// Utility namespace of methods to initialize queue stats.
struct QueueStatsUtil {
    // CLASS METHODS

    /// Initialize the statistics for the queues (domain level) keeping the
    /// specified `historySize` of history: return the created top level
    /// stat context to use as parent of all domains statistics.  Use the
    /// specified `allocator` for all stat context and stat values.
    static bsl::shared_ptr<bmqst::StatContext>
    initializeStatContextDomains(int historySize, bslma::Allocator* allocator);

    /// Initialize the statistics for the queues (client level) keeping the
    /// specified `historySize` of history: return the created top level
    /// stat context to use as parent of all domains statistics.  Use the
    /// specified `allocator` for all stat context and stat values.
    static bsl::shared_ptr<bmqst::StatContext>
    initializeStatContextClients(int historySize, bslma::Allocator* allocator);

    /// Load in the specified `table` and `tip` the objects to print the
    /// specified `statContext` for the specified `historySize`.
    static void
    initializeTableAndTipDomains(bmqst::Table*                  table,
                                 bmqst::BasicTableInfoProvider* tip,
                                 int                            historySize,
                                 bmqst::StatContext*            statContext);

    /// Load in the specified `table` and `tip` the objects to print the
    /// specified `statContext` for the specified `historySize`.
    static void
    initializeTableAndTipClients(bmqst::Table*                  table,
                                 bmqst::BasicTableInfoProvider* tip,
                                 int                            historySize,
                                 bmqst::StatContext*            statContext);
};

// -----------------------
// struct DomainQueueStats
// -----------------------

/// Namespace for the constants of stat values that applies to the queues
/// on the domain
struct DomainQueueStats {
    enum Enum {
        /// Value:      Current number of clients who opened the queue with
        ///             the `WRITE` flag
        e_STAT_NB_PRODUCER

        ,
        /// Value:      Current number of clients who opened the queue with
        ///             the 'READ' flag
        e_STAT_NB_CONSUMER

        ,
        /// Value:      Current number of messages in the queue
        e_STAT_MESSAGES

        ,
        /// Value:      Accumulated bytes of all messages currently in the
        ///             queue
        e_STAT_BYTES

        ,
        /// Value:      Number of ack messages delivered by this queue
        e_STAT_ACK

        ,
        /// Value:      The time between PUT and ACK (in nanoseconds).
        e_STAT_ACK_TIME

        ,
        /// Value:      Number of NACK messages generated for this queue
        e_STAT_NACK

        ,
        /// Value:      Number of CONFIRM messages received by this queue
        e_STAT_CONFIRM

        ,
        /// Value:      The time between PUSH and CONFIRM (in nanoseconds).
        e_STAT_CONFIRM_TIME

        ,
        /// Value:      Number of messages rejected by this queue (RDA
        ///             reaching zero)
        e_STAT_REJECT

        ,
        /// Value:      The time spent by the message in the queue (in
        ///             nanoseconds).
        e_STAT_QUEUE_TIME

        ,
        /// Value:      Accumulated bytes of all messages ever pushed from
        ///             the queue
        /// Increment:  Number of messages ever pushed from the queue
        e_STAT_PUSH

        ,
        /// Value:      Accumulated bytes of all messages ever put in the
        ///             queue
        /// Increment:  Number of messages ever put in the queue
        e_STAT_PUT

        ,
        /// Value:      Accumulated number of messages ever GC'ed in the
        ///             queue
        e_STAT_GC_MSGS

        ,
        /// Value:      Role (Unknown, Primary, Replica, Proxy)
        e_STAT_ROLE

        ,
        /// Value:      The configured queue messages capacity
        e_CFG_MSGS

        ,
        /// Value:      The configured queue bytes capacity
        e_CFG_BYTES

        ,
        /// Value:      Accumulated number of messages in the strong
        ///             consistency queue expired before receiving quorum
        ///             Receipts
        e_STAT_NO_SC_MSGS

        ,
        // Value:      Current number of GUIDs stored in queue's history
        //             (does not include messages in the queue)
        e_STAT_HISTORY
    };
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------------
// class QueueStatsDomain
// ----------------------

inline bmqst::StatContext* QueueStatsDomain::statContext()
{
    return d_statContext_mp.get();
}

template <>
inline void QueueStatsDomain::onEvent<QueueStatsDomain::EventType::e_ACK>(
    BSLS_ANNOTATION_UNUSED bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");
    d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_ACK, 1);
}

template <>
inline void QueueStatsDomain::onEvent<QueueStatsDomain::EventType::e_ACK_TIME>(
    bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");
    d_statContext_mp->reportValue(DomainQueueStats::e_STAT_ACK_TIME, value);
}

template <>
inline void QueueStatsDomain::onEvent<QueueStatsDomain::EventType::e_NACK>(
    BSLS_ANNOTATION_UNUSED bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");
    // For NACK, we don't care about the bytes value
    d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_NACK, 1);
}

template <>
inline void QueueStatsDomain::onEvent<QueueStatsDomain::EventType::e_CONFIRM>(
    BSLS_ANNOTATION_UNUSED bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");
    // For CONFIRM, we don't care about the bytes value
    d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_CONFIRM, 1);
}

template <>
inline void
QueueStatsDomain::onEvent<QueueStatsDomain::EventType::e_CONFIRM_TIME>(
    bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");
    d_statContext_mp->reportValue(DomainQueueStats::e_STAT_CONFIRM_TIME,
                                  value);
}

template <>
inline void QueueStatsDomain::onEvent<QueueStatsDomain::EventType::e_REJECT>(
    BSLS_ANNOTATION_UNUSED bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");
    // For REJECT, we don't care about the bytes value
    d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_REJECT, 1);
}

template <>
inline void
QueueStatsDomain::onEvent<QueueStatsDomain::EventType::e_QUEUE_TIME>(
    bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");
    d_statContext_mp->reportValue(DomainQueueStats::e_STAT_QUEUE_TIME, value);
}

template <>
inline void QueueStatsDomain::onEvent<QueueStatsDomain::EventType::e_PUSH>(
    bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");
    d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_PUSH, value);
}

template <>
inline void QueueStatsDomain::onEvent<QueueStatsDomain::EventType::e_PUT>(
    bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");
    d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_PUT, value);
}

template <>
inline void
QueueStatsDomain::onEvent<QueueStatsDomain::EventType::e_ADD_MESSAGE>(
    bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");
    d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_BYTES, value);
    d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_MESSAGES, 1);
    if (!d_subContextsHolder.empty()) {
        bsl::list<StatSubContextMp>::iterator it = d_subContextsHolder.begin();
        while (it != d_subContextsHolder.end()) {
            it->get()->adjustValue(DomainQueueStats::e_STAT_BYTES, value);
            it->get()->adjustValue(DomainQueueStats::e_STAT_MESSAGES, 1);
            ++it;
        }
    }
}

template <>
inline void
QueueStatsDomain::onEvent<QueueStatsDomain::EventType::e_DEL_MESSAGE>(
    bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");
    d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_BYTES, -value);
    d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_MESSAGES, -1);
}

template <>
inline void
QueueStatsDomain::onEvent<QueueStatsDomain::EventType::e_GC_MESSAGE>(
    bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");
    d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_GC_MSGS, value);
}

template <>
inline void QueueStatsDomain::onEvent<QueueStatsDomain::EventType::e_PURGE>(
    BSLS_ANNOTATION_UNUSED bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");
    // NOTE: Setting the value like that will cause weird results if using
    //       the stat to get rates
    d_statContext_mp->setValue(DomainQueueStats::e_STAT_BYTES, 0);
    d_statContext_mp->setValue(DomainQueueStats::e_STAT_MESSAGES, 0);
    if (!d_subContextsHolder.empty()) {
        bsl::list<StatSubContextMp>::iterator it = d_subContextsHolder.begin();
        while (it != d_subContextsHolder.end()) {
            it->get()->setValue(DomainQueueStats::e_STAT_BYTES, 0);
            it->get()->setValue(DomainQueueStats::e_STAT_MESSAGES, 0);
            ++it;
        }
    }
}

template <>
inline void
QueueStatsDomain::onEvent<QueueStatsDomain::EventType::e_CHANGE_ROLE>(
    bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");
    d_statContext_mp->setValue(DomainQueueStats::e_STAT_ROLE, value);
}

template <>
inline void QueueStatsDomain::onEvent<QueueStatsDomain::EventType::e_CFG_MSGS>(
    bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");
    d_statContext_mp->setValue(DomainQueueStats::e_CFG_MSGS, value);
}

template <>
inline void
QueueStatsDomain::onEvent<QueueStatsDomain::EventType::e_CFG_BYTES>(
    bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");
    d_statContext_mp->setValue(DomainQueueStats::e_CFG_BYTES, value);
}

template <>
inline void
QueueStatsDomain::onEvent<QueueStatsDomain::EventType::e_NO_SC_MESSAGE>(
    bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");
    d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_NO_SC_MSGS, value);
}

template <>
inline void
QueueStatsDomain::onEvent<QueueStatsDomain::EventType::e_UPDATE_HISTORY>(
    bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");
    d_statContext_mp->setValue(DomainQueueStats::e_STAT_HISTORY, value);
}

// -----------------------------
// struct QueueStatsDomain::Role
// -----------------------------

// FREE OPERATORS
inline bsl::ostream& operator<<(bsl::ostream&                stream,
                                QueueStatsDomain::Role::Enum value)
{
    return QueueStatsDomain::Role::print(stream, value, 0, -1);
}
// ----------------------
// class QueueStatsClient
// ----------------------

inline bmqst::StatContext* QueueStatsClient::statContext()
{
    return d_statContext_mp.get();
}

}  // close package namespace
}  // close enterprise namespace

#endif
