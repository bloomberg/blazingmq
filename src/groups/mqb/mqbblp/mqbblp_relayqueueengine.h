// Copyright 2021-2023 Bloomberg Finance L.P.
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

// mqbblp_relayqueueengine.h                                          -*-C++-*-
#ifndef INCLUDED_MQBBLP_RELAYQUEUEENGINE
#define INCLUDED_MQBBLP_RELAYQUEUEENGINE

//@PURPOSE: Provide a QueueEngine in a relay BlazingMQ node (replica or proxy)
//
//@CLASSES:
//  mqbblp::RelayQueueEngine: QueueEngine in a relay BlazingMQ node
//  (replica/proxy)
//
//@DESCRIPTION: 'mqbblp::RelayQueueEngine' provides an 'mqbi::QueueEngine'
// implementation for a relay BlazingMQ node (replica or proxy).
//
/// KNOWN Issues:
///------------
// Since messages are not rejected back to upstream, there is a potential
// situation of pseudo-starvation: let's imagine a proxy has two readers, one
// with maxUnackedMessages of 10, and one with maxUnackedMessages of 100,
// upstream will then deliver up to 110 messages to this proxy.  If the '100
// unackedMessages' goes down, this proxy will keep those messages to
// distribute them to the '10 unackedMessages', which might be undesirable;
// instead the proxy should 'reject' up to 100 messages.

// MQB

#include <mqbblp_queueengineutil.h>
#include <mqbblp_queuehandlecatalog.h>
#include <mqbblp_routers.h>
#include <mqbi_dispatcher.h>
#include <mqbi_queue.h>
#include <mqbi_queueengine.h>
#include <mqbi_storage.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_queueutil.h>
#include <bmqt_messageguid.h>

// MWC
#include <mwcc_orderedhashmap.h>
#include <mwcu_sharedresource.h>

// BDE
#include <ball_log.h>
#include <bdlmt_throttle.h>
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_unordered_map.h>
#include <bsl_utility.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_atomic.h>
#include <bsls_keyword.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class EventScheduler;
}
namespace mqbcmd {
class QueueEngine;
}
namespace mqbi {
class DispatcherClient;
}
namespace mqbs {
class VirtualStorageCatalog;
}

namespace mqbblp {

// FORWARD DECLARATION
class QueueState;

// ================================
// struct RelayQueueEngine_AppState
// ================================

/// RelayQueueEngine needs to keep track of all parameters advertised
/// upstream but not necessarily confirmed per each AppId (including
/// default).
///
/// The difference with RootQueueEngine is that they update queue handle
/// immediately upon `configureHandle` while RelayQueueEngine waits for
/// `onHandleConfiguredDispatched` to update the handle.  Hence the need to
/// have the (extra) cache which is updated immediately.
struct RelayQueueEngine_AppState : QueueEngineUtil_AppState {
    struct CachedParameters {
        bmqp_ctrlmsg::QueueHandleParameters d_handleParameters;
        bmqp_ctrlmsg::StreamParameters      d_streamParameters;
        unsigned int                        d_downstreamSubQueueId;

        CachedParameters(
            const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
            unsigned int                               downstreamSubQueueId,
            bslma::Allocator*                          allocator);
    };

    typedef bsl::unordered_map<mqbi::QueueHandle*, CachedParameters>
        CachedParametersMap;

    CachedParametersMap d_cache;
    // Parameters to advertise upstream for this app

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RelayQueueEngine_AppState,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    explicit RelayQueueEngine_AppState(
        unsigned int                  upstreamSubQueueId,
        const bsl::string&            appId,
        mqbi::Queue*                  queue,
        bdlmt::EventScheduler*        scheduler,
        Routers::QueueRoutingContext& queueContext,
        bslma::Allocator*             allocator = 0);
};

struct PushStream {
    // A Storage for one-time PUSH delivery (non the Reliable Broadcast mode).
    // When PUSH is a result of round-robin, the number of App ids in the PUSH
    // is not equal to the number of Apps known to the RelayQueueEngine.
    // An efficient iteration requires records of variable size per each GUID.
    // On the other side, there is only sequential access - either for each
    // GUID or for each App.  An 'Element' holding PUSH context for a GUID and
    // an App is in two lists - for the GUID for the App.  Removal can be
    // random for the GUID list and always sequential for the App list.
    // The storage still supports 'mqbi::StorageIterator' interface but the
    // meaning of 'appOrdinal' in the 'appMessageView' is different; the access
    // is always sequential with monotonically increasing 'appOrdinal' and the
    // 'appOrdinal' can be different for the same App depending on the GUID.
    // Upon GUIDs iteration followed by he GUID list iteration, if the App
    // succeeds in delivering the PUSH, the engine removes the 'Element' from
    // both lists.  If the App is at capacity, the 'Element' stays, the
    // iterations continue.  Upon 'onHandleUsable', the App need to catchup by
    // iterating the App list.

    // forward declaration
    struct Element;

    enum eElementList { e_GUID = 0, e_APP = 1, e_TOTAL = 2 };

    struct ElementBase {
        Element* d_next_p;
        Element* d_previous_p;

        ElementBase();
    };

    struct Elements {
        // A list of Elements to associate Elements with 1) GUID, 2) App.
        // In the case of GUID, the list is doubly-linked for random removal
        // In the case of App, the list is singly-linked; the removal is always
        // sequential.

      private:
        Element*     d_first_p;
        Element*     d_last_p;
        unsigned int d_numElements;

