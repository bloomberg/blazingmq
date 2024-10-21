// Copyright 2016-2023 Bloomberg Finance L.P.
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

// mqbblp_queuehandlecatalog.cpp                                      -*-C++-*-
#include <mqbblp_queuehandlecatalog.h>

#include <mqbscm_version.h>
// MQB
#include <mqbblp_queuehandle.h>
#include <mqbcmd_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_domain.h>
#include <mqbi_queue.h>

// BMQ
#include <bmqp_queueutil.h>
#include <bmqt_queueflags.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_print.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbblp {

namespace {

// ==========================
// class DefaultHandleFactory
// ==========================

/// Provide an implementation of the `mqbi::QueueHandleFactory` creating
/// `mqbblp::QueueHandle` objects.
class DefaultHandleFactory : public mqbi::QueueHandleFactory {
  public:
    // CREATOR

    /// Destructor.
    ~DefaultHandleFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATOR
    mqbi::QueueHandle*
    makeHandle(const bsl::shared_ptr<mqbi::Queue>& queue,
               const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                                                          clientContext,
               mqbstat::QueueStatsDomain*                 stats,
               const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
               bslma::Allocator* allocator) BSLS_KEYWORD_OVERRIDE;
    // Create a new handle, using the specified 'allocator', for the
    // specified 'queue' as requested by the specified 'clientContext' with
    // the specified 'parameters', and associated wit the specified
    // 'stats'.
};

// --------------------------
// class DefaultHandleFactory
// --------------------------

DefaultHandleFactory::~DefaultHandleFactory()
{
    // NOTHING
}

mqbi::QueueHandle* DefaultHandleFactory::makeHandle(
    const bsl::shared_ptr<mqbi::Queue>&                       queue,
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    mqbstat::QueueStatsDomain*                                stats,
    const bmqp_ctrlmsg::QueueHandleParameters&                handleParameters,
    bslma::Allocator*                                         allocator)
{
    return new (*allocator) mqbblp::QueueHandle(queue,
                                                clientContext,
                                                stats,
                                                handleParameters,
                                                allocator);
}

}  // close unnamed namespace

// ------------------------
// class QueueHandleCatalog
// ------------------------

void QueueHandleCatalog::queueHandleDeleter(mqbi::QueueHandle* handle)
{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(handle);

    // Notify cluster state about handle being destroyed.
    d_queue_p->domain()->cluster()->onQueueHandleDestroyed(d_queue_p,
                                                           d_queue_p->uri());

    // Delete the handle.  This must be the last thing to be done in this
    // routine, as it may end up deleting the queue instance as well, in case
    // this was the last handle to contain the queue's shared ptr (note that
    // queue's entry may already have been purged from the cluster state).
    d_allocator_p->deleteObject(handle);
}

QueueHandleCatalog::QueueHandleCatalog(mqbi::Queue*      queue,
                                       bslma::Allocator* allocator)
: d_queue_p(queue)
, d_handleFactory_mp(new(*allocator) DefaultHandleFactory(), allocator)
, d_handles(allocator)
, d_allocator_p(allocator)
{
    // NOTHING
}

QueueHandleCatalog::~QueueHandleCatalog()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_handles.empty());
}

QueueHandleCatalog& QueueHandleCatalog::setHandleFactory(
    bslma::ManagedPtr<mqbi::QueueHandleFactory>& factory)
{
    d_handleFactory_mp = factory;

    return *this;
}

mqbi::QueueHandle* QueueHandleCatalog::createHandle(
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    const bmqp_ctrlmsg::QueueHandleParameters&                handleParameters,
    mqbstat::QueueStatsDomain*                                stats)
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queue_p->dispatcher()->inDispatcherThread(d_queue_p));
    BSLS_ASSERT_SAFE(clientContext->client());

    // Retrieve queueSp from the domain, to be passed to the queue handle.
    bsl::shared_ptr<mqbi::Queue> queueSp;
    int rc = d_queue_p->domain()->lookupQueue(&queueSp, d_queue_p->uri());
    BSLS_ASSERT_SAFE(0 == rc);
    BSLS_ASSERT_SAFE(queueSp);
    static_cast<void>(rc);  // Suppress compiler warning

    // Extract the canonical handle parameters
    bmqp_ctrlmsg::QueueHandleParameters canonicalHandleParams;
    bmqp::QueueUtil::extractCanonicalHandleParameters(&canonicalHandleParams,
                                                      handleParameters);

    // Create the new handle
    mqbi::QueueHandle* handle = d_handleFactory_mp->makeHandle(
        queueSp,
        clientContext,
        stats,
        canonicalHandleParams,
        d_allocator_p);
    handle->setIsClientClusterMember(clientContext->isClusterMember());

    bsl::shared_ptr<mqbi::QueueHandle> queueHandleSp(
        handle,
        bdlf::BindUtil::bind(&QueueHandleCatalog::queueHandleDeleter,
                             this,
                             bdlf::PlaceHolders::_1),  // handle
        d_allocator_p);

    // Insert handle into map
    bsl::pair<HandleMap::iterator, HandleMap::InsertResult> insertResult =
        d_handles.insert(handle,
                         bsl::make_pair(clientContext->requesterId(),
                                        handleParameters.qId()),
                         queueHandleSp);

    BSLS_ASSERT_SAFE(insertResult.second == HandleMap::e_INSERTED);
    (void)insertResult;  // Compiler happiness

    return handle;
}

