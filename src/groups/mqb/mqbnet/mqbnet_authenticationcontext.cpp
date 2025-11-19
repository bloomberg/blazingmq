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

// mqbnet_authenticationcontext.cpp                           -*-C++-*-
#include <mqbnet_authenticationcontext.h>

#include <mqbscm_version.h>
// MQB
#include <mqbnet_initialconnectioncontext.h>

// BMQ
#include <bmqio_channel.h>
#include <bmqio_status.h>
#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>
#include <bmqu_weakmemfn.h>

// BDE
#include <ball_log.h>
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bsl_string.h>
#include <bsla_annotations.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbnet {

// --------------------------
// struct AuthenticationState
// --------------------------

bsl::ostream& AuthenticationState::print(bsl::ostream&             stream,
                                         AuthenticationState::Enum value,
                                         int                       level,
                                         int spacesPerLevel)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << AuthenticationState::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* AuthenticationState::toAscii(AuthenticationState::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(AUTHENTICATING)
        CASE(AUTHENTICATED)
        CASE(CLOSED)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool AuthenticationState::fromAscii(AuthenticationState::Enum* out,
                                    const bsl::string_view     str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(AuthenticationState::e_##M),   \
                                       str.data(),                            \
                                       static_cast<int>(str.length()))) {     \
        *out = AuthenticationState::e_##M;                                    \
        return true;                                                          \
    }

    CHECKVALUE(AUTHENTICATING)
    CHECKVALUE(AUTHENTICATED)
    CHECKVALUE(CLOSED)

#undef CHECKVALUE
    return false;
}

// ---------------------------
// class AuthenticationContext
// ---------------------------

AuthenticationContext::AuthenticationContext(
    InitialConnectionContext*                  initialConnectionContext,
    const bmqp_ctrlmsg::AuthenticationMessage& authenticationMessage,
    bmqp::EncodingType::Enum                   authenticationEncodingType,
    AuthenticationState::Enum                  state,
    bslma::Allocator*                          allocator)
: d_allocator_p(allocator)
, d_self(this)  // use default allocator
, d_mutex()
, d_authenticationResultSp()
, d_timeoutHandle()
, d_state(state)
, d_initialConnectionContext_p(initialConnectionContext)
, d_authenticationMessage(authenticationMessage)
, d_encodingType(authenticationEncodingType)
{
    // NOTHING
}

void AuthenticationContext::setAuthenticationResult(
    const bsl::shared_ptr<mqbplug::AuthenticationResult>& value)
{
    d_authenticationResultSp = value;
}

void AuthenticationContext::setAuthenticationMessage(
    const bmqp_ctrlmsg::AuthenticationMessage& value)
{
    // PRECONDITION
    BSLS_ASSERT_SAFE(d_state == AuthenticationState::e_AUTHENTICATED);

    d_authenticationMessage = value;
}

void AuthenticationContext::setAuthenticationEncodingType(
    bmqp::EncodingType::Enum value)
{
    // PRECONDITION
    BSLS_ASSERT_SAFE(d_state == AuthenticationState::e_AUTHENTICATED);

    d_encodingType = value;
}

int AuthenticationContext::setAuthenticatedAndScheduleReauthn(
    bsl::ostream&                            errorDescription,
    bdlmt::EventScheduler*                   scheduler_p,
    const bsl::optional<bsls::Types::Int64>& lifetimeMs,
    const bsl::shared_ptr<bmqio::Channel>&   channel)
{
    // executed by an *AUTHENTICATION* thread

    // PRECONDITION
    BSLS_ASSERT_SAFE(scheduler_p);

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    // d_state might be e_CLOSED if the connection is closed and
    // AuthenticationContext::onClose() is called before the authentication is
    // completed.
    if (d_state != AuthenticationState::e_AUTHENTICATING) {
        errorDescription << "State not AUTHENTICATING (is " << d_state << ")";
        return -1;
    }

    d_state = AuthenticationState::e_AUTHENTICATED;

    if (d_timeoutHandle) {
        scheduler_p->cancelEventAndWait(&d_timeoutHandle);
    }

    if (lifetimeMs.has_value()) {
        bsls::Types::Int64 lifetime = lifetimeMs.value();

        if (lifetime < 0) {
            BALL_LOG_WARN
                << "Authenticator returned negative remaining lifetime: "
                << bsl::to_string(lifetime)
                << ". Schedule reauthentication timer with lifetime set to 0.";
            lifetime = 0;
        }

        bmqu::MemOutStream errorStream;
        errorStream << "Authentication timeout after " << lifetime << " ms";

        scheduler_p->scheduleEvent(
            &d_timeoutHandle,
            bsls::TimeInterval(bmqsys::Time::nowMonotonicClock())
                .addMilliseconds(lifetime),
            bdlf::BindUtil::bind(
                bmqu::WeakMemFnUtil::weakMemFn(
                    &AuthenticationContext::onReauthenticateErrorOrTimeout,
                    d_self.acquireWeak()),
                -1,                       // errorCode
                "authenticationTimeout",  // errorName
                errorStream.str(),        // errorDescription
                channel                   // channel
                ));
    }

    return 0;
}

void AuthenticationContext::onReauthenticateErrorOrTimeout(
    int                                    errorCode,
    const bsl::string&                     errorName,
    const bsl::string&                     errorDescription,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(channel);

    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

        if (d_state == AuthenticationState::e_CLOSED) {
            return;
        }
    }  // UNLOCK

    BALL_LOG_ERROR << "Reauthentication error or timeout for '"
                   << channel->peerUri() << "' [error: " << errorDescription
                   << ", code: " << errorCode << "]";

    bmqio::Status status(bmqio::StatusCategory::e_CANCELED,
                         errorName,
                         errorCode,
                         d_allocator_p);
    channel->close(status);
}

void AuthenticationContext::onClose(bdlmt::EventScheduler* scheduler_p)
{
    // executed by *ANY* thread

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    if (d_state == AuthenticationState::e_CLOSED) {
        return;  // idempotent
    }
    d_state = AuthenticationState::e_CLOSED;

    if (d_timeoutHandle) {
        scheduler_p->cancelEventAndWait(&d_timeoutHandle);
    }
}

bool AuthenticationContext::tryStartReauthentication()
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    if (d_state == AuthenticationState::e_AUTHENTICATED) {
        d_state = AuthenticationState::e_AUTHENTICATING;
        return true;
    }

    return false;
}

const bsl::shared_ptr<mqbplug::AuthenticationResult>&
AuthenticationContext::authenticationResult() const
{
    return d_authenticationResultSp;
}

const bmqp_ctrlmsg::AuthenticationMessage&
AuthenticationContext::authenticationMessage() const
{
    // PRECONDITION
    BSLS_ASSERT_SAFE(d_state == AuthenticationState::e_AUTHENTICATING);

    return d_authenticationMessage;
}

bmqp::EncodingType::Enum AuthenticationContext::encodingType() const
{
    // PRECONDITION
    BSLS_ASSERT_SAFE(d_state == AuthenticationState::e_AUTHENTICATING);

    return d_encodingType;
}

InitialConnectionContext* AuthenticationContext::initialConnectionContext()
{
    return d_initialConnectionContext_p;
}

}  // namespace mqbnet
}  // namespace BloombergLP
