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

// mqbblp_recoverymanager.h                                           -*-C++-*-
#ifndef INCLUDED_MQBBLP_RECOVERYMANAGER
#define INCLUDED_MQBBLP_RECOVERYMANAGER

//@PURPOSE: Provide a mechanism to manage storage recovery in a cluster node.
//
//@CLASSES:
//  mqbblp::RecoveryManager: Mechanism to manage recovery in a cluster node.
//
//@DESCRIPTION: 'mqbblp::RecoveryManager' provides a mechanism to manage
// storage recovery in a cluster node.

// MQB

#include <mqbc_clusterdata.h>
#include <mqbc_clusternodesession.h>
#include <mqbc_clusterutil.h>
#include <mqbcfg_messages.h>
#include <mqbi_dispatcher.h>
#include <mqbnet_cluster.h>
#include <mqbnet_multirequestmanager.h>
#include <mqbs_datastore.h>
#include <mqbs_filestore.h>
#include <mqbs_filestoreset.h>
#include <mqbs_filestoreutil.h>
#include <mqbs_mappedfiledescriptor.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// MWC
#include <mwcu_blob.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_functional.h>
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_spinlock.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bslmt {
class Latch;
}
namespace mqbc {
class ClusterData;
}

namespace mqbblp {

// FORWARD DECLARATION
class RecoveryManager;
class RecoveryManager_ChunkDeleter;

// ===================================
// class RecoveryManager_PartitionInfo
// ===================================

/// Private class.  Implementation detail of `mqbblp::RecoveryManager`.
/// This class provides a VST representing some static information
/// associated with a storage partition.
class RecoveryManager_PartitionInfo {
  private:
    // DATA
    mqbi::DispatcherClientData d_dispData;

    bsls::AtomicBool d_initializeDispatcherClient;
    // Track initialization for associated
    // 'd_clientType'.

  public:
    // CREATORS
    RecoveryManager_PartitionInfo();

    RecoveryManager_PartitionInfo(const RecoveryManager_PartitionInfo& other);

    // MANIPULATORS
    RecoveryManager_PartitionInfo&
    operator=(const RecoveryManager_PartitionInfo& rhs);

    void setDispatcherClientData(const mqbi::DispatcherClientData& value);

    // ACCESSORS
    const mqbi::DispatcherClientData& dispatcherClientData() const;
    bool                              isInitialized() const;
};

// ======================================
// class RecoveryManager_FileTransferInfo
// ======================================

/// Private class.  Implementation detail of `mqbblp::RecoveryManager`.
/// This class provides a VST representing the details associated with a
/// file transfer operation.
class RecoveryManager_FileTransferInfo {
    // FRIENDS
    friend class RecoveryManager_ChunkDeleter;

  private:
    // DATA
    mqbs::MappedFileDescriptor d_journalFd;

    mqbs::MappedFileDescriptor d_dataFd;

    mqbs::MappedFileDescriptor d_qlistFd;

    bool d_areFileMapped;

    bsls::AtomicInt64 d_aliasedChunksCount;
    // Number of chunk blob buffers
    // referring to data/qlist/journal
    // files.  Separate counter per file is
    // not maintained, but can be done if
    // desired.

  public:
    // CREATORS
    RecoveryManager_FileTransferInfo();

    RecoveryManager_FileTransferInfo(
        const RecoveryManager_FileTransferInfo& other);

    // MANIPULATORS
    RecoveryManager_FileTransferInfo&
    operator=(const RecoveryManager_FileTransferInfo& rhs);

    bsls::Types::Int64          incrementAliasedChunksCount();
    mqbs::MappedFileDescriptor& journalFd();
    mqbs::MappedFileDescriptor& dataFd();
    mqbs::MappedFileDescriptor& qlistFd();
    void                        setAreFilesMapped(bool value);
    void                        clear();

    // ACCESSORS
    const mqbs::MappedFileDescriptor& journalFd() const;
    const mqbs::MappedFileDescriptor& dataFd() const;
    const mqbs::MappedFileDescriptor& qlistFd() const;
    bsls::Types::Int64                aliasedChunksCount() const;
    bool                              areFilesMapped() const;
};

// =====================================
// class RecoveryManager_RecoveryContext
// =====================================

/// Private class.  Implementation detail of `mqbblp::RecoveryManager`.
/// This class provides a VST representing the context associated with a
/// storage partition.
class RecoveryManager_RecoveryContext {
  public:
    // TYPES
    typedef bsl::vector<bsl::shared_ptr<bdlbb::Blob> > StorageEvents;

    typedef bsl::function<void(int                  partitionId,
                               int                  status,
                               const StorageEvents& bufferedEvents,
                               mqbnet::ClusterNode* recoveryPeer)>
        PartitionRecoveryCb;

  private:
    // PRIVATE TYPES
    typedef mqbs::FileStoreSet FileSet;

    typedef bdlmt::EventScheduler::EventHandle EventHandle;

    typedef bdlb::NullableValue<bmqp_ctrlmsg::SyncPoint> NullableSyncPoint;

  private:
    // DATA
    int d_numAttempts;
    // Number of attempts for storage sync
    // at startup.  This variable is
    // incremented everytime a request is
    // successfully sent to a peer.

    mqbs::MappedFileDescriptor d_journalFd;

    mqbs::MappedFileDescriptor d_dataFd;

    mqbs::MappedFileDescriptor d_qlistFd;

    FileSet d_fileSet;

    PartitionRecoveryCb d_recoveryCb;
    // Callback to be executed by the
    // recovery manager when recovery for
    // this partition is complete or
    // failed.  This callback must be
    // invoked by the recovery manager in
    // the dispatcher thread associated
    // with this partition.

    bmqp_ctrlmsg::SyncPoint d_oldSyncPoint;
    // Last valid sync point in the
    // journal.

    bsls::Types::Uint64 d_oldSyncPointOffset;
    // Zero value implies that there is no
    // valid sync point.

    bmqp_ctrlmsg::SyncPoint d_newSyncPoint;
    // New sync point received in the
    // stream.

    bsls::Types::Uint64 d_newSyncPointOffset;
    // Zero value implies that there is no
    // valid sync point.

    bsls::Types::Uint64 d_journalFileOffset;
    // Offset in journal file to which next
    // chunk of PATCH/FILE should be
    // written.

    bsls::Types::Uint64 d_dataFileOffset;
    // Offset in data file to which next
    // chunk of PATCH/FILE should be
    // written.

    bsls::Types::Uint64 d_qlistFileOffset;
    // Offset in qlist file to which next
    // chunk of FILE should be written.

    StorageEvents d_bufferedEvents;
    // List of storage events which are
    // buffered while recovery is in
    // progress.  Once recovery is
    // complete, these events are applied
    // to bring the node up-to-date with
    // this partition, and 'd_inRecovery'
    // flag is set to false.  Note that
    // first message in the first event
    // will be the new sync point.

    bool d_inRecovery;
    // Flag to indicate if this partition
    // is recovering.  This may or may not
    // mean that there is an active
    // recovery going on.  See
    // 'd_recoveryPeer_p' for that.

    mqbnet::ClusterNode* d_recoveryPeer_p;
    // Peer node which is serving recovery
    // request for this partition.  If this
    // is zero, it means there is no active
    // recovery in progress.

    bmqp_ctrlmsg::StorageSyncResponseType::Value d_responseType;
    // Type of storage sync response sent
    // by the peer.

