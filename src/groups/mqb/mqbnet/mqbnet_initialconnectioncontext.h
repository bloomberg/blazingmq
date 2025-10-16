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

// mqbnet_initialconnectioncontext.h                        -*-C++-*-
#ifndef INCLUDED_MQBNET_INITIALCONNECTIONCONTEXT
#define INCLUDED_MQBNET_INITIALCONNECTIONCONTEXT

/// @file mqbnet_initialconnectioncontext.h
/// @brief Context for authenticating and negotiating a new session.
///
/// InitialConnectionContext owns the transient state needed while a
/// connection is performing the initial handshake (authentication followed
/// by negotiation, or direct negotiation with implicit/anonymous
/// authentication).
///
/// Responsibilities:
/// - Hold caller‐supplied opaque pointers (user data / result state) so they
///   can flow between transport and application layers.
/// - Drive the read loop: schedule reads, accumulate bytes, decode control
///   messages (Authentication / Negotiation), and dispatch them.
/// - Track handshake state and invoke the completion callback exactly once
///   with either a fully constructed Session or an error.
/// - Bridge to AuthenticationContext / NegotiationContext once those phases
///   are established.
///
/// A single instance is created per inbound or outbound connection attempt
/// and is discarded once the channel is closed.

// MQB
#include <mqbnet_authenticator.h>
#include <mqbnet_connectiontype.h>
#include <mqbnet_negotiator.h>
#include <mqbplug_authenticator.h>

// BMQ
#include <bmqio_status.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>

// BDE
#include <ball_log.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_optional.h>
#include <bsl_string.h>
#include <bsl_variant.h>
#include <bslmt_mutex.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqio {
class Channel;
};

namespace mqbnet {

// FORWARD DECLARATION
class SessionEventProcessor;
class Cluster;
class Session;
class AuthenticationContext;
class NegotiationContext;

// =============================
// struct InitialConnectionState
// =============================

struct InitialConnectionState {
    // TYPES
    enum Enum {
        e_INITIAL                = 0,  // Initial state.
        e_AUTHENTICATING         = 1,  // First message is Auth Request.
        e_AUTHENTICATED          = 2,  // Authentication success.
        e_DEFAULT_AUTHENTICATING = 3,  // First message is Negotiation
        e_NEGOTIATING_OUTBOUND   = 4,  // Outbound negotiation.
        e_NEGOTIATED             = 5,  // Negotiation success.  Final state.
        e_FAILED                 = 6   // Final state.
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
    /// @bbref{InitialConnectionState::Enum} value.
    static bsl::ostream& print(bsl::ostream&                stream,
                               InitialConnectionState::Enum value,
                               int                          level = 0,
                               int spacesPerLevel                 = 4);

