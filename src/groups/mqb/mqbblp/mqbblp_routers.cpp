// Copyright 2017-2023 Bloomberg Finance L.P.
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

// mqbblp_routers.cpp                                                 -*-C++-*-
#include <mqbblp_routers.h>

#include <mqbscm_version.h>
// MQB
#include <mqbblp_queueengineutil.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbcmd_messages.h>

// BMQ
#include <bmqp_optionsview.h>
#include <bmqp_protocol.h>
#include <bmqp_queueid.h>
#include <bmqp_queueutil.h>

// MWC
#include <mwcu_blob.h>
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace mqbblp {

namespace {

/// VST to control the scope of Resolver
class ScopeExit {
    mqbblp::Routers::QueueRoutingContext& d_queue;

  public:
    // CREATORS
    ScopeExit(mqbblp::Routers::QueueRoutingContext& queue,
              const mqbi::StorageIterator*          currentMessage)
    : d_queue(queue)
    {
        d_queue.d_preader->next(currentMessage);
    }

    ~ScopeExit() { d_queue.d_preader->next(0); }

  private:
    // NOT IMPLEMENTED
    ScopeExit(const ScopeExit&);
    ScopeExit& operator=(const ScopeExit&);
};

}  // close unnamed namespace

// -----------------------
// class Routers::Consumer
// -----------------------

void Routers::Consumer::registerSubscriptions(mqbi::QueueHandle* handle)
{
    const Routers::SubscriptionList& subscriptions = d_highestSubscriptions;

    for (Routers::SubscriptionList::const_iterator it = subscriptions.begin();
         it != subscriptions.end();
         ++it) {
        const Subscription* subscription = *it;

        handle->registerSubscription(subscription->subQueueId(),
                                     subscription->d_downstreamSubscriptionId,
                                     subscription->d_ci,
                                     subscription->upstreamId());
    }
}

// =======================================
// struct Routers::MessagePropertiesReader
// =======================================

Routers::MessagePropertiesReader::MessagePropertiesReader(
    bmqp::SchemaLearner& schemaLearner,
    bslma::Allocator*    allocator)
: d_schemaLearner(schemaLearner)
, d_schemaLearnerContext(schemaLearner.createContext())
, d_properties(allocator)
, d_currentMessage_p(0)
, d_isDirty(false)
{
    // NOTHING
}

Routers::MessagePropertiesReader::~MessagePropertiesReader()
{
    // NOTHING
}

void Routers::MessagePropertiesReader::_set(
    bmqp::MessageProperties& properties)
{
    d_properties = properties;
}

bdld::Datum Routers::MessagePropertiesReader::get(const bsl::string& name,
                                                  bslma::Allocator*  allocator)
{
    if (d_isDirty) {
        if (!d_appData) {
            if (d_currentMessage_p && d_currentMessage_p->appData()) {
                d_appData = d_currentMessage_p->appData();
                d_messagePropertiesInfo =
                    d_currentMessage_p->attributes().messagePropertiesInfo();
            }
        }
        if (d_appData) {
            int rc = d_schemaLearner.read(d_schemaLearnerContext,
                                          &d_properties,
                                          d_messagePropertiesInfo,
                                          *d_appData);
            if (rc != 0) {
                BALL_LOG_TRACE << "Failed to read message schema [rc: " << rc
                               << "]";
            }
        }
        d_isDirty = false;
    }

    return d_properties.getPropertyRef(name, allocator);
}

void Routers::MessagePropertiesReader::next(
    const mqbi::StorageIterator* currentMessage)
{
    if (currentMessage == d_currentMessage_p && currentMessage) {
        return;  // RETURN
    }
    // Not loading mqbi::StorageIterator::appData to check equality

    clear();

    d_currentMessage_p = currentMessage;
}

void Routers::MessagePropertiesReader::clear()
{
    d_properties.clear();

    d_currentMessage_p = 0;
    d_appData.reset();
    d_isDirty = true;
}

void Routers::MessagePropertiesReader::next(
    const bsl::shared_ptr<bdlbb::Blob>& appData,
    const bmqp::MessagePropertiesInfo&  messagePropertiesInfo)
{
    clear();

    d_appData               = appData;
    d_messagePropertiesInfo = messagePropertiesInfo;
}

// ==========================
// struct Routers::Expression
// ==========================

bool Routers::Expression::evaluate()
{
    // Consider anything other than SimpleEvaluator as 'true'.
    if (!d_evaluator.isCompiled()) {
        return true;  // RETURN
    }

    if (d_evaluator.isValid()) {
        bool result = d_evaluator.evaluate(*d_evaluationContext_p);

        if (d_evaluationContext_p->hasError()) {
            return false;  // RETURN
        }

        return result;  // RETURN
    }

    // Consider compiled but not valid expressions as 'true'.
    return true;
}

bool Routers::PriorityGroup::evaluate(
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<bdlbb::Blob>& data)
{
    const Expressions::SharedItem it         = d_itId->value().d_itExpression;
    Expression&                   expression = it->value();

    return expression.evaluate();
}

unsigned int Routers::PriorityGroup::sId() const
{
    return d_itId->key();
}

void Routers::AppContext::loadApp(const char*        appId,
                                  mqbi::QueueHandle* handle,
                                  bsl::ostream*      errorStream,
                                  const mqbi::QueueHandle::StreamInfo& info,
                                  const AppContext* previous)
{
    BSLS_ASSERT_SAFE(appId && "Must provide 'appId'");

    const bsl::string& currentAppId = info.d_streamParameters.appId();

    if (currentAppId != appId) {
        return;  // RETURN
    }

    load(handle,
         errorStream,
         info.d_downstreamSubQueueId,
         info.d_upstreamSubQueueId,
         info.d_streamParameters,
         previous);
}

void Routers::AppContext::load(
    mqbi::QueueHandle*                    handle,
    bsl::ostream*                         errorStream,
    unsigned int                          downstreamSubQueueId,
    unsigned int                          upstreamSubQueueId,
    const bmqp_ctrlmsg::StreamParameters& streamParameters,
    const AppContext*                     previous)
{
    // Rebuild the routing for the 'handle' given the 'streamParameters'.  If
    // the same Subscription expression is reused, keep the previously
    // generated subscriptionId.  For which purpose, use 'previous' for the
    // lookups.

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(handle && "Must provide 'handle'");

    // Unique Consumer per App
    Consumers::SharedItem itConsumer = d_consumers.record(
        handle,
        Consumer(streamParameters, downstreamSubQueueId, d_allocator_p));

    // For convenience, the flag is used for correct comma ', ' printing when
    // multiple errors are logged.
    bool loggedErrors = false;

    const size_t nSubscriptions = streamParameters.subscriptions().size();
    for (size_t i = 0; i < nSubscriptions; ++i) {
        const bmqp_ctrlmsg::Subscription& config =
            streamParameters.subscriptions()[i];
        const bmqp_ctrlmsg::Expression& expr = config.expression();

        // Unique Expression per queue
        Expressions::SharedItem itExpression = d_queue.d_expressions.find(
            expr);

        if (!itExpression) {
            itExpression = d_queue.d_expressions.record(expr, Expression());

            // Resolve the expression right away

            if (bmqp_ctrlmsg::ExpressionVersion::E_VERSION_1 ==
                expr.version()) {
                Expression& expression = itExpression->value();

                if (expr.text().length()) {
                    expression.d_evaluationContext_p =
                        &d_queue.d_evaluationContext;

                    int rc = expression.d_evaluator.compile(
                        expr.text(),
                        d_compilationContext);
                    if (rc != 0 && errorStream != 0) {
                        bmqeval::ErrorType::Enum errorType =
                            static_cast<bmqeval::ErrorType::Enum>(rc);
                        if (loggedErrors) {
                            (*errorStream) << ", ";
                        }
                        loggedErrors = true;
                        (*errorStream)
                            << "[ expression: \"" << expr.text()
                            << "\", rc: " << rc << ", reason: \""
                            << bmqeval::ErrorType::toString(errorType)
                            << "\" ]";
                    }
                }
            }
        }

        // Unique Group per Expression per App
        PriorityGroups::SharedItem itGroup = d_groups.find(expr);

        if (!itGroup) {
            if (previous) {
                itGroup = previous->d_groups.find(expr);
            }

            unsigned int subscriptionId;

            if (!itGroup) {
                const mqbcfg::AppConfig& brkrCfg = mqbcfg::BrokerConfig::get();
                if (brkrCfg.brokerVersion() == bmqp::Protocol::k_DEV_VERSION ||
                    brkrCfg.configureStream()) {
                    // This must be the same as in
                    // 'ClusterQueueHelper::sendConfigureQueueRequest'

                    // Since the conversion to the old style drops the
                    // generated subscription id and uses subQueue id, MUST
                    // record subQueue id instead of generating new id.

                    subscriptionId = d_queue.nextSubscriptionId();
                }
                else {
                    subscriptionId = upstreamSubQueueId;
                }
            }
            else {
                subscriptionId = itGroup->value().sId();
            }

            const SubscriptionIds::SharedItem itId = d_queue.d_groupIds.record(
                subscriptionId,
                SubscriptionId(itExpression, upstreamSubQueueId));
            // This always adds new 'SubscriptionId' since 'PriorityGroups' has
            // stronger id uniqueness than 'Queue'.

            itGroup = d_groups.record(expr,
                                      PriorityGroup(itId, d_allocator_p));
        }

        const size_t nConsumers = config.consumers().size();

        for (size_t j = 0; j < nConsumers; ++j) {
            const bmqp_ctrlmsg::ConsumerInfo& ci       = config.consumers()[j];
            int                               priority = ci.consumerPriority();

            if (priority == bmqp::Protocol::k_CONSUMER_PRIORITY_INVALID) {
                // De-configuration should not have 'ConsumerInfo'.  But for
                // backward compatibility, handle invalid priority as an
                // indication of de-configuration.
                continue;  // CONTINUE
            }

            // Unique Priority level per App
            Priority& priorityLevel = d_priorities
                                          .emplace(priority,
                                                   Priority(d_allocator_p))
                                          .first->second;

            // Unique Subscriber per Priority level
            Subscribers::SharedItem itSubscriber =
                priorityLevel.d_subscribers.find(handle);

            if (!itSubscriber) {
                itSubscriber = priorityLevel.d_subscribers.record(
                    handle,
                    Subscriber(itConsumer, d_allocator_p));
            }
            Subscriber& subscriber = itSubscriber->value();

            // Downstream subscription has a back reference to the group.
            subscriber.d_subscriptions.emplace_back(ci,
                                                    config.sId(),
                                                    itSubscriber,
                                                    itGroup);
        }
    }
}

unsigned int Routers::AppContext::finalize()
{
    d_priorityCount = 0;

    clean();

    for (Priorities::iterator itPriority = d_priorities.begin();
         itPriority != d_priorities.end();) {
        Priority&                   level       = itPriority->second;
        BSLA_MAYBE_UNUSED const int priority    = itPriority->first;
        Subscribers&                subscribers = level.d_subscribers;
        Subscriber::Subscriptions   remove(d_allocator_p);

        for (Subscribers::const_iterator itSubscriber = subscribers.begin();
             itSubscriber != subscribers.end();
             ++itSubscriber) {
            Subscriber& subscriber = subscribers.value(itSubscriber);
            Subscriber::Subscriptions& subscriptions =
                subscriber.d_subscriptions;

            if (!subscriber.d_itConsumer->isValid()) {
                remove.splice(remove.end(), subscriptions);
                continue;  // CONTINUE
            }

            Consumer& consumer = subscriber.d_itConsumer->value();

            for (Subscriber::Subscriptions::iterator itSubscription =
                     subscriptions.begin();
                 itSubscription != subscriptions.end();
                 ++itSubscription) {
                const Subscription& subscription = *itSubscription;

                const PriorityGroups::SharedItem& itGroup =
                    subscription.d_itGroup;
                PriorityGroup& group = itGroup->value();

                BSLA_MAYBE_UNUSED const bmqp_ctrlmsg::ConsumerInfo& ci =
                    subscription.d_ci;

                BSLS_ASSERT_SAFE(priority == ci.consumerPriority());

                // Accumulate all priorities to inform upstream
                bool isFirstTime = group.add(&subscription);

                if (group.d_ci.size() == 1) {
                    int n = subscription.d_ci.consumerPriorityCount();

                    if (isFirstTime) {
                        // First time see this group Put the Group into the
                        // consumer selection list
                        level.d_highestGroups.emplace_back(itGroup);
                    }

                    // Put the subscription into the selection list
                    consumer.d_highestSubscriptions.emplace_back(
                        &subscription);

                    // Put the subscription into the round-robin list
                    group.d_highestSubscriptions.emplace_back(&subscription);

                    level.d_count += n;
                    d_priorityCount += n;
                }
                else {
                    // This 'subscription' has the 'expression' which is used
                    // by a higher priority subscription (group.d_ci[0])
                }
            }
        }
        remove.clear();
        if (level.d_subscribers.empty()) {
            itPriority = d_priorities.erase(itPriority);
        }
        else {
            ++itPriority;
        }
    }

    return d_priorityCount;
}

void Routers::AppContext::registerSubscriptions()
{
    for (Consumers::const_iterator itConsumer = d_consumers.begin();
         itConsumer != d_consumers.end();
         ++itConsumer) {
        Routers::Consumer& consumer = d_consumers.value(itConsumer);

        consumer.registerSubscriptions(itConsumer->first);
    }
}

void Routers::AppContext::clean()
{
    // Clear processing results which 'finalize' will re-populate
    for (PriorityGroups::const_iterator itGroup = d_groups.begin();
         itGroup != d_groups.end();
         ++itGroup) {
        PriorityGroup& group = d_groups.value(itGroup);
        group.d_ci.clear();
        group.d_highestSubscriptions.clear();
        group.d_canDeliver = true;
    }
    for (Consumers::const_iterator itConsumer = d_consumers.begin();
         itConsumer != d_consumers.end();
         ++itConsumer) {
        Consumer& consumer = d_consumers.value(itConsumer);
        consumer.d_highestSubscriptions.clear();
    }
}

unsigned int Routers::QueueRoutingContext::nextSubscriptionId()
{
    return ++d_nextSubscriptionId;
}

void Routers::QueueRoutingContext::loadInternals(mqbcmd::Routing* out) const
{
    // executed by the *QUEUE DISPATCHER* thread

    for (SubscriptionIds::const_iterator cit = d_groupIds.begin();
         cit != d_groupIds.end();
         ++cit) {
        mqbcmd::SubscriptionGroup outSG(d_allocator_p);
        const SubscriptionId&     sId = d_groupIds.value(cit);

        outSG.id()                 = cit->first;
        outSG.expression()         = sId.d_itExpression->key().text();
        outSG.upstreamSubQueueId() = sId.upstreamSubQueueId();

        PriorityGroup* priorityGroup = sId.d_priorityGroup;

        if (priorityGroup) {
            mqbcmd::PriorityGroup& pg = outSG.priorityGroup().makeValue();

            pg.id() = priorityGroup->sId();

            SubscriptionList& subscriptions =
                priorityGroup->d_highestSubscriptions;

            for (SubscriptionList::iterator itSubscription =
                     subscriptions.begin();
                 itSubscription != subscriptions.end();
                 ++itSubscription) {
                const Subscription* subscription = *itSubscription;
                const Subscriber&   subscriber =
                    subscription->d_itSubscriber->value();
                mqbcmd::Subscription  outSubscription;
                mqbcmd::ConsumerInfo& outCI = outSubscription.consumer();

                outSubscription.subscriber().downstreamSubQueueId() =
                    subscriber.d_itConsumer->value().d_downstreamSubQueueId;

                const bmqp_ctrlmsg::ConsumerInfo& ci = subscription->d_ci;

                outCI.consumerPriority()       = ci.consumerPriority();
                outCI.consumerPriorityCount()  = ci.consumerPriorityCount();
                outCI.maxUnconfirmedBytes()    = ci.maxUnconfirmedBytes();
                outCI.maxUnconfirmedMessages() = ci.maxUnconfirmedMessages();

                pg.highestSubscriptions().push_back(outSubscription);
            }
        }

        out->subscriptionGroups().push_back(outSG);
    }
}

void Routers::AppContext::generate(bmqp_ctrlmsg::StreamParameters* out) const
{
    BSLS_ASSERT_SAFE(out);

    size_t n = 0;  // subscriptions total

    out->subscriptions().clear();

    for (PriorityGroups::const_iterator itGroup = d_groups.begin();
         itGroup != d_groups.end();
         ++itGroup) {
        const PriorityGroup& group = d_groups.value(itGroup);

        BSLS_ASSERT_SAFE(group.d_highestSubscriptions.size() > 0);

        out->subscriptions().resize(n + 1);
        bmqp_ctrlmsg::Subscription& subscription = out->subscriptions()[n];
        ++n;

        subscription.expression() = itGroup->first;
        subscription.sId()        = group.sId();
        subscription.consumers()  = group.d_ci;
    }
}

void Routers::AppContext::reset()
{
    // Break circular dependency - Subscriber <-> Subscription
    Subscriber::Subscriptions temp(d_allocator_p);

    for (Priorities::iterator itPriority = d_priorities.begin();
         itPriority != d_priorities.end();
         ++itPriority) {
        Priority&    level       = itPriority->second;
        Subscribers& subscribers = level.d_subscribers;

        for (Subscribers::const_iterator itSubscriber = subscribers.begin();
             itSubscriber != subscribers.end();
             ++itSubscriber) {
            Subscriber& subscriber = subscribers.value(itSubscriber);

            temp.splice(temp.end(), subscriber.d_subscriptions);
        }
    }
    temp.clear();
    d_priorities.clear();
    BSLS_ASSERT_SAFE(d_groups.empty());
    BSLS_ASSERT_SAFE(d_consumers.empty());
}

Routers::Result Routers::AppContext::selectConsumer(
    const Visitor&               visitor,
    const mqbi::StorageIterator* currentMessage,
    unsigned int                 subscriptionId)
{
    BSLS_ASSERT_SAFE(currentMessage);

    PriorityGroup* group = 0;
    d_queue.d_evaluationContext.setPropertiesReader(d_queue.d_preader.get());
    ScopeExit scope(d_queue, currentMessage);

    if (subscriptionId != bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID) {
        SubscriptionIds::SharedItem itId = d_queue.d_groupIds.find(
            subscriptionId);

        if (itId) {
            // Use already selected existing Group
            group = itId->value().d_priorityGroup;
        }
    }
    if (group) {
        return d_router.iterateSubscriptions(visitor, *group)
                   ? e_SUCCESS
                   : e_NO_CAPACITY;  // RETURN
    }
    else {
        return d_router.iterateGroups(visitor, currentMessage);  // RETURN
    }
}

// -------------------------
// class Routers::RoundRobin
// -------------------------

Routers::Result
Routers::RoundRobin::iterateGroups(const Visitor&               visitor,
                                   const mqbi::StorageIterator* message)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(message);

    bool haveMatch        = false;
    bool noneHaveCapacity = true;

    for (Priorities::iterator itPriority = d_priorities.begin();
         itPriority != d_priorities.end() && !haveMatch;
         ++itPriority) {
        Priority::PriorityGroupList& groups =
            itPriority->second.d_highestGroups;

        for (Priority::PriorityGroupList::iterator itGroup = groups.begin();
             itGroup != groups.end();
             ++itGroup) {
            PriorityGroup& group = (*itGroup)->value();

            BSLS_ASSERT_SAFE(!group.d_highestSubscriptions.empty());

            if (group.d_canDeliver) {
                if (group.evaluate(message->appData())) {
                    if (iterateSubscriptions(visitor, group)) {
                        return e_SUCCESS;  // RETURN
                    }
                    group.d_canDeliver = false;
                    haveMatch          = true;
                    // Assume, no handle 'canDeliver' or delay is engaged.
                    // Do not "spill over" to lower priorities if there is a
                    // match at a higher priority.
                }
                else {
                    noneHaveCapacity = false;
                }
            }
        }
    }

    if (noneHaveCapacity) {
        return e_NO_CAPACITY_ALL;  // RETURN
    }
    else if (haveMatch) {
        return e_NO_CAPACITY;  // RETURN
    }
    else {
        return e_NO_SUBSCRIPTION;  // RETURN
    }
}

