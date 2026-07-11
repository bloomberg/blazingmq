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

#ifndef INCLUDED_MQBS_DATASTORE
#define INCLUDED_MQBS_DATASTORE

//@PURPOSE: Provide an interface for a BlazingMQ data store.
//
//@CLASSES:
//  mqbs::DataStoreRecordFlag:     Status of a record in data store.
//  mqbs::DataStoreRecordFlagUtil: 'mqbs::DataStoreRecordFlag' utility
//  mqbs::DataStoreRecord:         A record in data store.
//  mqbs::DataStoreConfig:         Configuration of a data store.
//  mqbs::DataStoreRecordHandle:   VST handle to a 'mqbs::DataStoreRecord'
//  mqbs::DataStore:               Interface for a BlazingMQ data store.
//
//@SEE ALSO: mqbs::FileStore
//
//@DESCRIPTION: 'mqbs::DataStore' provides an interface for a BlazingMQ data
// store.  Note that the main motivation for this interface is to make
// BlazingMQ storage mechanism testable.

// MQB
#include <mqbi_dispatcher.h>
#include <mqbi_storage.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqc_orderedhashmap.h>
#include <bmqt_messageguid.h>
#include <bmqt_uri.h>

#include <bmqc_orderedhashmap.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlmt_eventscheduler.h>
#include <bdlt_datetime.h>
#include <bsl_cstring.h>
#include <bsl_functional.h>
#include <bsl_list.h>
#include <bsl_map.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_type_traits.h>
#include <bsl_unordered_map.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslh_hash.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_isbitwiseequalitycomparable.h>
#include <bslmf_istriviallycopyable.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_systemclocktype.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATIONS
namespace bdlmt {
class EventScheduler;
}
namespace mqbi {
class Domain;
}
namespace bmqp_ctrlmsg {
class SyncPoint;
}
namespace mqbcmd {
class FileStore;
}
namespace mqbnet {
class ClusterNode;
}
namespace mqbs {
class ReplicatedStorage;
}

namespace mqbs {

// ======================
// struct DataStoreRecord
// ======================

/// This component provides a VST representing a record in the in-memory
/// queue of an instance of a concrete implementation of `mqbs::DataStore`.
struct DataStoreRecord {
  public:
    // PUBLIC DATA
    RecordType::Enum d_recordType;  // Type of the journal record

    mutable bool d_hasReceipt;
    // Strong consistency receipt.

    bsls::Types::Uint64 d_recordOffset;  // Offset of record in journal

    bsls::Types::Uint64 d_messageOffset;
    // Offset of the message in the DATA
    // file.  Zero unless d_recordType =
    // e_MESSAGE.  Note that this offset
    // represents the beginning of the
    // `mqbs::DataHeader` struct for the
    // message.

    unsigned int d_appDataUnpaddedLen;
    // Length (unpadded) of the app
    // data.  Zero unless d_recordType =
    // e_MESSAGE Note that this length
    // represents the size of application
    // data (ie, it skips the
    // `mqbs::DataHeader` and the options
    // area).

    unsigned int d_dataOrQlistRecordPaddedLen;
    // Length of the *entire* record if it
    // appears in the DATA or QLIST file. A
    // record appears in DATA file if
    // d_recordType == MESSAGE.  A record
    // appears in QLIST file if
    // d_recordType == QUEUE_OP *and*
    // QueueSubOpType == CREATE.  Note that
    // QueueSubOpType is not captured in
    // this structure.  If d_recordType ==
    // QUEUE_OP but QueueSubOpType !=
    // CREATE, this field must be
    // initialized to 0.  For any other
    // type, this field must be set to zero
    // as well.  Also note that when valid,
    // the length will include the
    // padding.

    bmqp::MessagePropertiesInfo d_messagePropertiesInfo;
    // Used only if d_recordType ==
    // e_MESSAGE

    bsls::Types::Int64 d_arrivalTimepoint;
    // Arrival timepoint of the message, in
    // nanoseconds from an arbitrary but
    // fixed point in time.  Note that this
    // field is meaningful only inside a
    // process, and only at the primary
    // node.  Also note that a zero
    // represents an unset value.  Lastly,
    // this field is used only if
    // d_recordType == e_MESSAGE.

    bsls::Types::Uint64 d_arrivalTimestamp;
    // Arrival timestamp of the message,
    // in seconds from epoch).  Used only
    // if d_recordType == e_MESSAGE

    // CREATORS
    DataStoreRecord();
    DataStoreRecord(RecordType::Enum    recordType,
                    bsls::Types::Uint64 recordOffset);
    DataStoreRecord(RecordType::Enum    recordType,
                    bsls::Types::Uint64 recordOffset,
                    unsigned int        dataOrQlistRecordPaddedLen);

    /// Create a placeholder record of the specified `recordType` with a zero
    /// offset and `d_hasReceipt` set to `false`.  Used to reserve a record
    /// (and thus a handle) for a write buffered during a rollover window; its
    /// offsets are patched in place when the write later drains to disk.
    explicit DataStoreRecord(RecordType::Enum recordType);

    // ACCESSORS
    RecordType::Enum type() const;
};

// =========================
// struct DataStoreRecordKey
// =========================

struct DataStoreRecordKey {
  public:
    // PUBLIC DATA
    bsls::Types::Uint64 d_sequenceNum;

    unsigned int d_primaryLeaseId;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DataStoreRecordKey,
                                   bslmf::IsBitwiseEqualityComparable)
    BSLMF_NESTED_TRAIT_DECLARATION(DataStoreRecordKey,
                                   bsl::is_trivially_copyable)

    // CREATORS
    DataStoreRecordKey();

    DataStoreRecordKey(const bsls::Types::Uint64 sequenceNum,
                       unsigned int              primaryLeaseId);

    // ACCESSORS

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

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const DataStoreRecord& value);

bsl::ostream& operator<<(bsl::ostream&             stream,
                         const DataStoreRecordKey& value);

/// Return `true` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return false otherwise.
bool operator==(const DataStoreRecordKey& lhs, const DataStoreRecordKey& rhs);

/// Return `false` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return `true` otherwise.
bool operator!=(const DataStoreRecordKey& lhs, const DataStoreRecordKey& rhs);

