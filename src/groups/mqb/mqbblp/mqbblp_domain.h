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

// mqbblp_domain.h                                                    -*-C++-*-
#ifndef INCLUDED_MQBBLP_DOMAIN
#define INCLUDED_MQBBLP_DOMAIN

/// @file mqbblp_domain.h
///
/// @brief Provide a concrete implementation of the @bbref{mqbi::Domain}
/// interface.
///
/// @bbref{mqbblp::Domain} is a concrete implementation of the
/// @bbref{mqbi::Domain} interface.

// MQB
#include <mqbc_clusterstate.h>
#include <mqbconfm_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_domain.h>
#include <mqbi_storage.h>
#include <mqbstat_domainstats.h>
#include <mqbu_capacitymeter.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_uri.h>

// BDE
#include <bdlb_nullablevalue.h>
#include <bdlbb_blob.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_mutex.h>
#include <bslmt_semaphore.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_keyword.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcmd {
class DomainCommand;
}
namespace mqbcmd {
class DomainResult;
}
namespace mqbcmd {
class PurgeQueueResult;
}
namespace mqbi {
class Dispatcher;
}
namespace mqbi {
class QueueHandle;
}
namespace mqbi {
class Queue;
}
namespace bmqst {
class StatContext;
}

namespace mqbblp {

// ============
// class Domain
// ============

/// Domain implementation
class Domain BSLS_KEYWORD_FINAL : public mqbi::Domain {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.DOMAIN");

  private:
    // PRIVATE TYPES

    /// Map of queues indexed by queueName (note: this only includes the
    /// name of the queue, and not the full uri with domain or appid).
    typedef bsl::unordered_map<bsl::string, bsl::shared_ptr<mqbi::Queue> >
                                     QueueMap;
    typedef QueueMap::iterator       QueueMapIter;
    typedef QueueMap::const_iterator QueueMapCIter;

    typedef mqbc::ClusterState::AppInfos AppInfos;
    typedef AppInfos::const_iterator AppInfosCIter;

    enum DomainState {
        e_STARTED  = 0,
        e_STOPPING = 1,
        e_STOPPED  = 2,
        /// Indicate the start of the first round of DOMAINS REMOVE
        e_REMOVING = 3
    };

  private:
    // DATA

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

    /// State of the domain.  Must be one of the values from `enum
    /// DomainState`.  This variable is atomic so that we don't need to acquire
    /// `d_mutex` before accessing it.
    bsls::AtomicInt d_state;

    /// Name of this domain.
    bsl::string d_name;

    /// Configuration for the domain.
    /// Read/write thread safe only from CLUSTER dispatcher thread.
    bsl::shared_ptr<mqbconfm::Domain> d_config_sp;

    /// Cluster to use by this domain.
    bsl::shared_ptr<mqbi::Cluster> d_cluster_sp;

    /// Dispatcher to use.
    mqbi::Dispatcher* d_dispatcher_p;

    /// Blob buffer factory to use.
    bdlbb::BlobBufferFactory* d_blobBufferFactory_p;

    /// Stat context dedicated to this domain, to use as the parent stat
    /// context for any domain in this domain.
    bmqst::StatContext* d_domainsStatContext_p;

    /// Statistics of the domain.
    mqbstat::DomainStats d_domainsStats;

    /// Stat context dedicated to this domain, to use as the parent stat
    /// context for any queue in this domain.
    bslma::ManagedPtr<bmqst::StatContext> d_queuesStatContext_mp;

    /// Domain resource capacity meter.
    mqbu::CapacityMeter d_capacityMeter;

    /// Map of active queues.
    QueueMap d_queues;

    /// Number of pending requests (i.e., openQueue requests sent to the
    /// cluster, but for which a response hasn't yet been received).
    bsls::AtomicInt d_pendingRequests;

    /// Callback to be invoked when all queues in this domain have been
    /// destroyed on shutdown.  This callback is non-null only if `d_state ==
    /// DomainStats::e_STOPPING`.
    mqbi::Domain::TeardownCb d_teardownCb;

    /// Callback to be invoked when all queues in this domain have been
    /// destroyed on domain deletion.  This callback is non-null only if
    /// `d_state == DomainStats::e_REMOVING`.
    mqbi::Domain::TeardownCb d_teardownRemoveCb;

    /// Mutex for protecting the queues map.
    mutable bslmt::Mutex d_mutex;

  private:
    // PRIVATE MANIPULATORS

    /// Method invoked in response to the `openQueue()` request sent to the
    /// cluster.  If the specified `status` is SUCCESS, the operation was a
    /// success and the specified `queue` contains the resulting queue;
    /// otherwise `status` contains the category, error code and description
    /// of the failure.  The specified `clientContext` and `parameters` are
    /// the ones that were provided by the caller of the open queue request.
    /// Invoke the specified `callback` with the specified
    /// `confirmationCookie` to propagate the result to the requester.
    void onOpenQueueResponse(
        const bmqp_ctrlmsg::Status&                       status,
        mqbi::QueueHandle*                                queuehandle,
        const bmqp_ctrlmsg::OpenQueueResponse&            openQueueResponse,
        const mqbi::Cluster::OpenQueueConfirmationCookie& confirmationCookie,
        const mqbi::Domain::OpenQueueCallback&            callback);

