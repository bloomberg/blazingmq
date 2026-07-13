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

#ifndef INCLUDED_MQBS_FILESTORE
#define INCLUDED_MQBS_FILESTORE

//@PURPOSE: Provide a file-backed BlazingMQ data store.
//
//@CLASSES:
//  mqbs::FileStore:         File-backed BlazingMQ data store.
//
//@SEE ALSO: mqbs::FileStoreProtocol
//
//@DESCRIPTION: This component provides a mechanism, 'mqbs:FileStore', to store
// BlazingMQ data in files on disk.  An instance of 'mqbs::FileStore' is also
// referred to as a partition.  A partition has a valid partitionId associated
// with it.  A partition also has a list of file sets where the file set at
// index 0 is the current one. When rollover occurs, the outstanding messages
// are moved from the active file set to a new rollover file set, which is
// then inserted to the front of the list.

// MQB

#include <mqbi_dispatcher.h>
#include <mqbnet_cluster.h>
#include <mqbnet_controlmessagetransmitter.h>
#include <mqbs_datastore.h>
#include <mqbs_fileset.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbs_storagecollectionutil.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_storageeventbuilder.h>
#include <bmqt_messageguid.h>
#include <bmqt_uri.h>

#include <bmqc_orderedhashmap.h>
#include <bmqma_countingallocatorstore.h>
#include <bmqu_atomicstate.h>
#include <bmqu_blob.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlcc_objectpool.h>
#include <bdlcc_sharedobjectpool.h>
#include <bdlmt_eventscheduler.h>
#include <bdlmt_fixedthreadpool.h>
#include <bdlmt_throttle.h>
#include <bsl_deque.h>
#include <bsl_functional.h>
#include <bsl_limits.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslh_hash.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_cpp11.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATIONS
namespace mqbcmd {
class FileStore;
class PurgeQueueResult;
}
namespace mqbi {
class Dispatcher;
}
namespace mqbi {
class Domain;
}
namespace mqbstat {
class PartitionStats;
}

namespace mqbs {

// FORWARD DECLARATIONS
class DataFileIterator;
class FileStore;
class FileStoreSet;
class JournalFileIterator;
class QlistFileIterator;
class ReplicatedStorage;

// ===============
// class FileStore
// ===============

/// Mechanism to store BlazingMQ messages in file. This component is
/// *const-thread* safe.
class FileStore BSLS_KEYWORD_FINAL : public DataStore {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBS.FILESTORE");

    // FRIENDS
    friend class FileStoreIterator;

  public:
    // TYPES

    typedef bmqp::BlobPoolUtil::BlobSpPool BlobSpPool;

    /// Pool of shared pointers to AtomicStates
    typedef bdlcc::SharedObjectPool<
        bmqu::AtomicState,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::Reset<bmqu::AtomicState> >
        StateSpPool;

    /// It is important for this to be a deque instead of a vector, because
    /// during recovery at startup, we insert recoverd SyncPts at the
    /// beginning of the container, which can be slow for a vector (plenty
    /// of reallocations and memmoves), specially if there are a large
    /// number of SyncPts.  A `std::list` can't be used because certain
    /// recovery logic carries out binary search on this container.
    typedef bsl::deque<bmqp_ctrlmsg::SyncPointOffsetPair> SyncPointOffsetPairs;

    typedef SyncPointOffsetPairs::const_iterator SyncPointOffsetConstIter;

    /// VST holding all parameters for the Raft primary message write path.
    /// Input fields are set by 'PartitionRaft' before calling
    /// 'formatMessageRecord'; output fields are filled in by
    /// 'formatMessageRecord'.
    struct PendingWrite {
        // INPUT — common
        bsls::Types::Uint64              d_id;
        RecordType::Enum                 d_recordType;
        SyncPointType::Enum              d_syncPointType;  // e_JOURNAL_OP only
        bsls::Types::Uint64              d_primaryLeaseId;
        bsls::Types::Uint64              d_sequenceNumber;
        mqbu::StorageKey                 d_queueKey;

        // INPUT — e_MESSAGE
        mqbi::StorageMessageAttributes d_attributes;

        DataStoreRecordHandle            d_handle;
        bmqt::MessageGUID                d_guid;
        bsl::shared_ptr<bdlbb::Blob>     d_appData;
        bsl::shared_ptr<bdlbb::Blob>     d_options;

        // INPUT — e_QUEUE_OP (creation)
        bmqt::Uri                        d_queueUri;
        const AppInfos*                  d_appIdKeyPairs_p;
        bsls::Types::Uint64              d_timestamp;
        bool                             d_isNewQueue;

        // INPUT — e_CONFIRM / e_DELETION / e_QUEUE_OP (purge, deletion)
        mqbu::StorageKey         d_appKey;
        ConfirmReason::Enum      d_confirmReason;        // e_CONFIRM
        DeletionRecordFlag::Enum d_deletionFlag;         // e_DELETION
        QueueOpType::Enum        d_queueOpType;          // e_QUEUE_OP
        unsigned int             d_startPrimaryLeaseId;  // e_PURGE
        bsls::Types::Uint64      d_startSequenceNumber;  // e_PURGE

        // OUTPUT (set by formatMessageRecord / formatQueueCreationRecord)
        bsls::Types::Uint64              d_journalOffset;
        bsls::Types::Uint64              d_dataOffset;
        bsl::shared_ptr<bdlbb::Blob>     d_entryBlob;
        bsls::Types::Uint64              d_qlistOffset;
        unsigned int                     d_qlistRecTotalLength;

        PendingWrite();

        /// Message record constructor.
        PendingWrite(mqbi::StorageMessageAttributes*     attributes,
                     const bmqt::MessageGUID&            guid,
                     const bsl::shared_ptr<bdlbb::Blob>& appData,
                     const bsl::shared_ptr<bdlbb::Blob>& options,
                     const mqbu::StorageKey&             queueKey);

        /// Queue creation record constructor.
        PendingWrite(const bmqt::Uri&        queueUri,
                     const mqbu::StorageKey& queueKey,
                     const AppInfos*         appIdKeyPairs,
                     bsls::Types::Uint64     timestamp,
                     bool                    isNewQueue);

        /// Sync-point record constructor.  'd_primaryLeaseId' (term) and
        /// 'd_sequenceNumber' (index) are filled in by
        /// 'PartitionRaftLog::append'.
        explicit PendingWrite(SyncPointType::Enum syncPointType);

        /// Confirm record constructor.
        PendingWrite(const bmqt::MessageGUID& guid,
                     const mqbu::StorageKey&  queueKey,
                     const mqbu::StorageKey&  appKey,
                     bsls::Types::Uint64      timestamp,
                     ConfirmReason::Enum      reason);

        /// Message-deletion record constructor.
        PendingWrite(const bmqt::MessageGUID& guid,
                     const mqbu::StorageKey&  queueKey,
                     DeletionRecordFlag::Enum deletionFlag,
                     bsls::Types::Uint64      timestamp);

        /// Queue-op (purge/deletion) record constructor.  For a purge,
        /// 'startPrimaryLeaseId' / 'startSequenceNumber' identify the start
        /// position; for a deletion they are 0.
        PendingWrite(QueueOpType::Enum       queueOpType,
                     const mqbu::StorageKey& queueKey,
                     const mqbu::StorageKey& appKey,
                     bsls::Types::Uint64     timestamp,
                     unsigned int            startPrimaryLeaseId,
                     bsls::Types::Uint64     startSequenceNumber);

        /// Return this object to a clean default state, releasing any owned
        /// blobs/attributes/handle and zeroing scalars.  Required by the
        /// `bdlcc::SharedObjectPool` `Reset` functor so a pooled object is
        /// clean when reused.
        void reset();
    };

    typedef bsl::shared_ptr<FileSet> FileSetSp;
    typedef bsl::vector<FileSetSp>   FileSets;

    typedef StorageCollectionUtil::StorageList StorageList;

    typedef bsl::vector<StorageCollectionUtil::StorageFilter> StorageFilters;
    typedef StorageFilters::const_iterator StorageFiltersconstIter;

    /// Map of primaryLeaseId -> highest sequence number
    typedef bsl::unordered_map<unsigned int, bsls::Types::Uint64>
                                               LeaseIdToSeqNumMap;
    typedef LeaseIdToSeqNumMap::const_iterator LeaseIdToSeqNumMapCIter;

    /// Counter for the number of messages and bytes in a queue.  Public
    /// because 'writeRolledOverRecord'/'writeRolledOverRecords' (used by
    /// the Raft rollover orchestration in 'PartitionRaftLog') take it.
    typedef bsl::pair<unsigned int, bsls::Types::Uint64> MessageByteCounter;

