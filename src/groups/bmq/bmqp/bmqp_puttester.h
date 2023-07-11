// Copyright 2017-2023 Bloomberg Finance L.P.
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

// bmqp_puttester.h                                                   -*-C++-*-
#ifndef INCLUDED_BMQP_PUTTESTER
#define INCLUDED_BMQP_PUTTESTER

//@PURPOSE: Provide utilities for testing BlazingMQ 'PUT' events.
//
//@CLASSES:
//  bmqp::PutTester: Utilities for testing BlazingMQ 'PUT' events.
//
//@DESCRIPTION: 'bmqp::PutTester' provides a set of utility methods to be used
// to setup 'PUT' event tests.
//
/// Thread Safety
///-------------
// Thread safe.

// BMQ

#include <bmqp_messageproperties.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqt_messageguid.h>

// MWC
#include <mwcu_blob.h>

// BDE
#include <bdlb_nullablevalue.h>
#include <bdlb_random.h>
#include <bdlbb_blob.h>
#include <bsl_cstddef.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

namespace bmqp {

// ================
// struct PutTester
// ================

/// Struct representing attributes of a PutMessage
struct PutTester {
    // Utilities for testing BlazingMQ 'PUT' events.
    typedef bdlb::NullableValue<bmqp::Protocol::MsgGroupId> NullableMsgGroupId;

    struct Data {
        // DATA
        bdlbb::Blob d_payload;

        bdlbb::Blob d_properties;

        bdlbb::Blob d_appData;

        NullableMsgGroupId d_msgGroupId;

        int d_queueId;

        int d_corrId;

        int d_msgLen;

        int d_propLen;

        bdlbb::Blob d_compressedAppData;

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(Data, bslma::UsesBslmaAllocator)

        // CREATORS
        Data(bdlbb::BlobBufferFactory* bufferFactory,
             bslma::Allocator*         allocator);

        Data(const Data& other, bslma::Allocator* allocator);
    };

    // CLASS METHODS

    /// PushEvent Utilities
    ///-------------------

    /// Populate specified `blob` with a `PUT` event which has specified
    /// `numMsgs` `PUT` messages, update specified `eh` with corresponding
    /// EventHeader, and update specified `data` with the expected values.
    static void populateBlob(bdlbb::Blob*              blob,
                             bmqp::EventHeader*        eh,
                             bsl::vector<Data>*        vec,
                             size_t                    numMsgs,
                             bdlbb::BlobBufferFactory* bufferFactory,
                             bool                      zeroLengthMsgs,
                             bslma::Allocator*         allocator,
                             bmqt::CompressionAlgorithmType::Enum cat =
                                 bmqt::CompressionAlgorithmType::e_NONE);

    /// Populate specified `blob` with a `PUT` event which contains one
    /// `PUT` message for specified `queueId` with specified `messageGUID`
    /// having ACK_REQUESTED flag set.  The message payload is compressed
    /// using specified `cat` compression algorithm type and `bufferFactory`
    /// to supply data buffers if needed.  Also update specified `eh` with
    /// corresponding EventHeader, specified `eb` and `ebLen` with the
    /// payload and its length, specified `headerPosition` and
    /// `payloadPosition` with corresponding positions of the PutHeader and
    /// the payload within the `blob`.
    ///
    /// Payload is 36 bytes.  Per BlazingMQ protocol, it will require 4
    /// bytes of padding (ie 1 word).
    ///
    /// TODO: once all clients move to SDK version which generates GUIDs for
    /// PUTs, this method should be removed.
    static void populateBlob(bdlbb::Blob*             blob,
                             bmqp::EventHeader*       eh,
                             bdlbb::Blob*             eb,
                             int*                     ebLen,
                             mwcu::BlobPosition*      headerPosition,
                             mwcu::BlobPosition*      payloadPosition,
                             int                      queueId,
                             const bmqt::MessageGUID& messageGUID,
                             bmqt::CompressionAlgorithmType::Enum cat =
                                 bmqt::CompressionAlgorithmType::e_NONE,
                             bdlbb::BlobBufferFactory* bufferFactory = 0,
                             bslma::Allocator*         allocator     = 0);

    /// Populate specified `blob` with a `PUT` event which contains one
    /// `PUT` message for specified `queueId` with specified `messageGUID`
    /// (or `correlationId` if the `messageGUID` is unset) having
    /// ACK_REQUESTED flag set if specified `isAckRequested` is set to true.
    /// The message payload is compressed using specified `cat` compression
    /// algorithm type and `bufferFactory` to supply data buffers if needed.
    /// Also update specified `eh` with corresponding EventHeader,
    /// specified `eb` and `ebLen` with the payload and its length,
    /// specified `headerPosition` and `payloadPosition` with
    /// corresponding positions of the PutHeader and the payload within
    /// the `blob`.
    ///
    /// Payload is 36 bytes.  Per BlazingMQ protocol, it will require 4
    /// bytes of padding (ie 1 word).
    ///
    /// TODO: once all clients move to SDK version which generates GUIDs for
    /// PUTs, `correlationId` parameter should be removed.
    static void populateBlob(bdlbb::Blob*              blob,
                             bmqp::EventHeader*        eh,
                             bmqp::PutHeader*          ph,
                             int                       length,
                             int                       queueId,
                             const bmqt::MessageGUID&  messageGUID,
                             int                       correlationId,
                             const MessageProperties&  properties,
                             bool                      isAckRequested,
                             bdlbb::BlobBufferFactory* bufferFactory,
                             bmqt::CompressionAlgorithmType::Enum cat,
                             bslma::Allocator* allocator = 0);

    /// Append at least `atLeastLen` bytes to the specified `blob`
    static void populateBlob(bdlbb::Blob* blob, int atLeastLen);
};

}  // close package namespace
}  // close enterprise namespace

#endif