    /// Return the non-modifiable string representation corresponding to
    /// the specified enumeration `value`, if it exists, and a unique
    /// (error) string otherwise.  The string representation of `value`
    /// matches its corresponding enumerator name with the `e_` prefix
    /// elided.  Note that specifying a `value` that does not match any of
    /// the enumerators will result in a string representation that is
    /// distinct from any of those corresponding to the enumerators, but is
    /// otherwise unspecified.
    static const char* toAscii(InitialConnectionState::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(InitialConnectionState::Enum* out,
                          const bslstl::StringRef&      str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                stream,
                         InitialConnectionState::Enum value);

// =============================
// struct InitialConnectionEvent
// =============================

struct InitialConnectionEvent {
    // TYPES
    enum Enum {
        e_NONE                = 0,
        e_OUTBOUND_NEGOTATION = 1,
        e_AUTH_REQUEST        = 2,  // handleAuthentication
        e_NEGOTIATION_MESSAGE = 3,  // handleNegotiationMessage
        e_AUTHN_SUCCESS       = 4,
        e_ERROR               = 5
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
    /// @bbref{InitialConnectionEvent::Enum} value.
    static bsl::ostream& print(bsl::ostream&                stream,
                               InitialConnectionEvent::Enum value,
                               int                          level = 0,
                               int spacesPerLevel                 = 4);

    /// Return the non-modifiable string representation corresponding to
    /// the specified enumeration `value`, if it exists, and a unique
    /// (error) string otherwise.  The string representation of `value`
    /// matches its corresponding enumerator name with the `e_` prefix
    /// elided.  Note that specifying a `value` that does not match any of
    /// the enumerators will result in a string representation that is
    /// distinct from any of those corresponding to the enumerators, but is
    /// otherwise unspecified.
    static const char* toAscii(InitialConnectionEvent::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(InitialConnectionEvent::Enum* out,
                          const bslstl::StringRef&      str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                stream,
                         InitialConnectionEvent::Enum value);

// ==============================
// class InitialConnectionContext
// ==============================

/// Each session being authenticated and negotiated get its own context.
class InitialConnectionContext
: public bsl::enable_shared_from_this<InitialConnectionContext> {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBNET.INITIALCONNECTIONCONTEXT");

  public:
    // TYPES
    typedef bsl::function<void(
        int                                    status,
        const bsl::string&                     errorDescription,
        const bsl::shared_ptr<Session>&        session,
        const bsl::shared_ptr<bmqio::Channel>& channel,
        const InitialConnectionContext*        initialConnectionContext)>
        InitialConnectionCompleteCb;

    typedef mqbnet::InitialConnectionState::Enum State;
    typedef mqbnet::InitialConnectionEvent::Enum Event;

  private:
    // DATA
    mutable bslmt::Mutex d_mutex;

    /// Authenticator to use for authenticating a connection.
    mqbnet::Authenticator* d_authenticator_p;

    /// Negotiator to use for converting a Channel to a Session.
    mqbnet::Negotiator* d_negotiator_p;

    /// Raw pointer, held not owned, to some user data
    /// the session factory will pass back to the
    /// 'resultCb' method (used to inform of the
    /// success/failure of a session negotiation).  This
    /// may or may not be set by the caller, during
    /// 'TcpSessionFactory::handleInitialConnection()';
    /// and may or may not be changed by the negotiator concrete
    /// implementation before invoking the
    /// 'InitialConnectionCompleteCb'.  This is used to bind low level
    /// data (from transport layer) to the session; and
    /// can be overriden/set by the negotiation
    /// implementation (typically for the case of
    /// 'listen' sessions, since those are
    /// 'sporadically' happening and there is not enough
    /// context at the transport layer to find back this
    /// data).
    void* d_resultState_p;

    /// Raw pointer, held not owned, to some user data
    /// the InitialConnectionContext can use
    /// while negotiating the session.  This may or may
    /// not be set by the caller, during
    /// 'TcpSessionFactory::handleInitialConnection()';
    /// and should not be changed during negotiation (this data is not
    /// used by the session factory, so changing it will
    /// have no effect).  This is used to bind high
    /// level data (from application layer) to the
    /// application layer (the negotiator concrete
    /// implementation) (typically for the case of
    /// 'connect' sessions to provide information to use
    /// for negotiating the session with the remote
    /// peer).
    void* d_userData_p;

    /// The channel to use for the initial connection.
    bsl::shared_ptr<bmqio::Channel> d_channelSp;

    /// The callback to invoke to notify of the status of the initial
    /// connection.
    InitialConnectionCompleteCb d_initialConnectionCompleteCb;

    /// Encoding for authentication messages.  Defaults to BER until the first
    /// inbound authentication message is decoded, then set to that message's
    /// encoding and reused for outbound replies.  Temporary field; copied into
    /// the AuthenticationContext later.
    bmqp::EncodingType::Enum d_authenticationEncodingType;

    /// The AuthenticationContext updated upon receiving an
    /// authentication message.
    bsl::shared_ptr<AuthenticationContext> d_authenticationCtxSp;

    /// The NegotiationContext updated upon receiving a negotiation message.
    bsl::shared_ptr<NegotiationContext> d_negotiationCtxSp;

    /// The state of the initial connection.
    State d_state;

    /// True if the session being negotiated originates
    /// from a remote peer (i.e., a 'listen'); false if
    /// it originates from us (i.e., a 'connect).
    bool d_isIncoming;

    /// True if the associated channel is closed (with `onClose`).
    bool d_isClosed;

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    InitialConnectionContext(const InitialConnectionContext&);  // = delete;
    InitialConnectionContext&
    operator=(const InitialConnectionContext&);  // = delete;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(InitialConnectionContext,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object having the specified `isIncoming` value.
    InitialConnectionContext(
        bool                                   isIncoming,
        mqbnet::Authenticator*                 authenticator,
        mqbnet::Negotiator*                    negotiator,
        void*                                  userData,
        void*                                  resultState,
        const bsl::shared_ptr<bmqio::Channel>& channel,
        const InitialConnectionCompleteCb&     initialConnectionCompleteCb,
        bslma::Allocator*                      allocator = 0);

    ~InitialConnectionContext();

  private:
    // PRIVATE MANIPULATORS
    void setState(State value);

    int readBlob(bsl::ostream& errorDescription,
                 bdlbb::Blob*  outPacket,
                 bool*         isFullBlob,
                 int*          numNeeded,
                 bdlbb::Blob*  blob);

    int processBlob(bsl::ostream& errorDescription, const bdlbb::Blob& blob);

    /// Decode the initial connection messages received in the specified
    /// `blob` and store it, on success, in the specified `message`, returning
    /// 0.  Return a non-zero code on error and populate the specified
    /// `errorDescription` with a description of the error.
    int decodeInitialConnectionMessage(
        bsl::ostream&                                   errorDescription,
        bsl::variant<bsl::monostate,
                     bmqp_ctrlmsg::AuthenticationMessage,
                     bmqp_ctrlmsg::NegotiationMessage>* message,
        const bdlbb::Blob&                              blob);

    /// Create and initialize a `NegotiationContext`.
    void createNegotiationContext();

    /// Perform default authentication using the anonymous credential for the
    /// current context.  Return a non-zero code on error and
    /// populate the specified `errorDescription` with a description of the
    /// error.
    int handleDefaultAuthentication(bsl::ostream& errorDescription);

  public:
    // MANIPULATORS

    /// Set the corresponding field to the specified `value`.
    void setResultState(void* value);
    void setAuthenticationContext(
        const bsl::shared_ptr<AuthenticationContext>& value);

    /// Called by the IO upon `onClose` signal
    void onClose();

    /// Read callback invoked when data is available on the channel.
    /// Process the received `blob` if `status` indicates success.
    /// Set `numNeeded` to request additional bytes if needed for a
    /// full message.
    void readCallback(const bmqio::Status& status,
                      int*                 numNeeded,
                      bdlbb::Blob*         blob);

    /// Schedule the next read operation on the channel.
    /// Return 0 on success, or a non-zero error code and populate
    /// `errorDescription` with details on failure.
    int scheduleRead(bsl::ostream& errorDescription);

    /// Process a handshake event with the given `statusCode` and
    /// `errorDescription`. The `input` specifies the event type, and
    /// `message` contains any associated control message data.
    /// This drives the authentication/negotiation state machine.
    void handleEvent(int                statusCode,
                     const bsl::string& errorDescription,
                     Event              input,
                     const bsl::variant<bsl::monostate,
                                        bmqp_ctrlmsg::AuthenticationMessage,
                                        bmqp_ctrlmsg::NegotiationMessage>&
                         message = bsl::monostate());

    // ACCESSORS

    /// Return the value of the corresponding field.
    bool                                   isIncoming() const;
    void*                                  userData() const;
    void*                                  resultState() const;
    const bsl::shared_ptr<bmqio::Channel>& channel() const;
    bmqp::EncodingType::Enum               authenticationEncodingType() const;
    const bsl::shared_ptr<AuthenticationContext>&
                                               authenticationContext() const;
    const bsl::shared_ptr<NegotiationContext>& negotiationContext() const;
    bool                                       isClosed() const;

    State state() const;

    void complete(int                                     rc,
                  const bsl::string&                      error,
                  const bsl::shared_ptr<mqbnet::Session>& session) const;
};

}  // close package namespace

// -----------------------------
// struct InitialConnectionState
// -----------------------------

// FREE OPERATORS
inline bsl::ostream&
mqbnet::operator<<(bsl::ostream&                        stream,
                   mqbnet::InitialConnectionState::Enum value)
{
    return InitialConnectionState::print(stream, value, 0, -1);
}

// -----------------------------
// struct InitialConnectionEvent
// -----------------------------

// FREE OPERATORS
inline bsl::ostream&
mqbnet::operator<<(bsl::ostream&                        stream,
                   mqbnet::InitialConnectionEvent::Enum value)
{
    return InitialConnectionEvent::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
