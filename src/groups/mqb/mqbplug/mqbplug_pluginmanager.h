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

// mqbplug_pluginmanager.h                                            -*-C++-*-
#ifndef INCLUDED_MQBPLUG_PLUGINMANAGER
#define INCLUDED_MQBPLUG_PLUGINMANAGER

//@PURPOSE: Manage dynamically loaded plugins on behalf of the BlazingMQ
// broker.
//
//@CLASSES:
//  mqbplug::PluginManager: Managed plugins for the BlazingMQ broker.
//
//@DESCRIPTION: This component provides a mechanism, 'mqbplug::PluginManager',
// that allows the BlazingMQ broker to manage plugin libraries loaded at
// runtime, which may provide one or more plugins of various kinds. This object
// is then injected into various other BlazingMQ classes, which thereby query
// the PluginManager to obtain factories for constructing implementations of
// various BlazingMQ interfaces.

// MQB

#include <mqbcfg_messages.h>
#include <mqbplug_plugininfo.h>
#include <mqbplug_pluginlibrary.h>
#include <mqbplug_plugintype.h>

// BDE
#include <bsl_map.h>
#include <bsl_ostream.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bsl_vector.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_cpp11.h>

namespace BloombergLP {

namespace mqbplug {
// ===================
// class PluginManager
// ===================

class PluginManager BSLS_CPP11_FINAL {
  private:
    // PRIVATE TYPES
    typedef bsl::unordered_map<bsl::string, int> RequiredPluginsRecord;

    // DATA

    // Allocator.
    bslma::Allocator* d_allocator_p;

    // Set of handles for loaded plugins as returned by `dlopen()`.
    // NOTE: This structure should be declared before `d_pluginLibraries`,
    //       to ensure that `dlclose()` is called last.
    bsl::vector<bslma::ManagedPtr<void> > d_pluginHandles;

    // Set of all known plugin-libraries, as provided by configuration.
    bsl::vector<bslma::ManagedPtr<PluginLibrary> > d_pluginLibraries;

    // Mapping from plugin-type to plugin-info VST for all enabled plugins.
    bsl::multimap<PluginType::Enum, const PluginInfo*> d_enabledPlugins;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    PluginManager(const PluginManager&);             // = delete;
    PluginManager& operator=(const PluginManager&);  // = delete;

  private:
    // PRIVATE METHODS

    /// Implementation detail of `PluginLibrary::start()`, which is invoked
    /// on each `*.so`-file found in each plugin-directory specified by the
    /// bmqbrkr configuration.
    ///
    /// Loads the shared object at `path` into memory; if the object is a
    /// bmqbrkr plugin, then it instantiates a `PluginLibrary` from the
    /// object's plugin entry point (i.e, "instantiatePluginLibrary").
    /// Shared objects that are not bmqbrkr plugins are loaded without
    /// error; such objects are thereafter closed with a warning diagnostic.
    ///
    /// Each key in `requiredPlugins` is looked up in the plugins provided
    /// by the `PluginLibrary`. If a unique plugin matches, then the
    /// associated `PluginInfo` is moved into `d_enabledPlugins`.
    ///
    /// The `PluginLibrary::activate()` method is invoked if and only if a
    /// plugin from the library is enabled. If no plugin is enabled, then
    /// the handle to the shared object will be allowed to expire and close
    /// at the conclusion of the method.
    ///
    /// It is not allowed for two plugins to match the same required plugin
    /// name, whether provided by separate shared objects or within the same
    /// library (note that names are caseless-compared). Loading a set of
    /// shared objects providing two or more such plugins will yield an
    /// error and terminate the broker process during startup.
    void loadPluginLibrary(const char*            path,
                           RequiredPluginsRecord* requiredPlugins,
                           bsl::ostream&          errorDescription,
                           int*                   rc);

    /// Implementation detail of `PluginLibrary::loadPluginLibrary()`, which
    /// is in turn an implementation detail of `PluginLibrary::start()`;
    /// this method is invoked on each `PluginLibrary` object corresponding
    /// to a shared object implementing the bmqbrkr plugin API.
    ///
    /// Handles the process of searching `pluginLibrary` for any required
    /// plugins, activating `pluginLibrary` if any such plugins are found,
    /// moving the associated `PluginInfo` objects into `d_enabledPlugins`,
    /// updating the `requiredPlugins`-counters to indicate which
    /// requirements have been satisfied, and using said counters to ensure
    /// the absence of multiple plugins satisfying the same requirement.
    /// Pushes the name of each plugin loaded into `pluginsProvided`.
    ///
    /// See `PluginLibrary::loadPluginLibrary()` comments for more detail.
    void enableRequiredPlugins(
        const bslstl::StringRef&                         pluginPath,
        const bslma::ManagedPtr<mqbplug::PluginLibrary>& pluginLibrary,
        RequiredPluginsRecord*                           requiredPlugins,
        bsl::vector<bsl::string>*                        pluginsProvided,
        bsl::ostream&                                    errorDescription,
        int*                                             rc);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(PluginManager, bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Constructor of a new plugin manager.
    PluginManager(bslma::Allocator* allocator = 0);

    int start(const mqbcfg::Plugins& pluginsConfig,
              bsl::ostream&          errorDescription);

    void stop();

    // ACCESSORS
    void get(PluginType::Enum                    type,
             bsl::unordered_set<PluginFactory*>* result) const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
