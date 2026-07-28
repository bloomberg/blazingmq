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

#ifndef INCLUDED_MQBPLUG_PLUGINFACTORY
#define INCLUDED_MQBPLUG_PLUGINFACTORY

//@PURPOSE: Provide definition of the base class for plugin factories.
//
//@CLASSES:
//  mqbplug::PluginFactory: base class for plugin factories.
//
//@DESCRIPTION: This component provide definition for class
// ('mqbplug::PluginFactory') used for having a common type for the factories.

// MQB

#include <bsl_cstddef.h>
#include <bsl_optional.h>

namespace BloombergLP {
namespace mqbplug {

// ===================
// class PluginFactory
// ===================

/// Base class solely used for having a common type for the factories.
class PluginFactory {
  public:
    // CREATORS
    PluginFactory();

    virtual ~PluginFactory();
};

// =======================
// class PluginFactoryUtil
// =======================

/// Base class solely used for having a common type for the factories.
struct PluginFactoryUtil {
    /// Search a range for a PluginFactory that has the subtype t_ITERATOR.
    /// Return `end` if no such value was found.
    template <typename t_TARGET, typename t_ITERATOR>
    static bsl::optional<t_TARGET*> findType(t_ITERATOR begin, t_ITERATOR end);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------
// class PluginFactory
// -------------------

// CREATORS
inline PluginFactory::PluginFactory()
{
    // NOTHING
}

// -----------------------
// class PluginFactoryUtil
// -----------------------

template <typename t_TARGET, typename t_ITERATOR>
bsl::optional<t_TARGET*> PluginFactoryUtil::findType(t_ITERATOR begin,
                                                     t_ITERATOR end)
{
    for (t_ITERATOR factoryIt = begin; factoryIt != end; ++factoryIt) {
        t_TARGET* candidateFactory = dynamic_cast<t_TARGET*>(*factoryIt);
        if (candidateFactory) {
            return bsl::make_optional(candidateFactory);
        }
    }
    return bsl::nullopt;
}

}  // close package namespace
}  // close enterprise namespace

#endif
