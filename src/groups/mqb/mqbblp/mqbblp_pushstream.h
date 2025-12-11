// Copyright 2024 Bloomberg Finance L.P.
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

// mqbblp_pushstream.h                                                -*-C++-*-
#ifndef INCLUDED_MQBBLP_PUSHSTREAM
#define INCLUDED_MQBBLP_PUSHSTREAM

/// @file mqbblp_pushstream.h
///
/// @brief Provide a Storage for one-time PUSH delivery
///
/// An additional Storage layer for one-time PUSH delivery at Replica/Proxy.
///
/// When PUSH message is a result of round-robin, the number of App ids in the
/// message may not be equal to the number of Apps known to the
/// `RelayQueueEngine`.  Moreover, the order of data replication in Replica may
/// not be equal to the order of PUSH messages.  The main storage `DataStream`
/// cannot feed the deliver logic, we need an additional layer.
///
/// This layer supports the @bbref{mqbi::StorageIterator} interface because
/// this is how the delivery logic accesses data in all cases including Primary
/// where the main `DataStream` storage is used.  And including the future
/// Reliable Broadcast mode.
///
/// An efficient iteration requires records of variable size per each GUID.  On
/// the other side, there is only sequential access - either for each GUID or
/// for each App.  An `Element` holding PUSH context for a GUID and an App is
/// in two lists - for the GUID for the App.  Removal can be random for the
/// GUID list and always sequential for the App list.
///
/// The storage supports @bbref{mqbi::StorageIterator} interface but the
/// meaning of `appOrdinal` in the `appMessageView` is different; the access is
/// always sequential with monotonically increasing `appOrdinal` and the
/// `appOrdinal` can be different for the same App depending on the GUID.
///
/// Upon GUIDs iteration followed by he GUID list iteration, if the App
/// succeeds in delivering the PUSH, the engine removes the `Element` from both
/// lists.  If the App is at capacity, the `Element` stays, the iterations
/// continue.  Upon `onHandleUsable`, the App need to catch up by iterating the
/// App list.

// MQB
#include <mqbi_storage.h>

// BMQ
#include <bmqc_orderedhashmap.h>
#include <bmqt_messageguid.h>

// BDE
#include <ball_log.h>
#include <bdlmt_throttle.h>
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bsl_unordered_map.h>
#include <bsl_utility.h>
#include <bslma_allocator.h>

namespace BloombergLP {

namespace mqbblp {

// FORWARD DECLARATION
struct RelayQueueEngine_AppState;

/// The ordered sequence of GUIDs for one-time delivery.
class PushStream {
  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(PushStream, bslma::UsesBslmaAllocator)

    // forward declaration
    struct Element;

    enum ElementList {
        e_GUID = 0  // column
        ,
        e_APP = 1  // row
        ,
        e_TOTAL = 2
    };

    struct ElementBase {
        Element* d_next_p;
        Element* d_previous_p;

        ElementBase();
    };

    /// A list of Elements to associate Elements with 1) GUID, 2) App.  In the
    /// case of GUID, the list is doubly-linked for random removal In the case
    /// of App, the list is singly-linked; the removal is always sequential.
    struct Elements {
      private:
        Element*     d_first_p;
        Element*     d_last_p;
        unsigned int d_numElements;

      private:
        void onRemove();
        void onAdd(Element* element);

      public:
        Elements();

        /// Add the specified `element` to doubly-linked list for GUID
        void add(Element* element, ElementList where);

        /// Remove the specified `element` from doubly-linked list for GUID
        void remove(Element* element, ElementList where);

        /// Return the first Element in the list
        Element* front() const;

        /// Return the last Element in the list
        Element* back() const;

        /// Return number of Elements in the list
        unsigned int numElements() const;
    };

    struct Message {
        Elements           d_appMessages;
        bsls::Types::Int64 d_sequenceNumber;

        explicit Message();

        /// Return number of Elements in the list
        unsigned int numElements() const;

        bsls::Types::Uint64 sequenceNumber() const;

        static bsls::Types::Int64 nextSequenceNumber();
    };

    struct App {
        Elements                                   d_elements;
        bsl::shared_ptr<RelayQueueEngine_AppState> d_app;
        /// Replica deduplicates PUSH for the same App in the same iteration
        /// (genCount).
        bsl::pair<bmqt::MessageGUID, int> d_lastGUID;

        App(const bsl::shared_ptr<RelayQueueEngine_AppState>& app);
        void add(Element* element);
        void remove(Element* element);

