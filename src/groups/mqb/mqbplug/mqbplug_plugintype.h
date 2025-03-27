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

// mqbplug_plugintype.h                                               -*-C++-*-
#ifndef INCLUDED_MQBPLUG_PLUGINTYPE
#define INCLUDED_MQBPLUG_PLUGINTYPE

//@PURPOSE: Provide an enum representing the type of a plugin.
//
//@CLASSES:
//  mqbplug::PluginType: Enum for plugin types.
//
//@DESCRIPTION: This component provide definition for enum
// ('mqbplug::PluginType') used to define plugin for BlazingMQ broker.

// MQB

// BDE
#include <bsl_ostream.h>
#include <bsl_string.h>

namespace BloombergLP {
namespace mqbplug {

// =================
// struct PluginType
// =================

/// This enum represents the `family` of plugins (stats, commands,
/// authentication, ...)
struct PluginType {
    // TYPES
    enum Enum { e_STATS_CONSUMER = 0, e_AUTHENTICATOR = 1 };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a `PluginType::Enum`
    /// value.
    static bsl::ostream& print(bsl::ostream&    stream,
                               PluginType::Enum value,
                               int              level          = 0,
                               int              spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << PluginType::toAscii(PluginType::e_STATS_CONSUMER);
    /// ```
    /// will print the following on standard output:
    /// ```
    /// STATS_CONSUMER
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(PluginType::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, PluginType::Enum value);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------
// struct PluginType
// -----------------

// FREE OPERATORS
inline bsl::ostream& operator<<(bsl::ostream&             stream,
                                mqbplug::PluginType::Enum value)
{
    return mqbplug::PluginType::print(stream, value, 0, -1);
}

}  // close package namespace
}  // close enterprise namespace

#endif
