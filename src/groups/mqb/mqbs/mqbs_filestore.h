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

// mqbs_filestore.h                                                   -*-C++-*-
#ifndef INCLUDED_MQBS_FILESTORE
#define INCLUDED_MQBS_FILESTORE

//@PURPOSE: Provide a file-backed BlazingMQ data store.
//
//@CLASSES:
//  mqbs::FileStore:         File-backed BlazingMQ data store.
//  mqbs::FileStoreIterator: Iterator over records in a 'mqbs::FileStore'
//  mqbs::FileStore_AliasedBufferDeleter
//
//@SEE ALSO: mqbs::FileStoreProtcol
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

// MWC
#include <mwcc_orderedhashmap.h>
#include <mwcma_countingallocatorstore.h>
#include <mwcu_blob.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlcc_objectpool.h>
#include <bdlcc_sharedobjectpool.h>
#include <bdlmt_eventscheduler.h>
#include <bdlmt_fixedthreadpool.h>
#include <bdlmt_throttle.h>
#include <bsl_deque.h>
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
class ClusterStats;
}

namespace mqbs {

// FORWARD DECLARATIONS
class DataFileIterator;
class FileStore;
class FileStoreSet;
class JournalFileIterator;
class QlistFileIterator;
class ReplicatedStorage;

// =====================================
// struct FileStore_AliasedBufferDeleter
// =====================================

struct FileStore_AliasedBufferDeleter {
    // PUBLIC DATA
    FileSet* d_fileSet_p;

    // CREATORS
    FileStore_AliasedBufferDeleter();

    // MANIPULATORS
    void setFileSet(FileSet* fileSet);

    void reset();
};

// ===============
// class FileStore
// ===============

/// Mechanism to store BlazingMQ messages in file. This component is
/// *const-thread* safe.
class FileStore : public DataStore {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBS.FILESTORE");

    // FRIENDS
    friend class FileStoreIterator;
    friend struct FileStore_AliasedBufferDeleter;

  public:
    // TYPES

    /// Pool of shared pointers to Blobs
    typedef bdlcc::SharedObjectPool<
        bdlbb::Blob,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::RemoveAll<bdlbb::Blob> >
        BlobSpPool;

    typedef bdlcc::SharedObjectPool<
        mwcu::AtomicState,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::Reset<mwcu::AtomicState> >
        StateSpPool;

    /// It is important for this to be a deque instead of a vector, because
    /// during recovery at startup, we insert recoverd SyncPts at the
    /// beginning of the container, which can be slow for a vector (plenty
    /// of reallocations and memmoves), specially if there are a large
    /// number of SyncPts.  A `std::list` can't be used because certain
    /// recovery logic carries out binary search on this container.
    typedef bsl::deque<bmqp_ctrlmsg::SyncPointOffsetPair> SyncPointOffsetPairs;

    typedef SyncPointOffsetPairs::const_iterator SyncPointOffsetConstIter;

    /// Type of the functor required by `applyForEachQueue`.
    typedef bsl::function<void(mqbi::Queue*)> QueueFunctor;

    typedef bsl::shared_ptr<FileSet> FileSetSp;
    typedef bsl::vector<FileSetSp>   FileSets;

    typedef StorageCollectionUtil::StorageList StorageList;

    typedef bsl::vector<StorageCollectionUtil::StorageFilter> StorageFilters;
    typedef StorageFilters::const_iterator StorageFiltersconstIter;

  private:
    // PRIVATE TYPES
    typedef FileStore_AliasedBufferDeleter        AliasedBufferDeleter;
    typedef bsl::shared_ptr<AliasedBufferDeleter> AliasedBufferDeleterSp;

    /// Pool of shared pointers to `AliasedBufferDeleter`
    typedef bdlcc::SharedObjectPool<
        AliasedBufferDeleter,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::Reset<AliasedBufferDeleter> >
        AliasedBufferDeleterSpPool;

    typedef DataStoreConfig::Records             Records;
    typedef DataStoreConfig::RecordIterator      RecordIterator;
    typedef DataStoreConfig::RecordConstIterator RecordConstIterator;

