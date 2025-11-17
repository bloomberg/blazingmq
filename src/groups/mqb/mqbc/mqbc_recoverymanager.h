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

// mqbc_recoverymanager.h                                             -*-C++-*-
#ifndef INCLUDED_MQBC_RECOVERYMANAGER
#define INCLUDED_MQBC_RECOVERYMANAGER

/// @file mqbc_recoverymanager.h
///
/// @brief Provide a mechanism to manage storage recovery in a cluster node.
///
/// @bbref{mqbc::RecoveryManager} provides a mechanism to manage storage
/// recovery in a cluster node.

// MQB
#include <mqbc_clusterdata.h>
#include <mqbcfg_messages.h>
#include <mqbnet_cluster.h>
#include <mqbs_datastore.h>
#include <mqbs_filestore.h>
#include <mqbs_filestoreset.h>
#include <mqbs_mappedfiledescriptor.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqp_requestmanager.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_keyword.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace mqbc {
// =====================
// class RecoveryManager
// =====================

/// This component provides a mechanism to manage storage recovery in a cluster
/// node.
class RecoveryManager {
  public:
    // TYPES
    /// Pool of shared pointers to Blobs
    typedef mqbs::FileStore::BlobSpPool BlobSpPool;

  private:
    // ==================
    // class ChunkDeleter
    // ==================

    /// Private class.  Implementation detail of @bbref{mqbc::RecoveryManager}.
    /// This class provides a custom deleter for a chunk of file aliasing to
    /// the mapped region.
    class ChunkDeleter {
      private:
        // DATA
        bsl::shared_ptr<mqbs::MappedFileDescriptor> d_mfd_sp;
        bsl::shared_ptr<bsls::AtomicInt>            d_counter_sp;

      public:
        // CREATORS

        /// Create a chunk deleter object with the specified `mfd` mapped
        /// file descriptor and specified `counter`.  Increment `counter`.
        explicit ChunkDeleter(
            const bsl::shared_ptr<mqbs::MappedFileDescriptor>& mfd,
            const bsl::shared_ptr<bsls::AtomicInt>&            counter);

        // ACCESSORS

        /// Functor which will close the underlying mapped file descriptor
        /// owned by this chunk deleter object, if decrementing the
        /// underlying counter makes it reach 0.
        void operator()(const void* ptr = 0) const;
    };

    // ========================
    // class ReceiveDataContext
    // ========================

    /// Private class.  Implementation detail of @bbref{mqbc::RecoveryManager}.
    /// This class contains important information to keep track of when
    /// receiving data chunks from an up-to-date node during recovery, such as
    /// the recovery data source, range of sequence numbers to recover, and
    /// current sequence number offset.
    class ReceiveDataContext {
      public:
        // TYPES
        typedef bsl::vector<bsl::shared_ptr<bdlbb::Blob> > StorageEvents;

      public:
        // DATA

        /// Peer node from which we are receiving recovery data.
        mqbnet::ClusterNode* d_recoveryDataSource_p;

        /// Whether self is expecting recovery data chunks.
        bool d_expectChunks;

        /// Id of the ReplicaDataRequest which signals the expectation of
        /// recovery data chunks.  This value is only meaningful if we are a
        /// replica receiving data from the primary.
        int d_recoveryRequestId;

        /// Beginning sequence number of recovery data chunks.  Note that self
        /// already contains message with this sequence number.  The first
        /// recovery data chunk is expected to have sequence number
        /// `d_beginSeqNum + 1`.
        bmqp_ctrlmsg::PartitionSequenceNumber d_beginSeqNum;

        /// Expected ending sequence number of recovery data chunks.
        bmqp_ctrlmsg::PartitionSequenceNumber d_endSeqNum;

        /// Self's current sequence number.
        bmqp_ctrlmsg::PartitionSequenceNumber d_currSeqNum;

      public:
        // CREATORS

        /// Create a default `ReceiveDataContext` object.
        ReceiveDataContext();

