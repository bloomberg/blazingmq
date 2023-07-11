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

// mqbplug_pluginfactory.h                                            -*-C++-*-
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

}  // close package namespace
}  // close enterprise namespace

#endif
