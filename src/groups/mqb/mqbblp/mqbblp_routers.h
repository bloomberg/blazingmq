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

/// @file mqbblp_routers.h
///
/// @brief Abstracts the message-routing related infrastructure.
///
/// Here is the relationship between major Topic Based Routing concepts:
///
/// ```
///         ---------------------------------------------------
///        |                                                   |
///       1|                                              many V
///        |                                                   V
///     SubStream <<---------->> QueueHandle <<---------->> Subscription
///                many-to-many               many-to-many
/// ```
///
/// A QueueEngine builds Routing data hierarchy upon processing
/// `ConfigureStream` request.  The topmost structure is `QueueRoutingContext`
/// holding the context shared by all SubStreams, followed by one `AppContext`
/// per each SubStream.  To build all routing structures, call
/// `AppContext::load` for each consumer, followed by one
/// `AppContext::finalize` call afterwards.  `AppContext::generate` then
/// generates `bmqp_ctrlmsg::StreamParameters` to communicate upstream.
///
/// To route next PUSH message, call `AppContext::selectConsumer` which
/// iterates all matching subscriptions until the specified visitor returns
/// `false`.
///
/// QueueEngine routes to highest priority _subscription_ consumers only.  Note
/// that different subscription may have different priority and that the same
/// subscription expression can be used with different priorities.  For
/// example,
///
/// ```
/// handle1: [  subscription1: {expression1, priority: 1},
///             subscription2: {expression2, priority: 2}
///          ]
/// handle2: [  subscription3: {expression1, priority: 2},
///             subscription4: {expression2, priority: 2},
///             subscription5: {expression3, priority: 1}
///          ]
/// ```
///
/// Here, we route:
///   - data matching `expression1` to `{handle2}`,
///   - data matching `expression2` to `{handle1, handle2}`, and
///   - data matching `expression3` to `{handle2}`.
///
/// Note that expressions can overlap. Also, we can use lower (`1`) priority of
/// `expression3` as a hint to evaluate it after expressions with higher (`2`)
/// priorities.
///
/// Therefore, the next structure is `Priority` which is one per received
/// priority per SubStream.  `Priority` keeps list of `PriorityGroup` objects
/// which group all subscriptions received from downstreams by priority and
/// subscription expression.
///
/// Next, each group keeps list of `Subscription` objects which are the
/// smallest units in the hierarchy.  The same consumer can have multiple
/// subscriptions.  Moreover, we group consumers per priority (like groups).
/// So, last pieces in our hierarchy are:
///
/// ```
/// Consumer <- Subscriber ->> Subscription
/// ```
///
/// In our example:
///
/// ```
/// AppContext: {
///                [consumer1: {handle1,
///                             ['subscription2'
///                             ]
///                            },
///                 consumer2: {handle2,
///                             ['subscription3',
///                              'subscription4',
///                              'subscription5'
///                             ]
///                            },
///                ],
///
///                [group1:    {expression1,
///                             ['subscription3']
///                            },
///                 group2:    {expression2},
///                             ['subscription2', 'subscription4'],
///                            },
///                 group3:    {expression3,
///                             ['subscription5']
///                            }
///                ],
///
///                [priority2: {'2',
///                             ['group1', 'group2'],
///                             [subscriber1: {consumer1,
///                                            [subscription2: {'group2',
///                                                             'subscriber1'}
///                                            ],
///                              subscriber2: {consumer2,
///                                            [subscription3: {'group1',
///                                                             'subscriber2'},
///                                             subscription4: {'group2',
///                                                             'subscriber2'}
///                                            ]
///                             ],
///                            },
///                 priority1: {'1',
///                             ['group3'],
///                             [subscriber1: {consumer1,
///                                            [subscription1: {'group1'},
///                              subscriber2: {consumer2,
///                                            [subscription5: {'group3'}
///                             ]
///                            }
///                ]
///             }
/// ```
///
/// Note that `Subscriber` keeps all received subscriptions while `Consumer`
/// and `PriorityGroup` keep filtered list of highest priority subscriptions.
/// In our example, `subscription1` is filtered out.  `subscription5` is not.
///
/// The main iteration order when routing is by priorities by expressions which
/// is in our example `[priority2: [group1, group2], priority1: [group3]]`.
/// The order of `[group1, group2]` evaluation is implementation-specific
/// (influenced by optimizations).
///
/// Another order is by highest-priority subscribers: `[consumer1:
/// subscription2], consumer2: [subscription3, subscription4, subscription5]]`.
/// This order is for broadcast queues.  Another usage is finding minimal delay
/// consumer for potentially poisonous message.
///
/// Thread Safety                                      {#mqbblp_routers_thread}
/// =============
///
/// NOT Thread-Safe.