/// Operator used to allow comparison between the specified `lhs` and `rhs`
/// CorrelationId objects so that CorrelationId can be used as key in a map.
bool operator<(const DataStoreRecordKey& lhs, const DataStoreRecordKey& rhs);

// FREE FUNCTIONS
template <class HASH_ALGORITHM>
void hashAppend(HASH_ALGORITHM& hashAlgo, const DataStoreRecordKey& key);

// ================================
// class DataStoreRecordKeyHashAlgo
// ================================

/// This class provides a hashing algorithm for `mqbs::DataStoreRecordKey`.
/// It provides an `almost` identity hash function.  Note that this class
/// provides a `hashing algorithm wrapper` (ie, a custom version of
/// `bslh::Hash<>`) in `bslh` framework lingo (see BDE "Modular Hashing"
/// document).  Note that this class is not templatized on a
/// `HASHING_ALGORITHM` (unlike recommended in the document).
class DataStoreRecordKeyHashAlgo {
  public:
    // TYPES
    typedef bsls::Types::Uint64 result_type;

    // ACCESSORS
    template <class TYPE>
    result_type operator()(const TYPE& type) const;
};

// =============================
// struct DataStoreRecordKeyLess
// =============================

struct DataStoreRecordKeyLess {
    // ACCESSORS

    /// Return `true` if the specified `lhs` should be considered as having
    /// a value less than the specified `rhs`.
    bool operator()(const DataStoreRecordKey& lhs,
                    const DataStoreRecordKey& rhs) const;
};

// ==============================
// class DataStoreConfigQueueInfo
// ==============================

/// This component provides a VST to capture basic information about a queue
/// recovered from the storage.
class DataStoreConfigQueueInfo {
  public:
    // TYPES
    typedef bmqc::OrderedHashMap<mqbu::StorageKey, bsl::string> AppInfos;

    /// Collection of intervals [`first`, `second`) where `first` is the start
    /// and `second` is the end of Purge range for some App that does not exist
    /// anymore - a "ghost" App.
    /// In other words, `first` is the oldest message which is younger than
    /// some "ghost" App, and any message after `second` is not purged.
    typedef bsl::multimap<DataStoreRecordKey, DataStoreRecordKey> PurgeOps;
    typedef PurgeOps::const_iterator                              PurgeOp;

    /// A collection of all "ghost" App with corresponding last PurgeOp.
    typedef bsl::unordered_map<mqbu::StorageKey, PurgeOp> Ghosts;

  private:
    // DATA
    bsl::string d_canonicalUri;

    int d_partitionId;

    AppInfos d_appIdKeyPairs;

    /// Ghosts are Apps which got removed before the current recovery.
    Ghosts d_ghosts;

    /// Second pass of `FileStore::recoverMessages` will cache Purge intervals
    /// for ghost Apps.
    mutable PurgeOps d_purgeOps;

    /// This queue has QueueCreationRecord in in the journal.
    /// As opposed to the case when the queue is in the CSL only.
    bool d_isRecorded;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DataStoreConfigQueueInfo,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    explicit DataStoreConfigQueueInfo(bslma::Allocator* basicAllocator = 0);

    DataStoreConfigQueueInfo(const DataStoreConfigQueueInfo& other,
                             bslma::Allocator* basicAllocator = 0);

    // MANIPULATORS
    void setCanonicalQueueUri(const bsl::string& value);

    void setPartitionId(int value);

    void setAsRecorded();

    /// Save the specified `appId` and the specified `appKey` as a valid App
    /// for which a Storage will be created.  If the same key was specified in
    /// a previous `addPurgeOp` call, remove the App from the cache of Ghost
    /// Apps.
    void addAppInfo(const bsl::string&      appId,
                    const mqbu::StorageKey& appKey,
                    bool                    withCSL);

    /// Cache the Purge interval from the specified `start` to the specified
    /// `end` for the specified `key` unless the `key` was specified in a
    /// previous `addAppInfo` call.
    void addPurgeOp(const mqbu::StorageKey&   key,
                    const DataStoreRecordKey& start,
                    const DataStoreRecordKey& end);

    /// Return the number of previously cached Purge intervals which contain
    /// the specified `current` message.  Erase all Purge intervals whose end
    /// end is less than the `current` value.
    unsigned int advanceAndCount(const DataStoreRecordKey& current) const;

    // ACCESSORS
    const bsl::string& canonicalQueueUri() const;

    int partitionId() const;

    const AppInfos& appIdKeyPairs() const;

    bool isRecorded() const;
};

// =====================
// class DataStoreConfig
// =====================

/// This component provides a VST for configuration of `mqbs::FileStore`.
class DataStoreConfig {
  public:
    // TYPES
    typedef bsl::unordered_map<mqbu::StorageKey,
                               DataStoreConfigQueueInfo,
                               bslh::Hash<mqbu::StorageKeyHashAlgo> >
        QueueKeyInfoMap;

    typedef QueueKeyInfoMap::iterator            QueueKeyInfoMapIter;
    typedef QueueKeyInfoMap::const_iterator      QueueKeyInfoMapConstIter;
    typedef bsl::pair<QueueKeyInfoMapIter, bool> QueueKeyInfoMapInsertRc;

    typedef bmqc::OrderedHashMap<DataStoreRecordKey,
                                 DataStoreRecord,
                                 DataStoreRecordKeyHashAlgo>
        Records;

    typedef Records::iterator RecordIterator;

    typedef Records::const_iterator RecordConstIterator;

    typedef mqbi::Storage::AppInfos AppInfos;

    /// Replicas create/update/delete storage upon Replication events
    /// (queueCreationCb/queueDeletionCb).
    typedef bsl::function<void(int                     partitionId,
                               const bmqt::Uri&        uri,
                               const mqbu::StorageKey& queueKey,
                               const AppInfos&         appIdKeyPairs,
                               bool                    isNewQueue)>
        QueueCreationCb;

    typedef bsl::function<void(int                     partitionId,
                               const bmqt::Uri&        uri,
                               const mqbu::StorageKey& queueKey,
                               const mqbu::StorageKey& appKey)>
        QueueDeletionCb;

    /// Signature of callback used by `mqbs::FileStore` to indicate the list
    /// of file-backed queues (and metadata) retrieved during recovery
    /// phase.
    typedef bsl::function<void(int partitionId, const QueueKeyInfoMap* queues)>
        RecoveredQueuesCb;

