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

// bmqio_channelutil.cpp                                              -*-C++-*-
#include <bmqio_channelutil.h>

#include <bmqscm_version.h>

#include <bmqio_resolveutil.h>
#include <bmqu_blob.h>

// BDE
#include <ball_log.h>
#include <bdlb_bigendian.h>
#include <bdlb_string.h>
#include <bdlbb_blobutil.h>
#include <bslma_newdeleteallocator.h>
#include <bslmt_once.h>
#include <bsls_alignmentfromtype.h>
#include <bsls_alignmentutil.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>

// NTC
#include <ntsa_error.h>
#include <ntsa_ipaddress.h>

namespace BloombergLP {
namespace bmqio {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("BMQIO.CHANNELUTIL");

/// The minimum size (in bytes) of a packet.
static const int k_MINIMUM_PACKET_LENGTH = sizeof(bdlb::BigEndianUint32);

inline bool
isValidPacketLength(int*                      packetLength,
                    const bdlbb::Blob&        inBlob,
                    const bmqu::BlobPosition& pos = bmqu::BlobPosition())
{
    void* offsetByte = inBlob.buffer(pos.buffer()).data() + pos.byte();

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(
            (bmqu::BlobUtil::bufferSize(inBlob, pos.buffer()) - pos.byte()) >=
                k_MINIMUM_PACKET_LENGTH &&
            bsls::AlignmentUtil::calculateAlignmentOffset(
                offsetByte,
                bsls::AlignmentFromType<bdlb::BigEndianUint32>::VALUE) == 0)) {
        // All length bytes are in one contiguous buffer at a position that is
        // aligned, so we can directly read from it.
        const bdlb::BigEndianUint32* lengthBE =
            reinterpret_cast<const bdlb::BigEndianUint32*>(offsetByte);
        *packetLength = *lengthBE;
    }
    else {
        // (extremely) unlikely case, the packet length bytes are spanning
        // across multiple buffers; or the buffer's data is not aligned; we
        // need to memcpy.
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        bdlb::BigEndianUint32 lengthBE;
        bmqu::BlobUtil::copyToRawBufferFromIndex(
            reinterpret_cast<char*>(&lengthBE),
            inBlob,
            pos.buffer(),
            pos.byte(),
            sizeof(bdlb::BigEndianUint32));
        *packetLength = lengthBE;
    }

    // Sanity check
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(*packetLength <=
                                              k_MINIMUM_PACKET_LENGTH)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Fatal unrecoverable error, the packet's length is smaller than the
        // minimum size of a packet.
        return false;  // RETURN
    }

    return true;
}

}  // close unnamed namespace

// ------------------
// struct ChannelUtil
// ------------------

int ChannelUtil::handleRead(bdlbb::Blob* outPacket,
                            int*         numNeeded,
                            bdlbb::Blob* inBlob)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(outPacket->length() ==
                     0);  // The out blob should be empty

    // We need at least 'k_MINIMUM_PACKET_LENGTH' bytes in the blob, since
    // those first bytes are encoding the full length (in bytes) of the packet.
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(inBlob->length() <
                                              k_MINIMUM_PACKET_LENGTH)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        *numNeeded = k_MINIMUM_PACKET_LENGTH;
        return 0;  // RETURN
    }

    // At this point, the 'inBlob' has at least 'k_MINIMUM_PACKET_LEGNTH' bytes
    // in it, extract those bytes (representing the full length of the entire
    // packet).  More than likely, those will all be in the same first buffer
    // of the blob, and should be aligned.
    int packetLength = 0;  // The full length of the packet, as extracted from
                           // reading the first bytes of the 'inBlob'.

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !isValidPacketLength(&packetLength, *inBlob))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return -1;  // RETURN
    }

    // We need the full message
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(inBlob->length() <
                                              packetLength)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        *numNeeded = packetLength;
        return 0;  // RETURN
    }

    // We have a complete message, move the bytes out of 'inBlob' into
    // 'outPacket'.
    bdlbb::BlobUtil::append(outPacket, *inBlob, 0, packetLength);
    bdlbb::BlobUtil::erase(inBlob, 0, packetLength);

    // Schedule read of the next packet
    *numNeeded = k_MINIMUM_PACKET_LENGTH;

    return 0;
}

