// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqba_domainresolver.h                                              -*-C++-*-
#ifndef INCLUDED_MQBA_DOMAINRESOLVER
#define INCLUDED_MQBA_DOMAINRESOLVER

/// @file mqba_domainresolver.h
///
/// @brief Provide a mechanism to resolve domain and their associated cluster.
///
///
/// @bbref{mqba::DomainResolver} provides a mechanism to resolve a domain, and
/// figure out the associated cluster where that domain lives.  Resolving a
/// domain means that, for a given domain name, on a given machine, it may be
/// translated to a more qualified domain.  This is done in order to provide
/// seamless segregation between tiers for a given domain, to account for a
/// dev, alpha, beta, prod, ..., cluster.
///
/// Caching                                      {#mqba_domainresolver_caching}
/// =======
///
/// Each resolved domain response is kept in a map, along with the last
/// modification timestamp of the script and associated configuration directory
/// at the time the entry was added to the cache.  When querying to resolve a
/// domain, this component will reuse that cache entry if it's not considered
/// stale.  An entry is stale if either the last modification timestamp of the
/// script or the one of the configuration directory is different than the
/// stored value in the cache for that entry.  This means that we need to
/// verify those timestampseverytime before checking the cache.  In order to
/// minimize filesystem overhead, we don't look up the timestamps more than
/// once per minute (see `k_SCRIPT_CHECK_TTL` value in the cpp file).
///
/// Thread-safety                                 {#mqba_domainresolver_thread}
/// =============
///
/// This object is *fully thread-safe*, meaning that two threads can safely
/// call any methods on the *same* *instance* without external synchronization.
///
/// @todo Add `commandHandler`
/// @todo Add statistics.
/// @todo Eventually split domain resolution and domain location in two
///       separate entities.

// MQB
#include <mqbi_domain.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bdlb_transparentstringhash.h>
#include <bdlt_datetime.h>
#include <bsl_functional.h>
#include <bsl_ostream.h>
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
class DomainResolverCommand;
}
namespace mqbcmd {
class Error;
}

namespace mqba {

// ====================
// class DomainResolver
// ====================

/// Mechanism to resolve domain and find their associated cluster
class DomainResolver {
  public:
    // TYPES

    /// Signature of a callback method for the `locateDomain`.  On success,
    /// `status` category is `SUCCESS` and `result` contains the
    /// configuration, on error `status` contains the category, error code
    /// and description of the error.
    typedef bsl::function<void(const bmqp_ctrlmsg::Status& status,
                               const bsl::string&          result)>
        LocateDomainCb;

  private:
    // PRIVATE TYPES

    /// Structure representing the result of resolving a domain in the cache
    /// map.
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

        /// Cached resolved domain name.
        bsl::string d_name;

        /// Cached cluster name.
        bsl::string d_cluster;

        /// Last modification timestamp of the config directory at the time
        /// this data was generated.
        bdlt::Datetime d_cfgDirTimestamp;
    };

    /// Map of domain name to cache entry.
    typedef bsl::unordered_map<bsl::string,
                               CacheEntry,
                               bdlb::TransparentStringHash,
                               bsl::equal_to<> >
        CacheMap;

  private:
    // DATA

    /// Protecting the CacheMap.
    bslmt::Mutex d_mutex;

    /// Cache map
    CacheMap d_cache;

    /// Last modification timestamp of the config directory.
    bdlt::Datetime d_lastCfgDirTimestamp;

    /// Time until which the `d_lastCfgDirTimestamp` should be considered
    /// valid.
    bsls::TimeInterval d_timestampsValidUntil;

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

  private:
    // PRIVATE MANIPULATORS

    /// Update the `d_lastCfgDirTimestamp` if the check happened longer
    /// than the TTL time ago.
    void updateTimestamps();

