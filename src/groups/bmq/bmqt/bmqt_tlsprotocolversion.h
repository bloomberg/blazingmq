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

// bmqt_tlsprotocol_version.h                                        -*-C++-*-
#ifndef INCLUDED_BMQT_TLSPROTOCOLVERSION
#define INCLUDED_BMQT_TLSPROTOCOLVERSION

///@PURPOSE: Provide a set of helpers for parsing TLS related config values.
///
///@CLASSES:
///  bmqu::TlsProtocolVersion: Struct containing enums representing TLS
///  protocol versions.
//
//   bmqu::TlsProtocolVersionUtil: Utility struct for processing TLS protocol
///  config values.
///
///@DESCRIPTION: 'bmqt::TlsProtocolVersion'
///
/// Functionality
/// -------------
/// TBD: Comment about Default vs Initial value -- assert vs safe mode
///
/// Usage
/// -----
/// This section illustrates the intended use of this component.
///
//// Example 1: TBD:
////- - - - - - - -
/// TBD:

#include <bmqt_resultcode.h>

namespace BloombergLP {
namespace bmqt {

// ======================
// struct TlsProtocolVersion
// ======================

struct TlsProtocolVersion {
    enum Value { e_TLS1_3 };

    // CLASS METHODS
    static bsl::ostream& print(bsl::ostream&             stream,
                               TlsProtocolVersion::Value value,
                               int                       level          = 0,
                               int                       spacesPerLevel = 4);

    static const char* toAscii(TlsProtocolVersion::Value value);

    static bool fromAscii(TlsProtocolVersion::Value* out,
                          const bslstl::StringRef&   str);
};

// ============================
// class TlsProtocolVersionUtil
// ============================

/// Helper struct for parsing TLS config values.
struct TlsProtocolVersionUtil {
    /// Parse the minimum TLS version specified in the TLS config string.
    ///
    /// TLS version strings are comma separated values of TLS versions.
    /// Currently, only `TLSv1.3` is supported. Invalid values are ignored,
    /// and the empty string results in an error.
    ///
    /// @param[out] outMinVersion The location to store the minimum version
    /// parsed
    /// @param versions A comma separated string of versions
    ///
    /// @returns Zero on success
    static GenericResult::Enum
    parseMinTlsVersion(TlsProtocolVersion::Value* outMinVersion,
                       const bsl::string&         versions);
};

}
}

#endif
