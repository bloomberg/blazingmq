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

// mwcio_testchannel.h                                                -*-C++-*-
#ifndef INCLUDED_MWCIO_TESTCHANNEL
#define INCLUDED_MWCIO_TESTCHANNEL

//@PURPOSE: Provide an implementation of the Channel protocol for test drivers
//
//@CLASSES:
// mwcio::TestChannel
//
//@SEE_ALSO:
//
//@DESCRIPTION: This component defines a mechanism, 'mwcio::TestChannel',
// which is a test implementation of the 'mwcio::Channel' protocol for use in
// test drivers.  It allows the user to set the return values of its
// functions, and stores the arguments used to call its methods.

// MWC

#include <mwcio_channel.h>

// BDE
#include <bsl_deque.h>
#include <bslmt_condition.h>
#include <bslmt_mutex.h>
#include <bsls_systemtime.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mwcio {

// =================
// class TestChannel
// =================

/// A test implementation of the `Channel` protocol for use in test
/// drivers.
class TestChannel : public Channel {
  public:
    // TYPES
    struct ReadCall {
        // DATA
        int                d_numBytes;
        ReadCallback       d_readCallback;
        bsls::TimeInterval d_timeout;

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(ReadCall, bslma::UsesBslmaAllocator)

        // CREATORS
        ReadCall(int                       numBytes,
                 const ReadCallback&       readCallback,
                 const bsls::TimeInterval& timeout,
                 bslma::Allocator*         basicAllocator)
        : d_numBytes(numBytes)
        , d_readCallback(bsl::allocator_arg_t(), basicAllocator, readCallback)
        , d_timeout(timeout)
        {
            // NOTHING
        }
    };

    struct WriteCall {
        // DATA
        bdlbb::Blob        d_blob;
        bsls::Types::Int64 d_watermark;

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(WriteCall, bslma::UsesBslmaAllocator)

        // CREATORS
        WriteCall(const bdlbb::Blob& blob,
                  bsls::Types::Int64 watermark,
                  bslma::Allocator*  basicAllocator)
        : d_blob(blob, basicAllocator)
        , d_watermark(watermark)
        {
            // NOTHING
        }
    };

    struct CancelReadCall {
        // NOTHING
    };

    struct CloseCall {
        // DATA
        Status d_status;

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(CloseCall, bslma::UsesBslmaAllocator)

        // CREATORS
        CloseCall(const Status& status, bslma::Allocator* basicAllocator)
        : d_status(status, basicAllocator)
        {
            // NOTHING
        }
    };

    struct ExecuteCall {
        // DATA
        ExecuteCb d_cb;

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(ExecuteCall, bslma::UsesBslmaAllocator)

        // CREATORS
        ExecuteCall(const ExecuteCb& cb, bslma::Allocator* basicAllocator)
        : d_cb(bsl::allocator_arg_t(), basicAllocator, cb)
        {
            // NOTHING
        }
    };

    struct OnCloseCall {
        // DATA
        CloseFn                   d_closeFn;
        bdlmt::SignalerConnection d_connection;

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(OnCloseCall, bslma::UsesBslmaAllocator)

        // CREATORS
        OnCloseCall(const CloseFn&                   closeFn,
                    const bdlmt::SignalerConnection& connection,
                    bslma::Allocator*                basicAllocator)
        : d_closeFn(bsl::allocator_arg_t(), basicAllocator, closeFn)
        , d_connection(connection)
        {
            // NOTHING
        }
    };

    struct OnWatermarkCall {
        // DATA
        WatermarkFn               d_watermarkFn;
        bdlmt::SignalerConnection d_connection;

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(OnWatermarkCall,
                                       bslma::UsesBslmaAllocator)

        // CREATORS
        OnWatermarkCall(const WatermarkFn&               watermarkFn,
                        const bdlmt::SignalerConnection& connection,
                        bslma::Allocator*                basicAllocator)
        : d_watermarkFn(bsl::allocator_arg_t(), basicAllocator, watermarkFn)
        , d_connection(connection)
        {
        }
    };

    typedef bdlmt::Signaler<CloseFnType>     CloseSignaler;
    typedef bdlmt::Signaler<WatermarkFnType> WatermarkSignaler;

