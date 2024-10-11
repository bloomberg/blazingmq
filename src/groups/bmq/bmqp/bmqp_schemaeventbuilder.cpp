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

// bmqp_schemaeventbuilder.cpp                                        -*-C++-*-
#include <bmqp_schemaeventbuilder.h>

#include <bmqscm_version.h>
// BDE
#include <bsl_algorithm.h>
#include <bsl_vector.h>

namespace BloombergLP {
namespace bmqp {

// ------------------------
// class SchemaEventBuilder
// ------------------------

// NOTHING: Template in header

// -----------------------------
// struct SchemaEventBuilderUtil
// -----------------------------
EncodingType::Enum SchemaEventBuilderUtil::bestEncodingSupported(
    const bsl::string& remoteFeatureSet)
{
    bsl::vector<bsl::string> encodingsSupported;
    if (!ProtocolUtil::loadFieldValues(&encodingsSupported,
                                       EncodingFeature::k_FIELD_NAME,
                                       remoteFeatureSet)) {
        // TBD: This is TEMPORARY only until all clients are properly
        // advertising their encoding features support; after that it can be
        // made return e_UNKNOWN, and this should reject the client at
        // negotiation.
        return EncodingType::e_BER;  // RETURN
    }

    // If remote supports BER, return BER
    if (bsl::find(encodingsSupported.cbegin(),
                  encodingsSupported.cend(),
                  bsl::string(EncodingFeature::k_ENCODING_BER)) !=
        encodingsSupported.cend()) {
        return EncodingType::e_BER;  // RETURN
    }

    // Else if remote supports JSON, return JSON
    if (bsl::find(encodingsSupported.cbegin(),
                  encodingsSupported.cend(),
                  bsl::string(EncodingFeature::k_ENCODING_JSON)) !=
        encodingsSupported.cend()) {
        return EncodingType::e_JSON;  // RETURN
    }

    // TBD: Else return BER -> This is TEMPORARY only until all clients are
    // properly advertising their encoding features support; after that it can
    // be made return e_UNKNOWN, and this should reject the client at
    // negotiation.
    return EncodingType::e_BER;
}

}  // close package namespace
}  // close enterprise namespace
