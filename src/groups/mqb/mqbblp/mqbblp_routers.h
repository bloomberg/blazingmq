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

// mqbblp_routers.h                                                   -*-C++-*-
#ifndef INCLUDED_MQBBLP_ROUTERS
#define INCLUDED_MQBBLP_ROUTERS

//@PURPOSE: Abstracts the message-routing related infrastructure.
//
//@CLASSES:
//  mqbblp::Routers: Data hierarchy to support data (PUSH) routing by Queue
//  engines.  Including support for Topic Based Routing.
//
//@DESCRIPTION:
//  Here is the relationship between major Topic Based Routing concepts:
//         ---------------------------------------------------
//        |                                                   |
//       1|                                              many V
//        |                                                   V
//     SubStream <<---------->> QueueHandle <<---------->> Subscription
//                many-to-many               many-to-many
//
//  A QueueEngine builds Routing data hierarchy upon processing ConfigureStream
//  request.  The topmost structure is 'QueueRoutingContext' holding the
//  context shared by all SubStreams, followed by one 'AppContext' per each
//  SubStream.  To build all routing structures, call 'AppContext::load' for
//  each consumer, followed by one 'AppContext::finalize' call afterwards.
//  'AppContext::generate' then generates 'bmqp_ctrlmsg::StreamParameters' to
//  communicate upstream.
//  To route next PUSH message, call 'AppContext::selectConsumer' which
//  iterates all matching subscriptions until the specified visitor returns
//  'false'.
//  QueueEngine routes to highest priority _subscription_ consumers only.  Note
//  that different subscription may have different priority and that the same
//  subscription expression can be used with different priorities.
//  For example,
//  handle1: [  subscription1: {expression1, priority: 1},
//              subscription2: {expression2, priority: 2}
//           ]
//  handle2: [  subscription3: {expression1, priority: 2},
//              subscription4: {expression2, priority: 2},
//              subscription5: {expression3, priority: 1}
//           ]
//  Here, we route:
//      - data matching 'expression1' to {'handle2'}
//      - data matching 'expression2' to {'handle1', 'handle2'}
//      - data matching 'expression3' to {'handle2'}
//  Note that expressions can overlap. Also, we can use lower ('1') priority of
//  'expression3' as a hint to evaluate it after expressions with higher ('2')
//  priorities.
//  Therefore, the next structure is 'Priority' which is one per received
//  priority per SubStream.  'Priority' keeps list of 'PriorityGroup' objects
//  which group all subscriptions received from downstreams by priority and
//  subscription expression.
//  Next, each group keeps list of 'Subscription' objects which are the
//  smallest units in the hierarchy.  The same consumer can have multiple
//  subscriptions.  Moreover, we group consumers per priority (like groups).
//  So, last pieces in our hierarchy are:
//  'Consumer' <- 'Subscriber' ->> 'Subscription'.
//  In our example:
//  AppContext: {
//                 [consumer1: {handle1,
//                              ['subscription2'
//                              ]
//                             },
//                  consumer2: {handle2,
//                              ['subscription3',
//                               'subscription4',
//                               'subscription5'
//                              ]
//                             },
//                 ],
//
//                 [group1:    {expression1,
//                              ['subscription3']
//                             },
//                  group2:    {expression2},
//                              ['subscription2', 'subscription4'],
//                             },
//                  group3:    {expression3,
//                              ['subscription5']
//                             }
//                 ],
//
//                 [priority2: {'2',
//                              ['group1', 'group2'],
//                              [subscriber1: {consumer1,
//                                             [subscription2: {'group2',
//                                                              'subscriber1'}
//                                             ],
//                               subscriber2: {consumer2,
//                                             [subscription3: {'group1',
//                                                              'subscriber2'},
//                                              subscription4: {'group2',
//                                                              'subscriber2'}
//                                             ]
//                              ],
//                             },
//                  priority1: {'1',
//                              ['group3'],
//                              [subscriber1: {consumer1,
//                                             [subscription1: {'group1'},
//                               subscriber2: {consumer2,
//                                             [subscription5: {'group3'}
//                              ]
//                             }
//                 ]
//              }
//  Note that 'Subscriber' keeps all received subscriptions while 'Consumer'
//  and 'PriorityGroup' keep filtered list of highest priority subscriptions.
//  In our example, 'subscription1' is filtered out.  'subscription5' is not.
//
//  The main iteration order when routing is by priorities by expressions which
//  is in our example [priority2: ['group1', 'group2'], priority1: ['group3']].
//  The order of ['group1', 'group2'] evaluation is implementation-specific
//  (influenced by optimizations).
//
//  Another order is by highest-priority subscribers:
//  [consumer1: 'subscription2'], consumer2: ['subscription3', 'subscription4',
//  'subscription5']].  This order is for broadcast queues.  Another usage is
//  finding minimal delay consumer for potentially poisonous message.
//
/// Thread Safety
///-------------
// NOT Thread-Safe.
//

