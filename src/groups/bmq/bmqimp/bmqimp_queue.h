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

// bmqimp_queue.h                                                     -*-C++-*-
#ifndef INCLUDED_BMQIMP_QUEUE
#define INCLUDED_BMQIMP_QUEUE

//@PURPOSE: Provide a type object to represent information about a queue.
//
//@CLASSES:
//  bmqimp::Queue: Representation of a Queue (properties, stats, state, ...)
//  bmqimp::QueueStatsUtil: Utility namespace for queues statistics
//
//@DESCRIPTION: 'bmqimp::Queue' is a structure to hold state and information
// about a queue, such as URI, id, as well as all its associated stats.
// 'bmqimp::QueueStatsUtil' is a utility namespace providing generic
// functionality related to stats associated to Queues.

// BMQ
#include <bmqimp_stat.h>

#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_queueid.h>
#include <bmqp_schemagenerator.h>
#include <bmqp_schemalearner.h>
#include <bmqt_correlationid.h>
#include <bmqt_queueflags.h>
#include <bmqt_queueoptions.h>
#include <bmqt_uri.h>

// MWC
#include <mwcst_statcontext.h>
#include <mwcst_statvalue.h>

// BDE
#include <bsl_iosfwd.h>
#include <bsl_optional.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace bmqimp {

// =================
// struct QueueState
// =================

/// This enum represents the state of a queue
struct QueueState {
    // TYPES
    enum Enum {
        e_OPENING_OPN = 1  // The queue is being opened, 1st phase
        ,
        e_OPENING_CFG = 2  // The queue is being opened, 2nd phase
        ,
        e_REOPENING_OPN = 3  // The queue is being reopened, 1st phase
        ,
        e_REOPENING_CFG = 4  // The queue is being reopened, 2nd phase
        ,
        e_OPENED = 5  // The queue is fully opened
        ,
        e_CLOSING_CFG = 6  // The queue is being closed, 1st phase
        ,
        e_CLOSING_CLS = 7  // The queue is being closed, 2nd phase
        ,
        e_CLOSED = 8  // The queue is fully closed
        ,
        e_PENDING = 9  // The queue is fully closed
        ,
        e_OPENING_OPN_EXPIRED = 10  // The queue open request has timed out
        ,
        e_OPENING_CFG_EXPIRED = 11  // The queue config request has timed out
        ,
        e_CLOSING_CFG_EXPIRED = 12  // The queue deconfig request has timed out
        ,
        e_CLOSING_CLS_EXPIRED = 13  // The queue close request has timed out
    };

    // PUBLIC CONSTANTS

    /// NOTE: This value must always be equal to the lowest type in the
    /// enum because it is being used as a lower bound to verify that a
    /// QueueState field is a supported type.
    static const int k_LOWEST_SUPPORTED_QUEUE_STATE = e_OPENING_OPN;

