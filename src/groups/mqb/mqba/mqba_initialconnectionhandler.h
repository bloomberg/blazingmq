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

// mqba_initialconnectionhandler.h                          -*-C++-*-
#ifndef INCLUDED_MQBA_INITIALCONNECTIONHANDLER
#define INCLUDED_MQBA_INITIALCONNECTIONHANDLER

#include <mqbnet_initialconnectionhandler.h>

// MQB
#include <mqbnet_initialconnectioncontext.h>
#include <mqbnet_negotiator.h>

// BMQ
#include <bmqio_status.h>
#include <bmqma_countingallocatorstore.h>
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlmt_fixedthreadpool.h>

namespace BloombergLP {

namespace mqba {

// FORWARD DECLARATION
class SessionNegotiator;

// ==============================
// class InitialConnectionHandler
// ==============================

class InitialConnectionHandler : public mqbnet::InitialConnectionHandler {
  private:
    // PRIVATE TYPES
    typedef bsl::shared_ptr<mqbnet::NegotiationContext> NegotiationContextSp;

  private:
    // DATA

    /// Negotiator to use for converting a Channel to a Session
    bslma::ManagedPtr<mqbnet::Negotiator> d_negotiator_mp;

    bdlmt::FixedThreadPool d_threadPool;

    /// Allocator store to spawn new allocators for sub components.
    bmqma::CountingAllocatorStore d_allocators;

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator not implemented.
    InitialConnectionHandler(const InitialConnectionHandler&);  // = delete
    InitialConnectionHandler&
    operator=(const InitialConnectionHandler&);  // = delete

  private:
    // PRIVATE MANIPULATORS

    /// Read callback method invoked when receiving data in the specified
    /// `blob`, if the specified `status` indicates success.  The specified
    /// `numNeeded` can be used to indicate if more bytes are needed in
    /// order to get a full message.  The specified `context` holds the
    /// initial connection context associated to this read.
    void readCallback(const bmqio::Status&        status,
                      int*                        numNeeded,
                      bdlbb::Blob*                blob,
                      const NegotiationContextSp& context);

    /// Decode the initial connection messages received in the specified `blob`
    /// and store it, on success, in the corresponding member of the specified
    /// `context`, returning 0.  Return a non-zero code on error and
    /// populate the specified `errorDescription` with a description of the
    /// error.
    int decodeInitialConnectionMessage(bsl::ostream& errorDescription,
                                       const NegotiationContextSp& context,
                                       bdlbb::Blob&                blob);

    /// Schedule a read for the initial connection of the session of the
    /// specified `context`.
    void scheduleRead(const NegotiationContextSp& context);

  public:
    // CREATORS

    InitialConnectionHandler(bslma::ManagedPtr<mqbnet::Negotiator>& negotiator,
                             bslma::Allocator*                      allocator);

    /// Destructor
    ~InitialConnectionHandler() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    void handleInitialConnection(
        const InitialConnectionContextSp&      context,
        const bsl::shared_ptr<bmqio::Channel>& channel,
        const mqbnet::InitialConnectionContext::InitialConnectionCompleteCb&
            initialConnectionCompleteCb) BSLS_KEYWORD_OVERRIDE;
};
}
}

#endif
