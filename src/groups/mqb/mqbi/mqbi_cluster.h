// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbi_cluster.h                                                     -*-C++-*-
#ifndef INCLUDED_MQBI_CLUSTER
#define INCLUDED_MQBI_CLUSTER

//@PURPOSE: Provide an interface for a Cluster.
//
//@CLASSES:
//  mqbi::Cluster:          Interface for a Cluster.
//  mqbi::ClusterErrorCode: Enum for various cluster related error codes.
//
//@DESCRIPTION: 'mqbi::Cluster' is the interface for a cluster object.  A
// cluster represents an abstraction concept of a group of objects, allowing to
// interact with them as if it was a single entity.  A cluster is always
// associated to an 'mqbnet::Cluster' object, which is used for the transport
// and communication.  'mqbi::ClusterErrorCode' provides a namespaced enum for
// regrouping various cluster related specific error codes.
//
// There are mainly three types of cluster:
//: o !local!:  a cluster consisting of only one machine
//: o !remote!: a cluster of machines to which the current one is not part of
//: o !member!: a cluster of machines to which the current one is part of
//
/// Thread Safety
///-------------
// Every method on an 'mqbi::Cluster' implementation must be thread-safe.

// MQB

#include <mqbcfg_messages.h>
#include <mqbconfm_messages.h>
#include <mqbi_dispatcher.h>
#include <mqbi_queue.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_requestmanager.h>

#include <bmqio_status.h>

// BDE
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_optional.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class FixedThreadPool;
}
namespace bmqt {
class Uri;
}
namespace mqbcmd {
class ClusterCommand;
}
namespace mqbcmd {
class ClusterResult;
}
namespace mqbcmd {
class ClusterStatus;
}
namespace mqbnet {
class Cluster;
}
namespace mqbnet {
class ClusterNode;
}
namespace mqbnet {
template <class REQUEST, class RESPONSE, class TARGET>
class MultiRequestManager;
}

namespace mqbi {

// FORWARD DECLARATION
class Domain;

// =======================
// struct ClusterErrorCode
// =======================

/// Enumeration for various cluster related error codes.
struct ClusterErrorCode {
    // TYPES
    enum Enum {
        // Generic
        // - - - -
        e_OK = 0,

        e_UNKNOWN = -10
        // Operation failed for unknown reason
        ,
        e_STOPPING = -11
        // The node (either current or remote) is being stopped

        // ClusterProxy specific
        // - - - - - - - - - - -
        ,
        e_ACTIVE_LOST = -100
        // The connection to the active node was lost

