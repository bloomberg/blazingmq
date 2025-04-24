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
#include <bsl_optional.h>

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
    typedef bsl::shared_ptr<mqbnet::InitialConnectionContext>
        InitialConnectionContextSp;

  private:
    // DATA

    /// Negotiator to use for converting a Channel to a Session
    bslma::ManagedPtr<mqbnet::Negotiator> d_negotiator_mp;

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
    void readCallback(const bmqio::Status&              status,
                      int*                              numNeeded,
                      bdlbb::Blob*                      blob,
                      const InitialConnectionContextSp& context);

    int readBlob(bsl::ostream&        errorDescription,
                 bdlbb::Blob*         outPacket,
                 bool*                isFullBlob,
                 const bmqio::Status& status,
                 int*                 numNeeded,
                 bdlbb::Blob*         blob);

    int processBlob(bsl::ostream&                     errorDescription,
                    bsl::shared_ptr<mqbnet::Session>* session,
                    bool*                             isContinueRead,
                    const bdlbb::Blob&                blob,
                    const InitialConnectionContextSp& context);

    /// Decode the initial connection messages received in the specified
    /// `blob` and store it, on success, in the specified optional
    /// `negotiationMsg`, returning 0.  Return a non-zero code on error and
    /// populate the specified `errorDescription` with a description of the
    /// error.
    int decodeInitialConnectionMessage(
        bsl::ostream&                                    errorDescription,
        const bdlbb::Blob&                               blob,
        bsl::optional<bmqp_ctrlmsg::NegotiationMessage>* negotiationMsg);

    /// Schedule a read for the initial connection of the session of the
    /// specified `context`.  Return a non-zero code on error and
    /// populate the specified `errorDescription` with a description of the
    /// error.
    int scheduleRead(bsl::ostream&                     errorDescription,
                     const InitialConnectionContextSp& context);

    /// Call the `InitialConnectionCompleteCb` with the specified `context`,
    /// return code `rc`, and `error` string to indicate the completion of
    /// negotiation.
    static void complete(const InitialConnectionContextSp&       context,
                         const int                               rc,
                         const bsl::string&                      error,
                         const bsl::shared_ptr<mqbnet::Session>& session);

  public:
    // CREATORS

    InitialConnectionHandler(bslma::ManagedPtr<mqbnet::Negotiator>& negotiator,
                             bslma::Allocator*                      allocator);

    /// Destructor
    ~InitialConnectionHandler() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Method invoked by the client of this object to negotiate a session.
    /// The specified `context` is an in-out member holding the initial
    /// connection context to use, including an `InitialConnectionCompleteCb`,
    /// which must be called with the result, whether success or failure, of
    /// the initial connection.
    /// The InitialConnectionHandler concrete implementation can modify some of
    /// the members during the initial connection (i.e., between the
    /// `handleInitialConnection()` method and the invocation of the
    /// `InitialConnectionCompleteCb` method.  Note that if no initial
    /// connection is needed, the `InitialConnectionCompleteCb` may be invoked
    /// directly from inside the call to `handleInitialConnection()`.
    void handleInitialConnection(const InitialConnectionContextSp& context)
        BSLS_KEYWORD_OVERRIDE;
};
}
}

#endif
