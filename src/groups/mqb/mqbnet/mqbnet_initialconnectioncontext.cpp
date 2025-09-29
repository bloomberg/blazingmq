// Copyright 2025 Bloomberg Finance L.P.
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

// mqbnet_initialconnectioncontext.cpp                       -*-C++-*-
#include <bslstl_sharedptr.h>
#include <mqbnet_initialconnectioncontext.h>

#include <mqbscm_version.h>

// MQB
#include <mqbnet_authenticationcontext.h>
#include <mqbnet_negotiationcontext.h>

// BMQ
#include <bmqio_channel.h>
#include <bmqio_channelutil.h>
#include <bmqp_event.h>
#include <bmqu_blob.h>
#include <bmqu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_memory.h>

namespace BloombergLP {
namespace mqbnet {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("MQBNET.INITIALCONNECTIONCONTEXT");

const int k_INITIALCONNECTION_READTIMEOUT = 3 * 60;  // 3 minutes

// Proxy that captures a shared_ptr to extend lifetime.
void readCallbackProxy(
    const bsl::shared_ptr<BloombergLP::mqbnet::InitialConnectionContext>& self,
    const BloombergLP::bmqio::Status& status,
    int*                              numNeeded,
    BloombergLP::bdlbb::Blob*         blob)
{
    self->readCallback(status, numNeeded, blob);
}

}

// -----------------------------
// struct InitialConnectionState
// -----------------------------

bsl::ostream& InitialConnectionState::print(bsl::ostream& stream,
                                            InitialConnectionState::Enum value,
                                            int                          level,
                                            int spacesPerLevel)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << InitialConnectionState::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* InitialConnectionState::toAscii(InitialConnectionState::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(INITIAL)
        CASE(AUTHENTICATING)
        CASE(AUTHENTICATED)
        CASE(DEFAULT_AUTHENTICATING)
        CASE(NEGOTIATING_OUTBOUND)
        CASE(NEGOTIATED)
        CASE(FAILED)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool InitialConnectionState::fromAscii(InitialConnectionState::Enum* out,
                                       const bslstl::StringRef&      str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(                                       \
            toAscii(InitialConnectionState::e_##M),                           \
            str.data(),                                                       \
            static_cast<int>(str.length()))) {                                \
        *out = InitialConnectionState::e_##M;                                 \
        return true;                                                          \
    }

    CHECKVALUE(INITIAL)
    CHECKVALUE(AUTHENTICATING)
    CHECKVALUE(AUTHENTICATED)
    CHECKVALUE(DEFAULT_AUTHENTICATING)
    CHECKVALUE(NEGOTIATING_OUTBOUND)
    CHECKVALUE(NEGOTIATED)
    CHECKVALUE(FAILED)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// -----------------------------
// struct InitialConnectionEvent
// -----------------------------

bsl::ostream& InitialConnectionEvent::print(bsl::ostream& stream,
                                            InitialConnectionEvent::Enum value,
                                            int                          level,
                                            int spacesPerLevel)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << InitialConnectionEvent::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* InitialConnectionEvent::toAscii(InitialConnectionEvent::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(NONE)
        CASE(OUTBOUND_NEGOTATION)
        CASE(AUTH_REQUEST)
        CASE(NEGOTIATION_MESSAGE)
        CASE(AUTH_SUCCESS)
        CASE(ERROR)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool InitialConnectionEvent::fromAscii(InitialConnectionEvent::Enum* out,
                                       const bslstl::StringRef&      str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(                                       \
            toAscii(InitialConnectionEvent::e_##M),                           \
            str.data(),                                                       \
            static_cast<int>(str.length()))) {                                \
        *out = InitialConnectionEvent::e_##M;                                 \
        return true;                                                          \
    }

    CHECKVALUE(NONE)
    CHECKVALUE(OUTBOUND_NEGOTATION)
    CHECKVALUE(AUTH_REQUEST)
    CHECKVALUE(NEGOTIATION_MESSAGE)
    CHECKVALUE(AUTH_SUCCESS)
    CHECKVALUE(ERROR)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// ------------------------------
// class InitialConnectionContext
// ------------------------------

InitialConnectionContext::InitialConnectionContext(
    bool                                   isIncoming,
    mqbnet::Authenticator*                 authenticator,
    mqbnet::Negotiator*                    negotiator,
    void*                                  userData,
    void*                                  resultState,
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const InitialConnectionCompleteCb&     initialConnectionCompleteCb,
    bslma::Allocator*                      allocator)
: d_mutex()
, d_authenticator_p(authenticator)
, d_negotiator_p(negotiator)
, d_resultState_p(resultState)
, d_userData_p(userData)
, d_channelSp(channel)
, d_initialConnectionCompleteCb(initialConnectionCompleteCb)
, d_authenticationEncodingType(bmqp::EncodingType::e_BER)
, d_authenticationCtxSp()
, d_negotiationCtxSp()
, d_state(InitialConnectionState::e_INITIAL)
, d_isIncoming(isIncoming)
, d_isClosed(false)
, d_allocator_p(allocator)
{
    // NOTHING
}

InitialConnectionContext::~InitialConnectionContext()
{
    // NOTHING
}

int InitialConnectionContext::readBlob(bsl::ostream&        errorDescription,
                                       bdlbb::Blob*         outPacket,
                                       bool*                isFullBlob,
                                       const bmqio::Status& status,
                                       int*                 numNeeded,
                                       bdlbb::Blob*         blob)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                  = 0,
        rc_READ_ERROR               = -1,
        rc_UNRECOVERABLE_READ_ERROR = -2
    };

    if (!status) {
        errorDescription << "Read error: " << status;
        return (10 * status.category()) + rc_READ_ERROR;  // RETURN
    }

    int rc = bmqio::ChannelUtil::handleRead(outPacket, numNeeded, blob);
    if (rc != 0) {
        // This indicates a non recoverable error...
        errorDescription << "Unrecoverable read error:\n"
                         << bmqu::BlobStartHexDumper(blob);
        return (rc * 10) + rc_UNRECOVERABLE_READ_ERROR;  // RETURN
    }

    if (outPacket->length() == 0) {
        // Don't yet have a full blob
        *isFullBlob = false;
        return rc_SUCCESS;  // RETURN
    }

    // Have a full blob, indicate no more bytes needed (we have to do this
    // because 'handleRead' above set it back to 4 at the end).
    *numNeeded = 0;

    return rc_SUCCESS;
}

int InitialConnectionContext::processBlob(bsl::ostream&      errorDescription,
                                          const bdlbb::Blob& blob)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                           = 0,
        rc_INVALID_INITIALCONNECTION_MESSAGE = -1,
    };

    bsl::optional<bsl::variant<bmqp_ctrlmsg::AuthenticationMessage,
                               bmqp_ctrlmsg::NegotiationMessage> >
        message;

    int rc = decodeInitialConnectionMessage(errorDescription, &message, blob);

    if (rc != rc_SUCCESS) {
        return (rc * 10) + rc_INVALID_INITIALCONNECTION_MESSAGE;  // RETURN
    }

    if (!message.has_value()) {
        errorDescription << "Decode AuthenticationMessage or "
                            "NegotiationMessage succeeds but nothing gets "
                            "loaded in.";
        return (rc * 10) + rc_INVALID_INITIALCONNECTION_MESSAGE;
    }

    if (bsl::holds_alternative<bmqp_ctrlmsg::AuthenticationMessage>(
            message.value())) {
        handleEvent(rc, bsl::string(), Event::e_AUTH_REQUEST, message);
    }
    else {
        handleEvent(rc, bsl::string(), Event::e_NEGOTIATION_MESSAGE, message);
    }

    return rc_SUCCESS;
}

int InitialConnectionContext::decodeInitialConnectionMessage(
    bsl::ostream& errorDescription,
    bsl::optional<bsl::variant<bmqp_ctrlmsg::AuthenticationMessage,
                               bmqp_ctrlmsg::NegotiationMessage> >* message,
    const bdlbb::Blob&                                              blob)
{
    BSLS_ASSERT(message);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                      = 0,
        rc_INVALID_MESSAGE              = -1,
        rc_INVALID_EVENT                = -2,
        rc_INVALID_AUTHENTICATION_EVENT = -3,
        rc_INVALID_CONTROL_EVENT        = -4
    };

    bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);

    bmqp::Event event(&blob, &localAllocator);

    if (!event.isValid()) {
        errorDescription << "Invalid negotiation message received "
                         << "(packet is not a valid BlazingMQ event):\n"
                         << bmqu::BlobStartHexDumper(&blob);
        return rc_INVALID_MESSAGE;  // RETURN
    }

    bmqp_ctrlmsg::AuthenticationMessage authenticationMessage;
    bmqp_ctrlmsg::NegotiationMessage    negotiationMessage;

    if (event.isAuthenticationEvent()) {
        BALL_LOG_DEBUG << "Received AuthenticationEvent: "
                       << bmqu::BlobStartHexDumper(&blob);
        const int rc = event.loadAuthenticationEvent(&authenticationMessage);
        if (rc != 0) {
            errorDescription
                << "Invalid message received [reason: 'authentication "
                   "event is not an AuthenticationMessage', rc: "
                << rc << "]:" << bmqu::BlobStartHexDumper(&blob);
            return rc_INVALID_AUTHENTICATION_EVENT;  // RETURN
        }

        d_authenticationEncodingType = event.authenticationEventEncodingType();
        *message                     = authenticationMessage;
    }
    else if (event.isControlEvent()) {
        BALL_LOG_DEBUG << "Received ControlEvent: "
                       << bmqu::BlobStartHexDumper(&blob);
        const int rc = event.loadControlEvent(&negotiationMessage);
        if (rc != 0) {
            errorDescription << "Invalid message received [reason: 'control "
                                "event is not a NegotiationMessage', rc: "
                             << rc << "]:" << bmqu::BlobStartHexDumper(&blob);
            return rc_INVALID_CONTROL_EVENT;  // RETURN
        }

        *message = negotiationMessage;
    }
    else {
        errorDescription
            << "Invalid initial connection message received "
            << "(packet is not an AuthenticationEvent or ControlEvent):\n"
            << bmqu::BlobStartHexDumper(&blob);
        return rc_INVALID_EVENT;  // RETURN
    }

    return rc_SUCCESS;
}

