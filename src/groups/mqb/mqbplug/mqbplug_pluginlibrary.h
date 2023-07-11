// Copyright 2022-2023 Bloomberg Finance L.P.
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

// mqbplug_pluginlibrary.h                                            -*-C++-*-
#ifndef INCLUDED_MQBPLUG_PLUGINLIBRARY
#define INCLUDED_MQBPLUG_PLUGINLIBRARY

//@PURPOSE: Provide definition of the base class for plugin libraries.
//
//@CLASSES:
//  mqbplug::PluginLibrary: base class for plugin libraries.
//
//@DESCRIPTION: This component provides definition for class
// 'mqbplug::PluginLibrary' used for having a common type for the libraries.

// MQB

#include <mqbplug_plugininfo.h>

// BDE
#include <bsl_vector.h>
#include <bslma_allocator.h>

namespace BloombergLP {

namespace mqbplug {

// ===================
// class PluginLibrary
// ===================

/// Base class for plugin libraries, that exposes metadata of each
/// plugin, as well as provides support for library lifetime management
/// method.
class PluginLibrary {
  public:
    // CREATORS

    /// Construct plugin library.
    PluginLibrary();

    virtual ~PluginLibrary();

    // MODIFIERS

    /// Invoked by `PluginManager` during broker startup if at least one
    /// enabled library is provided by the `PluginManager`. This allows the
    /// `PluginLibrary` to initialize some internal state shared among
    /// plugin objects, query its configuration, etc. Returns nonzero if an
    /// error occurred, indicating that no plugins should be loaded from the
    /// library (this is nonfatal for broker startup, unless an enabled
    /// plugin is provided by this library); otherwise, returns zero on
    /// success.
    virtual int activate() = 0;

    /// Invoked by `PluginManager` during broker shutdown, for cleanup and
    /// graceful shutdown.
    virtual void deactivate() = 0;

    // ACCESSORS

    /// Return the information about all plugins this PluginLibrary exposes.
    virtual const bsl::vector<PluginInfo>& plugins() const = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