// MQB
#include <mqbcmd_messages.h>
#include <mqbi_queue.h>
#include <mqbi_storage.h>

// BMQ
#include <bmqeval_simpleevaluator.h>
#include <bmqt_messageguid.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bsl_functional.h>
#include <bsl_limits.h>
#include <bsl_list.h>
#include <bsl_map.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_unordered_map.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bsla_annotations.h>
#include <bslh_hash.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcmd {
class RoundRobinRouter;
}

namespace mqbblp {

struct PropertyRef {
    const unsigned int d_ordinal;
    const bdld::Datum  d_value;

    PropertyRef(unsigned int ordinal, const bdld::Datum& value)
    : d_ordinal(ordinal)
    , d_value(value)
    {
        // NOTHING
    }
    PropertyRef(unsigned int       ordinal,
                const bdld::Datum& value,
                bslma::Allocator*  allocator)
    : d_ordinal(ordinal)
    , d_value(value.clone(allocator))
    {
        // NOTHING
    }
};

//  1.  Share Learning results across Apps
//  2.  Do not reset Learning results upon reconfigure
//  3.  Share Learning results across broadcast consumers
//
//  Allocate sequence numbers range per App / broadcast consumer
//  Keep result as weak_ptr - no.
//  Reuse PriorityGroup? - no.  Keep sId + lookup
//
//
//                                  Queue --->> sId  (unique across all Apps)
//                                                ^
//                                              | |
//                                              V
//      PG  <---- Item <--- shared_ptr <<--- old App context
//                                           must create new App context
//  How to use order of evaluations?

template <typename T>
class Learning {
  public:
    //    class PropertyHashAlgo {
    //      private:
    //        // DATA
    //        bsls::Types::Uint64 d_result;
    //
    //      public:
    //        // TYPES
    //        typedef bsls::Types::Uint64 result_type;
    //
    //        // CREATORS
    //
    //        /// Default constructor.
    //        PropertyHashAlgo()
    //        : d_result(0)
    //        {
    //            // NOTHING
    //        }
    //
    //        // MANIPULATORS
    //        void operator()(const PropertyRef& property)
    //        {
    //
    //        }
    //
    //        /// Compute and return the hash for the StorageKey.
    //        result_type computeHash()
    //        {
    //            return d_result;
    //        }
    //    };

    struct Property;

    typedef bsl::shared_ptr<Property> PropertyPtr;

    typedef bsl::unordered_map<PropertyRef, PropertyPtr> PropertyChoices;

    typedef bsl::multimap<unsigned int, T> Learned;
    struct Property {
        PropertyChoices    d_choices;

        Learned d_learned;

        Property(unsigned int initialNumBuckets, bslma::Allocator* allocator)
        : d_choices(initialNumBuckets, allocator)
        , d_learned(allocator)
        {
            // NOTHING
        }

        void reset()
        {
            d_choices.clear();
            d_learned.clear();
        }
    };

    struct Result {
        unsigned int d_sequenceNum;
        bool         d_isKnown;

        Result(unsigned int sequenceNumber)
        : d_sequenceNum(sequenceNumber)
        , d_isKnown(false)
        {
            // NOTHING
        }
        Result(const Result& other)
        : d_sequenceNum(other.d_sequenceNum)
        , d_isKnown(other.d_isKnown)
        {
            // NOTHING
        }
    };

    typedef bsl::unordered_map<unsigned int, Result> Results;