        /// Return 'true' if the specified `guid` and `genCount` are the same
        /// as in the last `setLastPush` call.
        bool isLastPush(const bmqt::MessageGUID& guid, int genCount);

        /// Cache the specified `guid` and the specified `genCount` for
        /// subsequent checks by `isLastPush`.
        void setLastPush(const bmqt::MessageGUID& guid, int genCount);

        const Element* last() const;
    };

    typedef bmqc::OrderedHashMap<bmqt::MessageGUID,
                                 Message,
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
        Element(const bmqp::RdaInfo&  rda,
                unsigned int          subscriptionId,
                const iterator&       iterator,
                const Apps::iterator& iteratorApp);

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

        /// Return pointer to the next Element associated with the same GUID
        /// or `0` if this is the last Element.
        Element* next() const;

        /// Return pointer to the next Element associated with the same App
        /// or `0` if this is the last Element.
        Element* nextInApp() const;

        // Return the datastream iterator referencing the GUID of this element.
        const iterator& iteratorGuid() const;
    };

    // PUBLIC DATA
    Stream d_stream;

    Apps d_apps;

    bsl::shared_ptr<bdlma::ConcurrentPool> d_pushElementsPool_sp;

    // CREATORS
    /// @brief Construct this object.
    /// @param pushElementsPool_sp The shared push element pool used to supply
    ///        objects to this PushStream.  If the provided pointer is null,
    ///        create its own object pool.
    /// @param allocator The allocator to use.
    explicit PushStream(
        const bsl::shared_ptr<bdlma::ConcurrentPool>& pushElementsPool_sp,
        bslma::Allocator*                             allocator);

    /// Introduce the specified `guid` to the Push Stream if it is not present.
    /// Return `false` if it is present and not the last one.  Otherwise, load
    /// the iterator pointing to the `guid` into the specified `itGuid`.
    bool findOrAddLast(iterator* itGuid, const bmqt::MessageGUID& guid);

    /// Remove the specified `element` from both GUID and App corresponding to
    /// the `element` (and specified when constructing the `element`).  If
    /// there are no more elements in the App, erase the App.  If the specified
    /// `canEraseGuid` is `true` and there are no more elements in the GUID,
    /// erase the GUID.
    void remove(Element* element, bool canEraseGuid);

    /// Remove all PushStream Elements corresponding to the specified
    /// `upstreamSubQueueId`.  Erase each corresponding GUIDs from the
    /// PushStream with no remaining Elements. Erase the corresponding App.
    /// Return the number of removed elements.
    unsigned int removeApp(unsigned int upstreamSubQueueId);

    /// Remove all PushStream Elements corresponding to the specified
    /// `itApp`.    Erase each corresponding GUIDs from the PushStream with no
    /// remaining Elements. Erase the corresponding App.
    /// Return the number of removed elements.
    unsigned int removeApp(Apps::iterator itApp);

    /// Remove all Elements, Apps, and GUIDs.
    unsigned int removeAll();

    /// Add new Element associated with the specified `rda`, `subscriptionId`,
    /// `iterator` pointing to the corresponding GUID, and `iteratorApp`
    /// pointing to the corresponding App.
    Element* add(const bmqp::RdaInfo&  rda,
                 unsigned int          subscriptionId,
                 const iterator&       iterator,
                 const Apps::iterator& iteratorApp);
};

// ========================
// class PushStreamIterator
// ========================

/// A mechanism to iterate the `PushStream`; see above.  To be used by the
/// `QueueEngine` routing in the same way as another
/// @bbref{mqbi::StorageIterator} implementation(s).
class PushStreamIterator : public mqbi::StorageIterator {
  private:
    // DATA
    mqbi::Storage* d_storage_p;

    mutable mqbi::StorageMessageAttributes d_attributes;

    /// If this variable is empty, it is assumed that attributes, message, and
    /// options have not been loaded in this iteration (see also
    /// `loadMessageAndAttributes` impl).
    mutable bsl::shared_ptr<bdlbb::Blob> d_appData_sp;

    mutable bsl::shared_ptr<bdlbb::Blob> d_options_sp;

  protected:
    PushStream* d_owner_p;

    /// Current (`mqbi::AppMessage`, `upstreamSubQueueId`) pair.
    mutable PushStream::Element* d_currentElement;

    /// Current ordinal corresponding to the `d_currentElement`.
    mutable unsigned int d_currentOrdinal;

    PushStream::iterator d_iterator;

  private:
    // NOT IMPLEMENTED
    PushStreamIterator(const StorageIterator&);                // = delete
    PushStreamIterator& operator=(const PushStreamIterator&);  // = delete