      private:
        void onRemove();
        void onAdd(Element* element);

      public:
        Elements();

        /// Add the specified 'element' to doubly-linked list for GUID
        void add(Element* element, eElementList where);

        /// Remove the specified 'element' from doubly-linked list for GUID
        void remove(Element* element, eElementList where);

        /// Return the first Element in the list
        Element*     first() const;
        unsigned int numElements() const;
    };

    struct App {
        Elements                                   d_elements;
        bsl::shared_ptr<RelayQueueEngine_AppState> d_app;

        App(const bsl::shared_ptr<RelayQueueEngine_AppState>& app);
        void add(Element* element);
        void remove(Element* element);
    };

    typedef mwcc::OrderedHashMap<bmqt::MessageGUID,
                                 Elements,
                                 bslh::Hash<bmqt::MessageGUIDHashAlgo> >
                                                  Stream;
    typedef Stream::iterator                      iterator;
    typedef bsl::unordered_map<unsigned int, App> Apps;

    struct Element {
        friend struct Elements;

      private:
        ElementBase          d_base[e_TOTAL];
        mqbi::AppMessage     d_app;
        const iterator       d_iteratorGuid;
        const Apps::iterator d_iteratorApp;

      public:
        Element(const bmqp::SubQueueInfo& subscription,
                const iterator&           iterator,
                const Apps::iterator&     iteratorApp);

        /// Return a modifiable reference to the App state associated with this
        /// Element.
        mqbi::AppMessage* appState();

        /// Return a non-modifiable reference to the App state associated with
        /// this Element.
        const mqbi::AppMessage* appView() const;

        /// Return the GUID associated with this Element.
        Elements& guid() const;
        App&      app() const;

        void eraseGuid(Stream& stream);
        void eraseApp(Apps& apps);

        /// Return true if this Element is associated with the specified
        /// 'iterator' position in the PushStream.
        bool isInStream(const PushStream::Stream::iterator& iterator) const;

        /// Return reference to the next Element associated with the same GUID
        /// or '0' if this is the last Element.
        Element* next() const;

        /// Return reference to the next Element associated with the same App
        /// or '0' if this is the last Element.
        Element* nextInApp() const;
    };

    Stream d_stream;

    Apps d_apps;

    bdlma::ConcurrentPool* d_pushElementsPool_p;

    PushStream(bdlma::ConcurrentPool* pushElementsPool);

    iterator add(const bmqt::MessageGUID& guid);

    /// Add the specified 'element' to both GUID and App corresponding to the
    /// 'element' (and specified when constructing the 'element').
    void add(Element* element);

    /// Remove the specified 'element' from both GUID and App corresponding to
    /// the 'element' (and specified when constructing the 'element').
    /// Erase the corresponding App from the PushStream if there is no
    /// remaining Elements in the App.
    /// Return the number of remaining Elements in the corresponding GUID.
    unsigned int remove(Element* element);

    /// Remove all PushStream Elements corresponding to the specified
    /// 'upstreamSubQueueId'.  Erase each corresponding GUIDs from the
    /// PushStream with no remaining Elements.
    /// Erase the corresponding App.
    unsigned int removeApp(unsigned int upstreamSubQueueId);

    /// Remove all PushStream Elements corresponding to the specified
    /// 'itApp'.    Erase each corresponding GUIDs from the  PushStream with no
    /// remaining Elements.
    /// Erase the corresponding App.
    unsigned int removeApp(Apps::iterator itApp);

    /// Remove all Elements, Apps, and GUIDs.
    unsigned int removeAll();

    /// Create new Element associated with the specified 'info',
    // 'upstreamSubQueueId', and 'iterator'.
    Element* create(const bmqp::SubQueueInfo& info,
                    const iterator&           iterator,
                    const Apps::iterator&     iteratorApp);

    /// Destroy the specified 'element'
    void destroy(Element* element, bool doKeepGuid);
};

// ==========================================
// class RelayQueueEngine_PushStorageIterator
// ==========================================

class RelayQueueEngine_PushStorageIterator : public mqbi::StorageIterator {
    // A mechanism to iterate the PushStream; see above.  To be used by the
    // QueueEngine routing in the same way as another 'mqbi::StorageIterator'
    // implementation(s).

  private:
    // DATA
    mqbi::Storage* d_storage_p;

    PushStream::iterator d_iterator;

    mutable mqbi::StorageMessageAttributes d_attributes;

    mutable bsl::shared_ptr<bdlbb::Blob> d_appData_sp;
    // If this variable is empty, it is
    // assumed that attributes, message,
    // and options have not been loaded in
    // this iteration (see also
    // 'loadMessageAndAttributes' impl).

    mutable bsl::shared_ptr<bdlbb::Blob> d_options_sp;

  protected:
    PushStream*                  d_owner_p;
    mutable PushStream::Element* d_currentElement;
    // Current ('mqbi::AppMessage', 'upstreamSubQueueId') pair.
    mutable unsigned int d_currentOrdinal;
    // Current ordinal corresponding to the 'd_currentElement'.

  private:
    // NOT IMPLEMENTED
    RelayQueueEngine_PushStorageIterator(const StorageIterator&);  // = delete
    RelayQueueEngine_PushStorageIterator&
    operator=(const RelayQueueEngine_PushStorageIterator&);  // = delete

  protected:
    // PRIVATE MANIPULATORS

    /// Clear previous state, if any.  This is required so that new state
    /// can be loaded in `appData`, `options` or `attributes` routines.
    void clear();

    // PRIVATE ACCESSORS