    struct Root {
        typedef bsl::unordered_map<unsigned int, PropertyPtr> Schemas;
        typedef bsl::unordered_map<bsl::string, unsigned int> NameRegistry;

        Results      d_results;
        unsigned int d_numKnown;
        unsigned int d_total;

        unsigned int d_initialNumBuckets;

        Schemas d_schemas;

        Property* d_start_p;
        Property* d_current_p;

        NameRegistry d_names;

        mutable bsls::Types::Uint64 d_hits;
        mutable bsls::Types::Uint64 d_iterations;

        Root(bslma::Allocator* allocator)
        : d_results(allocator)
        , d_numKnown(0)
        , d_total(0)
        , d_initialNumBuckets(0)
        , d_schemas(allocator)
        , d_start_p(0)
        , d_current_p(0)
        , d_names(allocator)
        , d_hits(0)
        , d_iterations(0)
        {
            // NOTHING

            startSchema(bmqp::MessagePropertiesInfo::k_NO_SCHEMA, allocator);
        }

        void setInitialNumBuckets(unsigned int initialNumBuckets)
        {
            d_initialNumBuckets = initialNumBuckets;
        }
        unsigned int numKnown() const { return d_numKnown; }

        unsigned int ordinal(const bsl::string& name)
        {
            NameRegistry::const_iterator cit = d_names.find(name);

            if (cit == d_names.cend()) {
                cit = d_names.emplace(name, d_names.size()).first;
            }

            return cit->second;
        }

        void startSchema(unsigned int id, bslma::Allocator* allocator)
        {
            typename Schemas::iterator it = d_schemas.find(id);

            if (it == d_schemas.end()) {
                PropertyPtr& newPropertyPtr = d_schemas[id];
                newPropertyPtr.createInplace(allocator,
                                             d_initialNumBuckets,
                                             allocator);
                d_start_p = newPropertyPtr.get();
            }
            else {
                d_start_p = it->second.get();
            }
            d_current_p = d_start_p;
        }

        void registerChoice(const PropertyRef& key,
                            bslma::Allocator*  allocator)
        {
            BSLS_ASSERT_SAFE(d_current_p);

            PropertyChoices& choices = d_current_p->d_choices;

            const typename PropertyChoices::const_iterator cit = choices.find(
                key);

            if (cit == choices.end()) {
                // seeing this MessageProperty value first time at this
                // iteration. need to deep copy.

                PropertyPtr next;

                next.createInplace(allocator, d_initialNumBuckets, allocator);

                choices.insert(bsl::make_pair(
                    PropertyRef(key.d_ordinal, key.d_value, allocator),
                    next));

                d_current_p = next.get();
            }
            else {
                d_current_p = cit->second.get();
            }
        }
        //        unsigned int addResult(unsigned int id,
        //                               unsigned int sequenceNumber,
        //                               bool         isKnown)
        //        {
        //            typename Results::iterator cit = d_results.find(id);
        //            if (cit == d_results.cend()) {
        //                cit = d_results.emplace(id, sequenceNumber).first;
        //                ++d_total;
        //            }
        //            else {
        //                BSLS_ASSERT_SAFE(sequenceNumber ==
        //                cit->second.d_sequenceNum);
        //            }
        //            if (isKnown) {
        //                if (!cit->second.d_isKnown) {
        //                    cit->second.d_isKnown = true;
        //                    ++d_numKnown;
        //                }
        //
        //                d_current_p->d_learned.emplace(sequenceNumber, id);
        //            }
        //
        //            return cit->first;
        //        }
        void
        addResult(unsigned int id, unsigned int sequenceNumber, bool isKnown)
        {
            if (isKnown) {
                d_current_p->d_learned.emplace(sequenceNumber, id);
            }
        }
        void rewind()
        {
            d_current_p = currentStart();
            ++d_iterations;
        }
        Property* currentStart() const { return d_start_p; }

