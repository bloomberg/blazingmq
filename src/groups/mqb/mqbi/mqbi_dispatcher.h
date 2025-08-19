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

// mqbi_dispatcher.h                                                  -*-C++-*-
#ifndef INCLUDED_MQBI_DISPATCHER
#define INCLUDED_MQBI_DISPATCHER

//@PURPOSE: Provide a protocol for dispatching events.
//
//@CLASSES:
//  mqbi::Dispatcher:           Protocol for dispatching events
//  mqbi::DispatcherClient:     Protocol for a client of the Dispatcher
//  mqbi::DispatcherClientData: VST for dispatcher client data
//  mqbi::DispatcherClientType: Enum for identifying the type of a client
//  mqbi::DispatcherEvent:      Context for an event dispatched
//  mqbi::DispatcherEventType:  Enum for the type of a dispatcher event
//
//@DESCRIPTION: 'mqbi::Dispatcher' is a protocol to dispatch events of type
// 'mqbi::DispatcherEventType' in the 'mqbi::DispatcherEvent' struct to clients
// implementing the 'mqbi::DispatcherClient' interface, which can have multiple
// type ('mqbi::DispatcherClientType').  The 'mqbi::DispatcherClientData'
// struct represents a state associated to a 'DispatcherClient' and used by the
// 'Dispatcher'.
//
/// Thread Safety
///-------------
//  mqbi::Dispatcher is thread safe
//
//
/// TODO: Design
///------------
//: o 'DispatcherClientData' is a concrete relationship between two interfaces
//:   ('Dispatcher' and 'DispatcherClient') and therefore is an implementation
//:   detail of a concrete 'Dispatcher' implementation.  It should be moved to
//:   the implementation (mqba) and become an imp detail of the dispatcher that
//:   no one should have knowledge about.  For efficiency purpose, the
//:   'Dispatcher' implementation should be able to access the associated
//:   'ClientData' of a client without lookup in a map (implying mutex lock),
//:   and so the 'DispatcherClient' interface should offer a way for the
//:   'Dispatcher' to store and retrieve metadata (pointer to the internal
//:   'DispatcherClientData').  For added type safety, consider using an
//:   interface exposing accessors to the Dispatcher, ProcessorHandle and
//:   DispatcherClientType.
//
//: o See if the duplication of DispatcherClient::dispatcher and
//:   DispatcherClient::DispatcherData::dispatcher() can be avoided.
//
//: o The only two ways to address a processor are either 'DispatcherClient*'
//:   or (DispatcherClientType, ProcessorHandle).  Therefore,
//:   'inDispatcherThread(const DispatcherClientData *data)' should be replaced
//:   to take the pair (DispatcherClientType, ProcessorHandle).
//
//: o Document what 'isRelay' really means.
//
//: o DispatcherEvent::queueId: is only set in queueHandle, so consider
//:   removing this attribute and use only the 'DispatcherEvent::queueHandle'
//:   attribute -- read comment in mqba::ClientSession to see whether this is
//:   safe to do.
//
//: o For the DispatcherEvent of type 'confirm', 'reject', 'push', 'put',
//:   'ack', they operate either on a blob or on the protocol structure,
//:   depending on a 'isRelay' flag.  This sounds dangerous and wrong.  It
//:   looks like the 'blob' is used when there (may be) multiple confirm
//:   messages, while 'confirmMessage' is used when there is only one which
//:   has already been decoded.  Once cleaned up and fixed, revisit the
//:   documentation of the various accessors for the associated
//:   DispatcherEvents view interfaces.  Investigate whether we could simply
//:   always use the blob, or if that would be inefficient and make usage more
//    complex.
//:   ANALYSIS of CONFIRM message (similar for the others):
//:     - creation:
//:         clientSession.: blob
//:         cluster.......: isRelay(false) blobclusterNode
//:         remoteQueue...: isRelay(true) confirmMessage partitionId
//:     - access:
//:         clientSession.: blob
//:         clusterProxy..: isRelay(true) confirmMessage
//:         cluster.......: isRelay(false) blob clusterNode(loggingOnly)
//:                         isRelay(true) confirmMessage partitionId
//
/// Executors support
///-----------------
// Implementations of the 'mqbi::Dispatcher' protocol are required to provide
// two types of executors.  The first being an executor, available through the
// dispatcher's 'executor' member function, to execute functors on a
// dispatcher's processor.  The second being an executor, available through the
// dispatcher's 'clientExecutor' member function, to execute functors, still in
// a dispatcher's processor, but directly by a dispatcher's client.  Submitting
// a functor via each of the executor's 'post' member functions shall be
// functionally equivalent to dispatching an event of type 'e_DISPATCHER' and
// 'e_CALLBACK' respectively.  The comparison of such executor objects and the
// blocking behavior of their 'dispatch' member functions is implementation-
// defined.  For more information about executors see the 'bmqex' package
// documentation.

// MQB

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqt_compressionalgorithmtype.h>
#include <bmqt_messageguid.h>

#include <bmqex_executor.h>

#include <bmqu_atomicstate.h>
#include <bmqu_managedcallback.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_variant.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_nullptr.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbnet {
class ClusterNode;
}

namespace mqbi {

// FORWARD DECLARATION
class Dispatcher;
class DispatcherClient;
class DispatcherClientData;
class DispatcherEvent;
class QueueHandle;

// ===========================
// struct DispatcherClientType
// ===========================

/// Enumeration for the different types of dispatcher clients.
struct DispatcherClientType {
    // TYPES
    enum Enum {
        /// type has not been specified
        e_UNDEFINED = -1,

        /// client is assimilated to a session
        e_SESSION = 0,

        /// client is assimilated to a queue
        e_QUEUE = 1,

        /// client is assimilated to a cluster
        e_CLUSTER = 2,

        /// represents all of the possible types (see below)
        e_ALL = 3
    };
    // NOTE: the 'e_ALL' type is used by certain Dispatcher methods to indicate
    //       they should be applied to all types of clients.