        /// Create a `ReceiveDataContext` object copying the specified `other`.
        ReceiveDataContext(const ReceiveDataContext& other);

        // MANIPULATORS

        /// Reset the members of this object.
        void reset();
    };

    // =====================
    // class RecoveryContext
    // =====================

    /// Private class.  Implementation detail of @bbref{mqbc::RecoveryManager}.
    /// This class contains important information to keep track during
    /// recovery, such as recovery file set, mapped journal/data fds, buffered
    /// storage events, and receive data context.
    class RecoveryContext {
      public:
        // TYPES
        typedef bsl::vector<bsl::shared_ptr<bdlbb::Blob> > StorageEvents;

      public:
        // DATA

        /// Recovery file set.
        mqbs::FileStoreSet d_recoveryFileSet;

        /// Journal file descriptor to use for recovery.
        mqbs::MappedFileDescriptor d_mappedJournalFd;

        /// Write offset of the journal file.
        bsls::Types::Uint64 d_journalFilePosition;

        /// Data file descriptor to use for recovery.
        mqbs::MappedFileDescriptor d_mappedDataFd;

        /// Write offset of the data file.
        bsls::Types::Uint64 d_dataFilePosition;

        /// QList file descriptor to use for recovery.
        mqbs::MappedFileDescriptor d_mappedQlistFd;

        /// Write offset of the QList file.
        bsls::Types::Uint64 d_qlistFilePosition;

        /// Peer node from which we are receiving live data.
        mqbnet::ClusterNode* d_liveDataSource_p;

        /// List of storage events which are buffered while recovery is in
        /// progress.  Once recovery is complete, these events are applied to
        /// bring the node up-to-date with this partition.
        StorageEvents d_bufferedEvents;

        /// Receive data context.
        ReceiveDataContext d_receiveDataContext;

        /// First sync point after rollover sequence number.
        bmqp_ctrlmsg::PartitionSequenceNumber
            d_firstSyncPointAfterRolloverSeqNum;

      public:
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(RecoveryContext,
                                       bslma::UsesBslmaAllocator)

        // CREATORS

        /// Create a default `RecoveryContext` object, using the specified
        /// `basicAllocator` for memory allocations.
        RecoveryContext(bslma::Allocator* basicAllocator = 0);

        /// Create a `RecoveryContext` object copying the specified 'other',
        /// using the specified `basicAllocator` for memory allocations.
        RecoveryContext(const RecoveryContext& other,
                        bslma::Allocator*      basicAllocator = 0);

        // MANIPULATORS

        /// Reset the members of this object.
        void reset();
    };

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBC.RECOVERYMANAGER");

  private:
    // PRIVATE TYPES

    /// Callback provided by @bbref{mqbc::StorageManager} to this component to
    /// indicate the status of sendDataChunks to the specified `destination`
    /// i.e. peer to which current node is sending data for the specified
    /// `partitionId`. The status is as per the specified `status`.
    typedef bsl::function<
        void(int partitionId, mqbnet::ClusterNode* destination, int status)>
        PartitionDoneSendDataChunksCb;

    /// Vector per partition of `RecoveryContext`.
    typedef bsl::vector<RecoveryContext> RecoveryContextVec;

    // This callback is only used when the self node is a replica.
    bsl::function<
        void(int partitionId, mqbnet::ClusterNode* destination, int status)>
        PartitionDoneRcvDataChunksCb;

  private:
    // DATA

    /// Allocator to use
    bslma::Allocator* d_allocator_p;

    bool d_qListAware;
    // Whether the broker still reads and writes to the to-be-deprecated Qlist
    // file.

    /// Blob shared pointer pool to use
    BlobSpPool* d_blobSpPool_p;

    /// Cluster configuration to use
    const mqbcfg::ClusterDefinition& d_clusterConfig;

    /// Configuration for file store to use
    const mqbs::DataStoreConfig d_dataStoreConfig;

    /// Associated non-persistent cluster data for this node
    const mqbc::ClusterData& d_clusterData;