    bmqp::RecoveryFileChunkType::Enum d_expectedChunkFileType;
    // Type of file chunk expected next
    // from the peer.

    unsigned int d_lastChunkSequenceNumber;
    // Sequence number of last chunk
    // received from the peer.

    EventHandle d_recoveryStartupWaitHandle;
    // When a node is started, each
    // partition looks to initiate recovery
    // (storage sync; see 'startRecovery').
    // At that time, an event is scheduled
    // for each partition to check if a
    // syncPt has been received for that
    // partition within a configured time
    // window.  This handle represents that
    // timed check.  If no syncPt is
    // received in that time, then self
    // node will initiate recovery w/ any
    // AVAILABLE peer.

    EventHandle d_recoveryStatusCheckHandle;
    // Once recovery for a partition has
    // been started with a peer, an event
    // is scheduled to check the status of
    // recovery, and cancel it if its not
    // complete within the stipulated time.

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RecoveryManager_RecoveryContext,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    explicit RecoveryManager_RecoveryContext(
        bslma::Allocator* basicAllocator = 0);

    RecoveryManager_RecoveryContext(
        const RecoveryManager_RecoveryContext& other,
        bslma::Allocator*                      basicAllocator = 0);

    // MANIPULATORS
    void setNumAttempts(int value);

    mqbs::MappedFileDescriptor& journalFd();

    mqbs::MappedFileDescriptor& dataFd();

    mqbs::MappedFileDescriptor& qlistFd();

    mqbs::FileStoreSet& fileSet();

    void setRecoveryCb(const PartitionRecoveryCb& recoveryCb);

    void setOldSyncPoint(const bmqp_ctrlmsg::SyncPoint& syncPoint);

    void setOldSyncPointOffset(bsls::Types::Uint64 offset);

    void setNewSyncPoint(const bmqp_ctrlmsg::SyncPoint& syncPoint);

    void setNewSyncPointOffset(bsls::Types::Uint64 offset);

    void setJournalFileOffset(bsls::Types::Uint64 offset);

    void setDataFileOffset(bsls::Types::Uint64 offset);

    void setQlistFileOffset(bsls::Types::Uint64 offset);

    void setRecoveryStatus(bool value);

    void setRecoveryPeer(mqbnet::ClusterNode* node);

    void setResponseType(bmqp_ctrlmsg::StorageSyncResponseType::Value value);

    void setExpectedChunkFileType(bmqp::RecoveryFileChunkType::Enum value);

    void setLastChunkSequenceNumber(unsigned int value);

    EventHandle& recoveryStartupWaitHandle();

    EventHandle& recoveryStatusCheckHandle();

    void addStorageEvent(const bsl::shared_ptr<bdlbb::Blob>& event);

    void purgeStorageEvents();

    void clear();

    // ACCESSORS
    int numAttempts() const;

    const mqbs::MappedFileDescriptor& journalFd() const;

    const mqbs::MappedFileDescriptor& dataFd() const;

    const mqbs::MappedFileDescriptor& qlistFd() const;

    const mqbs::FileStoreSet& fileSet() const;

    const PartitionRecoveryCb& recoveryCb() const;

    const bmqp_ctrlmsg::SyncPoint& oldSyncPoint() const;

    bsls::Types::Uint64 oldSyncPointOffset() const;

    const bmqp_ctrlmsg::SyncPoint& newSyncPoint() const;

    bsls::Types::Uint64 newSyncPointOffset() const;

    bsls::Types::Uint64 journalFileOffset() const;

    bsls::Types::Uint64 dataFileOffset() const;

    bsls::Types::Uint64 qlistFileOffset() const;

    bool inRecovery() const;

    mqbnet::ClusterNode* recoveryPeer() const;

    bmqp_ctrlmsg::StorageSyncResponseType::Value responseType() const;

    bmqp::RecoveryFileChunkType::Enum expectedChunkFileType() const;

    unsigned int lastChunkSequenceNumber() const;

    const StorageEvents& storageEvents() const;
};

// ========================================
// class RecoveryManager_PrimarySyncContext
// ========================================

/// Private class.  Implementation detail of `mqbblp::RecoveryManager`.
/// This class provides a VST representing the context associated with a
/// partition under primary-sync.
class RecoveryManager_PrimarySyncContext {
    // FRIENDS
    friend class RecoveryManager_ChunkDeleter;

  public:
    // PUBLIC TYPES
    typedef bsl::function<void(int,   // partitionId
                               int)>  // status
        PartitionPrimarySyncCb;

    typedef RecoveryManager_FileTransferInfo FileTransferInfo;

    /// This class provides a VST representing the state ("view") of a peer
    /// cluster node for a given partition.
    class PeerPartitionState {
      private:
        // DATA
        mqbnet::ClusterNode* d_peer_p;

        bmqp_ctrlmsg::PartitionSequenceNumber d_partitionSeqNum;
        // Peer's sequence number for the
        // associated partition

        bmqp_ctrlmsg::SyncPointOffsetPair d_lastSyncPointOffsetPair;
        // Peer's last sync point and its
        // offset for the associated partition

        bool d_needsPartitionSync;
        // Flag which indicates whether new
        // primary should attempt to sync this
        // peer's associated partition.  Note
        // that this flag doesn't mean that the
        // peer is necessarily behind.

      public:
        // CREATORS
        PeerPartitionState();

        PeerPartitionState(
            mqbnet::ClusterNode*                         peer,
            const bmqp_ctrlmsg::PartitionSequenceNumber& seqNum,
            const bmqp_ctrlmsg::SyncPointOffsetPair&     spOffsetPair,
            bool                                         needsPartitionSync);

        // MANIPULATORS
        void setPeer(mqbnet::ClusterNode* value);
        void setPartitionSequenceNum(
            const bmqp_ctrlmsg::PartitionSequenceNumber& value);
        void setLastSyncPointOffsetPair(
            const bmqp_ctrlmsg::SyncPointOffsetPair& value);
        void setNeedsPartitionSync(bool value);

        // ACCESSORS
        mqbnet::ClusterNode* peer() const;

        const bmqp_ctrlmsg::PartitionSequenceNumber&
        partitionSequenceNum() const;

        const bmqp_ctrlmsg::SyncPointOffsetPair&
        lastSyncPointOffsetPair() const;

        bool needsPartitionSync() const;
    };

    typedef bsl::vector<PeerPartitionState> PeerPartitionStates;

  private:
    // PRIVATE TYPES
    typedef bdlmt::EventScheduler::EventHandle EventHandle;

  private:
    // DATA
    RecoveryManager* d_rm_p;

    const mqbs::FileStore* d_fs_p;

    bool d_syncInProgress;
    // Flag to indicate if this partition
    // is under primary-sync.  This may or
    // may not mean that there is an active
    // primary-sync going on.  See
    // 'd_syncPeer_p' for that.

    mqbnet::ClusterNode* d_syncPeer_p;
    // Peer node which is serving primary
    // sync request for this partition.  If
    // this is zero, it means there is no
    // active primary-sync in progress.

    PartitionPrimarySyncCb d_primarySyncCb;
    // Callback to be executed by the
    // recovery manager when primary-sync
    // for this partition is complete or
    // failed.  This callback must be
    // invoked by the recovery manager in
    // the dispatcher thread associated
    // with this partition.