bool Routers::RoundRobin::iterateSubscriptions(const Visitor& visitor,
                                               PriorityGroup& group)
{
    SubscriptionList& subscriptions = group.d_highestSubscriptions;

    for (SubscriptionList::iterator itSubscription = subscriptions.begin();
         itSubscription != subscriptions.end();
         ++itSubscription) {
        const Subscription* subscription = *itSubscription;
        mqbi::QueueHandle*  handle       = subscription->handle();
        BSLS_ASSERT_SAFE(handle);

        if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(handle->canDeliver(
                subscription->d_downstreamSubscriptionId))) {
            if (visitor(subscription)) {
                // Before returning, move the subscription to the end if needed
                // for the round-robin.
                if (subscription->advance()) {
                    subscriptions.splice(subscriptions.end(),
                                         subscriptions,
                                         itSubscription);
                }
                return true;  // RETURN
            }
        }
    }

    return false;
}

bool Routers::AppContext::iterateConsumers(
    const Visitor&               visitor,
    const mqbi::StorageIterator* message)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(message);

    ScopeExit scope(d_queue, message);

    BSLS_ASSERT_SAFE(d_queue.d_preader.get());
    d_queue.d_evaluationContext.setPropertiesReader(d_queue.d_preader.get());

    for (Consumers::const_iterator itConsumer = d_consumers.begin();
         itConsumer != d_consumers.end();
         ++itConsumer) {
        const Routers::Subscription* subscription =
            selectSubscription(itConsumer->second.lock(), message);
        if (subscription) {
            if (visitor(subscription)) {
                return true;  // RETURN
            }
        }
    }

    return false;
}

