// Copyright 2024 Bloomberg Finance L.P.
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

#ifndef INCLUDED_M_BMQSTORAGEWRITER_UTIL
#define INCLUDED_M_BMQSTORAGEWRITER_UTIL

#include <bdljsn_json.h>
#include <bsl_cstdlib.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bsls_types.h>
#include <mqbu_storagekey.h>

namespace BloombergLP {
namespace m_bmqstoragewriter {

inline bsl::string getString(const bdljsn::JsonObject& obj,
                             const bsl::string_view&   key,
                             bslma::Allocator*         allocator)
{
    bdljsn::JsonObject::ConstIterator it = obj.find(key);
    if (it == obj.end()) {
        return bsl::string(allocator);
    }
    return bsl::string(it->second.theString(), allocator);
}

inline bsls::Types::Uint64 getUint64(const bdljsn::JsonObject& obj,
                                     const bsl::string_view&   key)
{
    bdljsn::JsonObject::ConstIterator it = obj.find(key);
    if (it == obj.end()) {
        return 0;
    }
    return bsl::strtoull(it->second.theString().c_str(), 0, 10);
}

inline int getInt(const bdljsn::JsonObject& obj, const bsl::string_view& key)
{
    bdljsn::JsonObject::ConstIterator it = obj.find(key);
    if (it == obj.end()) {
        return 0;
    }
    return bsl::atoi(it->second.theString().c_str());
}

inline mqbu::StorageKey keyFromHex(const bsl::string& hex)
{
    if (hex.size() != mqbu::StorageKey::e_KEY_LENGTH_HEX) {
        return mqbu::StorageKey();
    }
    return mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(),
                            hex.c_str());
}

}  // close package namespace
}  // close enterprise namespace

#endif
