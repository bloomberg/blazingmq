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

// bmqio_channelutil.h                                                -*-C++-*-
#ifndef INCLUDED_BMQIO_CHANNELUTIL
#define INCLUDED_BMQIO_CHANNELUTIL

//@PURPOSE: Provide utility functions related to channel manipulation.
//
//@CLASSES:
//  bmqio::ChannelUtil: namespace for channel manipulation utility  functions.
//
//@DESCRIPTION: 'bmqio::ChannelUtil' provides a utility namespace for channel
// manipulation functions.

// BDE
#include <bdlbb_blob.h>
#include <bsl_string.h>
#include <bsl_vector.h>

// NTC
#include <ntsa_ipaddress.h>

namespace BloombergLP {
namespace bmqio {

// ==================
// struct ChannelUtil
// ==================

/// Utility namespace for channel manipulation.
struct ChannelUtil {
  public:
    // CLASS METHODS

    /// Handle a `BlobBasedReadCallback` call from a `btemt_AsyncChannel`
    /// with the specified `numNeeded` and `inBlob`.  Assuming the first 4
    /// bytes of a message are a big endian length, update `numNeeded` and
    /// `inBlob`, and place the entire message in the specified `outPacket`
    /// if a whole message has been read.  Return `0` on success or `-1` if
    /// a fatal error occurred and the channel is in an unrecoverable state
    /// and should be closed.
    static int
    handleRead(bdlbb::Blob* outPacket, int* numNeeded, bdlbb::Blob* inBlob);

    /// Handle a `BlobBasedReadCallback` call from a `btemt_AsyncChannel`
    /// with the specified `numNeeded` and `inBlob`.  Assuming the first 4
    /// bytes of a message are a big endian length, update `numNeeded` and
    /// `inBlob`, and read as many complete packets from `inBlob` as
    /// possible and load them into the specified `outPackets`.  Return `0`
    /// on success or `-1` if a fatal error occurred and the channel is in
    /// an unrecoverable state and should be closed.
    static int handleRead(bsl::vector<bdlbb::Blob>* outPackets,
                          int*                      numNeeded,
                          bdlbb::Blob*              inBlob);

    /// Return true if the specified `host` corresponds to this host,
    /// whether because the hostname is `localhost`, or its resolved IP
    /// correspond to the primary IP of the machine.
    static bool isLocalHost(const bsl::string_view& host);

    /// Return true if the specified `ip` corresponds to one of the local
    /// IPs of this host.
    static bool isLocalHost(const ntsa::IpAddress& ip);
};

}  // close package namespace
}  // close enterprise namespace

#endif