const Routers::Subscription* Routers::AppContext::selectSubscription(
    const Consumers::SharedItem& itConsumer,
    const mqbi::StorageIterator* message) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(message);

    const mqbi::QueueHandle* handle        = itConsumer->key();
    const Consumer&          consumer      = itConsumer->value();
    const SubscriptionList&  subscriptions = consumer.d_highestSubscriptions;

    for (SubscriptionList::const_iterator it = subscriptions.begin();
         it != subscriptions.end();
         ++it) {
        const Subscription* subscription = *it;

        if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(handle->canDeliver(
                subscription->d_downstreamSubscriptionId))) {
            PriorityGroup& group = subscription->d_itGroup->value();
            if (group.evaluate(message->appData())) {
                return subscription;  // RETURN
            }
        }
    }

    // No subscriptions were able to process that message, or the handle is
    // busy.

    return 0;
}

void Routers::AppContext::apply()
{
    for (PriorityGroups::const_iterator itGroup = d_groups.begin();
         itGroup != d_groups.end();
         ++itGroup) {
        PriorityGroup& group = d_groups.value(itGroup);

        group.d_itId->value().d_priorityGroup = &group;
    }
}

void Routers::AppContext::loadInternals(mqbcmd::RoundRobinRouter* out) const
{
    for (Priorities::const_iterator itPriority = d_priorities.begin();
         itPriority != d_priorities.end();
         ++itPriority) {
        const Subscribers& subscribers =
            d_priorities.begin()->second.d_subscribers;

        for (Subscribers::const_iterator itSubscriber = subscribers.begin();
             itSubscriber != subscribers.end();
             ++itSubscriber) {
            const Subscriber& subscriber = subscribers.value(itSubscriber);
            const Subscriber::Subscriptions& subscriptions =
                subscriber.d_subscriptions;

            for (Subscriber::Subscriptions::const_iterator it =
                     subscriptions.begin();
                 it != subscriptions.end();
                 ++it) {
                const Subscription& subscription = *it;
                out->consumers().resize(out->consumers().size() + 1);
                mqbcmd::RouterConsumer& rc = out->consumers().back();

                rc.count()      = subscription.d_ci.consumerPriorityCount();
                rc.priority()   = subscription.d_ci.consumerPriority();
                rc.expression() = subscription.expression();

                mqbcmd::QueueHandle& queueHandle = rc.queueHandle();
                if (subscription.handle()->client()) {
                    queueHandle.clientDescription() =
                        subscription.handle()->client()->description();
                }
                else {
                    queueHandle.clientDescription() = "<no client>";
                }
            }
        }
    }
}