        void reset()
        {
            d_results.clear();
            d_numKnown = 0;
            d_total    = 0;

            d_hits       = 0;
            d_iterations = 0;

            PropertyPtr temp =
                d_schemas[bmqp::MessagePropertiesInfo::k_NO_SCHEMA];
            temp->reset();
            d_schemas.clear();
            d_schemas[bmqp::MessagePropertiesInfo::k_NO_SCHEMA] = temp;
            d_start_p                                           = temp.get();
        }
        Learned& learnedResults()
        {
            BSLS_ASSERT_SAFE(d_current_p);
            return d_current_p->d_learned;
        }

        void hit() { ++d_hits; }

        bsls::Types::Uint64 hits() const { return d_hits; }
        bsls::Types::Uint64 iterations() const { return d_iterations; }
    };
};

// =============
// class Routers
// =============

/// Data hierarchy to support data (PUSH) routing by Queue engines.  Including
/// support for Topic Based Routing.
class Routers {
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
                    BSLA_MAYBE_UNUSED size_t n = d_owner.erase(d_key);
                    BSLS_ASSERT_SAFE(n == 1);
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

        /// Time at which the last message was sent.
        bsls::TimeInterval d_timeLastMessageSent;

        bmqt::MessageGUID d_lastSentMessage;

        /// Subscriptions with highest priorities.
        SubscriptionList d_highestSubscriptions;

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

        unsigned int d_lastRun;
        bool         d_lastResult;

        Expression();

        Expression(const Expression& other);

        bool evaluate(unsigned int run);
    };

    typedef Registry<const bmqp_ctrlmsg::Expression, Expression> Expressions;
    struct PriorityGroup;

    /// VST representing all `Subscription`s with the same `Expression`.
    /// One per App.
    struct SubscriptionId {
        /// The Expression.
        const Expressions::SharedItem d_itExpression;
        const unsigned int            d_upstreamSubQueueId;
        PriorityGroup*                d_priorityGroup;

        SubscriptionId(const Expressions::SharedItem& itExpression,
                       const unsigned int             upstreamSubQueueId);

        unsigned int upstreamSubQueueId() const;
    };

    typedef Registry<unsigned int, SubscriptionId> SubscriptionIds;
    typedef bsl::function<bool(const Routers::Subscription*)> Visitor;

    /// VST representing all `Subscription`s with the same priority and the
    /// same Expression.
    /// One per App.
    struct PriorityGroup {
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(PriorityGroup,
                                       bslma::UsesBslmaAllocator)

        /// App Subscriptions having the same Expression and which priority is
        /// the highest one.
        SubscriptionList d_highestSubscriptions;

        const SubscriptionIds::SharedItem d_itId;

        /// Vector of accumulated ConsumerInfo in the descending order of
        /// priorities.
        bsl::vector<bmqp_ctrlmsg::ConsumerInfo> d_ci;

        bool d_canDeliver;

        PriorityGroup(const SubscriptionIds::SharedItem itId,
                      bslma::Allocator*                 allocator);
        PriorityGroup(const PriorityGroup& other, bslma::Allocator* allocator);

        ~PriorityGroup();

        bool add(const Subscription* subscription);

        bool evaluate(unsigned int run);

        /// Iterate all highest priority `Subscription`s within this object and
        /// scall the specified `visitor` for each `Subscription` which has
        /// `canDeliver` consumer.  If the `visitor` returns `true`, stop
        /// iterating, move the subscription to the end of the list if it has
        /// been selected `d_consumerPriorityCount` times, and return `true`.
        bool iterateSubscriptions(const Visitor& visitor);

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

        /// All Subscriptions have the same priority
        Subscriptions d_subscriptions;

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

        /// Transient state assisting round-robin.
        mutable int d_currentConsumerCount;

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

    typedef bsl::vector<PriorityGroups::SharedItem> PriorityGroupVector;

    /// Mechanism representing all `Subscription` `PriorityGroup`s with the
    /// same priority.  One per received priority per App
    struct Priority {
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(Priority, bslma::UsesBslmaAllocator)

        // DATA

        /// All Subscribers (one per handle) having subscription at this
        /// priority.
        Subscribers d_subscribers;

        /// Subscriptions grouped by their Expressions having highest priority
        /// equal to the priority of this object.
        PriorityGroupVector d_highestGroups;