  private:
    // DATA
    bdlbb::BlobBufferFactory* d_bufferFactory_p;

    bdlmt::EventScheduler* d_scheduler_p;

    bool d_preallocate;
    // Flag to indicate if file store
    // should attempt to pre-allocate files
    // on disk

    bool d_prefaultPages;
    // Flag to indicate whether to populate
    // (prefault) page tables for a
    // mapping.

    bslstl::StringRef d_location;

    bslstl::StringRef d_archiveLocation;

    bslstl::StringRef d_clusterName;

    int d_nodeId;

    int d_partitionId;

    bsls::Types::Uint64 d_maxDataFileSize;

    bsls::Types::Uint64 d_maxJournalFileSize;

    bsls::Types::Uint64 d_maxQlistFileSize;

    QueueCreationCb d_queueCreationCb;

    QueueDeletionCb d_queueDeletionCb;

    RecoveredQueuesCb d_recoveredQueuesCb;

    int d_maxArchivedFileSets;

  public:
    // CREATORS
    DataStoreConfig();

    // MANIPULATORS
    DataStoreConfig& setBufferFactory(bdlbb::BlobBufferFactory* value);
    DataStoreConfig& setScheduler(bdlmt::EventScheduler* value);
    DataStoreConfig& setPreallocate(bool value);
    DataStoreConfig& setPrefaultPages(bool value);
    DataStoreConfig& setLocation(const bslstl::StringRef& value);
    DataStoreConfig& setArchiveLocation(const bslstl::StringRef& value);
    DataStoreConfig& setClusterName(const bslstl::StringRef& value);
    DataStoreConfig& setNodeId(int value);
    DataStoreConfig& setPartitionId(int value);
    DataStoreConfig& setMaxDataFileSize(bsls::Types::Uint64 value);
    DataStoreConfig& setMaxJournalFileSize(bsls::Types::Uint64 value);
    DataStoreConfig& setMaxQlistFileSize(bsls::Types::Uint64 value);
    DataStoreConfig& setQueueCreationCb(const QueueCreationCb& value);
    DataStoreConfig& setQueueDeletionCb(const QueueDeletionCb& value);
    DataStoreConfig& setRecoveredQueuesCb(const RecoveredQueuesCb& value);

    /// Set the corresponding member to the specified `value` and return a
    /// reference offering modifiable access to this object.
    DataStoreConfig& setMaxArchivedFileSets(int value);

    // ACCESSORS
    bdlbb::BlobBufferFactory* bufferFactory() const;
    bdlmt::EventScheduler*    scheduler() const;
    bool                      hasPreallocate() const;
    bool                      hasPrefaultPages() const;
    const bslstl::StringRef&  location() const;
    const bslstl::StringRef&  archiveLocation() const;
    const bslstl::StringRef&  clusterName() const;
    int                       nodeId() const;
    int                       partitionId() const;
    bsls::Types::Uint64       maxDataFileSize() const;
    bsls::Types::Uint64       maxJournalFileSize() const;
    bsls::Types::Uint64       maxQlistFileSize() const;
    const QueueCreationCb&    queueCreationCb() const;
    const QueueDeletionCb&    queueDeletionCb() const;
    const RecoveredQueuesCb&  recoveredQueuesCb() const;

    /// Return the value of the corresponding member.
    int maxArchivedFileSets() const;

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

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const DataStoreConfig& value);

// ===========================
// class DataStoreRecordHandle
// ===========================

/// VST representing an opaque handle to an `mqbs::DataStoreRecord`.
class DataStoreRecordHandle {
    // FRIENDS
    friend bool operator==(const DataStoreRecordHandle&,
                           const DataStoreRecordHandle&);
    friend bool operator!=(const DataStoreRecordHandle&,
                           const DataStoreRecordHandle&);

  private:
    // PRIVATE TYPES
    typedef DataStoreConfig::RecordIterator RecordIterator;

  private:
    // DATA
    DataStoreConfig::Records::const_iterator d_iterator;

  public:
    // CREATORS

    /// Create an invalid handle. `isValid` returns false.
    DataStoreRecordHandle();
    explicit DataStoreRecordHandle(
        const DataStoreConfig::Records::const_iterator& iterator);

    // MANIPULATORS

    /// Mark the referenced record as having received quorum receipt.  The
    /// behavior is undefined unless `isValid()`.
    void setHasReceipt();

    // ACCESSORS

    /// Return true if this instance is valid, false otherwise.
    bool isValid() const;

    /// Return the type of record which is represented by this handle.
    /// Behavior is undefined unless `isValid` returns true;
    RecordType::Enum type() const;

    /// Return `true` is the record is eventually consistent or it has
    /// replication factor Receipts.
    bool hasReceipt() const;

    /// Return the hi-res timepoint of the record.
    bsls::Types::Int64 timepoint() const;

    /// Return the timestamp of the record.
    bsls::Types::Uint64 timestamp() const;

    /// Return the Primary LeaseId which created the record.
    unsigned int primaryLeaseId() const;

    bsls::Types::Uint64 sequenceNum() const;
};

// FREE OPERATORS
bool operator==(const DataStoreRecordHandle& lhs,
                const DataStoreRecordHandle& rhs);

bool operator!=(const DataStoreRecordHandle& lhs,
                const DataStoreRecordHandle& rhs);

// ===========================
// struct RecoveryRecordInfo
// ===========================

/// Lightweight metadata for a journal record, collected during FileStore
/// recovery and used as the Raft log index entry type.
struct RecoveryRecordInfo {
    bsls::Types::Uint64 d_primaryLeaseId;
    bsls::Types::Uint64 d_journalOffset;
    bsls::Types::Uint64 d_dataOffset;

    /// Type of the journal record this entry describes.  The Raft rollover
    /// orchestration uses this to route each log entry: normal records are
    /// copied via `FileStore::writeRolledOverRecord`, whereas `e_JOURNAL_OP`
    /// entries (sync points) are handled separately.
    RecordType::Enum d_recordType;

    /// Sync-point sub-type of the journal record this entry describes.  Only
    /// meaningful when `d_recordType == e_JOURNAL_OP`; `e_UNDEFINED`
    /// otherwise.  The Raft apply hook uses this to detect a committed
    /// `e_ROLLOVER` and trigger the file rollover.
    SyncPointType::Enum d_syncPointType;

