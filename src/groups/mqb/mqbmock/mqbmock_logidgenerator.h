// Copyright 2019-2023 Bloomberg Finance L.P.
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

// mqbmock_logidgenerator.h                                           -*-C++-*-
#ifndef INCLUDED_MQBMOCK_LOGIDGENERATOR
#define INCLUDED_MQBMOCK_LOGIDGENERATOR

//@PURPOSE: Mock implementation of the 'mqbsi::LogIdGenerator' interface.
//
//@CLASSES:
//  mqbmock::LogIdGenerator: Mock log Id generator implementation
//
//@DESCRIPTION: This component provides a mock implementation,
// 'mqbmock::LogIdGenerator', of the 'mqbsi::LogIdGenerator' interface that is
// used to emulate a real log Id generator for testing purposes.

// MQB

#include <mqbsi_log.h>
#include <mqbu_storagekey.h>

// BDE
#include <bsl_string.h>
#include <bsl_unordered_set.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace mqbmock {

// ====================
// class LogIdGenerator
// ====================

/// Mock implementation of the `mqbsi::LogIdGenerator` interface.
class LogIdGenerator : public mqbsi::LogIdGenerator {
  private:
    // DATA
    bsl::string                          d_logPrefix;
    bsl::unordered_set<mqbu::StorageKey> d_logIds;
    int                                  d_nextId;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(LogIdGenerator, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `mqbmock::LogIdGenerator` that generates log names having
    /// the specified `prefix`.  Use the specified `allocator` for memory
    /// allocations.
    LogIdGenerator(const char* prefix, bslma::Allocator* allocator);

    /// Destructor
    virtual ~LogIdGenerator() BSLS_KEYWORD_OVERRIDE;

    /// Register the specified `logId` among those generated by this object
    /// and return `true` if successful, `false` otherwise (e.g., `logId` is
    /// already registered).  The effect of this is that `logId` will not be
    /// returned by a future call to `generateLogId(...)`.
    virtual bool
    registerLogId(const mqbu::StorageKey& logId) BSLS_KEYWORD_OVERRIDE;

    /// Create a new log name and a new unique log ID that has not before
    /// been generated or registered by this object and load them into the
    /// specified `logName` and `logId`.
    virtual void generateLogId(bsl::string*      logName,
                               mqbu::StorageKey* logId) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
