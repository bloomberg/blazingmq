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

// mqbauthn_anonpasspluginlibrary.h                             -*-C++-*-
#ifndef INCLUDED_MQBAUTHN_ANONPASSPLUGINLIBRARY
#define INCLUDED_MQBAUTHN_ANONPASSPLUGINLIBRARY

/// @file mqbauthn_anonpasspluginlibrary.h
///
/// @brief Provide library of anonymous pass built-in authenticator plugin for
/// the broker.
///
/// @bbref{mqbauthn::PluginLibrary} provides the definition for the
/// 'PluginLibrary' class, which represents and publishes anonymous pass
/// authenticator plugin for interfacing with the BMQ broker (i.e.,
/// 'bmqbrkr.tsk').

// MQB
#include <mqbplug_plugininfo.h>
#include <mqbplug_pluginlibrary.h>

// BDE
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace mqbauthn {

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
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(PluginLibrary, bslma::UsesBslmaAllocator)

    // CREATORS
    explicit PluginLibrary(bslma::Allocator* allocator = 0);

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
};

}  // close package namespace
}  // close enterprise namespace

#endif
