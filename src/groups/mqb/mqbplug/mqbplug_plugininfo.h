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

// mqbplug_plugininfo.h                                               -*-C++-*-
#ifndef INCLUDED_MQBPLUG_PLUGININFO
#define INCLUDED_MQBPLUG_PLUGININFO

//@PURPOSE: Provide definition for plugin description.
//
//@CLASSES:
//  mqbplug::PluginInfo: VST representing metadata of a plugin.
//
//@DESCRIPTION: This component provide definitions for structure
// ('mqbplug::PluginInfo') used to define plugin for BlazingMQ broker.

// MQB

#include <mqbplug_pluginfactory.h>
#include <mqbplug_plugintype.h>

// BDE
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

namespace mqbplug {

// ================
// class PluginInfo
// ================

/// VST representing the `metadata` for a given Plugin: describing it and
/// exposing the factory method which it exposes.
class PluginInfo {
  public:
    // TYPES
    typedef bsl::shared_ptr<mqbplug::PluginFactory> PluginFactorySp;

  private:
    // DATA
    mqbplug::PluginType::Enum d_type;
    // Plugin type

    bsl::string d_name;
    // Plugin name

    bsl::string d_description;
    // Plugin description

    bsl::string d_version;
    // Plugin version

    PluginFactorySp d_factory_sp;
    // Plugin factory method

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(PluginInfo, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an instance of `PluginInfo` using the specified `type`,
    /// and `name`.  Optionally specify an `allocator` used to supply
    /// memory.
    PluginInfo(mqbplug::PluginType::Enum type,
               bsl::string_view          name,
               bslma::Allocator*         allocator = 0);

    /// Create an instance of `PluginInfo` having the same value as
    /// the specified `other`, that will use the optionally specified
    /// `allocator` to supply memory.
    PluginInfo(const PluginInfo& other, bslma::Allocator* allocator = 0);

    /// Destroy this object.
    ~PluginInfo();

    // MANIPULATORS
    PluginInfo& setType(mqbplug::PluginType::Enum value);
    PluginInfo& setName(bsl::string_view value);
    PluginInfo& setDescription(bsl::string_view value);
    PluginInfo& setVersion(bsl::string_view value);

    /// Set the corresponding field to the specified `value` and return a
    /// reference offering modifiable access to this object.
    PluginInfo& setFactory(const PluginFactorySp& value);

    // ACCESSORS

    /// Return plugin type.
    mqbplug::PluginType::Enum type() const;

    /// Return plugin name.
    const bsl::string& name() const;

    /// Return plugin description.
    const bsl::string& description() const;

    /// Return plugin version.
    const bsl::string& version() const;

    /// Return plugin factory method.
    const PluginFactorySp& factory() const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const PluginInfo& rhs);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

inline bsl::ostream& operator<<(bsl::ostream&              stream,
                                const mqbplug::PluginInfo& rhs)
{
    return rhs.print(stream, 0, -1);
}

// ----------------
// class PluginInfo
// ----------------

// MANIPULATORS
inline PluginInfo& PluginInfo::setType(mqbplug::PluginType::Enum value)
{
    d_type = value;
    return *this;
}

inline PluginInfo& PluginInfo::setName(bsl::string_view value)
{
    d_name = value;
    return *this;
}

inline PluginInfo& PluginInfo::setDescription(bsl::string_view value)
{
    d_description = value;
    return *this;
}

inline PluginInfo& PluginInfo::setVersion(bsl::string_view value)
{
    d_version = value;
    return *this;
}

inline PluginInfo& PluginInfo::setFactory(const PluginFactorySp& value)
{
    d_factory_sp = value;
    return *this;
}

// ACCESSORS
inline mqbplug::PluginType::Enum PluginInfo::type() const
{
    return d_type;
}

inline const bsl::string& PluginInfo::name() const
{
    return d_name;
}

inline const bsl::string& PluginInfo::description() const
{
    return d_description;
}

inline const bsl::string& PluginInfo::version() const
{
    return d_version;
}

inline const mqbplug::PluginInfo::PluginFactorySp& PluginInfo::factory() const
{
    return d_factory_sp;
}

}  // close package namespace
}  // close enterprise namespace

#endif