  private:
    // DATA
    Status                      d_readStatus;
    Status                      d_writeStatus;
    int                         d_executeRet;
    CloseSignaler               d_closeSignaler;
    WatermarkSignaler           d_watermarkSignaler;
    bsl::deque<ReadCall>        d_readCalls;
    bsl::deque<WriteCall>       d_writeCalls;
    bsl::deque<CancelReadCall>  d_cancelReadCalls;
    bsl::deque<CloseCall>       d_closeCalls;
    bsl::deque<ExecuteCall>     d_executeCalls;
    bsl::deque<OnCloseCall>     d_onCloseCalls;
    bsl::deque<OnWatermarkCall> d_onWatermarkCalls;
    mwct::PropertyBag           d_properties;
    bsl::string                 d_peerUri;
    bslmt::Mutex                d_mutex;
    bslmt::Condition            d_condition;
    bool                        d_isFinal;
    bool                        d_hasNoMoreWriteCalls;
    bslma::Allocator*           d_allocator_p;

    // NOT IMPLEMENTED
    TestChannel(const TestChannel&);
    TestChannel& operator=(const TestChannel&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TestChannel, bslma::UsesBslmaAllocator)

    // CREATORS
    explicit TestChannel(bslma::Allocator* basicAllocator = 0);
    ~TestChannel() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Reset this object to its default-constructed state.
    void reset();

    /// Set the status returned by calls to `read` to the specified
    /// `status`.
    void setReadStatus(const Status& status);

    /// Set the status returned by calls to `write` to the specified
    /// `status`.
    void setWriteStatus(const Status& status);

    /// Set the value returned by calls to `execute` to the specified
    /// `ret`.
    void setExecuteRet(int ret);

    /// Set the string returned by `peerUri` to the specified `value`.
    void setPeerUri(const bslstl::StringRef& value);

    bsl::deque<ReadCall>&       readCalls();
    bsl::deque<WriteCall>&      writeCalls();
    bsl::deque<CancelReadCall>& cancelReadCalls();
    bsl::deque<CloseCall>&      closeCalls();
    bsl::deque<ExecuteCall>&    executeCalls();
    bsl::deque<OnCloseCall>&    onCloseCalls();

    /// Return a reference providing modifiable access to the deque
    /// containing the stored calls to the corresponding function.
    bsl::deque<OnWatermarkCall>& onWatermarkCalls();

    bool waitFor(int                       size     = 1,
                 bool                      isFinal  = true,
                 const bsls::TimeInterval& interval = bsls::TimeInterval(0.1));

    /// Since writing is now asynchronous, need to synchronize
    bool waitFor(const bdlbb::Blob&        blob,
                 const bsls::TimeInterval& interval,
                 bool                      pop = true);

    /// Pops a write-call from those written to the channel (FIFO ordering).
    WriteCall popWriteCall();

    /// Pops a close-call from those written to the channel (FIFO ordering).
    CloseCall popCloseCall();

    bool      closeCallsEmpty();

    CloseSignaler& closeSignaler();

    /// Return a reference providing modifiable access to the corresponding
    /// signaler.
    WatermarkSignaler& watermarkSignaler();

    // ACCESSORS
    const Status& writeStatus() const;
    bool          hasNoMoreWriteCalls() const;

    // Channel
    void read(Status*                   status,
              int                       numBytes,
              const ReadCallback&       readCallback,
              const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    void write(Status*            status,
               const bdlbb::Blob& blob,
               bsls::Types::Int64 watermark = bsl::numeric_limits<int>::max())
        BSLS_KEYWORD_OVERRIDE;

    void cancelRead() BSLS_KEYWORD_OVERRIDE;
    void close(const Status& status = Status()) BSLS_KEYWORD_OVERRIDE;
    int  execute(const ExecuteCb& cb) BSLS_KEYWORD_OVERRIDE;
    bdlmt::SignalerConnection onClose(const CloseFn& cb) BSLS_KEYWORD_OVERRIDE;
    bdlmt::SignalerConnection
    onWatermark(const WatermarkFn& cb) BSLS_KEYWORD_OVERRIDE;
    mwct::PropertyBag& properties() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    // Channel
    const bsl::string&       peerUri() const BSLS_KEYWORD_OVERRIDE;
    const mwct::PropertyBag& properties() const BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