    DataStoreRecordHandle d_handle;

    RecoveryRecordInfo();

    RecoveryRecordInfo(
        bsls::Types::Uint64          primaryLeaseId,
        bsls::Types::Uint64          journalOffset,
        bsls::Types::Uint64          dataOffset,
        RecordType::Enum             recordType,
        const DataStoreRecordHandle& handle = DataStoreRecordHandle(),
        SyncPointType::Enum syncPointType   = SyncPointType::e_UNDEFINED);
};

// ======================
// class StoragesMonitor
// ======================

class StoragesMonitor {
  public:
    // TYPES
    typedef bsl::shared_ptr<ReplicatedStorage> StorageSp;
    typedef mqbi::Storage::AppInfos            Apps;

  public:
    // CREATORS
    virtual ~StoragesMonitor();

    // MANIPULATORS
    virtual void
    onStorageRegistered(int                                       partitionId,
                        const bmqt::Uri&                          uri,
                        const StorageSp&                          storageSp,
                        const DataStoreConfigQueueInfo::AppInfos& apps) = 0;

    virtual void onStorageRegistered(int              partitionId,
                                     const bmqt::Uri& uri,
                                     const StorageSp& storageSp,
                                     const mqbi::Storage::AppInfos& apps) = 0;

    virtual void onStorageUnregistered(int              partitionId,
                                       const bmqt::Uri& uri) = 0;

    virtual void onStoragesCleared(int partitionId) = 0;

    virtual void onRecovered(int partitionId) = 0;

    // ACCESSORS

    virtual StorageSp find(const bmqt::Uri& queueUri) = 0;

    virtual void loadAllStorages(bsl::vector<StorageSp>* result,
                                 int                     partitionId) = 0;

    /// Return true if the queue having the specified `uri` and assigned to
    /// the specified `partitionId` has no messages, false in any other case.
    virtual bool isStorageEmpty(const bmqt::Uri& uri,
                                int              partitionId) const = 0;

    /// Return true if this monitor drives a Raft-replicated partition (i.e.
    /// the owning `FileStore` is on the Raft write path), false for the legacy
    /// path.  Used to assert that legacy-only `FileStore` methods are never
    /// invoked on a Raft partition.
    virtual bool isRaft() const = 0;
};

// =================
// class RecordStore
// =================

/// Narrow interface used by 'FileBackedStorage' to write and read records
/// for a single partition.  Both the legacy 'FileStore' (via 'DataStore')
/// and 'PartitionRaft' implement this interface.
class RecordStore {
  public:
    // TYPES
    typedef mqbi::Storage::AppInfos AppInfos;

    /// A filtered list of storages and the filter predicates applied to
    /// produce it (structurally identical to the `mqbs::StorageCollectionUtil`
    /// types; spelled out here to avoid an include cycle via
    /// `mqbs_replicatedstorage.h`).
    typedef bsl::vector<const ReplicatedStorage*>         StorageList;
    typedef bsl::function<bool(const ReplicatedStorage*)> StorageFilter;
    typedef bsl::vector<StorageFilter>                    StorageFilters;

  public:
    // CREATORS
    virtual ~RecordStore();

    // MANIPULATORS

    /// Write the specified `appData` and `options` belonging to specified
    /// `queueKey` and having specified `guid` and `attributes` to the data
    /// store, and update the specified `handle` with an identifier which
    /// can be used to retrieve the message.  Return zero on success,
    /// non-zero value otherwise.
    virtual int writeMessageRecord(mqbi::StorageMessageAttributes* attributes,
                                   DataStoreRecordHandle*          handle,
                                   const bmqt::MessageGUID&        guid,
                                   const bsl::shared_ptr<bdlbb::Blob>& appData,
                                   const bsl::shared_ptr<bdlbb::Blob>& options,
                                   const mqbu::StorageKey& queueKey) = 0;

    /// Write a CONFIRM record to the data store with the specified
    /// `queueKey`, optional `appKey`, `guid`, `timestamp` and `reason`.
    /// Return zero on success, non-zero value otherwise.
    virtual int writeConfirmRecord(DataStoreRecordHandle*   handle,
                                   const bmqt::MessageGUID& guid,
                                   const mqbu::StorageKey&  queueKey,
                                   const mqbu::StorageKey&  appKey,
                                   bsls::Types::Uint64      timestamp,
                                   ConfirmReason::Enum      reason) = 0;

    /// Write a DELETION record to the data store with the specified
    /// `queueKey`, `flag`, `guid` and `timestamp`.  Return zero on success,
    /// non-zero value otherwise.
    virtual int writeDeletionRecord(const bmqt::MessageGUID& guid,
                                    const mqbu::StorageKey&  queueKey,
                                    DeletionRecordFlag::Enum deletionFlag,
                                    bsls::Types::Uint64      timestamp) = 0;

    /// Write a QUEUE_OP creation record for the specified `queueUri` with
    /// the specified `queueKey`, `appIdKeyPairs` and `timestamp`.  Set
    /// `isNewQueue` to true for a brand-new queue, false for an app-ID
    /// addition.  Return zero on success, non-zero otherwise.
    virtual int writeQueueCreationRecord(DataStoreRecordHandle*  handle,
                                         const bmqt::Uri&        queueUri,
                                         const mqbu::StorageKey& queueKey,
                                         const AppInfos&         appIdKeyPairs,
                                         bsls::Types::Uint64     timestamp,
                                         bool isNewQueue) = 0;

    /// Register the specified `storage` with this record store so that
    /// rollover can copy its outstanding records.
    virtual void registerStorage(ReplicatedStorage* storage) = 0;

    virtual void unregisterStorage(const ReplicatedStorage* storage) = 0;

    /// Create and load into the specified `storageSp` an instance of
    /// ReplicatedStorage for the queue having the specified `queueUri`
    /// and `queueKey` and belonging to the specified `domain`.
    virtual void createStorage(bsl::shared_ptr<ReplicatedStorage>* storageSp,
                               const bmqt::Uri&                    queueUri,
                               const mqbu::StorageKey&             queueKey,
                               mqbi::Domain*                       domain) = 0;