// MQB

#include <mqbblp_messagegroupidhelper.h>
#include <mqbcmd_messages.h>
#include <mqbi_queue.h>
#include <mqbi_storage.h>

// BMQ
#include <bmqeval_simpleevaluator.h>
#include <bmqt_messageguid.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bsl_list.h>
#include <bsl_map.h>
#include <bsl_ostream.h>
#include <bsl_unordered_map.h>
#include <bslma_managedptr.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcmd {
class RoundRobinRouter;
}

namespace mqbblp {

// =============
// class Routers
// =============

class Routers {
    // Abstracts the message-routing related infrastructure.

  public:
    // PUBLIC TYPES

    /// Mechanism to maintain weak references to items in `unordered_map` in
    /// such a way that when item refcount drops to 0 or when item is
    /// invalidated , the corresponding map entry is removed.
    /// This is to track Topic Based Routing data structures.
    template <class KEY, class VALUE>
    class Registry {
      public:
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(Registry, bslma::UsesBslmaAllocator)

        // PUBLIC TYPES
        struct Item;

        typedef typename bsl::unordered_map<KEY, bsl::weak_ptr<Item> > Base;

        struct Item {
            // TRAITS
            BSLMF_NESTED_TRAIT_DECLARATION(Item, bslma::UsesBslmaAllocator)

            /// Back reference to the map holding the item so that it can
            /// remove itself from the map when refcount drops to 0 or upon
            /// `invalidate()`
            Base& d_owner;

            const KEY                 d_key;
            bsls::ObjectBuffer<VALUE> d_value;
            bool                      d_isValid;

            Item(const KEY&        key,
                 const VALUE&      value,
                 Base&             owner,
                 bslma::Allocator* allocator)
            : d_owner(owner)
            , d_key(key)
            , d_value()
            , d_isValid(true)
            {
                bslalg::ScalarPrimitives::copyConstruct(d_value.address(),
                                                        value,
                                                        allocator);
                BSLS_ASSERT_SAFE(d_owner.find(d_key) != d_owner.end());
            }

            ~Item()
            {
                invalidate();
                d_value.object().~VALUE();
            }
            void release()
            {
                if (isValid()) {
                    size_t n = d_owner.erase(d_key);
                    BSLS_ASSERT_SAFE(n == 1);
                    (void)n;
                }
            }
            VALUE&     value() { return d_value.object(); }
            const KEY& key()
            {
                BSLS_ASSERT_SAFE(isValid());
                return d_key;
            }
            bool isValid() { return d_isValid; }
            void invalidate()
            {
                release();
                d_isValid = false;
            }
        };
        typedef typename Base::const_iterator const_iterator;

        /// Serves as an external references to the Item.
        typedef typename bsl::shared_ptr<Item> SharedItem;

      private:
        // PRIVATE DATA
        Base              d_impl;
        bslma::Allocator* d_allocator_p;

      public:
        Registry(bslma::Allocator* allocator)
        : d_impl(allocator)
        , d_allocator_p(allocator)
        {
            // NOTHING
        }
        Registry(const Registry& other, bslma::Allocator* allocator)
        : d_impl(other.d_impl, allocator)
        , d_allocator_p(allocator)
        {
            // NOTHING
        }
        ~Registry() { clear(); }

