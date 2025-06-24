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

// mqbplug_plugininfo.cpp                                             -*-C++-*-
#include <mqbplug_plugininfo.h>

#include <mqbscm_version.h>
// BDE
#include <bsl_string_view.h>
#include <bslim_printer.h>
#include <bslma_default.h>

namespace BloombergLP {
namespace mqbplug {

// ----------------
// class PluginInfo
// ----------------

// CREATORS
PluginInfo::PluginInfo(mqbplug::PluginType::Enum type,
                       bsl::string_view          name,
                       bslma::Allocator*         allocator)
: d_type(type)
, d_name(name, allocator)
, d_description(allocator)
, d_version(allocator)
, d_factory_sp()
{
    // NOTHING
}

PluginInfo::PluginInfo(const PluginInfo& other, bslma::Allocator* allocator)
: d_type(other.d_type)
, d_name(other.d_name, allocator)
, d_description(other.d_description, allocator)
, d_version(other.d_version, allocator)
, d_factory_sp(other.d_factory_sp)
{
    // NOTHING
}

PluginInfo::~PluginInfo()
{
    // NOTHING
}

// ACCESSORS
bsl::ostream&
PluginInfo::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);

    printer.start();
    printer.printAttribute("type", type());
    printer.printAttribute("name", name());
    printer.printAttribute("description", description());
    printer.printAttribute("version", version());
    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
