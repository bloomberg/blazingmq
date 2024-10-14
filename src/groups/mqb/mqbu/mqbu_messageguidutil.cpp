// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbu_messageguidutil.cpp                                           -*-C++-*-
#include <mqbu_messageguidutil.h>

#include <mqbscm_version.h>
/// Implementation Notes
///====================
//
/// MessageGUID layout [16 bytes]
///-----------------------------
//..
//   +---------------+---------------+---------------+---------------+
//   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
//   +---------------+---------------+---------------+---------------+
//   |GV |        Counter (22 bits)                  | TimerTick MSB |
//   +---------------+---------------+---------------+---------------+
//   |                     TimerTick (next 32 bits)                  |
//   +---------------+---------------+---------------+---------------+
//   |       TimerTick (lowest 24 bits)              |  BrokerId     |
//   +---------------+---------------+---------------+---------------+
//   |                           BrokerId                            |
//   +---------------+---------------+---------------+---------------+
//
//       GV...: Guid Version
//       MSB..: Most significant byte
//..
//
// BrokerID:
//   The brokerId is the first 5 bytes of the MD5 of {IP + timestamp + PID}.

// BMQ
#include <bmqio_resolveutil.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_bigendian.h>
#include <bdlb_bitmaskutil.h>
#include <bdlb_print.h>
#include <bdlde_md5.h>
#include <bdlma_localsequentialallocator.h>
#include <bdls_processutil.h>
#include <bsl_cstring.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_string.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_systemtime.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

// NTC
#include <ntsa_error.h>
#include <ntsa_ipv4address.h>

namespace BloombergLP {
namespace mqbu {

namespace {  // unnamed namespace
const int k_GUID_VERSION = 0;

// Note that guid version & counter are captured in 3 bytes.  Following
// constants (*_BITS and *_START_IDX) assume that layout.
const int k_GUID_VERSION_BITS      = 2;
const int k_GUID_VERSION_START_IDX = 22;
const int k_GUID_VERSION_MASK =
    bdlb::BitMaskUtil::one(k_GUID_VERSION_START_IDX, k_GUID_VERSION_BITS);

const int k_COUNTER_BITS      = 22;
const int k_COUNTER_START_IDX = 0;
const int k_COUNTER_MASK      = bdlb::BitMaskUtil::one(k_COUNTER_START_IDX,
                                                  k_COUNTER_BITS);

const int k_BROKER_ID_LEN_BINARY = 5;
const int k_BROKER_ID_LEN_HEX    = 2 * k_BROKER_ID_LEN_BINARY;

const int k_GUID_VERSION_AND_COUNTER_LEN = 3;

char g_brokerId[k_BROKER_ID_LEN_BINARY]     = {0};
char g_brokerIdHex[k_BROKER_ID_LEN_HEX + 1] = {0};
// NOTE: each character takes two char in hex, and we add a terminating
//       null character.

/// Initialize `g_counter` to -1, so that pre-incremented value starts from
/// zero.
bsls::AtomicInt g_counter(-1);

}  // close unnamed namespace

// ---------------------
// class MessageGUIDUtil
// ---------------------

// CLASS LEVEL METHODS
void MessageGUIDUtil::initialize()
{
    // NOTE: 'BE' suffix in variable name implies that variable's value is
    //       big-endian (network byte order).

    // PRECONDITIONS
    BSLS_ASSERT_OPT(g_counter == -1);

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
    error = bmqio::ResolveUtil::getIpAddress(&defaultIP, hostname);
    if (error.code() != ntsa::Error::e_OK) {
        BALL_LOG_ERROR << "Failed to get IP address of the host '" << hostname
                       << "' error: " << error;
        BSLS_ASSERT_OPT(false && "Failed to get IP address of the host");
        return;  // RETURN
    }

    // 'ntsa::Ipv4Address::value()' returns an unsigned int
    // in network byte order
    const bsl::uint32_t ipAddressBE = defaultIP.value();

    // Get timestamp (seconds from epoch)
    bdlb::BigEndianInt64 secondsBE = bdlb::BigEndianInt64::make(
        bsls::SystemTime::nowRealtimeClock().totalSeconds());

    // Note that above, we could use totalMilliseconds(), totalMicroseconds()
    // or totalNanoseconds() instead of totalSeconds() but they may exhibit
    // undefined behavior in certain conditions (see their contract).

    // Get PID
    bdlb::BigEndianInt32 pidBE = bdlb::BigEndianInt32::make(
        bdls::ProcessUtil::getProcessId());

    // Calculate md5(IP + timestamp + PID)
    bdlde::Md5            md5;
    bdlde::Md5::Md5Digest md5Buffer;
    md5.update(&ipAddressBE, sizeof(ipAddressBE));
    md5.update(&secondsBE, sizeof(secondsBE));
    md5.update(&pidBE, sizeof(pidBE));
    md5.loadDigest(&md5Buffer);

    // BrokerId == first 'k_BROKER_ID_LEN_BINARY' bytes of the md5 hash
    bsl::memcpy(g_brokerId, md5Buffer.buffer(), k_BROKER_ID_LEN_BINARY);

    // NOTE: since we know the size, the `defaultAllocator` will never be
    //       used here
    bdlma::LocalSequentialAllocator<k_BROKER_ID_LEN_HEX> localAllocator(0);
    bmqu::MemOutStream                                   os(&localAllocator);
    bdlb::Print::singleLineHexDump(os, g_brokerId, k_BROKER_ID_LEN_BINARY);
    bsl::memcpy(g_brokerIdHex, os.str().data(), os.str().length());

    BALL_LOG_INFO << "GUID generator initialized ["
                  << "currentTimerTick: " << bsls::TimeUtil::getTimer()
                  << ", IPAddress: " << defaultIP << ", secondsFromEpoch: "
                  << static_cast<bsls::Types::Int64>(secondsBE)
                  << ", pid: " << static_cast<int>(pidBE)
                  << ", brokerID: " << g_brokerIdHex << "]";
}

void MessageGUIDUtil::generateGUID(bmqt::MessageGUID* guid)
{
    // NOTE: we could keep track of last timer tick value and reset the counter
    //       to zero if timer's value is different from last value, but during
    //       testing some collisions were observed using that approach. So we
    //       just increment the counter and let it wrap over to zero.
    //
    // NOTE: 'BE' suffix in variable name implies that variable's value is
    //       big-endian (network byte order)

    // Get a snapshot of timer tick  and counter values
    bsls::Types::Int64 timerTick = bsls::TimeUtil::getTimer();
    unsigned int       counter   = static_cast<unsigned int>(++g_counter);

    // Below, we use our knowledge of internal memory layout of
    // bmqt::MessageGUID to populate its data member.  Alternatives are:
    //   o having setters in bmqt::MessageGUID, which is not ideal given that
    //     it should be completely opaque to clients.
    //   o using friendship
    char* buffer = reinterpret_cast<char*>(guid);

    // Populate GuidVersionAndCounter
    unsigned int versionAndCounter = (k_GUID_VERSION
                                      << k_GUID_VERSION_START_IDX) |
                                     (counter & k_COUNTER_MASK);

    BSLS_ASSERT_SAFE((versionAndCounter & 0xFF000000) == 0);  // MSB is zero

    bdlb::BigEndianUint32 versionAndCounterBE = bdlb::BigEndianUint32::make(
        versionAndCounter);

    // Copy 3 lower bytes of 'versionAndCounterBE'
    bsl::memcpy(
        buffer,
        (reinterpret_cast<char*>(&versionAndCounterBE) + 1),  // skip MSB
        k_GUID_VERSION_AND_COUNTER_LEN);

    // Populate TimerTick
    bdlb::BigEndianInt64 timerTickBE = bdlb::BigEndianInt64::make(timerTick);
    bsl::memcpy(buffer + k_GUID_VERSION_AND_COUNTER_LEN,
                &timerTickBE,
                sizeof(timerTickBE));

    // Populate BrokerId hash
    bsl::memcpy(buffer + k_GUID_VERSION_AND_COUNTER_LEN + sizeof(timerTickBE),
                g_brokerId,
                k_BROKER_ID_LEN_BINARY);
}

const char* MessageGUIDUtil::brokerIdHex()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(g_brokerIdHex[0] != 0 &&
                     "MessageGUIDUtil has not been Initialized");