    /// Load the internal state of this iterator instance with the
    /// attributes and blob pointed to by the MessageGUID to which this
    /// iterator is currently pointing.  Behavior is undefined if `atEnd()`
    /// returns true or if underlying storage does not contain the
    /// MessageGUID being pointed to by this iterator.  Return `false` if
    /// data are already loaded; return `true` otherwise.
    bool loadMessageAndAttributes() const;

  public:
    // CREATORS

    /// Create a new VirtualStorageIterator from the specified `storage` and
    /// pointing at the specified `initialPosition`.
    RelayQueueEngine_PushStorageIterator(
        mqbi::Storage*              storage,
        PushStream*                 owner,
        const PushStream::iterator& initialPosition);

    /// Destructor
    ~RelayQueueEngine_PushStorageIterator() BSLS_KEYWORD_OVERRIDE;

    /// Remove the current element ('mqbi::AppMessage', 'upstreamSubQueueId'
    /// pair) from the current PUSH GUID.
    /// The behavior is undefined unless `atEnd` returns `false`.
    void removeCurrentElement();

    /// Return the number of elements ('mqbi::AppMessage', 'upstreamSubQueueId'
    /// pairs) fir the current PUSH GUID.
    /// The behavior is undefined unless `atEnd` returns `false`.
    unsigned int numApps() const;

    /// Return the current element ('mqbi::AppMessage', 'upstreamSubQueueId'
    /// pair).
    /// The behavior is undefined unless `atEnd` returns `false`.
    virtual PushStream::Element* element(unsigned int appOrdinal) const;

    // MANIPULATORS
    bool advance() BSLS_KEYWORD_OVERRIDE;

    /// If the specified 'atEnd' is 'true', reset the iterator to point to the
    /// to the end of the underlying storage.  Otherwise, reset the iterator to
    /// point first item, if any, in the underlying storage.
    void reset(const bmqt::MessageGUID& where = bmqt::MessageGUID())
        BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return a reference offering non-modifiable access to the guid
    /// associated to the item currently pointed at by this iterator.  The
    /// behavior is undefined unless `atEnd` returns `false`.
    const bmqt::MessageGUID& guid() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering non-modifiable access to the App state
    /// associated to the item currently pointed at by this iterator.  The
    /// behavior is undefined unless `atEnd` returns `false`.
    const mqbi::AppMessage&
    appMessageView(unsigned int appOrdinal) const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering modifiable access to the App state
    /// associated to the item currently pointed at by this iterator.  The
    /// behavior is undefined unless `atEnd` returns `false`.
    mqbi::AppMessage&
    appMessageState(unsigned int appOrdinal) BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering non-modifiable access to the application
    /// data associated with the item currently pointed at by this iterator.
    /// The behavior is undefined unless `atEnd` returns `false`.
    const bsl::shared_ptr<bdlbb::Blob>& appData() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering non-modifiable access to the options
    /// associated with the item currently pointed at by this iterator.  The
    /// behavior is undefined unless `atEnd` returns `false`.
    const bsl::shared_ptr<bdlbb::Blob>& options() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering non-modifiable access to the attributes
    /// associated with the message currently pointed at by this iterator.
    /// The behavior is undefined unless `atEnd` returns `false`.
    const mqbi::StorageMessageAttributes&
    attributes() const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if this iterator is currently at the end of the items'
    /// collection, and hence doesn't reference a valid item.
    bool atEnd() const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if this iterator is currently not at the end of the
    /// `items` collection and the message currently pointed at by this
    /// iterator has received replication factor Receipts.
    bool hasReceipt() const BSLS_KEYWORD_OVERRIDE;
};

// ============================
// class VirtualStorageIterator
// ============================

class RelayQueueEngine_VirtualPushStorageIterator
: public RelayQueueEngine_PushStorageIterator {
    // A mechanism to iterate Elements related to one App only.

  private:
    // DATA

    PushStream::Apps::iterator d_itApp;
    // An iterator to the App being iterated

  private:
    // NOT IMPLEMENTED
    RelayQueueEngine_VirtualPushStorageIterator(
        const RelayQueueEngine_VirtualPushStorageIterator&);  // = delete
    RelayQueueEngine_VirtualPushStorageIterator&
    operator=(const RelayQueueEngine_VirtualPushStorageIterator&);  // = delete

  public:
    // CREATORS

    /// Create a new VirtualStorageIterator from the specified `storage` and
    /// pointing at the specified `initialPosition`.
    RelayQueueEngine_VirtualPushStorageIterator(
        unsigned int                upstreamSubQueueId,
        mqbi::Storage*              storage,
        PushStream*                 owner,
        const PushStream::iterator& initialPosition);

    /// Destructor
    ~RelayQueueEngine_VirtualPushStorageIterator() BSLS_KEYWORD_OVERRIDE;

    /// Return the current element ('mqbi::AppMessage', 'upstreamSubQueueId'
    /// pair).
    /// The behavior is undefined unless `atEnd` returns `false`.
    virtual PushStream::Element* element(unsigned int appOrdinal) const;

    // MANIPULATORS
    bool advance() BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if this iterator is currently at the end of the items'
    /// collection, and hence doesn't reference a valid item.
    bool atEnd() const BSLS_KEYWORD_OVERRIDE;
};

// FREE OPERATORS
bool operator==(const RelayQueueEngine_VirtualPushStorageIterator& lhs,
                const RelayQueueEngine_VirtualPushStorageIterator& rhs);

// ======================
// class RelayQueueEngine
// ======================

/// QueueEngine implementation for a BlazingMQ relay node (replica/proxy).
class RelayQueueEngine : public mqbi::QueueEngine {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.RELAYQUEUEENGINE");

