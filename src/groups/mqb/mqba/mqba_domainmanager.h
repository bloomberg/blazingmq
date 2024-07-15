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

// mqba_domainmanager.h                                               -*-C++-*-
#ifndef INCLUDED_MQBA_DOMAINMANAGER
#define INCLUDED_MQBA_DOMAINMANAGER

//@PURPOSE: Provide a manager for all queue domains.
//
//@CLASSES:
//  mqba::DomainManager: Manager for all queue domains.
//
//@DESCRIPTION: The 'mqba::DomainManager' provides a manager for all queue
// domains.  The 'DomainManager' exposes a factory-method 'getDomain' to
// retrieve a 'mqbi::Domain' (if already previously opened and still active),
// or will try to create and configure one using one of the registered factory
// methods.
//
/// Thread Safety
///-------------
// This component is thread safe.

// MQB
#include <mqbconfm_messages.h>
#include <mqbi_domain.h>

// BMQ
#include <bmqt_uri.h>

// MWC
#include <mwct_valueorerror.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bsl_utility.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_mutex.h>
#include <bsls_atomic.h>
#include <bsls_cpp11.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bslmt {
class Latch;
}
namespace mqbblp {
class ClusterCatalog;
}
namespace mqbcmd {
class DomainsCommand;
}
namespace mqbcmd {
class DomainsResult;
}
namespace mqbi {
class Cluster;
}
namespace mqbi {
class Dispatcher;
}
namespace mqbs {
class StorageDomain;
}
namespace mwcst {
class StatContext;
}

namespace mqba {

// FORWARD DECLARATION
class ConfigProvider;
class DomainResolver;

// ===================
// class DomainManager
// ===================

/// Manager for all queue domains.
class DomainManager BSLS_CPP11_FINAL : public mqbi::DomainFactory {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBA.DOMAINMANAGER");

  public:
    // TYPES
    typedef bsl::function<int(bsl::ostream& errorDescription,
                              bslma::ManagedPtr<mqbs::StorageDomain>* out,
                              const bsl::string&                      name,
                              const mqbconfm::StorageDefinition&      config)>
        DomainStorageCreator;

    /// Signature of a factory method to create a domain in the specified
    /// `out` and having the specified `name`, using the specified
    /// `allocator` and passing in the specified `dispatcher` and
    /// `domainStatContext`, that should be used as the parent stat context
    /// for any queue in the domain.
    typedef bsl::function<void(
        bslma::ManagedPtr<mqbi::Domain>*       out,
        const bsl::string&                     name,
        mqbi::Dispatcher*                      dispatcher,
        const bsl::shared_ptr<mqbi::Cluster>&  cluster,
        bdlbb::BlobBufferFactory*              blobBufferFactory,
        mwcst::StatContext*                    domainsStatContext,
        bslma::ManagedPtr<mwcst::StatContext>& queuesStatContext,
        bslma::Allocator*                      allocator)>
        FactoryMethod;

  private:
    // PRIVATE TYPES

    /// Shared pointer to Domain
    typedef bsl::shared_ptr<mqbi::Domain> DomainSp;

    /// Map of domains indexed by domain name
    typedef bsl::unordered_map<bsl::string, DomainSp> DomainSpMap;

    /// Map of factory methods, indexed by type name
    typedef bsl::unordered_map<bsl::string, FactoryMethod> FactoryMethodMap;

    /// Managed pointer to a domain resolver
    typedef bslma::ManagedPtr<DomainResolver> DomainResolverMp;

    /// First element is the pointer to the domain. Second element is `true`
    /// if an insert was performed or `false` otherwise.
    typedef bsl::pair<DomainSp, bool> UpsertDomainSuccess;

    struct Error;
    // A type with details on errors for 'upsertDomain()'.

    /// The return type for `upsertDomain()` might be a value or an error.
    typedef mwct::ValueOrError<UpsertDomainSuccess, Error> UpsertDomainValue;

    typedef mwct::ValueOrError<DomainSp, Error> DecodeAndUpsertValue;
    // The return type for 'decodeAndUpsert()' might be a value or an error.

  private:
    // DATA
    ConfigProvider* d_configProvider_p;
    // ConfigProvider to use, held not owned

    bdlbb::BlobBufferFactory* d_blobBufferFactory_p;
    // BlobBufferFactory to use, held not owned