        /// Sum of those priorityCounts which Subscription's highest priority
        /// is this one.
        size_t d_count;

        explicit Priority(bslma::Allocator* allocator);
        Priority(const Priority& other, bslma::Allocator* allocator);

        ~Priority();
    };

    typedef Learning<unsigned int> LearningIds;

    struct MessagePropertiesReader : public bmqeval::PropertiesReader {
      private:
        // CLASS-SCOPE CATEGORY
        BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.MESSAGEPROPERTIESREADER");

        // DATA

        /// Use own context;
        bmqp::SchemaLearner& d_schemaLearner;

        bmqp::SchemaLearner::Context d_schemaLearnerContext;

        bmqp::MessageProperties      d_properties;
        const mqbi::StorageIterator* d_currentMessage_p;
        bsl::shared_ptr<bdlbb::Blob> d_appData;
        bmqp::MessagePropertiesInfo  d_messagePropertiesInfo;

        bool d_needsData;

        LearningIds::Root* d_root_p;

        unsigned int d_runs;

        bslma::Allocator* d_allocator_p;

      public:
        // TYPES
        MessagePropertiesReader(bmqp::SchemaLearner& schemaLearner,
                                LearningIds::Root*   root,
                                bslma::Allocator*    allocator);

        ~MessagePropertiesReader() BSLS_KEYWORD_OVERRIDE;

        void _set(bmqp::MessageProperties& properties);

        bdld::Datum get(const bsl::string& name,
                        bslma::Allocator*  allocator) BSLS_KEYWORD_OVERRIDE;

        /// Prepare the reader for the next message given the specified
        /// `currentMessage`.
        void start(const mqbi::StorageIterator* currentMessage);

        /// Prepare the reader for the next message given the specified
        /// `appData` and `messagePropertiesInfo`.
        void start(const bsl::shared_ptr<bdlbb::Blob>& appData,
                   const bmqp::MessagePropertiesInfo&  messagePropertiesInfo);

        /// Reset the reader to the state of empty properties.
        void clear();

        void startIterating(LearningIds::Root* root);

        void rewind();
        void
        learn(unsigned int id, unsigned int sequenceNumber, bool doesMatch);
        //        void skip(unsigned int                      id,
        //                  unsigned int                      sequenceNumber);

        void stopIterating();

        unsigned int numRuns() const;
    };

    /// Mechanism to assist `Expression`s evaluation optimization to avoid
    /// evaluating the same `Expression` more than once.
    struct QueueRoutingContext {
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(QueueRoutingContext,
                                       bslma::UsesBslmaAllocator)

        // DATA

        /// Registry of all expressions received by this queue across all
        /// subQueues.
        Expressions d_expressions;

        /// Generates unique upstream id.
        unsigned int d_nextSubscriptionId;

        /// Subscriptions grouped by expression and advertised upstream with
        /// unique ids.
        SubscriptionIds d_groupIds;

        LearningIds::Root d_root;

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

        void loadInternals(mqbcmd::Routing* out, unsigned int max) const;
    };

    enum Result {
        /// Found Subscription and there is capacity.
        e_SUCCESS = 0,
        /// No matching Subscription.
        e_NO_SUBSCRIPTION = 1,
        /// Found Subscription(s) without capacity.
        e_NO_CAPACITY = 2,
        /// All Subscription(s) are without capacity.
        e_NO_CAPACITY_ALL = 3,
        /// Delay due to Potentially Poisonous data.
        e_DELAY = 4,
        /// Not valid anymore due to Confirm/Purge.
        e_INVALID = 5
    };

    typedef bsl::map<int, Priority, std::greater<int> > Priorities;

    // Mechanism aggregating all data structures for TBR.  One per App.
    struct AppContext {
      public:
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(AppContext, bslma::UsesBslmaAllocator)

        // DATA

        /// Subscriptions grouped by expression.
        PriorityGroups d_groups;

        Priorities d_priorities;
        Consumers  d_consumers;

        QueueRoutingContext& d_queue;

        bmqeval::CompilationContext d_compilationContext;