    virtual int writeQueuePurgeRecord(DataStoreRecordHandle*       handle,
                                      const mqbu::StorageKey&      queueKey,
                                      const mqbu::StorageKey&      appKey,
                                      bsls::Types::Uint64          timestamp,
                                      const DataStoreRecordHandle& start) = 0;

    virtual int writeQueueDeletionRecord(DataStoreRecordHandle*  handle,
                                         const mqbu::StorageKey& queueKey,
                                         const mqbu::StorageKey& appKey,
                                         bsls::Types::Uint64 timestamp) = 0;

    /// Remove the record identified by the specified `handle`.  Behavior is
    /// undefined unless `handle` is valid and represents a record in the
    /// data store.
    virtual void removeRecordRaw(const DataStoreRecordHandle& handle) = 0;

    /// Execute the specified `functor` on the dispatcher thread associated
    /// with this record store's partition.  Used to hop onto the partition
    /// thread for operations such as rollover.  Legacy `FileStore` dispatches
    /// via its own `DispatcherClient`; `PartitionRaft` delegates to its
    /// `FileStore`.
    virtual void execute(const mqbi::Dispatcher::VoidFunction& functor) = 0;

    /// Roll over the partition's files: start a new file set carrying the
    /// outstanding records and archive the current one.  Return zero on
    /// success, non-zero otherwise.  Legacy `FileStore` performs the rollover
    /// directly; `PartitionRaft` drives it through Raft (propose `e_ROLLOVER`
    /// then orchestrate).  Used by the admin `rollover` command so it routes
    /// to the correct mechanism per mode.  The behavior is undefined unless
    /// called on this record store's dispatcher thread (see `execute`).
    virtual int rollover() = 0;

    /// Enable or disable writing to this partition per the specified
    /// `enable`.  Used by the admin partition enable/disable command.
    virtual void setAvailabilityStatus(bool enable) = 0;

    /// Set the strong-consistency replication factor (quorum) to the
    /// specified `factor`.  Used by the admin replication QUORUM tunable.
    virtual void setReplicationFactor(int factor) = 0;

    /// Attempt to rollover the journal if needed after a purge has cleared
    /// outstanding records.
    virtual void onPurgeComplete() = 0;

    /// Flush any buffered replication messages to the peers.  Behaviour is
    /// undefined unless this cluster node is the primary for this partition.
    virtual void flushStorage() = 0;

    /// Set the last strong consistency point (primary lease id and sequence
    /// number) for this partition. Used during recovery.
    virtual void setLastStrongConsistency(unsigned int        primaryLeaseId,
                                          bsls::Types::Uint64 sequenceNum) = 0;

    // ACCESSORS

    virtual void loadMessageRaw(bsl::shared_ptr<bdlbb::Blob>*   appData,
                                bsl::shared_ptr<bdlbb::Blob>*   options,
                                mqbi::StorageMessageAttributes* attributes,
                                const DataStoreRecordHandle& handle) const = 0;

    virtual void
    loadMessageAttributesRaw(mqbi::StorageMessageAttributes* buffer,
                             const DataStoreRecordHandle&    handle) const = 0;

    virtual void
    loadQueueOpRecordRaw(QueueOpRecord*               buffer,
                         const DataStoreRecordHandle& handle) const = 0;

    virtual unsigned int
    getMessageLenRaw(const DataStoreRecordHandle& handle) const = 0;

    /// Return the current primary leaseId for this partition.
    virtual unsigned int primaryLeaseId() const = 0;

    /// Return `true` if there was Replication Receipt for the specified
    /// `handle`.
    virtual bool hasReceipt(const DataStoreRecordHandle& handle) const = 0;

    /// Return the partition id associated with this record store.
    virtual int partitionId() const = 0;

    /// Return `true` if this node is the leader/primary for this partition.
    virtual bool isLeader() const = 0;

    /// Load a summary of this partition into the specified `summary`.  Used by
    /// the admin partition/cluster summary command.
    virtual void loadSummary(mqbcmd::FileStore* summary) const = 0;

    /// Load into the specified `storages` the list of storages of this
    /// partition matching every predicate in the specified `filters`.  Used
    /// by the admin domain queue-status command.
    virtual void getStorages(StorageList*          storages,
                             const StorageFilters& filters) const = 0;

    virtual StoragesMonitor* storagesMonitor() = 0;

    /// Return the records container for this partition.
    virtual const DataStoreConfig::Records& records() const = 0;

    /// Return the total number of records in this partition.
    virtual bsls::Types::Uint64 numRecords() const = 0;

    /// Load message record data for the specified iterator.
    virtual void loadMessageRecord(
        MessageRecord*                                  buffer,
        const DataStoreConfig::Records::const_iterator& it) const = 0;

    /// Load confirm record data for the specified iterator.
    virtual void loadConfirmRecord(
        ConfirmRecord*                                  buffer,
        const DataStoreConfig::Records::const_iterator& it) const = 0;

    /// Load queue op record data for the specified iterator.
    virtual void loadQueueOpRecord(
        QueueOpRecord*                                  buffer,
        const DataStoreConfig::Records::const_iterator& it) const = 0;

    /// Convert a Records::iterator to a DataStoreRecordHandle.
    virtual void recordIteratorToHandle(
        DataStoreRecordHandle*                          handle,
        const DataStoreConfig::Records::const_iterator& it) const = 0;

    /// Return a printable description of the client (e.g., for logging).
    /// The returned view is valid for the lifetime of this object and must
    /// be copied before any deferred use.
    virtual bsl::string_view description() const = 0;

    virtual bool isFileSetAvailable() const = 0;
};

// ===============
// class DataStore
// ===============

/// This component provides an interface for a BlazingMQ data store.
class DataStore : public RecordStore, public mqbi::DispatcherClient {
  public:
    // TYPES
    typedef mqbi::Storage::AppInfos AppInfos;

    typedef DataStoreConfig::QueueKeyInfoMap QueueKeyInfoMap;

  public:
    // CREATORS
    virtual ~DataStore() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Open this instance using the optionally specified `queueKeyInfoMap`.
    /// Return zero on success, non-zero value otherwise.
    virtual int open(QueueKeyInfoMap* queueKeyInfoMap) = 0;

