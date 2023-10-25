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

// bmqimp_messagedumper.cpp                                           -*-C++-*-
#include <bmqimp_messagedumper.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqimp_messagecorrelationidcontainer.h>
#include <bmqimp_queue.h>
#include <bmqimp_queuemanager.h>
#include <bmqp_ackmessageiterator.h>
#include <bmqp_confirmmessageiterator.h>
#include <bmqp_protocolutil.h>
#include <bmqp_pushmessageiterator.h>
#include <bmqp_putmessageiterator.h>
#include <bmqt_correlationid.h>
#include <bmqt_messageguid.h>

// MWC
#include <mwcu_blob.h>
#include <mwcu_memoutstream.h>

// BDE
#include <bdlb_chartype.h>
#include <bdlb_string.h>
#include <bdlbb_blob.h>
#include <bdlt_timeunitratio.h>
#include <bsl_cstdlib.h>  // for bsl::atoi
#include <bsl_sstream.h>
#include <bsl_string.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqimp {

namespace {

/// Populate the specified `dumpMessageType` with the appropriate value
/// parsed from the specified `messageTypeStr` and return 0 if successful,
/// non-zero otherwise.
int parseMessageType(bmqp_ctrlmsg::DumpMsgType::Value* dumpMessageType,
                     const bsl::string&                messageTypeStr)
{
    // PRECONDTIONS
    BSLS_ASSERT_SAFE(dumpMessageType);

    enum RcEnum {
        // Enum for the various RC error categories
        rc_SUCCESS         = 0,
        rc_INVALID_MSGTYPE = -1
    };

    if (bdlb::String::areEqualCaseless(messageTypeStr, "in")) {
        *dumpMessageType = bmqp_ctrlmsg::DumpMsgType::E_INCOMING;
    }
    else if (bdlb::String::areEqualCaseless(messageTypeStr, "out")) {
        *dumpMessageType = bmqp_ctrlmsg::DumpMsgType::E_OUTGOING;
    }
    else if (bdlb::String::areEqualCaseless(messageTypeStr, "push")) {
        *dumpMessageType = bmqp_ctrlmsg::DumpMsgType::E_PUSH;
    }
    else if (bdlb::String::areEqualCaseless(messageTypeStr, "ack")) {
        *dumpMessageType = bmqp_ctrlmsg::DumpMsgType::E_ACK;
    }
    else if (bdlb::String::areEqualCaseless(messageTypeStr, "put")) {
        *dumpMessageType = bmqp_ctrlmsg::DumpMsgType::E_PUT;
    }
    else if (bdlb::String::areEqualCaseless(messageTypeStr, "confirm")) {
        *dumpMessageType = bmqp_ctrlmsg::DumpMsgType::E_CONFIRM;
    }
    else {
        return rc_INVALID_MSGTYPE;  // RETURN
    }

    return rc_SUCCESS;
}

}  // close unnamed namespace

// --------------------------
// MessageDumper::DumpContext
// --------------------------

// CREATORS
MessageDumper::DumpContext::DumpContext()
: d_isEnabled(false)
, d_actionType(static_cast<int>(bmqp_ctrlmsg::DumpActionType::E_OFF))
, d_actionValue(0)
{
    // NOTHING
}

// MANIPULATORS
void MessageDumper::DumpContext::reset()
{
    d_isEnabled   = false;
    d_actionType  = static_cast<int>(bmqp_ctrlmsg::DumpActionType::E_OFF);
    d_actionValue = 0;
}

// -------------
// MessageDumper
// -------------

// PRIVATE MANIPULATORS
void MessageDumper::processDumpMessageHelper(
    DumpContext*                      dumpContext,
    const bmqp_ctrlmsg::DumpMessages& dumpMsg)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dumpContext && "'dumpContext' must be specified");

    switch (dumpMsg.dumpActionType()) {
    case bmqp_ctrlmsg::DumpActionType::E_ON: {
        dumpContext->d_isEnabled  = true;
        dumpContext->d_actionType = static_cast<int>(dumpMsg.dumpActionType());
        // NOTE: Enabling or disabling (E_ON or E_OFF, respectively) preserves
        //       the action value.
    } break;  // BREAK
    case bmqp_ctrlmsg::DumpActionType::E_OFF: {
        dumpContext->d_isEnabled  = false;
        dumpContext->d_actionType = static_cast<int>(dumpMsg.dumpActionType());
        // NOTE: Enabling or disabling (E_ON or E_OFF, respectively) preserves
        //       the action value.
    } break;  // BREAK
    case bmqp_ctrlmsg::DumpActionType::E_MESSAGE_COUNT: {
        BSLS_ASSERT_SAFE(dumpMsg.dumpActionValue() > 0 &&
                         "DumpMessages 'messageCount' must be positive");
        dumpContext->d_isEnabled  = true;
        dumpContext->d_actionType = static_cast<int>(
            bmqp_ctrlmsg::DumpActionType::E_MESSAGE_COUNT);
        dumpContext->d_actionValue = dumpMsg.dumpActionValue();
    } break;  // BREAK
    case bmqp_ctrlmsg::DumpActionType::E_TIME_IN_SECONDS: {
        dumpContext->d_isEnabled  = true;
        dumpContext->d_actionType = static_cast<int>(
            bmqp_ctrlmsg::DumpActionType::E_TIME_IN_SECONDS);
        dumpContext->d_actionValue = mwcsys::Time::highResolutionTimer() +
                                     (dumpMsg.dumpActionValue() *
                                      bdlt::TimeUnitRatio::k_NS_PER_S);
    } break;  // BREAK
    default: {
        BALL_LOG_ERROR << "Received an invalid dump message: " << dumpMsg;
    }
    }
}