    /// NOTE: This value must always be equal to the highest *supported*
    /// type in the enum because it is being used to verify a QueueState
    /// field is a supported type.
    static const int k_HIGHEST_SUPPORTED_QUEUE_STATE = e_CLOSING_CLS_EXPIRED;

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// `GenericResult::Enum` value.
    static bsl::ostream& print(bsl::ostream&    stream,
                               QueueState::Enum value,
                               int              level          = 0,
                               int              spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(QueueState::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, QueueState::Enum value);

// =====================
// struct QueueStatsUtil
// =====================

/// Utility namespace providing generic functionality related to stats
/// associated to Queues.
struct QueueStatsUtil {
    // MANIPULATORS

    /// Create the various stat context, table and tips for stats related to
    /// queues in the specified `stat`.  This will create a subcontext of
    /// the specified `rootStatContext`.  The `stat` contains delta and
    /// noDelta version of stats.  Delta ones correspond to stats between
    /// the specified `start` and `end` snapshot location.  Use the
    /// specified `allocator` for any memory allocation.
    static void
    initializeStats(Stat*                                     stat,
                    mwcst::StatContext*                       rootStatContext,
                    const mwcst::StatValue::SnapshotLocation& start,
                    const mwcst::StatValue::SnapshotLocation& end,
                    bslma::Allocator*                         allocator);
};

// ===========
// class Queue
// ===========

/// Representation of a Queue (properties, stats, state, ...)
class Queue {
  public:
    // PUBLIC TYPES
    typedef bsl::pair<unsigned int, bmqt::CorrelationId> SubscriptionHandle;
    // Not using private 'bmqt::SubscriptionHandle' ctor

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use

    bsls::AtomicInt d_state;
    // State of the queue.  Possible values
    // for the integer are one of
    // 'QueueState::Enum'.

    bmqp_ctrlmsg::QueueHandleParameters d_handleParameters;
    // Parameters used at open queue (i.e.,
    // in the openQueue request message)

    bool d_atMostOnce;
    // This queue's routing configuration
    // requires confirm messages

    bool d_hasMultipleSubStreams;
    // Flag indicating if this queue's
    // routing configuration is associated
    // with multiple subStreams
    // (i.e. fanout mode)

    bmqt::Uri d_uri;
    // URI of the queue (same as the one in
    // d_handleParameters, but as a
    // 'bmqt::Uri' type)

    bmqt::QueueOptions d_options;
    // Options last requested by the client
    // application. These may differ from
    // the options last pushed to the
    // broker (e.g. if the queue is
    // currently suspended).

    int d_pendingConfigureId;
    // Id of the currently pending
    // configureQueue request for this
    // queue (if any, or set to
    // 'k_INVALID_CONFIGURE_ID' if none).
    // This is used so that we can prevent
    // concurrent configure, and cancel any
    // outstanding configure before closing
    // the queue.

    bsl::optional<int> d_requestGroupId;
    // Id that may be assigned to the
    // control requests related to this
    // queue object.

    bmqt::CorrelationId d_correlationId;
    // User-specified correlation id of the
    // queue

    bslma::ManagedPtr<mwcst::StatContext> d_stats_mp;
    // Stats context associated to this
    // queue.  Valid only if the queue is
    // open and 'registerStatContext()' has
    // been called

    bsls::AtomicBool d_isSuspended;
    // Whether the queue is suspended.
    // While suspended, a queue receives no
    // messages from the broker, and
    // attempts to pack a message destined
    // for the queue will be rejected.

    bsls::AtomicBool d_isOldStyle;
    // Temporary; shall remove after 2nd
    // roll out of "new style" brokers.

    bool d_isSuspendedWithBroker;
    // Whether the queue is suspended from
    // the perspective of the broker.

    bmqp::SchemaGenerator d_schemaGenerator;

    bmqp::SchemaLearner d_schemaLearner;

    bmqp::SchemaLearner::Context d_schemaLearnerContext;

    bmqp_ctrlmsg::StreamParameters d_config;

    bsl::unordered_map<unsigned int, SubscriptionHandle>
        d_registeredInternalSubscriptionIds;
    // This keeps SubscriptionHandle (id and CorrelationId) for Configure
    // response processing.
    // Supporting multiple concurrent Configure requests.
    // TODO: This should go into ConfigureRequest context.

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented
    Queue(const Queue& other, bslma::Allocator* allocator = 0);
    Queue& operator=(const Queue& other);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Queue, bslma::UsesBslmaAllocator)

    // PUBLIC CONSTANTS
    static const int k_INVALID_QUEUE_ID = -1;

    /// Null id of a pending configure request
    static const int k_INVALID_CONFIGURE_ID = -1;

    // CREATORS

    /// Create a new `Queue` object, using the specified `allocator`.
    explicit Queue(bslma::Allocator* allocator);

    // MANIPULATORS

    /// Set the `SubQueueId` parameter of this Queue to the specified
    /// `value`.  The behavior is undefined unless this Queue's uri contains
    /// an appId, or if a `SubQueueId` has already been set.
    Queue& setSubQueueId(unsigned int value);

    Queue& setState(QueueState::Enum value);
    Queue& setId(int value);
    Queue& setCorrelationId(const bmqt::CorrelationId& value);
    Queue& setUri(const bmqt::Uri& value);
    Queue& setFlags(bsls::Types::Uint64 value);
    Queue& setAtMostOnce(const bool atMostOnce);
    Queue& setHasMultipleSubStreams(const bool value);
    Queue& setOptions(const bmqt::QueueOptions& value);
    Queue& setPendingConfigureId(int value);
    Queue& setRequestGroupId(int value);
    Queue& setIsSuspended(bool value);
    Queue& setIsSuspendedWithBroker(bool value);

    /// Set the corresponding member of this object to the specified `value`
    /// and return a reference offering modifiable access to this object.
    Queue& setConfig(const bmqp_ctrlmsg::StreamParameters& value);

    /// Temporary; shall remove after 2nd roll out of "new style" brokers.
    Queue& setOldStyle(bool value);

    /// Create a new subcontext for this queue, out of the specified
    /// `parentStatContext`.  The behavior is undefined unless this method
    /// is called on valid queue in opened state.  The behavior is also
    /// undefined it this method is called more than once.
    void registerStatContext(mwcst::StatContext* parentStatContext);

    /// Update the stats of this queue by reporting a new message of the
    /// specified `size` was received (if the specified `isOut` is false) or
    /// sent (if `isOut` is true).
    void statUpdateOnMessage(int size, bool isOut);

    /// Update the stats of this queue by reporting a new message has been
    /// compressed with the specified compression `ratio`.
    void statReportCompressionRatio(double ratio);

    /// Clears the stat context associated to this queue (typically used
    /// when this queue is closed, after the session has been stopped to
    /// reinitialize the state before a new start).
    void clearStatContext();

    void
    registerInternalSubscriptionId(unsigned int internalSubscriptionId,
                                   unsigned int subscriptionHandleId,
                                   const bmqt::CorrelationId& correlationId);
    // Keep the specified 'subscriptionHandleId' and 'correlationId'
    // associated with the specified 'internalSubscriptionId' between
    // Configure request and Configure response (until
    // 'extractSubscriptionHandle').

    SubscriptionHandle
    extractSubscriptionHandle(unsigned int internalSubscriptionId);
    // Lookup, copy, erase, and return the copy of what was registered
    // by 'registerInternalSubscriptionId'.

    // ACCESSORS

    /// Return true if this Queue object has a SubQueueId having the default
    /// value, and false otherwise.  Note that a Queue for which the
    /// SubQueueId has not been set will have the default value.
    bool hasDefaultSubQueueId() const;

    /// Return the SubQueueId of this Queue.  Note that the default
    /// SubQueueId is returned if the SubQueueId has not been set.
    unsigned int subQueueId() const;

    QueueState::Enum                           state() const;
    int                                        id() const;
    const bmqt::CorrelationId&                 correlationId() const;
    const bmqt::Uri&                           uri() const;
    bsls::Types::Uint64                        flags() const;
    bool                                       atMostOnce() const;
    bool                                       hasMultipleSubStreams() const;
    const bmqt::QueueOptions&                  options() const;
    int                                        pendingConfigureId() const;
    bsl::optional<int>                         requestGroupId() const;
    const bmqp_ctrlmsg::QueueHandleParameters& handleParameters() const;
    const mwcst::StatContext*                  statContext() const;
    bool                                       isSuspended() const;

    /// Return the corresponding member of this object.
    bool isSuspendedWithBroker() const;

    /// Temporary; shall remove after 2nd roll out of "new style" brokers.
    bool                                  isOldStyle() const;
    const bmqp_ctrlmsg::StreamParameters& config() const;

    bmqp::SchemaGenerator&        schemaGenerator();
    bmqp::SchemaLearner&          schemaLearner();
    bmqp::SchemaLearner::Context& schemaLearnerContext();

    /// Return whether this Queue is valid, i.e., is associated to an
    /// initialized queue.
    bool isValid() const;

    /// Return whether this Queue is valid, i.e., is associated to an opened
    /// queue.
    bool isOpened() const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Return `true` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return false otherwise.
bool operator==(const Queue& lhs, const Queue& rhs);

/// Return `false` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return `true` otherwise.
bool operator!=(const Queue& lhs, const Queue& rhs);

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const Queue& rhs);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------
// class Queue
// -----------

// MANIPULATORS
inline Queue& Queue::setSubQueueId(unsigned int value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_uri.id().empty());