int ChannelUtil::handleRead(bsl::vector<bdlbb::Blob>* outPackets,
                            int*                      numNeeded,
                            bdlbb::Blob*              inBlob)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(outPackets);
    BSLS_ASSERT_SAFE(outPackets->empty());  // The out blobs should be empty

    int consumedBytes = 0;  // Offset in input blob
    int packetLength  = 0;  // The full length of the packet, as
                            // extracted from reading the first
                            // bytes of the packet in 'inBlob'
    bmqu::BlobPosition offset;

    // We need at least 'k_MINIMUM_PACKET_LENGTH' bytes in the blob, since
    // those first bytes are encoding the full length (in bytes) of the packet.
    while (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(
        (inBlob->length() - consumedBytes) >= k_MINIMUM_PACKET_LENGTH)) {
        // At this point, the 'inBlob' has at least 'k_MINIMUM_PACKET_LEGNTH'
        // bytes left to read, extract those bytes (representing the full
        // length of the entire packet).  More than likely, those will all be
        // in the same buffer of the blob, and should be aligned.
        const int rc =
            bmqu::BlobUtil::findOffset(&offset, *inBlob, offset, packetLength);
        BSLS_ASSERT_OPT(rc == 0);

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                !isValidPacketLength(&packetLength, *inBlob, offset))) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            // Erase the read bytes from the input blob and return error
            bdlbb::BlobUtil::erase(inBlob, 0, consumedBytes);
            return -1;  // RETURN
        }

        // We need the full message
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                (inBlob->length() - consumedBytes) < packetLength)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            // Erase the read bytes from the input blob and schedule read of
            // the next packet
            bdlbb::BlobUtil::erase(inBlob, 0, consumedBytes);
            *numNeeded = packetLength;
            return 0;  // RETURN
        }

        // We have a complete message.  Move the bytes out of 'inBlob' into a
        // standalone packet in 'outPackets'.
        outPackets->emplace_back();
        bdlbb::BlobUtil::append(&outPackets->back(),
                                *inBlob,
                                consumedBytes,
                                packetLength);

        consumedBytes += packetLength;
    }

    // Erase the read bytes from the input blob
    bdlbb::BlobUtil::erase(inBlob, 0, consumedBytes);

    // Schedule read of the next packet
    *numNeeded = k_MINIMUM_PACKET_LENGTH;

    return 0;
}

bool ChannelUtil::isLocalHost(const bsl::string_view& host)
{
    static bsl::uint32_t s_localIpAddress;  // IPAddress of the localHost,
                                            // primary network

    BSLMT_ONCE_DO
    {
        // Resolve the default address of this host
        bsl::string hostname;
        if (bmqio::ResolveUtil::getHostname(&hostname).code() !=
            ntsa::Error::e_OK) {
            BSLS_ASSERT_OPT(false && "Failed to get local host name");
            return false;  // RETURN
        }

        ntsa::Ipv4Address defaultIP;
        if (bmqio::ResolveUtil::getIpAddress(&defaultIP, hostname).code() !=
            ntsa::Error::e_OK) {
            BSLS_ASSERT_OPT(false && "Failed to get IP address of the host.");
            return false;  // RETURN
        }

        s_localIpAddress = defaultIP.value();
    }

    if (bdlb::String::areEqualCaseless(bsl::string(host), "localhost")) {
        return true;  // RETURN
    }

    ntsa::Ipv4Address ipAddress;
    ntsa::Error error = bmqio::ResolveUtil::getIpAddress(&ipAddress, host);
    if (error.code() != ntsa::Error::e_OK) {
        BALL_LOG_WARN << "#TCP_IPRESOLUTION_FAILURE "
                      << "Failed resolving ipAddress for '" << host
                      << "', error: " << error;
        return false;  // RETURN
    }

    return (s_localIpAddress == ipAddress.value());
}

bool ChannelUtil::isLocalHost(const ntsa::IpAddress& ip)
{
    static bsl::vector<ntsa::IpAddress>* s_localAddresses_p = 0;

    BSLMT_ONCE_DO
    {
        static bsl::vector<ntsa::IpAddress>& s_localAddresses = *(
            new bsl::vector<ntsa::IpAddress>(
                &bslma::NewDeleteAllocator::singleton()));
        // Heap allocate it to prevent 'exit-time-destructor needed' compiler
        // warning.  Causes valgrind-reported memory leak.
        // Pass 'bslma::NewDeleteAllocator' in order to prevent using of
        // default allocator.

        ntsa::Error error = bmqio::ResolveUtil::getLocalIpAddress(
            &s_localAddresses);
        BSLS_ASSERT_OPT(error.code() == ntsa::Error::e_OK &&
                        "Unable to obtain local addresses");

        s_localAddresses_p = &s_localAddresses;
    }

    bsl::vector<ntsa::IpAddress>::const_iterator it;
    for (it = s_localAddresses_p->begin(); it != s_localAddresses_p->end();
         ++it) {
        if (it->equals(ip)) {
            return true;  // RETURN
        }
    }

    return false;
}

}  // close package namespace
}  // close enterprise namespace