    /// Vector per partition which maintains information about
    /// `RecoveryContext`.
    ///
    /// THREAD: Except during the ctor, the i-th index of this data member
    ///         **must** be accessed in the associated Queue dispatcher thread
    ///         for the i-th partitionId.
    RecoveryContextVec d_recoveryContextVec;

  private:
    // NOT IMPLEMENTED
    RecoveryManager(const RecoveryManager&) BSLS_KEYWORD_DELETED;
    RecoveryManager& operator=(const RecoveryManager&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RecoveryManager, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `RecoveryManager` object with the specified `clusterConfig`,
    /// `clusterData` and `dataStoreConfig`. Use the specified `allocator`
    /// for any memory allocation.
    RecoveryManager(const mqbcfg::ClusterDefinition& clusterConfig,
                    mqbc::ClusterData&               clusterData,
                    const mqbs::DataStoreConfig&     dataStoreConfig,
                    bslma::Allocator*                allocator);

    /// Destroy this object.
    ~RecoveryManager();

    // MANIPULATORS

    /// Start the component. Incase of errors, use the specified
    /// `errorDescription`.
    int start(bsl::ostream& errorDescription);

    /// Stop the component which includes cleanup of any asynchronous
    /// uncompleted events.
    void stop();

    /// Deprecate the active file set of the specified `partitionId`, called
    /// when self's storage is out of sync with primary and cannot be healed
    /// trivially.
    ///
    /// THREAD: Executed by the queue dispatcher thread associated with the
    /// specified `partitionId`.
    void deprecateFileSet(int partitionId);

    /// Set the expected receive data chunk range for the specified
    /// 'partitionId' to be from the specified 'source' from the specified
    /// 'beginSeqNum' to the specified 'endSeqNum', based on information
    /// from the optionally specified 'requestId'.  If the specified 'fs' is
    /// not open, ensure that the journal and data files in the recovery
    /// file set is open.
    ///
    /// THREAD: Executed by the queue dispatcher thread associated with the
    /// specified 'partitionId'.
    void setExpectedDataChunkRange(
        int                                          partitionId,
        const mqbs::FileStore&                       fs,
        mqbnet::ClusterNode*                         source,
        const bmqp_ctrlmsg::PartitionSequenceNumber& beginSeqNum,
        const bmqp_ctrlmsg::PartitionSequenceNumber& endSeqNum,
        int                                          requestId = -1);

    /// Reset the receive data context for the specified `partitionId.`
    void resetReceiveDataCtx(int partitionId);

    /// Send data chunks for the specified `partitionId` to the specified
    /// `destination` starting from specified `beginSeqNum` upto specified
    /// `endSeqNum` using data from specified `fs`. Send the status of this
    /// operation back to the caller using the specified `doneDataChunksCb`.
    /// Note, we mmap the files for every call to this function. Return 0 on
    /// success and non-zero otherwise.
    ///
    /// THREAD: Executed in the dispatcher thread associated with the
    /// specified `partitionId`.
    int processSendDataChunks(
        int                                          partitionId,
        mqbnet::ClusterNode*                         destination,
        const bmqp_ctrlmsg::PartitionSequenceNumber& beginSeqNum,
        const bmqp_ctrlmsg::PartitionSequenceNumber& endSeqNum,
        const mqbs::FileStore&                       fs,
        PartitionDoneSendDataChunksCb                doneDataChunksCb);

    /// Process the recovery data chunks contained in the specified `blob`
    /// sent by the specified `source` for the specified `partitionId`.
    /// Forward the processing to the specified `fs` if `fs` is open. Use the
    /// specified `firstSyncPointAfterRolloverSeqNum` to update journal header.
    /// Return 0 on success and non-zero code on error.
    ///
    /// THREAD: Executed in the dispatcher thread associated with the
    /// specified `partitionId`.
    int processReceiveDataChunks(const bsl::shared_ptr<bdlbb::Blob>& blob,
                                 mqbnet::ClusterNode*                source,
                                 mqbs::FileStore*                    fs,
                                 int partitionId,
                                 const bmqp_ctrlmsg::PartitionSequenceNumber&
                                     firstSyncPointAfterRolloverSeqNum);

    /// Create the internal recovery file set for the specified
    /// `partitionId`, using the specified `fs`.  Return 0 on success, non
    /// zero value otherwise along with populating the specified
    /// `errorDescription` with a brief reason for logging purposes.
    ///
    /// THREAD: Executed in the dispatcher thread associated with the
    /// specified `partitionId`.
    int createRecoveryFileSet(bsl::ostream&    errorDescription,
                              mqbs::FileStore* fs,
                              int              partitionId);

    /// Retrieve the appropriate journal fd + position, data fd + position,
    /// and recovery file set belonging to the specified `partitionId`.
    /// Return 0 on success, non zero value otherwise along with populating
    /// the specified `errorDescription` with a brief reason for logging
    /// purposes.  Note that a return value of `1` is special and indicates
    /// that no recovery file set is found.
    ///
    /// THREAD: Executed in the dispatcher thread associated with the
    /// specified `partitionId`.
    int openRecoveryFileSet(bsl::ostream& errorDescription, int partitionId);

    /// Close the recovery file set for the specified 'partitionId'.  Return
    /// 0 on success, non zero value otherwise.
    ///
    /// THREAD: Executed in the dispatcher thread associated with the
    /// specified `partitionId`.
    int closeRecoveryFileSet(int partitionId);

    /// Recover latest sequence number from storage for the specified
    /// `partitionId` and populate the output in the specified `seqNum`.
    /// If `firstSyncPointAfterRolllover` is true, recover the first sync point
    /// after rollover sequence number instead of the latest sequence number.
    /// Return 0 on success and non-zero otherwise.
    ///
    /// THREAD: Executed in the dispatcher thread associated with the
    /// specified `partitionId`.
    int recoverSeqNum(bmqp_ctrlmsg::PartitionSequenceNumber* seqNum,
                      int                                    partitionId,
                      bool firstSyncPointAfterRolllover = false);

    /// Return recovered partition max file sizes for the specified 
    /// `partitionId`.
    ///
    /// THREAD: Executed in the dispatcher thread associated with te
    /// specified `partitionId`.
    bmqp_ctrlmsg::PartitionMaxFileSizes recoverPartitionMaxFileSizes(int partitionId);

    /// Set the live data source of the specified 'partitionId' to the
    /// specified 'source', and clear any existing buffered storage events.
    ///
    /// THREAD: Executed in the dispatcher thread associated with the
    /// specified `partitionId`.
    void setLiveDataSource(mqbnet::ClusterNode* source, int partitionId);

    /// Buffer the storage event for the specified `partitionId` contained
    /// in the specified `blob` sent from the specified `source`.
    ///
    /// THREAD: Executed in the dispatcher thread associated with the
    /// specified `partitionId`.
    void bufferStorageEvent(int                                 partitionId,
                            const bsl::shared_ptr<bdlbb::Blob>& blob,
                            mqbnet::ClusterNode*                source);

    /// Load into the specified `out` all buffered storage events for the
    /// specified `partitionId`, verifying that they are sent from the
    /// specified `source`, then clear the buffer.  Return 0 on success and
    /// non-zero otherwise.
    ///
    /// THREAD: Executed in the dispatcher thread associated with the
    /// specified `partitionId`.
    int
    loadBufferedStorageEvents(bsl::vector<bsl::shared_ptr<bdlbb::Blob> >* out,
                              const mqbnet::ClusterNode* source,
                              int                        partitionId);

    /// Cleared all buffered storage event for the specified `partitionId`.
    ///
    /// THREAD: Executed in the dispatcher thread associated with the
    /// specified `partitionId`.
    void clearBufferedStorageEvent(int partitionId);

    // ACCESSORS

    /// Return true if the specified `partitionId` is expecting data chunks,
    /// false otherwise.
    ///
    /// THREAD: Executed in the dispatcher thread associated with the
    /// specified `partitionId`.
    bool expectedDataChunks(int partitionId) const;

    /// Load into the specified `out` a ReplicaDataResponsePush using
    /// information in self's ReceiveDataContext for the specified
    /// `partitionId`.
    ///
    /// THREAD: Executed in the dispatcher thread associated with the
    /// specified `partitionId`.
    void loadReplicaDataResponsePush(bmqp_ctrlmsg::ControlMessage* out,
                                     int partitionId) const;
};

// ============================================================================
//                            INLINE DEFINITIONS
// ============================================================================

// ------------------
// class ChunkDeleter
// ------------------

// CREATORS
inline RecoveryManager::ChunkDeleter::ChunkDeleter(
    const bsl::shared_ptr<mqbs::MappedFileDescriptor>& mfd,
    const bsl::shared_ptr<bsls::AtomicInt>&            counter)
: d_mfd_sp(mfd)
, d_counter_sp(counter)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_mfd_sp);
    BSLS_ASSERT_SAFE(d_counter_sp);

    ++(*d_counter_sp);
}