    bmqp_ctrlmsg::SubQueueIdInfo& subQueueIdInfo =
        d_handleParameters.subIdInfo().makeValueInplace();
    subQueueIdInfo.appId() = d_uri.id();
    subQueueIdInfo.subId() = value;

    return *this;
}

inline Queue& Queue::setState(QueueState::Enum value)
{
    d_state = static_cast<int>(value);
    return *this;
}

inline Queue& Queue::setId(int value)
{
    d_handleParameters.qId() = value;
    return *this;
}

inline Queue& Queue::setCorrelationId(const bmqt::CorrelationId& value)
{
    d_correlationId = value;
    return *this;
}

inline Queue& Queue::setUri(const bmqt::Uri& value)
{
    d_uri                    = value;
    d_handleParameters.uri() = d_uri.asString();
    return *this;
}

inline Queue& Queue::setFlags(bsls::Types::Uint64 value)
{
    d_handleParameters.flags() = value;

    // Currently the SDK doesn't support reconfigure flags of a queue.  Even if
    // setFlags is only called once, safer to reset the values upfront
    d_handleParameters.readCount()  = 0;
    d_handleParameters.writeCount() = 0;
    d_handleParameters.adminCount() = 0;

    if (bmqt::QueueFlagsUtil::isReader(value)) {
        d_handleParameters.readCount() = 1;
    }
    if (bmqt::QueueFlagsUtil::isWriter(value)) {
        d_handleParameters.writeCount() = 1;
    }
    if (bmqt::QueueFlagsUtil::isAdmin(value)) {
        d_handleParameters.adminCount() = 1;
    }

    return *this;
}