int QueueHandleCatalog::releaseHandleHelper(
    bsl::shared_ptr<mqbi::QueueHandle>*        handleSp,
    bsls::Types::Uint64*                       lostFlags,
    mqbi::QueueHandle*                         handle,
    const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
    bool                                       isFinal)
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queue_p->dispatcher()->inDispatcherThread(d_queue_p));

    HandleMap::iterator handleIt = d_handles.findByKey1(handle);
    if (handleIt == d_handles.end()) {
        BALL_LOG_ERROR << "#QUEUE_IMPROPER_BEHAVIOR "
                       << d_queue_p->description()
                       << ": releasing an unknown handle [handle parameters: "
                       << handleParameters << "]";
        BSLS_ASSERT_SAFE(false && "Release of invalid handle");
        return -1;  // RETURN
    }

    bmqp_ctrlmsg::QueueHandleParameters currentHandleParameters =
        handle->handleParameters();
    bsls::Types::Uint64 oldFlags = currentHandleParameters.flags();

    // Update queue's aggregated parameters
    bmqp::QueueUtil::subtractHandleParameters(&currentHandleParameters,
                                              handleParameters,
                                              isFinal);

    handle->setHandleParameters(currentHandleParameters);

    *handleSp  = handleIt->value();
    *lostFlags = bmqt::QueueFlagsUtil::removals(
        oldFlags,
        currentHandleParameters.flags());

    const bool allCountsZero = bmqp::QueueUtil::isEmpty(
        currentHandleParameters);

    const RequesterKey requesterKey = handleIt->key2();

    // Check if the handle still represents a valid downstream client
    if (allCountsZero || isFinal) {
        // This release corresponds to the last downstream client of this
        // queue; we can now delete the handle.
        BALL_LOG_INFO << d_queue_p->description() << ": deleting handle "
                      << "[requester: " << requesterKey.first << ":"
                      << requesterKey.second
                      << ", lastReleased handle parameters: "
                      << handleParameters << "]";
        d_handles.erase(handleIt);
        *lostFlags = oldFlags;  // Handle is deleted so all flags are lost.
    }

    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM
            << d_queue_p->description() << ": reconfigured handle "
            << "[requester: " << requesterKey.first << ":"
            << requesterKey.second
            << ", removed handle parameters: " << handleParameters
            << ", isFinal: " << isFinal
            << ", remaining handle parameters: " << currentHandleParameters
            << "], lostFlags: '";
        bmqt::QueueFlagsUtil::prettyPrint(BALL_LOG_OUTPUT_STREAM, *lostFlags);
        BALL_LOG_OUTPUT_STREAM << "'";
    }

    return 0;
}

int QueueHandleCatalog::handlesCount() const
{
    return d_handles.size();
}

bool QueueHandleCatalog::hasHandle(const mqbi::QueueHandle* handle) const
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queue_p->dispatcher()->inDispatcherThread(d_queue_p));

    return d_handles.findByKey1(const_cast<mqbi::QueueHandle*>(handle)) !=
           d_handles.end();
}

mqbi::QueueHandle* QueueHandleCatalog::getHandleByRequester(
    const mqbi::QueueHandleRequesterContext& context,
    int                                      queueId) const
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queue_p->dispatcher()->inDispatcherThread(d_queue_p));

    HandleMap::iterator it = d_handles.findByKey2(
        bsl::make_pair(context.requesterId(), queueId));
    return (it != d_handles.end()) ? it->value().get() : 0;
}

void QueueHandleCatalog::loadHandles(
    bsl::vector<mqbi::QueueHandle*>* out) const
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queue_p->dispatcher()->inDispatcherThread(d_queue_p));

    out->reserve(d_handles.size());

    for (HandleMap::iterator it = d_handles.begin(); it != d_handles.end();
         ++it) {
        out->push_back(it->key1());
    }
}

void QueueHandleCatalog::iterateConsumers(const Visitor& visitor) const
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queue_p->dispatcher()->inDispatcherThread(d_queue_p));

    for (HandleMap::iterator itHandle = d_handles.begin();
         itHandle != d_handles.end();
         ++itHandle) {
        mqbi::QueueHandle* handle(itHandle->value().get());

        if (!bmqt::QueueFlagsUtil::isReader(
                handle->handleParameters().flags())) {
            continue;  // CONTINUE
        }
        for (mqbi::QueueHandle::SubStreams::const_iterator citer =
                 handle->subStreamInfos().begin();
             citer != handle->subStreamInfos().end();
             ++citer) {
            const bmqp_ctrlmsg::StreamParameters& parameters =
                citer->second.d_streamParameters;

            if (parameters.subscriptions().size() > 0) {
                visitor(handle, citer->second);
            }
        }
    }
}

void QueueHandleCatalog::loadInternals(
    bsl::vector<mqbcmd::QueueHandle>* out) const
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queue_p->dispatcher()->inDispatcherThread(d_queue_p));

    out->reserve(d_handles.size());
    for (HandleMap::iterator it = d_handles.begin(); it != d_handles.end();
         ++it) {
        out->resize(out->size() + 1);
        mqbi::QueueHandle* handle = it->key1();
        bmqu::MemOutStream description;
        description << handle << "  ~ " << handle->client()->description();
        out->back().clientDescription() = description.str();
        handle->loadInternals(&out->back());
    }
}

bsls::Types::Int64 QueueHandleCatalog::countUnconfirmed() const
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queue_p->dispatcher()->inDispatcherThread(d_queue_p));

    bsls::Types::Int64 result = 0;

    for (HandleMap::const_iterator cit = d_handles.begin();
         cit != d_handles.end();
         ++cit) {
        const mqbi::QueueHandle* handle(cit->value().get());

        result += handle->countUnconfirmed(
            bmqp::QueueId::k_UNASSIGNED_SUBQUEUE_ID);
    }
    return result;
}

}  // close package namespace
}  // close enterprise namespace
