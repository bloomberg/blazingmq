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

// mqbmock_queuehandle.cpp                                            -*-C++-*-
#include <mqbmock_queuehandle.h>

#include <mqbscm_version.h>
// MQB
#include <mqbcmd_messages.h>
#include <mqbi_queueengine.h>
#include <mqbi_storage.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqt_queueflags.h>

#include <bmqu_memoutstream.h>
#include <bmqu_outstreamformatsaver.h>
#include <bmqu_printutil.h>

// BDE
#include <bdlbb_blobutil.h>
#include <bsl_algorithm.h>
#include <bsl_iostream.h>
#include <bsl_numeric.h>
#include <bsl_string.h>
#include <bsl_utility.h>

namespace BloombergLP {
namespace mqbmock {

// -----------------
// class QueueHandle
// -----------------

// PRIVATE ACCESSORS
void QueueHandle::assertConsistentSubStreamInfo(const bsl::string& appId,
                                                unsigned int subQueueId) const
{
    // Consistency
    const bool isDefaultSubStream = appId ==
                                        bmqp::ProtocolUtil::k_DEFAULT_APP_ID &&
                                    subQueueId ==
                                        bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID;
    const bool isFanoutConsumerSubStream =
        appId != bmqp::ProtocolUtil::k_DEFAULT_APP_ID &&
        subQueueId != bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID;
    BSLS_ASSERT_OPT(isDefaultSubStream || isFanoutConsumerSubStream);
}

unsigned int
QueueHandle::subscription2downstreamSubQueueId(unsigned int sId) const
{
    Subscriptions::const_iterator cit = d_subscriptions.find(sId);
    unsigned int                  downstreamSubQueueId;

    if (cit == d_subscriptions.end()) {
        // To assist those tests which do not bother to configureQueue after
        // OpenQueue, assume this is about bmqp::ProtocolUtil::k_DEFAULT_APP_ID
        SubStreams::const_iterator citInfo = d_subStreamInfos.find(
            bmqp::ProtocolUtil::k_DEFAULT_APP_ID);
        BSLS_ASSERT_OPT(citInfo != d_subStreamInfos.end());

        downstreamSubQueueId = citInfo->second.d_downstreamSubQueueId;
    }
    else {
        downstreamSubQueueId = cit->second.d_downstreamSubQueueId;
    }

    return downstreamSubQueueId;
}

// CREATORS
QueueHandle::QueueHandle(
    const bsl::shared_ptr<mqbi::Queue>&                       queueSp,
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    BSLS_ANNOTATION_UNUSED mqbstat::QueueStatsDomain* domainStats,
    const bmqp_ctrlmsg::QueueHandleParameters&        handleParameters,
    bslma::Allocator*                                 allocator)
: d_handleParameters(allocator)
, d_downstreams(allocator)
, d_subStreamInfos(allocator)
, d_subscriptions(allocator)
, d_client_p(clientContext->client())
, d_queue_sp(queueSp)
, d_unconfirmedMessageMonitor(0, 0, 0, 0, 0, 0)
, d_schemaLearnerContext(
      d_queue_sp ? d_queue_sp->schemaLearner().createContext() : 0)
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queue_sp && "'queue' must not be 0");

    setHandleParameters(handleParameters);
}

QueueHandle::~QueueHandle()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_subStreamInfos.empty());
    // All resources should have been released prior to destruction of the
    // QueueHandle.
}

// MANIPULATORS
//   (virtual: mqbi::QueueHandle)
mqbi::Queue* QueueHandle::queue()
{
    return d_queue_sp.get();
}

mqbi::DispatcherClient* QueueHandle::client()
{
    return d_client_p;
}

