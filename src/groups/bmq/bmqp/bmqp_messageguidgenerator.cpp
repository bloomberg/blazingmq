// Copyright 2021-2023 Bloomberg Finance L.P.
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

// bmqp_messageguidgenerator.cpp                                      -*-C++-*-
#include <bmqp_messageguidgenerator.h>

#include <bmqscm_version.h>

#include <bmqio_resolveutil.h>
#include <bmqu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <bdlb_bigendian.h>
#include <bdlb_bitmaskutil.h>
#include <bdlb_print.h>
#include <bdlde_md5.h>
#include <bdlma_localsequentialallocator.h>
#include <bdls_processutil.h>
#include <bsl_cstring.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bsls_assert.h>
#include <bsls_systemtime.h>
#include <bsls_timeutil.h>

// NTC
#include <ntsa_error.h>
#include <ntsa_ipaddress.h>

namespace BloombergLP {
namespace bmqp {

BALL_LOG_SET_NAMESPACE_CATEGORY("BMQP.MESSAGEGUIDGENERATOR");

namespace {

// The following constants represent the position and size of the version and
// counter fields in the first word (4 bytes).
static const int k_GUID_VERSION_BITS      = 2;
static const int k_GUID_VERSION_START_IDX = 30;
static const int k_GUID_VERSION_MASK =
    bdlb::BitMaskUtil::one(k_GUID_VERSION_START_IDX, k_GUID_VERSION_BITS);

static const int k_COUNTER_BITS      = 22;
static const int k_COUNTER_START_IDX = 8;
static const int k_COUNTER_MASK = bdlb::BitMaskUtil::one(k_COUNTER_START_IDX,
                                                         k_COUNTER_BITS);

const int k_GUID_VERSION_AND_COUNTER_BYTES = 3;

/// Number of bytes used to encode those various fields.
const int k_TIMERTICK_BYTES = 7;

}  // close unnamed namespace

// --------------------------
// class MessageGUIDGenerator
// --------------------------

// CREATORS
MessageGUIDGenerator::MessageGUIDGenerator(int sessionId, bool doIpResolving)
: d_clientId()     // init array with zeros
, d_clientIdHex()  // init array with zeros
, d_counter(-1)    // Initialize 'd_counter' to -1, so that pre-incremented
                   // value starts  from zero.
, d_nanoSecondsFromEpoch(
      bsls::SystemTime::nowRealtimeClock().totalNanoseconds())
, d_timerBaseOffset(bsls::TimeUtil::getTimer())
{
    // NOTE: 'BE' suffix in variable name implies that variable's value is
    //       big-endian (network byte order).

    // Get hostname
    bsl::string hostname;
    ntsa::Error error = bmqio::ResolveUtil::getHostname(&hostname);

    if (error.code() != ntsa::Error::e_OK) {
        BALL_LOG_ERROR << "Failed to get local hostname, error: " << error;
        BSLS_ASSERT_OPT(false && "Failed to get local hostname");
        return;  // RETURN
    }

    // Get IP
    ntsa::Ipv4Address defaultIP;
    bool              useIP = doIpResolving;

    if (doIpResolving) {
        error = bmqio::ResolveUtil::getIpAddress(&defaultIP, hostname);

        if (error.code() != ntsa::Error::e_OK) {
            // IP address retrieval can fail in case process is running in a
            // dpkg chroot jail etc, where /etc/hosts, /etc/resolv.conf, etc
            // may not be present.  We fall back to using hostname in this
            // scenario.
            BALL_LOG_ERROR << "Failed to get IP address of local host '"
                           << hostname << "' error: " << error
                           << ". Using hostname instead of IP address to "
                           << "generate client ID.";
            useIP = false;
        }
    }

    // Get timestamp (seconds from epoch)
    const bdlb::BigEndianInt64 secondsBE = bdlb::BigEndianInt64::make(
        bsls::SystemTime::nowRealtimeClock().totalSeconds());

    // Get PID
    const bdlb::BigEndianInt32 pidBE = bdlb::BigEndianInt32::make(
        bdls::ProcessUtil::getProcessId());

    // Get sessionId
    const bdlb::BigEndianInt32 sessionIdBE = bdlb::BigEndianInt32::make(
        sessionId);

    // Calculate md5(HostId + timestamp + PID + sessionId)
    bdlde::Md5            md5;
    bdlde::Md5::Md5Digest md5Buffer;
    if (useIP) {
        // 'ntsa::Ipv4Address::value()' returns an unsigned int
        // in network byte order
        const bsl::uint32_t ipAddressBE = defaultIP.value();
        md5.update(&ipAddressBE, sizeof(ipAddressBE));
    }
    else {
        md5.update(hostname.c_str(), hostname.size());
    }
    md5.update(&secondsBE, sizeof(secondsBE));
    md5.update(&pidBE, sizeof(pidBE));
    md5.update(&sessionIdBE, sizeof(sessionIdBE));
    md5.loadDigest(&md5Buffer);

    // ClientId == first 'k_CLIENT_ID_LEN_BINARY' bytes of the md5 hash
    bsl::memcpy(d_clientId, md5Buffer.buffer(), k_CLIENT_ID_LEN_BINARY);

    // NOTE: since we know the size, the `defaultAllocator` will never be
    //       used here
    bdlma::LocalSequentialAllocator<k_CLIENT_ID_LEN_HEX> localAllocator(0);
    bmqu::MemOutStream                                   os(&localAllocator);
    bdlb::Print::singleLineHexDump(os, d_clientId, k_CLIENT_ID_LEN_BINARY);
    bsl::memcpy(d_clientIdHex, os.str().data(), os.str().length());

    BALL_LOG_INFO << "GUID generator initialized ["
                  << "IPAddress: " << defaultIP << ", hostname: '" << hostname
                  << "', used IPAddress " << bsl::boolalpha << useIP
                  << ", nanoSecondsFromEpoch: " << d_nanoSecondsFromEpoch
                  << ", pid: " << static_cast<int>(pidBE)
                  << ", sessionId: " << sessionId
                  << ", clientID: " << d_clientIdHex << "]";
}

void MessageGUIDGenerator::generateGUID(bmqt::MessageGUID* guid)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(guid);