inline Queue& Queue::setAtMostOnce(const bool atMostOnce)
{
    d_atMostOnce = atMostOnce;
    return *this;
}

inline Queue& Queue::setHasMultipleSubStreams(const bool value)
{
    d_hasMultipleSubStreams = value;
    return *this;
}

inline Queue& Queue::setOptions(const bmqt::QueueOptions& value)
{
    d_options = value;
    return *this;
}

inline Queue& Queue::setPendingConfigureId(int value)
{
    d_pendingConfigureId = value;
    return *this;
}

inline Queue& Queue::setRequestGroupId(int value)
{
    d_requestGroupId = value;
    return *this;
}

inline Queue& Queue::setIsSuspended(bool value)
{
    d_isSuspended = value;
    return *this;
}

inline Queue& Queue::setOldStyle(bool value)
{
    if (!value) {
        d_isOldStyle = value;
    }
    else {
        BSLS_ASSERT_OPT(d_isOldStyle);
        // do not support changing from 'false' to 'true' (from 'new' to 'old')
    }
    return *this;
}

inline Queue& Queue::setIsSuspendedWithBroker(bool value)
{
    d_isSuspendedWithBroker = value;
    return *this;
}

inline Queue& Queue::setConfig(const bmqp_ctrlmsg::StreamParameters& value)
{
    d_config = value;
    return *this;
}

inline void
Queue::registerInternalSubscriptionId(unsigned int internalSubscriptionId,
                                      unsigned int subscriptionHandleId,
                                      const bmqt::CorrelationId& correlationId)
{
    d_registeredInternalSubscriptionIds.emplace(
        internalSubscriptionId,
        SubscriptionHandle(subscriptionHandleId, correlationId));
}

inline Queue::SubscriptionHandle
Queue::extractSubscriptionHandle(unsigned int internalSubscriptionId)
{
    bsl::unordered_map<unsigned int, SubscriptionHandle>::const_iterator cit =
        d_registeredInternalSubscriptionIds.find(internalSubscriptionId);

    if (cit == d_registeredInternalSubscriptionIds.end()) {
        return {internalSubscriptionId, bmqt::CorrelationId()};  // RETURN
    }

    SubscriptionHandle result(cit->second);

    d_registeredInternalSubscriptionIds.erase(cit);

    return result;
}