  private:
    // PRIVATE TYPES
    typedef bslma::ManagedPtr<mqbi::StorageIterator> StorageIterMp;
    typedef bsl::shared_ptr<mqbi::StorageIterator>   StorageIterSp;

    /// (queueHandlePtr, downstreamSubQueueId)
    typedef QueueHandleCatalog::DownstreamKey DownstreamKey;

    typedef RelayQueueEngine_AppState  App_State;
    typedef bsl::shared_ptr<App_State> AppStateSp;

    /// (subId) -> App_State map
    typedef bsl::unordered_map<unsigned int, AppStateSp> AppsMap;

    /// (appId) -> App_State map
    typedef bsl::unordered_map<bsl::string, AppStateSp> AppIds;

    class AutoPurger;
    // A guard helper class.

    friend class AutoPurger;
    // Has to have access to private member variables.

    /// This struct serves as multiplexor when sending configure request(s)
    /// (plural in the case of wildcard) to upstream.  Once all responses
    /// are collected (there are no more references), it calls the specified
    /// callback
    struct ConfigureContext {
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(ConfigureContext,
                                       bslma::UsesBslmaAllocator)

        mqbi::QueueHandle::HandleConfiguredCallback d_configuredCb;
        bmqp_ctrlmsg::Status                        d_status;
        const bmqp_ctrlmsg::StreamParameters        d_streamParameters;

        /// Routing data advertised upstream (including Subscription Ids).
        /// Upon response from upstream, this becomes effective.
        bsl::shared_ptr<Routers::AppContext> d_routing_sp;

        bslma::Allocator* d_allocator_p;

        ConfigureContext(
            const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb,
            const bmqp_ctrlmsg::StreamParameters& streamParameters,
            bslma::Allocator*                     allocator);

        ~ConfigureContext();

        void setStatus(const bmqp_ctrlmsg::StatusCategory::Value& category,
                       int                                        code,
                       const bslstl::StringRef&                   message);

        void invokeCallback();

        void resetCallback();

        void reset();

        void initializeRouting(Routers::QueueRoutingContext& queueContext);
    };

  private:
    // DATA
    QueueState* d_queueState_p;

    PushStream d_pushStream;

    const mqbconfm::Domain d_domainConfig;

    AppsMap d_apps;
    // Map of (appId) to App_State.

    AppIds d_appIds;
    // (appId) -> App_State map

    mwcu::SharedResource<RelayQueueEngine> d_self;
    // Used to avoid executing a callback if
    // the engine has been destroyed.  For
    // example, upon queue converting to local.

    bdlmt::Throttle d_throttledRejectedMessages;
    // Throttler for REJECTs.

    QueueEngineUtil_AppsDeliveryContext d_appsDeliveryContext;
    // Reusable apps delivery context
    bslma::ManagedPtr<RelayQueueEngine_PushStorageIterator> d_storageIter_mp;
    // Storage iterator to the PushStream

    bslma::ManagedPtr<mqbi::StorageIterator> d_realStorageIter_mp;
    // Storage iterator to access storage state.

    bslma::Allocator* d_allocator_p;
    // Allocator to use.
  private:
    // PRIVATE CLASS METHODS

    /// Routine executed after `getHandle` method on this queue engine
    /// instance has been invoked, where the specified `ptr` is a pointer
    /// to the queue engine instance, and the specified `cookie` is a
    /// pointer to a boolean flag indicating whether the handle was created
    /// or not.
    static void onHandleCreation(void* ptr, void* cookie);

  private:
    // PRIVATE MANIPULATORS

    /// Schedule processing of the stream configuration response from
    /// upstream of the specified `handle` with the specified
    /// `downStreamParameters` per the specified `status`.  The specified
    /// `upStreamParameters` is the upstream view of the stream parameters
    /// of this node.  When the specified `context` reference count drops to
    /// zero, invoke associated callback. Note that the specified `self`
    /// must be locked to ensure the engine is still alive at the time the
    /// callback is invoked.
    ///
    /// THREAD: This method is called from any thread.
    void onHandleConfigured(
        const bsl::weak_ptr<RelayQueueEngine>&   self,
        const bmqp_ctrlmsg::Status&              status,
        const bmqp_ctrlmsg::StreamParameters&    upStreamParameters,
        mqbi::QueueHandle*                       handle,
        const bmqp_ctrlmsg::StreamParameters&    downStreamParameters,
        const bsl::shared_ptr<ConfigureContext>& context);

    /// Process stream configuration response from upstream of the specified
    /// `handle` with the specified `downStreamParameters` per the specified
    /// `status`.  The specified `upStreamParameters` is the upstream view
    /// of the stream parameters of this node.  When the specified `context`
    /// reference count drops to zero, invoke associated callback.  Note
    /// that the specified `self` must be locked to ensure the engine is
    /// still alive at the time the callback is invoked.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void onHandleConfiguredDispatched(
        const bsl::weak_ptr<RelayQueueEngine>&   self,
        const bmqp_ctrlmsg::Status&              status,
        const bmqp_ctrlmsg::StreamParameters&    upStreamParameters,
        mqbi::QueueHandle*                       handle,
        const bmqp_ctrlmsg::StreamParameters&    downStreamParameters,
        const bsl::shared_ptr<ConfigureContext>& context);