// CLASS METHODS
int MessageDumper::parseCommand(
    bmqp_ctrlmsg::DumpMessages* dumpMessagesCommand,
    const bslstl::StringRef&    command)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dumpMessagesCommand &&
                     "'dumpMessagesCommand' must be provided");

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                  = 0,
        rc_INVALID_MSGTYPE          = -1,
        rc_MISSING_ACTIONTYPE       = -2,
        rc_INVALID_ACTIONTYPE       = -3,
        rc_INVALID_ACTIONTYPE_VALUE = -4
    };

    bsl::istringstream sstr(command);

    // 1. Extract first parameter (MsgType): in|out|push|ack|put|confirm
    bsl::string messageTypeStr = "";
    sstr >> messageTypeStr;

    int rc = parseMessageType(&dumpMessagesCommand->msgTypeToDump(),
                              messageTypeStr);
    if (rc != 0) {
        BALL_LOG_WARN << "Invalid message dumping command: "
                      << "expected 'in','out', 'push', 'ack', 'put', or "
                      << "'confirm', got '" << messageTypeStr << "'";
        return rc_INVALID_MSGTYPE;  // RETURN
    }

    // 2. Extract and validate second parameter (ActionType): on|off|100|10s
    bsl::string dumpActionType = "";
    sstr >> dumpActionType;

    if (sstr.fail()) {
        BALL_LOG_WARN << "Invalid message dumping command: "
                      << "no action type provided !";
        return rc_MISSING_ACTIONTYPE;  // RETURN
    }

    if (bdlb::String::areEqualCaseless(dumpActionType, "on")) {
        dumpMessagesCommand->dumpActionType() =
            bmqp_ctrlmsg::DumpActionType::E_ON;
    }
    else if (bdlb::String::areEqualCaseless(dumpActionType, "off")) {
        dumpMessagesCommand->dumpActionType() =
            bmqp_ctrlmsg::DumpActionType::E_OFF;
    }
    else {
        const int dumpActionValue = bsl::atoi(dumpActionType.c_str());
        if (dumpActionValue <= 0) {
            // No conversion can be performed
            BALL_LOG_WARN << "Invalid message dumping command: "
                          << "expected 'on', 'off', or a positive numeric "
                          << "value, got '" << dumpActionType << "'";
            return rc_INVALID_ACTIONTYPE;  // RETURN
        }

        //: o If last character is a digit, message count was specified.
        //: o If last character is 's' || 'S', time in seconds was specified.
        //: o If last character is something else, its an invalid argument.
        char lastChar = dumpActionType[dumpActionType.length() - 1];
        if (bdlb::CharType::isDigit(lastChar)) {
            dumpMessagesCommand->dumpActionType() =
                bmqp_ctrlmsg::DumpActionType::E_MESSAGE_COUNT;
        }
        else if ('s' == bdlb::CharType::toLower(lastChar)) {
            dumpMessagesCommand->dumpActionType() =
                bmqp_ctrlmsg::DumpActionType::E_TIME_IN_SECONDS;
        }
        else {
            BALL_LOG_WARN << "Invalid message dumping command: "
                          << "invalid action type value '" << dumpActionType
                          << "'";
            return rc_INVALID_ACTIONTYPE_VALUE;  // RETURN
        }

        dumpMessagesCommand->dumpActionValue() = dumpActionValue;
    }

    return rc_SUCCESS;
}

// CREATORS
MessageDumper::MessageDumper(
    QueueManager*                  queueManager,
    MessageCorrelationIdContainer* messageCorrelationIdContainer,
    bdlbb::BlobBufferFactory*      bufferFactory,
    bslma::Allocator*              allocator)