void QueueHandle::registerSubStream(const bmqp_ctrlmsg::SubQueueIdInfo& stream,
                                    unsigned int upstreamSubQueueId,
                                    const mqbi::QueueCounts& counts)
{
    assertConsistentSubStreamInfo(stream.appId(), stream.subId());

    SubStreams::iterator it = d_subStreamInfos.find(stream.appId());

    if (it == d_subStreamInfos.end()) {
        d_downstreams.emplace(stream.subId(),
                              Downstream(upstreamSubQueueId, d_allocator_p));
        d_subStreamInfos.insert(bsl::make_pair(stream.appId(),
                                               StreamInfo(counts,
                                                          stream.subId(),
                                                          upstreamSubQueueId,
                                                          d_allocator_p)));
        return;  // RETURN
    }

    it->second.d_counts += counts;
}

void QueueHandle::registerSubscription(unsigned int downstreamSubId,
                                       unsigned int downstreamId,
                                       const bmqp_ctrlmsg::ConsumerInfo& ci,
                                       unsigned int upstreamId)
{
    Downstreams::iterator itDownstream = d_downstreams.find(downstreamSubId);
    BSLS_ASSERT_SAFE(itDownstream != d_downstreams.end());

    Subscriptions::iterator itS = d_subscriptions.find(downstreamId);

    if (itS == d_subscriptions.end()) {
        itS = d_subscriptions
                  .emplace(downstreamId,
                           Subscription(downstreamSubId, upstreamId))
                  .first;
    }

    // If maxUnconfirmedMessages == 0 or maxUnconfirmedBytes == 0, disable
    // delivery.  Otherwise, enable delivery.

    if (ci.maxUnconfirmedMessages() == 0 || ci.maxUnconfirmedBytes() == 0) {
        itDownstream->second.d_canDeliver = false;
    }
    else {
        itDownstream->second.d_canDeliver = true;
    }
}

bool QueueHandle::unregisterSubStream(const bmqp_ctrlmsg::SubQueueIdInfo& info,
                                      const mqbi::QueueCounts& counts,
                                      bool                     isFinal)
{
    SubStreams::iterator it = d_subStreamInfos.find(info.appId());

    BSLS_ASSERT_SAFE(it != d_subStreamInfos.end());

    it->second.d_counts -= counts;

    if ((0 == it->second.d_counts.d_readCount &&
         0 == it->second.d_counts.d_writeCount) ||
        isFinal) {
        d_subStreamInfos.erase(it);
        d_downstreams.erase(info.subId());
        return true;  // RETURN
    }
    return false;
}

mqbi::QueueHandle*
QueueHandle::setIsClientClusterMember(BSLS_ANNOTATION_UNUSED bool value)
{
    return this;
}

void QueueHandle::postMessage(
    BSLS_ANNOTATION_UNUSED const bmqp::PutHeader& putHeader,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<bdlbb::Blob>& appData,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<bdlbb::Blob>& options)
{
    // NOTHING
}

void QueueHandle::confirmMessage(const bmqt::MessageGUID& msgGUID,
                                 unsigned int             downstreamSubQueueId)
{
    // Update unconfirmed messages collection
    Downstreams::iterator mapIter = d_downstreams.find(downstreamSubQueueId);

    BSLS_ASSERT_OPT(mapIter != d_downstreams.end());

    GUIDMap&                guids    = mapIter->second.d_unconfirmedMessages;
    GUIDMap::const_iterator msgCiter = guids.find(msgGUID);
    if (msgCiter != guids.end()) {
        guids.erase(msgCiter);
    }

    // Inform the queue about that confirm.
    unsigned int upstreamSubQueueId = mapIter->second.d_upstreamSubQueueId;
    d_queue_sp->confirmMessage(msgGUID, upstreamSubQueueId, this);

    // Do not call onHandleUsable for it interfere with scheduled delivery
    // since the mock dispatcher executes everything in the caller thread, we
    // end up having two threads working on the same redelivery list.
}