        SharedItem record(const KEY& key, const VALUE& value)
        {
            typename Base::const_iterator cit = d_impl.find(key);

            if (cit == d_impl.end()) {
                typename Base::iterator it =
                    d_impl.insert(bsl::make_pair(key, bsl::weak_ptr<Item>()))
                        .first;
                SharedItem result(
                    new (*d_allocator_p)
                        Item(it->first, value, d_impl, d_allocator_p),
                    d_allocator_p);

                it->second = result;

                return result;
            }
            else {
                return cit->second.lock();
            }
        }
        SharedItem find(const KEY& key) const
        {
            typename Base::const_iterator cit = d_impl.find(key);

            if (cit == d_impl.end()) {
                return SharedItem();
            }
            else {
                return cit->second.lock();
            }
        }
        bool   empty() const { return d_impl.empty(); }
        size_t size() const { return d_impl.size(); }
        void   clear()
        {
            typename Base::const_iterator cit;
            while ((cit = d_impl.begin()) != d_impl.end()) {
                cit->second.lock()->invalidate();
            }
            d_impl.clear();
        }
        const_iterator begin() const { return d_impl.begin(); }
        const_iterator end() const { return d_impl.end(); }
        bool           hasItem(const KEY& key) const
        {
            return d_impl.find(key) != d_impl.end();
        }
        static VALUE& value(const_iterator& cit)
        {
            return cit->second.lock()->value();
        }
    };

    struct Subscription;
    typedef bsl::list<const Subscription*> SubscriptionList;

    /// Represents the current state of the queue handle. Used to throttle
    /// potentially poisonous messages.
    /// One per handle per App.
    struct Consumer {
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(Consumer, bslma::UsesBslmaAllocator)

        // DATA
        const bmqp_ctrlmsg::StreamParameters d_streamParameters;

        bsls::TimeInterval d_timeLastMessageSent;
        // Time at which the last
        // message was sent.

        bmqt::MessageGUID d_lastSentMessage;

        SubscriptionList d_highestSubscriptions;
        // Subscriptions with
        // highest priorities.

        unsigned int d_downstreamSubQueueId;

        // CREATORS

        /// Creates a new `Consumer` using the specified
        /// `queueStreamParameters`.
        Consumer(const bmqp_ctrlmsg::StreamParameters& streamParameters,
                 unsigned int                          subQueueId,
                 bslma::Allocator*                     allocator);
        Consumer(const Consumer& other, bslma::Allocator* allocator = 0);

        ~Consumer();

        void registerSubscriptions(mqbi::QueueHandle* handle);
    };

    struct MessagePropertiesReader;

    /// VST to store context for optimization of `Subscription`s evaluation.
    /// One per Queue.
    struct Expression {
        // DATA
        bmqeval::SimpleEvaluator d_evaluator;

        bmqeval::EvaluationContext* d_evaluationContext_p;

        Expression();

        Expression(const Expression& other);

        bool evaluate();
    };

    typedef Registry<const bmqp_ctrlmsg::Expression, Expression> Expressions;
    struct PriorityGroup;

    /// VST representing all `Subscription`s with the same `Expression`.
    /// One per App.
    struct SubscriptionId {
        const Expressions::SharedItem d_itExpression;
        // The Expression.
        const unsigned int d_upstreamSubQueueId;
        PriorityGroup*     d_priorityGroup;

        SubscriptionId(const Expressions::SharedItem& itExpression,
                       const unsigned int             upstreamSubQueueId);

        unsigned int upstreamSubQueueId() const;
    };

    typedef Registry<unsigned int, SubscriptionId> SubscriptionIds;

    /// VST representing all `Subscription`s with the same priority and the
    /// same Expression.
    /// One per App.
    struct PriorityGroup {
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(PriorityGroup,
                                       bslma::UsesBslmaAllocator)

        SubscriptionList d_highestSubscriptions;
        // App Subscriptions having the same Expression
        // and which priority is the highest one.

        const SubscriptionIds::SharedItem d_itId;

        bsl::vector<bmqp_ctrlmsg::ConsumerInfo> d_ci;
        // Vector of accumulated ConsumerInfo in the
        // descending order of priorities.

        bool d_canDeliver;

        PriorityGroup(const SubscriptionIds::SharedItem itId,
                      bslma::Allocator*                 allocator);
        PriorityGroup(const PriorityGroup& other, bslma::Allocator* allocator);

        ~PriorityGroup();

        bool add(const Subscription* subscription);

        bool evaluate(const bsl::shared_ptr<bdlbb::Blob>& data);

        unsigned int sId() const;
    };

    typedef Registry<mqbi::QueueHandle*, Consumer> Consumers;
    typedef Registry<const bmqp_ctrlmsg::Expression, PriorityGroup>
        PriorityGroups;