// ACCESSORS
inline QueueState::Enum Queue::state() const
{
    return static_cast<QueueState::Enum>(d_state.load());
}

inline bool Queue::hasDefaultSubQueueId() const
{
    return subQueueId() == bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID;
}

inline unsigned int Queue::subQueueId() const
{
    if (d_handleParameters.subIdInfo().isNull()) {
        return bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID;  // RETURN
    }

    return d_handleParameters.subIdInfo().value().subId();
}

inline int Queue::id() const
{
    return d_handleParameters.qId();
}

inline const bmqt::CorrelationId& Queue::correlationId() const
{
    return d_correlationId;
}

inline const bmqt::Uri& Queue::uri() const
{
    return d_uri;
}

inline bsls::Types::Uint64 Queue::flags() const
{
    return d_handleParameters.flags();
}

inline bool Queue::atMostOnce() const
{
    return d_atMostOnce;
}

inline bool Queue::hasMultipleSubStreams() const
{
    return d_hasMultipleSubStreams;
}

inline const bmqt::QueueOptions& Queue::options() const
{
    return d_options;
}

inline int Queue::pendingConfigureId() const
{
    return d_pendingConfigureId;
}

inline bsl::optional<int> Queue::requestGroupId() const
{
    return d_requestGroupId;
}

inline const bmqp_ctrlmsg::QueueHandleParameters&
Queue::handleParameters() const
{
    return d_handleParameters;
}

inline const mwcst::StatContext* Queue::statContext() const
{
    return d_stats_mp.get();
}

inline bool Queue::isSuspended() const
{
    return d_isSuspended;
}

inline bool Queue::isOldStyle() const
{
    return d_isOldStyle;
}

inline bool Queue::isSuspendedWithBroker() const
{
    return d_isSuspendedWithBroker;
}

inline const bmqp_ctrlmsg::StreamParameters& Queue::config() const
{
    return d_config;
}

inline bmqp::SchemaLearner& Queue::schemaLearner()
{
    return d_schemaLearner;
}

inline bmqp::SchemaLearner::Context& Queue::schemaLearnerContext()
{
    return d_schemaLearnerContext;
}

inline bmqp::SchemaGenerator& Queue::schemaGenerator()
{
    return d_schemaGenerator;
}

inline bool Queue::isValid() const
{
    const QueueState::Enum queueState = state();
    return ((d_handleParameters.qId() !=
             static_cast<unsigned int>(k_INVALID_QUEUE_ID)) &&
            (queueState == QueueState::e_OPENED ||
             queueState == QueueState::e_PENDING ||
             queueState == QueueState::e_REOPENING_OPN ||
             queueState == QueueState::e_REOPENING_CFG));
}

inline bool Queue::isOpened() const
{
    const QueueState::Enum queueState = state();
    return ((d_handleParameters.qId() !=
             static_cast<unsigned int>(k_INVALID_QUEUE_ID)) &&
            queueState == QueueState::e_OPENED);
}

}  // close package namespace

// -----------------
// struct QueueState
// -----------------

// FREE OPERATORS

inline bsl::ostream& bmqimp::operator<<(bsl::ostream&            stream,
                                        bmqimp::QueueState::Enum value)
{
    return bmqimp::QueueState::print(stream, value, 0, -1);
}

// -----------
// class Queue
// -----------

// FREE OPERATORS
inline bool bmqimp::operator==(const bmqimp::Queue& lhs,
                               const bmqimp::Queue& rhs)
{
    return lhs.state() == rhs.state() &&
           lhs.correlationId() == rhs.correlationId() &&
           lhs.uri() == rhs.uri() && lhs.flags() == rhs.flags();
}

inline bool bmqimp::operator!=(const bmqimp::Queue& lhs,
                               const bmqimp::Queue& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream& bmqimp::operator<<(bsl::ostream&        stream,
                                        const bmqimp::Queue& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
