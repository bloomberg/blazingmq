// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqt_messageguid.cpp                                               -*-C++-*-
#include <bmqt_messageguid.h>

#include <bmqscm_version.h>
// BDE
#include <bdlb_print.h>
#include <bsl_cstring.h>
#include <bsl_ostream.h>
#include <bslmf_assert.h>
#include <bsls_alignmentfromtype.h>

namespace BloombergLP {
namespace bmqt {

namespace {
BSLMF_ASSERT(16 == MessageGUID::e_SIZE_BINARY);
BSLMF_ASSERT(sizeof(MessageGUID) == MessageGUID::e_SIZE_BINARY);
// Compile-time assertion of size of MessageGUID (critical it doesn't
// change).  If its size changes, 'bmqt::MessageGUIDHashAlgo' must also be
// revisited since it assumes size of '16'.

BSLMF_ASSERT(0 == sizeof(MessageGUID) % 4);
// Compile-time assertion to ensure size of MessageGUID is a multiple of 4
// (Some structs in bmqp::Protocol rely on it)

BSLMF_ASSERT(1 == bsls::AlignmentFromType<MessageGUID>::VALUE);
// Compile-time assertion of alignment of MessageGUID

const char k_HEX_INT_TABLE[24] = {0,  1,  2,  3,  4,  5,  6,  7,
                                  8,  9,  99, 99, 99, 99, 99, 99,
                                  99, 10, 11, 12, 13, 14, 15, 99};
// Conversion table used to convert a hexadecimal value to it's int
// representation. (99 is because in the ASCII table, '9' is 57 and 'A' is
// 65, so the 99 represents unexpected invalid value in the input).

const char k_INT_HEX_TABLE[16] = {'0',
                                  '1',
                                  '2',
                                  '3',
                                  '4',
                                  '5',
                                  '6',
                                  '7',
                                  '8',
                                  '9',
                                  'A',
                                  'B',
                                  'C',
                                  'D',
                                  'E',
                                  'F'};
// Conversion table used to convert an int number to its hexadecimal
// representation.
}  // close unnamed namespace

// -----------------
// class MessageGUID
// -----------------

const char MessageGUID::k_UNSET_GUID[e_SIZE_BINARY] = {0};

// CLASS LEVEL METHODS
bool MessageGUID::isValidHexRepresentation(const char* buffer)
{
    for (int i = 0; i < MessageGUID::e_SIZE_HEX; ++i) {
        if (!(buffer[i] >= '0' && buffer[i] <= '9') &&
            !(buffer[i] >= 'A' && buffer[i] <= 'F')) {
            return false;  // RETURN
        }
    }

    return true;
}

// MANIPULATORS
MessageGUID& MessageGUID::fromBinary(const unsigned char* buffer)
{
    bsl::memcpy(d_buffer, buffer, MessageGUID::e_SIZE_BINARY);
    return *this;
}

MessageGUID& MessageGUID::fromHex(const char* buffer)
{
    for (int i = 0; i < MessageGUID::e_SIZE_BINARY; ++i) {
        const int index = 2 * i;
        const int ch1   = buffer[index + 0] - '0';
        const int ch2   = buffer[index + 1] - '0';

        d_buffer[i] = (k_HEX_INT_TABLE[ch1] << 4) | (k_HEX_INT_TABLE[ch2]);
    }

    return *this;
}

// ACCESSORS
void MessageGUID::toBinary(unsigned char* destination) const
{
    bsl::memcpy(destination, d_buffer, MessageGUID::e_SIZE_BINARY);
}

void MessageGUID::toHex(char* destination) const
{
    for (int i = 0; i < MessageGUID::e_SIZE_BINARY; i += 2) {
        const int           index = 2 * i;
        const unsigned char ch1   = d_buffer[i + 0];
        const unsigned char ch2   = d_buffer[i + 1];

        destination[index + 0] = k_INT_HEX_TABLE[ch1 >> 4];
        destination[index + 1] = k_INT_HEX_TABLE[ch1 & 0xF];
        destination[index + 2] = k_INT_HEX_TABLE[ch2 >> 4];
        destination[index + 3] = k_INT_HEX_TABLE[ch2 & 0xF];
    }
}

bsl::ostream&
MessageGUID::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);

    if (isUnset()) {
        stream << "** UNSET **";
    }
    else {
        char buffer[MessageGUID::e_SIZE_HEX];
        toHex(buffer);
        stream.write(buffer, MessageGUID::e_SIZE_HEX);
    }

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