    // NOTE: 'BE' suffix in variable name implies that variable's value is
    //       big-endian (network byte order)

    // Get a snapshot of timer tick and counter values
    const bsls::Types::Int64 timerTickDiff = bsls::TimeUtil::getTimer() -
                                             d_timerBaseOffset;
    const unsigned int counter = ++d_counter;

    // Below, we use our knowledge of internal memory layout of
    // bmqt::MessageGUID to populate its data member.  Alternatives are:
    //: o having setters in bmqt::MessageGUID, which is not ideal given that it
    //:   should be completely opaque to clients.
    //: o using friendship
    char* buffer = reinterpret_cast<char*>(guid);

    // Populate GUIDVersionAndCounter
    const unsigned int versionAndCounter = (k_GUID_VERSION
                                            << k_GUID_VERSION_START_IDX) |
                                           ((counter << k_COUNTER_START_IDX) &
                                            k_COUNTER_MASK);

    const bdlb::BigEndianUint32 versionAndCounterBE =
        bdlb::BigEndianUint32::make(versionAndCounter);
    // Populate VersionAndCounter (MSB 3 bytes from versionAndCounterBE)
    bsl::memcpy(buffer,
                reinterpret_cast<const char*>(&versionAndCounterBE),
                k_GUID_VERSION_AND_COUNTER_BYTES);
    buffer += k_GUID_VERSION_AND_COUNTER_BYTES;

    // Populate TimerTick
    const bdlb::BigEndianInt64 timerTickDiffBE = bdlb::BigEndianInt64::make(
        timerTickDiff);
    bsl::memcpy(buffer,
                reinterpret_cast<const char*>(&timerTickDiffBE) +
                    1,  // skip MSB
                k_TIMERTICK_BYTES);
    buffer += k_TIMERTICK_BYTES;

    // Populate ClientId hash
    bsl::memcpy(buffer, d_clientId, k_CLIENT_ID_LEN_BINARY);
}

int MessageGUIDGenerator::extractFields(int*                     version,
                                        unsigned int*            counter,
                                        bsls::Types::Int64*      timerTick,
                                        bsl::string*             clientId,
                                        const bmqt::MessageGUID& guid)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(version);
    BSLS_ASSERT_SAFE(counter);
    BSLS_ASSERT_SAFE(timerTick);
    BSLS_ASSERT_SAFE(clientId);