    // CONSTANTS
    static const int k_COUNT = 3;  // Total number of different ClientTypes.

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// `DispatcherClientType::Enum` value.
    static bsl::ostream& print(bsl::ostream&              stream,
                               DispatcherClientType::Enum value,
                               int                        level          = 0,
                               int                        spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `BMQT_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(DispatcherClientType::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(DispatcherClientType::Enum* out,
                          const bslstl::StringRef&    str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&              stream,
                         DispatcherClientType::Enum value);

// ==========================
// struct DispatcherEventType
// ==========================

/// Enumeration for the different types of dispatcher events.
struct DispatcherEventType {
    // TYPES
    enum Enum {
        /// invalid event
        e_UNDEFINED = 0,

        e_DISPATCHER          = 1,
        e_CALLBACK            = 2,
        e_CONTROL_MSG         = 3,
        e_CONFIRM             = 4,
        e_REJECT              = 5,
        e_PUSH                = 6,
        e_PUT                 = 7,
        e_ACK                 = 8,
        e_CLUSTER_STATE       = 9,
        e_STORAGE             = 10,
        e_RECOVERY            = 11,
        e_REPLICATION_RECEIPT = 12
    };
    // NOTE: Events of type 'e_DISPATCHER' are similar to those of type
    //       'e_CALLBACK' in the sense that they both represent a callback to
    //       be invoked on the thread associated to the target destination
    //       dispatcher client.  However, the major difference resides in when
    //       that callback is invoked: unlike all other types, 'e_DISPATCHER'
    //       events are handled internally by the dispatcher itself, and not
    //       sent to the 'onDispatcherEvent()' method of the targeted
    //       destination dispatcher client.  This will also not trigger the
    //       destination to be added to the flush list.
    //
    //       The purpose is that this event can be used for operations such as
    //       'finalize' (i.e., destroy) of a client where calling any method on
    //       the destination object might be undefined due to the object no
    //       longer being alive.
    //
    //       Unless needed, always prefer to use the 'e_CALLBACK' type to give
    //       more control over to the target destination dispatcher client: the
    //       client will be able to do some pre and post callback invocation
    //       duty (such as flushing some state).

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// `DispatcherClientType::Enum` value.
    static bsl::ostream& print(bsl::ostream&             stream,
                               DispatcherEventType::Enum value,
                               int                       level          = 0,
                               int                       spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `BMQT_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(DispatcherEventType::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(DispatcherEventType::Enum* out,
                          const bslstl::StringRef&   str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&             stream,
                         DispatcherEventType::Enum value);

// ================
// class Dispatcher
// ================

/// Pure interface for a dispatcher mechanism to dispense `DispatcherEvent`
/// objects of type `DispatcherEventType` to clients of type
/// `DispatcherClient`.
class Dispatcher {
  public:
    // TYPES

    /// Type representing a handle to a processor in the dispatcher.
    typedef int ProcessorHandle;

    /// Signature of a `void` functor method.
    typedef bmqu::ManagedCallback::VoidFunctor VoidFunctor;

    // PUBLIC CLASS DATA

    /// Value of an invalid processor handle.
    static const ProcessorHandle k_INVALID_PROCESSOR_HANDLE = -1;

  public:
    // CREATORS

    /// Destructor of this object.
    virtual ~Dispatcher();

    // MANIPULATORS

    /// Associate the specified `client` to one of the dispatcher's
    /// processors in charge of clients of the specified `type`.  Use the
    /// processor provided in the optionally specified `handle` if it is
    /// valid, or let the dispatcher automatically affect a processor to the
    /// `client` (by using some internal load-balancing mechanism for
    /// example) if `handle` represents the `k_INVALID_PROCESSOR_HANDLE`.
    /// This operation is a no-op if the `client` is already associated with
    /// a processor *and* `handle` is invalid.  If `handle` if valid, the
    /// behavior is undefined unless `client` is not yet associated with any
    /// processor.  As of result of this operation, the
    /// `dispatcherClientData` of `client` will be populated.  Return the
    /// processor handle associated to `client`.
    ///
    /// NOTE: specifying a valid `handle` is useful when BlazingMQ broker
    ///       requires a client to be associated to the same processor
    ///       across brokers' instantiations.
    virtual ProcessorHandle
    registerClient(DispatcherClient*          client,
                   DispatcherClientType::Enum type,
                   ProcessorHandle handle = k_INVALID_PROCESSOR_HANDLE) = 0;

    /// Remove the association of the specified `client` from its processor,
    /// and mark as invalid the `processorHandle` from the client's
    /// `dispatcherClientData` member.  This operation is a no-op if the
    /// `client` is not associated with any processor.
    virtual void unregisterClient(DispatcherClient* client) = 0;

    /// Retrieve an event from the event pool to send to the specified
    /// `client`.  Once populated, the returned event *must* be enqueued for
    /// processing by calling `dispatchEvent` otherwise it will be leaked.
    virtual DispatcherEvent* getEvent(const DispatcherClient* client) = 0;

    /// Retrieve an event from the event pool to send to a client of the
    /// specified `type`.  Once populated, the returned event *must* be
    /// enqueued for processing by calling `dispatchEvent` otherwise it will
    /// be leaked.
    virtual DispatcherEvent* getEvent(DispatcherClientType::Enum type) = 0;

    /// Dispatch the specified `event` to the specified `destination`.  The
    /// behavior is undefined unless `event` was obtained by a call to
    /// `getEvent` with a type matching the one of `destination`.
    virtual void dispatchEvent(DispatcherEvent*  event,
                               DispatcherClient* destination) = 0;

    /// Dispatch the specified `event` to the processor in charge of clients
    /// of the specified `type` and associated with the specified `handle`.
    /// The behavior is undefined unless `event` was obtained by a call to
    /// `getEvent` with a matching `type`..
    virtual void dispatchEvent(DispatcherEvent*           event,
                               DispatcherClientType::Enum type,
                               ProcessorHandle            handle) = 0;

    /// Execute the specified `functor`, using the optionally specified
    /// dispatcher `type`, in the processor associated to the specified
    /// `client`.  The behavior is undefined unless `type` is `e_DISPATCHER`
    /// or `e_CALLBACK`.
    virtual void execute(
        const VoidFunctor&        functor,
        DispatcherClient*         client,
        DispatcherEventType::Enum type = DispatcherEventType::e_CALLBACK) = 0;

    /// Execute the specified `functor`, using the `e_DISPATCHER` event
    /// type, in the processor associated to the specified `client`.
    ///
    /// TBD: `DispatcherClientData` is considered an internal imp-detail
    ///      type used by Dispatcher, and this `execute` overload should
    ///      ideally be removed.
    virtual void execute(const VoidFunctor&          functor,
                         const DispatcherClientData& client) = 0;

    /// Execute the specified `functor` in the processors in charge of
    /// clients of the specified `type`, and invoke the specified
    /// `doneCallback` (if any) when all the relevant processors are done
    /// executing the `functor`.
    virtual void execute(const VoidFunctor&         functor,
                         DispatcherClientType::Enum type,
                         const VoidFunctor& doneCallback = VoidFunctor()) = 0;

    /// Enqueue an event to the processor associated to the specified
    /// `client` or pair of the specified `type` and `handle` and block
    /// until this event gets dequeued.  This is typically used by a
    /// `dispatcherClient`, in its destructor, to drain the dispatcher's
    /// queue and ensure no more events are to be expected for that
    /// `client`.  The behavior is undefined if `synchronize` is being
    /// invoked from the `client`s thread.
    virtual void synchronize(DispatcherClient* client) = 0;
    virtual void synchronize(DispatcherClientType::Enum type,
                             ProcessorHandle            handle)   = 0;

    // ACCESSORS

    /// Return the number of processors dedicated for dispatching clients of
    /// the specified `type`.
    virtual int numProcessors(DispatcherClientType::Enum type) const = 0;

    /// Return whether the current thread is the dispatcher thread
    /// associated to the specified `client`.  This is useful for
    /// preconditions assert validation.
    virtual bool inDispatcherThread(const DispatcherClient* client) const = 0;

    /// Return whether the current thread is the dispatcher thread
    /// associated to the specified dispatcher client `data`.  This is
    /// useful for preconditions assert validation.
    virtual bool
    inDispatcherThread(const DispatcherClientData* data) const = 0;

    /// Return an executor object suitable for executing function objects on
    /// the processor in charge of the specified `client`.  The behavior is
    /// undefined unless the specified `client` is registered on this
    /// dispatcher and the client type is not `e_UNDEFINED` or `e_ALL`.
    ///
    /// Note that the returned executor can be used to submit work even
    /// after the specified `client` has been unregistered from this
    /// dispatcher.
    virtual bmqex::Executor executor(const DispatcherClient* client) const = 0;

    /// Return an executor object suitable for executing function objects by
    /// the specified `client` on the processor in charge of that client.
    /// The behavior is undefined unless the specified `client` is
    /// registered on this dispatcher and the client type is not
    /// `e_UNDEFINED` or `e_ALL`.
    ///
    /// Note that submitting work on the returned executor is undefined
    /// behavior if the specified `client` was unregistered from this
    /// dispatcher.
    virtual bmqex::Executor
    clientExecutor(const mqbi::DispatcherClient* client) const = 0;
};

// ===============================
// class DispatcherDispatcherEvent
// ===============================

/// Event of type `e_DISPATCHER`.
class DispatcherDispatcherEvent {
  private:
    // DATA

    /// Callback embedded in this event.
    bmqu::ManagedCallback d_callback;

    /// Callback embedded in this event.
    /// This callback is called when the
    /// 'Dispatcher::execute' method is
    /// used to enqueue an event to
    /// multiple processors, and will be
    /// called when the last processor
    /// finished processing it.
    bmqu::ManagedCallback d_finalizeCallback;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DispatcherDispatcherEvent,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Construct using the specified `allocator`.
    explicit DispatcherDispatcherEvent(bslma::Allocator* allocator = 0);

    DispatcherDispatcherEvent(
        bslmf::MovableRef<DispatcherDispatcherEvent> other);

    // ACCESSORS

    /// Return a reference not offering modifiable access to the callback
    /// associated to this event.
    const bmqu::ManagedCallback& callback() const;

    /// Return a reference not offering modifiable access to the finalize
    /// callback, if any, associated to this event.
    const bmqu::ManagedCallback& finalizeCallback() const;

    // MANIPULATORS

    /// Set the corresponding field to the specified `value` and return a
    /// reference to the object currently being built.
    bmqu::ManagedCallback& callback();

    bmqu::ManagedCallback& finalizeCallback();
};

// =============================
// class DispatcherCallbackEvent
// =============================

/// Event of type `e_CALLBACK`.
class DispatcherCallbackEvent {
  private:
    // DATA

    /// Callback embedded in this event.
    bmqu::ManagedCallback d_callback;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DispatcherCallbackEvent,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Construct using the specified `allocator`.
    explicit DispatcherCallbackEvent(bslma::Allocator* allocator = 0);

    DispatcherCallbackEvent(bslmf::MovableRef<DispatcherCallbackEvent> other);

    // ACCESSORS

    /// Return a reference not offering modifiable access to the callback
    /// associated to this event.
    const bmqu::ManagedCallback& callback() const;

    // MANIPULATORS

    /// Set the corresponding field to the specified `value` and return a
    /// reference to the object currently being built.
    bmqu::ManagedCallback& callback();
};

// ===================================
// class DispatcherControlMessageEvent
// ===================================

/// Event of type `e_CONTROL_MSG`.
class DispatcherControlMessageEvent {
  private:
    // DATA

    /// ControlMessage in this event.
    bmqp_ctrlmsg::ControlMessage d_controlMessage;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DispatcherControlMessageEvent,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Construct using the optionally specified `allocator`
    explicit DispatcherControlMessageEvent(bslma::Allocator* allocator = 0);

    DispatcherControlMessageEvent(
        bslmf::MovableRef<DispatcherControlMessageEvent> other);

    // ACCESSORS

    /// Return a reference not offering modifiable access to the control
    /// message associated to this event.
    const bmqp_ctrlmsg::ControlMessage& controlMessage() const;

    // MANIPULATORS

    /// Set the corresponding field to the specified `value` and return a
    /// reference to the object currently being built.
    DispatcherControlMessageEvent&
    setControlMessage(const bmqp_ctrlmsg::ControlMessage& value);
};

// ============================
// class DispatcherConfirmEvent
// ============================

/// Event of type `e_CONFIRM`.
class DispatcherConfirmEvent {
  private:
    // DATA

    /// Blob of data embedded in this event.
    bsl::shared_ptr<bdlbb::Blob> d_blob_sp;

    /// 'ClusterNode' associated to this event.
    mqbnet::ClusterNode* d_clusterNode_p;

    /// ConfirmMessage in this event.
    bmqp::ConfirmMessage d_confirmMessage;

    /// PartitionId of the message in this event.
    int d_partitionId;

    /// Flag indicating if this is a relay event.
    bool d_isRelay;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DispatcherConfirmEvent,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor.
    explicit DispatcherConfirmEvent(bslma::Allocator* allocator = 0);

    DispatcherConfirmEvent(bslmf::MovableRef<DispatcherConfirmEvent> other);

    // ACCESSORS

    /// Return a reference not offering modifiable access to the blob
    /// associated to this event.  The blob represents the raw content of
    /// the `bmqp::Event` this putEvent originates from.  The `blob` is
    /// only valid when `isRelay() == false`.  Typically, `blob` is used
    /// when there may be multiple push messages; while `guid` and `queueId`
    /// are used when only one is present.
    const bsl::shared_ptr<bdlbb::Blob>& blob() const;

    /// Return a pointer to the cluster node this event originate from, or
    /// null if it doesn't come from a cluster node.  This is mainly useful
    /// for logging purposes.
    mqbnet::ClusterNode* clusterNode() const;

    /// Return a reference not offering modifiable access to the confirm
    /// message associated to this event.  This protocol struct is only
    /// valid when `isRelay() == true`.
    const bmqp::ConfirmMessage& confirmMessage() const;

    /// Return the partitionId affected to the queue associated to this
    /// put message.  This is only valid when `isRelay() == true`.
    int partitionId() const;

    /// Return whether this event is a relay event or not.
    bool isRelay() const;

    // MANIPULATORS
    DispatcherConfirmEvent& setBlob(const bsl::shared_ptr<bdlbb::Blob>& value);

    DispatcherConfirmEvent& setClusterNode(mqbnet::ClusterNode* value);

    DispatcherConfirmEvent&
    setConfirmMessage(const bmqp::ConfirmMessage& value);

    DispatcherConfirmEvent& setPartitionId(int value);

    DispatcherConfirmEvent& setIsRelay(bool value);
};

// ===========================
// class DispatcherRejectEvent
// ===========================

/// DispatcherEvent interface view of an event of type `e_REJECT`.
class DispatcherRejectEvent {
  private:
    // DATA
    /// Blob of data embedded in this event.
    bsl::shared_ptr<bdlbb::Blob> d_blob_sp;

    /// 'ClusterNode' associated to this event.
    mqbnet::ClusterNode* d_clusterNode_p;

    /// RejectMessage in this event.
    bmqp::RejectMessage d_rejectMessage;

    /// PartitionId of the message in this event.
    int d_partitionId;

    /// Flag indicating if this is a relay event.
    bool d_isRelay;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DispatcherRejectEvent,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor.
    explicit DispatcherRejectEvent(bslma::Allocator* allocator = 0);

    DispatcherRejectEvent(bslmf::MovableRef<DispatcherRejectEvent> other);

    // ACCESSORS

    /// Return a reference not offering modifiable access to the blob
    /// associated to this event.  The blob represents the raw content of
    /// the `bmqp::Event` this putEvent originates from.  The `blob` is
    /// only valid when `isRelay() == false`.  Typically, `blob` is used
    /// when there may be multiple push messages; while `guid` and `queueId`
    /// are used when only one is present.
    const bsl::shared_ptr<bdlbb::Blob>& blob() const;

    /// Return a pointer to the cluster node this event originate from, or
    /// null if it doesn't come from a cluster node.  This is mainly useful
    /// for logging purposes.
    mqbnet::ClusterNode* clusterNode() const;

    /// Return a reference not offering modifiable access to the reject
    /// message associated to this event.  This protocol struct is only
    /// valid when `isRelay() == true`.
    const bmqp::RejectMessage& rejectMessage() const;

    /// Return the partitionId affected to the queue associated to this
    /// put message.  This is only valid when `isRelay() == true`.
    int partitionId() const;

    /// Return whether this event is a relay event or not.
    bool isRelay() const;

    // MANIPULATORS

    DispatcherRejectEvent& setBlob(const bsl::shared_ptr<bdlbb::Blob>& value);

    DispatcherRejectEvent& setClusterNode(mqbnet::ClusterNode* value);

    DispatcherRejectEvent& setRejectMessage(const bmqp::RejectMessage& value);

    DispatcherRejectEvent& setPartitionId(int value);

    DispatcherRejectEvent& setIsRelay(bool value);
};

// =========================
// class DispatcherPushEvent
// =========================

/// DispatcherEvent interface view of an event of type `e_PUSH`.
class DispatcherPushEvent {
  private:
    // DATA
    /// Blob of data embedded in this event.
    bsl::shared_ptr<bdlbb::Blob> d_blob_sp;

    /// Blob of options embedded in this event.
    bsl::shared_ptr<bdlbb::Blob> d_options_sp;

    /// 'ClusterNode' associated to this event.
    mqbnet::ClusterNode* d_clusterNode_p;

    /// GUID of the message in this event.
    bmqt::MessageGUID d_guid;

    /// Flags indicating if the associated message has message properties or
    /// not (first) and if so, if the properties are compressed or not (second)
    bmqp::MessagePropertiesInfo d_messagePropertiesInfo;

    /// Id associated to the queue this event is about (in the context of the
    /// Client, i.e., this is the upstream SDK <-> Broker queueId).
    int d_queueId;

    bmqt::CompressionAlgorithmType::Enum d_compressionAlgorithmType;

    bool d_isOutOfOrder;

    /// Flag indicating if this is a relay event.
    bool d_isRelay;

    /// subQueueInfos associated with the message in this event
    bmqp::Protocol::SubQueueInfosArray d_subQueueInfos;

    /// Message Group Id associated with the message in this event
    bmqp::Protocol::MsgGroupId d_msgGroupId;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DispatcherPushEvent,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// CONSTRUCTOR
    explicit DispatcherPushEvent(bslma::Allocator* allocator = 0);

    DispatcherPushEvent(bslmf::MovableRef<DispatcherPushEvent> other);

    // mqba_clientsession.t.cpp
    DispatcherPushEvent(const bsl::shared_ptr<bdlbb::Blob>&  blob_sp,
                        const bmqt::MessageGUID&             msgGUID,
                        const bmqp::MessagePropertiesInfo&   mp,
                        int                                  queueId,
                        bmqt::CompressionAlgorithmType::Enum cat,
                        bslma::Allocator*                    allocator = 0);

    // mqbblp_clusterproxy.cpp
    DispatcherPushEvent(const bsl::shared_ptr<bdlbb::Blob>& blob_sp,
                        bslma::Allocator*                   allocator = 0);

    // mqbblp_queue.cpp
    DispatcherPushEvent(const bsl::shared_ptr<bdlbb::Blob>&  blob_sp,
                        const bsl::shared_ptr<bdlbb::Blob>&  options_sp,
                        const bmqt::MessageGUID&             msgGUID,
                        const bmqp::MessagePropertiesInfo&   mp,
                        bmqt::CompressionAlgorithmType::Enum cat,
                        bool                                 isOutOfOrder,
                        bslma::Allocator*                    allocator = 0);

    // mqbblp_queuehandle.cpp
    DispatcherPushEvent(
        const bsl::shared_ptr<bdlbb::Blob>&       blob_sp,
        const bmqt::MessageGUID&                  msgGUID,
        const bmqp::MessagePropertiesInfo&        mp,
        int                                       queueId,
        bmqt::CompressionAlgorithmType::Enum      cat,
        bool                                      isOutOfOrder,
        const bmqp::Protocol::SubQueueInfosArray& subQueueInfos,
        const bmqp::Protocol::MsgGroupId&         msgGroupId,
        bslma::Allocator*                         allocator = 0);

    // ACCESSORS

    /// Return a reference not offering modifiable access to the blob
    /// associated to this event.  The blob represents the raw content of
    /// the `bmqp::Event` this putEvent originates from.  The `blob` is
    /// only valid when `isRelay() == false`.  Typically, `blob` is used
    /// when there may be multiple push messages; while `guid` and `queueId`
    /// are used when only one is present.
    const bsl::shared_ptr<bdlbb::Blob>& blob() const;

    const bsl::shared_ptr<bdlbb::Blob>& options() const;

    /// Return a pointer to the cluster node this event originate from, or
    /// null if it doesn't come from a cluster node.  This is mainly useful
    /// for logging purposes.
    mqbnet::ClusterNode* clusterNode() const;

    /// Return whether this event is a relay event or not.
    bool isRelay() const;

    /// Return the queueId associated to this event.  This data member is
    /// only valid when `isRelay() == true`.
    int queueId() const;

    /// Return a reference not offering modifiable access to the
    /// subQueueInfos associated with a message in this event.
    const bmqp::Protocol::SubQueueInfosArray& subQueueInfos() const;

    /// Return (true, *) if the associated PUSH message contains message
    /// properties.  Return (true, true) if the properties is de-compressed
    /// even if the `compressionAlgorithmType` is not `e_NONE`.
    const bmqp::MessagePropertiesInfo& messagePropertiesInfo() const;

    /// Return the compression algorithm type using which a message in this
    /// event is compressed.
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType() const;

    /// Return 'true' if the associated PUSH message is Out-of-Order - not the
    /// first delivery attempt or put-aside (no matching subscription).
    bool isOutOfOrderPush() const;

    /// Return a reference not offering modifiable access to the GUID
    /// associated to this event.  This data member is only valid when
    /// `isRelay() == true`.
    const bmqt::MessageGUID& guid() const;

    /// Return a reference not offering modifiable access to the Message
    /// Group Id associated with a message in this event.
    const bmqp::Protocol::MsgGroupId& msgGroupId() const;

    // MANIPULATORS

    DispatcherPushEvent&
    setCompressionAlgorithmType(bmqt::CompressionAlgorithmType::Enum value);

    DispatcherPushEvent& setOutOfOrderPush(bool value);

    DispatcherPushEvent&
    setMsgGroupId(const bmqp::Protocol::MsgGroupId& value);

    DispatcherPushEvent& setQueueId(int value);

    DispatcherPushEvent&
    setSubQueueInfos(const bmqp::Protocol::SubQueueInfosArray& value);

    DispatcherPushEvent&
    setMessagePropertiesInfo(const bmqp::MessagePropertiesInfo& value);

    DispatcherPushEvent& setIsRelay(bool value);

    DispatcherPushEvent& setOptions(const bsl::shared_ptr<bdlbb::Blob>& value);

    DispatcherPushEvent& setGuid(const bmqt::MessageGUID& value);

    DispatcherPushEvent& setBlob(const bsl::shared_ptr<bdlbb::Blob>& value);

    DispatcherPushEvent& setClusterNode(mqbnet::ClusterNode* value);
};

// ========================
// class DispatcherPutEvent
// ========================

/// DispatcherEvent interface view of an event of type `e_PUT`.
class DispatcherPutEvent {
  private:
    // DATA
    /// Blob of data embedded in this event.
    bsl::shared_ptr<bdlbb::Blob> d_blob_sp;

    /// Blob of options embedded in this event.
    bsl::shared_ptr<bdlbb::Blob> d_options_sp;

    /// 'ClusterNode' associated to this event.
    mqbnet::ClusterNode* d_clusterNode_p;

    /// PutHeader in this event.
    bmqp::PutHeader d_putHeader;

    /// Queue Handle if this event.
    QueueHandle* d_queueHandle_p;

    /// PartitionId of the message in this event.
    int d_partitionId;

    /// Flag indicating if this is a relay event.
    bool d_isRelay;

    bsls::Types::Uint64 d_genCount;

    bsl::shared_ptr<bmqu::AtomicState> d_state;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DispatcherPutEvent,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor.
    DispatcherPutEvent(bslma::Allocator* allocator = 0);

    DispatcherPutEvent(bslmf::MovableRef<DispatcherPutEvent> other);

    // ACCESSORS

    /// Return a reference not offering modifiable access to the blob
    /// associated to this event.  The blob represents the raw content of
    /// the `bmqp::Event` this putEvent originates from.  The `blob` is
    /// only valid when `isRelay() == false`.  Typically, `blob` is used
    /// when there may be multiple push messages; while `guid` and `queueId`
    /// are used when only one is present.
    const bsl::shared_ptr<bdlbb::Blob>& blob() const;

    const bsl::shared_ptr<bdlbb::Blob>& options() const;

    /// Return a pointer to the cluster node this event originate from, or
    /// null if it doesn't come from a cluster node.  This is mainly useful
    /// for logging purposes.
    mqbnet::ClusterNode* clusterNode() const;

    /// Return whether this event is a relay event or not.
    bool isRelay() const;

    /// Return the partitionId affected to the queue associated to this
    /// put message.  This is only valid when `isRelay() == true`.
    int partitionId() const;

    /// Return a reference not offering modifiable access to the put header
    /// associated to this event.  This protocol struct is only valid when
    /// `isRelay() == true`.
    const bmqp::PutHeader& putHeader() const;

    /// TBD:
    QueueHandle* queueHandle() const;

    /// PUT messages carry `genCount`; if there is a mismatch between PUT
    /// `genCount` and current upstream 'genCount, then the PUT message gets
    /// dropped to avoid out of order PUTs.
    bsls::Types::Uint64 genCount() const;

    const bsl::shared_ptr<bmqu::AtomicState>& state() const;

    /// MANIPULATORS
    DispatcherPutEvent& setPutHeader(const bmqp::PutHeader& value);

    DispatcherPutEvent& setQueueHandle(QueueHandle* value);

    DispatcherPutEvent& setGenCount(unsigned int genCount);

    DispatcherPutEvent&
    setState(const bsl::shared_ptr<bmqu::AtomicState>& state);

    DispatcherPutEvent& setBlob(const bsl::shared_ptr<bdlbb::Blob>& value);

    DispatcherPutEvent& setIsRelay(bool value);

    DispatcherPutEvent& setOptions(const bsl::shared_ptr<bdlbb::Blob>& value);

    DispatcherPutEvent& setPartitionId(int value);

    DispatcherPutEvent& setClusterNode(mqbnet::ClusterNode* value);
};

// ========================
// class DispatcherAckEvent
// ========================

/// DispatcherEvent interface view of an event of type `e_ACK`.
class DispatcherAckEvent {
  private:
    // DATA
    /// Blob of data embedded in this event.
    bsl::shared_ptr<bdlbb::Blob> d_blob_sp;

    /// Blob of options embedded in this event.
    bsl::shared_ptr<bdlbb::Blob> d_options_sp;

    /// 'ClusterNode' associated to this event.
    mqbnet::ClusterNode* d_clusterNode_p;

    /// AckMessage in this event.
    bmqp::AckMessage d_ackMessage;

    /// Flag indicating if this is a relay event.
    bool d_isRelay;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DispatcherAckEvent,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor.
    explicit DispatcherAckEvent(bslma::Allocator* allocator = 0);

    DispatcherAckEvent(bslmf::MovableRef<DispatcherAckEvent> other);

    // ACCESSORS

    /// Return a reference not offering modifiable access to the ack message
    /// associated to this event.  This protocol struct is only valid when
    /// `isRelay() == true`.
    const bmqp::AckMessage& ackMessage() const;

    const bsl::shared_ptr<bdlbb::Blob>& options() const;

    /// Return whether this event is a relay event or not.
    bool isRelay() const;

    /// Return a reference not offering modifiable access to the blob
    /// associated to this event.  The blob represents the raw content of
    /// the `bmqp::Event` this putEvent originates from.  The `blob` is
    /// only valid when `isRelay() == false`.  Typically, `blob` is used
    /// when there may be multiple push messages; while `guid` and `queueId`
    /// are used when only one is present.
    const bsl::shared_ptr<bdlbb::Blob>& blob() const;

    /// Return a pointer to the cluster node this event originate from, or
    /// null if it doesn't come from a cluster node.  This is mainly useful
    /// for logging purposes.
    mqbnet::ClusterNode* clusterNode() const;

    // MANIPULATORS
    // todo docs
    bmqp::AckMessage& ackMessage();

    DispatcherAckEvent& setBlob(const bsl::shared_ptr<bdlbb::Blob>& value);

    DispatcherAckEvent& setIsRelay(bool value);

    DispatcherAckEvent& setOptions(const bsl::shared_ptr<bdlbb::Blob>& value);

    DispatcherAckEvent& setAckMessage(const bmqp::AckMessage& value);

    DispatcherAckEvent& setClusterNode(mqbnet::ClusterNode* value);
};

// =================================
// class DispatcherClusterStateEvent
// =================================

/// DispatcherEvent interface view of an event of type `e_CLUSTER_STATE`.
class DispatcherClusterStateEvent {
  private:
    // DATA
    /// Blob of data embedded in this event.
    bsl::shared_ptr<bdlbb::Blob> d_blob_sp;

    /// 'ClusterNode' associated to this event.
    mqbnet::ClusterNode* d_clusterNode_p;

    /// Flag indicating if this is a relay event.
    bool d_isRelay;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DispatcherClusterStateEvent,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor.
    explicit DispatcherClusterStateEvent(bslma::Allocator* allocator = 0);

    DispatcherClusterStateEvent(
        bslmf::MovableRef<DispatcherClusterStateEvent> other);

    // ACCESSORS

    /// Return a reference not offering modifiable access to the blob
    /// associated to this event.  The blob represents the raw content of
    /// the `bmqp::Event` this putEvent originates from.  The `blob` is
    /// only valid when `isRelay() == false`.  Typically, `blob` is used
    /// when there may be multiple push messages; while `guid` and `queueId`
    /// are used when only one is present.
    const bsl::shared_ptr<bdlbb::Blob>& blob() const;

    /// Return whether this event is a relay event or not.
    bool isRelay() const;

    /// Return a pointer to the cluster node this event originate from, or
    /// null if it doesn't come from a cluster node.  This is mainly useful
    /// for logging purposes.
    mqbnet::ClusterNode* clusterNode() const;

    // MANIPULATORS
    DispatcherClusterStateEvent&
    setBlob(const bsl::shared_ptr<bdlbb::Blob>& value);

    DispatcherClusterStateEvent& setIsRelay(bool value);

    DispatcherClusterStateEvent& setClusterNode(mqbnet::ClusterNode* value);
};

// ============================
// class DispatcherStorageEvent
// ============================

/// DispatcherEvent interface view of an event of type `e_ACK`.
class DispatcherStorageEvent {
  private:
    // DATA
    /// Blob of data embedded in this event.
    bsl::shared_ptr<bdlbb::Blob> d_blob_sp;

    /// 'ClusterNode' associated to this event.
    mqbnet::ClusterNode* d_clusterNode_p;

    /// Flag indicating if this is a relay event.
    bool d_isRelay;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DispatcherStorageEvent,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor.
    explicit DispatcherStorageEvent(bslma::Allocator* allocator = 0);

    DispatcherStorageEvent(bslmf::MovableRef<DispatcherStorageEvent> other);

    // ACCESSORS

    /// Return whether this event is a relay event or not.
    bool isRelay() const;

    /// Return a reference not offering modifiable access to the blob
    /// associated to this event.  The blob represents the raw content of
    /// the `bmqp::Event` this putEvent originates from.  The `blob` is
    /// only valid when `isRelay() == false`.  Typically, `blob` is used
    /// when there may be multiple push messages; while `guid` and `queueId`
    /// are used when only one is present.
    const bsl::shared_ptr<bdlbb::Blob>& blob() const;

    /// Return a pointer to the cluster node this event originate from, or
    /// null if it doesn't come from a cluster node.  This is mainly useful
    /// for logging purposes.
    mqbnet::ClusterNode* clusterNode() const;

    // MANIPULATORS
    DispatcherStorageEvent& setBlob(const bsl::shared_ptr<bdlbb::Blob>& value);

    DispatcherStorageEvent& setIsRelay(bool value);

    DispatcherStorageEvent& setClusterNode(mqbnet::ClusterNode* value);
};

// =============================
// class DispatcherRecoveryEvent
// =============================

/// DispatcherEvent interface view of an event of type `e_RECOVERY`.
class DispatcherRecoveryEvent {
  private:
    // DATA
    /// Blob of data embedded in this event.
    bsl::shared_ptr<bdlbb::Blob> d_blob_sp;

    /// 'ClusterNode' associated to this event.
    mqbnet::ClusterNode* d_clusterNode_p;

    /// Flag indicating if this is a relay event.
    bool d_isRelay;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DispatcherRecoveryEvent,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor.
    explicit DispatcherRecoveryEvent(bslma::Allocator* allocator = 0);

    DispatcherRecoveryEvent(bslmf::MovableRef<DispatcherRecoveryEvent> other);

    // ACCESSORS

    /// Return whether this event is a relay event or not.
    bool isRelay() const;

    /// Return a reference not offering modifiable access to the blob
    /// associated to this event.  The blob represents the raw content of
    /// the `bmqp::Event` this putEvent originates from.  The `blob` is
    /// only valid when `isRelay() == false`.  Typically, `blob` is used
    /// when there may be multiple push messages; while `guid` and `queueId`
    /// are used when only one is present.
    const bsl::shared_ptr<bdlbb::Blob>& blob() const;

    /// Return a pointer to the cluster node this event originate from, or
    /// null if it doesn't come from a cluster node.  This is mainly useful
    /// for logging purposes.
    mqbnet::ClusterNode* clusterNode() const;

    // MANIPULATORS
    DispatcherRecoveryEvent&
    setBlob(const bsl::shared_ptr<bdlbb::Blob>& value);

    DispatcherRecoveryEvent& setIsRelay(bool value);

    DispatcherRecoveryEvent& setClusterNode(mqbnet::ClusterNode* value);
};

// ============================
// class DispatcherReceiptEvent
// ============================

/// DispatcherEvent interface view of an event of type `e_RECOVERY`.
class DispatcherReceiptEvent {
  private:
    // DATA
    /// Blob of data embedded in this event.
    bsl::shared_ptr<bdlbb::Blob> d_blob_sp;

    /// 'ClusterNode' associated to this event.
    mqbnet::ClusterNode* d_clusterNode_p;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DispatcherRecoveryEvent,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor.
    explicit DispatcherReceiptEvent(bslma::Allocator* allocator = 0);

    DispatcherReceiptEvent(bslmf::MovableRef<DispatcherReceiptEvent> other);

    // ACCESSORS

    /// Return a reference not offering modifiable access to the blob
    /// associated to this event.  The blob represents the raw content of
    /// the `bmqp::Event` this putEvent originates from.  The `blob` is
    /// only valid when `isRelay() == false`.  Typically, `blob` is used
    /// when there may be multiple push messages; while `guid` and `queueId`
    /// are used when only one is present.
    const bsl::shared_ptr<bdlbb::Blob>& blob() const;

    /// Return a pointer to the cluster node this event originate from, or
    /// null if it doesn't come from a cluster node.  This is mainly useful
    /// for logging purposes.
    mqbnet::ClusterNode* clusterNode() const;

    // MANIPULATORS
    DispatcherReceiptEvent&
    setBlob(const bsl::shared_ptr<bdlbb::Blob>& value);

    DispatcherReceiptEvent& setClusterNode(mqbnet::ClusterNode* value);
};

// =====================
// class DispatcherEvent
// =====================

/// Value semantic like object holding the context information of an Event
/// dispatched by the Dispatcher.
class DispatcherEvent {
  private:
    // TYPES
    typedef bsl::variant<bsl::monostate,
                         DispatcherDispatcherEvent,
                         DispatcherCallbackEvent,
                         DispatcherControlMessageEvent,
                         DispatcherConfirmEvent,
                         DispatcherRejectEvent,
                         DispatcherPushEvent,
                         DispatcherPutEvent,
                         DispatcherAckEvent,
                         DispatcherClusterStateEvent,
                         DispatcherStorageEvent,
                         DispatcherRecoveryEvent,
                         DispatcherReceiptEvent>
        EventImpl;

    // DATA

    /// Allocator to use
    bslma::Allocator* d_allocator_p;

    /// Source ('producer') of this event, if any.
    DispatcherClient* d_source_p;

    /// Destination ('consumer') for this event.
    DispatcherClient* d_destination_p;

    /// Type of the Event.
    DispatcherEventType::Enum d_type;

    EventImpl d_eventImpl;

    /// In-place storage for the callback in this event.
    bmqu::ManagedCallback d_callback;

    /// Callback embedded in this event.  This callback is called when the
    /// 'Dispatcher::execute' method is used to enqueue an event to multiple
    /// processors, and will be called when the last processor finished
    /// processing it.
    bmqu::ManagedCallback d_finalizeCallback;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DispatcherEvent, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor of a `DispatcherEvent`, using the specified `allocator`.
    explicit DispatcherEvent(bslma::Allocator* allocator = 0);

  public:
    // MANIPULATORS
    DispatcherDispatcherEvent& makeDispatcherEvent();

    DispatcherCallbackEvent& makeCallbackEvent();

    DispatcherControlMessageEvent& makeControlMessageEvent();

    DispatcherConfirmEvent& makeConfirmEvent();

    DispatcherRejectEvent& makeRejectEvent();

    DispatcherPushEvent& makePushEvent();

    DispatcherPushEvent&
    makePushEvent(const bsl::shared_ptr<bdlbb::Blob>& blob_sp);

    DispatcherPushEvent&
    makePushEvent(const bsl::shared_ptr<bdlbb::Blob>&  blob_sp,
                  const bsl::shared_ptr<bdlbb::Blob>&  options_sp,
                  const bmqt::MessageGUID&             msgGUID,
                  const bmqp::MessagePropertiesInfo&   mp,
                  bmqt::CompressionAlgorithmType::Enum cat,
                  bool                                 isOutOfOrder);

    DispatcherPushEvent&
    makePushEvent(const bsl::shared_ptr<bdlbb::Blob>&       blob_sp,
                  const bmqt::MessageGUID&                  msgGUID,
                  const bmqp::MessagePropertiesInfo&        mp,
                  int                                       queueId,
                  bmqt::CompressionAlgorithmType::Enum      cat,
                  bool                                      isOutOfOrder,
                  const bmqp::Protocol::SubQueueInfosArray& subQueueInfos,
                  const bmqp::Protocol::MsgGroupId&         msgGroupId);

    DispatcherPutEvent& makePutEvent();

    DispatcherReceiptEvent& makeReceiptEvent();

    DispatcherAckEvent& makeAckEvent();

    DispatcherStorageEvent& makeStorageEvent();

    DispatcherRecoveryEvent& makeRecoveryEvent();

    DispatcherClusterStateEvent& makeClusterStateEvent();

    DispatcherEvent& setSource(DispatcherClient* value);
    DispatcherEvent& setDestination(DispatcherClient* value);

    /// Reset all members of this `DispatcherEvent` to a default value.
    void reset();

    // ACCESSORS

    /// Return the type of this event.
    DispatcherEventType::Enum type() const;

    /// Return the DispatcherClient source (`producer`) of this event, if
    /// specified by the producer of this event.
    DispatcherClient* source() const;

    /// Return the DispatcherClient destination target (`consumer`) of this
    /// event.
    DispatcherClient* destination() const;

    template <class EventType>
    const EventType& getAs() const;

    template <class EventType>
    EventType& getAs();

    /// Return this object as the corresponding event type.  The behavior is
    /// undefined unless `type()` returns the appropriate matching type.
    const DispatcherReceiptEvent* asReceiptEvent() const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const DispatcherEvent& rhs);

// ==========================
// class DispatcherClientData
// ==========================

/// Value semantic type for dispatcher client data, holding the state and
/// link between the Dispatcher and the DispatcherClient.
class DispatcherClientData {
  private:
    // DATA
    DispatcherClientType::Enum d_clientType;
    // Type of dispatcher client.

    Dispatcher::ProcessorHandle d_processorHandle;
    // Processor handle to which the client is
    // associated with.

    bool d_addedToFlushList;
    // Flag indicating whether the dispatcher
    // added the corresponding client to its
    // internal flush list -- this is a
    // Dispatcher internal member that should
    // only be manipulated by the dispatcher, and
    // not the clients.

    Dispatcher* d_dispatcher_p;
    // The dispatcher associated with the client.

  public:
    // CREATORS

    /// Default constructor
    explicit DispatcherClientData();

    // MANIPULATORS
    DispatcherClientData& setClientType(DispatcherClientType::Enum value);
    DispatcherClientData&
    setProcessorHandle(Dispatcher::ProcessorHandle value);
    DispatcherClientData& setAddedToFlushList(bool value);

    /// Set the corresponding member to the specified `value` and return a
    /// reference offering modifiable access to this object.
    DispatcherClientData& setDispatcher(Dispatcher* value);

    /// Return a pointer to the dispatcher associated with this object; or
    /// null is this client is not (yet) registered to a dispatcher.
    Dispatcher* dispatcher();

    // ACCESSORS
    DispatcherClientType::Enum  clientType() const;
    Dispatcher::ProcessorHandle processorHandle() const;
    bool                        addedToFlushList() const;

    /// Return the value of the corresponding member.
    const Dispatcher* dispatcher() const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&               stream,
                         const DispatcherClientData& rhs);

// ======================
// class DispatcherClient
// ======================

/// Interface for a client of the Dispatcher.
class DispatcherClient {
  public:
    // CREATORS

    /// Destructor.
    virtual ~DispatcherClient();

    // MANIPULATORS

    /// Return a pointer to the dispatcher this client is associated with.
    virtual Dispatcher* dispatcher() = 0;

    /// Return a reference offering modifiable access to the
    /// DispatcherClientData of this client.
    virtual DispatcherClientData& dispatcherClientData() = 0;

    /// Called by the `Dispatcher` when it has the specified `event` to
    /// deliver to the client.
    virtual void onDispatcherEvent(const DispatcherEvent& event) = 0;

    /// Called by the dispatcher to flush any pending operation; mainly
    /// used to provide batch and nagling mechanism.
    virtual void flush() = 0;

    // ACCESSORS

    /// Return a pointer to the dispatcher this client is associated with.
    virtual const Dispatcher* dispatcher() const = 0;

    /// Return a reference not offering modifiable access to the
    /// DispatcherClientData of this client.
    virtual const DispatcherClientData& dispatcherClientData() const = 0;

    /// Return a printable description of the client (e.g., for logging).
    virtual const bsl::string& description() const = 0;
};

// FREE OPERATORS

/// Format the specified `client` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const DispatcherClient& client);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class DispatcherEvent
// ---------------------

inline DispatcherEvent::DispatcherEvent(bslma::Allocator* allocator)

: d_allocator_p(bslma::Default::allocator(allocator))
, d_source_p(0)
, d_destination_p(0)
, d_type(DispatcherEventType::e_UNDEFINED)
, d_eventImpl(bsl::allocator_arg, d_allocator_p)
{
    // NOTHING
}

inline DispatcherEvent& DispatcherEvent::setSource(DispatcherClient* value)
{
    d_source_p = value;
    return *this;
}

inline DispatcherEvent&
DispatcherEvent::setDestination(DispatcherClient* value)
{
    d_destination_p = value;
    return *this;
}

inline void DispatcherEvent::reset()
{
    // TODO lazy undefine
    // keep `d_allocator_p`
    d_eventImpl     = bsl::monostate();
    d_type          = DispatcherEventType::e_UNDEFINED;
    d_source_p      = 0;
    d_destination_p = 0;
}

inline DispatcherDispatcherEvent& DispatcherEvent::makeDispatcherEvent()
{
    d_type = DispatcherEventType::e_DISPATCHER;
    return d_eventImpl.emplace<DispatcherDispatcherEvent>();
}

inline DispatcherCallbackEvent& DispatcherEvent::makeCallbackEvent()
{
    d_type = DispatcherEventType::e_CALLBACK;
    return d_eventImpl.emplace<DispatcherCallbackEvent>();
}

inline DispatcherControlMessageEvent&
DispatcherEvent::makeControlMessageEvent()
{
    d_type = DispatcherEventType::e_CONTROL_MSG;
    return d_eventImpl.emplace<DispatcherControlMessageEvent>();
}

inline DispatcherConfirmEvent& DispatcherEvent::makeConfirmEvent()
{
    d_type = DispatcherEventType::e_CONFIRM;
    return d_eventImpl.emplace<DispatcherConfirmEvent>();
}

inline DispatcherRejectEvent& DispatcherEvent::makeRejectEvent()
{
    d_type = DispatcherEventType::e_REJECT;
    return d_eventImpl.emplace<DispatcherRejectEvent>();
}

inline DispatcherPushEvent& DispatcherEvent::makePushEvent()
{
    d_type = DispatcherEventType::e_PUSH;
    return d_eventImpl.emplace<DispatcherPushEvent>();
}

inline DispatcherPushEvent&
DispatcherEvent::makePushEvent(const bsl::shared_ptr<bdlbb::Blob>& blob_sp)
{
    d_type = DispatcherEventType::e_PUSH;
    return d_eventImpl.emplace<DispatcherPushEvent>(blob_sp);
}

inline DispatcherPushEvent&
DispatcherEvent::makePushEvent(const bsl::shared_ptr<bdlbb::Blob>&  blob_sp,
                               const bsl::shared_ptr<bdlbb::Blob>&  options_sp,
                               const bmqt::MessageGUID&             msgGUID,
                               const bmqp::MessagePropertiesInfo&   mp,
                               bmqt::CompressionAlgorithmType::Enum cat,
                               bool isOutOfOrder)
{
    d_type = DispatcherEventType::e_PUSH;
    return d_eventImpl.emplace<DispatcherPushEvent>(blob_sp,
                                                    options_sp,
                                                    msgGUID,
                                                    mp,
                                                    cat,
                                                    isOutOfOrder);
}

inline DispatcherPushEvent& DispatcherEvent::makePushEvent(
    const bsl::shared_ptr<bdlbb::Blob>&       blob_sp,
    const bmqt::MessageGUID&                  msgGUID,
    const bmqp::MessagePropertiesInfo&        mp,
    int                                       queueId,
    bmqt::CompressionAlgorithmType::Enum      cat,
    bool                                      isOutOfOrder,
    const bmqp::Protocol::SubQueueInfosArray& subQueueInfos,
    const bmqp::Protocol::MsgGroupId&         msgGroupId)
{
    d_type = DispatcherEventType::e_PUSH;
    return d_eventImpl.emplace<DispatcherPushEvent>(blob_sp,
                                                    msgGUID,
                                                    mp,
                                                    queueId,
                                                    cat,
                                                    isOutOfOrder,
                                                    subQueueInfos,
                                                    msgGroupId);
}

inline DispatcherPutEvent& DispatcherEvent::makePutEvent()
{
    d_type = DispatcherEventType::e_PUT;
    return d_eventImpl.emplace<DispatcherPutEvent>();
}

inline DispatcherReceiptEvent& DispatcherEvent::makeReceiptEvent()
{
    d_type = DispatcherEventType::e_REPLICATION_RECEIPT;
    return d_eventImpl.emplace<DispatcherReceiptEvent>();
}

inline DispatcherAckEvent& DispatcherEvent::makeAckEvent()
{
    d_type = DispatcherEventType::e_ACK;
    return d_eventImpl.emplace<DispatcherAckEvent>();
}

inline DispatcherStorageEvent& DispatcherEvent::makeStorageEvent()
{
    d_type = DispatcherEventType::e_STORAGE;
    return d_eventImpl.emplace<DispatcherStorageEvent>();
}

inline DispatcherRecoveryEvent& DispatcherEvent::makeRecoveryEvent()
{
    d_type = DispatcherEventType::e_RECOVERY;
    return d_eventImpl.emplace<DispatcherRecoveryEvent>();
}

inline DispatcherClusterStateEvent& DispatcherEvent::makeClusterStateEvent()
{
    d_type = DispatcherEventType::e_CLUSTER_STATE;
    return d_eventImpl.emplace<DispatcherClusterStateEvent>();
}

inline DispatcherEventType::Enum DispatcherEvent::type() const
{
    return d_type;
}

inline DispatcherClient* DispatcherEvent::source() const
{
    return d_source_p;
}

inline DispatcherClient* DispatcherEvent::destination() const
{
    return d_destination_p;
}

template <class EventType>
inline const EventType& DispatcherEvent::getAs() const
{
    return bsl::get<EventType>(d_eventImpl);
}

template <class EventType>
inline EventType& DispatcherEvent::getAs()
{
    return bsl::get<EventType>(d_eventImpl);
}

// --------------------------
// class DispatcherClientData
// --------------------------

inline DispatcherClientData::DispatcherClientData()
: d_clientType(DispatcherClientType::e_UNDEFINED)
, d_processorHandle(Dispatcher::k_INVALID_PROCESSOR_HANDLE)
, d_addedToFlushList(false)
, d_dispatcher_p(0)
{
    // NOTHING
}

inline DispatcherClientData&
DispatcherClientData::setClientType(DispatcherClientType::Enum value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_clientType == DispatcherClientType::e_UNDEFINED &&
                     "Dispatcher type can only be set once");

    d_clientType = value;
    return *this;
}

inline DispatcherClientData&
DispatcherClientData::setProcessorHandle(Dispatcher::ProcessorHandle value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        (d_processorHandle == Dispatcher::k_INVALID_PROCESSOR_HANDLE ||
         value == Dispatcher::k_INVALID_PROCESSOR_HANDLE) &&
        "Processor handle can only be set once");

    d_processorHandle = value;
    return *this;
}

inline DispatcherClientData&
DispatcherClientData::setAddedToFlushList(bool value)
{
    d_addedToFlushList = value;
    return *this;
}

inline DispatcherClientData&
DispatcherClientData::setDispatcher(Dispatcher* value)
{
    d_dispatcher_p = value;
    return *this;
}

inline Dispatcher* DispatcherClientData::dispatcher()
{
    return d_dispatcher_p;
}

inline DispatcherClientType::Enum DispatcherClientData::clientType() const
{
    return d_clientType;
}

inline Dispatcher::ProcessorHandle
DispatcherClientData::processorHandle() const
{
    return d_processorHandle;
}

inline bool DispatcherClientData::addedToFlushList() const
{
    return d_addedToFlushList;
}

inline const Dispatcher* DispatcherClientData::dispatcher() const
{
    return d_dispatcher_p;
}

}  // close package namespace

// ---------------------------
// struct DispatcherClientType
// ---------------------------

inline bsl::ostream& mqbi::operator<<(bsl::ostream&                    stream,
                                      mqbi::DispatcherClientType::Enum value)
{
    return mqbi::DispatcherClientType::print(stream, value, 0, -1);
}

// --------------------------
// struct DispatcherEventType
// --------------------------

inline bsl::ostream& mqbi::operator<<(bsl::ostream&                   stream,
                                      mqbi::DispatcherEventType::Enum value)
{
    return mqbi::DispatcherEventType::print(stream, value, 0, -1);
}

// ---------------------
// class DispatcherEvent
// ---------------------

inline bsl::ostream& mqbi::operator<<(bsl::ostream&                stream,
                                      const mqbi::DispatcherEvent& rhs)
{
    return rhs.print(stream, 0, -1);
}

// --------------------------
// class DispatcherClientData
// --------------------------

inline bsl::ostream& mqbi::operator<<(bsl::ostream&                     stream,
                                      const mqbi::DispatcherClientData& rhs)
{
    return rhs.print(stream, 0, -1);
}

// ----------------------
// class DispatcherClient
// ----------------------

inline bsl::ostream& mqbi::operator<<(bsl::ostream&                 stream,
                                      const mqbi::DispatcherClient& client)
{
    stream << client.description();
    return stream;
}

}  // close enterprise namespace

#endif