    /// VST representing `Priority` consumer.
    /// One per Handle per `Priority` per App.
    struct Subscriber {
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(Subscriber, bslma::UsesBslmaAllocator)

        // TYPES

        /// `Subscriber` owns `Subscription`s.
        typedef bsl::list<Subscription> Subscriptions;

        // DATA
        Subscriptions d_subscriptions;
        // All Subscriptions have the same
        // priority

        const Consumers::SharedItem d_itConsumer;

        // CREATORS

        /// Creates a new `Subscriber` using the specified `subQueueId`.
        Subscriber(const Consumers::SharedItem& consumer,
                   bslma::Allocator*            allocator);
        Subscriber(const Subscriber& other, bslma::Allocator* allocator);

        ~Subscriber();
    };

    typedef Registry<mqbi::QueueHandle*, Subscriber> Subscribers;

    /// VST representing one `Subscription`.
    /// One per each received `ConsumerInfo`.
    struct Subscription {
        const bmqp_ctrlmsg::ConsumerInfo d_ci;
        const unsigned int               d_downstreamSubscriptionId;
        mutable int                      d_currentConsumerCount;
        // Transient state assisting round-robin.

        const Subscribers::SharedItem    d_itSubscriber;
        const PriorityGroups::SharedItem d_itGroup;

        Subscription(const bmqp_ctrlmsg::ConsumerInfo& ci,
                     unsigned int                      subscriptionId,
                     const Subscribers::SharedItem&    itSubscriber,
                     const PriorityGroups::SharedItem& group);

        ~Subscription();

        /// Return `true` to move this `Subscription` to the end of
        /// round-robin list according to the `d_consumerPriorityCount`
        bool advance() const;

        mqbi::QueueHandle* handle() const;

        unsigned int subQueueId() const;

        Routers::Consumer* consumer() const;

        const bsl::string& expression() const;

        unsigned int upstreamId() const;
    };

    /// Mechanism representing all `Subscription` `PriorityGroup`s with the
    /// same priority.
    /// One per received priority per App
    struct Priority {
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(Priority, bslma::UsesBslmaAllocator)

        // TYPES
        typedef bsl::list<PriorityGroups::SharedItem> PriorityGroupList;

        // DATA
        Subscribers d_subscribers;
        // All Subscribers (one per handle) having
        // subscription at this priority.

        PriorityGroupList d_highestGroups;
        // Subscriptions grouped by their Expressions
        // having highest priority equal to the
        // priority of this object.

        size_t d_count;
        // Sum of those priorityCounts which
        // Subscription's highest priority is this one.

        explicit Priority(bslma::Allocator* allocator);
        Priority(const Priority& other, bslma::Allocator* allocator);

        ~Priority();
    };

    typedef bsl::map<int, Priority, std::greater<int> > Priorities;

    struct MessagePropertiesReader : public bmqeval::PropertiesReader {
      private:
        // CLASS-SCOPE CATEGORY
        BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.MESSAGEPROPERTIESREADER");

        // DATA
        bmqp::SchemaLearner& d_schemaLearner;
        // Use own context;

        bmqp::SchemaLearner::Context d_schemaLearnerContext;

        bmqp::MessageProperties      d_properties;
        const mqbi::StorageIterator* d_currentMessage_p;
        bsl::shared_ptr<bdlbb::Blob> d_appData;
        bmqp::MessagePropertiesInfo  d_messagePropertiesInfo;
        bool                         d_isDirty;

      public:
        MessagePropertiesReader(bmqp::SchemaLearner& schemaLearner,
                                bslma::Allocator*    allocator);

        ~MessagePropertiesReader() BSLS_KEYWORD_OVERRIDE;

        void _set(bmqp::MessageProperties& properties);

        bdld::Datum get(const bsl::string& name,
                        bslma::Allocator*  allocator) BSLS_KEYWORD_OVERRIDE;

        // Prepare the reader for the next message given the specified
        // 'currentMessage' or 'appData' and 'messagePropertiesInfo'.
        void next(const mqbi::StorageIterator* currentMessage);
        void next(const bsl::shared_ptr<bdlbb::Blob>& appData,
                  const bmqp::MessagePropertiesInfo&  messagePropertiesInfo);
        void clear();
        // Reset the reader to the state of empty properties.
    };

