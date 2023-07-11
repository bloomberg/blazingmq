// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mqbi_appkeygenerator.h                                             -*-C++-*-
#ifndef INCLUDED_MQBI_APPKEYGENERATOR
#define INCLUDED_MQBI_APPKEYGENERATOR

//@PURPOSE: Provide an interface for an AppKeyGenerator.
//
//@CLASSES:
//  mqbi::AppKeyGenerator: interface for an AppKeyGenerator
//
//@DESCRIPTION: 'mqbi::AppKeyGenerator' is an interface for a generator of
// 'mqbu::StorageKey' objects.

// MQB

// BDE
#include <bsl_string.h>

namespace BloombergLP {

// FORWARD DECLARATIONS
namespace mqbu {
class StorageKey;
}

namespace mqbi {

// =====================
// class AppKeyGenerator
// =====================

/// Interface for an AppKeyGenerator.
class AppKeyGenerator {
  public:
    // CREATORS

    /// Destroy this object.
    virtual ~AppKeyGenerator();

    // MANIPULATORS

    /// Return a unique appKey for the specified `appId` for a queue
    /// assigned to the specified `partitionId`.  Behavior is undefined
    /// unless this method is invoked in the dispatcher thread of the
    /// `partitionId`.
    virtual mqbu::StorageKey generateAppKey(const bsl::string& appId,
                                            int partitionId) = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