    /// Close this instance.  If the optional `flush` flag is true, flush
    /// the data store to the backup storage (e.g., disk) if applicable.
    /// If the optional `archive` flag is true, archive the data store.  Return
    /// zero on success, non-zero value otherwise.
    virtual int close(bool flush = false, bool archive = false) = 0;

    /// Create and load into the specified `storageSp` an instance of
    /// Queue List related
    /// -------------

    /// Journal related
    /// ---------------

    virtual int writeSyncPointRecord(const bmqp_ctrlmsg::SyncPoint& syncPoint,
                                     SyncPointType::Enum            type,
                                     unsigned int        primaryLeaseId,
                                     bsls::Types::Uint64 sequenceNumber) = 0;

    /// Process the specified storage event `blob` containing one or more
    /// storage messages.  The behavior is undefined unless each message in
    /// the event belongs to this partition, and has same primary and
    /// primary leaseId as expected by this data store instance.
    virtual void processStorageEvent(const bsl::shared_ptr<bdlbb::Blob>& blob,
                                     bool                 isPartitionSyncEvent,
                                     mqbnet::ClusterNode* source) = 0;

    /// Process the specified recovery event `blob` containing one or more
    /// storage messages.  Return zero on success, non-zero value otherwise.
    /// The behavior is undefined unless each message in the event belongs
    /// to this partition.
    virtual int
    processRecoveryEvent(const bsl::shared_ptr<bdlbb::Blob>& blob) = 0;

    /// Process Receipt for the specified `primaryLeaseId` and
    /// `sequenceNum`.  The behavior is undefined unless the event belongs
    /// to this partition and unless the `primaryLeaseId` and `sequenceNum`
    /// match a record of the `StorageMessageType::e_DATA` type.
    virtual void processReceiptEvent(unsigned int         primaryLeaseId,
                                     bsls::Types::Uint64  sequenceNum,
                                     mqbnet::ClusterNode* source) = 0;

    /// Replication related
    /// -------------------

    /// Set the specified `primaryNode` with the specified `primaryLeaseId`
    /// as the active primary for this data store partition.  Note that
    /// `primaryNode` could refer to the node which owns this data store.  If
    /// the specified `isRaft` is true, only the primary-identity bookkeeping
    /// is performed; the legacy sync-point machinery (recurring sync-point/
    /// highwatermark timers, the "issue a sync point on behalf of the previous
    /// primary" step, the immediate sync point, and the replica implicit
    /// receipt) is skipped because Raft drives all of that through its own
    /// log.
    virtual void setActivePrimary(mqbnet::ClusterNode* primaryNode,
                                  unsigned int         primaryLeaseId,
                                  bool                 isRaft = false) = 0;

    /// Clear the current primary associated with this partition.
    virtual void clearPrimary() = 0;

    // ACCESSORS

    /// Return true if this instance is open, false otherwise.
    virtual bool isOpen() const = 0;

    /// Return configuration associated with this instance.
    virtual const DataStoreConfig& config() const = 0;

    /// Return the replication factor associated with this data store.
    virtual unsigned int clusterSize() const = 0;

    virtual void
    loadMessageRecordRaw(MessageRecord*               buffer,
                         const DataStoreRecordHandle& handle) const = 0;

    virtual void
    loadConfirmRecordRaw(ConfirmRecord*               buffer,
                         const DataStoreRecordHandle& handle) const = 0;

    virtual void
    loadDeletionRecordRaw(DeletionRecord*              buffer,
                          const DataStoreRecordHandle& handle) const = 0;

};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------------
// struct DataStoreRecord
// ----------------------

inline DataStoreRecord::DataStoreRecord()
: d_recordType(RecordType::e_UNDEFINED)
, d_hasReceipt(true)
, d_recordOffset(0)
, d_messageOffset(0)
, d_appDataUnpaddedLen(0)
, d_dataOrQlistRecordPaddedLen(0)
, d_messagePropertiesInfo()
, d_arrivalTimepoint(0LL)
, d_arrivalTimestamp(0LL)
{
    // NOTHING
}

inline DataStoreRecord::DataStoreRecord(RecordType::Enum    recordType,
                                        bsls::Types::Uint64 recordOffset)
: d_recordType(recordType)
, d_hasReceipt(true)
, d_recordOffset(recordOffset)
, d_messageOffset(0)
, d_appDataUnpaddedLen(0)
, d_dataOrQlistRecordPaddedLen(0)
, d_messagePropertiesInfo()
, d_arrivalTimepoint(0LL)
, d_arrivalTimestamp(0LL)
{
    // NOTHING
}

inline DataStoreRecord::DataStoreRecord(
    RecordType::Enum    recordType,
    bsls::Types::Uint64 recordOffset,
    unsigned int        dataOrQlistRecordPaddedLen)
: d_recordType(recordType)
, d_hasReceipt(true)
, d_recordOffset(recordOffset)
, d_messageOffset(0)
, d_appDataUnpaddedLen(0)
, d_dataOrQlistRecordPaddedLen(dataOrQlistRecordPaddedLen)
, d_messagePropertiesInfo()
, d_arrivalTimepoint(0LL)
, d_arrivalTimestamp(0LL)
{
    // NOTHING
}

inline DataStoreRecord::DataStoreRecord(RecordType::Enum recordType)
: d_recordType(recordType)
, d_hasReceipt(false)
, d_recordOffset(0)
, d_messageOffset(0)
, d_appDataUnpaddedLen(0)
, d_dataOrQlistRecordPaddedLen(0)
, d_messagePropertiesInfo()
, d_arrivalTimepoint(0LL)
, d_arrivalTimestamp(0LL)
{
    // NOTHING
}

// -------------------------
// struct DataStoreRecordKey
// -------------------------

// CREATORS
inline DataStoreRecordKey::DataStoreRecordKey()
: d_sequenceNum(0)
, d_primaryLeaseId(0)
{
    // NOTHING
}

inline DataStoreRecordKey::DataStoreRecordKey(
    const bsls::Types::Uint64 sequenceNum,
    unsigned int              primaryLeaseId)
: d_sequenceNum(sequenceNum)
, d_primaryLeaseId(primaryLeaseId)
{
    // NOTHING
}