    return g_brokerIdHex;
}

void MessageGUIDUtil::extractFields(int*                     version,
                                    unsigned int*            counter,
                                    bsls::Types::Int64*      timerTick,
                                    bsl::string*             brokerId,
                                    const bmqt::MessageGUID& guid)
{
    const char* buffer = reinterpret_cast<const char*>(&guid);

    // Version and counter
    bdlb::BigEndianUint32 versionAndCounterBE = bdlb::BigEndianUint32::make(0);
    bsl::memcpy(reinterpret_cast<char*>(&versionAndCounterBE) +
                    1,  // copy 3 LSB
                buffer,
                k_GUID_VERSION_AND_COUNTER_LEN);

    unsigned int versionAndCounter = static_cast<unsigned int>(
        versionAndCounterBE);

    *version = ((versionAndCounter & k_GUID_VERSION_MASK) >>
                k_GUID_VERSION_START_IDX);
    *counter = (versionAndCounter & k_COUNTER_MASK);

    // TimerTick
    bdlb::BigEndianInt64 timerTickBE;
    bsl::memcpy(&timerTickBE,
                buffer + k_GUID_VERSION_AND_COUNTER_LEN,
                sizeof(bdlb::BigEndianInt64));
    *timerTick = static_cast<bsls::Types::Int64>(timerTickBE);

    // BrokerId
    bdlma::LocalSequentialAllocator<k_BROKER_ID_LEN_HEX> localAlloc(0);
    bmqu::MemOutStream                                   os(&localAlloc);
    bdlb::Print::singleLineHexDump(os,
                                   buffer + k_GUID_VERSION_AND_COUNTER_LEN +
                                       sizeof(bdlb::BigEndianInt64),
                                   k_BROKER_ID_LEN_BINARY);
    brokerId->assign(os.str());
}

bsl::ostream& MessageGUIDUtil::print(bsl::ostream&            stream,
                                     const bmqt::MessageGUID& guid)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    if (guid.isUnset()) {
        stream << "** UNSET **";
        return stream;  // RETURN
    }

    int                version;
    unsigned int       counter;
    bsls::Types::Int64 timerTick;
    bsl::string        brokerId;

    extractFields(&version, &counter, &timerTick, &brokerId, guid);
    stream << "[version: " << version << ", "
           << "counter: " << counter << ", "
           << "timerTick: " << timerTick << ", "
           << "brokerId: " << brokerId << "]";

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
