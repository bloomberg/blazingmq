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

#include <bmqprometheus_version.h>

namespace BloombergLP {

#define STRINGIFY2(a) #a
#define STRINGIFY(a) STRINGIFY2(a)

#define PROMETHEUS_VERSION_STRING                                             \
    "BLP_LIB_PROMETHEUS_" STRINGIFY(PROMETHEUS_VERSION_MAJOR) "." STRINGIFY(  \
        PROMETHEUS_VERSION_MINOR) "." STRINGIFY(PROMETHEUS_VERSION_PATCH)

const char* bmqprometheus::Version::s_ident = "$Id: " PROMETHEUS_VERSION_STRING
                                              " $";
const char* bmqprometheus::Version::s_what = "@(#)" PROMETHEUS_VERSION_STRING;

const char* bmqprometheus::Version::PROMETHEUS_S_VERSION =
    PROMETHEUS_VERSION_STRING;
const char* bmqprometheus::Version::s_dependencies      = "";
const char* bmqprometheus::Version::s_buildInfo         = "";
const char* bmqprometheus::Version::s_timestamp         = "";
const char* bmqprometheus::Version::s_sourceControlInfo = "";

}  // close enterprise namespace
