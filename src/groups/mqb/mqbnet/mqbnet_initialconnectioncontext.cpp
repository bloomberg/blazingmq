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
#include <mqbnet_initialconnectioncontext.h>

#include <mqbscm_version.h>

// BMQ
#include <bmqio_channel.h>

// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bsl_memory.h>

namespace BloombergLP {
namespace mqbnet {

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

InitialConnectionContext::InitialConnectionContext(bool isIncoming)
: d_resultState_p(0)
, d_userData_p(0)
, d_channelSp()
, d_initialConnectionCompleteCb()
, d_handleEventCb()
, d_authenticationEncodingType(bmqp::EncodingType::e_BER)
, d_authenticationCtxSp()
, d_negotiationCtxSp()
, d_state(InitialConnectionState::e_INITIAL)
, d_mutex()
, d_isIncoming(isIncoming)
, d_isClosed(false)
{
    // NOTHING
}

InitialConnectionContext::~InitialConnectionContext()
{
    // NOTHING
}

InitialConnectionContext& InitialConnectionContext::setUserData(void* value)
{
    d_userData_p = value;
    return *this;
}

InitialConnectionContext& InitialConnectionContext::setResultState(void* value)
{
    d_resultState_p = value;
    return *this;
}

InitialConnectionContext& InitialConnectionContext::setChannel(
    const bsl::shared_ptr<bmqio::Channel>& value)
{
    d_channelSp = value;
    return *this;
}

InitialConnectionContext& InitialConnectionContext::setCompleteCb(
    const InitialConnectionCompleteCb& value)
{
    d_initialConnectionCompleteCb = value;
    return *this;
}

InitialConnectionContext&
InitialConnectionContext::setHandleEventCb(const HandleEventCb& value)
{
    d_handleEventCb = value;
    return *this;
}

InitialConnectionContext&
InitialConnectionContext::setAuthenticationEncodingType(
    bmqp::EncodingType::Enum value)
{
    d_authenticationEncodingType = value;
    return *this;
}

InitialConnectionContext& InitialConnectionContext::setAuthenticationContext(
    const bsl::shared_ptr<AuthenticationContext>& value)
{
    d_authenticationCtxSp = value;
    return *this;
}

InitialConnectionContext& InitialConnectionContext::setNegotiationContext(
    const bsl::shared_ptr<NegotiationContext>& value)
{
    d_negotiationCtxSp = value;
    return *this;
}

void InitialConnectionContext::onClose()
{
    d_isClosed = true;
}

InitialConnectionContext&
InitialConnectionContext::setState(InitialConnectionState::Enum value)
{
    d_state = value;
    return *this;
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

const InitialConnectionContext::HandleEventCb&
InitialConnectionContext::handleEventCb() const
{
    return d_handleEventCb;
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

bslmt::Mutex& InitialConnectionContext::mutex()
{
    return d_mutex;
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
