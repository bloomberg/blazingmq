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

// bmqimp_messagedumper.h                                             -*-C++-*-
#ifndef INCLUDED_BMQIMP_MESSAGEDUMPER
#define INCLUDED_BMQIMP_MESSAGEDUMPER

//@PURPOSE: Provide a mechanism to dump messages.
//
//@CLASSES:
//  bmqimp::MessageDumper: Meschanism to dump messages
//
//@DESCRIPTION: This component defines a mechanism, 'bmqimp::MessageDumper', to
// simplify dumping messages.

// BMQ

#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqp_protocol.h>

// MWC
#include <mwcsys_time.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bsl_iostream.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_atomic.h>
#include <bsls_cpp11.h>
#include <bsls_performancehint.h>

namespace BloombergLP {

namespace bmqimp {

// FORWARD DECLARE
class QueueManager;
class MessageCorrelationIdContainer;

// ===================
// class MessageDumper
// ===================

/// Mechanism to dump messages.
class MessageDumper BSLS_CPP11_FINAL {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQIMP.MESSAGEDUMPER");

  private:
    // PRIVATE TYPES

    /// Structure encapsulating the type and value associated with a dump
    /// action.
    struct DumpContext {
        // DATA
        mutable bsls::AtomicBool d_isEnabled;
        // Flag indicating if the dump action
        // associated with this object is
        // currently enabled; used for
        // optimization of the 'isEnabled'
        // method

        bsls::AtomicInt d_actionType;
        // Dump action type. Value will be one
        // of bmqp_ctrlmsg::DumpActionType

        bsls::AtomicInt64 d_actionValue;
        // Dump action value of the
        // corresponding action
        // type. (e.g. number of next
        // incoming/outgoing messages to dump,
        // time in nanoseconds until which
        // incoming messages will be dumped,
        // etc.)

        // CREATORS

        /// Create a `bmqimp::MessageDumper::DumpContext` object with dump
        /// action set to OFF.
        explicit DumpContext();

        // MANIPULATORS

        /// Reset this object to correspond to an OFF dump action.
        void reset();

        // ACCESSORS

        /// Return true if the dump action associated with this object is
        /// currently enabled, and false otherwise.
        bool isEnabled() const;
    };

  private:
    // DATA
    QueueManager* d_queueManager_p;
    // Queue manager, needed to lookup
    // queues when dumping events

    MessageCorrelationIdContainer* d_messageCorrelationIdContainer_p;
    // Message correlationId container

    DumpContext d_pushContext;
    // Context associated with dumping PUSH
    // events

    DumpContext d_ackContext;
    // Context associated with dumping ACK
    // events

    DumpContext d_putContext;
    // Context associated with dumping PUT
    // events

    DumpContext d_confirmContext;
    // Context associated with dumping
    // CONFIRM events

    bdlbb::BlobBufferFactory* d_bufferFactory_p;

    bslma::Allocator* d_allocator_p;
    // Allocator to use

  private:
    // NOT IMPLEMENTED
    MessageDumper(const MessageDumper&) BSLS_CPP11_DELETED;
    MessageDumper& operator=(const MessageDumper&) BSLS_CPP11_DELETED;

    // PRIVATE MANIPULATORS

    /// Decrement the count of messages to dump in the specified
    /// `dumpContext` if the corresponding action type restricts the number
    /// of messages to dump and return true if the number of messages to
    /// dump has been exhausted, otherwise do nothing and return false.
    bool decrementMessageCount(DumpContext* dumpContext);

    /// Update the dump action context parameters pointed to by the
    /// specified `dumpContext` as per the specified `dumpMsg`.
    void processDumpMessageHelper(DumpContext* dumpContext,
                                  const bmqp_ctrlmsg::DumpMessages& dumpMsg);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MessageDumper, bslma::UsesBslmaAllocator)

  public:
    // CLASS METHODS

    /// Load into the specified `dumpMessagesCommand` the associated command
    /// for dumping messages parsed from the specified `command` according
    /// to the following pattern:
    /// ```
    ///  IN|OUT|PUSH|ACK|PUT|CONFIRM ON|OFF|100|10s
    /// ```
    /// where each token has a specific meaning:
    /// * **IN**      : incoming (`PUSH` and `ACK`) events
    /// * **OUT**     : outgoing (`PUT` and `CONFIRM`) events
    /// * **PUSH**    : incoming `PUSH` events
    /// * **ACK**     : incoming `ACK` events
    /// * **PUT**     : outgoing `PUT` events
    /// * **CONFIRM** : outgoing `CONFIRM` events
    /// * **ON**      : turn on message dumping until explicitly turned off
    /// * **OFF**     : turn off message dumping
    /// * **100**     : turn on message dumping for the next 100 messages
    /// * **10s**     : turn on message dumping for the next 10 seconds
    /// Above, the numerical values `100` and `10` are just for illustration
    /// purposes, and client can choose appropriate positive numeric value
    /// for them.  If a token contains a numerical value, message count was
    /// specified.  If a token contains a numerical value with `s` || `S` at
    /// the end, time in seconds was specified.  Also, the pattern is
    /// case-insensitive.  Return zero if `command` is valid and
    /// `dumpMessagesCommand` has been populated, and non-zero otherwise.
    static int parseCommand(bmqp_ctrlmsg::DumpMessages* dumpMessagesCommand,
                            const bslstl::StringRef&    command);

