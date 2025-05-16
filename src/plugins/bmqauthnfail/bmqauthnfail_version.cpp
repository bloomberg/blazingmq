// Copyright 2016-2025 Bloomberg Finance L.P.
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

#include <bmqauthnfail_version.h>

namespace BloombergLP {

#define STRINGIFY2(a) #a
#define STRINGIFY(a) STRINGIFY2(a)

#define AUTHNFAIL_VERSION_STRING                                              \
    "BLP_LIB_AUTHNFAIL_" STRINGIFY(AUTHNFAIL_VERSION_MAJOR) "." STRINGIFY(    \
        AUTHNFAIL_VERSION_MINOR) "." STRINGIFY(AUTHNFAIL_VERSION_PATCH)

const char* bmqauthnfail::Version::s_ident = "$Id: " AUTHNFAIL_VERSION_STRING
                                             " $";
const char* bmqauthnfail::Version::s_what = "@(#)" AUTHNFAIL_VERSION_STRING;

const char* bmqauthnfail::Version::AUTHNFAIL_S_VERSION =
    AUTHNFAIL_VERSION_STRING;
const char* bmqauthnfail::Version::s_dependencies      = "";
const char* bmqauthnfail::Version::s_buildInfo         = "";
const char* bmqauthnfail::Version::s_timestamp         = "";
const char* bmqauthnfail::Version::s_sourceControlInfo = "";

}  // close enterprise namespace