    typedef bsl::pair<RecordIterator, bool> InsertRc;

    /// Counter for the number of messages and bytes in a queue
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

    typedef bdlmt::EventScheduler::RecurringEventHandle RecurringEventHandle;

    typedef DataStoreConfig::QueueKeyInfoMapConstIter QueueKeyInfoMapConstIter;
    typedef DataStoreConfig::QueueKeyInfoMapInsertRc  QueueKeyInfoMapInsertRc;

    typedef mqbi::Storage::AppIdKeyPair  AppIdKeyPair;
    typedef mqbi::Storage::AppIdKeyPairs AppIdKeyPairs;

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
        DataStoreRecordKey d_key;
        // last Receipt from/to this
        // node (Replica/Primary).
        bsl::shared_ptr<bdlbb::Blob> d_blob_sp;
        // Receipt to this node.
        bsl::shared_ptr<mwcu::AtomicState> d_state;

        NodeContext(bdlbb::BlobBufferFactory* factory,
                    const DataStoreRecordKey& key,
                    bslma::Allocator*         basicAllocator = 0);
    };
    typedef mwcc::OrderedHashMap<DataStoreRecordKey,
                                 ReceiptContext,
                                 DataStoreRecordKeyHashAlgo>
        Unreceipted;

    /// Map of NodeId -> NodeContext to assist in Receipt processing
    typedef bsl::unordered_map<int, NodeContext> NodeReceiptContexts;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    mwcma::CountingAllocatorStore d_allocators;
    // Allocator store to spawn new
    // allocators for sub-components

    mwcma::CountingAllocatorStore d_storageAllocatorStore;
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

    mqbstat::ClusterStats* d_clusterStats_p;
    // Stat object associated to the
    // Cluster this FileStore belongs to,
    // used to report partition level
    // metrics.

    mutable BlobSpPool* d_blobSpPool_p;
    // Pool of shared pointers to blobs to
    // use.

    StateSpPool* d_statePool_p;

    mutable AliasedBufferDeleterSpPool d_aliasedBufferDeleterSpPool;

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

    unsigned int d_primaryLeaseId;

    bsls::Types::Uint64 d_sequenceNum;
    // Sequence number of the last
    // replicated message

    SyncPointOffsetPairs d_syncPoints;
    // List of (syncPoints, offset) pairs,
    // from oldest to newest

    StoragesMap d_storages;
    // Map [QueueKey->ReplicatedStorage*]

    bdlmt::Throttle d_alarmSoftLimiter;
    // Throttler for alarming on soft
    // limits of partition files

    bool d_isCSLModeEnabled;
    // Whether CSL mode is enabled

    bool d_isFSMWorkflow;
    // Whether CSL FSM worklow is in
    // effect

    bool d_ignoreCrc32c;
    // Whether to ignore Crc32
    // calculations.  We should only set
    // this to true during testing.

    int d_nagglePacketCount;
    // Max number of messages in the
    // 'd_storageEventBuilder' before
    // flushing the builder.  Depending
    // the cluster channels load, it can
    // grow or shrink.

    bmqp::StorageEventBuilder d_storageEventBuilder;
    // Storage event builder to use.