    DomainResolverMp d_domainResolver_mp;
    // DomainResolver

    mqbblp::ClusterCatalog* d_clusterCatalog_p;
    // ClusterCatalog to use, held, not owned

    mwcst::StatContext* d_domainsStatContext_p;
    // Top-level stat context for all
    // domains stats

    mwcst::StatContext* d_queuesStatContext_p;
    // Top-level stat context for all
    // domains/queues stats

    mqbi::Dispatcher* d_dispatcher_p;
    // Dispatcher to use, held not owned

    mutable bslmt::Mutex d_mutex;
    // Mutex for thread-safety of this component

    DomainSpMap d_domains;
    // Map of domains

    bsls::AtomicBool d_isStarted;
    // Is the domain manager started

    bslma::Allocator* d_allocator_p;
    // Allocator to use

  private:
    // PRIVATE MANIPULATORS

    /// Callback method when a location for the specified `domain` has been
    /// received.  On success, the specified `status` category is SUCCESS
    /// and the specified `domainLocation` contains the resolved domain
    /// information, on error `status` contains the category, error code and
    /// description of the issue.  In any-case, invoke the specified
    /// `callback`.
    void onDomainResolverDomainLocatedCb(
        const bmqp_ctrlmsg::Status&                status,
        const bsl::string&                         domainLocation,
        const bsl::string&                         domain,
        const mqbi::DomainFactory::CreateDomainCb& callback);

    /// Callback method when a domain config has been received for the
    /// specified `domain` with the specified `domainLocation` from the
    /// config provider.  If the specified `configProviderStatus` is 0, the
    /// operation was successful and the specified `configProviderResult`
    /// contains the JSON text corresponding to the configuration for the
    /// domain; otherwise, `configProviderResult` contains a description of
    /// the error and the backup method is tried.  Calls the specified
    /// `callback` when done.  This is a "top-level" private manipulator and
    /// can be used whenever deemed necessary.
    void onConfigProviderDomainConfigCb(
        int                                        configProviderStatus,
        const bsl::string&                         configProviderResult,
        const bsl::string&                         domain,
        const bsl::string&                         domainLocation,
        const mqbi::DomainFactory::CreateDomainCb& callback);

    /// Initializes a domain using an empty `DomainDefinition` and then
    /// calls `upsertDomain()` to add it to the specified `domain` and
    /// `domainLocation`.  The specified `callback` is used to communicate
    /// success or failure to the caller.  This is a "top-level" private
    /// manipulator and can be used whenever deemed necessary.
    void initializeNonClusterMemberDomain(
        const bsl::string&                         domain,
        const bsl::string&                         domainLocation,
        const mqbi::DomainFactory::CreateDomainCb& callback);

    /// If the specified `status` indicates success, the specified `result`
    /// string gets decoded as JSON and it's then inserted or updated on the
    /// domain manager in accordance with the specified `domain` and
    /// `clusterName` arguments.  The specified `out` is set to an
    /// appropriate value or a suitable error in case any part of the
    /// operation fails.
    DecodeAndUpsertValue& decodeAndUpsert(DecodeAndUpsertValue* out,
                                          const int             status,
                                          const bsl::string&    result,
                                          const bsl::string&    domain,
                                          const bsl::string&    clusterName);

    /// Inserts or updates the domain given in the specified `domain` and
    /// `clusterName` to the value defined in the specified `definition`.
    /// This is an atomic operation that uses locks.  The caller can
    /// optionally specify `cluster` if already created. The specified `out`
    /// is set to a value on success.  The value is a pair that has the
    /// relevant `DomainSp` pointer as first member and a boolean as a
    /// second member, which will be `true` if a new record was inserted to
    /// the manager and `false` otherwise.  `out` will be set to suitable
    /// error values in case any part of the operation fails.
    UpsertDomainValue&
    upsertDomain(UpsertDomainValue*             out,
                 const mqbconfm::Domain&        definition,
                 const bsl::string&             domain,
                 const bsl::string&             clusterName,
                 bsl::shared_ptr<mqbi::Cluster> cluster = 0);

    /// Callback passed to the domain with the specified `domainName` when
    /// it is being torn down.  Specified `latch` is used to notify
    /// completion of the callback.
    void onDomainClosed(const bsl::string& domainName, bslmt::Latch* latch);

  private:
    // PRIVATE ACCESSORS