    const char* buffer = reinterpret_cast<const char*>(&guid);

    // Version and counter
    bdlb::BigEndianUint32 versionAndCounterBE = bdlb::BigEndianUint32::make(0);
    bsl::memcpy(reinterpret_cast<char*>(&versionAndCounterBE),
                buffer,
                sizeof(versionAndCounterBE));
    buffer += k_GUID_VERSION_AND_COUNTER_BYTES;
    // NOTE: We read the full 4 bytes, because we'll do the bit shifting to
    //       extract the fields, but only advance the buffer by 3 bytes.

    const unsigned int versionAndCounter = static_cast<unsigned int>(
        versionAndCounterBE);

    *version = ((versionAndCounter & k_GUID_VERSION_MASK) >>
                k_GUID_VERSION_START_IDX);

    if (*version != k_GUID_VERSION) {
        return -1;  // RETURN
    }

    *counter = (versionAndCounter & k_COUNTER_MASK) >> k_COUNTER_START_IDX;

    // TimerTick
    bdlb::BigEndianInt64 timerTickBE = bdlb::BigEndianInt64::make(0);
    bsl::memcpy(reinterpret_cast<char*>(&timerTickBE) + 1,  // skip MSB
                buffer,
                k_TIMERTICK_BYTES);
    *timerTick = static_cast<bsls::Types::Int64>(timerTickBE);
    buffer += k_TIMERTICK_BYTES;

    // ClientId
    bdlma::LocalSequentialAllocator<k_CLIENT_ID_LEN_HEX> localAlloc(0);
    bmqu::MemOutStream                                   os(&localAlloc);
    bdlb::Print::singleLineHexDump(os, buffer, k_CLIENT_ID_LEN_BINARY);
    clientId->assign(os.str());

    return 0;  // RETURN
}

bmqt::MessageGUID MessageGUIDGenerator::testGUID()
{
    static bsls::AtomicUint s_counter(0);

    bmqt::MessageGUID  guid;
    const unsigned int counter = ++s_counter;
    char*              buffer  = reinterpret_cast<char*>(&guid);

    // Populate GUIDVersion
    const unsigned int version = (k_GUID_VERSION << k_GUID_VERSION_START_IDX);
    const bdlb::BigEndianUint32 versionBE = bdlb::BigEndianUint32::make(
        version);

    bsl::memcpy(buffer,
                reinterpret_cast<const char*>(&versionBE),
                k_GUID_VERSION_AND_COUNTER_BYTES);

    buffer += k_GUID_VERSION_AND_COUNTER_BYTES + k_TIMERTICK_BYTES + 2;

    const bdlb::BigEndianUint32 counterBE = bdlb::BigEndianUint32::make(
        counter);

    bsl::memcpy(buffer,
                reinterpret_cast<const char*>(&counterBE),
                k_CLIENT_ID_LEN_BINARY - 2);

    return guid;
}

bsl::ostream& MessageGUIDGenerator::print(bsl::ostream&            stream,
                                          const bmqt::MessageGUID& guid)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    if (guid.isUnset()) {
        stream << "** UNSET **";
        return stream;  // RETURN
    }

    bdlma::LocalSequentialAllocator<k_CLIENT_ID_LEN_HEX> localAlloc(0);

    int                version;
    unsigned int       counter;
    bsls::Types::Int64 timerTick;
    bsl::string        clientId(&localAlloc);

    int rc = extractFields(&version, &counter, &timerTick, &clientId, guid);

    if (rc != 0) {
        // Not version 1?
        stream << "[Unsupported GUID version " << version << "]";
        return stream;  // RETURN
    }

    stream << version << "-" << counter << "-" << timerTick << "-" << clientId;

    return stream;
}

bmqp_ctrlmsg::GuidInfo MessageGUIDGenerator::guidInfo() const
{
    bmqp_ctrlmsg::GuidInfo ret;

    ret.clientId()             = d_clientIdHex;
    ret.nanoSecondsFromEpoch() = d_nanoSecondsFromEpoch;

    return ret;
}

}  // close package namespace
}  // close enterprise namespace