    // PRIVATE MANIPULATORS
    //   (virtual: mqbc::ClusterStateObserver)

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    Domain(const Domain&);             // = delete;
    Domain& operator=(const Domain&);  // = delete;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Domain, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor of a new object having the specified `name` and
    /// associated with the specified `cluster`.  Use the specified
    /// `dispatcher` and `blobBufferFactory`.  The specified `statContext`
    /// is the domains root stat context under which this domain should
    /// register its own statistics.  Use the specified `allocator` for
    /// creation of the object.
    Domain(const bsl::string&                     name,
           mqbi::Dispatcher*                      dispatcher,
           bdlbb::BlobBufferFactory*              blobBufferFactory,
           const bsl::shared_ptr<mqbi::Cluster>&  cluster,
           bmqst::StatContext*                    domainsStatContext,
           bslma::ManagedPtr<bmqst::StatContext>& queuesStatContext,
           bslma::Allocator*                      allocator);

    /// Destructor
    ~Domain() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Configure this `Domain` using the specified `config`.  Return 0 on
    /// success, or a non-zero return code and fill in a description of the
    /// error in the specified `errorDescription` otherwise.  Note that
    /// calling `configure` on an already configured domain should
    /// atomically reconfigure that domain (and all of it's queues) with the
    /// new configuration (or fail and leave the storage untouched).
    int configure(bsl::ostream&           errorDescription,
                  const mqbconfm::Domain& config) BSLS_KEYWORD_OVERRIDE;

    /// Teardown this `Domain` instance and invoke the specified
    /// `teardownCb` callback when done.  This method is called during
    /// shutdown of the broker to offer Domain an opportunity to sync,
    /// serialize it's queues in a graceful manner.  Note: the domain is in
    /// charge of all the queues it owns, and hence must stop them if needs
    /// be.
    void
    teardown(const mqbi::Domain::TeardownCb& teardownCb) BSLS_KEYWORD_OVERRIDE;

    /// Teardown this `Domain` instance and invoke the specified
    /// `teardownCb` callback when done.  This method is called during
    /// DOMAIN REMOVE command to offer Domain an opportunity to
    /// sync, serialize it's queues in a graceful manner.  Note: the domain is
    /// in charge of all the queues it owns, and hence must stop them if needs
    /// be.
    void teardownRemove(const TeardownCb& teardownCb) BSLS_KEYWORD_OVERRIDE;

    /// Create/Open with the specified `handleParameters` the queue having
    /// the specified `uri` for the requester client represented with the
    /// specified `clientContext`.  Invoke the specified `callback` with the
    /// result (success or failure) of the operation.  Note that `callback`
    /// may be invoked from the same thread as the caller, or from a
    /// different one.  Note also that `uri` is simply the parsed one from
    /// `handleParameters`.
    void openQueue(const bmqt::Uri& uri,
                   const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                                                              clientContext,
                   const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                   const mqbi::Domain::OpenQueueCallback&     callback)
        BSLS_KEYWORD_OVERRIDE;

    /// Take ownership of the specified `queue`.  Return 0 on success, or a
    /// non-zero return code otherwise.
    int registerQueue(const bsl::shared_ptr<mqbi::Queue>& queueSp)
        BSLS_KEYWORD_OVERRIDE;

    /// Reverse method of `registerQueue`, invoked when the last
    /// `QueueHandle` associated to the specified `queue` has been deleted
    /// and `queue` can now safely be deleted from the domain.
    void unregisterQueue(mqbi::Queue* queue) BSLS_KEYWORD_OVERRIDE;

    /// Return the resource capacity meter associated to this domain, if
    /// any, or a null pointer otherwise.
    mqbu::CapacityMeter* capacityMeter() BSLS_KEYWORD_OVERRIDE;

    /// Process the specified `command`, and load the result in the
    /// specified `result`.  Return zero on success, or a nonzero value
    /// otherwise.
    int
    processCommand(mqbcmd::DomainResult*        result,
                   const mqbcmd::DomainCommand& command) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Load into the specified `out` the queue corresponding to the
    /// specified `uri`, if found.  Return 0 on success, or a non-zero
    /// return code otherwise.
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

    /// @return the configuration of this domain, or an empty pointer if the
    ///         domain is not configured.
    /// THREAD: safe to access only from CLUSTER dispatcher thread.
    const bsl::shared_ptr<mqbconfm::Domain>&
    config() const BSLS_KEYWORD_OVERRIDE;

    /// Return the `DomainStats` object associated to this Domain.
    mqbstat::DomainStats* domainStats() BSLS_KEYWORD_OVERRIDE;

    /// Return the stat context associated to this Domain.
    bmqst::StatContext* queueStatContext() BSLS_KEYWORD_OVERRIDE;

    /// Return the cluster associated to this Domain.
    mqbi::Cluster* cluster() const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `config` the routing configuration which
    /// should be used by all queues under this domain.
    void loadRoutingConfiguration(bmqp_ctrlmsg::RoutingConfiguration* config)
        const BSLS_KEYWORD_OVERRIDE;

    /// Check the state of the queues in this domain, return false if there's
    /// queues opened or opening, or if the domain is closed or closing.
    bool tryRemove() BSLS_KEYWORD_OVERRIDE;

    /// Check the state of the domain, return true if the first round
    /// of DOMAINS REMOVE is completed
    bool isRemoveComplete() const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------
// class Domain
// ------------

inline mqbu::CapacityMeter* Domain::capacityMeter()
{
    if (d_cluster_sp->isRemote()) {
        // No domain resource monitoring for remote domains
        return 0;  // RETURN
    }

    return &d_capacityMeter;
}

inline const bsl::string& Domain::name() const
{
    return d_name;
}

inline const bsl::shared_ptr<mqbconfm::Domain>& Domain::config() const
{
    return d_config_sp;
}

inline mqbstat::DomainStats* Domain::domainStats()
{
    return &d_domainsStats;
}

inline bmqst::StatContext* Domain::queueStatContext()
{
    return d_queuesStatContext_mp.get();
}

inline mqbi::Cluster* Domain::cluster() const
{
    return d_cluster_sp.get();
}

}  // close package namespace
}  // close enterprise namespace

#endif