    typedef bsl::unordered_map<mqbu::StorageKey,
                               MessageByteCounter,
                               bslh::Hash<mqbu::StorageKeyHashAlgo> >
                                               QueueKeyCounterMap;
    typedef QueueKeyCounterMap::iterator       QueueKeyCounterMapIter;
    typedef QueueKeyCounterMap::const_iterator QueueKeyCounterMapCIter;

    typedef bsl::vector<bsl::pair<mqbu::StorageKey, MessageByteCounter> >
                                                QueueKeyCounterList;
    typedef QueueKeyCounterList::const_iterator QueueKeyCounterListCIter;

  private:
    // PRIVATE TYPES
    typedef DataStoreConfig::Records             Records;
    typedef DataStoreConfig::RecordIterator      RecordIterator;
    typedef DataStoreConfig::RecordConstIterator RecordConstIterator;

    typedef bsl::pair<RecordIterator, bool> InsertRc;

    typedef bdlmt::EventScheduler::RecurringEventHandle RecurringEventHandle;

    typedef DataStoreConfig::QueueKeyInfoMapConstIter QueueKeyInfoMapConstIter;
    typedef DataStoreConfig::QueueKeyInfoMapInsertRc  QueueKeyInfoMapInsertRc;

    typedef mqbi::Storage::AppInfos AppInfos;

    typedef StorageCollectionUtil::StoragesMap         StoragesMap;
    typedef StorageCollectionUtil::StorageMapIter      StorageMapIter;
    typedef StorageCollectionUtil::StorageMapConstIter StorageMapConstIter;

    /// This context we keep for un-receipted messages.
    struct ReceiptContext {
        const mqbu::StorageKey  d_queueKey;
        const bmqt::MessageGUID d_guid;
        const RecordIterator    d_handle;
        mqbi::QueueHandle*      d_qH;
        int                     d_count;  // Primary initializes this as
                                          // zero and increments for each
                                          // Receipt. When it reaches
                                          // 'd_replicationFactor', the
                                          // Receipt'ed messages are
                                          // strong consistent.

        ReceiptContext(const mqbu::StorageKey&  queueKey,
                       const bmqt::MessageGUID& guid,
                       const RecordIterator&    handle,
                       int                      count,
                       mqbi::QueueHandle*       qH);
    };

    struct NodeContext {
        /// Last Receipt from/to this node (Replica/Primary).
        DataStoreRecordKey d_key;

        /// Receipt to this node.
        bsl::shared_ptr<bdlbb::Blob> d_blob_sp;

        bsl::shared_ptr<bmqu::AtomicState> d_state;

        NodeContext(BlobSpPool* blobSpPool_p, const DataStoreRecordKey& key);
    };
    typedef bmqc::OrderedHashMap<DataStoreRecordKey,
                                 ReceiptContext,
                                 DataStoreRecordKeyHashAlgo>
        Unreceipted;

    /// Map of NodeId -> NodeContext to assist in Receipt processing
    typedef bsl::unordered_map<int, NodeContext> NodeReceiptContexts;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    bmqma::CountingAllocatorStore d_allocators;
    // Allocator store to spawn new
    // allocators for sub-components

    bmqma::CountingAllocatorStore d_storageAllocatorStore;
    // Allocator store to pass to all the
    // queue storages (file-backed or
    // in-memory) assigned to this
    // FileStore instance.  We don't want
    // every queue storage object to
    // maintain its own allocator store as
    // it pollutes the stat allocator
    // table output with multiple rows for
    // every storage object.

    DataStoreConfig d_config;
    // Configuration of this instance
    // provided upon construction

    bsl::string d_partitionDescription;
    // Brief description for logging

    mqbi::DispatcherClientData d_dispatcherClientData;
    // Dispatcher client data associated
    // with this instance.

    bsl::shared_ptr<mqbstat::PartitionStats> d_partitionStats_sp;
    // Stat object associated to the
    // Partition this FileStore belongs to,
    // used to report partition level
    // metrics.

    mutable BlobSpPool* d_blobSpPool_p;
    // Pool of shared pointers to blobs to
    // use.

    StateSpPool* d_statePool_p;

    bsls::AtomicBool d_isOpen;
    // Flag to indicate open/close status
    // of this instance.

    bool d_isStopping;
    // Flag to indicate if self node is
    // stopping.

    bool d_flushWhenClosing;
    // Flag to indicate if flush when this
    // instance is closing.

    bool d_lastSyncPtReceived;
    // Flag to indicate if self is
    // stopping and has received the
    // "last" SyncPt, and must not process
    // any further storage events.

    Records d_records;
    // List of outstanding records, in
    // order of arrival

    Unreceipted d_unreceipted;
    // Ordered list of records pending
    // Receipt.

    /// For weak consistency only.
    /// The container that holds keys to storages where we put messages since
    /// the last storage event builder flush.  Used to notify these queues on
    /// replication complete, so they change from processing PUTs to PUSHes.
    bsl::unordered_set<mqbu::StorageKey> d_replicationNotifications;

    int d_replicationFactor;

    NodeReceiptContexts d_nodes;

    DataStoreRecordKey d_lastRecoveredStrongConsistency;

    FileSets d_fileSets;
    // List of file sets.  File set at
    // index 0 is the current one.  Rest
    // are the ones which are still being
    // referenced by message records.
    // When rollover occurs, the
    // outstanding messages are moved from
    // the active file set to a new
    // rollover file set, which is then
    // inserted to the front of the list.

    mqbnet::Cluster* d_cluster_p;

    bdlmt::FixedThreadPool* d_miscWorkThreadPool_p;
    // Thread pool used for any standalone
    // work that can be offloaded to
    // non-partition-dispatcher threads.

    RecurringEventHandle d_syncPointEventHandle;

    RecurringEventHandle d_partitionHighwatermarkEventHandle;

    bool d_isPrimary;

    mqbnet::ClusterNode* d_primaryNode_p;

    /// Lease id of the current primary.  The current sequence number is
    /// always `d_highestSeqNums[d_primaryLeaseId]`.
    unsigned int d_primaryLeaseId;

    SyncPointOffsetPairs d_syncPoints;
    // List of (syncPoints, offset) pairs,
    // from oldest to newest

    StoragesMap d_storages;
    // Map [QueueKey->ReplicatedStorage*]

    StoragesMonitor* d_storagesMonitor_p;

    bdlmt::Throttle d_alarmSoftLimiter;
    // Throttler for alarming on soft
    // limits of partition files

    bool d_isFSMWorkflow;
    // Whether FSM workflow is enabled

    bool d_qListAware;
    // Whether the broker still reads and writes to the to-be-deprecated Qlist
    // file.

    bmqp::StorageEventBuilder d_storageEventBuilder;
    // Storage event builder to use.

    bmqp_ctrlmsg::PartitionSequenceNumber d_firstSyncPointAfterRolloverSeqNum;
    // First sync point after rollover sequence number, it is set at the last
    // step of rollover, together with journal file header
    // `firstSyncPointOffsetWords`. It is used to determine if cluster node
    // missed rollover.

    /// Map of primaryLeaseId -> highest sequence number observed, initialized
    /// during recovery, namely `recoverMessages`.  This map contains the
    /// highest sequence number for all primary leases including the current
    /// one.
    LeaseIdToSeqNumMap d_highestSeqNums;

    /// Control message transmitter to use.
    mqbnet::ControlMessageTransmitter d_messageTransmitter;

