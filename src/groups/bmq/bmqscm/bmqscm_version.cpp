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

// bmqscm_version.cpp                                                 -*-C++-*-
#include <bmqscm_version.h>

#include <bmqscm_version.h>
namespace BloombergLP {

#define STRINGIFY2(a) #a
#define STRINGIFY(a) STRINGIFY2(a)

#define BMQSCM_VERSION_STRING                                                 \
    "BLP_LIB_BMQ_" STRINGIFY(BMQ_VERSION_MAJOR) "." STRINGIFY(                \
        BMQ_VERSION_MINOR) "." STRINGIFY(BMQ_VERSION_PATCH)

const char* bmqscm::Version::s_ident = "$Id: " BMQSCM_VERSION_STRING " $";
const char* bmqscm::Version::s_what  = "@(#)" BMQSCM_VERSION_STRING;

const char* bmqscm::Version::BMQSCM_S_VERSION    = BMQSCM_VERSION_STRING;
const char* bmqscm::Version::s_dependencies      = "";
const char* bmqscm::Version::s_buildInfo         = "";
const char* bmqscm::Version::s_timestamp         = "";
const char* bmqscm::Version::s_sourceControlInfo = "";

}  // close enterprise namespace