// FREE FUNCTIONS
template <class HASH_ALGORITHM>
void hashAppend(HASH_ALGORITHM& hashAlgo, const mqbs::DataStoreRecordKey& key)
{
    using bslh::hashAppend;  // for ADL
    hashAppend(hashAlgo, key.d_sequenceNum);
    hashAppend(hashAlgo, key.d_primaryLeaseId);
}

// --------------------------------
// class DataStoreRecordKeyHashAlgo
// --------------------------------

// ACCESSORS
template <class TYPE>
inline DataStoreRecordKeyHashAlgo::result_type
DataStoreRecordKeyHashAlgo::operator()(const TYPE& type) const
{
    return type.d_sequenceNum +
           (static_cast<bsls::Types::Uint64>(type.d_primaryLeaseId) << 32);
}

// -----------------------------
// struct DataStoreRecordKeyLess
// -----------------------------

inline bool
DataStoreRecordKeyLess::operator()(const DataStoreRecordKey& lhs,
                                   const DataStoreRecordKey& rhs) const
{
    // Compare PrimaryLeaseId followed by SequenceNum
    if (lhs.d_primaryLeaseId != rhs.d_primaryLeaseId) {
        return lhs.d_primaryLeaseId < rhs.d_primaryLeaseId;  // RETURN
    }

    if (lhs.d_sequenceNum != rhs.d_sequenceNum) {
        return lhs.d_sequenceNum < rhs.d_sequenceNum;  // RETURN
    }

    return false;
}

// ------------------------------
// class DataStoreConfigQueueInfo
// ------------------------------

// CREATORS
inline DataStoreConfigQueueInfo::DataStoreConfigQueueInfo(
    bslma::Allocator* basicAllocator)
: d_canonicalUri(basicAllocator)
, d_partitionId(mqbi::Storage::k_INVALID_PARTITION_ID)
, d_appIdKeyPairs(basicAllocator)
, d_ghosts(basicAllocator)
, d_purgeOps(basicAllocator)
, d_isRecorded(false)
{
}

inline DataStoreConfigQueueInfo::DataStoreConfigQueueInfo(
    const DataStoreConfigQueueInfo& other,
    bslma::Allocator*               basicAllocator)
: d_canonicalUri(other.d_canonicalUri, basicAllocator)
, d_partitionId(other.d_partitionId)
, d_appIdKeyPairs(other.d_appIdKeyPairs, basicAllocator)
, d_ghosts(other.d_ghosts, basicAllocator)
, d_purgeOps(other.d_purgeOps, basicAllocator)
, d_isRecorded(other.d_isRecorded)
{
}

// MANIPULATORS
inline void
DataStoreConfigQueueInfo::setCanonicalQueueUri(const bsl::string& value)
{
    d_canonicalUri = value;
}

inline void DataStoreConfigQueueInfo::setPartitionId(int value)
{
    d_partitionId = value;
}

inline void DataStoreConfigQueueInfo::setAsRecorded()
{
    d_isRecorded = true;
}

// ACCESSORS
inline const bsl::string& DataStoreConfigQueueInfo::canonicalQueueUri() const
{
    return d_canonicalUri;
}

inline int DataStoreConfigQueueInfo::partitionId() const
{
    return d_partitionId;
}

inline const DataStoreConfigQueueInfo::AppInfos&
DataStoreConfigQueueInfo::appIdKeyPairs() const
{
    return d_appIdKeyPairs;
}

inline bool DataStoreConfigQueueInfo::isRecorded() const
{
    return d_isRecorded;
}

// ---------------------
// class DataStoreConfig
// ---------------------

// MANIPULATORS
inline DataStoreConfig&
DataStoreConfig::setBufferFactory(bdlbb::BlobBufferFactory* value)
{
    d_bufferFactory_p = value;
    return *this;
}

inline DataStoreConfig&
DataStoreConfig::setScheduler(bdlmt::EventScheduler* value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(value->clockType() == bsls::SystemClockType::e_MONOTONIC);

    d_scheduler_p = value;
    return *this;
}

inline DataStoreConfig& DataStoreConfig::setPreallocate(bool value)
{
    d_preallocate = value;
    return *this;
}

inline DataStoreConfig& DataStoreConfig::setPrefaultPages(bool value)
{
    d_prefaultPages = value;
    return *this;
}

inline DataStoreConfig&
DataStoreConfig::setLocation(const bslstl::StringRef& value)
{
    d_location = value;
    return *this;
}

inline DataStoreConfig&
DataStoreConfig::setArchiveLocation(const bslstl::StringRef& value)
{
    d_archiveLocation = value;
    return *this;
}

inline DataStoreConfig&
DataStoreConfig::setClusterName(const bslstl::StringRef& value)
{
    d_clusterName = value;
    return *this;
}

inline DataStoreConfig& DataStoreConfig::setNodeId(int value)
{
    d_nodeId = value;
    return *this;
}

inline DataStoreConfig& DataStoreConfig::setPartitionId(int value)
{
    d_partitionId = value;
    return *this;
}

inline DataStoreConfig&
DataStoreConfig::setMaxDataFileSize(bsls::Types::Uint64 value)
{
    d_maxDataFileSize = value;
    return *this;
}

inline DataStoreConfig&
DataStoreConfig::setMaxJournalFileSize(bsls::Types::Uint64 value)
{
    d_maxJournalFileSize = value;
    return *this;
}

inline DataStoreConfig&
DataStoreConfig::setMaxQlistFileSize(bsls::Types::Uint64 value)
{
    d_maxQlistFileSize = value;
    return *this;
}

inline DataStoreConfig&
DataStoreConfig::setQueueCreationCb(const QueueCreationCb& value)
{
    d_queueCreationCb = value;
    return *this;
}

inline DataStoreConfig&
DataStoreConfig::setQueueDeletionCb(const QueueDeletionCb& value)
{
    d_queueDeletionCb = value;
    return *this;
}

inline DataStoreConfig&
DataStoreConfig::setRecoveredQueuesCb(const RecoveredQueuesCb& value)
{
    d_recoveredQueuesCb = value;
    return *this;
}

inline DataStoreConfig& DataStoreConfig::setMaxArchivedFileSets(int value)
{
    d_maxArchivedFileSets = value;
    return *this;
}

// ACCESSORS
inline bdlbb::BlobBufferFactory* DataStoreConfig::bufferFactory() const
{
    return d_bufferFactory_p;
}