    /// Lookup entry for the specified `domainName` in the cache, and fill the
    /// data in the specified `resolvedDomainName` and `clusterName` if found
    /// and not stale; otherwise return false and leave `resolvedDomainName`
    /// and `clusterName` untouched.  The behavior of this function is
    /// undefined unless `resolvedDomainName` points to a valid string object
    /// and `clusterName` points to a valid string object.  Note that if the
    /// entry is found but has expired, this will erase it from the cache.
    ///
    /// @attention `d_mutex` *MUST* be locked prior to calling this function.
    ///
    /// @attention The caller must call `updateScriptTimestamp()` to update the
    ///            timestamps prior to calling this method.
    bool cacheLookup(bsl::string*     resolvedDomainName,
                     bsl::string*     clusterName,
                     bsl::string_view domainName);

    /// Get the data corresponding to the specified `domainName` from the
    /// cache, or query it from the script storing the result in the cache.
    /// Return 0 on success, populating the specified `resolvedDomainName` and
    /// `clusterName` with the result, or return a non-zero value otherwise,
    /// populating the specified `errorDescription` with a description of the
    /// error otherwise.  The behavior of this function is undefined unless
    /// `resolvedDomainName` points to a valid string object and `clusterName`
    /// points to a valid string object.
    int getOrRead(bsl::ostream&    errorDescription,
                  bsl::string*     resolvedDomainName,
                  bsl::string*     clusterName,
                  bsl::string_view domainName);

    /// Uses `getOrQuery()` to retrieve data corresponding to the specified
    /// `domainName`.  Fills in the specified `resolvedDomainName` and
    /// `clusterName` with the result.  Returns `E_SUCCESS` on success or
    /// `E_UNKNOWN` otherwise.  The behavior of this function is undefined
    /// unless `resolvedDomainName` points to a valid string object and
    /// `clusterName` points to a valid string object.
    bmqp_ctrlmsg::Status getOrReadDomain(bsl::string*     resolvedDomainName,
                                         bsl::string*     clusterName,
                                         bsl::string_view domainName);

  private:
    // NOT IMPLEMENTED
    DomainResolver(const DomainResolver&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    DomainResolver& operator=(const DomainResolver&) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DomainResolver, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `DomainResolver` using the specified `allocator` for
    /// any memory allocation.
    DomainResolver(bslma::Allocator* allocator);

    /// Destructor
    ~DomainResolver();

    // MANIPULATORS

    /// Start this component, and return 0 on success, or a non-zero error
    /// code otherwise and populate the specified `errorDescription` with a
    /// description of the error.
    int start(bsl::ostream& errorDescription);

    /// Stop this component.
    void stop();

    /// Qualify the specified `domainName` and invoke the specified
    /// `callback` with the result.
    void qualifyDomain(bsl::string_view domainName,
                       const mqbi::DomainFactory::QualifiedDomainCb& callback);

    /// Resolve the location of the specified `domainName` and invoke the
    /// specified `callback` with the result.
    void locateDomain(bsl::string_view      domainName,
                      const LocateDomainCb& callback);

    /// Clear all cache if the optionally specified `domainName` is not
    /// provided, else clear only the entry related to `domainName`.
    void clearCache(bsl::string_view domainName = "");

    /// Process the specified `command`, and write an error message into the
    /// error object if applicable. Return zero on success or a nonzero
    /// value otherwise.
    int processCommand(const mqbcmd::DomainResolverCommand& command,
                       mqbcmd::Error*                       error);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------------
// struct DomainResolver::CacheEntry
// ---------------------------------

inline DomainResolver::CacheEntry::CacheEntry(bslma::Allocator* allocator)
: d_name(allocator)
, d_cluster(allocator)
, d_cfgDirTimestamp()
{
}

inline DomainResolver::CacheEntry::CacheEntry(
    const DomainResolver::CacheEntry& rhs,
    bslma::Allocator*                 allocator)
: d_name(rhs.d_name, allocator)
, d_cluster(rhs.d_cluster, allocator)
, d_cfgDirTimestamp(rhs.d_cfgDirTimestamp)
{
}

}  // close package namespace
}  // close enterprise namespace

#endif
