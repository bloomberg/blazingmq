// Copyright 2020-2024 Bloomberg Finance L.P.
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

// mqbcmd_jsonprinter.cpp                                            -*-C++-*-
#include <mqbcmd_jsonprinter.h>

#include <mqbscm_version.h>

// BDE
#include <baljsn_decoder.h>
#include <baljsn_encoder.h>
#include <ball_log.h>

namespace BloombergLP {
namespace mqbcmd {

namespace {
const char k_LOG_CATEGORY[] = "MQBCMD.JSONPRINTER";
}  // close unnamed namespace

// ------------------
// struct JsonPrinter
// ------------------

bsl::ostream& JsonPrinter::print(bsl::ostream& os,
                                 const Result& result,
                                 bool          pretty,
                                 int           level,
                                 int           spacesPerLevel)
{
    bslma::Allocator* alloc = bslma::Default::allocator(0);

    baljsn::Encoder        encoder(alloc);
    baljsn::EncoderOptions options;
    options.setEncodingStyle(pretty ? baljsn::EncoderOptions::e_PRETTY
                                    : baljsn::EncoderOptions::e_COMPACT);
    options.setInitialIndentLevel(level);
    options.setSpacesPerLevel(spacesPerLevel);

    const int rc = encoder.encode(os, result, options);
    if (0 != rc) {
        BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);
        BALL_LOG_ERROR << "failed to encode Result [" << result.selectionName()
                       << "], rc = " << rc;
    }
    return os;
}

bsl::ostream&
JsonPrinter::printResponses(bsl::ostream&            os,
                            const RouteResponseList& responseList,
                            bool                     pretty,
                            int                      level,
                            int                      spacesPerLevel)
{
    // First need to decode the string responses back to a result format.
    baljsn::Decoder        decoder;
    baljsn::DecoderOptions decoderOptions;
    decoderOptions.setSkipUnknownElements(true);

    mqbcmd::RouteResponseResultList responseResultList;

    bsl::vector<mqbcmd::RouteResponseResult>& responseResults =
        responseResultList.responses();

    for (bsl::vector<mqbcmd::RouteResponse>::const_iterator rit =
             responseList.responses().begin();
         rit != responseList.responses().end();
         rit++) {
        mqbcmd::RouteResponseResult result;
        result.source() = rit->source();
        bsl::istringstream jsonStream(rit->response());
        const int          rc = decoder.decode(jsonStream,
                                      &result.result(),
                                      decoderOptions);
        if (0 != rc) {
            BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);
            BALL_LOG_ERROR << "failed to decode result [" << rit->response()
                           << "] , rc = " << rc;
            return os;
        }
        responseResults.push_back(result);
    }

    bslma::Allocator* alloc = bslma::Default::allocator(0);

    baljsn::Encoder        encoder(alloc);
    baljsn::EncoderOptions options;
    options.setEncodingStyle(pretty ? baljsn::EncoderOptions::e_PRETTY
                                    : baljsn::EncoderOptions::e_COMPACT);
    options.setInitialIndentLevel(level);
    options.setSpacesPerLevel(spacesPerLevel);

    const int rc = encoder.encode(os, responseResultList, options);
    if (0 != rc) {
        BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);
        BALL_LOG_ERROR << "failed to encode response list, rc = " << rc;
    }
    return os;
}

}  // close package namespace
}  // close enterprise namespace