    /// Schedule processing of the release response from upstream of the
    /// specified `handle` with the specified `handleParameters` per the
    /// specified `status` and associated with the specified `clientCb`.
    /// The specified `isFinal` indicates if the handle operation is final.
    ///
    /// THREAD: This method is called from any thread.
    void onHandleReleased(
        const bmqp_ctrlmsg::Status&                                  status,
        mqbi::QueueHandle*                                           handle,
        const bmqp_ctrlmsg::QueueHandleParameters&                   hp,
        const bsl::shared_ptr<QueueEngineUtil_ReleaseHandleProctor>& context);

    void onHandleReleasedDispatched(
        const bmqp_ctrlmsg::Status&                                  status,
        mqbi::QueueHandle*                                           handle,
        const bmqp_ctrlmsg::QueueHandleParameters&                   hp,
        const bsl::shared_ptr<QueueEngineUtil_ReleaseHandleProctor>& context);

    /// Attempt to deliver outstanding messages, if any, to the active
    /// consumers.  Behavior is undefined unless there is at least one
    /// active consumer.
    void deliverMessages();
    void processAppRedelivery(unsigned int upstreamSubQueueId, App_State* app);

    /// Configure the specified `handle` with the specified
    /// `streamParameters` for the specified `appState`.  When the specified
    /// `context` reference count drops to zero, invoke associated callback.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void configureApp(App_State&                            appState,
                      mqbi::QueueHandle*                    handle,
                      const bmqp_ctrlmsg::StreamParameters& streamParameters,
                      const bsl::shared_ptr<ConfigureContext>& context);

    void releaseHandleImpl(
        mqbi::QueueHandle*                                           handle,
        const bmqp_ctrlmsg::QueueHandleParameters&                   hp,
        const bsl::shared_ptr<QueueEngineUtil_ReleaseHandleProctor>& proctor);

    /// Rebuild the effective upstream state for the specified
    /// `upstreamSubQueueId` using parameters cached in the specified
    /// `appState`.  The `upstreamSubQueueId` is k_DEFAULT_SUBQUEUE_ID
    /// unless the queue is of the fanout type.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void rebuildUpstreamState(Routers::AppContext* context,
                              App_State*           appState,
                              unsigned int         upstreamSubQueueId,
                              const bsl::string&   appId);

    /// Make effective the previously built routing results containing
    /// Subscription ids advertised upstream.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void applyConfiguration(App_State& appState, ConfigureContext& context);

    mqbi::Storage* storage() const;

    App_State* findApp(unsigned int upstreamSubQueueId) const;

    bool checkForDuplicate(const App_State*         app,
                           const bmqt::MessageGUID& msgGUID);

    void storePush(mqbi::StorageMessageAttributes*     attributes,
                   const bmqt::MessageGUID&            msgGUID,
                   const bsl::shared_ptr<bdlbb::Blob>& appData,
                   bool                                isOutOfOrder);

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    RelayQueueEngine(const RelayQueueEngine&);
    RelayQueueEngine& operator=(const RelayQueueEngine&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RelayQueueEngine, bslma::UsesBslmaAllocator)

  public:
    // CREATORS
    RelayQueueEngine(QueueState*             queueState,
                     const mqbconfm::Domain& domainConfig,
                     bslma::Allocator*       allocator);

    /// Destructor
    virtual ~RelayQueueEngine() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual mqbi::QueueEngine)

    /// Configure this instance.  Return zero on success, non-zero value
    /// otherwise and populate the specified `errorDescription`.
    virtual int
    configure(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Reset the internal state of this engine.  If the optionally specified
    /// 'keepConfirming' is 'true', keep the data structures for CONFIRMs
    /// processing.
    virtual void resetState(bool isShuttingDown = false) BSLS_KEYWORD_OVERRIDE;

    /// Rebuild the internal state of this engine.  This method is invoked
    /// when the queue this engine is associated with is created from an
    /// existing one, and takes ownership of the already created handles
    /// (typically happens when the queue gets converted between local and
    /// remote).  Return zero on success, non-zero value otherwise and
    /// populate the specified `errorDescription`.  Note that
    /// `rebuildInternalState` must be called on an empty-state object
    /// (i.e., which has just been constructed, or following a call to
    /// `resetState`) after it has been configured.
    virtual int
    rebuildInternalState(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Obtain and return a handle to this queue for the client identified
    /// with the specified `clientContext`, using the specified
    /// `handleParameters`, and invoke the specified `callback` when
    /// finished. In case of error, return a null pointer.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual mqbi::QueueHandle*
    getHandle(const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                                                          clientContext,
              const bmqp_ctrlmsg::QueueHandleParameters&  handleParameters,
              unsigned int                                upstreamSubQueueId,
              const mqbi::QueueHandle::GetHandleCallback& callback)
        BSLS_KEYWORD_OVERRIDE;

    /// Configure the specified `handle` with the specified
    /// `streamParameters` and invoke the specified `configuredCb` when
    /// finished.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void configureHandle(
        mqbi::QueueHandle*                                 handle,
        const bmqp_ctrlmsg::StreamParameters&              streamParameters,
        const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb)
        BSLS_KEYWORD_OVERRIDE;

    /// Reconfigure the specified `handle` by releasing the specified
    /// `parameters` from its current settings and invoke the specified
    /// `releasedCb` when done.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void
    releaseHandle(mqbi::QueueHandle*                         handle,
                  const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                  bool                                       isFinal,
                  const mqbi::QueueHandle::HandleReleasedCallback& releasedCb)
        BSLS_KEYWORD_OVERRIDE;

    /// Called when the specified `handle` is usable and ready to receive
    /// messages (usually meaning its client has become available) for the
    /// specified `upstreamSubscriptionId` subscription of the queue.  When
    /// this method is called, the queue engine should deliver outstanding
    /// messages to the `handle`.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void
    onHandleUsable(mqbi::QueueHandle* handle,
                   unsigned int upstreamSubscriptionId) BSLS_KEYWORD_OVERRIDE;

    /// Called by the mqbi::Queue when a new message with the specified
    /// `msgGUID` is available on the queue and ready to be sent to eventual
    /// interested clients.  If available, the specified `source` points to
    /// the originator of the message.
    virtual void
    afterNewMessage(const bmqt::MessageGUID& msgGUID,
                    mqbi::QueueHandle*       source) BSLS_KEYWORD_OVERRIDE;

    /// Called by the `mqbi::Queue` when the message identified by the
    /// specified `msgGUID` is confirmed for the specified
    /// `upstreamSubQueueId` stream of the queue on behalf of the client
    /// identified by the specified `handle`.  Return a negative value on
    /// error (GUID was not found, etc.), 0 if this confirm was for the last
    /// reference to that message and it can be deleted from the queue's
    /// associated storage, or 1 if there are still references.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual int
    onConfirmMessage(mqbi::QueueHandle*       handle,
                     const bmqt::MessageGUID& msgGUID,
                     unsigned int upstreamSubQueueId) BSLS_KEYWORD_OVERRIDE;

    /// Called by the `mqbi::Queue` when the message identified by the
    /// specified `msgGUID` is rejected for the specified
    /// `upstreamSubQueueId` stream of the queue on behalf of the client
    /// identified by the specified `handle`.  Return resulting RDA counter.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    int onRejectMessage(mqbi::QueueHandle*       handle,
                        const bmqt::MessageGUID& msgGUID,
                        unsigned int upstreamSubQueueId) BSLS_KEYWORD_OVERRIDE;

    /// Called by the mqbi::Queue before a message with the specified
    /// `msgGUID` is removed from the queue (either it's TTL expired, it was
    /// confirmed by all recipients, etc). The QueueEngine may use this to
    /// update the positions of the QueueHandles it manages.
    virtual void beforeMessageRemoved(const bmqt::MessageGUID& msgGUID)
        BSLS_KEYWORD_OVERRIDE;

    /// Called by the mqbi::Queue *after* *all* messages are removed from
    /// the storage for the client identified by the specified `appId` and
    /// `appKey` (queue has been deleted or purged by admin task, etc).
    /// QueueEngine may use this to update the positions of the QueueHandles
    /// it manages.  Note that `appKey` may be null, in which case the
    /// `purge` action is applicable to the entire queue.  Also note that
    /// `appId` must be empty if and only if `appKey` is null.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void
    afterQueuePurged(const bsl::string&      appId,
                     const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    /// Periodically invoked with the current time provided in the specified
    /// `currentTimer`; can be used for regular status check, such as for
    /// ensuring messages on the queue are flowing and not accumulating.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void
    onTimer(bsls::Types::Int64 currentTimer) BSLS_KEYWORD_OVERRIDE;

    /// Called after the specified `appIdKeyPair` has been dynamically
    /// registered.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void afterAppIdRegistered(
        const mqbi::Storage::AppIdKeyPair& appIdKeyPair) BSLS_KEYWORD_OVERRIDE;

    /// Called after the specified `appIdKeyPair` has been dynamically
    /// unregistered.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void afterAppIdUnregistered(
        const mqbi::Storage::AppIdKeyPair& appIdKeyPair) BSLS_KEYWORD_OVERRIDE;

    /// Not valid for 'RelayQueueEngine'
    mqbi::StorageResult::Enum evaluateAutoSubscriptions(
        const bmqp::PutHeader&              putHeader,
        const bsl::shared_ptr<bdlbb::Blob>& appData,
        const bmqp::MessagePropertiesInfo&  mpi,
        bsls::Types::Uint64                 timestamp) BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// If the specified 'isOutOfOrder' is 'true', insert the specified
    // 'msgGUID' into the corresponding App redelivery list.  Otherwise, insert
    // the 'msgGUID' into the PushStream; insert PushStream Elements
    // ('mqbi::AppMessage', 'upstreamSubQueueId') pairs for each recognized App
    /// in the specified 'subscriptions'.
    unsigned int push(mqbi::StorageMessageAttributes*           attributes,
                      const bmqt::MessageGUID&                  msgGUID,
                      const bsl::shared_ptr<bdlbb::Blob>&       appData,
                      const bmqp::Protocol::SubQueueInfosArray& subscriptions,
                      bool                                      isOutOfOrder);
    // ACCESSORS

    /// Return the reference count that should be applied to a message
    /// posted to the queue managed by this engine.  Note that returned
    /// value may or may not be equal to `numOpenReaderHandles()` depending
    /// upon the specific type of this engine.
    virtual unsigned int messageReferenceCount() const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `out` object the internal information about
    /// this queue engine and associated queue handles.
    virtual void
    loadInternals(mqbcmd::QueueEngine* out) const BSLS_KEYWORD_OVERRIDE;

    /// Load upstream subQueue id into the specified `subQueueId` given the
    /// specified upstream `subscriptionId`.
    /// Each subStream has unique Subscription ids.
    bool subscriptionId2upstreamSubQueueId(const bmqt::MessageGUID& msgGUID,
                                           unsigned int*            subQueueId,
                                           unsigned int subscriptionId) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------------