  private:
    // NOT IMPLEMENTED
    FileStore(const FileStore&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    FileStore& operator=(const FileStore&) BSLS_CPP11_DELETED;

  private:
    // PRIVATE MANIPULATORS

    /// Return a mutable reference to the current sequence number entry in
    /// `d_highestSeqNums`, i.e., `d_highestSeqNums[d_primaryLeaseId]`.
    /// Note that this will insert a zero entry if one does not exist.
    bsls::Types::Uint64& currentSeqNumRef();

    /// Create all the relevant files names, open them for writing and
    /// populate the specified `fileSetSp` with relevant information.
    /// Return zero on success and non-zero value otherwise.  Note that all
    /// current values in the `fileSet` (if any) are overwritten, and no
    /// attempt is made to close any valid files which `fileSet` may be
    /// holding.  Note that BlazingMQ header and file-specific header are
    /// written to the newly created files.
    int create(FileSetSp* fileSetSp);

    /// Truncate the files contained in the specified `fileSet` to their
    /// current sizes.  Note that files are not closed.
    void truncate(FileSet* fileSet);

    /// Optionally flush, unmap and close all files contained in the
    /// specified `fileSetRef`.  Note that files are *not* truncated.  Also
    /// note that no data is written to any file in this method.  This
    /// method will be typically preceded by an invocation to `truncate()`.
    /// Note that choice of a reference instead of a pointer for
    /// `fileSetRef` is deliberate, in order to avoid the accidental
    /// invocation of other flavor of `close`.  Return 0 on success and
    /// non-zero rc on failure.
    int close(FileSet& fileSetRef, bool flush);

    /// Move all files contained in the specified `fileSet` to the archive
    /// location as specified in this instance's configuration provided at
    /// construction.  Note that files are not truncated or closed.  Return 0
    /// on success and non-zero rc on failure.
    int archive(FileSet* fileSet);

    /// Garbage-collect the specified `fileSet` by calling a standalone
    /// worker thread to close and archive all files in the `fileSet`.
    ///
    /// THREAD: This method can be invoked from *any* thread.
    void gc(FileSet* fileSet);

    /// Garbage-collect the specified `fileSet` belonging to the specified
    /// `partitionId` by calling a standalone worker thread to close and
    /// archive all files in the `fileSet`.
    ///
    /// THREAD: This method should only be invoked by the partition
    /// *dispatcher* thread.
    void gcDispatched(int partitionId, FileSet* fileSet);

    /// Garbage-collect the specified `fileSet` by closing and archiving all
    /// files in the `fileSet`.
    ///
    /// THREAD: This method is invoked in a thread from the miscellaneous
    /// *worker* thread pool.
    void gcWorkerDispatched(const bsl::shared_ptr<FileSet>& fileSet);

    /// Open this instance in non-recovery mode.  Return zero on success and
    /// a non-zero value otherwise.  Note that this routine can be used in
    /// recovery mode when there are no files to recover messages from.
    int openInNonRecoveryMode();

    /// Open this instance in recovery mode using the specified
    /// `queueKeyInfoMap`.  Return zero on success and a non-zero value
    /// otherwise.  Note that return value of `1` indicates no files were
    /// found to recover messages from.  All other failure conditions should
    /// be treated as fatal.  Also note that file store will attempt to
    /// recover any outstanding messages from the files found at the
    /// location indicated by the configuration of this instance.
    int openInRecoveryMode(bsl::ostream&                   errorDescription,
                           QueueKeyInfoMap*                queueKeyInfoMap,
                           bsl::deque<RecoveryRecordInfo>* recoveryIndex = 0);

    /// Make two passes over the journal file iterator `jit` in reverse
    /// iteration.
    ///
    /// - First pass: Retrieve the list of non-deleted queues from `jit`.  If
    /// the specified `withCSL` is `false`, populate `queueKeyInfoMap` with
    /// that list; otherwise, use information from `queueKeyInfoMap` already
    /// populated by the CSL to validate against that list.
    ///
    /// - Second pass: Iterate over jit`, `dit`, and optionally `qit` if
    /// `d_qListAware` is true, and retrieve outstanding records for all those
    /// non-deleted queues in `queueKeyInfoMap` to populate into `d_records`.
    /// Moreover, update `journalOffset`, `dataOffset` and `qlistOffset` to the
    /// end of the journal, data and qlist files respectively.  Also, populate
    /// the map of primaryLeaseId to highest sequence number.
    ///
    /// Return zero on success, non zero value otherwise.  The behavior is
    /// undefined unless the journal iterator `jit` is in reverse mode.
    ///
    /// WARNING: This method invalidates all iterators.
    int recoverMessages(QueueKeyInfoMap*                queueKeyInfoMap,
                        bsls::Types::Uint64*            journalOffset,
                        bsls::Types::Uint64*            qlistOffset,
                        bsls::Types::Uint64*            dataOffset,
                        JournalFileIterator*            jit,
                        QlistFileIterator*              qit,
                        DataFileIterator*               dit,
                        bool                            withCSL,
                        bsl::deque<RecoveryRecordInfo>* recoveryIndex = 0);

    /// Rollover the outstanding messages belonging to the storages mapped
    /// to this file store, from active file set into the rollover file set,
    /// and make rolled over file set the new active file set.  Return zero
    /// on success, non-zero value otherwise.  Note that in its *current*
    /// implementation, this routine has no side-effect in case of failure.
    int rolloverImpl(bsls::Types::Uint64 timestamp);

    int rolloverIfNeeded(FileType::Enum           fileType,
                         const FileSet::FileInfo& fileInfo,
                         bsls::Types::Uint64      requestedSpace);

    /// Write a QUEUE_OP record to the journal with the specified
    /// `queueKey`, optional `appKey`, `timestamp`, `opValue` and `subValue`
    /// to the journal.  If the specified `startPrimaryLeaseId` and
    /// `startSequenceNum` are set, they specify the beginning of the range for
    /// which this QUEUE_OP applies; otherwise, the range includes all records.
    /// Return zero on success, non-zero value otherwise.
    int writeQueueOpRecord(DataStoreRecordHandle*  handle,
                           const mqbu::StorageKey& queueKey,
                           const mqbu::StorageKey& appKey,
                           QueueOpType::Enum       queueOpFlag,
                           bsls::Types::Uint64     timestamp,
                           unsigned int            startPrimaryLeaseId,
                           bsls::Types::Uint64     startSequenceNum);

    /// Write a QUEUE_OP record to the journal for the specified `queueKey`,
    /// optional `appKey`, `queueOpFlag`, `timestamp`,
    /// `startPrimaryLeaseId`, and `startSequenceNum`.  Performs the actual
    /// writing after all checks have been done by the caller.
    void writeQueueOpRecordImpl(DataStoreRecordHandle*  handle,
                                const mqbu::StorageKey& queueKey,
                                const mqbu::StorageKey& appKey,
                                QueueOpType::Enum       queueOpFlag,
                                bsls::Types::Uint64     timestamp,
                                unsigned int            startPrimaryLeaseId,
                                bsls::Types::Uint64     startSequenceNum);

    /// Issue a sync point.
    ///
    /// THREAD: This method is called from the scheduler thread.
    void issueSyncPointCb();

    /// Callback invoked to dispatch a function to alarm if any of the
    /// partition's files have reached the high watermark (soft limit) for
    /// outstanding bytes.
    ///
    /// THREAD: This method is called from the scheduler thread.
    void alarmHighwatermarkIfNeededCb();

    /// Alarm if any of the partition's files have reached the high
    /// watermark (soft limit) for outstanding bytes.
    ///
    /// THREAD: This method is called from the partition thread.
    void alarmHighwatermarkIfNeededDispatched();

    /// Issue a sync point.
    ///
    /// THREAD: This method executes in the partition dispatcher thread.
    void issueSyncPointDispatched(int partitionId);

    /// Issue a regular sync point if a new record has been published since
    /// the last one.
    void issueSyncPointIfNeeded();

    /// Write the specified `syncPoint` of the specified `type` to the
    /// journal, replicate it to followers, and record it in `d_syncPoints`.
    int issueSyncPointInternal(SyncPointType::Enum            type,
                               const bmqp_ctrlmsg::SyncPoint& syncPoint);

    int writeMessageRecord(const bmqp::StorageHeader&          header,
                           const mqbs::RecordHeader&           recHeader,
                           const bsl::shared_ptr<bdlbb::Blob>& event,
                           const bmqu::BlobPosition&           recordPosition);

    int writeQueueCreationRecord(const bmqp::StorageHeader&          header,
                                 const mqbs::RecordHeader&           recHeader,
                                 const bsl::shared_ptr<bdlbb::Blob>& event,
                                 const bmqu::BlobPosition& recordPosition);

    int writeJournalRecord(const bmqp::StorageHeader&          header,
                           const mqbs::RecordHeader&           recHeader,
                           const bsl::shared_ptr<bdlbb::Blob>& event,
                           const bmqu::BlobPosition&           recordPosition,
                           bmqp::StorageMessageType::Enum      messageType);

    /// Replicate the record of specified `type` starting at specified
    /// `journalOffset` in the JOURNAL file.  The behavior is undefined if
    /// `type` is `e_DATA` or `e_QLIST`.  Note that upon return of this
    /// method, it is guaranteed that a valid packet was created and
    /// enqueued to be sent to all peers; it does not mean that packet was
    /// successfully sent to any/all peers.
    void replicateRecord(bmqp::StorageMessageType::Enum type,
                         bsls::Types::Uint64            journalOffset,
                         bool immediateFlush = false);

    /// Replicate the record of specified `type` starting at specified
    /// `journalOffset` in the journal file, and having the associated DATA
    /// or QLIST block starting at the specified `dataOffset` and of the
    /// specified `totalDataLen` length.  Use the specified `flags` for the
    /// packet header.  The behavior is undefined unless `type` is `e_DATA`
    /// or `e_QLIST`.  Note that upon return of this method, it is
    /// guaranteed that a valid packet was created and enqueued to be sent
    /// to all peers; it does not mean that packet was successfully sent to
    /// any/all peers.
    void replicateRecord(bmqp::StorageMessageType::Enum type,
                         int                            flags,
                         bsls::Types::Uint64            journalOffset,
                         bsls::Types::Uint64            dataOffset,
                         unsigned int                   totalDataLen);

    /// Executed in the scheduler's dispatcher thread.
    void deleteArchiveFilesCb();

    /// Validate whether we can write and replicate a record, returning
    /// false if self node is stopping, the journal file is not available,
    /// or we are not the primary node, and true otherwise.
    int validateWritingRecord(const bmqt::MessageGUID& guid,
                              const mqbu::StorageKey&  queueKey);

    /// Nack (as UNKNOWN) message with the specified `recordKey` if it is
    /// still pending receipt of quorum Receipts.
    void cancelUnreceipted(const DataStoreRecordKey& recordKey);

    /// Generate Replication Receipt for the specified `node` confirming the
    /// receipt of message with the specified `primaryLeaseId` and
    /// `sequenceNumber`.  Store cumulative receipt in the specified
    /// `nodeContext`.
    NodeContext* generateReceipt(NodeContext*         nodeContext,
                                 mqbnet::ClusterNode* node,
                                 unsigned int         primaryLeaseId,
                                 bsls::Types::Uint64  sequenceNumber);

    /// Send previously generated Replication Receipt to the specified `node`
    /// using the specified `nodeContext`.
    void sendReceipt(mqbnet::ClusterNode* node, NodeContext* nodeContext);

    /// Generate and send Replication Receipt for the
    /// `d_lastRecoveredStrongConsistency`, if any, to the current primary.
    void sendImplicitReceipt();

    /// Insert the specified `record` value by the specified `key` into the
    /// list of outstanding records, and assign to the specified `handle` an
    /// iterator to the inserted record.
    void insertDataStoreRecord(RecordIterator*           recordIt,
                               const DataStoreRecordKey& key,
                               const DataStoreRecord&    record);
    void insertDataStoreRecord(DataStoreRecordHandle*    handle,
                               const DataStoreRecordKey& key,
                               const DataStoreRecord&    record);

    /// Bind the specified `record` (with all offsets filled in by a
    /// `format*Record`) to `d_records` under the specified `key`, updating
    /// the specified `pw`'s handle to identify it.  If `pw->d_handle` is
    /// already valid it refers to a *placeholder* record previously created
    /// by `reservePendingRecord` (Raft on-commit-rollover drain): update that
    /// entry in place (preserving its `d_hasReceipt`) rather than inserting a
    /// new one, so the handle already returned to the caller stays valid.
    /// Otherwise insert a fresh entry (the normal, non-buffered path).
    void bindOrUpdateRecord(PendingWrite*             pw,
                            const DataStoreRecordKey& key,
                            const DataStoreRecord&    record);

    const RecordIterator& handleTorRecordIterator(const DataStoreRecordHandle& handle) const;

    /// Replicate a record having the specified `messageType` and
    /// `recordOffset`, insert a corresponding record having the specified
    /// `recordType` and `recordOffset` into the list of outstanding
    /// records, and assign to the specified `handle` an iterator to the
    /// inserted record.
    void replicateAndInsertDataStoreRecord(
        DataStoreRecordHandle*         handle,
        bmqp::StorageMessageType::Enum messageType,
        RecordType::Enum               recordType,
        bsls::Types::Uint64            recordOffset);

    /// Flush the storage if the specified `immediateFlush` is `true` or the
    /// `d_storageEventBuilder` is over the `k_NAGLE_PACKET_COUNT` limit.
    void flushIfNeeded(bool immediateFlush);

    // PRIVATE ACCESSORS

    /// Return true if the specified BlazingMQ `file` needs to be rollover
    /// if it is desired to write data of specified `length` at the
    /// specified `position` in the file, false otherwise.
    bool needRollover(const MappedFileDescriptor& file,
                      bsls::Types::Uint64         position,
                      unsigned int                length) const;

    void aliasMessage(bsl::shared_ptr<bdlbb::Blob>* appData,
                      bsl::shared_ptr<bdlbb::Blob>* options,
                      const DataStoreRecord&        record) const;

    /// Attempt to garbage-collect messages for which TTL has expired.
    /// Note that this routine is no-op unless at the primary node.
    void gcExpiredMessages();

    /// Delete an history of guids or messages maintained by this data
    /// store.  Note that this routine is invoked at primary as well as
    /// replica nodes.
    void gcHistory();

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(FileStore, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an instance with the specified `config`, `processorId`,
    /// `dispatcher`, `cluster`, `clusterStats`, `blobSpPool`,
    /// `miscWorkThreadPool`, `isFSMWorkflow` and `allocator`.
    FileStore(const DataStoreConfig&                          config,
              int                                             processorId,
              mqbi::Dispatcher*                               dispatcher,
              mqbnet::Cluster*                                cluster,
              const bsl::shared_ptr<mqbstat::PartitionStats>& partitionStats,
              BlobSpPool*                                     blobSpPool,
              StateSpPool*                                    statePool,
              bdlmt::FixedThreadPool* miscWorkThreadPool,
              bool                    isFSMWorkflow,
              bool                    doesFSMwriteQLIST,
              int                     replicationFactor,
              StoragesMonitor*        storagesMonitor,
              bslma::Allocator*       allocator);

    /// Destroy this instance.  The behavior is undefined unless this
    /// instance has been closed.
    ~FileStore() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbi::DispatcherClient)

    /// Called by the `Dispatcher` when it has the specified `event` to
    /// deliver to the client.
    void onDispatcherEvent(const mqbi::DispatcherEvent& event)
        BSLS_KEYWORD_OVERRIDE;

    /// Called by the dispatcher to flush any pending operation.
    void flush() BSLS_KEYWORD_OVERRIDE;

    /// Iterate over all storages and run garbage collection.
    /// Regularily invoked from the scheduler and executed from the dispatcher
    /// thread.
    void scheduledCleanupStorages();

    /// Return a pointer to the dispatcher this client is associated with.
    mqbi::Dispatcher* dispatcher() BSLS_KEYWORD_OVERRIDE;

    /// Return a reference to the dispatcherClientData.
    mqbi::DispatcherClientData& dispatcherClientData() BSLS_KEYWORD_OVERRIDE;

    void dispatchEvent(mqbi::Dispatcher::DispatcherEventRvRef event);

#if BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES
    // C++03 compatibility:
    // It is not possible to cast mqbevt movable refs to mqbi movable ref.
    template <class EVENT_TYPE>
    void dispatchEvent(bslmf::MovableRef<bsl::shared_ptr<EVENT_TYPE> > event)
    {
        mqbi::Dispatcher::DispatcherEventSp base(
            bslmf::MovableRefUtil::access(event));
        dispatchEvent(bslmf::MovableRefUtil::move(base));
    }
#endif

    /// Execute the specified `functor`, using the `e_CALLBACK` event
    /// type, in the processor associated to this object.
    void execute(const mqbi::Dispatcher::VoidFunction& functor)
        BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Open this instance using the optionally specified `queueKeyInfoMap`.
    /// Return zero on success, non-zero value otherwise.
    int open(QueueKeyInfoMap* queueKeyInfoMap) BSLS_KEYWORD_OVERRIDE;

    /// Open this instance for Raft mode.  Populate the specified
    /// `recoveryIndex` with a `RecoveryRecordInfo` for every journal
    /// record after the rollover boundary, in forward order.  Return zero
    /// on success, non-zero value otherwise.
    int openForRaft(bsl::deque<RecoveryRecordInfo>* recoveryIndex);

    /// Close this instance.  If the optional `flush` flag is true, flush
    /// the data store to disk.  If the optional `archive` flag is true,
    /// archive the data store.  Return zero on success, non-zero value
    /// otherwise.
    int close(bool flush = false, bool archive = false) BSLS_KEYWORD_OVERRIDE;

    /// Create and load into the specified `storageSp` an instance of
    /// ReplicatedStorage for the queue having the specified `queueUri` and
    /// `queueKey` and belonging to the specified `domain`.  Behavior is
    /// undefined unless `storageSp` and `domain` are non-null.
    void createStorage(bsl::shared_ptr<ReplicatedStorage>* storageSp,
                       const bmqt::Uri&                    queueUri,
                       const mqbu::StorageKey&             queueKey,
                       mqbi::Domain* domain) BSLS_KEYWORD_OVERRIDE;

    /// Payload related
    /// ---------------

    /// Write the specified `appData` and `options` belonging to specified
    /// `queueKey` and having specified `guid` and `attributes` to the data
    /// store, and update the specified `handle` with an identifier which
    /// can be used to retrieve the message.
    int
    writeMessageRecord(mqbi::StorageMessageAttributes*     attributes,
                       DataStoreRecordHandle*              handle,
                       const bmqt::MessageGUID&            guid,
                       const bsl::shared_ptr<bdlbb::Blob>& appData,
                       const bsl::shared_ptr<bdlbb::Blob>& options,
                       const mqbu::StorageKey& queueKey) BSLS_KEYWORD_OVERRIDE;

    /// Qlist related
    /// -------------

    /// Write a record for the specified `queueUri` with specified
    /// `queueKey`, and `timestamp` to the data file.  If the specified
    /// `appIdKeyPairs` vector is non-empty, write those fields to the
    /// record as well.  Return zero on success, non-zero value otherwise.
    int writeQueueCreationRecord(DataStoreRecordHandle*  handle,
                                 const bmqt::Uri&        queueUri,
                                 const mqbu::StorageKey& queueKey,
                                 const AppInfos&         appIdKeyPairs,
                                 bsls::Types::Uint64     timestamp,
                                 bool isNewQueue) BSLS_KEYWORD_OVERRIDE;

    int writeQueuePurgeRecord(DataStoreRecordHandle*       handle,
                              const mqbu::StorageKey&      queueKey,
                              const mqbu::StorageKey&      appKey,
                              bsls::Types::Uint64          timestamp,
                              const DataStoreRecordHandle& start)
        BSLS_KEYWORD_OVERRIDE;

    int writeQueueDeletionRecord(DataStoreRecordHandle*  handle,
                                 const mqbu::StorageKey& queueKey,
                                 const mqbu::StorageKey& appKey,
                                 bsls::Types::Uint64     timestamp)
        BSLS_KEYWORD_OVERRIDE;

    /// Journal related
    /// ---------------

    /// Write a CONFIRM record to the journal with the specified `queueKey`,
    /// optional `appKey`, `guid`, `timestamp` and `reason` to the
    /// journal.  Return zero on success, non-zero value otherwise.
    int writeConfirmRecord(DataStoreRecordHandle*   handle,
                           const bmqt::MessageGUID& guid,
                           const mqbu::StorageKey&  queueKey,
                           const mqbu::StorageKey&  appKey,
                           bsls::Types::Uint64      timestamp,
                           ConfirmReason::Enum reason) BSLS_KEYWORD_OVERRIDE;

    /// Write a DELETION record to the journal with the specified
    /// `queueKey`, `flag`, `guid` and `timestamp` to the journal.  Return
    /// zero on success, non-zero value otherwise.
    int
    writeDeletionRecord(const bmqt::MessageGUID& guid,
                        const mqbu::StorageKey&  queueKey,
                        DeletionRecordFlag::Enum deletionFlag,
                        bsls::Types::Uint64 timestamp) BSLS_KEYWORD_OVERRIDE;

    int writeSyncPointRecord(const bmqp_ctrlmsg::SyncPoint& syncPoint,
                             SyncPointType::Enum            type,
                             unsigned int                   primaryLeaseId,
                             bsls::Types::Uint64            sequenceNumber)
        BSLS_KEYWORD_OVERRIDE;

    /// Raft integration
    /// -----------------

    /// Write a pre-formatted record to the appropriate mmap files and load
    /// its physical metadata (journal offset, data/qlist offset, record type,
    /// and record handle) into the specified 'info'.  'data' contains the
    /// journal record (and, on the replica path, the payload appended after
    /// it).  On the primary path the optionally specified 'payload' carries
    /// the data-file or qlist content separately, avoiding an intermediate
    /// combined blob; when 'payload' is null the payload bytes are read from
    /// 'data' starting after the journal record.  'info's sequence number and
    /// primary lease id are owned by the Raft layer and are *not* touched
    /// here.  Update file positions and insert into 'd_records'.  Return 0 on
    /// success, non-zero on error.
    int writeFormattedRecord(const bdlbb::Blob&                  data,
                             RecoveryRecordInfo*                 info,
                             const bsl::shared_ptr<bdlbb::Blob>& payload = {});

    /// Notify that a record has been committed by Raft quorum on a replica.
    /// The specified `data` blob contains the journal record (and optional
    /// payload).  The specified `handle` identifies the record in
    /// `d_records`.  Read queueKey/guid from the blob, look up the
    /// storage, and call the appropriate `ReplicatedStorage::process*`
    /// method.
    void onRecordCommittedReplica(const bdlbb::Blob&           data,
                                  const DataStoreRecordHandle& handle);

    /// Drop this FileStore's (raw) references to the partition's storages.
    /// Invoked on a Raft snapshot install after the `StoragesMonitor` has
    /// destroyed the storage objects (`onStoragesCleared`): the raw pointers
    /// in `d_storages` are now dangling and must not be looked up until the
    /// reopen re-registers fresh storages.
    void clearStorages();

    /// Reconstruct the queue URI and appId/appKey pairs of a locally-written,
    /// Raft-committed `e_QUEUE_OP` (CREATION or ADDITION) record identified by
    /// the specified `handle`, by reading the local journal (for the QLIST
    /// offset patched in at append time) and local QLIST file.  Load the
    /// results into the specified `uri` and `appIdKeyPairs`.  Return 0 on
    /// success, and a non-zero value if the local QLIST record cannot be read
    /// (e.g. this node is not QLIST-aware).
    int loadQueueCreationInfo(bmqt::Uri*                   uri,
                              AppInfos*                    appIdKeyPairs,
                              const DataStoreRecordHandle& handle) const;

    /// Notify that the record described by the specified 'pw' has been
    /// committed by Raft quorum on the primary.  For a MESSAGE record, mark
    /// its handle as receipted and, if the queue still exists, invoke
    /// 'queue->onReceipt()'.  Non-MESSAGE records produce no handle and are
    /// ignored.
    void onRecordCommittedPrimary(PendingWrite& pw);

    /// Read the record at the specified 'journalOffset' (and any
    /// associated data payload) into the specified 'out' blob using
    /// zero-copy aliased BlobBuffers from the mmap'd files.  Return 0 on
    /// success, non-zero on error.
    int readRecord(bsl::shared_ptr<bdlbb::Blob>* out,
                   bsls::Types::Uint64           journalOffset) const;

    /// Truncate the journal file at the specified 'offset'.  Return 0 on
    /// success, non-zero on error.
    int truncateJournal(bsls::Types::Uint64 offset);

    /// Truncate the data file at the specified 'offset'.  Return 0 on
    /// success, non-zero on error.
    int truncateData(bsls::Types::Uint64 offset);

    /// Truncate the qlist file at the specified 'offset'.  A no-op returning
    /// 0 when this partition is not qlist-aware.  Return 0 on success,
    /// non-zero on error.
    int truncateQlist(bsls::Types::Uint64 offset);

    /// Remove from 'd_records' all entries whose journal offset is at or
    /// above the specified 'journalOffset'.  Reverse-walks from the tail,
    /// relying on the monotonic relationship between sequence numbers and
    /// journal offsets.
    void truncateRecords(bsls::Types::Uint64 journalOffset);

    /// Look up the record identified by the specified 'primaryLeaseId' and
    /// 'sequenceNumber' in d_records.  Load the journal offset into the
    /// specified 'journalOffset' and the data/qlist offset into the
    /// specified 'dataOffset' (0 if not applicable).  Return 0 on
    /// success, non-zero if not found.
    int lookupRecord(bsls::Types::Uint64* journalOffset,
                     bsls::Types::Uint64* dataOffset,
                     unsigned int         primaryLeaseId,
                     bsls::Types::Uint64  sequenceNumber) const;

    /// Raft on-commit-rollover buffering: reserve a *placeholder*
    /// `DataStoreRecord` in `d_records` keyed by the specified
    /// `primaryLeaseId` and `sequenceNumber`, with a placeholder offset (0),
    /// `d_hasReceipt = false`, and the specified `recordType`; load into the
    /// specified `handleOut` an iterator to the reserved entry.  The physical
    /// offsets are patched in later by `bindOrUpdateRecord` when the buffered
    /// write drains into the new file after rollover.  Because the underlying
    /// container keeps iterators/references stable across inserts, the handle
    /// returned here can be handed to the caller immediately and remains valid
    /// until the drain patches (or `dropPendingRecord` erases) the entry.
    void reservePendingRecord(PendingWrite*       pw,
                              unsigned int        primaryLeaseId,
                              bsls::Types::Uint64 sequenceNumber,
                              RecordType::Enum    recordType);

    /// Erase the placeholder `DataStoreRecord` identified by the specified
    /// `handle` (previously created by `reservePendingRecord`).  Used to clean
    /// up buffered writes that are dropped without being committed (e.g. on
    /// leadership loss).  No-op if `handle` is invalid.
    void dropPendingRecord(const DataStoreRecordHandle& handle);

    /// Raft primary path: write a message record directly to mmap using the
    /// input fields of the specified 'pw'.  No StorageEvent replication is
    /// performed.  Fills 'pw->d_journalOffset', 'pw->d_dataOffset',
    /// 'pw->d_entryBlob' (zero-copy mmap alias), and 'pw->d_handle'.
    int formatMessageRecord(PendingWrite* pw);

    /// Raft primary path: write a queue creation record (journal + qlist)
    /// directly to mmap using the input fields of the specified 'pw'
    /// (d_queueUri, d_queueKey, d_appIdKeyPairs_p, d_timestamp,
    /// d_isNewQueue).  Fills 'pw->d_journalOffset', 'pw->d_entryBlob'
    /// (journal record + qlist bytes), and 'pw->d_handle'.
    int formatQueueCreationRecord(PendingWrite* pw);

    /// Raft primary path: write a regular sync-point journal record directly
    /// to mmap for the specified 'pw' (using its 'd_primaryLeaseId' = term and
    /// 'd_sequenceNumber' = index for the record header PSN, and the active
    /// file set's current offsets).  Fills 'pw->d_journalOffset' and
    /// 'pw->d_entryBlob' (zero-copy mmap alias of the journal record).
    int formatSyncPointRecord(PendingWrite* pw);

    /// Raft primary path: write a confirm record (journal only) directly to
    /// mmap using the input fields of the specified 'pw' (d_queueKey,
    /// d_appKey, d_guid, d_timestamp, d_confirmReason), stamping the record
    /// header PSN from 'pw->d_primaryLeaseId' / 'pw->d_sequenceNumber'.  Fills
    /// 'pw->d_journalOffset', 'pw->d_entryBlob' (zero-copy mmap alias of the
    /// journal record), and 'pw->d_handle'.
    int formatConfirmRecord(PendingWrite* pw);

    /// Raft primary path: write a message-deletion record (journal only)
    /// directly to mmap using the input fields of the specified 'pw'
    /// (d_queueKey, d_guid, d_timestamp, d_deletionFlag), stamping the record
    /// header PSN from 'pw->d_primaryLeaseId' / 'pw->d_sequenceNumber'.  Fills
    /// 'pw->d_journalOffset' and 'pw->d_entryBlob' (zero-copy mmap alias of
    /// the journal record).  Does not produce a handle (no DataStoreRecord).
    int formatDeletionRecord(PendingWrite* pw);

    /// Raft primary path: write a queue-purge record (journal only) directly
    /// to mmap using the input fields of the specified 'pw' (d_queueKey,
    /// d_appKey, d_timestamp, d_startPrimaryLeaseId, d_startSequenceNumber),
    /// stamping the record header PSN from 'pw->d_primaryLeaseId' /
    /// 'pw->d_sequenceNumber'.  Fills 'pw->d_journalOffset', 'pw->d_entryBlob'
    /// (zero-copy mmap alias of the journal record), and 'pw->d_handle'.
    int formatQueuePurgeRecord(PendingWrite* pw);

    /// Raft primary path: write a queue-deletion record (journal only)
    /// directly to mmap using the input fields of the specified 'pw'
    /// (d_queueKey, d_appKey, d_timestamp), stamping the record header PSN
    /// from 'pw->d_primaryLeaseId' / 'pw->d_sequenceNumber'.  Fills
    /// 'pw->d_journalOffset', 'pw->d_entryBlob' (zero-copy mmap alias of the
    /// journal record), and 'pw->d_handle'.
    int formatQueueDeletionRecord(PendingWrite* pw);

    /// Rollover the specified `record` from `oldFileSet` to the `newFileSet`,
    /// and if it is a message record, update the counter of the corresponding
    /// queue by one in the specified `queueKeyCounterMap`.  The record is
    /// copied from the current active (front) file set into the specified
    /// `newFileSet`, and `record`'s offsets are updated in place to the new
    /// file set.  The behavior is undefined unless `record` is not a
    /// JOURNAL_OP record.  (Public so the Raft rollover orchestration in
    /// `PartitionRaftLog` can drive the copy loop itself, in strict index
    /// order.)
    void writeRolledOverRecord(DataStoreRecord*    record,
                               QueueKeyCounterMap* queueKeyCounterMap,
                               FileSet*            newFileSet);

    /// Copy every outstanding record in `d_records` whose sequence number is
    /// at most the specified `maxSequenceNum` into the specified `newFileSet`
    /// (via `writeRolledOverRecord`), accumulating per-queue counters into
    /// the specified `queueKeyCounterMap`.  Used for the committed-prefix
    /// portion of a rollover; the uncommitted tail is driven separately by
    /// the caller in strict index order.  Legacy `rolloverImpl` calls this
    /// with `sequenceNumber()` (the current/highest sequence number) to copy
    /// all outstanding records.
    void writeRolledOverRecords(FileSet*            newFileSet,
                                    QueueKeyCounterMap* queueKeyCounterMap,
                                    bsls::Types::Uint64 maxSequenceNum);

    /// Copy the single `JournalOpRecord` (sync point) located at the
    /// specified `oldJournalOffset` in the current active (front, old) file
    /// set's journal into the specified `newFileSet`'s journal at its current
    /// position, bump that position, and return the new journal offset of the
    /// copied record.  Sync points carry no data/qlist payload and are not
    /// counted in outstanding bytes (matching
    /// `writeFirstSyncPointAfterRollover` and `formatSyncPointRecord`).  The
    /// behavior is undefined unless the record at `oldJournalOffset` is a
    /// `JournalOpRecord`.  Used by the Raft rollover orchestration to relocate
    /// the (journal-op only) log tail that a leadership change can leave after
    /// `e_ROLLOVER`.
    bsls::Types::Uint64
    writeRolledOverJournalOpRecord(FileSet*            newFileSet,
                                   bsls::Types::Uint64 oldJournalOffset);

    /// Create and open a new file set to receive the rolled-over records,
    /// loading it into the specified `newFileSet` (BlazingMQ and
    /// file-specific headers are written).  Return zero on success and a
    /// non-zero value otherwise.  This is the "prepare" step of a rollover:
    /// it is invoked both by legacy `rolloverImpl` and (public) by the Raft
    /// rollover orchestration in `PartitionRaftLog`, which then drives the
    /// record-copy loop and `writeFirstSyncPointAfterRollover` itself.
    int prepareRolloverFileSet(FileSetSp* newFileSet);

    /// Write the first (marker) sync point into the JOURNAL of the specified
    /// `newFileSet` at the end of a rollover, deriving its PSN and
    /// `primaryNodeId` from the `e_ROLLOVER` sync point located at the
    /// specified `rolloverSyncPointOffset` in the current active (front) file
    /// set, using the new file's positions for the DATA/QLIST offsets, and
    /// stamping the specified `timestamp`.  This updates
    /// `JournalFileHeader.d_firstSyncPointAfterRolloverOffset` (whose non-zero
    /// value marks the rollover as successfully finished, aiding crash
    /// recovery) and resets `d_syncPoints` to hold only this marker.  This
    /// must be the last write to occur in a rollover.  Legacy `rolloverImpl`
    /// passes `d_syncPoints.back().offset()`; the Raft orchestration passes
    /// the old-file offset of the `e_ROLLOVER` log entry (`d_syncPoints` is
    /// not maintained on the Raft write path).
    void writeFirstSyncPointAfterRollover(
        FileSet*            newFileSet,
        bsls::Types::Uint64 rolloverSyncPointOffset,
        bsls::Types::Uint64 timestamp);

    /// Complete a rollover once the specified `newActiveFileSetSp` has been
    /// fully populated: truncate the current active (old, front) file set,
    /// garbage-collect it if it holds no outstanding aliased references
    /// (otherwise drop its aliased chunk), insert `newActiveFileSetSp` as the
    /// new front of `d_fileSets`, and schedule deletion of any archived file
    /// sets.  (Public so the Raft rollover orchestration in `PartitionRaftLog`
    /// can drive it itself.)
    void finalizeRolloverFileSet(const FileSetSp& newActiveFileSetSp);

    /// Log a per-queue summary (message and byte counts) of a rollover from
    /// the specified `queueKeyCounterMap` accumulated by
    /// `writeRolledOverRecord`.
    void logRolloverQueueSummary(const QueueKeyCounterMap& queueKeyCounterMap);

    /// Return the record-header timestamp of the JournalOpRecord located at
    /// the specified `journalOffset` in the current active (front) file
    /// set's journal.  Read-only; no side effects.  (Used by the Raft
    /// rollover orchestration to stamp the rollover marker with the
    /// `e_ROLLOVER` record's own timestamp.)
    bsls::Types::Uint64
    journalOpTimestampAt(bsls::Types::Uint64 journalOffset) const;

    /// Return `true` if the active (front) file set cannot physically
    /// accommodate the next record, i.e. a rollover is required before it can
    /// be written.  The JOURNAL is always required to hold the next common
    /// record (plus reserved areas); the specified `dataBytes` and
    /// `qlistBytes` are the additional DATA and QLIST space the record needs
    /// (pass 0 where not applicable).  Has no side effects.  (Used by the Raft
    /// write path to decide when to trigger `PartitionRaft::proposeRollover`.)
    bool primaryNeedsRollover(bsls::Types::Uint64 dataBytes,
                              bsls::Types::Uint64 qlistBytes) const;

    /// Remove the record identified by the specified `handle`.  The
    /// behavior is undefined unless `handle` is valid and represents a
    /// record in the data store.
    void
    removeRecordRaw(const DataStoreRecordHandle& handle) BSLS_KEYWORD_OVERRIDE;

    /// Attempt to rollover the journal if needed after a purge has cleared
    /// outstanding records.
    void onPurgeComplete() BSLS_KEYWORD_OVERRIDE;

    /// Process the specified storage event `blob` containing one or more
    /// storage messages.  The behavior is undefined unless each message in
    /// the event belongs to this partition, and has same primary and
    /// primary leaseId as expected by this data store instance.
    void
    processStorageEvent(const bsl::shared_ptr<bdlbb::Blob>& blob,
                        bool                 isPartitionSyncEvent,
                        mqbnet::ClusterNode* source) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified recovery event `blob` containing one or more
    /// storage messages.  Return zero on success, non-zero value otherwise.
    /// The behavior is undefined unless each message in the event belongs
    /// to this partition.
    int processRecoveryEvent(const bsl::shared_ptr<bdlbb::Blob>& blob)
        BSLS_KEYWORD_OVERRIDE;

    /// Process Receipt for the specified `primaryLeaseId` and
    /// `sequenceNum`.  The behavior is undefined unless the event belongs
    /// to this partition and unless the `primaryLeaseId` and `sequenceNum`
    /// match a record of the `StorageMessageType::e_DATA` type.
    void
    processReceiptEvent(unsigned int         primaryLeaseId,
                        bsls::Types::Uint64  sequenceNum,
                        mqbnet::ClusterNode* source) BSLS_KEYWORD_OVERRIDE;

    /// Replication related
    /// -------------------

    /// Request the data store to issue a SyncPt.
    int issueSyncPoint();

    /// Set the specified `primaryNode` with the specified `primaryLeaseId`
    /// as the active primary for this data store partition.  Note that
    /// `primaryNode` could refer to the node which owns this data store.
    void setActivePrimary(mqbnet::ClusterNode* primaryNode,
                          unsigned int         primaryLeaseId,
                          bool isRaft = false) BSLS_KEYWORD_OVERRIDE;

    /// Clear the current primary associated with this partition.
    void clearPrimary() BSLS_KEYWORD_OVERRIDE;

    /// Flush any buffered replication messages to the peers.  Behaviour is
    /// undefined unless this cluster node is the primary for this partition.
    void flushStorage() BSLS_KEYWORD_OVERRIDE;

    /// Flush weak consistency queues that have replicated messages since the
    /// last call.  This method has no effect if `d_storageEventBuilder` is not
    /// empty, and must only be called after `flushStorage`.  Behaviour is
    /// undefined unless this cluster node is the primary for this partition.
    void notifyQueuesOnReplicatedBatch();

    /// mqbs::FileStore specific MANIPULATORS

    /// Perform complete rollover of this partition and issue necessary sync
    /// points.
    int rollover() BSLS_KEYWORD_OVERRIDE;

    void registerStorage(ReplicatedStorage* storage) BSLS_KEYWORD_OVERRIDE;

    void unregisterStorage(const ReplicatedStorage* storage)
        BSLS_KEYWORD_OVERRIDE;

    StoragesMonitor* storagesMonitor() BSLS_KEYWORD_OVERRIDE;

    const DataStoreConfig::Records& records() const BSLS_KEYWORD_OVERRIDE;

    void loadMessageRecord(MessageRecord* buffer,
                           const DataStoreConfig::Records::const_iterator& it)
        const BSLS_KEYWORD_OVERRIDE;

    void loadConfirmRecord(ConfirmRecord* buffer,
                           const DataStoreConfig::Records::const_iterator& it)
        const BSLS_KEYWORD_OVERRIDE;

    void loadQueueOpRecord(QueueOpRecord* buffer,
                           const DataStoreConfig::Records::const_iterator& it)
        const BSLS_KEYWORD_OVERRIDE;

    void recordIteratorToHandle(DataStoreRecordHandle* handle,
                                const DataStoreConfig::Records::const_iterator&
                                    it) const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if this partition is Raft-replicated, `false` for the
    /// legacy path.  Safe to call even when no `StoragesMonitor` is set (e.g.
    /// in tests): returns `false` in that case.
    bool isRaft() const;

    void cancelTimersAndWait();

    void processShutdownEvent();

    /// Set the availability of this partition to the specified boolean
    /// `value`.
    void setAvailabilityStatus(bool value) BSLS_KEYWORD_OVERRIDE;

    /// Set the replication factor for strong consistency to `factor`.
    void setReplicationFactor(int factor) BSLS_KEYWORD_OVERRIDE;

    /// This will be used as Implicit Receipt
    void setLastStrongConsistency(unsigned int        primaryLeaseId,
                                  bsls::Types::Uint64 sequenceNum)
        BSLS_KEYWORD_OVERRIDE;

    /// Encode and broadcast the specified `message` to all cluster nodes.
    void broadcastMessage(const bmqp_ctrlmsg::ControlMessage& message);

    /// Encode and send the specified schema `message` to the specified
    /// peer `destination`.
    void sendMessage(const bmqp_ctrlmsg::ControlMessage& message,
                     mqbnet::ClusterNode*                destination);

    /// Load into the specified `storages` the list of queue storages for
    /// which all filters from the specified `filters` are returning true.
    void
    getStorages(StorageList*          storages,
                const StorageFilters& filters) const BSLS_KEYWORD_OVERRIDE;

    /// Load the summary of this partition to the specified `fileStore`
    /// object.
    ///
    /// THREAD: Executed by the queue dispatcher thread associated with the
    ///         specified `fileStore`'s partitionId.
    void loadSummary(mqbcmd::FileStore* fileStore) const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if this node is the active primary for this partition.
    bool isLeader() const BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return true if this instance is open, false otherwise.
    bool isOpen() const BSLS_KEYWORD_OVERRIDE;

    /// Return configuration associated with this instance.
    const DataStoreConfig& config() const BSLS_KEYWORD_OVERRIDE;

    /// Return the replication factor associated with this data store.
    unsigned int clusterSize() const BSLS_KEYWORD_OVERRIDE;

    /// Return total number of records currently present in the data store.
    bsls::Types::Uint64 numRecords() const BSLS_KEYWORD_OVERRIDE;

    void loadMessageRecordRaw(MessageRecord*               buffer,
                              const DataStoreRecordHandle& handle) const
        BSLS_KEYWORD_OVERRIDE;

    void loadConfirmRecordRaw(ConfirmRecord*               buffer,
                              const DataStoreRecordHandle& handle) const
        BSLS_KEYWORD_OVERRIDE;

    void loadDeletionRecordRaw(DeletionRecord*              buffer,
                               const DataStoreRecordHandle& handle) const
        BSLS_KEYWORD_OVERRIDE;

    void loadQueueOpRecordRaw(QueueOpRecord*               buffer,
                              const DataStoreRecordHandle& handle) const
        BSLS_KEYWORD_OVERRIDE;

    void loadMessageAttributesRaw(mqbi::StorageMessageAttributes* buffer,
                                  const DataStoreRecordHandle&    handle) const
        BSLS_KEYWORD_OVERRIDE;

    void loadMessageRaw(bsl::shared_ptr<bdlbb::Blob>*   appData,
                        bsl::shared_ptr<bdlbb::Blob>*   options,
                        mqbi::StorageMessageAttributes* attributes,
                        const DataStoreRecordHandle&    handle) const
        BSLS_KEYWORD_OVERRIDE;

    unsigned int getMessageLenRaw(const DataStoreRecordHandle& handle) const
        BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    int processorId() const;

    //   (virtual: mqbi::DispatcherClient)

    /// Return a pointer to the dispatcher this client is associated with.
    const mqbi::Dispatcher* dispatcher() const BSLS_KEYWORD_OVERRIDE;

    const mqbi::DispatcherClientData&
    dispatcherClientData() const BSLS_KEYWORD_OVERRIDE;

    /// Return a printable description of the client (e.g. for logging).
    bsl::string_view description() const BSLS_KEYWORD_OVERRIDE;

    /// Return the partition id associated with this record store.
    int partitionId() const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `fileStoreSet` the active (current) file
    /// set.  The behavior is undefined unless there is a fileSet in current
    /// use.
    void loadCurrentFiles(mqbs::FileStoreSet* fileStoreSet) const;

    /// Return a sorted list of pairs of sync point and corresponding
    /// journal offset maintained by this instance.  Note that sorting order
    /// is from oldest sync point to newest.
    const SyncPointOffsetPairs& syncPoints() const;

    /// Return the current primary node for this partition.
    mqbnet::ClusterNode* primaryNode() const;

    /// Return the current primary leaseId for this partition.
    unsigned int primaryLeaseId() const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if there was Replication Receipt for the specified
    /// `handle`.
    bool hasReceipt(const DataStoreRecordHandle& handle) const
        BSLS_KEYWORD_OVERRIDE;

    bool isFileSetAvailable() const BSLS_KEYWORD_OVERRIDE;

    /// Return the current sequence number for this partition.
    bsls::Types::Uint64 sequenceNumber() const;

    /// Return the replication factor for strong consistency.
    int replicationFactor() const;

    /// Return the current write position (in bytes) of the active data file.
    bsls::Types::Uint64 dataFilePosition() const;

    /// Return the current write position (in bytes) of the active qlist file.
    bsls::Types::Uint64 qlistFilePosition() const;

    /// Return the first sync point after rollover sequence number.
    const bmqp_ctrlmsg::PartitionSequenceNumber&
    firstSyncPointAfterRolloverSeqNum() const;

    /// Return the map of primaryLeaseId to highest sequence number, including
    /// the current primary.
    const LeaseIdToSeqNumMap& highestSeqNums() const;

    /// Return a brief description of the partition for logging purposes.
    const bsl::string& partitionDesc() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------------------
// class FileStore::ReceiptContext
// -------------------------------

inline FileStore::ReceiptContext::ReceiptContext(
    const mqbu::StorageKey&  queueKey,
    const bmqt::MessageGUID& guid,
    const RecordIterator&    handle,
    int                      count,
    mqbi::QueueHandle*       qH)
: d_queueKey(queueKey)
, d_guid(guid)
, d_handle(handle)
, d_qH(qH)
, d_count(count)
{
    // NOTHING
}

// ----------------------------
// class FileStore::NodeContext
// ----------------------------

inline FileStore::NodeContext::NodeContext(BlobSpPool* blobSpPool_p,
                                           const DataStoreRecordKey& key)
: d_key(key)
, d_blob_sp(blobSpPool_p->getObject())
{
    // NOTHING
}

// ---------------
// class FileStore
// ---------------

// PRIVATE MANIPULATORS
inline bsls::Types::Uint64& FileStore::currentSeqNumRef()
{
    return d_highestSeqNums[d_primaryLeaseId];
}

inline void FileStore::insertDataStoreRecord(RecordIterator* recordIt,
                                             const DataStoreRecordKey& key,
                                             const DataStoreRecord&    record)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(recordIt);

    InsertRc insertRc = d_records.insert(bsl::make_pair(key, record));
    BSLS_ASSERT_SAFE(insertRc.second);

    *recordIt = insertRc.first;
}

inline void FileStore::insertDataStoreRecord(DataStoreRecordHandle*    handle,
                                             const DataStoreRecordKey& key,
                                             const DataStoreRecord&    record)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(handle);
    RecordIterator recordIt;
    insertDataStoreRecord(&recordIt, key, record);

