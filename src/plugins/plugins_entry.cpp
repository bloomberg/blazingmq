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

// plugins_entry.cpp                                                -*-C++-*-

// PLUGINS
#include <plugins_pluginlibrary.h>
#include <plugins_version.h>

// MQB
#include <mqbplug_pluginlibrary.h>

// BDE
#include <ball_log.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bslma_managedptr.h>

using namespace BloombergLP;

extern "C" {
void instantiatePluginLibrary(
    bslma::ManagedPtr<mqbplug::PluginLibrary>* library,
    bslma::Allocator*                          allocator);
}  // close extern "C"

void instantiatePluginLibrary(
    bslma::ManagedPtr<mqbplug::PluginLibrary>* library,
    bslma::Allocator*                          allocator)
{
    BALL_LOG_SET_CATEGORY("PLUGINS.ENTRY");

    BALL_LOG_INFO << "Instantiating 'libplugins.so' plugin library "
                     "(version: "
                  << plugins::Version::version() << ")";

    *library = bslma::ManagedPtrUtil::allocateManaged<plugins::PluginLibrary>(
        allocator);
}