// struct RelayQueueEngine_AppState
// --------------------------------

inline RelayQueueEngine_AppState::RelayQueueEngine_AppState(
    unsigned int                  upstreamSubQueueId,
    const bsl::string&            appId,
    mqbi::Queue*                  queue,
    bdlmt::EventScheduler*        scheduler,
    Routers::QueueRoutingContext& queueContext,
    bslma::Allocator*             allocator)
: QueueEngineUtil_AppState(queue,
                           scheduler,
                           queueContext,
                           upstreamSubQueueId,
                           appId,
                           mqbu::StorageKey::k_NULL_KEY,
                           allocator)
, d_cache(allocator)
{
    // NOTHING
}

// --------------------------------------------------
// struct RelayQueueEngine_AppState::CachedParameters
// --------------------------------------------------

inline RelayQueueEngine_AppState::CachedParameters::CachedParameters(
    const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
    unsigned int                               downstreamSubQueueId,
    bslma::Allocator*                          allocator)
: d_handleParameters(handleParameters)
, d_streamParameters(allocator)
, d_downstreamSubQueueId(downstreamSubQueueId)
{
    // NOTHING
}

// -----------------------------------------
// struct RelayQueueEngine::ConfigureContext
// -----------------------------------------

inline RelayQueueEngine::ConfigureContext::ConfigureContext(
    const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb,
    const bmqp_ctrlmsg::StreamParameters&              streamParameters,
    bslma::Allocator*                                  allocator)