void QueueHandle::rejectMessage(const bmqt::MessageGUID& msgGUID,
                                unsigned int             downstreamSubQueueId)
{
    // Update unconfirmed messages collection
    Downstreams::iterator mapIter = d_downstreams.find(downstreamSubQueueId);

    BSLS_ASSERT_OPT(mapIter != d_downstreams.end());

    Downstream&             downstream = mapIter->second;
    GUIDMap&                guids      = downstream.d_unconfirmedMessages;
    GUIDMap::const_iterator msgCiter   = guids.find(msgGUID);
    if (msgCiter != guids.end()) {
        guids.erase(msgCiter);
    }

    unsigned int upstreamSubQueueId = downstream.d_upstreamSubQueueId;
    // Inform the queue about that reject.
    d_queue_sp->rejectMessage(msgGUID, upstreamSubQueueId, this);

    // If can deliver, schedule a delivery by indicating to the associated
    // queue engine that this handle is now back to being usable.
    if (downstream.d_canDeliver) {
        // REVISIT: make _canDeliver apply to subscription and not subQueue
        // Iterate all subscriptions meanwhile

        for (Subscriptions::const_iterator cit = d_subscriptions.begin();
             cit != d_subscriptions.end();
             ++cit) {
            if (cit->second.d_downstreamSubQueueId == downstreamSubQueueId) {
                d_queue_sp->queueEngine()->onHandleUsable(
                    this,
                    cit->second.d_upstreamSubscriptionId);
            }
        }
    }
}

void QueueHandle::onAckMessage(
    BSLS_ANNOTATION_UNUSED const bmqp::AckMessage& ackMessage)
{
    // NOTHING
}

void QueueHandle::deliverMessage(
    const mqbi::StorageIterator& message,
    BSLS_ANNOTATION_UNUSED const bmqp::Protocol::MsgGroupId& msgGroupId,
    const bmqp::Protocol::SubQueueInfosArray&                subscriptions,
    BSLS_ANNOTATION_UNUSED bool                              isOutOfOrder)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(
        bmqt::QueueFlagsUtil::isReader(handleParameters().flags()));

    for (size_t i = 0; i < subscriptions.size(); ++i) {
        BSLS_ASSERT_OPT(canDeliver(subscriptions[i].id()));
    }

    for (bmqp::Protocol::SubQueueInfosArray::size_type i = 0;
         i < subscriptions.size();
         ++i) {
        unsigned int sId              = subscriptions[i].id();
        unsigned int downstreamSubId  = subscription2downstreamSubQueueId(sId);
        Downstreams::iterator mapIter = d_downstreams.find(downstreamSubId);
        BSLS_ASSERT_OPT(mapIter != d_downstreams.end());

        GUIDMap& guids = mapIter->second.d_unconfirmedMessages;

        bsl::pair<GUIDMap::iterator, bool> insertRC = guids.insert(
            bsl::make_pair(message.guid(),
                           bsl::make_pair(message.appData(), sId)));
        BSLS_ASSERT_OPT(insertRC.second);
        (void)insertRC;  // Compiler happiness
    }
}

void QueueHandle::deliverMessageNoTrack(
    const mqbi::StorageIterator&              message,
    const bmqp::Protocol::MsgGroupId&         msgGroupId,
    const bmqp::Protocol::SubQueueInfosArray& subscriptions)
{
    // Delegate, from a simplified mock perspective.
    deliverMessage(message, msgGroupId, subscriptions, false);
}

void QueueHandle::configure(
    const bmqp_ctrlmsg::StreamParameters&              streamParameters,
    const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb)
{
    // executed by *ANY* thread

    d_queue_sp->configureHandle(this, streamParameters, configuredCb);
}

void QueueHandle::deconfigureAll(
    const mqbi::QueueHandle::VoidFunctor& deconfiguredCb)
{
    mqbi::QueueHandle::SubStreams::const_iterator citer =
        d_subStreamInfos.begin();
    for (; citer != d_subStreamInfos.end(); ++citer) {
        bmqp_ctrlmsg::StreamParameters nullStreamParameters;
        nullStreamParameters.appId() = citer->first;

        configure(nullStreamParameters,
                  mqbi::QueueHandle::HandleConfiguredCallback());
    }
    deconfiguredCb();
}

void QueueHandle::release(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::QueueHandleParameters&
                                 handleParameters,
    BSLS_ANNOTATION_UNUSED bool  isFinal,
    BSLS_ANNOTATION_UNUSED const HandleReleasedCallback& releasedCb)
{
    // NOTHING
}