    // CREATORS

    /// Create a `bmqimp::MessageDumper` object using the specified
    /// `queueManager` and `messageCorrelationIdContainer`.  Use the
    /// specified, `bufferFactory` and `allocator` to supply memory.
    MessageDumper(QueueManager*                  queueManager,
                  MessageCorrelationIdContainer* messageCorrelationIdContainer,
                  bdlbb::BlobBufferFactory*      bufferFactory,
                  bslma::Allocator*              allocator);

    // MANIPULATORS
    int processDumpCommand(const bmqp_ctrlmsg::DumpMessages& command);
    // Process the specified dump 'command'.  Update the state of this
    // object and return 0 if successful, otherwise do nothing and return
    // non-zero error code.

    void reset();
    // Reset the state of this object to its default state, thus
    // nullifying all previously processed dump commands.

    /// Print to the specified `out` the specified `event` of PUSH messages.
    /// The behavior is undefined unless `event` is a valid PUSH event and
    /// PUSH event dump is enabled as indicated by `isEventDumpEnabled`.
    /// Additionally, the behavior is undefined unless each PUSH message in
    /// the `event` has at most one SubQueueId. Note, that `bufferFactory`
    /// is used internally by the push message iterator hidden in function
    /// implementation.
    void dumpPushEvent(bsl::ostream& out, const bmqp::Event& event);

    /// Print to the specified `out` the specified `event` of CONFIRM
    /// messages.  The behavior is undefined unless `event` is a valid
    /// CONFIRM event and CONFIRM event dump is enabled as indicated by
    /// `isEventDumpEnabled`.
    void dumpConfirmEvent(bsl::ostream& out, const bmqp::Event& event);

    /// Print to the specified `out` the specified `event` of PUT messages.
    /// The behavior is undefined unless `event` is a valid PUT event and
    /// PUT event dump is enabled as indicated by `isEventDumpEnabled`.
    void dumpPutEvent(bsl::ostream&             out,
                      const bmqp::Event&        event,
                      bdlbb::BlobBufferFactory* bufferFactory);

    /// Print to the specified `out` the specified `event` of ACK messages.
    /// The behavior is undefined unless `event` is a valid ACK event and
    /// ACK event dump is enabled as indicated by `isEventDumpEnabled`.
    void dumpAckEvent(bsl::ostream& out, const bmqp::Event& event);

    // ACCESSORS

    /// Return true if dumping is enabled for the next specified `type` of
    /// event, and false otherwise.  Note that event dumping could only be
    /// enabled for `PUSH`, `ACK`, `PUT`, and `CONFIRM` events, if
    /// configured appropriately and per the configuration specified in
    /// `processDumpCommand`.
    template <bmqp::EventType::Enum type>
    bool isEventDumpEnabled() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------
// MessageDumper::DumpContext
// --------------------------

// ACCESSORS
inline bool MessageDumper::DumpContext::isEnabled() const
{
    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(!d_isEnabled)) {
        // Short-circuit 'isEnabled' computation because we can ascertain it's
        // going to be false
        return false;  // RETURN
    }

    BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

    switch (d_actionType) {
    case bmqp_ctrlmsg::DumpActionType::E_OFF: {
        d_isEnabled = false;
    } break;  // BREAK
    case bmqp_ctrlmsg::DumpActionType::E_ON: {
        d_isEnabled = true;
    } break;  // BREAK
    case bmqp_ctrlmsg::DumpActionType::E_MESSAGE_COUNT: {
        d_isEnabled = (d_actionValue > 0);  // Still more messages to dump
    } break;                                // BREAK
    case bmqp_ctrlmsg::DumpActionType::E_TIME_IN_SECONDS: {
        d_isEnabled = (d_actionValue >= mwcsys::Time::highResolutionTimer());
        // Expiration time is in the future
    } break;  // BREAK
    }

    return d_isEnabled;
}

// -------------
// MessageDumper
// -------------

// PRIVATE MANIPULATORS
inline bool MessageDumper::decrementMessageCount(DumpContext* dumpContext)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dumpContext);

    if (dumpContext->d_actionType !=
        bmqp_ctrlmsg::DumpActionType::E_MESSAGE_COUNT) {
        // Do nothing and return false
        return false;  // RETURN
    }

    // Decrement number of messages left to dump and return whether the number
    // of messages has been exhausted.
    --(dumpContext->d_actionValue);
    return (dumpContext->d_actionValue <= 0);
}

// ACCESSORS
template <bmqp::EventType::Enum type>
inline bool MessageDumper::isEventDumpEnabled() const
{
    return false;
}

template <>
inline bool MessageDumper::isEventDumpEnabled<bmqp::EventType::e_PUSH>() const
{
    return d_pushContext.isEnabled();
}

template <>
inline bool MessageDumper::isEventDumpEnabled<bmqp::EventType::e_ACK>() const
{
    return d_ackContext.isEnabled();
}

template <>
inline bool MessageDumper::isEventDumpEnabled<bmqp::EventType::e_PUT>() const
{
    return d_putContext.isEnabled();
}

template <>
inline bool
MessageDumper::isEventDumpEnabled<bmqp::EventType::e_CONFIRM>() const
{
    return d_confirmContext.isEnabled();
}

}  // close package namespace
}  // close enterprise namespace

#endif