void Routers::RoundRobin::print(bsl::ostream& os,
                                int           level,
                                int           spacesPerLevel) const
{
    os << mwcu::PrintUtil::indent(level, spacesPerLevel) << "Consumers: "
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "-----------------";

    for (Priorities::const_iterator itPriority = d_priorities.begin();
         itPriority != d_priorities.end();
         ++itPriority) {
        const Subscribers& subscribers =
            d_priorities.begin()->second.d_subscribers;

        for (Subscribers::const_iterator itSubscriber = subscribers.begin();
             itSubscriber != subscribers.end();
             ++itSubscriber) {
            const Subscriber& subscriber = subscribers.value(itSubscriber);
            const Subscriber::Subscriptions& subscriptions =
                subscriber.d_subscriptions;

            for (Subscriber::Subscriptions::const_iterator it =
                     subscriptions.begin();
                 it != subscriptions.end();
                 ++it) {
                const Subscription& subscription = *it;

                os << mwcu::PrintUtil::newlineAndIndent(level + 1,
                                                        spacesPerLevel)
                   << "priority .........: "
                   << subscription.d_ci.consumerPriorityCount()
                   << mwcu::PrintUtil::newlineAndIndent(level + 1,
                                                        spacesPerLevel)
                   << "count ............: "
                   << subscription.d_ci.consumerPriorityCount()
                   << mwcu::PrintUtil::newlineAndIndent(level + 1,
                                                        spacesPerLevel)
                   << "expression .......:"
                   << subscription.d_itGroup->value()
                          .d_itId->value()
                          .d_itExpression->key()
                          .text()
                   << mwcu::PrintUtil::newlineAndIndent(level + 1,
                                                        spacesPerLevel)
                   << "client---- .......:"
                   << subscriber.d_itConsumer->key()->client()->description();
            }
        }
    }

    os << "\n";
}

}  // close package namespace
}  // close enterprise namespace