    /// Mechanism to assist `Expression`s evaluation optimization to avoid
    /// evaluating the same `Expression` more than once.
    struct QueueRoutingContext {
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(QueueRoutingContext,
                                       bslma::UsesBslmaAllocator)

        // DATA
        Expressions d_expressions;
        // Registry of all expressions received by
        // this queue across all subQueues.

        unsigned int d_nextSubscriptionId;
        // Generates unique upstream id.

        SubscriptionIds d_groupIds;
        // Subscriptions grouped by expression
        // and advertised upstream with unique ids.

        bsl::shared_ptr<MessagePropertiesReader> d_preader;

        bmqeval::EvaluationContext d_evaluationContext;

        bslma::Allocator* d_allocator_p;

        QueueRoutingContext(bmqp::SchemaLearner& schemaLearner,
                            bslma::Allocator*    allocator);
        ~QueueRoutingContext();

        /// Generate `Subscription`s Id for upstream.
        unsigned int nextSubscriptionId();

        bool onUsable(unsigned int* upstreamSubQueueId,
                      unsigned int  upstreamSubscriptionId);

        void loadInternals(mqbcmd::Routing* out) const;
    };

    typedef bsl::function<bool(const Routers::Subscription*)> Visitor;

    enum Result {
        e_SUCCESS = 0  // Found Subscription and there is capacity
        ,
        e_NO_SUBSCRIPTION = 1  // No matching Subscription
        ,
        e_NO_CAPACITY = 2  // Found Subscription(s) without capacity
        ,
        e_NO_CAPACITY_ALL = 3  // All Subscription(s) are without capacity
        ,
        e_DELAY = 4  // Delay due to Potentially Poisonous data
    };

    /// Class that implements round-robin routing policy.
    class RoundRobin {
      private:
        // PRIVATE DATA
        Priorities& d_priorities;

      public:
        // CREATORS

        /// Creates a new `RoundRobin` using the specified `consumers`. See
        /// `d_context` for further information regarding the ownership
        /// semantics for `consumers`.
        explicit RoundRobin(Priorities& priorities);

        // MANIPULATORS

        /// Iterate all `Group`s and call the specified `visitor` for each
        /// highest priority `Subscription` which has `canDeliver` consumer
        /// and which `Expression` matches the specified `currentMessage`.
        /// If the `visitor` returns `true`, stop iterating, move the
        /// subscription to the end of round-robind selection list if it has
        /// been selected `d_consumerPriorityCount` times , and return
        /// `true`.
        Result iterateGroups(const Visitor&               visitor,
                             const mqbi::StorageIterator* currentMessage);

        /// Iterate all highest priority `Subscription`s within the
        /// specified `group` and call the specified `visitor` for each
        /// `Subscription` which has `canDeliver` consumer.  If the
        /// `visitor` returns `true`, stop iterating, move the subscription
        /// to the end of round-robind selection list if it has been
        /// selected `d_consumerPriorityCount` times, and return `true`.
        bool iterateSubscriptions(const Visitor& visitor,
                                  PriorityGroup& group);

        void print(bsl::ostream& os, int level, int spacesPerLevel) const;
    };

    struct AppContext {
      public:
        // Mechanism aggregating all data structures for TBR.
        // One per App.

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(AppContext, bslma::UsesBslmaAllocator)

        // DATA
        PriorityGroups d_groups;
        // Subscriptions grouped by expression

        Priorities d_priorities;
        Consumers  d_consumers;

        QueueRoutingContext& d_queue;

        RoundRobin d_router;
        // Round-robin routing policy.

        bmqeval::CompilationContext d_compilationContext;

        bslma::Allocator* d_allocator_p;

        AppContext(QueueRoutingContext& queue, bslma::Allocator* allocator);

        ~AppContext();

        /// Parse the specified `streamParameters` if it matches the
        /// specified `appId` and associate the results with the specified
        /// `handle` and `subId` for TBR.
        void loadApp(const char*                          appId,
                     mqbi::QueueHandle*                   handle,
                     bsl::ostream*                        errorStream,
                     const mqbi::QueueHandle::StreamInfo& info,
                     const AppContext*                    previous);