  protected:
    // PRIVATE ACCESSORS

    /// Load the internal state of this iterator instance with the
    /// attributes and blob pointed to by the MessageGUID to which this
    /// iterator is currently pointing.  Behavior is undefined if `atEnd()`
    /// returns true or if underlying storage does not contain the
    /// MessageGUID being pointed to by this iterator.  Return `false` if
    /// data are already loaded; return `true` otherwise.
    bool loadMessageAndAttributes() const;

    const PushStream::Message& message() const;

  public:
    // CREATORS

    /// Create a new VirtualStorageIterator from the specified `storage` and
    /// pointing at the specified `initialPosition`.
    PushStreamIterator(mqbi::Storage*              storage,
                       PushStream*                 owner,
                       const PushStream::iterator& initialPosition);

    /// Destructor
    virtual ~PushStreamIterator() BSLS_KEYWORD_OVERRIDE;

    /// Remove the current element (`mqbi::AppMessage`, `upstreamSubQueueId`
    /// pair) from the current PUSH GUID.
    /// The behavior is undefined unless `atEnd` returns `false`.
    void removeCurrentElement();

    /// Remove all elements (`mqbi::AppMessage`, `upstreamSubQueueId` pairs)
    /// from the current PUSH GUID.
    /// The behavior is undefined unless `atEnd` returns `false`.
    void removeAllElements();

    /// Return the number of elements (`mqbi::AppMessage`, `upstreamSubQueueId`
    /// pairs) for the current PUSH GUID.
    /// The behavior is undefined unless `atEnd` returns `false`.
    unsigned int numApps() const;

    bsls::Types::Uint64 sequenceNumber() const;

    /// Return the current element (`mqbi::AppMessage`, `upstreamSubQueueId`
    /// pair).
    /// The behavior is undefined unless `atEnd` returns `false`.
    virtual PushStream::Element* element(unsigned int appOrdinal) const;

    // MANIPULATORS
    /// Clear any cached data associated with this iterator, if any.
    /// The cache might be initialized within `appData`, `options` or
    /// `attributes` routines.
    /// TODO: refactor iterators to remove cached data.
    void clearCache() BSLS_KEYWORD_OVERRIDE;

    bool advance() BSLS_KEYWORD_OVERRIDE;

    /// If the specified `atEnd` is `true`, reset the iterator to point to the
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

    bool asFarAs(const bdlb::Variant<bsls::Types::Uint64, bmqt::MessageGUID>&
                     stop) const BSLS_KEYWORD_OVERRIDE;
};

// ============================
// class VirtualStorageIterator
// ============================

/// A mechanism to iterate `Element`s related to one App only.
class VirtualPushStreamIterator : public PushStreamIterator {
  private:
    // NOT IMPLEMENTED
    VirtualPushStreamIterator(const VirtualPushStreamIterator&);  // = delete
    VirtualPushStreamIterator&
    operator=(const VirtualPushStreamIterator&);  // = delete

  public:
    // CREATORS

    /// Create a new VirtualStorageIterator from the specified `storage` and
    /// pointing at the specified `initialPosition`.
    VirtualPushStreamIterator(unsigned int   upstreamSubQueueId,
                              mqbi::Storage* storage,
                              PushStream*    owner);

    /// Destructor
    virtual ~VirtualPushStreamIterator() BSLS_KEYWORD_OVERRIDE;

    /// Remove the current element (`mqbi::AppMessage`, `upstreamSubQueueId`
    /// pair) from the current PUSH GUID.
    /// The behavior is undefined unless `atEnd` returns `false`.
    void removeCurrentElement();

    /// Return the number of elements (`mqbi::AppMessage`, `upstreamSubQueueId`
    /// pairs) for the current PUSH GUID.
    /// The behavior is undefined unless `atEnd` returns `false`.
    unsigned int numApps() const;