void QueueHandle::drop(bool doDeconfigure)
{
    d_queue_sp->dropHandle(this, doDeconfigure);
    // Above call may delete 'this'.
}

void QueueHandle::clearClient(bool hasLostClient)
{
    if (!hasLostClient) {
        return;  // RETURN
    }

    for (Downstreams::iterator itContext = d_downstreams.begin();
         itContext != d_downstreams.end();
         ++itContext) {
        GUIDMap& guids = itContext->second.d_unconfirmedMessages;
        for (GUIDMap::iterator msgCiter = guids.begin();
             msgCiter != guids.end();
             ++msgCiter) {
            d_queue_sp->rejectMessage(msgCiter->first,
                                      itContext->second.d_upstreamSubQueueId,
                                      this);
        }
        // guids.clear();
    }
    d_client_p = 0;
}

int QueueHandle::transferUnconfirmedMessageGUID(
    const mqbi::RedeliveryVisitor& out,
    unsigned int                   subQueueId)
{
    Downstreams::iterator mapIter = d_downstreams.find(subQueueId);
    BSLS_ASSERT_OPT(mapIter != d_downstreams.end());

    GUIDMap& guids  = mapIter->second.d_unconfirmedMessages;
    int      result = guids.size();

    if (out) {
        for (GUIDMap::const_iterator msgIter = guids.begin();
             msgIter != guids.end();
             ++msgIter) {
            out(msgIter->first);
        }
    }

    // Clear unconfirmed messages
    guids.clear();

    return result;
}

mqbi::QueueHandle* QueueHandle::setHandleParameters(
    const bmqp_ctrlmsg::QueueHandleParameters& handleParameters)
{
    d_handleParameters = handleParameters;

    return this;
}

mqbi::QueueHandle* QueueHandle::setStreamParameters(
    const bmqp_ctrlmsg::StreamParameters& streamParameters)
{
    SubStreams::iterator infoIter = d_subStreamInfos.find(
        streamParameters.appId());

    BSLS_ASSERT_SAFE(infoIter != d_subStreamInfos.end());

    // Finally, set the streamParameters
    infoIter->second.d_streamParameters = streamParameters;

    return this;
}

// MANIPULATORS
//   (specific to mqbmock::QueueHandle)
QueueHandle& QueueHandle::_setCanDeliver(bool value)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queue_sp && "Queue has not been set");

    return _setCanDeliver(bmqp::ProtocolUtil::k_DEFAULT_APP_ID, value);
}

QueueHandle& QueueHandle::_setCanDeliver(const bsl::string& appId, bool value)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queue_sp && "Queue has not been set");

    SubStreams::const_iterator citInfo = d_subStreamInfos.find(appId);
    BSLS_ASSERT_OPT(citInfo != d_subStreamInfos.end());

    unsigned int downstreamSubQueueId = citInfo->second.d_downstreamSubQueueId;
    Downstreams::iterator it = d_downstreams.find(downstreamSubQueueId);

    BSLS_ASSERT_OPT(it != d_downstreams.end());

    assertConsistentSubStreamInfo(appId,
                                  citInfo->second.d_downstreamSubQueueId);

    bool prevValue          = it->second.d_canDeliver;
    it->second.d_canDeliver = value;

    if (prevValue == false && value == true) {
        // Mimic hitting low watermark for maxUnconfirmed

        // REVISIT: make _canDeliver apply to subscription and not subQueue
        // Iterate all subscriptions meanwhile

        for (Subscriptions::const_iterator cit = d_subscriptions.begin();
             cit != d_subscriptions.end();
             ++cit) {
            if (cit->second.d_downstreamSubQueueId == downstreamSubQueueId) {
                d_queue_sp->queueEngine()->onHandleUsable(
                    this,
                    cit->second.d_upstreamSubscriptionId);
            }
        }
    }

    return *this;
}