    recordIteratorToHandle(handle, recordIt);
}

inline const FileStore::RecordIterator& FileStore::handleTorRecordIterator(
        const DataStoreRecordHandle& handle) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(handle.isValid());

    const RecordIterator& recordIt = *reinterpret_cast<const RecordIterator*>(
        &handle);
    BSLS_ASSERT_SAFE(d_records.end() != d_records.find(recordIt->first));

    return recordIt;
}

void handleTorRecordIterator();


// PRIVATE ACCESSORS
inline const bsl::string& FileStore::partitionDesc() const
{
    return d_partitionDescription;
}

inline bool FileStore::needRollover(const MappedFileDescriptor& file,
                                    bsls::Types::Uint64         position,
                                    unsigned int                length) const
{
    BSLS_ASSERT_SAFE(position <= file.fileSize());
    return file.fileSize() < (position + length);
}

inline void
FileStore::dispatchEvent(mqbi::Dispatcher::DispatcherEventRvRef event)
{
    dispatcher()->dispatchEvent(bslmf::MovableRefUtil::move(event), this);
}

inline void FileStore::execute(const mqbi::Dispatcher::VoidFunction& functor)
{
    dispatcher()->execute(functor,
                          this,
                          mqbi::DispatcherEventType::e_CALLBACK);
}

