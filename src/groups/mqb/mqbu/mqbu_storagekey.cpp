// Copyright 2016-2023 Bloomberg Finance L.P.
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

// mqbu_storagekey.cpp                                                -*-C++-*-
#include <mqbu_storagekey.h>

#include <mqbscm_version.h>
// BSL
#include <bslmf_assert.h>

namespace BloombergLP {
namespace mqbu {

BSLMF_ASSERT(StorageKey::e_KEY_LENGTH_BINARY == sizeof(StorageKey));

// ----------------
// class StorageKey
// ----------------

const char*      StorageKey::k_NULL_KEY_BUFFER("\0\0\0\0\0");
const StorageKey StorageKey::k_NULL_KEY;

// ------------------------
// struct StorageKeyHexUtil
// ------------------------

const char StorageKeyHexUtil::k_INT_TO_HEX_TABLE[16] = {'0',
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

/// Conversion table used to convert a hexadecimal value to it's int
/// representation. (99 is because in the ASCII table, `9` is 57 and `A` is
/// 65, so the 99 represents unexpected invalid value in the input).
const char StorageKeyHexUtil::k_HEX_TO_INT_TABLE[24] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  99, 99,
    99, 99, 99, 99, 99, 10, 11, 12, 13, 14, 15, 99};

void StorageKeyHexUtil::hexToBinary(char*       bin,
                                    size_t      binLength,
                                    const char* hex)
{
    for (size_t i = 0; i < binLength; ++i) {
        const size_t index = 2 * i;
        const int    ch1   = hex[index + 0] - '0';
        const int    ch2   = hex[index + 1] - '0';

        bin[i] = (k_HEX_TO_INT_TABLE[ch1] << 4) | (k_HEX_TO_INT_TABLE[ch2]);
    }
}

void StorageKeyHexUtil::binaryToHex(char*       hex,
                                    const char* bin,
                                    size_t      binLength)
{
    for (size_t i = 0; i < binLength; ++i) {
        const size_t        index = 2 * i;
        const unsigned char ch    = bin[i];

        hex[index]     = k_INT_TO_HEX_TABLE[ch >> 4];
        hex[index + 1] = k_INT_TO_HEX_TABLE[ch & 0xF];
    }
}

}  // close package namespace
}  // close enterprise namespace