void InitialConnectionContext::createNegotiationContext()
{
    if (d_negotiationCtxSp) {
        return;  // RETURN
    }

    d_negotiationCtxSp = bsl::allocate_shared<mqbnet::NegotiationContext>(
        d_allocator_p,
        this  // initialConnectionContext
    );
}

int InitialConnectionContext::handleDefaultAuthentication(
    bsl::ostream& errorDescription)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!authenticationContext());

    if (!d_authenticator_p->anonymousCredential()) {
        errorDescription << "Anonymous credential is disallowed, "
                         << "cannot negotiate without authentication.";
        return -1;  // RETURN
    }

    bmqp_ctrlmsg::AuthenticationMessage authenticationMessage;
    bmqp_ctrlmsg::AuthenticateRequest&  authenticateRequest =
        authenticationMessage.makeAuthenticateRequest();

    const mqbcfg::Credential& anonymousCredential =
        d_authenticator_p->anonymousCredential().value();
    authenticateRequest.mechanism() = anonymousCredential.mechanism();
    authenticateRequest.data()      = bsl::vector<char>(
        anonymousCredential.identity().begin(),
        anonymousCredential.identity().end());

    bsl::shared_ptr<InitialConnectionContext> self = shared_from_this();

    const int rc = d_authenticator_p->handleAuthentication(
        errorDescription,
        self,
        authenticationMessage);

    return rc;
}

InitialConnectionContext& InitialConnectionContext::setResultState(void* value)
{
    d_resultState_p = value;
    return *this;
}

InitialConnectionContext& InitialConnectionContext::setAuthenticationContext(
    const bsl::shared_ptr<AuthenticationContext>& value)
{
    d_authenticationCtxSp = value;
    return *this;
}

InitialConnectionContext&
InitialConnectionContext::setState(InitialConnectionState::Enum value)
{
    d_state = value;
    return *this;
}

void InitialConnectionContext::onClose()
{
    d_isClosed = true;
}

