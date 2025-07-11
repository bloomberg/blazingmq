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

// mqbi_domain.h                                                      -*-C++-*-
#ifndef INCLUDED_MQBI_DOMAIN
#define INCLUDED_MQBI_DOMAIN

//@PURPOSE: Provide an interface for a Domain and a DomainFactory.
//
//@CLASSES:
//  mqbi::Domain:        Interface for a Domain.
//  mqbi::DomainFactory: Interface for a Domain factory.
//
//@DESCRIPTION: An 'mqbi::Domain' is responsible for managing (creating,
// reusing, keeping track of) queues, their stats, and resources usage.  An
// 'mqbi::DomainFactory' is a small factory interface for a component which can
// create 'mqbi::Domain' components.
//
/// NOTE
///---------------------------
// The 'Domain' interface currently doesn't expose a 'closeQueue' method:
// 'openQueue' returns a smart pointer to a queueHandle, so the domain
// implementation can keep track of the queue clients if needed by using a
// refcount mechanism.
//
/// Thread Safety
///-------------
// Components implementing the 'mqbi::Domain' interface *MUST* be thread safe.

// MQB

#include <mqbconfm_messages.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_uri.h>

// BDE
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_managedptr.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcmd {
class DomainCommand;
}
namespace mqbcmd {
class DomainResult;
}
namespace mqbu {
class CapacityMeter;
}
namespace mqbstat {
class DomainStats;
}
namespace bmqst {
class StatContext;
}

namespace mqbi {

// FORWARD DECLARATION
class Cluster;
class DispatcherClient;
class Queue;
class QueueHandle;
class QueueHandleRequesterContext;

// ============
// class Domain
// ============

/// Interface for a Domain.
class Domain {
  public:
    // TYPES

    /// Signature of the callback function for the `teardown` method when
    /// domain with the specified `domainName` has stopped.
    typedef bsl::function<void(const bsl::string& domainName)> TeardownCb;

    /// Refer to the similar typedef in `mqbi::Cluster` for description.
    typedef bsl::shared_ptr<QueueHandle*> OpenQueueConfirmationCookie;

    /// Signature of the callback function for the `openQueue` method.  If
    /// the specified `status` is SUCCESS, then the specified `handle` is
    /// populated with the resulting queue handle to use by the callee; and
    /// the specified `openQueueResponse` contains the upstream response (if
    /// applicable, otherwise and injected response having valid routing
    /// configuration) otherwise `queueHandle` is in an undefined state and
    /// `status` contains the category, error code and description of the
    /// error that occurred.  In case of success, the specified
    /// `confirmationCookie` must be confirmed (set to `true`) by the
    /// requester (see meaning in the
    /// `mqbi::Cluster::OpenQueueConfirmationCookie` typedef).
    typedef bsl::function<void(
        const bmqp_ctrlmsg::Status&            status,
        QueueHandle*                           handle,
        const bmqp_ctrlmsg::OpenQueueResponse& openQueueResponse,
        const OpenQueueConfirmationCookie&     confirmationCookie)>
        OpenQueueCallback;

  public:
    // CREATORS

    /// Destructor
    virtual ~Domain();

    // MANIPULATORS

    /// Configure this `Domain` using the specified `config`.  Return 0 on
    /// success, or a non-zero return code and fill in a description of the
    /// error in the specified `errorDescription` otherwise.  Note that
    /// calling `configure` on an already configured domain should
    /// atomically reconfigure that domain (and all of it's queues) with the
    /// new configuration (or fail and leave the storage untouched).
    virtual int configure(bsl::ostream&           errorDescription,
                          const mqbconfm::Domain& config) = 0;

    /// Teardown this `Domain` instance and invoke the specified
    /// `teardownCb` callback when done.  This method is called during
    /// shutdown of the broker to offer Domain an opportunity to sync,
    /// serialize it's queues in a graceful manner.  Note: the domain is in
    /// charge of all the queues it owns, and hence must stop them if needs
    /// be.
    virtual void teardown(const TeardownCb& teardownCb) = 0;

    /// Teardown this `Domain` instance and invoke the specified
    /// `teardownCb` callback when done.  This method is called during
    /// DOMAIN REMOVE command to offer Domain an opportunity to
    /// sync, serialize it's queues in a graceful manner.  Note: the domain is
    /// in charge of all the queues it owns, and hence must stop them if needs
    /// be.
    virtual void teardownRemove(const TeardownCb& teardownCb) = 0;

    /// Create/Open with the specified `handleParameters` the queue having
    /// the specified `uri` for the requester client represented with the
    /// specified `clientContext`.  Invoke the specified `callback` with the
    /// result (success or failure) of the operation.  Note that `callback`
    /// may be invoked from the same thread as the caller, or from a
    /// different one.  Note also that `uri` is simply the parsed one from
    /// `handleParameters`.
    virtual void openQueue(
        const bmqt::Uri&                                    uri,
        const bsl::shared_ptr<QueueHandleRequesterContext>& clientContext,
        const bmqp_ctrlmsg::QueueHandleParameters&          handleParameters,
        const OpenQueueCallback&                            callback) = 0;