    /// Returns `true` if the location of the specified `domainLocation` is
    /// part of a cluster of `type` `Bloomberg`, and `false`, otherwise.
    bool isClusterMember(const bsl::string& domainLocation) const;

  private:
    // NOT IMPLEMENTED
    DomainManager(const DomainManager&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    DomainManager& operator=(const DomainManager&) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DomainManager, bslma::UsesBslmaAllocator)

  private:
    // PRIVATE CLASS METHODS

    /// Performs the specified `callback` with values provided by the
    /// specified `response` variable.  Depending on if `response` indicates
    /// a success or failure, a suitable data structure will be built and
    /// used to invoke `callback`.
    static void
    invokeCallback(const DecodeAndUpsertValue&                response,
                   const mqbi::DomainFactory::CreateDomainCb& callback);

    /// A utility function that converts the specified `value` to
    /// `DomainSp` by, effectively, discarding the second member of the
    /// `UpsertDomainSuccess` pair and keeping just the first.
    static DomainSp toDomainSp(const UpsertDomainSuccess& value);

  public:
    // CREATORS

    /// Constructor for a new `DomainManager` using the specified
    /// `blobBufferFactory`, `configProvider`, `clusterCatalog`,
    /// `dispatcher`, `statContext` and `allocator`.
    DomainManager(ConfigProvider*           configProvider,
                  bdlbb::BlobBufferFactory* blobBufferFactory,
                  mqbblp::ClusterCatalog*   clusterCatalog,
                  mqbi::Dispatcher*         dispatcher,
                  mwcst::StatContext*       domainsStatContext,
                  mwcst::StatContext*       queuesstatContext,
                  bslma::Allocator*         allocator);

    /// Destructor
    ~DomainManager() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Start the domain manager, and return 0 on success, or non-zero and
    /// populate the specified `errorDescription` on failure.
    int start(bsl::ostream& errorDescription);

    /// Stop the domain manager.
    void stop();

    /// Load into the specified `domainSp` the domain corresponding to the
    /// specified `domainName`, if found.  Return 0 on success, or a
    /// non-zero return code and fill in a description of the error in the
    /// specified `errorDescription` otherwise.
    int locateDomain(DomainSp* domain, const bsl::string& domainName);

    /// Load into the specified `domainSp` the domain corresponding to the
    /// specified `domainName`, if found. If not found then attempt to create
    /// the domain corresponding to `domainName` and load the result into the
    /// specified `domainSp`. Return 0 on sucess, or a non-zero return code
    /// on failure.
    int locateOrCreateDomain(DomainSp* domain, const bsl::string& domainName);

    /// Load into the specified `domainSp` the domain corresponding to the
    /// specified `domainName`, if found. If not found then attempt to create
    /// the domain corresponding to `domainName` and load the result into the
    /// specified `domainSp`. Return 0 on success, or a non-zero return code on
    /// failure.
    int locateOrCreateDomain(DomainSp* domain, const bsl::string& domainName);

    /// Process the specified `command` and load the result of the command
    /// in the specified `result`.  Return zero on success or a nonzero
    /// value otherwise.
    int processCommand(mqbcmd::DomainsResult*        result,
                       const mqbcmd::DomainsCommand& command);

    // MANIPULATORS
    //   (virtual: mqbi::DomainFactory)

    /// Asynchronously qualify the domain with the specified `name`,
    /// invoking the specified `callback` with the result.  Note that the
    /// `callback` may be executed directly from the caller's thread, or may
    /// be invoked asynchronously at a later time from a different thread.
    void qualifyDomain(const bslstl::StringRef&                      name,
                       const mqbi::DomainFactory::QualifiedDomainCb& callback)
        BSLS_KEYWORD_OVERRIDE;

    /// Asynchronously get the domain with the specified fully resolved
    /// `name`, invoking the specified `callback` with the result.  Note
    /// that the `callback` may be executed directly from the caller's
    /// thread, or may be invoked asynchronously at a later time from a
    /// different thread.
    void createDomain(const bsl::string&                         name,
                      const mqbi::DomainFactory::CreateDomainCb& callback)
        BSLS_KEYWORD_OVERRIDE;

    /// Synchronously get the domain with the specified `name`.  Return null
    /// if the domain has not been previously created via `createDomain`.
    mqbi::Domain*
    getDomain(const bsl::string& name) const BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