: d_configuredCb(configuredCb)
, d_status()
, d_streamParameters(streamParameters, allocator)
, d_routing_sp()
, d_allocator_p(allocator)
{
    d_status.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
    d_status.code()     = 0;
}

inline RelayQueueEngine::ConfigureContext::~ConfigureContext()
{
    invokeCallback();
}

inline void RelayQueueEngine::ConfigureContext::setStatus(
    const bmqp_ctrlmsg::StatusCategory::Value& category,
    int                                        code,
    const bslstl::StringRef&                   message)
{
    d_status.category() = category;
    d_status.code()     = code;
    d_status.message()  = message;
}

inline void RelayQueueEngine::ConfigureContext::invokeCallback()
{
    if (d_configuredCb) {
        d_configuredCb(d_status, d_streamParameters);
        resetCallback();
    }
}

inline void RelayQueueEngine::ConfigureContext::resetCallback()
{
    d_configuredCb = bsl::nullptr_t();
}

inline void RelayQueueEngine::ConfigureContext::initializeRouting(
    Routers::QueueRoutingContext& queueContext)
{
    d_routing_sp.reset(new (*d_allocator_p)
                           Routers::AppContext(queueContext, d_allocator_p),
                       d_allocator_p);
}

// --------------------------
// struct PushStream::Element
// --------------------------
inline PushStream::ElementBase::ElementBase()
: d_next_p(0)
, d_previous_p(0)
{
    // NOTHING
}

inline PushStream::Element::Element(const bmqp::SubQueueInfo& subscription,
                                    const iterator&           iterator,
                                    const Apps::iterator&     iteratorApp)
: d_app(subscription.rdaInfo())
, d_iteratorGuid(iterator)
, d_iteratorApp(iteratorApp)
{
    d_app.d_subscriptionId = subscription.id();
}

inline void PushStream::Element::eraseGuid(PushStream::Stream& stream)
{
    stream.erase(d_iteratorGuid);
}

inline void PushStream::Element::eraseApp(PushStream::Apps& apps)
{
    apps.erase(d_iteratorApp);
}

inline mqbi::AppMessage* PushStream::Element::appState()
{
    return &d_app;
}

inline const mqbi::AppMessage* PushStream::Element::appView() const
{
    return &d_app;
}

inline PushStream::Elements& PushStream::Element::guid() const
{
    return d_iteratorGuid->second;
}

inline PushStream::App& PushStream::Element::app() const
{
    return d_iteratorApp->second;
}

inline bool PushStream::Element::isInStream(
    const PushStream::Stream::iterator& iterator) const
{
    return d_iteratorGuid != iterator;
}

inline PushStream::Element* PushStream::Element::next() const
{
    return d_base[e_GUID].d_next_p;
}

inline PushStream::Element* PushStream::Element::nextInApp() const
{
    return d_base[e_APP].d_next_p;
}

// ---------------------------
// struct PushStream::Elements
// ---------------------------

inline PushStream::Elements::Elements()
: d_first_p(0)
, d_last_p(0)
, d_numElements(0)
{
    // NOTHING
}

inline void PushStream::Elements::onAdd(Element* element)
{
    if (++d_numElements == 1) {
        BSLS_ASSERT_SAFE(d_first_p == 0);
        BSLS_ASSERT_SAFE(d_last_p == 0);

        d_first_p = element;
        d_last_p  = element;
    }
    else {
        BSLS_ASSERT_SAFE(d_first_p);
        BSLS_ASSERT_SAFE(d_last_p);

        d_last_p = element;
    }
}

inline void PushStream::Elements::onRemove()
{
    BSLS_ASSERT_SAFE(d_numElements);

    if (--d_numElements == 0) {
        BSLS_ASSERT_SAFE(d_first_p == 0);
        BSLS_ASSERT_SAFE(d_last_p == 0);
    }
    else {
        BSLS_ASSERT_SAFE(d_first_p);
        BSLS_ASSERT_SAFE(d_last_p);
    }
}

