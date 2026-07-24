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

#ifndef INCLUDED_MQBAUTHZ_ANONPASSPLUGINLIBRARY
#define INCLUDED_MQBAUTHZ_ANONPASSPLUGINLIBRARY

/// @file mqbauthz_pluginlibrary.h
///
/// @brief Provide library of all built-in authorizer plugins for the
/// broker.

// MQB
#include <mqbplug_plugininfo.h>
#include <mqbplug_pluginlibrary.h>

// BDE
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslmf_movableref.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace mqbauthz {

// ===================
// class PluginLibrary
// ===================

class PluginLibrary : public mqbplug::PluginLibrary {
  private:
    // DATA
    bsl::vector<mqbplug::PluginInfo> d_plugins;

  private:
    // NOT IMPLEMENTED
    PluginLibrary(const PluginLibrary& other) BSLS_KEYWORD_DELETED;
    PluginLibrary& operator=(const PluginLibrary& other) BSLS_KEYWORD_DELETED;

  public:
    // TYPES
    typedef bsl::allocator<> allocator_type;

    // CREATORS
    explicit PluginLibrary(const allocator_type& allocator = allocator_type());

    ~PluginLibrary() BSLS_KEYWORD_OVERRIDE;

    // MODIFIERS

    /// Called by 'PluginManager' during broker startup if at least one
    /// enabled plugin is provided by this library.
    int activate() BSLS_KEYWORD_OVERRIDE;

    /// Called by 'PluginManager' during broker shutdown if at least one
    /// enabled plugin is provided by this library.
    void deactivate() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    const bsl::vector<mqbplug::PluginInfo>&
    plugins() const BSLS_KEYWORD_OVERRIDE;

    allocator_type get_allocator() const;
};

// INLINE DEFINITIONS

inline PluginLibrary::allocator_type PluginLibrary::get_allocator() const
{
    return d_plugins.get_allocator();
}

}  // close package namespace
}  // close enterprise namespace

#endif
