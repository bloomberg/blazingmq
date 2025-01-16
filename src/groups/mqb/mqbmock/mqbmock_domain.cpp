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

// mqbmock_domain.cpp                                                 -*-C++-*-
#include <mqbmock_domain.h>

#include <mqbscm_version.h>
// MQB
#include <mqbcmd_messages.h>
#include <mqbi_queue.h>
#include <mqbstat_queuestats.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsls_annotation.h>

namespace BloombergLP {
namespace mqbmock {

// ------------
// class Domain
// ------------

// CREATORS
Domain::Domain(mqbi::Cluster* cluster, bslma::Allocator* allocator)
: d_name("mock-domain", allocator)
, d_cluster_p(cluster)
, d_domainsStatContext(
      mqbstat::DomainStatsUtil::initializeStatContext(5, allocator))
, d_statContext(
      mqbstat::QueueStatsUtil::initializeStatContextDomains(5, allocator))
// NOTE: Some test drivers require a few snapshot, hence the 'arbitrary' 5
//       used here
, d_config(allocator)
, d_capacityMeter("domain [" + d_name + "]", allocator)
, d_queues(allocator)
{
    // NOTE: Traditionally done in 'configure()' where the limits can be
    //       injected through the 'config'
    bsls::Types::Int64 messages =
        bsl::numeric_limits<bsls::Types::Int64>::max();
    bsls::Types::Int64 bytes = bsl::numeric_limits<bsls::Types::Int64>::max();

    d_capacityMeter.setLimits(messages, bytes);

    d_domainsStats.initialize(this, d_domainsStatContext.get(), allocator);
}

Domain::~Domain()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queues.empty());
}

// MANIPULATORS
int Domain::configure(BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription,
                      const mqbconfm::Domain&              config)
{
    d_config = config;

    return 0;
}

void Domain::teardown(
    BSLS_ANNOTATION_UNUSED const mqbi::Domain::TeardownCb& teardownCb)
{
    // NOTHING
}

void Domain::teardownRemove(
    BSLS_ANNOTATION_UNUSED const mqbi::Domain::TeardownCb& teardownCb)
{
    // NOTHING
}

void Domain::openQueue(
    BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
    BSLS_ANNOTATION_UNUSED const
        bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::QueueHandleParameters&
                                 handleParameters,
    BSLS_ANNOTATION_UNUSED const mqbi::Domain::OpenQueueCallback& callback)
{
    // NOTHING
}

int Domain::registerQueue(bsl::ostream&                       errorDescription,
                          const bsl::shared_ptr<mqbi::Queue>& queueSp)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queueSp && "'queue' must not be null");
    BSLS_ASSERT_SAFE(lookupQueue(0, queueSp->uri()) != 0 &&
                     "'queue' already registered with the domain");

    int rc = queueSp->configure(errorDescription,
                                false,  // isReconfigure
                                true);  // wait
    if (rc != 0) {
        return rc;  // RETURN
    }

    d_queues[queueSp->uri().queue()] = queueSp;

    return 0;
}

void Domain::unregisterQueue(mqbi::Queue* queue)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queue && "'queue' must not be null");
    BSLS_ASSERT_SAFE(lookupQueue(0, queue->uri()) == 0 &&
                     "'queue' was not registered with the domain");

    QueueMap::const_iterator it = d_queues.find(queue->uri().queue());
    if (it == d_queues.end()) {
        return;  // RETURN
    }

    d_queues.erase(it);
}

void Domain::unregisterAppId(BSLS_ANNOTATION_UNUSED const bsl::string& appId)
{
    // NOTHING
}

mqbu::CapacityMeter* Domain::capacityMeter()
{
    return &d_capacityMeter;
}

// ACCESSORS
//   (virtual: mqbi::Domain)
int Domain::processCommand(mqbcmd::DomainResult*        result,
                           const mqbcmd::DomainCommand& command)
{
    bmqu::MemOutStream os;
    os << "MockDomain::processCommand '" << command << "' not implemented!";
    result->makeError().message() = os.str();
    return -1;
}

int Domain::lookupQueue(bsl::shared_ptr<mqbi::Queue>* out,
                        const bmqt::Uri&              uri) const
{
    QueueMap::const_iterator it = d_queues.find(uri.queue());
    if (it == d_queues.end()) {
        return -1;  // RETURN
    }

    if (out) {
        // Some callers may just want to know if the queue exists and don't
        // need it returned
        *out = it->second;
    }

    return 0;
}

void Domain::loadAllQueues(
    bsl::vector<bsl::shared_ptr<mqbi::Queue> >* out) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    for (QueueMapCIter cit = d_queues.cbegin(); cit != d_queues.cend();
         ++cit) {
        out->push_back(cit->second);
    }
}

void Domain::loadAllQueues(bsl::vector<bmqt::Uri>* out) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    for (QueueMapCIter cit = d_queues.cbegin(); cit != d_queues.cend();
         ++cit) {
        out->push_back(cit->second->uri());
    }
}

const bsl::string& Domain::name() const
{
    return d_name;
}

const mqbconfm::Domain& Domain::config() const
{
    return d_config;
}

mqbstat::DomainStats* Domain::domainStats()
{
    return &d_domainsStats;
}

bmqst::StatContext* Domain::queueStatContext()
{
    return d_statContext.get();
}

mqbi::Cluster* Domain::cluster() const
{
    return d_cluster_p;
}

void Domain::loadRoutingConfiguration(
    BSLS_ANNOTATION_UNUSED bmqp_ctrlmsg::RoutingConfiguration* config) const
{
    // NOTHING
}

bool Domain::tryRemove()
{
    BSLS_ASSERT_SAFE(false && "NOT IMPLEMENTED!");
    return true;
}

bool Domain::isRemoveComplete() const
{
    BSLS_ASSERT_SAFE(false && "NOT IMPLEMENTED!");
    return true;
}

// -------------------
// class DomainFactory
// -------------------

// CREATORS
DomainFactory::DomainFactory(mqbi::Domain& domain, bslma::Allocator* allocator)
: d_domain(domain)
, d_allocator_p(allocator)
{
}

DomainFactory::~DomainFactory()
{
}

// MANIPULATORS
void DomainFactory::qualifyDomain(const bslstl::StringRef& name,
                                  const QualifiedDomainCb& callback)
{
    bmqp_ctrlmsg::Status success(d_allocator_p);
    success.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
    callback(success, name);
}

void DomainFactory::createDomain(
    BSLS_ANNOTATION_UNUSED const bsl::string& name,
    const CreateDomainCb&                     callback)
{
    bmqp_ctrlmsg::Status success(d_allocator_p);
    success.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
    callback(success, &d_domain);
}

mqbi::Domain*
DomainFactory::getDomain(BSLS_ANNOTATION_UNUSED const bsl::string& name) const
{
    return &d_domain;
}

}  // close package namespace
}  // close enterprise namespace