: d_queueManager_p(queueManager)
, d_messageCorrelationIdContainer_p(messageCorrelationIdContainer)
, d_pushContext()
, d_ackContext()
, d_putContext()
, d_confirmContext()
, d_bufferFactory_p(bufferFactory)
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queueManager && "'queueManager' must be provided");
    BSLS_ASSERT_SAFE(messageCorrelationIdContainer &&
                     "'messageCorrelationIdContainer' must be provided");
}

// MANIPULATORS
int MessageDumper::processDumpCommand(
    const bmqp_ctrlmsg::DumpMessages& command)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS              = 0,
        rc_INVALID_DUMP_MESSAGE = -1
    };

    switch (command.msgTypeToDump()) {
    case bmqp_ctrlmsg::DumpMsgType::E_INCOMING: {
        processDumpMessageHelper(&d_pushContext, command);
        processDumpMessageHelper(&d_ackContext, command);
    } break;  // BREAK
    case bmqp_ctrlmsg::DumpMsgType::E_OUTGOING: {
        processDumpMessageHelper(&d_putContext, command);
        processDumpMessageHelper(&d_confirmContext, command);
    } break;  // BREAK
    case bmqp_ctrlmsg::DumpMsgType::E_PUSH: {
        processDumpMessageHelper(&d_pushContext, command);
    } break;  // BREAK
    case bmqp_ctrlmsg::DumpMsgType::E_ACK: {
        processDumpMessageHelper(&d_ackContext, command);
    } break;  // BREAK
    case bmqp_ctrlmsg::DumpMsgType::E_PUT: {
        processDumpMessageHelper(&d_putContext, command);
    } break;  // BREAK
    case bmqp_ctrlmsg::DumpMsgType::E_CONFIRM: {
        processDumpMessageHelper(&d_confirmContext, command);
    } break;  // BREAK
    default: {
        BALL_LOG_ERROR << "Received an invalid dump message: "
                       << "[msgTypeToDump: "
                       << static_cast<int>(command.msgTypeToDump())
                       << ", dumpActionType: " << command.dumpActionType()
                       << ", dumpActionValue: " << command.dumpActionValue()
                       << "]";
        return rc_INVALID_DUMP_MESSAGE;  // RETURN
    }
    }

    return rc_SUCCESS;
}

void MessageDumper::reset()
{
    d_pushContext.reset();
    d_ackContext.reset();
    d_putContext.reset();
    d_confirmContext.reset();
}

// ACCESSORS
void MessageDumper::dumpPushEvent(bsl::ostream& out, const bmqp::Event& event)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(event.isPushEvent() && "'event' must be of type PUSH");

    enum { e_NUM_BYTES_IN_BLOB_TO_DUMP = 256 };

    out << "Dumping a PUSH message event.\n";

    bdlma::LocalSequentialAllocator<1024> lsa(d_allocator_p);
    bmqp::PushMessageIterator             iter(d_bufferFactory_p, &lsa);
    event.loadPushMessageIterator(&iter, true);

    BSLS_ASSERT(iter.isValid());

    int msgNum = 0;
    int rc     = 0;
    while ((rc = iter.next()) == 1) {
        int                 qId = bmqimp::Queue::k_INVALID_QUEUE_ID;
        unsigned int        subscriptionId;
        bmqp::RdaInfo       rdaInfo;
        bmqt::CorrelationId correlationId;
        unsigned int        subscriptionHandle;

        iter.extractQueueInfo(&qId, &subscriptionId, &rdaInfo);

        QueueManager::QueueSp queue =
            d_queueManager_p->lookupQueueBySubscriptionId(&correlationId,
                                                          &subscriptionHandle,
                                                          qId,
                                                          subscriptionId);
        BSLS_ASSERT_SAFE(queue);

        out << "PUSH Message #" << ++msgNum << ": "
            << "[messageGUID: " << iter.header().messageGUID()
            << ", queue: " << queue->uri()
            << ", queueId: " << bmqp::QueueId(qId, queue->subQueueId())
            << ", rdaCounter: " << rdaInfo
            << ", correlationId: " << correlationId;

        if (iter.hasMsgGroupId()) {
            bmqp::Protocol::MsgGroupId msgGroupId(d_allocator_p);
            iter.extractMsgGroupId(&msgGroupId);
            out << ", msgGroupId: \"" << msgGroupId << "\"";
        }

        out << ", payload length: " << iter.messagePayloadSize() << "], first "
            << e_NUM_BYTES_IN_BLOB_TO_DUMP
            << "-bytes hex dump of the payload:\n";

        bdlbb::Blob payload(d_allocator_p);
        if (iter.loadMessagePayload(&payload) == 0) {
            out << mwcu::BlobStartHexDumper(&payload,
                                            e_NUM_BYTES_IN_BLOB_TO_DUMP);
        }
        else {
            out << "** Failed to extract msg **\n";
        }

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                decrementMessageCount(&d_pushContext))) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            // We've exhausted the number of PUSH messages to dump.
            break;  // BREAK
        }
    }

    if (rc < 0) {
        // Invalid PushMessage Event
        out << "Invalid 'PushEvent' [rc: " << rc << "]:\n";
        iter.dumpBlob(out);
        return;  // RETURN
    }
}