inline void PushStream::Elements::remove(Element* element, eElementList where)
{
    BSLS_ASSERT_SAFE(element);

    if (d_first_p == element) {
        BSLS_ASSERT_SAFE(element->d_base[where].d_previous_p == 0);

        d_first_p = element->d_base[where].d_next_p;
    }
    else {
        BSLS_ASSERT_SAFE(element->d_base[where].d_previous_p);

        element->d_base[where].d_previous_p->d_base[where].d_next_p =
            element->d_base[where].d_next_p;
    }

    if (d_last_p == element) {
        BSLS_ASSERT_SAFE(element->d_base[where].d_next_p == 0);

        d_last_p = element->d_base[where].d_previous_p;
    }
    else {
        BSLS_ASSERT_SAFE(element->d_base[where].d_next_p);

        element->d_base[where].d_next_p->d_base[where].d_previous_p =
            element->d_base[where].d_previous_p;
    }

    onRemove();

    element->d_base[where].d_previous_p = element->d_base[where].d_next_p = 0;
}

inline void PushStream::Elements::add(Element* element, eElementList where)
{
    BSLS_ASSERT_SAFE(element->d_base[where].d_previous_p == 0);
    BSLS_ASSERT_SAFE(element->d_base[where].d_next_p == 0);

    element->d_base[where].d_previous_p = d_last_p;

    if (d_last_p) {
        BSLS_ASSERT_SAFE(d_last_p->d_base[where].d_next_p == 0);

        d_last_p->d_base[where].d_next_p = element;
    }

    onAdd(element);
}

inline PushStream::Element* PushStream::Elements::first() const
{
    return d_first_p;
}

inline unsigned int PushStream::Elements::numElements() const
{
    return d_numElements;
}

inline PushStream::App::App(
    const bsl::shared_ptr<RelayQueueEngine_AppState>& app)
: d_elements()
, d_app(app)
{
}

inline void PushStream::App::add(Element* element)
{
    d_elements.add(element, e_APP);
}
inline void PushStream::App::remove(Element* element)
{
    d_elements.remove(element, e_APP);
}

// ------------------
// struct PushStream
// -----------------

inline PushStream::PushStream(bdlma::ConcurrentPool* pushElementsPool)
: d_pushElementsPool_p(pushElementsPool)
{
    BSLS_ASSERT_SAFE(d_pushElementsPool_p->blockSize() == sizeof(Element));
}
inline PushStream::Element*
PushStream::create(const bmqp::SubQueueInfo& subscription,
                   const iterator&           it,
                   const Apps::iterator&     iteratorApp)
{
    BSLS_ASSERT_SAFE(it != d_stream.end());

    Element* element = new (d_pushElementsPool_p->allocate())
        Element(subscription, it, iteratorApp);
    return element;
}

inline void PushStream::destroy(Element* element, bool doKeepGuid)
{
    if (element->app().d_elements.numElements() == 0) {
        element->eraseApp(d_apps);
    }

    if (!doKeepGuid && element->guid().numElements() == 0) {
        element->eraseGuid(d_stream);
    }

    d_pushElementsPool_p->deallocate(element);
}

inline PushStream::iterator PushStream::add(const bmqt::MessageGUID& guid)
{
    return d_stream.insert(bsl::make_pair(guid, Elements())).first;
}

inline void PushStream::add(Element* element)
{
    // Add to the GUID
    BSLS_ASSERT_SAFE(element);
    BSLS_ASSERT_SAFE(element->isInStream(d_stream.end()));

    element->guid().add(element, e_GUID);

    // Add to the App
    element->app().add(element);
}

inline unsigned int PushStream::remove(Element* element)
{
    BSLS_ASSERT_SAFE(element);
    BSLS_ASSERT_SAFE(element->isInStream(d_stream.end()));

    // remove from the App
    element->app().remove(element);

    // remove from the guid
    element->guid().remove(element, e_GUID);

    return element->guid().numElements();
}

inline unsigned int PushStream::removeApp(unsigned int upstreamSubQueueId)
{
    // remove from the App
    Apps::iterator itApp = d_apps.find(upstreamSubQueueId);

    unsigned int numMessages = 0;
    if (itApp != d_apps.end()) {
        numMessages = removeApp(itApp);
    }

    return numMessages;
}

inline unsigned int PushStream::removeApp(Apps::iterator itApp)
{
    unsigned int numElements = itApp->second.d_elements.numElements();
    for (unsigned int count = 0; count < numElements; ++count) {
        Element* element = itApp->second.d_elements.first();

        remove(element);

        destroy(element, false);
        // do not keep Guid
    }

    return numElements;
}

inline unsigned int PushStream::removeAll()
{
    unsigned int numMessages = 0;

    while (!d_apps.empty()) {
        numMessages += removeApp(d_apps.begin());
    }

    return numMessages;
}

// ----------------------
// class RelayQueueEngine
// ----------------------

inline RelayQueueEngine::App_State*
RelayQueueEngine::findApp(unsigned int upstreamSubQueueId) const
{
    AppsMap::const_iterator cit = d_apps.find(upstreamSubQueueId);

    if (cit == d_apps.end()) {
        return 0;
    }
    else {
        return cit->second.get();
    }
}

inline unsigned int RelayQueueEngine::messageReferenceCount() const
{
    // Irrespective of number of worker-consumers or their status (dead or
    // alive), we always return 1
    return 1;
}

}  // close package namespace
}  // close enterprise namespace

#endif