void InitialConnectionContext::readCallback(const bmqio::Status& status,
                                            int*                 numNeeded,
                                            bdlbb::Blob*         blob)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS            = 0,
        rc_READ_BLOB_ERROR    = -1,
        rc_PROCESS_BLOB_ERROR = -2,
    };

    BALL_LOG_TRACE << "InitialConnectionContext readCb: [status: " << status
                   << ", peer: '" << channel()->peerUri() << "']";

    bdlbb::Blob        outPacket;
    bool               isFullBlob = true;
    bmqu::MemOutStream errStream;
    int                rc = rc_SUCCESS;

    rc = readBlob(errStream, &outPacket, &isFullBlob, status, numNeeded, blob);
    if (rc != rc_SUCCESS) {
        handleEvent((rc * 10) + rc_READ_BLOB_ERROR,
                    errStream.str(),
                    Event::e_ERROR);
        return;  // RETURN
    }

    if (!isFullBlob) {
        return;  // RETURN
    }

    rc = processBlob(errStream, outPacket);
    if (rc != rc_SUCCESS) {
        handleEvent((rc * 10) + rc_PROCESS_BLOB_ERROR,
                    errStream.str(),
                    Event::e_ERROR);
        return;  // RETURN
    }
}

int InitialConnectionContext::scheduleRead(bsl::ostream& errorDescription)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS    = 0,
        rc_READ_ERROR = -1
    };

    bsl::shared_ptr<InitialConnectionContext> self = shared_from_this();

    // Schedule a TimedRead
    bmqio::Status status;
    channel()->read(&status,
                    bmqp::Protocol::k_PACKET_MIN_SIZE,
                    bdlf::BindUtil::bind(&readCallbackProxy,
                                         self,
                                         bdlf::PlaceHolders::_1,   // status
                                         bdlf::PlaceHolders::_2,   // numNeeded
                                         bdlf::PlaceHolders::_3),  // blob
                    bsls::TimeInterval(k_INITIALCONNECTION_READTIMEOUT));

    if (!status) {
        errorDescription << "Read failed while negotiating: " << status;
        return rc_READ_ERROR;  // RETURN
    }

    return rc_SUCCESS;
}