        // Cluster specific
        // - - - - - - - -
        ,
        e_NOT_LEADER = -200
        // The node is not the leader of the cluster
        ,
        e_NOT_PRIMARY = -201
        // The node is not the primary of the partition
        ,
        e_NO_PARTITION = -202
        // Unable to find a partition for the queue
        ,
        e_NODE_DOWN = -203
        // The connection with the remote node went down
        ,
        e_UNKNOWN_QUEUE = -204
        // The node is not aware of that queue
        ,
        e_LIMIT = -205
        // A limit has been reached, currently:
        //: o too many active queues in the domain
        ,
        e_NOT_FOLLOWER = -206
        // The node is not a follower in the cluster
        ,
        e_NOT_REPLICA = -207
        // The node is not a replica of the partition
        ,
        e_CSL_FAILURE = -208
        // Failure to apply to the CSL
    };

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
    /// `ClusterErrorCode::Enum` value.
    static bsl::ostream& print(bsl::ostream&          stream,
                               ClusterErrorCode::Enum value,
                               int                    level          = 0,
                               int                    spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `BMQT_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(ClusterErrorCode::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(ClusterErrorCode::Enum*  out,
                          const bslstl::StringRef& str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, ClusterErrorCode::Enum value);

// =============
// class Cluster
// =============

/// Interface for a Cluster.
class Cluster : public DispatcherClient {
  public:
    // TYPES

    /// Type of a `cookie` provided in the `OpenQueueCallback` to confirm
    /// processing of the `openQueue` response by the requester.  Opening a
    /// queue is fully async, and it could happen that the requester went
    /// down before the `openQueue` response got delivered to it.  In this
    /// case, we must rollback upstream state.  This cookie is used for
    /// that: it is initialized to zero (in the `Cluster` implementation),
    /// and carried over to the original requester of the `openQueue`.  If
    /// the requester is not able to process the openQueue response, it
    /// needs to set this cookie to the queue handle which it received, so
    /// that the operation can be rolled back.
    typedef bsl::shared_ptr<QueueHandle*> OpenQueueConfirmationCookie;

    /// Signature of the callback passed to the `openQueue()` method: if the
    /// specified `status` is SUCCESS, the operation was a success and the
    /// specified `queueHandle` contains the queue handle, and the specified
    /// `openQueueResponse` contains the upstream response (if applicable,
    /// otherwise an injected response having valid routing configuration);
    /// otherwise `status` contains the category, error code and description
    /// of the failure.  In case of success, the specified
    /// `confirmationCookie` must be confirmed (set to `true`) by the
    /// requester (see meaning in the `OpenQueueConfirmationCookie` typedef
    /// above).
    typedef bsl::function<void(
        const bmqp_ctrlmsg::Status&            status,
        QueueHandle*                           queueHandle,
        const bmqp_ctrlmsg::OpenQueueResponse& openQueueResponse,
        const OpenQueueConfirmationCookie&     confirmationCookie)>
        OpenQueueCallback;

    // TYPES

    /// Signature of the callback to provide to the `releaseQueueHandle`
    /// method.  The specified `status` conveys the result of operation.
    typedef bsl::function<void(const bmqp_ctrlmsg::Status& status)>
        HandleReleasedCallback;

    /// Type of the RequestManager used by the cluster.
    typedef bmqp::RequestManager<bmqp_ctrlmsg::ControlMessage,
                                 bmqp_ctrlmsg::ControlMessage>
        RequestManagerType;

    typedef mqbnet::MultiRequestManager<bmqp_ctrlmsg::ControlMessage,
                                        bmqp_ctrlmsg::ControlMessage,
                                        mqbnet::ClusterNode*>
        MultiRequestManagerType;

    /// Signature of a `void` functor method.
    typedef bsl::function<void(void)> VoidFunctor;

  public:
    // CREATORS

    /// Destructor.
    ~Cluster() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Start the `Cluster`.  Starting the cluster implies performing any
    /// one-time initialization operations required to configure the
    /// cluster.  Return 0 on success and non-zero otherwise populating the
    /// specified `errorDescription` with the reason of the error.
    virtual int start(bsl::ostream& errorDescription) = 0;

    /// Initiate the shutdown of the cluster and invoke the specified
    /// `callback` upon completion of (asynchronous) shutdown sequence. It
    /// is expected that `stop()` will be called soon after this routine is
    /// invoked.  If the optional (temporary) specified 'supportShutdownV2' is
    /// 'true' execute shutdown logic V2 where upstream (not downstream) nodes
    /// deconfigure  queues and the shutting down node (not downstream) wait
    /// for CONFIRMS.
    virtual void initiateShutdown(const VoidFunctor& callback,
                                  bool supportShutdownV2 = false) = 0;

    /// Stop the `Cluster`; this is the counterpart of the `start()`
    /// operation.
    virtual void stop() = 0;

    /// Return a reference offering modifiable access to the net cluster
    /// used by this cluster.
    virtual mqbnet::Cluster& netCluster() = 0;

    /// Return a reference offering modifiable access to the request manager
    /// used by this cluster.
    virtual RequestManagerType& requestManager() = 0;

    // Return a reference offering a modifiable access to the multi request
    // manager used by this cluster.
    virtual MultiRequestManagerType& multiRequestManager() = 0;

    /// Send the specified `request` with the specified `timeout` to the
    /// specified `target` node.  If `target` is 0, it is the Cluster's
    /// implementation responsibility to decide which node to use (in
    /// `ClusterProxy` this will be the current `activeNode`, in `Cluster`
    /// this will be the current `leader`).  Return a status category
    /// indicating the result of the send operation.
    virtual bmqt::GenericResult::Enum
    sendRequest(const RequestManagerType::RequestSp& request,
                mqbnet::ClusterNode*                 target,
                bsls::TimeInterval                   timeout) = 0;

    /// Process the specified `response` message as a response to previously
    /// transmitted request.
    virtual void
    processResponse(const bmqp_ctrlmsg::ControlMessage& response) = 0;

    /// Open the queue with the specified `uri`, belonging to the specified
    /// `domain` with the specified `parameters` from a client identified
    /// with the specified `clientContext`.  Invoke the specified `callback`
    /// with the result of the operation (regardless of success or failure).
    virtual void openQueue(
        const bmqt::Uri&                                    uri,
        Domain*                                             domain,
        const bmqp_ctrlmsg::QueueHandleParameters&          handleParameters,
        const bsl::shared_ptr<QueueHandleRequesterContext>& clientContext,
        const OpenQueueCallback&                            callback) = 0;

    /// Configure the specified `upstreamSubQueueId` subStream of the
    /// specified `queue` with the specified `streamParameters`, and invoke
    /// the specified `callback` when finished.
    virtual void
    configureQueue(Queue*                                queue,
                   const bmqp_ctrlmsg::StreamParameters& streamParameters,
                   unsigned int                          upstreamSubQueueId,
                   const QueueHandle::HandleConfiguredCallback& callback) = 0;

    /// Configure the specified `upstreamSubQueueId` subStream of the
    /// specified `queue` with the specified `handleParameters` and invoke
    /// the specified `callback` when finished.
    virtual void
    configureQueue(Queue*                                     queue,
                   const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                   unsigned int                  upstreamSubQueueId,
                   const HandleReleasedCallback& callback) = 0;

    /// Invoked whenever an attempt was made to create a queue handle for
    /// the specified `queue` having the specified `uri`, with
    /// `handleCreated` flag indicating if the handle was created or not.
    virtual void onQueueHandleCreated(Queue*           queue,
                                      const bmqt::Uri& uri,
                                      bool             handleCreated) = 0;

    /// Invoked whenever a queue handle associated with the specified
    /// `queue` having the specified `uri` has been destroyed.
    virtual void onQueueHandleDestroyed(Queue*           queue,
                                        const bmqt::Uri& uri) = 0;

    /// Invoked whenever a `domain` previously configured with `oldDefn`
    /// is reconfigured with the definition `newDefn`.
    virtual void onDomainReconfigured(const Domain&           domain,
                                      const mqbconfm::Domain& oldDefn,
                                      const mqbconfm::Domain& newDefn) = 0;

    /// Process the specified `command`, and load the result in the
    /// specified `result`.  Return 0 if the command was successfully
    /// processed, or a non-zero value otherwise.
    virtual int processCommand(mqbcmd::ClusterResult*        result,
                               const mqbcmd::ClusterCommand& command) = 0;

    /// Load the cluster state to the specified `out` object.
    virtual void loadClusterStatus(mqbcmd::ClusterResult* out) = 0;

    /// Purge and force GC queues in this cluster on a given domain.
    virtual void purgeAndGCQueueOnDomain(mqbcmd::ClusterResult* result,
                                         const bsl::string& domainName) = 0;

    // ACCESSORS

    /// Return the name of this cluster.
    virtual const bsl::string& name() const = 0;

    /// Return a reference not offering modifiable access to the net cluster
    /// used by this cluster.
    virtual const mqbnet::Cluster& netCluster() const = 0;

    /// Return true if this cluster is a local cluster.
    virtual bool isLocal() const = 0;

    /// Return true if this cluster is a remote cluster.
    virtual bool isRemote() const = 0;

    /// Return true if this cluster is a `real` cluster, that is it is a
    /// member of a cluster.
    virtual bool isClusterMember() const = 0;

    /// Return true if this cluster is in the process of restoring its
    /// state; that is reopening the queues which were previously opened
    /// before a failover (active node switch, primary switch, ...).
    virtual bool isFailoverInProgress() const = 0;

    /// Return true if this cluster is stopping *or* has stopped, false
    /// otherwise.  Note that a cluster which has not been started will also
    /// return true.  TBD: this accessor should be replaced by something
    /// like `status()` which should return an enum specify various states
    /// like started/stopped/stopping etc.
    virtual bool isStopping() const = 0;

    /// Print a summary of the cluster state to the specified `os`.
    /// Optionally specify an initial indentation `level`, whose absolute
    /// value is incremented recursively for nested objects. If `level` is
    /// specified, optionally specify `spacesPerLevel`, whose absolute value
    /// indicates the number of spaces per indentation level for this and
    /// all of its nested objects. If `level` is negative, suppress
    /// indentation of the first line. If `spacesPerLevel` is negative,
    /// format the entire output on one line, suppressing all but the
    /// initial indentation (as governed by `level`).
    virtual void printClusterStateSummary(bsl::ostream& out,
                                          int           level = 0,
                                          int spacesPerLevel  = 0) const = 0;

    /// Return boolean flag indicating if CSL Mode is enabled.
    virtual bool isCSLModeEnabled() const;

    /// Return boolean flag indicating if CSL FSM workflow is in effect.
    virtual bool isFSMWorkflow() const;

    /// Return boolean flag indicating whether the broker still writes to the
    /// to-be-deprecated QLIST file when FSM workflow is enabled.
    virtual bool doesFSMwriteQLIST() const;

    /// Returns a pointer to cluster config if this `mqbi::Cluster`
    /// represents a cluster, otherwise null.
    virtual const mqbcfg::ClusterDefinition* clusterConfig() const = 0;

    /// Returns a pointer to cluster proxy config if this `mqbi::Cluster`
    /// represents a proxy, otherwise null.
    virtual const mqbcfg::ClusterProxyDefinition*
    clusterProxyConfig() const = 0;

    /// Gets all the nodes which are a primary for some partition of this
    /// cluster, storing each external node into the given `nodes` vector
    /// and/or marking `isSelfPrimary` as true if the self node is a primary.
    /// The self node will never be added to the `nodes` vector. Populates `rc`
    /// with 0 on success or a non-zero error code on failure. In the case of
    /// an error, the `errorDescription` output stream will be populated. Note
    /// this function uses an out parameter for the return code, `rc`. This is
    /// because this function is designed to be called by the dispatcher
    /// thread, so the return type of this function should be `void`.
    virtual void getPrimaryNodes(int*          rc,
                                 bsl::ostream& errorDescription,
                                 bsl::vector<mqbnet::ClusterNode*>* nodes,
                                 bool* isSelfPrimary) const = 0;

    /// Gets the node which is the primary for the given partitionId or sets
    /// `isSelfPrimary` to true if the caller is the primary. Note that the
    /// self node will never be populated into the given `node` pointer.
    /// Populates `rc` with 0 on success or a non-zero error code on failure.
    /// In the case of an error, the `errorDescription` output stream will be
    /// populated. Note this function uses an out parameter for the return
    /// code, `rc`. This is because this function is designed to be called by
    /// the dispatcher thread, so the return type of this function should be
    /// `void`.
    virtual void getPartitionPrimaryNode(int*          rc,
                                         bsl::ostream& errorDescription,
                                         mqbnet::ClusterNode** node,
                                         bool*                 isSelfPrimary,
                                         int partitionId) const = 0;

    /// Return `true` if this node is shutting down using new shutdown logic.
    /// This can only be true when all cluster nodes support StopRequest V2.
    virtual bool isShutdownLogicOn() const = 0;
};

struct ClusterResources {
    // Resources to use for all queues in all clusters
  public:
    // TYPES

    /// Pool of shared pointers to Blobs
    typedef bdlcc::SharedObjectPool<
        bdlbb::Blob,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::RemoveAll<bdlbb::Blob> >
        BlobSpPool;

  private:
    // PRIVATE DATA

    /// EventScheduler to use
    bdlmt::EventScheduler* d_scheduler_p;

    /// Blob buffer factory to use
    bdlbb::BlobBufferFactory* d_bufferFactory_p;

    /// Pool of shared pointers to blob to
    /// use.
    BlobSpPool* d_blobSpPool_p;

    /// Pool of PushStream elements for Proxy/Replica QueueEngine.
    bsl::shared_ptr<bdlma::ConcurrentPool> d_pushElementsPool_sp;

  public:
    // CREATORS

    explicit ClusterResources(bdlmt::EventScheduler*    scheduler,
                              bdlbb::BlobBufferFactory* bufferFactory,
                              BlobSpPool*               blobSpPool);

    explicit ClusterResources(
        bdlmt::EventScheduler*                        scheduler,
        bdlbb::BlobBufferFactory*                     bufferFactory,
        BlobSpPool*                                   blobSpPool,
        const bsl::shared_ptr<bdlma::ConcurrentPool>& pushElementsPool_sp);

    ClusterResources(const ClusterResources& copy);

    // ACCESSORS

    /// Returns a pointer to the event scheduler
    bdlmt::EventScheduler* scheduler() const;

    /// Returns a pointer to the blob buffer factory
    bdlbb::BlobBufferFactory* bufferFactory() const;

    /// Returns a pointer to the shared blob objects pool
    BlobSpPool* blobSpPool() const;

    /// Returns a shared pointer to the concurrent pool for Push elements
    const bsl::shared_ptr<bdlma::ConcurrentPool>& pushElementsPool() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

inline bool Cluster::isCSLModeEnabled() const
{
    return false;
}

inline bool Cluster::isFSMWorkflow() const
{
    return false;
}

inline bool Cluster::doesFSMwriteQLIST() const
{
    return false;
}

inline ClusterResources::ClusterResources(
    bdlmt::EventScheduler*    scheduler,
    bdlbb::BlobBufferFactory* bufferFactory,
    BlobSpPool*               blobSpPool)
: d_scheduler_p(scheduler)
, d_bufferFactory_p(bufferFactory)
, d_blobSpPool_p(blobSpPool)
, d_pushElementsPool_sp()
{
    BSLS_ASSERT_SAFE(d_scheduler_p);
    BSLS_ASSERT_SAFE(d_bufferFactory_p);
    BSLS_ASSERT_SAFE(d_blobSpPool_p);
}

inline ClusterResources::ClusterResources(
    bdlmt::EventScheduler*                        scheduler,
    bdlbb::BlobBufferFactory*                     bufferFactory,
    BlobSpPool*                                   blobSpPool,
    const bsl::shared_ptr<bdlma::ConcurrentPool>& pushElementsPool_sp)
: d_scheduler_p(scheduler)
, d_bufferFactory_p(bufferFactory)
, d_blobSpPool_p(blobSpPool)
, d_pushElementsPool_sp(pushElementsPool_sp)
{
    BSLS_ASSERT_SAFE(d_scheduler_p);
    BSLS_ASSERT_SAFE(d_bufferFactory_p);
    BSLS_ASSERT_SAFE(d_blobSpPool_p);
    BSLS_ASSERT_SAFE(d_pushElementsPool_sp);
}

inline ClusterResources::ClusterResources(const ClusterResources& copy)
: d_scheduler_p(copy.d_scheduler_p)
, d_bufferFactory_p(copy.d_bufferFactory_p)
, d_blobSpPool_p(copy.d_blobSpPool_p)
, d_pushElementsPool_sp(copy.d_pushElementsPool_sp)
{
    // NOTHING
}

inline bdlmt::EventScheduler* ClusterResources::scheduler() const
{
    return d_scheduler_p;
}
// EventScheduler to use

inline bdlbb::BlobBufferFactory* ClusterResources::bufferFactory() const
{
    return d_bufferFactory_p;
}
// Blob buffer factory to use

inline ClusterResources::BlobSpPool* ClusterResources::blobSpPool() const
{
    return d_blobSpPool_p;
}
// Pool of shared pointers to blob to
// use.

inline const bsl::shared_ptr<bdlma::ConcurrentPool>&
ClusterResources::pushElementsPool() const
{
    return d_pushElementsPool_sp;
}

}  // close package namespace

// -----------------------
// struct ClusterErrorCode
// -----------------------

inline bsl::ostream& mqbi::operator<<(bsl::ostream&                stream,
                                      mqbi::ClusterErrorCode::Enum value)
{
    return mqbi::ClusterErrorCode::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
