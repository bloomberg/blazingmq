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

// mqbmock_appkeygenerator.h                                          -*-C++-*-
#ifndef INCLUDED_MQBMOCK_APPKEYGENERATOR
#define INCLUDED_MQBMOCK_APPKEYGENERATOR

//@PURPOSE: Provide a mock implementation of 'mqbi::AppKeyGenerator'.
//
//@CLASSES:
//  mqbmock::AppKeyGenerator: mock AppKeyGenerator implementation
//
//@DESCRIPTION: This component provides a mock implementation,
// 'mqbmock::AppKeyGenerator'.

// MQB

#include <mqbi_appkeygenerator.h>
#include <mqbu_storagekey.h>

// BDE
#include <bsl_string.h>

namespace BloombergLP {
namespace mqbmock {

// =====================
// class AppKeyGenerator
// =====================

class AppKeyGenerator : public mqbi::AppKeyGenerator {
    // DATA
    mqbu::StorageKey d_key;

    // NOT IMPLEMENTED
    AppKeyGenerator& operator=(const AppKeyGenerator&) BSLS_CPP11_DELETED;

  public:
    // CREATORS
    virtual ~AppKeyGenerator() BSLS_CPP11_OVERRIDE;

    // MANIPULATORS

    /// Return a unique appKey for the specified `appId` for a queue
    /// assigned to the specified `partitionId`.  Behavior is undefined
    /// unless this method is invoked in the dispatcher thread of the
    /// `partitionId`.
    virtual mqbu::StorageKey
    generateAppKey(const bsl::string& appId,
                   int                partitionId) BSLS_CPP11_OVERRIDE;

    /// Set the key returned by `generateAppKey` to the specified `key`.
    void setKey(const mqbu::StorageKey& key);
};

// ============================================================================
//                            INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class AppKeyGenerator
// ---------------------

inline void AppKeyGenerator::setKey(const mqbu::StorageKey& key)
{
    d_key = key;
}

}  // close package namespace
}  // close enterprise namespace

#endif
