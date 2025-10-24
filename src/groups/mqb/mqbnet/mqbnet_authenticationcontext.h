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

// mqbnet_authenticationcontext.h                                -*-C++-*-
#ifndef INCLUDED_MQBNET_AUTHENTICATIONCONTEXT
#define INCLUDED_MQBNET_AUTHENTICATIONCONTEXT

/// @file mqbnet_authenticationcontext.h
///
/// @brief Provide the context for authenticating connections.
///
/// An instance is created per connection being authenticated. It tracks
/// the authentication state, the resulting principal, and (for the
/// initial pass) the associated InitialConnectionContext.

// MQB
#include <mqbnet_connectiontype.h>
#include <mqbplug_authenticator.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqu_sharedresource.h>

// BDE
#include <ball_log.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_memory.h>
#include <bsl_string_view.h>
#include <bslma_allocator.h>
#include <bslmt_mutex.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqio {
class Channel;
}
namespace mqbplug {
class AuthenticationResult;
}
namespace mqbnet {
class InitialConnectionContext;
}

namespace mqbnet {

// ===========================
// class AuthenticationContext
// ===========================

struct AuthenticationState {
    enum Enum {
        e_AUTHENTICATING = 0,  // Authentication is in progress.
        e_AUTHENTICATED,       // Authentication is completed.
        e_CLOSED               // Channel is closed.
    };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration
    /// `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii`
    /// for what constitutes the string representation of a
    /// @bbref{AuthenticationState::Enum} value.
    static bsl::ostream& print(bsl::ostream&             stream,
                               AuthenticationState::Enum value,
                               int                       level          = 0,
                               int                       spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to
    /// the specified enumeration `value`, if it exists, and a unique
    /// (error) string otherwise.  The string representation of `value`
    /// matches its corresponding enumerator name with the `e_` prefix
    /// elided.  Note that specifying a `value` that does not match any of
    /// the enumerators will result in a string representation that is
    /// distinct from any of those corresponding to the enumerators, but is
    /// otherwise unspecified.
    static const char* toAscii(AuthenticationState::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(AuthenticationState::Enum* out,
                          const bsl::string_view     str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&             stream,
                         AuthenticationState::Enum value);

/// VST for the context associated with an connection being authenticated.
class AuthenticationContext {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBNET.AUTHENTICATIONCONTEXT");

  public:
    // TYPES
    typedef AuthenticationState::Enum          State;
    typedef bdlmt::EventScheduler::EventHandle EventHandle;

  private:
    // DATA

    /// Used to make sure no callback is invoked on a destroyed object.
    bmqu::SharedResource<AuthenticationContext> d_self;

    bslmt::Mutex d_mutex;

    /// The authentication result to be used for authorization. It is first set
    /// during the initial authentication, and can be updated later
    /// during reauthentication.
    bsl::shared_ptr<mqbplug::AuthenticationResult> d_authenticationResultSp;

    /// Handle to the reauthentication timer event, if any.
    EventHandle d_timeoutHandle;

    State d_state;

    /// The initial connection context associated with this authentication
    /// context.
    /// It is set during the initial authentication, and is null for
    /// reauthentication.
    InitialConnectionContext* d_initialConnectionContext_p;

    bmqp_ctrlmsg::AuthenticationMessage d_authenticationMessage;

    /// The encoding type used for sending a message. It should match with the
    /// encoding type of the received message.
    bmqp::EncodingType::Enum d_authenticationEncodingType;

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    AuthenticationContext(const AuthenticationContext&);  // = delete;
    AuthenticationContext&
    operator=(const AuthenticationContext&);  // = delete;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(AuthenticationContext,
                                   bslma::UsesBslmaAllocator)
    // CREATORS
    AuthenticationContext(
        InitialConnectionContext*                  initialConnectionContext,
        const bmqp_ctrlmsg::AuthenticationMessage& authenticationMessage,
        bmqp::EncodingType::Enum                   authenticationEncodingType,
        State                                      state,
        bslma::Allocator*                          allocator = 0);

    // MANIPULATORS
    void setAuthenticationResult(
        const bsl::shared_ptr<mqbplug::AuthenticationResult>& value);
    void setInitialConnectionContext(InitialConnectionContext* value);
    void
    setAuthenticationMessage(const bmqp_ctrlmsg::AuthenticationMessage& value);
    void setAuthenticationEncodingType(bmqp::EncodingType::Enum value);
    void setConnectionType(ConnectionType::Enum value);

    /// Schedule a reauthentication timer using the specified `scheduler_p`
    /// with the specified `lifetimeMs`.
    int scheduleReauthn(bsl::ostream&             errorDescription,
                        bdlmt::EventScheduler*    scheduler_p,
                        const bsl::optional<int>& lifetimeMs,
                        const bsl::shared_ptr<bmqio::Channel>& channel);

    /// Close the specified `channel` with an `errorCode` and `errorName`
    /// indicating the reauthentication error or authentication timeout for
    /// the current context.
    void onReauthenticateErrorOrTimeout(
        const int                              errorCode,
        const bsl::string&                     errorName,
        const bsl::shared_ptr<bmqio::Channel>& channel);

    /// Called when a channel is closing. Cancel any outstanding
    /// reauthentication timer using the specified `scheduler_p`.
    void onClose(bdlmt::EventScheduler* scheduler_p);

    /// Attempt to begin reauthentication by transitioning the state from
    /// AUTHENTICATED to AUTHENTICATING.
    /// Return true if the transition occurred (i.e. state was AUTHENTICATED);
    /// otherwise return false (already authenticating, closed, or not yet
    /// authenticated).
    bool tryStartReauthentication();

    // ACCESSORS

    const bsl::shared_ptr<mqbplug::AuthenticationResult>&
                              authenticationResult() const;
    InitialConnectionContext* initialConnectionContext() const;
    const bmqp_ctrlmsg::AuthenticationMessage& authenticationMessage() const;
    bmqp::EncodingType::Enum authenticationEncodingType() const;
    ConnectionType::Enum     connectionType() const;
};

}  // close package namespace

// --------------------------
// struct AuthenticationState
// --------------------------

// FREE OPERATORS
inline bsl::ostream&
mqbnet::operator<<(bsl::ostream&                     stream,
                   mqbnet::AuthenticationState::Enum value)
{
    return AuthenticationState::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
