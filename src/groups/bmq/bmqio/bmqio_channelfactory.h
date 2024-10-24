// Copyright 2019-2023 Bloomberg Finance L.P.
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

// bmqio_channelfactory.h                                             -*-C++-*-
#ifndef INCLUDED_BMQIO_CHANNELFACTORY
#define INCLUDED_BMQIO_CHANNELFACTORY

//@PURPOSE: Provide a protocol for a 'bmqio::Channel' factory.
//
//@CLASSES:
//  bmqio::ChannelFactory:      protocol for a 'bmqio::Channel' factory
//  bmqio::ChannelFactoryEvent: event type for connec/listen notifications
//
//@SEE_ALSO:
//  bmqio_channel
//  bmqio_channeldecorator
//
//@DESCRIPTION: This component defines a pure protocol,
// 'bmqio::ChannelFactory', which is a factory of objects that implement the
// 'bmqio::Channel' protocol.  It also provides an enum,
// 'bmqio::ChannelFactoryEvent' for categorizing the various events
// notifications resulting from an operation on the factory.

#include <bmqio_status.h>

// BDE
#include <bsl_functional.h>
#include <bsl_iosfwd.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_managedptr.h>

namespace BloombergLP {
namespace bmqio {

// FORWARD DECLARATION
class Channel;
class ConnectOptions;
class ListenOptions;

// ==========================
// struct ChannelFactoryEvent
// ==========================

/// This enum represents the type of events which can be emitted by
/// operations on the `bmqio::ChannelFactory` object.
struct ChannelFactoryEvent {
    // TYPES
    enum Enum {
        e_CHANNEL_UP             = 1,
        e_CONNECT_ATTEMPT_FAILED = 2,
        e_CONNECT_FAILED         = 3
    };

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
    /// `ChannelFactoryEvent::Enum` value.
    static bsl::ostream& print(bsl::ostream&             stream,
                               ChannelFactoryEvent::Enum value,
                               int                       level          = 0,
                               int                       spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(ChannelFactoryEvent::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&             stream,
                         ChannelFactoryEvent::Enum value);

// ===================================
// class ChannelFactoryOperationHandle
// ===================================

/// Handle to an ongoing `listen` or `connect` operation of a
/// `ChannelFactory`.
class ChannelFactoryOperationHandle {
  public:
    // CREATORS
    virtual ~ChannelFactoryOperationHandle();

    // MANIPULATORS

    /// Cancel the operation.
    virtual void cancel() = 0;

    /// Return a reference providing modifiable access to the properties of
    /// this object.
    virtual bmqvt::PropertyBag& properties() = 0;

    // ACCESSORS

    /// Return a reference providing const access to the properties of this
    /// object.
    virtual const bmqvt::PropertyBag& properties() const = 0;
};

// ====================
// class ChannelFactory
// ====================

/// Protocol for a factory of `bmqio::Channel` objects.
class ChannelFactory {
  public:
    // PUBLIC TYPES
    typedef ChannelFactoryOperationHandle OpHandle;

    /// Callback provided to `connect` or `listen` to inform the user of
    /// operation results.  The specified `event` indicates the type of the
    /// notification with the specified `status` providing additional
    /// information about the source of error (when 'event ==
    /// e_CONNECT_FAILED').  The specified `handle` is the handle returned
    /// by the corresponding call to `connect` or `listen`, while the
    /// specified `userData` is the `userData` that was provided to said
    /// call.  The specified `channel` is provided on `e_CHANNEL_UP`.
    typedef bsl::function<void(ChannelFactoryEvent::Enum       event,
                               const Status&                   status,
                               const bsl::shared_ptr<Channel>& channel)>
        ResultCallback;

    // CREATORS

    /// Destroy this object.
    virtual ~ChannelFactory();

    // MANIPULATORS

    /// Listen for connections according to the specified `options` (whose
    /// meaning is implementation-defined), and invoke the specified `cb`
    /// when they are created or a connection attempt fails.  Load into the
    /// optionally-specified `handle` a handle that can be used to cancel
    /// this operation.  Return `e_SUCCESS` on success or a failure
    /// StatusCategory on error, populating the optionally-specified
    /// `status` with more detailed error information.
    virtual void listen(Status*                      status,
                        bslma::ManagedPtr<OpHandle>* handle,
                        const ListenOptions&         options,
                        const ResultCallback&        cb) = 0;

    /// Attempt to establish a connection according to the specified
    /// `options` (whose meaning is implementation-defined), and invoke the
    /// specified `cb` when it is created or the connection attempt fails.
    /// Load into the optionally-specified `handle` a handle that can be
    /// used to cancel this operation.  Return `e_SUCCESS` on success or a
    /// failure StatusCategory on error, populating the
    /// optionally-specified `status` with more detailed error information.
    /// Note that if `options.autoReconnect()` is true, then the client
    /// expects for the connection to be automatically recreated whenever
    /// it receives a `ChannelFactoryEvent::e_CONNECT_FAILED` or whenever
    /// the channel produced as a result of this connection is closed.  If
    /// `options.autoReconnect()` is `true` and the implementing
    /// `ChannelFactory` doesn't provide this behavior, it must fail the
    /// connection immediately.
    virtual void connect(Status*                      status,
                         bslma::ManagedPtr<OpHandle>* handle,
                         const ConnectOptions&        options,
                         const ResultCallback&        cb) = 0;
};

}  // close package namespace

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------
// struct ChannelFactoryEvent
// --------------------------

inline bsl::ostream& bmqio::operator<<(bsl::ostream&                    stream,
                                       bmqio::ChannelFactoryEvent::Enum value)
{
    return ChannelFactoryEvent::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
