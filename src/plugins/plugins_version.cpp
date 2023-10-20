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

// plugins_version.cpp                                              -*-C++-*-
#include <plugins_version.h>

namespace BloombergLP {

#define STRINGIFY2(a) #a
#define STRINGIFY(a) STRINGIFY2(a)

#define PLUGINS_VERSION_STRING                                                \
    "BLP_LIB_PLUGINS_" STRINGIFY(PLUGINS_VERSION_MAJOR) "." STRINGIFY(        \
        PLUGINS_VERSION_MINOR) "." STRINGIFY(PLUGINS_VERSION_PATCH)

const char* plugins::Version::s_ident = "$Id: " PLUGINS_VERSION_STRING " $";
const char* plugins::Version::s_what  = "@(#)" PLUGINS_VERSION_STRING;

const char* plugins::Version::PLUGINS_S_VERSION   = PLUGINS_VERSION_STRING;
const char* plugins::Version::s_dependencies      = "";
const char* plugins::Version::s_buildInfo         = "";
const char* plugins::Version::s_timestamp         = "";
const char* plugins::Version::s_sourceControlInfo = "";

}  // close enterprise namespace

// ----------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2023
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ------------------------------- END-OF-FILE --------------------------------
