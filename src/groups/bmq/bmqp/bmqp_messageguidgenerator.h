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

// bmqp_messageguidgenerator.h                                        -*-C++-*-
#ifndef INCLUDED_BMQP_MESSAGEGUIDGENERATOR
#define INCLUDED_BMQP_MESSAGEGUIDGENERATOR

//@PURPOSE: Provide a mechanism to generate bmqt::MessageGUIDs.
//
//@CLASSES:
//  bmqp::MessageGUIDGenerator : Generator class for bmqt::MessageGUID
//
//@SEE_ALSO:
// bmqt::MessageGUID
// mqbu::MessageGUIDUtil (for GUID version 0 generation)
//
//@DESCRIPTION: 'bmqp::MessageGUIDGenerator' provides a class to generate
// 'bmqt::MessageGUID' according to the layout defined in the below section.
//
//
/// MessageGUID layout [16 bytes]
///-----------------------------
//..
//   +---------------+---------------+---------------+---------------+
//   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
//   +---------------+---------------+---------------+---------------+
//   |GV |       Counter (22 bits)                   | TimerTick MSB |
//   +---------------+---------------+---------------+---------------+
//   |                     TimerTick (next 32 bits)                  |
//   +---------------+---------------+---------------+---------------+
//   |  TimerTick (lowest 16 bits)   |     ClientId MSBs (16 bits)   |
//   +---------------+---------------+---------------+---------------+
//   |                   ClientId (lowest 32 bits)                   |
//   +---------------+---------------+---------------+---------------+
//
//       GV...: Guid Version
//       MSB..: Most significant byte
//..
//
// ClientID:
//   The clientId is the 6 LSBs of the MD5 of
//   {IP + timestamp + PID + sessionId}.
//
/// MessageGUID format notes and limitation
///---------------------------------------
//: o !counter!: used to account for potential lack of resolution of the timer.
//:   With 22 bits, this scheme allows up to 2^22 - 1 = 4,194,303 unique GUID
//:   generation per timer clock resolution.
//: o !timerTick!: represents the number of nanoseconds since this
//:   'MessageGUIDGenerator' was instantiated.  In order to account for
//:   potentially low resolution timers on some platforms, the counter field is
//:   encoded on 22 bits, leaving only 56 bits for the timer; therefore this
//:   value does not contain the raw timer value, but is rather offsetted back
//:   to a '0-index'.  In other words we skip 1 MSB from the 8 bytes of the
//:   timer value.  With 56 bits, 2.28 years in nanoseconds can be represented;
//:   implying there could be hypothetical GUID collision if a producer stays
//:   up for that long and posts on a queue with a higher TTL.  Note that
//:   having this timer (as opposed to a single large monotonically increasing
//:   counter) allows to retrieve creation time of a given GUID, which is
//:   useful upon troubleshooting.
//
/// MessageGUID deciphering
///-----------------------
// See test case -1 of the associated unit test for extracting fields and
// retrieving absolute time value from a given GUID.

// BMQ

#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_messageguid.h>

// BDE
#include <bsl_iosfwd.h>
#include <bsl_string.h>
#include <bsls_atomic.h>
#include <bsls_cpp11.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bmqp {

// ==========================
// class MessageGUIDGenerator
// ==========================

/// Provides a mechanism to generate bmqt::MessageGUIDs.
class MessageGUIDGenerator {
  private:
    // PRIVATE CONSTANTS

    // This generator only deals with version 1 of the GUID layout.
    static const int k_GUID_VERSION = 1;

    static const int k_CLIENT_ID_LEN_BINARY = 6;

    // Number of bytes used to store the clientId in binary and hex format.
    static const int k_CLIENT_ID_LEN_HEX = 2 * k_CLIENT_ID_LEN_BINARY;

  private:
    // DATA
    char d_clientId[k_CLIENT_ID_LEN_BINARY];

    // NOTE: each character takes two char in hex, and we add a terminating
    //       null character.
    char d_clientIdHex[k_CLIENT_ID_LEN_HEX + 1];

    // Monotonically incrementing counter per every GUID generated.
    bsls::AtomicUint d_counter;

    // This can be used to retrieve the timestamp from the TimerTick part
    // of the GUID (see test -1 of this component).
    const bsls::Types::Int64 d_nanoSecondsFromEpoch;

    // The offset by which the timer is adjusted (see reason in
    // `MessageGUID format notes and limitation` section of the component
    // level documentation).
    const bsls::Types::Int64 d_timerBaseOffset;

  private:
    // NOT IMPLEMENTED
    MessageGUIDGenerator(const MessageGUIDGenerator&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator not implemented
    MessageGUIDGenerator&
    operator=(const MessageGUIDGenerator&) BSLS_CPP11_DELETED;

  public:
    // CREATORS

    /// Create a `bmqp::MessageGUIDGenerator` object and perform an
    /// initialization using the specified `sessionId`.  Attempt to resolve
    /// hostname ip address is done if `doIpResolving` flag is set.
    explicit MessageGUIDGenerator(int sessionId, bool doIpResolving = true);

    // MANIPULATORS

    /// Generate a new MessageGUID. This method can be called simultaneously
    /// from multiple threads.  Behavior is undefined unless specified
    /// `guid` is non-null.
    void generateGUID(bmqt::MessageGUID* guid);

    // ACCESSORS

    /// Return the hexadecimal representation of the unique id associated to
    /// this client (refer to the `MessageGUID layout` section in the
    /// component level documentation for details about how it is computed).
    const char* clientIdHex() const;

    /// Return the `guidInfo` populated with the attributes of this object.
    bmqp_ctrlmsg::GuidInfo guidInfo() const;

    /// --------------------------
    /// For testing/debugging only
    /// --------------------------

    /// Load into the specified `version`, `counter`, `timerTick` and
    /// `clientId` the corresponding fields from the specified `guid`. Note
    /// that `clientId` will be loaded in hex format.  Return 0 if the
    /// specified `guid` is version 1, otherwise load only `version` and
    /// return non-zero code.
    static int extractFields(int*                     version,
                             unsigned int*            counter,
                             bsls::Types::Int64*      timerTick,
                             bsl::string*             clientId,
                             const bmqt::MessageGUID& guid);

    /// Return a test MessageGUID.  This method can be called simultaneously
    /// from multiple threads.
    static bmqt::MessageGUID testGUID();

    /// Print internal details of the specified `guid` to the specified
    /// `stream` in a human-readable format if specified `guid` is version
    /// 1, otherwise print error.  This method is for testing/debugging
    /// only.
    static bsl::ostream& print(bsl::ostream&            stream,
                               const bmqt::MessageGUID& guid);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------
// class MessageGUIDGenerator
// --------------------------

// ACCESSORS
inline const char* MessageGUIDGenerator::clientIdHex() const
{
    return d_clientIdHex;
}

}  // close package namespace
}  // close enterprise namespace

#endif