        /// Parse the specified `streamParameters` and associate the results
        /// with the specified  `handle` and `subId` for TBR.
        void load(mqbi::QueueHandle*                    handle,
                  bsl::ostream*                         errorStream,
                  unsigned int                          downstreamSubQueueId,
                  unsigned int                          upstreamSubQueueId,
                  const bmqp_ctrlmsg::StreamParameters& streamParameters,
                  const AppContext*                     previous);

        /// Make a pass on results of previous parsing and build round-robin
        /// lists of highest priority `Subscription`s.
        size_t finalize();

        void registerSubscriptions();

        /// Remove round-robin lists of highest priority `Subscription`s
        /// (undo `finalize`) and remove invalidated `Consumer`s.
        void clean();

        /// Remove all results of parsing.
        void reset();

        /// If the specified `currentMessage` refers to a known `Group`,
        /// iterate all highest priority `Subscription`s within the `group`
        /// and call the specified `visitor` for each highest priority
        /// `Subscription` which has `canDeliver` consumer.  If the
        /// `currentMessage` does not refer to a known `PriorityGroup`,
        /// iterate all highest priority `Subscription` for all
        /// `PriorityGroup`s and call the specified `visitor` for each
        /// highest priority `Subscription` which has `canDeliver` consumer
        /// and which `Expression` matches the `currentMessage`.
        /// If the `visitor` returns `true`, stop iterating, move the
        /// subscription to the end of round-robin selection list if it has
        /// been selected `d_consumerPriorityCount` times, and return
        /// `true`.
        Routers::Result
        selectConsumer(const Visitor&               visitor,
                       const mqbi::StorageIterator* currentMessage);

        /// Iterate all highest priority `Subscriber`s and call the
        /// specified `visitor` for each highest priority `Subscription`
        /// which has `canDeliver` consumer and which `Expression` matches
        /// the specified `currentMessage`.  If the `visitor` returns
        /// `true`, stop iterating and return `true`.
        bool iterateConsumers(const Visitor&               visitor,
                              const mqbi::StorageIterator* message);

        /// Iterate all highest priority highest priority `Subscription`s
        /// within the specified `itSubscriber` and return the first one
        /// which has `canDeliver` consumer and which `Expression` matches
        /// the specified `message`.  Otherwise, return `0`.
        const Subscription*
        selectSubscription(const Consumers::SharedItem& itConsumer,
                           const mqbi::StorageIterator* message) const;

        /// Make `SubscriptionId` reference previously generated groups,
        void apply();

        // ACCESSORS
        void loadInternals(mqbcmd::RoundRobinRouter* out) const;
        // Load into the specified 'out', internal details about this object.
        void generate(bmqp_ctrlmsg::StreamParameters* streamParameters) const;

