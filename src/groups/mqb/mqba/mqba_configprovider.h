// Copyright 2014-2023 Bloomberg Finance L.P.
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

// mqba_configprovider.h                                              -*-C++-*-
#ifndef INCLUDED_MQBA_CONFIGPROVIDER
#define INCLUDED_MQBA_CONFIGPROVIDER

/// @file mqba_configprovider.h
///
/// @brief Provide a mechanism to retrieve configuration information.
///
/// @bbref{mqba::ConfigProvider} is a mechanism to retrieve configuration
/// information for domain, ...
///
/// @todo Add commandHandler.
/// @todo Add statistics.

// BDE
#include <ball_log.h>
#include <bdlb_transparentstringhash.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_mutex.h>
#include <bsls_cpp11.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcmd {
class ConfigProviderCommand;
}
namespace mqbcmd {
class Error;
}

namespace mqba {

// ====================
// class ConfigProvider
// ====================

/// Mechanism to retrieve configuration information
class ConfigProvider {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBA.CONFIGPROVIDER");

  public:
    // TYPES

    /// Signature of a callback method for the `getDomainConfig`. On
    /// success, `status` is 0 and `result` contains the configuration, on
    /// error `status` is non-zero and `result` contains a description of
    /// the error.
    typedef bsl::function<void(int status, const bsl::string& result)>
        GetDomainConfigCb;

  private:
    // PRIVATE TYPES

    /// Enum of the various modes at which this component can operate
    struct Mode {
        // TYPES
        enum Enum {
            /// Use conf service, failover to disk backup.
            e_NORMAL,
            /// Skip conf service, only use disk backup.
            e_FORCE_BACKUP
        };
    };

    /// Struct to represent a configuration response entry in the cache.
    struct CacheEntry {
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(CacheEntry, bslma::UsesBslmaAllocator)

        // CREATORS

        /// Create a new object, using the specified `allocator`.  If
        /// `allocator` is 0, the currently installed default allocator is
        /// used.
        explicit CacheEntry(bslma::Allocator* allocator = 0);

        /// Create a copy of the specified `rhs` object.  Use the optionally
        /// specified `allocator` to supply memory.  If `allocator` is 0, the
        /// currently installed default allocator is used.
        CacheEntry(const CacheEntry& rhs, bslma::Allocator* allocator = 0);

        // PUBLIC DATA

        /// Data to cache.
        bsl::string d_data;
        /// Time after which this entry is no longer valid.
        bsls::TimeInterval d_expireTime;
    };

    typedef bsl::unordered_map<bsl::string,
                               CacheEntry,
                               bdlb::TransparentStringHash,
                               bsl::equal_to<> >
        CacheMap;

  private:
    // DATA
    Mode::Enum d_mode;

    bslmt::Mutex d_mutex;

    /// Cache (with a small TTL for its entries).  The key is the `domainName`
    /// for the domain config.  This cache has a small TTL and is used to
    /// prevent flooding the conf service with the same request if, for example
    /// during turnaround, lots of tasks come up at the same time and request
    /// the same domain.
    CacheMap d_cache;

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

  private:
    // PRIVATE MANIPULATORS

    /// Lookup entry with the specified `key` in the cache and fill the data in
    /// the specified `config` if found and expiry time has not yet been
    /// reached; otherwise return false and leave `config` untouched.  The
    /// behavior of this function is undefined unless `config` points to a
    /// valid string object.  Note that if the entry is found but has expired,
    /// this will erase it from the cache.
    bool cacheLookup(bsl::string* config, bsl::string_view key);

    /// Add entry with the specified `key` and with the specified `config` as
    /// data to the cache, resetting its expiry time.  Note that this will
    /// overwrite any existing entry with the specified `key` in the cache.
    void cacheAdd(bsl::string_view key, const bsl::string& config);

  private:
    // NOT IMPLEMENTED
    ConfigProvider(const ConfigProvider&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    ConfigProvider& operator=(const ConfigProvider&) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ConfigProvider, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `ConfigProvider` using the specified `allocator` for
    /// any memory allocation.
    ConfigProvider(bslma::Allocator* allocator);

    /// Destructor
    ~ConfigProvider();

    // MANIPULATORS

    /// Start this component.  Return 0 on success or a non-zero value and
    /// populate the specified `errorDescription` with a description of the
    /// error otherwise.
    int start(bsl::ostream& errorDescription);

    /// Stop this component.
    void stop();

    /// Asynchronously get the configuration for the specified `domainName`
    /// and invoke the specified `callback` with the result.
    void getDomainConfig(bsl::string_view         domainName,
                         const GetDomainConfigCb& callback);

    /// Clear all cache if the optionally specified `domainName` is not
    /// provided, else clear only the entry related to `domainName`.
    void clearCache(bsl::string_view domainName = "");

    /// Process the specified `command` and write an error message into the
    /// error object if applicable.
    int processCommand(const mqbcmd::ConfigProviderCommand& command,
                       mqbcmd::Error*                       error);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------------
// struct ConfigProvider::CacheEntry
// ---------------------------------

inline ConfigProvider::CacheEntry::CacheEntry(bslma::Allocator* allocator)
: d_data(allocator)
, d_expireTime()
{
}

inline ConfigProvider::CacheEntry::CacheEntry(
    const ConfigProvider::CacheEntry& rhs,
    bslma::Allocator*                 allocator)
: d_data(rhs.d_data, allocator)
, d_expireTime(rhs.d_expireTime)
{
}

}  // close package namespace
}  // close enterprise namespace

#endif
