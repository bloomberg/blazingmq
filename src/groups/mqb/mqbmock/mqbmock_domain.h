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

// mqbmock_domain.h                                                   -*-C++-*-
#ifndef INCLUDED_MQBMOCK_DOMAIN
#define INCLUDED_MQBMOCK_DOMAIN

//@PURPOSE: Provide a mock implementation of the 'mqbi::Domain' interface.
//
//@CLASSES:
//  mqbmock::Domain: mock domain implementation
//
//@DESCRIPTION: This component provides a mock implementation,
// 'mqbmock::Domain', of the 'mqbi::Domain' interface that is used to emulate
// a a real domain for testing purposes.
//
/// Notes
///------
// At the time of this writing, this component implements desired behavior for
// only those methods of the base protocols that are needed for testing
// 'mqbi::QueueEngine' concrete implementations and 'mqba::ClientSession';
// other methods a no-op and/or return bogus or error values.
//
// Additionally, the set of methods that are specific to this component is the
// minimal set required for testing concrete implementations of
// 'mqbi::QueueEngine' and 'mqba::ClientSession'.  These methods are denoted
// with a leading underscore ('_').

// MQB

#include <mqbconfm_messages.h>
#include <mqbi_domain.h>
#include <mqbstat_domainstats.h>
#include <mqbu_capacitymeter.h>

// MWC
#include <mwcst_statcontext.h>

// BDE
#include <bsl_iosfwd.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_cpp11.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqp_ctrlmsg {
class QueueHandleParameters;
}
namespace bmqp_ctrlmsg {
class QueueStreamParameters;
}
namespace bmqt {
class Uri;
}
namespace mqbcmd {
class DomainCommand;
}
namespace mqbi {
class Cluster;
}
namespace mqbi {
class Queue;
}
namespace mqbi {
class QueueHandleRequesterContext;
}
namespace mqbu {
class CapacityMeter;
}

namespace mqbmock {

// ============
// class Domain
// ============

/// Mock domain implementation of the `mqbi::Domain` inteface.
class Domain : public mqbi::Domain {
  private:
    // PRIVATE TYPES

    /// Map of queues indexed by queueName (note: this only includes the
    /// name of the queue, and not the full uri with domain or appid).
    typedef bsl::unordered_map<bsl::string, bsl::shared_ptr<mqbi::Queue> >
                                     QueueMap;
    typedef QueueMap::const_iterator QueueMapCIter;

  private:
    // DATA

    // Name of this domain
    bsl::string d_name;

    // Cluster to use by this domain Dispatcher *d_dispatcher_p Dispatcher
    // to use Stat context dedicated to this domain, to use as the parent
    // stat context for any queue in this domain Configuration for the
    // domain
    mqbi::Cluster* d_cluster_p;

    bsl::shared_ptr<mwcst::StatContext> d_domainsStatContext;

    mqbstat::DomainStats d_domainsStats;

    // Stat context dedicated to this domain, to use as the parent stat
    // context for any queue in this domain
    bsl::shared_ptr<mwcst::StatContext> d_statContext;

    // Configuration for the domain
    mqbconfm::Domain d_config;

    // Domain resource capacity meter
    mqbu::CapacityMeter d_capacityMeter;

    // Map of active queues
    QueueMap d_queues;

  private:
    // NOT IMPLEMENTED
    Domain(const Domain&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    Domain& operator=(const Domain&) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Domain, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `mqbi::MockDomain` object associated with the specified
    /// `cluster`.  Use the specified `allocator` for any memory allocation.
    Domain(mqbi::Cluster* cluster, bslma::Allocator* allocator);

    /// Destructor.  Do nothing.
    ~Domain() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbi::Domain)

    /// Configure this `Domain` using the specified `config`.  Return 0 on
    /// success, or a non-zero return code and fill in a description of the
    /// error in the specified `errorDescription` otherwise.  Note that
    /// calling `configure` on an already configured domain should
    /// atomically reconfigure that domain (and all of it's queues) with the
    /// new configuration (or fail and leave the storage untouched).
    int configure(bsl::ostream&           errorDescription,
                  const mqbconfm::Domain& config) BSLS_KEYWORD_OVERRIDE;

    /// Do some logging.
    void teardown(const Domain::TeardownCb& teardownCb) BSLS_KEYWORD_OVERRIDE;