// MANIPULATORS
inline void
FileStore::setLastStrongConsistency(unsigned int        primaryLeaseId,
                                    bsls::Types::Uint64 sequenceNum)
{
    d_lastRecoveredStrongConsistency.d_primaryLeaseId = primaryLeaseId;
    d_lastRecoveredStrongConsistency.d_sequenceNum    = sequenceNum;

    sendImplicitReceipt();
}

inline void
FileStore::broadcastMessage(const bmqp_ctrlmsg::ControlMessage& message)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(inDispatcherThread());

    flushStorage();
    d_messageTransmitter.broadcastMessage(message);
}

inline void FileStore::sendMessage(const bmqp_ctrlmsg::ControlMessage& message,
                                   mqbnet::ClusterNode* destination)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(inDispatcherThread());

    flushStorage();
    d_messageTransmitter.sendMessage(message, destination);
}

// ACCESSORS
inline const mqbi::DispatcherClientData&
FileStore::dispatcherClientData() const
{
    return d_dispatcherClientData;
}

inline mqbi::Dispatcher* FileStore::dispatcher()
{
    return dispatcherClientData().dispatcher();
}

inline const mqbi::Dispatcher* FileStore::dispatcher() const
{
    return dispatcherClientData().dispatcher();
}

inline mqbi::DispatcherClientData& FileStore::dispatcherClientData()
{
    return d_dispatcherClientData;
}