    /// Return the current element (`mqbi::AppMessage`, `upstreamSubQueueId`
    /// pair).
    /// The behavior is undefined unless `atEnd` returns `false`.
    PushStream::Element*
    element(unsigned int appOrdinal) const BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    bool advance() BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if this iterator is currently at the end of the items'
    /// collection, and hence doesn't reference a valid item.
    bool atEnd() const BSLS_KEYWORD_OVERRIDE;
};

// FREE OPERATORS
bool operator==(const VirtualPushStreamIterator& lhs,
                const VirtualPushStreamIterator& rhs);

// --------------------------
// struct PushStream::Element
// --------------------------
inline PushStream::ElementBase::ElementBase()
: d_next_p(0)
, d_previous_p(0)
{
    // NOTHING
}

inline PushStream::Element::Element(const bmqp::RdaInfo&  rda,
                                    unsigned int          subscriptionId,
                                    const iterator&       iterator,
                                    const Apps::iterator& iteratorApp)
: d_app(rda)
, d_iteratorGuid(iterator)
, d_iteratorApp(iteratorApp)
{
    d_app.d_subscriptionId = subscriptionId;
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
    return d_iteratorGuid->second.d_appMessages;
}

inline PushStream::App& PushStream::Element::app() const
{
    return d_iteratorApp->second;
}

inline PushStream::Element* PushStream::Element::next() const
{
    return d_base[e_GUID].d_next_p;
}

inline PushStream::Element* PushStream::Element::nextInApp() const
{
    return d_base[e_APP].d_next_p;
}

inline const PushStream::iterator& PushStream::Element::iteratorGuid() const
{
    return d_iteratorGuid;
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

inline void PushStream::Elements::remove(Element* element, ElementList where)
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

inline void PushStream::Elements::add(Element* element, ElementList where)
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

inline PushStream::Element* PushStream::Elements::front() const
{
    return d_first_p;
}

inline PushStream::Element* PushStream::Elements::back() const
{
    return d_last_p;
}

inline unsigned int PushStream::Elements::numElements() const
{
    return d_numElements;
}

inline PushStream::Message::Message()
: d_appMessages()
, d_sequenceNumber(nextSequenceNumber())
{
}

/// Return number of Elements in the list
inline unsigned int PushStream::Message::numElements() const
{
    return d_appMessages.numElements();
}

inline bsls::Types::Uint64 PushStream::Message::sequenceNumber() const
{
    return d_sequenceNumber;
}

inline bsls::Types::Int64 PushStream::Message::nextSequenceNumber()
{
    static bsls::Types::Int64 s_value = 0;

    return ++s_value;
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

inline bool PushStream::App::isLastPush(const bmqt::MessageGUID& lastGUID,
                                        int                      genCount)
{
    return d_lastGUID.second == genCount && d_lastGUID.first == lastGUID;
}

inline void PushStream::App::setLastPush(const bmqt::MessageGUID& lastGUID,
                                         int                      genCount)
{
    d_lastGUID = bsl::make_pair(lastGUID, genCount);
}

inline const PushStream::Element* PushStream::App::last() const
{
    return d_elements.back();
}

// ------------------
// struct PushStream
// -----------------

inline PushStream::Element* PushStream::add(const bmqp::RdaInfo& rda,
                                            unsigned int    subscriptionId,
                                            const iterator& it,
                                            const Apps::iterator& iteratorApp)
{
    BSLS_ASSERT_SAFE(it != d_stream.end());

    Element* element = new (d_pushElementsPool_sp->allocate())
        Element(rda, subscriptionId, it, iteratorApp);

    // Add to the GUID
    BSLS_ASSERT_SAFE(element);
    BSLS_ASSERT_SAFE(element->iteratorGuid() != d_stream.end());

    element->guid().add(element, e_GUID);

    // Add to the App
    element->app().add(element);

    return element;
}

inline bool PushStream::findOrAddLast(iterator*                itGuid,
                                      const bmqt::MessageGUID& guid)
{
    BSLS_ASSERT_SAFE(itGuid);

    iterator existing = d_stream.find(guid);

    if (existing == d_stream.end()) {
        *itGuid = d_stream.insert(bsl::make_pair(guid, Message())).first;
        return true;  // RETURN
    }
    if (existing == --d_stream.end()) {
        *itGuid = existing;
        return true;  // RETURN
    }

    // Anything other than the last is out of order
    return false;
}

inline void PushStream::remove(Element* element, bool canEraseGuid)
{
    BSLS_ASSERT_SAFE(element);
    BSLS_ASSERT_SAFE(element->iteratorGuid() != d_stream.end());

    // remove from the App
    element->app().remove(element);

    // remove from the guid
    element->guid().remove(element, e_GUID);

    if (element->app().d_elements.numElements() == 0) {
        element->eraseApp(d_apps);
    }

    if (canEraseGuid && element->guid().numElements() == 0) {
        element->eraseGuid(d_stream);
    }

    d_pushElementsPool_sp->deallocate(element);
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
        Element* element = itApp->second.d_elements.front();

        remove(element, true);
        // do not keep Guid.  This relies on either 'beforeOneAppRemoved' or
        // resetting iterator(s).
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

}  // close package namespace

}  // close enterprise namespace

#endif