    /// Take ownership of the specified `queue`, and eventually configure
    /// it.  Return 0 on success, or a non-zero return code otherwise.
    virtual int registerQueue(const bsl::shared_ptr<Queue>& queueSp) = 0;

    /// Reverse method of `registerQueue`, invoked when the last
    /// `QueueHandle` associated to the specified `queue` has been deleted
    /// and `queue` can now safely be deleted from the domain.
    virtual void unregisterQueue(Queue* queue) = 0;

    /// Return the resource capacity meter associated to this domain, if
    /// any, or a null pointer otherwise.
    virtual mqbu::CapacityMeter* capacityMeter() = 0;

    /// Process the specified `command`, and load the result in the
    /// specified `result`.  Return zero on success, or a nonzero value
    /// otherwise.
    virtual int processCommand(mqbcmd::DomainResult*        result,
                               const mqbcmd::DomainCommand& command) = 0;

    // ACCESSORS

    /// Load into the specified `out` the queue corresponding to the
    /// specified `uri`, if found.  Return 0 on success, or a non-zero
    /// return code otherwise.
    virtual int lookupQueue(bsl::shared_ptr<Queue>* out,
                            const bmqt::Uri&        uri) const = 0;

    /// Load into the specified `out` all queues belonging to this domain.
    virtual void
    loadAllQueues(bsl::vector<bsl::shared_ptr<Queue> >* out) const = 0;

    /// Load into the specified `out` the URI of all queues belonging to
    /// this domain.
    virtual void loadAllQueues(bsl::vector<bmqt::Uri>* out) const = 0;

    /// Return the name of this domain.
    virtual const bsl::string& name() const = 0;

    /// Return the configuration of this domain.
    virtual const mqbconfm::Domain& config() const = 0;

    /// Return the `DomainStats` object associated to this Domain.
    virtual mqbstat::DomainStats* domainStats() = 0;

    /// Return the stat context associated to this Domain.
    virtual bmqst::StatContext* queueStatContext() = 0;

    /// Return the cluster associated to this Domain.
    virtual Cluster* cluster() const = 0;

    /// Load into the specified `config` the routing configuration which
    /// should be used by all queues under this domain.
    virtual void loadRoutingConfiguration(
        bmqp_ctrlmsg::RoutingConfiguration* config) const = 0;

    /// Check the state of the queues in this domain, return false if there's
    /// queues opened or opening, or if the domain is closed or closing.
    virtual bool tryRemove() = 0;

    /// Check the state of the domain, return true if the first round
    /// of DOMAINS REMOVE is completed
    virtual bool isRemoveComplete() const = 0;
};

// ===================
// class DomainFactory
// ===================

/// Interface for a Domain factory.
class DomainFactory {
  public:
    // TYPES

    /// Signature of the `getDomain` callback method: if the specified
    /// `status` is SUCCESS, then the operation was success and the
    /// specified `domain` is populated with the configured domain;
    /// otherwise `status` contains the category, error code and description
    /// of the failure.
    typedef bsl::function<void(const bmqp_ctrlmsg::Status& status,
                               mqbi::Domain*               domain)>
        CreateDomainCb;

    /// Signature of the `qualifyDomain` callback method: if the specified
    /// `status` is SUCCESS, then the operation was success and the
    /// specified `resolvedDomain` correspond to the resolved name;
    /// otherwise `status` contains the category, error code and description
    /// of the failure.
    typedef bsl::function<void(const bmqp_ctrlmsg::Status& status,
                               const bsl::string& domainQualification)>
        QualifiedDomainCb;

  public:
    // CREATORS

    /// Destructor
    virtual ~DomainFactory();

    // MANIPULATORS

    /// Asynchronously qualify the domain with the specified `name`,
    /// invoking the specified `callback` with the result.  Note that the
    /// `callback` may be executed directly from the caller's thread, or may
    /// be invoked asynchronously at a later time from a different thread.
    virtual void qualifyDomain(const bslstl::StringRef& name,
                               const QualifiedDomainCb& callback) = 0;

    /// Asynchronously get the domain with the specified `name`, invoking
    /// the specified `callback` with the result.  Note that the `callback`
    /// may be executed directly from the caller's thread, or may be invoked
    /// asynchronously at a later time from a different thread.
    virtual void createDomain(const bsl::string&    name,
                              const CreateDomainCb& callback) = 0;

    /// Synchronously get the domain with the specified `name`.  Return null
    /// if the domain has not been previously created via `createDomain`.
    virtual Domain* getDomain(const bsl::string& name) const = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