// ------------------------
// class ReceiveDataContext
// ------------------------

// CREATORS
inline RecoveryManager::ReceiveDataContext::ReceiveDataContext()
: d_recoveryDataSource_p(0)
, d_expectChunks(false)
, d_recoveryRequestId(-1)
, d_beginSeqNum()
, d_endSeqNum()
, d_currSeqNum()
{
    // NOTHING
}

inline RecoveryManager::ReceiveDataContext::ReceiveDataContext(
    const ReceiveDataContext& other)
: d_recoveryDataSource_p(other.d_recoveryDataSource_p)
, d_expectChunks(other.d_expectChunks)
, d_recoveryRequestId(other.d_recoveryRequestId)
, d_beginSeqNum(other.d_beginSeqNum)
, d_endSeqNum(other.d_endSeqNum)
, d_currSeqNum(other.d_currSeqNum)
{
    // NOTHING
}

// ---------------------
// class RecoveryContext
// ---------------------

// CREATORS
inline RecoveryManager::RecoveryContext::RecoveryContext(
    bslma::Allocator* basicAllocator)
: d_recoveryFileSet(basicAllocator)
, d_mappedJournalFd()
, d_journalFilePosition(0)
, d_mappedDataFd()
, d_dataFilePosition(0)
, d_mappedQlistFd()
, d_qlistFilePosition(0)
, d_liveDataSource_p(0)
, d_bufferedEvents(basicAllocator)
, d_receiveDataContext()
, d_firstSyncPointAfterRolloverSeqNum()
{
    // NOTHING
}