void MessageDumper::dumpAckEvent(bsl::ostream& out, const bmqp::Event& event)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(event.isAckEvent() && "'event' must be of type ACK");

    out << "Dumping an ACK message event.\n";

    bmqp::AckMessageIterator iter;
    event.loadAckMessageIterator(&iter);

    BSLS_ASSERT(iter.isValid());

    bmqt::CorrelationId correlationId;
    int                 msgNum = 0;
    while (iter.next() == 1) {
        d_messageCorrelationIdContainer_p->find(&correlationId,
                                                iter.message().messageGUID());
        out << "ACK Message #" << ++msgNum << ": "
            << "[correlationId: " << correlationId << ", status: "
            << bmqp::ProtocolUtil::ackResultFromCode(iter.message().status())
            << ", messageGUID: " << iter.message().messageGUID() << "]"
            << "\n";

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                decrementMessageCount(&d_ackContext))) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            // We've exhausted the number of ACK messages to dump.
            break;  // BREAK
        }
    }
}

void MessageDumper::dumpPutEvent(bsl::ostream&             out,
                                 const bmqp::Event&        event,
                                 bdlbb::BlobBufferFactory* bufferFactory)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(event.isPutEvent() && "'event' must be of type PUT");

    enum { e_NUM_BYTES_IN_BLOB_TO_DUMP = 256 };

    out << "Dumping a PUT message event.\n";

    bdlma::LocalSequentialAllocator<1024> lsa(d_allocator_p);
    bmqp::PutMessageIterator              iter(bufferFactory, &lsa);
    event.loadPutMessageIterator(&iter, true);

    BSLS_ASSERT_SAFE(iter.isValid());

    bmqt::CorrelationId correlationId;
    int                 msgNum = 0;
    while (iter.next() == 1) {
        int rc = d_messageCorrelationIdContainer_p->find(
            &correlationId,
            iter.header().messageGUID());
        BSLS_ASSERT_SAFE((rc == 0) && "correlationId not found");
        (void)rc;  // Compiler happiness

        out << "PUT Message #" << ++msgNum << ": "
            << "[correlationId: " << correlationId << ", queue: "
            << d_queueManager_p
                   ->lookupQueue(bmqp::QueueId(iter.header().queueId()))
                   ->uri();

        if (iter.hasMsgGroupId()) {
            bmqp::Protocol::MsgGroupId msgGroupId(d_allocator_p);
            iter.extractMsgGroupId(&msgGroupId);
            out << ", msgGroupId: \"" << msgGroupId << "\"";
        }

        out << ", payload length: " << iter.messagePayloadSize() << "]"
            << ", first " << e_NUM_BYTES_IN_BLOB_TO_DUMP << "-bytes "
            << "hex dump of the payload:\n";

        bdlbb::Blob payload(d_allocator_p);
        if (iter.loadMessagePayload(&payload) == 0) {
            out << mwcu::BlobStartHexDumper(&payload,
                                            e_NUM_BYTES_IN_BLOB_TO_DUMP);
        }
        else {
            out << "** Failed to extract msg **\n";
        }

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                decrementMessageCount(&d_putContext))) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            // We've exhausted the number of PUT messages to dump.
            break;  // BREAK
        }
    }
}

void MessageDumper::dumpConfirmEvent(bsl::ostream&      out,
                                     const bmqp::Event& event)
{
    // PRECONDTIONS
    BSLS_ASSERT_SAFE(event.isConfirmEvent() &&
                     "'event' must be of type CONFIRM");

    out << "Dumping a CONFIRM message event.\n";

    bmqp::ConfirmMessageIterator iter;
    event.loadConfirmMessageIterator(&iter);

    BSLS_ASSERT_SAFE(iter.isValid());

    int msgNum = 0;
    while (iter.next() == 1) {
        const bmqp::ConfirmMessage& confirmMsg = iter.message();
        const bmqp::QueueId         queueId(confirmMsg.queueId(),
                                    confirmMsg.subQueueId());

        out << "CONFIRM Message #" << ++msgNum << ": "
            << "[messageGUID: " << confirmMsg.messageGUID()
            << ", queue: " << d_queueManager_p->lookupQueue(queueId)->uri()
            << ", queueId: " << queueId << "]"
            << "\n";

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                decrementMessageCount(&d_confirmContext))) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            // We've exhausted the number of CONFIRM messages to dump.
            break;  // BREAK
        }
    }
}

}  // close package namespace
}  // close enterprise namespace