void InitialConnectionContext::handleEvent(
    int                statusCode,
    const bsl::string& errorDescription,
    Event              input,
    const bsl::optional<bsl::variant<bmqp_ctrlmsg::AuthenticationMessage,
                                     bmqp_ctrlmsg::NegotiationMessage> >&
        message)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0,
        rc_ERROR   = -1
    };

    bsl::shared_ptr<InitialConnectionContext> self = shared_from_this();

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BALL_LOG_INFO << "Enter InitialConnectionContext::handleEvent: "
                  << "state = " << d_state << ", event = " << input
                  << "; peerUri =" << d_channelSp->peerUri()
                  << "; context address = " << this;

    State oldState = d_state;
    State newState = d_state;

    bmqu::MemOutStream errStream;
    int                rc = rc_SUCCESS;

    switch (input) {
    case Event::e_OUTBOUND_NEGOTATION: {
        if (oldState == State::e_INITIAL) {
            newState = State::e_NEGOTIATING_OUTBOUND;
            setState(newState);

            createNegotiationContext();

            rc = d_negotiator_p->negotiateOutbound(errStream, self);
            if (rc == rc_SUCCESS) {
                rc = scheduleRead(errStream);
            }
        }
        else {
            rc = rc_ERROR;
            errStream << "Unexpected event received: " << oldState << " -> "
                      << input;
        }
        break;
    }
    case Event::e_AUTH_REQUEST: {
        const bmqp_ctrlmsg::AuthenticationMessage& authenticationMsg =
            bsl::get<bmqp_ctrlmsg::AuthenticationMessage>(message.value());

        if (oldState == State::e_INITIAL) {
            newState = State::e_AUTHENTICATING;
            setState(newState);

            rc = d_authenticator_p->handleAuthentication(errStream,
                                                         self,
                                                         authenticationMsg);
        }
        else {
            rc = rc_ERROR;
            errStream << "Unexpected event received: " << oldState << " -> "
                      << input;
        }
        break;
    }
    case Event::e_NEGOTIATION_MESSAGE: {
        const bmqp_ctrlmsg::NegotiationMessage& negotiationMsg =
            bsl::get<bmqp_ctrlmsg::NegotiationMessage>(message.value());

        if (oldState == State::e_INITIAL &&
            negotiationMsg.isClientIdentityValue()) {
            newState = State::e_DEFAULT_AUTHENTICATING;
            setState(newState);

            createNegotiationContext();
            negotiationContext()->setNegotiationMessage(negotiationMsg);

            rc = handleDefaultAuthentication(errStream);
        }
        else if (oldState == State::e_AUTHENTICATED &&
                 negotiationMsg.isClientIdentityValue()) {
            newState = State::e_NEGOTIATED;
            setState(newState);

            createNegotiationContext();
            negotiationContext()->setNegotiationMessage(negotiationMsg);
        }
        else if (oldState == State::e_NEGOTIATING_OUTBOUND &&
                 negotiationMsg.isBrokerResponseValue()) {
            // Received a BrokerResponse
            newState = State::e_NEGOTIATED;
            setState(newState);

            BSLS_ASSERT_SAFE(negotiationContext());
            negotiationContext()->setNegotiationMessage(negotiationMsg);
        }
        else {
            rc = rc_ERROR;
            errStream << "Unexpected event received: " << oldState << " -> "
                      << input << " [ negotiationMsg: " << negotiationMsg
                      << " ]";
        }
        break;
    }
    case Event::e_AUTH_SUCCESS: {
        if (oldState == State::e_AUTHENTICATING) {
            newState = State::e_AUTHENTICATED;
            setState(newState);

            // Now read Negotiation message
            rc = scheduleRead(errStream);
        }
        else if (oldState == State::e_DEFAULT_AUTHENTICATING) {
            newState = State::e_NEGOTIATED;
            setState(newState);

            BSLS_ASSERT_SAFE(negotiationContext());
            BSLS_ASSERT_SAFE(negotiationContext()
                                 ->negotiationMessage()
                                 .isClientIdentityValue());
        }
        else {
            rc = rc_ERROR;
            errStream << "Unexpected event received: " << oldState << " -> "
                      << input;
        }
        break;
    }
    case Event::e_ERROR: {
        rc = statusCode;
        errStream << errorDescription;
    } break;
    case Event::e_NONE: {
        // NOT IMPLEMENTED
        BSLS_ASSERT_SAFE(!"Unexpected event received: e_NONE");
        break;
    }
    default:
        rc = rc_ERROR;
        errStream << "InitialConnectionContext: "
                  << "unexpected event received: " << input;
    }

    BALL_LOG_INFO << "In initial connection state transition: " << oldState
                  << " -> (" << input << ") -> " << newState;

    bsl::shared_ptr<mqbnet::Session> session;

    if (rc == 0 && newState == State::e_NEGOTIATED) {
        rc = d_negotiator_p->createSessionOnMsgType(errStream, &session, this);
        BALL_LOG_INFO << "Created a session with " << channel()->peerUri();
    }

    if (rc != 0 || newState == State::e_NEGOTIATED) {
        BALL_LOG_INFO << "Finished initial connection with rc = " << rc
                      << ", error = '" << errStream.str() << "'";
        guard.release()->unlock();
        complete(rc, errStream.str(), session);
    }
}

bool InitialConnectionContext::isState(State state)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);

    return d_state == state;
}

bool InitialConnectionContext::isIncoming() const
{
    return d_isIncoming;
}

void* InitialConnectionContext::userData() const
{
    return d_userData_p;
}

void* InitialConnectionContext::resultState() const
{
    return d_resultState_p;
}

const bsl::shared_ptr<bmqio::Channel>&
InitialConnectionContext::channel() const
{
    return d_channelSp;
}

bmqp::EncodingType::Enum
InitialConnectionContext::authenticationEncodingType() const
{
    return d_authenticationEncodingType;
}

const bsl::shared_ptr<AuthenticationContext>&
InitialConnectionContext::authenticationContext() const
{
    return d_authenticationCtxSp;
}

const bsl::shared_ptr<NegotiationContext>&
InitialConnectionContext::negotiationContext() const
{
    return d_negotiationCtxSp;
}

InitialConnectionState::Enum InitialConnectionContext::state() const
{
    return d_state;
}

void InitialConnectionContext::complete(
    int                                     rc,
    const bsl::string&                      error,
    const bsl::shared_ptr<mqbnet::Session>& session) const
{
    BSLS_ASSERT_SAFE(d_initialConnectionCompleteCb);

    d_initialConnectionCompleteCb(rc, error, session, channel(), this);
}

bool InitialConnectionContext::isClosed() const
{
    return d_isClosed;
}

}  // close package namespace
}  // close enterprise namespace
