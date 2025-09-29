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
/// @brief Provide the context for handling an initial connection.
///
/// @bbref{mqbnet::InitialConnectionContext} is a holds the context associated
/// with a session being negotiated and functions necessary for handling an
/// initial connection.
///
/// For the data it holds, it allows bi-directional generic communication
/// between the application layer and the transport layer: for example, a user
/// data information can be passed in at application layer, kept and carried
/// over in the transport layer and retrieved in the negotiator concrete
/// implementation.  Similarly, a 'cookie' can be passed in from application
/// layer, to the result callback notification in the transport layer (usefull
/// for 'listen-like' established connection where the entry point doesn't
/// allow to bind specific user data, which then can be retrieved at
/// application layer during negotiation).
///
/// For the initial connection logic, It either reads incoming Authentication
/// and Negotiation messages from the IO layer, dispatches them to the
/// appropriate handler for processing, or calling the appropriate handler to
/// send outbound Authentication and Negotiation messages.

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
#include <bsl_functional.h>
#include <bsl_iosfwd.h>
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
        e_AUTH_SUCCESS        = 4,
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
    bslmt::Mutex d_mutex;

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

    /// The encoding type that an authentication message uses.
    /// This is set by the `Authenticator` when it receives an
    /// authentication message, and is used when it sends an
    /// authentication message back to the remote peer.
    bmqp::EncodingType::Enum d_authenticationEncodingType;

    /// The AuthenticationContext updated upon receiving an
    /// authentication message.
    bsl::shared_ptr<AuthenticationContext> d_authenticationCtxSp;

    /// The NegotiationContext updated upon receiving a negotiation message.
    bsl::shared_ptr<NegotiationContext> d_negotiationCtxSp;

    /// The state of the initial connection.
    InitialConnectionState::Enum d_state;

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
    BSLMF_NESTED_TRAIT_DECLARATION(AuthenticationContext,
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
    InitialConnectionContext& setState(InitialConnectionState::Enum value);

    int readBlob(bsl::ostream&        errorDescription,
                 bdlbb::Blob*         outPacket,
                 bool*                isFullBlob,
                 const bmqio::Status& status,
                 int*                 numNeeded,
                 bdlbb::Blob*         blob);

    int processBlob(bsl::ostream& errorDescription, const bdlbb::Blob& blob);

    /// Decode the initial connection messages received in the specified
    /// `blob` and store it, on success, in the specified optional
    /// `negotiationMsg`, returning 0.  Return a non-zero code on error and
    /// populate the specified `errorDescription` with a description of the
    /// error.
    int decodeInitialConnectionMessage(
        bsl::ostream& errorDescription,
        bsl::optional<bsl::variant<bmqp_ctrlmsg::AuthenticationMessage,
                                   bmqp_ctrlmsg::NegotiationMessage> >*
                           message,
        const bdlbb::Blob& blob);

    /// Create and initialize a `NegotiationContext`.
    void createNegotiationContext();

    /// Perform default authentication using the anonymous credential for the
    /// current context.  Return a non-zero code on error and
    /// populate the specified `errorDescription` with a description of the
    /// error.
    int handleDefaultAuthentication(bsl::ostream& errorDescription);

    // PRIVATE ACCESSORS
    InitialConnectionState::Enum state() const;

  public:
    // MANIPULATORS

    /// Set the corresponding field to the specified `value` and return a
    /// reference offering modifiable access to this object.
    InitialConnectionContext& setResultState(void* value);
    InitialConnectionContext& setAuthenticationContext(
        const bsl::shared_ptr<AuthenticationContext>& value);

    /// Called by the IO upon `onCLose` signal
    void onClose();

    /// Read callback method invoked when receiving data in the specified
    /// `blob`, if the specified `status` indicates success.  The specified
    /// `numNeeded` can be used to indicate if more bytes are needed in
    /// order to get a full message.  The specified `context` holds the
    /// initial connection context associated to this read.
    void readCallback(const bmqio::Status& status,
                      int*                 numNeeded,
                      bdlbb::Blob*         blob);

    /// Schedule a read for the initial connection of the session of the
    /// current context.  Return a non-zero code on error and
    /// populate the specified `errorDescription` with a description of the
    /// error.
    int scheduleRead(bsl::ostream& errorDescription);

    /// Handle an event occurs under the current state given the specified
    /// `statusCode` and `errorDescription`. The specified `input` is the event
    /// to handle, the current context is associated to this event, and the
    /// specified `message` is an optional message that may be used to handle
    /// the event.
    void handleEvent(
        int                statusCode,
        const bsl::string& errorDescription,
        Event              input,
        const bsl::optional<bsl::variant<bmqp_ctrlmsg::AuthenticationMessage,
                                         bmqp_ctrlmsg::NegotiationMessage> >&
            message = bsl::nullopt);

    /// Compare the current state with the specified `state`.  Return true if
    /// they are the same, and false otherwise.
    bool isState(State state);

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