        bool hasHandle(mqbi::QueueHandle* handle) const;
    };
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------------
// struct Routers::Consumer
// -----------------------------

inline Routers::Consumer::Consumer(
    const bmqp_ctrlmsg::StreamParameters& streamParameters,
    unsigned int                          subQueueId,
    bslma::Allocator*                     allocator)
: d_streamParameters(streamParameters, allocator)
, d_timeLastMessageSent(0)
, d_lastSentMessage()
, d_highestSubscriptions(allocator)
, d_downstreamSubQueueId(subQueueId)
{
    // NOTHING
}

inline Routers::Consumer::Consumer(const Consumer&   other,
                                   bslma::Allocator* allocator)
: d_streamParameters(other.d_streamParameters, allocator)
, d_timeLastMessageSent(other.d_timeLastMessageSent)
, d_lastSentMessage(other.d_lastSentMessage)
, d_highestSubscriptions(other.d_highestSubscriptions, allocator)
, d_downstreamSubQueueId(other.d_downstreamSubQueueId)
{
    // NOTHING
}

inline Routers::Consumer::~Consumer()
{
    // NOTHING
}
// -----------------------------
// struct Routers::AppContext
// -----------------------------

inline Routers::AppContext::AppContext(QueueRoutingContext& queue,
                                       bslma::Allocator*    allocator)
: d_groups(allocator)
, d_priorities(allocator)
, d_consumers(allocator)
, d_queue(queue)
, d_router(d_priorities)
, d_compilationContext(allocator)
, d_allocator_p(allocator)
{
    // NOTHING
    BSLS_ASSERT_SAFE(allocator);
}

inline Routers::AppContext::~AppContext()
{
    reset();
}

inline bool Routers::AppContext::hasHandle(mqbi::QueueHandle* handle) const
{
    return d_consumers.hasItem(handle);
}

// -----------------------------------
// struct Routers::QueueRoutingContext
// -----------------------------------

inline Routers::QueueRoutingContext::QueueRoutingContext(
    bmqp::SchemaLearner& schemaLearner,
    bslma::Allocator*    allocator)
: d_expressions(allocator)
, d_nextSubscriptionId(0)
, d_groupIds(allocator)
, d_preader(new (*allocator) MessagePropertiesReader(schemaLearner, allocator),
            allocator)
, d_evaluationContext(0, allocator)
, d_allocator_p(allocator)
{
    d_evaluationContext.setPropertiesReader(d_preader.get());
}

inline Routers::QueueRoutingContext::~QueueRoutingContext()
{
    // 'd_groupIds.empty()' must be 'true' except for one corner case:
    // - 'AppContext' is created as part of 'RelayQueueEngine::configureHandle'
    // - that context is bound to the configure request waiting response
    // - that context contains items 'registered' in this 'QueueRoutingContext'
    // - before the response, 'Queue::convertToLocalDispatched' happens
    // - and the 'RemoteQueue' destructs, including this 'QueueRoutingContext'
    // - but the functor still keeps (obsolete) 'AppContext'

    // Note that the 'Registry::Registry' calls 'invalidate' on each 'Item' in
    // the dtor.  'Item::invalidate' disables access to the registry.
    // Therefore, the registry can destruct before its items.
}

inline bool
Routers::QueueRoutingContext::onUsable(unsigned int* upstreamSubQueueId,
                                       unsigned int  upstreamSubscriptionId)
{
    SubscriptionIds::SharedItem si = d_groupIds.find(upstreamSubscriptionId);
    if (si) {
        if (si->value().d_priorityGroup) {
            si->value().d_priorityGroup->d_canDeliver = true;
            *upstreamSubQueueId = si->value().d_upstreamSubQueueId;
            return true;  // RETURN
        }
    }
    return false;
}

// -----------------------------
// struct Routers::Expression
// -----------------------------

// CREATORS
inline Routers::Expression::Expression()
: d_evaluator()
, d_evaluationContext_p(0)
{
}

inline Routers::Expression::Expression(const Expression& other)
: d_evaluator(other.d_evaluator)
, d_evaluationContext_p(other.d_evaluationContext_p)
{
    // NOTHING
}
// -----------------------------
// struct Routers::Subscription
// -----------------------------

inline Routers::Subscription::Subscription(
    const bmqp_ctrlmsg::ConsumerInfo& ci,
    unsigned int                      subscriptionId,
    const Subscribers::SharedItem&    itSubscriber,
    const PriorityGroups::SharedItem& group)
: d_ci(ci)
, d_downstreamSubscriptionId(subscriptionId)
, d_currentConsumerCount(ci.consumerPriorityCount())
, d_itSubscriber(itSubscriber)
, d_itGroup(group)
{
    // NOTHING
}

inline Routers::Subscription::~Subscription()
{
    // NOTHING
}

inline bool Routers::Subscription::advance() const
{
    BSLS_ASSERT_SAFE(d_currentConsumerCount);

    if (--d_currentConsumerCount == 0) {
        d_currentConsumerCount = d_ci.consumerPriorityCount();
        return true;  // RETURN
    }
    return false;
}

inline mqbi::QueueHandle* Routers::Subscription::handle() const
{
    BSLS_ASSERT_SAFE(d_itSubscriber->value().d_itConsumer->key());
    return d_itSubscriber->value().d_itConsumer->key();
}

inline unsigned int Routers::Subscription::subQueueId() const
{
    return d_itSubscriber->value()
        .d_itConsumer->value()
        .d_downstreamSubQueueId;
}

inline Routers::Consumer* Routers::Subscription::consumer() const
{
    return &d_itSubscriber->value().d_itConsumer->value();
}

inline const bsl::string& Routers::Subscription::expression() const
{
    return d_itGroup->key().text();
}

inline unsigned int Routers::Subscription::upstreamId() const
{
    return d_itGroup->value().d_itId->d_key;
}

// ------------------------------
// struct Routers::SubscriptionId
// ------------------------------
inline Routers::SubscriptionId::SubscriptionId(
    const Expressions::SharedItem& itExpression,
    const unsigned int             upstreamSubQueueId)
: d_itExpression(itExpression)
, d_upstreamSubQueueId(upstreamSubQueueId)
, d_priorityGroup(0)
{
    // NOTHING
}

inline unsigned int Routers::SubscriptionId::upstreamSubQueueId() const
{
    return d_upstreamSubQueueId;
}

// -----------------------------
// struct Routers::PriorityGroup
// -----------------------------

inline Routers::PriorityGroup::PriorityGroup(
    const SubscriptionIds::SharedItem itId,
    bslma::Allocator*                 allocator)
: d_highestSubscriptions(allocator)
, d_itId(itId)
, d_ci(allocator)
, d_canDeliver(true)
{
    // NOTHING
}

inline Routers::PriorityGroup::PriorityGroup(const PriorityGroup& other,
                                             bslma::Allocator*    allocator)
: d_highestSubscriptions(other.d_highestSubscriptions, allocator)
, d_itId(other.d_itId)
, d_ci(other.d_ci, allocator)
, d_canDeliver(other.d_canDeliver)
{
    // NOTHING
}

inline Routers::PriorityGroup::~PriorityGroup()
{
    if (d_itId->value().d_priorityGroup == this) {
        d_itId->value().d_priorityGroup = 0;
    }
}

inline bool Routers::PriorityGroup::add(const Subscription* subscription)
{
    BSLS_ASSERT_SAFE(subscription->d_ci.consumerPriorityCount());

    const bsls::Types::Int64 k_int64Max =
        bsl::numeric_limits<bsls::Types::Int64>::max();

    // This relies on the order (priority) of calling 'add'
    size_t                            n     = d_ci.size();
    bool                              isNew = true;
    const bmqp_ctrlmsg::ConsumerInfo& inCi  = subscription->d_ci;

    if (n == 0) {
        d_ci.resize(1);
    }
    else if (d_ci[n - 1].consumerPriority() == inCi.consumerPriority()) {
        --n;
        isNew = false;
    }
    else {
        d_ci.resize(n + 1);
    }
    bmqp_ctrlmsg::ConsumerInfo* outCi = &d_ci[n];

    outCi->consumerPriority() = inCi.consumerPriority();
    outCi->consumerPriorityCount() += inCi.consumerPriorityCount();

    if (outCi->maxUnconfirmedMessages() >
        k_int64Max - inCi.maxUnconfirmedMessages()) {
        outCi->maxUnconfirmedMessages() = k_int64Max;
    }
    else {
        outCi->maxUnconfirmedMessages() += inCi.maxUnconfirmedMessages();
    }

    if (outCi->maxUnconfirmedBytes() >
        k_int64Max - inCi.maxUnconfirmedBytes()) {
        outCi->maxUnconfirmedBytes() = k_int64Max;
    }
    else {
        outCi->maxUnconfirmedBytes() += inCi.maxUnconfirmedBytes();
    }
    return isNew;
}

// -----------------------------
// struct Routers::Subscriber
// -----------------------------

inline Routers::Subscriber::Subscriber(const Consumers::SharedItem& consumer,
                                       bslma::Allocator*            allocator)
: d_subscriptions(allocator)
, d_itConsumer(consumer)
{
    // NOTHING
}

inline Routers::Subscriber::Subscriber(const Subscriber& other,
                                       bslma::Allocator* allocator)
: d_subscriptions(other.d_subscriptions, allocator)
, d_itConsumer(other.d_itConsumer)
{
    // NOTHING
}

inline Routers::Subscriber::~Subscriber()
{
    // NOTHING
}

// -----------------------------
// struct Routers::Priority
// -----------------------------

inline Routers::Priority::Priority(bslma::Allocator* allocator)
: d_subscribers(allocator)
, d_highestGroups(allocator)
, d_count(0)
{
    // NOTHING
}

inline Routers::Priority::Priority(const Priority&   other,
                                   bslma::Allocator* allocator)
: d_subscribers(other.d_subscribers, allocator)
, d_highestGroups(other.d_highestGroups, allocator)
, d_count(other.d_count)
{
    // NOTHING
}

inline Routers::Priority::~Priority()
{
    // NOTHING
}
// -----------------------------
// struct Routers::RoundRobin
// -----------------------------

inline Routers::RoundRobin::RoundRobin(Priorities& priorities)
: d_priorities(priorities)
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace

#endif