void QueueHandle::_resetUnconfirmed(const bsl::string& appId)
{
    SubStreams::const_iterator citInfo = d_subStreamInfos.find(appId);
    BSLS_ASSERT_OPT(citInfo != d_subStreamInfos.end());

    Downstreams::iterator it = d_downstreams.find(
        citInfo->second.d_downstreamSubQueueId);

    BSLS_ASSERT_OPT(it != d_downstreams.end());

    assertConsistentSubStreamInfo(appId,
                                  citInfo->second.d_downstreamSubQueueId);

    // Clear unconfirmed messages
    it->second.d_unconfirmedMessages.clear();
}

// ACCESSORS
//   (virtual mqbi::QueueHandle)
const mqbi::DispatcherClient* QueueHandle::client() const
{
    return d_client_p;
}

const mqbi::Queue* QueueHandle::queue() const
{
    return d_queue_sp.get();
}

unsigned int QueueHandle::id() const
{
    return d_handleParameters.qId();
}

const bmqp_ctrlmsg::QueueHandleParameters&
QueueHandle::handleParameters() const
{
    return d_handleParameters;
}

const mqbi::QueueHandle::SubStreams& QueueHandle::subStreamInfos() const
{
    return d_subStreamInfos;
}

bool QueueHandle::isClientClusterMember() const
{
    return false;
}

bool QueueHandle::canDeliver(unsigned int downstreamSubscriptionId) const
{
    unsigned int downstreamSubQueueId = subscription2downstreamSubQueueId(
        downstreamSubscriptionId);

    Downstreams::const_iterator citDownstream = d_downstreams.find(
        downstreamSubQueueId);
    BSLS_ASSERT_OPT(citDownstream != d_downstreams.end());
    return citDownstream->second.d_canDeliver;
}

const bsl::vector<const mqbu::ResourceUsageMonitor*>
QueueHandle::unconfirmedMonitors(
    BSLS_ANNOTATION_UNUSED const bsl::string& appId) const
{
    bsl::vector<const mqbu::ResourceUsageMonitor*> out(d_allocator_p);
    out.push_back(&d_unconfirmedMessageMonitor);
    return out;
}

bsls::Types::Int64 QueueHandle::countUnconfirmed(unsigned int subId) const
{
    bsls::Types::Int64 result = 0;
    if (subId == bmqp::QueueId::k_UNASSIGNED_SUBQUEUE_ID) {
        for (Downstreams::const_iterator itStream = d_downstreams.begin();
             itStream != d_downstreams.end();
             ++itStream) {
            const Downstream& downstream = itStream->second;
            result += downstream.d_unconfirmedMessages.size();
        }
    }
    else {
        Downstreams::const_iterator cit = d_downstreams.find(subId);
        BSLS_ASSERT_OPT(cit != d_downstreams.end());
        result += cit->second.d_unconfirmedMessages.size();
    }
    return result;
}

void QueueHandle::loadInternals(mqbcmd::QueueHandle* out) const
{
    bmqu::MemOutStream os;
    os << d_handleParameters;
    out->parametersJson() = os.str();

    bsl::vector<mqbcmd::QueueHandleSubStream>& subStreams = out->subStreams();
    subStreams.reserve(d_subStreamInfos.size());
    for (SubStreams::const_iterator infoCiter = d_subStreamInfos.begin();
         infoCiter != d_subStreamInfos.end();
         ++infoCiter) {
        subStreams.resize(subStreams.size() + 1);
        mqbcmd::QueueHandleSubStream& subStream = subStreams.back();
        unsigned int       subId = infoCiter->second.d_downstreamSubQueueId;
        const bsl::string& appId = infoCiter->first;
        bmqp_ctrlmsg::SubQueueIdInfo subStreamInfo;

        subStreamInfo.appId() = appId;
        subStreamInfo.subId() = subId;

        subStream.subId() = subId;
        subStream.appId().makeValue(appId);

        os.reset();
        os << infoCiter->second.d_streamParameters;
        subStream.parametersJson() = os.str();

        Downstreams::const_iterator cit = d_downstreams.find(subId);

        BSLS_ASSERT_OPT(cit != d_downstreams.end());

        subStream.numUnconfirmedMessages().makeValue(
            cit->second.d_unconfirmedMessages.size());
    }
}

