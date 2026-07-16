// Copyright 2026 Bloomberg Finance L.P.
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

// bmqt_sessionoptions.cpp                                            -*-C++-*-
#include <bmqt_tlsprotocolversion.h>

// BMQ
#include <bmqscm_version.h>
#include <bmqt_resultcode.h>
#include <bmqu_stringutil.h>

// BDE
#include <bdlb_string.h>
#include <bdls_filesystemutil.h>
#include <bslim_printer.h>
#include <bslma_allocator.h>

namespace BloombergLP {
namespace bmqt {

// =========================
// struct TlsProtocolVersion
// =========================

bsl::ostream& TlsProtocolVersion::print(bsl::ostream&             stream,
                                        TlsProtocolVersion::Value value,
                                        int                       level,
                                        int spacesPerLevel)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printValue(TlsProtocolVersion::toAscii(value));
    printer.end();

    return stream;
}

const char* TlsProtocolVersion::toAscii(TlsProtocolVersion::Value value)
{
    // Use openssl compatible string representation
    switch (value) {
    case e_TLS1_3: return "TLSv1.3";
    default: return "(* UNKNOWN *)";
    }
}

bool TlsProtocolVersion::fromAscii(TlsProtocolVersion::Value* out,
                                   const bslstl::StringRef&   str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(TlsProtocolVersion::e_##M),    \
                                       str.data(),                            \
                                       str.length())) {                       \
        *out = TlsProtocolVersion::e_##M;                                     \
        return true;                                                          \
    }

    CHECKVALUE(TLS1_3);

    // Invalid string
    return false;

#undef CHECKVALUE
}

// =============================
// struct TlsProtocolVersionUtil
// =============================

GenericResult::Enum TlsProtocolVersionUtil::parseMinTlsVersion(
    TlsProtocolVersion::Value* outMinVersion,
    const bsl::string&         versions)
{
    BSLS_ASSERT(outMinVersion);

    bsl::vector<bslstl::StringRef> vs =
        bmqu::StringUtil::strTokenizeRef(versions, ", \t");
    for (size_t i = 0; i < vs.size(); i++) {
        bmqt::TlsProtocolVersion::Value version;

        if (TlsProtocolVersion::fromAscii(&version, vs[i])) {
            // We only support TLS v1.3
            if (version == TlsProtocolVersion::e_TLS1_3) {
                *outMinVersion = version;
                return GenericResult::e_SUCCESS;
            }
        }
    }

    return GenericResult::e_INVALID_ARGUMENT;
}

}
}