        unsigned int d_priorityCount;

        unsigned int d_upstreamSubQueueId;

        bslma::Allocator* d_allocator_p;

        AppContext(QueueRoutingContext& queue,
                   unsigned int         upstreamSubQueueId,
                   bslma::Allocator*    allocator);

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
        unsigned int finalize();

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
                       const mqbi::StorageIterator* currentMessage,
                       unsigned int                 subscriptionId);

        /// Iterate all `Group`s and call the specified `visitor` for each
        /// highest priority `Subscription` which has `canDeliver` consumer
        /// and which `Expression` matches the specified `currentMessage`.
        /// If the `visitor` returns `true`, stop iterating, move the
        /// subscription to the end of round-robind selection list if it has
        /// been selected `d_consumerPriorityCount` times , and return
        /// `true`.
        Result iterateGroups(const Visitor& visitor, LearningIds::Root& root);

        /// Iterate all highest priority `Subscriber`s and call the
        /// specified `visitor` for each highest priority `Subscription`
        /// which has `canDeliver` consumer and which `Expression` matches
        /// the specified `currentMessage`.  If the `visitor` returns
        /// `true`, stop iterating and return `true`.
        bool iterateConsumers(const Visitor&               visitor,
                              const mqbi::StorageIterator* message);

        void print(bsl::ostream& os, int level, int spacesPerLevel) const;

        /// Iterate all highest priority highest priority `Subscription`s
        /// within the specified `itSubscriber` and return the first one
        /// which has `canDeliver` consumer and which `Expression` matches
        /// the specified `message`.  Otherwise, return `0`.
        const Subscription*
        selectSubscription(const Consumers::SharedItem& itConsumer) const;

        /// Make `SubscriptionId` reference previously generated groups,
        void apply();

        LearningIds::Root& root() { return d_queue.d_root; }

        // ACCESSORS

        /// Load into the specified 'out', internal details about this object.
        void loadInternals(mqbcmd::RoundRobinRouter* out) const;

        void generate(bmqp_ctrlmsg::StreamParameters* streamParameters) const;

        bool hasHandle(mqbi::QueueHandle* handle) const;

        unsigned int priorityCount() const;

        unsigned int id() const;

        static unsigned int nextId();
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
                                       unsigned int         upstreamSubQueueId,

                                       bslma::Allocator* allocator)
: d_groups(allocator)
, d_priorities(allocator)
, d_consumers(allocator)
, d_queue(queue)
, d_compilationContext(allocator)
, d_priorityCount(0)
, d_upstreamSubQueueId(upstreamSubQueueId)
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

inline unsigned int Routers::AppContext::priorityCount() const
{
    return d_priorityCount;
}

inline unsigned int Routers::AppContext::id() const
{
    return d_upstreamSubQueueId;
}

inline unsigned int Routers::AppContext::nextId()
{
    static unsigned int s_nextId = 0;

    return s_nextId++;
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
, d_root(allocator)
, d_preader(new(*allocator) MessagePropertiesReader(schemaLearner, &d_root, allocator),
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
, d_lastRun(0)
, d_lastResult(false)
{
}

inline Routers::Expression::Expression(const Expression& other)
: d_evaluator(other.d_evaluator)
, d_evaluationContext_p(other.d_evaluationContext_p)
, d_lastRun(other.d_lastRun)
, d_lastResult(other.d_lastResult)
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
    return d_itGroup->value().sId();
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

// FREE FUNCTIONS

/// Apply the specified `hashAlgo` to the specified `queueId`.
template <class HASH_ALGORITHM>
void hashAppend(HASH_ALGORITHM& hashAlgo, const PropertyRef& property)
{
    bslh::hashAppend(hashAlgo, property.d_ordinal);
    bdld::hashAppend(hashAlgo, property.d_value);
}

inline bool operator==(const PropertyRef& lhs, const PropertyRef& rhs)
{
    return lhs.d_ordinal == rhs.d_ordinal && lhs.d_value == rhs.d_value;
}

}  // close package namespace
}  // close enterprise namespace

#endif