inline bdlmt::EventScheduler* DataStoreConfig::scheduler() const
{
    return d_scheduler_p;
}

inline bool DataStoreConfig::hasPreallocate() const
{
    return d_preallocate;
}

inline bool DataStoreConfig::hasPrefaultPages() const
{
    return d_prefaultPages;
}

inline const bslstl::StringRef& DataStoreConfig::location() const
{
    return d_location;
}

inline const bslstl::StringRef& DataStoreConfig::archiveLocation() const
{
    return d_archiveLocation;
}

inline const bslstl::StringRef& DataStoreConfig::clusterName() const
{
    return d_clusterName;
}

inline int DataStoreConfig::nodeId() const
{
    return d_nodeId;
}

inline int DataStoreConfig::partitionId() const
{
    return d_partitionId;
}

inline bsls::Types::Uint64 DataStoreConfig::maxDataFileSize() const
{
    return d_maxDataFileSize;
}

inline bsls::Types::Uint64 DataStoreConfig::maxJournalFileSize() const
{
    return d_maxJournalFileSize;
}

inline bsls::Types::Uint64 DataStoreConfig::maxQlistFileSize() const
{
    return d_maxQlistFileSize;
}

inline const DataStoreConfig::QueueCreationCb&
DataStoreConfig::queueCreationCb() const
{
    return d_queueCreationCb;
}

inline const DataStoreConfig::QueueDeletionCb&
DataStoreConfig::queueDeletionCb() const
{
    return d_queueDeletionCb;
}

inline const DataStoreConfig::RecoveredQueuesCb&
DataStoreConfig::recoveredQueuesCb() const
{
    return d_recoveredQueuesCb;
}

inline int DataStoreConfig::maxArchivedFileSets() const
{
    return d_maxArchivedFileSets;
}

// ---------------------------
// class DataStoreRecordHandle
// ---------------------------

// PRIVATE CREATORS
inline DataStoreRecordHandle::DataStoreRecordHandle(
    const DataStoreConfig::Records::const_iterator& iterator)
: d_iterator(iterator)
{
}

// CREATORS
inline DataStoreRecordHandle::DataStoreRecordHandle()
: d_iterator()
{
}

// MANIPULATORS

inline void DataStoreRecordHandle::setHasReceipt()
{
    BSLS_ASSERT_SAFE(isValid());
    d_iterator->second.d_hasReceipt = true;
}

// ACCESSORS
inline bool DataStoreRecordHandle::isValid() const
{
    return d_iterator != RecordIterator();
}

inline RecordType::Enum DataStoreRecordHandle::type() const
{
    BSLS_ASSERT_SAFE(isValid());
    return d_iterator->second.d_recordType;
}

inline bool DataStoreRecordHandle::hasReceipt() const
{
    BSLS_ASSERT_SAFE(isValid());
    return d_iterator->second.d_hasReceipt;
}

inline bsls::Types::Int64 DataStoreRecordHandle::timepoint() const
{
    BSLS_ASSERT_SAFE(isValid());
    return d_iterator->second.d_arrivalTimepoint;
}

inline bsls::Types::Uint64 DataStoreRecordHandle::timestamp() const
{
    BSLS_ASSERT_SAFE(isValid());
    return d_iterator->second.d_arrivalTimestamp;
}

inline unsigned int DataStoreRecordHandle::primaryLeaseId() const
{
    BSLS_ASSERT_SAFE(isValid());
    return d_iterator->first.d_primaryLeaseId;
}

inline bsls::Types::Uint64 DataStoreRecordHandle::sequenceNum() const
{
    BSLS_ASSERT_SAFE(isValid());
    return d_iterator->first.d_sequenceNum;
}

// =======================
// struct DataStoreRecord
// =======================

inline RecordType::Enum DataStoreRecord::type() const
{
    return d_recordType;
}

}  // close package namespace

// -------------------------
// struct DataStoreRecordKey
// -------------------------

// FREE OPERATORS
inline bsl::ostream& mqbs::operator<<(bsl::ostream&                stream,
                                      const mqbs::DataStoreRecord& value)
{
    stream << "DataStoreRecord[type=" << value.d_recordType
           << " offset=" << value.d_recordOffset
           << " hasReceipt=" << bsl::boolalpha << value.d_hasReceipt
           << " msgOffset=" << value.d_messageOffset
           << " appDataLen=" << value.d_appDataUnpaddedLen
           << " padLen=" << value.d_dataOrQlistRecordPaddedLen
           << " timepoint=" << value.d_arrivalTimepoint
           << " timestamp=" << value.d_arrivalTimestamp << "]";
    return stream;
}

inline bsl::ostream& mqbs::operator<<(bsl::ostream&                   stream,
                                      const mqbs::DataStoreRecordKey& value)
{
    return value.print(stream, 0, -1);
}

inline bool mqbs::operator==(const mqbs::DataStoreRecordKey& lhs,
                             const mqbs::DataStoreRecordKey& rhs)
{
    return lhs.d_primaryLeaseId == rhs.d_primaryLeaseId &&
           lhs.d_sequenceNum == rhs.d_sequenceNum;
}

inline bool mqbs::operator!=(const mqbs::DataStoreRecordKey& lhs,
                             const mqbs::DataStoreRecordKey& rhs)
{
    return !(lhs == rhs);
}

inline bool mqbs::operator<(const mqbs::DataStoreRecordKey& lhs,
                            const mqbs::DataStoreRecordKey& rhs)
{
    DataStoreRecordKeyLess less;
    return less(lhs, rhs);
}

// ---------------------------
// class DataStoreRecordHandle
// ---------------------------

// FREE OPERATORS
inline bool mqbs::operator==(const mqbs::DataStoreRecordHandle& lhs,
                             const mqbs::DataStoreRecordHandle& rhs)
{
    return lhs.d_iterator == rhs.d_iterator;
}

inline bool mqbs::operator!=(const mqbs::DataStoreRecordHandle& lhs,
                             const mqbs::DataStoreRecordHandle& rhs)
{
    return !(lhs == rhs);
}

// ---------------------
// class DataStoreConfig
// ---------------------

// FREE OPERATORS
inline bsl::ostream& mqbs::operator<<(bsl::ostream&                stream,
                                      const mqbs::DataStoreConfig& value)
{
    return value.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