bmqp::SchemaLearner::Context& QueueHandle::schemaLearnerContext() const
{
    return d_schemaLearnerContext;
}

// ACCESSORS
//   (specific to mqbmock::QueueHandle)
bsl::string QueueHandle::_messages(const bsl::string& appId) const
{
    SubStreams::const_iterator citInfo = d_subStreamInfos.find(appId);

    if (citInfo == d_subStreamInfos.end()) {
        return "";  // RETURN
    }

    Downstreams::const_iterator cit = d_downstreams.find(
        citInfo->second.d_downstreamSubQueueId);

    if (cit == d_downstreams.end()) {
        return "";  // RETURN
    }

    // bsl::vector<bsl::string> msgs(d_allocator_p);
    bmqu::MemOutStream out(d_allocator_p);
    for (GUIDMap::const_iterator msgCiter =
             cit->second.d_unconfirmedMessages.begin();
         msgCiter != cit->second.d_unconfirmedMessages.end();
         ++msgCiter) {
        BSLS_ASSERT_SAFE(msgCiter->second.first &&
                         "Null shared_ptr<bdlbb::Blob> in messages map");

        if (msgCiter != cit->second.d_unconfirmedMessages.begin()) {
            out << ",";
        }

        bdlbb::BlobUtil::asciiDump(out, *(msgCiter->second.first));
    }

    return out.str();
}

int QueueHandle::_numMessages(const bsl::string& appId) const
{
    SubStreams::const_iterator citInfo = d_subStreamInfos.find(appId);

    if (citInfo == d_subStreamInfos.end()) {
        return 0;  // RETURN
    }

    Downstreams::const_iterator cit = d_downstreams.find(
        citInfo->second.d_downstreamSubQueueId);

    if (cit == d_downstreams.end()) {
        return 0;  // RETURN
    }

    return cit->second.d_unconfirmedMessages.size();
}

size_t QueueHandle::_numActiveSubstreams() const
{
    return d_subStreamInfos.size();
}

const bsl::string QueueHandle::_appIds() const
{
    bmqu::MemOutStream       out(d_allocator_p);
    bsl::vector<bsl::string> appIds(d_allocator_p);

    for (SubStreams::const_iterator citer = d_subStreamInfos.begin();
         citer != d_subStreamInfos.end();
         ++citer) {
        appIds.emplace_back(citer->first);
    }

    bsl::sort(appIds.begin(), appIds.end());

    for (size_t i = 0; i < appIds.size(); ++i) {
        if (i != 0) {
            out << ",";
        }

        if (appIds[i] == bmqp::ProtocolUtil::k_DEFAULT_APP_ID) {
            // Non-fanout "appId", mark with '-'
            out << '-';
        }
        else {
            out << appIds[i];
        }
    }

    return out.str();
}

const bmqp_ctrlmsg::StreamParameters&
QueueHandle::_streamParameters(const bsl::string& appId) const
{
    SubStreams::const_iterator itInfo = d_subStreamInfos.find(appId);

    BSLS_ASSERT_OPT(itInfo != d_subStreamInfos.end() &&
                    "'appId' not registered");

    return itInfo->second.d_streamParameters;
}

// ---------------------------
// struct QueueHandle::Context
// ---------------------------

QueueHandle::Downstream::Downstream(unsigned int      upstreamSubQueueId,
                                    bslma::Allocator* allocator)
: d_unconfirmedMessages(allocator)
, d_canDeliver(true)
, d_upstreamSubQueueId(upstreamSubQueueId)
{
    // NOTHING
}

QueueHandle::Downstream::Downstream(const Downstream& other,
                                    bslma::Allocator* allocator)
: d_unconfirmedMessages(other.d_unconfirmedMessages, allocator)
, d_canDeliver(other.d_canDeliver)
, d_upstreamSubQueueId(other.d_upstreamSubQueueId)
{
    // NOTHING
}
}

}
