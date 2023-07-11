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

// mwcio_channel.h                                                    -*-C++-*-
#ifndef INCLUDED_MWCIO_CHANNEL
#define INCLUDED_MWCIO_CHANNEL

//@PURPOSE: Provide a pure protocol for a bi-directional async channel.
//
//@CLASSES:
//  mwcio::Channel: Interface for a bi-directional async channel.
//
//@DESCRIPTION: This component provides a pure protocol, 'mwcio::Channel', to
// be implemented by any transport mechanism to asynchronously send and receive
// arbitrary blobs of data.
//
// A 'mwcio::Channel' provides operations to asynchronously read and write data
// (e.g. blobs).

// MWC

#include <mwcio_status.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlmt_signaler.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_string.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mwcio {

// ===========================
// struct ChannelWatermarkType
// ===========================

/// Watermark event types supported by a `Channel`
struct ChannelWatermarkType {
    enum Enum { e_LOW_WATERMARK = 1, e_HIGH_WATERMARK = 2 };

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
    /// `ChannelWatermarkType::Enum` value.
    static bsl::ostream& print(bsl::ostream&              stream,
                               ChannelWatermarkType::Enum value,
                               int                        level          = 0,
                               int                        spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(ChannelWatermarkType::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(ChannelWatermarkType::Enum* out,
                          const bslstl::StringRef&    str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&              stream,
                         ChannelWatermarkType::Enum value);

// =============
// class Channel
// =============

/// Pure protocol for a bi-directional async channel.
class Channel {
  public:
    // TYPES

    /// A callback of this type is invoked for read when either the
    /// requested number of bytes are available, or an error occurs.  The
    /// callback is invoked with three arguments: (1) a `mwcio::Status` that
    /// indicates the result of the read operation, and if this status is
    /// equal to `MWCIO_SUCCESS`, (2) a pointer to an integer value where
    /// the callee can indicate how many more bytes are needed to complete
    /// the read operation, and (3) a modifiable `bdlbb::Blob` object
    /// containing the payload.  The caller is responsible for taking
    /// ownership of a certain number of bytes in the `bdlbb::Blob` and
    /// re-adjusting it appropriately.  Note that the read operation is not
    /// considered completed until the callee indicates that zero more bytes
    /// are needed (argument 2).  Also note that the last two arguments are
    /// ignored if the first argument doesn't indicate success.
    typedef bsl::function<
        void(const Status& status, int* numNeeded, bdlbb::Blob* blob)>
        ReadCallback;

    typedef void CloseFnType(const Status& status);

    /// Callback invoked when a `Channel` is closed with the specified
    /// `status`.
    typedef bsl::function<CloseFnType> CloseFn;

    typedef void WatermarkFnType(ChannelWatermarkType::Enum type);

    /// Callback invoked when a watermark event of the specified `type`
    /// occurs on the channel.
    typedef bsl::function<WatermarkFnType> WatermarkFn;

    /// Callback that can be passed to `execute`.
    typedef bsl::function<void()> ExecuteCb;

  public:
    // CREATORS

    /// Destroy this object.
    virtual ~Channel();

    // MANIPULATORS

    /// Initiate an asynchronous (timed) read operation on this channel, or
    /// append this request to the currently pending requests if an
    /// asynchronous read operation was already initiated, with an
    /// associated specified relative `timeout`.  When at least the
    /// specified `numBytes` of data are available after all previous
    /// requests have been processed, if any, or when the timeout is
    /// reached, the specified `readCallback` will be invoked (with
    /// `category() == e_SUCCESS` or `category() == e_TIMEOUT`,
    /// respectively).  Return `e_SUCCESS` on success, or a different value
    /// on failure, populating the optionally specified `status` with
    /// additional information about the failure.
    virtual void
    read(Status*                   status,
         int                       numBytes,
         const ReadCallback&       readCallback,
         const bsls::TimeInterval& timeout = bsls::TimeInterval()) = 0;

    /// Enqueue the specified message `blob` to be written to this channel.
    /// Optionally provide `highWaterMark` to specify the maximum data size
    /// that can be enqueued.  If `highWaterMark` is not specified then
    /// INT_MAX is used.  Return 0 on success, and a non-zero value
    /// otherwise.  On error, the return value may equal to one of the
    /// enumerators in `mwcio_ChannelStatus::Type`.  Note that success does
    /// not imply that the data has been written or will be successfully
    /// written to the underlying stream used by this channel.  Also note
    /// that in addition to `highWatermark` the enqueued portion must also
    /// be less than a high watermark value supplied at the construction of
    /// this channel for the write to succeed.
    virtual void
    write(Status*            status,
          const bdlbb::Blob& blob,
          bsls::Types::Int64 watermark = bsl::numeric_limits<int>::max()) = 0;

    /// Cancel all pending read requests, and invoke their read callbacks
    /// with a `mwcio::ChannelStatus::e_CANCELED` status.  Note that if the
    /// channel is active, the read callbacks are invoked in the thread in
    /// which the channel's data callbacks are invoked, else they are
    /// invoked in the thread calling `cancelRead`.
    virtual void cancelRead() = 0;

    /// Shutdown this channel, and cancel all pending read requests (but do
    /// not invoke them).  Pass the specified `status` to any registered
    /// `CloseFn`s.
    virtual void close(const Status& status = Status()) = 0;

    /// Execute the specified `cb` serialized with calls to any registered
    /// read callbacks, or any `close` or `watermark` event handlers for
    /// this channel.  Return `0` on success or a negative value if the `cb`
    /// could not be enqueued for execution.
    virtual int execute(const ExecuteCb& cb) = 0;

    /// Register the specified `cb` to be invoked when a `close` event
    /// occurs for this channel.  Return a `bdlmt::SignalerConnection`
    /// object than can be used to unregister the callback.
    virtual bdlmt::SignalerConnection onClose(const CloseFn& cb) = 0;

    /// Register the specified `cb` to be invoked when a `watermark` event
    /// occurs for this channel.  Return a `bdlmt::SignalerConnection`
    /// object than can be used to unregister the callback.
    virtual bdlmt::SignalerConnection onWatermark(const WatermarkFn& cb) = 0;

    /// Return a reference providing modifiable access to the properties of
    /// this Channel.
    virtual mwct::PropertyBag& properties() = 0;

    // VIRTUAL ACCESSORS

    /// Return the URI of the "remote" end of this channel.  It is up to the
    /// underlying implementation to define the format of the returned URI.
    virtual const bsl::string& peerUri() const = 0;

    /// Return a reference providing modifiable access to the properties of
    /// this Channel.
    virtual const mwct::PropertyBag& properties() const = 0;
};

}  // close package namespace

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------
// struct ChannelWatermarkType
// ---------------------------

inline bsl::ostream& mwcio::operator<<(bsl::ostream& stream,
                                       mwcio::ChannelWatermarkType::Enum value)
{
    return mwcio::ChannelWatermarkType::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
