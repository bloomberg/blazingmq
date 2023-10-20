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

// plugins_pluginlibrary.h                                          -*-C++-*-
#ifndef INCLUDED_PLUGINS_PLUGINLIBRARY
#define INCLUDED_PLUGINS_PLUGINLIBRARY

//@PURPOSE: Provide library of enterprise broker plugins for Bloomberg.
//
//@CLASSES:
//  plugins::PluginLibrary: Library of Bloomberg enterprise broker plugins.
//
//@DESCRIPTION: This component provides the definition for the 'PluginLibrary'
// class, which represents and publishes various plugins for interfacing
// between the BMQ broker (i.e., 'bmqbrkr.tsk') and various enterprise services
// available only within Bloomberg.

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
namespace plugins {
// ===================
// class PluginLibrary
// ===================

class PluginLibrary : public mqbplug::PluginLibrary {
  private:
    // DATA
    bsl::vector<mqbplug::PluginInfo> d_plugins;

  private:
    // NOT IMPLEMENTED
    PluginLibrary(const PluginLibrary&);
    PluginLibrary& operator=(const PluginLibrary&);
    // Copy constructor and assignment operator are not implemented.

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(PluginLibrary, bslma::UsesBslmaAllocator)

    // CREATORS
    explicit PluginLibrary(bslma::Allocator* allocator = 0);
    // Constructor.
    ~PluginLibrary() override;
    // Destructor.

    // MODIFIERS
    int activate() override;
    // Called by 'PluginManager' during broker startup if at least one
    // enabled plugin is provided by this library.

    void deactivate() override;
    // Called by 'PluginManager' during broker shutdown if at least one
    // enabled plugin is provided by this library.

    // ACCESSORS
    const bsl::vector<mqbplug::PluginInfo>& plugins() const override;
};

}  // close package namespace
}  // close enterprise namespace

#endif

// ----------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2023
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ------------------------------ END-OF-FILE ---------------------------------