    /// Create/Open with the specified `handleParameters` the queue having
    /// the specified `uri` for the requester client represented with the
    /// specified `clientContext`.  Invoke the specified `callback` with the
    /// result (success or failure) of the operation.  Note that `callback`
    /// may be invoked from the same thread as the caller, or from a
    /// different one.  Note also that `uri` is simply the parsed one from
    /// `parameters`.
    void openQueue(const bmqt::Uri& uri,
                   const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                                                              clientContext,
                   const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                   const mqbi::Domain::OpenQueueCallback&     callback)
        BSLS_KEYWORD_OVERRIDE;

    /// Take ownership of the specified `queue`, and eventually configure
    /// it.  Return 0 on success, or a non-zero return code otherwise,
    /// populating the specified `errorDescription` with a description of
    /// the failure.
    int registerQueue(bsl::ostream&                       errorDescription,
                      const bsl::shared_ptr<mqbi::Queue>& queueSp)
        BSLS_KEYWORD_OVERRIDE;

    /// Reverse method of `registerQueue`, invoked when the last
    /// `QueueHandle` associated to the specified `queue` has been deleted
    /// and `queue` can now safely be deleted from the domain.
    void unregisterQueue(mqbi::Queue* queue) BSLS_KEYWORD_OVERRIDE;

    /// Directly unregister the specified `appId` from this domain.
    virtual void unregisterAppId(const bsl::string& appId);

    /// Return the capacity meter associated to this domain, if any, or a
    /// null pointer otherwise.
    mqbu::CapacityMeter* capacityMeter() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual: mqbi::Domain)

    /// Process the specified `command`, and write the result to the
    /// specified `result`.  Return zero on success, or a nonzero value
    /// otherwise.
    int
    processCommand(mqbcmd::DomainResult*        result,
                   const mqbcmd::DomainCommand& command) BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `out`, if `out` is not 0, the queue
    /// corresponding to the specified `uri`, if found. Return 0 on success,
    /// or a non-zero return code otherwise.
    int lookupQueue(bsl::shared_ptr<mqbi::Queue>* out,
                    const bmqt::Uri& uri) const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `out` all queues belonging to this domain.
    void loadAllQueues(bsl::vector<bsl::shared_ptr<mqbi::Queue> >* out) const
        BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `out` all queues belonging to this domain.
    void
    loadAllQueues(bsl::vector<bmqt::Uri>* out) const BSLS_KEYWORD_OVERRIDE;

    /// Return the name of this domain.
    const bsl::string& name() const BSLS_KEYWORD_OVERRIDE;

    /// Return the configuration of this domain.
    const mqbconfm::Domain& config() const BSLS_KEYWORD_OVERRIDE;

    /// Return the `DomainStats` object associated to this Domain.
    mqbstat::DomainStats* domainStats() BSLS_KEYWORD_OVERRIDE;

    /// Return the stat context associated to this Domain.
    mwcst::StatContext* queueStatContext() BSLS_KEYWORD_OVERRIDE;

    /// Return the cluster associated to this Domain.
    mqbi::Cluster* cluster() const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `config` the routing configuration which
    /// should be used by all queues under this domain.
    void loadRoutingConfiguration(bmqp_ctrlmsg::RoutingConfiguration* config)
        const BSLS_KEYWORD_OVERRIDE;
};

// ===================
// class DomainFactory
// ===================

/// Mock implementation of the `mqbi::DomainFactory` inteface.
class DomainFactory : public mqbi::DomainFactory {
  private:
    // DATA
    mqbi::Domain& d_domain;
    // The domain object to return when
    // 'createDomain()' is called.

    bslma::Allocator* d_allocator_p;
    // Allocator to use

  private:
    // NOT IMPLEMENTED
    DomainFactory(const Domain&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    DomainFactory& operator=(const DomainFactory&) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DomainFactory, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `mqbi::MockDomainFactory` object associated with the
    /// specified `domain`.  Use the specified `allocator` for any memory
    /// allocation.
    DomainFactory(mqbi::Domain& domain, bslma::Allocator* allocator);

    /// Destructor
    ~DomainFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Asynchronously qualify the domain with the specified `name`,
    /// invoking the specified `callback` with the result.  `callback` will
    /// be executed directly from the caller's thread.
    void
    qualifyDomain(const bslstl::StringRef& name,
                  const QualifiedDomainCb& callback) BSLS_KEYWORD_OVERRIDE;

    /// Asynchronously get the domain with the specified `name`, invoking
    /// the specified `callback` with the result.  `callback` will be
    /// executed directly from the caller's thread.
    void createDomain(const bsl::string&    name,
                      const CreateDomainCb& callback) BSLS_KEYWORD_OVERRIDE;

    /// Synchronously get the domain with the specified `name`.  Return null
    /// if the domain has not been previously created via `createDomain`.
    mqbi::Domain*
    getDomain(const bsl::string& name) const BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