inline bsl::string_view FileStore::description() const
{
    return partitionDesc();
}

inline int FileStore::partitionId() const
{
    return d_config.partitionId();
}

inline bool FileStore::isOpen() const
{
    return d_isOpen;
}

inline const DataStoreConfig& FileStore::config() const
{
    return d_config;
}

inline unsigned int FileStore::clusterSize() const
{
    return static_cast<unsigned int>(d_cluster_p->nodes().size());
}

inline StoragesMonitor* FileStore::storagesMonitor()
{
    // Never invoked on a Raft partition: PartitionRaft::storagesMonitor()
    // returns its own independently-held pointer rather than delegating
    // here. Not asserted since this FileStore's own d_storagesMonitor_p is
    // still valid and identical; calling it would not be wrong, merely
    // unused.
    return d_storagesMonitor_p;
}

inline const DataStoreConfig::Records& FileStore::records() const
{
    return d_records;
}

inline bsls::Types::Uint64 FileStore::numRecords() const
{
    return d_records.size();
}

inline int FileStore::processorId() const
{
    return d_dispatcherClientData.processorHandle();
}

inline const FileStore::SyncPointOffsetPairs& FileStore::syncPoints() const
{
    return d_syncPoints;
}

inline mqbnet::ClusterNode* FileStore::primaryNode() const
{
    return d_primaryNode_p;
}

