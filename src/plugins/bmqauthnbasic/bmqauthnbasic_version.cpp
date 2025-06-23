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

#include <bmqauthnbasic_version.h>

namespace BloombergLP {

#define STRINGIFY2(a) #a
#define STRINGIFY(a) STRINGIFY2(a)

#define AUTHNBASIC_VERSION_STRING                                             \
    "BLP_LIB_AUTHNBASIC_" STRINGIFY(AUTHNBASIC_VERSION_MAJOR) "." STRINGIFY(  \
        AUTHNBASIC_VERSION_MINOR) "." STRINGIFY(AUTHNBASIC_VERSION_PATCH)

const char* bmqauthnbasic::Version::s_ident = "$Id: " AUTHNBASIC_VERSION_STRING
                                              " $";
const char* bmqauthnbasic::Version::s_what = "@(#)" AUTHNBASIC_VERSION_STRING;

const char* bmqauthnbasic::Version::AUTHNBASIC_S_VERSION =
    AUTHNBASIC_VERSION_STRING;
const char* bmqauthnbasic::Version::s_dependencies      = "";
const char* bmqauthnbasic::Version::s_buildInfo         = "";
const char* bmqauthnbasic::Version::s_timestamp         = "";
const char* bmqauthnbasic::Version::s_sourceControlInfo = "";

}  // close enterprise namespace