inline RecoveryManager::RecoveryContext::RecoveryContext(
    const RecoveryContext& other,
    bslma::Allocator*      basicAllocator)
: d_recoveryFileSet(other.d_recoveryFileSet, basicAllocator)
, d_mappedJournalFd(other.d_mappedJournalFd)
, d_journalFilePosition(other.d_journalFilePosition)
, d_mappedDataFd(other.d_mappedDataFd)
, d_dataFilePosition(other.d_dataFilePosition)
, d_mappedQlistFd(other.d_mappedQlistFd)
, d_qlistFilePosition(other.d_qlistFilePosition)
, d_liveDataSource_p(other.d_liveDataSource_p)
, d_bufferedEvents(other.d_bufferedEvents)
, d_receiveDataContext(other.d_receiveDataContext)
, d_firstSyncPointAfterRolloverSeqNum(
      other.d_firstSyncPointAfterRolloverSeqNum)
{
    // NOTHING
}

// ---------------------
// class RecoveryManager
// ---------------------

// ACCESSORS
inline bool RecoveryManager::expectedDataChunks(int partitionId) const
{
    // executed by the *QUEUE DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId <
                         d_clusterConfig.partitionConfig().numPartitions());

    return d_recoveryContextVec[partitionId]
        .d_receiveDataContext.d_expectChunks;
}

}  // close package namespace
}  // close enterprise namespace

#endif