inline bool FileStore::isLeader() const
{
    return 0 != d_primaryNode_p &&
           d_primaryNode_p->nodeId() == d_config.nodeId();
}

inline bool FileStore::isRaft() const
{
    return d_storagesMonitor_p && d_storagesMonitor_p->isRaft();
}

inline unsigned int FileStore::primaryLeaseId() const
{
    return d_primaryLeaseId;
}

inline bsls::Types::Uint64 FileStore::sequenceNumber() const
{
    LeaseIdToSeqNumMapCIter cit = d_highestSeqNums.find(d_primaryLeaseId);
    return (cit != d_highestSeqNums.end()) ? cit->second : 0;
}

inline int FileStore::replicationFactor() const
{
    return d_replicationFactor;
}

inline const bmqp_ctrlmsg::PartitionSequenceNumber&
FileStore::firstSyncPointAfterRolloverSeqNum() const
{
    return d_firstSyncPointAfterRolloverSeqNum;
}

inline const FileStore::LeaseIdToSeqNumMap& FileStore::highestSeqNums() const
{
    return d_highestSeqNums;
}

inline bool FileStore::isFileSetAvailable() const
{
    BSLS_ASSERT_SAFE(0 < d_fileSets.size());

    FileSet* activeFileSet = d_fileSets[0].get();

    return activeFileSet->d_journalFileAvailable;
}

}  // close package namespace

}  // close enterprise namespace

#endif