    bmqp_ctrlmsg::PartitionSequenceNumber d_selfPartitionSeqNum;

    bmqp_ctrlmsg::SyncPointOffsetPair d_selfLastSyncPtOffsetPair;

    FileTransferInfo d_fileTransferInfo;
    // TBD: when partition replay from
    // archived files is supported, we may
    // need to have a vector of this
    // variable.

    PeerPartitionStates d_peerPartitionStates;
    // Handle to one-time timer event to
    // check primary-sync status for this
    // partition.

    EventHandle d_syncStatusEventHandle;
    // Handle to one-time timer event to
    // check primary-sync status for this
    // partition.

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RecoveryManager_PrimarySyncContext,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    explicit RecoveryManager_PrimarySyncContext(
        bslma::Allocator* basicAllocator = 0);

    RecoveryManager_PrimarySyncContext(
        const RecoveryManager_PrimarySyncContext& other,
        bslma::Allocator*                         basicAllocator = 0);

    // MANIPULATORS
    void setRecoveryManager(RecoveryManager* value);

    void setFileStore(const mqbs::FileStore* value);

    void setPrimarySyncInProgress(bool value);

    void setPrimarySyncPeer(mqbnet::ClusterNode* node);

    void setPartitionPrimarySyncCb(const PartitionPrimarySyncCb& value);

    void setSelfPartitionSequenceNum(
        const bmqp_ctrlmsg::PartitionSequenceNumber& value);

    void setSelfLastSyncPtOffsetPair(
        const bmqp_ctrlmsg::SyncPointOffsetPair& value);

    FileTransferInfo& fileTransferInfo();

    PeerPartitionStates& peerPartitionStates();

    EventHandle& primarySyncStatusEventHandle();

    void clear();

    // ACCESSORS
    RecoveryManager* recoveryManager() const;

    const mqbs::FileStore* fileStore() const;

    bool primarySyncInProgress() const;

    mqbnet::ClusterNode* syncPeer() const;

    const PartitionPrimarySyncCb& partitionPrimarySyncCb() const;

    const bmqp_ctrlmsg::PartitionSequenceNumber&
    selfPartitionSequenceNum() const;

    const bmqp_ctrlmsg::SyncPointOffsetPair& selfLastSyncPtOffsetPair() const;

    const FileTransferInfo& fileTransferInfo() const;