  private:
    // NOT IMPLEMENTED
    FileStore(const FileStore&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    FileStore& operator=(const FileStore&) BSLS_CPP11_DELETED;

  private:
    // PRIVATE MANIPULATORS

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
    /// invocation of other flavor of `close`.
    void close(FileSet& fileSetRef, bool flush);

    /// Move all files contained in the specified `fileSet` to the archive
    /// location as specified in this instance's configuration provided at
    /// construction.  Note that files are not truncated or closed.
    void archive(FileSet* fileSet);

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
    int openInRecoveryMode(bsl::ostream&          errorDescription,
                           const QueueKeyInfoMap& queueKeyInfoMap);

    /// In non-FSM workflow, populate the specified `queueKeyInfoMap`
    /// container with the messages recovered from the BlazingMQ files
    /// represented by the specified `jit`, `qit` and `dit` iterators.
    /// Else, use the information from `queueKeyInfoMap` to validate against
    /// the messages recovered from `jit` and `dit`; QList file will not be
    /// used since `queueKeyInfoMap` already contains such queue
    /// information.  Return zero on success, non zero value otherwise.  The
    /// behavior is undefined unless the journal iterator `jit` is in
    /// reverse mode.  Note that this method invalidates all iterators.
    int recoverMessages(QueueKeyInfoMap*     queueKeyInfoMap,
                        bsls::Types::Uint64* journalOffset,
                        bsls::Types::Uint64* qlistOffset,
                        bsls::Types::Uint64* dataOffset,
                        JournalFileIterator* jit,
                        QlistFileIterator*   qit,
                        DataFileIterator*    dit);

    /// Rollover the outstanding messages belonging to the storages mapped
    /// to this file store, from active file set into the rollover file set,
    /// and make rolled over file set the new active file set.  Return zero
    /// on success, non-zero value otherwise.  Note that in its *current*
    /// implementation, this routine has no side-effect in case of failure.
    int rollover(bsls::Types::Uint64 timestamp);

    /// If the specified `file` of specified `fileType` having specified
    /// `currentSize` and `fileName` cannot accommodate additional
    /// `requestedSpace`, roll over the `file`.  Return zero on success,
    /// non-zero value otherwise.  Note that in case roll over is not
    /// needed, zero is returned.
    int rolloverIfNeeded(FileType::Enum              fileType,
                         const MappedFileDescriptor& file,
                         const bsl::string&          fileName,
                         bsls::Types::Uint64         currentSize,
                         unsigned int                requestedSpace);

    /// Write a QUEUE_OP record to the journal with the specified
    /// `queueKey`, optional `appKey`, `timestamp`, `opValue` and `subValue`
    /// to the journal.  Return zero on success, non-zero value otherwise.
    int writeQueueOpRecord(DataStoreRecordHandle*  handle,
                           const mqbu::StorageKey& queueKey,
                           const mqbu::StorageKey& appKey,
                           QueueOpType::Enum       queueOpFlag,
                           bsls::Types::Uint64     timestamp);

    /// Rollover over the specified `record` from `oldFileSet` to the
    /// `newFileSet`, and if it is a message record, update the counter of
    /// the corresponding queue by one in the specified
    /// `queueKeyCounterMap`.
    void writeRolledOverRecord(DataStoreRecord*    record,
                               QueueKeyCounterMap* queueKeyCounterMap,
                               FileSet*            oldFileSet,
                               FileSet*            newFileSet);

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

    int issueSyncPointInternal(SyncPointType::Enum            type,
                               bool                           immediateFlush,
                               const bmqp_ctrlmsg::SyncPoint* syncPoint = 0);

    int writeMessageRecord(const bmqp::StorageHeader&          header,
                           const mqbs::RecordHeader&           recHeader,
                           const bsl::shared_ptr<bdlbb::Blob>& event,
                           const mwcu::BlobPosition&           recordPosition);

    int writeQueueCreationRecord(const bmqp::StorageHeader&          header,
                                 const mqbs::RecordHeader&           recHeader,
                                 const bsl::shared_ptr<bdlbb::Blob>& event,
                                 const mwcu::BlobPosition& recordPosition);

    int writeJournalRecord(const bmqp::StorageHeader&          header,
                           const mqbs::RecordHeader&           recHeader,
                           const bsl::shared_ptr<bdlbb::Blob>& event,
                           const mwcu::BlobPosition&           recordPosition,
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

    /// Insert the specified `record` value by the specified `key` into the
    /// list of outstanding records, and assign to the specified `handle` an
    /// iterator to the inserted record.
    void insertDataStoreRecord(RecordIterator*           recordIt,
                               const DataStoreRecordKey& key,
                               const DataStoreRecord&    record);
    void insertDataStoreRecord(DataStoreRecordHandle*    handle,
                               const DataStoreRecordKey& key,
                               const DataStoreRecord&    record);
    void recordIteratorToHandle(DataStoreRecordHandle* handle,
                                const RecordIterator   recordIt);

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

    ///  if the specified `immediateFlush` is `true` or the
    /// `d_storageEventBuilder` is over the `d_nagglePacketCount` limit.
    void flushIfNeeded(bool immediateFlush);

    // PRIVATE ACCESSORS

    /// Return a brief description of the partition for logging purposes.
    const bsl::string& partitionDesc() const;

    /// Return true if the specified BlazingMQ `file` needs to be rollover
    /// if it is desired to write data of specified `length` at the
    /// specified `position` in the file, false otherwise.
    bool needRollover(const MappedFileDescriptor& file,
                      bsls::Types::Uint64         position,
                      unsigned int                length) const;

    void aliasMessage(bsl::shared_ptr<bdlbb::Blob>* appData,
                      bsl::shared_ptr<bdlbb::Blob>* options,
                      const DataStoreRecord&        record) const;

    /// Attempt to garbage-collect messages for which TTL has expired where
    /// the specified `currentTimeUtc` is the current timestamp (UTC).
    /// Return `true`, if there are expired items unprocessed because of the
    /// batch size limitation.   Note that this routine is no-op unless at
    /// the primary node".
    bool gcExpiredMessages(const bdlt::Datetime& currentTimeUtc);

    /// Delete an history of guids or messages maintained by this data
    /// store.  Return `true`, if there are expired items unprocessed
    /// because of the batch size limitation.  Note that this routine is
    /// invoked at primary as well as replica nodes.
    bool gcHistory();

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(FileStore, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an instance with the specified `config`, `processorId`,
    /// `dispatcher`, `cluster`, `clusterStats`, `blobSpPool`,
    /// `miscWorkThreadPool`, `isCSLModeEnabled`, `isFSMWorkflow` and
    /// `allocator`.
    FileStore(const DataStoreConfig&  config,
              int                     processorId,
              mqbi::Dispatcher*       dispatcher,
              mqbnet::Cluster*        cluster,
              mqbstat::ClusterStats*  clusterStats,
              BlobSpPool*             blobSpPool,
              StateSpPool*            statePool,
              bdlmt::FixedThreadPool* miscWorkThreadPool,
              bool                    isCSLModeEnabled,
              bool                    isFSMWorkflow,
              int                     replicationFactor,
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

    /// Called by the dispatcher to flush any pending operation.. mainly
    /// used to provide batch and nagling mechanism.
    void flush() BSLS_KEYWORD_OVERRIDE;

    /// Return a pointer to the dispatcher this client is associated with.
    mqbi::Dispatcher* dispatcher() BSLS_KEYWORD_OVERRIDE;

    /// Return a reference to the dispatcherClientData.
    mqbi::DispatcherClientData& dispatcherClientData() BSLS_KEYWORD_OVERRIDE;

    void dispatchEvent(mqbi::DispatcherEvent* event);

    /// Execute the specified `functor`, using the `e_CALLBACK` event
    /// type, in the processor associated to this object.
    void execute(const mqbi::Dispatcher::VoidFunctor& functor);

    // MANIPULATORS

    /// Open this instance using the specified `queueKeyInfoMap`. Return
    /// zero on success, non-zero value otherwise.
    int open(const QueueKeyInfoMap& queueKeyInfoMap = QueueKeyInfoMap())
        BSLS_KEYWORD_OVERRIDE;

    /// Close this instance.  If the optional `flush` flag is true, flush
    /// the data store to disk.
    void close(bool flush = false) BSLS_KEYWORD_OVERRIDE;

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
                                 const AppIdKeyPairs&    appIdKeyPairs,
                                 bsls::Types::Uint64     timestamp,
                                 bool isNewQueue) BSLS_KEYWORD_OVERRIDE;

    int
    writeQueuePurgeRecord(DataStoreRecordHandle*  handle,
                          const mqbu::StorageKey& queueKey,
                          const mqbu::StorageKey& appKey,
                          bsls::Types::Uint64 timestamp) BSLS_KEYWORD_OVERRIDE;

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
                             SyncPointType::Enum type) BSLS_KEYWORD_OVERRIDE;

    /// Remove the record identified by the specified `handle`.  Return zero
    /// on success, non-zero value if `handle` is invalid.  Behavior is
    /// undefined unless `handle` represents a record in the data store.
    int
    removeRecord(const DataStoreRecordHandle& handle) BSLS_KEYWORD_OVERRIDE;

    /// Remove the record identified by the specified `handle`.  The
    /// behavior is undefined unless `handle` is valid and represents a
    /// record in the data store.
    void
    removeRecordRaw(const DataStoreRecordHandle& handle) BSLS_KEYWORD_OVERRIDE;

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
    int issueSyncPoint() BSLS_KEYWORD_OVERRIDE;

    /// Set the specified `primaryNode` with the specified `primaryLeaseId`
    /// as the active primary for this data store partition.  Note that
    /// `primaryNode` could refer to the node which owns this data store.
    void setActivePrimary(mqbnet::ClusterNode* primaryNode,
                          unsigned int primaryLeaseId) BSLS_KEYWORD_OVERRIDE;

    /// Clear the current primary associated with this partition.
    void clearPrimary() BSLS_KEYWORD_OVERRIDE;

    /// If the specified `storage` is `true`, flush any buffered replication
    /// messages to the peers.  If the specified `queues` is `true`, `flush`
    /// all associated queues.  Behavior is undefined unless this node is
    /// the primary for this partition.
    void dispatcherFlush(bool storage, bool queues) BSLS_KEYWORD_OVERRIDE;

    /// Call `onReplicatedBatch` on all associated queues if the storage
    /// builder is empty (just flushed).
    void notifyQueuesOnReplicatedBatch();

    /// Invoke the specified `functor` with each queue associated to the
    /// partition represented by this FileStore if the partition was
    /// successfully opened.  The behavior is undefined unless invoked from
    /// the queue thread corresponding to this partition.
    void applyForEachQueue(const QueueFunctor& functor) const;

    /// mqbs::FileStore specific MANIPULATORS

    /// Deprecate the active file set.  Behavior is undefined unless this
    /// instance is closed.
    ///
    /// NOTE: This routine is dangerous and archives storage files. Must be
    /// used with caution.
    void deprecateFileSet();

    /// Initiate a forced rollover of this partition.
    void forceRollover();

    void registerStorage(ReplicatedStorage* storage);

    void unregisterStorage(const ReplicatedStorage* storage);

    void cancelTimersAndWait();

    void processShutdownEvent();

    /// Set the availability of this partition to the specified boolean
    /// `value`.
    void setAvailabilityStatus(bool value);

    /// Set the replication factor for strong consistency to `factor`.
    void setReplicationFactor(int factor);

    /// Set the ignore Crc32c flag to the specified `value`.  We should only
    /// set this to true during testing.
    void setIgnoreCrc32c(bool value);

    // This will be used as Implicit Receipt
    void setLastStrongConsistency(unsigned int        primaryLeaseId,
                                  bsls::Types::Uint64 sequenceNum);

    /// Load into the specified `storages` the list of queue storages for
    /// which all filters from the specified `filters` are returning true.
    void getStorages(StorageList*          storages,
                     const StorageFilters& filters) const;

    /// Load the summary of this partition to the specified `fileStore`
    /// object.
    ///
    /// THREAD: Executed by the queue dispatcher thread associated with the
    ///         specified `fileStore`'s partitionId.
    void loadSummary(mqbcmd::FileStore* fileStore) const;

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
    const bsl::string& description() const BSLS_KEYWORD_OVERRIDE;

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

    /// Return the current sequence number for this partition.
    bsls::Types::Uint64 sequenceNumber() const;

    bool inDispatcherThread() const;

    /// Return the replication factor for strong consistency.
    int replicationFactor() const;
};

// =======================
// class FileStoreIterator
// =======================

/// Mechanism to iterate over records in a `mqbs::FileStore`.
class FileStoreIterator {
  private:
    // PRIVATE TYPES
    typedef DataStoreConfig::RecordIterator RecordIterator;

  private:
    // DATA
    FileStore* d_store_p;  // Held

    bool d_firstInvocation;

    RecordIterator d_iterator;

  public:
    // CREATORS

    /// Behavior is undefined unless specified `store` outlives this
    /// instance.
    explicit FileStoreIterator(FileStore* store);

    // MANIPULATORS
    bool next();

    // ACCESSORS

    /// Behavior undefined unless last call to `next` returned true.
    RecordType::Enum type() const;

    /// Behavior undefined unless last call to `next` returned true.
    DataStoreRecordHandle handle() const;

    /// Behavior undefined unless last call to `next` returned true.
    void loadMessageRecord(MessageRecord* buffer) const;

    /// Behavior undefined unless last call to `next` returned true.
    void loadConfirmRecord(ConfirmRecord* buffer) const;

    /// Behavior undefined unless last call to `next` returned true.
    void loadDeletionRecord(DeletionRecord* buffer) const;

    /// Behavior undefined unless last call to `next` returned true.
    void loadQueueOpRecord(QueueOpRecord* buffer) const;

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
bsl::ostream& operator<<(bsl::ostream& stream, const FileStoreIterator& rhs);

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

inline FileStore::NodeContext::NodeContext(bdlbb::BlobBufferFactory* factory,
                                           const DataStoreRecordKey& key,
                                           bslma::Allocator* basicAllocator)
: d_key(key)
, d_blob_sp(new(*basicAllocator) bdlbb::Blob(factory, basicAllocator),
            basicAllocator)
{
    // NOTHING
}

// ---------------
// class FileStore
// ---------------

// PRIVATE MANIPULATORS
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

inline void FileStore::recordIteratorToHandle(DataStoreRecordHandle* handle,
                                              const RecordIterator   recordIt)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(handle);

    RecordIterator& recordItRef = *reinterpret_cast<RecordIterator*>(handle);
    recordItRef                 = recordIt;
}

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

inline void FileStore::dispatchEvent(mqbi::DispatcherEvent* event)
{
    dispatcher()->dispatchEvent(event, this);
}

inline void FileStore::execute(const mqbi::Dispatcher::VoidFunctor& functor)
{
    dispatcher()->execute(functor,
                          this,
                          mqbi::DispatcherEventType::e_CALLBACK);
}

inline bool FileStore::inDispatcherThread() const
{
    return dispatcher()->inDispatcherThread(this);
}

// MANIPULATORS
inline void FileStore::setIgnoreCrc32c(bool value)
{
    d_ignoreCrc32c = value;
}

inline void
FileStore::setLastStrongConsistency(unsigned int        primaryLeaseId,
                                    bsls::Types::Uint64 sequenceNum)
{
    d_lastRecoveredStrongConsistency.d_primaryLeaseId = primaryLeaseId;
    d_lastRecoveredStrongConsistency.d_sequenceNum    = sequenceNum;
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

inline const bsl::string& FileStore::description() const
{
    return partitionDesc();
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
    return d_cluster_p->nodes().size();
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

inline unsigned int FileStore::primaryLeaseId() const
{
    return d_primaryLeaseId;
}

inline bsls::Types::Uint64 FileStore::sequenceNumber() const
{
    return d_sequenceNum;
}

inline int FileStore::replicationFactor() const
{
    return d_replicationFactor;
}

// -----------------------
// class FileStoreIterator
// -----------------------

// CREATORS
inline FileStoreIterator::FileStoreIterator(FileStore* store)
: d_store_p(store)
, d_firstInvocation(true)
, d_iterator()
{
    BSLS_ASSERT_SAFE(d_store_p);
}

// ACCESSORS
inline RecordType::Enum FileStoreIterator::type() const
{
    return d_iterator->second.d_recordType;
}

inline DataStoreRecordHandle FileStoreIterator::handle() const
{
    // Here we use our knowledge of internal layout of 'DataStoreRecordHandle'.
    DataStoreRecordHandle result;
    RecordIterator& recordItRef = *reinterpret_cast<RecordIterator*>(&result);
    recordItRef                 = d_iterator;
    return result;
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& mqbs::operator<<(bsl::ostream&            stream,
                                      const FileStoreIterator& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