    const PeerPartitionStates& peerPartitionStates() const;
};

// =========================================
// struct RecoveryManager_RequestContextType
// =========================================

/// This struct contains an enum which defines the type of request context.
struct RecoveryManager_RequestContextType {
    // PUBLIC TYPES
    enum Enum { e_UNDEFINED = 0, e_RECOVERY = 1, e_PARTITION_SYNC = 2 };

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
    /// what constitutes the string representation of a `FileType::Enum`
    /// value.
    static bsl::ostream& print(bsl::ostream&                            stream,
                               RecoveryManager_RequestContextType::Enum value,
                               int level          = 0,
                               int spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << RecoveryManager_RequestContextType::toAscii(
    ///                    RecoveryManager_RequestContextType::e_RECOVERY);
    /// ```
    /// will print the following on standard output:
    /// ```
    /// RECOVERY
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(RecoveryManager_RequestContextType::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                            stream,
                         RecoveryManager_RequestContextType::Enum value);

// ====================================
// class RecoveryManager_RequestContext
// ====================================

/// Private class.  Implementation detail of `mqbblp::RecoveryManager`.
/// This class provides a VST representing the context associated with a
/// recovery or partition sync request originating from a peer requester
/// node for a specific partition.
class RecoveryManager_RequestContext {
  public:
    // PUBLIC TYPES
    typedef RecoveryManager_FileTransferInfo FileTransferInfo;

  private:
    // PRIVATE TYPES
    typedef RecoveryManager_RequestContextType RequestContextType;

    // DATA
    RequestContextType::Enum d_contextType;

    mqbnet::ClusterNode* d_requester_p;

    int d_partitionId;

    RecoveryManager* d_recoveryManager_p;

    FileTransferInfo d_fileTransferInfo;

  public:
    // CREATORS
    RecoveryManager_RequestContext();

    // MANIPULATORS
    void              setContextType(RequestContextType::Enum value);
    void              setRequesterNode(mqbnet::ClusterNode* value);
    void              setPartitionId(int partitionId);
    void              setRecoveryManager(RecoveryManager* value);
    FileTransferInfo& fileTransferInfo();

    // ACCESSORS
    RequestContextType::Enum contextType() const;
    mqbnet::ClusterNode*     requesterNode() const;
    int                      partitionId() const;
    RecoveryManager*         recoveryManager() const;
    const FileTransferInfo&  fileTransferInfo() const;
};

// ==================================
// class RecoveryManager_ChunkDeleter
// ==================================

/// Private class.  Implementation detail of `mqbblp::RecoveryManager`.
/// This class provides a VST representing a custom deleter for a chunk of
/// file aliasing to the mapped region.
class RecoveryManager_ChunkDeleter {
  private:
    // PRIVATE TYPES
    typedef RecoveryManager_RequestContext     RequestContext;
    typedef RecoveryManager_PrimarySyncContext PrimarySyncContext;

    // DATA
    RequestContext*     d_requestContext_p;
    PrimarySyncContext* d_primarySyncContext_p;

  public:
    // CREATORS
    explicit RecoveryManager_ChunkDeleter(RequestContext* requestContext);

    explicit RecoveryManager_ChunkDeleter(
        PrimarySyncContext* primarySyncContext);

    // ACCESSORS
    void operator()(const void* ptr) const;
};

// =====================
// class RecoveryManager
// =====================

/// This component provides a mechanism to manage storage recovery in a
/// cluster node.
class RecoveryManager : public mqbnet::ClusterObserver {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.RECOVERYMANAGER");

    // FRIENDS
    friend class RecoveryManager_ChunkDeleter;

  private:
    // PRIVATE TYPES
    typedef RecoveryManager_FileTransferInfo FileTransferInfo;

    typedef RecoveryManager_ChunkDeleter ChunkDeleter;

    typedef RecoveryManager_PartitionInfo PartitionInfo;

    typedef bsl::vector<PartitionInfo> PartitionsInfo;

    typedef RecoveryManager_RecoveryContext RecoveryContext;

    typedef bsl::vector<RecoveryContext> RecoveryContexts;

    typedef RecoveryManager_PrimarySyncContext PrimarySyncContext;

    typedef bsl::vector<PrimarySyncContext> PrimarySyncContexts;

    typedef PrimarySyncContext::PeerPartitionState PeerPartitionState;

    typedef PrimarySyncContext::PeerPartitionStates PeerPartitionStates;

    typedef RecoveryManager_RequestContextType RequestContextType;

    typedef RecoveryManager_RequestContext RequestContext;

    /// Choice of `list` instead of `vector` is deliberate because we keep
    /// track of an element in the container via its iterator, and don't
    /// want it to get invalidated (see implementation of
    /// `processStorageSyncRequest` for details).
    typedef bsl::list<RequestContext> RequestContexts;

    typedef RequestContexts::iterator RequestContextIter;

    typedef bdlmt::EventScheduler::EventHandle EventHandle;

    typedef mqbc::ClusterMembership::ClusterNodeSessionSp ClusterNodeSessionSp;

    typedef mqbc::ClusterMembership::ClusterNodeSessionMap
        ClusterNodeSessionMap;

    typedef mqbc::ClusterMembership::ClusterNodeSessionMapConstIter
        ClusterNodeSessionMapConstIter;

    typedef bsl::vector<mqbnet::ClusterNode*> ClusterNodes;

    typedef ClusterNodes::iterator ClusterNodesIter;

    typedef ClusterNodes::const_iterator ClusterNodesConstIter;

    typedef RecoveryContext::PartitionRecoveryCb PartitionRecoveryCb;

    typedef PrimarySyncContext::PartitionPrimarySyncCb PartitionPrimarySyncCb;

    typedef mqbc::ClusterData::RequestManagerType RequestManagerType;

    typedef mqbc::ClusterData::MultiRequestManagerType MultiRequestManagerType;

    typedef MultiRequestManagerType::RequestContextSp RequestContextSp;

    typedef MultiRequestManagerType::NodeResponsePair NodeResponsePair;

    typedef MultiRequestManagerType::NodeResponsePairs NodeResponsePairs;

    typedef MultiRequestManagerType::NodeResponsePairsIter
        NodeResponsePairsIter;

    typedef MultiRequestManagerType::NodeResponsePairsConstIter
        NodeResponsePairsConstIter;

    typedef mqbs::FileStore::SyncPointOffsetPairs SyncPointOffsetPairs;

    typedef mqbs::FileStore::SyncPointOffsetConstIter SyncPointOffsetConstIter;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    bsls::AtomicBool d_isStarted;

    mqbi::Dispatcher* d_dispatcher_p;

    const mqbcfg::ClusterDefinition& d_clusterConfig;

    mqbi::DispatcherClient* d_cluster_p;

    mqbc::ClusterData* d_clusterData_p;

    mqbs::DataStoreConfig d_dataStoreConfig;

    PartitionsInfo d_partitionsInfo;
    // List of partitions information.
    // Note each entry in this list remains
    // valid all the time.  This variable
    // is manipulated only from the storage
    // (queue) dispatcher thread.

    RecoveryContexts d_recoveryContexts;
    // Vector of contexts of partitions
    // which are under recovery.  Items
    // from this vector are *not* removed
    // even after corresponding partitions
    // have recovered, but a flag is set to
    // indicate that the partition is no
    // longer under recovery.  This
    // variable is manipulated only from
    // the storage (queue) dispatcher
    // thread.

    PrimarySyncContexts d_primarySyncContexts;
    // Vector of contexts of partitions
    // which are under partition primary
    // sync.  Items from this vector are
    // *not* removed even after
    // corresponding partitions have synced
    // and this node can transition to
    // primary for that partition, but a
    // flag is set to indicate that
    // partition is no longer under primary
    // sync.  This variable is manipulated
    // only from the storage (queue)
    // dispatcher thread.

    RequestContexts d_recoveryRequestContexts;
    // List of contexts of recovery sync
    // requests for which this node is
    // sending responses to the requester
    // peer node.  Once the response has
    // been sent, the request context is
    // removed from this list.

    RequestContexts d_primarySyncRequestContexts;
    // List of contexts of primary sync
    // requests for which this node is
    // sending responses to the requester
    // peer node.  Once the response has
    // been sent, the request context is
    // removed from this list.

    bsls::SpinLock d_recoveryRequestContextLock;
    // Lock to protect access to
    // 'd_recoveryRequestContexts'.

    bsls::SpinLock d_primarySyncRequestContextLock;
    // Lock to protect access to
    // 'd_primarySyncRequestContexts'.

  private:
    // NOT IMPLEMENTED
    RecoveryManager(const RecoveryManager&);
    RecoveryManager& operator=(const RecoveryManager&);

  private:
    // PRIVATE MANIPULATORS

    /// Callback executed when the startup wait timer fires for the
    /// specified `partitionId`.  Executed by the scheduler thread.
    void recoveryStartupWaitCb(int partitionId);

    /// Executed by the cluster's dispatcher thread.
    void recoveryStartupWaitDispatched(int partitionId);

    /// Executed by the storage (queue) dispatcher thread.
    void
    recoveryStartupWaitPartitionDispatched(int                 partitionId,
                                           const ClusterNodes& availableNodes);

    /// Executed by the scheduler's dispatcher thread.
    void recoveryStatusCb(int partitionId);

    /// Executed by the storage (queue) dispatcher thread.
    void recoveryStatusDispatched(int partitionId);

    /// Executed by the scheduler's dispatcher thread.
    void primarySyncStatusCb(int partitionId);

    /// Executed by the storage (queue) dispatcher thread.
    void primarySyncStatusDispatched(int partitionId);

    /// Executed by the storage (queue) dispatcher thread.
    void onNodeDownDispatched(int partitionId, mqbnet::ClusterNode* node);

    /// Executed by the storage (queue) dispatcher thread.
    void partitionSyncCleanupDispatched(int partitionId);

    void sendStorageSyncRequesterHelper(RecoveryContext* context,
                                        int              partitionId);

    /// Executed by any thread.
    void onStorageSyncResponse(const RequestManagerType::RequestSp& context,
                               const mqbnet::ClusterNode*           responder);

    /// Executed by the dispatcher thread identified by the specified
    /// `processorId` assigned to the specified `partitionId`.
    void onStorageSyncResponseDispatched(
        int                                  partitionId,
        const RequestManagerType::RequestSp& context,
        const mqbnet::ClusterNode*           responder);

    /// Executed by the dispatcher thread associated with the specified
    /// `partitionId`.
    void onPartitionRecoveryStatus(int partitionId, int status);

    /// Executed by the dispatcher thread associated with the specified
    /// `partitionId`.
    void onPartitionPrimarySyncStatus(int partitionId, int status);

    void stopDispatched(int partitionId, bslmt::Latch* latch);

    int sendFile(RequestContext*                   context,
                 bsls::Types::Uint64               beginOffset,
                 bsls::Types::Uint64               endOffset,
                 unsigned int                      chunkSize,
                 bmqp::RecoveryFileChunkType::Enum chunkFileType);

    int replayPartition(
        RequestContext*                              requestContext,
        PrimarySyncContext*                          primarySyncContext,
        mqbnet::ClusterNode*                         destination,
        const bmqp_ctrlmsg::PartitionSequenceNumber& fromSequenceNum,
        const bmqp_ctrlmsg::PartitionSequenceNumber& toSequenceNum,
        bsls::Types::Uint64                          fromSyncPtOffset);

    void syncPeerPartitions(PrimarySyncContext* primarySyncCtx);

    int syncPeerPartition(PrimarySyncContext*       primarySyncCtx,
                          const PeerPartitionState& ppState);

    /// Process the partition-sync-state-query response contained in the
    /// specified `requestContext`.
    ///
    /// THREAD: This method is invoked in the associated cluster's IO
    ///         thread.
    void
    onPartitionSyncStateQueryResponse(const RequestContextSp& requestContext);

    /// Process the partition-sync-state-query response contained in the
    /// specified `requestContext`.
    ///
    /// THREAD: This method is invoked in the associated partition's
    ///         dispatcher thread.
    void onPartitionSyncStateQueryResponseDispatched(
        int                     partitionId,
        const RequestContextSp& requestContext);

    void onPartitionSyncDataQueryResponse(
        const RequestManagerType::RequestSp& context,
        const mqbnet::ClusterNode*           responder);

    // Process the partition-sync-data-query response contained in the
    // specified 'requestContext'.
    //
    // THREAD: This method is invoked in the associated cluster's IO
    //         thread.

    /// Process the partition-sync-data-query response contained in the
    /// specified `requestContext`.
    ///
    /// THREAD: This method is invoked in the associated partition's
    ///         dispatcher thread.
    void onPartitionSyncDataQueryResponseDispatched(
        int                                  partitionId,
        const RequestManagerType::RequestSp& context,
        const mqbnet::ClusterNode*           responder);

    bool hasSyncPoint(bmqp_ctrlmsg::SyncPoint* syncPoint,
                      mqbs::RecordHeader*      syncPointRecHeader,
                      mwcu::BlobPosition*      syncPointHeaderPosition,
                      int*                     messageNumber,
                      bsls::Types::Uint64*     journalOffset,
                      int                      partitionId,
                      const bsl::shared_ptr<bdlbb::Blob>& blob,
                      const mqbnet::ClusterNode*          source);

  public:
    // CREATORS

    /// Create a `RecoveryManager` object with the specified
    /// `clusterConfig`, `cluster`, `dataStoreConfig` and `dispatcher`.  Use
    /// the specified `allocator` for any memory allocation.
    RecoveryManager(const mqbcfg::ClusterDefinition& clusterConfig,
                    mqbi::DispatcherClient*          cluster,
                    mqbc::ClusterData*               clusterData,
                    const mqbs::DataStoreConfig&     dataStoreConfig,
                    mqbi::Dispatcher*                dispatcher,
                    bslma::Allocator*                allocator);

    /// Destroy this object.
    ~RecoveryManager() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    int start(bsl::ostream& errorDescription);

    void stop();

    /// Executed in the dispatcher thread associated with the specified
    /// `partitionId`.
    void startRecovery(int                               partitionId,
                       const mqbi::DispatcherClientData& dispatcherData,
                       const PartitionRecoveryCb&        partitionRecoveryCb);

    /// Executed in the dispatcher thread associated with the specified
    /// `partitionId`.
    void processStorageEvent(int                                 partitionId,
                             const bsl::shared_ptr<bdlbb::Blob>& blob,
                             mqbnet::ClusterNode*                source);

    /// Executed in the dispatcher thread associated with the partition
    /// belonging to the specified `message`/`fs`.
    void processStorageSyncRequest(const bmqp_ctrlmsg::ControlMessage& message,
                                   mqbnet::ClusterNode*                source,
                                   const mqbs::FileStore*              fs);

    /// Executed in the dispatcher thread associated with the specified
    /// `partitionId`.
    void processRecoveryEvent(int                                 partitionId,
                              const bsl::shared_ptr<bdlbb::Blob>& blob,
                              mqbnet::ClusterNode*                source);

    /// Executed in the dispatcher thread associated with the specified
    /// `partitionId`.
    void processShutdownEvent(int partitionId);

    /// Executed in the dispatcher thread associated with the specified
    /// `partitionId`.
    void startPartitionPrimarySync(
        const mqbs::FileStore*                   fs,
        const bsl::vector<mqbnet::ClusterNode*>& peers,
        const PartitionPrimarySyncCb&            partitionPrimarySyncCb);

    void processPartitionSyncStateRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source,
        const mqbs::FileStore*              fs);

    void processPartitionSyncDataRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source,
        const mqbs::FileStore*              fs);

    void processPartitionSyncDataRequestStatus(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source);

    // MANIPULATORS
    //   (virtual: mqbnet::ClusterObserver)

    /// Notification method to indicate that the specified `node` is now
    /// available (if the specified `isAvailable` is true) or not available
    /// (if `isAvailable` is false).  This method is invoked in any thread
    /// (mostly in `mqbblp::Cluster` IO thread).
    void onNodeStateChange(mqbnet::ClusterNode* node,
                           bool isAvailable) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    bool isStarted() const;

    bool isRecoveryInProgress(int partitionId) const;

    mqbnet::ClusterNode* recoveryPeer(int partitionId) const;

    bool isPrimarySyncInProgress(int partitionId) const;

    mqbnet::ClusterNode* primarySyncPeer(int partitionId) const;
};

// ============================================================================
//                            INLINE DEFINITIONS
// ============================================================================

// -----------------------------------
// class RecoveryManager_PartitionInfo
// -----------------------------------
// CREATORS
inline RecoveryManager_PartitionInfo::RecoveryManager_PartitionInfo()
: d_dispData()
, d_initializeDispatcherClient(false)
{
}

inline RecoveryManager_PartitionInfo::RecoveryManager_PartitionInfo(
    const RecoveryManager_PartitionInfo& other)
: d_dispData(other.d_dispData)
, d_initializeDispatcherClient(other.d_initializeDispatcherClient.load())
{
}

// MANIPULATORS
inline RecoveryManager_PartitionInfo& RecoveryManager_PartitionInfo::operator=(
    const RecoveryManager_PartitionInfo& rhs)
{
    if (this != &rhs) {
        d_dispData                   = rhs.d_dispData;
        d_initializeDispatcherClient = rhs.d_initializeDispatcherClient.load();
    }

    return *this;
}

inline void RecoveryManager_PartitionInfo::setDispatcherClientData(
    const mqbi::DispatcherClientData& value)
{
    d_dispData                   = value;
    d_initializeDispatcherClient = true;
}

// ACCESSORS
inline const mqbi::DispatcherClientData&
RecoveryManager_PartitionInfo::dispatcherClientData() const
{
    BSLS_ASSERT_SAFE(isInitialized());
    return d_dispData;
}

inline bool RecoveryManager_PartitionInfo::isInitialized() const
{
    return d_initializeDispatcherClient;
}

// --------------------------------------
// class RecoveryManager_FileTransferInfo
// --------------------------------------

// CREATORS
inline RecoveryManager_FileTransferInfo::RecoveryManager_FileTransferInfo()
: d_journalFd()
, d_dataFd()
, d_qlistFd()
, d_areFileMapped(false)
, d_aliasedChunksCount(0)
{
}

inline RecoveryManager_FileTransferInfo::RecoveryManager_FileTransferInfo(
    const RecoveryManager_FileTransferInfo& other)
: d_journalFd(other.d_journalFd)
, d_dataFd(other.d_dataFd)
, d_qlistFd(other.d_qlistFd)
, d_areFileMapped(other.d_areFileMapped)
, d_aliasedChunksCount(
      static_cast<bsls::Types::Int64>(other.d_aliasedChunksCount))
{
}

// MANIPULATORS
inline RecoveryManager_FileTransferInfo&
RecoveryManager_FileTransferInfo::operator=(
    const RecoveryManager_FileTransferInfo& rhs)
{
    if (this != &rhs) {
        d_journalFd          = rhs.d_journalFd;
        d_dataFd             = rhs.d_dataFd;
        d_qlistFd            = rhs.d_qlistFd;
        d_areFileMapped      = rhs.d_areFileMapped;
        d_aliasedChunksCount = static_cast<bsls::Types::Int64>(
            rhs.d_aliasedChunksCount);
    }

    return *this;
}

inline bsls::Types::Int64
RecoveryManager_FileTransferInfo::incrementAliasedChunksCount()
{
    return ++d_aliasedChunksCount;
}

inline mqbs::MappedFileDescriptor&
RecoveryManager_FileTransferInfo::journalFd()
{
    return d_journalFd;
}

inline mqbs::MappedFileDescriptor& RecoveryManager_FileTransferInfo::dataFd()
{
    return d_dataFd;
}

inline mqbs::MappedFileDescriptor& RecoveryManager_FileTransferInfo::qlistFd()
{
    return d_qlistFd;
}

inline void RecoveryManager_FileTransferInfo::setAreFilesMapped(bool value)
{
    d_areFileMapped = value;
}

// ACCESSORS
inline const mqbs::MappedFileDescriptor&
RecoveryManager_FileTransferInfo::journalFd() const
{
    return d_journalFd;
}

inline const mqbs::MappedFileDescriptor&
RecoveryManager_FileTransferInfo::dataFd() const
{
    return d_dataFd;
}

inline const mqbs::MappedFileDescriptor&
RecoveryManager_FileTransferInfo::qlistFd() const
{
    return d_qlistFd;
}

inline bool RecoveryManager_FileTransferInfo::areFilesMapped() const
{
    return d_areFileMapped;
}

inline bsls::Types::Int64
RecoveryManager_FileTransferInfo::aliasedChunksCount() const
{
    return d_aliasedChunksCount;
}

// -------------------------------------
// class RecoveryManager_RecoveryContext
// -------------------------------------

inline RecoveryManager_RecoveryContext::RecoveryManager_RecoveryContext(
    bslma::Allocator* basicAllocator)
: d_numAttempts(0)
, d_journalFd()
, d_dataFd()
, d_qlistFd()
, d_fileSet(basicAllocator)
, d_recoveryCb(bsl::allocator_arg, basicAllocator)
, d_oldSyncPoint()
, d_oldSyncPointOffset(0)
, d_newSyncPoint()
, d_newSyncPointOffset(0)
, d_journalFileOffset(0)
, d_dataFileOffset(0)
, d_qlistFileOffset(0)
, d_bufferedEvents(basicAllocator)
, d_inRecovery(false)
, d_recoveryPeer_p(0)
, d_responseType(bmqp_ctrlmsg::StorageSyncResponseType::E_UNDEFINED)
, d_expectedChunkFileType(bmqp::RecoveryFileChunkType::e_UNDEFINED)
, d_lastChunkSequenceNumber(0)
, d_recoveryStartupWaitHandle()
, d_recoveryStatusCheckHandle()
{
}

inline RecoveryManager_RecoveryContext::RecoveryManager_RecoveryContext(
    const RecoveryManager_RecoveryContext& other,
    bslma::Allocator*                      basicAllocator)
: d_numAttempts(0)
, d_journalFd(other.d_journalFd)
, d_dataFd(other.d_dataFd)
, d_qlistFd(other.d_qlistFd)
, d_fileSet(other.d_fileSet, basicAllocator)
, d_recoveryCb(bsl::allocator_arg, basicAllocator, other.d_recoveryCb)
, d_oldSyncPoint(other.d_oldSyncPoint)
, d_oldSyncPointOffset(other.d_oldSyncPointOffset)
, d_newSyncPoint(other.d_newSyncPoint)
, d_newSyncPointOffset(other.d_newSyncPointOffset)
, d_journalFileOffset(other.d_journalFileOffset)
, d_dataFileOffset(other.d_dataFileOffset)
, d_qlistFileOffset(other.d_qlistFileOffset)
, d_bufferedEvents(other.d_bufferedEvents, basicAllocator)
, d_inRecovery(other.d_inRecovery)
, d_recoveryPeer_p(other.d_recoveryPeer_p)
, d_responseType(other.d_responseType)
, d_expectedChunkFileType(other.d_expectedChunkFileType)
, d_lastChunkSequenceNumber(other.d_lastChunkSequenceNumber)
, d_recoveryStartupWaitHandle(other.d_recoveryStartupWaitHandle)
, d_recoveryStatusCheckHandle(other.d_recoveryStatusCheckHandle)
{
}

// MANIPULATORS
inline void RecoveryManager_RecoveryContext::setNumAttempts(int value)
{
    d_numAttempts = value;
}

inline mqbs::MappedFileDescriptor& RecoveryManager_RecoveryContext::journalFd()
{
    return d_journalFd;
}

inline mqbs::MappedFileDescriptor& RecoveryManager_RecoveryContext::dataFd()
{
    return d_dataFd;
}

inline mqbs::MappedFileDescriptor& RecoveryManager_RecoveryContext::qlistFd()
{
    return d_qlistFd;
}

inline mqbs::FileStoreSet& RecoveryManager_RecoveryContext::fileSet()
{
    return d_fileSet;
}

inline void RecoveryManager_RecoveryContext::setRecoveryStatus(bool value)
{
    d_inRecovery = value;
}

inline void
RecoveryManager_RecoveryContext::setRecoveryPeer(mqbnet::ClusterNode* node)
{
    d_recoveryPeer_p = node;
}

inline void RecoveryManager_RecoveryContext::setResponseType(
    bmqp_ctrlmsg::StorageSyncResponseType::Value value)
{
    d_responseType = value;
}

inline void RecoveryManager_RecoveryContext::setExpectedChunkFileType(
    bmqp::RecoveryFileChunkType::Enum value)
{
    d_expectedChunkFileType = value;
}

inline void
RecoveryManager_RecoveryContext::setLastChunkSequenceNumber(unsigned int value)
{
    d_lastChunkSequenceNumber = value;
}

inline bdlmt::EventScheduler::EventHandle&
RecoveryManager_RecoveryContext::recoveryStartupWaitHandle()
{
    return d_recoveryStartupWaitHandle;
}

inline bdlmt::EventScheduler::EventHandle&
RecoveryManager_RecoveryContext::recoveryStatusCheckHandle()
{
    return d_recoveryStatusCheckHandle;
}

inline void RecoveryManager_RecoveryContext::setRecoveryCb(
    const PartitionRecoveryCb& recoveryCb)
{
    d_recoveryCb = recoveryCb;
}

inline void RecoveryManager_RecoveryContext::setOldSyncPoint(
    const bmqp_ctrlmsg::SyncPoint& syncPoint)
{
    d_oldSyncPoint = syncPoint;
}

inline void RecoveryManager_RecoveryContext::setOldSyncPointOffset(
    bsls::Types::Uint64 offset)
{
    d_oldSyncPointOffset = offset;
}

inline void RecoveryManager_RecoveryContext::setNewSyncPoint(
    const bmqp_ctrlmsg::SyncPoint& syncPoint)
{
    d_newSyncPoint = syncPoint;
}

inline void RecoveryManager_RecoveryContext::setNewSyncPointOffset(
    bsls::Types::Uint64 offset)
{
    d_newSyncPointOffset = offset;
}

inline void RecoveryManager_RecoveryContext::setJournalFileOffset(
    bsls::Types::Uint64 offset)
{
    d_journalFileOffset = offset;
}

inline void
RecoveryManager_RecoveryContext::setDataFileOffset(bsls::Types::Uint64 offset)
{
    d_dataFileOffset = offset;
}

inline void
RecoveryManager_RecoveryContext::setQlistFileOffset(bsls::Types::Uint64 offset)
{
    d_qlistFileOffset = offset;
}

inline void RecoveryManager_RecoveryContext::addStorageEvent(
    const bsl::shared_ptr<bdlbb::Blob>& event)
{
    d_bufferedEvents.push_back(event);
}

inline void RecoveryManager_RecoveryContext::purgeStorageEvents()
{
    d_bufferedEvents.clear();
}

// ACCESSORS
inline int RecoveryManager_RecoveryContext::numAttempts() const
{
    return d_numAttempts;
}

inline const mqbs::MappedFileDescriptor&
RecoveryManager_RecoveryContext::journalFd() const
{
    return d_journalFd;
}

inline const mqbs::MappedFileDescriptor&
RecoveryManager_RecoveryContext::dataFd() const
{
    return d_dataFd;
}

inline const mqbs::MappedFileDescriptor&
RecoveryManager_RecoveryContext::qlistFd() const
{
    return d_qlistFd;
}

inline const mqbs::FileStoreSet&
RecoveryManager_RecoveryContext::fileSet() const
{
    return d_fileSet;
}

inline const RecoveryManager_RecoveryContext::PartitionRecoveryCb&
RecoveryManager_RecoveryContext::recoveryCb() const
{
    return d_recoveryCb;
}

inline const bmqp_ctrlmsg::SyncPoint&
RecoveryManager_RecoveryContext::oldSyncPoint() const
{
    return d_oldSyncPoint;
}

inline bsls::Types::Uint64
RecoveryManager_RecoveryContext::oldSyncPointOffset() const
{
    return d_oldSyncPointOffset;
}

inline const bmqp_ctrlmsg::SyncPoint&
RecoveryManager_RecoveryContext::newSyncPoint() const
{
    return d_newSyncPoint;
}

inline bsls::Types::Uint64
RecoveryManager_RecoveryContext::newSyncPointOffset() const
{
    return d_newSyncPointOffset;
}

inline bsls::Types::Uint64
RecoveryManager_RecoveryContext::journalFileOffset() const
{
    return d_journalFileOffset;
}

inline bsls::Types::Uint64
RecoveryManager_RecoveryContext::dataFileOffset() const
{
    return d_dataFileOffset;
}

inline bsls::Types::Uint64
RecoveryManager_RecoveryContext::qlistFileOffset() const
{
    return d_qlistFileOffset;
}

inline bool RecoveryManager_RecoveryContext::inRecovery() const
{
    return d_inRecovery;
}

inline mqbnet::ClusterNode*
RecoveryManager_RecoveryContext::recoveryPeer() const
{
    return d_recoveryPeer_p;
}

inline bmqp_ctrlmsg::StorageSyncResponseType::Value
RecoveryManager_RecoveryContext::responseType() const
{
    return d_responseType;
}

inline bmqp::RecoveryFileChunkType::Enum
RecoveryManager_RecoveryContext::expectedChunkFileType() const
{
    return d_expectedChunkFileType;
}

inline unsigned int
RecoveryManager_RecoveryContext::lastChunkSequenceNumber() const
{
    return d_lastChunkSequenceNumber;
}

inline const bsl::vector<bsl::shared_ptr<bdlbb::Blob> >&
RecoveryManager_RecoveryContext::storageEvents() const
{
    return d_bufferedEvents;
}

// ============================================================
// class RecoveryManager_PrimarySyncContext::PeerPartitionState
// ============================================================

// CREATORS
inline RecoveryManager_PrimarySyncContext::PeerPartitionState::
    PeerPartitionState()
: d_peer_p(0)
, d_partitionSeqNum()
, d_lastSyncPointOffsetPair()
, d_needsPartitionSync(false)
{
    d_partitionSeqNum.primaryLeaseId() = 0;
    d_partitionSeqNum.sequenceNumber() = 0;
}

inline RecoveryManager_PrimarySyncContext::PeerPartitionState::
    PeerPartitionState(mqbnet::ClusterNode*                         peer,
                       const bmqp_ctrlmsg::PartitionSequenceNumber& seqNum,
                       const bmqp_ctrlmsg::SyncPointOffsetPair& spOffsetPair,
                       bool needsPartitionSync)
: d_peer_p(peer)
, d_partitionSeqNum(seqNum)
, d_lastSyncPointOffsetPair(spOffsetPair)
, d_needsPartitionSync(needsPartitionSync)
{
}

// MANIPULATORS
inline void RecoveryManager_PrimarySyncContext::PeerPartitionState::setPeer(
    mqbnet::ClusterNode* value)
{
    d_peer_p = value;
}

inline void RecoveryManager_PrimarySyncContext::PeerPartitionState::
    setPartitionSequenceNum(const bmqp_ctrlmsg::PartitionSequenceNumber& value)
{
    d_partitionSeqNum = value;
}

inline void RecoveryManager_PrimarySyncContext::PeerPartitionState::
    setLastSyncPointOffsetPair(const bmqp_ctrlmsg::SyncPointOffsetPair& value)
{
    d_lastSyncPointOffsetPair = value;
}

inline void
RecoveryManager_PrimarySyncContext::PeerPartitionState::setNeedsPartitionSync(
    bool value)
{
    d_needsPartitionSync = value;
}

// ACCESSORS
inline mqbnet::ClusterNode*
RecoveryManager_PrimarySyncContext::PeerPartitionState::peer() const
{
    return d_peer_p;
}

inline const bmqp_ctrlmsg::PartitionSequenceNumber&
RecoveryManager_PrimarySyncContext::PeerPartitionState::partitionSequenceNum()
    const
{
    return d_partitionSeqNum;
}

inline const bmqp_ctrlmsg::SyncPointOffsetPair&
RecoveryManager_PrimarySyncContext::PeerPartitionState::
    lastSyncPointOffsetPair() const
{
    return d_lastSyncPointOffsetPair;
}

inline bool
RecoveryManager_PrimarySyncContext::PeerPartitionState::needsPartitionSync()
    const
{
    return d_needsPartitionSync;
}

// ---------------------------------------
// class RecoveryManager_PrimarySyncContext
// ---------------------------------------

// CREATORS
inline RecoveryManager_PrimarySyncContext::RecoveryManager_PrimarySyncContext(
    bslma::Allocator* basicAllocator)
: d_rm_p(0)
, d_fs_p(0)
, d_syncInProgress(false)
, d_syncPeer_p(0)
, d_primarySyncCb(bsl::allocator_arg, basicAllocator)
, d_selfPartitionSeqNum()
, d_selfLastSyncPtOffsetPair()
, d_fileTransferInfo()
, d_peerPartitionStates(basicAllocator)
, d_syncStatusEventHandle()
{
    d_selfPartitionSeqNum.primaryLeaseId() = 0;
    d_selfPartitionSeqNum.sequenceNumber() = 0;
}

inline RecoveryManager_PrimarySyncContext::RecoveryManager_PrimarySyncContext(
    const RecoveryManager_PrimarySyncContext& other,
    bslma::Allocator*                         basicAllocator)
: d_rm_p(other.d_rm_p)
, d_fs_p(other.d_fs_p)
, d_syncInProgress(other.d_syncInProgress)
, d_syncPeer_p(other.d_syncPeer_p)
, d_primarySyncCb(bsl::allocator_arg, basicAllocator, other.d_primarySyncCb)
, d_selfPartitionSeqNum(other.d_selfPartitionSeqNum)
, d_selfLastSyncPtOffsetPair(other.d_selfLastSyncPtOffsetPair)
, d_fileTransferInfo(other.d_fileTransferInfo)
, d_peerPartitionStates(other.d_peerPartitionStates, basicAllocator)
, d_syncStatusEventHandle(other.d_syncStatusEventHandle)
{
}

// MANIPULATORS
inline void
RecoveryManager_PrimarySyncContext::setRecoveryManager(RecoveryManager* value)
{
    d_rm_p = value;
}

inline void
RecoveryManager_PrimarySyncContext::setFileStore(const mqbs::FileStore* value)
{
    d_fs_p = value;
}

inline void
RecoveryManager_PrimarySyncContext::setPrimarySyncInProgress(bool value)
{
    d_syncInProgress = value;
}

inline void RecoveryManager_PrimarySyncContext::setPrimarySyncPeer(
    mqbnet::ClusterNode* node)
{
    d_syncPeer_p = node;
}

inline void RecoveryManager_PrimarySyncContext::setPartitionPrimarySyncCb(
    const PartitionPrimarySyncCb& value)
{
    d_primarySyncCb = value;
}

inline void RecoveryManager_PrimarySyncContext::setSelfPartitionSequenceNum(
    const bmqp_ctrlmsg::PartitionSequenceNumber& value)
{
    d_selfPartitionSeqNum = value;
}

inline void RecoveryManager_PrimarySyncContext::setSelfLastSyncPtOffsetPair(
    const bmqp_ctrlmsg::SyncPointOffsetPair& value)
{
    d_selfLastSyncPtOffsetPair = value;
}

inline RecoveryManager_PrimarySyncContext::FileTransferInfo&
RecoveryManager_PrimarySyncContext::fileTransferInfo()
{
    return d_fileTransferInfo;
}

inline RecoveryManager_PrimarySyncContext::PeerPartitionStates&
RecoveryManager_PrimarySyncContext::peerPartitionStates()
{
    return d_peerPartitionStates;
}

inline bdlmt::EventScheduler::EventHandle&
RecoveryManager_PrimarySyncContext::primarySyncStatusEventHandle()
{
    return d_syncStatusEventHandle;
}

// ACCESSORS
inline RecoveryManager*
RecoveryManager_PrimarySyncContext::recoveryManager() const
{
    return d_rm_p;
}

inline const mqbs::FileStore*
RecoveryManager_PrimarySyncContext::fileStore() const
{
    return d_fs_p;
}

inline bool RecoveryManager_PrimarySyncContext::primarySyncInProgress() const
{
    return d_syncInProgress;
}

inline mqbnet::ClusterNode*
RecoveryManager_PrimarySyncContext::syncPeer() const
{
    return d_syncPeer_p;
}

inline const RecoveryManager_PrimarySyncContext::PartitionPrimarySyncCb&
RecoveryManager_PrimarySyncContext::partitionPrimarySyncCb() const
{
    return d_primarySyncCb;
}

inline const bmqp_ctrlmsg::PartitionSequenceNumber&
RecoveryManager_PrimarySyncContext::selfPartitionSequenceNum() const
{
    return d_selfPartitionSeqNum;
}

inline const bmqp_ctrlmsg::SyncPointOffsetPair&
RecoveryManager_PrimarySyncContext::selfLastSyncPtOffsetPair() const
{
    return d_selfLastSyncPtOffsetPair;
}

inline const RecoveryManager_PrimarySyncContext::FileTransferInfo&
RecoveryManager_PrimarySyncContext::fileTransferInfo() const
{
    return d_fileTransferInfo;
}

inline const RecoveryManager_PrimarySyncContext::PeerPartitionStates&
RecoveryManager_PrimarySyncContext::peerPartitionStates() const
{
    return d_peerPartitionStates;
}

// ------------------------------------
// class RecoveryManager_RequestContext
// ------------------------------------

// CREATORS
inline RecoveryManager_RequestContext::RecoveryManager_RequestContext()
: d_contextType(RequestContextType::e_UNDEFINED)
, d_requester_p(0)
, d_partitionId(mqbs::DataStore::k_INVALID_PARTITION_ID)
, d_recoveryManager_p(0)
{
}

inline void
RecoveryManager_RequestContext::setContextType(RequestContextType::Enum value)
{
    BSLS_ASSERT_SAFE(RequestContextType::e_UNDEFINED != value);
    d_contextType = value;
}

inline void
RecoveryManager_RequestContext::setRequesterNode(mqbnet::ClusterNode* value)
{
    BSLS_ASSERT_SAFE(value);
    d_requester_p = value;
}

inline void RecoveryManager_RequestContext::setPartitionId(int partitionId)
{
    BSLS_ASSERT_SAFE(mqbs::DataStore::k_INVALID_PARTITION_ID != partitionId);
    d_partitionId = partitionId;
}

inline void
RecoveryManager_RequestContext::setRecoveryManager(RecoveryManager* value)
{
    BSLS_ASSERT_SAFE(value);
    d_recoveryManager_p = value;
}

inline RecoveryManager_RequestContext::FileTransferInfo&
RecoveryManager_RequestContext::fileTransferInfo()
{
    return d_fileTransferInfo;
}

// ACCESSORS
inline RecoveryManager_RequestContextType::Enum
RecoveryManager_RequestContext::contextType() const
{
    return d_contextType;
}

inline mqbnet::ClusterNode*
RecoveryManager_RequestContext::requesterNode() const
{
    return d_requester_p;
}

inline int RecoveryManager_RequestContext::partitionId() const
{
    return d_partitionId;
}

inline RecoveryManager* RecoveryManager_RequestContext::recoveryManager() const
{
    return d_recoveryManager_p;
}

inline const RecoveryManager_RequestContext::FileTransferInfo&
RecoveryManager_RequestContext::fileTransferInfo() const
{
    return d_fileTransferInfo;
}

// ----------------------------------
// class RecoveryManager_ChunkDeleter
// ----------------------------------

// CREATORS
inline RecoveryManager_ChunkDeleter::RecoveryManager_ChunkDeleter(
    RequestContext* requestContext)
: d_requestContext_p(requestContext)
, d_primarySyncContext_p(0)
{
}

inline RecoveryManager_ChunkDeleter::RecoveryManager_ChunkDeleter(
    PrimarySyncContext* primarySyncContext)
: d_requestContext_p(0)
, d_primarySyncContext_p(primarySyncContext)
{
}

// ---------------------
// class RecoveryManager
// ---------------------

// ACCESSORS
inline bool RecoveryManager::isStarted() const
{
    return d_isStarted;
}

inline bool RecoveryManager::isRecoveryInProgress(int partitionId) const
{
    return d_recoveryContexts[partitionId].inRecovery();
}

inline mqbnet::ClusterNode*
RecoveryManager::recoveryPeer(int partitionId) const
{
    return d_recoveryContexts[partitionId].recoveryPeer();
}

inline bool RecoveryManager::isPrimarySyncInProgress(int partitionId) const
{
    return d_primarySyncContexts[partitionId].primarySyncInProgress();
}

inline mqbnet::ClusterNode*
RecoveryManager::primarySyncPeer(int partitionId) const
{
    return d_primarySyncContexts[partitionId].syncPeer();
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream&
mqbblp::operator<<(bsl::ostream&                                    stream,
                   mqbblp::RecoveryManager_RequestContextType::Enum value)
{
    return mqbblp::RecoveryManager_RequestContextType::print(stream,
                                                             value,
                                                             0,
                                                             -1);
}

}  // close enterprise namespace

#endif
